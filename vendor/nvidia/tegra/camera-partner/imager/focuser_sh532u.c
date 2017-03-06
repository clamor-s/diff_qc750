/*
 * Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
#if (BUILD_FOR_AOS == 0)

#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h> // must keep this header or break the build

#include "focuser_sh532u.h"
#include "imager_hal.h"
#include "imager_util.h"
#include "nvassert.h"

#define FOCUSER_DB_PRINTF(...)
//#define FOCUSER_DB_PRINTF(...)  NvOsDebugPrintf(__VA_ARGS__)

#define FOCUSER_POSITIONS 1000 // just hardcode the OMX->ODM intf to 1000 positions
#define FOCUSER_POSITION_UNINITIALIZED (-1)

#define TIME_DIFF(_from, _to) \
    (((_from) <= (_to)) ? ((_to) - (_from)) : \
     (~0UL - ((_from) - (_to)) + 1))

const float SH532U_FOCAL_LENGTH    = 4.42;
const float SH532U_FNUMBER         = 2.80;
const int   SH532U_ACTUATOR_RANGE  = 1000;
const int   SH532U_SETTLETIME      = 70;
const float HYPERFOCAL_RATIO       = 41.2f/224.4f; // Ratio source: SEMCO

static void Focuser_UpdateSettledPosition(FocuserCtxt *ctxt)
{
    enum sh532u_move_status status = SH532U_STATE_UNKNOWN;
    int ret = 0;
    NvU32 TimeElapse = 0;
    NvU32 CurrentTime = NvOdmOsGetTimeMS();

    // Settled position has been updated?
    if (ctxt->Position == ctxt->RequestedPosition)
        return;

    // I2C may fail this function, but it is understood as a harmless error at this point.
    ret = ioctl(ctxt->focuser_fd, SH532U_IOCTL_GET_MOVE_STATUS, &status);
    if (ret < 0) {
        FOCUSER_DB_PRINTF("Warning: ioctl failed to retrieve SH532U status: %s\n", strerror(errno));
    }

    CurrentTime = NvOdmOsGetTimeMS();
    TimeElapse = TIME_DIFF(ctxt->cmdTime, CurrentTime);

    if ((status == SH532U_LENS_SETTLED) || TimeElapse >= ctxt->ModuleParam.settle_time) {
        ctxt->Position = ctxt->RequestedPosition;
    }
}

/**
 * setPosition.
 *
 *                  INF           MAC
 * OMXPos            0        FOCUSER_POSITION
 * driverPos     POS_HIGH        POS_LOW
 */
static NvBool setPosition(FocuserCtxt *ctxt, NvU32 Position)
{
    int target = ctxt->ModuleParam.pos_high -
        ((Position * (ctxt->ModuleParam.pos_high - ctxt->ModuleParam.pos_low))
          / FOCUSER_POSITIONS);

    if (ioctl(ctxt->focuser_fd, SH532U_IOCTL_SET_POSITION, target) < 0) {
        FOCUSER_DB_PRINTF("%s: ioctl to set focus failed - %s\n",
                __FILE__,
                strerror(errno));
        return NV_FALSE;
    }

    ctxt->RequestedPosition = Position;
    ctxt->cmdTime = NvOdmOsGetTimeMS();
    ctxt->settleTime = SH532U_SETTLETIME;
    return NV_TRUE;
}

static void Focuser_Close(NvOdmImagerHandle hImager);

/**
 * Focuser_Open
 * Initialize Focuser's private context.
 */
static NvBool Focuser_Open(NvOdmImagerHandle hImager)
{
    FocuserCtxt *ctxt = NULL;

    FOCUSER_DB_PRINTF("Focuser_Open\n");

    if (!hImager || !hImager->pFocuser)
        return NV_FALSE;

    ctxt = NvOdmOsAlloc(sizeof(*ctxt));
    if (!ctxt)
        goto fail;
    hImager->pFocuser->pPrivateContext = ctxt;

    NvOdmOsMemset(ctxt, 0, sizeof(FocuserCtxt));


    ctxt->PowerLevel = NvOdmImagerPowerLevel_Off;
    ctxt->cmdTime = 0;
    ctxt->RequestedPosition = ctxt->Position = 0;
    ctxt->DelayedFPos = FOCUSER_POSITION_UNINITIALIZED;
    ctxt->ModuleParam.focal_length = SH532U_FOCAL_LENGTH;
    ctxt->ModuleParam.fnumber      = SH532U_FNUMBER;

    return NV_TRUE;

fail:

    FOCUSER_DB_PRINTF("Focuser_Open FAILED\n");
    Focuser_Close(hImager);
    return NV_FALSE;
}

/**
 * Focuser_Close
 * Free focuser's context and resources.
 */
