/*
 * Copyright (c) 2010-2012, NVIDIA CORPORATION. All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "recovery_utils.h"
#include "nvstormgr.h"
#include "nvsystem_utils.h"
#include "nvstormgr.h"
#include "nvddk_blockdevmgr.h"
#include "nvddk_blockdev.h"
#include "nvassert.h"
#include "nvbu.h"
#include "nvbu_private.h"
#include "ap20/nvboot_bct.h"
#include "nvfuse.h"
#include "nvcrypto_hash.h"
#include "nvfsmgr_defs.h"
#include "nvodm_system_update.h"
#include "fastboot.h"
#include "nvos.h"
#include "nvaboot.h"
#include "nv_sha.h"
#include "nv_rsa.h"
#include "nv_secureblob.h"
#include "nvcrypto_cipher.h"
#ifndef NV_LDK_BUILD
#include "publickey.h"
#endif
#include "t30/arapbpm.h"

#define UPDATE_MAGIC       "MSM-RADIO-UPDATE"
#define UPDATE_MAGIC_SIZE  16
#define UPDATE_VERSION     0x00010000
#define PARTITION_NAME_LENGTH 4
#define UPDATE_CHUNK (10*1024*1024)
#define MISC_PAGES 3
#define MISC_COMMAND_PAGE 1
#define DATA_SIZE 134144
#define NV_AES_HASH_BLOCK_LEN (16)
#define APBDEV_PMC_SCRATCH0_0_KEENHI_REBOOT_FLAG_BIT 29
#define APBDEV_PMC_SCRATCH0_0_FASTBOOT_FLAG_BIT 30
#define APBDEV_PMC_SCRATCH0_0_RCK_FLAG_BIT 31
#define APBDEV_PMC_SCRATCH0_0_FORCE_RECOVERY_FLAG_BIT 1

#define GET_VERSION(x)   ((x) & 0x7FFFFFFF)
#define IS_ENCRYPTED(x)  (((x) & 0x80000000) >> (8*sizeof(NvU32)-1))

typedef enum
{
    RecoveryBitmapType_InstallingFirmware,
    RecoveryBitmapType_InstallationError,
    RecoveryBitmapType_Nums = 0x7fffffff
}RecoveryBitmapType;

typedef struct
{
    char command[32];
    char status[32];
    char recovery[1024];
}BootloaderMessage ;

typedef struct
{
    unsigned char MAGIC[UPDATE_MAGIC_SIZE];
    unsigned Version;

    unsigned Size;

    unsigned EntriesOffset;
    unsigned NumEntries;

    unsigned BitmapWidth;
    unsigned BitmapHeight;
    unsigned BitmapBpp;

    unsigned BusyBitmapOffset;
    unsigned BusyBitmapLength;

    unsigned FailBitmapOffset;
    unsigned FailBitmapLength;
}UpdateHeader;

typedef struct
{
    char PartName[PARTITION_NAME_LENGTH];
    unsigned ImageOffset;
    unsigned ImageLength;
    unsigned Version;
}ImageEntry;

static NvBitHandle s_hBit;
static NvBctHandle s_hBct;
static NvU32 s_nBl;
static NvBuBlInfo *s_BlInfo = 0;
static NvBool s_BlUpdated;
static char s_Status[32];
BootloaderMessage BootMessage;
#ifdef NV_LDK_BUILD
/* LDK build doesn't use OTA */
static NvBigIntModulus RSAPublicKey;
NvS32 NvRSAVerifySHA1Dgst(NvU8 *Dgst, NvBigIntModulus *PubKeyN,
                          NvU32 *PubKeyE, NvU32 *Sign)
{
    return 1;
}
NvS32 NvSHA_Init(NvSHA_Data *pSHAData)
{
    return 1;
}
NvS32 NvSHA_Update(NvSHA_Data *pSHAData, NvU8 *pMsg, NvU32 MsgLen)
{
    return 1;
}
NvS32 NvSHA_Finalize(NvSHA_Data *pSHAData, NvSHA_Hash *pSHAHash)
{
    return 1;
}
#endif

static NvError
GetStorageGeometry( NvU32 PartitionID, NvU32 *start_block,
    NvU32 *start_page )
{
    NvBool b;
    NvError e = NvSuccess;
    NvPartInfo part_info;
    NvU32 sectors_per_block;
    NvU32 bytes_per_sector;
    NvU32 phys_sector;
    NvU32 size = sizeof(NvBootDevType);
    NvU32 inst = 0;
    NvU32 block_size_log2;
    NvU32 page_size_log2;

    NV_CHECK_ERROR_CLEANUP(
        NvPartMgrGetPartInfo( PartitionID, &part_info )
    );

    b = NvSysUtilGetDeviceInfo( PartitionID,
        (NvU32)part_info.StartLogicalSectorAddress,
        &sectors_per_block, &bytes_per_sector, &phys_sector );
    if (!b)
    {
        e = NvError_DeviceNotFound;
        goto fail;
    }

    size = 4;
    inst = 0;

    NV_CHECK_ERROR_CLEANUP(
        NvBctGetData( s_hBct, NvBctDataType_BootDeviceBlockSizeLog2, &size,
            &inst, &block_size_log2 )
    );

    NV_CHECK_ERROR_CLEANUP(
        NvBctGetData( s_hBct, NvBctDataType_BootDevicePageSizeLog2, &size,
            &inst, &page_size_log2 )
    );

    /* convert to what the bootrom thinks is right. */
    *start_block = (phys_sector * bytes_per_sector) /
        (1 << block_size_log2);
    *start_page = (phys_sector * bytes_per_sector) %
        (1 << block_size_log2);
    *start_page /= (1 << page_size_log2);

fail:
    return e;
}

static NvError GetBLInfo(void)
{
    NvError e = NvSuccess;

    if (!s_BlInfo)
    {
        NV_CHECK_ERROR_CLEANUP(
            NvBitInit( &s_hBit )
        );

        NV_CHECK_ERROR_CLEANUP(
            NvBctInit( 0, 0, &s_hBct )
        );

        NV_CHECK_ERROR_CLEANUP(
            NvBuQueryBlOrder( s_hBct, s_hBit, &s_nBl, 0 )
        );

        s_BlInfo = NvOsAlloc( sizeof(NvBuBlInfo) * s_nBl );
        NV_ASSERT(s_BlInfo);
        if (!s_BlInfo)
        {
            e = NvError_InsufficientMemory;
            goto fail;
        }

        NV_CHECK_ERROR_CLEANUP(
            NvBuQueryBlOrder( s_hBct, s_hBit, &s_nBl, s_BlInfo )
        );
    }

fail:
    return e;
}

