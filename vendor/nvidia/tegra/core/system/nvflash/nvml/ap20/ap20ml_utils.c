/*
 * Copyright (c) 2007 - 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvml_utils.h"
#include "nvml_device.h"
#include "nvboot_sdmmc.h"
#include "nvboot_snor.h"
#include "ap20/arfuse.h"
#include "nvml_aes.h"
#include "nvml_hash.h"
#if AES_KEYSCHED_LOCK_WAR_BUG_598910
#include "aes_keyschedule_lock.h"
#endif

#define MISC_PA_BASE 0x70000000UL
#define MISC_PA_LEN  4096

#define FUSE_RESERVED_SW_0_BOOT_DEVICE_SELECT_RANGE       2:0
#define FUSE_RESERVED_SW_0_SKIP_DEV_SEL_STRAPS_RANGE      3:3
#define FUSE_RESERVED_SW_0_SW_RESERVED_RANGE              7:4

extern NvBctHandle BctDeviceHandle;

/*
 * NvStrapDevSel: Enumerated list of devices selectable via straps.
 */
typedef enum
{
    NvStrapDevSel_Emmc_Primary_x4 = 0, /* eMMC primary (x4)          */
    NvStrapDevSel_Emmc_Primary_x8,     /* eMMC primary (x8)          */
    NvStrapDevSel_Emmc_Secondary_x4,   /* eMMC secondary (x4)        */
    NvStrapDevSel_Nand,                /* NAND (x8 or x16)           */
    NvStrapDevSel_Nand_42nm,           /* NAND (x8 or x16)           */
    NvStrapDevSel_MobileLbaNand,       /* mobileLBA NAND             */
    NvStrapDevSel_MuxOneNand,          /* MuxOneNAND                 */
    NvStrapDevSel_Esd_x4,              /* eSD (x4)                   */
    NvStrapDevSel_SpiFlash,            /* SPI Flash                  */
    NvStrapDevSel_Snor_Muxed_x16,      /* Sync NOR (Muxed, x16)      */
    NvStrapDevSel_Snor_Muxed_x32,      /* Sync NOR (Muxed, x32)      */
    NvStrapDevSel_Snor_NonMuxed_x16,   /* Sync NOR (NonMuxed, x16)   */
    NvStrapDevSel_FlexMuxOneNand,      /* FlexMuxOneNAND             */
    NvStrapDevSel_Reserved_0xd,        /* Reserved (0xd)             */
    NvStrapDevSel_Reserved_0xe,        /* Reserved (0xe)             */
    NvStrapDevSel_UseFuses,            /* Use fuses instead          */

    /* The following definitions must be last. */
    NvStrapDevSel_Num, /* Must appear after the last legal item */
    NvStrapDevSel_Force32 = 0x7FFFFFFF
} NvStrapDevSel;

static NvBootFuseBootDevice StrapDevSelMap[] =
{
    NvBootFuseBootDevice_Sdmmc,         // NvStrapDevSel_Emmc_Primary_x4
    NvBootFuseBootDevice_Sdmmc,         // NvStrapDevSel_Emmc_Primary_x8
    NvBootFuseBootDevice_Sdmmc,         // NvStrapDevSel_Emmc_Secondary_x4
    NvBootFuseBootDevice_NandFlash,     // NvStrapDevSel_Nand
    NvBootFuseBootDevice_NandFlash,     // NvStrapDevSel_Nand_42nm
    NvBootFuseBootDevice_MobileLbaNand, // NvStrapDevSel_MobileLbaNand
    NvBootFuseBootDevice_MuxOneNand,    // NvStrapDevSel_MuxOneNand
    NvBootFuseBootDevice_Sdmmc,         // NvStrapDevSel_Esd_x4
    NvBootFuseBootDevice_SpiFlash,      // NvStrapDevSel_SpiFlash
    NvBootFuseBootDevice_SnorFlash,     // NvStrapDevSel_Snor_Muxed_x16
    NvBootFuseBootDevice_SnorFlash,     // NvStrapDevSel_Snor_Muxed_x32
    NvBootFuseBootDevice_SnorFlash,     // NvStrapDevSel_Snor_NonMuxed_x16
    NvBootFuseBootDevice_MuxOneNand,    // NvStrapDevSel_FlexMuxOneNand
    NvBootFuseBootDevice_Sdmmc,         // NvStrapDevSel_Reserved_0xd
    NvBootFuseBootDevice_Sdmmc,         // NvStrapDevSel_Reserved_0xe
    NvBootFuseBootDevice_Sdmmc,         // NvStrapDevSel_UseFuses
};

