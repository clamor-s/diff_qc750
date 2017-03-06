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
#include <tps61050.h>
#endif

#include "flash_tps61050.h"
#include "imager_util.h"
#include "nvodm_services.h"
#include "nvodm_query_discovery_imager.h"

typedef struct NvOdmFlashTPS61050ContextRec
{
    int torch_fd; /* file handle to the kernel tps61050 driver */
} NvOdmFlashTPS61050Context;

/*
 * each ODM sensor/focuser/flash driver should at least
 * define the following APIs:
 * - Open
 * - Close
 * - GetCapabilities
 * - GetPowerLevel
 * - SetParameter
 * - GetParameter
 */

static NvBool FlashTPS61050_Open(NvOdmImagerHandle hImager);

static void FlashTPS61050_Close(NvOdmImagerHandle hImager);

static void
FlashTPS61050_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities);

static NvBool
FlashTPS61050_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel);

static NvBool
FlashTPS61050_SetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    const void *pValue);

static NvBool
FlashTPS61050_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue);

static NvBool FlashTPS61050_Open(NvOdmImagerHandle hImager)
{
#if (BUILD_FOR_AOS == 0)
    NvOdmFlashTPS61050Context *pContext = NULL;

    if (!hImager || !hImager->pFlash)
    {
        NvOsDebugPrintf("%s(%d): cannot open tps61050 driver\n",
                __func__, __LINE__);
        return NV_FALSE;
    }

    pContext = (NvOdmFlashTPS61050Context *)
        NvOdmOsAlloc(sizeof(NvOdmFlashTPS61050Context));
    if (!pContext)
        return NV_FALSE;

    NvOdmOsMemset(pContext, 0, sizeof(NvOdmFlashTPS61050Context));

    hImager->pFlash->pPrivateContext = pContext;

    pContext->torch_fd = open("/dev/tps61050-1", O_RDWR);
    if (pContext->torch_fd < 0)
    {
        NvOsDebugPrintf("%s(%d): cannot open tps61050 driver: %s\n",
                __func__, __LINE__, strerror(errno));
        pContext->torch_fd = -1;
        return NV_FALSE;
    }

    NV_DEBUG_PRINTF(("%s(%d): TPS61050 torch_fd: %d\n",
            __func__, __LINE__, pContext->torch_fd));
#endif

    return NV_TRUE;
}

static void FlashTPS61050_Close(NvOdmImagerHandle hImager)
{
#if (BUILD_FOR_AOS == 0)
    NvOdmFlashTPS61050Context *pContext = NULL;

    if (!hImager || !hImager->pFlash || !hImager->pFlash->pPrivateContext)
    {
        NvOsDebugPrintf("%s(%d): cannot close tps61050 driver\n",
                __func__, __LINE__);
        return;
    }

    pContext = (NvOdmFlashTPS61050Context *)hImager->pFlash->pPrivateContext;

    if (pContext && pContext->torch_fd != -1)
    {
        close(pContext->torch_fd);
        pContext->torch_fd = -1;
    }

    if (pContext) {
        NvOdmOsFree(pContext);
        hImager->pFlash->pPrivateContext = NULL;
    }
#endif
}

static void
FlashTPS61050_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities)
{
#if (BUILD_FOR_AOS == 0)
    if (!hImager || !hImager->pFlash || !hImager->pFlash->pPrivateContext)
    {
        NvOsDebugPrintf("%s(%d): cannot get tps61050 capabilities\n",
                __func__, __LINE__);
        return;
    }

    NvOdmFlashTPS61050Context *pContext =
        (NvOdmFlashTPS61050Context *)hImager->pFlash->pPrivateContext;

    if (pContext->torch_fd == -1)
    {
        NvOsDebugPrintf("%s(%d): tps61050 driver not available, set GUID to 0\n",
                __func__, __LINE__);
        pCapabilities->FlashGUID = 0; /* make flash unavailable to upper layer */
    }
#endif
}

static NvBool
FlashTPS61050_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel)
{
#if (BUILD_FOR_AOS == 0)
    if (!hImager || !hImager->pFlash || !hImager->pFlash->pPrivateContext)
    {
        NvOsDebugPrintf("%s(%d): cannot set tps61050 power level\n",
                __func__, __LINE__);
        return NV_FALSE;
    }

    NvOdmFlashTPS61050Context *pContext =
        (NvOdmFlashTPS61050Context *)hImager->pFlash->pPrivateContext;

    if (pContext->torch_fd == -1)
    {
        NvOsDebugPrintf("%s(%d): tps61050 driver not available, don't set power level\n",
                __func__, __LINE__);
        return NV_TRUE;
    }

    if (ioctl(pContext->torch_fd, TPS61050_IOCTL_PWR, PowerLevel) < 0)
    {
        NvOsDebugPrintf("%s(%d): ioctl to set power level (%d) failed: %s\n",
                __func__, __LINE__, PowerLevel, strerror(errno));
        return NV_FALSE;
    }
#endif

    return NV_TRUE;
}

