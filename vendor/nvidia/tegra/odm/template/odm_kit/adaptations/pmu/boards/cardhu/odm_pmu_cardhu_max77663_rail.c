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
#include "max77663/nvodm_pmu_max77663_pmu_driver.h"

static NvOdmPmuRailProps s_OdmMax77663RailProps[] = {
    {Max77663PmuSupply_Invalid, CardhuPowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_SD0, CardhuPowerRails_VDD_CPU_PMU, 0, 0, 1000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_SD1, CardhuPowerRails_VDD_CORE, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_SD2, CardhuPowerRails_AVDD_HDMI_PLL, 0, 0, 1800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_SD3, CardhuPowerRails_VDD_GEN1V5, 0, 0, 1500, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_LDO0, CardhuPowerRails_VDD_DDR_HS, 0, 0, 1000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_LDO1, CardhuPowerRails_AVDD_PLL_A_P_C_S, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_LDO2, CardhuPowerRails_AVDD_DSI_CSI, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_LDO3, CardhuPowerRails_VDDIO_SDMMC4, 0, 0, 1800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_LDO4, CardhuPowerRails_VDD_RTC, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_LDO5, CardhuPowerRails_VDDIO_SDMMC1, 0, 0, 3300, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_LDO6, CardhuPowerRails_VDDIO_SYS, 0, 0, 1800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_LDO7, CardhuPowerRails_Invalid, 0, 0, 3300, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_LDO8, CardhuPowerRails_VCORE_MMC, 0, 0, 2850, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_GPIO0, CardhuPowerRails_AVDD_SATA, 0, 0, 3300, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_GPIO1, CardhuPowerRails_EN_3V3_SYS, 0, 0, 3300, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_GPIO2, CardhuPowerRails_EN_5V0, 0, 0, 5000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_GPIO3, CardhuPowerRails_EN_DDR, 0, 0, 1500, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_GPIO4, CardhuPowerRails_Invalid, 0, 0, 5000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_GPIO5, CardhuPowerRails_Invalid, 0, 0, 1800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_GPIO6, CardhuPowerRails_LED1, 0, 0, 1800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_GPIO7, CardhuPowerRails_LED2, 0, 0, 1800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
};

static PmuChipDriverOps s_Max77663OdmProps = {
    Max77663PmuGetCaps, Max77663PmuGetVoltage, Max77663PmuSetVoltage,
};

static void InitMapMax77663(NvU32 *pRailMap)
{
    pRailMap[CardhuPowerRails_VDD_CPU_PMU] = Max77663PmuSupply_SD0;

    pRailMap[CardhuPowerRails_VDD_CORE] = Max77663PmuSupply_SD1;
    pRailMap[CardhuPowerRails_EN_VDDIO_DDR_1V2] = Max77663PmuSupply_SD1;

    pRailMap[CardhuPowerRails_AVDD_HDMI_PLL] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_AVDD_USB_PLL] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_AVDD_OSC] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_VDD_1V8_SATELITE] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_VDDIO_UART] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_VDDIO_AUDIO] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_VDDIO_BB] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_VDDIO_LCD_PMU] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_VDDIO_CAM] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_VDDIO_VI] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_LDO6] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_LDO7] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_LDO8] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_VCORE_AUDIO] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_AVCORE_AUDIO] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_VDDIO_SDMMC3] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_VCORE1_LPDDR2] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_VCOM_1V8] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_PMUIO_1V8] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_AVDD_IC_USB] = Max77663PmuSupply_SD2;
    pRailMap[CardhuPowerRails_VDD_GEN1V8] = Max77663PmuSupply_SD2;

    pRailMap[CardhuPowerRails_VDD_GEN1V5] = Max77663PmuSupply_SD3;
    pRailMap[CardhuPowerRails_VCORE_LCD] = Max77663PmuSupply_SD3;
    pRailMap[CardhuPowerRails_TRACK_LDO1] = Max77663PmuSupply_SD3;
    pRailMap[CardhuPowerRails_EXTERNAL_LDO_1V2] = Max77663PmuSupply_SD3;
    pRailMap[CardhuPowerRails_VCORE_CAM1] = Max77663PmuSupply_SD3;
    pRailMap[CardhuPowerRails_VCORE_CAM2] = Max77663PmuSupply_SD3;
    pRailMap[CardhuPowerRails_AVDD_PEXB] = Max77663PmuSupply_SD3;
    pRailMap[CardhuPowerRails_VDD_PEXB] = Max77663PmuSupply_SD3;
    pRailMap[CardhuPowerRails_AVDD_PEX_PLL] = Max77663PmuSupply_SD3;
    pRailMap[CardhuPowerRails_AVDD_PEXA] = Max77663PmuSupply_SD3;
    pRailMap[CardhuPowerRails_VDD_PEXA] = Max77663PmuSupply_SD3;
    pRailMap[CardhuPowerRails_VCOM_1V2] = Max77663PmuSupply_SD3;
    pRailMap[CardhuPowerRails_VDIO_HSIC] = Max77663PmuSupply_SD3;

    pRailMap[CardhuPowerRails_VDD_DDR_HS] = Max77663PmuSupply_LDO0;

    pRailMap[CardhuPowerRails_AVDD_PLL_A_P_C_S] = Max77663PmuSupply_LDO1;
    pRailMap[CardhuPowerRails_AVDD_PLLM] = Max77663PmuSupply_LDO1;
    pRailMap[CardhuPowerRails_AVDD_PLLU_D] = Max77663PmuSupply_LDO1;
    pRailMap[CardhuPowerRails_AVDD_PLLU_D2] = Max77663PmuSupply_LDO1;
    pRailMap[CardhuPowerRails_AVDD_PLLX] = Max77663PmuSupply_LDO1;

    pRailMap[CardhuPowerRails_AVDD_DSI_CSI] = Max77663PmuSupply_LDO2;

    pRailMap[CardhuPowerRails_VDDIO_SDMMC4] = Max77663PmuSupply_LDO3;

    pRailMap[CardhuPowerRails_VDD_RTC] = Max77663PmuSupply_LDO4;

    pRailMap[CardhuPowerRails_VDDIO_SDMMC1] = Max77663PmuSupply_LDO5;

    pRailMap[CardhuPowerRails_VDDIO_SYS] = Max77663PmuSupply_LDO6;

    pRailMap[CardhuPowerRails_VCORE_MMC] = Max77663PmuSupply_LDO8;

    pRailMap[CardhuPowerRails_AVDD_SATA] = Max77663PmuSupply_GPIO0;
    pRailMap[CardhuPowerRails_VDD_SATA] = Max77663PmuSupply_GPIO0;
    pRailMap[CardhuPowerRails_AVDD_SATA_PLL] = Max77663PmuSupply_GPIO0;
    pRailMap[CardhuPowerRails_AVDD_PLLE] = Max77663PmuSupply_GPIO0;

    pRailMap[CardhuPowerRails_EN_3V3_SYS] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_AVDD_VDAC] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_LVDS] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_PNL] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VCOM_3V3] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_3V3] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDDIO_PEXCTL] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_HVDD_PEX] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_AVDD_HDMI] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VPP_FUSE] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_AVDD_USB] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_DDR_RX] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VCORE_NAND] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_HVDD_SATA] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDDIO_GMI_PMU] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_AVDD_CAM1] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_AF] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_AVDD_CAM2] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_ACC] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_PHTN] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDDIO_TP] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_LED] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDDIO_CEC] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_CMPS] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_TEMP] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VPP_KFUSE] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDDIO_TS] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_IRLED] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_1WIRE] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_AVDDIO_AUDIO] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_EC] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VCOM_PA] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_3V3_DEVICES] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_3V3_DOCK] = Max77663PmuSupply_GPIO1;
    pRailMap[CardhuPowerRails_VDD_SPKR] = Max77663PmuSupply_GPIO1;

    pRailMap[CardhuPowerRails_EN_5V0] = Max77663PmuSupply_GPIO2;
    pRailMap[CardhuPowerRails_VDD_5V0_SYS] = Max77663PmuSupply_GPIO2;
    pRailMap[CardhuPowerRails_EN_5V_CP] = Max77663PmuSupply_GPIO2;
    pRailMap[CardhuPowerRails_VDD_5V0_SBY] = Max77663PmuSupply_GPIO2;
    pRailMap[CardhuPowerRails_VDD_HALL] = Max77663PmuSupply_GPIO2;
    pRailMap[CardhuPowerRails_VTERM_DDR] = Max77663PmuSupply_GPIO2;
    pRailMap[CardhuPowerRails_V2REF_DDR] = Max77663PmuSupply_GPIO2;
    pRailMap[CardhuPowerRails_VDD_5V0_PMU] = Max77663PmuSupply_GPIO2;
    pRailMap[CardhuPowerRails_VDD_5V0_VBUSR] = Max77663PmuSupply_GPIO2;

    pRailMap[CardhuPowerRails_EN_DDR] = Max77663PmuSupply_GPIO3;
    pRailMap[CardhuPowerRails_MEM_VDDIO_DDR] = Max77663PmuSupply_GPIO3;
    pRailMap[CardhuPowerRails_T30_VDDIO_DDR] = Max77663PmuSupply_GPIO3;

    pRailMap[CardhuPowerRails_LED1] = Max77663PmuSupply_GPIO6;

    pRailMap[CardhuPowerRails_LED2] = Max77663PmuSupply_GPIO7;
}

void
*CardhuPmu_GetMax77663RailMap(
    NvOdmPmuDeviceHandle hPmuDevice,
    PmuChipDriverOpsHandle *hDriverOps,
    NvU32 *pRailMap,
    NvOdmBoardInfo *pProcBoard,
    NvOdmBoardInfo *pPmuBoard)
{
    NvOdmPmuDriverInfoHandle hPmuDriver = NULL;

    hPmuDriver = Max77663PmuDriverOpen(hPmuDevice, s_OdmMax77663RailProps,
                                       Max77663PmuSupply_Num);
    if (!hPmuDriver)
    {
        NvOdmOsPrintf("%s(): The Max77663PmuDriverOpen() fails\n", __func__);
        return NULL;
    }

    InitMapMax77663(pRailMap);

    *hDriverOps = &s_Max77663OdmProps;
    return hPmuDriver;
}

