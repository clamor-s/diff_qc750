/*
 * Copyright (c) 2005-2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/*
 * ap15rm_hwcontext_reginfo.h
 *
 * Structure definition and extern for h/w register information
 */

#ifndef NVRM_HWCONTEXT_REGINFO_H
#define NVRM_HWCONTEXT_REGINFO_H

#include "nvcommon.h"

typedef struct {
    NvS16 count;
    NvU16 offset;
#if NV_DEBUG
    char *name;
#endif
} NvRm3DContext;

extern const NvRm3DContext Rm3DContext[];

typedef struct {
    NvS16 count;
    NvU16 offset;
    char *name;
} NvRmMpeContext;

extern const NvRmMpeContext RmMpeContext[];

#endif
