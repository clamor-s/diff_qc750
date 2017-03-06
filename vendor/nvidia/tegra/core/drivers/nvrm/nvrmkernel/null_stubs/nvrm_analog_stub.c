
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
#include "nvrm_analog.h"

NvU32 NvRmUsbDetectChargerState( NvRmDeviceHandle hDevice, NvU32 wait )
{
    return 0;
}

NvBool NvRmUsbIsConnected( NvRmDeviceHandle hDevice )
{
    return NV_FALSE;
}

NvU8 NvRmAnalogGetTvDacConfiguration(
    NvRmDeviceHandle hDevice,
    NvRmAnalogTvDacType Type)
{
    return 0;
}

NvError NvRmAnalogInterfaceControl(
    NvRmDeviceHandle hDevice,
    NvRmAnalogInterface Interface,
    NvBool Enable,
    void* Config,
    NvU32 ConfigLength)
{
    return NvError_NotSupported;
}
