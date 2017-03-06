/*
 * Copyright (c) 2009 - 2010 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include "nvcommon.h"
#include "nvidlcmd.h"
#include "nvos.h"
#include "nvrm_ioctls.h"
#include "nvassert.h"

typedef union {
    struct cmsghdr msg;
    NvU8           data[CMSG_SPACE(sizeof(int))];
} NvDaemonFdMsg;

NvError NvIdlHelperFifoWrite(void *fifo, const void *buffer, size_t len)
{
    int fd = (int)fifo;
    const char *buff = (const char *)buffer;
    size_t cnt = 0;
    ssize_t rc;

    if ((fd<=0) || !buffer)
        return NvError_BadParameter;

    if (!len)
        return NvSuccess;


    do {
        rc = send(fd, buff, len-cnt, 0);
        if (rc>0) {
            cnt += (size_t)rc;
            buff += (size_t)rc;
        }
    } while (cnt<len && (rc>0 || (rc==-1 && errno==EINTR)));

    if (cnt==len)
        return NvSuccess;

    return NvError_FileWriteFailed;

}

NvError NvIdlHelperFifoRead(void *fifo, void *buffer, size_t len, size_t *read)
{
    int fd = (int)fifo;
    char *buff = (char *)buffer;
    size_t cnt = 0;
    ssize_t rc;

    if ((fd <= 0) || !buffer)
        return NvError_BadParameter;

    if (!len) {
        if (read)
            *read = 0;
        return NvSuccess;
    }

    do {
        rc = recv(fd, buff, len-cnt, MSG_WAITALL);
        if (rc>0) {
            buff += (size_t)rc;
            cnt += (size_t)rc;
        }
    } while (cnt<len && (rc>0 || (rc==-1 && errno==EINTR)));

    if (read)
        *read = cnt;

    if (rc<=0)
        return NvError_FileReadFailed;

    return NvSuccess;
}

/* global lists used by FIFO management code */
static NvU32 s_RmClientId = 0;
static NvU32 s_DaemonClientId = 0;

static NvU32 GetFifoTls(void)
{
    static pthread_mutex_t tls_alloc_lock = PTHREAD_MUTEX_INITIALIZER;
    static NvU32 tls_key = NVOS_INVALID_TLS_INDEX;

    if (tls_key != NVOS_INVALID_TLS_INDEX)
        return tls_key;

    pthread_mutex_lock(&tls_alloc_lock);
    if (tls_key == NVOS_INVALID_TLS_INDEX) {
        tls_key = NvOsTlsAlloc();
        if (tls_key == NVOS_INVALID_TLS_INDEX)
            NvOsDebugPrintf("failed to allocate a TLS key for per-thread FIFO");
    }
    pthread_mutex_unlock(&tls_alloc_lock);
    return tls_key;
}

/* communicates with the daemon to initialize a new FIFO pair with a valid
 * connection */
static int daemon_fd = -1;

static NvOsFileHandle GetIoctlFile( void )
{
    static NvOsFileHandle handle = NULL;
    NvError e;

    if (handle) {
        return handle;
    }

    e = NvOsFopen("/dev/nvrm",
                  NVOS_OPEN_READ | NVOS_OPEN_WRITE, &handle);

    if (e != NvSuccess) {
        return NULL;
    }
    return handle;
}

