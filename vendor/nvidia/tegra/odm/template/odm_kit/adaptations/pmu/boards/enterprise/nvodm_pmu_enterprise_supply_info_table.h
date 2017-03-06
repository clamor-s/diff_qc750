/*
 * Copyright (c) 2011-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
/**
 * @file
 * <b>NVIDIA Tegra ODM Kit: Enterprise power rails</b>
 *
 * @b Description: Defines the enterprise power rail to be used by
 *                          enterprise odm driver.
 *
 */

#ifndef INCLUDED_NVODM_PMU_ENTERPRISE_SUPPLY_INFO_TABLE_H
#define INCLUDED_NVODM_PMU_ENTERPRISE_SUPPLY_INFO_TABLE_H

#if defined(__cplusplus)
extern "C"
{
#endif

typedef enum
{
    EnterprisePowerRails_Invalid = 0x0,
    EnterprisePowerRails_VDD_CPU,
    EnterprisePowerRails_VDD_CORE,
    EnterprisePowerRails_VDD_RTC,
    EnterprisePowerRails_VDD_DDR_HS,
    EnterprisePowerRails_AVDD_PLLA_P_C_S,
    EnterprisePowerRails_AVDD_PLLM,
    EnterprisePowerRails_AVDD_PLLU_D,
    EnterprisePowerRails_AVDD_PLLU_D2,
    EnterprisePowerRails_AVDD_PLLX,
    EnterprisePowerRails_VDDIO_SYS,
    EnterprisePowerRails_VDDIO_UART,
    EnterprisePowerRails_VDDIO_LCD,
    EnterprisePowerRails_VDDIO_BB,
    EnterprisePowerRails_AVDD_OSC,
    EnterprisePowerRails_VDDIO_AUDIO,
    EnterprisePowerRails_VDDIO_GMI,
    EnterprisePowerRails_AVDD_USB,
    EnterprisePowerRails_AVDD_USB_PLL,
    EnterprisePowerRails_VDDIO_CAM,
    EnterprisePowerRails_VDDIO_SDMMC1,
    EnterprisePowerRails_VDDIO_SDMMC3,
    EnterprisePowerRails_VDDIO_SDMMC4,
    EnterprisePowerRails_AVDD_VDAC,
    EnterprisePowerRails_VPP_FUSE,
    EnterprisePowerRails_VDDIO_HSIC,
    EnterprisePowerRails_VDDIO_IC_USB,
    EnterprisePowerRails_VDD_DDR_RX,
    EnterprisePowerRails_AVDD_DSI_CSI,
    EnterprisePowerRails_AVDD_HDMI,
    EnterprisePowerRails_AVDD_HDMI_PLL,
    EnterprisePowerRails_VDDIO_DDR,
    EnterprisePowerRails_VDDIO_VI,
    EnterprisePowerRails_VCORE_LCD,
    EnterprisePowerRails_VDDIO_LCD_PMU,
    EnterprisePowerRails_CAM1_SENSOR,
    EnterprisePowerRails_CAM2_SENSOR,
    EnterprisePowerRails_AVDD_PLLE,
    EnterprisePowerRails_VDD_FUSE,
    EnterprisePowerRails_VDD_LCD,
    EnterprisePowerRails_VDD_SDMMC,
    EnterprisePowerRails_VDD_5V15,
    EnterprisePowerRails_VDD_3V3,
    EnterprisePowerRails_HDMI_5V0,
    EnterprisePowerRails_Num,
    EnterprisePowerRails_Force32 = 0x7FFFFFFF
} EnterprisePowerRails;

#if defined(__cplusplus)
}
#endif

#endif /* INCLUDED_NVODM_PMU_ENTERPRISE_SUPPLY_INFO_TABLE_H */
