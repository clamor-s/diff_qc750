
#define NV_IDL_IS_STUB

/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvos.h"
#include "nvassert.h"
#include "nvidlcmd.h"
#include "nvrm_memctrl.h"

NvError NvRmCorePerfMonStop(
    NvRmDeviceHandle hRmDevice,
    NvU32 *pCountListSize,
    NvU32 *pCountList,
    NvU32 *pTotalCycleCount)
{
    if (pCountListSize)
        *pCountListSize = 0;
    return NvSuccess;
}

NvError NvRmCorePerfMonStart(
    NvRmDeviceHandle hRmDevice,
    NvU32 *pEventListSize,
    NvU32 *pEventList)
{
    if (pEventListSize)
        *pEventListSize = 0;
    return NvSuccess;
}

NvError ReadObsData(
    NvRmDeviceHandle rm,
    NvRmModuleID modId,
    NvU32 start_index,
    NvU32 length,
    NvU32 *value)
{
    return NvError_NotSupported;
}

void McStat_Report(
    NvU32 client_id_0,
    NvU32 client_0_cycles,
    NvU32 client_id_1,
    NvU32 client_1_cycles,
    NvU32 llc_client_id,
    NvU32 llc_client_clocks,
    NvU32 llc_client_cycles,
    NvU32 mc_clocks)
{
}

void McStat_Stop(
    NvRmDeviceHandle rm,
    NvU32 *client_0_cycles,
    NvU32 *client_1_cycles,
    NvU32 *llc_client_cycles,
    NvU32 *llc_client_clocks,
    NvU32 *mc_clocks)
{
    if (client_0_cycles)
        *client_0_cycles = 0;
    if (client_1_cycles)
        *client_1_cycles = 0;
    if (llc_client_cycles)
        *llc_client_cycles = 0;
    if (llc_client_clocks)
        *llc_client_clocks = 0;
    if (mc_clocks)
        *mc_clocks = 0;
}

void McStat_Start(
    NvRmDeviceHandle rm,
    NvU32 client_id_0,
    NvU32 client_id_1,
    NvU32 llc_client_id)
{
}
