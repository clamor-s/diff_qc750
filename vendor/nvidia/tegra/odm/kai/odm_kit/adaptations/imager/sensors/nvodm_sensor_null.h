/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVODM_SENSOR_NULL_H
#define INCLUDED_NVODM_SENSOR_NULL_H

#if defined(__cplusplus)
extern "C"
{
#endif

#define SENSOR_NULL_DEVICE_ADDR 0x00

// Function Declarations
NvBool Sensor_NullYuv_Open(NvOdmImagerDeviceHandle sensor);
NvBool Sensor_NullBayer_Open(NvOdmImagerDeviceHandle sensor);
void Sensor_Null_Close(NvOdmImagerDeviceHandle sensor);
void    Sensor_Null_ListModes(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerSensorMode *timings,
        NvS32 *numberOfModes);
void    Sensor_Null_GetCapabilities(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerCapabilities *capabilities);
NvBool Sensor_Null_SetMode(
        NvOdmImagerDeviceHandle sensor,
        const SetModeParameters *pParameters,
        NvOdmImagerSensorMode *pSelectedMode,
        SetModeParameters *pResult);
NvBool Sensor_Null_SetPowerLevel(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerPowerLevel powerLevel);
NvBool Sensor_Null_SetParameter(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerParameter param,
        NvS32 sizeOfValue,
        const void *value);
NvBool Sensor_Null_GetParameter(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerParameter param,
        NvS32 sizeOfValue,
        void *value);

#if defined(__cplusplus)
}
#endif

/** @} */

#endif // INCLUDED_NVODM_SENSOR_NULL_H

