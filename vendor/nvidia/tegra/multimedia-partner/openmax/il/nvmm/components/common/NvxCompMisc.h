/* Copyright (c) 2006 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an
 * express license agreement from NVIDIA Corporation is strictly prohibited.
 */

/** NvxCompReg.h
 *
 *  NVIDIA's lists of components for OMX Component Registration.
 */

#ifndef _NVX_COMP_MISC_H
#define _NVX_COMP_MISC_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <OMX_Core.h>
#include <stdlib.h>

typedef struct {
    const OMX_U8    *data;
    int             size;
    OMX_U8          *dataEnd;
    OMX_U8          *currentPtr;
    OMX_U32         currentBuffer;
    int             bitCount;
    int             zeroCount;
} NVX_OMX_NALU_BIT_STREAM;

typedef struct {
    OMX_U32 profile_idc;
    OMX_U32 constraint_set0123_flag;
    OMX_U32 level_idc;

    OMX_U32 nWidth;
    OMX_U32 nHeight;
} NVX_OMX_SPS;

typedef struct {
    OMX_U32 entropy_coding_mode_flag;
} NVX_OMX_PPS;

OMX_BOOL NVOMX_ParseSPS(OMX_U8* data, int len, NVX_OMX_SPS *pSPS);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