static NvError InitFifo(NvIdlFifoPair *pFifo)
{
    static pthread_mutex_t daemon_lock = PTHREAD_MUTEX_INITIALIZER;
    NvError                e           = NvSuccess;
    NvU32                  cmd_buf[5];
    struct iovec           iov[1];
    struct msghdr          msghdr;
    struct cmsghdr        *cmsghdrp;
    struct pollfd          pfd;
    NvDaemonFdMsg          daemon_msg;
    int                    rc;
    int                    conn_fd = 0;
    NvU32                  start;

    pthread_mutex_lock(&daemon_lock);

    if (daemon_fd == -1) {
        struct sockaddr_un remote;
        socklen_t          len;

        NV_ASSERT(!s_RmClientId);

        daemon_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (daemon_fd == -1) {
            NvOsDebugPrintf("failed to create daemon communication node.\n");
            e = NvError_AccessDenied;
            goto out;
        }

        start = NvOsGetTimeMS();
        do {
            struct stat node_stat;
            if (NvOsGetTimeMS()-start >= 2000) {
                NvOsDebugPrintf("2s elapsed in InitFifo; is daemon started?\n");
                start = NvOsGetTimeMS();
            }
            rc = stat(NVRM_DAEMON_SOCKNAME,&node_stat);
            if (rc==-1 && errno==ENOENT) {
                NvOsSleepMS(100);
            }
        } while (rc==-1 && errno==ENOENT);

        if (rc==-1) {
            NvOsDebugPrintf("failed to get information of daemon communication node.\n");
            e = NvError_AccessDenied;
            goto out;
        }

        do {
            remote.sun_family = AF_UNIX;
            len = (socklen_t)NvOsStrlen(NVRM_DAEMON_SOCKNAME);
            NvOsStrncpy(remote.sun_path, NVRM_DAEMON_SOCKNAME, (size_t)len+1);
            len += (socklen_t)sizeof(remote.sun_family);
            rc = connect(daemon_fd, (struct sockaddr *)&remote, len);

            if (rc == -1)
                NvOsSleepMS(1);
        } while (rc==-1);

        if (!rc) {
            NvOsFileHandle hRm = GetIoctlFile();
            NvError status;

            pfd.fd = daemon_fd;
            pfd.revents = 0;
            pfd.events = POLLIN | POLLHUP;
            rc = poll(&pfd, 1, 1000);

            if (rc <= 0) {
                NvOsDebugPrintf("failed to wait event.\n");
                rc = -1;
            }

            else {
                e = NvIdlHelperFifoRead((void*)daemon_fd, &status,
                        sizeof(status), NULL);

                if (e == NvSuccess)
                {
                    e = status;
                    if( e != NvSuccess )
                        NvOsDebugPrintf("client is not in ready state (err=0x%x)\n",e);
                }else
                    NvOsDebugPrintf("failed to read FIFO (err=0x%x).\n", e);

                if (!hRm)
                    e = NvError_KernelDriverNotFound;

                if (e==NvSuccess) {
                    e = NvOsIoctl(hRm, NvRmIoctls_NvRmGetClientId,
                            &s_RmClientId, 0, 0, sizeof(s_RmClientId));
                }
            }

        }

        if (rc==-1 || e!=NvSuccess || !s_RmClientId) {
            if( !s_RmClientId )
                NvOsDebugPrintf("can not get valid client ID.\n");
            if( e!=NvSuccess )
                NvOsDebugPrintf("failed to connect to client (err=0x%x).\n",e);
            close(daemon_fd);
            daemon_fd = -1;
            e = NvError_AccessDenied;
            goto out;
        }
    }

    NV_ASSERT(daemon_fd!=-1 && s_RmClientId);

    cmd_buf[0] = NvRmDaemonCode_FifoCreate;
    cmd_buf[1] = s_DaemonClientId;
    cmd_buf[2] = s_RmClientId;
    cmd_buf[3] = syscall(__NR_gettid);
    cmd_buf[4] = getpid();

    e = NvIdlHelperFifoWrite((void *)daemon_fd, cmd_buf, sizeof(cmd_buf));
    if (e != NvSuccess) {
        NvOsDebugPrintf("failed to issue command to daemon\n");
        goto out;
    }

    /* allow the daemon at most 1 second to process the command request */
    pfd.fd = daemon_fd;
    pfd.revents = 0;
    pfd.events = POLLIN | POLLHUP;
    rc = poll(&pfd, 1, 1000);
    if (rc <= 0) {
        NvOsDebugPrintf("daemon timeout to wait for event\n");
        e = NvError_Timeout;
        goto out;
    }

    e = NvIdlHelperFifoRead((void *)daemon_fd, cmd_buf,
            sizeof(cmd_buf[0]), NULL);

    if (e == NvSuccess)
        e = cmd_buf[0];

    if (e != NvSuccess) {
        NvOsDebugPrintf("daemon failed to create new connection\n");
        goto out;
    }

    memset(&msghdr, 0, sizeof(msghdr));
    iov[0].iov_base = cmd_buf;
    iov[0].iov_len  = sizeof(cmd_buf[0]);
    msghdr.msg_control = daemon_msg.data;
    msghdr.msg_controllen = sizeof(daemon_msg.data);
    msghdr.msg_iov = iov;
    msghdr.msg_iovlen = 1;

    rc = recvmsg(daemon_fd, &msghdr, MSG_WAITALL);
    if (rc<=0) {
        NvOsDebugPrintf("failed to read new connection from daemon\n");
        e = NvError_FileReadFailed;
        goto out;
    }

    cmsghdrp = CMSG_FIRSTHDR(&msghdr);
    if (!cmsghdrp || cmsghdrp->cmsg_len!=CMSG_LEN(sizeof(int)) ||
        cmsghdrp->cmsg_level!=SOL_SOCKET || cmsghdrp->cmsg_type!=SCM_RIGHTS) {
        NvOsDebugPrintf("invalid message received\n");
        e = NvError_InvalidSize;
        goto out;
    }

    // why do we set s_DaemonClientId even when it's already non-zero?
    // this is because the s_DaemonClientId may already become stale,
    // in this case, daemon side will allocate a new reftrack client
    // for us and we need to update.
    s_DaemonClientId = cmd_buf[0];

    // gcc 4.4.1 complains about aliasing in the following statement.
    // A union makes the compiler happy.
    //
    // conn_fd = *((int*)CMSG_DATA(cmsghdrp));
    {
        union {
            unsigned char *c;
            int *i;
        } u;
        u.c = CMSG_DATA(cmsghdrp);
        conn_fd = u.i[0];
    }

 out:
    pthread_mutex_unlock(&daemon_lock);

    pFifo->FifoIn = (void *)conn_fd;
    pFifo->FifoOut = (void *)conn_fd;
    return e;
}

