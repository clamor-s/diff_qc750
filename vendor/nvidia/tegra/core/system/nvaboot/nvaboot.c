/*
 * Copyright (c) 2009-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#include "nvcommon.h"
#include "nvos.h"
#include "nvaboot.h"
#include "nvaboot_private.h"
#include "nvboot_bct.h"
#include "nvbct.h"
#include "nvassert.h"
#include "nvrm_hardware_access.h"
#include "nvrm_memmgr.h"
#include "nvrm_init.h"
#include "nvrm_drf.h"
#include "nvrm_module.h"
#include "nvfsmgr.h"
#include "nvstormgr.h"
#include "nvpartmgr.h"
#include "nvfuse.h"
#include "nvddk_blockdevmgr.h"
#include "nvddk_blockdev.h"
#include "arictlr.h"
#include "arapb_misc.h"
#include "arfic_dist.h"
#include "arpl310.h"
#include "nvrm_arm_cp.h"
#include "nvodm_query.h"
#include "nvodm_services.h"
#include "nvbootargs.h"
#include "nvddk_aes_blockdev_defs.h"
#include "nvddk_aes_blockdev.h"
#include "nvddk_fuse.h"
#include "arclk_rst.h"
#include "arfuse.h"
#include "nvaboot_sanitize_keys.h"
#if HAS_SE_CONTROLLER_INSTANCE
#include "nvddk_se_blockdev.h"
#endif

#include "nv3p_transport.h"
#include "nvddk_usbf.h"
#include "nvrm_power_private.h"
#include "crc32.h"

#define MAX_NUM_IMAGES 4
#define MAX_NUM_REGISTERS 4
#define OVERLAP_MASK ((1UL<<MAX_NUM_IMAGES)-1)
#define BUFFER_SIZE_BYTES 0x140
#define AES_ENCRYPTION NV_TRUE
#define AES_DECRYPTION NV_FALSE
#define AES_KEY_LENGTH_BYTES   16
#define AES_IV_LENGTH_BYTES    16
#define AES_TEXT_MSG_LENGTH_BYTES   16
#define DEDICATED_SLOT NV_TRUE
#define AES_TEGRA_ALL_REVS (~0ul)
#define RESERVED_SIZE_FROM_TOP (32*1024*1024)

static struct NvAbootRec s_Aboot = { 0, NULL, NvRmModuleID_Invalid, 0,
                                {0,0}, NULL, NULL, 0, NV_FALSE };
static NvError AbootPrivLoadPartitionTable(
    NvDdkBlockDevMgrDeviceId DdkDeviceId,
    NvU32 DevInstance,
    const NvBctAuxInfo *pAuxInfo);
static NvError AbootPrivGetOsInfo(NvBctAuxInfo *pAuxInfo);
static NvDdkBlockDevMgrDeviceId AbootPrivGetSecBootDevId(NvU32 * pInstance);
static NvError AbootPrivGetChipId(
    NvRmDeviceHandle RmHandle,
    NvU32 *ChipId);
static NvBool NvAbootChipCompare(
    const NvRmDeviceHandle hRmDevice,
    const NvU32 Chip,
    const NvU32 MajorRev,
    const NvU32 MinorRev);
static NvBool NvAbootIsAP20A03pChip(const NvRmDeviceHandle hRmDevice);

/* Memory Layout DB: tracks the base/size of the carveouts
 * (for example, for SecureOS and VPR).
 */
typedef struct NvAbootLayoutRec
{
    NvUPtr Base;
    NvU64 Size;
} NvAbootMemLayoutRec;
static NvAbootMemLayoutRec NvAbootMemLayout[NvAbootMemLayout_Num];

/* Although it is really confusing to see NvBl show up here, without any
 * makefile dependencies on NvBl, this function is actually implemented by
 * AOS' bootloader code in exactly the same fashion as the NvBl code. */
extern NvBool NvBlQueryGetNv3pServerFlag(void);

extern NV_NAKED NvU32 NvAbootPrivVaToPa(void *va);

extern NV_NAKED void NvAbootPrivDoJump(NvU32 *pKernelRegs,
    NvU32 NumKernelRegs, NvUPtr KernelStartAddr);

extern NV_NAKED void NvAbootPrivSanitizeUniverse(NvAbootHandle hAboot);

#ifdef CONFIG_TRUSTED_FOUNDATIONS
#define TF_EKS_PARTITION_LEN            (81200)
#define TF_EKS_NAME                     "EKS"       /* Encrypted Key Store */
#define TF_EKS_MAGIC_STR                "NVEKSP"    /* EKS Partition Magic ID */
#define TF_EKS_MAGIC_ID_LEN             (7+1)       /* EKSP MAGIC ID LEN */

typedef struct {
   char     szMagicID[TF_EKS_MAGIC_ID_LEN];
   NvU32    nNumOfEncrypedKeys;
   NvU8     data[4];
}TFEKS_TYPE;

extern NV_NAKED void NvAbootPrivDoJump_TF( NvU32 *pKernelRegs,
                            NvU32 NumKernelRegs, TFSW_PARAMS *pTfswParams);
extern NvU32 SOSKeyIndexOffset;
#endif

#ifdef LPM_BATTERY_CHARGING
void NvAbootClocksInterface(NvAbootClocksId Id, NvRmDeviceHandle hRm)
{

    NvU32 ChipId;

    if (AbootPrivGetChipId(hRm, &ChipId)!=NvSuccess)
        NV_ABOOT_BREAK();

    switch (ChipId)
    {
        case 0x20:
            /* Not Implemented */
            break;
        case 0x30:
            NvAbootT30Clocks(Id, hRm);
            break;
        default:
            NV_ABOOT_BREAK();
    }
}
#endif

/**
 * NvAbootPrivGicDisableInterrupts
 *   Disables all interrupt sources at the interrupt controller prior to
 *   changing OS contexts
 */

void AbootPrivGicDisableInterrupts(NvAbootHandle hAboot)
{
    NvU32        Num;
    NvRmPhysAddr Base;
    NvU32        Len;
    NvU8        *pCtlr;
    NvU32        i;

    NvRmModuleGetBaseAddress(hAboot->hRm,
         NVRM_MODULE_ID(NvRmPrivModuleID_ArmPerif,0), &Base, &Len);
    if (NvRmPhysicalMemMap(Base, Len, NVOS_MEM_READ_WRITE,
            NvOsMemAttribute_Uncached, (void **)&pCtlr)==NvSuccess)
    {
        Num = NV_READ32(pCtlr + FIC_DIST_IC_TYPE_0);
        Num = NV_DRF_VAL(FIC_DIST, IC_TYPE, IT_LINES_NUMBER, Num);
        for (i=0; i<=Num; i++)
            NV_WRITE32(pCtlr + (FIC_DIST_ENABLE_CLEAR_0_0 + (i*4)), ~0UL);
    }
}

//  This just computes a constant value given MAX_NUM_IMAGES, where the first
//  bit of every MAX_NUM_IMAGES bits is 1, and others are 0 (e.g.,
//  MAX_NUM_IMAGES==4 will return 0x1111).  It would be nice if there was a
//  preprocess bit-twiddling trick to compute this, but I can't
static NvU32 ComputePerSrcOverlapMask(void)
{
    NvU32 Mask = 0;
    NvU32 i;

    for (i=0; i<MAX_NUM_IMAGES; i++)
        Mask |= (1<<(i*MAX_NUM_IMAGES));

    return Mask;
}

static NvU32 ComputeOverlapVector(
    NvUPtr       *pDsts,
    void        **pSrcs,
    NvU32        *pSizes,
    NvU32         NumImages)
{
    NvU32 i, j;
    NvU32 OverlapMask = 0;
    for (i=0; i<NumImages; i++)
    {
        if (!pSizes[i])
            continue;

        for (j=0; j<NumImages; j++)
        {
            NvUPtr SrcPa;
            NvUPtr DstPa = pDsts[i];
            NvU32  Len = pSizes[i];

            if (j==i || !pSizes[j])
                continue;

            SrcPa = NvAbootPrivVaToPa(pSrcs[j]);
            if (!SrcPa)
            {
                // Address not valid.
                NV_ASSERT(!"Address not valid");
                continue;
            }
            if ((DstPa>=SrcPa && DstPa<SrcPa+Len) ||
                (DstPa+Len>=SrcPa && DstPa+Len<SrcPa+Len))
                OverlapMask |= (1<<(MAX_NUM_IMAGES*i + j));
        }
    }

    return OverlapMask;
}

