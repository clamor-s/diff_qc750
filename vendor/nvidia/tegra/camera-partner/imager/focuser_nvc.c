/*
 * Copyright (c) 2011-2012, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
#if (BUILD_FOR_AOS == 0)
#define NV_ENABLE_DEBUG_PRINTS 0

#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <asm/types.h>
#include <nvc.h>
#include <nvc_focus.h>

#include "focuser_nvc.h"
#include "imager_hal.h"
#include "imager_util.h"
#include "focuser_common.h"


typedef struct FocuserNvcContextRec
{
    int focus_fd; /* file handle to the kernel nvc driver */
    struct nv_focuser_config Config;
} FocuserNvcContext;

static void SetFocuserCapabilities(FocuserNvcContext *pContext,
                        NvOdmImagerFocuserCapabilities *pCaps);

static NvBool
FocuserNvc_Open(
    NvOdmImagerHandle hImager)
{
    FocuserNvcContext *pCtxt;
    unsigned Instance;
    char DevName[NVODMIMAGER_IDENTIFIER_MAX];

    if (!hImager || !hImager->pFocuser)
    {
        NvOsDebugPrintf("%s: No hImager->pFocuser\n",
                        __func__);
        return NV_FALSE;
    }

    pCtxt = (FocuserNvcContext *)NvOdmOsAlloc(sizeof(*pCtxt));
    if (!pCtxt)
    {
        NvOsDebugPrintf("%s: Memory allocation failed\n", __func__);
        return NV_FALSE;
    }

    Instance = (unsigned)(hImager->pFocuser->GUID & 0xF);
    if (Instance)
        sprintf(DevName, "/dev/focuser.%u", Instance);
    else
        sprintf(DevName, "/dev/focuser");
    pCtxt->focus_fd = open(DevName, O_RDWR);
    if (pCtxt->focus_fd < 0)
    {
        NvOdmOsFree(pCtxt);
        hImager->pFocuser->pPrivateContext = NULL;
        NvOsDebugPrintf("%s: cannot open focuser driver: %s Err: %s\n",
                        __func__, DevName, strerror(errno));
        return NV_FALSE;
    }

    hImager->pFocuser->pPrivateContext = pCtxt;
    NvOsDebugPrintf("%s: %s driver focus_fd opened as: %d\n",
                    __func__, DevName, pCtxt->focus_fd);
    return NV_TRUE;
}

static void
FocuserNvc_Close(
    NvOdmImagerHandle hImager)
{
    FocuserNvcContext *pCtxt;

    if (!hImager || !hImager->pFocuser || !hImager->pFocuser->pPrivateContext)
    {
        NvOsDebugPrintf("%s: No hImager->pFocuser->pPrivateContext\n",
                        __func__);
        return;
    }

    pCtxt = (FocuserNvcContext *)hImager->pFocuser->pPrivateContext;
    close(pCtxt->focus_fd);
    NvOdmOsFree(pCtxt);
    hImager->pFocuser->pPrivateContext = NULL;
}

static NvBool
FocuserNvc_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel)
{
    FocuserNvcContext *pCtxt;
    int err;

    if (!hImager || !hImager->pFocuser || !hImager->pFocuser->pPrivateContext)
    {
        NvOsDebugPrintf("%s: No hImager->pFocuser->pPrivateContext\n",
                        __func__);
        return NV_FALSE;
    }

    pCtxt = (FocuserNvcContext *)hImager->pFocuser->pPrivateContext;
    err = ioctl(pCtxt->focus_fd, NVC_IOCTL_PWR_WR, PowerLevel);
    if (err < 0)
    {
        NvOsDebugPrintf("%s: ioctl to set power level (%d) failed: %s\n",
                        __func__, PowerLevel, strerror(errno));
        return NV_FALSE;
    }

    return NV_TRUE;
}

static void
FocuserNvc_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities)
{
}

