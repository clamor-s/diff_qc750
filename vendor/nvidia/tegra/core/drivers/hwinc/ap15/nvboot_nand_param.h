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
 * Defines the parameters and data structure for NAND devices.
 */

#ifndef INCLUDED_NVBOOT_NAND_PARAM_H
#define INCLUDED_NVBOOT_NAND_PARAM_H

#include "nvcommon.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/// Defines Address format of Nand Flash.
typedef enum
{
    /// Specifies the Column Row address format.
    NvBootNandAddressFormat_ColumnRow = 1,

    /// Specifies the Column Page Block address format.
    NvBootNandAddressFormat_ColumnPageBlock,

    NvBootNandAddressFormat_Count,
    NvBootNandAddressFormat_Force32 = 0x7FFFFFFF
} NvBootNandAddressFormat;

/**
 * Defines HW Ecc options that can be selected.
 * Rs4 means Reed-Solomon ECC algorithm with 4 symbol error correction.
 * Rs6 and Rs8 are with 6 and 8 symbol corrections respectively.
 */
typedef enum
{
    // This should be starting from zero. Don't change.

    /// Specifies no ECC.
    NvBootNandEccSelection_Off = 0,

    /// Specifies Reed-Solomon ECC w/4 symbol correction.
    NvBootNandEccSelection_Rs4,

    /// Specifies Reed-Solomon ECC w/6 symbol correction.
    NvBootNandEccSelection_Rs6,

    /// Specifies Reed-Solomon ECC w/8 symbol correction.
    NvBootNandEccSelection_Rs8,

    NvBootNandEccSelection_Count,
    NvBootNandEccSelection_Force32 = 0x7FFFFFFF
} NvBootNandEccSelection;

/// Defines the params that can be configured for NAND devices.
typedef struct NvBootNandParamsRec
{
    /// Specifies the number of address cycles.
    NvU8 NumOfAddressCycles;

    /// Specifies the address format.
    NvBootNandAddressFormat AddressFormat;

    /// Specifies the value to be programmed to Nand Timing Register 1.
    NvU32 NandTiming;

    /// Specifies the value to be programmed to Nand Timing Register 2.
    NvU32 NandTiming2;

    /**
      * Specifies the clock divider for the PLL_P 432MHz source. 
      * If it is set to 18, then clock source to Nand controller is 
      * 432 / 18 = 24MHz.
      */
    NvU8 ClockDivider;

    /// Specifies the selected ECC.
    NvBootNandEccSelection EccSelection;

    /// Specifies whether the ONFI support is enabled (NV_TRUE) or
    /// disabled (NV_FALSE).
    NvBool EnableOnfiSupport;
} NvBootNandParams;

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_NVBOOT_NAND_PARAM_H */
