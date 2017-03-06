/*
 * Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 *
 * @file
 * <b>NVIDIA APX ODM Kit::
 *         Implementation of the ODM Peripheral Discovery API for E1294</b>
 *
 * @b Description: Specifies the peripheral connectivity database NvOdmIoAddress
 *                 entries for the peripherals on E1294 module.
 */

#include "../nvodm_query_kbc_gpio_def.h"

// Key Pad
static const NvOdmIoAddress s_KeyPadAddresses[] =
{
    // instance = 1 indicates Column info.
    // instance = 0 indicates Row info.
    // address holds KBC pin number used for row/column.

    // All Row info has to be defined contiguously from 0 to max.
    { NvOdmIoModule_Kbd, 0x00, NvOdmKbcGpioPin_KBRow0, 0}, // Row 0
    { NvOdmIoModule_Kbd, 0x00, NvOdmKbcGpioPin_KBRow1, 0}, // Row 1

    // All Column info has to be defined contiguously from 0 to max.
    { NvOdmIoModule_Kbd, 0x01, NvOdmKbcGpioPin_KBCol0, 0}, // Column 0
    { NvOdmIoModule_Kbd, 0x01, NvOdmKbcGpioPin_KBCol1, 0}, // Column 1
    { NvOdmIoModule_Kbd, 0x01, NvOdmKbcGpioPin_KBCol2, 0}, // Column 2
    { NvOdmIoModule_Kbd, 0x01, NvOdmKbcGpioPin_KBCol3, 0}, // Column 3
};

//  LVDS LCD Display
static const NvOdmIoAddress s_LvdsDisplayAddresses[] =
{
    { NvOdmIoModule_Display, 0, 0, 0 },
    { NvOdmIoModule_I2c, 0x00, 0xA0, 0 },
    { NvOdmIoModule_Pwm, 0x00, 0, 0 },
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_LCD_PANEL_3V3, 0 },
    //{ NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO4, 0},     /* VDDIO_LCD (AON:VDD_1V8) */
    //{ NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO3, 0 },    /* VDD_LVDS (VDD_3V3) */
};

