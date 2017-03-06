/*
 * Copyright (c) 2007-2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVODM_SENSOR_MI5130_H
#define INCLUDED_NVODM_SENSOR_MI5130_H

#if defined(__cplusplus)
extern "C"
{
#endif

#include "nvodm_imager_device.h"

#define SENSOR_MI5130_DEVID         0x5130
#define SENSOR_MI5130_DEVICE_ADDR   0x20

// Function Declarations
NvBool Sensor_MI5130_Open(NvOdmImagerDeviceHandle sensor);

void Sensor_MI5130_Close(NvOdmImagerDeviceHandle sensor);

#if defined(__cplusplus)
}
#endif

/** @} */

#endif // INCLUDED_NVODM_SENSOR_MI5130_H

