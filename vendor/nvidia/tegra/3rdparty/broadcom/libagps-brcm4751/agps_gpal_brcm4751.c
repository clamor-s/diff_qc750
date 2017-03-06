/*
# Copyright (c) 2011, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*************************************************************************************************
 * $Revision: #1 $
 * $Date: $
 * $Author: $
 ************************************************************************************************/

/**
 * @addtogroup agps
 * @{
 */

/**
 * @file agrild_gpal_brcm4751.c GPS BRCM4751 adaptation layer
 *
 */

/*************************************************************************************************
 * Project header files. The first in this list should always be the public interface for this
 * file. This ensures that the public interface header file compiles standalone.
 ************************************************************************************************/

#include "agps.h"
#include "agps_gpal.h"
#include "ril_asn1.h"

/*************************************************************************************************
 * Standard header files (e.g. <string.h>, <stdlib.h> etc.
 ************************************************************************************************/

/**************************************************************************************************************/
/* Private Macros  */
/**************************************************************************************************************/

/**************************************************************************************************************/
/* Private Functions Declarations (if needed) */
/**************************************************************************************************************/

static void *agps_GpalBrcm4751Init(const agps_GpalCallbacks *callbacks, void *agps_handle);

static agps_ReturnCode agps_GpalBrcm4751SendDownlinkMsg(void *gps_handle,
                                         agps_GpalProtocol protocol,
                                         const char *msg,
                                         unsigned int size);

static agps_ReturnCode agps_GpalBrcm4751SetUeState(void *gps_handle,
                                    agps_GpalProtocol protocol,
                                    agps_GpalUeState state);

static agps_ReturnCode agps_GpalBrcm4751ResetAssistanceData(void *gps_handle);

static agps_ReturnCode agps_GpalBrcm4751DeInit(void *gps_handle);

static BrcmLbs_Result brcm_OnSendToNetwork(BrcmLbsRilAsn1_Protocol protocol, const unsigned char *msg, size_t size, BrcmLbs_UserData userData);

static agps_ReturnCode agps_GpalBrcm4751SetCalibrationStatus(void *gps_handle, int locked);

static BrcmLbs_Result brcm_OnCalibrationStart(BrcmLbs_UserData userData);

static BrcmLbs_Result brcm_OnCalibrationEnd(BrcmLbs_UserData userData);

/**************************************************************************************************************/
/* Private Type Declarations */
/**************************************************************************************************************/

/** type definition for the state of the GPAL for BRCM7451 */
typedef struct
{
    void *agps_handle;
    OsHandle brcm_handle;
    const agps_GpalCallbacks *callbacks;
} agps_GpalBrcm4751State;

/**************************************************************************************************************/
/* Private Variable Definitions  */
/**************************************************************************************************************/

/**
 * GPAL->aGPS callbacks
 */
static const agps_GpalFunctions gpal_brcm4751_functions = {
    agps_GpalBrcm4751Init,
    agps_GpalBrcm4751SendDownlinkMsg,
    agps_GpalBrcm4751SetUeState,
    agps_GpalBrcm4751ResetAssistanceData,
    agps_GpalBrcm4751SetCalibrationStatus,
    agps_GpalBrcm4751DeInit,
};

/**
 * BRCM->GPAL callbacks
 */
static BrcmLbsRilAsn1_Callbacks gpal_brcm4751_callbacks = {
    brcm_OnSendToNetwork,
};

/**
 * BRCM->GPAL calibration callbacks
 */
static BrcmLbsRilCntin_Callbacks gpal_brcm4751_cal_callbacks = {
    brcm_OnCalibrationStart,
    brcm_OnCalibrationEnd,
};

/**
 * GPAL state
 */
static agps_GpalBrcm4751State gpal_brcm4751_state;


/*************************************************************************************************
 * Private function definitions
 ************************************************************************************************/

/**
 * NVidia -> Broadcom protocol code conversion
 * @param protocol The NVidia protocol code
 * @return the Broadcom protocol code
 */
static BrcmLbsRilAsn1_Protocol agps2brcmProtocol(agps_GpalProtocol protocol)
{
    BrcmLbsRilAsn1_Protocol ret;

    switch (protocol)
    {
    case AGPS_PROTOCOL_RRLP:
        ret = BRCM_LBS_RIL_RRLP;
        break;
    case AGPS_PROTOCOL_RRC:
        ret = BRCM_LBS_RIL_RRC;
        break;
    case AGPS_PROTOCOL_LPP:
    default:
        ret = BRCM_LBS_RIL_LTE;
        break;
    }

    return ret;

}

/**
 * Broadcom->NVidia protocol code conversion
 * @param protocol The Broadcom protocol code
 * @return the NVidia protocol code
 */
