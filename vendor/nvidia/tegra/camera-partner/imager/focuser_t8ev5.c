/*
 * Copyright (c) 2010-2011 NVIDIA Corporation.  All rights reserved.
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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#endif

#include "focuser_t8ev5.h"
#include "imager_hal.h"
#include "imager_util.h"
#include "nvassert.h"

//#define FOCUSER_DB_PRINTF(...)
#define FOCUSER_DB_PRINTF(...)  NvOsDebugPrintf(__VA_ARGS__)

#define FOCUSER_POSITIONS(ctx) ((ctx)->Config.pos_high - (ctx)->Config.pos_low)
#define GRAVITY_FUDGE(x) (85 * (x) / 100)

#define TIME_DIFF(_from, _to) \
    (((_from) <= (_to)) ? ((_to) - (_from)) : \
     (~0UL - ((_from) - (_to)) + 1))

/**
 * Focuser's context
 */
typedef struct
{
    int focuser_fd;
    NvOdmImagerPowerLevel PowerLevel;
    NvU32 cmdTime;             /* time of latest focus command issued */
    NvU32 settleTime;          /* time to wait for latest seek */
    NvU32 Position;            /* The last settled focus position. */
    NvU32 RequestedPosition;   /* The last requested focus position. */
    NvS32 DelayedFPos;         /* Focusing position for delayed request */
    struct t8ev5_config Config;
} FocuserCtxt;

static void Focuser_UpdateSettledPosition(FocuserCtxt *ctxt)
{
    NvU32 CurrentTime = 0;

    /*  Settled position has been updated? */
    if (ctxt->Position == ctxt->RequestedPosition)
        return;

    CurrentTime = NvOdmOsGetTimeMS();

    /*  Previous requested position settled? */
    if (TIME_DIFF(ctxt->cmdTime, CurrentTime) >= ctxt->settleTime)
        ctxt->Position = ctxt->RequestedPosition;
}

/**
 * setPosition.
 */
static NvBool setPosition(FocuserCtxt *ctxt, NvU32 Position)
{
#if (BUILD_FOR_AOS == 0)
    if (ioctl(ctxt->focuser_fd, T8EV5_IOCTL_SET_POSITION, Position + ctxt->Config.pos_low) < 0)
    {
        NvOsDebugPrintf("%s: ioctl to set focus failed - %s\n",
                __FILE__,
                strerror(errno));
        return NV_FALSE;
    }

    ctxt->RequestedPosition = Position;
    ctxt->cmdTime = NvOdmOsGetTimeMS();
    ctxt->settleTime = ctxt->Config.settle_time;
#endif
    return NV_TRUE;
}

static void FocuserT8ev5_Close(NvOdmImagerHandle hImager);

/**
 * Focuser_Open
 * Initialize Focuser's private context.
 */
static NvBool FocuserT8ev5_Open(NvOdmImagerHandle hImager)
{
#if (BUILD_FOR_AOS == 0)

    FocuserCtxt *ctxt = NULL;

    NvOsDebugPrintf("FocuserT8ev5_Open\n");

    if (!hImager || !hImager->pFocuser)
        return NV_FALSE;

    ctxt = NvOdmOsAlloc(sizeof(*ctxt));
    if (!ctxt)
        goto fail;
    hImager->pFocuser->pPrivateContext = ctxt;

    NvOdmOsMemset(ctxt, 0, sizeof(FocuserCtxt));

    /*
      The file descriptor manipulation is done now in _Open/_Close
      instead of _SetPowerLevel, because _GetParameter queries info
      available only from the sensor driver (i.e. not hardcoded here).
     */

    ctxt->focuser_fd = open("/dev/t8ev5_focuser", O_RDWR);
    if (ctxt->focuser_fd < 0)
    {
        NvOsDebugPrintf("Can not open focuser device: %s\n", strerror(errno));
        return NV_FALSE;
    }

    if (ioctl(ctxt->focuser_fd, T8EV5_IOCTL_GET_CONFIG, &ctxt->Config) < 0)
    {
        NvOsDebugPrintf("Can not open get focuser config: %s\n", strerror(errno));
        close(ctxt->focuser_fd);
        ctxt->focuser_fd = -1;
        return NV_FALSE;
    }

    ctxt->PowerLevel = NvOdmImagerPowerLevel_Off;
    ctxt->cmdTime = 0;
    ctxt->RequestedPosition = ctxt->Position = 0;
    ctxt->DelayedFPos = -1;

    return NV_TRUE;

fail:
    NvOsDebugPrintf("FocuserT8ev5_Open FAILED\n");
    FocuserT8ev5_Close(hImager);
    return NV_FALSE;
#else
	return NV_TRUE;
#endif
}

/**
 * Focuser_Close
 * Free focuser's context and resources.
 */
static void FocuserT8ev5_Close(NvOdmImagerHandle hImager)
{
    FocuserCtxt *pContext = NULL;

    NvOsDebugPrintf("Focuser_Close\n");

    if (hImager && hImager->pFocuser && hImager->pFocuser->pPrivateContext)
    {
        pContext = (FocuserCtxt *) hImager->pFocuser->pPrivateContext;
#if (BUILD_FOR_AOS == 0)
        close(pContext->focuser_fd);
#endif
        NvOdmOsFree(pContext);
        hImager->pFocuser->pPrivateContext = NULL;
    }
}

/**
 * Focuser_SetPowerLevel
 * Set the focuser's power level.
 */
