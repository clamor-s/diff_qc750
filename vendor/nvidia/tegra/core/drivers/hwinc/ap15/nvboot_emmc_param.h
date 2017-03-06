/**
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * Defines the parameters and data structure for eMMC devices.
 */

#ifndef INCLUDED_NVBOOT_EMMC_PARAM_H
#define INCLUDED_NVBOOT_EMMC_PARAM_H

#include "nvcommon.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/// Defines various data widths supported by eMMC.
typedef enum
{
    /// Specifies a 1 bit interface to eMMC.
    NvBootEmmcDataWidth_1Bit = 0,

    /// Specifies a 4 bit interface to eMMC.
    NvBootEmmcDataWidth_4Bit = 1,

    /// Specifies a 8 bit interface to eMMC.
    NvBootEmmcDataWidth_8Bit = 2,

    NvBootEmmcDataWidth_Num,
    NvBootEmmcDataWidth_Force32 = 0x7FFFFFFF
} NvBootEmmcDataWidth;

// Defines various clock rates to access the eMMC card at.
typedef enum
{
    NvBootEmmcCardClock_Identification = 1,
    NvBootEmmcCardClock_DataTransfer,
    NvBootEmmcCardClock_Num,
    NvBootEmmcCardClock_Force32 = 0x7FFFFFFF
} NvBootEmmcCardClock;

/// Defines the params that can be configured for eMMC devices.
typedef struct NvBootEmmcParamsRec
{
    /**
     * Specifies the clock divider for the HsmmcController clock source. 
     * Hsmmc Controller gets PLL_P clock, which is 432MHz, as a clock source.
     * If it is set to 18, then clock Frequency at which Hsmmc controller runs 
     * is 432/18 = 24MHz.
     */
    NvU32 ClockDivider;

    /// Specifies the data bus width. Supported data widths are 4 bit and 8 bit.
    NvBootEmmcDataWidth DataWidth;
} NvBootEmmcParams;

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_NVBOOT_EMMC_PARAM_H */