#define NAND_CONFIG(PageOffset, BlockOffset) \
    (((PageOffset) << 8) | ((BlockOffset) << 10))

#define SDMMC_CONFIG(BusWidth, SdMmcSel, PinMux) \
    (((BusWidth) << 0) | ((SdMmcSel) << 1) | ((PinMux) << 2))

#define SNOR_CONFIG(NonMuxed, BusWidth) \
    (((BusWidth) << 0) | ((NonMuxed) << 1))

static NvU32 StrapDevConfigMap[] =
{
    SDMMC_CONFIG(0, 0, 0), // NvBootStrapDevSel_Emmc_Primary_x4
    SDMMC_CONFIG(1, 0, 0), // NvBootStrapDevSel_Emmc_Primary_x8
    SDMMC_CONFIG(0, 0, 1), // NvBootStrapDevSel_Emmc_Secondary_x4
    NAND_CONFIG(0, 0),     // NvBootStrapDevSel_Nand
    NAND_CONFIG(1, 1),     // NvBootStrapDevSel_Nand_42nm
    0, // No config data   // NvBootStrapDevSel_MobileLbaNand
    0, // MuxOneNand       // NvBootStrapDevSel_MuxOneNand
    SDMMC_CONFIG(0, 1, 0), // NvBootStrapDevSel_Esd_x4
    0, // No config data   // NvBootStrapDevSel_SpiFlash
    SNOR_CONFIG(0, 0),     // NvBootStrapDevSel_Snor_Muxed_x16
    SNOR_CONFIG(0, 1),     // NvBootStrapDevSel_Snor_Muxed_x32
    SNOR_CONFIG(1, 0),     // NvBootStrapDevSel_Snor_NonMuxed_x16
    1, // FlexMuxOneNand   // NvBootStrapDevSel_FlexMuxOneNand
    0, // No config data   // NvBootStrapDevSel_Reserved_0xd
    0, // No config data   // NvBootStrapDevSel_Reserved_0xe
    0 // No config data   // NvBootStrapDevSel_UseFuses
};

static NvError
NvMlUtilsConvertBootFuseDeviceToBootDevice(
    NvBootFuseBootDevice NvBootFuse,
    NvBootDevType *pBootDevice);

