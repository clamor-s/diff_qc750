/*
 * Copyright (c) 2009-2012 NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvodm_pmu.h"
#include "nvodm_query_discovery.h"
#include "pmu_hal.h"
#include "nvodm_services.h"
#include "tps6586x/nvodm_pmu_tps6586x.h"
#include "max8907b/max8907b.h"
#include "max8907b/max8907b_rtc.h"
#include "boards/cardhu/nvodm_pmu_cardhu.h"
#include "boards/kai/nvodm_pmu_kai.h"
#include "boards/enterprise/nvodm_pmu_enterprise.h"
#include "boards/p1852/nvodm_pmu_p1852.h"

static NvOdmPmuDevice*
GetPmuInstance(NvOdmPmuDeviceHandle hDevice)
{
    static NvOdmPmuDevice Pmu;
    static NvBool         first = NV_TRUE;

    if (first)
    {
        NvOdmOsMemset(&Pmu, 0, sizeof(Pmu));
        first = NV_FALSE;
#ifdef NV_TARGET_VENTANA
        //  fill in HAL functions here.
        Pmu.Hal = NV_TRUE;
        Pmu.pfnSetup = Tps6586xSetup;
        Pmu.pfnRelease = Tps6586xRelease;
        Pmu.pfnGetCaps = Tps6586xGetCapabilities;
        Pmu.pfnGetVoltage = Tps6586xGetVoltage;
        Pmu.pfnSetVoltage = Tps6586xSetVoltage;
        Pmu.pfnGetAcLineStatus = Tps6586xGetAcLineStatus;
        Pmu.pfnGetBatteryStatus = Tps6586xGetBatteryStatus;
        Pmu.pfnGetBatteryData = Tps6586xGetBatteryData;
        Pmu.pfnGetBatteryFullLifeTime = Tps6586xGetBatteryFullLifeTime;
        Pmu.pfnGetBatteryChemistry = Tps6586xGetBatteryChemistry;
        Pmu.pfnSetChargingCurrent = Tps6586xSetChargingCurrent;
        Pmu.pfnInterruptHandler = Tps6586xInterruptHandler;
        Pmu.pfnReadRtc = Tps6586xReadRtc;
        Pmu.pfnWriteRtc = Tps6586xWriteRtc;
        Pmu.pfnIsRtcInitialized = Tps6586xIsRtcInitialized;
        Pmu.pfnPowerOff = Tps6586xPowerOff;
#elif NV_TARGET_CARDHU
        //  fill in HAL functions here.
        Pmu.Hal = NV_TRUE;
        Pmu.pfnSetup = CardhuPmuSetup;
        Pmu.pfnRelease = CardhuPmuRelease;
        Pmu.pfnGetCaps = CardhuPmuGetCapabilities;
        Pmu.pfnGetVoltage = CardhuPmuGetVoltage;
        Pmu.pfnSetVoltage = CardhuPmuSetVoltage;
        Pmu.pfnGetAcLineStatus = CardhuPmuGetAcLineStatus;
        Pmu.pfnGetBatteryStatus = CardhuPmuGetBatteryStatus;
        Pmu.pfnGetBatteryData = CardhuPmuGetBatteryData;
        Pmu.pfnGetBatteryFullLifeTime = CardhuPmuGetBatteryFullLifeTime;
        Pmu.pfnGetBatteryChemistry = CardhuPmuGetBatteryChemistry;
        Pmu.pfnSetChargingCurrent = CardhuPmuSetChargingCurrent;
        Pmu.pfnInterruptHandler = CardhuPmuInterruptHandler;
        Pmu.pfnReadRtc = CardhuPmuReadRtc;
        Pmu.pfnWriteRtc = CardhuPmuWriteRtc;
        Pmu.pfnIsRtcInitialized = CardhuPmuIsRtcInitialized;
        Pmu.pfnPowerOff = CardhuPmuPowerOff;
#elif NV_TARGET_KAI
        //  fill in HAL functions here.
        Pmu.Hal = NV_TRUE;
        Pmu.pfnSetup = KaiPmuSetup;
        Pmu.pfnRelease = KaiPmuRelease;
        Pmu.pfnGetCaps = KaiPmuGetCapabilities;
        Pmu.pfnGetVoltage = KaiPmuGetVoltage;
        Pmu.pfnSetVoltage = KaiPmuSetVoltage;
        Pmu.pfnGetAcLineStatus = KaiPmuGetAcLineStatus;
        Pmu.pfnGetBatteryStatus = KaiPmuGetBatteryStatus;
        Pmu.pfnGetBatteryData = KaiPmuGetBatteryData;
        Pmu.pfnGetBatteryFullLifeTime = KaiPmuGetBatteryFullLifeTime;
        Pmu.pfnGetBatteryChemistry = KaiPmuGetBatteryChemistry;
        Pmu.pfnSetChargingCurrent = KaiPmuSetChargingCurrent;
        Pmu.pfnInterruptHandler = KaiPmuInterruptHandler;
        Pmu.pfnReadRtc = KaiPmuReadRtc;
        Pmu.pfnWriteRtc = KaiPmuWriteRtc;
        Pmu.pfnIsRtcInitialized = KaiPmuIsRtcInitialized;
        Pmu.pfnPowerOff = KaiPmuPowerOff;
#elif NV_TARGET_WHISTLER
        Pmu.pfnSetup                  = Max8907bSetup;
        Pmu.pfnRelease                = Max8907bRelease;
        Pmu.pfnGetCaps                = Max8907bGetCapabilities;
        Pmu.pfnGetVoltage             = Max8907bGetVoltage;
        Pmu.pfnSetVoltage             = Max8907bSetVoltage;
        Pmu.pfnGetAcLineStatus        = Max8907bGetAcLineStatus;
        Pmu.pfnGetBatteryStatus       = Max8907bGetBatteryStatus;
        Pmu.pfnGetBatteryData         = Max8907bGetBatteryData;
        Pmu.pfnGetBatteryFullLifeTime = Max8907bGetBatteryFullLifeTime;
        Pmu.pfnGetBatteryChemistry    = Max8907bGetBatteryChemistry;
        Pmu.pfnSetChargingCurrent     = Max8907bSetChargingCurrent;
        Pmu.pfnInterruptHandler       = Max8907bInterruptHandler;
        Pmu.pfnReadRtc                = Max8907bRtcCountRead;
        Pmu.pfnWriteRtc               = Max8907bRtcCountWrite;
        Pmu.pfnIsRtcInitialized       = Max8907bIsRtcInitialized;
        Pmu.pPrivate                  = NULL;
        Pmu.Hal                       = NV_TRUE;
        Pmu.Init                      = NV_FALSE;
        Pmu.pfnPowerOff               = Max8907bPowerOff;
#elif NV_TARGET_ENTERPRISE
        Pmu.Hal = NV_TRUE;
        Pmu.pfnSetup = EnterprisePmuSetup;
        Pmu.pfnRelease = EnterprisePmuRelease;
        Pmu.pfnGetCaps = EnterprisePmuGetCapabilities;
        Pmu.pfnGetVoltage = EnterprisePmuGetVoltage;
        Pmu.pfnSetVoltage = EnterprisePmuSetVoltage;
        Pmu.pfnGetAcLineStatus = EnterprisePmuGetAcLineStatus;
        Pmu.pfnGetBatteryStatus = EnterprisePmuGetBatteryStatus;
        Pmu.pfnGetBatteryData = EnterprisePmuGetBatteryData;
        Pmu.pfnGetBatteryFullLifeTime = EnterprisePmuGetBatteryFullLifeTime;
        Pmu.pfnGetBatteryChemistry = EnterprisePmuGetBatteryChemistry;
        Pmu.pfnSetChargingCurrent = EnterprisePmuSetChargingCurrent;
        Pmu.pfnInterruptHandler = EnterprisePmuInterruptHandler;
        Pmu.pfnReadRtc = EnterprisePmuReadRtc;
        Pmu.pfnWriteRtc = EnterprisePmuWriteRtc;
        Pmu.pfnIsRtcInitialized = EnterprisePmuIsRtcInitialized;
        Pmu.pfnPowerOff = EnterprisePmuPowerOff;
#elif defined(NV_TARGET_P1852)
        Pmu.Hal = NV_TRUE;
        Pmu.pfnSetup = P1852PmuSetup;
        Pmu.pfnRelease = P1852PmuRelease;
        Pmu.pfnGetCaps = P1852PmuGetCapabilities;
        Pmu.pfnGetVoltage = P1852PmuGetVoltage;
        Pmu.pfnSetVoltage = P1852PmuSetVoltage;
        Pmu.pfnGetAcLineStatus = NULL;
        Pmu.pfnGetBatteryStatus = NULL;
        Pmu.pfnGetBatteryData = NULL;
        Pmu.pfnGetBatteryFullLifeTime = NULL;
        Pmu.pfnGetBatteryChemistry = NULL;
        Pmu.pfnSetChargingCurrent = NULL;
        Pmu.pfnInterruptHandler = NULL;
        Pmu.pfnReadRtc = NULL;
        Pmu.pfnWriteRtc = NULL;
        Pmu.pfnIsRtcInitialized = NULL;
#endif
    }

    if (hDevice && Pmu.Hal)
        return &Pmu;

    return NULL;
}

NvBool
NvOdmPmuDeviceOpen(NvOdmPmuDeviceHandle *hDevice)
{
    NvOdmPmuDevice *pmu = GetPmuInstance((NvOdmPmuDeviceHandle)1);

    *hDevice = (NvOdmPmuDeviceHandle)0;

    if (!pmu || !pmu->pfnSetup || pmu->Init)
    {
        *hDevice = (NvOdmPmuDeviceHandle)1;
        return NV_TRUE;
    }

    if (pmu->pfnSetup(pmu))
    {
        *hDevice = (NvOdmPmuDeviceHandle)1;
        pmu->Init = NV_TRUE;
        return NV_TRUE;
    }

    return NV_FALSE;
}

void NvOdmPmuDeviceClose(NvOdmPmuDeviceHandle hDevice)
{
    NvOdmPmuDevice *pmu = GetPmuInstance(hDevice);    

    if (!pmu)
        return;

    if (pmu->pfnRelease)
        pmu->pfnRelease(pmu);

    pmu->Init = NV_FALSE;
}

void
NvOdmPmuGetCapabilities(NvU32 vddId,
                        NvOdmPmuVddRailCapabilities* pCapabilities)
{
    //  use a manual handle, since this function doesn't takea  handle
    NvOdmPmuDevice* pmu = GetPmuInstance((NvOdmPmuDeviceHandle)1);

    if (pmu && pmu->pfnGetCaps)
        pmu->pfnGetCaps(vddId, pCapabilities);
    else if (pCapabilities)
    {
        NvOdmOsMemset(pCapabilities, 0, sizeof(NvOdmPmuVddRailCapabilities));
        pCapabilities->OdmProtected = NV_TRUE;
    }
}


NvBool
NvOdmPmuGetVoltage(NvOdmPmuDeviceHandle hDevice,
                   NvU32 vddId,
                   NvU32* pMilliVolts)
{
    NvOdmPmuDevice* pmu = GetPmuInstance(hDevice);

    if (pmu && pmu->pfnGetVoltage)
        return pmu->pfnGetVoltage(pmu, vddId, pMilliVolts);

    return NV_TRUE;
}

NvBool
NvOdmPmuSetVoltage(NvOdmPmuDeviceHandle hDevice,
                   NvU32 VddId,
                   NvU32 MilliVolts,
                   NvU32* pSettleMicroSeconds)
{
    NvOdmPmuDevice *pmu = GetPmuInstance(hDevice);
    NvU32 SettlingTime = 0;
    NvBool ErrorVal = NV_TRUE;


    if (pmu && pmu->pfnSetVoltage)
    {
        ErrorVal = pmu->pfnSetVoltage(pmu, VddId, MilliVolts, &SettlingTime);
        if ((!pSettleMicroSeconds) && SettlingTime)
            NvOdmOsWaitUS(SettlingTime);
    }

    if (pSettleMicroSeconds)
        *pSettleMicroSeconds = SettlingTime;

    return ErrorVal;
}


NvBool 
NvOdmPmuGetAcLineStatus(NvOdmPmuDeviceHandle hDevice, 
                        NvOdmPmuAcLineStatus *pStatus)
{
    NvOdmPmuDevice* pmu = GetPmuInstance(hDevice);

    if (pmu && pmu->pfnGetAcLineStatus)
        return pmu->pfnGetAcLineStatus(pmu, pStatus);

    return NV_TRUE;
}


NvBool 
NvOdmPmuGetBatteryStatus(NvOdmPmuDeviceHandle hDevice, 
                         NvOdmPmuBatteryInstance BatteryInst,
                         NvU8 *pStatus)
{
    NvOdmPmuDevice *pmu = GetPmuInstance(hDevice);

    if (pmu && pmu->pfnGetBatteryStatus)
        return pmu->pfnGetBatteryStatus(pmu, BatteryInst, pStatus);

    return NV_TRUE;
}

NvBool
NvOdmPmuGetBatteryData(NvOdmPmuDeviceHandle hDevice, 
                       NvOdmPmuBatteryInstance BatteryInst,
                       NvOdmPmuBatteryData *pData)
{
    NvOdmPmuDevice *pmu = GetPmuInstance(hDevice);

    if (pmu && pmu->pfnGetBatteryData)
        return pmu->pfnGetBatteryData(pmu, BatteryInst, pData);

    pData->batteryLifePercent = NVODM_BATTERY_DATA_UNKNOWN;
    pData->batteryLifeTime = NVODM_BATTERY_DATA_UNKNOWN;
    pData->batteryVoltage = NVODM_BATTERY_DATA_UNKNOWN;
    pData->batteryCurrent = NVODM_BATTERY_DATA_UNKNOWN;
    pData->batteryAverageCurrent = NVODM_BATTERY_DATA_UNKNOWN;
    pData->batteryAverageInterval = NVODM_BATTERY_DATA_UNKNOWN;
    pData->batteryMahConsumed = NVODM_BATTERY_DATA_UNKNOWN;
    pData->batteryTemperature = NVODM_BATTERY_DATA_UNKNOWN;
    pData->batterySoc = NVODM_BATTERY_DATA_UNKNOWN;

    return NV_TRUE;
}


void
NvOdmPmuGetBatteryFullLifeTime(NvOdmPmuDeviceHandle hDevice, 
                               NvOdmPmuBatteryInstance BatteryInst,
                               NvU32 *pLifeTime)
{
    NvOdmPmuDevice *pmu = GetPmuInstance(hDevice);

    if (pmu && pmu->pfnGetBatteryFullLifeTime)
        pmu->pfnGetBatteryFullLifeTime(pmu, BatteryInst, pLifeTime);

    else
    {
        if (pLifeTime)
            *pLifeTime = NVODM_BATTERY_DATA_UNKNOWN;
    }
}


void
NvOdmPmuGetBatteryChemistry(NvOdmPmuDeviceHandle hDevice, 
                            NvOdmPmuBatteryInstance BatteryInst,
                            NvOdmPmuBatteryChemistry *pChemistry)
{
    NvOdmPmuDevice *pmu = GetPmuInstance(hDevice);

    if (pmu && pmu->pfnGetBatteryChemistry)
        pmu->pfnGetBatteryChemistry(pmu, BatteryInst, pChemistry);
    else
    {
        if (pChemistry)
            *pChemistry = NVODM_BATTERY_DATA_UNKNOWN;
    }
}

NvBool 
NvOdmPmuSetChargingCurrent(NvOdmPmuDeviceHandle hDevice, 
                           NvOdmPmuChargingPath ChargingPath, 
                           NvU32 ChargingCurrentLimitMa,
                           NvOdmUsbChargerType ChargerType)
{
    NvOdmPmuDevice *pmu = GetPmuInstance(hDevice);

    if (pmu && pmu->pfnSetChargingCurrent)
        return pmu->pfnSetChargingCurrent(pmu, ChargingPath, ChargingCurrentLimitMa, ChargerType);

    return NV_TRUE;
}


void NvOdmPmuInterruptHandler(NvOdmPmuDeviceHandle hDevice)
{
    NvOdmPmuDevice *pmu = GetPmuInstance(hDevice);

    if (pmu && pmu->pfnInterruptHandler)
        pmu->pfnInterruptHandler(pmu);
}

NvBool NvOdmPmuReadRtc(
    NvOdmPmuDeviceHandle  hDevice,
    NvU32 *Count)
{
    NvOdmPmuDevice *pmu = GetPmuInstance(hDevice);

    if (pmu && pmu->pfnReadRtc)
        return pmu->pfnReadRtc(pmu, Count);
    return NV_FALSE;
}


NvBool NvOdmPmuWriteRtc(
    NvOdmPmuDeviceHandle  hDevice,
    NvU32 Count)
{
    NvOdmPmuDevice *pmu = GetPmuInstance(hDevice);

    if (pmu && pmu->pfnWriteRtc)
        return pmu->pfnWriteRtc(pmu, Count);
    return NV_FALSE;
}

NvBool
NvOdmPmuIsRtcInitialized(NvOdmPmuDeviceHandle hDevice)
{
    NvOdmPmuDevice *pmu = GetPmuInstance(hDevice);

    if (pmu && pmu->pfnIsRtcInitialized)
        return pmu->pfnIsRtcInitialized(pmu);
        
    return NV_FALSE;
}

NvBool
NvOdmPmuPowerOff(NvOdmPmuDeviceHandle hDevice)
{
    NvOdmPmuDevice *pmu = GetPmuInstance(hDevice);

    if (pmu && pmu->pfnPowerOff)
        return pmu->pfnPowerOff(pmu);

    return NV_FALSE;
}
