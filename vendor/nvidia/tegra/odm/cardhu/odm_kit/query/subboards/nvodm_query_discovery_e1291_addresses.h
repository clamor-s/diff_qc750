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
 * @b Description: Specifies the peripheral connectivity database NvOdmIoAddress
 *                 entries for the peripherals on E1291 module.
 */

#include "pmu/boards/cardhu/nvodm_pmu_cardhu_supply_info_table.h"

// RTC voltage rail
static const NvOdmIoAddress s_VddRtcAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_RTC, 0 }  /* VDD_RTC -> LD02 */
};

// Core voltage rail
static const NvOdmIoAddress s_VddCoreAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_CORE, 0 }  /* VDD_CORE -> VDD1 */
};

// EN_VDDIO_DDR_1V2
static const NvOdmIoAddress s_EnVddioDdr1V2Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_EN_VDDIO_DDR_1V2, 0 }
};

// VDD_GEN1V5
static const NvOdmIoAddress s_VddGen1V5Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_GEN1V5, 0 }
};

// Vcore LCD
static const NvOdmIoAddress s_VcoreLcdAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VCORE_LCD, 0 }
};
// Vcore Cam1
static const NvOdmIoAddress s_VcoreCam1Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VCORE_CAM1, 0 }
};

// Vcore Cam2
static const NvOdmIoAddress s_VcoreCam2Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VCORE_CAM2, 0 }
};

// VDD_CPU_PMU
static const NvOdmIoAddress s_VddCpuPmuAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_CPU_PMU, 0 }
};


// ADVDD_HDMI_PLL
static const NvOdmIoAddress s_AVddHdmiPllAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_HDMI_PLL, 0 }
};

// ADVDD_USB_PLL
static const NvOdmIoAddress s_AVddUsbPllAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_USB_PLL, 0 }
};

// ADVDD_Osc
static const NvOdmIoAddress s_AVddOscAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_OSC, 0 }
};

// VDDIO_SYS
static const NvOdmIoAddress s_VddIoSysAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_SYS, 0 }
};

// VDDIO_SDMMC4
static const NvOdmIoAddress s_VddIoSdmmc4Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_SDMMC4, 0 }
};

// VDDIO_UART
static const NvOdmIoAddress s_VddIoUartAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_UART, 0 }
};

// VDDIO_Audio
static const NvOdmIoAddress s_VddIoAudioAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_AUDIO, 0 }
};

// VDDIO_BB
static const NvOdmIoAddress s_VddIoBBAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_BB, 0 }
};

// VDDIO_LCD_PMU
static const NvOdmIoAddress s_VddIoLcdPmuAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_LCD_PMU, 0 }
};

// VDDIO_CAM
static const NvOdmIoAddress s_VddIoCamAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_CAM, 0 }
};

// VDDIO_VI
static const NvOdmIoAddress s_VddIoViAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_VI, 0 } /* VDD_GEN1V8 */
};

// VCORE_AUDIO
static const NvOdmIoAddress s_VCoreAudioAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VCORE_AUDIO, 0 } /* VDD_GEN1V8 */
};

// AVCORE_AUDIO
static const NvOdmIoAddress s_AvCoreAudioAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVCORE_AUDIO, 0 } /* VDD_GEN1V8 */
};

// VDDIO_SDMMC3
static const NvOdmIoAddress s_VddIoSdmmc3Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_SDMMC3, 0 } /* VDD_GEN1V8 */
};

// VCORE1_LPDDR2
static const NvOdmIoAddress s_Vcore1Lpddr2Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VCORE1_LPDDR2, 0 } /* VDD_GEN1V8 */
};

// VCOM_1V8
static const NvOdmIoAddress s_VCom1V8Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VCOM_1V8, 0 } /* VDD_GEN1V8 */
};

// PMUIO_1V8
static const NvOdmIoAddress s_PmuIo1V8Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_PMUIO_1V8, 0 } /* VDD_GEN1V8 */
};

// AVdd_IC_USB
static const NvOdmIoAddress s_AVddIcUsbAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_IC_USB, 0 } /* VDD_GEN1V8 */
};

