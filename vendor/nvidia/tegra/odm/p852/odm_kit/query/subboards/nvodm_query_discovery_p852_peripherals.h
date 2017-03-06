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
    s_p852PllUAddresses,
    NV_ARRAY_SIZE(s_p852PllUAddresses),
    NvOdmPeripheralClass_Other
},

// RTC (NV reserved)
{
    NV_VDD_RTC_ODM_ID,
    s_p852RtcAddresses,
    NV_ARRAY_SIZE(s_p852RtcAddresses),
    NvOdmPeripheralClass_Other
},

// CORE (NV reserved)
{
    NV_VDD_CORE_ODM_ID,
    s_p852CoreAddresses,
    NV_ARRAY_SIZE(s_p852CoreAddresses),
    NvOdmPeripheralClass_Other
},

// CPU (NV reserved)
/* NVTODO : See _addresses.h */
{
    NV_VDD_CPU_ODM_ID,
    s_ffaCpuAddresses,
    NV_ARRAY_SIZE(s_ffaCpuAddresses),
    NvOdmPeripheralClass_Other
},

// PLLA (NV reserved)
{
    NV_VDD_PLLA_ODM_ID,
    s_p852PllAAddresses,
    NV_ARRAY_SIZE(s_p852PllAAddresses),
    NvOdmPeripheralClass_Other
},

// PLLM (NV reserved)
{
    NV_VDD_PLLM_ODM_ID,
    s_p852PllMAddresses,
    NV_ARRAY_SIZE(s_p852PllMAddresses),
    NvOdmPeripheralClass_Other
},

// PLLP (NV reserved)
{
    NV_VDD_PLLP_ODM_ID,
    s_p852PllPAddresses,
    NV_ARRAY_SIZE(s_p852PllPAddresses),
    NvOdmPeripheralClass_Other
},

// PLLC (NV reserved)
{
    NV_VDD_PLLC_ODM_ID,
    s_p852PllCAddresses,
    NV_ARRAY_SIZE(s_p852PllCAddresses),
    NvOdmPeripheralClass_Other
},

// PLLE (NV reserved)
{
    NV_VDD_PLLE_ODM_ID,
    s_p852PllEAddresses,
    NV_ARRAY_SIZE(s_p852PllEAddresses),
    NvOdmPeripheralClass_Other
},

// PLLU (NV reserved)
{
    NV_VDD_PLLU_ODM_ID,
    s_p852PllUAddresses,
    NV_ARRAY_SIZE(s_p852PllUAddresses),
    NvOdmPeripheralClass_Other
},

// PLLS (NV reserved)
{
    NV_VDD_PLLS_ODM_ID,
    s_p852PllSAddresses,
    NV_ARRAY_SIZE(s_p852PllSAddresses),
    NvOdmPeripheralClass_Other
},

// OSC VDD (NV reserved)
{
    NV_VDD_OSC_ODM_ID,
    s_p852VddOscAddresses,
    NV_ARRAY_SIZE(s_p852VddOscAddresses),
    NvOdmPeripheralClass_Other
},

// PLLX (NV reserved)
{
    NV_VDD_PLLX_ODM_ID,
    s_p852PllXAddresses,
    NV_ARRAY_SIZE(s_p852PllXAddresses),
    NvOdmPeripheralClass_Other
},

// PLL_USB (NV reserved)
{
    NV_VDD_PLL_USB_ODM_ID,
    s_p852PllUsbAddresses,
    NV_ARRAY_SIZE(s_p852PllUsbAddresses),
    NvOdmPeripheralClass_Other
},

// System IO VDD (NV reserved)
{
    NV_VDD_SYS_ODM_ID,
    s_p852VddSysAddresses,
    NV_ARRAY_SIZE(s_p852VddSysAddresses),
    NvOdmPeripheralClass_Other
},

// USB VDD (NV reserved)
{
    NV_VDD_USB_ODM_ID,
    s_p852VddUsbAddresses,
    NV_ARRAY_SIZE(s_p852VddUsbAddresses),
    NvOdmPeripheralClass_Other
},

// MIPI VDD (NV reserved)
{
    NV_VDD_MIPI_ODM_ID,
    s_p852VddMipiAddresses,
    NV_ARRAY_SIZE(s_p852VddMipiAddresses),
    NvOdmPeripheralClass_Other
},

// LCD VDD (NV reserved)
{
    NV_VDD_LCD_ODM_ID,
    s_p852VddLcdAddresses,
    NV_ARRAY_SIZE(s_p852VddLcdAddresses),
    NvOdmPeripheralClass_Other
},

// AUDIO VDD (NV reserved)
{
    NV_VDD_AUD_ODM_ID,
    s_p852VddAudAddresses,
    NV_ARRAY_SIZE(s_p852VddAudAddresses),
    NvOdmPeripheralClass_Other
},

