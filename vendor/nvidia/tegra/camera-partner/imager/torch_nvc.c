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
#include <nvc.h>
#include <nvc_torch.h>
#endif //(BUILD_FOR_AOS == 0)

#include "torch_nvc.h"
#include "imager_util.h"
#include "nvodm_services.h"
#include "nvodm_query_discovery_imager.h"

#if (BUILD_FOR_AOS == 0)
typedef struct TorchNvcContextRec
{
    int torch_fd; /* file handle to the kernel nvc_torch driver */
    NvOdmImagerFlashCapabilities FlashCaps;
    NvOdmImagerTorchCapabilities TorchCaps;
    struct nvc_torch_flash_capabilities FlashDrvrCaps;
    struct nvc_torch_level_info flash_levels[NVODM_IMAGER_MAX_FLASH_LEVELS - 1];
    struct nvc_torch_torch_capabilities TorchDrvrCaps;
    __s32 guideNum[NVODM_IMAGER_MAX_TORCH_LEVELS - 1];
} TorchNvcContext;
#endif

static NvBool
TorchNvc_Open(
    NvOdmImagerHandle hImager);

static void
TorchNvc_Close(
    NvOdmImagerHandle hImager);

static void
TorchNvc_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities);

static NvBool
TorchNvc_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel);

static NvBool
TorchNvc_SetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    const void *pValue);

static NvBool
TorchNvc_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue);

#if (BUILD_FOR_AOS == 0)
static void TransferFlashCapabilities(
    struct nvc_torch_flash_capabilities *pFlashCaps_K, NvOdmImagerFlashCapabilities *pFlashCaps_Odm)
{
    unsigned i;

    NvOdmOsMemset((void *)pFlashCaps_Odm, 0, sizeof(NvOdmImagerFlashCapabilities));

    pFlashCaps_Odm->NumberOfLevels = pFlashCaps_K->numberoflevels;
    if (pFlashCaps_Odm->NumberOfLevels > NVODM_IMAGER_MAX_FLASH_LEVELS)
    {
        pFlashCaps_Odm->NumberOfLevels = NVODM_IMAGER_MAX_FLASH_LEVELS;
    }

    for (i = 0; i < pFlashCaps_Odm->NumberOfLevels; i++)
    {
        pFlashCaps_Odm->levels[i].guideNum =
                        (NvF32)pFlashCaps_K->levels[i].guidenum;
        pFlashCaps_Odm->levels[i].sustainTime =
                        pFlashCaps_K->levels[i].sustaintime;
        pFlashCaps_Odm->levels[i].rechargeFactor =
                        (NvF32)pFlashCaps_K->levels[i].rechargefactor;
    }
}

static void TransferTorchCapabilities(
    struct nvc_torch_torch_capabilities *pTorchCaps_K, NvOdmImagerTorchCapabilities *pTorchCaps_Odm)
{
    unsigned i;

    NvOdmOsMemset((void *)pTorchCaps_Odm, 0, sizeof(NvOdmImagerTorchCapabilities));

    pTorchCaps_Odm->NumberOfLevels = pTorchCaps_K->numberoflevels;
    if (pTorchCaps_Odm->NumberOfLevels > NVODM_IMAGER_MAX_TORCH_LEVELS)
    {
        pTorchCaps_Odm->NumberOfLevels = NVODM_IMAGER_MAX_TORCH_LEVELS;
    }

    for (i = 0; i < pTorchCaps_Odm->NumberOfLevels; i++)
    {
        pTorchCaps_Odm->guideNum[i] = (NvF32)pTorchCaps_K->guidenum[i];
    }
}
#endif

