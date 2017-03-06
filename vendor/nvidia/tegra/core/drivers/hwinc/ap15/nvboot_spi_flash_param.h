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
 * Defines the parameters and data structure for SPI FLASH devices.
 */

#ifndef INCLUDED_NVBOOT_SPI_FLASH_PARAM_H
#define INCLUDED_NVBOOT_SPI_FLASH_PARAM_H

#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * Defines the parameters SPI FLASH devices.
 */
typedef struct NvBootSpiFlashParamsRec
{
   /**
    *  Specifies the type of command for read operations.
    *  NV_FALSE specifies a NORMAL_READ Command
    *  NV_TRUE  specifies a FAST_READ   Command
    */
    NvBool ReadCommandTypeFast;

    /**
     * Specifes the clock divider to use.
     * The value is a 7-bit value based on an input clock of 432Mhz.
     * Divider = (432+ DesiredFrequency-1)/DesiredFrequency;
     * Typical values:
     *     NORMAL_READ at 20MHz: 22
     *     FAST_READ   at 33MHz: 14
     *     FAST_READ   at 40MHz: 11
     *     FAST_READ   at 50MHz:  9
     */
    NvU8 ClockDivider;
} NvBootSpiFlashParams;

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_NVBOOT_SPI_FLASH_PARAM_H */

