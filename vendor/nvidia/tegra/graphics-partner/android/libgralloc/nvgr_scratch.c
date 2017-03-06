/*
 * Copyright (c) 2011-2013 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvgralloc.h"
#include "nvgr_scratch.h"
#include "nvgrsurfutil.h"
#include "cutils/log.h"

#if NVGR_USE_TRIPLE_BUFFERING
#define NVGR_NUM_SCRATCH_BUFFERS     3
#else
#define NVGR_NUM_SCRATCH_BUFFERS     2
#endif

struct NvGrScratchMachineRec {
    NvGrScratchClient client;
    NvGrModule *ctx;
    size_t num_sets;
    NvGrScratchSet sets[0];
};

typedef struct NvGrScratchMachineRec NvGrScratchMachine;

static void NvGrFreeScratchSet(NvGrModule *ctx, NvGrScratchSet *buf)
{
    int ii;

    for (ii = 0; ii < NVGR_NUM_SCRATCH_BUFFERS; ii++) {
        if (buf->buffers[ii]) {
            NvGrFreeInternal(ctx, buf->buffers[ii]);
            // Set buffer handle to NULL to avoid double frees
            buf->buffers[ii] = NULL;
        }
    }

    buf->state = NVGR_SCRATCH_BUFFER_STATE_FREE;
}

static int NvGrAllocScratchSet(NvGrModule *ctx, NvGrScratchSet *buf,
                               unsigned int width, unsigned int height,
                               NvColorFormat nv_format,
                               NvRmSurfaceLayout layout)
{
    int stride, usage, ret;
    int format;
    NvNativeHandle *nvbuf;
    int ii;

    usage = GRALLOC_USAGE_HW_2D | GRALLOC_USAGE_HW_FB;

    format = NvGrGetHalFormat(nv_format);

    for (ii = 0; ii < NVGR_NUM_SCRATCH_BUFFERS; ii++) {
        ret = NvGrAllocInternal(ctx, width, height, format,
                                usage, layout,
                                &buf->buffers[ii]);

        if (ret != 0) {
            ALOGE("%s: NvGrAllocInternal failed", __FUNCTION__);
            // Free the already allocated buffers of the set
            NvGrFreeScratchSet(ctx, buf);
            return ret;
        }
    }

    buf->format = nv_format;
    buf->state = NVGR_SCRATCH_BUFFER_STATE_ALLOCATED;
    buf->active_index = 0;
    buf->last_buffer = NULL;
    buf->last_write_count = -1;
    buf->width = width;
    buf->height = height;
    buf->layout = layout;

    return ret;
}

static NvGrScratchSet *
FindScratchSet(NvGrScratchMachine *sm, unsigned int width,
               unsigned int height, NvColorFormat format,
               NvRmSurfaceLayout layout)
{
    NvGrScratchSet *found = NULL, *found_allocated = NULL;
    size_t ii;

    /* scan the pool of available rotate buffers looking for one that
     * matches the layer
     */
    for (ii = 0; ii < sm->num_sets; ii++) {
        NvGrScratchSet *cur = &sm->sets[ii];

        switch (cur->state) {
        case NVGR_SCRATCH_BUFFER_STATE_FREE:
            if (!found) {
                found = cur;
            }
            break;
        case NVGR_SCRATCH_BUFFER_STATE_ALLOCATED:
            /* Check for an exact match */
            if ((cur->buffers[0])->Surf[0].Width == width &&
                (cur->buffers[0])->Surf[0].Height == height &&
                (cur->format == format) &&
                (cur->layout == layout)) {
                /* found a match */
                return cur;
            }
            if (!found_allocated) {
                found_allocated = cur;
            }
            break;
        case NVGR_SCRATCH_BUFFER_STATE_ASSIGNED:
        case NVGR_SCRATCH_BUFFER_STATE_LOCKED:
            /* Already assigned - do not touch */
            break;
        }
    }

    /* In order to reduce thrashing, reusing an allocated buffer is
     * the last option.
     */
    if (!found) {
        NvGrFreeScratchSet(sm->ctx, found_allocated);
        found = found_allocated;
    }

    if (!found) {
        ALOGE("%s: no free slots!", __FUNCTION__);
        /* While this is usually a non-fatal error, its a silent perf
         * issue if we fail to alloc a scratch buffer.  Hitting this
         * case means we require more scratch buffers than we planned
         * for, so it would be good to understand why and possibly
         * increase the pool size.  No customer builds were harmed
         * during the making of this motion picture.
         */
        NV_ASSERT(found);
        return NULL;
    }

    if (0 != NvGrAllocScratchSet(sm->ctx, found, width, height, format, layout)) {
        return NULL;
    }

    return found;
}

