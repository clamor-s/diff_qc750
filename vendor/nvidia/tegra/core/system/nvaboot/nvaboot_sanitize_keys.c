/*
 * Copyright (c) 2009-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvaboot_sanitize_keys.h"

#include "nvddk_blockdevmgr_defs.h"
#include "nvddk_blockdevmgr.h"
#include "nvddk_aes_blockdev_defs.h"
#include "nvcrypto_random.h"
#include "nvassert.h"
#if HAS_SE_CONTROLLER_INSTANCE
#include "nvddk_se_blockdev.h"
#endif


static NvError NvAbootPrivDeriveKey(NvDdkBlockDevHandle AesHandle,
                NvDdkAesKeyType key, NvU8 *buff, NvU32 size);
static NvError NvAbootPrivSetAndLockSSK(NvDdkBlockDevHandle AesHandle,
                NvU8 *buff, NvU32 size);
static NvError NvAbootPrivClearSBK(NvDdkBlockDevHandle AesHandle);
static NvError NvAbootPrivGenSeed(NvU8 *pBuff, NvU32 size);
#if HAS_SE_CONTROLLER_INSTANCE
static NvError NvAbootPrivSeSetAndLockSSK(NvDdkBlockDevHandle SeAesHandle,
                NvU8 *buff, NvU32 Size);

static NvError NvAbootPrivSeClearSBK(NvDdkBlockDevHandle SeAesHandle);
#endif
NvError
NvAbootPrivSanitizeKeys(void)
{
    NvError e = NvError_NotInitialized;
    NvDdkBlockDevHandle AesHandle;
    NvDdkBlockDevHandle SeAesHandle;

    NvU8 os_ssk_buf[NvDdkAesKeySize_128Bit] = {
                0x65, 0x0f, 0x2d, 0x25, 0x19, 0xde, 0x53, 0xdd,
                0x9a, 0xde, 0xa3, 0x54, 0x6f, 0xb0, 0xf5, 0xa4};

    // Open AES Engine.
    NV_CHECK_ERROR_CLEANUP(NvDdkBlockDevMgrDeviceOpen(
                               NvDdkBlockDevMgrDeviceId_Aes,
                               0, /* Instance */
                               0, /* MinorInstance */
                               &AesHandle));

    // Step 1: Generate OS_SSK from SSK.
    NV_CHECK_ERROR_CLEANUP( NvAbootPrivDeriveKey(AesHandle,
                        NvDdkAesKeyType_SecureStorageKey,
                        (NvU8 *)&os_ssk_buf, sizeof(os_ssk_buf)) );

    // Step 2: Overwrite and lock OS_SSK.
    NV_CHECK_ERROR_CLEANUP( NvAbootPrivSetAndLockSSK(AesHandle,
                        (NvU8 *)&os_ssk_buf, sizeof(os_ssk_buf)) );

    // Step 3: Clear SBK in AES Engines.
    NV_CHECK_ERROR_CLEANUP( NvAbootPrivClearSBK(AesHandle) );

#if HAS_SE_CONTROLLER_INSTANCE
    // Open SE Engine
    NV_CHECK_ERROR_CLEANUP(NvDdkBlockDevMgrDeviceOpen(
                NvDdkBlockDevMgrDeviceId_Se,
                0,
                0,
                &SeAesHandle));
    //Set and lock SSK
    NV_CHECK_ERROR_CLEANUP( NvAbootPrivSeSetAndLockSSK(SeAesHandle,
                        (NvU8 *)&os_ssk_buf, sizeof(os_ssk_buf)));

    //Clear SBK
    NV_CHECK_ERROR_CLEANUP(NvAbootPrivSeClearSBK(SeAesHandle));
#endif
    e = NvSuccess;
fail:
    // close AES handle
    AesHandle->NvDdkBlockDevClose(AesHandle);
#if HAS_SE_CONTROLLER_INSTANCE
    SeAesHandle->NvDdkBlockDevClose(SeAesHandle);
#endif
    return e;
}


#ifdef CONFIG_TRUSTED_FOUNDATIONS
/*
   This function must be called before NvAbootPrivSanitizeKeys as all the mechanism used inside are based
   on values present in the AES engines.
 */
