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
 * <b>NVIDIA Tegra ODM Kit: Enterprise pmu driver interface.</b>
 *
 * @b Description: Defines the public interface for the pmu driver for
 *                 enterprise.
 *
*/

#ifndef INCLUDED_NVODM_PMU_ENTERPRISE_H
#define INCLUDED_NVODM_PMU_ENTERPRISE_H

#include "nvodm_pmu.h"

#if defined(__cplusplus)
extern "C"
{
#endif

NvU32 EnterprisePmuPercentCharged(NvU32 miliVolts);

void
EnterprisePmuGetCapabilities(
    NvU32 vddRail,
    NvOdmPmuVddRailCapabilities* pCapabilities);

NvBool
EnterprisePmuGetVoltage(
    NvOdmPmuDeviceHandle hDevice,
    NvU32 vddRail,
    NvU32* pMilliVolts);

NvBool
EnterprisePmuSetVoltage(
    NvOdmPmuDeviceHandle hDevice,
    NvU32 vddRail,
    NvU32 MilliVolts,
    NvU32* pSettleMicroSeconds);

NvBool EnterprisePmuSetup(NvOdmPmuDeviceHandle hDevice);

void EnterprisePmuRelease(NvOdmPmuDeviceHandle hDevice);

NvBool
EnterprisePmuGetAcLineStatus(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuAcLineStatus *pStatus);

NvBool
EnterprisePmuGetBatteryStatus(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvU8 *pStatus);

NvBool
EnterprisePmuGetBatteryData(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvOdmPmuBatteryData *pData);

void
EnterprisePmuGetBatteryFullLifeTime(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvU32 *pLifeTime);

void
EnterprisePmuGetBatteryChemistry(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuBatteryInstance batteryInst,
    NvOdmPmuBatteryChemistry *pChemistry);

NvBool EnterprisePmuSetChargingCurrent(
    NvOdmPmuDeviceHandle hDevice,
    NvOdmPmuChargingPath chargingPath,
    NvU32 chargingCurrentLimitMa,
    NvOdmUsbChargerType ChargerType);

void EnterprisePmuInterruptHandler(NvOdmPmuDeviceHandle  hDevice);

NvBool EnterprisePmuReadRtc(NvOdmPmuDeviceHandle hDevice, NvU32 *Count);

NvBool EnterprisePmuWriteRtc( NvOdmPmuDeviceHandle  hDevice, NvU32 Count);

NvBool EnterprisePmuIsRtcInitialized( NvOdmPmuDeviceHandle  hDevice);

NvBool EnterprisePmuPowerOff( NvOdmPmuDeviceHandle  hDevice);

#if defined(__cplusplus)
}
#endif

#endif /* INCLUDED_NVODM_PMU_ENTERPRISE_H */
