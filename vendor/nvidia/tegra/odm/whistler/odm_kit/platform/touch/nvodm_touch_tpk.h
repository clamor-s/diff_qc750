/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 * 
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVODM_TOUCH_TPK_H
#define INCLUDED_NVODM_TOUCH_TPK_H

#include "nvodm_touch_int.h"
#include "nvodm_services.h"

#if defined(__cplusplus)
extern "C"
{
#endif

NvBool TPK_Open( NvOdmTouchDeviceHandle *hDevice);

void TPK_GetCapabilities(NvOdmTouchDeviceHandle hDevice, NvOdmTouchCapabilities* pCapabilities);

NvBool TPK_ReadCoordinate( NvOdmTouchDeviceHandle hDevice, NvOdmTouchCoordinateInfo *coord);

NvBool TPK_EnableInterrupt(NvOdmTouchDeviceHandle hDevice, NvOdmOsSemaphoreHandle hInterruptSemaphore);

NvBool TPK_HandleInterrupt(NvOdmTouchDeviceHandle hDevice);

NvBool TPK_GetSampleRate(NvOdmTouchDeviceHandle hDevice, NvOdmTouchSampleRate* pTouchSampleRate);

NvBool TPK_SetSampleRate(NvOdmTouchDeviceHandle hDevice, NvU32 rate);

NvBool TPK_PowerControl(NvOdmTouchDeviceHandle hDevice, NvOdmTouchPowerModeType mode);

NvBool TPK_GetCalibrationData(NvOdmTouchDeviceHandle hDevice, NvU32 NumOfCalibrationData, NvS32* pRawCoordBuffer);

NvBool TPK_PowerOnOff(NvOdmTouchDeviceHandle hDevice, NvBool OnOff);

void TPK_Close( NvOdmTouchDeviceHandle hDevice);


#if defined(__cplusplus)
}
#endif


#endif // INCLUDED_NVODM_TOUCH_TPK_H