static agps_GpalProtocol brcm2agpsProtocol(BrcmLbsRilAsn1_Protocol protocol)
{
    agps_GpalProtocol ret;

    switch (protocol)
    {
    case BRCM_LBS_RIL_RRLP:
        ret = AGPS_PROTOCOL_RRLP;
        break;
    case BRCM_LBS_RIL_RRC:
        ret = AGPS_PROTOCOL_RRC;
        break;
    case BRCM_LBS_RIL_LTE:
    default:
        ret = AGPS_PROTOCOL_LPP;
        break;
    }

    return ret;

}

/**
 * Initialise GPAL. This function is called upon initialization
 * of the aGPS daemon. This calls into the BRCM init function to
 * provide GPS->GPAL callbacks. The aGPS handle is used for
 * GPAL->aGPS function calls.
 * @param callbacks pointer to callback structure
 * @param agps_handle Handle to be used when calling callbacks
 * @return GPAL handle
 */
static void *agps_GpalBrcm4751Init(const agps_GpalCallbacks *callbacks, void *agps_handle)
{
    OsHandle res;
    void *ret = NULL;

    gpal_brcm4751_state.callbacks = callbacks;
    gpal_brcm4751_state.agps_handle = agps_handle;

    res = BrcmLbsRilAsn1_init(&gpal_brcm4751_callbacks, (BrcmLbs_UserData)&gpal_brcm4751_state);

    ALOGI("%s: Initialized BRCM lib, handle=0x%08x\n", __FUNCTION__, (unsigned int)res);

    if (res != OS_HANDLE_INVALID)
    {
        ret = (void*)&gpal_brcm4751_state;
        gpal_brcm4751_state.brcm_handle = res;

        BrcmLbsRilCntin_init(&gpal_brcm4751_cal_callbacks, (BrcmLbs_UserData)&gpal_brcm4751_state);
    }

    return ret;
}

static void dumpBuffer(const char *prefix, const char *buffer, int len)
{
    int i;
    const int line_width = 64;

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
                sprintf(tmpbuf2,"%02x", buffer[i+j]);
                strcat(tmpbuf,tmpbuf2);
            }
        }
        ALOGD("%s %s\n", prefix, tmpbuf);
        i+=line_width;
    }
}

/**
 * Send ASN1-encoded message to GPS.
 * @param gpal_handle GPAL handle (returned by
 *                    agps_GpalBrcm4751Init() )
 * @param protocol Protocol used to encode ASN1 message (RRC,
 * RRLP, ...)
 * @param msg Pointer to buffer that contains message
 * @param size Size of buffer pointed to by msg
 * @return Error code
 */
static agps_ReturnCode agps_GpalBrcm4751SendDownlinkMsg(void *gpal_handle,
                                         agps_GpalProtocol protocol,
                                         const char *msg,
                                         unsigned int size)
{
    BrcmLbs_Result res;
    agps_ReturnCode ret;
    unsigned char *tmp;

    ALOGD("%s: Sending downlink msg to brcm - protocol=%d size=%d\n", __FUNCTION__, (int)protocol, size);

    res = BrcmLbsRilAsn1_sendToGps(gpal_brcm4751_state.brcm_handle,
                                   agps2brcmProtocol(protocol),
                                   (const unsigned char*)msg,
                                   size);

    if (BRCM_LBS_OK != res)
    {
        ALOGE("%s: Error %d when sending downlink msg to BRCM GPS\n", __FUNCTION__, res);
        ret = -AGPS_ERROR_GPAL_DOWNLINK_MSG;
    }
    else
    {
	    ret = AGPS_OK;
    }

    return ret;
}

/**
 * Let GPS know about UE state
 * @param gpal_handle GPAL handle (returned by
 *                    agps_GpalBrcm4751Init() )
 * @param protocol Protocol (RRC, RRLP, ...)
 * @param state State
 * @return error code
 */
static agps_ReturnCode agps_GpalBrcm4751SetUeState(void *gpal_handle,
                                    agps_GpalProtocol protocol,
                                    agps_GpalUeState state)
{
    return AGPS_OK;
}

/**
 * Reset assistance data
 * @param gpal_handle GPAL handle (returned by
 *                    agps_GpalBrcm4751Init() )
 * @return error code
 */
static agps_ReturnCode agps_GpalBrcm4751ResetAssistanceData(void *gpal_handle)
{
    BrcmLbs_Result res;
    agps_ReturnCode ret;

    ALOGV("%s: Resetting BRCM assistance data\n", __FUNCTION__);

    res = BrcmLbsRilAsn1_resetAssistanceData(gpal_brcm4751_state.brcm_handle,
                                             agps2brcmProtocol(AGPS_PROTOCOL_RRC) );

    if (BRCM_LBS_OK != res)
    {
        ALOGE("%s: Error %d when resetting GPS assistance data\n", __FUNCTION__, res);
        ret = -AGPS_ERROR_GPAL_RESET_ASSISTANCE_DATA;
    }
    else
    {
        ret = AGPS_OK;
    }

    return ret;
}

