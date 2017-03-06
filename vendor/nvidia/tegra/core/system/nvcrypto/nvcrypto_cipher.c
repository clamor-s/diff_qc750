/*
 * Copyright (c) 2008 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvcrypto_cipher.h"
#include "nvcrypto_cipher_aes.h"

NvError
NvCryptoCipherSelectAlgorithm(
    NvCryptoCipherAlgoType CipherAlgoType,
    NvCryptoCipherAlgoHandle *pCipherAlgoHandle)
{
    NvError e = NvSuccess;
    
    if (!pCipherAlgoHandle)
        return NvError_InvalidAddress;

    switch (CipherAlgoType)
    {
        case NvCryptoCipherAlgoType_AesCbc:
            NV_CHECK_ERROR(NvCryptoCipherSelectAlgorithmAes(
                               CipherAlgoType,
                               pCipherAlgoHandle));
            break;
        default:
            e = NvError_BadParameter;
            break;
    }
    
    return e;
}