static NvError NvAbootPrivCopyImages(
    NvAbootHandle hAboot,
    NvUPtr       *pDsts,
    void        **pSrcs,
    NvU32        *pSizes,
    NvU32         NumImages)
{
    NvU32 i;
    NvU32 OverlapMask = 0;
    const NvU32 BitMask = ComputePerSrcOverlapMask();
    NvBool Copied = NV_FALSE;
    NvU32 attempts = 0;

    NV_ASSERT(NumImages <= MAX_NUM_IMAGES);

    if (!NumImages)
        return NvError_BadParameter;

    /*  If there is insufficient carveout memory for resolving circular
     *  staging dependencies (in the worst-case, 3 of the 4 copied images must
     *  fit in carveout), this function will infinite loop. */
    do
    {
        OverlapMask = ComputeOverlapVector(pDsts, pSrcs, pSizes, NumImages);

        /*  Copy all source images whose destination does not overlap any other
         *  source image first.  Once an image is copied, clear out its source
         *  overlap bits for every other copy, since the source image can be
         *  safely overwritten by a subsequent copy.  Repeat as long as images
         *  are being copied (this is equivalent to sorting the copies based
         *  on destination address overlap). */
        do
        {
            Copied = NV_FALSE;
            for (i=0; i<NumImages; i++)
            {
                if (pSizes[i] &&
                    !(OverlapMask&(OVERLAP_MASK<<(MAX_NUM_IMAGES*i))))
                {
                    void* pDst = (void*)pDsts[i];
                    /* FIXME:  AOS NvOsPhysicalMemMap doesn't seem to work
                     * here.
                    NV_ASSERT_SUCCESS(
                        NvOsPhysicalMemMap(pDsts[i], pSizes[i], 0,
                            NVOS_MEM_READ_WRITE, &pDst)
                    );*/
                    NvOsMemmove(pDst, pSrcs[i], pSizes[i]);
                    OverlapMask &= ~(BitMask<<i);
                    pSizes[i] = 0;
                    Copied = NV_TRUE;
                }
            }
        } while (Copied);

        /* For each uncopied source image, try to allocate a staging buffer
         * in carveout memory for the copy. */
        for (i=0; i<NumImages && OverlapMask; i++)
        {
            NvRmHeap heap = NvRmHeap_ExternalCarveOut;
            if (OverlapMask & (BitMask<<i) && pSizes[i])
            {
                NvRmMemHandle mh;
                void *pMem;
                if (NvRmMemHandleCreate(hAboot->hRm, &mh, pSizes[i])!=NvSuccess)
                    continue;

                if (NvRmMemAllocTagged(mh, &heap, 1, 32,
                        NvOsMemAttribute_WriteCombined,
                        NVRM_MEM_TAG_SYSTEM_MISC)!=NvSuccess)
                {
                    NvRmMemHandleFree(mh);
                    continue;
                }

                if (NvRmMemMap(mh, 0, pSizes[i], NVOS_MEM_READ_WRITE,
                        &pMem)!=NvSuccess)
                {
                    NvRmMemHandleFree(mh);
                    continue;
                }
                NvOsMemcpy(pMem, pSrcs[i], pSizes[i]);
                NvOsFlushWriteCombineBuffer();
                pSrcs[i] = pMem;
            }
        }
        attempts++;
    } while (OverlapMask && attempts<MAX_NUM_IMAGES*MAX_NUM_IMAGES);

    return (OverlapMask) ? NvError_NotSupported : NvSuccess;
}

extern NvError Nv3pServer(void);

NvError NvAboot3pServer(NvAbootHandle hAboot)
{
    return Nv3pServer();
}

NvError NvAbootPrivSaveSdramParams(NvAbootHandle hAboot)
{
    NvU32 ChipId;
    NvError e;

    if (AbootPrivGetChipId(hAboot->hRm, &ChipId)!=NvSuccess)
        NV_ABOOT_BREAK();

    switch (ChipId)
    {
    case 0x20:
        e = NvAbootPrivAp20SaveSdramParams(hAboot);
        break;
    case 0x30:
        e = NvAbootPrivT30SaveSdramParams(hAboot);
        break;
    default:
        NV_ABOOT_BREAK();
    }

    return e;
}

static NvU8 *s_SrcBuffer = NULL;
static NvU8 *s_EcryptBuffer = NULL;
static NvU8 *s_DecryptBuffer = NULL;
static NvU8 s_ZeroKey[NvDdkAesConst_MaxKeyLengthBytes] = {0};

typedef struct AesTestVectorData_Rec
{
    NvU8 Key[AES_KEY_LENGTH_BYTES];
    NvU8 Iv[AES_IV_LENGTH_BYTES];
    NvU8 PlainText[AES_TEXT_MSG_LENGTH_BYTES];
    NvU8 CipherText[AES_TEXT_MSG_LENGTH_BYTES];
    NvU8 NofBlocks;
} AesTestVectorData;

static AesTestVectorData s_ZeroKeyVectorKeySchedulerSupported =
{
    {   // 128-bit Key
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    },
    {   // 128-bit IV
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    },
    {   // Plain Text
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    },
    {   // Cipher text
        0x7A, 0xCA, 0x0F, 0xD9, 0xBC, 0xD6, 0xEC, 0x7C,
        0x9F, 0x97, 0x46, 0x66, 0x16, 0xE6, 0xA2, 0x82
    },
    // Number of AES blocks in the Text
    1
};

static void InitializeBuffers(NvU8 *Buffer, NvU32 Size)
{
    NvU32 i;
    for (i=0; i<Size; i++)
        Buffer[i]=0;
}

static NvError AesSelectKeySlot(
    NvDdkBlockDevHandle hAesBlockDev,
    NvDdkAesKeyType KeyType,
    NvBool IsDedicatedKeySlot,
    NvU8 * pKeyData)
{
    NvError e;
    NvDdkAesBlockDevIoctl_SelectKeyInputArgs Key;

    NvOsMemset(&Key, 0, sizeof(NvDdkAesBlockDevIoctl_SelectKeyInputArgs));

    Key.IsDedicatedKeySlot = IsDedicatedKeySlot;
    Key.KeyLength = NvDdkAesKeySize_128Bit;
    Key.KeyType = KeyType;
    if (KeyType == NvDdkAesKeyType_UserSpecified)
        NvOsMemcpy(Key.Key, pKeyData, NvDdkAesKeySize_128Bit);

    e =  hAesBlockDev->NvDdkBlockDevIoctl(
            hAesBlockDev,
            NvDdkAesBlockDevIoctlType_SelectKey,
            sizeof(NvDdkAesBlockDevIoctl_SelectKeyInputArgs),
            0,
            &Key,
            NULL);

    NV_CHECK_ERROR(e);
    if (KeyType == NvDdkAesKeyType_UserSpecified)
    {
        NvOsMemset(&Key, 0, sizeof(NvDdkAesBlockDevIoctl_SelectKeyInputArgs));
    }
    return e;
}

static NvBool DataCompare(NvU8 *SrcBuffer, NvU8 *DstBuffer, NvU32 Size, NvBool PrintDiff)
{
    NvU32 i = 0;

    while ((i < Size) && (SrcBuffer[i] == DstBuffer[i]))
        i++;
    if (i == Size)
        return NV_FALSE;
    else
        return NV_TRUE;
}

static void GenerateData(NvU8 *Buffer, NvU32 Size)
{
    NvU32 i;
    for (i=0; i<Size; i++)
    {
        Buffer[i]= (i%0xFF);
    }
}

static NvError AesSelectOperation(
    NvDdkBlockDevHandle hAesBlockDev,
    NvDdkAesOperationalMode OpMode,
    NvBool IsEncryption)
{
    NvDdkAesBlockDevIoctl_SelectOperationInputArgs Operation;
    NvOsMemset(&Operation, 0, sizeof(NvDdkAesBlockDevIoctl_SelectOperationInputArgs));

    Operation.IsEncrypt = IsEncryption;
    Operation.OpMode = OpMode;

    return hAesBlockDev->NvDdkBlockDevIoctl(
        hAesBlockDev,
        NvDdkAesBlockDevIoctlType_SelectOperation,
        sizeof(NvDdkAesBlockDevIoctl_SelectOperationInputArgs),
        0,
        &Operation,
        NULL);
}

static NvError AesProcessBuffer(
    NvDdkBlockDevHandle hAesBlockDev,
    NvU32 BufferSize,
    NvU8 *SrcBuffer,
    NvU8 *DestBuffer,
    NvU32 SkipOffset)
{
    NvDdkAesBlockDevIoctl_ProcessBufferInputArgs ProcessBuffer;

    NvOsMemset(&ProcessBuffer, 0, sizeof(NvDdkAesBlockDevIoctl_ProcessBufferInputArgs));
    ProcessBuffer.BufferSize = BufferSize;
    ProcessBuffer.SrcBuffer = SrcBuffer;
    ProcessBuffer.DestBuffer = DestBuffer;
    ProcessBuffer.SkipOffset = SkipOffset;
    return hAesBlockDev->NvDdkBlockDevIoctl(
            hAesBlockDev,
            NvDdkAesBlockDevIoctlType_ProcessBuffer,
            sizeof(NvDdkAesBlockDevIoctl_ProcessBufferInputArgs),
            0,
            &ProcessBuffer,
            NULL);
}

static void
AesFreeUpResources(
    NvDdkBlockDevHandle hAesBlockDev
    )
{
    hAesBlockDev->NvDdkBlockDevClose(hAesBlockDev);
    NvDdkAesBlockDevDeinit();
}

static NvError AesSetIV(NvDdkBlockDevHandle hAesBlockDev, NvU8 *InitVector)
{
    NvDdkAesBlockDevIoctl_SetInitialVectorInputArgs ProcessBuffer;
    NvOsMemcpy(ProcessBuffer.InitialVector, InitVector, NvDdkAesConst_IVLengthBytes);

    return hAesBlockDev->NvDdkBlockDevIoctl(
        hAesBlockDev,
        NvDdkAesBlockDevIoctlType_SetInitialVector,
        sizeof(NvDdkAesBlockDevIoctl_SetInitialVectorInputArgs),
        0,
        &ProcessBuffer,
        NULL);
}

