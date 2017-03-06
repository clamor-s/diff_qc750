/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvddk_fuse.h"
#include "nvassert.h"
#include "nvrm_drf.h"
#include "nvrm_hardware_access.h"
#include "arfuse_common.h"
#include "nvddk_fuse_bridge.h"
#include "t30/arfuse.h"
#include "t30/nvboot_bct.h"
#include "t30/nvddk_t30_fuse_priv.h"

#define UID_UID_0_CID_RANGE     63:60
#define UID_UID_0_VENDOR_RANGE  59:56
#define UID_UID_0_FAB_RANGE     55:50
#define UID_UID_0_LOT_RANGE     49:24
#define UID_UID_0_WAFER_RANGE   23:18
#define UID_UID_0_X_RANGE       17:9
#define UID_UID_0_Y_RANGE       8:0

#define BOOT_DEVICE_CONFIG_INSTANCE_SHIFT 6

#define NVDDK_SECURE_BOOT_KEY_BYTES  (16)
#define NVDDK_RESERVED_ODM_BYTES  (32)
#define NVDDK_UID_BYTES (8)

// Size of the data items, in bytes
NvU32 s_DataSize[] =
{
    0, // FuseDataType_None
    sizeof(NvU32),  // FuseDataType_DeviceKey
    sizeof(NvBool), // FuseDataType_JtagDisable
    sizeof(NvBool), // FuseDataType_KeyProgrammed
    sizeof(NvBool), // FuseDataType_OdmProductionEnable
    sizeof(NvU32), // NvDdkFuseDataType_SecBootDeviceConfig
    NVDDK_SECURE_BOOT_KEY_BYTES, // FuseDataType_SecureBootKey
    sizeof(NvU32),  // FuseDataType_Sku
    sizeof(NvU32),  // FuseDataType_SpareBits
    sizeof(NvU32),  // FuseDataType_SwReserved
    sizeof(NvBool), // FuseDataType_SkipDevSelStraps
    sizeof(NvU32),  // FuseDataType_SecBootDeviceSelectRaw
    NVDDK_RESERVED_ODM_BYTES,  // NvDdkFuseDataType_ReservedOdm fuse
    sizeof(NvU32),  // NvDdkFuseDataType_PackageInfo
    NVDDK_UID_BYTES, // FuseDataType_Uid
    sizeof(NvU32),  // FuseDataType_SecBootDeviceSelect
    sizeof(NvU32),   // NvDdkFuseDataType_OpMode
    sizeof(NvU32)   // NvDdkFuseDataType_SecBootDevInst
};

static NvBootDevType MapFuseDevToBootDevType[] =
{
    NvBootDevType_Sdmmc,         // NvBootFuseBootDevice_Sdmmc
    NvBootDevType_Snor,          // NvBootFuseBootDevice_SnorFlash
    NvBootDevType_Spi,           // NvBootFuseBootDevice_SpiFlash
    NvBootDevType_Nand,          // NvBootFuseBootDevice_NandFlash
    NvBootDevType_MobileLbaNand, // NvBootFuseBootDevice_MobileLbaNand
    NvBootDevType_MuxOneNand,    // NvBootFuseBootDevice_MuxOneNand
    NvBootDevType_Sata,           // NvBootFuseBootDevice_Sata
    NvBootDevType_Sdmmc3,        // NvBootFuseBootDevice_BootRom_Reserved_Sdmmc3
};

