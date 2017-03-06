/*
 * Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_pmu.h"
#include "nvodm_pmu_cardhu.h"
#include "cardhu_pmu.h"
#include "nvodm_pmu_cardhu_supply_info_table.h"
#include "ricoh583/nvodm_pmu_ricoh583_pmu_driver.h"

static NvOdmPmuRailProps s_OdmRicoh583RailProps[] = {
    {Ricoh583PmuSupply_Invalid, CardhuPowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_DC0, CardhuPowerRails_VDD_CPU_PMU, 0, 0, 1000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_DC1, CardhuPowerRails_VDD_CORE, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_DC2, CardhuPowerRails_VDDIO_SYS, 0, 0, 1800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_DC3, CardhuPowerRails_VDD_GEN1V5, 0, 0, 1500, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_LDO0, CardhuPowerRails_VDD_DDR_HS, 0, 0, 1000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_LDO1, CardhuPowerRails_AVDD_PLL_A_P_C_S, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_LDO2, CardhuPowerRails_AVDD_DSI_CSI, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_LDO3, CardhuPowerRails_AVDD_VDAC, 0, 0, 3300, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_LDO4, CardhuPowerRails_VDD_RTC, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_LDO5, CardhuPowerRails_VDDIO_SDMMC1, 0, 0, 3300, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_LDO6, CardhuPowerRails_Invalid, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_LDO7, CardhuPowerRails_AVDD_PEXB, 0, 0, 1050, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_LDO8, CardhuPowerRails_AVDD_SATA, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_LDO9, CardhuPowerRails_Invalid, 0, 0, 1800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_GPIO0, CardhuPowerRails_Invalid, 0, 0, 5000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_GPIO1, CardhuPowerRails_VDD_3V3_DEVICES, 0, 0, 3300, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_GPIO2, CardhuPowerRails_Invalid, 0, 0, 5000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_GPIO3, CardhuPowerRails_V2REF_DDR, 0, 0, 1050, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_GPIO4, CardhuPowerRails_EN_5V0, 0, 0, 5000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_GPIO5, CardhuPowerRails_Invalid, 0, 0, 5000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_GPIO6, CardhuPowerRails_LED1, 0, 0, 1050, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Ricoh583PmuSupply_GPIO7, CardhuPowerRails_LED2, 0, 0, 1050, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
};

static PmuChipDriverOps s_Rico583OdmProps = {
    Ricoh583PmuGetCaps, Ricoh583PmuGetVoltage, Ricoh583PmuSetVoltage,
};

static void InitMapRicoh583(NvU32 *pRailMap)
{
    pRailMap[CardhuPowerRails_VDD_RTC] = Ricoh583PmuSupply_LDO4;
//    pRailMap[CardhuPowerRails_VDD_CORE] = Ricoh583PmuSupply_DC1;
    pRailMap[CardhuPowerRails_EN_VDDIO_DDR_1V2] = Ricoh583PmuSupply_DC1;
    pRailMap[CardhuPowerRails_VDD_GEN1V5] = Ricoh583PmuSupply_DC3;
    pRailMap[CardhuPowerRails_VCORE_LCD] = Ricoh583PmuSupply_DC3;
    pRailMap[CardhuPowerRails_TRACK_LDO1] = Ricoh583PmuSupply_DC3;
    pRailMap[CardhuPowerRails_EXTERNAL_LDO_1V2] = Ricoh583PmuSupply_DC3;
    pRailMap[CardhuPowerRails_VCORE_CAM1] = Ricoh583PmuSupply_DC3;
    pRailMap[CardhuPowerRails_VCORE_CAM2] = Ricoh583PmuSupply_DC3;
    pRailMap[CardhuPowerRails_VDD_CPU_PMU] = Ricoh583PmuSupply_DC0;
    pRailMap[CardhuPowerRails_AVDD_HDMI_PLL] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_AVDD_USB_PLL] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_AVDD_OSC] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_VDDIO_SYS] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_VDDIO_SDMMC4] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_VDD_1V8_SATELITE] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_VDDIO_UART] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_VDDIO_AUDIO] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_VDDIO_BB] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_VDDIO_LCD_PMU] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_VDDIO_CAM] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_VDDIO_VI] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_LDO6] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_LDO7] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_LDO8] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_VCORE_AUDIO] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_AVCORE_AUDIO] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_VDDIO_SDMMC3] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_VCORE1_LPDDR2] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_VCOM_1V8] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_PMUIO_1V8] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_AVDD_IC_USB] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_AVDD_PEXB] = Ricoh583PmuSupply_LDO7;
    pRailMap[CardhuPowerRails_VDD_PEXB] = Ricoh583PmuSupply_LDO7;
    pRailMap[CardhuPowerRails_AVDD_PEX_PLL] = Ricoh583PmuSupply_LDO7;
    pRailMap[CardhuPowerRails_AVDD_PEXA] = Ricoh583PmuSupply_LDO7;
    pRailMap[CardhuPowerRails_VDD_PEXA] = Ricoh583PmuSupply_LDO7;
    pRailMap[CardhuPowerRails_AVDD_SATA] = Ricoh583PmuSupply_LDO8;
    pRailMap[CardhuPowerRails_VDD_SATA] = Ricoh583PmuSupply_LDO8;
    pRailMap[CardhuPowerRails_AVDD_SATA_PLL] = Ricoh583PmuSupply_LDO8;
    pRailMap[CardhuPowerRails_AVDD_PLLE] = Ricoh583PmuSupply_LDO8;
    pRailMap[CardhuPowerRails_VDDIO_SDMMC1] = Ricoh583PmuSupply_LDO5;
    pRailMap[CardhuPowerRails_AVDD_VDAC] = Ricoh583PmuSupply_LDO3;
    pRailMap[CardhuPowerRails_AVDD_DSI_CSI] = Ricoh583PmuSupply_LDO2;
    pRailMap[CardhuPowerRails_AVDD_PLL_A_P_C_S] = Ricoh583PmuSupply_LDO1;
    pRailMap[CardhuPowerRails_AVDD_PLLM] = Ricoh583PmuSupply_LDO1;
    pRailMap[CardhuPowerRails_AVDD_PLLU_D] = Ricoh583PmuSupply_LDO1;
    pRailMap[CardhuPowerRails_AVDD_PLLU_D2] = Ricoh583PmuSupply_LDO1;
    pRailMap[CardhuPowerRails_AVDD_PLLX] = Ricoh583PmuSupply_LDO1;
    pRailMap[CardhuPowerRails_VDD_DDR_HS] = Ricoh583PmuSupply_LDO0;
    pRailMap[CardhuPowerRails_VDD_LVDS] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_PNL] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VCOM_3V3] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_3V3] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VCORE_MMC] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDDIO_PEXCTL] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_HVDD_PEX] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_AVDD_HDMI] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VPP_FUSE] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_AVDD_USB] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_DDR_RX] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VCORE_NAND] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_HVDD_SATA] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDDIO_GMI_PMU] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_AVDD_CAM1] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_AF] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_AVDD_CAM2] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_ACC] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_PHTN] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDDIO_TP] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_LED] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDDIO_CEC] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_CMPS] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_TEMP] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VPP_KFUSE] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDDIO_TS] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_IRLED] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_1WIRE] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_AVDDIO_AUDIO] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_EC] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VCOM_PA] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_HALL] = Ricoh583PmuSupply_GPIO4;
    pRailMap[CardhuPowerRails_USB1_VBUS] = Ricoh583PmuSupply_GPIO4;
    pRailMap[CardhuPowerRails_VDDIO_VGA] = Ricoh583PmuSupply_GPIO4;
    pRailMap[CardhuPowerRails_VDD_SPKR] = Ricoh583PmuSupply_GPIO4;
    pRailMap[CardhuPowerRails_VDDIO_HDMI] = Ricoh583PmuSupply_GPIO4;
    pRailMap[CardhuPowerRails_USB2_VBUS] = Ricoh583PmuSupply_GPIO4;
    pRailMap[CardhuPowerRails_USB3_VBUS] = Ricoh583PmuSupply_GPIO4;
//    pRailMap[CardhuPowerRails_VCOM_3V0] = Ricoh583PmuSupply_
//    pRailMap[CardhuPowerRails_AVDD_IC_USB_3V0] = Ricoh583PmuSupply_
//    pRailMap[CardhuPowerRails_VCOM_1V2] = Ricoh583PmuSupply_
//    pRailMap[CardhuPowerRails_VDIO_HSIC] = Ricoh583PmuSupply_
//    pRailMap[CardhuPowerRails_VDD_VBRTR] = Ricoh583PmuSupply_
//    pRailMap[CardhuPowerRails_AVDD_LCD1] = Ricoh583PmuSupply_
//    pRailMap[CardhuPowerRails_AVDD_LCD2] = Ricoh583PmuSupply_
//    pRailMap[CardhuPowerRails_VDD_BL1_P] = Ricoh583PmuSupply_
//    pRailMap[CardhuPowerRails_VDD_BL2_P] = Ricoh583PmuSupply_
    pRailMap[CardhuPowerRails_VDD_GEN1V8] = Ricoh583PmuSupply_DC2;
    pRailMap[CardhuPowerRails_EN_5V_CP] = Ricoh583PmuSupply_GPIO4;
    pRailMap[CardhuPowerRails_EN_DDR] = Ricoh583PmuSupply_GPIO3;
    pRailMap[CardhuPowerRails_EN_3V3_SYS] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_EN_5V0] = Ricoh583PmuSupply_GPIO4;
    pRailMap[CardhuPowerRails_LED1] = Ricoh583PmuSupply_GPIO6;
    pRailMap[CardhuPowerRails_LED2] = Ricoh583PmuSupply_GPIO7;
    pRailMap[CardhuPowerRails_VDD_5V0_SBY] = Ricoh583PmuSupply_GPIO4;
    pRailMap[CardhuPowerRails_VTERM_DDR] = Ricoh583PmuSupply_GPIO3;
    pRailMap[CardhuPowerRails_V2REF_DDR] = Ricoh583PmuSupply_GPIO3;
    pRailMap[CardhuPowerRails_MEM_VDDIO_DDR] = Ricoh583PmuSupply_GPIO3;
    pRailMap[CardhuPowerRails_T30_VDDIO_DDR] = Ricoh583PmuSupply_GPIO3;

    pRailMap[CardhuPowerRails_VDD_5V0_SYS] = Ricoh583PmuSupply_GPIO4;
    pRailMap[CardhuPowerRails_TOUCHPANEL_3V3] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_3V3_DEVICES] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_3V3_DOCK] = Ricoh583PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_1V8_DEVICES] = Ricoh583PmuSupply_DC2;

    pRailMap[CardhuPowerRails_VDD_5V0_PMU] = Ricoh583PmuSupply_GPIO4;
    pRailMap[CardhuPowerRails_VDD_5V0_VBUSR] = Ricoh583PmuSupply_GPIO4;
}

void
*CardhuPmu_GetRicoh583RailMap(
    NvOdmPmuDeviceHandle hPmuDevice,
    PmuChipDriverOpsHandle *hDriverOps,
    NvU32 *pRailMap,
    NvOdmBoardInfo *pProcBoard,
    NvOdmBoardInfo *pPmuBoard)
{
    NvOdmPmuDriverInfoHandle hPmuDriver = NULL;

    hPmuDriver = Ricoh583PmuDriverOpen(hPmuDevice, s_OdmRicoh583RailProps,
                                Ricoh583PmuSupply_Num);
    if (!hPmuDriver)
    {
        NvOdmOsPrintf("%s(): The Ricoh583PmuDriverOpen() fails\n", __func__);
        return NULL;
    }

    InitMapRicoh583(pRailMap);

    if (!((pProcBoard->SKU & SKU_DCDC_TPS62361_SUPPORT) ||
            (pPmuBoard->SKU & SKU_DCDC_TPS62361_SUPPORT)))
        pRailMap[CardhuPowerRails_VDD_CORE] = Ricoh583PmuSupply_DC1;

    *hDriverOps = &s_Rico583OdmProps;
    return hPmuDriver;
}