static NvError
AesPerformOperation(
    NvDdkBlockDevHandle hAesBlockDev,
    NvBool IsEncryption,
    NvBool IsDedicatedKeySlot,
    NvDdkAesKeyType KeyType,
    NvU8 *pKeyData,
    NvU8 *pIV,
    NvU32 BufferSize,
    NvU8 *IpBuf,
    NvU8 *OpBuf)
{
    NvError e;
    NV_CHECK_ERROR (
        AesSelectOperation(hAesBlockDev,NvDdkAesOperationalMode_Cbc,IsEncryption)
    );

    // Select the key slot for Encryption
    NV_CHECK_ERROR (
        AesSelectKeySlot(
            hAesBlockDev,
            KeyType,
            IsDedicatedKeySlot,
            pKeyData)
    );

    if (pIV)
    {
        NV_CHECK_ERROR(
            AesSetIV(
            hAesBlockDev,
            pIV)
        );
    }

    NV_CHECK_ERROR(
        AesProcessBuffer(
            hAesBlockDev,
            BufferSize,
            IpBuf,
            OpBuf,
            0)
    );

    return e;
}

static NvBool AesClearSBKTest()
{
    NvError e;
    NvRmDeviceHandle hRmDevice = NULL;
    NvDdkBlockDevHandle hAesBlockDev;
    NvU32 NumberOfBlocks = s_ZeroKeyVectorKeySchedulerSupported.NofBlocks;
    NvU32 BufferSize = NumberOfBlocks * NvDdkAesConst_BlockLengthBytes;
    NvBool ret = NV_FALSE;
    NvBool ControlWithSe = NV_FALSE;
#if HAS_SE_CONTROLLER_INSTANCE
    NvDdkSeAesOperation OpInfo;
    NvDdkSeAesKeyInfo   KeyInfo;
    NvDdkSeAesSetIvInfo SetIvInfo;
    NvDdkSeAesProcessBufferInfo PbInfo;
#endif
    InitializeBuffers(s_SrcBuffer, BUFFER_SIZE_BYTES);
    InitializeBuffers(s_EcryptBuffer, BUFFER_SIZE_BYTES);
    InitializeBuffers(s_DecryptBuffer, BUFFER_SIZE_BYTES);

    // get rm handle
    NV_CHECK_ERROR_CLEANUP(NvRmOpen(&hRmDevice, 0));
    // Initialize AES engine
    NV_CHECK_ERROR_CLEANUP(NvDdkAesBlockDevInit(hRmDevice));
    // Open AES Block Dev
    NV_CHECK_ERROR_CLEANUP(NvDdkAesBlockDevOpen(0,0, &hAesBlockDev));

    NV_CHECK_ERROR_CLEANUP(
        AesPerformOperation(
            hAesBlockDev,
            AES_ENCRYPTION,
            DEDICATED_SLOT,
            NvDdkAesKeyType_SecureBootKey,
            NULL,
            s_ZeroKey,
            BufferSize,
            s_ZeroKeyVectorKeySchedulerSupported.PlainText,
            s_EcryptBuffer
        )
    );

    if (DataCompare(
            s_EcryptBuffer,
            s_ZeroKeyVectorKeySchedulerSupported.CipherText,
            BufferSize,
            NV_FALSE))
        goto fail;

    NV_CHECK_ERROR_CLEANUP(
        AesPerformOperation(
            hAesBlockDev,
            AES_DECRYPTION,
            DEDICATED_SLOT,
            NvDdkAesKeyType_SecureBootKey,
            NULL,
            s_ZeroKey,
            BufferSize,
            s_ZeroKeyVectorKeySchedulerSupported.CipherText,
            s_DecryptBuffer
        )
    );

    if (DataCompare(s_DecryptBuffer,
        s_ZeroKeyVectorKeySchedulerSupported.PlainText, BufferSize, NV_FALSE))
        goto fail;
    AesFreeUpResources(hAesBlockDev);
    ControlWithSe = NV_TRUE;
#if HAS_SE_CONTROLLER_INSTANCE
    /* Open SE BlockDevice */
    NV_CHECK_ERROR_CLEANUP(NvDdkSeBlockDevOpen(0, 0, &hAesBlockDev));
    /* Select Operation */
    OpInfo.OpMode = NvDdkSeAesOperationalMode_Cbc;
    OpInfo.IsEncrypt = NV_TRUE;
    NV_CHECK_ERROR_CLEANUP(
            hAesBlockDev->NvDdkBlockDevIoctl(
                hAesBlockDev,
                NvDdkSeAesBlockDevIoctlType_SelectOperation,
                sizeof(NvDdkSeAesOperation),
                0,
                (const void *)&OpInfo,
                NULL));
    /* Select the key */
    KeyInfo.KeyType = NvDdkSeAesKeyType_SecureBootKey;
    KeyInfo.KeyLength = NvDdkSeAesKeySize_128Bit;
    NV_CHECK_ERROR_CLEANUP(
            hAesBlockDev->NvDdkBlockDevIoctl(
                hAesBlockDev,
                NvDdkSeAesBlockDevIoctlType_SelectKey,
                sizeof(NvDdkSeAesKeyInfo),
                0,
                (const void *)&KeyInfo,
                NULL));
    SetIvInfo.pIV = s_ZeroKey;
    SetIvInfo.VectorSize = 16;
    NV_CHECK_ERROR_CLEANUP(
            hAesBlockDev->NvDdkBlockDevIoctl(
                hAesBlockDev,
                NvDdkSeAesBlockDevIoctlType_SetIV,
                sizeof(NvDdkSeAesSetIvInfo),
                0,
                (const void *)&SetIvInfo,
                NULL));
    /* process buffer */
    PbInfo.pSrcBuffer = s_ZeroKeyVectorKeySchedulerSupported.PlainText;
    PbInfo.pDstBuffer = s_DecryptBuffer;
    PbInfo.SrcBufSize = BufferSize;
    NV_CHECK_ERROR_CLEANUP(
            hAesBlockDev->NvDdkBlockDevIoctl(
                hAesBlockDev,
                NvDdkSeAesBlockDevIoctlType_ProcessBuffer,
                sizeof(NvDdkSeAesProcessBufferInfo),
                0,
                (const void *)&PbInfo,
                NULL));
    if (DataCompare(s_DecryptBuffer,
                s_ZeroKeyVectorKeySchedulerSupported.CipherText, BufferSize, NV_TRUE))
        goto fail;
#endif
    ret = NV_TRUE;
fail:
    if (!ControlWithSe)
        AesFreeUpResources(hAesBlockDev);
    else
    {
#if HAS_SE_CONTROLLER_INSTANCE
        hAesBlockDev->NvDdkBlockDevClose(hAesBlockDev);
#endif
    }
    NvRmClose(hRmDevice);
    return ret;
}