static NvError GetBCTHandle(NvStorMgrFileHandle hStagingFile, ImageEntry *Entry,
                                    NvBctHandle *pRcvBCTHandle, NvBool IsEncrypted)
{
    NvError e = NvSuccess;
    NvU8 *bctbuf = NULL;
    NvU32 bctbufsize = 0;
    NvU32 bytes = 0;
    NvCryptoCipherAlgoHandle hCipher = 0;
    NvCryptoCipherAlgoParams cipher_params;
    NvBootConfigTable *pBCT = NULL;

    if (!hStagingFile || !Entry)
    {
        e = NvError_BadParameter;
        goto fail;
    }

    bctbufsize = Entry->ImageLength;

    /*  read the BCT to a buffer */
    bctbuf = NvOsAlloc(bctbufsize);
    if (bctbuf == NULL)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    NV_CHECK_ERROR_CLEANUP(
        NvStorMgrFileSeek(hStagingFile,
                          (Entry->ImageOffset)+ (sizeof(NvSignedUpdateHeader)),
                          NvOsSeek_Set)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvStorMgrFileRead(hStagingFile, bctbuf, bctbufsize, &bytes)
    );
    if (bytes != bctbufsize)
    {
        e = NvError_FileReadFailed;
        goto fail;
    }

    if (IsEncrypted)
    {
        /* allocate cipher algorithm   */
        NV_CHECK_ERROR_CLEANUP(
            NvCryptoCipherSelectAlgorithm(NvCryptoCipherAlgoType_AesCbc,
                                          &hCipher)
        );

        /* configure algorithm  */
        NvOsMemset(&cipher_params, 0, sizeof(cipher_params));
        cipher_params.AesCbc.IsEncrypt = NV_FALSE;
        cipher_params.AesCbc.KeyType = NvCryptoCipherAlgoAesKeyType_SecureBootKey;
        cipher_params.AesCbc.KeySize = NvCryptoCipherAlgoAesKeySize_128Bit;

        NvOsMemset(cipher_params.AesCbc.InitialVectorBytes, 0x0,
                   sizeof(cipher_params.AesCbc.InitialVectorBytes));
        cipher_params.AesCbc.PaddingType =
            NvCryptoPaddingType_ExplicitBitPaddingOptional;

        NV_CHECK_ERROR_CLEANUP(
            hCipher->SetAlgoParams( hCipher, &cipher_params )
        );

        /* decrypt bulk of BCT (in place decryption)  */
        NV_CHECK_ERROR_CLEANUP(
            hCipher->ProcessBlocks( hCipher, bctbufsize, bctbuf, bctbuf,
                                    NV_TRUE, NV_TRUE )
        );
    }

    NV_CHECK_ERROR_CLEANUP(
        NvBctInit(&bctbufsize, (void*)bctbuf, pRcvBCTHandle)
    );

fail:
    return e;
}

static NvError
UpdateBLParamsOfBCT(NvU32 Slot, NvU32 version, NvU32 start_block, NvU32 start_page,
                            NvU32 length, NvU32 hash_size, NvU8 *hash)
{
    NvError e = NvSuccess;
    NvU32 tempslot = Slot;
    NvU32 size = sizeof(version);

    /*  Update new Bootloader version & length & block & page*/
    NV_CHECK_ERROR_CLEANUP(
        NvBctSetData(s_hBct, NvBctDataType_BootLoaderVersion,
                    &size, &tempslot, (void*)(&version))
    );

    NV_CHECK_ERROR_CLEANUP(
        NvBctSetData(s_hBct, NvBctDataType_BootLoaderLength,
                    &size, &tempslot, (void*)(&length))
    );

    NV_CHECK_ERROR_CLEANUP(
        NvBctSetData(s_hBct, NvBctDataType_BootLoaderStartBlock,
                    &size, &tempslot, (void*)(&start_block))
    );

    NV_CHECK_ERROR_CLEANUP(
        NvBctSetData(s_hBct, NvBctDataType_BootLoaderStartSector,
                    &size, &tempslot, (void*)(&start_page))
    );

    /*  Update Bootloader hash value */
    NV_CHECK_ERROR_CLEANUP(
        NvBctSetData(s_hBct, NvBctDataType_BootLoaderCryptoHash,
                    &hash_size, &tempslot, hash)
    );

fail:
    return(e);
}

static NvError
UpdateParamsOfBCT(NvBctHandle RcvBCTHandle, NvBctDataType ParamNum,
                          NvBctDataType Param)
{
    NvError e = NvSuccess;
    NvU8  *buf = 0;
    NvU32 size = sizeof(NvU32);
    NvU32 Instance = 0, i = 0;
    NvU32 NumOfValidInstances = 0;

    /*  Get Number of Valid Instances */
    NV_CHECK_ERROR_CLEANUP(
        NvBctGetData(RcvBCTHandle, ParamNum, &size, &Instance, &NumOfValidInstances)
    );

    /*  Update Number of Valid Instances to BCT */
    NV_CHECK_ERROR_CLEANUP(
        NvBctSetData(s_hBct, ParamNum, &size, &Instance, &NumOfValidInstances)
    );

    /*  Update All valid instances to BCT */
    size = 0;
    NV_CHECK_ERROR_CLEANUP(
        NvBctGetData(RcvBCTHandle, Param, &size, &Instance, buf)
    );

    buf = NvOsAlloc(size);
    if (buf == NULL)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    for (i = 0 ; i < NumOfValidInstances ; i++)
    {
        NV_CHECK_ERROR_CLEANUP(
            NvBctGetData(RcvBCTHandle, Param, &size, &i, buf)
        );
        NV_CHECK_ERROR_CLEANUP(
            NvBctSetData(s_hBct, Param, &size, &i, buf)
        );
    }
fail:
    NvOsFree(buf);
    return(e);
}

static NvError UpdateBCTToDevice()
{
    NvBlOperatingMode op_mode;
    NvU32 size = 0;
    NvU32 bct_part_id;
    NvU8 *buf = 0;
    NvError e = NvSuccess;

    /* get the bct size */
    NV_CHECK_ERROR_CLEANUP(
        NvBctInit( &size, 0, 0 )
    );

    buf = NvOsAlloc( size );
    NV_ASSERT(buf);
    if (!buf)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    NV_CHECK_ERROR_CLEANUP(
        NvPartMgrGetIdByName( "BCT", &bct_part_id )
    );

    op_mode = NvFuseGetOperatingMode();

    /* re-sign and write the BCT */
    NV_CHECK_ERROR_CLEANUP(
        NvBuBctCryptoOps( s_hBct, op_mode, &size, buf,
            NvBuBlCryptoOp_EncryptAndSign )
    );
    NV_CHECK_ERROR_CLEANUP(
        NvBuBctUpdate( s_hBct, s_hBit, bct_part_id, size, buf )
    );
fail:
    NvOsFree(buf);
    return e;
}

