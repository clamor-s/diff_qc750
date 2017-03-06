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
 *         Implementation of the ODM Peripheral Discovery API</b>
 *
 * @b Description: Specifies the peripheral connectivity database Peripheral
 *                 entries for the peripherals on E1291 module.
 */

// PLLD (NV reserved)  It is removed from Ap20, so moving to PLL_A_P_C_S
{
    NV_VDD_PLLD_ODM_ID,
    s_AVddPllAPCSAddresses,
    NV_ARRAY_SIZE(s_AVddPllAPCSAddresses),
    NvOdmPeripheralClass_Other
},

//SOC (NV reserved) It is removed on T30, so setting to VddIoSys
{
    NV_VDD_SoC_ODM_ID,
    s_VddIoSysAddresses,
    NV_ARRAY_SIZE(s_VddIoSysAddresses),
    NvOdmPeripheralClass_Other
},

// MIPI VDD (NV reserved) / MIPI and DSI are combined in T30
{
    NV_VDD_MIPI_ODM_ID,
    s_AVddVDsiCsiAddresses,
    NV_ARRAY_SIZE(s_AVddVDsiCsiAddresses),
    NvOdmPeripheralClass_Other
},

// DDR VDD (NV reserved)
{
    NV_VDD_DDR_ODM_ID,
    s_EnVddioDdr1V2Addresses,
    NV_ARRAY_SIZE(s_EnVddioDdr1V2Addresses),
    NvOdmPeripheralClass_Other
},

// RTC (NV reserved)
{
    NV_VDD_RTC_ODM_ID,
    s_VddRtcAddresses,
    NV_ARRAY_SIZE(s_VddRtcAddresses),
    NvOdmPeripheralClass_Other
},

// CORE (NV reserved)
{
    NV_VDD_CORE_ODM_ID,
    s_VddCoreAddresses,
    NV_ARRAY_SIZE(s_VddCoreAddresses),
    NvOdmPeripheralClass_Other
},

// CPU (NV reserved)
{
    NV_VDD_CPU_ODM_ID,
    s_VddCpuPmuAddresses,
    NV_ARRAY_SIZE(s_VddCpuPmuAddresses),
    NvOdmPeripheralClass_Other
},

// PLLA (NV reserved)
{
    NV_VDD_PLLA_ODM_ID,
    s_AVddPllAPCSAddresses,
    NV_ARRAY_SIZE(s_AVddPllAPCSAddresses),
    NvOdmPeripheralClass_Other
},

// PLLM (NV reserved)
{
    NV_VDD_PLLM_ODM_ID,
    s_AVddPllMAddresses,
    NV_ARRAY_SIZE(s_AVddPllMAddresses),
    NvOdmPeripheralClass_Other
},

// PLLP (NV reserved)
{
    NV_VDD_PLLP_ODM_ID,
    s_AVddPllAPCSAddresses,
    NV_ARRAY_SIZE(s_AVddPllAPCSAddresses),
    NvOdmPeripheralClass_Other
},

// PLLC (NV reserved)
{
    NV_VDD_PLLC_ODM_ID,
    s_AVddPllAPCSAddresses,
    NV_ARRAY_SIZE(s_AVddPllAPCSAddresses),
    NvOdmPeripheralClass_Other
},

// PLLE (NV reserved)
{
    NV_VDD_PLLE_ODM_ID,
    s_AVddPllEAddresses,
    NV_ARRAY_SIZE(s_AVddPllEAddresses),
    NvOdmPeripheralClass_Other
},

// PLLU (NV reserved)
{
    NV_VDD_PLLU_ODM_ID,
    s_AVddPllUDAddresses,
    NV_ARRAY_SIZE(s_AVddPllUDAddresses),
    NvOdmPeripheralClass_Other
},

// PLLU1 (NV reserved)
{
    NV_VDD_PLLU1_ODM_ID,
    s_AVddPllUD2Addresses,
    NV_ARRAY_SIZE(s_AVddPllUD2Addresses),
    NvOdmPeripheralClass_Other
},

// PLLS (NV reserved)
{
    NV_VDD_PLLS_ODM_ID,
    s_AVddPllAPCSAddresses,
    NV_ARRAY_SIZE(s_AVddPllAPCSAddresses),
    NvOdmPeripheralClass_Other
},

// HDMI PLL (NV reserved)
{
    NV_VDD_PLLHDMI_ODM_ID,
    s_AVddHdmiPllAddresses,
    NV_ARRAY_SIZE(s_AVddHdmiPllAddresses),
    NvOdmPeripheralClass_Other
},

// OSC VDD (NV reserved)
{
    NV_VDD_OSC_ODM_ID,
    s_AVddOscAddresses,
    NV_ARRAY_SIZE(s_AVddOscAddresses),
    NvOdmPeripheralClass_Other
},

// PLLX (NV reserved)
{
    NV_VDD_PLLX_ODM_ID,
    s_AVddPllXAddresses,
    NV_ARRAY_SIZE(s_AVddPllXAddresses),
    NvOdmPeripheralClass_Other
},

// PLL_USB (NV reserved)
{
    NV_VDD_PLL_USB_ODM_ID,
    s_AVddUsbPllAddresses,
    NV_ARRAY_SIZE(s_AVddUsbPllAddresses),
    NvOdmPeripheralClass_Other
},

// System IO VDD (NV reserved)
{
    NV_VDD_SYS_ODM_ID,
    s_VddIoSysAddresses,
    NV_ARRAY_SIZE(s_VddIoSysAddresses),
    NvOdmPeripheralClass_Other
},

