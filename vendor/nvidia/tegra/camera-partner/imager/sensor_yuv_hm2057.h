/*
 * Copyright (c) 2008-2013 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef SENSOR_HM2057_YUV_H
#define SENSOR_HM2057_YUV_H

#include "imager_hal.h"
#include "nvodm_query_discovery_imager.h"

#if defined(__cplusplus)
extern "C"
{
#endif

NvBool SensorYuv_Hm2057_GetHal(NvOdmImagerHandle hImager);

#if defined(__cplusplus)
}
#endif

#endif  //SENSOR_YUV_H
