/*
 * Copyright (c) 2011-2012 NVIDIA CORPORATION.  All rights reserved.
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
 *                          Enterprise pmu driver interface.</b>
 *
 * @b Description: Implementation of the battery driver api of
                   public interface for the pmu driver for enterprise.
 *
 */

#include "nvodm_pmu.h"
#include "nvodm_pmu_enterprise.h"
#include "enterprise_pmu.h"
#include "nvassert.h"
#include "tps8003x/nvodm_pmu_tps8003x_adc_driver.h"
#include "tps8003x/nvodm_pmu_tps8003x_battery_charging_driver.h"
#include "tps8003x/nvodm_pmu_tps8003x_pmu_driver.h"

#define AdcVoltageChannel 7
// threshold for battery status. need to fine tune based on battery/system characterisation
#define NVODM_BATTERY_FULL_VOLTAGE_MV      4200
#define NVODM_BATTERY_HIGH_VOLTAGE_MV      3900
#define NVODM_BATTERY_BOOTLOADER_VOLTAGE_MV 3700
#define NVODM_BATTERY_LOW_VOLTAGE_MV       3300
#define NVODM_BATTERY_CRITICAL_VOLTAGE_MV  3100
#define MAX_WATCH_DOG 0x7f   // 2 minutes

#define Mx1000   ((1000 * (100 - 1))/(NVODM_BATTERY_FULL_VOLTAGE_MV \
      - NVODM_BATTERY_BOOTLOADER_VOLTAGE_MV))
#define Bx1000   ((1000 * 100) - (Mx1000 * NVODM_BATTERY_FULL_VOLTAGE_MV))

typedef struct EnterpriseBatDriverInfoRec
{
    NvOdmBatChargingDriverInfoHandle hBChargingDriver;
    NvOdmAdcDriverInfoHandle hAdcDriver;
} EnterpriseBatDriverInfo;

static EnterpriseBatDriverInfo s_EnterpriseBatInfo = {NULL, NULL};

NvU32 EnterprisePmuPercentCharged(NvU32 miliVolts)
{
    //Calculate percent
    NvU32 Percent = (Mx1000 * miliVolts + Bx1000 + 500) / 1000;
    //Limit 0 to 100
    if (Percent > 100) return 100;
    return Percent;
}

NvBool
EnterprisePmuGetAcLineStatus(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuAcLineStatus *pStatus)
{
    NV_ASSERT(hDevice);
    NV_ASSERT(pStatus);

    // NOTE: Not implemented completely as it is only for bootloader.

    *pStatus = NvOdmPmuAcLine_Online;
    return NV_TRUE;
}

NvBool
EnterprisePmuGetBatteryStatus(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvU8 *pStatus)
{
    NvU32 VbatSense = 0;
    NvBool ret;

    NV_ASSERT(hDevice);
    NV_ASSERT(pStatus);
    NV_ASSERT(batteryInst <= NvOdmPmuBatteryInst_Num);

    if (batteryInst == NvOdmPmuBatteryInst_Main)
    {
        ret = Tps8003xAdcDriverReadData(s_EnterpriseBatInfo.hAdcDriver, AdcVoltageChannel, &VbatSense);
        if (!ret)
        {
            NvOdmOsPrintf("Adc read failed \n");
            return NV_FALSE;
        }

        if (VbatSense > NVODM_BATTERY_HIGH_VOLTAGE_MV)  // modify these parameters on need
            *pStatus |= NVODM_BATTERY_STATUS_HIGH;
        else if ((VbatSense < NVODM_BATTERY_LOW_VOLTAGE_MV)&&
            (VbatSense > NVODM_BATTERY_CRITICAL_VOLTAGE_MV))
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
EnterprisePmuGetBatteryData(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvOdmPmuBatteryData *pData)
{
    NvOdmPmuBatteryData batteryData;
    NvU32 VbatSense;
    NvBool ret;


    NV_ASSERT(hDevice);
    NV_ASSERT(pData);
    NV_ASSERT(batteryInst <= NvOdmPmuBatteryInst_Num);

    batteryData.batteryLifePercent     = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryLifeTime        = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryVoltage         = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryCurrent         = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryAverageCurrent  = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryAverageInterval = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryMahConsumed     = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryTemperature     = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batterySoc             = NVODM_BATTERY_DATA_UNKNOWN;

    ret = Tps8003xAdcDriverReadData(s_EnterpriseBatInfo.hAdcDriver, AdcVoltageChannel, &VbatSense);
    if (!ret)
    {
        NvOdmOsPrintf("Adc read failed \n");
        return NV_FALSE;
    }
    batteryData.batteryVoltage = VbatSense;
    batteryData.batterySoc = EnterprisePmuPercentCharged(VbatSense);

    ret = Tps8003xBatChargingDriverUpdateWatchDog(s_EnterpriseBatInfo.hBChargingDriver, MAX_WATCH_DOG);
    if (!ret)
    {
        NvOdmOsPrintf("Update Watch Dog failed \n");
        return NV_FALSE;
    }
    *pData = batteryData;
    return NV_TRUE;
}

void
EnterprisePmuGetBatteryFullLifeTime(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvU32 *pLifeTime)
{
    *pLifeTime = NVODM_BATTERY_DATA_UNKNOWN;
}

void
EnterprisePmuGetBatteryChemistry(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvOdmPmuBatteryChemistry *pChemistry)
{
    *pChemistry = NvOdmPmuBatteryChemistry_LION;
}


NvBool
EnterprisePmuSetChargingCurrent(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuChargingPath chargingPath,
    NvU32 chargingCurrentLimitMa,
    NvOdmUsbChargerType ChargerType)
{
    NvBool ret;

    ret = Tps8003xBatChargingDriverSetCurrent(s_EnterpriseBatInfo.hBChargingDriver, chargingPath, chargingCurrentLimitMa, ChargerType);
    return ret;
}

void
*EnterprisePmu_Tps80031_BatterySetup(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmBoardInfo *pProcBoard,
    NvOdmBoardInfo *pPmuBoard)
{
    if (s_EnterpriseBatInfo.hAdcDriver && s_EnterpriseBatInfo.hBChargingDriver)
         return &s_EnterpriseBatInfo;

    s_EnterpriseBatInfo.hAdcDriver = Tps8003xAdcDriverOpen(hDevice);
    if (!s_EnterpriseBatInfo.hAdcDriver)
    {
          NvOdmOsPrintf("%s(): Error in call Tps8003xAdcDriverOpen()\n");
          return NULL;
    }

    s_EnterpriseBatInfo.hBChargingDriver = Tps8003xBatChargingDriverOpen(hDevice, NULL);
    if (!s_EnterpriseBatInfo.hBChargingDriver)
    {
          NvOdmOsPrintf("%s(): Error in call Tps8003xBatChargingDriverOpen()\n");
          Tps8003xAdcDriverClose(s_EnterpriseBatInfo.hAdcDriver);
          s_EnterpriseBatInfo.hAdcDriver = NULL;
          return NULL;
    }

   return &s_EnterpriseBatInfo;
}

NvBool
EnterprisePmuPowerOff(
    NvOdmPmuDeviceHandle hDevice)
{
    NvBool ret = NV_FALSE;

    if (s_EnterpriseBatInfo.hBChargingDriver)
        ret = Tps8003xPmuPowerOff(s_EnterpriseBatInfo.hBChargingDriver);

    return ret;
}
