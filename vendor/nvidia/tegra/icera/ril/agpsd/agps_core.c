
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
 * @file agps_port.c NVidia aGPS RIL Core Layer
 *
 */

/*************************************************************************************************
 * Project header files. The first in this list should always be the public interface for this
 * file. This ensures that the public interface header file compiles standalone.
 ************************************************************************************************/

#include "agps_core.h"
#include "agps_port.h"
#include "agps_frame.h"
#include "agps_daemon.h"
#include "agps_gpal_test.h"
#include "agps_gpal.h"

/*************************************************************************************************
 * Standard header files (e.g. <string.h>, <stdlib.h> etc.
 ************************************************************************************************/

/**************************************************************************************************************/
/* Private Macros  */
/**************************************************************************************************************/

#define AGPS_CORE_MAX_PAYLOAD_SIZE ( 1024 )

/**************************************************************************************************************/
/* Private Functions Declarations (if needed) */
/**************************************************************************************************************/

static agps_ReturnCode agps_CoreGpalSendUplinkMsg(agps_GpalProtocol protocol,
                                           const char *msg,
                                           unsigned int size,
                                           void *agps_handle);

static agps_ReturnCode agps_CoreGpalCalibrationStart(void *agps_handle);

static agps_ReturnCode agps_CoreGpalCalibrationStop(void *agps_handle);


/**************************************************************************************************************/
/* Private Type Declarations */
/**************************************************************************************************************/

typedef struct
{
    agps_CoreConfig config;
    void *gpal_handle;
    const agps_GpalFunctions *gpal_functions;
} agps_CoreState;

/**************************************************************************************************************/
/* Private Variable Definitions  */
/**************************************************************************************************************/

/**
 * the CORE layer state
 */
static agps_CoreState agps_core_state;

/**
 * Buffer to read frames
 */
static char agps_core_buffer[AGPS_CORE_MAX_PAYLOAD_SIZE];

/**
 * GPAL->aGPS callbacks
 */
const agps_GpalCallbacks agps_core_gpal_callbacks = {
    agps_CoreGpalSendUplinkMsg,
    agps_CoreGpalCalibrationStart,
    agps_CoreGpalCalibrationStop
};

/*************************************************************************************************
 * Private function definitions
 ************************************************************************************************/

/**
 * Process an incoming frame
 * @param payload_type Payload type
 * @param buffer Pointer to buffer containing message
 * @param len Size of buffer pointed to by buffer
 * @return AGPS_OK on success, other return codes on error
 */
static agps_ReturnCode agps_CoreProcessRxFrame(agps_FramePayloadType payload_type, const char *buffer, int len)
{
    agps_ReturnCode ret = AGPS_OK;

    switch (payload_type)
    {
    /*Downlink */
    case AGPS_RRC_DL_DCCH:
    	ALOGI("%s: Calling GPAL send_downlink_msg (RRC) \n", __FUNCTION__);
        ret = agps_core_state.gpal_functions->send_downlink_msg(agps_core_state.gpal_handle,
                                                                AGPS_PROTOCOL_RRC,
                                                                buffer,
                                                                len);
    	break;
    case AGPS_DL_RRLP:
        ALOGI("%s: Calling GPAL send_downlink_msg (RRLP) \n", __FUNCTION__);
        ret = agps_core_state.gpal_functions->send_downlink_msg(agps_core_state.gpal_handle,
                                                                AGPS_PROTOCOL_RRLP,
                                                                buffer,
                                                                len);
    	break;
    case AGPS_MM_RESET_UE_IND:
        ALOGI("%s: Calling GPAL reset_assistance_data\n", __FUNCTION__);
        ret = agps_core_state.gpal_functions->reset_assistance_data(agps_core_state.gpal_handle);
        break;
    case AGPS_CALIBRATION_STATUS:
        ALOGI("%s: Received AGPS_CALIBRATION_STATUS - status = %d\n", __FUNCTION__, (int)buffer[0]);
        ret = agps_core_state.gpal_functions->set_calibration_status(agps_core_state.gpal_handle,
                                                                     (int)buffer[0]);
        break;
    case AGPS_RRC_EVENT_MSG:
    case AGPS_STACK_RAT_INFO:
    case AGPS_GRR_HO_IND:
    case AGPS_STATE_CHANGE:
    case AGPS_RRC_HO_IND:
    case AGPS_MM_SYNC_IND:
        ALOGI("%s: Unhandled payload type (%d)\n", __FUNCTION__, payload_type);
        break;
     /*Uplink */
    case AGPS_RRLP_UL_RRLP:
    case AGPS_RRC_REPORT_MEAS_RESULT:
    case AGPS_RRC_REPORT_EVENT_RESULT:
    case inUsedbyHost0:
    case inUsedbyHost1:
    case AGPS_LCS_END_IND:
    case AGPS_RRC_DATA_IND:
    case AGPS_CALIBRATION_START_REQ:
    case AGPS_CALIBRATION_STOP_REQ:
        ALOGI("%s: Received unexpected uplink message (%d)\n", __FUNCTION__, payload_type);
        break;
    default:
        ALOGE("%s: Unknown payload type (%d)\n", __FUNCTION__, payload_type);
        ret = -AGPS_ERROR_UNKNOWN_PAYLOAD_TYPE;
        break;
    }

    return ret;
}