static NvBool TorchNvc_Open(NvOdmImagerHandle hImager)
{
#if (BUILD_FOR_AOS == 0)
    TorchNvcContext *pCtxt = NULL;
    unsigned Instance;
    char DevName[NVODMIMAGER_IDENTIFIER_MAX];
    struct nvc_param params;
    int err;

    if (!hImager || !hImager->pFlash)
    {
        NvOsDebugPrintf("%s err: No hImager->pFlash\n", __func__);
        return NV_FALSE;
    }

    pCtxt = (TorchNvcContext *)NvOdmOsAlloc(sizeof(*pCtxt));
    if (!pCtxt)
    {
        NvOsDebugPrintf("%s err: NvOdmOsAlloc\n", __func__);
        return NV_FALSE;
    }

    hImager->pFlash->pPrivateContext = pCtxt;
    Instance = (unsigned)(hImager->pFlash->GUID & 0xF);
    if (Instance)
        sprintf(DevName, "/dev/torch.%u", Instance);
    else
        sprintf(DevName, "/dev/torch");
    pCtxt->torch_fd = open(DevName, O_RDWR);
    if (pCtxt->torch_fd < 0)
    {
        NvOsDebugPrintf("%s err: cannot open nvc torch driver: %s\n",
                        __func__, strerror(errno));
        TorchNvc_Close(hImager);
        return NV_FALSE;
    }

    params.param = NVC_PARAM_FLASH_CAPS;
    params.sizeofvalue = sizeof(pCtxt->FlashDrvrCaps) + sizeof(pCtxt->flash_levels);
    params.p_value = &pCtxt->FlashDrvrCaps;
    err = ioctl(pCtxt->torch_fd, NVC_IOCTL_PARAM_RD, &params);
    if (err < 0)
    {
        NvOsDebugPrintf("%s: ioctl to get flash caps failed: %s\n",
                        __func__, strerror(errno));
        TorchNvc_Close(hImager);
        return NV_FALSE;
    }

    /* populate the flash capabilities converting int to float */
    TransferFlashCapabilities(&pCtxt->FlashDrvrCaps, &pCtxt->FlashCaps);

    params.param = NVC_PARAM_TORCH_CAPS;
    params.sizeofvalue = sizeof(pCtxt->TorchDrvrCaps) + sizeof(pCtxt->guideNum);
    params.p_value = &pCtxt->TorchDrvrCaps;
    err = ioctl(pCtxt->torch_fd, NVC_IOCTL_PARAM_RD, &params);
    if (err < 0)
    {
        NvOsDebugPrintf("%s: ioctl to get torch caps failed: %s\n",
                        __func__, strerror(errno));
        TorchNvc_Close(hImager);
        return NV_FALSE;
    }

    /* populate the torch capabilities converting int to float */
    TransferTorchCapabilities(&pCtxt->TorchDrvrCaps, &pCtxt->TorchCaps);

    NvOsDebugPrintf("%s: torch_fd opened as: %d\n",
                    __func__, pCtxt->torch_fd);
#endif //(BUILD_FOR_AOS == 0)
    return NV_TRUE;
}

static void TorchNvc_Close(NvOdmImagerHandle hImager)
{
#if (BUILD_FOR_AOS == 0)
    TorchNvcContext *pCtxt = NULL;

    if (!hImager || !hImager->pFlash || !hImager->pFlash->pPrivateContext)
    {
        NvOsDebugPrintf("%s: No hImager->pFlash->pPrivateContext\n",
                        __func__);
        return;
    }

    pCtxt = (TorchNvcContext *)hImager->pFlash->pPrivateContext;
    close(pCtxt->torch_fd);
    NvOdmOsFree(pCtxt);
    hImager->pFlash->pPrivateContext = NULL;
#endif //(BUILD_FOR_AOS == 0)
}

static void
TorchNvc_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities)
{
}

static NvBool
TorchNvc_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel)
{
#if (BUILD_FOR_AOS == 0)
    int err;

    if (!hImager || !hImager->pFlash || !hImager->pFlash->pPrivateContext)
    {
        NvOsDebugPrintf("%s: No hImager->pFlash->pPrivateContext\n",
                        __func__);
        return NV_FALSE;
    }

    TorchNvcContext *pCtxt =
        (TorchNvcContext *)hImager->pFlash->pPrivateContext;
    err = ioctl(pCtxt->torch_fd, NVC_IOCTL_PWR_WR, PowerLevel);
    if (err < 0)
    {
        NvOsDebugPrintf("%s: ioctl to set power level (%d) failed: %s\n",
                        __func__, PowerLevel, strerror(errno));
        return NV_FALSE;
    }
#endif
    return NV_TRUE;
}

