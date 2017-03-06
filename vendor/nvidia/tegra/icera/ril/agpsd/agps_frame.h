/*************************************************************************************************
 * NVidia Corporation
 * Copyright (c) 2011
 * All rights reserved
 *************************************************************************************************
  * $Revision: #1 $
 * $Date: 2010/08/04 $
 * $Author: gheinrich $
 ************************************************************************************************/

/**
 * @addtogroup agps
 * @{
 */

/**
 * @file agps_frame.h NVidia aGPS RIL Frame
 *       (encapsulation/de-encapsulation)
 *
 */

#ifndef AGPS_FRAME_H
#define AGPS_FRAME_H

/*************************************************************************************************
 * Project header files
 ************************************************************************************************/

#include "agps.h"

/*************************************************************************************************
 * Standard header files (e.g. <string.h>, <stdlib.h> etc.
 ************************************************************************************************/

/*************************************************************************************************
 * Macros
 ************************************************************************************************/

/*************************************************************************************************
 * Public Types
 ************************************************************************************************/

typedef enum
{
    AGPS_RRC_EVENT_MSG =0,
    AGPS_STACK_RAT_INFO,
    /*Downlink */
    AGPS_DL_RRLP ,
    AGPS_RRC_DATA_IND,
    AGPS_GRR_HO_IND,
    AGPS_STATE_CHANGE,
    AGPS_RRC_HO_IND,
    AGPS_MM_SYNC_IND,
     /*Uplink */
    AGPS_RRLP_UL_RRLP,
    AGPS_RRC_REPORT_MEAS_RESULT,
    AGPS_RRC_REPORT_EVENT_RESULT,
    inUsedbyHost0,
    inUsedbyHost1,
    AGPS_LCS_END_IND,
    AGPS_MM_RESET_UE_IND,
    /* UL/DL DCCH */
    AGPS_RRC_UL_DCCH,
    AGPS_RRC_DL_DCCH,
    /* calibration */
    AGPS_CALIBRATION_START_REQ,
    AGPS_CALIBRATION_STATUS,
    AGPS_CALIBRATION_STOP_REQ,
    /* this should be the last item */
    AGPS_INVALID_PAYLOAD_TYPE
} agps_FramePayloadType;

/*************************************************************************************************
 * Public variable declarations
 ************************************************************************************************/

typedef struct
{
    const char *device_path;
} agps_FrameConfig;

/*************************************************************************************************
 * Public function declarations
 ************************************************************************************************/

/**
 * Initialize the frame layer
 * @param config Pointer to FRAME layer config
 * @return see agps_ReturnCode
 */
agps_ReturnCode agps_FrameInitialize(agps_FrameConfig *config);

/**
 * Un-initialize frame layer
 * @return see agps_ReturnCode
 */
agps_ReturnCode agps_FrameUnitialize(void);

/**
 * Receive a frame
 * @param payload_type Payload type
 * @param buffer Buffer to copy payload into
 * @param len Size of buffer pointed to by buffer
 * @param bytes_received Pointer to integer to write payload
 *                       size into
 * @return see agps_ReturnCode
 */
agps_ReturnCode agps_FrameRecv(agps_FramePayloadType *payload_type, char *buffer, int len, int *bytes_received);


/**
 * Send a frame
 *
 * @param payload_type Payload type
 * @param buffer Buffer to get payload data from
 * @param len Size of buffer pointed to by buffer
 * @return see agps_ReturnCode
 */
agps_ReturnCode agps_FrameSend(agps_FramePayloadType payload_type, const char *buffer, int payload_size);

#endif /* AGPS_FRAME_H */

