
/*************************************************************************************************
 * NVidia Corporation
 * Copyright (c) 2011
 * All rights reserved
 *************************************************************************************************
 * $Revision: #1 $
 * $Date: $
 * $Author: $
 ************************************************************************************************/

/**
 * @addtogroup agps
 * @{
 */

/**
 * @file agps_transport.c NVidia aGPS RIL transport layer
 *
 *
 */

/*************************************************************************************************
 * Project header files. The first in this list should always be the public interface for this
 * file. This ensures that the public interface header file compiles standalone.
 ************************************************************************************************/

#include "agps_transport.h"

/*************************************************************************************************
 * Standard header files (e.g. <string.h>, <stdlib.h> etc.
 ************************************************************************************************/

#include <stdio.h>
/* vfprintf... */
#include <stdarg.h>
/* errno... */
#include <errno.h>
/* open */
#include <sys/types.h>  /* lseek */
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

/**************************************************************************************************************/
/* Private Macros  */
/**************************************************************************************************************/

#define AGPS_TRANSPORT_OPEN_MIN_DELAY 1
#define AGPS_TRANSPORT_OPEN_MAX_DELAY 16

/**************************************************************************************************************/
/* Private Functions Declarations (if needed) */
/**************************************************************************************************************/

/**************************************************************************************************************/
/* Private Type Declarations */
/**************************************************************************************************************/

/**************************************************************************************************************/
/* Private Variable Definitions  */
/**************************************************************************************************************/

/*************************************************************************************************
 * Private function definitions
 ************************************************************************************************/

/*************************************************************************************************
 * Public function definitions (Not doxygen commented, as they should be exported in the header
 * file)
 ************************************************************************************************/

int agps_TransportOpen(const char *pathname)
{
    int fd;
    int arg;
    int delay = AGPS_TRANSPORT_OPEN_MIN_DELAY;

    ALOGD("%s: Opening %s \n", __FUNCTION__, pathname );

    do
    {
        /* open device file */
        fd = open(pathname, O_RDWR, 0);

        /* configure terminal */
        if (fd>=0)
        {
            struct termios options;

            ALOGD("%s: Configuring %s \n", __FUNCTION__, pathname );

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
            arg = TIOCM_DTR;
            ioctl(fd, TIOCMBIS, &arg);

            break;
        }
        else
        {
            if (delay < AGPS_TRANSPORT_OPEN_MAX_DELAY)
            {
                ALOGE("%s: Failed to open file [%s] - errno = %d \n", __FUNCTION__, pathname, fd);
            }

            sleep(delay);

            if (delay < AGPS_TRANSPORT_OPEN_MAX_DELAY)
            {
                delay *= 2;

                if (delay == AGPS_TRANSPORT_OPEN_MAX_DELAY )
                {
                    ALOGE("%s: Retrying silently \n", __FUNCTION__);
                }
            }
        }
    } while (1);

    return fd;
}

int agps_TransportClose(int fd)
{
    ALOGD("%s: Closing file descriptor \n", __FUNCTION__);

    return close(fd);
}

int agps_TransportRead(int fd, char *buffer, int len)
{
    int res, ret;
    int bytes_read;

    bytes_read = 0;
    ret = 0;

    /* loop until we have read the requested number of bytes */
    while (bytes_read < len)
    {
        res = read(fd, buffer + bytes_read, len - bytes_read);

        if (res > 0)
        {
             bytes_read += res;
        }
        else
        {
             /* 0 means EOF, strictly negative codes mean error */
             ret = res<0 ? res : -1;
             break;
        }
    }

    return ret;
}

int agps_TransportWrite(int fd, const char *buffer, int count)
{
    int res, ret;
    int bytes_written;

    bytes_written = 0;
    ret = 0;

    /* loop until we have written the requested number of bytes */
    while (bytes_written < count)
    {
        res = write(fd, buffer + bytes_written, count - bytes_written);

        if (res > 0)
        {
             bytes_written += res;
        }
        else
        {
             /* 0 means EOF, strictly negative codes mean error */
             ret = res<0 ? res : -1;
             break;
        }
    }

    return ret;
}