static NvBool AesLocksskTest()
{
    NvError e;
    NvRmDeviceHandle hRmDevice = NULL;
    NvDdkBlockDevHandle hAesBlockDev;
    NvU32 NumberOfBlocks = 20;
    NvU32 BufferSize = NumberOfBlocks * NvDdkAesConst_BlockLengthBytes;
    NvU8 *TestBuf = 0;
    NvDdkAesBlockDevIoctl_SetAndLockSecureStorageKeyInputArgs setandlock;
    NvU8 Key[NvDdkAesKeySize_128Bit];
    NvU32 i;
    NvU32 PhyAddress;
    NvRmMemHandle TestHandle = NULL;
    NvBool ret = NV_FALSE;
    NvBool ControlWithSe = NV_FALSE;
#if HAS_SE_CONTROLLER_INSTANCE
    NvDdkSeAesOperation OpInfo;
    NvDdkSeAesSsk SskInfo;
    NvDdkSeAesSetIvInfo SetIvInfo;
    NvDdkSeAesKeyInfo   KeyInfo;
    NvDdkSeAesProcessBufferInfo PbInfo;
#endif
    //Opening Device
    NV_CHECK_ERROR_CLEANUP(NvRmOpen(&hRmDevice, 0));
    NV_CHECK_ERROR_CLEANUP(
        NvRmMemHandleCreate(hRmDevice, &TestHandle, BUFFER_SIZE_BYTES)
    );
    NV_CHECK_ERROR_CLEANUP(
        NvRmMemAllocTagged(TestHandle, 0, 0, 4, NvOsMemAttribute_Uncached,
                NVRM_MEM_TAG_SYSTEM_MISC)
    );

    PhyAddress = NvRmMemPin(TestHandle);
    if(PhyAddress == 0xFFFFFFFF)
        goto fail;
    NV_CHECK_ERROR_CLEANUP(
        NvRmMemMap(
            TestHandle,
            0,
            BUFFER_SIZE_BYTES,
            NVOS_MEM_READ_WRITE,
            (void **)&TestBuf
        )
    );

    NvOsMemset(Key, 0, NvDdkAesKeySize_128Bit);

    InitializeBuffers(s_SrcBuffer, BUFFER_SIZE_BYTES);
    InitializeBuffers(s_EcryptBuffer, BUFFER_SIZE_BYTES);
    InitializeBuffers(s_DecryptBuffer, BUFFER_SIZE_BYTES);
    InitializeBuffers(TestBuf, BUFFER_SIZE_BYTES);

    NV_CHECK_ERROR_CLEANUP(NvDdkAesBlockDevInit(hRmDevice));

    NV_CHECK_ERROR_CLEANUP(NvDdkAesBlockDevOpen(0,0, &hAesBlockDev));

    //Fill Source buffer with Known Data
    GenerateData(s_SrcBuffer, BufferSize);

    NV_CHECK_ERROR_CLEANUP(
        AesPerformOperation(
            hAesBlockDev,
            AES_ENCRYPTION,
            DEDICATED_SLOT,
            NvDdkAesKeyType_SecureStorageKey,
            NULL,
            s_ZeroKey,
            BufferSize,
            s_SrcBuffer,
            s_EcryptBuffer
        )
    );

    NV_CHECK_ERROR_CLEANUP(
        AesPerformOperation(
            hAesBlockDev,
            AES_ENCRYPTION,
            DEDICATED_SLOT,
            NvDdkAesKeyType_UserSpecified,
            s_ZeroKey,
            NULL,
            BufferSize,
            s_SrcBuffer,
            TestBuf)
    );

    if (!DataCompare(s_EcryptBuffer, TestBuf, BufferSize, NV_FALSE))
    {
        for (i=0; i<NvDdkAesKeySize_128Bit; i++)
        {
            Key[i] = ~Key[i];
        }
    }

    NV_CHECK_ERROR_CLEANUP(
        AesPerformOperation(
            hAesBlockDev,
            AES_DECRYPTION,
            DEDICATED_SLOT,
            NvDdkAesKeyType_SecureStorageKey,
            NULL,
            s_ZeroKey,
            BufferSize,
            s_SrcBuffer,
            s_DecryptBuffer
        )
    );
    setandlock.KeyLength = NvDdkAesKeySize_128Bit;
    NvOsMemcpy(setandlock.Key, Key, NvDdkAesKeySize_128Bit);

    NV_CHECK_ERROR_CLEANUP(
        hAesBlockDev->NvDdkBlockDevIoctl(
            hAesBlockDev,
            NvDdkAesBlockDevIoctlType_SetAndLockSecureStorageKey,
            sizeof(NvDdkAesBlockDevIoctl_SetAndLockSecureStorageKeyInputArgs),
            0,
            &setandlock,
            NULL
        )
    );

    NV_CHECK_ERROR_CLEANUP(
        AesPerformOperation(
            hAesBlockDev,
            AES_ENCRYPTION,
            DEDICATED_SLOT,
            NvDdkAesKeyType_SecureStorageKey,
            NULL,
            s_ZeroKey,
            BufferSize,
            s_SrcBuffer,
            TestBuf
        )
    );

    if (DataCompare(TestBuf, s_EcryptBuffer, BufferSize, NV_FALSE))
        goto fail;

    NV_CHECK_ERROR_CLEANUP(AesPerformOperation(
        hAesBlockDev,
        AES_DECRYPTION,
        DEDICATED_SLOT,
        NvDdkAesKeyType_SecureStorageKey,
        NULL,
        s_ZeroKey,
        BufferSize,
        s_SrcBuffer,
        TestBuf
        )
    );

    if (DataCompare(TestBuf, s_DecryptBuffer, BufferSize, NV_FALSE))
        goto fail;
    AesFreeUpResources(hAesBlockDev);
    ControlWithSe = NV_TRUE;
#if HAS_SE_CONTROLLER_INSTANCE
    NvOsMemset(Key, 0, NvDdkAesKeySize_128Bit);
    /* Open Se engine */
    NV_CHECK_ERROR_CLEANUP(NvDdkSeBlockDevOpen(0,0, &hAesBlockDev));
    /* select Operation */
    OpInfo.OpMode = NvDdkSeAesOperationalMode_Cbc;
    OpInfo.IsEncrypt = NV_TRUE;
    NV_CHECK_ERROR_CLEANUP(
            hAesBlockDev->NvDdkBlockDevIoctl(
                hAesBlockDev,
                NvDdkSeAesBlockDevIoctlType_SelectOperation,
                sizeof(NvDdkSeAesOperation),
                0,
                (const void *)&OpInfo,
                NULL));
    /* Set key */
    KeyInfo.KeyType =   NvDdkSeAesKeyType_SecureStorageKey;
    KeyInfo.KeyLength = NvDdkSeAesKeySize_128Bit;
    NV_CHECK_ERROR_CLEANUP(
            hAesBlockDev->NvDdkBlockDevIoctl(
                hAesBlockDev,
                NvDdkSeAesBlockDevIoctlType_SelectKey,
                sizeof(NvDdkSeAesKeyInfo),
                0,
                (const void *)&KeyInfo,
                NULL));
    /* Set Iv */
    SetIvInfo.VectorSize = NvDdkSeAesConst_IVLengthBytes;
    SetIvInfo.pIV = s_ZeroKey;
    NV_CHECK_ERROR_CLEANUP(
            hAesBlockDev->NvDdkBlockDevIoctl(
                hAesBlockDev,
                NvDdkSeAesBlockDevIoctlType_SetIV,
                sizeof(NvDdkSeAesSetIvInfo),
                0,
                (const void *)&SetIvInfo,
                NULL));
    /* process buffer */
    PbInfo.pSrcBuffer = s_SrcBuffer;
    PbInfo.pDstBuffer = s_EcryptBuffer;
    PbInfo.SrcBufSize = BufferSize;
    NV_CHECK_ERROR_CLEANUP(
            hAesBlockDev->NvDdkBlockDevIoctl(
                hAesBlockDev,
                NvDdkSeAesBlockDevIoctlType_ProcessBuffer,
                sizeof(NvDdkSeAesProcessBufferInfo),
                0,
                (const void *)&PbInfo,
                NULL));
    /* Get the Output with Zero key And Zero Iv*/
    /* Set IV */
    SetIvInfo.pIV = s_ZeroKey;
    SetIvInfo.VectorSize = NvDdkSeAesConst_IVLengthBytes;
    NV_CHECK_ERROR_CLEANUP(
            hAesBlockDev->NvDdkBlockDevIoctl(
                hAesBlockDev,
                NvDdkSeAesBlockDevIoctlType_SetIV,
                sizeof(NvDdkSeAesSetIvInfo),
                0,
                (const void *)&SetIvInfo,
                NULL));
    /* Set Key */
    KeyInfo.KeyType =   NvDdkSeAesKeyType_UserSpecified;
    NvOsMemcpy(KeyInfo.Key, s_ZeroKey, NvDdkSeAesKeySize_128Bit);
    NV_CHECK_ERROR_CLEANUP(
            hAesBlockDev->NvDdkBlockDevIoctl(
                hAesBlockDev,
                NvDdkSeAesBlockDevIoctlType_SelectKey,
                sizeof(NvDdkSeAesKeyInfo),
                0,
                (const void *)&KeyInfo,
                NULL));
    PbInfo.pDstBuffer = s_DecryptBuffer;
    NV_CHECK_ERROR_CLEANUP(
            hAesBlockDev->NvDdkBlockDevIoctl(
                hAesBlockDev,
                NvDdkSeAesBlockDevIoctlType_ProcessBuffer,
                sizeof(NvDdkSeAesProcessBufferInfo),
                0,
                (const void *)&PbInfo,
                NULL));
    if (!DataCompare(s_DecryptBuffer, s_EcryptBuffer, BufferSize, NV_TRUE))
    {
        /* change the key */
        for (i = 0; i < NvDdkSeAesKeySize_128Bit; i++)
        {
            Key[i] = ~Key[i];
        }
    }
    /* Set and lock Ssk */
    SskInfo.pSsk = Key;
    NV_CHECK_ERROR_CLEANUP(
            hAesBlockDev->NvDdkBlockDevIoctl(
                hAesBlockDev,
                NvDdkSeAesBlockDevIoctlType_SetAndLockSecureStorageKey,
                sizeof(NvDdkSeAesSsk),
                0,
                (const void *)&SskInfo,
                NULL));
    /* Set the key */
    KeyInfo.KeyType = NvDdkSeAesKeyType_SecureStorageKey;
    NV_CHECK_ERROR_CLEANUP(
            hAesBlockDev->NvDdkBlockDevIoctl(
                hAesBlockDev,
                NvDdkSeAesBlockDevIoctlType_SelectKey,
                sizeof(NvDdkSeAesKeyInfo),
                0,
                (const void *)&KeyInfo,
                NULL));
    /* Set Iv */
    SetIvInfo.pIV = s_ZeroKey;
    SetIvInfo.VectorSize = 16;
    NV_CHECK_ERROR_CLEANUP(
            hAesBlockDev->NvDdkBlockDevIoctl(
                hAesBlockDev,
                NvDdkSeAesBlockDevIoctlType_SetIV,
                sizeof(NvDdkSeAesSetIvInfo),
                0,
                (const void *)&SetIvInfo,
                NULL));
    /* Process Buffer */
    PbInfo.pDstBuffer = TestBuf;
    NV_CHECK_ERROR_CLEANUP(
            hAesBlockDev->NvDdkBlockDevIoctl(
                hAesBlockDev,
                NvDdkSeAesBlockDevIoctlType_ProcessBuffer,
                sizeof(NvDdkSeAesProcessBufferInfo),
                0,
                (const void *)&PbInfo,
                NULL));
    if (DataCompare(TestBuf, s_EcryptBuffer, BufferSize, NV_TRUE))
    {
        goto fail;
    }
#endif
    ret = NV_TRUE;
fail:
    if (!ControlWithSe)
        AesFreeUpResources(
                hAesBlockDev
                );
    else
    {
#if HAS_SE_CONTROLLER_INSTANCE
        hAesBlockDev->NvDdkBlockDevClose(hAesBlockDev);
#endif
    }
    NvRmMemUnmap(TestHandle, TestBuf, BUFFER_SIZE_BYTES);
    NvRmMemUnpin(TestHandle);
    NvRmMemHandleFree(TestHandle);
    return ret;
}


