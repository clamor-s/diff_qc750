/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
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
 *                 entries for the peripherals on E1565 module.
 */
{
    SENSOR_T8EV5_GUID,
    s_ffaImagerT8EV5Addresses,
    NV_ARRAY_SIZE(s_ffaImagerT8EV5Addresses),
    NvOdmPeripheralClass_Imager
},

{
    SENSOR_YUV_GC0308_GUID,
    s_ffaImagerGC0308Addresses,
    NV_ARRAY_SIZE(s_ffaImagerGC0308Addresses),
    NvOdmPeripheralClass_Imager
},

// PLLD (NV reserved)  It is removed from Ap20, so moving to PLL_A_P_C_S
{
    NV_VDD_PLLD_ODM_ID,
    s_AVddPllsAddresses,
    NV_ARRAY_SIZE(s_AVddPllsAddresses),
    NvOdmPeripheralClass_Other
},

// SOC (NV reserved) It is removed on T30, so setting to VddIoSys
{
    NV_VDD_SoC_ODM_ID,
    s_VddIoSysAddresses,
    NV_ARRAY_SIZE(s_VddIoSysAddresses),
    NvOdmPeripheralClass_Other
},

// MIPI VDD (NV reserved) / MIPI and DSI are combined in T30
{
    NV_VDD_MIPI_ODM_ID,
    s_AVddDsiCsiAddresses,
    NV_ARRAY_SIZE(s_AVddDsiCsiAddresses),
    NvOdmPeripheralClass_Other
},

// DDR VDD (NV reserved)
{
    NV_VDD_DDR_ODM_ID,
    s_VddDdr3L1V35Addresses,
    NV_ARRAY_SIZE(s_VddDdr3L1V35Addresses),
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
    s_VddCpuAddresses,
    NV_ARRAY_SIZE(s_VddCpuAddresses),
    NvOdmPeripheralClass_Other
},

// PLLA (NV reserved)
{
    NV_VDD_PLLA_ODM_ID,
    s_AVddPllsAddresses,
    NV_ARRAY_SIZE(s_AVddPllsAddresses),
    NvOdmPeripheralClass_Other
},

// PLLM (NV reserved)
{
    NV_VDD_PLLM_ODM_ID,
    s_AVddPllsAddresses,
    NV_ARRAY_SIZE(s_AVddPllsAddresses),
    NvOdmPeripheralClass_Other
},

// PLLP (NV reserved)
{
    NV_VDD_PLLP_ODM_ID,
    s_AVddPllsAddresses,
    NV_ARRAY_SIZE(s_AVddPllsAddresses),
    NvOdmPeripheralClass_Other
},

// PLLC (NV reserved)
{
    NV_VDD_PLLC_ODM_ID,
    s_AVddPllsAddresses,
    NV_ARRAY_SIZE(s_AVddPllsAddresses),
    NvOdmPeripheralClass_Other
},

// PLLE (NV reserved)
{
    NV_VDD_PLLE_ODM_ID,
    s_AVddPllsAddresses,
    NV_ARRAY_SIZE(s_AVddPllsAddresses),
    NvOdmPeripheralClass_Other
},

// PLLU (NV reserved)
{
    NV_VDD_PLLU_ODM_ID,
    s_AVddPllsAddresses,
    NV_ARRAY_SIZE(s_AVddPllsAddresses),
    NvOdmPeripheralClass_Other
},

// PLLU1 (NV reserved)
{
    NV_VDD_PLLU1_ODM_ID,
    s_AVddPllsAddresses,
    NV_ARRAY_SIZE(s_AVddPllsAddresses),
    NvOdmPeripheralClass_Other
},

// PLLS (NV reserved)
{
    NV_VDD_PLLS_ODM_ID,
    s_AVddPllsAddresses,
    NV_ARRAY_SIZE(s_AVddPllsAddresses),
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
    s_AVddPllsAddresses,
    NV_ARRAY_SIZE(s_AVddPllsAddresses),
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
    s_VddIoLcdAddresses,
    NV_ARRAY_SIZE(s_VddIoLcdAddresses),
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
    s_VddIoAudioAddresses,
    NV_ARRAY_SIZE(s_VddIoAudioAddresses),
    NvOdmPeripheralClass_Other
},

// DDR_RX (NV reserved)
{
    NV_VDD_DDR_RX_ODM_ID,
    s_VddDdrRxAddresses,
    NV_ARRAY_SIZE(s_VddDdrRxAddresses),
    NvOdmPeripheralClass_Other
},

// VDD_EMMC_CORE (NV reserved)
{
    NV_VDD_NAND_ODM_ID,
    s_VddEmmcCoreAddresses,
    NV_ARRAY_SIZE(s_VddEmmcCoreAddresses),
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
    s_AVddUsbAddresses,
    NV_ARRAY_SIZE(s_AVddUsbAddresses),
    NvOdmPeripheralClass_Other
},

//  Max77663
{
    NV_ODM_GUID('m','a','x','7','7','6','6','3'),
    s_Max77663Addresses,
    NV_ARRAY_SIZE(s_Max77663Addresses),
    NvOdmPeripheralClass_Other
},

// Kai PMU
{
    NV_ODM_GUID('k','a','i','p','m','u',' ',' '),
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