/**
 * Send ASN1-encoded message to baseband
 * @param protocol Protocol used for encoding message
 * @param buffer Pointer to buffer containing message
 * @param len Size of buffer pointed to by buffer
 * @param agps_handle Handle passed to gpal_init function
 * @return AGPS_OK on success, other return codes on error
 */
static agps_ReturnCode agps_CoreGpalSendUplinkMsg(agps_GpalProtocol protocol,
                                           const char *msg,
                                           unsigned int size,
                                           void *agps_handle)
{
    agps_ReturnCode res;
    agps_ReturnCode ret;

    switch (protocol)
    {
    case AGPS_PROTOCOL_RRC:
    case AGPS_PROTOCOL_RRLP:
        res = agps_FrameSend(protocol==AGPS_PROTOCOL_RRC ? AGPS_RRC_UL_DCCH : AGPS_RRLP_UL_RRLP,
                             msg,
                             size);
        if (res != AGPS_OK)
        {
            ALOGE("%s: Failed to send frame, error %d\n", __FUNCTION__, res);
            ret = -AGPS_ERROR_FRAME_SEND;
        }
        else
        {
            ret = AGPS_OK;
        }
        break;
    case AGPS_PROTOCOL_LPP:
    default:
        ret = -AGPS_ERROR_PROTOCOL_UNSUPPORTED;
        break;
    }
    return ret;
}

/**
 * Request to start calibration
 * @param agps_handle Handle passed to gpal_init function
 * @return AGPS_OK on success, other return codes on error
 */
agps_ReturnCode agps_CoreGpalCalibrationStart(void *agps_handle)
{
    agps_ReturnCode ret;

    do
    {
        ret = agps_FrameSend(AGPS_CALIBRATION_START_REQ,
                             NULL,
                             0);

        if (ret != AGPS_OK)
        {
            ALOGE("%s: Error code %d when sending AGPS_CALIBRATION_START_REQ \n", __FUNCTION__, ret);
            break;
        }

    } while (0);

    return ret;
}

/**
 * Request to stop calibration
 * @param agps_handle Handle passed to gpal_init function
 * @return AGPS_OK on success, other return codes on error
 */
agps_ReturnCode agps_CoreGpalCalibrationStop(void *agps_handle)
{
    agps_ReturnCode ret;

    do
    {
        ret = agps_FrameSend(AGPS_CALIBRATION_STOP_REQ,
                             NULL,
                             0);

        if (ret != AGPS_OK)
        {
            ALOGE("%s: Error code %d when sending AGPS_CALIBRATION_STOP_REQ \n", __FUNCTION__, ret);
            break;
        }

    } while (0);

    return ret;
}

