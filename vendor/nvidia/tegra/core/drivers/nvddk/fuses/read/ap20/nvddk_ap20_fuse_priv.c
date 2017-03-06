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
#include "nvrm_hardware_access.h"
#include "nvrm_drf.h"
#include "arfuse_common.h"
#include "nvddk_fuse_bridge.h"
#include "ap20/arfuse.h"
#include "ap20/nvboot_bct.h"
#include "ap20/nvddk_ap20_fuse_priv.h"

#define NVDDK_SECURE_BOOT_KEY_BYTES  (16)
#define NVDDK_RESERVED_ODM_BYTES  (32)
#define NVDDK_UID_BYTES (8)

// Size of the data items, in bytes
static NvU32 s_DataSize[] =
{
    0, // FuseDataType_None
    sizeof(NvU32),  // FuseDataType_DeviceKey
    sizeof(NvBool), // FuseDataType_JtagDisable
    sizeof(NvBool), // FuseDataType_KeyProgrammed
    sizeof(NvBool), // FuseDataType_OdmProductionEnable
    sizeof(NvU32),  // NvDdkFuseDataType_SecBootDeviceConfig
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
    sizeof(NvU32), // NvDdkFuseDataType_OpMode
    sizeof(NvU32) // NvDdkFuseDataType_SecBootDevInst
};

static NvBootDevType MapFuseDevToBootDevType[] =
{
    NvBootDevType_Sdmmc,
    NvBootDevType_Snor,
    NvBootDevType_Spi,
    NvBootDevType_Nand,
    NvBootDevType_MobileLbaNand,
    NvBootDevType_MuxOneNand,
};

static void Ap20GetUniqueId(NvU64 *pUniqueId)
{
     NvU32 Uid0; // Lower 32 bits of the UID
     NvU32 Uid1; // Upper 32 bits of the UID

     NV_ASSERT(pUniqueId != NULL);
     Uid0 = NV_READ32(NV_ADDRESS_MAP_FUSE_BASE + FUSE_JTAG_SECUREID_0_0);
     Uid1 = NV_READ32(NV_ADDRESS_MAP_FUSE_BASE + FUSE_JTAG_SECUREID_1_0);
     *pUniqueId = ((NvU64)Uid0) | ((NvU64)Uid1 << 32);
}

static NvU32 Ap20GetFuseToDdkDevMap(NvU32 RegData)
{
    if (RegData >= (int)((sizeof(MapFuseDevToBootDevType)) / sizeof(NvBootDevType)))
        return  -1;
    return MapFuseDevToBootDevType[RegData];
}

static NvU32 Ap20GetSecBootDevInstance(NvU32 BootDev, NvU32 BoodDevConfig)
{
    if (NvBootDevType_Sdmmc == BootDev)
    {
        return SDMMC_CONTROLLER_INSTANCE_3;
    }
    return 0;
}

static NvU32 Ap20GetArrayLength(void)
{
    return ((sizeof(s_DataSize)) / (sizeof(NvU32)));
}

static NvU32 Ap20GetTypeSize(NvDdkFuseDataType Type)
{
    NV_ASSERT(Type < NvDdkFuseDataType_Num);
    return s_DataSize[(NvU32)Type];
}

void MapAp20ChipProperties(FuseCore *pFusecore)
{
    if (!pFusecore)
        return;

    pFusecore->pGetArrayLength        = Ap20GetArrayLength;
    pFusecore->pGetTypeSize           = Ap20GetTypeSize;
    pFusecore->pGetSecBootDevInstance = Ap20GetSecBootDevInstance;
    pFusecore->pGetFuseToDdkDevMap    = Ap20GetFuseToDdkDevMap;
    pFusecore->pGetUniqueId           = Ap20GetUniqueId;
}