// DDR VDD (NV reserved)
{
    NV_VDD_DDR_ODM_ID,
    s_p852VddDdrAddresses,
    NV_ARRAY_SIZE(s_p852VddDdrAddresses),
    NvOdmPeripheralClass_Other
},

// DDR_RX (NV reserved)
{
    NV_VDD_DDR_RX_ODM_ID,
    s_p852VddDdrRxAddresses,
    NV_ARRAY_SIZE(s_p852VddDdrRxAddresses),
    NvOdmPeripheralClass_Other
},

// NAND VDD (NV reserved)
{
    NV_VDD_NAND_ODM_ID,
    s_p852VddNandAddresses,
    NV_ARRAY_SIZE(s_p852VddNandAddresses),
    NvOdmPeripheralClass_Other
},

// UART VDD (NV reserved)
{
    NV_VDD_UART_ODM_ID,
    s_p852VddUartAddresses,
    NV_ARRAY_SIZE(s_p852VddUartAddresses),
    NvOdmPeripheralClass_Other
},

// SDIO VDD (NV reserved)
{
    NV_VDD_SDIO_ODM_ID,
    s_p852VddSdioAddresses,
    NV_ARRAY_SIZE(s_p852VddSdioAddresses),
    NvOdmPeripheralClass_Other
},

// VDAC VDD (NV reserved)
{
    NV_VDD_VDAC_ODM_ID,
    s_p852VddVdacAddresses,
    NV_ARRAY_SIZE(s_p852VddVdacAddresses),
    NvOdmPeripheralClass_Other
},

// VI VDD (NV reserved)
{
    NV_VDD_VI_ODM_ID,
    s_p852VddViAddresses,
    NV_ARRAY_SIZE(s_p852VddViAddresses),
    NvOdmPeripheralClass_Other
},

// BB VDD (NV reserved)
{
    NV_VDD_BB_ODM_ID,
    s_p852VddBbAddresses,
    NV_ARRAY_SIZE(s_p852VddBbAddresses),
    NvOdmPeripheralClass_Other
},

#if 0
//  VBUS
{
    NV_VDD_VBUS_ODM_ID,
    s_p852VddVBusAddresses,
    NV_ARRAY_SIZE(s_p852VddVBusAddresses),
    NvOdmPeripheralClass_Other
},
#endif

// ULPI USB
{
    NV_VDD_USB2_VBUS_ODM_ID,
    s_p852VddUlpiUsbAddresses,
    NV_ARRAY_SIZE(s_p852VddUlpiUsbAddresses),
    NvOdmPeripheralClass_Other
},

//SOC
{
    NV_VDD_SoC_ODM_ID,
    s_p852VddSocAddresses,
    NV_ARRAY_SIZE(s_p852VddSocAddresses),
    NvOdmPeripheralClass_Other
},

//  PMU0
{
    NV_ODM_GUID('t','p','s','6','5','8','6','x'),
    s_Pmu0Addresses,
    NV_ARRAY_SIZE(s_Pmu0Addresses),
    NvOdmPeripheralClass_Other
},

/*********************************************
 * NVTODO : Check these. Refer _addresses.h  *
 *********************************************/

// p852 RGB LCD Display
{
    NV_ODM_GUID('R','G','B','_','W','V','G','A'), // RGB panel
    s_p852LvdsDisplayAddresses,
    NV_ARRAY_SIZE(s_p852LvdsDisplayAddresses),
    NvOdmPeripheralClass_Display
},

// HDMI (based on Concorde 2 design)
//{
//    NV_ODM_GUID('f','f','a','2','h','d','m','i'),
//    s_p852HdmiAddresses,
//    NV_ARRAY_SIZE(s_p852HdmiAddresses),
//    NvOdmPeripheralClass_Display
//},

// CRT (based on Concorde 2 design)
//{
//    NV_ODM_GUID('f','f','a','_','_','c','r','t'),
//    s_p852CrtAddresses,
//    NV_ARRAY_SIZE(s_p852CrtAddresses),
//    NvOdmPeripheralClass_Display
//},

// USB Mux J7A1 and J6A1
//{
//   NV_ODM_GUID('u','s','b','m','x','J','7','6'),
//    s_p852UsbMuxAddress,
//    NV_ARRAY_SIZE(s_p852UsbMuxAddress),
//    NvOdmPeripheralClass_Other
//},

//  SMSC83340 ULPI USB PHY
{
    NV_ODM_GUID('s','m','s','8','3','3','4','0'),
    s_p852UlpiUsbAddresses,
    NV_ARRAY_SIZE(s_p852UlpiUsbAddresses),
    NvOdmPeripheralClass_Other
},

// Temperature Monitor (TMON)
{
    NV_ODM_GUID('a','d','t','7','4','6','1',' '),
    s_Tmon0Addresses,
    NV_ARRAY_SIZE(s_Tmon0Addresses),
    NvOdmPeripheralClass_Other
},

// NOTE: This list *must* end with a trailing comma.
