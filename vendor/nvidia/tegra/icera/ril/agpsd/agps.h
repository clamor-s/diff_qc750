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
 * @file agps.h NVidia aGPS RIL
 *
 */

#ifndef AGPS_H
#define AGPS_H

/*************************************************************************************************
 * Project header files
 ************************************************************************************************/

#define LOG_TAG "AGPSD"
#include <utils/Log.h>
#include <cutils/properties.h> /* system properties */

/*************************************************************************************************
 * Standard header files (e.g. <string.h>, <stdlib.h> etc.
 ************************************************************************************************/

/*************************************************************************************************
 * Macros
 ************************************************************************************************/

/*************************************************************************************************
 * Public Types
 ************************************************************************************************/

/** enumeration of return codes */
typedef enum
{
    AGPS_OK,
    AGPS_ERROR_FILE_OPEN,
    AGPS_ERROR_FILE_CLOSE,
    AGPS_ERROR_FILE_READ,
    AGPS_ERROR_FILE_WRITE,
    AGPS_ERROR_BUFFER_TOO_SMALL,
    AGPS_ERROR_INVALID_PARAMETER,
    AGPS_ERROR_INVALID_FRAME,
    AGPS_ERROR_REF_CLOCK_UNAVAILABLE,
    AGPS_ERROR_TESTCASE,
    AGPS_ERROR_UNKNOWN_PAYLOAD_TYPE,
    AGPS_ERROR_GPAL_DOWNLINK_MSG,
    AGPS_ERROR_GPAL_RESET_ASSISTANCE_DATA,
    AGPS_ERROR_GPAL_DEINIT,
    AGPS_ERROR_GPAL_SET_CALIBRATION_STATUS,
    AGPS_ERROR_PROTOCOL_UNSUPPORTED,
    AGPS_ERROR_FRAME_SEND,
    AGPS_ERROR_FEATURE_UNSUPPORTED,
    AGPS_ERROR_OUT_OF_MEMORY,
    AGPS_ERROR_TOO_MANY_OPTIONS,
    AGPS_ERROR_INVALID_OPTION_SPEC,
    AGPS_ERROR_OPTION_PARSING,
    AGPS_ERROR_TIMER_CREATE,
} agps_ReturnCode;

/*************************************************************************************************
 * Public variable declarations
 ************************************************************************************************/

/*************************************************************************************************
 * Public function declarations
 ************************************************************************************************/

#endif /* AGPS_H */

