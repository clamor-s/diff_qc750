/**
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/*
 * set.h - Definitions for the buildimage state setting code.
 */

/*
 * TODO / Notes
 * - Add doxygen commentary
 */

#ifndef INCLUDED_AP20_SET_H
#define INCLUDED_AP20_SET_H

#include "../nvbuildbct_config.h"
#include "../nvbuildbct.h"
#include "../parse.h"

#if defined(__cplusplus)
extern "C"
{
#endif

int
Ap20SetBootLoader(BuildBctContext *Context,
                  NvU8 *Filename,
                  NvU32 LoadAddress,
                  NvU32 EntryPoint);

void
Ap20ComputeRandomAesBlock(BuildBctContext *Context);

int
Ap20SetRandomAesBlock(BuildBctContext *Context, NvU32 Value, NvU8* AesBlock);

int
Ap20SetSdmmcParam(BuildBctContext *Context,
                  NvU32              Index,
                  ParseToken         Token,
                  NvU32              Value);

int
Ap20SetNandParam(BuildBctContext *Context,
                  NvU32              Index,
                  ParseToken         Token,
                  NvU32              Value);

int
Ap20SetNorParam(BuildBctContext *Context,
                  NvU32              Index,
                  ParseToken         Token,
                  NvU32              Value);

int
Ap20SetSpiFlashParam(BuildBctContext *Context,
                  NvU32              Index,
                  ParseToken         Token,
                  NvU32              Value);

int
Ap20SetSdramParam(BuildBctContext *Context,
                  NvU32              Index,
                  ParseToken         Token,
                  NvU32              Value);

int
Ap20SetOsMediaEmmcParam(BuildBctContext *Context,
                  NvU32              Index,
                  ParseToken         Token,
                  NvU32              Value);

int
Ap20SetOsMediaNandParam(BuildBctContext *Context,
                  NvU32              Index,
                  ParseToken         Token,
                  NvU32              Value);

int
Ap20SetValue(BuildBctContext *Context,
                  ParseToken Token,
                  NvU32 Value);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_SET_H */
