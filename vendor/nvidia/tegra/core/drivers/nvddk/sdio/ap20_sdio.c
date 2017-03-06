/*
 * Copyright (c) 2011-2012, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * @brief <b>NVIDIA Driver Development Kit:
 *                 NvDDK AP20 SDIO Driver Implementation</b>
 *
 * @b Description: Implementation of the NvDDK SDIO API.
 *
 */

#include "nvddk_sdio_private.h"
#include "ap20/arsdmmc.h"

static void AP20SdioSetUhsmode(NvDdkSdioDeviceHandle hSdio, NvDdkSdioUhsMode Uhsmode)
{
    // AP20 doesn't support uhs mode. Do nothing here
}

static void AP20SdioSetTapDelay(NvDdkSdioDeviceHandle hSdio)
{
    // In AP20, tap delay is set in the clk and rst controller registers. Do nothing here
}

static void AP20SdioGetHigherCapabilities(NvDdkSdioDeviceHandle hSdio,
    NvDdkSdioHostCapabilities *pHostCaps)
{
    // AP20 doesn't support any higher capabilities. Set all higher capabilities to false
    pHostCaps->IsDDRmodeSupported = NV_FALSE;
}

static void Ap20SdioDpdEnable(NvDdkSdioDeviceHandle hSdio)
{
    // Not supported
    return;
}

static void Ap20SdioDpdDisable(NvDdkSdioDeviceHandle hSdio)
{
    // Not supported
    return;
}
void AP20SdioOpenHWInterface(NvDdkSdioDeviceHandle hSdio)
{
    hSdio->NvDdkSdioGetHigherCapabilities = (void *)AP20SdioGetHigherCapabilities;
    hSdio->NvDdkSdioSetTapDelay = (void *)AP20SdioSetTapDelay;
    hSdio->NvDdkSdioConfigureUhsMode = (void *)AP20SdioSetUhsmode;
    hSdio->NvDdkSdioDpdEnable = (void *)Ap20SdioDpdEnable;
    hSdio->NvDdkSdioDpdDisable = (void *)Ap20SdioDpdDisable;
}

