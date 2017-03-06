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
 * @b Description: Specifies the peripheral connectivity database Peripheral entries
 *                 for the peripherals on p852 module.
 *       Peripheral list on p852: TI TPS65861x, SPI1 for off board ENC28J60 SPI Ethernet module
 */
// AP20 doesn't have PLL_D rail.
// PLLD (NV reserved) / Use PLL_U
{
    NV_VDD_PLLD_ODM_ID,
    s_p852_b00_PllUAddresses,
    NV_ARRAY_SIZE(s_p852_b00_PllUAddresses),
    NvOdmPeripheralClass_Other
},

// RTC (NV reserved)
{
    NV_VDD_RTC_ODM_ID,
    s_p852_b00_RtcAddresses,
    NV_ARRAY_SIZE(s_p852_b00_RtcAddresses),
    NvOdmPeripheralClass_Other
},

// CORE (NV reserved)
{
    NV_VDD_CORE_ODM_ID,
    s_p852_b00_CoreAddresses,
    NV_ARRAY_SIZE(s_p852_b00_CoreAddresses),
    NvOdmPeripheralClass_Other
},

// PLLA (NV reserved)
{
    NV_VDD_PLLA_ODM_ID,
    s_p852_b00_PllAAddresses,
    NV_ARRAY_SIZE(s_p852_b00_PllAAddresses),
    NvOdmPeripheralClass_Other
},

// PLLM (NV reserved)
{
    NV_VDD_PLLM_ODM_ID,
    s_p852_b00_PllMAddresses,
    NV_ARRAY_SIZE(s_p852_b00_PllMAddresses),
    NvOdmPeripheralClass_Other
},

// PLLP (NV reserved)
{
    NV_VDD_PLLP_ODM_ID,
    s_p852_b00_PllPAddresses,
    NV_ARRAY_SIZE(s_p852_b00_PllPAddresses),
    NvOdmPeripheralClass_Other
},

// PLLC (NV reserved)
{
    NV_VDD_PLLC_ODM_ID,
    s_p852_b00_PllCAddresses,
    NV_ARRAY_SIZE(s_p852_b00_PllCAddresses),
    NvOdmPeripheralClass_Other
},

// PLLE (NV reserved)
{
    NV_VDD_PLLE_ODM_ID,
    s_p852_b00_PllEAddresses,
    NV_ARRAY_SIZE(s_p852_b00_PllEAddresses),
    NvOdmPeripheralClass_Other
},

// PLLU (NV reserved)
{
    NV_VDD_PLLU_ODM_ID,
    s_p852_b00_PllUAddresses,
    NV_ARRAY_SIZE(s_p852_b00_PllUAddresses),
    NvOdmPeripheralClass_Other
},

// PLLS (NV reserved)
{
    NV_VDD_PLLS_ODM_ID,
    s_p852_b00_PllSAddresses,
    NV_ARRAY_SIZE(s_p852_b00_PllSAddresses),
    NvOdmPeripheralClass_Other
},

// OSC VDD (NV reserved)
{
    NV_VDD_OSC_ODM_ID,
    s_p852_b00_VddOscAddresses,
    NV_ARRAY_SIZE(s_p852_b00_VddOscAddresses),
    NvOdmPeripheralClass_Other
},

// PLLX (NV reserved)
{
    NV_VDD_PLLX_ODM_ID,
    s_p852_b00_PllXAddresses,
    NV_ARRAY_SIZE(s_p852_b00_PllXAddresses),
    NvOdmPeripheralClass_Other
},

// PLL_USB (NV reserved)
{
    NV_VDD_PLL_USB_ODM_ID,
    s_p852_b00_PllUsbAddresses,
    NV_ARRAY_SIZE(s_p852_b00_PllUsbAddresses),
    NvOdmPeripheralClass_Other
},

// System IO VDD (NV reserved)
{
    NV_VDD_SYS_ODM_ID,
    s_p852_b00_VddSysAddresses,
    NV_ARRAY_SIZE(s_p852_b00_VddSysAddresses),
    NvOdmPeripheralClass_Other
},

// USB VDD (NV reserved)
{
    NV_VDD_USB_ODM_ID,
    s_p852_b00_VddUsbAddresses,
    NV_ARRAY_SIZE(s_p852_b00_VddUsbAddresses),
    NvOdmPeripheralClass_Other
},

// MIPI VDD (NV reserved)
{
    NV_VDD_MIPI_ODM_ID,
    s_p852_b00_VddMipiAddresses,
    NV_ARRAY_SIZE(s_p852_b00_VddMipiAddresses),
    NvOdmPeripheralClass_Other
},

// LCD VDD (NV reserved)
{
    NV_VDD_LCD_ODM_ID,
    s_p852_b00_VddLcdAddresses,
    NV_ARRAY_SIZE(s_p852_b00_VddLcdAddresses),
    NvOdmPeripheralClass_Other
},

// AUDIO VDD (NV reserved)
{
    NV_VDD_AUD_ODM_ID,
    s_p852_b00_VddAudAddresses,
    NV_ARRAY_SIZE(s_p852_b00_VddAudAddresses),
    NvOdmPeripheralClass_Other
},