static NvError UpdateRedundantBLandMB(NvU8 *buf, NvU32 bufsize, NvU32 CurrentLoadAddress,
                                      NvU32 Version, NvBool IsEncrypted)
{
    NvError e = NvSuccess;
    NvU8 hash[NVCRYPTO_CIPHER_AES_BLOCK_BYTES];
    NvU32 hash_size = 0;
    NvU32 i = 0;
    NvU32 pad_length;
    NvU32 start_block;
    NvU32 start_page;
    NvU32 PartitionId = -1;
    NvBlOperatingMode op_mode;

    NV_ASSERT(s_BlInfo);
    NV_ASSERT(s_hBct);

    op_mode = NvFuseGetOperatingMode();

    hash_size = sizeof(hash);
    for (i = 0 ; i < s_nBl ; i++)
    {
        if (s_BlInfo[i].LoadAddress == CurrentLoadAddress)
        {
            NV_CHECK_ERROR_CLEANUP(
                NvBuQueryBlPartitionIdBySlot(s_hBct, i, &PartitionId)
            );
            NV_CHECK_ERROR_CLEANUP(
                NvSysUtilEncyptedBootloaderWrite(buf,
                                                 bufsize,
                                                 PartitionId,
                                                 op_mode,
                                                 hash,
                                                 hash_size,
                                                 &pad_length,
                                                 IsEncrypted)
            );

            /* get obnoxious storage geometry stuff */
            start_block = 0;
            start_page = 0;
            NV_CHECK_ERROR_CLEANUP(
                GetStorageGeometry(PartitionId, &start_block, &start_page)
            );

            /* finally update bl info */
            NV_CHECK_ERROR_CLEANUP(
                UpdateBLParamsOfBCT(i, Version, start_block, start_page,
                                    pad_length, hash_size, hash)
            );

            /*  Write the infomation back to BCT */
            NV_CHECK_ERROR_CLEANUP(
                UpdateBCTToDevice()
            );
        }
    }

fail:
    return e;
}

static NvError
CheckAndUpdateBCT(NvStorMgrFileHandle hStagingFile, ImageEntry* Entry, NvBool *BCT)
{
    NvError e = NvSuccess;
    NvBctHandle RcvBCTHandle = NULL;
    NV_ASSERT(BCT);

    if (! NvOsStrncmp(Entry->PartName,"BCT",PARTITION_NAME_LENGTH))
    {
        NV_CHECK_ERROR_CLEANUP(
            GetBCTHandle(hStagingFile, Entry, &RcvBCTHandle,
                                 IS_ENCRYPTED(Entry->Version))
        );
    }
    else
        goto fail;

    if (RcvBCTHandle != NULL)
    {
        /*  Update SDParam Info */
        NV_CHECK_ERROR_CLEANUP(
            UpdateParamsOfBCT(RcvBCTHandle,
                                      NvBctDataType_NumValidSdramConfigs,
                                      NvBctDataType_SdramConfigInfo)
        );

        /*  Update SDParam Info */
        NV_CHECK_ERROR_CLEANUP(
            UpdateParamsOfBCT(RcvBCTHandle,
                                      NvBctDataType_NumValidBootDeviceConfigs,
                                      NvBctDataType_BootDeviceConfigInfo)
        );

        /* Update type of device */
        NV_CHECK_ERROR_CLEANUP(
            UpdateParamsOfBCT(RcvBCTHandle,
                                      NvBctDataType_NumValidDevType,
                                      NvBctDataType_DevType)
        );

        /*  Write the infomation back to BCT */
        NV_CHECK_ERROR_CLEANUP(
            UpdateBCTToDevice()
        );
        *BCT = NV_TRUE;
        NvOsDebugPrintf("complete update BCT\n");
    }
    else
    {
        e = NvError_BadParameter;
        goto fail;
    }

fail:
    NvOsFree(RcvBCTHandle);
    return e;
}


static NvError
CheckAndUpdateBootloader(NvStorMgrFileHandle hStagingFile,
        ImageEntry* Entry, NvBool *Bootloader)
{
    NvError e = NvSuccess;
    NvU32 i;
    NvU32 bytes;
    NvU8 *buf = 0;
    NvU32 entry_point = 0;
    NvU32 load_addr = 0;
    NvU32 PartitionId = -1;
    NvU32 CurrentLoadAddress = 0;
    NvU32 size = sizeof(NvU32);

    NV_ASSERT(hStagingFile );
    NV_ASSERT(Entry );
    NV_ASSERT(Bootloader );

    if (!hStagingFile || !Entry || !Bootloader)
    {
        e = NvError_BadParameter;
        goto fail;
    }
    *Bootloader = NV_FALSE;
    NV_CHECK_ERROR_CLEANUP(
        NvPartMgrGetIdByName( Entry->PartName, &PartitionId )
    );

    NV_CHECK_ERROR_CLEANUP(GetBLInfo());

    /* look for the partition id in the bootloader order array */
    for (i = 0; i < s_nBl; i++)
    {
        if (s_BlInfo[i].PartitionId == PartitionId)
        {
            /* version must be larger */
            NvBool result;
            result = NvOdmSysUpdateIsUpdateAllowed( NvOdmSysUpdateBinaryType_Bootloader,
                                s_BlInfo[i].Version, GET_VERSION(Entry->Version));
            if (result)
            {
                *Bootloader = NV_TRUE;
                CurrentLoadAddress = s_BlInfo[i].LoadAddress;
            }
            else
            {
                // It is a bootloader but current version is not supported
                *Bootloader = NV_TRUE;
                e = NvError_SysUpdateInvalidBLVersion;
                goto fail;
            }
            break;
        }
    }

    if (!*Bootloader)
    {
        /* not a bootloader, skip the rest */
        e = NvSuccess;
        goto fail;
    }

    buf = NvOsAlloc( (NvU32)Entry->ImageLength);
    NV_ASSERT(buf);
    if (!buf)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    NV_CHECK_ERROR_CLEANUP(
        NvStorMgrFileSeek(hStagingFile,
                          (Entry->ImageOffset)+ (sizeof(NvSignedUpdateHeader)),
                          NvOsSeek_Set)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvStorMgrFileRead(hStagingFile, buf, (NvU32)Entry->ImageLength, &bytes)
    );
    if (bytes != (NvU32)Entry->ImageLength)
    {
        e = NvError_FileReadFailed;
        goto fail;
    }

    NV_CHECK_ERROR_CLEANUP(
        UpdateRedundantBLandMB(buf, (NvU32)Entry->ImageLength, CurrentLoadAddress,
                               GET_VERSION(Entry->Version),IS_ENCRYPTED(Entry->Version))
    );

fail:
    NvOsFree( buf );
    return e;

}

