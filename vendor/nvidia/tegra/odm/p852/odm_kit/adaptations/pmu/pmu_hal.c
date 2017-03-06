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
#include "nvodm_pmu_tps6586x.h"

static NvOdmPmuDevice*
GetPmuInstance(NvOdmPmuDeviceHandle hDevice)
{
    static NvOdmPmuDevice Pmu;
    static NvBool         first = NV_TRUE;

    if (first)
    {
        NvOdmOsMemset(&Pmu, 0, sizeof(Pmu));
        first = NV_FALSE;

        if (NvOdmPeripheralGetGuid(NV_ODM_GUID('t','p','s','6','5','8','6','x')))
        {
            //  fill in HAL functions here.
            Pmu.VddRefCntList = &Pmu.InitialRefCnts;
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
        }

        NvOdmOsMemset(&Pmu.InitialRefCnts, 0, sizeof(Pmu.InitialRefCnts));
    }

    if (hDevice && Pmu.Hal)
        return &Pmu;

    return NULL;
}


//  Returns a pointer to the reference count for the specified rail,
//  adding it to the list if necessary.
static NvU32*
GetPmuRailRefCnt(NvOdmPmuDevice* pPmu,
                 NvU32 VddId)
{
    unsigned int i;
    NvU32 CurrVoltage;
    PmuRailRefCnt *CurrRec = pPmu->VddRefCntList;
    NvU32 CurrCnt = (pPmu->VddCnt & (NV_ARRAY_SIZE(pPmu->InitialRefCnts.VddId)-1));
    NvU32 *RefCnt = NULL;

    if (!CurrCnt)
    {
        CurrCnt = NV_ARRAY_SIZE(pPmu->InitialRefCnts.VddId);
        CurrRec = CurrRec->Next;
    }

    while (CurrRec)
    {
        for (i=0; i<CurrCnt; i++)
        {
            if (CurrRec->VddId[i]==VddId)
                return &CurrRec->RefCnt[i];
        }
        CurrRec = CurrRec->Next;
        CurrCnt = NV_ARRAY_SIZE(pPmu->InitialRefCnts.VddId);
    }

    //  Allocate a new head pointer if out of space.
    if (!pPmu->VddCnt || (pPmu->VddCnt&0xf))
    {
        pPmu->VddRefCntList->VddId[(pPmu->VddCnt & 0xf)] = VddId;
        RefCnt = &pPmu->VddRefCntList->RefCnt[(pPmu->VddCnt & 0xf)];
    }
    else
    {
        CurrRec = NvOdmOsAlloc(sizeof(PmuRailRefCnt));
        if (!CurrRec)
            return NULL;
        NvOdmOsMemset(CurrRec, 0, sizeof(PmuRailRefCnt));

        CurrRec->Next = pPmu->VddRefCntList;
        pPmu->VddRefCntList = CurrRec;
        CurrRec->VddId[0] = VddId;
        RefCnt = &CurrRec->RefCnt[0];
    }

    *RefCnt = 0;
    pPmu->VddCnt++;

    if (pPmu->pfnGetVoltage)
    {
        if (pPmu->pfnGetVoltage(pPmu, VddId, &CurrVoltage))
            *RefCnt = ((CurrVoltage==0) ? 0 : 1);
        else
            return NULL;

    }

    return RefCnt;
}

NvBool
NvOdmPmuDeviceOpen(NvOdmPmuDeviceHandle *hDevice)
{
    NvOdmPmuDevice *pmu = GetPmuInstance((NvOdmPmuDeviceHandle)1);

    *hDevice = (NvOdmPmuDeviceHandle)0;

    if (!pmu || !pmu->pfnSetup)
        return NV_FALSE;

    if (pmu->Init)
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
    PmuRailRefCnt  *CurrRec;
    NvU32 CurrCnt;

    if (!pmu)
        return;

    if (pmu->pfnRelease)
        pmu->pfnRelease(pmu);

    CurrCnt = (pmu->VddCnt & (NV_ARRAY_SIZE(pmu->InitialRefCnts.VddId)-1));
    CurrRec = pmu->VddRefCntList;
    if (!CurrCnt)
    {
        CurrCnt = NV_ARRAY_SIZE(pmu->InitialRefCnts.VddId);
        CurrRec = CurrRec->Next;
    }

    while (CurrRec)
    {
        unsigned int i;
        for (i=0; i<CurrCnt; i++)
        {
            if (CurrRec->RefCnt[i])
            {
                //  TODO:  should loose reference counts be garbage collected?
            }
        }
        if (CurrRec != &pmu->InitialRefCnts)
        {
            PmuRailRefCnt *NextRec = CurrRec->Next;
            NvOdmOsFree(CurrRec);
            CurrRec = NextRec;
        }
        CurrCnt = NV_ARRAY_SIZE(pmu->InitialRefCnts.VddId);
    }

    pmu->VddRefCntList = &pmu->InitialRefCnts;
    pmu->VddRefCntList->Next = NULL;
    pmu->VddCnt = 0;
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
    NvU32 *RailVal = NULL;
    NvU32 Update = 0;

    if (!pmu || !pmu->pfnSetVoltage)
    {
        if(NULL != pSettleMicroSeconds)
        *pSettleMicroSeconds = 0;
        return NV_TRUE;
    }

    RailVal = GetPmuRailRefCnt(pmu, VddId);

    if (RailVal)
    {
        Update = ((*RailVal && MilliVolts) || ((*RailVal==1) && !MilliVolts));
        Update = (Update  || ((*RailVal == 0) && MilliVolts));

        if (MilliVolts)
            *RailVal = *RailVal + 1;
        else if (*RailVal)
            *RailVal = *RailVal - 1;
    }
    else
        Update = 1;  // Always update if we can't find the reference count

    if (Update)
        return pmu->pfnSetVoltage(pmu, VddId, MilliVolts, pSettleMicroSeconds);

    if(NULL != pSettleMicroSeconds)
        *pSettleMicroSeconds = 0;

    return NV_TRUE;
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
    pData->batterySoc         = NVODM_BATTERY_DATA_UNKNOWN;

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
        *pLifeTime = NVODM_BATTERY_DATA_UNKNOWN;
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
        *pChemistry = NVODM_BATTERY_DATA_UNKNOWN;
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

NvBool NvOdmPmuPowerOff( NvOdmPmuDeviceHandle hDevice)
{
    NvOdmPmuDevice *pmu = GetPmuInstance(hDevice);

    if (pmu && pmu->pfnPowerOff)
        pmu->pfnPowerOff(pmu);
    return NV_FALSE;
}
