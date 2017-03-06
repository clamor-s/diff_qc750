/*
 * Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
 * 
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_DC_TVO_COMMON_H
#define INCLUDED_DC_TVO_COMMON_H

#include "nvcommon.h"
#include "nverror.h"
#include "nvos.h"

#if defined(__cplusplus)
extern "C"
{
#endif

typedef struct TvoEncoderDataRec
{
    NvU8 addr;
    NvU8 data;
} TvoEncoderData;

/* the artvo spec doesn't #define the indirect register offsets. bug 347933. */
#if !defined(NV_PTVO_INDIR_CHROMA_BYTE3)

#define NV_PTVO_INDIR_CHROMA_BYTE3          0x0
#define NV_PTVO_INDIR_CHROMA_BYTE2          0x1
#define NV_PTVO_INDIR_CHROMA_BYTE1          0x2
#define NV_PTVO_INDIR_CHROMA_BYTE0          0x3
#define NV_PTVO_INDIR_CHROMA_PHASE          0x4
#define NV_PTVO_INDIR_MISC0                 0x5
#define NV_PTVO_INDIR_MISC1                 0x6
#define NV_PTVO_INDIR_MISC2                 0x7
#define NV_PTVO_INDIR_HSYNC_WIDTH           0x8
#define NV_PTVO_INDIR_BURST_WIDTH           0x9
#define NV_PTVO_INDIR_BACK_PORCH            0xa
#define NV_PTVO_INDIR_CB_BURST_LEVEL        0xb
#define NV_PTVO_INDIR_CR_BURST_LEVEL        0xc
#define NV_PTVO_INDIR_SLAVE_MODE            0xd
#define NV_PTVO_INDIR_BLACK_LEVEL_MSB       0xe
#define NV_PTVO_INDIR_BLACK_LEVEL_LSB       0xf
#define NV_PTVO_INDIR_BLANK_LEVEL_MSB       0x10
#define NV_PTVO_INDIR_BLANK_LEVEL_LSB       0x11
#define NV_PTVO_INDIR_NUM_LINES_MSB         0x17
#define NV_PTVO_INDIR_NUM_LINES_LSB         0x18
#define NV_PTVO_INDIR_WHITE_LEVEL_MSB       0x1e
#define NV_PTVO_INDIR_WHITE_LEVEL_LSB       0x1f
#define NV_PTVO_INDIR_CR_GAIN               0x20
#define NV_PTVO_INDIR_CB_GAIN               0x22
#define NV_PTVO_INDIR_TINT                  0x25
#define NV_PTVO_INDIR_BREEZE_WAY            0x29
#define NV_PTVO_INDIR_FRONT_PORCH           0x2c
#define NV_PTVO_INDIR_ACTIVELINE_MSB        0x31
#define NV_PTVO_INDIR_ACTIVELINE_LSB        0x32
#define NV_PTVO_INDIR_FIRSTVIDEOLINE        0x33
#define NV_PTVO_INDIR_MISC3                 0x34
#define NV_PTVO_INDIR_SYNC_LEVEL            0x35
#define NV_PTVO_INDIR_VBI_BLANK_LEVEL_MSB   0x3c
#define NV_PTVO_INDIR_VBI_BLANK_LEVEL_LSB   0x3d
#define NV_PTVO_INDIR_SOFT_RESET            0x3e
#define NV_PTVO_INDIR_PRODUCT_VERSION       0x3f
#define NV_PTVO_INDIR_CHROMA_FREQ2_BYTE3    0x50
#define NV_PTVO_INDIR_CHROMA_FREQ2_BYTE2    0x51
#define NV_PTVO_INDIR_CHROMA_FREQ2_BYTE1    0x52
#define NV_PTVO_INDIR_CHROMA_FREQ2_BYTE0    0x53

/* leaving out the CC, TT, and macrovision registers for now */
#endif

/**
 * Macrovision data is packed into a glob. The glob layout is as follows:
 *
 * Glob header:
 *
 *  0                   1                   2                   3   
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | version                                                       | 
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | num entries                                                   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | entries                                                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * ....
 * 
 * Entry:
 *
 *  0                   1                   2                   3   
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | entry size (including entry header)                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | entry type                                                    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | hardware type                                                 |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | num registers                                                 |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | reg addr                      | reg data                      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * ...
 *
 * NOTE: if the performance of the data lookup becomes a bottle-neck, then
 *       adding an index into the glob header should fix it.
 */

#define NV_MV_GLOB_HEADER_SIZE          (2 * 4)
#define NV_MV_GLOB_ENTRY_HEADER_SIZE    (4 * 4)

typedef enum
{
    NvMvGlobEntryType_Ntsc_Disable = 0x0,
    NvMvGlobEntryType_Ntsc_NoSplitBurst = 0x1,
    NvMvGlobEntryType_Ntsc_SplitBurst_2_Line = 0x2,
    NvMvGlobEntryType_Ntsc_SplitBurst_4_Line = 0x3,

    NvMvGlobEntryType_Pal_Disable = 0x4,
    NvMvGlobEntryType_Pal_Enable = 0x5,

    NvMvGlobEntryType_HighDef_Enable = 0x6,
    NvMvGlobEntryType_HighDef_Disable = 0x7,

    NvMvGlobEntryType_Force32 = 0x7FFFFFFF,
} NvMvGlobEntryType;

typedef enum
{
    NvMvGlobHardwareType_Cve3 = 0x1,
    NvMvGlobHardwareType_Cve5 = 0x2,

    NvMvGlobHardwareType_Force32 = 0x7FFFFFFF,
} NvMvGlobHardwareType;

/**
 *
 */
typedef void * (*NvOdmDispTvoGetGlobType)( NvU32 *size );

/**
 * De-allocates/clears glob memory.
 */
typedef void (*NvOdmDispTvoReleaseGlobType)( void *glob );

void * DcTvoCommonGlobOpen( void );
void DcTvoCommonGlobClose( void *glob );

NvBool DcTvoCommonGetMvData( void *glob, NvMvGlobEntryType entry_type,
    NvMvGlobHardwareType hw_type, NvU32 *num, TvoEncoderData **data );

#if defined(__cplusplus)
}
#endif

#endif