static NvError
UpdateSinglePartition(NvStorMgrFileHandle hReadFile,
                      NvStorMgrFileHandle hWriteFile, NvU32 size)
{
    NvError e = NvSuccess;
    NvU8 *buf = 0;
    NvBool LookForSparseHeader = NV_TRUE;
    NvBool SparseImage = NV_FALSE;
    NvBool IsSparseStart = NV_FALSE;
    NvBool IsLastBuffer = NV_FALSE;
    NvU32 len = 0;
    NvU32 bytes = 0;
    NvFileSystemIoctl_WriteVerifyModeSelectInputArgs IoctlArgs;

    NV_ASSERT(hReadFile);
    NV_ASSERT(hWriteFile);

    buf = NvOsAlloc(UPDATE_CHUNK);
    if (!buf)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    // Enable write verify
    IoctlArgs.ReadVerifyWriteEnabled = NV_TRUE;
    NV_CHECK_ERROR_CLEANUP(NvStorMgrFileIoctl(
        hWriteFile,
        NvFileSystemIoctlType_WriteVerifyModeSelect,
        sizeof(IoctlArgs),
        0,
        &IoctlArgs,
        NULL));

    while (size)
    {
        len = (NvU32)NV_MIN(UPDATE_CHUNK, size);

        NV_CHECK_ERROR_CLEANUP(
            NvStorMgrFileRead(hReadFile, buf, len, &bytes)
        );
        if (bytes != len)
        {
            e = NvError_FileReadFailed;
            goto fail;
        }

        if (LookForSparseHeader)
        {
            if (NvSysUtilCheckSparseImage(buf, len))
            {
                SparseImage = NV_TRUE;
                IsSparseStart = NV_TRUE;
            }
            LookForSparseHeader = NV_FALSE;
        }

        if (!SparseImage)
        {
            NV_CHECK_ERROR_CLEANUP(
                NvStorMgrFileWrite( hWriteFile, buf, len, &bytes )
            );
            if (bytes != len)
            {
                e = NvError_InvalidSize;
                goto fail;
            }
        }
        else
        {
            if (!(size - len))
                IsLastBuffer = NV_TRUE;
            NV_CHECK_ERROR_CLEANUP(NvSysUtilUnSparse(hWriteFile,
                                                     buf,
                                                     len,
                                                     IsSparseStart,
                                                     IsLastBuffer,
                                                     NV_FALSE,
                                                     NULL,
                                                     NULL));
            IsSparseStart = NV_FALSE;
        }
        size -= len;
    }
fail:
    // Disable write verify
    IoctlArgs.ReadVerifyWriteEnabled = NV_FALSE;
    if (hWriteFile)
    {
        (void)NvStorMgrFileIoctl(
            hWriteFile,
            NvFileSystemIoctlType_WriteVerifyModeSelect,
            sizeof(IoctlArgs),
            0,
            &IoctlArgs,
            NULL);
    }
    NvOsFree(buf);
    return e;
}

static NvError
UpdatePartitionsFromBlob(NvStorMgrFileHandle hStagingFile, ImageEntry *Entry)
{
    NvError e = NvSuccess;
    NvBool Bootloader = NV_FALSE;
    NvBool BCT = NV_FALSE;
    NvStorMgrFileHandle hWriteFile = 0;
    NvU32 size;

    NV_ASSERT(hStagingFile );
    NV_ASSERT(Entry);

    if (!hStagingFile || !Entry)
    {
        e = NvError_BadParameter;
        goto fail;
    }

    // check for bootloader update
    e = CheckAndUpdateBootloader(hStagingFile, Entry, &Bootloader);

    if (e != NvSuccess)
    {
        if (!s_hBit)
        {
            /* the bootrom didn't boot the system, this must be a test loaded
             * over jtag. continue happily.
             */
            Bootloader = NV_FALSE;
        }
        else
        {
            /* bootloader failed to install */
            if (e == NvError_SysUpdateInvalidBLVersion || e == NvError_SysUpdateBLUpdateNotAllowed)
            {
            }
            goto fail;
        }

    }
    if (Bootloader)
       goto fail;
    // there is no botloader to update. .check for BCT update

    NV_CHECK_ERROR_CLEANUP(
        CheckAndUpdateBCT(hStagingFile, Entry, &BCT)
    );
    if (BCT)
        goto fail;

    NV_CHECK_ERROR_CLEANUP(
        NvStorMgrFileSeek(hStagingFile,
                          (Entry->ImageOffset) + (sizeof(NvSignedUpdateHeader)),
                          NvOsSeek_Set)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvStorMgrFileOpen(Entry->PartName, NVOS_OPEN_WRITE,
            &hWriteFile )
    );

    NV_CHECK_ERROR_CLEANUP(
        UpdateSinglePartition(hStagingFile, hWriteFile, (NvU32)Entry->ImageLength)
    );

fail:

    if (e != NvSuccess)
    {
        NvOsSnprintf(s_Status, sizeof(s_Status), "failed-update-%s", Entry->PartName);
    }

    (void)NvStorMgrFileClose( hWriteFile );
    return e;
}