static NvGrScratchSet *ScratchAssign(NvGrScratchClient *sc, int transform,
                                     unsigned int width, unsigned int height,
                                     NvColorFormat format, NvRect *src_crop)
{
    NvGrScratchMachine *sm = (NvGrScratchMachine *) sc;
    NvGrScratchSet *buf;
    unsigned int align_width, align_height;
    NvRmSurfaceLayout layout;

    // Ensure even size for YUV surfaces
    if (NV_COLOR_GET_COLOR_SPACE(format) != NvColorSpace_LinearRGBA) {
        width = NvGrAlignUp(width, 2);
        height = NvGrAlignUp(height, 2);
    }

    if (transform & HAL_TRANSFORM_ROT_90) {
        int fast_rotate_blit_align = NvGrGetFastRotateAlign(format);

        NVGR_SWAP(width, height);
        align_width = NvGrAlignUp(width, fast_rotate_blit_align);
        align_height = NvGrAlignUp(height, fast_rotate_blit_align);
        /* Tiled for fast rotation */
        layout = NvRmSurfaceLayout_Tiled;
    } else {
        align_width = width;
        align_height = height;
        /* Untiled allows 3D compositing on T20/T30 */
        layout = NvRmSurfaceLayout_Pitch;
    }

    buf = FindScratchSet(sm, align_width, align_height, format, layout);

    if (buf) {
        NV_ASSERT(buf->state == NVGR_SCRATCH_BUFFER_STATE_ALLOCATED);
        buf->state = NVGR_SCRATCH_BUFFER_STATE_ASSIGNED;

        /* Store the blit size.  If padding is required this will be
         * applied inside ddk2d.
         */
        buf->width = width;
        buf->height = height;

        if (src_crop) {
            /* If the crop has changed, invalidate the cached image */
            if (memcmp(src_crop, &buf->src_crop, sizeof(buf->src_crop))) {
                buf->last_buffer = NULL;
                buf->last_write_count = -1;
            }
            buf->src_crop = *src_crop;
            buf->use_src_crop = NV_TRUE;
        } else {
            buf->use_src_crop = NV_FALSE;
        }
    }

    return buf;
}

static void ScratchLock(NvGrScratchClient *sc, NvGrScratchSet *buf)
{
    NV_ASSERT(buf->state == NVGR_SCRATCH_BUFFER_STATE_ASSIGNED);
    buf->state = NVGR_SCRATCH_BUFFER_STATE_LOCKED;
}

static void ScratchUnlock(NvGrScratchClient *sc, NvGrScratchSet *buf)
{
    NV_ASSERT(buf->state == NVGR_SCRATCH_BUFFER_STATE_LOCKED ||
              buf->state == NVGR_SCRATCH_BUFFER_STATE_ALLOCATED);
    buf->state = NVGR_SCRATCH_BUFFER_STATE_ALLOCATED;
}

static void ScratchFrameStart(NvGrScratchClient *sc)
{
    NvGrScratchMachine *sm = (NvGrScratchMachine *) sc;
    size_t ii;

    /* reset the list of scratch buffers */
    for (ii = 0; ii < sm->num_sets; ii++) {
        NvGrScratchSet *buf = &sm->sets[ii];

        if (buf->state == NVGR_SCRATCH_BUFFER_STATE_ASSIGNED) {
            buf->state = NVGR_SCRATCH_BUFFER_STATE_ALLOCATED;
        }
    }
}

static void ScratchFrameEnd(NvGrScratchClient *sc)
{
    NvGrScratchMachine *sm = (NvGrScratchMachine *) sc;
    size_t ii;

    for (ii = 0; ii < sm->num_sets; ii++) {
        NvGrScratchSet *buf = &sm->sets[ii];

        if (buf->state == NVGR_SCRATCH_BUFFER_STATE_ALLOCATED) {
            /* Buffer is not used in this config and may be released */
            NvGrFreeScratchSet(sm->ctx, buf);
        }
    }
}

static NvNativeHandle *ScratchGetBuffer(NvGrScratchClient *sc,
                                        NvGrScratchSet *buf)
{
    return buf->buffers[buf->active_index];
}

