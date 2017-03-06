/*
 * Copyright (c) 2005 - 2008 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvfxmath.h"


NvSFx NvSFxAsinD (NvSFx x)
{
    x <<= 7;
    return NvSFxAtan2D(x, NvSFxSqrt (NV_SFX_MUL (NV_SFX_ADD (NV_SFX_ONE << 7, x), 
                                                 NV_SFX_SUB (NV_SFX_ONE << 7, x))));
}

NvSFx NvSFxAcosD (NvSFx x)
{
    x <<= 7;
    return NvSFxAtan2D(NvSFxSqrt (NV_SFX_MUL (NV_SFX_ADD (NV_SFX_ONE << 7, x), 
                                            NV_SFX_SUB (NV_SFX_ONE << 7, x))), x);
}