NvMlDevMgrCallbacks s_DeviceCallbacks[] =
{
    {
        /* Callbacks for the default (unset) device */
        (NvBootDeviceGetParams)NULL,
        (NvBootDeviceValidateParams)NULL,
        (NvBootDeviceGetBlockSizes)NULL,
        (NvBootDeviceInit)NULL,
        NULL,
        NULL,
        NULL,
    },
    {
        /* Callbacks for the NAND x8 device */
        (NvBootDeviceGetParams)NvBootNandGetParams,
        (NvBootDeviceValidateParams)NvBootNandValidateParams,
        (NvBootDeviceGetBlockSizes)NvBootNandGetBlockSizes,
        (NvBootDeviceInit)NvBootNandInit,
        NvBootNandReadPage,
        NvBootNandQueryStatus,
        NvBootNandShutdown,
    },
    {
        /* Callbacks for the SNOR device */
        (NvBootDeviceGetParams)NvBootSnorGetParams,
        (NvBootDeviceValidateParams)NvBootSnorValidateParams,
        (NvBootDeviceGetBlockSizes)NvBootSnorGetBlockSizes,
        (NvBootDeviceInit)NvBootSnorInit,
        NvBootSnorReadPage,
        NvBootSnorQueryStatus,
        NvBootSnorShutdown,
    },
    {
        /* Callbacks for the SPI Flash device */
        (NvBootDeviceGetParams)NvBootSpiFlashGetParams,
        (NvBootDeviceValidateParams)NvBootSpiFlashValidateParams,
        (NvBootDeviceGetBlockSizes)NvBootSpiFlashGetBlockSizes,
        (NvBootDeviceInit)NvBootSpiFlashInit,
        NvBootSpiFlashReadPage,
        NvBootSpiFlashQueryStatus,
        NvBootSpiFlashShutdown,
    },
    {
        /* Callbacks for the SDMMC  device */
        (NvBootDeviceGetParams)NvBootSdmmcGetParams,
        (NvBootDeviceValidateParams)NvBootSdmmcValidateParams,
        (NvBootDeviceGetBlockSizes)NvBootSdmmcGetBlockSizes,
        (NvBootDeviceInit)NvBootSdmmcInit,
        NvBootSdmmcReadPage,
        NvBootSdmmcQueryStatus,
        NvBootSdmmcShutdown,
    },
    {
        /* Callbacks for the Irom device */
        (NvBootDeviceGetParams)NULL,
        (NvBootDeviceValidateParams)NULL,
        (NvBootDeviceGetBlockSizes)NULL,
        (NvBootDeviceInit)NULL,
        NULL,
        NULL,
        NULL,
    },
    {
        /* Callbacks for the Uart device */
        (NvBootDeviceGetParams)NULL,
        (NvBootDeviceValidateParams)NULL,
        (NvBootDeviceGetBlockSizes)NULL,
        (NvBootDeviceInit)NULL,
        NULL,
        NULL,
        NULL,
    },
    {
        /* Callbacks for the Usb device */
        (NvBootDeviceGetParams)NULL,
        (NvBootDeviceValidateParams)NULL,
        (NvBootDeviceGetBlockSizes)NULL,
        (NvBootDeviceInit)NULL,
        NULL,
        NULL,
        NULL,
    },
    {
        /* Callbacks for the NAND x16 device */
        (NvBootDeviceGetParams)NvBootNandGetParams,
        (NvBootDeviceValidateParams)NvBootNandValidateParams,
        (NvBootDeviceGetBlockSizes)NvBootNandGetBlockSizes,
        (NvBootDeviceInit)NvBootNandInit,
        NvBootNandReadPage,
        NvBootNandQueryStatus,
        NvBootNandShutdown,
    },
    {
        /* Callbacks for the MuxOnenand device */
        (NvBootDeviceGetParams)NULL,
        (NvBootDeviceValidateParams)NULL,
        (NvBootDeviceGetBlockSizes)NULL,
        (NvBootDeviceInit)NULL,
        NULL,
        NULL,
        NULL,
    },
    {
        /* Callbacks for the MobileLbaNand device */
        (NvBootDeviceGetParams)NULL,
        (NvBootDeviceValidateParams)NULL,
        (NvBootDeviceGetBlockSizes)NULL,
        (NvBootDeviceInit)NULL,
        NULL,
        NULL,
        NULL,
    },
};

NvBootError NvMlUtilsSetupBootDevice(void *Context)
{
    NvU32                ConfigIndex;
    NvBootFuseBootDevice FuseDev;
    NvMlContext* pContext = (NvMlContext*)Context;
    NvBootDevType BootDevType = NvBootDevType_None;
    NvBootError e = NvBootError_Success;

    /* Query device info from fuses. */
    NvMlGetBootDeviceType(&FuseDev);
    NvMlGetBootDeviceConfig(&ConfigIndex);
    NvMlUtilsConvertBootFuseDeviceToBootDevice(FuseDev,&BootDevType);
    /* Set pin muxing appropriate to the boot device */
    ///TODO: Need to pass right valuse for secong argument(config)
    NvBootPadsConfigForBootDevice(FuseDev, 0);

    /* Initialize the device manager. */
    e = NvMlDevMgrInit(&(pContext->DevMgr),
                           BootDevType,
                          ConfigIndex);

    return e;
}
void
NvMlDoHashBlocks (
    NvBootAesEngine engine,
    NvBootAesIv *pK1,
    NvU8 *pData,
    NvBootHash *pHash,
    NvU32 numBlocks,
    NvBool startMsg,
    NvBool endMsg)
{
    // This implementation is not intended to be used when
    // hashing is not enabled. If hashing is not enabled in the
    // AES engine, hashing should be performed on 1 block at
    // a time.

    // Check if the engine is idle before performing an operation
    // Checking before would ensure that hashing operations are
    // non blocking AFAP.
    while (NvMlAesIsEngineBusy(engine))
        ;

    NvMlHashBlocks(engine,
                    pK1,
                    pData,
                    pHash,
                    numBlocks,
                    startMsg,
                    endMsg);

}

