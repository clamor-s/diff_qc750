
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
 * @file agrild_port.c NVidia aGPS RIL Frame
 *       (encapsulate/de-encapsulate) layer
 *
 */

/*************************************************************************************************
 * Project header files. The first in this list should always be the public interface for this
 * file. This ensures that the public interface header file compiles standalone.
 ************************************************************************************************/

#include "agps_frame.h"
#include "agps_transport.h"

/*************************************************************************************************
 * Standard header files (e.g. <string.h>, <stdlib.h> etc.
 ************************************************************************************************/

#include <netinet/in.h>

/**************************************************************************************************************/
/* Private Macros  */
/**************************************************************************************************************/

#define AGPS_FRAME_HEADER_ID (0x1234)
#define AGPS_FRAME_FOOTER_ID (0x5678)

#define AGPS_FRAME_HEADER_SIZE (5)
#define AGPS_FRAME_FOOTER_SIZE (2)

/**************************************************************************************************************/
/* Private Functions Declarations (if needed) */
/**************************************************************************************************************/

/**************************************************************************************************************/
/* Private Type Declarations */
/**************************************************************************************************************/

/** type definition for the FRAME layer state */
typedef struct
{
    agps_FrameConfig config; /**< config */
    int fd;                   /**< device file descriptor */
} agps_FrameState;

/** type definition for the frame header */
typedef struct
{
    unsigned short header_id;
    unsigned short packet_size;
    unsigned char  payload_type;
} agps_FrameHeader;

/** type definition for the frame footer */
typedef struct
{
    unsigned short footer_id;
} agps_FrameFooter;

/**************************************************************************************************************/
/* Private Variable Definitions  */
/**************************************************************************************************************/

static agps_FrameState agps_frame_state;

/*************************************************************************************************
 * Private function definitions
 ************************************************************************************************/

static void dumpBuffer(const char *prefix, const char *buffer, int len)
{
    int i;
    const int line_width = 32;

    for (i=0;i<len; i++)
    {
        int j;
        static char tmpbuf[200];
        tmpbuf[0]='\0';
        for (j=0; j<line_width; j++)
        {
            if (i+j<len)
            {
                static char tmpbuf2[200];
                sprintf(tmpbuf2,"0x%02x,", buffer[i+j]);
                strcat(tmpbuf,tmpbuf2);
            }
        }
        ALOGD("%s %s\n", prefix, tmpbuf);
        i+=line_width;
    }
}

/*************************************************************************************************
 * Public function definitions (Not doxygen commented, as they should be exported in the header
 * file)
 ************************************************************************************************/

agps_ReturnCode agps_FrameInitialize(agps_FrameConfig *config)
{
    agps_ReturnCode ret;
    int fd;

    /* the following do while (0) is used to avoid nested if and return statements */
    do
    {
        if (!config )
        {
            ALOGE("%s: Pointer to config is NULL \n", __FUNCTION__ );
            ret = -AGPS_ERROR_INVALID_PARAMETER;
            break;
        }

        /* copy config */
        memcpy(&agps_frame_state.config, config, sizeof(agps_FrameConfig) );

        /* open device path */
        agps_frame_state.fd = agps_TransportOpen(agps_frame_state.config.device_path);
        if (agps_frame_state.fd < 0)
        {
            ALOGE("%s: Failed to open file [%s] \n", __FUNCTION__, agps_frame_state.config.device_path );
            ret = -AGPS_ERROR_FILE_OPEN;
            break;
        }

        ret = AGPS_OK;

    } while (0);

    return ret;
}

agps_ReturnCode agps_FrameUnitialize(void)
{
    agps_ReturnCode ret;

    if (agps_TransportClose(agps_frame_state.fd) == 0 )
    {
        ret = AGPS_OK;
    }
    else
    {
        ALOGE("%s: Failed to close file [%s] \n", __FUNCTION__, agps_frame_state.config.device_path );
        ret = -AGPS_ERROR_FILE_CLOSE;
    }

    agps_frame_state.fd = -1;

    return ret;
}