// AVdd_PEXB
static const NvOdmIoAddress s_AVddPexBAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_PEXB, 0 } /* 1.05V */
};
// VDD_PEXB
static const NvOdmIoAddress s_VddPexBAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_PEXB, 0 } /* 1.05V */
};
// AVdd_PEX_PLL
static const NvOdmIoAddress s_AVddPexPllAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_PEX_PLL, 0 } /* 1.05V */
};

// AVdd_PEXA
static const NvOdmIoAddress s_AVddPexAAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_PEXA, 0 } /* 1.05V */
};

// VDD_PEXA
static const NvOdmIoAddress s_VddPexAAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_PEXA, 0 } /* 1.05V */
};

// AVdd_SATA
static const NvOdmIoAddress s_AVddSataAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_SATA, 0 } /* 1.05V */
};

// VDD_SATA
static const NvOdmIoAddress s_VddSataAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_SATA, 0 } /* 1.05V */
};

// AVdd_SATA_PLL
static const NvOdmIoAddress s_AVddSataPllAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_SATA_PLL, 0 } /* 1.05V */
};
// AVdd_PLLE
static const NvOdmIoAddress s_AVddPllEAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_PLLE, 0 } /* 1.05V */
};

// VDDIO_SDMMC1
static const NvOdmIoAddress s_VddioSdmmc1ddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_SDMMC1, 0 }
};

// AVdd_VDAC
static const NvOdmIoAddress s_AVddVdacAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_VDAC, 0 }
};

// AVdd_DSI_CSI
static const NvOdmIoAddress s_AVddVDsiCsiAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_DSI_CSI, 0 }
};

// AVdd_PLLA_P_C_S
static const NvOdmIoAddress s_AVddPllAPCSAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_PLL_A_P_C_S, 0 } /* 1V1 */
};

// AVdd_PLLM
static const NvOdmIoAddress s_AVddPllMAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_PLLM, 0 } /* 1V1 */
};

// AVdd_PLLU_D
static const NvOdmIoAddress s_AVddPllUDAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_PLLU_D, 0 } /* 1V1 */
};

// AVdd_PLLU_D2
static const NvOdmIoAddress s_AVddPllUD2Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_PLLU_D2, 0 } /* 1V1 */
};

// AVdd_PLLX
static const NvOdmIoAddress s_AVddPllXAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_PLLX, 0 } /* 1V1 */
};

// VDD_DDR_HS
static const NvOdmIoAddress s_VddDdrHsAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_DDR_HS, 0 } /* 1V0 */
};

// VDD_LVDS
static const NvOdmIoAddress s_VddLvdsAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_LVDS, 0 } /* 3.3V */
};

// VDD_PNL
static const NvOdmIoAddress s_VddPnlAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_PNL, 0 } /* 3.3V */
};
// VCORE_MMC
static const NvOdmIoAddress s_VCoreMmcAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VCORE_MMC, 0 } /* 3.3V */
};
// VDDIO_PEX_CTL
static const NvOdmIoAddress s_VddioPexCtlAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_PEXCTL, 0 } /* 3.3V */
};
// HVDD_PEX
static const NvOdmIoAddress s_HVddPexAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_HVDD_PEX, 0 } /* 3.3V */
};
// AVDD_HDMI
static const NvOdmIoAddress s_AVddHdmiAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_HDMI, 0 } /* 3.3V */
};

// VPP_FUSE
static const NvOdmIoAddress s_VppFuseAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VPP_FUSE, 0 } /* 3.3V */
};
// AVDD_USB
static const NvOdmIoAddress s_AVddUsbAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_USB, 0 } /* 3.3V */
};
// VDD_DDR_RX
static const NvOdmIoAddress s_VddDdrRxAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_DDR_RX, 0 } /* 3.3V */
};
// VCORE_NAND
static const NvOdmIoAddress s_VCoreNandAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VCORE_NAND, 0 } /* 3.3V */
};
// HVDD_SATA
static const NvOdmIoAddress s_HVddSataAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_HVDD_SATA, 0 } /* 3.3V */
};

// VDDIO_GMI_PMU
static const NvOdmIoAddress s_VddIoGmiPmuAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_GMI_PMU, 0 } /* 3.3V */
};
// AVDD_CAM1
static const NvOdmIoAddress s_AVddCam1Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_CAM1, 0 } /* 3.3V */
};
// VDD_AF
static const NvOdmIoAddress s_VddAfAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_AF, 0 } /* 3.3V */
};
// AVDD_CAM2
static const NvOdmIoAddress s_AVddCam2Addresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_CAM2, 0 } /* 3.3V */
};