void
NvMlUtilsInitHash(NvBootAesEngine engine)
{
    NvMlAesEnableHashingInEngine(engine, NV_TRUE);
}

void
NvMlUtilsDeInitHash(NvBootAesEngine engine)
{
    NvMlAesEnableHashingInEngine(engine, NV_FALSE);
}

void
NvMlUtilsReadHashOutput(
    NvBootAesEngine engine,
    NvBootHash *pHash)
{
    if (NvMlAesIsHashingEnabled(engine))
    {
        // Check if engine is idle before reading the output
        while ( NvMlAesIsEngineBusy(engine) )
            ;
        NvMlAesReadHashOutput(engine, (NvU8 *)pHash);
    }
}

NvError
NvMlUtilsConvert3pToBootFuseDevice(
    Nv3pDeviceType DevNv3p,
    NvBootFuseBootDevice *pDevNvBootFuse)
{
    NvError errVal = NvError_Success;
    switch (DevNv3p)
    {
        case Nv3pDeviceType_Nand:
            *pDevNvBootFuse = NvBootFuseBootDevice_NandFlash;
            break;
        case Nv3pDeviceType_Spi:
            *pDevNvBootFuse = NvBootFuseBootDevice_SpiFlash;
            break;
        case Nv3pDeviceType_Nand_x16:
            *pDevNvBootFuse = NvBootFuseBootDevice_NandFlash_x16;
            break;
        case Nv3pDeviceType_Emmc:
            *pDevNvBootFuse = NvBootFuseBootDevice_Sdmmc;
            break;
        case Nv3pDeviceType_MuxOneNand:
            *pDevNvBootFuse = NvBootFuseBootDevice_MuxOneNand;
            break;
        case Nv3pDeviceType_MobileLbaNand:
            *pDevNvBootFuse = NvBootFuseBootDevice_MobileLbaNand;
            break;
        default:
            *pDevNvBootFuse = NvBootFuseBootDevice_Max;
            errVal = NvError_BadParameter;
            break;
    }
    return errVal;
}

NvError
NvMlUtilsConvertBootFuseDeviceTo3p(
    NvBootFuseBootDevice NvBootFuse,
    Nv3pDeviceType *pNv3pDevice)
{
    NvError errVal = NvError_Success;
    switch (NvBootFuse)
    {
        case NvBootFuseBootDevice_NandFlash:
            *pNv3pDevice = Nv3pDeviceType_Nand;
            break;
        case NvBootFuseBootDevice_Sdmmc:
            *pNv3pDevice = Nv3pDeviceType_Emmc;
            break;
        case NvBootFuseBootDevice_SpiFlash:
            *pNv3pDevice = Nv3pDeviceType_Spi;
            break;
        case NvBootFuseBootDevice_MuxOneNand:
            *pNv3pDevice = Nv3pDeviceType_MuxOneNand;
            break;
        case NvBootFuseBootDevice_MobileLbaNand:
            *pNv3pDevice = Nv3pDeviceType_MobileLbaNand;
            break;
        default:
            *pNv3pDevice = Nv3pDeviceType_Force32;
            errVal = NvError_BadParameter;
            break;
    }
    return errVal;
}

