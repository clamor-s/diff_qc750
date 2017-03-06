/*
 * Copyright (c) 2007-2011 NVIDIA Corporation.  All rights reserved.
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
 * @b Description: Specifies the peripheral connectivity database NvOdmIoAddress entries
 *                 for the peripherals on p852 module.
 *       Peripheral list on p852: TI TPS65861x, , SPI1 for off board ENC28J60 SPI Ethernet module
 */

#include "../../adaptations/pmu/nvodm_pmu_tps6586x_supply_info_table.h"
#include "../../adaptations/tmon/adt7461/nvodm_tmon_adt7461_channel.h"
#include "nvodm_tmon.h"

// RTC voltage rail
static const NvOdmIoAddress s_p852_b00_RtcAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO2 }  /* VDD_RTC -> LCREG */
};

// Core voltage rail
static const NvOdmIoAddress s_p852_b00_CoreAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_DCD0 }  /* VDD_CORE -> DCD1 */
};

// PLLA voltage rail
static const NvOdmIoAddress s_p852_b00_PllAAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO1 } /* AVDDPLLX_1V2 -> D3REG */
};

// PLLM voltage rail
static const NvOdmIoAddress s_p852_b00_PllMAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO1 } /* AVDDPLLX_1V2 -> D3REG */
};

// PLLP voltage rail
static const NvOdmIoAddress s_p852_b00_PllPAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO1 } /* AVDDPLLX_1V2 -> D3REG */
};

// PLLC voltage rail
static const NvOdmIoAddress s_p852_b00_PllCAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO1 } /* AVDDPLLX_1V2 -> D3REG */
};

// PLLE voltage rail
static const NvOdmIoAddress s_p852_b00_PllEAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_DCD1 } /* AVDDPLLE -> D3REG */
};

// PLLU voltage rail
static const NvOdmIoAddress s_p852_b00_PllUAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO1 } /* AVDD_PLLU -> D3REG */
};

// PLLS voltage rail
static const NvOdmIoAddress s_p852_b00_PllSAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO1 } /* AVDDPLLS -> D3REG */
};

// PLLX voltage rail
static const NvOdmIoAddress s_p852_b00_PllXAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO1 } /* AVDDPLLX -> D3REG */
};

// PLL_USB voltage rail
// B00 changes : T20_AVDD_USB_PLL is derived from SM2.
static const NvOdmIoAddress s_p852_b00_PllUsbAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_DCD2 }
};

// OSC voltage rail
static const NvOdmIoAddress s_p852_b00_VddOscAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO4 } /* AVDD_OSC -> DCD2 */
};

// SYS IO voltage rail
static const NvOdmIoAddress s_p852_b00_VddSysAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO4 } /* VDDIO_SYS -> IOREG */
};
/************************ PLL, CPU , RTC ****************************************/
// NVTODO : Add SNOR

// DDR voltage rail
// NVTODO: VDDIO_DDR seems to be always on on P852. Remove this?
static const NvOdmIoAddress s_p852_b00_VddDdrAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_DCD0 }  /* VDDIO_DDR -> DCD2 */
};

// NAND voltage rail
static const NvOdmIoAddress s_p852_b00_VddNandAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_DCD2 } /* VDDIO_NAND -> DCD2 */
};

/*************************** Memory, NAND, NOR *************************************/

// USB - MUX NVTODO : Confirm if this is on P852
//static const NvOdmIoAddress s_p852_b00_UsbMuxAddress[] =
//{
//    {NvOdmIoModule_Usb, 1, 0}
//};


// USB voltage rail
// B00 changes : T20_AVDD_USB is derived from SM2.
static const NvOdmIoAddress s_p852_b00_VddUsbAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_DCD2 }
};

/********************************  USB ************************************************/
// Super power voltage rail for the SOC

// NVTODO: Verify this on P852
static const NvOdmIoAddress s_p852_b00_VddSocAddresses[]=
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_SoC } /* VDD SOC */
};


// PEX_CLK voltage rail
// B00 changes : PEX clock is derived from LDO5.
static const NvOdmIoAddress s_p852_b00_VddPexClkAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO5 } /* VDDIO_PEX_CLK -> VOUT11 */
};

// PMU0
static const NvOdmIoAddress s_p852_b00_Pmu0Addresses[] =
{
    { NvOdmIoModule_I2c_Pmu, 0x00, 0x68 }, /* I2C bus */
};

/*************************************************************************************
 * NVTODO : The following modules are to be moved into E1155 subboard. Will check in *
 * Whistler if the definitions are to be on the processor module or on the baseboard.*
 * As per the convention followed on Whistler all the Voltage Rails are on the board *
 * which has the PMU. We still need to add the voltage rails of the new modules added*
 * on P852 and remove ones that do not exist (like HDMI ?)                           *
 *************************************************************************************/

// MIPI voltage rail
// NVTODO: Doesn't seem to be connected anywhere in schematic
static const NvOdmIoAddress s_p852_b00_VddMipiAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_DCD0 } /* VDD_MIPI -> D4REG */
};

