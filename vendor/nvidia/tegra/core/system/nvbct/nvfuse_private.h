/**
 * Copyright (c) 2009 - 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVFUSE_PRIVATE_H
#define INCLUDED_NVFUSE_PRIVATE_H

#include "nvcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct NvFuseHalRec
{
    NvBool (*IsOdmProductionMode)(void);
    NvBool (*IsNvProductionMode)(void);
    NvBool (*IsSbkAllZeros)(void);
    void (*GetSecBootDevice)(NvU32*, NvU32*);
} NvFuseHal;

NvBool NvFuseGetAp20Hal(NvFuseHal *);
NvBool NvFuseGetT30Hal(NvFuseHal *);

#ifdef __cplusplus
}
#endif

#endif