NvError
NvMlUtilsConvertBootFuseDeviceToBootDevice(
    NvBootFuseBootDevice NvBootFuse,
    NvBootDevType *pBootDevice)
{
    NvError errVal = NvError_Success;
    switch (NvBootFuse)
    {
        case NvBootFuseBootDevice_NandFlash:
            *pBootDevice = NvBootDevType_Nand;
            break;
        case NvBootFuseBootDevice_Sdmmc:
            *pBootDevice = NvBootDevType_Sdmmc;
            break;
        case NvBootFuseBootDevice_SpiFlash:
            *pBootDevice = NvBootDevType_Spi;
            break;
        case NvBootFuseBootDevice_MuxOneNand:
            *pBootDevice = NvBootDevType_MuxOneNand;
            break;
        case NvBootFuseBootDevice_MobileLbaNand:
            *pBootDevice = NvBootDevType_MobileLbaNand;
            break;
        case NvBootFuseBootDevice_SnorFlash:
            *pBootDevice = NvBootDevType_Snor;
            break;
        default:
            *pBootDevice = NvBootDevType_Force32;
            errVal = NvError_BadParameter;
            break;
    }
    return errVal;
}

NvBootDevType
NvMlUtilsGetSecondaryBootDevice(void)
{
    NvBootDevType        DevType;
    NvBootFuseBootDevice FuseDev;
    /* Query device info from fuses. */
    NvMlGetBootDeviceType(&FuseDev);

    switch( FuseDev )
    {
        case NvBootFuseBootDevice_Sdmmc:
            DevType = NvBootDevType_Sdmmc;
            break;
        case NvBootFuseBootDevice_SnorFlash:
            DevType = NvBootDevType_Snor;
            break;
        case NvBootFuseBootDevice_SpiFlash:
            DevType = NvBootDevType_Spi;
            break;
        case NvBootFuseBootDevice_NandFlash:
            DevType = NvBootDevType_Nand;
            break;
        case NvBootFuseBootDevice_MobileLbaNand:
            DevType = NvBootDevType_MobileLbaNand;
            break;
        case NvBootFuseBootDevice_MuxOneNand:
            DevType = NvBootDevType_MuxOneNand;
            break;
        default:
            DevType = NvBootDevType_None;
            break;
    }
    return DevType;
}

static NvBool NvFuseSkipDevSelStraps(void)
{
    NvU32 RegData;

    RegData = NV_READ32(NV_ADDRESS_MAP_FUSE_BASE + FUSE_RESERVED_SW_0);
    RegData = NV_DRF_VAL(FUSE, RESERVED_SW, SKIP_DEV_SEL_STRAPS, RegData);

    if (RegData)
        return NV_TRUE;
    else
        return NV_FALSE;
}


static NvStrapDevSel NvStrapDeviceSelection(void)
{
     NvU32 RegData;
    NvU8 *pMisc;
    NvError e;

    NV_CHECK_ERROR_CLEANUP(
        NvOsPhysicalMemMap(MISC_PA_BASE, MISC_PA_LEN,
            NvOsMemAttribute_Uncached, NVOS_MEM_READ_WRITE, (void **)&pMisc));

    RegData = NV_READ32(pMisc + APB_MISC_PP_STRAPPING_OPT_A_0);

    NvOsPhysicalMemUnmap(pMisc, MISC_PA_LEN);

    return (NvStrapDevSel)(NV_DRF_VAL(APB_MISC_PP,
                                          STRAPPING_OPT_A,
                                          BOOT_SELECT,
                                          RegData));

fail:
    return NvStrapDevSel_Emmc_Primary_x4;
}

void NvMlUtilsGetBootDeviceType(NvBootFuseBootDevice *pDevType)
{

    NvStrapDevSel StrapDevSel;

    // Read device selcted using the strap
    StrapDevSel = NvStrapDeviceSelection();

    /* Determine if the fuse data should be overwritten by strap data. */
    if (!NvFuseSkipDevSelStraps() && StrapDevSel != NvStrapDevSel_UseFuses)
    {
        /* Handle out-of-bound values */
        if (StrapDevSel >= NvStrapDevSel_Num)
            StrapDevSel = NvStrapDevSel_Emmc_Primary_x4;

        *pDevType  = StrapDevSelMap[StrapDevSel];
    }
    else
    {
        // Read from fuse
        NvBootFuseGetBootDevice(pDevType);
    }
    if (*pDevType >= NvBootFuseBootDevice_Max)
        *pDevType = NvBootFuseBootDevice_Max;
}

