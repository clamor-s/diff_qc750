/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * @brief <b>nVIDIA Driver Development Kit:
 *           Mpeg2 Video Decode control</b>
 *
 * @b Description: Defines the common structures for mpeg2 video decoders
 */

#ifndef INCLUDED_NVMPEG2VDEC_COMMON_H
#define INCLUDED_NVMPEG2VDEC_COMMON_H

#include "nvsm_mpeg2.h"
#include "nvmm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MPEG2VDEC_MEMALIGN(x, align) ((((x)-1) | ((align)-1)) + 1)

// macroblock type
#define MACROBLOCK_INTRA                        1
#define MACROBLOCK_PATTERN                      2
#define MACROBLOCK_MOTION_BACKWARD              4
#define MACROBLOCK_MOTION_FORWARD               8
#define MACROBLOCK_QUANT                        16
#define SPATIAL_TEMPORAL_WEIGHT_CODE_FLAG       32
#define PERMITTED_SPATIAL_TEMPORAL_WEIGHT_CLASS 64

#define MACROBLOCK_SKIPPED                      128

// motion_type
#define MC_FIELD 1
#define MC_FRAME 2
#define MC_16X8  2
#define MC_DMV   3

// mv_format
#define MV_FIELD 0
#define MV_FRAME 1

// chroma_format
#define CHROMA420 1
#define CHROMA422 2
#define CHROMA444 3

// picture structure
#define TOP_FIELD     1
#define BOTTOM_FIELD  2
#define FRAME_PICTURE 3

// picture coding type
#define NO_TYPE     0
#define I_TYPE      1
#define P_TYPE      2
#define B_TYPE      3
#define D_TYPE      4

// dct type
#define FRAME_DCT   0
#define FIELD_DCT   1

typedef struct
{
    NvS16   MV[2][2][2];
    NvU8    MVField[2][2];
    NvS8    DMVector[2];
    NvU8    MBType;
    NvU8    MotionType;
    NvU8    ValidMB;
    NvU8    CBP;
    NvU8    DCTType;
}MPEG2MBData;

typedef struct
{
    NvU32 Width;
    NvU32 Height;
    NvU32 ActualWidth;
    NvU32 ActualHeight;
    NvU32 CodedWidth;
    NvU32 CodedHeight;
    NvU32 DisplayWidth;
    NvU32 DisplayHeight;
    NvU32 MaxWidth;
    NvU32 MaxHeight;
    NvU32 MBAmax;
    NvU32 ConcealmentMV;
    NvU32 IntraMBCount;
    NvU8  PicStruct;
    NvU8  PicCodingType;
    NvU8  TopFieldFirst;
    NvU8  Progressive;
#define PROGRESSIVE_FRAME 1
#define PROGRESSIVE_SEQUENCE 2
    MPEG2MBData *pMB;
    NvMMSurfaceDescriptor   hIQ;
}NvMPEG2VDec_Common;

#ifdef __cplusplus
}
#endif

#endif