static NvBool
TorchNvc_SetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    const void *pValue)
{
#if (BUILD_FOR_AOS == 0)
    TorchNvcContext *pCtxt = NULL;
    struct nvc_param params;
    NvOdmImagerFlashCapabilities *pFlashOdmCaps;
    NvOdmImagerTorchCapabilities *pTorchOdmCaps;
    NvU32 DesiredLevel;
    NvF32 FloatVal = *(NvF32*)pValue;
    int err;

    if (!hImager || !hImager->pFlash || !hImager->pFlash->pPrivateContext)
    {
        NvOsDebugPrintf("%s: No hImager->pFlash->pPrivateContext\n",
                        __func__);
        return NV_FALSE;
    }

    pCtxt = (TorchNvcContext *)hImager->pFlash->pPrivateContext;

    params.param = Param;
    params.sizeofvalue = (__u32)SizeOfValue;
    params.p_value = &pValue;
    switch (Param)
    {
        case NvOdmImagerParameter_FlashLevel:
            pFlashOdmCaps = &pCtxt->FlashCaps;
            for (DesiredLevel = 0;
                 DesiredLevel < pFlashOdmCaps->NumberOfLevels;
                 DesiredLevel++)
            {
                if (FloatVal == pFlashOdmCaps->levels[DesiredLevel].guideNum)
                    break;
            }
            params.param = NVC_PARAM_FLASH_LEVEL;
            params.sizeofvalue = sizeof(DesiredLevel);
            params.p_value = &DesiredLevel;
            break;

        case NvOdmImagerParameter_TorchLevel:
            pTorchOdmCaps = &pCtxt->TorchCaps;
            for (DesiredLevel = 0;
                 DesiredLevel < pTorchOdmCaps->NumberOfLevels;
                 DesiredLevel++)
            {
                if (FloatVal == pTorchOdmCaps->guideNum[DesiredLevel])
                    break;
            }
            params.param = NVC_PARAM_TORCH_LEVEL;
            params.sizeofvalue = sizeof(DesiredLevel);
            params.p_value = &DesiredLevel;
            break;

        case NvOdmImagerParameter_FlashPinState:
            params.param = NVC_PARAM_FLASH_PIN_STATE;
            break;

        default:
            break;
    }
    err = ioctl(pCtxt->torch_fd, NVC_IOCTL_PARAM_WR, &params);
    if (err < 0)
    {
        NvOsDebugPrintf("%s: ioctl to set parameter %x failed: %s\n",
                        __func__, Param, strerror(errno));
        return NV_FALSE;
    }
#endif //(BUILD_FOR_AOS == 0)
    return NV_TRUE;
}

static NvBool
TorchNvc_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue)
{
#if (BUILD_FOR_AOS == 0)
    TorchNvcContext *pCtxt = NULL;
    struct nvc_param params;
    int err;

    if (!hImager || !hImager->pFlash || !hImager->pFlash->pPrivateContext)
    {
        NvOsDebugPrintf("%s: No hImager->pFlash->pPrivateContext\n",
                        __func__);
        return NV_FALSE;
    }

    pCtxt = (TorchNvcContext *)hImager->pFlash->pPrivateContext;

    params.param = Param;
    params.sizeofvalue = (__u32)SizeOfValue;
    params.p_value = pValue;
    switch(Param)
    {
        case NvOdmImagerParameter_FlashCapabilities:
            NvOdmOsMemcpy(pValue, &pCtxt->FlashCaps,
                          sizeof(NvOdmImagerFlashCapabilities));
            return NV_TRUE;

        case NvOdmImagerParameter_TorchCapabilities:
            NvOdmOsMemcpy(pValue, &pCtxt->TorchCaps,
                          sizeof(NvOdmImagerTorchCapabilities));
            return NV_TRUE;

        case NvOdmImagerParameter_FlashPinState:
            params.param = NVC_PARAM_FLASH_PIN_STATE;
            break;

        default:
            break;
    }
    err = ioctl(pCtxt->torch_fd, NVC_IOCTL_PARAM_RD, &params);
    if (err < 0)
    {
        NvOsDebugPrintf("%s: ioctl to get parameter %d failed: %s\n",
                        __func__, Param, strerror(errno));
        return NV_FALSE;
    }
#endif //(BUILD_FOR_AOS == 0)
    return NV_TRUE;
}

NvBool TorchNvc_GetHal(NvOdmImagerHandle hImager)
{
    if (!hImager || !hImager->pFlash)
    {
        NvOsDebugPrintf("%s err: No hImager->pFlash\n", __func__);
        return NV_FALSE;
    }

    hImager->pFlash->pfnOpen = TorchNvc_Open;
    hImager->pFlash->pfnClose = TorchNvc_Close;
    hImager->pFlash->pfnGetCapabilities = TorchNvc_GetCapabilities;
    hImager->pFlash->pfnSetPowerLevel = TorchNvc_SetPowerLevel;
    hImager->pFlash->pfnSetParameter = TorchNvc_SetParameter;
    hImager->pFlash->pfnGetParameter = TorchNvc_GetParameter;
    return NV_TRUE;
}