void NvMlUtilsGetBootDeviceConfig(NvU32 *pConfigVal)
{
    NvStrapDevSel StrapDevSel;

    // Read device selcted using the strap
    StrapDevSel = NvStrapDeviceSelection();

    /* Determine if the fuse data should be overwritten by strap data. */
    if (!NvFuseSkipDevSelStraps() && StrapDevSel != NvStrapDevSel_UseFuses)
    {
        /* Handle out-of-bound values */
        if (StrapDevSel >= NvStrapDevSel_Num)
            StrapDevSel = NvStrapDevSel_Emmc_Primary_x4;

        *pConfigVal = StrapDevConfigMap[StrapDevSel];
    }
    else
    {
        // Read from fuse
        NvBootFuseGetBootDeviceConfiguration(pConfigVal);
    }
}

NvError NvMlUtilsValidateBctDevType()
{
    NvBootFuseBootDevice FuseDev;
    NvBootDevType BootDev;
    NvU32 Size = sizeof(NvBootDevType);
    NvU32 Instance = 0;
    NvError e = NvError_BadParameter;

    NV_CHECK_ERROR_CLEANUP(
        NvBctGetData(BctDeviceHandle, NvBctDataType_DevType,
                         &Size, &Instance, &BootDev)
    );

    NvMlUtilsGetBootDeviceType(&FuseDev);
    if (FuseDev != NvBootFuseBootDevice_Max)
    {
        if (FuseDev == BootDev)
        {
            e = NvSuccess;
        }
    }

fail:
    return e;
}


void SetPllmInRecovery(void)
{
    // This implementation is required only in T30 since there is no
    //PLLM auto restart mechanism on ap15/20
}

NvU32 NvMlUtilsGetBctSize(void)
{
    return sizeof(NvBootConfigTable);
}

#if AES_KEYSCHED_LOCK_WAR_BUG_598910
void NvMlUtilsLockAesKeySchedule(void)
{
    NvAesDisableKeyScheduleRead();
}
#endif

void NvMlUtilsFuseGetUniqueId(void * Id)
{
    NvBootFuseGetUniqueId((NvU64 *)Id);
}

void
NvMlUtilsSskGenerate(NvBootAesEngine SbkEngine,
    NvBootAesKeySlot SbkEncryptSlot,
    NvU8 *ssk)
{
    const NvU32 BlockLengthBytes = NVBOOT_AES_BLOCK_LENGTH_BYTES;
    NvU8 uid[NVBOOT_AES_BLOCK_LENGTH_BYTES];
    NvU8 dk[NVBOOT_AES_BLOCK_LENGTH_BYTES];
    NvU64 uid64; // TODO PS -- change fuse api to take an NvU8 pointer
    NvU32 i;

    // SSK is calculated as follows --
    //
    // SSK = AES(SBK; UID ^ AES(SBK; DK; Encrypt); Encrypt)

    // We start with an assumption that DK is zero
    NvOsMemset(dk, 0, NVBOOT_AES_BLOCK_LENGTH_BYTES);

    // copy the 4 byte device key into the consecutive word locations
    // to frame the 16 byte AES block;
    for (i = 0; i < 4; i++)
    {
        dk[i+4] = dk[i+8] = dk[i+12] = dk[i];
    }

    // Make sure engine is idle, then select SBK encryption key.  Encrypt
    // Device Key with the SBK, storing the result in dk.  Finally, wait
    // for engine to become idle.

    while (NvMlAesIsEngineBusy(SbkEngine));
    NvMlAesSelectKeyIvSlot(SbkEngine, SbkEncryptSlot);
    NvMlAesStartEngine(SbkEngine, 1 /*1 block*/, dk, dk, NV_TRUE);
    while (NvMlAesIsEngineBusy(SbkEngine));

    // Get Unique ID from the fuse (64 bit = 8 bytes)
    NvBootFuseGetUniqueId(&uid64);
    // TODO PS -- change NvBootFuseGetUniqueId() to return an array of bytes
    //            instead of an NvU64
    //
    // until then, swap byte ordering to make SSK calculation cleaner;
    // after the fix, the Unique ID can be read directly into the uid[] array
    // and the variable uid64 can be eliminated
    for (i = 0; i < 8; i++) {
        uid[7-i] = uid64 & 0xff;
        uid64 >>= 8;
    }

    // copy the 64 bit of UID again at the end of first UID
    // to make 128 bit AES block.
    NvOsMemcpy(&uid[8], &uid[0], BlockLengthBytes >> 1);

    // XOR the uid and AES(SBK, DK, Encrypt)
    for (i = 0; i < BlockLengthBytes; i++)
        ssk[i] = uid[i] ^ dk[i];

    // Make sure engine is idle, then select SBK encryption key.  Compute
    // the final SSK value by perform a second SBK encryption operation,
    // and store the result in sskdk.  Wait for engine to become idle.
    //
    // Note: re-selecting the SBK key slot is needed in order to reset the
    //       IV to the pre-set value

    while (NvMlAesIsEngineBusy(SbkEngine));
    NvMlAesSelectKeyIvSlot(SbkEngine, SbkEncryptSlot);
    NvMlAesStartEngine(SbkEngine, 1 /*1 block*/, ssk, ssk, NV_TRUE);
    while (NvMlAesIsEngineBusy(SbkEngine));

    // Re-select slot to clear any left-over state information
    NvMlAesSelectKeyIvSlot(SbkEngine, SbkEncryptSlot);

}