#if (BUILD_FOR_AOS == 0)
static NvBool
FlashTPS61050_FlashCapsRd(
    NvOdmFlashTPS61050Context *pContext,
    struct tps61050_flash_capabilities *pCastFlash)
{
    struct tps61050_param params;
    NvOdmImagerFlashCapabilities *pFlashCast;
    NvU32 i;

    params.param = NvOdmImagerParameter_FlashCapabilities;
    params.sizeofvalue = (sizeof(struct tps61050_flash_capabilities));
    params.p_value = (struct tps61050_flash_capabilities*)pCastFlash;
    if (ioctl(pContext->torch_fd, TPS61050_IOCTL_PARAM_RD, &params) < 0)
    {
        NvOsDebugPrintf("%s(%d): ioctl to get flash caps failed: %s\n",
                __func__, __LINE__, strerror(errno));
        return NV_FALSE;
    }

    pFlashCast = (NvOdmImagerFlashCapabilities*)pCastFlash;
    for (i = 0; i <= pFlashCast->NumberOfLevels; i++)
    {
        pFlashCast->levels[i].guideNum =
                        (NvF32)pCastFlash->levels[i].guidenum;
        pFlashCast->levels[i].rechargeFactor =
                        (NvF32)pCastFlash->levels[i].rechargefactor;
    }
    return NV_TRUE;
}

static NvBool
FlashTPS61050_TorchCapsRd(
    NvOdmFlashTPS61050Context *pContext,
    struct tps61050_torch_capabilities *pCastTorch)
{
    struct tps61050_param params;
    NvOdmImagerTorchCapabilities *pTorchCast;
    NvU32 i;

    params.param = NvOdmImagerParameter_TorchCapabilities;
    params.sizeofvalue = (sizeof(struct tps61050_torch_capabilities));
    params.p_value = (struct tps61050_torch_capabilities*)pCastTorch;
    if (ioctl(pContext->torch_fd, TPS61050_IOCTL_PARAM_RD, &params) < 0)
    {
        NvOsDebugPrintf("%s(%d): ioctl to get torch caps failed: %s\n",
                __func__, __LINE__, strerror(errno));
        return NV_FALSE;
    }

    pTorchCast = (NvOdmImagerTorchCapabilities*)pCastTorch;
    for (i = 0; i <= pTorchCast->NumberOfLevels; i++)
    {
        pTorchCast->guideNum[i] = (NvF32)pCastTorch->guidenum[i];
    }
    return NV_TRUE;
}
#endif // (BUILD_FOR_AOS == 0)

static NvBool
FlashTPS61050_SetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    const void *pValue)
{
#if (BUILD_FOR_AOS == 0)
    if (!hImager || !hImager->pFlash || !hImager->pFlash->pPrivateContext)
    {
        NvOsDebugPrintf("%s(%d): cannot set tps61050 parameter\n",
                __func__, __LINE__);
        return NV_FALSE;
    }

    NvOdmFlashTPS61050Context *pContext =
        (NvOdmFlashTPS61050Context *)hImager->pFlash->pPrivateContext;
    NvOdmImagerFlashCapabilities *pFlashCaps;
    NvOdmImagerTorchCapabilities *pTorchCaps;
    struct tps61050_param params;
    struct tps61050_flash_capabilities *pCastFlash;
    struct tps61050_torch_capabilities *pCastTorch;
    NvU32 DesiredLevel = 0;
    NvF32 FloatVal = *(NvF32*)pValue;

    if (pContext->torch_fd == -1)
    {
        NvOsDebugPrintf("%s(%d): tps61050 driver not available, don't set parameter\n",
                __func__, __LINE__);
        return NV_TRUE;
    }

    switch (Param)
    {
        case NvOdmImagerParameter_FlashLevel:
            if (!(pCastFlash = (struct tps61050_flash_capabilities*)
                  NvOdmOsAlloc(sizeof(struct tps61050_flash_capabilities))))
                goto FailFlash;

            if (!FlashTPS61050_FlashCapsRd(pContext, pCastFlash))
                goto FailFlash;

            // find the requested flash level
            pFlashCaps = (NvOdmImagerFlashCapabilities*)pCastFlash;
            for (DesiredLevel = 0; DesiredLevel < pFlashCaps->NumberOfLevels;
                                                        DesiredLevel++)
            {
                if (FloatVal == pFlashCaps->levels[DesiredLevel].guideNum)
                    break;
            }
            params.sizeofvalue = sizeof(DesiredLevel);
            params.p_value = &DesiredLevel;
            NvOdmOsFree(pCastFlash);
            break;

FailFlash:
            NvOdmOsFree(pCastFlash);
            return NV_FALSE;

        case NvOdmImagerParameter_TorchLevel:
            if (!(pCastTorch = (struct tps61050_torch_capabilities*)
                  NvOdmOsAlloc(sizeof(struct tps61050_torch_capabilities))))
                goto FailTorch;

            if (!FlashTPS61050_TorchCapsRd(pContext, pCastTorch))
                goto FailTorch;

            // find the requested torch level
            pTorchCaps = (NvOdmImagerTorchCapabilities*)pCastTorch;
            for (DesiredLevel = 0; DesiredLevel < pTorchCaps->NumberOfLevels;
                                                        DesiredLevel++)
            {
                if (FloatVal == pTorchCaps->guideNum[DesiredLevel])
                    break;
            }
            params.sizeofvalue = sizeof(DesiredLevel);
            params.p_value = &DesiredLevel;
            NvOdmOsFree(pCastTorch);
            break;

FailTorch:
            NvOdmOsFree(pCastTorch);
            return NV_FALSE;

        case NvOdmImagerParameter_FlashPinState:
            params.sizeofvalue = SizeOfValue;
            params.p_value = &pValue;
            break;

        default:
            NV_ASSERT(!"unsurported parameter cannot be set in tps61050!");
            return NV_FALSE;
    }

    params.param = Param;
    if (ioctl(pContext->torch_fd, TPS61050_IOCTL_PARAM_WR, &params) < 0)
    {
        NvOsDebugPrintf("%s(%d): ioctl to set parameter failed: %s\n",
                __func__, __LINE__, strerror(errno));
        return NV_FALSE;
    }
#endif

    return NV_TRUE;
}

