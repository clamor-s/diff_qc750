/*
 * Copyright (c) 2009-2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvos.h"
#include "nvaboot.h"
#include "nvaboot_private.h"
#include "nvbootargs.h"
#include "nvappmain.h"
#include "nvutil.h"
#include "nvfuse.h"
#include "nvrm_boot.h"
#include "nvrm_hardware_access.h"
#include "nvddk_kbc.h"
#include "nvddk_blockdevmgr.h"

#include "nvbl_query.h"
#include "nvodm_query.h"
#include "ap20/arapbpm.h"
#include "ap20/nvboot_warm_boot_0.h"
#include "ap20/arclk_rst.h"
#include "nvcrypto_cipher.h"
#include "nvcrypto_hash.h"
#include "nvos.h"

//----------------------------------------------------------------------------------------------
// Configuration parameters
//----------------------------------------------------------------------------------------------

#define OBFUSCATE_PLAIN_TEXT        1       // Set nonzero to obfuscate the plain text LP0 exit code

//----------------------------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------------------------

// NOTE: If more than one of the following is enabled, only one of them will
//       actually be used. RANDOM takes precedence over PATTERN, and PATTERN
//       takes precedence over ZERO.
#define RANDOM_AES_BLOCK_IS_RANDOM  0       // Set nonzero to randomize the header RandomAesBlock
#define RANDOM_AES_BLOCK_IS_PATTERN 0       // Set nonzero to patternize the header RandomAesBlock
#define RANDOM_AES_BLOCK_IS_ZERO    1       // Set nonzero to clear the header RandomAesBlock


// Address at which AP20 LP0 exit code runs
// The address must not overlap the bootrom's IRAM usage
#define AP20_LP0_EXIT_RUN_ADDRESS   0x40020000
#define AP20_IRAM_BASE              0x40000000

//----------------------------------------------------------------------------------------------
// Global External Labels
//----------------------------------------------------------------------------------------------

// Until this code is placed into the TrustZone hypervisor, it will be sitting
// as plain-text in the non-secure bootloader. Use random meaninless strings
// for the entry points to hide the true purpose of this code to make it a
// little harder to identify from a symbol table entry.

void adSklS9DjzHw6Iuy34J8o7dli4ueHy0jg(void);       // Start of LP0 exit assembly code
void xak81lsdmLKSqkl903zLjWpv1b3TfD78k3(void);      // End of LP0 exit assembly code


//----------------------------------------------------------------------------------------------
// Local Functions
//----------------------------------------------------------------------------------------------

#if RANDOM_AES_BLOCK_IS_RANDOM

#define NV_CAR_REGR(reg) NV_READ32((0x60006000 + CLK_RST_CONTROLLER_##reg##_0))

static NvU64 AbootPrivQueryRandomSeed(void)
{
    NvU32   Reg;        // Scratch register
    NvU32   LFSR;       // Linear Feedback Shift Register value
    NvU64   seed = 0;   // Result
    NvU32   i;          // Loop index

    for (i = 0;
         i < (sizeof(NvU64)*8)/NV_FIELD_SIZE(CLK_RST_CONTROLLER_PLL_LFSR_0_RND_RANGE);
         ++i)
    {
        // Read the Linear Feedback Shift Register.
        Reg = NV_CAR_REGR(PLL_LFSR);
        LFSR = NV_DRF_VAL(CLK_RST_CONTROLLER, PLL_LFSR, RND, Reg);

        // Shift the seed and or in this part of the value.
        seed <<= NV_FIELD_SIZE(CLK_RST_CONTROLLER_PLL_LFSR_0_RND_RANGE);
        seed |= LFSR;
    }

    return seed;
}
#endif

static void AbootPrivDetermineCryptoOptions(NvBool* IsEncrypted,
                                            NvBool* IsSigned,
                                            NvBool* UseZeroKey)
{
    NV_ASSERT(IsEncrypted != NULL);
    NV_ASSERT(IsSigned    != NULL);
    NV_ASSERT(UseZeroKey  != NULL);

    switch (NvFuseGetOperatingMode())
    {
        case NvBlOperatingMode_NvProduction:
            *IsEncrypted = NV_FALSE;
            *IsSigned    = NV_TRUE;
            *UseZeroKey  = NV_TRUE;
            break;

        case NvBlOperatingMode_OdmProductionOpen:
            *IsEncrypted = NV_FALSE;
            *IsSigned    = NV_TRUE;
            *UseZeroKey  = NV_FALSE;
            break;

        case NvBlOperatingMode_OdmProductionSecure:
            *IsEncrypted = NV_TRUE;
            *IsSigned    = NV_TRUE;
            *UseZeroKey  = NV_FALSE;
            break;

        case NvBlOperatingMode_Undefined:   // Preproduction or FA
        default:
            *IsEncrypted = NV_FALSE;
            *IsSigned    = NV_FALSE;
            *UseZeroKey  = NV_FALSE;
            break;
    }
}

//----------------------------------------------------------------------------------------------
static NvError AbootPrivEncryptCodeSegment(NvU32 Destination,
                                           NvU32 Source,
                                           NvU32 Length,
                                           NvBool UseZeroKey)
{
    NvError e = NvError_NotInitialized;         // Error code
    NvCryptoCipherAlgoHandle    pAlgo = NULL;   // Crypto algorithm handle
    NvCryptoCipherAlgoParams    Params;         // Crypto algorithm parameters
    const void*                 pSource;        // Pointer to source
    void*                       pDestination;   // Pointer to destination

    // Select crypto algorithm.
    NV_CHECK_ERROR_CLEANUP(
        NvCryptoCipherSelectAlgorithm(
            NvCryptoCipherAlgoType_AesCbc,
            &pAlgo));

    NvOsMemset(&Params, 0, sizeof(Params));

    // Configure crypto algorithm
    if (UseZeroKey)
    {
        Params.AesCbc.KeyType   = NvCryptoCipherAlgoAesKeyType_UserSpecified;
    }
    else
    {
        Params.AesCbc.KeyType   = NvCryptoCipherAlgoAesKeyType_SecureBootKey;
    }
    Params.AesCbc.IsEncrypt     = NV_TRUE;
    Params.AesCbc.KeySize       = NvCryptoCipherAlgoAesKeySize_128Bit;
    Params.AesCbc.PaddingType   = NvCryptoPaddingType_ExplicitBitPaddingOptional;

    NV_CHECK_ERROR_CLEANUP(pAlgo->SetAlgoParams(pAlgo, &Params));

    // Calculate AES block parameters.
    pSource      = (const void*)(Source + offsetof(NvBootWb0RecoveryHeader, RandomAesBlock));
    pDestination = (void*)(Destination  + offsetof(NvBootWb0RecoveryHeader, RandomAesBlock));
    Length      -= offsetof(NvBootWb0RecoveryHeader, RandomAesBlock);

    // Perform encryption.
    e = pAlgo->ProcessBlocks(pAlgo, Length, pSource, pDestination, NV_TRUE, NV_TRUE);

fail:
    pAlgo->ReleaseAlgorithm(pAlgo);
    return e;
}

//----------------------------------------------------------------------------------------------
static NvError AbootPrivSignCodeSegment(NvU32 Source,
                                        NvU32 Length,
                                        NvBool UseZeroKey)
{
    NvError e = NvError_NotInitialized;         // Error code
    NvCryptoHashAlgoHandle      pAlgo = NULL;   // Crypto algorithm handle
    NvCryptoHashAlgoParams      Params;         // Crypto algorithm parameters
    const void*                 pSource;        // Pointer to source
    NvU32   hashSize = sizeof(NvBootHash);      // Size of hash signature

    // Select crypto algorithm.
    NV_CHECK_ERROR_CLEANUP(
        NvCryptoHashSelectAlgorithm(
            NvCryptoHashAlgoType_AesCmac,
            &pAlgo));

    NvOsMemset(&Params, 0, sizeof(Params));

    // Configure crypto algorithm
    if (UseZeroKey)
    {
        Params.AesCmac.KeyType  = NvCryptoCipherAlgoAesKeyType_UserSpecified;
    }
    else
    {
        Params.AesCmac.KeyType  = NvCryptoCipherAlgoAesKeyType_SecureBootKey;
    }
    Params.AesCmac.IsCalculate  = NV_TRUE;
    Params.AesCmac.KeySize      = NvCryptoCipherAlgoAesKeySize_128Bit;
    Params.AesCmac.PaddingType  = NvCryptoPaddingType_ImplicitBitPaddingOptional;
    NV_CHECK_ERROR_CLEANUP(pAlgo->SetAlgoParams(pAlgo, &Params));

    // Calculate AES block parameters.
    pSource = (const void*)(Source + offsetof(NvBootWb0RecoveryHeader, RandomAesBlock));
    Length -= offsetof(NvBootWb0RecoveryHeader, RandomAesBlock);

    // Calculate the signature.
    NV_CHECK_ERROR_CLEANUP(pAlgo->ProcessBlocks(pAlgo, Length, pSource, NV_TRUE, NV_TRUE));

    // Read back the signature and store it in the insecure header.
    e = pAlgo->QueryHash(pAlgo, &hashSize,
            (NvU8*)(Source + offsetof(NvBootWb0RecoveryHeader, Hash)));

fail:

    pAlgo->ReleaseAlgorithm(pAlgo);
    return e;

}

//----------------------------------------------------------------------------------------------
// Global Functions
//----------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------
NvError NvAbootInitializeCodeSegment(NvAbootHandle hAboot,
                                     NvRmMemHandle hSegment,
                                     NvU32 SegAddress)
{
    // Until this code makes it into the TrustZone hypervisor, return NvError_NotInitialized
    // for everything except success to avoid giving anything away since this will be sitting
    // in a plain-text code segment.
    NvError e = NvError_NotInitialized;         // Error code

    NvU32                       Start;          // Start of the actual code
    NvU32                       End;            // End of the actual code
    NvU32                       ActualLength;   // Length of the actual code
    NvU32                       Length;         // Length of the signed/encrypted code
    NvRmPhysAddr                SdramBase;      // Start of SDRAM
    NvU32                       SdramSize;      // Size of SDRAM
    NvU32                       SegLength;      // Segment length
    NvU32                       SegPhysStart;   // Segment physical start address
    NvU32                       SegPhysEnd;     // Segment physical end address
    NvBootWb0RecoveryHeader*    pSrcHeader;     // Pointer to source LP0 exit header
    NvBootWb0RecoveryHeader*    pDstHeader;     // Pointer to destination LP0 exit header
    NvBool                      IsEncrypted;    // Segment is encrypted
    NvBool                      IsSigned;       // Segment is signed
    NvBool                      UseZeroKey;     // Use key of all zeros

    // Determine crypto options.
    AbootPrivDetermineCryptoOptions(&IsEncrypted, &IsSigned, &UseZeroKey);

    // Get the actual code limits.
    Start  = (NvU32)adSklS9DjzHw6Iuy34J8o7dli4ueHy0jg;
    End    = (NvU32)xak81lsdmLKSqkl903zLjWpv1b3TfD78k3;
    ActualLength = End - Start;
    Length = ((ActualLength + 15) >> 4) << 4;

    // Must have a valid Aboot handle and memory handle.
    if ((hAboot == NULL) || (hSegment == NULL))
    {
        // e = NvError_BadParameter;
        goto fail;
    }

    // Get the base address and size of SDRAM.
    NV_CHECK_ERROR_CLEANUP(NvRmOpen(&(hAboot->hRm), 0));
    NvRmModuleGetBaseAddress(hAboot->hRm,
                             NVRM_MODULE_ID(NvRmPrivModuleID_ExternalMemory, 0),
                             &SdramBase, &SdramSize);
    NvRmClose(hAboot->hRm);
    SdramSize = NvOdmQueryMemSize(NvOdmMemoryType_Sdram);

    SegLength = NvRmMemGetSize(hSegment);
    SegPhysStart = NvRmMemGetAddress(hSegment, 0);
    SegPhysEnd = NvRmMemGetAddress(hSegment, SegLength-1);

    // The region specified by SegAddress must be in SDRAM and must be
    // nonzero in length.
    if ((SegLength  == 0)
    ||  (SegAddress == 0)
    ||  (SegPhysStart < SdramBase)
    ||  (SegPhysStart >= (SdramBase + SdramSize - 1))
    ||  (SegPhysEnd <= SdramBase)
    ||  (SegPhysEnd > (SdramBase + SdramSize - 1)))
    {
        // e = NvError_BadParameter;
        goto fail;
    }

    // Things must be 16-byte aligned.
    if ((SegLength & 0xF) || (SegAddress & 0xF))
    {
        // e = NvError_BadParameter;
        goto fail;
    }

    // Will the code fit?
    if (SegLength < Length)
    {
        // e = NvError_InsufficientMemory;
        goto fail;
    }

    // Get a pointers to the source and destination region header.
    pSrcHeader = (NvBootWb0RecoveryHeader*)Start;
    pDstHeader = (NvBootWb0RecoveryHeader*)SegAddress;

    // Populate the RandomAesBlock as requested.
    #if RANDOM_AES_BLOCK_IS_RANDOM
    {
        NvU64*  pRandomAesBlock = (NvU64*)&(pSrcHeader->RandomAesBlock);
        NvU64*  pEnd            = (NvU64*)(((NvU32)pRandomAesBlock)
                                + sizeof(pSrcHeader->RandomAesBlock));
        do
        {
            *pRandomAesBlock++ = AbootPrivQueryRandomSeed();

        } while (pRandomAesBlock < pEnd);
    }
    #elif RANDOM_AES_BLOCK_IS_PATTERN
    {
        NvU32*  pRandomAesBlock = (NvU32*)&(pSrcHeader->RandomAesBlock);
        NvU32*  pEnd            = (NvU32*)(((NvU32)pRandomAesBlock)
                                + sizeof(pSrcHeader->RandomAesBlock));

        do
        {
            *pRandomAesBlock++ = RANDOM_AES_BLOCK_IS_PATTERN;

        } while (pRandomAesBlock < pEnd);
    }
    #elif RANDOM_AES_BLOCK_IS_ZERO
    {
        NvU32*  pRandomAesBlock = (NvU32*)&(pSrcHeader->RandomAesBlock);
        NvU32*  pEnd            = (NvU32*)(((NvU32)pRandomAesBlock)
                                + sizeof(pSrcHeader->RandomAesBlock));

        do
        {
            *pRandomAesBlock++ = 0;

        } while (pRandomAesBlock < pEnd);
    }
    #endif

    // Populate the header.
    pSrcHeader->LengthInsecure     = Length;
    pSrcHeader->LengthSecure       = Length;
    pSrcHeader->Destination        = AP20_LP0_EXIT_RUN_ADDRESS;
    pSrcHeader->EntryPoint         = AP20_LP0_EXIT_RUN_ADDRESS;
    pSrcHeader->RecoveryCodeLength = Length - sizeof(NvBootWb0RecoveryHeader);

    // Need to encrypt?
    if (IsEncrypted)
    {
        // Yes, do it.
        NV_CHECK_ERROR_CLEANUP(AbootPrivEncryptCodeSegment(SegAddress, Start, Length, UseZeroKey));
        pDstHeader->LengthInsecure = Length;
    }
    else
    {
        // No, just copy the code directly.
        NvOsMemcpy(pDstHeader, pSrcHeader, Length);
    }

    // Clear the signature in the destination code segment.
    NvOsMemset(&(pDstHeader->Hash), 0, sizeof(pDstHeader->Hash));

    // Need to sign?
    if (IsSigned)
    {
        // Yes, do it.
        NV_CHECK_ERROR_CLEANUP(AbootPrivSignCodeSegment(SegAddress, Length, UseZeroKey));
    }

    // Pass the LP0 exit code segment address to the OS.

fail:

#if OBFUSCATE_PLAIN_TEXT

    // Blast the plain-text LP0 exit code in the bootloader image.
    NvOsMemset((void*)Start, 0, ActualLength);

    // Clean up the caches.
    NvOsDataCacheWritebackInvalidate();
    NvOsInstrCacheInvalidate();

#endif

    return e;
}