void
NvMlUtilsClocksStartPll(NvU32 PllSet,
    NvBootSdramParams *pData,
    NvU32 StableTime)
{
    //do nothing
}

NvU32 NvMlUtilsGetDataOffset(void)
{
    return sizeof(NvBootHash);
}

NvU32 NvMlUtilsGetPLLPFreq(void)
{
    return PLLP_FIXED_FREQ_KHZ_432000;
}

#if NO_BOOTROM

void NvMlUtilsPreInitBit(NvBootInfoTable *pBitTable)
{
    NvOsMemset( (void*) pBitTable, 0, sizeof(NvBootInfoTable) ) ;

    pBitTable->BootRomVersion = NVBOOT_VERSION(2,1);
    pBitTable->DataVersion = NVBOOT_VERSION(2,1);
    pBitTable->RcmVersion = NVBOOT_VERSION(2,1);
    pBitTable->BootType = NvBootType_Recovery;
    pBitTable->PrimaryDevice = NvBootDevType_Irom;

    pBitTable->OscFrequency = NvBootClocksGetOscFreq();
    // Workaround for missing BIT in Boot ROM version 1.000
    //
    // Rev 1.000 of the Boot ROM incorrectly clears the BIT when a Forced
    // Recovery is performed.  Fortunately, very few fields of the BIT are
    // set as a result of a Forced Recovery, so most everything can be
    // reconstructed after the fact.
    //
    // One piece of information that is lost is the source of the
    // Forced Recovery -- by strap or by flag (PMC Scratch0).  If the
    // flag was set, then it is cleared and the ClearedForceRecovery
    // field is set to NV_TRUE.  Since the BIT has been cleared by
    // time an applet is allowed to execute, there's no way to
    // determine if the flag in PMC Scratch0 has been cleared.  If the
    // strap is not set, though, then one can infer that the flag must
    // have been set (and subsequently cleared).  However, if the
    // strap is not set, there's no way to know whether or not the
    // flag was also set.

    // TODO -- check Force Recovery strap to partially infer whether PMC
    //         Scratch0 flag was cleared (see note above for details)

    pBitTable->ClearedForceRecovery = NV_FALSE;
    pBitTable->SafeStartAddr = (NvU32)pBitTable + sizeof(NvBootInfoTable);

    //Claim that we booted off the first bootloader
    pBitTable->BlState[0].Status = NvBootRdrStatus_Success;
    pBitTable->DevInitialized = NV_TRUE;

}

NvBool NvMlUtilsStartUsbEnumeration()
{
    //do nothing for ap20 and t30;
    return NV_TRUE;
}

#endif
