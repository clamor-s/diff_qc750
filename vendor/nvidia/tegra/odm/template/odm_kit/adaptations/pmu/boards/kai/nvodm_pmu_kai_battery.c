/*
 * Copyright (c) 2012 NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
/**
 * @file
 * <b>NVIDIA Tegra ODM Kit: Implementation of public API of
 *                          Kai pmu driver interface.</b>
 *
 * @b Description: Implementation of the battery driver api of
                   public interface for the pmu driver for kai.
 *
 */

#include "nvodm_pmu.h"
#include "nvodm_pmu_kai.h"
#include "kai_pmu.h"
#include "nvassert.h"
#include "max17048/nvodm_pmu_max17048_fuel_gauge_driver.h"
#include "smb349/nvodm_pmu_smb349_battery_charger_driver.h"
#include "max77663/nvodm_pmu_max77663_pmu_driver.h"

typedef struct KaiBatDriverInfoRec
{
    NvOdmBatChargingDriverInfoHandle hBChargingDriver;
    NvOdmFuelGaugeDriverInfoHandle hFuelGaugeDriver;
} KaiBatDriverInfo;

static KaiBatDriverInfo s_KaiBatInfo = {NULL, NULL};

NvBool
KaiPmuGetAcLineStatus(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuAcLineStatus *pStatus)
{
    NV_ASSERT(hDevice);
    NV_ASSERT(pStatus);

    if(!hDevice || !pStatus)
        return NV_FALSE;
    // NOTE: Not implemented completely as it is only for bootloader.

    *pStatus = NvOdmPmuAcLine_Online;
    return NV_TRUE;
}

NvBool
KaiPmuGetBatteryStatus(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvU8 *pStatus)
{
    NvU32 Soc = 0;
    NvBool ret;

    NV_ASSERT(hDevice);
    NV_ASSERT(pStatus);
    NV_ASSERT(batteryInst < NvOdmPmuBatteryInst_Num);

    if(!hDevice || !pStatus)
        return NV_FALSE;
    if(batteryInst >= NvOdmPmuBatteryInst_Num)
        return NV_FALSE;

    if (batteryInst == NvOdmPmuBatteryInst_Main)
    {
        ret = Max17048FuelGaugeDriverReadSoc(&Soc);
        if (!ret)
        {
            NvOdmOsPrintf("Fuel guage Soc read failed \n");
            return NV_FALSE;
        }

        if (Soc > MAX17048_BATTERY_LOW)  // modify these parameters on need
            *pStatus |= NVODM_BATTERY_STATUS_HIGH;
        else if ((Soc <= MAX17048_BATTERY_LOW)&&
                 (Soc > MAX17048_BATTERY_CRITICAL))
            *pStatus |= NVODM_BATTERY_STATUS_LOW;
        else
            *pStatus |= NVODM_BATTERY_STATUS_CRITICAL;
    }
    else
    {
        *pStatus = NVODM_BATTERY_STATUS_UNKNOWN;
    }
    return NV_TRUE;
}

NvBool
KaiPmuGetBatteryData(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvOdmPmuBatteryData *pData)
{
    NvOdmPmuBatteryData batteryData;
    NvU32 Soc = 0;
    NvBool ret;

    NV_ASSERT(hDevice);
    NV_ASSERT(pData);
    NV_ASSERT(batteryInst < NvOdmPmuBatteryInst_Num);

    if(!hDevice || !pData)
        return NV_FALSE;
    if(batteryInst >= NvOdmPmuBatteryInst_Num)
        return NV_FALSE;

    batteryData.batteryLifePercent     = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryLifeTime        = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryVoltage         = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryCurrent         = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryAverageCurrent  = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryAverageInterval = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryMahConsumed     = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryTemperature     = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batterySoc             = NVODM_BATTERY_DATA_UNKNOWN;

    ret = Max17048FuelGaugeDriverReadSoc(&Soc);
    if (!ret)
    {
        NvOdmOsPrintf("Fuel guage Soc read failed \n");
        return NV_FALSE;
    }
    batteryData.batterySoc = Soc;

    *pData = batteryData;
    return NV_TRUE;
}

void
KaiPmuGetBatteryFullLifeTime(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvU32 *pLifeTime)
{
    NV_ASSERT(hDevice);
    NV_ASSERT(batteryInst < NvOdmPmuBatteryInst_Num);

    *pLifeTime = NVODM_BATTERY_DATA_UNKNOWN;
}

void
KaiPmuGetBatteryChemistry(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvOdmPmuBatteryChemistry *pChemistry)
{
    NV_ASSERT(hDevice);
    NV_ASSERT(batteryInst < NvOdmPmuBatteryInst_Num);

    *pChemistry = NvOdmPmuBatteryChemistry_LION;
}


NvBool
KaiPmuSetChargingCurrent(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuChargingPath chargingPath,
    NvU32 chargingCurrentLimitMa,
    NvOdmUsbChargerType ChargerType)
{
    NvBool ret;

    ret = Smb349BatChargingDriverSetCurrent(s_KaiBatInfo.hBChargingDriver, chargingPath,
                                            chargingCurrentLimitMa, ChargerType);
    return ret;
}

void
*KaiPmu_BatterySetup(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmBoardInfo *pProcBoard,
    NvOdmBoardInfo *pPmuBoard)
{
    NV_ASSERT(hDevice);

    if(!hDevice)
         return NULL;

    if (s_KaiBatInfo.hFuelGaugeDriver && s_KaiBatInfo.hBChargingDriver)
         return &s_KaiBatInfo;

    s_KaiBatInfo.hFuelGaugeDriver = Max17048FuelGaugeDriverOpen(hDevice);
    if (!s_KaiBatInfo.hFuelGaugeDriver)
    {
          NvOdmOsPrintf("%s(): Error in call Max17048FuelGaugeDriverOpen()\n", __func__);
          return NULL;
    }

    s_KaiBatInfo.hBChargingDriver = Smb349BatChargingDriverOpen(hDevice, NULL);
    if (!s_KaiBatInfo.hBChargingDriver)
    {
          NvOdmOsPrintf("%s(): Error in call Smb349BatChargingDriverOpen()\n", __func__);
          Max17048FuelGaugeDriverClose();
          s_KaiBatInfo.hFuelGaugeDriver = NULL;
          return NULL;
    }

   return &s_KaiBatInfo;
}