static NvBool
FocuserNvc_SetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    const void *pValue)
{
    FocuserNvcContext *pCtxt;
    struct nvc_param params;
    int err;

    if (!hImager || !hImager->pFocuser || !hImager->pFocuser->pPrivateContext)
    {
        NvOsDebugPrintf("%s: No hImager->pFocuser->pPrivateContext\n",
                        __func__);
        return NV_FALSE;
    }

    pCtxt = (FocuserNvcContext *)hImager->pFocuser->pPrivateContext;
    params.param = Param;
    params.sizeofvalue = SizeOfValue;
    params.p_value = (void *)pValue;
    switch(Param) {
        case NvOdmImagerParameter_FocuserLocus:
            params.param = NVC_PARAM_LOCUS;
            break;

        case NvOdmImagerParameter_FocuserStereo:
        case NvOdmImagerParameter_StereoCameraMode:
            params.param = NVC_PARAM_STEREO;
            break;

        case NvOdmImagerParameter_Reset:
            params.param = NVC_PARAM_RESET;
            break;

        case NvOdmImagerParameter_SelfTest:
            params.param = NVC_PARAM_SELF_TEST;
            break;

        case NvOdmImagerParameter_FocuserCapabilities:
            SetFocuserCapabilities(pCtxt, (NvOdmImagerFocuserCapabilities *)pValue);
            params.param = NVC_PARAM_CAPS;
            params.sizeofvalue = sizeof(pCtxt->Config);
            params.p_value = (void *) &pCtxt->Config;
            break;

        default:
            NvOsDebugPrintf("%s: ioctl default case\n", __func__);
            break;
    }

    err = ioctl(pCtxt->focus_fd, NVC_IOCTL_PARAM_WR, &params);
    if (err < 0)
    {
        NvOsDebugPrintf("%s: ioctl to set parameter failed: %s\n",
                        __func__, strerror(errno));
        return NV_FALSE;
    }

    return NV_TRUE;
}


static void GetFocuserCapabilities(FocuserNvcContext *pContext,
                                NvOdmImagerFocuserCapabilities *pCaps)
{
    NvU32 i, j;

    memset(pCaps, 0, sizeof(*pCaps));
    pCaps->version = NVODMIMAGER_AF_FOCUSER_CODE_VERSION;
    pCaps->settleTime = pContext->Config.settle_time;

    pCaps->rangeEndsReversed = pContext->Config.range_ends_reversed;
    pCaps->positionWorkingLow = pContext->Config.pos_working_low;
    pCaps->positionWorkingHigh = pContext->Config.pos_working_high;

    pCaps->positionActualLow = pContext->Config.pos_actual_low;
    pCaps->positionActualHigh = pContext->Config.pos_actual_high;

    pCaps->slewRate = pContext->Config.slew_rate;
    pCaps->circleOfConfusion = pContext->Config.circle_of_confusion;

    NV_DEBUG_PRINTF(("%s: pCaps->afConfigSetSize %d",
                __FUNCTION__, pCaps->afConfigSetSize));
    NV_DEBUG_PRINTF(("%s: posActualLow %d high %d positionWorkingLow %d"
                    " positionWorkingHigh %d", __FUNCTION__,
                    pCaps->positionActualLow,
                    pCaps->positionActualHigh,
                    pCaps->positionWorkingLow,
                    pCaps->positionWorkingHigh));

    for (i = 0; i < pContext->Config.num_focuser_sets; i++)
    {
        pCaps->afConfigSet[i].posture =
                                pContext->Config.focuser_set[i].posture;
        pCaps->afConfigSet[i].macro =
                                pContext->Config.focuser_set[i].macro;
        pCaps->afConfigSet[i].hyper =
                                pContext->Config.focuser_set[i].hyper;
        pCaps->afConfigSet[i].inf =
                                pContext->Config.focuser_set[i].inf;
        pCaps->afConfigSet[i].hysteresis =
                                pContext->Config.focuser_set[i].hysteresis;
        pCaps->afConfigSet[i].settle_time =
                                pContext->Config.focuser_set[i].settle_time;
        pCaps->afConfigSet[i].macro_offset =
                                pContext->Config.focuser_set[i].macro_offset;
        pCaps->afConfigSet[i].inf_offset =
                                pContext->Config.focuser_set[i].inf_offset;
        pCaps->afConfigSet[i].num_dist_pairs =
                                pContext->Config.focuser_set[i].num_dist_pairs;

        NV_DEBUG_PRINTF(("i %d posture %d macro %d hyper %d inf %d "
                         "hyst %d setttle_time %d macro_offset %d "
                         "inf_offset %d num_pairs %d",
            i, pCaps->afConfigSet[i].posture, pCaps->afConfigSet[i].macro,
            pCaps->afConfigSet[i].hyper, pCaps->afConfigSet[i].inf,
            pCaps->afConfigSet[i].hysteresis, pCaps->afConfigSet[i].settle_time,
            pCaps->afConfigSet[i].macro_offset, pCaps->afConfigSet[i].inf_offset,
            pCaps->afConfigSet[i].num_dist_pairs));

        for (j = 0; j < pCaps->afConfigSet[i].num_dist_pairs; j++)
        {
            pCaps->afConfigSet[i].dist_pair[j].fdn =
                    pContext->Config.focuser_set[i].dist_pair[j].fdn;
            pCaps->afConfigSet[i].dist_pair[j].distance =
                    pContext->Config.focuser_set[i].dist_pair[j].distance;

            NV_DEBUG_PRINTF(("i %d j %d fdn %d distance %d", i, j,
                        pCaps->afConfigSet[i].dist_pair[j].fdn,
                        pCaps->afConfigSet[i].dist_pair[j].distance));
        }
    }

}