NvError
NvAbootPrivGenerateTFKeys(NvU8 *pTFBuff, NvU32 size)
{
    NvError e = NvError_NotInitialized;
    NvDdkBlockDevHandle AesHandle;
    NvU32 cnt = 0;
    NvU32 step = 0;
    NvU8  header[2] = {0x01, 0xAA};

    NvU8 tf_ssk_buf[NvDdkAesKeySize_128Bit] = {
                        0x30, 0xd1, 0x95, 0x88, 0x30, 0x56, 0xc1, 0xc6,
                        0x52, 0x81, 0x88, 0xb9, 0x75, 0xa4, 0x77, 0x63};
    NvU8 tf_sbk_buf[NvDdkAesKeySize_128Bit] = {
                        0x59, 0xe2, 0x63, 0x61, 0x24, 0x67, 0x4c, 0xcd,
                        0x84, 0x0c, 0xb4, 0x2c, 0x8e, 0x84, 0xf1, 0x16};
    NvU8 tf_seed_buf[TF_SEED_SIZE];
    NvU32 tf_buf_size = NvDdkAesKeySize_128Bit;

    // Open AES Engine.
    NV_CHECK_ERROR_CLEANUP(NvDdkBlockDevMgrDeviceOpen(
                               NvDdkBlockDevMgrDeviceId_Aes,
                               0, /* Instance */
                               0, /* MinorInstance */
                               &AesHandle) );

    // Generate TF_SSK from SSK and TF_SBK from SBK
    NV_CHECK_ERROR_CLEANUP( NvAbootPrivDeriveKey(AesHandle,
                        NvDdkAesKeyType_SecureStorageKey,
                        (NvU8 *)&tf_ssk_buf, tf_buf_size) );
    NV_CHECK_ERROR_CLEANUP( NvAbootPrivDeriveKey(AesHandle,
                        NvDdkAesKeyType_SecureBootKey,
                        (NvU8 *)&tf_sbk_buf, tf_buf_size) );

    // Generate a random buffer
    NV_CHECK_ERROR_CLEANUP( NvAbootPrivGenSeed((NvU8 *)&tf_seed_buf,
                TF_SEED_SIZE) );

    if ( size<TF_KEYS_BUFFER_SIZE )
    {
      e = NvError_InsufficientMemory;
      goto fail;
    }

    // Copy materials in pTFBuff (TF_SSK/TF_SBK... include a small header and garbadge)
    NvOsMemcpy(pTFBuff, &header, sizeof(header));
    cnt = 4;
    step = tf_buf_size/2;
    NvOsMemcpy(pTFBuff+cnt, tf_seed_buf, step);
    cnt += step;
    NvOsMemcpy(pTFBuff+cnt, tf_ssk_buf, step);
    cnt += step;
    NvOsMemcpy(pTFBuff+cnt, tf_sbk_buf, step);
    cnt += step;

    cnt += 8;
    NvOsMemcpy(pTFBuff+cnt, &tf_seed_buf[step], step);
    cnt += step;
    NvOsMemcpy(pTFBuff+cnt, &tf_ssk_buf[step], step);
    cnt += step;
    NvOsMemcpy(pTFBuff+cnt, &tf_sbk_buf[step], step);
    cnt += step;

    // fill last 64 bytes of buffer with Seed
    NvOsMemcpy(pTFBuff+TF_SEED_SIZE, &tf_seed_buf, TF_SEED_SIZE);

    e = NvSuccess;
fail:
    // close AES handle
    AesHandle->NvDdkBlockDevClose(AesHandle);
    return e;
}
#endif



static NvError
NvAbootPrivDeriveKey(NvDdkBlockDevHandle AesHandle,
                    NvDdkAesKeyType key, NvU8 *buff, NvU32 size)
{
    NvError e = NvError_NotInitialized;
    NvDdkAesBlockDevIoctl_SelectOperationInputArgs  aesOperation;
    NvDdkAesBlockDevIoctl_SelectKeyInputArgs        aesKey;
    NvDdkAesBlockDevIoctl_SetInitialVectorInputArgs initialVector;
    NvDdkAesBlockDevIoctl_ProcessBufferInputArgs    bufferInput;

    // Basic check : buff size must = key size
    NV_ASSERT( size ==  NvDdkAesKeySize_128Bit );

    // Select operation
    aesOperation.IsEncrypt = NV_TRUE;
    aesOperation.OpMode = NvDdkAesOperationalMode_Cbc;
    NV_CHECK_ERROR(AesHandle->NvDdkBlockDevIoctl(
                       AesHandle,
                       NvDdkAesBlockDevIoctlType_SelectOperation,
                       sizeof(aesOperation),
                       0,
                       &aesOperation,
                       NULL));

    // Select key
    NvOsMemset(&aesKey, 0, sizeof(aesKey));
    aesKey.KeyType = key;
    aesKey.KeyLength = NvDdkAesKeySize_128Bit;
    aesKey.IsDedicatedKeySlot = NV_FALSE;
    NV_CHECK_ERROR(AesHandle->NvDdkBlockDevIoctl(
                       AesHandle,
                       NvDdkAesBlockDevIoctlType_SelectKey,
                       sizeof(aesKey),
                       0,
                       &aesKey,
                       NULL));

    // Set IV
    NvOsMemset(&initialVector, 0x0, sizeof(initialVector));
    NV_CHECK_ERROR(AesHandle->NvDdkBlockDevIoctl(
                      AesHandle,
                      NvDdkAesBlockDevIoctlType_SetInitialVector,
                      sizeof(initialVector),
                      0,
                      &initialVector,
                      NULL));

    // Encrypt buff
    bufferInput.BufferSize = NvDdkAesKeySize_128Bit;
    bufferInput.SrcBuffer = buff;
    bufferInput.DestBuffer = buff;
    bufferInput.SkipOffset = 0;
    NV_CHECK_ERROR(AesHandle->NvDdkBlockDevIoctl(
                       AesHandle,
                       NvDdkAesBlockDevIoctlType_ProcessBuffer,
                       sizeof(bufferInput),
                       0,
                       &bufferInput,
                       NULL));

    e = NvSuccess;
    return e;
}


