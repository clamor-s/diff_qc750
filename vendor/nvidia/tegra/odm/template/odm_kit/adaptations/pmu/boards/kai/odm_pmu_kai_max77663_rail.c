/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_pmu.h"
#include "nvodm_pmu_kai.h"
#include "kai_pmu.h"
#include "nvodm_pmu_kai_supply_info_table.h"
#include "max77663/nvodm_pmu_max77663_pmu_driver.h"

static NvOdmPmuRailProps s_OdmMax77663RailProps[] = {
    {Max77663PmuSupply_Invalid, KaiPowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_SD0, KaiPowerRails_VDD_CPU, 0, 0, 1000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_SD1, KaiPowerRails_VDD_CORE, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_SD2, KaiPowerRails_VDD_GEN1V8, 0, 0, 1800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_SD3, KaiPowerRails_VDD_DDR3L_1V35, 0, 0, 1350, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_LDO0, KaiPowerRails_VDD_DDR_HS, 0, 0, 1000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_LDO1, KaiPowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_LDO2, KaiPowerRails_VDD_DDR_RX, 0, 0, 2800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_LDO3, KaiPowerRails_VDD_EMMC_CORE, 0, 0, 2850, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_LDO4, KaiPowerRails_VDD_RTC, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_LDO5, KaiPowerRails_VDD_SENSOR_2V8, 0, 0, 2800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_LDO6, KaiPowerRails_VDDIO_SDMMC1, 0, 0, 3300, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_LDO7, KaiPowerRails_AVDD_DSI_CSI, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_LDO8, KaiPowerRails_AVDD_PLLS, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_GPIO0, KaiPowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_GPIO1, KaiPowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_GPIO2, KaiPowerRails_EN_AVDD_HDMI_USB, 0, 0, 3300, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_GPIO3, KaiPowerRails_EN_3V3_SYS, 0, 0, 3300, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_GPIO4, KaiPowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_GPIO5, KaiPowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_GPIO6, KaiPowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Max77663PmuSupply_GPIO7, KaiPowerRails_LED2, 0, 0, 1800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
};

static PmuChipDriverOps s_Max77663OdmProps = {
    Max77663PmuGetCaps, Max77663PmuGetVoltage, Max77663PmuSetVoltage,
};

static void InitMapMax77663(NvU32 *pRailMap)
{
    pRailMap[KaiPowerRails_VDD_CPU] = Max77663PmuSupply_SD0;

    pRailMap[KaiPowerRails_VDD_CORE] = Max77663PmuSupply_SD1;

    pRailMap[KaiPowerRails_VDD_GEN1V8] = Max77663PmuSupply_SD2;
    pRailMap[KaiPowerRails_AVDD_HDMI_PLL] = Max77663PmuSupply_SD2;
    pRailMap[KaiPowerRails_AVDD_USB_PLL] = Max77663PmuSupply_SD2;
    pRailMap[KaiPowerRails_AVDD_OSC] = Max77663PmuSupply_SD2;
    pRailMap[KaiPowerRails_VDDIO_SYS] = Max77663PmuSupply_SD2;
    pRailMap[KaiPowerRails_VDDIO_SDMMC4] = Max77663PmuSupply_SD2;
    pRailMap[KaiPowerRails_VDDIO_UART] = Max77663PmuSupply_SD2;
    pRailMap[KaiPowerRails_VDDIO_BB] = Max77663PmuSupply_SD2;
    pRailMap[KaiPowerRails_VDDIO_LCD] = Max77663PmuSupply_SD2;
    pRailMap[KaiPowerRails_VDDIO_AUDIO] = Max77663PmuSupply_SD2;
    pRailMap[KaiPowerRails_VDDIO_CAM] = Max77663PmuSupply_SD2;
    pRailMap[KaiPowerRails_VDDIO_SDMMC3] = Max77663PmuSupply_SD2;
    pRailMap[KaiPowerRails_VDDIO_VI] = Max77663PmuSupply_SD2;

    pRailMap[KaiPowerRails_VDD_DDR3L_1V35] = Max77663PmuSupply_SD3;

    pRailMap[KaiPowerRails_VDD_DDR_HS] = Max77663PmuSupply_LDO0;

    pRailMap[KaiPowerRails_VDD_DDR_RX] = Max77663PmuSupply_LDO2;

    pRailMap[KaiPowerRails_VDD_EMMC_CORE] = Max77663PmuSupply_LDO3;

    pRailMap[KaiPowerRails_VDD_RTC] = Max77663PmuSupply_LDO4;

    pRailMap[KaiPowerRails_VDD_SENSOR_2V8] = Max77663PmuSupply_LDO5;

    pRailMap[KaiPowerRails_VDDIO_SDMMC1] = Max77663PmuSupply_LDO6;

    pRailMap[KaiPowerRails_AVDD_DSI_CSI] = Max77663PmuSupply_LDO7;

    pRailMap[KaiPowerRails_AVDD_PLLS] = Max77663PmuSupply_LDO8;

    pRailMap[KaiPowerRails_EN_AVDD_HDMI_USB] = Max77663PmuSupply_GPIO2;
    pRailMap[KaiPowerRails_AVDD_HDMI] = Max77663PmuSupply_GPIO2;
    pRailMap[KaiPowerRails_AVDD_USB] = Max77663PmuSupply_GPIO2;
    pRailMap[KaiPowerRails_VDDIO_GMI] = Max77663PmuSupply_GPIO2;

    pRailMap[KaiPowerRails_EN_3V3_SYS] = Max77663PmuSupply_GPIO3;
    pRailMap[KaiPowerRails_VDD_3V3_SYS] = Max77663PmuSupply_GPIO3;
    pRailMap[KaiPowerRails_VCORE_LCD] = Max77663PmuSupply_GPIO3;

    pRailMap[KaiPowerRails_LED2] = Max77663PmuSupply_GPIO7;
}

void
*KaiPmu_GetMax77663RailMap(
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