static void SetFocuserCapabilities(FocuserNvcContext *pContext,
                                    NvOdmImagerFocuserCapabilities *pCaps)
{
    NvU32 i, j;

    memset(&pContext->Config, 0, sizeof(pContext->Config));
    pContext->Config.settle_time = pCaps->settleTime;
    pContext->Config.range_ends_reversed = pCaps->rangeEndsReversed;

    pContext->Config.pos_working_low = pCaps->positionWorkingLow;
    pContext->Config.pos_working_high = pCaps->positionWorkingHigh;

    // The actual low and high are part of the ODM/Kernel. Blocks-camera may
    // not know the first time. In this case, it will send down 0s.
    // Do not overwrite in that case.
    pContext->Config.pos_actual_low = pCaps->positionActualLow;
    pContext->Config.pos_actual_high = pCaps->positionActualHigh;

    pContext->Config.slew_rate = pCaps->slewRate;
    pContext->Config.circle_of_confusion = pCaps->circleOfConfusion;

    NV_DEBUG_PRINTF(("%s: pCaps->afConfigSetSize %d",
                __FUNCTION__, pCaps->afConfigSetSize));
    NV_DEBUG_PRINTF(("%s: posActualLow %d high %d positionWorkingLow %d"
                    " positionWorkingHigh %d", __FUNCTION__,
                    pContext->Config.pos_actual_low,
                    pContext->Config.pos_actual_high,
                    pCaps->positionWorkingLow,
                    pCaps->positionWorkingHigh));

    for (i = 0; i < pCaps->afConfigSetSize; i++)
    {
        pContext->Config.focuser_set[i].posture =
                                pCaps->afConfigSet[i].posture;
        pContext->Config.focuser_set[i].macro =
                                pCaps->afConfigSet[i].macro;
        pContext->Config.focuser_set[i].hyper =
                                pCaps->afConfigSet[i].hyper;
        pContext->Config.focuser_set[i].inf =
                                pCaps->afConfigSet[i].inf;
        pContext->Config.focuser_set[i].hysteresis =
                                pCaps->afConfigSet[i].hysteresis;
        pContext->Config.focuser_set[i].settle_time =
                                pCaps->afConfigSet[i].settle_time;
        pContext->Config.focuser_set[i].macro_offset =
                                pCaps->afConfigSet[i].macro_offset;
        pContext->Config.focuser_set[i].inf_offset =
                                pCaps->afConfigSet[i].inf_offset;
        pContext->Config.focuser_set[i].num_dist_pairs =
                                pCaps->afConfigSet[i].num_dist_pairs;

        NV_DEBUG_PRINTF(("i %d posture %d macro %d hyper %d inf %d "
                         "hyst %d setttle_time %d macro_offset %d "
                         "inf_offset %d num_pairs %d",
            i, pCaps->afConfigSet[i].posture, pCaps->afConfigSet[i].macro,
            pCaps->afConfigSet[i].hyper, pCaps->afConfigSet[i].inf,
            pCaps->afConfigSet[i].hysteresis, pCaps->afConfigSet[i].settle_time,
            pCaps->afConfigSet[i].macro_offset, pCaps->afConfigSet[i].inf_offset,
            pCaps->afConfigSet[i].num_dist_pairs));

        for (j = 0; j < pCaps->afConfigSet[i].num_dist_pairs; j++)
        {
            pContext->Config.focuser_set[i].dist_pair[j].fdn =
                        pCaps->afConfigSet[i].dist_pair[j].fdn;
            pContext->Config.focuser_set[i].dist_pair[j].distance =
                        pCaps->afConfigSet[i].dist_pair[j].distance;

            NV_DEBUG_PRINTF(("i %d j %d fdn %d distance %d", i, j,
                        pCaps->afConfigSet[i].dist_pair[j].fdn,
                        pCaps->afConfigSet[i].dist_pair[j].distance));
        }
    }

}

