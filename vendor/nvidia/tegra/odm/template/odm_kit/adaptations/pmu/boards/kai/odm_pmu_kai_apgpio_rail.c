/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
 /**
  * @file
  * <b>NVIDIA Tegra ODM Kit: Kai pmu rail driver using apgpio pmu driver</b>
  *
  * @b Description: Implements the pmu controls api based on AP GPIO pmu driver.
  */

#include "nvodm_pmu.h"
#include "nvodm_pmu_kai.h"
#include "kai_pmu.h"
#include "nvodm_pmu_kai_supply_info_table.h"
#include "apgpio/nvodm_pmu_apgpio.h"
#include "nvodm_query_gpio.h"

typedef enum
{
    KaiAPGpioRails_Invalid = 0,

    // AP GPIO (GPIO_PBB4)
    KaiAPGpioRails_VDD_1V8_CAM,
    // AP GPIO (LCD_M1)
    KaiAPGpioRails_VCORE_LCD,
    // AP GPIO (GMI_AD8)
    KaiAPGpioRails_VDD_BACKLIGHT,
    // AP GPIO (BB_DAP3_DOUT)
    KaiAPGpioRails_VDD_HDMI_CON,
    // AP GPIO (BB_DAP3_FS)
    KaiAPGpioRails_VDD_MINI_CARD,
    // AP GPIO (LCD_M1)
    KaiAPGpioRails_VDD_LCD_PANEL,
    // AP GPIO (KB_ROW8)
    KaiAPGpioRails_VDD_CAM3,
    // AP GPIO (SDMMC3_DAT5)
    KaiAPGpioRails_VDD_COM_BD,
    // AP GPIO (BB_DAP3_DIN)
    KaiAPGpioRails_VDD_SD_SLOT,
    // AP GPIO (LCD_PW1)
    KaiAPGpioRails_VPP_FUSE,

    KaiAPGpioRails_Num,
    KaiAPGpioRails_Force32 = 0x7FFFFFFF,
} KaiAPGpioRails;

/* RailId, GpioPort,pioPin,
   IsActiveLow, IsInitEnable, OnValue, OffValue,
   Enable, Disable, IsOdmProt, Delay
*/
#define PORT(x) ((x) - 'a')
static ApGpioInfo s_KaiApiGpioInfo[KaiAPGpioRails_Num] =
{
    {   // Invalid
        KaiAPGpioRails_Invalid, KaiPowerRails_Invalid, 0, 0,
        NV_FALSE, NV_FALSE, 0, 0,
        NULL, NULL,
        NV_FALSE, 0,
    },

    {   // GPIO_PBB4 BB.04
        KaiAPGpioRails_VDD_1V8_CAM, KaiPowerRails_Invalid, 26 + PORT('b'), 4,
        NV_FALSE, NV_FALSE, 1800, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },

    {   // LCD_M1 W.01
        KaiAPGpioRails_VCORE_LCD, KaiPowerRails_Invalid, PORT('w'), 1,
        NV_FALSE, NV_FALSE, 11000, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },

    {   // GMI_AD8 H.00
        KaiAPGpioRails_VDD_BACKLIGHT, KaiPowerRails_Invalid, PORT('h'), 0,
        NV_FALSE, NV_FALSE, 9750, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },

    {   // BB_DAP3_DOUT P.02
        KaiAPGpioRails_VDD_HDMI_CON, KaiPowerRails_Invalid, PORT('p'), 2,
        NV_FALSE, NV_FALSE, 5000, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },

    {   // BB_DAP3_FS P.00
        KaiAPGpioRails_VDD_MINI_CARD, KaiPowerRails_Invalid, PORT('p'), 0,
        NV_FALSE, NV_FALSE, 3300, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },

    {   // LCD_M1 W.01
        KaiAPGpioRails_VDD_LCD_PANEL, KaiPowerRails_Invalid, PORT('w'), 1,
        NV_FALSE, NV_FALSE, 3300, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },

    {   // KB_ROW8 S.00
        KaiAPGpioRails_VDD_CAM3, KaiPowerRails_Invalid, PORT('s'), 0,
        NV_FALSE, NV_FALSE, 3300, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },

    {   // SDMMC3_DAT5 D.00
        KaiAPGpioRails_VDD_COM_BD, KaiPowerRails_Invalid, PORT('d'), 0,
        NV_FALSE, NV_FALSE, 3300, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },

    {   // BB_DAP3_DIN P.01
        KaiAPGpioRails_VDD_SD_SLOT, KaiPowerRails_Invalid, PORT('p'), 1,
        NV_FALSE, NV_FALSE, 3300, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },

    {   // LCD_PW1 C.01
        KaiAPGpioRails_VPP_FUSE, KaiPowerRails_Invalid, PORT('c'), 1,
        NV_FALSE, NV_FALSE, 3300, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },
};

