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
 * @b Description: Specifies the peripheral connectivity
 *                 database Peripheral entries for the E91109
 *                 Processor Module.
 */

// HDMI
{
    NV_ODM_GUID('f','f','a','_','h','d','m','i'),
    s_ffaHdmiAddresses,
    NV_ARRAY_SIZE(s_ffaHdmiAddresses),
    NvOdmPeripheralClass_Display
},

// CRT
{
    NV_ODM_GUID('f','f','a','_','-','c','r','t'),
    s_ffaCrtAddresses,
    NV_ARRAY_SIZE(s_ffaCrtAddresses),
    NvOdmPeripheralClass_Display
},

// TV Out Video Dac
{
    NV_ODM_GUID('f','f','a','t','v','o','u','t'),
    s_ffaVideoDacAddresses,
    NV_ARRAY_SIZE(s_ffaVideoDacAddresses),
    NvOdmPeripheralClass_Display
},

// Temperature Monitor (TMON)
{
    NV_ODM_GUID('a','d','t','7','4','6','1',' '),
    s_Tmon0Addresses,
    NV_ARRAY_SIZE(s_Tmon0Addresses),
    NvOdmPeripheralClass_Other
},
 
// NOTE: This list *must* end with a trailing comma.