// USB VDD (NV reserved)
{
    NV_VDD_USB_ODM_ID,
    s_AVddUsbAddresses,
    NV_ARRAY_SIZE(s_AVddUsbAddresses),
    NvOdmPeripheralClass_Other
},

// HDMI VDD (NV reserved)
{
    NV_VDD_HDMI_ODM_ID,
    s_AVddHdmiAddresses,
    NV_ARRAY_SIZE(s_AVddHdmiAddresses),
    NvOdmPeripheralClass_Other
},

// Power for HDMI Hotplug
{
    NV_VDD_HDMI_INT_ID,
    s_VddIoLcdPmuAddresses,
    NV_ARRAY_SIZE(s_VddIoLcdPmuAddresses),
    NvOdmPeripheralClass_Other,
},

// LCD VDD (NV reserved)
{
    NV_VDD_LCD_ODM_ID,
    s_VcoreLcdAddresses,
    NV_ARRAY_SIZE(s_VcoreLcdAddresses),
    NvOdmPeripheralClass_Other
},

// AUDIO VDD (NV reserved)
{
    NV_VDD_AUD_ODM_ID,
    s_AVddAudioAddresses,
    NV_ARRAY_SIZE(s_AVddAudioAddresses),
    NvOdmPeripheralClass_Other
},

// DDR_RX (NV reserved)
{
    NV_VDD_DDR_RX_ODM_ID,
    s_VddDdrRxAddresses,
    NV_ARRAY_SIZE(s_VddDdrRxAddresses),
    NvOdmPeripheralClass_Other
},

// NAND VDD (NV reserved)
{
    NV_VDD_NAND_ODM_ID,
    s_VCoreNandAddresses,
    NV_ARRAY_SIZE(s_VCoreNandAddresses),
    NvOdmPeripheralClass_Other
},

// UART VDD (NV reserved)
{
    NV_VDD_UART_ODM_ID,
    s_VddIoUartAddresses,
    NV_ARRAY_SIZE(s_VddIoUartAddresses),
    NvOdmPeripheralClass_Other
},

// SDIO VDD (NV reserved)
{
    NV_VDD_SDIO_ODM_ID,
    s_VddIoSdmmc3Addresses,
    NV_ARRAY_SIZE(s_VddIoSdmmc3Addresses),
    NvOdmPeripheralClass_Other
},

// VDAC VDD (NV reserved)
{
    NV_VDD_VDAC_ODM_ID,
    s_AVddVdacAddresses,
    NV_ARRAY_SIZE(s_AVddVdacAddresses),
    NvOdmPeripheralClass_Other
},

// VI VDD (NV reserved)
{
    NV_VDD_VI_ODM_ID,
    s_VddIoViAddresses,
    NV_ARRAY_SIZE(s_VddIoViAddresses),
    NvOdmPeripheralClass_Other
},

// BB VDD (NV reserved)
{
    NV_VDD_BB_ODM_ID,
    s_VddIoBBAddresses,
    NV_ARRAY_SIZE(s_VddIoBBAddresses),
    NvOdmPeripheralClass_Other
},

//  VBUS for USB1
{
    NV_VDD_VBUS_ODM_ID,
    s_ffaVddUsb1VBusAddresses,
    NV_ARRAY_SIZE(s_ffaVddUsb1VBusAddresses),
    NvOdmPeripheralClass_Other
},

//  VBUS for USB3
{
    NV_VDD_USB3_VBUS_ODM_ID,
    s_ffaVddUsb3VBusAddresses,
    NV_ARRAY_SIZE(s_ffaVddUsb3VBusAddresses),
    NvOdmPeripheralClass_Other
},

//  PMU0
{
    NV_ODM_GUID('t','p','s','6','5','9','1','x'),
    s_Pmu0Addresses,
    NV_ARRAY_SIZE(s_Pmu0Addresses),
    NvOdmPeripheralClass_Other
},

//  tps6236x
{
    NV_ODM_GUID('t','p','s','6','2','3','6','x'),
    s_Tps6236xddresses,
    NV_ARRAY_SIZE(s_Tps6236xddresses),
    NvOdmPeripheralClass_Other
},

//  Ricoh583
{
    NV_ODM_GUID('r','i','c','o','h','5','8','3'),
    s_Rico583Addresses,
    NV_ARRAY_SIZE(s_Rico583Addresses),
    NvOdmPeripheralClass_Other
},

//  TCA6416
{
    NV_ODM_GUID('t','c','a','6','4','1','6',' '),
    s_Tca6416Addresses,
    NV_ARRAY_SIZE(s_Tca6416Addresses),
    NvOdmPeripheralClass_Other
},

//  Max77663
{
    NV_ODM_GUID('m','a','x','7','7','6','6','3'),
    s_Max77663Addresses,
    NV_ARRAY_SIZE(s_Max77663Addresses),
    NvOdmPeripheralClass_Other
},

// Cardhu
{
    NV_ODM_GUID('c','a','r','d','h','u','p','m'),
    s_Pmu0Addresses,
    NV_ARRAY_SIZE(s_Pmu0Addresses),
    NvOdmPeripheralClass_Other
},

// test rail
{
    NV_ODM_GUID('T','E','S','T','R','A','I','L'),
    s_AllRailAddresses,
    NV_ARRAY_SIZE(s_AllRailAddresses),
    NvOdmPeripheralClass_Other,
},

// NOTE: This list *must* end with a trailing comma.