/*************************************************************************************************
 * Public function definitions (Not doxygen commented, as they should be exported in the header
 * file)
 ************************************************************************************************/

void *agps_CoreMain(void *arg)
{
    agps_FrameConfig agps_frame_config;
    agps_ReturnCode res;
    agps_FramePayloadType payload_type;
    int bytes_received;
    void *gpal_lib_handle;
    const agps_GpalFunctions* (*gpal_create_fn)(void);
    void *agps_handle = NULL;

    /* copy config from argument */
    memcpy(&agps_core_state.config, (char*)arg, sizeof(agps_core_state.config) );

    /* initial values */
    agps_core_state.gpal_functions = NULL;

    /* fill in frame layer config items */
    agps_frame_config.device_path = agps_core_state.config.device_path;

    /* the following do while (0) is used to avoid nested if and return statements */
    do
    {
        /* initialize frame layer */
        res = agps_FrameInitialize(&agps_frame_config);
        if (res != AGPS_OK)
        {
            ALOGE("%s: Error %d while initializing frame layer \n", __FUNCTION__, res);
            break;
        }

        /* find test case handler (if a test case was provided) */
        if (agps_core_state.config.test_case != NULL)
        {
            /* initialize pseudo-GPAL */
            ALOGI("%s: test case %s\n", __FUNCTION__, agps_core_state.config.test_case);

            agps_core_state.gpal_functions = agps_GpalTestCreate(agps_core_state.config.test_case);
        }
        else
        {
            /* initialize GPAL */
            gpal_lib_handle = agps_PortOpenLib(agps_core_state.config.gpal_lib_path);
            if (NULL == gpal_lib_handle)
            {
                ALOGE("%s: failed to open GPAL lib (%s)\n", __FUNCTION__, agps_core_state.config.gpal_lib_path);
                break;
            }

            /* get GPAL functional interface */
            gpal_create_fn = (const agps_GpalFunctions *(*)(void))agps_PortGetLibSymbol(gpal_lib_handle, "agps_GpalCreate");
            if (NULL == gpal_create_fn)
            {
                ALOGE("%s: failed to retrieve pointer to agps_GpalCreate function\n", __FUNCTION__);
                break;
            }

            agps_core_state.gpal_functions = gpal_create_fn();

        }

        if (NULL == agps_core_state.gpal_functions)
        {
            ALOGE("%s: failed to retrieve GPAL functional interface\n", __FUNCTION__);
            break;
        }

        /* initialize GPAL */
        agps_core_state.gpal_handle = agps_core_state.gpal_functions->init(&agps_core_gpal_callbacks, NULL);
        if (NULL == agps_core_state.gpal_handle)
        {
            ALOGE("%s: failed to initialize GPAL\n", __FUNCTION__);
            break;
        }

        /* process all incoming frames */
        while (1)
        {
            /* get next frame */
            res = agps_FrameRecv(&payload_type, agps_core_buffer, AGPS_CORE_MAX_PAYLOAD_SIZE, &bytes_received );

            if (AGPS_OK != res)
            {
                ALOGE("%s: Error code %d when receiving frame \n", __FUNCTION__, res);
                break;
            }

            /* process frame */
            res = agps_CoreProcessRxFrame(payload_type, agps_core_buffer, bytes_received);

            if (res != AGPS_OK)
            {
                ALOGE("%s: Error code %d when processing frame \n", __FUNCTION__, res);
                break;
            }
        } /* end of while (1) loop */

    } while (0);

    /* un-initialize GPAL */
    if (agps_core_state.gpal_functions)
    {
        res = agps_core_state.gpal_functions->deinit(agps_core_state.gpal_handle);

        if (AGPS_OK != res)
        {
            ALOGE("%s: Error code %d when un-initializing GPAL \n", __FUNCTION__, res);
        }
    }

    /* un-initialize frame layer */
    agps_FrameUnitialize();

    /* send exit signal */
    agps_DaemonExit();

    return NULL;
}

