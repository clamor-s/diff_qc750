/*
 * Copyright (c) 2011-2012 NVIDIA Corporation.  All rights reserved.
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
#include "tps6591x/nvodm_pmu_tps6591x_pmu_driver.h"

static NvOdmPmuRailProps s_OdmRailProps[] = {
    {TPS6591xPmuSupply_Invalid, CardhuPowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_VIO, CardhuPowerRails_VDDIO_SYS, 0, 0, 1800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_VDD1, CardhuPowerRails_VDD_CORE, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_VDD2, CardhuPowerRails_VDD_GEN1V5, 0, 0, 1500, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_VDDCTRL, CardhuPowerRails_VDD_CPU_PMU, 0, 0, 1000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_LDO1, CardhuPowerRails_AVDD_PEXB, 0, 0, 1050, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_LDO2, CardhuPowerRails_AVDD_SATA, 0, 0, 1050, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_LDO3, CardhuPowerRails_VDDIO_SDMMC1, 0, 0, 3300, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_LDO4, CardhuPowerRails_VDD_RTC, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_LDO5, CardhuPowerRails_AVDD_VDAC, 0, 0, 2800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_LDO6, CardhuPowerRails_AVDD_DSI_CSI, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_LDO7, CardhuPowerRails_AVDD_PLL_A_P_C_S, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_LDO8, CardhuPowerRails_VDD_DDR_HS, 0, 0, 1000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_RTC_OUT, CardhuPowerRails_Invalid, 0, 0, 1800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_GPO0, CardhuPowerRails_EN_5V_CP, 0, 0, 5000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_GPO1, CardhuPowerRails_LED1, 0, 0, 1050, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_GPO2, CardhuPowerRails_EN_5V0, 0, 0, 5000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_GPO3, CardhuPowerRails_LED2, 0, 0, 1050, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_GPO4, CardhuPowerRails_Invalid, 0, 0, 5000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_GPO5, CardhuPowerRails_Invalid, 0, 0, 5000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_GPO6, CardhuPowerRails_V2REF_DDR, 0, 0, 1050, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_GPO7, CardhuPowerRails_VDD_3V3_DEVICES, 0, 0, 3300, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {TPS6591xPmuSupply_GPO8, CardhuPowerRails_Invalid, 0, 0, 5000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
};

static PmuChipDriverOps s_tps6591xOps = {
    Tps6591xPmuGetCaps, Tps6591xPmuGetVoltage, Tps6591xPmuSetVoltage,
};

static void InitMap(NvU32 *pRailMap)
{
    pRailMap[CardhuPowerRails_VDD_RTC] = TPS6591xPmuSupply_LDO4;
//    pRailMap[CardhuPowerRails_VDD_CORE] = TPS6591xPmuSupply_VDD1;
    pRailMap[CardhuPowerRails_EN_VDDIO_DDR_1V2] = TPS6591xPmuSupply_VDD1;
    pRailMap[CardhuPowerRails_VDD_GEN1V5] = TPS6591xPmuSupply_VDD2;
    pRailMap[CardhuPowerRails_VCORE_LCD] = TPS6591xPmuSupply_VDD2;
    pRailMap[CardhuPowerRails_TRACK_LDO1] = TPS6591xPmuSupply_VDD2;
    pRailMap[CardhuPowerRails_EXTERNAL_LDO_1V2] = TPS6591xPmuSupply_VDD2;
    pRailMap[CardhuPowerRails_VCORE_CAM1] = TPS6591xPmuSupply_VDD2;
    pRailMap[CardhuPowerRails_VCORE_CAM2] = TPS6591xPmuSupply_VDD2;
    pRailMap[CardhuPowerRails_VDD_CPU_PMU] = TPS6591xPmuSupply_VDDCTRL;
    pRailMap[CardhuPowerRails_AVDD_HDMI_PLL] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_AVDD_USB_PLL] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_AVDD_OSC] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_VDDIO_SYS] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_VDDIO_SDMMC4] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_VDD_1V8_SATELITE] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_VDDIO_UART] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_VDDIO_AUDIO] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_VDDIO_BB] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_VDDIO_LCD_PMU] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_VDDIO_CAM] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_VDDIO_VI] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_LDO6] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_LDO7] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_LDO8] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_VCORE_AUDIO] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_AVCORE_AUDIO] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_VDDIO_SDMMC3] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_VCORE1_LPDDR2] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_VCOM_1V8] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_PMUIO_1V8] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_AVDD_IC_USB] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_AVDD_PEXB] = TPS6591xPmuSupply_LDO1;
    pRailMap[CardhuPowerRails_VDD_PEXB] = TPS6591xPmuSupply_LDO1;
    pRailMap[CardhuPowerRails_AVDD_PEX_PLL] = TPS6591xPmuSupply_LDO1;
    pRailMap[CardhuPowerRails_AVDD_PEXA] = TPS6591xPmuSupply_LDO1;
    pRailMap[CardhuPowerRails_VDD_PEXA] = TPS6591xPmuSupply_LDO1;
    pRailMap[CardhuPowerRails_AVDD_SATA] = TPS6591xPmuSupply_LDO2;
    pRailMap[CardhuPowerRails_VDD_SATA] = TPS6591xPmuSupply_LDO2;
    pRailMap[CardhuPowerRails_AVDD_SATA_PLL] = TPS6591xPmuSupply_LDO2;
    pRailMap[CardhuPowerRails_AVDD_PLLE] = TPS6591xPmuSupply_LDO2;
    pRailMap[CardhuPowerRails_VDDIO_SDMMC1] = TPS6591xPmuSupply_LDO5;
    pRailMap[CardhuPowerRails_AVDD_VDAC] = TPS6591xPmuSupply_LDO5;
    pRailMap[CardhuPowerRails_AVDD_DSI_CSI] = TPS6591xPmuSupply_LDO6;
    pRailMap[CardhuPowerRails_AVDD_PLL_A_P_C_S] = TPS6591xPmuSupply_LDO7;
    pRailMap[CardhuPowerRails_AVDD_PLLM] = TPS6591xPmuSupply_LDO7;
    pRailMap[CardhuPowerRails_AVDD_PLLU_D] = TPS6591xPmuSupply_LDO7;
    pRailMap[CardhuPowerRails_AVDD_PLLU_D2] = TPS6591xPmuSupply_LDO7;
    pRailMap[CardhuPowerRails_AVDD_PLLX] = TPS6591xPmuSupply_LDO7;
    pRailMap[CardhuPowerRails_VDD_DDR_HS] = TPS6591xPmuSupply_LDO8;
    pRailMap[CardhuPowerRails_VDD_LVDS] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDD_PNL] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VCOM_3V3] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDD_3V3] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VCORE_MMC] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDDIO_PEXCTL] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_HVDD_PEX] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_AVDD_HDMI] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VPP_FUSE] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_AVDD_USB] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDD_DDR_RX] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VCORE_NAND] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_HVDD_SATA] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDDIO_GMI_PMU] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_AVDD_CAM1] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDD_AF] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_AVDD_CAM2] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDD_ACC] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDD_PHTN] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDDIO_TP] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDD_LED] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDDIO_CEC] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDD_CMPS] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDD_TEMP] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VPP_KFUSE] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDDIO_TS] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDD_IRLED] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDD_1WIRE] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_AVDDIO_AUDIO] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDD_EC] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VCOM_PA] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDD_HALL] = TPS6591xPmuSupply_GPO0;
    pRailMap[CardhuPowerRails_USB1_VBUS] = TPS6591xPmuSupply_GPO2;
    pRailMap[CardhuPowerRails_VDDIO_VGA] = TPS6591xPmuSupply_GPO2;
    pRailMap[CardhuPowerRails_VDD_SPKR] = TPS6591xPmuSupply_GPO2;
    pRailMap[CardhuPowerRails_VDDIO_HDMI] = TPS6591xPmuSupply_GPO2;
    pRailMap[CardhuPowerRails_USB2_VBUS] = TPS6591xPmuSupply_GPO2;
    pRailMap[CardhuPowerRails_USB3_VBUS] = TPS6591xPmuSupply_GPO2;
//    pRailMap[CardhuPowerRails_VCOM_3V0] = TPS6591xPmuSupply_
//    pRailMap[CardhuPowerRails_AVDD_IC_USB_3V0] = TPS6591xPmuSupply_
//    pRailMap[CardhuPowerRails_VCOM_1V2] = TPS6591xPmuSupply_
//    pRailMap[CardhuPowerRails_VDIO_HSIC] = TPS6591xPmuSupply_
//    pRailMap[CardhuPowerRails_VDD_VBRTR] = TPS6591xPmuSupply_
//    pRailMap[CardhuPowerRails_AVDD_LCD1] = TPS6591xPmuSupply_
//    pRailMap[CardhuPowerRails_AVDD_LCD2] = TPS6591xPmuSupply_
//    pRailMap[CardhuPowerRails_VDD_BL1_P] = TPS6591xPmuSupply_
//    pRailMap[CardhuPowerRails_VDD_BL2_P] = TPS6591xPmuSupply_
    pRailMap[CardhuPowerRails_VDD_GEN1V8] = TPS6591xPmuSupply_VIO;
    pRailMap[CardhuPowerRails_EN_5V_CP] = TPS6591xPmuSupply_GPO0;
    pRailMap[CardhuPowerRails_EN_DDR] = TPS6591xPmuSupply_GPO6;
    pRailMap[CardhuPowerRails_EN_3V3_SYS] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_EN_5V0] = TPS6591xPmuSupply_GPO2;
    pRailMap[CardhuPowerRails_LED1] = TPS6591xPmuSupply_GPO1;
    pRailMap[CardhuPowerRails_LED2] = TPS6591xPmuSupply_GPO3;
    pRailMap[CardhuPowerRails_VDD_5V0_SBY] = TPS6591xPmuSupply_GPO0;
    pRailMap[CardhuPowerRails_VTERM_DDR] = TPS6591xPmuSupply_GPO6;
    pRailMap[CardhuPowerRails_V2REF_DDR] = TPS6591xPmuSupply_GPO6;
    pRailMap[CardhuPowerRails_MEM_VDDIO_DDR] = TPS6591xPmuSupply_GPO6;
    pRailMap[CardhuPowerRails_T30_VDDIO_DDR] = TPS6591xPmuSupply_GPO6;

    pRailMap[CardhuPowerRails_VDD_5V0_SYS] = TPS6591xPmuSupply_GPO2;
    pRailMap[CardhuPowerRails_TOUCHPANEL_3V3] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDD_3V3_DEVICES] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDD_3V3_DOCK] = TPS6591xPmuSupply_GPO7;
    pRailMap[CardhuPowerRails_VDD_1V8_DEVICES] = TPS6591xPmuSupply_VIO;

    pRailMap[CardhuPowerRails_VDD_5V0_PMU] = TPS6591xPmuSupply_GPO2;
    pRailMap[CardhuPowerRails_VDD_5V0_VBUSR] = TPS6591xPmuSupply_GPO2;
}

void *CardhuPmu_GetTps6591xMap(NvOdmPmuDeviceHandle hDevice,
    PmuChipDriverOpsHandle *hDriverOps, NvU32 *pRailMap,
    NvOdmBoardInfo *pProcBoard, NvOdmBoardInfo *pPmuBoard)
{
    int vsels;
    NvOdmPmuDriverInfoHandle hPmuDriver = NULL;

    hPmuDriver = Tps6591xPmuDriverOpen(hDevice, s_OdmRailProps, TPS6591xPmuSupply_Num);
    if (!hPmuDriver)
    {
        NvOdmOsPrintf("%s(): The Tps6591xSetup() fails\n", __func__);
        return NULL;
    }

    InitMap(pRailMap);

    if (pProcBoard->BoardID == PROC_BOARD_E1198)
    {
        switch(pProcBoard->Fab) {
        case BOARD_FAB_A00:
        case BOARD_FAB_A01:
        case BOARD_FAB_A02:
            if (!((pProcBoard->SKU & SKU_DCDC_TPS62361_SUPPORT) ||
                (pPmuBoard->SKU & SKU_DCDC_TPS62361_SUPPORT)))
                pRailMap[CardhuPowerRails_VDD_CORE] = TPS6591xPmuSupply_VDD1;
            break;
        case BOARD_FAB_A03:
            vsels = (((pProcBoard->SKU >> 1) & 0x2) | (pProcBoard->SKU & 0x1));
            switch(vsels) {
            case 0:
                pRailMap[CardhuPowerRails_VDD_CORE] = TPS6591xPmuSupply_VDD1;
                break;
            }
        }
    } else if (!((pProcBoard->SKU & SKU_DCDC_TPS62361_SUPPORT) ||
                (pPmuBoard->SKU & SKU_DCDC_TPS62361_SUPPORT)))
    {
        pRailMap[CardhuPowerRails_VDD_CORE] = TPS6591xPmuSupply_VDD1;
    }


    *hDriverOps = &s_tps6591xOps;
    return hPmuDriver;
}
