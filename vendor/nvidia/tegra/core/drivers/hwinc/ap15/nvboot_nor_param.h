/**
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * Defines the parameters and data structure for NOR devices.
 */

#ifndef INCLUDED_NVBOOT_NOR_PARAM_H
#define INCLUDED_NVBOOT_NOR_PARAM_H

#include "nvcommon.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/// Defines the params that can be configured for NOR devices.
typedef struct NvBootNorParamsRec
{
    /// Specifies the value to be programmed into the Nor timing register.
    NvU32 NorTiming;
} NvBootNorParams;

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_NVBOOT_NOR_PARAM_H */
