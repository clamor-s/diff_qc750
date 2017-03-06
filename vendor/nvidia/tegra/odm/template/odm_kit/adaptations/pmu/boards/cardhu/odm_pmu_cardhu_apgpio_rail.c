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
  * <b>NVIDIA Tegra ODM Kit: Cardhu pmu rail driver using apgpio pmu driver</b>
  *
  * @b Description: Implements the pmu controls api based on AP GPIO pmu driver.
  */

#include "nvodm_pmu.h"
#include "nvodm_pmu_cardhu.h"
#include "cardhu_pmu.h"
#include "nvodm_pmu_cardhu_supply_info_table.h"
#include "apgpio/nvodm_pmu_apgpio.h"
#include "nvodm_query_gpio.h"

typedef enum
{
    CardhuAPGpioRails_Invalid = 0,

    // AP GPIO (GMI_CS2)
    CardhuAPGpioRails_VDD_BACKLIGHT,
    // AP GPIO (VI_VSYNCH)
    CardhuAPGpioRails_VDD_3V3_MINI_CARD,
    // AP GPIO (VI_D6)
    CardhuAPGpioRails_VDD_LCD_PANEL_3V3,
    // AP GPIO (KB_ROW8)
    CardhuAPGpioRails_VDD_CAM_3V3,
    // AP GPIO (SDMMC3_DAT5)
    CardhuAPGpioRails_VDD_COM_BD_3V3,
    // AP GPIO (VI_HSYNC)
    CardhuAPGpioRails_VDD_SD_SLOT_3V3,
    // AP GPIO (SDMMC_DAT4)
    CardhuAPGpioRails_VDD_EMMC_CORE_3V3,
    // AP GPIO (VI_D09)
    CardhuAPGpioRails_VDD_HVDD_PEX_3V3,
    // AP GPIO (VI_D08)
    CardhuAPGpioRails_VDD_VPP_FUSE_3V3,
    // AP GPIO (GMI_RST)
    CardhuAPGpioRails_VDD_VBUS_MICRO_USB,
    // AP GPIO (GMI_AD15)
    CardhuAPGpioRails_VDD_VBUS_TYPEA_USB,
    // AP GPIO (VI_PCLK)
    CardhuAPGpioRails_VDD_HDMI_CON,
    // AP GPIO (GPIO_PBB4) For following 3
    CardhuAPGpioRails_VDD_1V8_CAM1,
    // AP GPIO (GPIO_PX2)
    CardhuAPGpioRails_DIS_5V,

    CardhuAPGpioRails_Num,
    CardhuAPGpioRails_Force32 = 0x7FFFFFFF,
} CardhuAPGpioRails;

static NvBool Enable(NvOdmServicesGpioHandle hOdmGpio, NvOdmGpioPinHandle hPinGpio,
                    NvU32 GpioPort, NvU32 GpioPin)
{
    NvOdmGpioConfig(hOdmGpio, hPinGpio, NvOdmGpioPinMode_InputData);
    NvOdmGpioConfig(hOdmGpio, hPinGpio, NvOdmGpioPinMode_Tristate);
    return NV_TRUE;
}
static NvBool Disable(NvOdmServicesGpioHandle hOdmGpio, NvOdmGpioPinHandle hPinGpio,
                    NvU32 GpioPort, NvU32 GpioPin)
{
     NvOdmGpioSetState(hOdmGpio, hPinGpio, NvOdmGpioPinActiveState_Low);
     NvOdmGpioConfig(hOdmGpio, hPinGpio, NvOdmGpioPinMode_Output);
     return NV_TRUE;
}