// VDD_ACC
static const NvOdmIoAddress s_VddAccAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_ACC, 0 } /* 3.3V */
};

// VDD_PHTN
static const NvOdmIoAddress s_VddPhtnAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_PHTN, 0 } /* 3.3V */
};
// VDDIO_TP
static const NvOdmIoAddress s_VddioTpAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_TP, 0 } /* 3.3V */
};
// VDD_LED
static const NvOdmIoAddress s_VddLedAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_LED, 0 } /* 3.3V */
};
// VDDIO_CEC
static const NvOdmIoAddress s_VddioCecAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_CEC, 0 } /* 3.3V */
};
// VDD_CMPS
static const NvOdmIoAddress s_VddCmpsAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_CMPS, 0 } /* 3.3V */
};

// VDD_TEMP
static const NvOdmIoAddress s_VddTempAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_TEMP, 0 } /* 3.3V */
};
// VPP_KFUSE
static const NvOdmIoAddress s_VppKFuseAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VPP_KFUSE, 0 } /* 3.3V */
};
// VDDIO_TS
static const NvOdmIoAddress s_VddioTsAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_TS, 0 } /* 3.3V */
};
// VDD_IRLED
static const NvOdmIoAddress s_VddIrLedAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_IRLED, 0 } /* 3.3V */
};
// VDDIO_1WIRE
static const NvOdmIoAddress s_Vddio1WireAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_1WIRE, 0 } /* 3.3V */
};

// AVDD_AUDIO
static const NvOdmIoAddress s_AVddAudioAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDDIO_AUDIO, 0 } /* 3.3V */
};
// VDD_EC
static const NvOdmIoAddress s_VddEcAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_EC, 0 } /* 3.3V */
};
// VCOM_PA
static const NvOdmIoAddress s_VcomPaAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VCOM_PA, 0 } /* 3.3V */
};

// PMU0
static const NvOdmIoAddress s_Pmu0Addresses[] =
{
    { NvOdmIoModule_I2c, 0x4, 0x5A, 0 },
};

// TPS6236x
static const NvOdmIoAddress s_Tps6236xddresses[] =
{
    { NvOdmIoModule_I2c, 0x4, 0xC0, 0 },
};

// RICOH583
static const NvOdmIoAddress s_Rico583Addresses[] =
{
    { NvOdmIoModule_I2c, 0x4, 0x68, 0 },
};

// TCA6416
static const NvOdmIoAddress s_Tca6416Addresses[] =
{
    { NvOdmIoModule_I2c, 0x4, 0x40, 0 },
};

// MAX77663
static const NvOdmIoAddress s_Max77663Addresses[] =
{
    { NvOdmIoModule_I2c, 0x4, 0x38, 0 },
};

// USB1 VBus voltage rail
static const NvOdmIoAddress s_ffaVddUsb1VBusAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_VBUS_MICRO_USB, 0 },
};

// USB3 VBus voltage rail
static const NvOdmIoAddress s_ffaVddUsb3VBusAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_VBUS_TYPEA_USB, 0 },
};


// All rail
static const NvOdmIoAddress s_AllRailAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_SYS},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_CORE},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VCORE_CAM1},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_CPU_PMU},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_PEXB},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_SATA},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_SDMMC1},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_RTC},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_VDAC},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_DSI_CSI},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_PLLM},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_DDR_HS},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_RTC},

    // External power rail control through pmu gpio
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_LED1},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_LED2},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_5V0_SBY},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VTERM_DDR},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_5V0_SYS},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_EN_3V3_SYS},

    // External power rail control through AP-GPIO
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_BACKLIGHT},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_3V3_MINI_CARD},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_LCD_PANEL_3V3},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_CAM3_3V3},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_COM_BD_3V3},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_SD_SLOT_3V3},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_EMMC_CORE_3V3},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_HVDD_PEX_3V3},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_VPP_FUSE_3V3},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_VBUS_MICRO_USB},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_VBUS_TYPEA_USB},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_HDMI_CON},
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_1V8_CAM1},
};