static NvBool
FocuserNvc_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue)
{
    FocuserNvcContext *pCtxt;
    struct nvc_param params;
    int err;

    if (!hImager || !hImager->pFocuser || !hImager->pFocuser->pPrivateContext)
    {
        NvOsDebugPrintf("%s: No hImager->pFocuser->pPrivateContext\n",
                        __func__);
        return NV_FALSE;
    }

    pCtxt = (FocuserNvcContext*)hImager->pFocuser->pPrivateContext;
    params.param = Param;
    params.sizeofvalue = SizeOfValue;
    params.p_value = pValue;
    switch(Param) {
        case NvOdmImagerParameter_FocuserLocus:
            params.param = NVC_PARAM_LOCUS;
            break;

        case NvOdmImagerParameter_FocalLength:
            params.param = NVC_PARAM_FOCAL_LEN;
            break;

        case NvOdmImagerParameter_MaxAperture:
            params.param = NVC_PARAM_MAX_APERTURE;
            break;

        case NvOdmImagerParameter_FNumber:
            params.param = NVC_PARAM_FNUMBER;
            break;

        case NvOdmImagerParameter_FocuserCapabilities:
            memset(&pCtxt->Config, 0, sizeof(pCtxt->Config));
            params.p_value = &pCtxt->Config;
            params.param = NVC_PARAM_CAPS;
            params.sizeofvalue = sizeof(pCtxt->Config);
            break;

        default:
            break;
    }

    err = ioctl(pCtxt->focus_fd, NVC_IOCTL_PARAM_RD, &params);
    if (err < 0)
    {
        NvOsDebugPrintf("%s: ioctl to get parameter failed: %s\n",
                        __func__, strerror(errno));
        return NV_FALSE;
    }

    if(Param == NvOdmImagerParameter_FocuserCapabilities)
    {
        NvOdmImagerFocuserCapabilities *pCaps = (NvOdmImagerFocuserCapabilities *) pValue;

        GetFocuserCapabilities(pCtxt, pCaps);

        NV_DEBUG_PRINTF(("%s: After: Version %d actuatorRange %d settleTime %d"
            " positionActualLow %d positionActualHigh %d positionWorkingLow %d "
            "positionWorkingHigh %d", __func__, pCaps->version,
            pCaps->actuatorRange, pCaps->settleTime,
            pCaps->positionActualLow, pCaps->positionActualHigh,
            pCaps->positionWorkingLow, pCaps->positionWorkingHigh));
    }

    return NV_TRUE;
}

NvBool
FocuserNvc_GetHal(
    NvOdmImagerHandle hImager)
{
    if (!hImager || !hImager->pFocuser)
    {
        NvOsDebugPrintf("%s: No hImager->pFocuser\n", __func__);
        return NV_FALSE;
    }

    hImager->pFocuser->pfnOpen = FocuserNvc_Open;
    hImager->pFocuser->pfnClose = FocuserNvc_Close;
    hImager->pFocuser->pfnGetCapabilities = FocuserNvc_GetCapabilities;
    hImager->pFocuser->pfnSetPowerLevel = FocuserNvc_SetPowerLevel;
    hImager->pFocuser->pfnSetParameter = FocuserNvc_SetParameter;
    hImager->pFocuser->pfnGetParameter = FocuserNvc_GetParameter;

    return NV_TRUE;
}

#endif //(BUILD_FOR_AOS == 0)