static void Focuser_Close(NvOdmImagerHandle hImager)
{
    FocuserCtxt *pContext = NULL;

    FOCUSER_DB_PRINTF("Focuser_Close\n");

    if (hImager && hImager->pFocuser && hImager->pFocuser->pPrivateContext) {
        pContext = (FocuserCtxt *) hImager->pFocuser->pPrivateContext;
        close(pContext->focuser_fd);
        NvOdmOsFree(pContext);
        hImager->pFocuser->pPrivateContext = NULL;
    }
    else
        NV_ASSERT(!"Focuser_Close is called before Focuser is initialied!\n");

}

/**
 * Focuser_SetPowerLevel
 * Set the focuser's power level.
 */
static NvBool Focuser_SetPowerLevel(NvOdmImagerHandle hImager,
                                    NvOdmImagerPowerLevel PowerLevel)
{
    NvBool Status = NV_TRUE;
    FOCUSER_DB_PRINTF("Focuser_SetPowerLevel %d\n", PowerLevel);
    FocuserCtxt *fContext =
       (FocuserCtxt*)hImager->pFocuser->pPrivateContext;

    if (fContext->PowerLevel == PowerLevel)
        return NV_TRUE;

    switch(PowerLevel)
    {
        case NvOdmImagerPowerLevel_On:
            if (fContext->PowerLevel == NvOdmImagerPowerLevel_Standby) {
                NV_ASSERT(!"Focuser should not be re-opened after standby!\n");
                return NV_FALSE;
            }

            fContext->focuser_fd = open("/dev/sh532u", O_RDWR);

            if (fContext->focuser_fd <= 0) {
                FOCUSER_DB_PRINTF("Can not open focuser device: %s\n", strerror(errno));
                return NV_FALSE;
            }

            if (ioctl(fContext->focuser_fd, SH532U_IOCTL_GET_CONFIG,
                  &fContext->ModuleParam) < 0) {
                FOCUSER_DB_PRINTF("Can not open get focuser config: %s\n", strerror(errno));
                close(fContext->focuser_fd);
                fContext->focuser_fd = -1;
                return NV_FALSE;
            }

            // Hyperfocal_ratio is used to simplify calculating hyperfocal distance.
            fContext->ModuleParam.pos_high = fContext->ModuleParam.pos_high +
                (fContext->ModuleParam.pos_high - fContext->ModuleParam.pos_low) * HYPERFOCAL_RATIO;

            if (fContext->DelayedFPos != FOCUSER_POSITION_UNINITIALIZED)
            {
                Status = setPosition(fContext, fContext->DelayedFPos);
                fContext->DelayedFPos = FOCUSER_POSITION_UNINITIALIZED;
            }

            if (!Status)
                return NV_FALSE;
            break;

        case NvOdmImagerPowerLevel_Standby:
            if (fContext->PowerLevel == NvOdmImagerPowerLevel_Off) {
                NV_ASSERT(!"Focuser should not be standby after power-off!\n");
                return NV_FALSE;
            }
            break;

        case NvOdmImagerPowerLevel_Off:
            if (fContext->focuser_fd <= 0)  {
                NV_ASSERT(!"Invalid states when Focuser is powered off!\n");
                return NV_FALSE;
            }

            close(fContext->focuser_fd);
            fContext->focuser_fd = -1;

            break;

        default:
            NV_ASSERT(!"Unknown power level!\n");
            return NV_FALSE;
            break;
    }

    fContext->PowerLevel = PowerLevel;

    return NV_TRUE;
}

/**
 * Focuser_GetCapabilities
 * Get focuser's capabilities.
 */
static void Focuser_GetCapabilities(NvOdmImagerHandle hImager,
                                    NvOdmImagerCapabilities *pCapabilities)
{
    FOCUSER_DB_PRINTF("Focuser_GetCapabilities - stubbed\n");
}

/**
 * Focuser_SetParameter
 * Set focuser's parameter
 */