agps_ReturnCode agps_FrameRecv(agps_FramePayloadType *payload_type, char *buffer, int len, int *bytes_received)
{
    agps_ReturnCode ret;
    agps_FrameHeader header;
    agps_FrameFooter footer;
    int res;
    int payload_size;

    ALOGI("%s: Waiting for downlink frame \n", __FUNCTION__ );

    /* the following do while (0) is used to avoid nested if and return statements */
    do
    {
        if ( (!bytes_received ) || (!payload_type) || (!buffer) )
        {
            ALOGE("%s: One of (bytes_received,payload_type,buffer)=(0x%08x,0x%08x,0x%08x) is NULL \n", __FUNCTION__,
                 (unsigned int)bytes_received, (unsigned int)payload_type, (unsigned int)buffer );
            ret = -AGPS_ERROR_INVALID_PARAMETER;
            break;
        }

        /* read header (blocking) */
        res = agps_TransportRead( agps_frame_state.fd, (char*)&header, AGPS_FRAME_HEADER_SIZE );
        if (res != 0)
        {
            ALOGE("%s: I/O error %d while reading header\n", __FUNCTION__, res );
            ret = -AGPS_ERROR_FILE_READ;
            break;
        }

        /* endianness conversions */
        header.header_id = ntohs(header.header_id);
        header.packet_size = ntohs(header.packet_size);
        payload_size = header.packet_size - AGPS_FRAME_HEADER_SIZE - AGPS_FRAME_FOOTER_SIZE;

        if (header.header_id != AGPS_FRAME_HEADER_ID)
        {
            ALOGE("%s: header_id mismatch (expected 0x%x, got 0x%x) \n", __FUNCTION__,
                  AGPS_FRAME_HEADER_ID, header.header_id);
            ret = -AGPS_ERROR_INVALID_FRAME;
            break;
        }

        if (header.payload_type > AGPS_INVALID_PAYLOAD_TYPE)
        {
            ALOGE("%s: invalid payload type (0x%x) \n", __FUNCTION__, header.payload_type);
            ret = -AGPS_ERROR_INVALID_FRAME;
            break;
        }

        *payload_type = header.payload_type;

        if (payload_size > len)
        {
            ALOGE("%s: Provided buffer too small (need %d bytes, have %d bytes) \n", __FUNCTION__,
                 payload_size, len);
            ret = -AGPS_ERROR_BUFFER_TOO_SMALL;
            break;
        }

        /* read payload (blocking) */

        ALOGD("%s: Header received, now reading payload (%d bytes)\n", __FUNCTION__, payload_size );

        res = agps_TransportRead( agps_frame_state.fd, buffer, payload_size );
        if (res != 0)
        {
            ALOGE("%s: I/O error %d while reading payload\n", __FUNCTION__, res );
            ret = -AGPS_ERROR_FILE_READ;
            break;
        }

        *bytes_received = payload_size;

        /* read footer (blocking) */
        res = agps_TransportRead( agps_frame_state.fd, (char*)&footer, AGPS_FRAME_FOOTER_SIZE );
        if (res != 0)
        {
            ALOGE("%s: I/O error %d while reading footer\n", __FUNCTION__, res );
            ret = -AGPS_ERROR_FILE_READ;
            break;
        }

        /* endianness conversion */
        footer.footer_id = ntohs(footer.footer_id);

        if (footer.footer_id != AGPS_FRAME_FOOTER_ID)
        {
            ALOGE("%s: footer_id mismatch (expected 0x%x, got 0x%x) \n", __FUNCTION__,
                  AGPS_FRAME_FOOTER_ID, footer.footer_id);
            ret = -AGPS_ERROR_INVALID_FRAME;
            break;
        }

        ALOGI("%s: Frame received with payload_type=%d, payload_size=%d \n", __FUNCTION__,
             header.payload_type, payload_size  );

#if 0
        if (payload_size>0)
        {
            dumpBuffer("mdm->ap:",  buffer, payload_size);
        }
#endif

        ret = AGPS_OK;

    } while (0);

    return ret;
}

agps_ReturnCode agps_FrameSend(agps_FramePayloadType payload_type, const char *buffer, int payload_size)
{
    agps_FrameHeader header;
    agps_FrameFooter footer;
    agps_ReturnCode ret;
    int res;

    do
    {

        /* create header */
        header.header_id = htons(AGPS_FRAME_HEADER_ID);
        header.payload_type = payload_type;
        header.packet_size = htons(payload_size + AGPS_FRAME_HEADER_SIZE + AGPS_FRAME_FOOTER_SIZE);

        /* send header */
        res = agps_TransportWrite( agps_frame_state.fd, (char*)&header, AGPS_FRAME_HEADER_SIZE );
        if (res != 0)
        {
            ALOGE("%s: I/O error %d while writing header\n", __FUNCTION__, res );
            ret = -AGPS_ERROR_FILE_WRITE;
            break;
        }

        /* send payload */
        res = agps_TransportWrite( agps_frame_state.fd, buffer, payload_size );
        if (res != 0)
        {
            ALOGE("%s: I/O error %d while writing payload\n", __FUNCTION__, res );
            ret = -AGPS_ERROR_FILE_READ;
            break;
        }

        /* send footer */
        footer.footer_id = htons(AGPS_FRAME_FOOTER_ID);

        res = agps_TransportWrite( agps_frame_state.fd, (char*)&footer, AGPS_FRAME_FOOTER_SIZE );
        if (res != 0)
        {
            ALOGE("%s: I/O error %d while writing footer\n", __FUNCTION__, res );
            ret = -AGPS_ERROR_FILE_WRITE;
            break;
        }

        /* done ! */

        ALOGI("%s: Frame sent with payload_type=%d, payload_size=%d \n", __FUNCTION__,
             payload_type, payload_size  );

#if 0
        if (payload_size>0)
        {
            dumpBuffer("ap->mdm: ", buffer, payload_size);
        }
#endif

        ret = AGPS_OK;

    } while (0);

    return ret;
}
