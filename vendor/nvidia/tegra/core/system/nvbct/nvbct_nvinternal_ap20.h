/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef _INCLUDE_NVBCT_NVINTERNAL_H
#define _INCLUDE_NVBCT_NVINTERNAL_H

#include "nvbootargs.h"
#include "nvbct_customer_platform_data.h"
#include "nvbct.h"
#define EMBEDDED_P852_PROJECT 60852

/* 128 bit (16 bytes) AES is used */
#define NVBOOT_CMAC_AES_HASH_LENGTH_BYTES 16
#define NV_BCT_NUM_HASHED_PARTITION 10

typedef struct NvBctHashedPartitionInfoRec
{
    NvU32 PartitionId;
    NvU32 CryptoHash[NVBOOT_CMAC_AES_HASH_LENGTH_BYTES / 4];
} NvBctHashedPartitionInfo;

#define INTERFACE_NAME_LEN 5
#define ETH_ALEN 6
typedef struct MacIdArrayRawRec {
    NvS8 Interface[INTERFACE_NAME_LEN];
    NvU8 MacId[ETH_ALEN];
} NvMacIdInfoDataRaw;

typedef struct BoardSerialNumRawRec {
    NvU8 SerialNum[8];
} NvBoardSerialNumDataRaw;

typedef struct NvBctHashedPartitionInfoRawRec
{
    NvU8 PartitionId[4];
    NvU8 CryptoHash[NVBOOT_CMAC_AES_HASH_LENGTH_BYTES];
} NvBctHashedPartitionInfoRaw;

#define CURRRENT_SKUINFO_VERSION_AP20 2
typedef struct NvSkuInfoRawRec
{
    NvU8 Version;
    NvU8 TestConfig;
    NvU8 BomPrefix[2];
    NvU8 Project[4];
    NvU8 SkuId[4];
    NvU8 Revision[2];
    NvU8 SkuVersion[2];
    NvU8 Reserved[8];
} NvSkuInfoRaw;

#define CURRRENT_PRODUCT_INFO_VERSION_AP20 2
typedef struct NvProductInfoRawRec
{
    NvU8 BomPrefix[2];
    NvU8 Project[4];
    NvU8 SkuId[4];
    NvU8 Revision[2];
    NvU8 SkuVersion[2];
}NvProdInfoRaw;

/*
 * Note though structure name is NvInternalOneTimeRaw but we are storing
 * hash value on each boot. This was needs so that we can remain compatible with
 * the MODs structure and the release branches
 */
typedef struct NvInternalOneTimeRawRec
{
    NvU8 Reserved1[2];
    NvProdInfoRaw ProductInfoRaw;
    NvBoardSerialNumDataRaw BoardSerialNumDataRaw;
    NvU8 ExtraBoardInfoVersion[4];
    NvBctHashedPartitionInfoRaw BctHashedPartInfoRaw[NV_BCT_NUM_HASHED_PARTITION];
    NvU8 Reserverd2[8];
    NvMacIdInfoDataRaw MacIdInfoDataRaw;
    NvU8 Reserved3[5];
    NvSkuInfoRaw SkuInfoRaw;
}NvInternalOneTimeRaw;

typedef struct NvInternalInfoRawRec
{
    NvInternalOneTimeRaw InternalOneTimeRaw;
} NvInternalInfoRaw;

#endif