static NvBool VerifyLocks()
{
    NvRmDeviceHandle hRmDevice = NULL;
    NvRmMemHandle SrcHandle = NULL;
    NvRmMemHandle EncryptHandle = NULL;
    NvRmMemHandle DecryptHandle = NULL;
    NvError e;
    NvU32 PhyAddress;
    NvBool ret = NV_FALSE;

    NV_CHECK_ERROR_CLEANUP(NvRmOpen(&hRmDevice, 0));
    NV_CHECK_ERROR_CLEANUP(
        NvRmMemHandleCreate(hRmDevice, &SrcHandle, BUFFER_SIZE_BYTES));

    NV_CHECK_ERROR_CLEANUP(
        NvRmMemAllocTagged(SrcHandle,0, 0, 4, NvOsMemAttribute_Uncached,
                NVRM_MEM_TAG_SYSTEM_MISC));
    PhyAddress = NvRmMemPin(SrcHandle);
    if(PhyAddress == 0xFFFFFFFF)
        goto fail;

    NV_CHECK_ERROR_CLEANUP(
        NvRmMemMap(
            SrcHandle,
            0,
            BUFFER_SIZE_BYTES,
            NVOS_MEM_READ_WRITE,
            (void **)&s_SrcBuffer
        )
    );

    NV_CHECK_ERROR_CLEANUP(
        NvRmMemHandleCreate(hRmDevice, &EncryptHandle, BUFFER_SIZE_BYTES));

    NV_CHECK_ERROR_CLEANUP(
        NvRmMemAllocTagged(EncryptHandle,0, 0, 4, NvOsMemAttribute_Uncached,
                NVRM_MEM_TAG_SYSTEM_MISC));
    PhyAddress = NvRmMemPin(EncryptHandle);
    if(PhyAddress == 0xFFFFFFFF)
        goto fail;
    NV_CHECK_ERROR_CLEANUP(
        NvRmMemMap(
            EncryptHandle,
            0,
            BUFFER_SIZE_BYTES,
            NVOS_MEM_READ_WRITE,
            (void **)&s_EcryptBuffer
        )
    );

    NV_CHECK_ERROR_CLEANUP(
        NvRmMemHandleCreate(hRmDevice, &DecryptHandle, BUFFER_SIZE_BYTES));
    NV_CHECK_ERROR_CLEANUP(
        NvRmMemAllocTagged(DecryptHandle,0, 0, 4, NvOsMemAttribute_Uncached,
                NVRM_MEM_TAG_SYSTEM_MISC));
    PhyAddress = NvRmMemPin(DecryptHandle);
    if(PhyAddress == 0xFFFFFFFF)
        goto fail;
    NV_CHECK_ERROR_CLEANUP(
        NvRmMemMap(
            DecryptHandle,
            0,
            BUFFER_SIZE_BYTES,
            NVOS_MEM_READ_WRITE,
            (void **)&s_DecryptBuffer
        )
    );

    if(!AesClearSBKTest())
        goto fail;

    //Lock SSK should not be called for all AP20 A03 chips except AP20  A03p
    //since A03 chips does not support custom SSK update
    if(NvAbootChipCompare(hRmDevice, 0x20, 0x1, 0x3) && !NvAbootIsAP20A03pChip(hRmDevice))
    {
        //Do nothing, skip the AES lock SSK test
    }
    else
    {
        if(!AesLocksskTest())
            goto fail;
    }
    ret = NV_TRUE;
fail:
    NvRmMemUnmap(SrcHandle, s_SrcBuffer, BUFFER_SIZE_BYTES);
    NvRmMemUnpin(SrcHandle);
    NvRmMemHandleFree(SrcHandle);

    NvRmMemUnmap(EncryptHandle, s_EcryptBuffer, BUFFER_SIZE_BYTES);
    NvRmMemUnpin(EncryptHandle);
    NvRmMemHandleFree(EncryptHandle);

    NvRmMemUnmap(DecryptHandle, s_DecryptBuffer, BUFFER_SIZE_BYTES);
    NvRmMemUnpin(DecryptHandle);
    NvRmMemHandleFree(DecryptHandle);

    NvRmClose(hRmDevice);
    return ret;
}

NvBool
NvAbootChipCompare(
    const NvRmDeviceHandle hRmDevice,
    const NvU32 Chip,
    const NvU32 MajorRev,
    const NvU32 MinorRev)
{
    NvU32 RegVal;
    NvU32 ChipId;
    NvU32 MajorId;
    NvU32 MinorId;

    NV_ASSERT(hRmDevice);
    RegVal = NV_REGR(hRmDevice,
        NVRM_MODULE_ID(NvRmModuleID_Misc, 0), APB_MISC_GP_HIDREV_0);

    ChipId = NV_DRF_VAL(APB_MISC, GP_HIDREV, CHIPID, RegVal);
    MajorId = NV_DRF_VAL(APB_MISC, GP_HIDREV, MAJORREV, RegVal);
    MinorId = NV_DRF_VAL(APB_MISC, GP_HIDREV, MINORREV, RegVal);

    return ((Chip == ChipId) &&
        (MajorRev == MajorId || MajorRev == AES_TEGRA_ALL_REVS) &&
        (MinorRev == MinorId || MinorRev == AES_TEGRA_ALL_REVS));
}

NvBool NvAbootIsAP20A03pChip(const NvRmDeviceHandle hRmDevice)
{
    NvU32 ClkVal = 0;
    NvU32 SpareFuse18 = 0;
    NvU32 SpareFuse19 = 0;
    NvU32 ChipId;

    NV_ASSERT(hRmDevice);
    //Verify it is an AP20
    if(AbootPrivGetChipId(hRmDevice, &ChipId) != NvSuccess)
        NV_ABOOT_BREAK();
    if(ChipId != 0x20)
        return NV_FALSE;
    // Enable fuse values to be visible before reading the fuses
    ClkVal = NV_REGR(hRmDevice,
        NVRM_MODULE_ID( NvRmPrivModuleID_ClockAndReset, 0 ),
        CLK_RST_CONTROLLER_MISC_CLK_ENB_0);
    ClkVal = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, MISC_CLK_ENB,
        CFG_ALL_VISIBLE, 1, ClkVal);
    NV_REGW(hRmDevice,
        NVRM_MODULE_ID( NvRmPrivModuleID_ClockAndReset, 0),
        CLK_RST_CONTROLLER_MISC_CLK_ENB_0, ClkVal);

    SpareFuse18 = NV_REGR(hRmDevice,
        NVRM_MODULE_ID(NvRmModuleID_Fuse, 0), FUSE_SPARE_BIT_18_0);
    SpareFuse18 = NV_DRF_VAL(FUSE, SPARE_BIT_18, SPARE_BIT_18, SpareFuse18);
    SpareFuse19 = NV_REGR(hRmDevice,
        NVRM_MODULE_ID(NvRmModuleID_Fuse, 0), FUSE_SPARE_BIT_19_0);
    SpareFuse19 = NV_DRF_VAL(FUSE, SPARE_BIT_19, SPARE_BIT_19, SpareFuse19);
    // Disable fuse values visibility
    ClkVal = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, MISC_CLK_ENB,
                CFG_ALL_VISIBLE, 0, ClkVal);
    NV_REGW(hRmDevice,
        NVRM_MODULE_ID(NvRmPrivModuleID_ClockAndReset, 0),
        CLK_RST_CONTROLLER_MISC_CLK_ENB_0, ClkVal);

    if (SpareFuse18 | SpareFuse19)
        return NV_TRUE;
    return NV_FALSE;
}

#ifdef CONFIG_TRUSTED_FOUNDATIONS
static NvError NvAbootPrivReadEncryptedKeys(NvUPtr **ppEncryptedKeys, NvU32 *pNumOfEncryptedKeys)
{
    NvU64 nEksLength = 0;
    TFEKS_TYPE *pTfEks = NULL;
    NvU32 nReadBytes = 0;
    NvU32 nCrc32Eks = 0, nCrc32 = 0;
    NvError e = NvSuccess;
    NvStorMgrFileHandle hPartFile = 0;
    static NvU32 partitionBuffer[(TF_EKS_PARTITION_LEN / sizeof(NvU32)) + 1];

    pTfEks = (TFEKS_TYPE*)partitionBuffer;

    if ( (e = NvStorMgrFileOpen(TF_EKS_NAME, NVOS_OPEN_READ, &hPartFile)) != NvSuccess ) {
        goto fail;
    }

    if ( (e = NvStorMgrFileRead(hPartFile, &nEksLength, 4, &nReadBytes)) != NvSuccess ) {
        goto fail;
    }

    if ( (e = NvStorMgrFileRead(hPartFile, pTfEks, nEksLength - 4, &nReadBytes)) != NvSuccess ) {
        goto fail;
    }

    if (nEksLength - 4 != nReadBytes)
    {
        e = NvError_InvalidState;
        goto fail;
    }

    if (NvOsStrncmp( (char*)TF_EKS_MAGIC_STR, pTfEks->szMagicID, TF_EKS_MAGIC_ID_LEN) ) {
        e = NvError_InvalidState;
        goto fail;
    }

    if ( (e = NvStorMgrFileRead(hPartFile, &nCrc32Eks, 4, &nReadBytes)) != NvSuccess ) {
        goto fail;
    }

    nCrc32 = NvComputeCrc32( 0, (const NvU8 *)pTfEks, nEksLength - 4 );
    if ( nCrc32 != nCrc32Eks ) {
        NvOsDebugPrintf( "Crc didn't match %d, %x : %x\n", nEksLength - 4, nCrc32, nCrc32Eks );
        e = NvError_InvalidState;
        goto fail;
    }

    *ppEncryptedKeys = (NvUPtr*)pTfEks->data;
    *pNumOfEncryptedKeys = pTfEks->nNumOfEncrypedKeys;

fail:
    if (hPartFile)
        (void)NvStorMgrFileClose(hPartFile);

    return e;
}
#endif