static void ApGpioGetCapsWrap(void *pDriverData, NvU32 vddRail,
        NvOdmPmuVddRailCapabilities* pCapabilities)
{
        OdmPmuApGpioGetCapabilities((NvOdmPmuApGpioDriverHandle)pDriverData, vddRail,
                     pCapabilities);
}

static NvBool ApGpioGetVoltageWrap(void *pDriverData, NvU32 vddRail, NvU32* pMilliVolts)
{
        return OdmPmuApGpioGetVoltage((NvOdmPmuApGpioDriverHandle)pDriverData, vddRail, pMilliVolts);
}

static NvBool ApGpioSetVoltageWrap(void *pDriverData, NvU32 vddRail, NvU32 MilliVolts,
                NvU32* pSettleMicroSeconds)
{
        return OdmPmuApGpioSetVoltage((NvOdmPmuApGpioDriverHandle)pDriverData, vddRail, MilliVolts,
                        pSettleMicroSeconds);
}

static PmuChipDriverOps s_PmuApGpioDriverOps =
{ApGpioGetCapsWrap, ApGpioGetVoltageWrap, ApGpioSetVoltageWrap};

static void InitGpioRailMap(NvU32 *pRailMap)
{
    pRailMap[KaiPowerRails_VDD_1V8_CAM] = KaiAPGpioRails_VDD_1V8_CAM;
    pRailMap[KaiPowerRails_VCORE_LCD] = KaiAPGpioRails_VCORE_LCD;
    pRailMap[KaiPowerRails_VDD_BACKLIGHT] = KaiAPGpioRails_VDD_BACKLIGHT;
    pRailMap[KaiPowerRails_VDD_HDMI_CON] = KaiAPGpioRails_VDD_HDMI_CON;
    pRailMap[KaiPowerRails_VDD_MINI_CARD] = KaiAPGpioRails_VDD_MINI_CARD;
    pRailMap[KaiPowerRails_VDD_LCD_PANEL] = KaiAPGpioRails_VDD_LCD_PANEL;
    pRailMap[KaiPowerRails_VDD_CAM3] = KaiAPGpioRails_VDD_CAM3;
    pRailMap[KaiPowerRails_VDD_COM_BD] = KaiAPGpioRails_VDD_COM_BD;
    pRailMap[KaiPowerRails_VDD_SD_SLOT] = KaiAPGpioRails_VDD_SD_SLOT;
    pRailMap[KaiPowerRails_VPP_FUSE] = KaiAPGpioRails_VPP_FUSE;

    s_KaiApiGpioInfo[KaiAPGpioRails_VDD_1V8_CAM].SystemRailId = KaiPowerRails_VDD_1V8_CAM;
    s_KaiApiGpioInfo[KaiAPGpioRails_VCORE_LCD].SystemRailId = KaiPowerRails_VCORE_LCD;
    s_KaiApiGpioInfo[KaiAPGpioRails_VDD_BACKLIGHT].SystemRailId = KaiPowerRails_VDD_BACKLIGHT;
    s_KaiApiGpioInfo[KaiAPGpioRails_VDD_HDMI_CON].SystemRailId = KaiPowerRails_VDD_HDMI_CON;
    s_KaiApiGpioInfo[KaiAPGpioRails_VDD_MINI_CARD].SystemRailId = KaiPowerRails_VDD_MINI_CARD;
    s_KaiApiGpioInfo[KaiAPGpioRails_VDD_LCD_PANEL].SystemRailId = KaiPowerRails_VDD_LCD_PANEL;
    s_KaiApiGpioInfo[KaiAPGpioRails_VDD_CAM3].SystemRailId = KaiPowerRails_VDD_CAM3;
    s_KaiApiGpioInfo[KaiAPGpioRails_VDD_COM_BD].SystemRailId = KaiPowerRails_VDD_COM_BD;
    s_KaiApiGpioInfo[KaiAPGpioRails_VDD_SD_SLOT].SystemRailId = KaiPowerRails_VDD_SD_SLOT;
    s_KaiApiGpioInfo[KaiAPGpioRails_VPP_FUSE].SystemRailId = KaiPowerRails_VPP_FUSE;
}

void *KaiPmu_GetApGpioRailMap(NvOdmPmuDeviceHandle hPmuDevice,
    PmuChipDriverOpsHandle *hDriverOps, NvU32 *pRailMap, NvOdmBoardInfo *pProcBoard,
    NvOdmBoardInfo *pPmuBoard)
{

    NvOdmPmuApGpioDriverHandle hApGpioDriver = NULL;

    hApGpioDriver = OdmPmuApGpioOpen(hPmuDevice, s_KaiApiGpioInfo,
                                    KaiAPGpioRails_Num);
    if (!hApGpioDriver)
    {
        NvOdmOsPrintf("%s(): Fails in opening GpioPmu Handle\n");
        return NULL;
    }
    InitGpioRailMap(pRailMap);
    *hDriverOps = &s_PmuApGpioDriverOps;
    return hApGpioDriver;
}
