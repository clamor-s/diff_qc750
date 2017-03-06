
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
#include "nvrm_pcie.h"

void NvRmUnmapPciMemory(
    NvRmDeviceHandle hDeviceHandle,
    NvRmPhysAddr mem,
    NvU32 size)
{
}

NvRmPhysAddr NvRmMapPciMemory(
    NvRmDeviceHandle hDeviceHandle,
    NvRmPciPhysAddr mem,
    NvU32 size)
{
    return ~0;
}

NvError NvRmRegisterPcieLegacyHandler(
    NvRmDeviceHandle hDeviceHandle,
    NvU32 function_device_bus,
    NvOsSemaphoreHandle sem,
    NvBool InterruptEnable)
{
    return NvError_AccessDenied;
}

NvError NvRmRegisterPcieMSIHandler(
    NvRmDeviceHandle hDeviceHandle,
    NvU32 function_device_bus,
    NvU32 index,
    NvOsSemaphoreHandle sem,
    NvBool InterruptEnable)
{
    return NvError_AccessDenied;
}

NvError NvRmReadWriteConfigSpace(
    NvRmDeviceHandle hDeviceHandle,
    NvU32 bus_number,
    NvRmPcieAccessType type,
    NvU32 offset,
    NvU8 *Data,
    NvU32 DataLen)
{
    return NvError_AccessDenied;
}