NvError NvAbootCopyAndJump(
    NvAbootHandle hAboot,
    NvUPtr       *pDsts,
    void        **pSrcs,
    NvU32        *pSizes,
    NvU32         NumImages,
    NvUPtr        KernelStart,
    NvU32        *pKernelRegisters,
    NvU32         NumKernelRegisters)
{
    NvError e;
#ifdef CONFIG_TRUSTED_FOUNDATIONS
    TFSW_PARAMS *pTfswParams = (TFSW_PARAMS *)KernelStart;

    /* Destination for TF is in a region not mapped by the Bootlaoder.
     * Thus, this function will not consider the last image (which is the TF).
     * Once this function called, any alloc/memory write are dangerous!!! (traces do alloc!)*/
    NumImages--;

    /* Generate the keys that will be used by the TF */
    e = NvAbootPrivGenerateTFKeys((NvU8 *)pTfswParams->pTFKeys, TF_KEYS_BUFFER_SIZE);
    if (e != NvSuccess)
        return e;

    /* If keys are not available in bootloader binary, check EKS partition */
    if (SOSKeyIndexOffset == 0) {
        e = NvAbootPrivReadEncryptedKeys((NvUPtr **)&pTfswParams->pEncryptedKeys,
                                                   &pTfswParams->nNumOfEncryptedKeys );
        if (e != NvSuccess) {
            pTfswParams->pEncryptedKeys = (NvUPtr)NULL;
            pTfswParams->nNumOfEncryptedKeys = 0;

            NvOsDebugPrintf("!!!ERROR!!! EKS Partition is not available\n");
        }
    }

    e = NvAbootPrivSanitizeKeys();
    if (e != NvSuccess)
        goto fail;

#else
    NvDdkBlockDevHandle hAesBlockDev;

    //SBK should always be cleared and SSK should always be locked for security reasons,
    //any failure to clear the SBK or lock the SSK should restart the system

    NV_CHECK_ERROR_CLEANUP(NvDdkAesBlockDevOpen(0, 0, &hAesBlockDev));

    NV_CHECK_ERROR_CLEANUP(
        hAesBlockDev->NvDdkBlockDevIoctl(
            hAesBlockDev,
            NvDdkAesBlockDevIoctlType_ClearSecureBootKey,
            0,
            0,
            NULL,
            NULL
        )
    );

    NV_CHECK_ERROR_CLEANUP(
        hAesBlockDev->NvDdkBlockDevIoctl(
            hAesBlockDev,
            NvDdkAesBlockDevIoctlType_LockSecureStorageKey,
            0,
            0,
            NULL,
            NULL
        )
    );
    hAesBlockDev->NvDdkBlockDevClose(hAesBlockDev);

#if HAS_SE_CONTROLLER_INSTANCE
    NV_CHECK_ERROR_CLEANUP(NvDdkSeBlockDevOpen(0, 0, &hAesBlockDev));

    NV_CHECK_ERROR_CLEANUP(
            hAesBlockDev->NvDdkBlockDevIoctl(
                hAesBlockDev,
                NvDdkSeAesBlockDevIoctlType_ClearSecureBootKey,
                0,
                0,
                NULL,
                NULL
        )
    );

    NV_CHECK_ERROR_CLEANUP(
            hAesBlockDev->NvDdkBlockDevIoctl(
                hAesBlockDev,
                NvDdkSeAesBlockDevIoctlType_LockSecureStorageKey,
                0,
                0,
                NULL,
                NULL
        )
    );

    hAesBlockDev->NvDdkBlockDevClose(hAesBlockDev);
#endif
#endif

    if(!VerifyLocks())
        goto fail;

    if (NumImages > MAX_NUM_IMAGES ||
        NumKernelRegisters > MAX_NUM_REGISTERS)
        return NvError_NotSupported;

    NvOsMutexLock(hAboot->hStorageMutex);
    if (hAboot->BlockMgrInit)
    {
        NvFsMgrDeinit();
        NvStorMgrDeinit();
        NvPartMgrDeinit();
        NvDdkBlockDevMgrDeinit();
    }

    //  Restore IRAM
    NvOsMemcpy(hAboot->pIramPhys, hAboot->pIramBootShadow, hAboot->IramSize);

    e = NvAbootPrivCopyImages(hAboot, pDsts, pSrcs, pSizes, NumImages);

    if (e != NvSuccess)
        return e;

    if (NvAbootPrivSaveSdramParams(hAboot))
        goto fail;
    // Please do not remove the below print used for tracking boot time
    NvOsDebugPrintf("Jumping to kernel at:%d ms\n", NvOsGetTimeMS());
    /*
     * This place is your last chance to dirty the cache...
     */
    NvAbootPrivSanitizeUniverse(hAboot);
    // Do *NOT* call anything after this point is reached!!!
#ifdef CONFIG_TRUSTED_FOUNDATIONS
    NvAbootPrivDoJump_TF(pKernelRegisters, NumKernelRegisters, pTfswParams);
#else
    NvAbootPrivDoJump(pKernelRegisters, NumKernelRegisters, KernelStart);
#endif
fail:
    NvAbootReset(hAboot);
    return NvError_ResourceError;
}

NvError NvAbootOpen(
    NvAbootHandle *phAboot,
    NvU32 *EnterNv3pServer,
    NvU32 *Boot)
{
    NvRmPhysAddr  Base;
    NvU32         Len;
    NvU32         Num;
    NvU32         Instance = 0;
    NvDdkBlockDevMgrDeviceId BootDev;
    NvError       e;
    NvBool        RecoveryMode;
    NvBool        BdInit = NV_FALSE;
    NvBool        PmInit = NV_FALSE;
    NvBool        SmInit = NV_FALSE;
    NvBool        FsInit = NV_FALSE;

    if (phAboot == NULL)
    {
        return NvError_BadParameter;
    }

    s_Aboot.hStorageMutex = NULL;
    s_Aboot.hRm = NULL;

    NV_CHECK_ERROR_CLEANUP(NvOsMutexCreate(&s_Aboot.hStorageMutex));

    RecoveryMode = NvBlQueryGetNv3pServerFlag();

    NV_CHECK_ERROR_CLEANUP(AbootPrivGetOsInfo(&s_Aboot.OsImage));

    if (EnterNv3pServer)
        *EnterNv3pServer = (NvU32)RecoveryMode;

    NV_CHECK_ERROR_CLEANUP(NvRmOpenNew(&s_Aboot.hRm));

    BootDev = AbootPrivGetSecBootDevId(&Instance);

    if (Boot)
        *Boot = (NvU32)BootDev;

    Num = NvRmModuleGetNumInstances(s_Aboot.hRm, NvRmPrivModuleID_Iram);
    NvRmModuleGetBaseAddress(s_Aboot.hRm,
        NVRM_MODULE_ID(NvRmPrivModuleID_Iram, 0), &Base, &Len);
    Len *= Num;
    s_Aboot.IramSize = Len;

    NV_CHECK_ERROR_CLEANUP(NvRmPhysicalMemMap(Base, Len, NVOS_MEM_READ_WRITE,
            NvOsMemAttribute_Uncached, (void**)&s_Aboot.pIramPhys));

    s_Aboot.pIramBootShadow = NvOsAlloc(Len);
    if (!s_Aboot.pIramBootShadow)
        goto fail;

    NvOsMemcpy(s_Aboot.pIramBootShadow, s_Aboot.pIramPhys, Len);
    s_Aboot.BlockDevId = BootDev;

    if ((s_Aboot.BlockDevId  == NvDdkBlockDevMgrDeviceId_SDMMC) ||
            (s_Aboot.BlockDevId  == NvDdkBlockDevMgrDeviceId_Spi))
        s_Aboot.BlockDevInstance = Instance;
    else
        s_Aboot.BlockDevInstance = 0;

    NV_CHECK_ERROR_CLEANUP(NvDdkBlockDevMgrInit());
    BdInit = NV_TRUE;
    NV_CHECK_ERROR_CLEANUP(NvPartMgrInit());
    PmInit = NV_TRUE;
    NV_CHECK_ERROR_CLEANUP(NvStorMgrInit());
    SmInit = NV_TRUE;
    NV_CHECK_ERROR_CLEANUP(NvFsMgrInit());
    FsInit = NV_TRUE;

    if (!RecoveryMode)
        NV_CHECK_ERROR_CLEANUP(
            AbootPrivLoadPartitionTable(s_Aboot.BlockDevId,
                 s_Aboot.BlockDevInstance, &s_Aboot.OsImage)
        );

    //  Restore IRAM
    NvOsMemcpy(s_Aboot.pIramPhys, s_Aboot.pIramBootShadow, s_Aboot.IramSize);
    *phAboot = (NvAbootHandle)&s_Aboot;
    return NvError_Success;

 fail:
    if (FsInit)
        NvFsMgrDeinit();
    if (SmInit)
        NvStorMgrDeinit();
    if (PmInit)
        NvPartMgrDeinit();
    if (BdInit)
        NvDdkBlockDevMgrDeinit();

    if (FsInit || SmInit || PmInit || BdInit)
        NvOsMemcpy(s_Aboot.pIramPhys, s_Aboot.pIramBootShadow,
            s_Aboot.IramSize);

    if (s_Aboot.hRm)
    {
        NvRmClose(s_Aboot.hRm);
        s_Aboot.hRm = NULL;
    }

    NvOsMutexDestroy(s_Aboot.hStorageMutex);
    s_Aboot.hStorageMutex = NULL;
    *phAboot = NULL;

    return e;
}