static NvBool Focuser_SetParameter(NvOdmImagerHandle hImager,
                                   NvOdmImagerParameter Param,
                                   NvS32 SizeOfValue, const void *pValue)
{
    NvBool Status = NV_FALSE;
    FocuserCtxt *pContext;

    FOCUSER_DB_PRINTF("Focuser_SetParameter (%d)\n", Param);
    if (!hImager || !hImager->pFocuser || !hImager->pFocuser->pPrivateContext) {
        NV_ASSERT(!"Focuser_SetParameter is called before Focuser is initialied!\n");
        return NV_FALSE;
    }

    pContext = (FocuserCtxt *) hImager->pFocuser->pPrivateContext;
    switch (Param)
    {
        case NvOdmImagerParameter_FocuserLocus:
        {
            FOCUSER_DB_PRINTF("Focuser_SetParameter NvOdmImagerParameter_FocuserLocus\n");
            NvU32 Position = *((NvU32 *) pValue);

            if (Position >= (unsigned int)FOCUSER_POSITIONS) {
                FOCUSER_DB_PRINTF("position %x %x out of bounds\n", Position, FOCUSER_POSITIONS);
                Status = NV_FALSE;
                break;
            }

            if (pContext->PowerLevel != NvOdmImagerPowerLevel_On) {
                FOCUSER_DB_PRINTF("imager power not on, delayed position %x\n", Position);
                pContext->DelayedFPos = (NvS32) Position;
                Status = NV_TRUE;
                break;
            }

            Status = setPosition(pContext, Position);
            break;
        }

       case NvOdmImagerParameter_StereoCameraMode:
        {
            FOCUSER_DB_PRINTF("Focuser: Set StereoCameraMode\n");
            if( ioctl(pContext->focuser_fd, SH532U_IOCTL_SET_CAMERA_MODE, *((NvOdmImagerStereoCameraMode *) pValue) ) < 0 )
            {
                FOCUSER_DB_PRINTF("Cannot set camera mode to focuse\n");
                Status = NV_FALSE;
            }
            else
                Status = NV_TRUE;
        }
            break;

        default:
            FOCUSER_DB_PRINTF("Unsupported param (%d)\n", Param);
    }
    return Status;
}

/**
 * Focuser_GetParameter
 * Get focuser's parameter
 */
static NvBool Focuser_GetParameter(NvOdmImagerHandle hImager,
                                   NvOdmImagerParameter Param,
                                   NvS32 SizeOfValue,
                                   void *pValue)
{
    NvF32 *pFValue = (NvF32*) pValue;
    NvU32 *pUValue = (NvU32*) pValue;
    FocuserCtxt *pContext;

    FOCUSER_DB_PRINTF("Focuser_GetParameter (%d)\n", Param);

    if (!hImager || !hImager->pFocuser || !hImager->pFocuser->pPrivateContext) {
        NV_ASSERT(!"Focuser_GetParameter is called before Focuser is initialied!\n");
        return NV_FALSE;
    }

    pContext = (FocuserCtxt*)hImager->pFocuser->pPrivateContext;
    switch(Param) {
        case NvOdmImagerParameter_FocuserLocus:
            // The last requested position has been settled?
            Focuser_UpdateSettledPosition(pContext);
            *pUValue = pContext->Position;
            FOCUSER_DB_PRINTF("position = %d\n", *pUValue);
        break;

        case NvOdmImagerParameter_FocalLength:
            *pFValue = pContext->ModuleParam.focal_length;
            FOCUSER_DB_PRINTF("focal length = %f\n", *pFValue);
        break;

        case NvOdmImagerParameter_MaxAperture:
            *pFValue = pContext->ModuleParam.focal_length / pContext->ModuleParam.fnumber;
            FOCUSER_DB_PRINTF("max aperture = %f\n", *pFValue);
        break;

        case NvOdmImagerParameter_FNumber:
            *pFValue = pContext->ModuleParam.fnumber;
            FOCUSER_DB_PRINTF("fnumber = %f\n", *pFValue);
        break;

        case NvOdmImagerParameter_FocuserCapabilities:
        {
            NvOdmImagerFocuserCapabilities *caps =
                    (NvOdmImagerFocuserCapabilities *) pValue;

            if (SizeOfValue != (NvS32) sizeof(NvOdmImagerFocuserCapabilities)) {
                FOCUSER_DB_PRINTF("Bad size\n");
                return NV_FALSE ;
            }

            NvOdmOsMemset(caps, 0, sizeof(NvOdmImagerFocuserCapabilities));
            caps->actuatorRange     = SH532U_ACTUATOR_RANGE;
            caps->settleTime     = SH532U_SETTLETIME;
            FOCUSER_DB_PRINTF("range = %d, time = %d\n", caps->actuatorRange, caps->settleTime);
        }
        break;

        default:
            FOCUSER_DB_PRINTF("Unsupported param (%d)\n", Param);
            return NV_FALSE;
    }

    return NV_TRUE;
}

/**
 * FocuserSH532U_GetHal.
 * return the hal functions associated with focuser
 */
NvBool FocuserSH532U_GetHal(NvOdmImagerHandle hImager)
{
    if (!hImager || !hImager->pFocuser)
        return NV_FALSE;

    hImager->pFocuser->pfnOpen = Focuser_Open;
    hImager->pFocuser->pfnClose = Focuser_Close;
    hImager->pFocuser->pfnGetCapabilities = Focuser_GetCapabilities;
    hImager->pFocuser->pfnSetPowerLevel = Focuser_SetPowerLevel;
    hImager->pFocuser->pfnSetParameter = Focuser_SetParameter;
    hImager->pFocuser->pfnGetParameter = Focuser_GetParameter;

    return NV_TRUE;
}
#endif
