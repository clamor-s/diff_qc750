/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * <b>NVIDIA APX ODM Kit::
 *         Implementation of the ODM Peripheral Discovery API</b>
 *
 * @b Description: Specifies the peripheral connectivity
 *                 database NvOdmIoAddress entries for the E1129
 *                 keypad module.
 */
#include "../nvodm_query_kbc_gpio_def.h"

// Key Pad
static const NvOdmIoAddress s_KeyPadAddresses[] =
{
    // instance = 1 indicates Column info.
    // instance = 0 indicates Row info.
    // address holds KBC pin number used for row/column.

    // All Row info has to be defined contiguously from 0 to max.
    { NvOdmIoModule_Kbd,0x00, NvOdmKbcGpioPin_KBRow0, 0}, // Row 0
    { NvOdmIoModule_Kbd,0x00, NvOdmKbcGpioPin_KBRow1, 0}, // Row 1
    { NvOdmIoModule_Kbd,0x00 ,NvOdmKbcGpioPin_KBRow2, 0}, // Row 2

    // All Column info has to be defined contiguously from 0 to max.
    { NvOdmIoModule_Kbd,0x01, NvOdmKbcGpioPin_KBCol0, 0}, // Column 0
    { NvOdmIoModule_Kbd,0x01, NvOdmKbcGpioPin_KBCol1, 0}, // Column 1
};

// s_ffa ScrollWheel...  only supported for personality 1
static const NvOdmIoAddress s_ffaScrollWheelAddresses[] =
{
    { NvOdmIoModule_Gpio, 0x10, 0x3, 0 }, // GPIO Port q - Pin3
    { NvOdmIoModule_Gpio, 0x11, 0x3, 0 }, // GpIO Port r - Pin 3
    { NvOdmIoModule_Gpio, 0x10, 0x5, 0 }, // GPIO Port q - Pin 5
    { NvOdmIoModule_Gpio, 0x10, 0x4, 0 }, // GPIO Port q - Pin 4
};