/* RailId, GpioPort,pioPin,
   IsActiveLow, IsInitEnable, OnValue, OffValue,
   Enable, Disable, IsOdmProt, Delay
*/
#define PORT(x) ((x) - 'a')
static ApGpioInfo s_CardhuApiGpioInfo[CardhuAPGpioRails_Num] =
{
    {   // Invalid
        CardhuAPGpioRails_Invalid, CardhuPowerRails_Invalid, 0, 0,
        NV_FALSE, NV_FALSE, 0, 0,
        NULL, NULL,
        NV_FALSE, 0,
    },

    {   // GMI_CS2 K.03
        CardhuAPGpioRails_VDD_BACKLIGHT, CardhuPowerRails_Invalid, PORT('k'), 3,
        NV_FALSE, NV_FALSE, 3300, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },

    {   // VI_VSYNC- D.06
        CardhuAPGpioRails_VDD_3V3_MINI_CARD, CardhuPowerRails_Invalid, PORT('d'), 6,
        NV_FALSE, NV_FALSE, 3300, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },
    {   // VI_D6-> L04
        CardhuAPGpioRails_VDD_LCD_PANEL_3V3, CardhuPowerRails_Invalid, PORT('l'), 4,
        NV_FALSE, NV_TRUE, 3300, 0,
        NULL, NULL,
        NV_TRUE, 10,
    },
    {   // KB_ROW8 S.0
        CardhuAPGpioRails_VDD_CAM_3V3, CardhuPowerRails_Invalid, PORT('s'), 0,
        NV_FALSE, NV_FALSE, 3300, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },
    {   // SDMMC3_DAT5 D.00
        CardhuAPGpioRails_VDD_COM_BD_3V3, CardhuPowerRails_Invalid, PORT('d'), 0,
        NV_FALSE, NV_FALSE, 3300, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },
    {   // VI_HSYNC- D.07
        CardhuAPGpioRails_VDD_SD_SLOT_3V3, CardhuPowerRails_Invalid, PORT('d'), 7,
        NV_FALSE, NV_TRUE, 3300, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },
    {   // SDMMC_DAT4 D01
        CardhuAPGpioRails_VDD_EMMC_CORE_3V3, CardhuPowerRails_Invalid, PORT('d'), 1,
        NV_FALSE, NV_TRUE, 3300, 0,
        NULL, NULL,
        NV_TRUE, 10,
    },
    {   // VI_D09 L7
        CardhuAPGpioRails_VDD_HVDD_PEX_3V3, CardhuPowerRails_Invalid, PORT('l'), 7,
        NV_FALSE, NV_FALSE, 3300, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },
    {   // VI_D08 L6
        CardhuAPGpioRails_VDD_VPP_FUSE_3V3, CardhuPowerRails_Invalid, PORT('l'), 6,
        NV_FALSE, NV_FALSE, 3300, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },
    {   // GMI_RTS I4
        CardhuAPGpioRails_VDD_VBUS_MICRO_USB, CardhuPowerRails_Invalid, PORT('i'), 4,
        NV_FALSE, NV_FALSE, 5000, 0,
        Enable, Disable,
        NV_FALSE, 10,
    },
    {   // GMI_AD15 H7
        CardhuAPGpioRails_VDD_VBUS_TYPEA_USB, CardhuPowerRails_Invalid, PORT('h'), 7,
        NV_FALSE, NV_FALSE, 5000, 0,
        Enable, Disable,
        NV_FALSE, 10,
    },
    {   // VI_PCLK T00
        CardhuAPGpioRails_VDD_HDMI_CON, CardhuPowerRails_Invalid, PORT('t'), 0,
        NV_FALSE, NV_FALSE, 5000, 0,
        Enable, Disable,
        NV_FALSE, 10,
    },

    {   // GPIO_PBB4
        CardhuAPGpioRails_VDD_1V8_CAM1, CardhuPowerRails_Invalid, 26 + PORT('b'), 4,
        NV_FALSE, NV_FALSE, 3300, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },
    {   // GPIO_PX2
        CardhuAPGpioRails_DIS_5V, CardhuPowerRails_Invalid, PORT('x'), 2,
        NV_TRUE, NV_FALSE, 5000, 0,
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
    pRailMap[CardhuPowerRails_EN_VDD_BL] = CardhuAPGpioRails_VDD_BACKLIGHT;
    pRailMap[CardhuPowerRails_VDD_BACKLIGHT] = CardhuAPGpioRails_VDD_BACKLIGHT;
    pRailMap[CardhuPowerRails_VDD_3V3_MINI_CARD] = CardhuAPGpioRails_VDD_3V3_MINI_CARD;
    pRailMap[CardhuPowerRails_EN_3V3_MODEM] = CardhuAPGpioRails_VDD_3V3_MINI_CARD;
    pRailMap[CardhuPowerRails_VDD_LCD_PANEL_3V3] = CardhuAPGpioRails_VDD_LCD_PANEL_3V3;
    pRailMap[CardhuPowerRails_VDD_CAM3_3V3] =  CardhuAPGpioRails_VDD_CAM_3V3;
    pRailMap[CardhuPowerRails_VDD_COM_BD_3V3] = CardhuAPGpioRails_VDD_COM_BD_3V3;
    pRailMap[CardhuPowerRails_VDDIO_SD_SLOT_3V3] = CardhuAPGpioRails_VDD_SD_SLOT_3V3;
    pRailMap[CardhuPowerRails_VDDIO_EMMC_CORE_3V3] = CardhuAPGpioRails_VDD_EMMC_CORE_3V3;
    pRailMap[CardhuPowerRails_VDD_HVDD_PEX_3V3] = CardhuAPGpioRails_VDD_HVDD_PEX_3V3;
    pRailMap[CardhuPowerRails_VDD_VPP_FUSE_3V3] = CardhuAPGpioRails_VDD_VPP_FUSE_3V3;
    pRailMap[CardhuPowerRails_VDD_1V8_CAM1] = CardhuAPGpioRails_VDD_1V8_CAM1;
    pRailMap[CardhuPowerRails_VDD_1V8_CAM2] = CardhuAPGpioRails_VDD_1V8_CAM1;
    pRailMap[CardhuPowerRails_VDD_1V8_CAM3] = CardhuAPGpioRails_VDD_1V8_CAM1;
    pRailMap[CardhuPowerRails_VDD_VBUS_MICRO_USB] = CardhuAPGpioRails_VDD_VBUS_MICRO_USB;
    pRailMap[CardhuPowerRails_VDD_VBUS_TYPEA_USB] = CardhuAPGpioRails_VDD_VBUS_TYPEA_USB;
    pRailMap[CardhuPowerRails_VDD_HDMI_CON] = CardhuAPGpioRails_VDD_HDMI_CON;
    pRailMap[CardhuPowerRails_DIS_5V] = CardhuAPGpioRails_DIS_5V;


    pRailMap[CardhuPowerRails_EN_USB1_VBUS_OC] = CardhuAPGpioRails_VDD_VBUS_MICRO_USB;
    pRailMap[CardhuPowerRails_EN_USB3_VBUS_OC] = CardhuAPGpioRails_VDD_VBUS_TYPEA_USB;

    s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_BACKLIGHT].SystemRailId = CardhuPowerRails_EN_VDD_BL;
    s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_3V3_MINI_CARD].SystemRailId = CardhuPowerRails_VDD_3V3_MINI_CARD;
    s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_LCD_PANEL_3V3].SystemRailId = CardhuPowerRails_VDD_LCD_PANEL_3V3;
    s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_CAM_3V3].SystemRailId = CardhuPowerRails_VDD_CAM3_3V3;
    s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_COM_BD_3V3].SystemRailId = CardhuPowerRails_VDD_COM_BD_3V3;
    s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_SD_SLOT_3V3].SystemRailId = CardhuPowerRails_VDDIO_SD_SLOT_3V3;
    s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_EMMC_CORE_3V3].SystemRailId = CardhuPowerRails_VDDIO_EMMC_CORE_3V3;
    s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_HVDD_PEX_3V3].SystemRailId = CardhuPowerRails_VDD_HVDD_PEX_3V3;
    s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_VPP_FUSE_3V3].SystemRailId = CardhuPowerRails_VDD_VPP_FUSE_3V3;
    s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_1V8_CAM1].SystemRailId = CardhuPowerRails_VDD_1V8_CAM1;
    s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_VBUS_MICRO_USB].SystemRailId = CardhuPowerRails_VDD_VBUS_MICRO_USB;
    s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_VBUS_TYPEA_USB].SystemRailId = CardhuPowerRails_VDD_VBUS_TYPEA_USB;
    s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_HDMI_CON].SystemRailId = CardhuPowerRails_VDD_HDMI_CON;
    s_CardhuApiGpioInfo[CardhuAPGpioRails_DIS_5V].SystemRailId = CardhuPowerRails_DIS_5V;
}

