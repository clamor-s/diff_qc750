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
 * @b Description: Specifies the peripheral connectivity database NvOdmIoAddress
 *                 entries for the peripherals on E1565 module.
 */

#include "pmu/boards/kai/nvodm_pmu_kai_supply_info_table.h"

static const NvOdmIoAddress s_ffaImagerOV2710Addresses[] =
{
    { NvOdmIoModule_I2c, 0x08, 0x6C, 0 },
    { NvOdmIoModule_VideoInput, 0x00, NVODM_CAMERA_SERIAL_CSI_D1B, 0 },
    { NvOdmIoModule_ExternalClock, 2, 0, 0 } // CSUS
};
static const NvOdmIoAddress s_ffaImagerGC0308Addresses[] =
{
    { NvOdmIoModule_I2c,  0x02, 0x42, 0 },  
    //{ NvOdmIoModule_Gpio, NVODM_PORT('d'), 2 | NVODM_IMAGER_RESET_AL, 0 },
    //{ NvOdmIoModule_Gpio, NVODM_PORT('b')+26, 5 | NVODM_IMAGER_POWERDOWN_AL, 0 },
    //{ NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO9, 0 },
    { NvOdmIoModule_VideoInput, 0x00, NVODM_CAMERA_PARALLEL_VD0_TO_VD7, 0 },
    { NvOdmIoModule_ExternalClock, 2, 0, 0 }
};
static const NvOdmIoAddress s_ffaImagerT8EV5Addresses[] =
{
    { NvOdmIoModule_I2c,  0x02, 0x3c, 0 },  
    { NvOdmIoModule_VideoInput, 0x00, NVODM_CAMERA_PARALLEL_VD0_TO_VD7, 0 },
    { NvOdmIoModule_ExternalClock, 2, 0, 0 }
};
// VDD_CPU
static const NvOdmIoAddress s_VddCpuAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_CPU, 0 }  /* VDD_CPU -> SD0 */
};

// VDD_CORE
static const NvOdmIoAddress s_VddCoreAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_CORE, 0 }  /* VDD_CORE -> SD1 */
};

// VDD_GEN1V8
static const NvOdmIoAddress s_VddGen1V8Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_GEN1V8, 0 }  /* VDD_GEN1V8 -> SD2 */
};

// VDD_DDR3L_1V35
static const NvOdmIoAddress s_VddDdr3L1V35Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_DDR3L_1V35, 0 }  /* VDD_DDR3L_1V35 -> SD3 */
};

// VDD_DDR_HS
static const NvOdmIoAddress s_VddDdrHsAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_DDR_HS, 0 }  /* VDD_DDR_HS -> LDO0 */
};

// VDD_DDR_RX
static const NvOdmIoAddress s_VddDdrRxAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_DDR_RX, 0 }  /* VDD_DDR_RX -> LDO2 */
};

// VDD_EMMC_CORE
static const NvOdmIoAddress s_VddEmmcCoreAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_EMMC_CORE, 0 }  /* VDD_EMMC_CORE -> LDO3 */
};

// VDD_SENSOR_2V8
static const NvOdmIoAddress s_VddSensor2V8Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_SENSOR_2V8, 0 }  /* VDD_SENSOR_2V8 -> LDO5 */
};

// VDD_RTC
static const NvOdmIoAddress s_VddRtcAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_RTC, 0 }  /* VDD_RTC -> LDO4 */
};

// VDDIO_SDMMC1
static const NvOdmIoAddress s_VddIoSdmmc1Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_SDMMC1, 0 }  /* VDDIO_SDMMC1 -> LDO6 */
};

// AVDD_DSI_CSI
static const NvOdmIoAddress s_AVddDsiCsiAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_AVDD_DSI_CSI, 0 }  /* AVDD_DSI_CSI -> LDO7 */
};

// AVDD_PLLS
static const NvOdmIoAddress s_AVddPllsAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_AVDD_PLLS, 0 }  /* AVDD_PLLS -> LDO8 */
};

// VDD_3V3_SYS
static const NvOdmIoAddress s_Vdd3V3SysAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_3V3_SYS, 0 }  /* 3.3V */
};

// AVDD_HDMI_PLL
static const NvOdmIoAddress s_AVddHdmiPllAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_AVDD_HDMI_PLL, 0 }  /* AVDD_HDMI_PLL -> VDD_GEN1V8 */
};

