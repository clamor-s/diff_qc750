/*
 * Copyright (c) 2011-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 *
 * @file
 * <b>NVIDIA APX ODM Kit::
 *         Implementation of the ODM Peripheral Discovery API</b>
 *
 * @b Description: Specifies the peripheral connectivity database Peripheral
 *                 entries for the peripherals on E1294 module.
 */

//#define E1211_PERIPHERALS_ENABLE_OV5640
// Imager - Rear Left Sensor
{
    SENSOR_BAYER_OV5650_GUID,
    s_ffaImagerOV5650Addresses,
    NV_ARRAY_SIZE(s_ffaImagerOV5650Addresses),
    NvOdmPeripheralClass_Imager
},

#ifdef E1211_PERIPHERALS_ENABLE_OV5640
// Imager - Front sensor override
{
    SENSOR_YUV_OV5640_GUID,
    s_ffaImagerOV5640Addresses,
    NV_ARRAY_SIZE(s_ffaImagerOV5640Addresses),
    NvOdmPeripheralClass_Imager
},
#else
// Imager - Front Sensor
{
    SENSOR_BAYER_OV2710_GUID,
    s_ffaImagerOV2710Addresses,
    NV_ARRAY_SIZE(s_ffaImagerOV2710Addresses),
    NvOdmPeripheralClass_Imager
},
#endif

// Imager - Rear Left Sensor (Repeated until right/left support is completed)
{
    SENSOR_BAYER_OV5650_GUID,
    s_ffaImagerOV5650Addresses,
    NV_ARRAY_SIZE(s_ffaImagerOV5650Addresses),
    NvOdmPeripheralClass_Imager
},
// Imager - Rear Stereo Sensor
{
    SENSOR_BYRST_OV5650_GUID,
    s_ffaImagerOV5650SAddresses,
    NV_ARRAY_SIZE(s_ffaImagerOV5650SAddresses),
    NvOdmPeripheralClass_Imager
},
// Imager - Flash
{
    TORCH_NVC_GUID,
    NULL,
    0,
    NvOdmPeripheralClass_Other
},
