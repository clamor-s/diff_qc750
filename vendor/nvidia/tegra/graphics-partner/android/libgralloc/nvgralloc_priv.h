/*
 * Copyright (c) 2012-2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_NVGRALLOC_PRIV_H
#define INCLUDED_NVGRALLOC_PRIV_H

typedef struct
{
    int Width;
    int Height;
    int Format;
    int Usage;
    NvRmSurfaceLayout Layout;
} NvGrAllocParameters;

typedef struct NvGrAllocDevRec
{
    alloc_device_t Base;

    // Private methods and data
    int (*alloc_ext)(struct NvGrAllocDevRec *dev, NvGrAllocParameters *params, NvNativeHandle** handle);

} NvGrAllocDev;


#endif
