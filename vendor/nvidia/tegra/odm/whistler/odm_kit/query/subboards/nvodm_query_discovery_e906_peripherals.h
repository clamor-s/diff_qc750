/*
 * Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
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
 *                 database Peripheral entries for the E906 LCD
 *                 Module.
 */

// LCD module
{
    // Sharp WVGA panel with AP20 backlight control
    NV_ODM_GUID('S','H','P','_','A','P','2','0'),
    s_ffaMainDisplayAddresses,
    NV_ARRAY_SIZE(s_ffaMainDisplayAddresses),
    NvOdmPeripheralClass_Display,
},

// LCD module
{
    NV_ODM_GUID('f','f','a','_','l','c','d','0'),
    s_SonyPanelAddresses,
    NV_ARRAY_SIZE(s_SonyPanelAddresses),
    NvOdmPeripheralClass_Display,
},

// DSI module
{
    NV_ODM_GUID('s','h','a','r','p','d','s','i'),
    s_DsiAddresses,
    NV_ARRAY_SIZE(s_DsiAddresses),
    NvOdmPeripheralClass_Display,
},

//  Touch Panel
{
    NV_ODM_GUID('t','p','k','t','o','u','c','h'),
    s_ffaTouchPanelAddresses,
    NV_ARRAY_SIZE(s_ffaTouchPanelAddresses),
    NvOdmPeripheralClass_HCI
},

// NOTE: This list *must* end with a trailing comma.
