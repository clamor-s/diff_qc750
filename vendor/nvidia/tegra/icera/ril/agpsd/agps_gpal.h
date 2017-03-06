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
 * @file agps_gpal.h NVidia aGPS RIL GPS Platform Abstraction
 *       Layer
 *
 */

#ifndef AGPS_GPAL_H
#define AGPS_GPAL_H

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

/**
* Protocol
*/
typedef enum {
    AGPS_PROTOCOL_RRLP,
    AGPS_PROTOCOL_RRC,
    AGPS_PROTOCOL_LPP
} agps_GpalProtocol;

/**
* UE State
*/
typedef enum {
    AGPS_UE_STATE_CELL_DCH,
    AGPS_UE_STATE_CELL_FACH,
    AGPS_UE_STATE_CELL_PCH,
    AGPS_UE_STATE_URA_PCH,
    AGPS_UE_STATE_IDLE
} agps_GpalUeState;

/**
* Send uplink message to network
*
* @param protocol The protocol used when encoding the message
* @param msg Buffer to ASN1-encoded message
* @param size Size of buffer pointed to by msg
* @param agps_handle Handle provided by agps_GpalInit()
* @return Return code
*/
typedef agps_ReturnCode (*agps_GpalSendUplinkMsg)(agps_GpalProtocol protocol,
                                       const char *msg,
                                       unsigned int size,
                                       void *agps_handle);

/**
* Start calibration (request the baseband reference clock)
*
* The aGPS daemon is expected to send a message to the baseband
* to request the reference clock. The aGPS daemon must
* subsequently call agps_GpalCalibrationStatus() to let the
* GPAL know if/when calibration can start
*
* @param agps_handle Handle provided by agps_GpalInit()
* @return Return code
*/
typedef agps_ReturnCode (*agps_GpalCalibrationStart)(void *agps_handle);

/**
* Stop calibration (release request for baseband reference
* clock)
*
* If the baseband was able to provide a stable clock, this
* function will return AGPS_OK, otherwise, it will return an
* error (AGPS_ERROR_REF_CLOCK_UNAVAILABLE)
*
* @param agps_handle Handle provided by agps_GpalInit()
* @return Return code
*/
typedef agps_ReturnCode (*agps_GpalCalibrationStop)(void *agps_handle);

/**
 *  GPAL callbacks (GPAL->aGPS)
 */
typedef struct
{
    agps_GpalSendUplinkMsg send_uplink_msg;
    agps_GpalCalibrationStart calibration_start;
    agps_GpalCalibrationStop calibration_stop;
} agps_GpalCallbacks;

/**
 * Initialize the GPS Platform Abstraction Layer
 *
 * @param callbacks Pointer to callbacks structure
 * @param agps_handle Handle to be passed to callbacks
 * @return handle to be passed to GPAL functions
 */
typedef void * (*agps_GpalInit)(const agps_GpalCallbacks *callbacks, void *agps_handle);

/**
 * De-Initialize the GPS Platform Abstraction Layer
 *
 * @return gps_handle Handle provided by agps_GpalInit()
 */
typedef agps_ReturnCode (*agps_GpalDeInit)(void *gps_handle);

/**
* Send downlink message to GPS
*
* @param gps_handle The handle returned by agps_GpalInit()
* @param protocol The protocol used when encoding the message
* @param msg Buffer to ASN1-encoded message
* @param size Size of buffer pointed to by msg
* @return Return code
*/
typedef agps_ReturnCode (*agps_GpalSendDownlinkMsg)(void *gps_handle,
                                                    agps_GpalProtocol protocol,
                                                    const char *msg,
                                                    unsigned int size);


/**
* Set calibration status following a call to
* agps_GpalCalibrationStart() or agps_GpalCalibrationStop()
*
* If the baseband is in a state which does not allow for clock
* calibration the 'locked' will be set to 0. If the reference
* clock is suitable for calibration, 'locked' with be set to 1
*
* @param gps_handle The handle returned by agps_GpalInit()
* @param locked flag to indicate whether the reference clock is
*               locked
* @return Return code
*/
typedef agps_ReturnCode (*agps_GpalCalibrationStatus)(void *gps_handle,
                                                      int locked);

/**
* Set UE state
*
* @param gps_handle The handle returned by agps_GpalInit()
* @param protocol The protocol used when encoding the message
* @param state UE state
* @return Return code
*/
typedef agps_ReturnCode (*agps_GpalSetUeState)(void *gps_handle,
                                               agps_GpalProtocol protocol,
                                               agps_GpalUeState state);

/**
* Reset assistance data
*
* @param gps_handle The handle returned by agps_GpalInit()
* @return Return code
*/
typedef agps_ReturnCode (*agps_GpalResetAssistanceData)(void *gps_handle);

/** GPAL functions (aGPS->GPS)   */
typedef struct
{
    agps_GpalInit init;
    agps_GpalSendDownlinkMsg send_downlink_msg;
    agps_GpalSetUeState set_ue_state;
    agps_GpalResetAssistanceData reset_assistance_data;
    agps_GpalCalibrationStatus set_calibration_status;
    agps_GpalDeInit deinit;
} agps_GpalFunctions;

/*************************************************************************************************
 * Public variable declarations
 ************************************************************************************************/

/*************************************************************************************************
 * Public function declarations
 ************************************************************************************************/

/**
* Create GPAL interface
*
* @return Pointer to agps_GpalFunctions structure
*/
const agps_GpalFunctions* agps_GpalCreate(void);

#endif /* AGPS_GPAL_H */