/**
 * Set calibration status
 *
 * @param gpal_handle GPAL handle (returned by
 *                    agps_GpalBrcm4751Init() )
 * @param locked Flag to indicate whether reference clock is
 *               stable
 * @return error code
 */
static agps_ReturnCode agps_GpalBrcm4751SetCalibrationStatus(void *gpal_handle, int locked)
{
    BrcmLbs_Result res;
    agps_ReturnCode ret;

    res = BrcmLbsRilCntin_CalibrationStatus(gpal_brcm4751_state.brcm_handle, locked);

    if (BRCM_LBS_OK != res)
    {
        ALOGE("%s: Error %d when setting calibration status\n", __FUNCTION__, res);
        ret = -AGPS_ERROR_GPAL_SET_CALIBRATION_STATUS;
    }
    else
    {
        ret = AGPS_OK;
    }

    return ret;
}

/**
 * Unitialize GPAL
 * @param gpal_handle GPAL handle (returned by
 *                    agps_GpalBrcm4751Init() )
 * @return error code
 */
static agps_ReturnCode agps_GpalBrcm4751DeInit(void *gpal_handle)
{
    BrcmLbs_Result res;
    agps_ReturnCode ret;

    res = BrcmLbsRilAsn1_deinit(gpal_brcm4751_state.brcm_handle);

    if (BRCM_LBS_OK == res)
    {
        res = BrcmLbsRilCntin_deinit(gpal_brcm4751_state.brcm_handle);
    }

    if (BRCM_LBS_OK != res)
    {
        ALOGE("%s: Error %d when uninitializing BRCM aGPS lib\n", __FUNCTION__, res);
        ret = -AGPS_ERROR_GPAL_DEINIT;
    }
    else
    {
        ret = AGPS_OK;
    }

    return ret;
}

/**
 * Called by GPS to send ASN1-encoded message to network
 * @param gpal_handle GPAL handle (returned by
 *                    agps_GpalBrcm4751Init() )
 * @param protocol Protocol used to encode ASN1 message (RRC,
 * RRLP, ...)
 * @param msg Pointer to buffer that contains message
 * @param size Size of buffer pointed to by msg
 * @param userData GPAL handle
 * @return error code
 */
static BrcmLbs_Result brcm_OnSendToNetwork(BrcmLbsRilAsn1_Protocol protocol,
                                           const unsigned char *msg,
                                           size_t size,
                                           BrcmLbs_UserData userData)
{
    BrcmLbs_Result ret;
    agps_ReturnCode result;
    agps_GpalProtocol agps_protocol;

    agps_protocol = brcm2agpsProtocol(protocol);

    ALOGD("%s: Sending uplink msg to aGPS - protocol=%d size=%d\n", __FUNCTION__, agps_protocol, size);

    result = gpal_brcm4751_state.callbacks->send_uplink_msg(agps_protocol,
                                                         (const char*)msg,
                                                         size,
                                                         gpal_brcm4751_state.agps_handle);

    if (AGPS_OK != result)
    {
        ALOGE("%s: Error %d when sending uplink msg to aGPS\n", __FUNCTION__, result);
        ret = BRCM_LBS_ERROR_FAILED;
    }
    else
    {
        ret = BRCM_LBS_OK;
    }

    return ret;
}

/**
 * Called by GPS to start calibration
 */
static BrcmLbs_Result brcm_OnCalibrationStart(BrcmLbs_UserData userData)
{
    agps_ReturnCode res;
    BrcmLbs_Result ret;

    ALOGD("%s: Calibration start\n", __FUNCTION__);

    res = gpal_brcm4751_state.callbacks->calibration_start(gpal_brcm4751_state.agps_handle);

    if (AGPS_OK != res)
    {
        ALOGE("%s: Error %d when starting calibration\n", __FUNCTION__, res);
        ret = BRCM_LBS_ERROR_FAILED;
    }
    else
    {
        ret = BRCM_LBS_OK;
    }

    return ret;
}

/**
 * Called by GPS to stop calibration
 */
static BrcmLbs_Result brcm_OnCalibrationEnd(BrcmLbs_UserData userData)
{
    agps_ReturnCode res;
    BrcmLbs_Result ret;

    ALOGD("%s: Calibration stop\n", __FUNCTION__);

    res = gpal_brcm4751_state.callbacks->calibration_stop(gpal_brcm4751_state.agps_handle);

    if (AGPS_OK != res)
    {
        ALOGE("%s: Error %d when stopping calibration\n", __FUNCTION__, res);
        ret = BRCM_LBS_ERROR_FAILED;
    }
    else
    {
        ret = BRCM_LBS_OK;
    }

    return ret;
}

/*************************************************************************************************
 * Public function definitions (Not doxygen commented, as they should be exported in the header
 * file)
 ************************************************************************************************/

const agps_GpalFunctions *agps_GpalCreate(void)
{
    return &gpal_brcm4751_functions;
}

