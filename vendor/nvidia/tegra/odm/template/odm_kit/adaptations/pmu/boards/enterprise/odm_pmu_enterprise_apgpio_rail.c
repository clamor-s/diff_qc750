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
  * <b>NVIDIA Tegra ODM Kit: Enterprise pmu rail driver using apgpio pmu driver</b>
  *
  * @b Description: Implements the pmu controls api based on AP GPIO pmu driver.
  */

#include "nvodm_pmu.h"
#include "nvodm_pmu_enterprise.h"
#include "enterprise_pmu.h"
#include "nvodm_pmu_enterprise_supply_info_table.h"
#include "apgpio/nvodm_pmu_apgpio.h"
#include "nvodm_query_gpio.h"

typedef enum
{
    EnterpriseAPGpioRails_Invalid = 0,

    // AP GPIO (LCD_D16)
    EnterpriseAPGpioRails_VDD_FUSE,
    // AP GPIO (LCD_D17)
    EnterpriseAPGpioRails_VDD_SDMMC,
    // AP GPIO (LCD_PWR0)
    EnterpriseAPGpioRails_VDD_LCD,

    EnterpriseAPGpioRails_Num,
    EnterpriseAPGpioRails_Force32 = 0x7FFFFFFF,
} EnterpriseAPGpioRails;

/* RailId, GpioPort,pioPin,
   IsActiveLow, IsInitEnable, OnValue, OffValue,
   Enable, Disable, IsOdmProt, Delay
*/
#define PORT(x) ((x) - 'a')
static ApGpioInfo s_EnterpriseApiGpioInfo[EnterpriseAPGpioRails_Num] =
{
    {   // Invalid
        EnterpriseAPGpioRails_Invalid, EnterprisePowerRails_Invalid, 0, 0,
        NV_FALSE, NV_FALSE, 0, 0,
        NULL, NULL,
        NV_FALSE, 0,
    },

    {   // LCD_D16-> GPIO M.0
        EnterpriseAPGpioRails_VDD_FUSE, EnterprisePowerRails_VDD_FUSE, PORT('m'), 0,
        NV_FALSE, NV_FALSE, 3300, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },

    {   // LCD_D17-> GPIO M.1
        EnterpriseAPGpioRails_VDD_SDMMC, EnterprisePowerRails_VDD_SDMMC, PORT('m'), 1,
        NV_FALSE, NV_FALSE, 3300, 0,
        NULL, NULL,
        NV_FALSE, 10,
    },
    {   // LCD_PWR0-> GPIO B.2
        EnterpriseAPGpioRails_VDD_LCD, EnterprisePowerRails_VDD_LCD, PORT('b'), 2,
        NV_FALSE, NV_TRUE, 1800, 0,
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
    pRailMap[EnterprisePowerRails_VDD_FUSE] = EnterpriseAPGpioRails_VDD_FUSE;
    pRailMap[EnterprisePowerRails_VDD_SDMMC] = EnterpriseAPGpioRails_VDD_SDMMC;
    pRailMap[EnterprisePowerRails_VDDIO_LCD] = EnterpriseAPGpioRails_VDD_LCD;
}

void *EnterprisePmu_GetApGpioRailMap(NvOdmPmuDeviceHandle hPmuDevice,
    PmuChipDriverOpsHandle *hDriverOps, NvU32 *pRailMap, NvOdmBoardInfo *pProcBoard,
    NvOdmBoardInfo *pPmuBoard)
{

    NvOdmPmuApGpioDriverHandle hApGpioDriver = NULL;

    hApGpioDriver = OdmPmuApGpioOpen(hPmuDevice, s_EnterpriseApiGpioInfo,
                                    EnterpriseAPGpioRails_Num);
    if (!hApGpioDriver)
    {
        NvOdmOsPrintf("%s(): Fails in opening GpioPmu Handle\n", __func__);
        return NULL;
    }
    InitGpioRailMap(pRailMap);
    *hDriverOps = &s_PmuApGpioDriverOps;
    return hApGpioDriver;
}
