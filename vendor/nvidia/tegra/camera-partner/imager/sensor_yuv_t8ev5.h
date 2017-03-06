/**
 * Copyright (c) 2008 NVIDIA Corporation.  All rights reserved.
 * 
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef SENSOR_YUV_T8EV5_H
#define SENSOR_YUV_T8EV5_H

#include "imager_hal.h"
#include "nvodm_query_discovery_imager.h"
#define FOCUSER_T8EV5_GUID         NV_ODM_GUID('f', '_', 'T', '8', 'E', 'V', '5', '0')
#define SENSOR_T8EV5_GUID         NV_ODM_GUID('s', '_', 'T', '8', 'E', 'V', '5', '0')

#if defined(__cplusplus)
extern "C"
{
#endif

NvBool SensorYuv_T8EV5_GetHal(NvOdmImagerHandle hImager);

#if defined(__cplusplus)
}
#endif

#endif  //SENSOR_YUV_MT9P111_H