void *CardhuPmu_GetApGpioRailMap(NvOdmPmuDeviceHandle hPmuDevice,
    PmuChipDriverOpsHandle *hDriverOps, NvU32 *pRailMap, NvOdmBoardInfo *pProcBoard,
    NvOdmBoardInfo *pPmuBoard)
{

    NvOdmPmuApGpioDriverHandle hApGpioDriver = NULL;

    if (((pProcBoard->BoardID == PROC_BOARD_E1198) && (pProcBoard->Fab >= BOARD_FAB_A02)) ||
        ((pProcBoard->BoardID == PROC_BOARD_E1291) && (pProcBoard->Fab >= BOARD_FAB_A03))) {
            s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_BACKLIGHT].GpioPort = 26 + PORT('d');
            s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_BACKLIGHT].GpioPin = 2;
            s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_BACKLIGHT].IsInitEnable = NV_TRUE;
    }

    if(((pProcBoard->BoardID == PROC_BOARD_E1198) || (pProcBoard->BoardID == PROC_BOARD_E1291))
        && (pProcBoard->Fab >= BOARD_FAB_A04))
    {
        s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_VBUS_MICRO_USB].GpioPort = 26 + PORT('d');
        s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_VBUS_MICRO_USB].GpioPin = 6;
        s_CardhuApiGpioInfo[CardhuAPGpioRails_VDD_VBUS_MICRO_USB].IsInitEnable = NV_FALSE;
    }

    hApGpioDriver = OdmPmuApGpioOpen(hPmuDevice, s_CardhuApiGpioInfo,
                                    CardhuAPGpioRails_Num);
    if (!hApGpioDriver)
    {
        NvOdmOsPrintf("%s(): Fails in opening GpioPmu Handle\n", __func__);
        return NULL;
    }
    InitGpioRailMap(pRailMap);
    *hDriverOps = &s_PmuApGpioDriverOps;


    return hApGpioDriver;
}
