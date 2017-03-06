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
 * <b>NVIDIA APX ODM Kit::
 *         Implementation of the ODM Peripheral Discovery API</b>
 *
 * @b Description: Specifies the peripheral connectivity database Peripheral entries
 *                 for the E911 5MP + 1.3MP + VGA VI Camera module.
 */
#include "../include/nvodm_imager_guids.h"

// !!! DON'T MOVE THINGS AROUND !!!
// Position 0 is used as default primary for E912
// Position 1 is used as default secondary for E912 and E911
// Position 2 is used as default primary for E911

// Imager - Primary
// E912 A01 and Whistler Imager
{
    SENSOR_BAYER_OV5650_GUID,
    s_ffaImagerOV5650Addresses,
    NV_ARRAY_SIZE(s_ffaImagerOV5650Addresses),
    NvOdmPeripheralClass_Imager
},
// Imager - Secondary
// sensor for SEMCO VGA
{
    // Aptina (Micron) SOC380
    SENSOR_YUV_SOC380_GUID,
    s_ffaImagerSOC380Addresses,
    NV_ARRAY_SIZE(s_ffaImagerSOC380Addresses),
    NvOdmPeripheralClass_Imager
},

// Dummy Entry for Whistler
{
    MI5130_GUID,
    s_ffaImagerOV5650Addresses,
    NV_ARRAY_SIZE(s_ffaImagerOV5650Addresses),
    NvOdmPeripheralClass_Imager
},

// OV5650 stereo Imager
{
    SENSOR_BYRST_OV5650_GUID,
    s_ffaImagerOV5650StereoAddresses,
    NV_ARRAY_SIZE(s_ffaImagerOV5650StereoAddresses),
    NvOdmPeripheralClass_Imager
},

// focuser for OV5650 module
{
    // VCM driver IC AD5820 Analog Devices
    AD5820_GUID,
    s_ffaImagerAD5820Addresses,
    NV_ARRAY_SIZE(s_ffaImagerAD5820Addresses),
    NvOdmPeripheralClass_Imager
},

// flash device
{
    LTC3216_GUID,
    s_ffaFlashLTC3216Addresses,
    NV_ARRAY_SIZE(s_ffaFlashLTC3216Addresses),
    NvOdmPeripheralClass_Other
},

{
    COMMONIMAGER_GUID,
    s_CommonImagerAddresses,
    NV_ARRAY_SIZE(s_CommonImagerAddresses),
    NvOdmPeripheralClass_Other
},