NvError
NvAbootPrivGetStartPhysicalPage(
       NvAbootHandle hAboot,
       NvU32         PartitionId,
       NvU64         LogicalPage,
       NvU64         *PhysicalPage)
{
    NvDdkBlockDevIoctl_GetPartitionPhysicalSectorsInputArgs InArgs;
    NvDdkBlockDevIoctl_GetPartitionPhysicalSectorsOutputArgs OutArgs;
    NvFsMountInfo       minfo;
    NvDdkBlockDevHandle hDev = NULL;
    NvError             e;

    NV_CHECK_ERROR_CLEANUP(NvPartMgrGetFsInfo( PartitionId, &minfo ));

    NV_CHECK_ERROR_CLEANUP(
        NvDdkBlockDevMgrDeviceOpen( (NvDdkBlockDevMgrDeviceId)minfo.DeviceId,
            minfo.DeviceInstance, PartitionId, &hDev )
    );

    InArgs.PartitionLogicalSectorStart = LogicalPage;
    // FIXME: Since end physical unused we set stop logical sector same as start
    InArgs.PartitionLogicalSectorStop = InArgs.PartitionLogicalSectorStart;

    NV_CHECK_ERROR_CLEANUP(hDev->NvDdkBlockDevIoctl(hDev,
            NvDdkBlockDevIoctlType_GetPartitionPhysicalSectors,
            sizeof(NvDdkBlockDevIoctl_GetPartitionPhysicalSectorsInputArgs),
            sizeof(NvDdkBlockDevIoctl_GetPartitionPhysicalSectorsOutputArgs),
            &InArgs,
            &OutArgs)
    );

    *PhysicalPage = OutArgs.PartitionPhysicalSectorStart;
    // FIXME: if we need physical stop sector we may need to send
    // extra argument stop logical sector as argument

fail:
    if (hDev)
        hDev->NvDdkBlockDevClose( hDev );
    return e;
}

NvError NvAbootGetPartitionParameters(
    NvAbootHandle hAboot,
    const char *NvPartName,
    NvU64      *StartPhysicalSector,
    NvU64      *NumLogicalSectors,
    NvU32      *SectorSize)
{
    NvError e;
    NvU32 PartId;
    NvPartInfo PartInfo;
    NvDdkBlockDevInfo BlockInfo;
    NvDdkBlockDevHandle hBlock = NULL;

    if (!hAboot || !NvPartName || !StartPhysicalSector || !NumLogicalSectors || !SectorSize)
        return NvError_BadParameter;

    NvOsMutexLock(hAboot->hStorageMutex);

    NV_CHECK_ERROR_CLEANUP(
         NvDdkBlockDevMgrDeviceOpen(hAboot->BlockDevId,
             hAboot->BlockDevInstance, 0, &hBlock)
    );

    NV_CHECK_ERROR_CLEANUP(NvPartMgrGetIdByName(NvPartName, &PartId));

    NV_CHECK_ERROR_CLEANUP(NvPartMgrGetPartInfo(PartId, &PartInfo));

    NV_CHECK_ERROR_CLEANUP(
        NvAbootPrivGetStartPhysicalPage(hAboot, PartId,
             PartInfo.StartLogicalSectorAddress, StartPhysicalSector)
    );

    *NumLogicalSectors = PartInfo.NumLogicalSectors;

    hBlock->NvDdkBlockDevGetDeviceInfo(hBlock, &BlockInfo);
    *SectorSize = BlockInfo.BytesPerSector;

    e = NvSuccess;
 fail:
    if (hBlock)
        hBlock->NvDdkBlockDevClose(hBlock);
    NvOsMutexUnlock(hAboot->hStorageMutex);
    return e;
}

NvError NvAbootGetPartitionParametersbyId(
    NvAbootHandle hAboot,
    NvU32      PartId,
    NvU64      *StartPhysicalSector,
    NvU64      *NumPhysicalSectors,
    NvU32      *SectorSize)
{
    NvError e;
    NvPartInfo PartInfo;
    NvDdkBlockDevInfo BlockInfo;
    NvDdkBlockDevHandle hBlock = NULL;
    NvFsMountInfo pFsMountInfo;

    if (!hAboot || !StartPhysicalSector ||
            !NumPhysicalSectors || !SectorSize)
        return NvError_BadParameter;

    NvOsMutexLock(hAboot->hStorageMutex);
    NV_CHECK_ERROR_CLEANUP( NvPartMgrGetFsInfo( PartId, &pFsMountInfo) );

    NV_CHECK_ERROR_CLEANUP(
         NvDdkBlockDevMgrDeviceOpen(pFsMountInfo.DeviceId,
             pFsMountInfo.DeviceInstance, PartId, &hBlock)
    );

    NV_CHECK_ERROR_CLEANUP(NvPartMgrGetPartInfo(PartId, &PartInfo));

    NV_CHECK_ERROR_CLEANUP(
        NvAbootPrivGetStartPhysicalPage(hAboot, PartId,
             PartInfo.StartLogicalSectorAddress, StartPhysicalSector)
    );

    *NumPhysicalSectors = (PartInfo.EndPhysicalSectorAddress
                            - PartInfo.StartPhysicalSectorAddress) + 1;

    hBlock->NvDdkBlockDevGetDeviceInfo(hBlock, &BlockInfo);
    *SectorSize = BlockInfo.BytesPerSector;

    e = NvSuccess;
 fail:
    if (hBlock)
        hBlock->NvDdkBlockDevClose(hBlock);
    NvOsMutexUnlock(hAboot->hStorageMutex);
    return e;
}

void NvAbootGetDeviceInstance(
                NvAbootHandle hAboot,
                NvU32 *DevInstance)
{
    *DevInstance = hAboot->BlockDevInstance;
}

static NvError AbootPrivGetChipId(
    NvRmDeviceHandle RmHandle,
    NvU32 *ChipId)
{
    NvRmPhysAddr MiscBase;
    NvU32        MiscSize;
    NvU8        *pMisc;
    NvU32        reg;
    NvError      e = NvSuccess;

    NvRmModuleGetBaseAddress(RmHandle,
        NVRM_MODULE_ID(NvRmModuleID_Misc, 0), &MiscBase, &MiscSize);

    NV_CHECK_ERROR_CLEANUP(
        NvRmPhysicalMemMap(MiscBase, MiscSize, NVOS_MEM_READ_WRITE,
            NvOsMemAttribute_Uncached, (void**)&pMisc)
    );

    reg = NV_READ32(pMisc + APB_MISC_GP_HIDREV_0);
    *ChipId = NV_DRF_VAL(APB_MISC_GP, HIDREV, CHIPID, reg);

fail:
    return e;
}

NvAbootChipSku NvAbootPrivGetChipSku(NvAbootHandle hAboot)
{
    static NvAbootChipSku ChipSku = NvAbootChipSku_Inval;
    NvU32 Id;

    if (ChipSku != NvAbootChipSku_Inval)
    {
        /* if loaded, return cached value */
        return ChipSku;
    }

    if (AbootPrivGetChipId(hAboot->hRm, &Id) != NvSuccess)
    {
        NV_ABOOT_BREAK();
        return ChipSku;
    }

    if (Id == 0x20)
    {
        ChipSku = NvAbootChipSku_T20;
    }
    else if (Id == 0x30)
    {
        NvU32 SkuInfo, PkgInfo;
        NvU32 size = sizeof(NvU32);

        NvDdkFuseGet(NvDdkFuseDataType_Sku, &SkuInfo, &size);
        NvDdkFuseGet(NvDdkFuseDataType_PackageInfo, &PkgInfo, &size);

        ChipSku = NvAbootChipSku_T30;
        if (SkuInfo == 0xA0)
        {
            ChipSku = NvAbootChipSku_T37;
            if ((PkgInfo & 0xF) == 0x2)
                ChipSku = NvAbootChipSku_AP37;
        }
        else if (SkuInfo == 0x80)
            ChipSku = NvAbootChipSku_T33;
        else if ((SkuInfo == 0x81) && ((PkgInfo & 0xF) == 0x2))
            ChipSku = NvAbootChipSku_AP33;
    }
    return ChipSku;
}

void NvAbootReset(NvAbootHandle hAboot)
{
    NvU32 ChipId;

    if (AbootPrivGetChipId(hAboot->hRm, &ChipId)!=NvSuccess)
        NV_ABOOT_BREAK();

    switch (ChipId)
    {
    case 0x20:
        NvAbootPrivAp20Reset(hAboot);
        break;
    case 0x30:
        NvAbootPrivT30Reset(hAboot);
        break;
    default:
        NV_ABOOT_BREAK();
    }
}

