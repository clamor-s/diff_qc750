/*
* Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
*
* NVIDIA Corporation and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA Corporation is strictly prohibited.
*/

/** @file
* @brief <b>NVIDIA Driver Development Kit:
*           NvDDK Multimedia APIs</b>
*
* @b Description: NvMM AES format.
*/

#ifndef INCLUDED_NVMM_AES_H
#define INCLUDED_NVMM_AES_H

#include "nvcommon.h"
#include "nvos.h"

#if defined(__cplusplus)
extern "C"
{
#endif

typedef enum {
    NvMMMAesAlgorithmID_AES_ALGO_ID_NOT_ENCRYPTED = 0,
    NvMMMAesAlgorithmID_AES_ALGO_ID_CTR_MODE,
    NvMMMAesAlgorithmID_AES_ALGO_ID_CBC_MODE,
    NvMMMAesAlgorithmID_Force32 = 0x7FFFFFFF
} NvMMMAesAlgorithmID;

/** Metadata for AES widevine */
typedef struct NvMMAesWvMetadataRec
{
    NvU8    Iv[16];
    NvU32   NonAlignedOffset;
    NvMMMAesAlgorithmID AlgorithmID;
} NvMMAesWvMetadata;

#if defined(__cplusplus)
}
#endif

/** @} */

#endif // INCLUDED_NVMM_AES_H