static NvError VerifyBlobSignature(char *partname)
{
    NvSignedUpdateHeader *signedHeader = NULL;
    NvU32 *SignatureData = NULL;
    NvU8 *MessageBuffer = NULL;
    NvSHA_Data *SHAData = NULL;
    NvSHA_Hash *SHAHash = NULL;
    NvU32 *PubKeyE = NULL;
    NvStorMgrFileHandle hStagingFile = 0;
    NvU32 bytes = 0, so_far = 0;
    NvError e = NvSuccess;

    signedHeader = NvOsAlloc(sizeof(NvSignedUpdateHeader));
    if (signedHeader == NULL)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    SignatureData = NvOsAlloc(RSANUMBYTES);
    if (SignatureData == NULL)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    MessageBuffer = NvOsAlloc(SHA_1_CACHE_BUFFER_SIZE);
    if (MessageBuffer == NULL)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    SHAData = (NvSHA_Data*)NvOsAlloc(sizeof(NvSHA_Data));
    if (SHAData == NULL)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    SHAHash = (NvSHA_Hash*)NvOsAlloc(sizeof(NvSHA_Hash));
    if (SHAHash == NULL)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    PubKeyE = NvOsAlloc(RSANUMBYTES);
    if (PubKeyE == NULL){
        e = NvError_InsufficientMemory;
        goto fail;
    }
    NvOsMemset(PubKeyE, 0, RSANUMBYTES);
    PubKeyE[0] = RSA_PUBLIC_EXPONENT_VAL;

    NV_CHECK_ERROR_CLEANUP(
        NvStorMgrFileOpen(partname, NVOS_OPEN_READ, &hStagingFile)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvStorMgrFileRead(hStagingFile, signedHeader, sizeof(NvSignedUpdateHeader), &bytes)
    );
    if (bytes != sizeof(NvSignedUpdateHeader))
    {
        e = NvError_FileReadFailed;
        goto fail;
    }

    // TODO: Look for any other validation which can be performed here
    if (NvOsStrncmp((char*)signedHeader->MAGIC, SIGNED_UPDATE_MAGIC, SIGNED_UPDATE_MAGIC_SIZE))
    {
        e = NvError_InvalidState;
        goto fail;
    }

    /*  Initializing the SHA-1 Engine */
    if (NvSHA_Init(SHAData) == -1)
    {
        e = NvError_InvalidState;
        goto fail;
    }

    NV_CHECK_ERROR_CLEANUP(
        NvStorMgrFileSeek(hStagingFile, sizeof(NvSignedUpdateHeader), NvOsSeek_Set)
    );

    /*  Updating the SHA-1 engine hash value with the blob content */
    while (so_far < (signedHeader->ActualBlobSize))
    {
        NvU32 temp_size = SHA_1_CACHE_BUFFER_SIZE;
        if (((signedHeader->ActualBlobSize) - so_far) < temp_size)
            temp_size = (signedHeader->ActualBlobSize) - so_far;

        NV_CHECK_ERROR_CLEANUP(
            NvStorMgrFileRead(hStagingFile, MessageBuffer, temp_size, &bytes)
        );
        if (bytes != temp_size)
        {
            e = NvError_FileReadFailed;
            goto fail;
        }

        if (NvSHA_Update(SHAData, MessageBuffer, temp_size) == -1)
            goto fail;
        so_far += temp_size;
    }

    /*  Finalizing the SHA-1 engine hash value */
    if (NvSHA_Finalize(SHAData, SHAHash) == -1)
        goto fail;

    /*  Read Blob signature from file */
    NV_CHECK_ERROR_CLEANUP(
        NvStorMgrFileRead(hStagingFile, SignatureData, signedHeader->SignatureSize, &bytes)
    );
    if (bytes != signedHeader->SignatureSize)
    {
        e = NvError_FileReadFailed;
        goto fail;
    }

    /*  Signature of the file has to be verified here   */
    if (NvRSAVerifySHA1Dgst(SHAHash->Hash, &RSAPublicKey, PubKeyE, SignatureData) == -1)
    {
        e = NvError_InvalidState;
        goto fail;
    }

fail:
    NvOsFree(signedHeader);
    NvOsFree(SignatureData);
    NvOsFree(MessageBuffer);
    NvOsFree(PubKeyE);
    NvOsFree(SHAData);
    NvOsFree(SHAHash);
    (void)NvStorMgrFileClose( hStagingFile );
    if (e != NvSuccess)
    {
        NvOsDebugPrintf("\nBlob Signature Verification Failed\n");
    }
    return e;
}

NvError NvFastbootUpdateBin(char *PartName, NvU32 ImageSize,
                            const PartitionNameMap *NvPart)
{
    NvError e = NvSuccess;
    NvU8 *buf = 0;
    NvBool LookForSparseHeader = NV_TRUE;
    NvBool SparseImage = NV_FALSE;
    NvBool IsSparseStart = NV_FALSE;
    NvBool IsLastBuffer = NV_FALSE;
    NvU32 PartitionId = -1;
    NvU32 CurrentLoadAddress = 0;
    NvU32 bytes = 0;
    NvU32 i = 0;
    NvU32 pad_length = 0;
    NvU32 start_block = 0;
    NvU32 start_page = 0;
    NvU32 len;
    NvFileSystemIoctl_WriteVerifyModeSelectInputArgs IoctlArgs;
    NvBlOperatingMode op_mode;
    NvStorMgrFileHandle hStagingFile = NULL;
    NvStorMgrFileHandle hWriteFile = NULL;
    NvU8 hash[NVCRYPTO_CIPHER_AES_BLOCK_BYTES];

    if (!(NvOsStrncmp(NvPart->NvPartName, "NVC",sizeof("NVC")) &&
        NvOsStrncmp(NvPart->NvPartName, "EBT",sizeof("EBT"))))
    {
        NV_CHECK_ERROR_CLEANUP(
            NvPartMgrGetIdByName(NvPart->NvPartName, &PartitionId)
        );

        NV_CHECK_ERROR_CLEANUP(GetBLInfo());

        /* look for the partition id in the bootloader order array */
        for (i = 0; i < s_nBl; i++)
        {
            if (s_BlInfo[i].PartitionId == PartitionId)
            {
                CurrentLoadAddress = s_BlInfo[i].LoadAddress;
                break;
            }
        }

        buf = NvOsAlloc(ImageSize);
        NV_ASSERT(buf);
        if (!buf)
        {
            e = NvError_InsufficientMemory;
            goto fail;
        }

        NV_CHECK_ERROR_CLEANUP(
            NvStorMgrFileOpen(PartName, NVOS_OPEN_READ, &hStagingFile)
        );
        NV_CHECK_ERROR_CLEANUP(
            NvStorMgrFileRead(hStagingFile, buf, ImageSize, &bytes)
        );
        if (bytes != ImageSize)
        {
            e = NvError_FileReadFailed;
            goto fail;
        }
        NV_CHECK_ERROR_CLEANUP(
                    NvStorMgrFileClose(hStagingFile)
        );
        hStagingFile = NULL;

        NV_CHECK_ERROR_CLEANUP(
            UpdateRedundantBLandMB(buf, ImageSize, CurrentLoadAddress, 1, NV_FALSE)
        );

        NvOsFree(buf);
        buf = NULL;

    }   /*  end of bootloader updation */
    else
    {
        NV_CHECK_ERROR_CLEANUP(
            NvStorMgrFileOpen(PartName, NVOS_OPEN_READ, &hStagingFile)
        );
        NV_CHECK_ERROR_CLEANUP(
            NvStorMgrFileOpen(NvPart->NvPartName, NVOS_OPEN_WRITE, &hWriteFile)
        );
        NV_CHECK_ERROR_CLEANUP(
            UpdateSinglePartition(hStagingFile, hWriteFile, ImageSize)
        );

    }/* end of else */

fail:
    NvOsFree(buf);
    (void)NvStorMgrFileClose(hWriteFile);
    (void)NvStorMgrFileClose(hStagingFile);
    return e;
}