static NvBool
FlashTPS61050_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue)
{
#if (BUILD_FOR_AOS == 0)
    if (!hImager || !hImager->pFlash || !hImager->pFlash->pPrivateContext)
    {
        NvOsDebugPrintf("%s(%d): cannot get tps61050 parameter\n",
                __func__, __LINE__);
        return NV_FALSE;
    }

    NvOdmFlashTPS61050Context *pContext =
        (NvOdmFlashTPS61050Context *)hImager->pFlash->pPrivateContext;
    struct tps61050_param params;
    struct tps61050_flash_capabilities *pCastFlash;
    struct tps61050_torch_capabilities *pCastTorch;

    switch(Param)
    {
        case NvOdmImagerParameter_FlashCapabilities:
            if (!(pCastFlash = (struct tps61050_flash_capabilities*)NvOdmOsAlloc
                                (sizeof(struct tps61050_flash_capabilities))))
                goto FailFlash;

            if (FlashTPS61050_FlashCapsRd(pContext, pCastFlash))
                NvOdmOsMemcpy(pValue, pCastFlash,
                              sizeof(struct tps61050_flash_capabilities));
            else
                goto FailFlash;

            NvOdmOsFree(pCastFlash);
            break;

FailFlash:
            NvOdmOsFree(pCastFlash);
            return NV_FALSE;

        case NvOdmImagerParameter_TorchCapabilities:
            if (!(pCastTorch = (struct tps61050_torch_capabilities*)NvOdmOsAlloc
                                (sizeof(struct tps61050_torch_capabilities))))
                goto FailTorch;

            if (FlashTPS61050_TorchCapsRd(pContext, pCastTorch))
                NvOdmOsMemcpy(pValue, pCastTorch,
                              sizeof(struct tps61050_torch_capabilities));
            else
                goto FailTorch;

            NvOdmOsFree(pCastTorch);
            break;

FailTorch:
            NvOdmOsFree(pCastTorch);
            return NV_FALSE;

        case NvOdmImagerParameter_FlashPinState:
            params.param = Param;
            params.sizeofvalue = SizeOfValue;
            params.p_value = pValue;
            if (ioctl(pContext->torch_fd, TPS61050_IOCTL_PARAM_RD, &params) < 0)
            {
                NvOsDebugPrintf("%s(%d): ioctl to get parameter failed: %s\n",
                        __func__, __LINE__, strerror(errno));
                return NV_FALSE;
            }
            break;

        default:
            return NV_FALSE;
    }
#endif

    return NV_TRUE;
}

NvBool FlashTPS61050_GetHal(NvOdmImagerHandle hImager)
{
    if (!hImager || !hImager->pFlash)
    {
        NvOsDebugPrintf("%s(%d): GetHal() failed\n",
                __func__, __LINE__);
        return NV_FALSE;
    }

    hImager->pFlash->pfnOpen = FlashTPS61050_Open;
    hImager->pFlash->pfnClose = FlashTPS61050_Close;
    hImager->pFlash->pfnGetCapabilities = FlashTPS61050_GetCapabilities;
    hImager->pFlash->pfnSetPowerLevel = FlashTPS61050_SetPowerLevel;
    hImager->pFlash->pfnSetParameter = FlashTPS61050_SetParameter;
    hImager->pFlash->pfnGetParameter = FlashTPS61050_GetParameter;

    return NV_TRUE;
}
