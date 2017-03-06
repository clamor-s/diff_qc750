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
 * @b Description: Specifies the peripheral connectivity database NvOdmIoAddress
 *                 entries for the e1155 Firefly DevLite board.
 */

#include "../../adaptations/pmu/nvodm_pmu_tps6586x_supply_info_table.h"
#include "../include/nvodm_imager_guids.h"
// Audio Codec AD1937
static const NvOdmIoAddress s_e1155_p852_b00_AudioCodecAddresses[] =
{
    { NvOdmIoModule_I2c, 0x00, 0x0E },     /* Codec I2C_1 ->  */
};
// Audio Codec AD1937
static const NvOdmIoAddress s_p852_b00_LupinAudioCodecAddresses[] =
{
    { NvOdmIoModule_ExternalClock, 0, 0 }, //We just need Clock interface. I2C will be done by MMSE
};

// Bluetooth
// NVTODO: Verify this
static const NvOdmIoAddress s_e1155_p852_b00_BluetoothAddresses[] =
{
    { NvOdmIoModule_Uart, 0x2,  0x0 },
    //{ NvOdmIoModule_Gpio, 'u' - 'a', 0x0 },                    /* GPIO Port U and Pin 0 */
    // B00 changes : LDO7 not used
    //{ NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO7 }   /* VDD -> RF4REG */
};

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
// Wlan
//static const NvOdmIoAddress s_e1155_p852_b00_WlanAddresses[] =
//{
    //{ NvOdmIoModule_Sdio, 0x1, 0x0 },                      /* WLAN is on SD Bus */
    //{ NvOdmIoModule_Gpio, 0xa, 0x5 },                      /* GPIO Port K and Pin 5 - WIFI_PWR*/
    //{ NvOdmIoModule_Gpio, 0xa, 0x6 },                      /* GPIO Port K and Pin 6 - WIFI_RST */
    ////{ NvOdmIoModule_Vdd,  0x00, TPS6586xPmuSupply_LDO7 }   /* VDD -> HCREG */
//};

// Sdio
static const NvOdmIoAddress s_p852_b00_SdioAddresses[] =
{
    { NvOdmIoModule_Sdio, 0x0,  0x0 },                      /* SD Memory on SD Bus */
    { NvOdmIoModule_Sdio, 0x1,  0x0 },                      /* BT on SD Bus. NVTODO: Check this */
    { NvOdmIoModule_Sdio, 0x2,  0x0 },                      /* SD Memory on SD Bus */
    { NvOdmIoModule_Sdio, 0x3,  0x0 },                      /* P852 emmc */
    { NvOdmIoModule_Vdd,  0x00, TPS6586xPmuSupply_DCD2 },   /* VDD -> HCREG */
    { NvOdmIoModule_Vdd,  0x00, TPS6586xPmuSupply_DCD2 }    /* VDDIO_BB -> SDIO1 */
};

// VIP
#define ADV7180_PINS (NVODM_CAMERA_PARALLEL_VD0_TO_VD7)
static const NvOdmIoAddress s_p852_b00_ffaImagerADV7180Addresses[] =
{
    /* B00 changes : VDD_VI is derived from SM2
     * => Programming not needed
     */
    //{ NvOdmIoModule_Vdd,  0x00, TPS6586xPmuSupply_LDO7 },
    { NvOdmIoModule_VideoInput, 0x00, ADV7180_PINS },
    { NvOdmIoModule_ExternalClock, 2, 0 } // CSUS
};

static const NvOdmIoAddress s_p852_b00_CommonImagerAddresses[] =
{
    { NvOdmIoModule_ExternalClock, 2, 0 } // CSUS
};

//static const NvOdmIoAddress s_p852_b00_ffaVideoDacAddresses[] =
//{
//    { NvOdmIoModule_Tvo, 0x00, 0x00 },
    /* tvdac rail */
//    { NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO6 },    // AVDD_VDAC
//};
