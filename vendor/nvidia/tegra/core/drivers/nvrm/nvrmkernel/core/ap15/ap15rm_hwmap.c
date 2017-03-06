/*
 * Copyright (c) 2007 - 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvos.h"
#include "nvassert.h"
#include "nvrm_chiplib.h"

NvError NvRmPhysicalMemMap(
    NvRmPhysAddr     phys,
    size_t           size,
    NvU32            flags,
    NvOsMemAttribute memType,
    void           **ptr )
{
    if (NVCPU_IS_X86 || NvRmIsSimulation())
    {
        void *p = NvRmPrivChiplibMap(phys, size);
        if (!p)
            return NvError_InsufficientMemory;
        *ptr = p;
        return NvSuccess;
    }
    else
    {
        return NvOsPhysicalMemMap(phys, size, memType, flags, ptr);
    }
}

void NvRmPhysicalMemUnmap(void *ptr, size_t size)
{
    if (NVCPU_IS_X86 || NvRmIsSimulation())
    {
        NvRmPrivChiplibUnmap(ptr);
    }
    else
    {
        NvOsPhysicalMemUnmap(ptr, size);
    }
}