static NvBool FocuserT8ev5_SetPowerLevel(NvOdmImagerHandle hImager,
                NvOdmImagerPowerLevel PowerLevel)
{
    FocuserCtxt *pContext =
        (FocuserCtxt *) hImager->pFocuser->pPrivateContext;
    NvBool Status = NV_TRUE;
    NvS32 dPos;

    FOCUSER_DB_PRINTF("Focuser_SetPowerLevel (%d)\n", PowerLevel);

    if (pContext->PowerLevel == PowerLevel)
        return NV_TRUE;

    switch(PowerLevel)
    {
        case NvOdmImagerPowerLevel_On:

            /* Check for delayed focusing request: */
            dPos = pContext->DelayedFPos;
            if (dPos >= 0 && (NvU32) dPos < FOCUSER_POSITIONS(pContext))
            {
                Status = setPosition(pContext, dPos);
                pContext->DelayedFPos = -1;
            }
            break;
        case NvOdmImagerPowerLevel_Off:
            break;
        default:
            NvOsDebugPrintf("Focuser taking power level %d\n", PowerLevel);
            break;
    }

    pContext->PowerLevel = PowerLevel;
    return Status;
}

/**
 * Focuser_GetCapabilities
 * Get focuser's capabilities.
 */
static void FocuserT8ev5_GetCapabilities(NvOdmImagerHandle hImager,
            NvOdmImagerCapabilities *pCapabilities)
{
    FOCUSER_DB_PRINTF("Focuser_GetCapabilities - stubbed\n");
}

/**
 * Focuser_SetParameter
 * Set focuser's parameter
 */
static NvBool FocuserT8ev5_SetParameter(NvOdmImagerHandle hImager,
            NvOdmImagerParameter Param,
            NvS32 SizeOfValue, const void *pValue)
{
    NvBool Status = NV_FALSE;
    FocuserCtxt *pContext = (FocuserCtxt *) hImager->pFocuser->pPrivateContext;
    NvU32 Position;

    FOCUSER_DB_PRINTF("Focuser_SetParameter (%d)\n", Param);

    switch (Param)
    {
        case NvOdmImagerParameter_FocuserLocus:
        {
            Position = *((NvU32 *) pValue);
            FOCUSER_DB_PRINTF("FocuserLocus %d\n", Position);

            if (Position > FOCUSER_POSITIONS(pContext))
            {
                NvOsDebugPrintf("position out of bounds\n");
                break;
            }

            if (pContext->PowerLevel != NvOdmImagerPowerLevel_On)
            {
                pContext->DelayedFPos = (NvS32) Position;
                Status = NV_TRUE;
                break;
            }

            Status = setPosition(pContext, Position);
            break;
        }
        default:
            FOCUSER_DB_PRINTF("Unsupported param (%d)\n", Param);
    }
    return Status;
}

/**
 * Focuser_GetParameter
 * Get focuser's parameter
 */
static NvBool FocuserT8ev5_GetParameter(NvOdmImagerHandle hImager,
                NvOdmImagerParameter Param,
                NvS32 SizeOfValue,
                void *pValue)
{
    NvF32 *pFValue = (NvF32*) pValue;
    NvU32 *pUValue = (NvU32*) pValue;
    FocuserCtxt *pContext = (FocuserCtxt *) hImager->pFocuser->pPrivateContext;

    FOCUSER_DB_PRINTF("Focuser_GetParameter (%d)\n", Param);

    switch(Param) {
        case NvOdmImagerParameter_FocuserLocus:
            /* The last requested position has been settled? */
            Focuser_UpdateSettledPosition(pContext);
            *pUValue = pContext->Position;
            FOCUSER_DB_PRINTF("position = %d\n", *pUValue);
            break;

        case NvOdmImagerParameter_FocalLength:
            *pFValue = pContext->Config.focal_length;
            FOCUSER_DB_PRINTF("focal length = %f\n", *pFValue);
            break;

        case NvOdmImagerParameter_MaxAperture:
            *pFValue = pContext->Config.focal_length / pContext->Config.fnumber;
            FOCUSER_DB_PRINTF("max aperture = %f\n", *pFValue);
            break;

        case NvOdmImagerParameter_FNumber:
            *pFValue = pContext->Config.fnumber;
            FOCUSER_DB_PRINTF("fnumber = %f\n", *pFValue);
            break;

        case NvOdmImagerParameter_FocuserCapabilities:
        {
            NvOdmImagerFocuserCapabilities *caps =
            (NvOdmImagerFocuserCapabilities *) pValue;

            if (SizeOfValue != (NvS32) sizeof(NvOdmImagerFocuserCapabilities))
            {
                NvOsDebugPrintf("Bad size\n");
                return NV_FALSE;
            }

            NvOdmOsMemset(caps, 0, sizeof(NvOdmImagerFocuserCapabilities));
            caps->actuatorRange = FOCUSER_POSITIONS(pContext);
            caps->settleTime = pContext->Config.settle_time;
            FOCUSER_DB_PRINTF("range = %d, time = %d\n", caps->actuatorRange, caps->settleTime);
            break;
        }

        default:
            FOCUSER_DB_PRINTF("Unsupported param (%d)\n", Param);
            return NV_FALSE;
    }

    return NV_TRUE;
}

/**
 * Focuser_T8ev5_GetHal.
 * return the hal functions associated with focuser
 */
NvBool Focuser_T8ev5_GetHal(NvOdmImagerHandle hImager)
{
    if (!hImager || !hImager->pFocuser)
        return NV_FALSE;

    hImager->pFocuser->pfnOpen = FocuserT8ev5_Open;
    hImager->pFocuser->pfnClose = FocuserT8ev5_Close;
    hImager->pFocuser->pfnGetCapabilities = FocuserT8ev5_GetCapabilities;
    hImager->pFocuser->pfnSetPowerLevel = FocuserT8ev5_SetPowerLevel;
    hImager->pFocuser->pfnSetParameter = FocuserT8ev5_SetParameter;
    hImager->pFocuser->pfnGetParameter = FocuserT8ev5_GetParameter;

    return NV_TRUE;
}
