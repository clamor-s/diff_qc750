/*
 * Copyright (c) 2008 - 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
 #ifndef INCLUDED_NVBOOT_MISC_AP20_H
 #define INCLUDED_NVBOOT_MISC_AP20_H

#include "nvcommon.h"
#include "ap20/include/nvboot_clocks_int.h"
#include "ap20/include/nvboot_reset.h"

#define NV_ADDRESS_MAP_CAR_BASE         0x60006000
#define NV_ADDRESS_MAP_TMRUS_BASE       0x60005010
#define NV_ADDRESS_MAP_USB_BASE         0xC5000000
#define NV_ADDRESS_MAP_FUSE_BASE        0x7000F800

NvBootClocksOscFreq NvBootClocksGetOscFreqAp20(void);
void
NvBootClocksStartPllAp20(NvBootClocksPllId PllId,
                     NvU32 M,
                     NvU32 N,
                     NvU32 P,
                     NvU32 CPCON,
                     NvU32 LFCON,
                     NvU32 *StableTime );

void
NvBootClocksSetEnableAp20(NvBootClocksClockId ClockId, NvBool Enable);

void NvBootUtilWaitUSAp20( NvU32 usec );

NvU32 NvBootUtilGetTimeUSAp20( void );

void
NvBootResetSetEnableAp20(const NvBootResetDeviceId DeviceId, const NvBool Enable);

void NvBootFuseGetSkuRawAp20(NvU32 *pSku);

#endif //INCLUDED_NVBOOT_MISC_AP20_H