NvError NvFastbootLoadBootImg(NvU32 StagingPartitionId, NvU8 **pImage,
                                NvU32 BootImgSize, NvU8 *BootImgSignature)
{
    NvError e = NvSuccess;
    char PartName[NVPARTMGR_PARTITION_NAME_LENGTH];
    NvU8 *MessageBuffer = NULL;
    NvU8 *tempImage = NULL;
    NvSHA_Data *SHAData = NULL;
    NvSHA_Hash *SHAHash = NULL;
    NvU32 *PubKeyE = NULL;
    NvU32 bytes = 0, so_far = 0;
    NvStorMgrFileHandle hStagingFile = 0;

    /*  verify the signature of the blob */
    NV_CHECK_ERROR_CLEANUP(
        NvPartMgrGetNameById(StagingPartitionId, PartName)
    );

    *pImage = NvOsAlloc(BootImgSize);
    if (*pImage == NULL)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }
    tempImage = *pImage;

    MessageBuffer = NvOsAlloc(SHA_1_CACHE_BUFFER_SIZE);
    if (MessageBuffer == NULL)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    SHAData = (NvSHA_Data*)NvOsAlloc(sizeof(NvSHA_Data));
    if (SHAData == NULL)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    SHAHash = (NvSHA_Hash*)NvOsAlloc(sizeof(NvSHA_Hash));
    if (SHAHash == NULL)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    PubKeyE = NvOsAlloc(RSANUMBYTES);
    if (PubKeyE == NULL){
        e = NvError_InsufficientMemory;
        goto fail;
    }
    NvOsMemset(PubKeyE, 0, RSANUMBYTES);
    PubKeyE[0] = RSA_PUBLIC_EXPONENT_VAL;

    NV_CHECK_ERROR_CLEANUP(
        NvStorMgrFileOpen(PartName, NVOS_OPEN_READ, &hStagingFile)
    );

    /*  Initializing the SHA-1 Engine */
    if (NvSHA_Init(SHAData) == -1)
    {
        e = NvError_InvalidState;
        goto fail;
    }

    /*  Updating the SHA-1 engine hash value with the blob content */
    while (so_far < BootImgSize)
    {
        NvU32 temp_size = SHA_1_CACHE_BUFFER_SIZE;
        if ((BootImgSize - so_far) < temp_size)
            temp_size = BootImgSize - so_far;

        NV_CHECK_ERROR_CLEANUP(
            NvStorMgrFileRead(hStagingFile, MessageBuffer, temp_size, &bytes)
        );
        if (bytes != temp_size)
        {
            e = NvError_FileReadFailed;
            goto fail;
        }

        NvOsMemcpy(tempImage, MessageBuffer, bytes);
        tempImage += bytes;

        if (NvSHA_Update(SHAData, MessageBuffer, temp_size) == -1)
            goto fail;
        so_far += temp_size;
    }

    /*  Finalizing the SHA-1 engine hash value */
    if (NvSHA_Finalize(SHAData, SHAHash) == -1)
        goto fail;

    /*  Signature of the file has to be verified here   */
    if (NvRSAVerifySHA1Dgst(SHAHash->Hash, &RSAPublicKey, PubKeyE, (NvU32*)BootImgSignature) == -1)
    {
        e = NvError_InvalidState;
        goto fail;
    }

fail:
    NvOsFree(MessageBuffer);
    NvOsFree(PubKeyE);
    NvOsFree(SHAData);
    NvOsFree(SHAHash);
    (void)NvStorMgrFileClose( hStagingFile );
    if (e != NvSuccess)
    {
        NvOsFree(*pImage);
        *pImage= NULL;
    }
    return e;
}

NvError NvInstallBlob(char *PartName)
{
    NvError e = NvSuccess;
    NvU32 bytes;
    NvU32 i;
    NvStorMgrFileHandle hStagingFile = 0;
    UpdateHeader *header = 0;
    NvStorMgrFileHandle hPartFile = 0;
    NvU32 NumEntries = 0;
    ImageEntry *entries = 0;
    ImageEntry *TempEntry = 0;
    NvU32 SizeOfEntry = sizeof(ImageEntry);
    NvU32 FsType = (NvFsMgrFileSystemType)NvFsMgrFileSystemType_Basic;
    NvBlOperatingMode op_mode;

    // Memory Allocations
    header = NvOsAlloc(sizeof(UpdateHeader));
    if (header == NULL)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    NV_CHECK_ERROR_CLEANUP(
        VerifyBlobSignature(PartName)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvStorMgrFileOpen(PartName, NVOS_OPEN_READ, &hStagingFile)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvStorMgrFileSeek(hStagingFile, sizeof(NvSignedUpdateHeader), NvOsSeek_Set)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvStorMgrFileRead(hStagingFile, header, sizeof(UpdateHeader), &bytes)
    );

    if (bytes != sizeof(UpdateHeader))
    {
        e = NvError_FileReadFailed;
        goto fail;
    }

    NumEntries = header->NumEntries;
    entries = NvOsAlloc(NumEntries * SizeOfEntry);
    NV_ASSERT(entries);
    if (!entries)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }
    NV_CHECK_ERROR_CLEANUP(
        NvStorMgrFileSeek(hStagingFile,
                          (header->EntriesOffset)+(sizeof(NvSignedUpdateHeader)),
                          NvOsSeek_Set)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvStorMgrFileRead(hStagingFile, entries, NumEntries * SizeOfEntry, &bytes)
    );

    if (bytes != (NumEntries * SizeOfEntry))
    {
        e = NvError_FileReadFailed;
        goto fail;
    }
    op_mode = NvFuseGetOperatingMode();

    /*  Update the partitions */
    TempEntry = entries;
    for (i = 0; i < NumEntries; i++)
    {
        if (op_mode == NvBlOperatingMode_NvProduction && IS_ENCRYPTED(TempEntry->Version))
        {
            e = NvError_SysUpdateBLUpdateNotAllowed;
            NvOsDebugPrintf("\nUnsupported binary in blob\n");
            goto fail;
        }
        NvOsDebugPrintf("\nStart Updating %s\n",TempEntry->PartName);
        NV_CHECK_ERROR_CLEANUP(
            UpdatePartitionsFromBlob(hStagingFile, TempEntry)
        );
        NvOsDebugPrintf("\nEnd Updating %s\n",TempEntry->PartName);
        TempEntry = TempEntry + 1;
    }

fail:
    NvOsFree(header);
    NvOsFree(s_BlInfo);
    s_BlInfo = NULL;
    s_nBl = 0;
    NvOsFree(entries);
    (void)NvStorMgrFileClose( hStagingFile );
    return e;
}

static NvError GetCommand(NvAbootHandle hAboot, CommandType *Command)
{
    NvError e = NvError_BadParameter;
    char command[32];

    NV_ASSERT(Command);
    if (Command)
    {
        *Command = CommandType_None;
        NV_CHECK_ERROR_CLEANUP(
            NvAbootBootfsReadNBytes(hAboot, "MSC", 0, sizeof(command),
                (NvU8 *)command)
        );

        if (!NvOsStrncmp(command, "boot-recovery", NvOsStrlen("boot-recovery")))
        {
            *Command = CommandType_Recovery;
        }
        else if (!NvOsStrncmp(command, "update", NvOsStrlen("update")))
        {
            *Command = CommandType_Update;
        }
        e= NvSuccess;
    }

fail:
    return e;
}

