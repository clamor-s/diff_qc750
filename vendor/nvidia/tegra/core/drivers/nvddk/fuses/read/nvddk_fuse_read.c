/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvassert.h"
#include "nvrm_drf.h"
#include "nvrm_hardware_access.h"
#include "nvddk_operatingmodes.h"
#include "nvddk_fuse.h"
#include "arfuse_common.h"
#include "nvddk_fuse_bridge.h"
#include "ap20/nvddk_ap20_fuse_priv.h"
#include "t30/nvddk_t30_fuse_priv.h"

#define FUSE_NV_READ32(offset)\
        NV_READ32(NV_ADDRESS_MAP_APB_FUSE_BASE + offset)

#define FUSE_NV_WRITE32(offset, val)\
        NV_WRITE32(NV_ADDRESS_MAP_APB_FUSE_BASE + offset, val)

#define CLOCK_NV_READ32(offset)\
        NV_READ32(NV_ADDRESS_MAP_CAR_BASE + offset)

#define CLOCK_NV_WRITE32(offset, val)\
        NV_WRITE32(NV_ADDRESS_MAP_CAR_BASE + offset, val)

#define EXTRACT_BYTE_NVU32(Data32, ByteNum) \
    (((Data32) >> ((ByteNum)*8)) & 0xFF)

#define SWAP_BYTES_NVU32(Data32)                    \
    do {                                            \
        NV_ASSERT(sizeof(Data32)==4);               \
        (Data32) =                                  \
            (EXTRACT_BYTE_NVU32(Data32, 0) << 24) | \
            (EXTRACT_BYTE_NVU32(Data32, 1) << 16) | \
            (EXTRACT_BYTE_NVU32(Data32, 2) <<  8) | \
            (EXTRACT_BYTE_NVU32(Data32, 3) <<  0);  \
    } while (0)

#define NVDDK_DEVICE_KEY_BYTES       (4)
#define NVDDK_SECURE_BOOT_KEY_BYTES  (16)
#define NVDDK_RESERVED_ODM_BYTES  (32)
#define NVBOOT_DEV_CONFIG_0_CONF0_RANGE 5:0

#define FUSE_BOOT_DEVICE_INFO_0_BOOT_DEVICE_CONFIG_BITS_0_RANGE 5:0
#define FUSE_RESERVED_SW_0_BOOT_DEVICE_CONFIG_BITS_1_RANGE      5:2

#define AR_APB_MISC_BASE_ADDRESS 0x70000000
#define FUSE_PACKAGE_INFO_0_PACKAGE_INFO_RANGE                  1:0
#define NV_TEGRA_CHIPID_AP20 0x20
#define NV_TEGRA_CHIPID_T30  0x30

FuseCore *p_gsFuseCore = NULL;

static NvBool NvDdkFuseIsFailureAnalysisMode(void)
{
    volatile NvU32 RegValue;
    RegValue = FUSE_NV_READ32(FUSE_FA_0);
    if (RegValue)
    {
        return NV_TRUE;
    }
    else
    {
        return NV_FALSE;
    }
}

static NvError MapChipProperties(void)
{
    NvU32 RegData;
    NvU32 ChipId;

    if (p_gsFuseCore) return NvSuccess;

    p_gsFuseCore =  NvOsAlloc(sizeof(FuseCore));
    if (p_gsFuseCore == NULL)
        return NvError_InsufficientMemory;

    RegData = NV_READ32(AR_APB_MISC_BASE_ADDRESS + APB_MISC_GP_HIDREV_0);
    ChipId = NV_DRF_VAL(APB_MISC, GP_HIDREV, CHIPID, RegData);
    switch (ChipId)
    {
        case 0x20:
            MapAp20ChipProperties(p_gsFuseCore);
            break;
        case 0x30:
            MapT30ChipProperties(p_gsFuseCore);
            break;
        default:
            return NvError_NotSupported;
    }
    return NvSuccess;
}

#if ADDITIONAL_FUNCTIONALITY
static void FuseCopyBytes(NvU32 RegAddress, NvU8 *pByte, const NvU32 nBytes)
{
    NvU32 RegData;
    NvU32 i;

    NV_ASSERT((pByte != NULL) || (nBytes == 0));
    NV_ASSERT (RegAddress != 0);

    for (i = 0, RegData = 0; i < nBytes; i++)
    {
        if ((i&3) == 0)
        {
            RegData = FUSE_NV_READ32(RegAddress);
            RegAddress += 4;
        }
        pByte[i] = RegData & 0xFF;
        RegData >>= 8;
    }
}
#endif

static NvBool NvDdkFuseIsSbkSet(void)
{
    NvU32 AllKeysOred;

    AllKeysOred  = FUSE_NV_READ32(FUSE_PRIVATE_KEY0_NONZERO_0);
    AllKeysOred |= FUSE_NV_READ32(FUSE_PRIVATE_KEY1_NONZERO_0);
    AllKeysOred |= FUSE_NV_READ32(FUSE_PRIVATE_KEY2_NONZERO_0);
    AllKeysOred |= FUSE_NV_READ32(FUSE_PRIVATE_KEY3_NONZERO_0);
    if (AllKeysOred)
        return NV_TRUE;
    else
        return NV_FALSE;
}

