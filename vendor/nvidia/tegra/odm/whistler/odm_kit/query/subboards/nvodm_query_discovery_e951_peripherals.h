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
 *                 database Peripheral entries for the E951
 *                 COMMS module.
 */
// Bluetooth on COMMs Module
{
     NV_ODM_GUID('b','l','u','t','o','o','t','h'),
     ffaBluetoothAddresses,
     NV_ARRAY_SIZE(ffaBluetoothAddresses),
     NvOdmPeripheralClass_Other
},

// Sdio wlan  on COMMs Module
{
    NV_ODM_GUID('s','d','i','o','w','l','a','n'),
    s_ffaWlanAddresses,
    NV_ARRAY_SIZE(s_ffaWlanAddresses),
    NvOdmPeripheralClass_Other
},

// EMP Modem on COMMs Module
{
    NV_ODM_GUID('e','m','p',' ','_','m','d','m'),
    s_ffaEmpAddresses,
    NV_ARRAY_SIZE(s_ffaEmpAddresses),
    NvOdmPeripheralClass_Other
},

// EMP M570 Modem on COMMs Module
{
    NV_ODM_GUID('e','m','p',' ','M','5','7','0'),
    s_ffaEmpM570Addresses,
    NV_ARRAY_SIZE(s_ffaEmpM570Addresses),
    NvOdmPeripheralClass_Other
},

// NOTE: This list *must* end with a trailing comma.
