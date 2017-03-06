/**
 * Copyright (c) 2008 NVIDIA Corporation.  All rights reserved.
 * 
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef SENSOR_YUV_GC0308_H
#define SENSOR_YUV_GC0308_H

#include "imager_hal.h"
#include "nvodm_query_discovery_imager.h"
#define SENSOR_YUV_GC0308_GUID        NV_ODM_GUID('s', '_', 'G', 'C', '0', '3', '0', '8')//modifyed by jimmy
#define SENSOR_YUV_MT9P111_GUID        NV_ODM_GUID('s', '_', 'M', 'T', '9', 'P', '1', '1')//modifyed by jimmy
#if defined(__cplusplus)
extern "C"
{
#endif

NvBool SensorYuv_MT9P111GetHal(NvOdmImagerHandle hImager);

#if defined(__cplusplus)
}
#endif

#endif  //SENSOR_YUV_MT9P111_H
