/*
 * Copyright (c) 2007-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef NVODM_QUERY_DISCOVERY_IMAGER_H
#define NVODM_QUERY_DISCOVERY_IMAGER_H


#include "nvodm_query_discovery.h"

#if defined(__cplusplus)
extern "C"
{
#endif

// GUIDS
#define SENSOR_BAYER_GUID           NV_ODM_GUID('s', '_', 'S', 'M', 'P', 'B', 'A', 'Y')
#define SENSOR_YUV_GUID             NV_ODM_GUID('s', '_', 'S', 'M', 'P', 'Y', 'U', 'V')
#define FOCUSER_GUID                NV_ODM_GUID('f', '_', 'S', 'M', 'P', 'F', 'O', 'C')
#define FLASH_GUID                  NV_ODM_GUID('f', '_', 'S', 'M', 'P', 'F', 'L', 'A')

// for specific sensors
#define SENSOR_BYRRI_AR0832_GUID    NV_ODM_GUID('s', 'R', 'A', 'R', '0', '8', '3', '2')
#define SENSOR_BYRLE_AR0832_GUID    NV_ODM_GUID('s', 'L', 'A', 'R', '0', '8', '3', '2')
#define SENSOR_BYRST_AR0832_GUID    NV_ODM_GUID('s', 't', 'A', 'R', '0', '8', '3', '2')
#define SENSOR_BAYER_OV5630_GUID    NV_ODM_GUID('s', '_', 'O', 'V', '5', '6', '3', '0')
#define SENSOR_BAYER_OV5650_GUID    NV_ODM_GUID('s', '_', 'O', 'V', '5', '6', '5', '0')
#define SENSOR_BYRST_OV5650_GUID    NV_ODM_GUID('s', 't', 'O', 'V', '5', '6', '5', '0')
#define SENSOR_BAYER_OV9726_GUID    NV_ODM_GUID('s', '_', 'O', 'V', '9', '7', '2', '6')
#define SENSOR_BAYER_OV2710_GUID    NV_ODM_GUID('s', '_', 'O', 'V', '2', '7', '1', '0')

// for specific focuser
#define FOCUSER_AR0832_GUID         NV_ODM_GUID('f', '_', 'A', 'R', '0', '8', '3', '2')
#define FOCUSER_AD5820_GUID         NV_ODM_GUID('f', '_', 'A', 'D', '5', '8', '2', '0')

// for specific flash
#define FLASH_LTC3216_GUID          NV_ODM_GUID('l', '_', 'L', 'T', '3', '2', '1', '6')
#define FLASH_TPS61050_GUID         NV_ODM_GUID('l', '_', 'T', '6', '1', '0', '5', '0')
#define TORCH_NVC_GUID              NV_ODM_GUID('l', '_', 'N', 'V', 'C', 'A', 'M', '0')

// Switching the encoding from the VideoInput module address to use with
// each GPIO module address.
// NVODM_IMAGER_GPIO will tell the nvodm imager how to use each gpio
// A gpio can be used for powerdown (lo, hi) or !powerdown (hi, lo)
// used for reset (hi, lo, hi) or for !reset (lo, hi, lo)
// Or, for mclk or pwm (unimplemented yet)
// We have moved the flash to its own, so it is not needed here
#define NVODM_IMAGER_GPIO_PIN_SHIFT    (24)
#define NVODM_IMAGER_UNUSED            (0x0)
#define NVODM_IMAGER_RESET             (0x1 << NVODM_IMAGER_GPIO_PIN_SHIFT)
#define NVODM_IMAGER_RESET_AL          (0x2 << NVODM_IMAGER_GPIO_PIN_SHIFT)
#define NVODM_IMAGER_POWERDOWN         (0x3 << NVODM_IMAGER_GPIO_PIN_SHIFT)
#define NVODM_IMAGER_POWERDOWN_AL      (0x4 << NVODM_IMAGER_GPIO_PIN_SHIFT)
// only on VGP0
#define NVODM_IMAGER_MCLK              (0x8 << NVODM_IMAGER_GPIO_PIN_SHIFT)
// only on VGP6
#define NVODM_IMAGER_PWM               (0x9 << NVODM_IMAGER_GPIO_PIN_SHIFT)
// If flash code wants the gpio's labelled
// use for any purpose, or not at all
#define NVODM_IMAGER_FLASH0           (0x5 << NVODM_IMAGER_GPIO_PIN_SHIFT)
#define NVODM_IMAGER_FLASH1           (0x6 << NVODM_IMAGER_GPIO_PIN_SHIFT)
#define NVODM_IMAGER_FLASH2           (0x7 << NVODM_IMAGER_GPIO_PIN_SHIFT)
// Shutter control
#define NVODM_IMAGER_SHUTTER          (0xA << NVODM_IMAGER_GPIO_PIN_SHIFT)


#define NVODM_IMAGER_MASK              (0xF << NVODM_IMAGER_GPIO_PIN_SHIFT)
#define NVODM_IMAGER_CLEAR(_s)         ((_s) & ~(NVODM_IMAGER_MASK))
#define NVODM_IMAGER_IS_SET(_s)        (((_s) & (NVODM_IMAGER_MASK)) != 0)
#define NVODM_IMAGER_FIELD(_s)         ((_s) >> NVODM_IMAGER_GPIO_PIN_SHIFT)


#if defined(__cplusplus)
}
#endif

#endif //NVODM_QUERY_DISCOVERY_IMAGER_H
