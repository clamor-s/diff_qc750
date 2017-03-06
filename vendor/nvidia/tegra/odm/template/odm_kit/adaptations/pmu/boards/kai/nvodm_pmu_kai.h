/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVODM_PMU_KAI_H
#define INCLUDED_NVODM_PMU_KAI_H

#include "nvodm_pmu.h"

#if defined(__cplusplus)
extern "C"
{
#endif

void
KaiPmuGetCapabilities(
    NvU32 vddRail,
    NvOdmPmuVddRailCapabilities* pCapabilities);

NvBool
KaiPmuGetVoltage(
    NvOdmPmuDeviceHandle hDevice,
    NvU32 vddRail,
    NvU32* pMilliVolts);

NvBool
KaiPmuSetVoltage(
    NvOdmPmuDeviceHandle hDevice,
    NvU32 vddRail,
    NvU32 MilliVolts,
    NvU32* pSettleMicroSeconds);

NvBool KaiPmuSetup(NvOdmPmuDeviceHandle hDevice);

void KaiPmuRelease(NvOdmPmuDeviceHandle hDevice);

NvBool
KaiPmuGetAcLineStatus(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuAcLineStatus *pStatus);

NvBool
KaiPmuGetBatteryStatus(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvU8 *pStatus);

NvBool
KaiPmuGetBatteryData(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvOdmPmuBatteryData *pData);

void
KaiPmuGetBatteryFullLifeTime(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvU32 *pLifeTime);

void
KaiPmuGetBatteryChemistry(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvOdmPmuBatteryChemistry *pChemistry);

NvBool KaiPmuSetChargingCurrent(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuChargingPath chargingPath,
    NvU32 chargingCurrentLimitMa,
    NvOdmUsbChargerType ChargerType);

void KaiPmuInterruptHandler(NvOdmPmuDeviceHandle  hDevice);

NvBool KaiPmuReadRtc(NvOdmPmuDeviceHandle hDevice, NvU32 *Count);

NvBool KaiPmuWriteRtc( NvOdmPmuDeviceHandle  hDevice, NvU32 Count);

NvBool KaiPmuIsRtcInitialized( NvOdmPmuDeviceHandle  hDevice);

NvBool KaiPmuPowerOff(NvOdmPmuDeviceHandle hDevice);

#if defined(__cplusplus)
}
#endif

#endif /* INCLUDED_NVODM_PMU_KAI_H */
