/*
 * Copyright (c) 2011-2013 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */
#ifndef _NVGR_SCRATCH_H
#define _NVGR_SCRATCH_H
#include "nvgralloc.h"

#define NVGR_MAX_SCRATCH_BUFFERS     3

enum NvGrScratchState {
    NVGR_SCRATCH_BUFFER_STATE_FREE = 0,
    NVGR_SCRATCH_BUFFER_STATE_ALLOCATED,
    NVGR_SCRATCH_BUFFER_STATE_ASSIGNED,
    NVGR_SCRATCH_BUFFER_STATE_LOCKED,
};

struct NvGrScratchSetRec {
    NvNativeHandle *buffers[NVGR_MAX_SCRATCH_BUFFERS];
    NvNativeHandle *last_buffer;
    NvU32 last_write_count;
    int active_index;
    enum NvGrScratchState state;

    /* Requested format */
    NvColorFormat format;

    /* Requested size */
    unsigned int width;
    unsigned int height;
    NvRmSurfaceLayout layout;

    /* Source crop */
    NvRect src_crop;
    NvBool use_src_crop;

    /* ddk2d blit offset */
    NvPoint offset;
};

struct NvGrScratchClientRec {
    NvGrScratchSet *(*assign)(NvGrScratchClient *sc, int transform,
                              size_t width, size_t height,
                              NvColorFormat format, NvRect *src_crop);
    void (*lock)(NvGrScratchClient *sc, NvGrScratchSet *buf);
    void (*unlock)(NvGrScratchClient *sc, NvGrScratchSet *buf);
    NvNativeHandle *(*get_buffer)(NvGrScratchClient *sc, NvGrScratchSet *buf);
    void (*frame_start)(NvGrScratchClient *sc);
    void (*frame_end)(NvGrScratchClient *sc);
    int (*blit)(NvGrScratchClient *sc, NvNativeHandle *src, int srcIndex,
                NvGrScratchSet *buf, int transform, NvPoint *offset);
    int (*composite)(NvGrScratchClient *sc, struct NvGrCompositeListRec *list,
                     NvGrScratchSet *buf);
};

NvError NvGrScratchInit(NvGrModule *ctx);
void NvGrScratchDeInit(NvGrModule *ctx);
int NvGrScratchOpen(NvGrModule *ctx, size_t count, NvGrScratchClient **scratch);
void NvGrScratchClose(NvGrModule *ctx, NvGrScratchClient *scratch);
#endif