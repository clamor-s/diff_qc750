/*
 * Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
/**
 * @file
 * <b>NVIDIA Tegra ODM Kit: Dummy implementation of public API of
 *                          Enterprise pmu driver interface.</b>
 *
 * @b Description: Dummy Implementation of  the non-power regulator api of
 *                        public interface for the pmu driver for enterprise.
 *
 */

#include "nvodm_pmu.h"
#include "nvodm_pmu_enterprise.h"
#include "nvassert.h"

void EnterprisePmuInterruptHandler( NvOdmPmuDeviceHandle  hDevice)
{
    // NOTE: Not implemented completely as it is only for bootloader.
}

NvBool EnterprisePmuReadRtc( NvOdmPmuDeviceHandle  hDevice, NvU32 *Count)
{
    // NOTE: Not implemented completely as it is only for bootloader.
    *Count = 0;
    return NV_FALSE;
}

NvBool EnterprisePmuWriteRtc( NvOdmPmuDeviceHandle  hDevice, NvU32 Count)
{
    // NOTE: Not implemented completely as it is only for bootloader.
    return NV_FALSE;
}

NvBool EnterprisePmuIsRtcInitialized( NvOdmPmuDeviceHandle  hDevice)
{
    // NOTE: Not implemented completely as it is only for bootloader.
    return NV_FALSE;
}