// DDR VDD (NV reserved)
{
    NV_VDD_DDR_ODM_ID,
    s_p852_b00_VddDdrAddresses,
    NV_ARRAY_SIZE(s_p852_b00_VddDdrAddresses),
    NvOdmPeripheralClass_Other
},

// DDR_RX (NV reserved)
{
    NV_VDD_DDR_RX_ODM_ID,
    s_p852_b00_VddDdrRxAddresses,
    NV_ARRAY_SIZE(s_p852_b00_VddDdrRxAddresses),
    NvOdmPeripheralClass_Other
},

// NAND VDD (NV reserved)
{
    NV_VDD_NAND_ODM_ID,
    s_p852_b00_VddNandAddresses,
    NV_ARRAY_SIZE(s_p852_b00_VddNandAddresses),
    NvOdmPeripheralClass_Other
},

// UART VDD (NV reserved)
{
    NV_VDD_UART_ODM_ID,
    s_p852_b00_VddUartAddresses,
    NV_ARRAY_SIZE(s_p852_b00_VddUartAddresses),
    NvOdmPeripheralClass_Other
},

// SDIO VDD (NV reserved)
{
    NV_VDD_SDIO_ODM_ID,
    s_p852_b00_VddSdioAddresses,
    NV_ARRAY_SIZE(s_p852_b00_VddSdioAddresses),
    NvOdmPeripheralClass_Other
},

// VDAC VDD (NV reserved)
{
    NV_VDD_VDAC_ODM_ID,
    s_p852_b00_VddVdacAddresses,
    NV_ARRAY_SIZE(s_p852_b00_VddVdacAddresses),
    NvOdmPeripheralClass_Other
},

// VI VDD (NV reserved)
{
    NV_VDD_VI_ODM_ID,
    s_p852_b00_VddViAddresses,
    NV_ARRAY_SIZE(s_p852_b00_VddViAddresses),
    NvOdmPeripheralClass_Other
},

// BB VDD (NV reserved)
{
    NV_VDD_BB_ODM_ID,
    s_p852_b00_VddBbAddresses,
    NV_ARRAY_SIZE(s_p852_b00_VddBbAddresses),
    NvOdmPeripheralClass_Other
},

#if 0
//  VBUS
{
    NV_VDD_VBUS_ODM_ID,
    s_p852_b00_VddVBusAddresses,
    NV_ARRAY_SIZE(s_p852_b00_VddVBusAddresses),
    NvOdmPeripheralClass_Other
},
#endif

// ULPI USB
{
    NV_VDD_USB2_VBUS_ODM_ID,
    s_p852_b00_VddUlpiUsbAddresses,
    NV_ARRAY_SIZE(s_p852_b00_VddUlpiUsbAddresses),
    NvOdmPeripheralClass_Other
},

//SOC
{
    NV_VDD_SoC_ODM_ID,
    s_p852_b00_VddSocAddresses,
    NV_ARRAY_SIZE(s_p852_b00_VddSocAddresses),
    NvOdmPeripheralClass_Other
},

//  PMU0
{
    NV_ODM_GUID('t','p','s','6','5','8','6','x'),
    s_p852_b00_Pmu0Addresses,
    NV_ARRAY_SIZE(s_p852_b00_Pmu0Addresses),
    NvOdmPeripheralClass_Other
},

/*********************************************
 * NVTODO : Check these. Refer _addresses.h  *
 *********************************************/

// p852 RGB LCD Display
{
    NV_ODM_GUID('R','G','B','_','W','V','G','A'), // RGB panel
    s_p852_b00_LvdsDisplayAddresses,
    NV_ARRAY_SIZE(s_p852_b00_LvdsDisplayAddresses),
    NvOdmPeripheralClass_Display
},

// HDMI (based on Concorde 2 design)
//{
//    NV_ODM_GUID('f','f','a','2','h','d','m','i'),
//    s_p852_b00_HdmiAddresses,
//    NV_ARRAY_SIZE(s_p852_b00_HdmiAddresses),
//    NvOdmPeripheralClass_Display
//},

// CRT (based on Concorde 2 design)
//{
//    NV_ODM_GUID('f','f','a','_','_','c','r','t'),
//    s_p852_b00_CrtAddresses,
//    NV_ARRAY_SIZE(s_p852_b00_CrtAddresses),
//    NvOdmPeripheralClass_Display
//},

// USB Mux J7A1 and J6A1
//{
//   NV_ODM_GUID('u','s','b','m','x','J','7','6'),
//    s_p852_b00_UsbMuxAddress,
//    NV_ARRAY_SIZE(s_p852_b00_UsbMuxAddress),
//    NvOdmPeripheralClass_Other
//},

//  SMSC83340 ULPI USB PHY
{
    NV_ODM_GUID('s','m','s','8','3','3','4','0'),
    s_p852_b00_UlpiUsbAddresses,
    NV_ARRAY_SIZE(s_p852_b00_UlpiUsbAddresses),
    NvOdmPeripheralClass_Other
},

// Temperature Monitor (TMON)
{
    NV_ODM_GUID('a','d','t','7','4','6','1',' '),
    s_p852_b00_Tmon0Addresses,
    NV_ARRAY_SIZE(s_p852_b00_Tmon0Addresses),
    NvOdmPeripheralClass_Other
},

// NOTE: This list *must* end with a trailing comma.
