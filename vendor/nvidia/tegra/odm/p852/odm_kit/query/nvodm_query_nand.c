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
 * @brief <b>NVIDIA Driver Development Kit:
 *                  ODM Uart interface</b>
 *
 * @b Description: Implements the ODM for the Uart communication.
 *
 */

#include "nvodm_query_nand.h"
#include "nvcommon.h"

// fill params for all required nand flashes here.
// this list will end when vendor id and chipd id will be zero.
// hence, all supported chips should be listed before that.
NvOdmNandFlashParams g_Params[] =
{
    /*
    {
        VendorId, DeviceId, NandType, IsCopyBackCommandSupported, IsCacheWriteSupported, CapacityInMB, ZonesPerDevice,
        BlocksPerZone, OperationSuccessStatus, InterleaveCapability, EccAlgorithm,
        ErrorsCorrectable, SkippedSpareBytes,
        TRP, TRH (TREH), TWP, TWH, TCS, TWHR, TWB, TREA, TADL,
        TCLS, TCLH, TCH, TALS, TALH, TRC, TWC, TCR(TCLR), TAR, TRR, NandDeviceType, ReadIdFourthByte,
	HaveTimingRegisterSetting, TimingRegister1, TimingRegister2
    }
    */
     // Micron 29F4G08ABADA
     {
        0x2C, 0xDC, NvOdmNandFlashType_Slc, NV_TRUE, NV_FALSE, 512, 2,
        2048, 0x60, SINGLE_PLANE, NvOdmNandECCAlgorithm_ReedSolomon,
        NvOdmNandNumberOfCorrectableSymbolErrors_Four, NvOdmNandSkipSpareBytes_4,
        1, 1, 12, 12, 24, 58, 116, 24, 24,
        10, 5, 5, 10, 5, 20, 20, 10, 10, 12, NvOdmNandDeviceType_Type1, 0x95,
     },

     // Micron 29F4G16ABADA
     {
        0x2C, 0xCC, NvOdmNandFlashType_Slc, NV_TRUE, NV_FALSE, 512, 2,
        2048, 0x60, SINGLE_PLANE, NvOdmNandECCAlgorithm_ReedSolomon,
        NvOdmNandNumberOfCorrectableSymbolErrors_Four, NvOdmNandSkipSpareBytes_4,
        1, 1, 12, 12, 24, 58, 116, 24, 24,
        10, 5, 5, 10, 5, 20, 20, 10, 10, 12, NvOdmNandDeviceType_Type1, 0xD5,
     },


     // Hynix HY27UF084G2B (readid 4th byte 0x95)
     {
        0xAD, 0xDC, NvOdmNandFlashType_Slc, NV_TRUE, NV_FALSE, 512, 2,
        2048, 0x60, SINGLE_PLANE, NvOdmNandECCAlgorithm_ReedSolomon,
        NvOdmNandNumberOfCorrectableSymbolErrors_Four, NvOdmNandSkipSpareBytes_4,
        12, 1, 12, 1, 24, 58, 116, 24, 24,
        12, 5, 5, 12, 5, 25, 25, 1, 1, 1, NvOdmNandDeviceType_Type1, 0x95,
     },
    /* "This is the end of device list please do not modify this. To add support for more flash parts,
        add device category for those parts before this element"*/
    {
        0, 0, NvOdmNandFlashType_UnKnown, NV_FALSE, NV_FALSE, 0, 0,
        0, 0, SINGLE_PLANE, NvOdmNandECCAlgorithm_ReedSolomon,
        NvOdmNandNumberOfCorrectableSymbolErrors_Four, NvOdmNandSkipSpareBytes_4,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NvOdmNandDeviceType_Type1, 0,
    }
};

NvOdmNandFlashParams *NvOdmNandGetFlashInfo (NvU32 ReadID)
{
    NvU8 TempValue;
    NvU8 VendorId = 0;
    NvU8 DeviceId = 0;
    NvU8 ReadIdFourthByte = 0;
    NvOdmNandFlashType NandType;
    NvU8 i = 0;
    // To extract Vendor Id
    VendorId = (NvU8) (ReadID & 0xFF);
    // To extract Device Id
    DeviceId = (NvU8) ((ReadID >> DEVICE_SHIFT) & 0xFF);
    // To extract Fourth ID byte of Read ID - for checking if the flash is 42nm.
    ReadIdFourthByte = (NvU8) ((ReadID >> FOURTH_ID_SHIFT) & 0xFF);
    // To extract device Type Mask
    TempValue = (NvU8) ((ReadID >> FLASH_TYPE_SHIFT) & 0xC);
    if (TempValue)
    {
        NandType = NvOdmNandFlashType_Mlc;
    }
    else
    {
        NandType = NvOdmNandFlashType_Slc;
    }
    // following ORing is done to check if we reached the end of the list.
    while ((g_Params[i].VendorId) | (g_Params[i].DeviceId))
    {
        if ((g_Params[i].VendorId == VendorId) &&
            (g_Params[i].DeviceId == DeviceId) &&
            (g_Params[i].ReadIdFourthByte == ReadIdFourthByte) &&
            (g_Params[i].NandType == NandType))
        {
            return &g_Params[i];
        }
        else
            i++;
    }
    // This condition will be reached if "g_Params" is not having Parameters of the flash used.
    // Hence add the parameters required in the table.
    return NULL;
}