static NvError
WriteCommandsForBootloader(
    NvAbootHandle hAboot)
{
    NvError e = NvError_BadParameter;
    NvStorMgrFileHandle hFile = 0;
    NvU32 bytes;
    NvU64 Start, Num;
    NvU32 SectorSize;
    NvU8 *temp = 0;
    NvFileSystemIoctl_WriteTagDataDisableInputArgs IoctlArgs;

    NV_CHECK_ERROR_CLEANUP(
        NvAbootGetPartitionParameters(hAboot,
            "MSC", &Start, &Num,
            &SectorSize)
    );
    temp = NvOsAlloc(SectorSize * MISC_PAGES);
    NV_ASSERT(temp);
    if (temp)
    {
        NvOsMemset(temp,0, (SectorSize * MISC_PAGES));

        NV_CHECK_ERROR_CLEANUP(
            NvStorMgrFileOpen( "MSC", NVOS_OPEN_WRITE, &hFile )
        );

        NvOsStrncpy(BootMessage.command, "boot-recovery", sizeof("boot-recovery"));
        NvOsStrncpy(BootMessage.status, s_Status,
            NvOsStrlen(s_Status) > 32 ? 32 : NvOsStrlen(s_Status));

        NvOsMemcpy(&temp[SectorSize * MISC_COMMAND_PAGE], &BootMessage,
            sizeof(BootloaderMessage));

        IoctlArgs.TagDataWriteDisable = NV_TRUE;
        NV_CHECK_ERROR_CLEANUP(NvStorMgrFileIoctl(
            hFile,
            NvFileSystemIoctlType_WriteTagDataDisable,
            sizeof(IoctlArgs),
            0,
            &IoctlArgs,
            NULL));

        NV_CHECK_ERROR_CLEANUP(
            NvStorMgrFileWrite(hFile, temp, (SectorSize * MISC_PAGES), &bytes)
        );
        if (bytes != (SectorSize * MISC_PAGES))
        {
            e = NvError_FileWriteFailed;
            goto fail;
        }
    }
    else
    {
        e = NvError_InsufficientMemory;
    }
fail:
    NvOsFree(temp);
    if (hFile)
        (void)NvStorMgrFileClose( hFile );
    return e;
}

static NvBool CheckStagingUpdate(NvAbootHandle hAboot, char* partName)
{
    NvBool PendingUpdate = NV_FALSE;
    NvError e = NvSuccess;
    NvSignedUpdateHeader *signedHeader =
        (NvSignedUpdateHeader *)NvOsAlloc(sizeof(NvSignedUpdateHeader));

    NV_CHECK_ERROR_CLEANUP(
        NvAbootBootfsReadNBytes(hAboot, partName, 0,
            sizeof(NvSignedUpdateHeader), (NvU8 *)signedHeader)
    );

    if (!NvOsStrncmp((char*)((signedHeader)->MAGIC),
            SIGNED_UPDATE_MAGIC, SIGNED_UPDATE_MAGIC_SIZE) )
        PendingUpdate = NV_TRUE;

fail:
    return PendingUpdate;
}

