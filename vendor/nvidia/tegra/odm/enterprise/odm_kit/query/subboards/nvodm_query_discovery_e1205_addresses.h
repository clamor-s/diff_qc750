/*
 * Copyright (c) 2011-2012 NVIDIA Corporation.  All rights reserved.
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
 * @b Description: Specifies the peripheral connectivity database NvOdmIoAddress
 *                 entries for the peripherals on E1291 module.
 */

#include "pmu/boards/enterprise/nvodm_pmu_enterprise_supply_info_table.h"

#define AR0832_PINS (NVODM_CAMERA_SERIAL_CSI_D1A | \
                     NVODM_CAMERA_DEVICE_IS_DEFAULT)

// AR0832 Aptina Sensor
static const NvOdmIoAddress s_ffaImagerAR0832Addresses[] =
{
    { NvOdmIoModule_I2c,  0x02, 0x6c, 0 },
    { NvOdmIoModule_Vdd,  0x00, EnterprisePowerRails_CAM1_SENSOR, 0 },   // VDDIO_VI
    { NvOdmIoModule_VideoInput, 0x00, AR0832_PINS, 0 },
    { NvOdmIoModule_ExternalClock, 2, 0, 0 },                   // CSUS
};

// AR0832 Aptina Stereo Sensor
static const NvOdmIoAddress s_ffaImagerAR0832SAddresses[] =
{
    { NvOdmIoModule_I2c,  0x02, 0x6c, 0 },
    { NvOdmIoModule_Vdd,  0x00, EnterprisePowerRails_CAM1_SENSOR, 0 },   // VDDIO_VI
    { NvOdmIoModule_VideoInput, 0x00, NVODM_CAMERA_SERIAL_CSI_D1B, 0 },
    { NvOdmIoModule_ExternalClock, 2, 0, 0 },                   // CSUS
};

static const NvOdmIoAddress s_ffaImagerOV9726Addresses[] =
{
    { NvOdmIoModule_I2c, 0x08, 0x20, 0 },
    { NvOdmIoModule_Vdd,  0x00, EnterprisePowerRails_CAM2_SENSOR, 0 }, // VDDIO_VI
    { NvOdmIoModule_VideoInput, 0x00, NVODM_CAMERA_SERIAL_CSI_D1B, 0 },
    { NvOdmIoModule_ExternalClock, 2, 0, 0 }
};

// PMU0
static const NvOdmIoAddress s_Pmu0Addresses[] =
{
    { NvOdmIoModule_I2c, 0x4, 0x90, 0 },
    { NvOdmIoModule_I2c, 0x4, 0x24, 0 },
};

// RTC voltage rail
static const NvOdmIoAddress s_VddRtcAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDD_RTC, 0 }
};

// Core voltage rail
static const NvOdmIoAddress s_VddCoreAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDD_CORE, 0 }
};

// EN_VDDIO_DDR_1V2
static const NvOdmIoAddress s_EnVddioDdr1V2Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDDIO_DDR, 0 }
};

// Vcore LCD
static const NvOdmIoAddress s_VcoreLcdAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VCORE_LCD, 0 }
};

// VDD_CPU_PMU
static const NvOdmIoAddress s_VddCpuPmuAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDD_CPU, 0 }
};


// ADVDD_HDMI_PLL
static const NvOdmIoAddress s_AVddHdmiPllAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_AVDD_HDMI_PLL, 0 }
};

// ADVDD_USB_PLL
static const NvOdmIoAddress s_AVddUsbPllAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_AVDD_USB_PLL, 0 }
};

// ADVDD_Osc
static const NvOdmIoAddress s_AVddOscAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_AVDD_OSC, 0 }
};

// VDDIO_SYS
static const NvOdmIoAddress s_VddIoSysAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDDIO_SYS, 0 }
};

// VDDIO_UART
static const NvOdmIoAddress s_VddIoUartAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDDIO_UART, 0 }
};

// VDDIO_Audio
static const NvOdmIoAddress s_VddIoAudioAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDDIO_AUDIO, 0 }
};

// VDDIO_BB
static const NvOdmIoAddress s_VddIoBBAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDDIO_BB, 0 }
};

