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
 *                 database NvOdmIoAddress entries for the E906
 *                 LCD Module.
 */

#include "pmu/max8907b/max8907b_supply_info_table.h"

// Main LCD
static const NvOdmIoAddress s_ffaMainDisplayAddresses[] = 
{
    { NvOdmIoModule_Display, 0, 0, 0 },
    { NvOdmIoModule_Spi, 0x2, 0x2, 0 },                        // TBD (this is a guess)
    { NvOdmIoModule_Vdd, 0x00, Max8907bPmuSupply_LX_V3, 0 },   /* VDDIO_LCD -> V3 */
    { NvOdmIoModule_Vdd, 0x00, Max8907bPmuSupply_LDO5, 0 },    /* AVDD_LCD_1 -> VOUT5 */
    { NvOdmIoModule_Vdd, 0x00, Max8907bPmuSupply_LDO19, 0  },  /* AVDD_LCD_2 -> VOUT19 */
};

// Panel 86
// IMPORTANT: the board mux must be set to 0x5C and the display pinmux must
// be set to config5.
static const NvOdmIoAddress s_SonyPanelAddresses[] =
{
    { NvOdmIoModule_Gpio, (NvU32)'m'-'a', 3, 0 },   /* Reset */
    { NvOdmIoModule_Gpio, (NvU32)'b'-'a', 2, 0 },   /* On/off */
    { NvOdmIoModule_Gpio, (NvU32)'n'-'a', 4, 0 },   /* ModeSelect */
    { NvOdmIoModule_Gpio, (NvU32)'j'-'a', 3, 0 },   /* hsync */
    { NvOdmIoModule_Gpio, (NvU32)'j'-'a', 4, 0 },   /* vsync */
    { NvOdmIoModule_Gpio, (NvU32)'c'-'a', 1, 0 },   /* PowerEnable */
    { NvOdmIoModule_Gpio, 0xFFFFFFFF, 0xFFFFFFFF, 0 }, /* slin */
    { NvOdmIoModule_Display, 0, 0, 0 },
};

// DSI LCD
// WARNING: Whistler's board personality needs to be set to 077 for the
// reset gpio pin to work
static const NvOdmIoAddress s_DsiAddresses[] = 
{
    { NvOdmIoModule_Display, 0, 0, 0 },

    { NvOdmIoModule_Gpio, (NvU32)('c' - 'a'), 1, 0 },

    { NvOdmIoModule_Vdd, 0x00, Max8907bPmuSupply_LX_V3, 0 },   /* VDDIO_LCD -> V3 */
    { NvOdmIoModule_Vdd, 0x00, Max8907bPmuSupply_LDO5, 0 },    /* AVDD_LCD_1 -> VOUT5 */
    { NvOdmIoModule_Vdd, 0x00, Max8907bPmuSupply_LDO19, 0  },  /* AVDD_LCD_2 -> VOUT19 */
    { NvOdmIoModule_Vdd, 0x00, Max8907bPmuSupply_LDO17, 0  },  /* MIPI DSI 1.2V */
};

// TouchPanel
static const NvOdmIoAddress s_ffaTouchPanelAddresses[] = 
{
    { NvOdmIoModule_I2c, 0x00, 0x20, 0 },/* I2C device address is 0x20 */
    { NvOdmIoModule_Gpio, 'c' - 'a', 6, 0}, /* GPIO Port V and Pin 3 */
    { NvOdmIoModule_Vdd, 0x00, Max8907bPmuSupply_LDO19, 0 }
};
