
#define NV_IDL_IS_STUB

/*
 * Copyright (c) 2009-2012, NVIDIA CORPORATION.  All rights reserved.
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
#include "nvrm_pinmux.h"

NvError NvRmGetStraps(
    NvRmDeviceHandle hDevice,
    NvRmStrapGroup StrapGroup,
    NvU32 *pStrapValue)
{
    return NvError_NotSupported;
}

NvError NvRmGetModuleInterfaceCapabilities(
    NvRmDeviceHandle hRm,
    NvRmModuleID ModuleId,
    NvU32 CapStructSize,
    void *pCaps)
{
    return NvError_NotSupported;
}

NvU32 NvRmExternalClockConfig(
    NvRmDeviceHandle hDevice,
    NvU32 IoModule,
    NvU32 Instance,
    NvU32 Config,
    NvBool EnableTristate)
{
    return 0;
}

#ifndef SET_KERNEL_PINMUX
NvError NvRmSetOdmModuleTristate(
    NvRmDeviceHandle hDevice,
    NvU32 OdmModule,
    NvU32 OdmInstance,
    NvBool EnableTristate)
{
    return NvSuccess;
}

NvError NvRmSetModuleTristate(
    NvRmDeviceHandle hDevice,
    NvRmModuleID RmModule,
    NvBool EnableTristate)
{
    return NvSuccess;
}
#endif
