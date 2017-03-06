/*
 * Copyright (c) 2007 - 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVODM_MOUSE_INT_H
#define INCLUDED_NVODM_MOUSE_INT_H

#include "nvodm_services.h"
#include "nvodm_touch.h"

// Module debug: 0=disable, 1=enable
#define NVODMMOUSE_ENABLE_PRINTF (0)

#if (NVODMMOUSE_ENABLE_PRINTF)
#define NVODMMOUSE_PRINTF(x)   NvOdmOsDebugPrintf x
#else
#define NVODMMOUSE_PRINTF(x)
#endif

#if defined(__cplusplus)
extern "C"
{
#endif

typedef struct NvOdmMouseDeviceRec
{
    NvBool                      CompressionEnabled;
    NvU8                        CompressionState;
    NvU32                       NumBytesPerSample;
} NvOdmMouseDevice;

#if defined(__cplusplus)
}
#endif

/** @} */

#endif // INCLUDED_NVODM_MOUSE_INT_H

