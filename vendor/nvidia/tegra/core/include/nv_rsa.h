/*
 * Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef __NV_RSA_H__
#define __NV_RSA_H__

#include "nvos.h"

#define RSANUMBYTES (256)
#define RSA_PUBLIC_EXPONENT_VAL (65537)
#define RSA_LENGTH (2048)

//big int mod
#define BIGINT_MAX_DW 64

typedef struct _BigIntModulus
{
    NvU32 digits;
    NvU32 invdigit;
    NvU32 n[BIGINT_MAX_DW];
    NvU32 r2[BIGINT_MAX_DW];
} NvBigIntModulus;

/**
*   @param Dgst Message digest for which signature needs to be generated
*   @param PrivKeyN Private key modulus
*   @param PrivkeyD Private key exponent
*   @param Sign Generated signature for the message digest and privatekey
*   @retval 0 on success and -1 on failure
*/
NvS32 NvRSASignSHA1Dgst(NvU8 *Dgst, NvBigIntModulus *PrivKeyN,
                        NvU32 *PrivKeyD, NvU32 *Sign);

/**
*   @param Dgst Message digest for which signature to be verified
*   @param PrivKeyN Private key modulus
*   @param PrivkeyD Private key exponent
*   @param Sign signature which has to be verified for the given message
*               digest with the given public key
*   @retval 0 on success and -1 on failure
*/

NvS32 NvRSAVerifySHA1Dgst(NvU8 *Dgst, NvBigIntModulus *PubKeyN,
                          NvU32 *PubKeyE, NvU32 *Sign);


#endif  /*  #ifdef __NV_RSA_H__ */
