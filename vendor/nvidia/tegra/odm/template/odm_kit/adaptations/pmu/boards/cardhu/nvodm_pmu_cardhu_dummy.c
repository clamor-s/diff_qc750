/*
 * Copyright (c) 2011-2012 NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_pmu.h"
#include "nvodm_pmu_cardhu.h"
#include "nvassert.h"

NvBool
CardhuPmuGetAcLineStatus(
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
CardhuPmuGetBatteryStatus(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvU8 *pStatus)
{
    NV_ASSERT(hDevice);
    NV_ASSERT(pStatus);
    NV_ASSERT(batteryInst <= NvOdmPmuBatteryInst_Num);

   // NOTE: Not implemented completely as it is only for bootloader.
   *pStatus = NVODM_BATTERY_STATUS_NO_BATTERY;
    return NV_TRUE;
}

NvBool
CardhuPmuGetBatteryData(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvOdmPmuBatteryData *pData)
{
    NvOdmPmuBatteryData batteryData;

    batteryData.batteryLifePercent     = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryLifeTime        = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryVoltage         = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryCurrent         = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryAverageCurrent  = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryAverageInterval = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryMahConsumed     = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryTemperature     = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batteryVoltage         = NVODM_BATTERY_DATA_UNKNOWN;
    batteryData.batterySoc             = NVODM_BATTERY_DATA_UNKNOWN;

    NV_ASSERT(hDevice);
    NV_ASSERT(pData);
    NV_ASSERT(batteryInst <= NvOdmPmuBatteryInst_Num);

    // NOTE: Not implemented completely as it is only for bootloader.
    *pData = batteryData;

    return NV_TRUE;
}

void
CardhuPmuGetBatteryFullLifeTime(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvU32 *pLifeTime)
{
    *pLifeTime = NVODM_BATTERY_DATA_UNKNOWN;
}

void
CardhuPmuGetBatteryChemistry(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvOdmPmuBatteryChemistry *pChemistry)
{
    *pChemistry = NvOdmPmuBatteryChemistry_LION;
}


NvBool
CardhuPmuSetChargingCurrent(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuChargingPath chargingPath,
    NvU32 chargingCurrentLimitMa,
    NvOdmUsbChargerType ChargerType)
{
    NV_ASSERT(hDevice);

    // NOTE: Not implemented completely as it is only for bootloader.
    return NV_TRUE;
}

void CardhuPmuInterruptHandler( NvOdmPmuDeviceHandle  hDevice)
{
    // NOTE: Not implemented completely as it is only for bootloader.
}

NvBool CardhuPmuReadRtc( NvOdmPmuDeviceHandle  hDevice, NvU32 *Count)
{
    // NOTE: Not implemented completely as it is only for bootloader.
    *Count = 0;
    return NV_FALSE;
}

NvBool CardhuPmuWriteRtc( NvOdmPmuDeviceHandle  hDevice, NvU32 Count)
{
    // NOTE: Not implemented completely as it is only for bootloader.
    return NV_FALSE;
}

NvBool CardhuPmuIsRtcInitialized( NvOdmPmuDeviceHandle  hDevice)
{
    // NOTE: Not implemented completely as it is only for bootloader.
    return NV_FALSE;
}
