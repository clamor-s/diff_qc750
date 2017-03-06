/*
 * Copyright (c) 2011-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVODM_PMU_CARDHU_H
#define INCLUDED_NVODM_PMU_CARDHU_H

#include "nvodm_pmu.h"

#if defined(__cplusplus)
extern "C"
{
#endif

void
CardhuPmuGetCapabilities(
    NvU32 vddRail,
    NvOdmPmuVddRailCapabilities* pCapabilities);

NvBool
CardhuPmuGetVoltage(
    NvOdmPmuDeviceHandle hDevice,
    NvU32 vddRail,
    NvU32* pMilliVolts);

NvBool
CardhuPmuSetVoltage(
    NvOdmPmuDeviceHandle hDevice,
    NvU32 vddRail,
    NvU32 MilliVolts,
    NvU32* pSettleMicroSeconds);

NvBool CardhuPmuSetup(NvOdmPmuDeviceHandle hDevice);

void CardhuPmuRelease(NvOdmPmuDeviceHandle hDevice);

NvBool
CardhuPmuGetAcLineStatus(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuAcLineStatus *pStatus);

NvBool
CardhuPmuGetBatteryStatus(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvU8 *pStatus);

NvBool
CardhuPmuGetBatteryData(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvOdmPmuBatteryData *pData);

void
CardhuPmuGetBatteryFullLifeTime(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvU32 *pLifeTime);

void
CardhuPmuGetBatteryChemistry(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvOdmPmuBatteryChemistry *pChemistry);

NvBool CardhuPmuSetChargingCurrent(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuChargingPath chargingPath,
    NvU32 chargingCurrentLimitMa,
    NvOdmUsbChargerType ChargerType);

void CardhuPmuInterruptHandler(NvOdmPmuDeviceHandle  hDevice);

NvBool CardhuPmuReadRtc(NvOdmPmuDeviceHandle hDevice, NvU32 *Count);

NvBool CardhuPmuWriteRtc( NvOdmPmuDeviceHandle  hDevice, NvU32 Count);

NvBool CardhuPmuIsRtcInitialized( NvOdmPmuDeviceHandle  hDevice);

NvBool CardhuPmuPowerOff( NvOdmPmuDeviceHandle  hDevice);

#if defined(__cplusplus)
}
#endif

#endif /* INCLUDED_NVODM_PMU_CARDHU_H */
