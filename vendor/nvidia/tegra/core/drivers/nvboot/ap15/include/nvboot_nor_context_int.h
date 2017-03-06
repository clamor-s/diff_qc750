/**
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/*
 * nvboot_nor_context.h - Definitions for the NOR context structure.
 */

#ifndef INCLUDED_NVBOOT_NOR_CONTEXT_INT_H
#define INCLUDED_NVBOOT_NOR_CONTEXT_INT_H

#include "ap15/nvboot_nor_param.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/*
 * NvBootNorContext - The context structure for the NOR driver.
 * A pointer to this structure is passed into the driver's Init() routine.
 * This pointer is the only data that can be kept in a global variable for
 * later reference.
 */
typedef struct NvBootNorContextRec
{
    /// Nothing to store in the context.
    NvU32 DummyVariable;
} NvBootNorContext;


#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_NVBOOT_NOR_CONTEXT_INT_H */
