/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_NVDDK_FUSE_BRIDGE_H
#define INCLUDED_NVDDK_FUSE_BRIDGE_H

#if defined(__cplusplus)
extern "C"
{
#endif

#include "nvcommon.h"
#include "nvddk_fuse.h"

typedef struct FuseCoreRec {
    NvU32
        (*pGetArrayLength)(void);
    NvU32
        (*pGetTypeSize)(NvDdkFuseDataType Type);
    NvU32
        (*pGetSecBootDevInstance)(NvU32 BootDev, NvU32 BootDevConfig);
    NvU32
        (*pGetFuseToDdkDevMap)(NvU32 RegData);
    void
        (*pGetUniqueId)(NvU64 *pUniqueId);
}FuseCore;

#ifdef __cplusplus
}
#endif

#endif
