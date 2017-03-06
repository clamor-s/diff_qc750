/*
 * Copyright (c) 2008 NVIDIA Corporation.  All rights reserved.
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
 *         Abstraction layer stub for imager adaptations</b>
 */

#ifndef INCLUDED_NVODM_IMAGER_ADAPTATION_HAL_H
#define INCLUDED_NVODM_IMAGER_ADAPTATION_HAL_H

#include "nvodm_imager.h"
#include "nvos.h"
#include "nvodm_query_discovery.h"

#ifdef __cplusplus
extern "C"
{
#endif

//  A simple HAL for imager adaptations.  Most of these functions
//  map one-to-one with the ODM interface.

typedef NvBool (*pfnImagerSensorOpen)(NvOdmImagerHandle hImager);

typedef void (*pfnImagerSensorClose)(NvOdmImagerHandle hImager);

typedef void
(*pfnImagerSensorGetCapabilities)(NvOdmImagerHandle hImager,
                            NvOdmImagerCapabilities *pCapabilities);

typedef void
(*pfnImagerSensorListSensorModes)(NvOdmImagerHandle hImager,
                            NvOdmImagerSensorMode *pModes,
                            NvS32 *pNumberOfModes);

typedef NvBool
(*pfnImagerSensorSetSensorMode)(NvOdmImagerHandle hImager,
                            const SetModeParameters *pParameters,
                            NvOdmImagerSensorMode *pSelectedMode,
                            SetModeParameters *pResult);

typedef NvBool
(*pfnImagerSensorSetPowerLevel)(NvOdmImagerHandle hImager,
                          NvOdmImagerPowerLevel PowerLevel);

typedef NvBool
(*pfnImagerSensorSetParameter)(NvOdmImagerHandle hImager,
                         NvOdmImagerParameter Param,
                         NvS32 SizeOfValue,
                         const void *pValue);

typedef NvBool
(*pfnImagerSensorGetParameter)(NvOdmImagerHandle hImager,
                         NvOdmImagerParameter Param,
                         NvS32 SizeOfValue,
                         void *pValue);

typedef void
(*pfnImagerSensorGetPowerLevel)(NvOdmImagerHandle hImager,
                          NvOdmImagerPowerLevel *pPowerLevel);


typedef NvBool (*pfnImagerSubdeviceOpen)(NvOdmImagerHandle hImager);

typedef void (*pfnImagerSubdeviceClose)(NvOdmImagerHandle hImager);

typedef NvBool
(*pfnImagerSubdeviceSetPowerLevel)(NvOdmImagerHandle hImager,
                          NvOdmImagerPowerLevel PowerLevel);

typedef NvBool
(*pfnImagerSubdeviceSetParameter)(NvOdmImagerHandle hImager,
                         NvOdmImagerParameter Param,
                         NvS32 SizeOfValue,
                         const void *pValue);

typedef NvBool
(*pfnImagerSubdeviceGetParameter)(NvOdmImagerHandle hImager,
                         NvOdmImagerParameter Param,
                         NvS32 SizeOfValue,
                         void *pValue);

typedef void
(*pfnImagerSubdeviceGetCapabilities)(NvOdmImagerHandle hImager,
                            NvOdmImagerCapabilities *pCapabilities);

typedef NvBool
(*pfnImagerGetHal)(NvOdmImagerHandle hImager);


typedef struct NvOdmImagerSensorRec
{
    NvU64                                   GUID;
    const NvOdmPeripheralConnectivity*            pConnections;

    pfnImagerSensorOpen               pfnOpen;
    pfnImagerSensorClose              pfnClose;
    pfnImagerSensorGetCapabilities    pfnGetCapabilities;
    pfnImagerSensorListSensorModes    pfnListModes;
    pfnImagerSensorSetSensorMode      pfnSetMode;
    pfnImagerSensorSetPowerLevel      pfnSetPowerLevel;
    pfnImagerSensorGetPowerLevel      pfnGetPowerLevel;
    pfnImagerSensorSetParameter       pfnSetParameter;
    pfnImagerSensorGetParameter       pfnGetParameter;

    void*                             pPrivateContext;

} NvOdmImagerSensor;

typedef struct NvOdmImagerSubdeviceRec
{
    NvU64       GUID;
    const NvOdmPeripheralConnectivity*            pConnections;

    pfnImagerSubdeviceOpen               pfnOpen;
    pfnImagerSubdeviceClose              pfnClose;
    pfnImagerSubdeviceGetCapabilities    pfnGetCapabilities;
    pfnImagerSubdeviceSetPowerLevel      pfnSetPowerLevel;
    pfnImagerSubdeviceSetParameter       pfnSetParameter;
    pfnImagerSubdeviceGetParameter       pfnGetParameter;

    void*       pPrivateContext;
    
}NvOdmImagerSubdevice;

typedef struct NvOdmImagerRec
{
    NvOdmImagerSensor        *pSensor;
    NvOdmImagerSubdevice     *pFocuser;
    NvOdmImagerSubdevice     *pFlash;

    void                     *pPrivateContext;

} NvOdmImager;

#ifdef __cplusplus
}
#endif

#endif
