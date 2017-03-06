/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVODM_PMU_KAI_SUPPLY_INFO_TABLE_H
#define INCLUDED_NVODM_PMU_KAI_SUPPLY_INFO_TABLE_H

#if defined(__cplusplus)
extern "C"
{
#endif

typedef enum
{
    KaiPowerRails_Invalid = 0x0,
    KaiPowerRails_VDD_CPU,
    KaiPowerRails_VDD_CORE,
    KaiPowerRails_VDD_GEN1V8,
    KaiPowerRails_VDD_DDR3L_1V35,
    KaiPowerRails_VDD_DDR_HS, //5
    KaiPowerRails_VDD_DDR_RX,
    KaiPowerRails_VDD_EMMC_CORE,
    KaiPowerRails_VDD_SENSOR_2V8,
    KaiPowerRails_VDD_RTC,
    KaiPowerRails_VDDIO_SDMMC1, //10
    KaiPowerRails_AVDD_DSI_CSI,
    KaiPowerRails_AVDD_PLLS,
    KaiPowerRails_EN_AVDD_HDMI_USB,
    KaiPowerRails_EN_3V3_SYS,
    KaiPowerRails_LED2,  //15
    KaiPowerRails_AVDD_HDMI_PLL,
    KaiPowerRails_AVDD_USB_PLL,
    KaiPowerRails_AVDD_OSC,
    KaiPowerRails_VDDIO_SYS,
    KaiPowerRails_VDDIO_SDMMC4, //20
    KaiPowerRails_VDDIO_UART,
    KaiPowerRails_VDDIO_BB,
    KaiPowerRails_VDDIO_LCD,
    KaiPowerRails_VDDIO_AUDIO,
    KaiPowerRails_VDDIO_CAM, //25
    KaiPowerRails_VDDIO_SDMMC3,
    KaiPowerRails_VDDIO_VI,
    KaiPowerRails_VDD_3V3_SYS,
    KaiPowerRails_AVDD_HDMI,
    KaiPowerRails_AVDD_USB, //30
    KaiPowerRails_VDDIO_GMI,
    KaiPowerRails_VDD_1V8_CAM,
    KaiPowerRails_VCORE_LCD,
    KaiPowerRails_VDD_BACKLIGHT,
    KaiPowerRails_VDD_HDMI_CON, //35
    KaiPowerRails_VDD_MINI_CARD,
    KaiPowerRails_VDD_LCD_PANEL,
    KaiPowerRails_VDD_CAM3,
    KaiPowerRails_VDD_COM_BD,
    KaiPowerRails_VDD_SD_SLOT, //40
    KaiPowerRails_VPP_FUSE,

    KaiPowerRails_Num,
    KaiPowerRails_Force32 = 0x7FFFFFFF
} KaiPowerRails;

#if defined(__cplusplus)
}
#endif

#endif /* INCLUDED_NVODM_PMU_KAI_SUPPLY_INFO_TABLE_H */
