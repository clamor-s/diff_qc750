/* Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved. */
#include <termios.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/queue.h>
#include <assert.h>
#include <time.h>
#include <errno.h>

#include <linux/ioctl.h>
#include <linux/android_alarm.h>

#define STD_READ_TIMEOUT_MS         2000

/* unsollicited responde code timeout uncertainty */
#define UNSOLLICITED_TIMEOUT_UNCERTAINTY 0.4 /* 40% */

enum {
    MODE_MODEM,
    MODE_LOADER
};


static sem_t _read_sem;
static sem_t _lock;
static sem_t _read_write_atomicity_sem;
static sem_t _pending_writes_sem;

#define INIT_LOCK(a)    sem_init(&(a), 0, 1);
#define FREE_LOCK(a)    sem_destroy(&(a));
#define LOCK(a)         sem_wait(&(a))
#define UNLOCK(a)       sem_post(&(a));

static int pending_writes_count = 0;

#define AT_RSP_OK "\r\nOK\r\n"
#define AT_RSP_ERR "\r\nERROR\r\n"

char *deviceName = NULL;
FILE *logFileH = NULL;

#define TTY_LOG(format, ...) {\
                       char outstr[250];\
                       time_t t; \
                       struct tm *tmp;\
                       struct timespec ts;\
                       t = time(NULL); \
                       tmp = localtime(&t);\
                       clock_gettime(CLOCK_REALTIME,&ts);\
                       strftime(outstr, sizeof(outstr), "%d%b%y %T", tmp);\
                       printf("[%s:%ld.%ld] " format, outstr, ts.tv_sec, ts.tv_nsec, ## __VA_ARGS__); \
                       if (logFileH) fprintf(logFileH, "[%s] " format, outstr, ## __VA_ARGS__); \
                   }

/***********************************************************************************
 * Helper functions
 **********************************************************************************/
static int write_str(int fd, const char *str)
{
    return (int) write(fd, str, strlen(str));
}

static int read_timeout(int fd, char *str, int maxlen, int timeoutMs)
{
    fd_set set;
    struct timeval t;
    int res;

    /* _read_write_atomicity_sem is used to be sure that we won't be scheduled
     * for too long while entering the "select" function.
     * If we don't use that semaphore then in some cases select returns 0. Here is what happens:
     *    - We enter the select function with a valid timeout (2 seconds for example)
     *    - The os schedules the write thread for a long time that will exceed the timeout
     *    - When back to the reader thread, the select function exits with res=0 meaning timeout expired
     */
    LOCK(_read_write_atomicity_sem);
    FD_ZERO(&set);
    FD_SET(fd, &set);

    t.tv_usec = timeoutMs * 1000;
    t.tv_sec = t.tv_usec / 1000000;
    t.tv_usec = t.tv_usec % 1000000;

    res = select(fd+1, &set, NULL, NULL, &t);
    UNLOCK(_read_write_atomicity_sem);

    if (res == 1 && FD_ISSET(fd, &set))
    {
        res = read(fd, str, maxlen);
        return res < 0 ? 0 : res;
    }
    else
    {
        TTY_LOG("%s: select returned %d (errno=%d:%s)\n", __FUNCTION__, res, errno, strerror(errno) )
        return 0;
    }
}

static int read_min(int fd, char *str, int minlen, int maxlen, int timeoutMs, int maxPendingWrites)
{
    int count = 0;
    char *ptr = str;
    int total_read = 0;

    if (minlen < 0 || minlen > maxlen)
        return 0;

    do
    {
        count = read_timeout(fd, ptr, maxlen - total_read, timeoutMs);
        if (count > 0)
        {
            total_read += count;
            ptr += count;
        }
        else
            break;
    }
    while (total_read < minlen);
    pending_writes_count--;
    if ((maxPendingWrites != 0)&&((pending_writes_count+1) == maxPendingWrites))
    {
        UNLOCK(_pending_writes_sem);
    }

    return total_read;
}

static void read_flush(int fd)
{
    int count = 0;
    char buf[128];

    /* An ioctl to flush the buffer would be good but in-practice this would not
       always work due to flow control backing up data on the target to send */
    for ( ; ; )
    {
        count = read_timeout(fd, buf, sizeof(buf), 500);
        if (count == 0)
            break; /* Timeout */
        TTY_LOG("%s: Flushed %d bytes\n", __FUNCTION__, count);
    }
}

static void measure_start(struct timeval *start)
{
    gettimeofday(start, NULL);
}

static long measure_stop(struct timeval *start)
{
    struct timeval end;
    long tv_sec, tv_usec;

    gettimeofday(&end, NULL);

    tv_sec = end.tv_sec - start->tv_sec;
    tv_usec = end.tv_usec - start->tv_usec;
    tv_sec *= 1000000;

    return (tv_sec + tv_usec) / 1000;
}

static void wait_for(int minPauseMs, int maxPauseMs)
{
    int randomTimeMs;

    /* Fixed repeat delay time */
    if (minPauseMs >= maxPauseMs)
    {
        randomTimeMs = minPauseMs;
    }
    /* Random repeat delay time between boundaries */
    else
    {
        randomTimeMs = (rand() % (maxPauseMs - minPauseMs)) + minPauseMs;
    }

    /* now sleep */

    /* are we sleeping for less than 1000ms? */
    if (randomTimeMs < 1000)
    {
        if (randomTimeMs)
        {
            usleep(randomTimeMs * 1000 );
        }
    }
    else
    {
        /* sleep for short amounts of time and poll system time to find
           out if our wait time is over. we need to do this in order
           to support sleeping in suspend (when the system clock stops
           ticking) */

        struct timespec start_time;

        /* round to nearest second */
        int randomTimeSeconds = randomTimeMs/1000;

        clock_gettime(CLOCK_REALTIME,&start_time);

        do
        {
            struct timespec time_now;
            /* sleep for 750ms */
            usleep(750000);

            /* get new time */
            clock_gettime(CLOCK_REALTIME,&time_now);

            //TTY_LOG("time_now.v_sec = %ld, start_time=%ld randomTime=%d\n", time_now.tv_sec, start_time.tv_sec,randomTimeSeconds );

            if (time_now.tv_sec >= start_time.tv_sec + randomTimeSeconds )
            {
                break;
            }
        } while (1);
    }
}

/***********************************************************************************
 * request_loopback - Send AT%LOOPBACK request to modem.
 **********************************************************************************/
static int request_loopback(int fd, int timeout)
{
    const char *str = "AT%LOOPBACK=1,4\r\n";
    const char *resp = "AT%LOOPBACK=1,4";
    char strIn[50];
    int res;

    TTY_LOG("Requesting loopback mode %s", str);
    write_str(fd, str);

    TTY_LOG("Waiting for response\n");
    res = read_min(fd, strIn, 0, sizeof(strIn), timeout, 0);

    if (res == 0 || strncmp(strIn, resp, strlen(resp)   ) == 0)
        return 0;
    else
        return -1;
}

static int send_at_cmd_and_wait_response(int fd, int timeout, const char *cmd, char *rsp, int len, int verbose, int maxPendingWrites)
{
    int index = 0, nbread = 0, status = 0;

    /* Send cmd... */
    TTY_LOG("Sendind %s", cmd);
    write_str(fd, cmd);

    /* Wait for OK, ERROR or timeout... */
    if(!verbose)
    {
        TTY_LOG("Waiting for response\n");
    }
    /* Get response. ended on "\r\nOK\r\n" or "\r\nERROR\r\n" or timeout */
    index = 0;
    memset(rsp, 0 , len);
    do
    {
        if(len-index<=1)
        {
            TTY_LOG("No more space in tty receive buffer\n");
            return -1;
        }

        nbread = read_timeout(fd, rsp+index, len-index-1, timeout);
        if (nbread > 0)
        {
            /* answer data */
            rsp[index + nbread] = '\0';
            if(verbose)
            {
                TTY_LOG("%s", &rsp[index]);
            }
            index += nbread;
        }
        else
        {
            /* timeout */
            TTY_LOG("AT cmd error: timeout\n");
            return -1;
        }
    }
    while ((strstr(rsp, AT_RSP_OK) == NULL) && (strstr(rsp, AT_RSP_ERR) == NULL));

    return (strstr(rsp, AT_RSP_OK) != NULL) ? 0 : -1;
}

/***********************************************************************************
 * request_idrvstat - Send AT%IDRVSTAT request to modem.
 **********************************************************************************/
static int request_idrvstat(int fd, int timeout, char *stat, int len, int maxPendingWrites)
{
    return send_at_cmd_and_wait_response(fd, timeout, "AT%IDRVSTAT\r\n", stat, len, 1, maxPendingWrites);
}

/***********************************************************************************
 * activate_hibernate - Send AT%IPMDEBUG=4,5 request to modem.
 **********************************************************************************/
static int activate_hibernate(int fd, int timeout, int maxPendingWrites)
{
    int res;
    char buf[2048];
    char cmd[128];

    return send_at_cmd_and_wait_response(fd, timeout, "AT%IPMDEBUG=4,5\r\n", buf, 2048, 1, maxPendingWrites);

    return -1;
}

/***********************************************************************************
 * select_cfun_mode - Send AT+CFUN=<cfunMode> request to modem.
 **********************************************************************************/
static int select_cfun_mode(int fd, int timeout, int cfunMode, int maxPendingWrites)
{
    char buf[2048];
    char cfun_mode[12]; /* AT+CFUN=x\r\n*/

    if((cfunMode == 0) || (cfunMode == 1) || (cfunMode == 4))
    {
        sprintf(cfun_mode, "AT+CFUN=%d\r\n", cfunMode);

        if (cfunMode ==1)
        {
            send_at_cmd_and_wait_response(fd, timeout, "AT%NWSTATE=1\r\n", buf, 2048, 1, maxPendingWrites);
        }
    }
    else
    {
        TTY_LOG("CFUN error: invalid mode %d\n", cfunMode);
        return -1;
    }

    return send_at_cmd_and_wait_response(fd, timeout, cfun_mode, buf, 2048, 1, maxPendingWrites);
}

/***********************************************************************************
 * activate_target_upload - Send AT%IDUMMYU=<targetUploadFreq>,1 request to modem.
 **********************************************************************************/
static int activate_target_upload(int fd, int timeout, int targetUploadFreq, int maxPendingWrites)
{
    char buf[2048];
    char cmd[128];

    /* 1st make sure dummy upload is stopped... */
    int ret = send_at_cmd_and_wait_response(fd, timeout, "AT%IDUMMYU=1,0\r\n", buf, 2048, 1, maxPendingWrites);
    if(ret != -1)
    {
        /* Then start periodical dummy upload... */
        sprintf(cmd, "AT%%IDUMMYU=%d,1,1\r\n", targetUploadFreq);
        return send_at_cmd_and_wait_response(fd, timeout, cmd, buf, 2048, 1, maxPendingWrites);
    }

    return -1;
}

/***********************************************************************************
 * test_sequential - Send data and expect echoed copy to be returned.
 **********************************************************************************/
static int test_sequential(int fd, int minSize, int maxSize, int repeat, int minPauseMs, int maxPauseMs, int timeout, int verbose)
{
    int res = 0;
    int i;
    int length;
    char *inBuff;
    char *outBuff;
    struct timeval start;
    int size;
    long timeTaken;
    int l;

    outBuff = (char*)malloc(maxSize);
    if (!outBuff)
    {
        TTY_LOG("Failed to allocate buffer space\n");
        return -1;
    }

    inBuff = (char*)malloc(maxSize);
    if (!inBuff)
    {
        free(outBuff);
        TTY_LOG("Failed to allocate buffer space\n");
        return -1;
    }

    for (i=0; (i < repeat) || (repeat == -1); i++)
    {
        /* Use range of sizes? */
        if (minSize != maxSize)
            size = (rand() % (maxSize - minSize)) + minSize;
        /* Fixed size */
        else
            size = maxSize;

        /* Fill with different random pattern (ASCII) */
        for (l=0;l<size;l++)
            outBuff[l] = (rand() % 74) + 48; // 0...z

        if (verbose) TTY_LOG("%d. Sending test string (length %d)\n", i+1, size);

        measure_start(&start);

        /* Send to target */
        write(fd, outBuff, size);

        /* Wait for same size of data to be received */
        length = read_min(fd, inBuff, size, size, timeout, 0);

        timeTaken = measure_stop(&start);

        if (verbose) TTY_LOG("%d. Received test string (length %d) (%dms)\n", i+1, length, (int)timeTaken);

        if (length != size || memcmp(inBuff, outBuff, length) != 0)
        {
            res = -1;
            break;
        }

        /* Sleep for a bit (if required) */
        wait_for(minPauseMs, maxPauseMs);
    }

    free(outBuff);
    free(inBuff);

    return res;
}

/***********************************************************************************
 * Sent Items Queue
 **********************************************************************************/


/**
 * List item structure
 */
struct listentry
{
    char *ptr;
    int len;

    TAILQ_ENTRY(listentry) entries;
};

/**
 * Linked list declaration
 */
TAILQ_HEAD(listhead, listentry) _head;

/**
 * Init sent items list
 */
static void list_rq_init(void)
{
    INIT_LOCK(_lock);
    sem_init(&_read_sem, 0, 0);
    TAILQ_INIT(&_head);
}

/**
 * Add tx data to sent list
 */
static void list_rq_add(char *tx_buf, int size, int maxPendingWrites)
{
    struct listentry *entry = malloc(sizeof(struct listentry));
    assert(entry != NULL);

    entry->ptr = tx_buf;
    entry->len = size;

    /* Add to end of sent list */
    LOCK(_lock);
    TAILQ_INSERT_TAIL(&_head, entry, entries);
    UNLOCK(_lock);

    /* Notify possibly waiting read thread */
    sem_post(&_read_sem);

    if (pending_writes_count >= maxPendingWrites)
    {
        LOCK(_pending_writes_sem);
    }
}

/**
 * Get item from sent list (waits until one present)
 */
static int list_rq_get(char **dataSent)
{
    struct listentry *entry = NULL;
    int retval = 0;

    /* Wait until the queue contains a sent item... */
    int res = sem_wait(&_read_sem);

    /* Interrupted? */
    if (res)
        return -1;

    /* Remove head of list */
    LOCK(_lock);
    entry = TAILQ_FIRST(&_head);
    if (entry)
    {
        TAILQ_REMOVE(&_head, entry, entries);
    }
    UNLOCK(_lock);

    if (entry)
    {
        *dataSent = entry->ptr;
        retval = entry->len;

        /* Now free list entry allocated in list_rq_add() */
        free(entry);

        return retval;
    }
    else
        return -1;
}

/**
 * Cancel pending reads
 */
static void list_rq_cancel(void)
{
    /* Notify possibly waiting read thread */
    sem_post(&_read_sem);
}

/**
 * Destroy sent items list
 */
static void list_rq_destroy(void)
{
    struct listentry *entry;

    LOCK(_lock);
    while ((entry = TAILQ_FIRST(&_head)) != NULL)
    {
        /* Remove from list */
        TAILQ_REMOVE(&_head, entry, entries);

        /* Free allocated space */
        free(entry->ptr);
        free(entry);
    }
    UNLOCK(_lock);

    sem_destroy(&_read_sem);
    FREE_LOCK(_lock);
}

/***********************************************************************************
 * test_parallel - Send data in one thread and verify in another.
 **********************************************************************************/

struct read_thread_args
{
    int             file;
    int             maxSize;
    int             repeat;
    int             timeout;
    volatile int    error;
    int             maxPendingWrites;
    int             accumulatePendingWrites;
};

/**
 * Receive thread for test_parallel
 */
static void* test_read_thread(void *arg)
{
    struct read_thread_args *args = (struct read_thread_args*)arg;
    unsigned int count = 0;
    int error = 0;
    int fd = args->file;
    int reads_required = args->repeat;
    int forever = (reads_required == -1);
    int timeout = args->timeout;
    int length, read_len;
    int maxPendingWrites = args->maxPendingWrites;
    int accumulatePendingWrites = args->accumulatePendingWrites;
    char *rx_buf;
    char *dataSent = NULL;
    int accumulate = 1;

    rx_buf = (char*)malloc(args->maxSize);
    if (!rx_buf)
    {
        /* Return result via arg struct */
        TTY_LOG("read_thread:Could not allocate mem!\n");
        args->error = 1;
        return NULL;
    }

    /* Only stop thread when read has caught up with last write */
    for ( ; ; )
    {
        if (accumulatePendingWrites)
        {
            if (accumulate)  /* Accumutate the maximum of pending writes */
            {
                while (pending_writes_count < maxPendingWrites)
                {
                    wait_for(10, 10); /* wait a little */
                }
                accumulate = 0; /* Once the maximum accumulation is reached, we deaccumulate */
            }
            if (pending_writes_count == 1) /* Then, when no writes are accumulated, we restart the accumulation */
            {
               accumulate = 1;
            }
        }

        /* Pop next transaction from sent list */
        read_len = list_rq_get(&dataSent);
        if (read_len == -1)
        {
            TTY_LOG("%d. read_thread:interrupted!\n", count);
            error = 1;
            break;
        }

        /* Wait for header size of data to be received */
        length = read_min(fd, rx_buf, read_len, read_len, timeout, maxPendingWrites);
        if (length == 0)
        {
            TTY_LOG("%d. read_thread:timeout!\n", count);
            free(dataSent);
            error = 1;
            break;
        }

        /* Verify payload receive length */
        if (read_len != length)
        {
            TTY_LOG("%d. read_thread:receive length mismatch!\n", count);
            free(dataSent);
            error = 1;
            break;
        }

        /* Compare received data */
        if (memcmp(dataSent, rx_buf, length) != 0)
        {
            TTY_LOG("%d. read_thread:receive payload mismatch!\n", count);
            free(dataSent);
            error = 1;
            break;
        }

        free(dataSent);

        /* Last read completed? */
        if (!forever && ++count == (unsigned int)reads_required)
        {
            break;
        }
    }

    /* Return result via arg struct */
    args->error = error;

    free(rx_buf);

    return NULL;
}

/**
 * Write function to test_parallel
 */
static int test_parallel(int fd, int minSize, int maxSize, int repeat, int minPauseMs, int maxPauseMs, int timeout, int verbose, int maxPendingWrites, int accumulatePendingWrites)
{
    int i;
    struct timeval lastTime;
    long lastTimeCounter = 0;
    int bandwidthCurrent = 0;
    int bandwidthMax = 0;
    int bandwidthMin = 0;
    int bandwidthFirst = 1;
    int enable_bandwidth_mon = 0;
    struct read_thread_args readargs;
    pthread_t readthread;
    int size;
    int l;
    char *tx_buf = NULL;
    unsigned int total_sent = 0;

    /* Bandwidth measurement only has a point if we are not inserting delays between writes! */
    if (minPauseMs == maxPauseMs && maxPauseMs == 0)
        enable_bandwidth_mon = 1;
    else
        enable_bandwidth_mon = 0;

    /* Setup read thread args */
    readargs.file = fd;
    readargs.repeat = repeat;
    readargs.timeout = (maxPauseMs < timeout) ? timeout : maxPauseMs + 1000;
    readargs.error = 0;
    readargs.maxSize = maxSize;
    readargs.maxPendingWrites = maxPendingWrites;
    readargs.accumulatePendingWrites = accumulatePendingWrites;

    /* Create sent list (before start read thread!) */
    list_rq_init();

    /* Start read thread - pass thread arg from our stack so we must
       use pthread_join before exiting! */
    if (pthread_create(&readthread, NULL, test_read_thread, &readargs) != 0)
    {
        TTY_LOG("Failed to start receive thread\n");
        return -1;
    }

    lastTimeCounter = 0;
    measure_start(&lastTime);

    pending_writes_count = 0;

    for (i=0; (i < repeat) || (repeat == -1); i++)
    {
        /* Use range of sizes? */
        if (minSize != maxSize)
            size = (rand() % (maxSize - minSize)) + minSize;
        /* Fixed size */
        else
            size = maxSize;

        /* Allocate transmit space */
        tx_buf = (char*)malloc(size);
        if (!tx_buf)
        {
            TTY_LOG("Failed to allocate mem\n");
            break;
        }

        /* Fill with random pattern (ASCII) */
        for (l=0;l<size;l++)
            tx_buf[l] = (rand() % 74) + 48; // 0...z

        if (verbose) TTY_LOG("%d. Sending test sequence\n", i);

        /* Send to target */
        LOCK(_read_write_atomicity_sem);
        write(fd, tx_buf, size);
        pending_writes_count++;
        UNLOCK(_read_write_atomicity_sem);
        total_sent+= size;

        /* Add to sent list */
        list_rq_add(tx_buf, size, maxPendingWrites);

        /* Sleep for a bit (if required) */
        wait_for(minPauseMs, maxPauseMs);

        /* Bandwidth measurement only has a point if we are not inserting delays between writes! */
        if (enable_bandwidth_mon)
        {
            long timeTaken = measure_stop(&lastTime);

            /* One second has passed, record bandwidth */
            if (timeTaken >= 1000)
            {
                bandwidthCurrent = total_sent;

                /* Record highs and lows */
                if (bandwidthCurrent > bandwidthMax)
                    bandwidthMax = bandwidthCurrent;
                if (bandwidthCurrent < bandwidthMin || bandwidthFirst)
                    bandwidthMin = bandwidthCurrent;

                TTY_LOG("Current speed %dKB/s - min %dKB/s max %dKB/s\n", bandwidthCurrent / 1024, bandwidthMin / 1024, bandwidthMax / 1024);

                measure_start(&lastTime);
                total_sent = 0;
                bandwidthFirst = 0;
            }
        }

        /* Has the read thread already exited with an error? */
        if (readargs.error)
        {
            break;
        }
    }

    /* Unblock read thread (if stuck) */
    list_rq_cancel();

    /* Wait for the read thread to terminate */
    TTY_LOG("Waiting for reads to complete\n");
    pthread_join(readthread, NULL);

    /* Now that read thread has exited, destroy any remaining sent list */
    list_rq_destroy();

    if (!readargs.error)
    {
        if (enable_bandwidth_mon)
            TTY_LOG("Test complete - min %dKB/s max %dKB/s\n", bandwidthMin / 1024, bandwidthMax / 1024);
        return 0;
    }
    else
        return -1;
}

/***********************************************************************************
 * test_early_wakeup
 **********************************************************************************/

/**
 * test_early_wakeup:
 *  - activate hibernation if requested in command line:
 *    AT%IPMDEBUG=4,5
 *  - select required CFUN mode for hibernate test
 *  - "randomly" ask for IPM stats to check hibernation status
 *    and force random wake up from host.
 *  - optionally set a periodic RX from target with
 *    AT%IDUMMYU...
 */
static int test_early_wakeup(int fd,
                             int cfunMode,
                             int maxSleepTime,
                             int targetUpload,
                             int targetUploadFreq,
                             int minSize,
                             int maxSize,
                             int repeat,
                             int minPauseMs,
                             int maxPauseMs,
                             int timeout,
                             int verbose,
                             int maxPendingWrites,
                             int enableHibernate)
{
    char stat[2048];
    int res, count = 0;
    int reads_required = repeat;
    int forever = (reads_required == -1);

    if (enableHibernate)
    {
        res = activate_hibernate(fd, timeout, 1);
        if(res < 0)
        {
            TTY_LOG("%s: failed to activate hibernate\n", __FUNCTION__);
            return 1;
        }
    }

    res = select_cfun_mode(fd, 20000, cfunMode, maxPendingWrites);
    if(res < 0)
    {
        TTY_LOG("%s: failed to set CFUN mode %d\n", __FUNCTION__, cfunMode);
        return 1;
    }

    if(targetUpload)
    {
        res = activate_target_upload(fd, timeout, targetUploadFreq, maxPendingWrites);
        if(res < 0)
        {
            TTY_LOG("%s: failed to activate target upload\n", __FUNCTION__);
            return 1;
        }
    }

    int num;
    char *ptr, *line;
    char *power_off_str = "'n_power_off':";

    for(;;)
    {
        /* Request IPM stat */
        count++;
        TTY_LOG("Iteration # %d:\n", count);
        res = request_idrvstat(fd, timeout, stat, 2048, maxPendingWrites);
        if(res < 0)
        {
            break;
        }
        if(!forever && (count == reads_required))
        {
            break;
        }

        line = strtok(stat, "\n");
        while(line)
        {
            ptr = strstr(line, power_off_str);
            if(ptr)
            {
                sscanf(ptr + strlen(power_off_str), "%d\n", &num);
                TTY_LOG("Num of power off reported on DXP0: %d\n", num);
                break;
            }
            line = strtok(NULL, "\n");
        }

        if (logFileH)
        {
            fflush(logFileH);
        }

        /* Sleep for a bit (if required) */
        wait_for(minPauseMs, maxPauseMs);
    }

    if(targetUpload)
    {
        res = send_at_cmd_and_wait_response(fd, timeout, "AT%IDUMMYU=1,0\r\n", stat, 2048, 1, maxPendingWrites);
    }

    return res < 0 ? 1:0;
}


/***********************************************************************************
 * test_remote_wakeup
 **********************************************************************************/

/* Example : "%IDUMMYU: 421,0x0000000dd681bda2" will return 421 */
static unsigned int get_idummyu_number(const char * str)
{
   const char * p;
   const char * c;
   unsigned int res  = 0;
   unsigned int mult = 1;
   unsigned int i = 0;

   if (str == NULL)
      return 0;
   p = strstr(str,"IDUMMYU");
   if (p == NULL)
      return 0;
   c = strstr(p,",");
   if (c == NULL)
      return 0;
   c--; // Skip the ','
   while ((*(c-i) >= '0') && (*(c-i) <= '9') && (i < 7))
   {
      res += ((*(c-i)) - '0')*mult;
      mult*=10;
      i++;
   }
   return res;
}

/**
 * test_remote_wakeup:
 *  - activate hibernation if requested in command line:
 *    AT%IPMDEBUG=4,5
 *  - select required CFUN mode for hibernate test
 *  - set a periodic RX from target with AT%IDUMMYU...
 */
static int test_remote_wakeup(int fd,
                              int cfunMode,
                              int targetUploadFreq,
                              int repeat,
                              int verbose,
                              int maxPendingWrites,
                              int enableHibernate)
{
    char inbuf[2048];
    const char * p;
    char atbuf[100];
    int res, count = 0, idummyu_counter = 0;
    int forever = (repeat == -1);
    int timeout = (int)(targetUploadFreq * (1.0 + UNSOLLICITED_TIMEOUT_UNCERTAINTY) );
    int read_count;

    if (enableHibernate)
    {
        res = activate_hibernate(fd, timeout, maxPendingWrites);
        if(res < 0)
        {
            TTY_LOG("%s: failed to activate hibernate\n", __FUNCTION__);
            return 1;
        }
    }

    res = select_cfun_mode(fd, 20000, cfunMode, maxPendingWrites);
    if(res < 0)
    {
        TTY_LOG("%s: failed to set CFUN mode %d\n", __FUNCTION__,cfunMode);
        return 1;
    }

    res = activate_target_upload(fd, timeout, targetUploadFreq, maxPendingWrites);
    if(res < 0)
    {
        TTY_LOG("%s: failed to activate target upload\n", __FUNCTION__);
        return 1;
    }

    /* Flush tty layer buffer */
    read_flush(fd);

    for(;;)
    {
        if(!forever && (count == repeat))
        {
            break;
        }

        /* wait for unsollicited response code */
        memset(inbuf, 0, sizeof(inbuf));
        read_count = read_min(fd, inbuf, 1, sizeof(inbuf), timeout, maxPendingWrites);

        if (read_count == 0)
        {
            TTY_LOG("%s: timed out after %dms while waiting for unsollicited response code!\n", __FUNCTION__, timeout);
            break;
        }

		p = strstr(inbuf,"IDUMMYU");
		if (p == NULL)
		{
			TTY_LOG("%s: skipping this read!\n", __FUNCTION__);
		}
        while (p != NULL)
        {
			/* We got a IDUMMYU */
			int current_idummyu_counter;
			current_idummyu_counter = get_idummyu_number(p);

			// The first time take the idummyu number as reference
			if (count == 0)
			{
				idummyu_counter = current_idummyu_counter;
			}
			else
			{
				if (current_idummyu_counter != (idummyu_counter+1))
				{
					// The idummyu counter is wrong, ask for a coredump
					TTY_LOG("Expected idummy counter %d, got %d. Crashing the modem for coredump\n", idummyu_counter+1, current_idummyu_counter);
					write_str(fd, "AT%idbgtest=1\r\n");
					return 1;
				}
				else
				{
					idummyu_counter++;
				}
			}

			// Now send an AT command and wait for the OK response. Timeout is 1 second
			memset(atbuf, 0, sizeof(atbuf));
			res = send_at_cmd_and_wait_response(fd, 1000, "AT\r\n", atbuf, sizeof(atbuf), 1, maxPendingWrites);

			if (res != 0)
			{
				TTY_LOG("AT command after idummyu has not responded. Crashing the modem for coredump\n");
				write_str(fd, "AT%idbgtest=1\r\n");
				return 1;
			}

			count++;

			if (forever)
			{
				TTY_LOG("Iteration # %d (idummy %d):\n", count, current_idummyu_counter);
			}
			else
			{
				TTY_LOG("Iteration # %d/%d (idummy %d):\n", count, repeat, current_idummyu_counter);
			}

			p = strstr(p+6,"IDUMMYU"); /* Looking for the next idummyu in this string */
		}


        TTY_LOG("\n%s", inbuf);

        if (logFileH)
        {
            fflush(logFileH);
        }
    }

    /* stop sending unsollicited codes */
    res = send_at_cmd_and_wait_response(fd, timeout, "AT%IDUMMYU=1,0\r\n", inbuf, 2048, 1, maxPendingWrites);

    return res < 0 ? 1:0;
}

static int test_remote_wakeup_increase_delay(int fd,
                                       int cfunMode,
                                       int unsollicitedPeriod,
                                       int maxUnsollicitedPeriod,
                                       int unsollicitedPeriodIncrement,
                                       int timeout,
                                       int verbose,
                                       int maxPendingWrites,
                                       int enableHibernate)
{
    char inbuf[2048];
    int res=0, count = 0, idummyu_counter = 0;
    int read_count;
    int period = unsollicitedPeriod;
    char buf[2048];
    char cmd[128];
    const char * p;

    if (enableHibernate)
    {
        res = activate_hibernate(fd, timeout, maxPendingWrites);
        if(res < 0)
        {
            TTY_LOG("%s: failed to activate hibernate\n", __FUNCTION__);
            return 1;
        }
    }

    res = select_cfun_mode(fd, timeout, cfunMode, maxPendingWrites);
    if(res < 0)
    {
        TTY_LOG("%s: failed to set CFUN mode %d\n", __FUNCTION__,cfunMode);
        return 1;
    }

    /* 1st make sure dummy upload is stopped... */
    res = send_at_cmd_and_wait_response(fd, timeout, "AT%IDUMMYU=1,0\r\n", buf, 2048, 1, maxPendingWrites);
    if(res < 0)
    {
        TTY_LOG("%s: failed to stop target upload\n", __FUNCTION__);
        return 1;
    }

    sprintf(cmd, "AT%%IDUMMYU=%d,1,1\r\n", period);
    res = send_at_cmd_and_wait_response(fd, timeout, cmd, buf, 2048, 1, maxPendingWrites);
    if(res < 0)
    {
        TTY_LOG("%s: failed to activate target upload\n", __FUNCTION__);
        return 1;
    }

    for(;;)
    {
        if(period >= maxUnsollicitedPeriod)
        {
            break;
        }

        /* wait for unsollicited response code */
        memset(inbuf, 0, sizeof(inbuf));
        read_count = read_min(fd, inbuf, 1, sizeof(inbuf), 10000, maxPendingWrites);

        if (read_count == 0)
        {
            TTY_LOG("%s: timed out after %dms while waiting for unsollicited response code!\n", __FUNCTION__, timeout);
            res = 1;
            break;
        }


        p = strstr(inbuf,"IDUMMYU");
        if (p == NULL)
        {
            TTY_LOG("%s: skipping this read!\n", __FUNCTION__);
        }
        while (p != NULL)
        {
            /* We got a IDUMMYU */
            int current_idummyu_counter;
            current_idummyu_counter = get_idummyu_number(p);
            inbuf[read_count]=0;
            TTY_LOG("\n%s", inbuf);

            // The first time take the idummyu number as reference
            if (count == 0)
            {
                idummyu_counter = current_idummyu_counter;
            }
            else
            {
                if (current_idummyu_counter != (idummyu_counter+1))
                {
                    // The idummyu counter is wrong, ask for a coredump
                    TTY_LOG("Expected idummy counter %d, got %d. Crashing the modem for coredump\n", idummyu_counter+1, current_idummyu_counter);
                    write_str(fd, "AT%idbgtest=1\r\n");
                    return 1;
                }
                else
                {
                    idummyu_counter++;
                }
            }

            count++;
            p = strstr(p+6,"IDUMMYU"); /* Looking for the next idummyu in this string */

            if(p==NULL)
            {
                period += unsollicitedPeriodIncrement;

                 /* Then start periodical dummy upload... */
                 sprintf(cmd, "AT%%IDUMMYU=%d,1,1\r\n", period);
                 send_at_cmd_and_wait_response(fd, timeout, cmd, buf, 2048, 1, maxPendingWrites);

            }

        }

        if (logFileH)
        {
            fflush(logFileH);
        }
    }

    /* stop sending unsollicited codes */
    if(send_at_cmd_and_wait_response(fd, timeout, "AT%IDUMMYU=1,0\r\n", inbuf, 2048, 1, maxPendingWrites) <0)
    {
        res = 1;
    }

    return res;
}

/***********************************************************************************
 * test_reboot_reset - Test recovery afetr mode switcging and or assert/affault
 **********************************************************************************/
static int get_current_mode(int fd, int timeout, int maxPendingWrites)
{
    char buf[2048];
    int mode = -1;
    char *ptr, *line;
    char *mode_str = "MODE: ";

    int res = send_at_cmd_and_wait_response(fd, timeout, "AT%MODE?\r\n", buf, 2048, 1, maxPendingWrites);
    if(res == 0)
    {
        line = strtok(buf, "\n");
        while(line)
        {
            ptr = strstr(line, mode_str);
            if(ptr)
            {
                sscanf(ptr + strlen(mode_str), "%d\n", &mode);
                break;
            }
            line = strtok(NULL, "\n");
        }
    }
    else
    {
        TTY_LOG("AT cmd error: fail to get current mode\n");
    }

    return mode;
}

static int switch_in_mode_and_check(int *fd, int timeout, int maxPendingWrites, int req_mode, int cur_mode, int switchWait)
{
    char cmd[128];
    char buf[2048];
    int res = -1;

    do
    {
        /* Start mode switching */
        sprintf(cmd, "AT%%MODE=%d\r\n", req_mode);
        res = send_at_cmd_and_wait_response(*fd, timeout, cmd, buf, 2048, 1, maxPendingWrites);
        if(res < 0)
        {
            TTY_LOG("AT cmd error.\n");
            break;
        }
        close(*fd);

        /* We've just launch a mode switch: let's wait "a while"... */
        TTY_LOG("Waiting %ds after mode switch...\n", (switchWait/1000));
        wait_for(switchWait, switchWait);

        /* Try to re-open device... */
        *fd = open(deviceName, O_RDWR);
        if(*fd < 0)
        {
            TTY_LOG("Fail to open %s after switch from mode %d to mode %d\n", deviceName, cur_mode, req_mode);
            break;
        }

        cur_mode = get_current_mode(*fd, timeout, maxPendingWrites);
        if(cur_mode < 0)
        {
            break;
        }
        if(cur_mode != req_mode)
        {
            TTY_LOG("Mode switch failure: get mode %d instead of %d\n", cur_mode, req_mode);
            break;
        }

        res = 0;

    } while(0);

    return res;
}

static int create_target_fault(int *fd, int timeout, int maxPendingWrites, int dbg_test_idx, int crashWait)
{
    char cmd[128];
    char buf[2048];
    int res = -1, cur_mode;
    struct timeval start;
    long timeTaken;

    /* Remove previous crash infos... */
    res = send_at_cmd_and_wait_response(*fd, timeout, "AT%DEBUG=99\r\n", buf, 2048, 1, maxPendingWrites);
    if(res < 0)
    {
        TTY_LOG("AT cmd error.\n");
        return res;
    }

    /* Create target fault */
    sprintf(cmd, "AT%%IDBGTEST=%d\r\n", dbg_test_idx);
    /* Send cmd...: this cmd will never answer... */
    TTY_LOG("Sendind %s", cmd);
    write_str(*fd, cmd);
    close(*fd);
    /* We've just caused a crash: let's wait "a while" before continuing... */
    wait_for(3000, 3000);

    measure_start(&start);
    do
    {
        *fd = open(deviceName, O_RDWR);
        if(*fd >= 0)
        {
            cur_mode = get_current_mode(*fd, timeout, maxPendingWrites);
            if(cur_mode == MODE_MODEM)
            {
                /* wait a while before next loop to ensure we do not cause afault too soon
                   and avoid AT%IDBGTEST to be seen as systematic asserts and goes in loader.... */
                wait_for(10000, 10000);
                res = 0;
                break;
            }
        }

        timeTaken = measure_stop(&start);
        if(timeTaken > crashWait)
        {
            TTY_LOG("Crash recovery failure.\n");
            break;
        }
        else
        {
            wait_for(10000, 10000); /* 10s... */
            if(*fd >= 0)
            {
                close(*fd);
            }
        }

    } while(1);

    return res;
}

/**
 * Reboot/Reset test:
 *  - run AT%MODE & AT%IDBGTEST sequence in order to chek that
 *    modem and host are able to stay synced during any modem
 *    reset...
 */
static int test_reboot_reset(int fd,
                             int repeat,
                             int timeout,
                             int verbose,
                             int modeSwitch,
                             int crashTest,
                             int switchWait,
                             int crashWait,
                             int maxPendingWrites)
{
    int res = 0, count = 0;
    int reads_required = repeat;
    int forever = (reads_required == -1);
    int current_mode, dbg_test_idx;

    for(;;)
    {
        if(!forever && (count == reads_required))
        {
            break;
        }
        count++;
        TTY_LOG("Iteration # %d:\n", count);

        if(modeSwitch)
        {
            /* Get current mode */
            current_mode = get_current_mode(fd, timeout, maxPendingWrites);
            if(current_mode != MODE_LOADER)
            {
                /* Switch in loader mode and check it is OK... */
                res = switch_in_mode_and_check(&fd, timeout, maxPendingWrites, MODE_LOADER, current_mode, switchWait);
                if(res < 0)
                {
                    TTY_LOG("Could not switch in mode %d from mode %d\n", MODE_LOADER, current_mode);
                    break;
                }
            }

            /* Switch in modem mode and check it is OK... */
            res = switch_in_mode_and_check(&fd, timeout, maxPendingWrites, MODE_MODEM, MODE_LOADER, switchWait);
            if(res  < 0)
            {
                TTY_LOG("Could not switch in mode %d from mode %d\n", MODE_MODEM, MODE_LOADER);
                break;
            }
        }

        if(crashTest)
        {
            /* Uses AT%IDBGTEST: 17 indexes for different cause of crash
                6 is not index for a crash: skipped
                16 skipped for the moment as will trigger 3 consecutive asserts, a reboot in loader mode, etc... TODO
            */
            dbg_test_idx = (count % 17);
            if((dbg_test_idx == 6) || (dbg_test_idx == 16))
            {
                dbg_test_idx--;
            }
            res = create_target_fault(&fd, timeout, maxPendingWrites, dbg_test_idx, crashWait);
            if(res  < 0)
            {
                TTY_LOG("IDBGTEST with index %d failure.\n", dbg_test_idx);
                break;
            }
        }
    }

    return res < 0 ? 1:0;
}

/***********************************************************************************
 * Main
 **********************************************************************************/
static int check_loopback(int fd, int timeout, int noloopbackreq)
{
    int error = 0;

    /* Put the target in AT loop back mode */
    if (!noloopbackreq && request_loopback(fd, timeout))
    {
        TTY_LOG("[%s] Failed: Could not activate loopback mode\n", deviceName);
        error = 1;
    }

    return error;
}


int main(int argc, char *argv[])
{
    int fd;
    int error = 0;
    int minSize = 256;
    int maxSize = 256;
    int repeats = 100;
    int minPauseTime = 0;
    int maxPauseTime = 0;
    int opt;
    int showhelp = 0;
    int noloopbackreq = 0;
    int timeout = STD_READ_TIMEOUT_MS;
    int quietMode = 0;
    int testNumber = 1;
    int maxPendingWrites = 10;
    int cfunMode = 1;
    int enableHibernate = 0;
    int enableUnsollicited = 0;
    int unsollicitedPeriod = 100;
    int maxUnsollicitedPeriod = 1500;
    int unsollicitedPeriodIncrement = 10;
    int maxSleepTime = 25000;
    int modeSwitch = 1, crashTest = 1, switchWait = 15000, crashWait=120000;
    int rnd_seed = 0, rnd_seed_provided=0;
    char *logFileName = NULL;
    struct termios options;
    int termios_arg;

    int long_val;
    int long_opt_index = 0;
    struct option long_options[] = {
        /* long option array, where "name" is case sensitive*/
        {"help", 0, NULL, 'h'},             /* --help or -h will do the same... */
        {"no_smt", 0, &long_val, 'm'},      /* --no_smt  */
        {"no_ct", 0, &long_val, 'n'},       /* --no_ct  */
        {"switch_wait", 1, &long_val, 's'}, /* --switch_wait <value> */
        {"crash_wait", 1, &long_val, 'c'},  /* --crash_wait <value> */
        {"enable_hibernate", 0, &long_val, 'H'},  /* --enable_hibernate */
        {"max_pending_wr",   1, &long_val, 'W'},  /* --max_pending_wr <value> */
        {0, 0, 0, 0}                        /* usual array terminating item */
    };

    /************/

    /* Parse args */
    opterr = 0;
    while ((opt = getopt_long(argc, argv, "ahcqr:s:l:m:n:d:t:z:i:u:w:o:f:g:x:", long_options, &long_opt_index)) != -1)
    {
       switch (opt)
       {
       case 'd':
            deviceName = optarg;
            break;
       case 'o':
            logFileName = optarg;
            break;
       case 'r':
           repeats = atoi(optarg);
           break;
       case 'c':
           repeats = -1;
           break;
       case 'l':
           minSize = atoi(optarg);
           break;
       case 's':
           maxSize = atoi(optarg);
           break;
       case 'm':
           minPauseTime = atoi(optarg);
           break;
       case 'n':
           maxPauseTime = atoi(optarg);
           break;
       case 'a':
           noloopbackreq = 1;
           break;
       case 't':
           timeout = atoi(optarg);
           break;
       case 'z':
           testNumber = atoi(optarg);
           break;
       case 'w':
           maxPendingWrites = atoi(optarg);
           break;
       case 'q':
           quietMode = 1;
           break;
       case 'i':
           cfunMode = atoi(optarg);
           break;
       case 'j':
           maxSleepTime = atoi(optarg);
           break;
       case 'u':
           enableUnsollicited = 1;
           unsollicitedPeriod = atoi(optarg);
           break;
       case 'f':
           maxUnsollicitedPeriod = atoi(optarg);
           break;
       case 'g':
           unsollicitedPeriodIncrement = atoi(optarg);
           break;
       case 'x':
           rnd_seed =  atoi(optarg);
           rnd_seed_provided = 1;
           break;
       case 0: /* a "long" option */
           switch(long_val)
           {
           case 'm': /* --no_smt */
               modeSwitch = 0;
               break;
           case 'n': /* --no_ct */
               crashTest = 0;
               break;
           case 's': /* --switch_wait <value> */
               switchWait = atoi(optarg);
               break;
           case 'c': /* --crash_wait <value> */
               crashWait = atoi(optarg);
               break;
           case 'H': /* --enable_hibernate */
               enableHibernate = 1;
               break;
           case 'W': /* --max_pending_wr <value> */
               maxPendingWrites = atoi(optarg);
               break;
           }
           break;
       case 'h':
       default: /* '?' */
            showhelp = 1;
            break;
       }
    }

    if (maxSize < minSize)
    {
        maxSize = minSize;
    }
    
    if (showhelp || deviceName == NULL)
    {
        printf("Usage:\n   %s -d /dev/ttyS0\n", argv[0]);
        printf("      -d /dev/name    = Device name to use (required)\n");
        printf("      -r xx           = repeat test count (optional, default 100)\n");
        printf("      -c              = continuous test (optional, default off)\n");
        printf("      -l xx           = min size of large transfers (optional, default 256)\n");
        printf("      -s xx           = max size of large transfers (optional, default 256)\n");
        printf("      -m xx           = min pause time (ms) between requests (optional, default 0)\n");
        printf("      -n xx           = max pause time (ms) between requests (optional, default 0)\n");
        printf("      -t xx           = max receive timeout for target response (ms) (optional, default 2000ms)\n");
        printf("      -a              = Skip AT%%LOOPBACK request (optional, default AT%%LOOPBACK request sent)\n");
        printf("      -q              = Quiet mode (reduced logging) (optional, default off)\n");
        printf("      -i x            = Test early wakeup in given CFUN mode (0, 1 or 4)\n");
        printf("                        Incompatible with loopback mode...\n");
        printf("      -j x            = Set max sleep time in ms.\n");
        printf("      -u x            = Unsollicited response code period in ms (AT%%IDUMMYU)\n");
        printf("                       = for test with increasing delay this is the initial period\n");
        printf("      -f xx           = maximum period in ms for increasing delay test (default 1500)\n");
        printf("      -g xx           = period increment in ms for increasing delay test (default 10)\n");
        printf("      -z xx           = Test number to run\n");
        printf("         1            = Sequential mode - send request - wait for response before moving on (optional, default)\n");
        printf("         2            = Parallel mode (small xfer) - send requests without waiting (test large quantity of small xfers)\n");
        printf("         3            = Parallel mode (variable xfer) - send requests without waiting (test flow control / max throughput)\n");
        printf("         4            = Early wakeup test\n");
        printf("         5            = Reset/Reboot test\n");
        printf("         6            = Remote wakeup test\n");
        printf("         7            = Remote wakeup test with increasing delay \n");
        printf("         8            = Parallel mode (variable xfer) - send requests and force pending writes accumulation\n");
        printf("      -w xx           = Maximum number of pending writes (optional, default 10)\n");
        printf("      -x xx           = Seed for random generator (optional, seed generated from local time if not provided)\n");
        printf("      -o file         = Log to specified file\n");
        printf("\n");
        printf("Long options:\n");
        printf("      --no_smt        = No mode switch in reset/reboot test\n");
        printf("      --no_ct         = No crash test in reset/reboot test\n");
        printf("      --switch_wait x = Time to wait after a mode switch before any action\n");
        printf("                        In ms, default is 15000ms\n");
        printf("      --crash_wait x  = Time to take after a crash to try any action on device\n");
        printf("                        In ms, default is 120000ms.\n");
        printf("                        After this time, device is considered as out of order.\n");
        printf("      --enable_hibernate = Enable hibernate (early wakeup/remote wakeup tests)\n");
        printf("      --max_pending_wr x = Max pending writes (default 10) mostly applies to z8\n");
        return 0;
    }

    /* open log file if specified */
    if (logFileName)
    {
        logFileH = fopen(logFileName, "a" );
        if (logFileH != NULL)
        {
            printf("Opened %s for writing\n", logFileName);
        }
        else
        {
            printf("Could not open %s for writing\n", logFileName);
        }
    }

    TTY_LOG( "[%s] Opening device\n", deviceName );

    INIT_LOCK(_read_write_atomicity_sem);
    sem_init(&_pending_writes_sem, 0, 0);

    fd = open(deviceName, O_RDWR);
    if (fd >= 0)
    {
        TTY_LOG("[%s] Opened device\n", deviceName);

        /* configure terminal */
        TTY_LOG("%s: Configuring %s \n", __FUNCTION__, deviceName );

        /* get current options */
        tcgetattr( fd, &options );

        /* switch to RAW mode - disable echo, ... */
        options.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
        options.c_oflag &= ~OPOST;
        options.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
        options.c_cflag &= ~(CSIZE|PARENB);
        options.c_cflag |= CS8 | CLOCAL | CREAD;

        /* apply new settings */
        tcsetattr(fd, TCSANOW, &options);

        /* set DTR */
        termios_arg = TIOCM_DTR;
        ioctl(fd, TIOCMBIS, &termios_arg);

        /* send carriage return to terminate previous command (if any)*/
        write_str(fd, "\r\n");

        /* Flush tty layer buffer */
        read_flush(fd);

        if(rnd_seed_provided )
        {
            srand (rnd_seed);
        }
        else
        {
            srand (time(NULL) );
        }

        /* Run test */
        if (!error)
        {
            switch (testNumber)
            {

            /* Test Mode - Sequential mode */
            case 1:
                if(!check_loopback(fd, timeout, noloopbackreq))
                {
                    if (test_sequential(fd, minSize, maxSize, repeats, minPauseTime, maxPauseTime, timeout, !quietMode))
                    {
                        TTY_LOG("[%s] Failed: Transfer test %d error\n", deviceName, testNumber);
                        error = 1;
                    }
                }
            break;

            /* Test Mode - Small xfers */
            case 2:
                if(!check_loopback(fd, timeout, noloopbackreq))
                {
                    if (test_parallel(fd, 4, 4, repeats, minPauseTime, maxPauseTime, timeout, !quietMode, maxPendingWrites, 0))
                    {
                        TTY_LOG("[%s] Failed: Transfer test %d error\n", deviceName, testNumber);
                        error = 1;
                    }
                }
            break;

            /* Test Mode - Variable xfers */
            case 3:
                if(!check_loopback(fd, timeout, noloopbackreq))
                {
                    if (test_parallel(fd, minSize, maxSize, repeats, minPauseTime, maxPauseTime, timeout, !quietMode, maxPendingWrites, 0))
                    {
                        TTY_LOG("[%s] Failed: Transfer test %d error\n", deviceName, testNumber);
                    }
                }
            break;

            /* Early wakeup test */
            case 4:
                if (test_early_wakeup(fd,
                                      cfunMode,
                                      maxSleepTime,
                                      enableUnsollicited,
                                      unsollicitedPeriod,
                                      minSize,
                                      maxSize,
                                      repeats,
                                      minPauseTime,
                                      maxPauseTime,
                                      timeout,
                                      !quietMode,
                                      maxPendingWrites,
                                      enableHibernate))
                {
                    TTY_LOG("[%s] Failed: Early wakeup test error\n", deviceName);
                    error = 1;
                }
                break;

            /* Reboot/Reset test */
            case 5:
                if(test_reboot_reset(fd,
                                     repeats,
                                     timeout,
                                     !quietMode,
                                     modeSwitch,
                                     crashTest,
                                     switchWait,
                                     crashWait,
                                     maxPendingWrites))
                {
                    TTY_LOG("[%s] Failed: Reboot/Reset test error\n", deviceName);
                    error = 1;
                }
                break;

            /* Remote wakeup test */
            case 6:
                if (test_remote_wakeup(fd,
                                       cfunMode,
                                       unsollicitedPeriod,
                                       repeats,
                                       !quietMode,
                                       maxPendingWrites,
                                       enableHibernate))
                {
                    TTY_LOG("[%s] Failed: Remote wakeup test error\n", deviceName);
                    error = 1;
                }
                break;

            /* Remote wakeup test with increasing delay */
            case 7:
                if (test_remote_wakeup_increase_delay(fd,
                                       cfunMode,
                                       unsollicitedPeriod,
                                       maxUnsollicitedPeriod,
                                       unsollicitedPeriodIncrement,
                                       timeout,
                                       !quietMode,
                                       maxPendingWrites,
                                       enableHibernate))
                {
                    TTY_LOG("[%s] Failed: Remote wakeup test error\n", deviceName);
                    error = 1;
                }
                break;

            /* Test Mode - Pending writes accumulation */
            case 8:
                if(!check_loopback(fd, timeout, noloopbackreq))
                {
                    if (test_parallel(fd, minSize, maxSize, repeats, minPauseTime, maxPauseTime, timeout, !quietMode, maxPendingWrites, 1))
                    {
                        TTY_LOG("[%s] Failed: Transfer test %d error\n", deviceName, testNumber);
                    }
                }
            break;

            default:
                TTY_LOG("Unknown test!\n");
                error = 1;
                break;
            }
        }

        if (!error)
            TTY_LOG("[%s] Test completed: No errors\n", deviceName);

        close(fd);
    }
    else
    {
        TTY_LOG("[%s] Error opening device\n", deviceName);
        error = 1;
    }

    if(logFileH)
    {
        fclose(logFileH);
    }

    FREE_LOCK(_read_write_atomicity_sem);
    FREE_LOCK(_pending_writes_sem);

    return error;
}


