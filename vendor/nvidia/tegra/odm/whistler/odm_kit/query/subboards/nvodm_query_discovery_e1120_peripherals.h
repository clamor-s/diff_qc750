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
 * @b Description: Specifies the peripheral connectivity database Peripheral
 *                 entries for the E1120 AP20 Development System
 *                 Motherboard.
 */

//  Ethernet module
{
    NV_ODM_GUID('e','n','c','2','8','j','6','0'),
    s_enc28j60EthernetAddresses,
    NV_ARRAY_SIZE(s_enc28j60EthernetAddresses),
    NvOdmPeripheralClass_Other
},

//  Sdio module
{
    NV_ODM_GUID('s','d','i','o','_','m','e','m'),
    s_SdioAddresses,
    NV_ARRAY_SIZE(s_SdioAddresses),
    NvOdmPeripheralClass_Other,
},

//..Vibrate Module
{
    NV_ODM_GUID('v','i','b','r','a','t','o','r'),
    s_VibAddresses,
    NV_ARRAY_SIZE(s_VibAddresses),
    NvOdmPeripheralClass_Other,
},
// NOTE: This list *must* end with a trailing comma.