// LCD voltage rail
// B00 changes : VDD_LCD is derived from SM2
static const NvOdmIoAddress s_p852_b00_VddLcdAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_DCD2 }
};

// Audio voltage rail
static const NvOdmIoAddress s_p852_b00_VddAudAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_DCD2 } /* VDDIO_AUDIO -> DCD2 */
};

// DDR_RX voltage rail
static const NvOdmIoAddress s_p852_b00_VddDdrRxAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO9 }  /* VDDIO_RX_DDR(2.7-3.3) -> VOUT1 */
};


// UART voltage rail
static const NvOdmIoAddress s_p852_b00_VddUartAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_DCD2 } /* VDDIO_UART -> DCD2 */
};

// SDIO voltage rail
// BOO changes : derived from 3V3 (SM2)
static const NvOdmIoAddress s_p852_b00_VddSdioAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_DCD2 }
};

// VDAC voltage rail
static const NvOdmIoAddress s_p852_b00_VddVdacAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO6 } /* AVDD_VDAC -> RF2REG */
};

// VI voltage rail
// LCD voltage rail
// B00 changes : VDD_VI is derived from SM2
static const NvOdmIoAddress s_p852_b00_VddViAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_DCD2 }
};

// BB voltage rail
// B00 changes : VDD_BB is derived from SM2
static const NvOdmIoAddress s_p852_b00_VddBbAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_DCD2 }
};

// ULPI USB voltage rail
static const NvOdmIoAddress s_p852_b00_VddUlpiUsbAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_DCD2 } /* VDDIO_BB -> ULPI PHY */
};

// HSIC voltage rail
//static const NvOdmIoAddress s_p852_b00_VddHsicAddresses[] =
//{
//    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_Invalid } /* VDDIO_HSIC -> VOUT20 */
//};


// VBus voltage rail
//static const NvOdmIoAddress s_p852_b00_VddVBusAddresses[] =
//{
//    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_Invalid } /* VDDIO_VBUS -> USBREG */
//};

/*******************************************
 * NVTODO : Decide where the following     *
 * should go. E1155 / P852 ?               *
 *******************************************/

// SPI1 for Spi Ethernet Kitl only
//static const NvOdmIoAddress s_p852_b00_SpiEthernetAddresses[] =
//{
//    { NvOdmIoModule_Spi, 0, 0 },
//    { NvOdmIoModule_Gpio, (NvU32)'l'-'a', 7 },
//};

// p852 LVDS LCD Display
static const NvOdmIoAddress s_p852_b00_LvdsDisplayAddresses[] =
{
    { NvOdmIoModule_Display, 0, 0 },
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO5}, /* VDDIO_LCD/VDDIO_LCD_3V3 -> LDO0 */
    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO6}, /* VDDIO_LCD/VDDIO_LCD_3V3 -> LDO0 */
};

//  HDMI addresses based on Concorde 2 design

// NVTODO: Remove this?
//static const NvOdmIoAddress s_p852_b00_HdmiAddresses[] =
//{
    //{ NvOdmIoModule_Hdmi, 0, 0 },

    //// Display Data Channel (DDC) for Extended Display Identification
    //// Data (EDID)
    //{ NvOdmIoModule_I2c, 0x01, 0xA0 },

    //// HDCP ROM
    //{ NvOdmIoModule_I2c, 0x01, 0x74 },

    ///* AVDD_HDMI -> D1REG */
    //{ NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO4 },
    //{ NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO0},

    ///* lcd i/o rail (for hot plug pin) */
    //{ NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO3 },
//};

// CRT address based on Concorde 2 design
//static const NvOdmIoAddress s_p852_b00_CrtAddresses[] =
//{
    //{ NvOdmIoModule_Crt, 0, 0 },

    // Display Data Channel (DDC) for Extended Display Identification
    // Data (EDID)
    //{ NvOdmIoModule_I2c, 0x01, 0xA0 },

    /* tvdac rail */
    //{ NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO6 },

    /* lcd rail (required for crt out) */
    //{ NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO5 },
//};

// P852 ULPI USB - Configuration of reset pin
static const NvOdmIoAddress s_p852_b00_UlpiUsbAddresses[] =
{
    { NvOdmIoModule_Gpio, (NvU32)'i'-'a', 5 }, /* Reset pin for PHY */
};

static const NvOdmIoAddress s_p852_b00_Tmon0Addresses[] =
{
    { NvOdmIoModule_I2c, 0x01, 0x98 },                      /* I2C_GEN2 bus */
    { NvOdmIoModule_Gpio, (NvU32)'v'-'a', 3 },              /* GPIO Port V and Pin 3 */

    /* Temperature zone mapping */
    { NvOdmIoModule_Tsense, NvOdmTmonZoneID_Core, ADT7461ChannelID_Remote },   /* TSENSOR */
    { NvOdmIoModule_Tsense, NvOdmTmonZoneID_Ambient, ADT7461ChannelID_Local }, /* TSENSOR */
};