static NvError
NvAbootPrivSetAndLockSSK(NvDdkBlockDevHandle AesHandle, NvU8 *buff, NvU32 size)
{
    NvError e = NvError_NotInitialized;
    NvDdkAesBlockDevIoctl_SetAndLockSecureStorageKeyInputArgs keyArgs;

    // Basic check : buff size must = key size
    NV_ASSERT( size ==  NvDdkAesKeySize_128Bit );

    // Configure key
    keyArgs.KeyLength = NvDdkAesKeySize_128Bit;
    NvOsMemcpy (&keyArgs.Key, buff, NvDdkAesKeySize_128Bit);

    // Update SSK stored in AES Engine
    NV_CHECK_ERROR(AesHandle->NvDdkBlockDevIoctl(
                      AesHandle,
                      NvDdkAesBlockDevIoctlType_SetAndLockSecureStorageKey,
                      sizeof(keyArgs),
                      0,
                      &keyArgs,
                      NULL));

    e = NvSuccess;
    return e;
}
#if HAS_SE_CONTROLLER_INSTANCE
static NvError
NvAbootPrivSeSetAndLockSSK(NvDdkBlockDevHandle SeAesHandle, NvU8 *buff, NvU32 Size)
{
    NvError e = NvSuccess;
    NvDdkSeAesSsk SskArgs;

    NV_ASSERT(buff);
    NV_ASSERT(Size == NvDdkAesKeySize_128Bit);
    if (!buff || !(Size == NvDdkAesKeySize_128Bit))
    {
        e = NvError_BadParameter;
        goto fail;
    }

    SskArgs.pSsk = buff;
    //set and lock SSK
    NV_CHECK_ERROR(SeAesHandle->NvDdkBlockDevIoctl(
                SeAesHandle,
                NvDdkSeAesBlockDevIoctlType_SetAndLockSecureStorageKey,
                sizeof(SskArgs),
                0,
                (const void *)&SskArgs,
                NULL));
fail:
    return e;
}

static NvError
NvAbootPrivSeClearSBK(NvDdkBlockDevHandle SeAesHandle)
{
    NvError e = NvSuccess;

    /* Clear SBK */
     NV_CHECK_ERROR(SeAesHandle->NvDdkBlockDevIoctl(
                 SeAesHandle,
                 NvDdkSeAesBlockDevIoctlType_ClearSecureBootKey,
                 0,
                 0,
                 NULL,
                 NULL));
fail:
     return e;
}
#endif

static NvError
NvAbootPrivClearSBK(NvDdkBlockDevHandle AesHandle)
{
    NvError e = NvError_NotInitialized;

    // Update AES Engine
    NV_CHECK_ERROR(AesHandle->NvDdkBlockDevIoctl(
                      AesHandle,
                      NvDdkAesBlockDevIoctlType_ClearSecureBootKey,
                      0,
                      0,
                      NULL,
                      NULL));

    e = NvSuccess;
    return e;
}

// Random number generated from PLL linear feedback shift register.
#define READ32(reg)                         (*((volatile uint32_t *)(reg)))
#define CLK_RST_ADDR                        0x60006000
#define CLK_RST_CONTROLLER_PLL_LFSR_0       0x54
#define CLK_RST_CONTROLLER_PLL_LFSR_0_MASK  0xffff

static NvError
NvAbootPrivGenSeed(NvU8 *pBuff, NvU32 size)
{
    NvU16 PRNG = 0;
    NvU32 i = 0;
    NvU32 step = sizeof(PRNG);

    for (i=0; i<size; )
    {
        PRNG = (NvU16)( READ32(CLK_RST_ADDR + CLK_RST_CONTROLLER_PLL_LFSR_0) &
                CLK_RST_CONTROLLER_PLL_LFSR_0_MASK );
        pBuff[i] = PRNG&0xFF;
        pBuff[i+1] = (PRNG>>8)&0xFF;
        i+=step;
    }

    return NvSuccess;
}
