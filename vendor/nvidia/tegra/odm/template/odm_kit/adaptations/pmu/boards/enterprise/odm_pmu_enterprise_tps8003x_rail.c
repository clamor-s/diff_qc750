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
 * <b>NVIDIA Tegra ODM Kit: Enterprise power rails Implementation</b>
 *
 * @b Description: Implements the enterprise power rail by using tps80031.
 *
 */

#include "nvodm_pmu.h"
#include "nvodm_pmu_enterprise.h"
#include "enterprise_pmu.h"
#include "nvodm_pmu_enterprise_supply_info_table.h"
#include "tps8003x/nvodm_pmu_tps8003x_pmu_driver.h"

static NvOdmPmuRailProps s_OdmRailProps_a02[] = {
    {Tps8003xPmuSupply_Invalid, EnterprisePowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_VIO, EnterprisePowerRails_VDDIO_SYS, 0, 0, 1800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_SMPS1, EnterprisePowerRails_VDD_CPU, 0, 0, 1000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_SMPS2, EnterprisePowerRails_VDD_CORE, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_SMPS3, EnterprisePowerRails_VDDIO_DDR, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_SMPS4, EnterprisePowerRails_AVDD_DSI_CSI, 0, 0, 2100, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_VANA, EnterprisePowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDO1, EnterprisePowerRails_CAM1_SENSOR, 0, 0, 2850, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDO2, EnterprisePowerRails_AVDD_PLLE, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDO3, EnterprisePowerRails_Invalid, 0, 0, 2800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDO4, EnterprisePowerRails_VDDIO_LCD, 0, 0, 3300, NV_FALSE, NV_FALSE, NV_TRUE, NULL},
    {Tps8003xPmuSupply_LDO5, EnterprisePowerRails_AVDD_VDAC, 0, 0, 2850, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDO6, EnterprisePowerRails_VDD_DDR_RX, 0, 0, 2850, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDO7, EnterprisePowerRails_AVDD_PLLA_P_C_S, 0, 0, 1100, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDOLN, EnterprisePowerRails_VDD_DDR_HS, 0, 0, 1000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDOUSB, EnterprisePowerRails_AVDD_USB, 0, 0, 3300, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_VRTC, EnterprisePowerRails_Invalid, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_REGEN1, EnterprisePowerRails_VDD_5V15, 0, 0, 5150, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_REGEN2, EnterprisePowerRails_VDD_3V3, 0, 0, 3300, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_SYSEN, EnterprisePowerRails_HDMI_5V0, 0, 0, 5000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
};

static NvOdmPmuRailProps s_OdmRailProps_a03[] = {
    {Tps8003xPmuSupply_Invalid, EnterprisePowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_VIO, EnterprisePowerRails_VDDIO_SYS, 0, 0, 1800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_SMPS1, EnterprisePowerRails_VDD_CPU, 0, 0, 1000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_SMPS2, EnterprisePowerRails_VDD_CORE, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_SMPS3, EnterprisePowerRails_VDDIO_DDR, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_SMPS4, EnterprisePowerRails_VDD_DDR_RX, 0, 0, 2850, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_VANA, EnterprisePowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDO1, EnterprisePowerRails_VDD_DDR_HS, 0, 0, 1000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDO2, EnterprisePowerRails_AVDD_PLLE, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDO3, EnterprisePowerRails_Invalid, 0, 0, 1350, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDO4, EnterprisePowerRails_AVDD_DSI_CSI, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_TRUE, NULL},
    {Tps8003xPmuSupply_LDO5, EnterprisePowerRails_AVDD_VDAC, 0, 0, 2850, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDO6, EnterprisePowerRails_AVDD_USB_PLL, 0, 0, 1800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDO7, EnterprisePowerRails_VDDIO_LCD, 0, 0, 3000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDOLN, EnterprisePowerRails_AVDD_PLLA_P_C_S, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDOUSB, EnterprisePowerRails_AVDD_USB, 0, 0, 3300, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_VRTC, EnterprisePowerRails_Invalid, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_REGEN1, EnterprisePowerRails_VDD_5V15, 0, 0, 5150, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_REGEN2, EnterprisePowerRails_Invalid, 0, 0, 3300, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_SYSEN, EnterprisePowerRails_HDMI_5V0, 0, 0, 5000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
};
static NvOdmPmuRailProps s_OdmRailProps_tai[] = {
    {Tps8003xPmuSupply_Invalid, EnterprisePowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_VIO, EnterprisePowerRails_VDDIO_SYS, 0, 0, 1800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_SMPS1, EnterprisePowerRails_VDD_CPU, 0, 0, 1000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_SMPS2, EnterprisePowerRails_VDD_CORE, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_SMPS3, EnterprisePowerRails_VDDIO_DDR, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_SMPS4, EnterprisePowerRails_VDD_DDR_RX, 0, 0, 2850, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_VANA, EnterprisePowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDO1, EnterprisePowerRails_VDD_DDR_HS, 0, 0, 1000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDO2, EnterprisePowerRails_AVDD_PLLE, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDO3, EnterprisePowerRails_Invalid, 0, 0, 1350, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDO4, EnterprisePowerRails_AVDD_DSI_CSI, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_TRUE, NULL},
    {Tps8003xPmuSupply_LDO5, EnterprisePowerRails_AVDD_VDAC, 0, 0, 2850, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDO6, EnterprisePowerRails_AVDD_USB_PLL, 0, 0, 1800, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDO7, EnterprisePowerRails_VDDIO_LCD, 0, 0, 3000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDOLN, EnterprisePowerRails_AVDD_PLLA_P_C_S, 0, 0, 1200, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_LDOUSB, EnterprisePowerRails_AVDD_USB, 0, 0, 3300, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_VRTC, EnterprisePowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_REGEN1, EnterprisePowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_REGEN2, EnterprisePowerRails_Invalid, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
    {Tps8003xPmuSupply_SYSEN, EnterprisePowerRails_HDMI_5V0, 0, 0, 5000, NV_FALSE, NV_FALSE, NV_FALSE, NULL},
};


static void Tps80031GetCapsWrap(void *pDriverData, NvU32 vddRail,
        NvOdmPmuVddRailCapabilities* pCapabilities)
{
    Tps8003xPmuGetCaps((NvOdmPmuDriverInfoHandle)pDriverData, vddRail, pCapabilities);
}

static NvBool Tps80031GetVoltageWrap(void *pDriverData, NvU32 vddRail, NvU32* pMilliVolts)
{
    return Tps8003xPmuGetVoltage((NvOdmPmuDriverInfoHandle)pDriverData, vddRail, pMilliVolts);
}

static NvBool Tps80031SetVoltageWrap(void *pDriverData, NvU32 vddRail, NvU32 MilliVolts,
            NvU32* pSettleMicroSeconds)
{
    return Tps8003xPmuSetVoltage((NvOdmPmuDriverInfoHandle)pDriverData, vddRail, MilliVolts,
                        pSettleMicroSeconds);
}

static PmuChipDriverOps s_tps80031Ops = {
    Tps80031GetCapsWrap, Tps80031GetVoltageWrap, Tps80031SetVoltageWrap,
};

static void InitMap(NvU32 *pRailMap, NvOdmBoardInfo *pProcBoard)
{
    pRailMap[EnterprisePowerRails_VDD_CPU]         = Tps8003xPmuSupply_SMPS1;
    pRailMap[EnterprisePowerRails_VDD_CORE]        = Tps8003xPmuSupply_SMPS2;
    pRailMap[EnterprisePowerRails_VDD_RTC]         = Tps8003xPmuSupply_LDO2;
    pRailMap[EnterprisePowerRails_VDDIO_SYS]       = Tps8003xPmuSupply_VIO;
    pRailMap[EnterprisePowerRails_VDDIO_UART]      = Tps8003xPmuSupply_VIO;
    pRailMap[EnterprisePowerRails_VDDIO_BB]        = Tps8003xPmuSupply_VIO;
    pRailMap[EnterprisePowerRails_AVDD_OSC]        = Tps8003xPmuSupply_VIO;
    pRailMap[EnterprisePowerRails_VDDIO_AUDIO]     = Tps8003xPmuSupply_VIO;
    pRailMap[EnterprisePowerRails_VDDIO_GMI]       = Tps8003xPmuSupply_VIO;
    pRailMap[EnterprisePowerRails_AVDD_USB]        = Tps8003xPmuSupply_LDOUSB;
    pRailMap[EnterprisePowerRails_VDDIO_CAM]       = Tps8003xPmuSupply_VIO;
    pRailMap[EnterprisePowerRails_VDDIO_SDMMC1]    = Tps8003xPmuSupply_VIO;
    pRailMap[EnterprisePowerRails_VDDIO_SDMMC3]    = Tps8003xPmuSupply_LDO6;
//    pRailMap[EnterprisePowerRails_VDDIO_SDMMC4]    = Tps8003xPmuSupply_
    pRailMap[EnterprisePowerRails_AVDD_VDAC]       = Tps8003xPmuSupply_LDO5;
    pRailMap[EnterprisePowerRails_VPP_FUSE]        = Tps8003xPmuSupply_LDO5;
//    pRailMap[EnterprisePowerRails_VDDIO_HSIC]      = Tps8003xPmuSupply_
//    pRailMap[EnterprisePowerRails_VDDIO_IC_USB]    = Tps8003xPmuSupply_
    pRailMap[EnterprisePowerRails_AVDD_HDMI_PLL]   = Tps8003xPmuSupply_LDOUSB;
    pRailMap[EnterprisePowerRails_VDDIO_DDR]       = Tps8003xPmuSupply_SMPS3;
    pRailMap[EnterprisePowerRails_VDDIO_VI]        = Tps8003xPmuSupply_VIO;

    pRailMap[EnterprisePowerRails_VDDIO_LCD_PMU]   = Tps8003xPmuSupply_VIO;
    pRailMap[EnterprisePowerRails_AVDD_PLLE]       = Tps8003xPmuSupply_LDO2;

    if (pProcBoard->BoardID != PROC_BOARD_E1239)
    {
        pRailMap[EnterprisePowerRails_VDD_5V15]    = Tps8003xPmuSupply_REGEN1;
    }
    pRailMap[EnterprisePowerRails_HDMI_5V0]        = Tps8003xPmuSupply_SYSEN;

    if ((pProcBoard->Fab < 0x3) && (pProcBoard->BoardID != PROC_BOARD_E1239))
    {
        pRailMap[EnterprisePowerRails_VDD_DDR_HS]      = Tps8003xPmuSupply_LDOLN;
        pRailMap[EnterprisePowerRails_AVDD_PLLA_P_C_S] = Tps8003xPmuSupply_LDO7;
        pRailMap[EnterprisePowerRails_AVDD_PLLM]       = Tps8003xPmuSupply_LDO7;
        pRailMap[EnterprisePowerRails_AVDD_PLLU_D]     = Tps8003xPmuSupply_LDO7;
        pRailMap[EnterprisePowerRails_AVDD_PLLU_D2]     = Tps8003xPmuSupply_LDO7;
        pRailMap[EnterprisePowerRails_AVDD_PLLX]       = Tps8003xPmuSupply_LDO7;
        pRailMap[EnterprisePowerRails_VDDIO_LCD]       = Tps8003xPmuSupply_LDO4;
        pRailMap[EnterprisePowerRails_AVDD_USB_PLL]    = Tps8003xPmuSupply_VIO;
        pRailMap[EnterprisePowerRails_VDD_DDR_RX]      = Tps8003xPmuSupply_LDO6;
        pRailMap[EnterprisePowerRails_AVDD_DSI_CSI]    = Tps8003xPmuSupply_SMPS4;
        pRailMap[EnterprisePowerRails_AVDD_HDMI]       = Tps8003xPmuSupply_LDO4;
        pRailMap[EnterprisePowerRails_VCORE_LCD]       = Tps8003xPmuSupply_LDO4;
        pRailMap[EnterprisePowerRails_CAM1_SENSOR]     = Tps8003xPmuSupply_LDO1;
        pRailMap[EnterprisePowerRails_CAM2_SENSOR]     = Tps8003xPmuSupply_LDO1;
        pRailMap[EnterprisePowerRails_VDD_3V3]         = Tps8003xPmuSupply_REGEN2;
    }
    else
    {
        pRailMap[EnterprisePowerRails_VDD_DDR_HS]      = Tps8003xPmuSupply_LDO1;
        pRailMap[EnterprisePowerRails_AVDD_PLLA_P_C_S] = Tps8003xPmuSupply_LDOLN;
        pRailMap[EnterprisePowerRails_AVDD_PLLM]       = Tps8003xPmuSupply_LDOLN;
        pRailMap[EnterprisePowerRails_AVDD_PLLU_D]     = Tps8003xPmuSupply_LDOLN;
        pRailMap[EnterprisePowerRails_AVDD_PLLU_D2]     = Tps8003xPmuSupply_LDOLN;
        pRailMap[EnterprisePowerRails_AVDD_PLLX]       = Tps8003xPmuSupply_LDOLN;
        pRailMap[EnterprisePowerRails_VDDIO_LCD]       = Tps8003xPmuSupply_LDO7;
        pRailMap[EnterprisePowerRails_AVDD_USB_PLL]    = Tps8003xPmuSupply_LDO6;
        pRailMap[EnterprisePowerRails_VDD_DDR_RX]      = Tps8003xPmuSupply_SMPS4;
        pRailMap[EnterprisePowerRails_AVDD_DSI_CSI]    = Tps8003xPmuSupply_LDO1;
        pRailMap[EnterprisePowerRails_AVDD_HDMI]       = Tps8003xPmuSupply_LDOUSB;
        pRailMap[EnterprisePowerRails_VCORE_LCD]       = Tps8003xPmuSupply_LDO7;
        pRailMap[EnterprisePowerRails_CAM1_SENSOR]     = Tps8003xPmuSupply_LDO4;
        pRailMap[EnterprisePowerRails_CAM2_SENSOR]     = Tps8003xPmuSupply_LDO4;
        if (pProcBoard->BoardID != PROC_BOARD_E1239)
        {
            pRailMap[EnterprisePowerRails_VDD_3V3]         = Tps8003xPmuSupply_LDOUSB;
        }
    }
}

void *EnterprisePmu_GetTps80031Map(NvOdmPmuDeviceHandle hDevice,
    PmuChipDriverOpsHandle *hDriverOps, NvU32 *pRailMap,
    NvOdmBoardInfo *pProcBoard, NvOdmBoardInfo *pPmuBoard)
{

    NvOdmPmuDriverInfoHandle hPmuDriver = NULL;

    if (pProcBoard->BoardID == PROC_BOARD_E1239)
    {
        hPmuDriver = Tps8003xPmuDriverOpen(hDevice, s_OdmRailProps_tai, Tps8003xPmuSupply_Num);
    }
    else
    {
        if (pProcBoard->Fab < 0x3)
            hPmuDriver = Tps8003xPmuDriverOpen(hDevice, s_OdmRailProps_a02, Tps8003xPmuSupply_Num);
        else
            hPmuDriver = Tps8003xPmuDriverOpen(hDevice, s_OdmRailProps_a03, Tps8003xPmuSupply_Num);
    }

    if (!hPmuDriver)
    {
        NvOdmOsPrintf("%s(): The Tps8003xPmuDriverOpen() fails\n", __func__);
        return NULL;
    }

    InitMap(pRailMap, pProcBoard);

    *hDriverOps = &s_tps80031Ops;
    return hPmuDriver;
}