static NvDdkBlockDevMgrDeviceId AbootPrivGetSecBootDevId(NvU32 *pInstance)
{
    NvDdkBlockDevMgrDeviceId DdkDeviceId = NvDdkBlockDevMgrDeviceId_Invalid;
    NvU32 Instance;
    NvU32 BootDevice;

    if (pInstance == NULL)
        return NvDdkBlockDevMgrDeviceId_Invalid;

    NvFuseGetSecondaryBootDevice(&BootDevice, &Instance);
    switch (BootDevice)
    {
    case NvBootDevType_Nand_x8:
    case NvBootDevType_Nand_x16:
        DdkDeviceId = NvDdkBlockDevMgrDeviceId_Nand;
        break;
    case NvBootDevType_Sdmmc:
        DdkDeviceId = NvDdkBlockDevMgrDeviceId_SDMMC;
        break;
    case NvBootDevType_Spi:
        DdkDeviceId = NvDdkBlockDevMgrDeviceId_Spi;
        break;
    case NvBootDevType_Nor:
        DdkDeviceId = NvDdkBlockDevMgrDeviceId_Nor;
        break;
    default:
        break;
    }

    *pInstance = Instance;
    return DdkDeviceId;
}

static NvError AbootPrivGetOsInfo(NvBctAuxInfo *pAuxInfo)
{
    NvBctHandle hBct = NULL;
    NvU32 Size, Instance;
    NvU8 *pBuffer = NULL;
    NvError e = NvSuccess;

    Size = 0;
    Instance = 0;

    NV_CHECK_ERROR_CLEANUP(NvBctInit(&Size, NULL, &hBct));

    NV_CHECK_ERROR_CLEANUP(
        NvBctGetData(hBct, NvBctDataType_AuxDataAligned,
            &Size, &Instance, NULL)
    );
    pBuffer = NvOsAlloc(Size);
    if (!pBuffer)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    NV_CHECK_ERROR_CLEANUP(
        NvBctGetData(hBct, NvBctDataType_AuxDataAligned,
            &Size, &Instance, (NvU8 *)pBuffer)
    );

    NvOsMemcpy(pAuxInfo, pBuffer, sizeof(NvBctAuxInfo));

 fail:
    NvOsFree(pBuffer);
    NvBctDeinit(hBct);
    return e;
}

static NvError AbootPrivLoadPartitionTable(
    NvDdkBlockDevMgrDeviceId DdkDeviceId,
    NvU32 DevInstance,
    const NvBctAuxInfo *pAuxInfo)
{
    NvError e = NvSuccess;

    e = NvPartMgrLoadTable(DdkDeviceId, DevInstance,
            pAuxInfo->StartLogicalSector, pAuxInfo->NumLogicalSectors,
            NV_FALSE, NV_FALSE);

    return e;
}

NvBool NvAbootGetDramPartitionParameters(
    NvAbootHandle hAboot,
    NvAbootDramPart *pDramPart,
    NvU32 PartNumber)
{
    NvRmPhysAddr PhysBase;
    NvU32 PhysSize;

    if (PartNumber == 0)
    {
        /* Get the physical limits of the primary DRAM aperture. */
        NvRmModuleGetBaseAddress(hAboot->hRm, NvRmPrivModuleID_ExternalMemory,
                                 &PhysBase, &PhysSize);

        pDramPart->Type = NvAbootDramPart_Ram;
        pDramPart->Bottom = PhysBase;
        pDramPart->Top = pDramPart->Bottom + PhysSize;
        return NV_TRUE;
    }
    return NV_FALSE;
}

static void NvAbootInitializeMemoryLayout(NvAbootHandle hAboot)
{
    NvOdmOsOsInfo OsInfo;
    NvAbootDramPart DramPart;
    NvU64 MemSize, BankSize;

    /* The memory configuration of the bootloader may not be the same
       as that of the OS the bootloader is loading. Retrieve the memory
       size of the target OS. */
    NvOsMemset(&OsInfo, 0, sizeof(NvOdmOsOsInfo));
    OsInfo.OsType = NvOdmOsOs_Linux;
#if defined(ANDROID)
    OsInfo.Sku = NvOdmOsSku_Android;
#else
    OsInfo.Sku = NvOdmOsSku_Unknown;
#endif

    /* Get the size of DRAM available to the target OS. */
    MemSize = NvOdmQueryOsMemSize(NvOdmMemoryType_Sdram, &OsInfo);

    /* Get the physical limits of the primary DRAM aperture. */
    NvAbootGetDramPartitionParameters(hAboot, &DramPart, 0);

    /* If the contiguous memory size partition extends beyond end of the
       primary physical aperture, reduce the size to the size to the that
       of the physical aperture. */
    BankSize = MemSize;
    if (DramPart.Bottom + MemSize >= DramPart.Top)
    {
        BankSize = DramPart.Top - DramPart.Bottom;

        /* If the primary physical memory aperture ends at the last
           usable DRAM address (4GB - 1MB) throw away the top 1MB of
           DRAM since this memory is unusable. */
        if (DramPart.Top >= 0xFFF00000ull)
            MemSize -= NV_MIN(MemSize, 0x00100000);
    }

#ifdef CONFIG_TRUSTED_FOUNDATIONS
    /* Reduce what's available by what secure OS is using */
    NvAbootMemLayout[NvAbootMemLayout_SecureOs].Size = NvOdmQuerySecureRegionSize();
    BankSize -= NvAbootMemLayout[NvAbootMemLayout_SecureOs].Size;
    MemSize  -= NvAbootMemLayout[NvAbootMemLayout_SecureOs].Size;
    NvAbootMemLayout[NvAbootMemLayout_SecureOs].Base = DramPart.Bottom + BankSize;
#endif

    MemSize -= BankSize;
    NV_ASSERT((NvS32)MemSize >= 0);

    NvAbootMemLayout[NvAbootMemLayout_Primary].Base = DramPart.Bottom;
    NvAbootMemLayout[NvAbootMemLayout_Primary].Size = BankSize;
    NvAbootMemLayout[NvAbootMemLayout_Extended].Size = MemSize;
}

NvBool NvAbootMemLayoutBaseSize(NvAbootHandle hAboot, NvAbootMemLayoutType Type,
                                NvUPtr *Base, NvU64 *Size)
{
    if (!NvAbootMemLayout[NvAbootMemLayout_Primary].Size)
    {
        /* initialize memory layout, if hasn't been already */
        NvAbootInitializeMemoryLayout(hAboot);
        NV_ASSERT(NvAbootMemLayout[NvAbootMemLayout_Primary].Size);
    }

    if (Type < NvAbootMemLayout_Num)
    {
        if (Base)
            *Base = NvAbootMemLayout[Type].Base;
        if (Size)
            *Size = NvAbootMemLayout[Type].Size;
        return NV_TRUE;
    }
    return NV_FALSE;
}

extern NvU32 nvaos_GetMaxMemorySize( void );
extern NvUPtr nvaos_GetMemoryStart( void );

NvError NvOsBootArgGet( NvU32 key, void *arg, NvU32 size )
{
    if (key==NvBootArgKey_Carveout && size==sizeof(NvBootArgsCarveout))
    {
        NvBootArgsCarveout *carveout = (NvBootArgsCarveout *)arg;
        NvU32 MemSize = NvOdmQueryMemSize(NvOdmMemoryType_Sdram);
        NvU32 AosMax = nvaos_GetMaxMemorySize();

        carveout->size = NvOdmQueryCarveoutSize();
        carveout->base = nvaos_GetMemoryStart() + NV_MIN(AosMax,MemSize)
                         - carveout->size - RESERVED_SIZE_FROM_TOP;
        return NvSuccess;
    }
    return NvError_NotSupported;
}

#ifdef LPM_BATTERY_CHARGING
NvError NvAbootLowPowerMode(NvAbootLpmId Id, ...)
{
    NvError e = NvError_Success;
    NvRmDeviceHandle hRm;
    va_list ap;

    NV_CHECK_ERROR_CLEANUP(NvRmOpen(&hRm, 0));

    va_start( ap, Id );
    switch(Id)
    {
        case NvAbootLpmId_GoToLowPowerMode:
                NvOsDebugPrintf("Going to low power mode\n");
#if NVODM_CPU_BOOT_PERF_SET
                {
                    NvRmFreqKHz SocCpuKHz;

                    NV_ASSERT(hRm);
                    NvOsDebugPrintf("Lowering the cpu clock frequency\n");

                    SocCpuKHz = va_arg(ap, NvU32);

                    NvRmPrivSetCpuClock(hRm, SocCpuKHz);
                }
#endif
                NvOsDebugPrintf("Disabling the peripheral clocks\n");
                NvAbootClocksInterface(NvAbootClocksId_DisableClocks, hRm);
            break;
        case NvAbootLpmId_ComeOutOfLowPowerMode:
                NvOsDebugPrintf("Coming out of low power mode\n");
#if NVODM_CPU_BOOT_PERF_SET
                NV_ASSERT(hRm);
                NvOsDebugPrintf("Restoring the cpu clock frequency\n");
                //FIXME: There is similar call in NvAppMain
                NvRmPrivSetMaxCpuClock(hRm);
#endif
                NvOsDebugPrintf("Enabling the peripheral clocks\n");
                NvAbootClocksInterface(NvAbootClocksId_EnableClocks, hRm);
            break;
    }

fail:
    NvRmClose(hRm);
    hRm = NULL;

    return e;
}
#endif
