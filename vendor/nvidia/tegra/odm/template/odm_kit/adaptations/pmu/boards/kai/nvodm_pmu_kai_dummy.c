/*
 * Copyright (c) 2012 NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_pmu.h"
#include "nvodm_pmu_kai.h"
#include "nvassert.h"

void KaiPmuInterruptHandler( NvOdmPmuDeviceHandle  hDevice)
{
    // NOTE: Not implemented completely as it is only for bootloader.
}

NvBool KaiPmuReadRtc( NvOdmPmuDeviceHandle  hDevice, NvU32 *Count)
{
    // NOTE: Not implemented completely as it is only for bootloader.
    *Count = 0;
    return NV_FALSE;
}

NvBool KaiPmuWriteRtc( NvOdmPmuDeviceHandle  hDevice, NvU32 Count)
{
    // NOTE: Not implemented completely as it is only for bootloader.
    return NV_FALSE;
}

NvBool KaiPmuIsRtcInitialized( NvOdmPmuDeviceHandle  hDevice)
{
    // NOTE: Not implemented completely as it is only for bootloader.
    return NV_FALSE;
}
