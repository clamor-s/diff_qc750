/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/** 
 * @file
 * <b>NVIDIA Tegra ODM Kit:
 *         Abstraction layer stub for audio codec adaptations</b>
 */

#ifndef INCLUDED_NVODM_PMU_ADAPTATION_HAL_H
#define INCLUDED_NVODM_PMU_ADAPTATION_HAL_H

#include "nvcommon.h"
#include "nvodm_pmu.h"
#include "nvodm_services.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef NvBool (*pfnPmuSetup)(NvOdmPmuDeviceHandle);
typedef void   (*pfnPmuRelease)(NvOdmPmuDeviceHandle);
typedef void   (*pfnPmuGetCaps)(NvU32, NvOdmPmuVddRailCapabilities*);
typedef NvBool (*pfnPmuGetVoltage)(NvOdmPmuDeviceHandle, NvU32, NvU32*);
typedef NvBool (*pfnPmuSetVoltage)(NvOdmPmuDeviceHandle, NvU32, NvU32, NvU32*);
typedef NvBool (*pfnPmuGetAcLineStatus)(NvOdmPmuDeviceHandle, NvOdmPmuAcLineStatus*);
typedef NvBool (*pfnPmuGetBatteryStatus)(NvOdmPmuDeviceHandle, NvOdmPmuBatteryInstance, NvU8*);
typedef NvBool (*pfnPmuGetBatteryData)(NvOdmPmuDeviceHandle, NvOdmPmuBatteryInstance, NvOdmPmuBatteryData*);
typedef void   (*pfnPmuGetBatteryFullLifeTime)(NvOdmPmuDeviceHandle, NvOdmPmuBatteryInstance, NvU32*);
typedef void   (*pfnPmuGetBatteryChemistry)(NvOdmPmuDeviceHandle, NvOdmPmuBatteryInstance, NvOdmPmuBatteryChemistry*);
typedef NvBool (*pfnPmuSetChargingCurrent)(NvOdmPmuDeviceHandle, NvOdmPmuChargingPath, NvU32, NvOdmUsbChargerType);
typedef void   (*pfnPmuInterruptHandler)(NvOdmPmuDeviceHandle);
typedef NvBool (*pfnPmuReadRtc)(NvOdmPmuDeviceHandle, NvU32*);
typedef NvBool (*pfnPmuWriteRtc)(NvOdmPmuDeviceHandle, NvU32);
typedef NvBool (*pfnPmuIsRtcInitialized)(NvOdmPmuDeviceHandle);
typedef NvBool (*pfnPmuPowerOff)(NvOdmPmuDeviceHandle);

typedef struct NvOdmPmuDeviceRec
{ 
    pfnPmuSetup                  pfnSetup;
    pfnPmuRelease                pfnRelease;
    pfnPmuGetCaps                pfnGetCaps;
    pfnPmuGetVoltage             pfnGetVoltage;
    pfnPmuSetVoltage             pfnSetVoltage;
    pfnPmuGetAcLineStatus        pfnGetAcLineStatus;
    pfnPmuGetBatteryStatus       pfnGetBatteryStatus;
    pfnPmuGetBatteryData         pfnGetBatteryData;
    pfnPmuGetBatteryFullLifeTime pfnGetBatteryFullLifeTime;
    pfnPmuGetBatteryChemistry    pfnGetBatteryChemistry;
    pfnPmuSetChargingCurrent     pfnSetChargingCurrent;
    pfnPmuInterruptHandler       pfnInterruptHandler;
    pfnPmuReadRtc                pfnReadRtc;
    pfnPmuWriteRtc               pfnWriteRtc;
    pfnPmuIsRtcInitialized       pfnIsRtcInitialized;
    pfnPmuPowerOff               pfnPowerOff;
    void                        *pPrivate;
    NvBool                       Hal;
    NvBool                       Init;
} NvOdmPmuDevice;

#ifdef __cplusplus
}
#endif

#endif
