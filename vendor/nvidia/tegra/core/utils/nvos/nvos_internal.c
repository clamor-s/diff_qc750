/*
 * Copyright 2007 - 2008 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include <stdlib.h>
#include <ctype.h>
#include "nvos.h"
#include "nvos_internal.h"
#include "nvassert.h"

NvU32 NvOsHashJenkins(const void* data, NvU32 length)
{
    /* TODO [kraita 02/Dec/08] Insert proper Jenkins one at the time hash from, e.g.,
     * http://burtleburtle.net/bob/hash/doobs.html 
     * Once we have permission to do so.
     */
    NvU32 hash = 0;
    NvU32 shift = 0;
    NvU32 i;
    const NvU8* k = (const NvU8*)data;

    for (i=0; i < length; i++)
    {
        hash = (hash + ((k[i] << shift) | k[i]) * 31);
        shift = (shift + 6) & 0xf;        
    }

    return hash;
}