static int ScratchBlit(NvGrScratchClient *sc, NvNativeHandle *src, int srcIndex,
                       NvGrScratchSet *buf, int transform, NvPoint *offset)
{
    NvGrScratchMachine *sm = (NvGrScratchMachine *) sc;
    NvNativeHandle *dst;
    NvRect src_rect, dst_rect;
    NvRect *src_crop;
    NvError err;
    int status;

    NV_ASSERT(buf->state == NVGR_SCRATCH_BUFFER_STATE_ASSIGNED ||
              buf->state == NVGR_SCRATCH_BUFFER_STATE_LOCKED);

    if (src == buf->last_buffer &&
        src->Buf->writeCount == buf->last_write_count &&
        (src->Buf->Flags & NvGrBufferFlags_Posted)) {
        /* skip this blit -- buffer did not change */
        *offset = buf->offset;
        return 0;
    } else {
        /* do the blit into the next buffer */
        buf->active_index = (buf->active_index + 1) % NVGR_NUM_SCRATCH_BUFFERS;
        buf->last_buffer = src;
        buf->last_write_count = src->Buf->writeCount;
        src->Buf->Flags |= NvGrBufferFlags_Posted;
    }

    dst = buf->buffers[buf->active_index];

    if (buf->use_src_crop) {
        src_crop = &buf->src_crop;
    } else {
        /* Use the entire source */
        src_rect.top = 0;
        src_rect.left = 0;
        src_rect.right = src->Surf->Width;
        src_rect.bottom = src->Surf->Height;

        src_crop = &src_rect;
    }

    /* Use the scratch buffer size for dest, which may differ from the
     * surface size
     */
    dst_rect.top = 0;
    dst_rect.left = 0;
    dst_rect.right = buf->width;
    dst_rect.bottom = buf->height;

    status = NvGr2dBlitExt(sm->ctx, src, srcIndex, dst, 0, src_crop, &dst_rect,
                           transform & HAL_TRANSFORM_ROT_90, &buf->offset);
    *offset = buf->offset;
    return status;
}

static int ScratchComposite(NvGrScratchClient *sc,
                            struct NvGrCompositeListRec *list,
                            NvGrScratchSet *buf)
{
    NvGrScratchMachine *sm = (NvGrScratchMachine *) sc;
    NvNativeHandle *dst;

    NV_ASSERT(buf);
    if (!buf) {
        return -1;
    }

    NV_ASSERT(buf->state == NVGR_SCRATCH_BUFFER_STATE_ASSIGNED ||
              buf->state == NVGR_SCRATCH_BUFFER_STATE_LOCKED);

    /* do the blit into the next buffer */
    buf->active_index = (buf->active_index + 1) % NVGR_NUM_SCRATCH_BUFFERS;
    dst = buf->buffers[buf->active_index];

    sm->ctx->composite(sm->ctx, dst, list);

    return 0;
}

static NvGrScratchClient *ScratchClientAlloc(NvGrModule *ctx, size_t count)
{
    size_t bytes = sizeof(NvGrScratchMachine) + count * sizeof(NvGrScratchSet);
    NvGrScratchMachine *sm = (NvGrScratchMachine *) malloc(bytes);

    if (sm) {
        memset(sm, 0, bytes);
        sm->ctx = ctx;
        sm->num_sets = count;
        sm->client.assign = ScratchAssign;
        sm->client.lock = ScratchLock;
        sm->client.unlock = ScratchUnlock;
        sm->client.get_buffer = ScratchGetBuffer;
        sm->client.frame_start = ScratchFrameStart;
        sm->client.frame_end = ScratchFrameEnd;
        sm->client.blit = ScratchBlit;
        sm->client.composite = ScratchComposite;
    }

    return (NvGrScratchClient *) sm;
}

static void ScratchClientFree(NvGrModule *ctx, NvGrScratchClient *sc)
{
    NvGrScratchMachine *sm = (NvGrScratchMachine *) sc;

    if (sm) {
        size_t ii;

        for (ii = 0; ii < sm->num_sets; ii++) {
            NvGrFreeScratchSet(ctx, &sm->sets[ii]);
        }

        free(sm);
    }
}

static int FindClientSlot(NvGrModule *ctx, NvGrScratchClient *sc)
{
    size_t ii;

    for (ii = 0; ii < NVGR_MAX_SCRATCH_CLIENTS; ii++) {
        if (ctx->scratch_clients[ii] == sc) {
            return ii;
        }
    }

    return -1;
}

int NvGrScratchOpen(NvGrModule *ctx, size_t count, NvGrScratchClient **scratch)
{
    int slot = FindClientSlot(ctx, NULL);

    if (slot >= 0) {
        NvGrScratchClient *sc = ScratchClientAlloc(ctx, count);
        NV_ASSERT(ctx->scratch_clients[slot] == NULL);

        if (sc) {
            *scratch = ctx->scratch_clients[slot] = sc;
            return 0;
        }
    }

    return -1;
}

void NvGrScratchClose(NvGrModule *ctx, NvGrScratchClient *scratch)
{
    int slot = FindClientSlot(ctx, scratch);

    NV_ASSERT(slot >= 0 && slot < NVGR_MAX_SCRATCH_CLIENTS);
    NV_ASSERT(ctx->scratch_clients[slot] == scratch);
    ctx->scratch_clients[slot] = NULL;
    ScratchClientFree(ctx, scratch);
}

NvError NvGrScratchInit(NvGrModule *ctx)
{
    return NvSuccess;
}

void NvGrScratchDeInit(NvGrModule *ctx)
{
    size_t ii;

    for (ii = 0; ii < NVGR_MAX_SCRATCH_CLIENTS; ii++) {
        ScratchClientFree(ctx, ctx->scratch_clients[ii]);
        ctx->scratch_clients[ii] = NULL;
    }
}