static void T30GetUniqueId(NvU64 *pUniqueId)
{
    NvU64   Uid;            // Unique ID
    NvU32   Vendor;         // Vendor
    NvU32   Fab;            // Fab
    NvU32   Lot;            // Lot
    NvU32   Wafer;          // Wafer
    NvU32   X;              // X-coordinate
    NvU32   Y;              // Y-coordinate
    NvU32   Cid;            // Chip id
    NvU32   Reg;            // Scratch register
    NvU32   i;

    NV_ASSERT(pUniqueId != NULL);
    Cid = 0;
    // Vendor
    Reg = NV_READ32(NV_ADDRESS_MAP_FUSE_BASE + FUSE_OPT_VENDOR_CODE_0);
    Vendor = NV_DRF_VAL(FUSE, OPT_VENDOR_CODE, OPT_VENDOR_CODE, Reg);

    // Fab
    Reg = NV_READ32(NV_ADDRESS_MAP_FUSE_BASE + FUSE_OPT_FAB_CODE_0);
    Fab = NV_DRF_VAL(FUSE, OPT_FAB_CODE, OPT_FAB_CODE, Reg);

    // Lot code must be re-encoded from a 5 digit base-36 'BCD' number
    // to a binary number.
    Lot = 0;
    Reg = NV_READ32(NV_ADDRESS_MAP_FUSE_BASE + FUSE_OPT_LOT_CODE_0_0);
    Reg <<= 2;  // Two unused bits (5 6-bit 'digits' == 30 bits)
    for (i = 0; i < 5; ++i)
    {
        NvU32 Digit;

        Digit = (Reg & 0xFC000000) >> 26;
        NV_ASSERT(Digit < 36);
        Lot *= 36;
        Lot += Digit;
        Reg <<= 6;
    }

    // Wafer
    Reg = NV_READ32(NV_ADDRESS_MAP_FUSE_BASE + FUSE_OPT_WAFER_ID_0);
    Wafer = NV_DRF_VAL(FUSE, OPT_WAFER_ID, OPT_WAFER_ID, Reg);

    // X-coordinate
    Reg = NV_READ32(NV_ADDRESS_MAP_FUSE_BASE + FUSE_OPT_X_COORDINATE_0);
    X = NV_DRF_VAL(FUSE, OPT_X_COORDINATE, OPT_X_COORDINATE, Reg);

    // Y-coordinate
    Reg = NV_READ32(NV_ADDRESS_MAP_FUSE_BASE + FUSE_OPT_Y_COORDINATE_0);
    Y = NV_DRF_VAL(FUSE, OPT_Y_COORDINATE, OPT_Y_COORDINATE, Reg);

    Uid = NV_DRF64_NUM(UID, UID, CID, Cid)
        | NV_DRF64_NUM(UID, UID, VENDOR, Vendor)
        | NV_DRF64_NUM(UID, UID, FAB, Fab)
        | NV_DRF64_NUM(UID, UID, LOT, Lot)
        | NV_DRF64_NUM(UID, UID, WAFER, Wafer)
        | NV_DRF64_NUM(UID, UID, X, X)
        | NV_DRF64_NUM(UID, UID, Y, Y);

    *pUniqueId = Uid;
}

static NvU32 T30GetFuseToDdkDevMap(NvU32 RegData)
{
    if (RegData >= (int)((sizeof(MapFuseDevToBootDevType)) / sizeof(NvBootDevType)))
        return  -1;
    return MapFuseDevToBootDevType[RegData];
}

static NvU32 T30GetSecBootDevInstance(NvU32 BootDev, NvU32 BootDevConfig)
{
    if (NvBootDevType_Sdmmc == BootDev)
    {
        // Please refer to T30 bootrom wiki for the below logic
        if (BootDevConfig & (1 << BOOT_DEVICE_CONFIG_INSTANCE_SHIFT))
            return SDMMC_CONTROLLER_INSTANCE_2;
        else
            return SDMMC_CONTROLLER_INSTANCE_3;
    }
    else if (NvBootDevType_Spi == BootDev)
    {
        return 3;
    }
    // defualt
    return 0;
}

static NvU32 T30GetArrayLength(void)
{
    return ((sizeof(s_DataSize)) / (sizeof(NvU32)));
}

static NvU32 T30GetTypeSize(NvDdkFuseDataType Type)
{
    NV_ASSERT(Type < NvDdkFuseDataType_Num);
    return s_DataSize[(NvU32)Type];
}

void MapT30ChipProperties(FuseCore *pFusecore)
{
        if (!pFusecore) return;

            pFusecore->pGetArrayLength        = T30GetArrayLength;
            pFusecore->pGetTypeSize           = T30GetTypeSize;
            pFusecore->pGetSecBootDevInstance = T30GetSecBootDevInstance;
            pFusecore->pGetFuseToDdkDevMap    = T30GetFuseToDdkDevMap;
            pFusecore->pGetUniqueId           = T30GetUniqueId;
}