/**
 * Reports whether the ODM Production Mode fuse is set (burned)
 *
 * Note that this fuse by itself does not determine whether the chip is in
 * ODM Production Mode.
 *
 * @param none
 *
 * @return NV_TRUE if ODM Production Mode fuse is set (burned); else NV_FALSE
 */
static NvBool NvDdkFuseIsOdmProductionModeFuseSet(void)
{
    NvU32 RegValue;

    RegValue = FUSE_NV_READ32(FUSE_SECURITY_MODE_0);
    if (RegValue)
    {
        return NV_TRUE;
    }
    else
    {
        return NV_FALSE;
    }
}

static NvBool NvDdkFuseIsNvProductionModeFuseSet(void)
{
    NvU32 RegValue;
    RegValue = FUSE_NV_READ32(FUSE_PRODUCTION_MODE_0);
    if (RegValue)
    {
        return NV_TRUE;
    }
    return NV_FALSE;
}

static void NvDdkFuseGetOpMode(NvDdkFuseOperatingMode *pMode, NvBool isSbkBurned)
{
    if (NvDdkFuseIsFailureAnalysisMode())
    {
        *pMode = NvDdkFuseOperatingMode_FailureAnalysis;
    }
    else
    {
        if (NvDdkFuseIsOdmProductionModeFuseSet())
        {
            if (isSbkBurned)
            {
                 *pMode = NvDdkFuseOperatingMode_OdmProductionSecure;
            }
            else
            {
                *pMode = NvDdkFuseOperatingMode_OdmProductionOpen;
            }
        }
        else
        {
            if ( NvDdkFuseIsNvProductionModeFuseSet() )
            {
                *pMode = NvDdkFuseOperatingMode_NvProduction;
            }
            else
            {
                *pMode = NvDdkFuseOperatingMode_Preproduction;
            }
        }
    }
}

static NvU32 NvDdkFuseGetSecBootDeviceRaw(void)
{
    NvU32 RegData;

    RegData = FUSE_NV_READ32(FUSE_RESERVED_SW_0);
    RegData = NV_DRF_VAL(FUSE, RESERVED_SW, BOOT_DEVICE_SELECT, RegData);
    return RegData;
}

static NvU32 NvDdkFuseGetSecBootDevice(void)
{
    NvU32 RegData;

    RegData = NvDdkFuseGetSecBootDeviceRaw();
    return p_gsFuseCore->pGetFuseToDdkDevMap(RegData);
}

/* Expose (Visibility = 1) or hide (Visibility = 0) the fuse registers.
*/
static void SetFuseRegVisibility(NvU32 Visibility)
{
    NvU32 RegData;

    RegData = CLOCK_NV_READ32(CLK_RST_CONTROLLER_MISC_CLK_ENB_0);
    RegData = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER,
                                 MISC_CLK_ENB,
                                 CFG_ALL_VISIBLE,
                                 Visibility,
                                 RegData);
    CLOCK_NV_WRITE32(CLK_RST_CONTROLLER_MISC_CLK_ENB_0, RegData);
}

#if ADDITIONAL_FUNCTIONALITY
/**
 * Reports whether any of the SBK or DK fuses are set (burned)
 *
 * @params none
 *
 * @return NV_TRUE if the SBK or the DK is non-zero
 * @return NV_FALSE otherwise
 */
static NvBool IsSbkOrDkSet(void)
{
    NvU32 AllKeysOred;

    AllKeysOred  = NvDdkFuseIsSbkSet();
    AllKeysOred |= FUSE_NV_READ32(FUSE_PRIVATE_KEY4_NONZERO_0);

    if (AllKeysOred)
        return NV_TRUE;
    else
        return NV_FALSE;
}
#endif

static NvU32 GetBootDevInfo(void)
{
    NvU32 RegData;
    NvU32 Config;

    RegData = NV_READ32(NV_ADDRESS_MAP_FUSE_BASE + FUSE_BOOT_DEVICE_INFO_0);
    Config = NV_DRF_VAL(FUSE,
            BOOT_DEVICE_INFO,
            BOOT_DEVICE_CONFIG_BITS_0,
            RegData);

    return Config;
}

/**
 * This gets value from the fuse cache.
 *
 * To read from the actual fuses, NvDdkFuseSense() must be called first.
 * Note that NvDdkFuseSet() follwed by NvDdkFuseGet() for the same data will
 * return the set data, not the actual fuse values.
 *
 * By passing a size of zero, the caller is requesting tfor the
 * expected size.
 */