// AVDD_USB_PLL
static const NvOdmIoAddress s_AVddUsbPllAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_AVDD_USB_PLL, 0 }  /* AVDD_USB_PLL -> VDD_GEN1V8 */
};

// AVDD_OSC
static const NvOdmIoAddress s_AVddOscAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_AVDD_OSC, 0 }  /* AVDD_OSC -> VDD_GEN1V8 */
};

// VDDIO_SYS
static const NvOdmIoAddress s_VddIoSysAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_SYS, 0 }  /* VDDIO_SYS -> VDD_GEN1V8 */
};

// VDDIO_SDMMC4
static const NvOdmIoAddress s_VddIoSdmmc4Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_SDMMC4, 0 }  /* VDDIO_SDMMC4 -> VDD_GEN1V8 */
};

// VDDIO_UART
static const NvOdmIoAddress s_VddIoUartAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_UART, 0 }  /* VDDIO_UART -> VDD_GEN1V8 */
};

// VDDIO_BB
static const NvOdmIoAddress s_VddIoBBAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_BB, 0 }  /* VDDIO_BB -> VDD_GEN1V8 */
};

// VDDIO_LCD
static const NvOdmIoAddress s_VddIoLcdAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_LCD, 0 }  /* VDDIO_LCD -> VDD_GEN1V8 */
};

// VDDIO_AUDIO
static const NvOdmIoAddress s_VddIoAudioAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_AUDIO, 0 }  /* VDDIO_AUDIO -> VDD_GEN1V8 */
};

// VDDIO_CAM
static const NvOdmIoAddress s_VddIoCamAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_CAM, 0 }  /* VDDIO_CAM -> VDD_GEN1V8 */
};

// VDDIO_SDMMC3
static const NvOdmIoAddress s_VddIoSdmmc3Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_SDMMC3, 0 }  /* VDDIO_SDMMC3 -> VDD_GEN1V8 */
};

// VDDIO_VI
static const NvOdmIoAddress s_VddIoViAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_VI, 0 }  /* VDDIO_VI -> VDD_GEN1V8 */
};

// AVDD_HDMI
static const NvOdmIoAddress s_AVddHdmiAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_AVDD_HDMI, 0 }
};

// AVDD_USB
static const NvOdmIoAddress s_AVddUsbAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_AVDD_USB, 0 }
};

// Vcore LCD
static const NvOdmIoAddress s_VcoreLcdAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VCORE_LCD, 0 }
};

// PMU0
static const NvOdmIoAddress s_Pmu0Addresses[] =
{
    { NvOdmIoModule_I2c, 0x4, 0x78, 0 },
};

// MAX77663
static const NvOdmIoAddress s_Max77663Addresses[] =
{
    { NvOdmIoModule_I2c, 0x4, 0x78, 0 },
};

// All rail
static const NvOdmIoAddress s_AllRailAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_CPU},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_CORE},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_GEN1V8},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_DDR3L_1V35},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_DDR_HS},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_DDR_RX},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_EMMC_CORE},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_SENSOR_2V8},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_RTC},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_SDMMC1},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_AVDD_DSI_CSI},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_AVDD_PLLS},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_AVDD_HDMI_PLL},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_AVDD_USB_PLL},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_SYS},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_SDMMC4},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_UART},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_BB},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_LCD},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_AUDIO},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_CAM},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_SDMMC3},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_VI},

    // External power rail control through pmu gpio
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_EN_AVDD_HDMI_USB},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_EN_3V3_SYS},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_LED2},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_3V3_SYS},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_AVDD_HDMI},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_AVDD_USB},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDDIO_GMI},

    // External power rail control through AP-GPIO
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_1V8_CAM},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VCORE_LCD},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_BACKLIGHT},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_HDMI_CON},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_MINI_CARD},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_LCD_PANEL},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_CAM3},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_COM_BD},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_SD_SLOT},
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VPP_FUSE},
};

static const NvOdmIoAddress s_LvdsDisplayAddresses[] =
{
    { NvOdmIoModule_Display, 0, 0, 0 },
    { NvOdmIoModule_I2c, 0x00, 0xA0, 0 },
    { NvOdmIoModule_Pwm, 0x00, 0, 0 },
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_LCD_PANEL, 0 },
};