static void SetStatus(char *status)
{
    NvOsStrncpy(s_Status, status, NvOsStrlen(status));
}
NvError NvCheckIfNonExistWriteRebootFlag(NvAbootHandle hAboot)
{
	 NvError e = NvError_BadParameter;
	 NvRmDeviceHandle hRm = NULL;
	 NvU32 reg = 0;
	  char commands[32];
	  const char * partName = "UDE";
	  NvOsDebugPrintf("NvCheckIfNonExistWriteRebootFlag:: ========#####======>1\r\n");
        
	NvOsDebugPrintf("NvCheckIfNonExistWriteRebootFlag:: commands = %s========#####======>!\r\n",commands);
	
        e = NvAbootBootfsReadNBytes(hAboot, partName, 0, sizeof(commands),
                (NvU8 *)commands);
	if(e== NvSuccess){
		NvOsDebugPrintf("NvCheckIfNonExistWriteRebootFlag:: read success\r\n");
		if (NvOsStrncmp(commands, "rebooting", NvOsStrlen("rebooting")))
        	{
			memset(commands, 0, sizeof(commands));
			NvOsStrncpy(commands, "rebooting", NvOsStrlen("rebooting"));
		 	NV_CHECK_ERROR_CLEANUP( NvAbootBootfsWrite(hAboot, partName, (NvU8 *)(commands),sizeof(commands)));
	        	NvOsDebugPrintf("NvCheckIfNonExistWriteRebootFlag:: write commands = %s========#####======>!\r\n",commands);
        	}
	}else{
		e = NvRmOpen(&hRm, 0);
		if(e== NvSuccess){
	 		/* Reading SCRATCH0 to determine booting mode */
			reg = NV_REGR(hRm, NVRM_MODULE_ID(NvRmModuleID_Pmif, 0),
	        		APBDEV_PMC_SCRATCH0_0);

    			/* Read bit for fastboot */
   	 		reg |= (1 << APBDEV_PMC_SCRATCH0_0_FASTBOOT_FLAG_BIT);
    			/* Read bit for recovery */
    			reg |= (1 << APBDEV_PMC_SCRATCH0_0_RCK_FLAG_BIT);
			NvOsDebugPrintf("NvCheckIfNonExistWriteRebootFlag:: will write val to reg\r\n");
			NV_REGW(hRm, NVRM_MODULE_ID(NvRmModuleID_Pmif, 0),
            			APBDEV_PMC_SCRATCH0_0, reg);
			NvRmClose(hRm);
			hRm = NULL;
		}
	}
	
        
        e= NvSuccess;

fail:
	NvOsDebugPrintf("NvCheckIfNonExistWriteRebootFlag:: fail=======#####======>!\r\n");
	return e;
}
NvError NvCheckForReboot(NvAbootHandle hAboot, CommandType *Command)
{
	 NvError e = NvError_BadParameter;
	 NvError ota_e = NvError_BadParameter;
	 NvRmDeviceHandle hRm = NULL;
	 NvU32 reg = 0;
    	NvU32 EnterFastboot = 0;
    	NvU32 EnterRecoveryKernel = 0;
	NvU32 EneterKeenhiForcedRecovery =0;
	const char * ota_partName = "UDC";
	char ota_commands[4096];
	char *ota_flag ;
	
	 NV_ASSERT(Command);
	  char commands[32*2];
	  const char * partName = "UDE";
    if (Command)
    {
        *Command = CommandType_None;
	
          e = NvAbootBootfsReadNBytes(hAboot, partName, 0, sizeof(commands),
                (NvU8 *)commands);
	   ota_e = NvAbootBootfsReadNBytes(hAboot, ota_partName, 0, sizeof(ota_commands),
	                (NvU8 *)ota_commands);
	   ota_flag = &ota_commands[2048];
	NvOsDebugPrintf("NvCheckForReboot:: commands = %s,EnterFastboot=%d,EnterRecoveryKernel=%d========#####======>!\r\n",commands,EnterFastboot,EnterRecoveryKernel);
	
        if (e== NvSuccess&&!NvOsStrncmp(commands, "rebooting", NvOsStrlen("rebooting")))
        {
        	NvOsDebugPrintf("NvCheckForReboot:: read success\r\n");

        	*Command = CommandType_Rebooting;
		memset(commands, 0, sizeof(commands));
 		NV_CHECK_ERROR_CLEANUP( NvAbootBootfsWrite(hAboot, partName, (NvU8 *)(commands),sizeof(commands)));
        	
        }else if(ota_e== NvSuccess&&!NvOsStrncmp(ota_flag, "ota", NvOsStrlen("ota"))){
        	NvOsDebugPrintf("ota_abc1 = %c %c %c!\r\n",ota_flag[0],ota_flag[1],ota_flag[2]);
		*Command = CommandType_Rebooting;
		ota_flag[0]=ota_flag[1]= ota_flag[2]=0;
	 	NV_CHECK_ERROR_CLEANUP( NvAbootBootfsWrite(hAboot, ota_partName, (NvU8 *)(ota_commands),sizeof(ota_commands)));

		NV_CHECK_ERROR_CLEANUP(
			NvAbootBootfsReadNBytes(hAboot, ota_partName, 0, sizeof(ota_commands),(NvU8 *)ota_commands)
		);
		ota_flag = &ota_commands[2048];
		NvOsDebugPrintf("ota_abc2 = %c %c %c!\r\n",ota_flag[0],ota_flag[1],ota_flag[2]);
			
	}else{
		e = NvRmOpen(&hRm, 0);
		if(e== NvSuccess){
		 	 /* Reading SCRATCH0 to determine booting mode */
			reg = NV_REGR(hRm, NVRM_MODULE_ID(NvRmModuleID_Pmif, 0),
	        		APBDEV_PMC_SCRATCH0_0);
    				/* Read bit for fastboot */
   		 	//EnterFastboot = reg & (1 << APBDEV_PMC_SCRATCH0_0_FASTBOOT_FLAG_BIT);
    			/* Read bit for recovery */
    			//EnterRecoveryKernel = reg & (1 << APBDEV_PMC_SCRATCH0_0_RCK_FLAG_BIT);
		     //  EneterForcedRecovery = reg &  (1 << APBDEV_PMC_SCRATCH0_0_FORCE_RECOVERY_FLAG_BIT);
		       EnterFastboot = (reg >> 30) & 0x01;     //(1 << APBDEV_PMC_SCRATCH0_0_FASTBOOT_FLAG_BIT);
    			/* Read bit for recovery */
    			EnterRecoveryKernel =( reg >> 31) &  0x01; // (1 << APBDEV_PMC_SCRATCH0_0_RCK_FLAG_BIT);
		       EneterKeenhiForcedRecovery = (reg  >> 29)&  0x01;//  (1 << APBDEV_PMC_SCRATCH0_0_KEENHI_REBOOT_FLAG_BITs);
			NvOsDebugPrintf("NvCheckForReboot:0x%x,%d  %d   %d\r\n",reg,EnterFastboot,EnterRecoveryKernel,EneterKeenhiForcedRecovery);
	       	 if(EnterFastboot && EnterRecoveryKernel){

				*Command = CommandType_Rebooting;
		        	/* Clearing fastboot and RCK flag bits */
		        	reg = reg & ~((1 << APBDEV_PMC_SCRATCH0_0_RCK_FLAG_BIT) |
		            		(1 << APBDEV_PMC_SCRATCH0_0_FASTBOOT_FLAG_BIT));
		        	NV_REGW(hRm, NVRM_MODULE_ID(NvRmModuleID_Pmif, 0),
		            	APBDEV_PMC_SCRATCH0_0, reg);        	
				
				NvOsDebugPrintf("NvCheckForReboot:get bootloader and recover resion \r\n");
	        	}else if(EneterKeenhiForcedRecovery){
				*Command = CommandType_RePowerOffReboot;
		        	/* Clearing fastboot and RCK flag bits */
		        	reg = reg & ~((1 << APBDEV_PMC_SCRATCH0_0_KEENHI_REBOOT_FLAG_BIT));
		        	NV_REGW(hRm, NVRM_MODULE_ID(NvRmModuleID_Pmif, 0),
		            	APBDEV_PMC_SCRATCH0_0, reg);	
				NvOsDebugPrintf("APBDEV_PMC_SCRATCH0_0_FORCE_RECOVERY_FLAG_BITs \r\n");
	        	}
			 else if(EnterRecoveryKernel){
				*Command = CommandType_Rebooting;
			}else if(EnterFastboot){
				*Command = CommandType_Rebooting;
			}
			NvRmClose(hRm);
			hRm = NULL;
		 }
        }
		

        e= NvSuccess;
    }
	return e;

fail:
	 e= NvError_BadParameter;
	return e;
}
NvError NvCheckForUpdateAndRecovery(NvAbootHandle hAboot, CommandType *Command)
{
    NvError e = NvError_BadParameter;
    NvU32 StagingPartitionId = -1;
    NvBool StagingUpdate = NV_FALSE;
    char *partName = NULL;
    NV_ASSERT(hAboot);
    NV_ASSERT(Command);
    if (hAboot == 0 || Command == 0)
        goto fail;
    partName = NvOsAlloc(NVPARTMGR_PARTITION_NAME_LENGTH);
    if (partName == NULL)
    {
        goto fail;
    }
    NvOsMemset(partName, 0x0, NVPARTMGR_PARTITION_NAME_LENGTH);
    e = GetCommand(hAboot, Command);
    if (e == NvSuccess)
    {
        /* check for pending update in staging partition "USP"
         */
        if ((*Command) == CommandType_None)
        {
            /* Froyo/GB/ICS does not pass any info about update in the MSC partition
             * so we need to figure it out by reading USP parition
             */
            NvOsStrncpy(partName, "USP", NvOsStrlen("USP"));
            StagingUpdate = CheckStagingUpdate(hAboot, partName);
            if (StagingUpdate == NV_FALSE)
            {
                NvOsStrncpy(partName, "CAC", NvOsStrlen("CAC"));
                StagingUpdate = CheckStagingUpdate(hAboot, partName);
            }
            if (StagingUpdate == NV_TRUE)
            {
                e = NvPartMgrGetIdByName(partName, &StagingPartitionId);
                if (e != NvSuccess)
                {
                    // clear the staging partition
                    NvStorMgrFormat(partName);
                    // continue booting
                    *Command = CommandType_None;
                    e = NvSuccess;
                    goto fail;
                }
                *Command = CommandType_Update;
            }
        }
        if ((*Command) == CommandType_Update)
        {
            e = NvInstallBlob(partName);
            // clear the staging partition
            NvAbootBootfsFormat(partName);
            if( e != NvSuccess)
            {
                NvOsDebugPrintf("\nblob update failed\n");
                NvOsSleepMS(5000);
            }
            NvAbootReset(hAboot);
        }
    }

fail:
    NvOsFree(partName);
    return e;
}