NvError NvDdkFuseGet(NvDdkFuseDataType Type, void *pData, NvU32 *pSize)
{
    NvU32 RegData;
    NvU32 Size;
    NvU32 DataSizeArrayLen;
    NvU32 Config;
    NvU32 BootDev;
    NvError e;

    if (p_gsFuseCore == NULL) {
        e = MapChipProperties();
        if (e != NvSuccess)
            return e;
    }

    DataSizeArrayLen = p_gsFuseCore->pGetArrayLength();

    // Check the arguments
    // NvDdkFuseDataType_Num is not ap20 specific as s_DataSize
    if (Type == NvDdkFuseDataType_None ||
        (Type > (NvDdkFuseDataType)DataSizeArrayLen))
            return NvError_BadValue;
    if (pSize == NULL) return NvError_BadParameter;

    Size = p_gsFuseCore->pGetTypeSize(Type);
    if (*pSize == 0)
    {
        *pSize = Size;
        return NvError_Success;
    }

    if (*pSize < Size) return NvError_InsufficientMemory;
    if (pData == NULL) return NvError_BadParameter;

    switch (Type)
    {
#if ADDITIONAL_FUNCTIONALITY
        case NvDdkFuseDataType_DeviceKey:
            /*
             * Boot ROM expects DK to be stored in big-endian byte order;
             * since cpu is little-endian and client treats data as an NvU32,
             * perform byte swapping here
             */
            RegData = FUSE_NV_READ32(FUSE_PRIVATE_KEY4_0);
            SWAP_BYTES_NVU32(RegData);
            *((NvU32*)pData) = RegData;
            break;

        case NvDdkFuseDataType_JtagDisable:
            RegData = FUSE_NV_READ32(FUSE_ARM_DEBUG_DIS_0);
            *((NvBool*)pData) = RegData ? NV_TRUE : NV_FALSE;
            break;

        case NvDdkFuseDataType_KeyProgrammed:
            *((NvBool*)pData) = IsSbkOrDkSet();
            break;

        case NvDdkFuseDataType_OdmProduction:
            *((NvBool*)pData) = NvDdkFuseIsOdmProductionModeFuseSet();
            break;

        case NvDdkFuseDataType_SecureBootKey:
            FuseCopyBytes(FUSE_PRIVATE_KEY0_0,
                          pData,
                          NVDDK_SECURE_BOOT_KEY_BYTES);
            break;

        case NvDdkFuseDataType_SwReserved:
            RegData = FUSE_NV_READ32(FUSE_RESERVED_SW_0);
            RegData = NV_DRF_VAL(FUSE, RESERVED_SW, SW_RESERVED, RegData);
            *((NvU32*)pData) = RegData;
            break;

        case NvDdkFuseDataType_SkipDevSelStraps:
            RegData = FUSE_NV_READ32(FUSE_RESERVED_SW_0);
            RegData = NV_DRF_VAL(FUSE, RESERVED_SW, SKIP_DEV_SEL_STRAPS, RegData);
            *((NvU32*)pData) = RegData;
            break;

        case NvDdkFuseDataType_ReservedOdm:
            FuseCopyBytes(FUSE_RESERVED_ODM0_0,
                          pData,
                          NVDDK_RESERVED_ODM_BYTES);
            break;

        case NvDdkFuseDataType_PackageInfo:
            RegData = FUSE_NV_READ32(FUSE_PACKAGE_INFO_0);
            RegData = NV_DRF_VAL(FUSE, PACKAGE_INFO, PACKAGE_INFO, RegData);
            *((NvU32*)pData) = RegData;
            break;
#endif
       case NvDdkFuseDataType_Sku:
            RegData = FUSE_NV_READ32(FUSE_SKU_INFO_0);
            RegData = NV_DRF_VAL(FUSE, SKU_INFO, SKU_INFO, RegData);
            *((NvU32*)pData) = RegData;
            break;

	case NvDdkFuseDataType_SecBootDeviceSelect:
            RegData = NvDdkFuseGetSecBootDevice();
            if ((NvS32)RegData < 0)
                return NvError_InvalidState;
            *((NvU32*)pData) = RegData;
            break;

        case NvDdkFuseDataType_SecBootDeviceSelectRaw:
            RegData = NvDdkFuseGetSecBootDeviceRaw();
            *((NvU32*)pData) = RegData;
            break;

        case NvDdkFuseDataType_Uid:
            SetFuseRegVisibility(1);
            p_gsFuseCore->pGetUniqueId((NvU64*)pData);
            SetFuseRegVisibility(0);
            break;
        case  NvDdkFuseDataType_SecBootDeviceConfig:
            *((NvU32*)pData) = GetBootDevInfo();
            break;
        case NvDdkFuseDataType_OpMode:
            NvDdkFuseGetOpMode((NvDdkFuseOperatingMode *)pData, NvDdkFuseIsSbkSet());
            break;
        case NvDdkFuseDataType_SecBootDevInst:
            BootDev = NvDdkFuseGetSecBootDevice();
            if ((NvS32)BootDev < 0)
                return NvError_InvalidState;
            Config = GetBootDevInfo();
            RegData = p_gsFuseCore->pGetSecBootDevInstance(BootDev, Config);
            if ((NvS32)RegData < 0)
                return NvError_InvalidState;
            *((NvU32 *)pData) = RegData;
            break;
        default:
            return(NvError_BadValue);
    }
    return NvError_Success;
}
