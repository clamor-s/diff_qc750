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
 *                 database Peripheral entries for the E1129
 *                 keypad module.
 */

// Key Pad
{
    NV_ODM_GUID('k','e','y','b','o','a','r','d'),
    s_KeyPadAddresses,
    NV_ARRAY_SIZE(s_KeyPadAddresses),
    NvOdmPeripheralClass_HCI
},

// Scroll Wheel
{
    NV_ODM_GUID('s','c','r','o','l','w','h','l'),
    s_ffaScrollWheelAddresses,
    NV_ARRAY_SIZE(s_ffaScrollWheelAddresses),
    NvOdmPeripheralClass_HCI
},

// NOTE: This list *must* end with a trailing comma.
