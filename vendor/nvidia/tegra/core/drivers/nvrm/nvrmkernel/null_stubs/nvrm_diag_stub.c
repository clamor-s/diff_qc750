
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
#include "nvrm_diag.h"

NvError NvRmDiagGetTemperature(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmTmonZoneId ZoneId,
    NvS32 *pTemperatureC)
{
    return NvError_NotSupported;
}

NvBool NvRmDiagIsLockSupported(void)
{
    return NV_FALSE;
}

NvError NvRmDiagConfigurePowerRail(
    NvRmDiagPowerRailHandle hRail,
    NvU32 VoltageMV)
{
    return NvError_NotSupported;
}

NvError NvRmDiagModuleListPowerRails(
    NvRmDiagModuleID id,
    NvU32 *pListSize,
    NvRmDiagPowerRailHandle *phRailList)
{
    return NvError_NotSupported;
}

NvU64 NvRmDiagPowerRailGetName(NvRmDiagPowerRailHandle hRail)
{
    return 0;
}

NvError NvRmDiagListPowerRails(
    NvU32 *pListSize,
    NvRmDiagPowerRailHandle *phRailList)
{
    return NvError_NotSupported;
}

NvError NvRmDiagModuleReset(
    NvRmDiagModuleID id,
    NvBool KeepAsserted)
{
    return NvError_NotSupported;
}

NvError NvRmDiagClockScalerConfigure(
    NvRmDiagClockSourceHandle hScaler,
    NvRmDiagClockSourceHandle hInput,
    NvU32 M,
    NvU32 N)
{
    return NvError_NotSupported;
}

NvError NvRmDiagPllConfigure(
    NvRmDiagClockSourceHandle hPll,
    NvU32 M,
    NvU32 N,
    NvU32 P)
{
    return NvError_NotSupported;
}

NvU32 NvRmDiagOscillatorGetFreq(
    NvRmDiagClockSourceHandle hOscillator)
{
    return 0;
}

NvError NvRmDiagClockSourceListSources(
    NvRmDiagClockSourceHandle hSource,
    NvU32 *pListSize,
    NvRmDiagClockSourceHandle *phSourceList)
{
    return NvError_NotSupported;
}

NvRmDiagClockScalerType NvRmDiagClockSourceGetScaler(
    NvRmDiagClockSourceHandle hSource)
{
    return NvRmDiagClockScalerType_NoScaler;
}

NvRmDiagClockSourceType NvRmDiagClockSourceGetType(
    NvRmDiagClockSourceHandle hSource)
{
    return NvRmDiagClockSourceType_Oscillator;
}

NvU64 NvRmDiagClockSourceGetName( NvRmDiagClockSourceHandle hSource )
{
    return 0;
}

NvError NvRmDiagModuleClockConfigure(
    NvRmDiagModuleID id,
    NvRmDiagClockSourceHandle hSource,
    NvU32 divider,
    NvBool Source1st)
{
    return NvError_NotSupported;;
}

NvError NvRmDiagModuleClockEnable(NvRmDiagModuleID id, NvBool enable)
{
    return NvError_NotSupported;
}

NvError NvRmDiagModuleListClockSources(
    NvRmDiagModuleID id,
    NvU32 *pListSize,
    NvRmDiagClockSourceHandle *phSourceList)
{
    return NvError_NotSupported;
}

NvError NvRmDiagListClockSources(
    NvU32 *pListSize,
    NvRmDiagClockSourceHandle *phSourceList)
{
    return NvError_NotSupported;
}

NvError NvRmDiagListModules(NvU32 *pListSize, NvRmDiagModuleID *pIdList)
{
    return NvError_NotSupported;
}

NvError NvRmDiagEnable(NvRmDeviceHandle hDevice)
{
    return NvError_NotSupported;
}