static void FifoTerminator(void *context)
{
    NvIdlFifoPair *pFifo = (NvIdlFifoPair *)context;
    shutdown((int)pFifo->FifoIn, SHUT_RDWR);
    close((int)pFifo->FifoIn);
    NvOsFree(pFifo);
}

NvError NvIdlHelperGetFifoPair(NvIdlFifoPair **pFifo)
{
    NvIdlFifoPair *pAlloc = NULL;
    NvU32 tls_key = GetFifoTls();
    NvError e;

    if (tls_key == NVOS_INVALID_TLS_INDEX) {
        NvOsDebugPrintf("Invalid TLS key");
        return NvError_InsufficientMemory;
    }

    pAlloc = NvOsTlsGet(tls_key);
    if (pAlloc) {
        *pFifo = pAlloc;
        return NvSuccess;
    }

    pAlloc = NvOsAlloc(sizeof(NvIdlFifoPair));
    if (!pAlloc) {
        NvOsDebugPrintf("Unable to allocate FIFO for thread\n");
        return NvError_InsufficientMemory;
    }
    e = InitFifo(pAlloc);
    if (e==NvSuccess) {
        NvOsTlsSet(tls_key, pAlloc);

        /*
         * pthread_key destructor is not invoked for main thread.
         * If NvOsTlsAddTerminator() is called for main thread here,
         * NvOsAlloc in the function causes memory leak.
         * See bug 654447.
         */
        if (syscall(__NR_gettid) != getpid())
        {
            NvOsTlsAddTerminator(FifoTerminator, pAlloc);
        }
        *pFifo = pAlloc;
    }
    else {
        NvOsDebugPrintf("Failed to initialize FIFO\n");
        NvOsFree(pAlloc);
        *pFifo = NULL;
    }

    return e;
}

void NvIdlHelperReleaseFifoPair(NvIdlFifoPair *pFifo)
{
    NvU32 tls_key = GetFifoTls();
    if (pFifo != NvOsTlsGet(tls_key)) {
        NvOsDebugPrintf("Attempting to free a different thread's FIFO\n");
    }
}

#if __GNUC__ > 3
#define DESTRUCTOR(_x) void __attribute__ ((destructor)) _x
#else
#define DESTRUCTOR(_x) void _fini
#endif

DESTRUCTOR(idlhelper_fini)(void);
DESTRUCTOR(idlhelper_fini)(void)
{
    NvU32 tls_key = GetFifoTls();

    if (tls_key != NVOS_INVALID_TLS_INDEX)
    {
        /*
         * pthread_key destructor is not called for main thread.
         * FifoTerminator has to be called explicitly here.
         * See bug 654447.
         */
        void *p = NvOsTlsGet(tls_key);
        /*
         * pthread_key destructor should have been called for all threads other
         * main thread, so for main thread we shall terminate FIFO, but for
         * non-main thread, we shall not do that, because it should have been
         * freed already in pthread_key destructor. In case this has not
         * happened yet, that thread will eventually die, by that time the
         * pthread_key destructor will take care of things.
         */
        if (p && (getpid()==syscall(__NR_gettid)))
        {
            FifoTerminator(p);
        }

        NvOsTlsFree(tls_key);
    }

    if (daemon_fd != -1) {
        shutdown(daemon_fd, SHUT_RDWR);
        close(daemon_fd);
    }
}