// VDDIO_LCD_PMU
static const NvOdmIoAddress s_VddIoLcdPmuAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDDIO_LCD_PMU, 0 }
};

// VDDIO_CAM
static const NvOdmIoAddress s_VddIoCamAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDDIO_CAM, 0 }
};

// VDDIO_VI
static const NvOdmIoAddress s_VddIoViAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDDIO_VI, 0 }
};

// VDDIO_SDMMC3
static const NvOdmIoAddress s_VddIoSdmmc3Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDDIO_SDMMC3, 0 }
};

// AVdd_PLLE
static const NvOdmIoAddress s_AVddPllEAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_AVDD_PLLE, 0 }
};

// VDDIO_SDMMC1
static const NvOdmIoAddress s_VddioSdmmc1ddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDDIO_SDMMC1, 0 }
};

// AVdd_VDAC
static const NvOdmIoAddress s_AVddVdacAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_AVDD_VDAC, 0 }
};

// AVdd_DSI_CSI
static const NvOdmIoAddress s_AVddVDsiCsiAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_AVDD_DSI_CSI, 0 }
};

// AVdd_PLLA_P_C_S
static const NvOdmIoAddress s_AVddPllAPCSAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_AVDD_PLLA_P_C_S, 0 }
};

// AVdd_PLLM
static const NvOdmIoAddress s_AVddPllMAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_AVDD_PLLM, 0 }
};

// AVdd_PLLU_D
static const NvOdmIoAddress s_AVddPllUDAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_AVDD_PLLU_D, 0 }
};

// AVdd_PLLU_D2
static const NvOdmIoAddress s_AVddPllUD2Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_AVDD_PLLU_D2, 0 }
};

// AVdd_PLLX
static const NvOdmIoAddress s_AVddPllXAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_AVDD_PLLX, 0 }
};

// AVDD_HDMI
static const NvOdmIoAddress s_AVddHdmiAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_AVDD_HDMI, 0 }
};

// VPP_FUSE
static const NvOdmIoAddress s_VppFuseAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VPP_FUSE, 0 }
};

// AVDD_USB
static const NvOdmIoAddress s_AVddUsbAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_AVDD_USB, 0 }
};
// VDD_DDR_RX
static const NvOdmIoAddress s_VddDdrRxAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDD_DDR_RX, 0 }
};

// All rail
static const NvOdmIoAddress s_AllRailAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_AVDD_DSI_CSI, 0 },
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDDIO_SYS, 0 },
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDD_CPU, 0 },
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDD_CORE, 0 },
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDDIO_DDR, 0 },
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDD_RTC, 0 },
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDDIO_LCD, 0 },
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDDIO_SDMMC3, 0 },
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDD_DDR_HS, 0 },
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_AVDD_VDAC, 0 },
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_AVDD_USB, 0 },
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_AVDD_PLLM, 0 },

    // External power rail control through pmu gpio
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDD_5V15, 0 },
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDD_3V3, 0 },
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_HDMI_5V0, 0 },

    // External power rail control through AP-GPIO
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDD_FUSE, 0 },
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDD_SDMMC, 0 },
    { NvOdmIoModule_Vdd, 0x00, EnterprisePowerRails_VDD_LCD, 0 },
};

// Key Pad
static const NvOdmIoAddress s_KeyPadAddresses[] =
{
    // instance = 1 indicates Column info.
    // instance = 0 indicates Row info.
    // address holds KBC pin number used for row/column.

    // All Row info has to be defined contiguously from 0 to max.
    { NvOdmIoModule_Kbd, 0x00, NvOdmKbcGpioPin_KBRow0, 0}, // Row 0
    { NvOdmIoModule_Kbd, 0x00, NvOdmKbcGpioPin_KBRow1, 0}, // Row 1
    { NvOdmIoModule_Kbd, 0x00, NvOdmKbcGpioPin_KBRow2, 0}, // Row 2

    // All Column info has to be defined contiguously from 0 to max.
    { NvOdmIoModule_Kbd, 0x01, NvOdmKbcGpioPin_KBCol0, 0}, // Column 0
    { NvOdmIoModule_Kbd, 0x01, NvOdmKbcGpioPin_KBCol1, 0}, // Column 1
};

