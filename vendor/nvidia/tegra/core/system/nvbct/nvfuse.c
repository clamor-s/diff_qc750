/*
 * Copyright (c) 2009  - 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvassert.h"
#include "nvcommon.h"
#include "nvos.h"
#include "nvrm_hardware_access.h"
#include "nvfuse.h"
#include "nvfuse_private.h"
#include "nvbl_operatingmodes.h"
#include "nvcrypto_hash.h"
#include "nvcrypto_cipher.h"
#include "nvddk_blockdevmgr.h"

#if !NVODM_BOARD_IS_SIMULATION
static NvBool FuseIsSbkSet(NvFuseHal *pHal)
{
 /* The mini-loader build (NV_IS_AVP) does not support the NvCrypto
  * library, so the indirect way of detecting security fuses by checking
  * if there is a hash mismatch of a packet of zeros will not work.  Instead,
  * the fuses are read directly and check if all bits are zero */
#if !NV_IS_AVP
    NvCryptoHashAlgoHandle hAlgo = NULL;
    NvCryptoHashAlgoParams Params;
    NvError e;
    NvU8 HashZeros[] = {
        0x43, 0x87, 0xc1, 0x4b, 0x46, 0xef, 0x7e, 0x17,
        0x6d, 0xce, 0xef, 0xa8, 0x62, 0xd7, 0x2f, 0xf9 
    };
    NvU8 Dummy;
    NvU8 IsValid = NV_FALSE;

    if (NvDdkBlockDevMgrInit()!=NvSuccess)
        return NV_TRUE;

    NV_CHECK_ERROR_CLEANUP(
        NvCryptoHashSelectAlgorithm(NvCryptoHashAlgoType_AesCmac, &hAlgo)
    );

    Params.AesCmac.IsCalculate = NV_TRUE;
    Params.AesCmac.KeyType = NvCryptoCipherAlgoAesKeyType_SecureBootKey;
    Params.AesCmac.KeySize = NvCryptoCipherAlgoAesKeySize_128Bit;
    Params.AesCmac.PaddingType = NvCryptoPaddingType_ImplicitBitPaddingOptional;

    NV_CHECK_ERROR_CLEANUP(hAlgo->SetAlgoParams(hAlgo, &Params));

    NV_CHECK_ERROR_CLEANUP(
        hAlgo->ProcessBlocks(hAlgo, 0, &Dummy, NV_TRUE, NV_TRUE)
    );
    NV_CHECK_ERROR_CLEANUP(hAlgo->VerifyHash(hAlgo, HashZeros, &IsValid));

 fail:
    NvDdkBlockDevMgrDeinit();
    hAlgo->ReleaseAlgorithm(hAlgo);
    return !IsValid;
#else
    return !(pHal->IsSbkAllZeros());
#endif
}
#endif

NvBlOperatingMode NvFuseGetOperatingMode(void)
{
#if NVODM_BOARD_IS_SIMULATION
    //TODO:: need to get this value from command line
    return NvBlOperatingMode_NvProduction;
#else
    NvFuseHal Hal;
    if (NvFuseGetAp20Hal(&Hal) || NvFuseGetT30Hal(&Hal))
    {
        if (Hal.IsOdmProductionMode())
        {
            if (FuseIsSbkSet(&Hal))
                return NvBlOperatingMode_OdmProductionSecure;
            else
                return NvBlOperatingMode_OdmProductionOpen;
        }
        else if (Hal.IsNvProductionMode())
            return NvBlOperatingMode_NvProduction;
        else
            return NvBlOperatingMode_Undefined;
    }
    return NvBlOperatingMode_Undefined;
#endif
}

void NvFuseGetSecondaryBootDevice(NvU32* BootDevice, NvU32* Instance)
{
    NvFuseHal Hal;
    if (NvFuseGetAp20Hal(&Hal) || NvFuseGetT30Hal(&Hal))
        Hal.GetSecBootDevice(BootDevice, Instance);
}
