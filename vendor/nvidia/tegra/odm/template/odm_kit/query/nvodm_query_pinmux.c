/*
 * Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_query_pinmux.h"

static const NvU32 s_NvOdmPinMuxConfig_VideoInput[] = {
    0
};

static const NvU32 s_NvOdmPinMuxConfig_Hdcp[] = {
    NvOdmHdcpPinMap_Config1
};

static const NvU32 s_NvOdmPinMuxConfig_Hdmi[] = {
    NvOdmHdmiPinMap_Config1
};

static const NvU32 s_NvOdmPinMuxConfig_Display[] = {
    NvOdmDisplayPinMap_Config1,
    0
};

static const NvU32 s_NvOdmPinMuxConfig_ExternalClock[] = {
    NvOdmExternalClockPinMap_Config1,
    NvOdmExternalClockPinMap_Config1,
    NvOdmExternalClockPinMap_Config1
};

void NvOdmQueryPinMux(
    NvOdmIoModule IoModule,
    const NvU32 **pPinMuxConfigTable,
    NvU32 *pCount)
{
    switch (IoModule)
    {
    case NvOdmIoModule_Display:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_Display;
        *pCount = NV_ARRAY_SIZE(s_NvOdmPinMuxConfig_Display);
        break;

    case NvOdmIoModule_Hdcp:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_Hdcp;
        *pCount = NV_ARRAY_SIZE(s_NvOdmPinMuxConfig_Hdcp);
        break;

    case NvOdmIoModule_Hdmi:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_Hdmi;
        *pCount = NV_ARRAY_SIZE(s_NvOdmPinMuxConfig_Hdmi);
        break;

    case NvOdmIoModule_ExternalClock:
        *pPinMuxConfigTable = s_NvOdmPinMuxConfig_ExternalClock;
        *pCount = NV_ARRAY_SIZE(s_NvOdmPinMuxConfig_ExternalClock);
        break;

    case NvOdmIoModule_VideoInput:
        *pPinMuxConfigTable = NULL;
        *pCount = 0;
        break;

    default:
        *pCount = 0;
        break;
    }
}

void
NvOdmQueryClockLimits(
    NvOdmIoModule IoModule,
    const NvU32 **pClockSpeedLimits,
    NvU32 *pCount)
{
    switch (IoModule)
    {
        default:
            *pClockSpeedLimits = NULL;
            *pCount = 0;
            break;
    }
}

