/*
 * Copyright (c) 2007-2011 NVIDIA Corporation.  All rights reserved.
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
 *                 entries for the e1155 Firefly DevLite board.
 */
#include "../include/nvodm_imager_guids.h"

// Audio codec
// NVTODO: Add support for AD1937
{
    NV_ODM_GUID('a','d','e','v','1','9','3','7'),
    s_e1155AudioCodecAddresses,
    NV_ARRAY_SIZE(s_e1155AudioCodecAddresses),
    NvOdmPeripheralClass_Other
},
// Audio codec for Lupin
{
    NV_ODM_GUID('x','x','d','i','r','a','n','a'),
    s_LupinAudioCodecAddresses,
    NV_ARRAY_SIZE(s_LupinAudioCodecAddresses),
    NvOdmPeripheralClass_Other
},
// Bluetooth on COMMs Module
// NVTODO: Verify this
{
    NV_ODM_GUID('b','l','u','t','o','o','t','h'),
    s_e1155BluetoothAddresses,
    NV_ARRAY_SIZE(s_e1155BluetoothAddresses),
    NvOdmPeripheralClass_Other
},

/*
  * Due to pin conflicts, WLAN and MIO Ethernet KITL are mutually exclusive.
  *
  * NOTE: The following entry shall be included in EEPROM unconditionally.
  *       nvApGetAllPeripherals() contains logic that manages this for Vail/Concorde
  *       when the entry is read from EEPROM and checks if MIO Ethernet KITL is in use.
  *       If MIO Ethernet KITL is enabled, this entry is left blank by nvApGetAllPeripherals().
  *       Therefore, calls to NvOdmPeripheralGetGuid() will not find the WLAN GUID
  *       in this instance, even though the value resides on EEPROM.
  */
// Sdio wlan  on COMMs Module
//{
    //NV_ODM_GUID('s','d','i','o','w','l','a','n'),
    //s_e1155WlanAddresses,
    //NV_ARRAY_SIZE(s_e1155WlanAddresses),
    //NvOdmPeripheralClass_Other
//},

// Sdio
{
    NV_ODM_GUID('s','d','i','o','_','m','e','m'),
    s_p852SdioAddresses,
    NV_ARRAY_SIZE(s_p852SdioAddresses),
    NvOdmPeripheralClass_Other
},

// VIP
{
    ADV7180_GUID,
    s_ffaImagerADV7180Addresses,
    NV_ARRAY_SIZE(s_ffaImagerADV7180Addresses),
    NvOdmPeripheralClass_Imager
},
{
    COMMONIMAGER_GUID,
    s_CommonImagerAddresses,
    NV_ARRAY_SIZE(s_CommonImagerAddresses),
    NvOdmPeripheralClass_Other
},

// TV Out Video Dac
//{
//    NV_ODM_GUID('f','f','a','t','v','o','u','t'),
//    s_ffaVideoDacAddresses,
//    NV_ARRAY_SIZE(s_ffaVideoDacAddresses),
//    NvOdmPeripheralClass_Display
//},
// NOTE: This list *must* end with a trailing comma.
