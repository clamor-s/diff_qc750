/*
 * Copyright (c) 2009-2013 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvgralloc.h"
#include "nvassert.h"
#include <cutils/log.h>

static void CopyEdges(NvGrModule *m, NvNativeHandle *h,
                      NvDdk2dSurface *srcSurface,
                      NvDdk2dSurface *dstSurface);

int NvGr2dInit(NvGrModule *ctx)
{
    NvError err;
    int status = 0;

    err = NvDdk2dOpen(ctx->Rm, NULL, &ctx->Ddk2d);
    if (err != NvSuccess) {
        ALOGE("init_rotation_ctx: NvDdk2dOpen failed (%d)", err);
        goto deinit;
    }

    return 0;

deinit:
    NvGr2dDeInit(ctx);
    return -1;
}

void NvGr2dDeInit(NvGrModule *ctx)
{
    if (ctx->Ddk2d) {
        NvDdk2dClose(ctx->Ddk2d);
        ctx->Ddk2d = NULL;
    }
}

typedef enum {
    AccessRead,
    AccessWrite,
} AccessMode;

static NvDdk2dSurface *
get_2d_wrapper(NvGrModule *ctx, NvNativeHandle *buffer, int index)
{
    NV_ASSERT(index >= 0 && index < NVGR_MAX_DDK2D_SURFACES);

    if (buffer->ddk2dSurfaces[index] == NULL) {
        NvError err;
        NvDdk2dSurfaceType type;

        switch (buffer->SurfCount) {
        case 1:
            NV_ASSERT(index == 0);
            type = NvDdk2dSurfaceType_Single;
            break;
        case 2:
            if (buffer->Type == NV_NATIVE_BUFFER_YUV) {
                NV_ASSERT(index == 0);
                type = NvDdk2dSurfaceType_Y_UV;
            } else {
                NV_ASSERT(index < (int)buffer->SurfCount);
                type = NvDdk2dSurfaceType_Single;
            }
            break;
        case 3:
            NV_ASSERT(index == 0);
            type = NvDdk2dSurfaceType_Y_U_V;
            break;
        default:
            ALOGE("Unexpected SurfCount %d", buffer->SurfCount);
            NV_ASSERT(0);
            return NULL;
        }

        err = NvDdk2dSurfaceCreate(ctx->Ddk2d, type, &buffer->Surf[index],
                                   &buffer->ddk2dSurfaces[index]);
        if (err != NvSuccess) {
            ALOGE("%s: couldn't create ddk2d wrapper (err=%x)",
                  __FUNCTION__, err);
            return NULL;
        }
    }

    return buffer->ddk2dSurfaces[index];
}

static int acquire_buffer(NvGrModule *ctx, NvNativeHandle *buffer,
                          AccessMode access, NvRmFence *fences, int *numFences)
{
    /* lock surface */
    int err = ctx->Base.lock(&ctx->Base,
                             (buffer_handle_t) buffer,
                             access == AccessRead ?
                             GRALLOC_USAGE_HW_TEXTURE : // read usage
                             GRALLOC_USAGE_HW_RENDER,   // write usage
                             0, 0,
                             buffer->Surf[0].Width, buffer->Surf[0].Height,
                             NULL);

    if (err != 0) {
        ALOGE("%s: failed to lock surface (err=%d)", __FUNCTION__, err);
        return -1;
    }

    /* ask gralloc for the fences on the surface */
    ctx->getfences(&ctx->Base, (buffer_handle_t) buffer, fences, numFences);

    return 0;
}

static void release_buffer(NvGrModule *ctx, NvNativeHandle *buffer,
                           AccessMode access, NvRmFence *fences, int numFences)
{
    int err, ii;

    /* transfer 2d fences into gralloc, filtering invalid syncpoint IDs */
    for (ii = 0; ii < numFences; ii++) {
        if (fences[ii].SyncPointID != NVRM_INVALID_SYNCPOINT_ID) {
            ctx->addfence(&ctx->Base, (buffer_handle_t) buffer, &fences[ii]);
        }
    }

    /* unlock surface in gralloc */
    err = ctx->Base.unlock(&ctx->Base, (buffer_handle_t) buffer);
    if (err != 0) {
        ALOGE("%s: gralloc unlock failed (err=%d)", __FUNCTION__, err);
    }

    return;
}

static NvDdk2dSurface *acquire_surface(NvGrModule *ctx, NvNativeHandle *buffer,
                                       int index, AccessMode access)
{
    NvDdk2dSurface *surf = get_2d_wrapper(ctx, buffer, index);

    if (surf) {
        NvRmFence fences[NVGR_MAX_FENCES];
        int numFences = NVGR_MAX_FENCES;

        if (acquire_buffer(ctx, buffer, access, fences, &numFences)) {
            return NULL;
        }

        /* transfer gralloc fences into ddk2d:
         * do a CPU wait on any pending nvddk2d fences (there should be
         * none), then unlock with the fences from gralloc.
         *
         * Note that ddk2d access mode is the opposite of the gralloc
         * usage.  If we locked the gralloc buffer for read access, it
         * will return write fences, so we are giving ddk2d write fences.
         */
        NvDdk2dSurfaceLock(surf, access == AccessRead ?
                           NvDdk2dSurfaceAccessMode_Write :
                           NvDdk2dSurfaceAccessMode_Read,
                           NULL, NULL, NULL);
        NvDdk2dSurfaceUnlock(surf, fences, numFences);
    }

    return surf;
}

static void release_surface(NvGrModule *ctx, NvNativeHandle *buffer, int index,
                            AccessMode access)
{
    NvDdk2dSurface *surf = get_2d_wrapper(ctx, buffer, index);

    /* Not reachable if surface creation failed */
    NV_ASSERT(surf);

    if (surf) {
        NvRmFence fences[NVDDK_2D_MAX_FENCES];
        NvU32 numFences;

        /* Lock with usage opposite the 2d operation we just performed
         * to ensure we get the right kind of fence.  If we just read from
         * the surface we need to lock for write to get the read fence.
         */
        NvDdk2dSurfaceLock(surf, access == AccessRead ?
                           NvDdk2dSurfaceAccessMode_Write :
                           NvDdk2dSurfaceAccessMode_Read,
                           NULL, fences, &numFences);
        /* unlock without any fences */
        NvDdk2dSurfaceUnlock(surf, NULL, 0);

        release_buffer(ctx, buffer, access, fences, numFences);
    }
}

static int NeedFilter(int transform, NvRect *src, NvRect *dst)
{
    int src_w = src->right - src->left;
    int src_h = src->bottom - src->top;
    int dst_w = dst->right - dst->left;
    int dst_h = dst->bottom - dst->top;

    if (transform & HAL_TRANSFORM_ROT_90) {
        src_w -= dst_h;
        src_h -= dst_w;
    } else {
        src_w -= dst_w;
        src_h -= dst_h;
    }

    return src_w | src_h;
}

static void Intersection(NvRect* r, const NvRect* r0, const NvRect* r1)
{
    r->left = NV_MAX(r0->left, r1->left);
    r->top = NV_MAX(r0->top, r1->top);
    r->right = NV_MIN(r0->right, r1->right);
    r->bottom = NV_MIN(r0->bottom, r1->bottom);
}

static NvBool CompositionIsOpaque(NvGrCompositeList *list)
{
    int ii;

    /* Convex hull of the transparent area in this composition blit */
    NvRect transp = list->clip;

    /* Each opaque layer shrinks the transparent hull */
    for (ii = 0; ii < list->nLayers; ii++) {
        NvGrCompositeLayer *l = &list->layers[ii];
        if (l->blend != NVGR_COMPOSITE_BLEND_NONE)
            continue;
        if (l->dst.top <= transp.top && l->dst.bottom >= transp.top) {
            /* This opaque layer spans the remaining area vertically */
            if (l->dst.left <= transp.left) {
                transp.left = NV_MAX(transp.left, l->dst.right);
            } else if (l->dst.right >= transp.right) {
                transp.right = NV_MIN(transp.right, l->dst.left);
            }
        } else if (l->dst.left <= transp.left && l->dst.right >= transp.right) {
            /* This opaque layer spans the remaining area horizontally */
            if (l->dst.top <= transp.top) {
                transp.top = NV_MAX(transp.top, l->dst.bottom);
            } else if (l->dst.bottom >= transp.bottom) {
                transp.bottom = NV_MIN(transp.bottom, l->dst.top);
            }
        }
    }

    /* The composition is opaque if we end up with a degenerate
     * transparent area. */
    return (transp.left >= transp.right) || (transp.bottom <= transp.top);
}

int NvGr2dComposite(NvGrModule *ctx, NvNativeHandle *dst,
                    struct NvGrCompositeListRec *list)
{
    NvDdk2dBlitParameters params;
    NvDdk2dSurface *dstSurface;
    NvDdk2dSurface *srcSurface[NVGR_COMPOSITE_LIST_MAX];
    NvError err;
    int ii;

    /* Required on T20/T30 for NvDdk2D 3D backend */
    NV_ASSERT(dst->Surf[0].Layout == NvRmSurfaceLayout_Pitch);

    /* Lock all surfaces */
    dstSurface = acquire_surface(ctx, dst, 0, AccessWrite);
    if (!dstSurface) {
        ALOGE("%s: dst surface failed to lock", __FUNCTION__);
        return -1;
    }

    for (ii = 0; ii < list->nLayers; ii++) {
        NvNativeHandle *src = list->layers[ii].buffer;

        /* Required on T20/T30 for NvDdk2D 3D backend */
        NV_ASSERT(src->Surf[0].Layout == NvRmSurfaceLayout_Pitch);

        srcSurface[ii] = acquire_surface(ctx, src, 0, AccessRead);
        if (!srcSurface[ii]) {
            ALOGE("%s: src %d surface failed to lock", __FUNCTION__, ii);

            /* unlock surfaces locked so far */
            while (--ii >= 0) {
                release_surface(ctx, list->layers[ii].buffer, 0, AccessRead);
            }
            release_surface(ctx, dst, 0, AccessWrite);
            return -1;
        }
    }

    if (!CompositionIsOpaque(list)) {
        /* Clear the destination buffer if it is not covered by opaque
         * layers.
         */
        NvDdk2dColor fill;
        fill.Format = dst->Surf[0].ColorFormat;
        *((NvU32*)(void*)fill.Value) = 0;

        params.ValidFields = 0;
        NvDdk2dSetBlitFill(&params, &fill);

        err = NvDdk2dBlit(ctx->Ddk2d, dstSurface, NULL, NULL, NULL, &params);
        if (err != NvSuccess) {
            ALOGE("%s: Clearing dest surface failed (err=%d)", __FUNCTION__, err);
        }
    }

    params.Flags =
        NvDdk2dBlitFlags_AllowExtendedSrcRect |
        NvDdk2dBlitFlags_KeepTempMemory;
    params.Blend.PerPixelAlpha = NvDdk2dPerPixelAlpha_Enabled;
    params.Blend.ConstantAlpha = 255;
    params.ValidFields =
        NvDdk2dBlitParamField_Blend |
        NvDdk2dBlitParamField_Filter |
        NvDdk2dBlitParamField_Flags |
        NvDdk2dBlitParamField_Transform;

    for (ii = 0; ii < list->nLayers; ii++) {
        NvRect *srcRect = &list->layers[ii].src;
        NvRect *dstRect = &list->layers[ii].dst;
        NvRect clip;
        NvDdk2dFixedRect srcFixed;

        Intersection(&clip, dstRect, &list->clip);
        NvDdk2dSetBlitClipRects(&params, &clip, 1);

        if (NeedFilter(list->layers[ii].transform, srcRect, dstRect)) {
            params.Filter = NvDdk2dStretchFilter_Nicest;

            /* inset by 1/2 pixel to avoid filtering outside the image */
            srcFixed.left   = NV_SFX_FP_TO_FX(srcRect->left + 0.5);
            srcFixed.top    = NV_SFX_FP_TO_FX(srcRect->top + 0.5);
            srcFixed.right  = NV_SFX_FP_TO_FX(srcRect->right - 0.5);
            srcFixed.bottom = NV_SFX_FP_TO_FX(srcRect->bottom - 0.5);
        } else {
            params.Filter = NvDdk2dStretchFilter_Nearest;

            srcFixed.left   = NV_SFX_WHOLE_TO_FX(srcRect->left);
            srcFixed.top    = NV_SFX_WHOLE_TO_FX(srcRect->top);
            srcFixed.right  = NV_SFX_WHOLE_TO_FX(srcRect->right);
            srcFixed.bottom = NV_SFX_WHOLE_TO_FX(srcRect->bottom);
        }

        switch (list->layers[ii].transform) {
        case 0:
            params.Transform = NvDdk2dTransform_None;
            break;
        case HAL_TRANSFORM_ROT_90:
            params.Transform = NvDdk2dTransform_Rotate270;
            break;
        case HAL_TRANSFORM_ROT_180:
            params.Transform = NvDdk2dTransform_Rotate180;
            break;
        case HAL_TRANSFORM_ROT_270:
            params.Transform = NvDdk2dTransform_Rotate90;
            break;
        case HAL_TRANSFORM_FLIP_H:
            params.Transform = NvDdk2dTransform_FlipHorizontal;
            break;
        case HAL_TRANSFORM_FLIP_V:
            params.Transform = NvDdk2dTransform_FlipVertical;
            break;
        case HAL_TRANSFORM_FLIP_H | HAL_TRANSFORM_ROT_90:
            params.Transform = NvDdk2dTransform_InvTranspose;
            break;
        case HAL_TRANSFORM_FLIP_V | HAL_TRANSFORM_ROT_90:
            params.Transform = NvDdk2dTransform_Transpose;
            break;
        default:
            NV_ASSERT(!"Unknown transform");
            params.Transform = NvDdk2dTransform_None;
            break;
        }

        switch (list->layers[ii].blend) {
        case NVGR_COMPOSITE_BLEND_NONE:
            params.Blend.Func = NvDdk2dBlendFunc_Copy;
            break;
        case NVGR_COMPOSITE_BLEND_PREMULT:
            params.Blend.Func = NvDdk2dBlendFunc_SrcOver;
            break;
        case NVGR_COMPOSITE_BLEND_COVERAGE:
            params.Blend.Func = NvDdk2dBlendFunc_SrcOverNonPre;
            break;
        }

        err = NvDdk2dBlit(ctx->Ddk2d, dstSurface, dstRect,
                          srcSurface[ii], &srcFixed, &params);
        if (err != NvSuccess) {
            ALOGE("%s: NvDdk2dBlit failed (err=%d)", __FUNCTION__, err);
        }
    }

    /* Unlock all surfaces.  Note these are done in a separate loop
     * from above for two reasons:
     * 1) Needs to be this way if we switch to the ddk2d compositor
     *    interface which renders all layers in a single pass;
     * 2) Unlock causes a 2D flush and we only want to flush once.
     */
    for (ii = 0; ii < list->nLayers; ii++) {
        release_surface(ctx, list->layers[ii].buffer, 0, AccessRead);
    }

    release_surface(ctx, dst, 0, AccessWrite);

    return 0;
}

static int NvGr2dBlitCommon(NvGrModule *ctx, NvNativeHandle *src, int srcIndex,
                            NvNativeHandle *dst, int dstIndex,
                            NvRect *srcRect, NvRect *dstRect,
                            NvPoint *dstOffset, int transform, int edges)
{
    NvDdk2dSurface *srcSurface, *dstSurface;
    int status = 0;

    srcSurface = acquire_surface(ctx, src, srcIndex, AccessRead);
    dstSurface = acquire_surface(ctx, dst, dstIndex, AccessWrite);

    if (srcSurface && dstSurface) {
        NvDdk2dBlitParameters params;
        NvDdk2dBlitParametersOut paramsOut;
        NvDdk2dFixedRect srcFixed;
        NvError err;

        switch (transform) {
        case HAL_TRANSFORM_ROT_90:
            params.Transform = NvDdk2dTransform_Rotate270;
            break;
        case HAL_TRANSFORM_ROT_180:
            params.Transform = NvDdk2dTransform_Rotate180;
            break;
        case HAL_TRANSFORM_ROT_270:
            params.Transform = NvDdk2dTransform_Rotate90;
            break;
        default:
            params.Transform = NvDdk2dTransform_None;
            break;
        }
        params.Flags = NvDdk2dBlitFlags_AllowExtendedSrcRect;
        if (dstOffset) {
            params.Flags |= NvDdk2dBlitFlags_ReturnDstCrop;
        }
        params.ValidFields =
            NvDdk2dBlitParamField_Transform |
            NvDdk2dBlitParamField_Flags;

        if (NeedFilter(transform, srcRect, dstRect)) {
            params.Filter = NvDdk2dStretchFilter_Nicest;
            params.ValidFields |= NvDdk2dBlitParamField_Filter;

            /* inset by 1/2 pixel to avoid filtering outside the image */
            srcFixed.left   = NV_SFX_FP_TO_FX(srcRect->left + 0.5);
            srcFixed.top    = NV_SFX_FP_TO_FX(srcRect->top + 0.5);
            srcFixed.right  = NV_SFX_FP_TO_FX(srcRect->right - 0.5);
            srcFixed.bottom = NV_SFX_FP_TO_FX(srcRect->bottom - 0.5);
        } else {
            srcFixed.left   = NV_SFX_WHOLE_TO_FX(srcRect->left);
            srcFixed.top    = NV_SFX_WHOLE_TO_FX(srcRect->top);
            srcFixed.right  = NV_SFX_WHOLE_TO_FX(srcRect->right);
            srcFixed.bottom = NV_SFX_WHOLE_TO_FX(srcRect->bottom);
        }

        NvDdk2dModifyBlitFlags(&params, NvDdk2dBlitFlags_KeepTempMemory, 0);
        err = NvDdk2dBlitExt(ctx->Ddk2d, dstSurface, dstRect,
                             srcSurface, &srcFixed, &params,
                             dstOffset ? &paramsOut : NULL);
        if (err == NvSuccess) {
            if (dstOffset) {
                *dstOffset = paramsOut.DstOffset;
            }

            if (edges) {
                CopyEdges(ctx, src, srcSurface, dstSurface);
            }
        } else {
            ALOGE("%s: NvDdk2dBlit failed (err=%d)", __FUNCTION__, err);
            status = -1;
        }
    } else {
        ALOGE("%s: failed to lock surfaces", __FUNCTION__);
        status = -1;
    }

    /* unlock surfaces */
    if (srcSurface) {
        release_surface(ctx, src, dstIndex, AccessRead);
    }
    if (dstSurface) {
        release_surface(ctx, dst, dstIndex, AccessWrite);
    }

    return status;
}

int NvGr2dBlit(NvGrModule *ctx, NvNativeHandle *src, NvNativeHandle *dst,
               NvRect *srcRect, NvRect *dstRect, int transform, int edges)
{
    return NvGr2dBlitCommon(ctx, src, 0, dst, 0, srcRect, dstRect, NULL,
                            transform, edges);
}

int NvGr2dBlitExt(NvGrModule *ctx, NvNativeHandle *src, int srcIndex,
                  NvNativeHandle *dst, int dstIndex,
                  NvRect *srcRect, NvRect *dstRect, int transform,
                  NvPoint *dstOffset)
{
    return NvGr2dBlitCommon(ctx, src, srcIndex, dst, dstIndex, srcRect, dstRect,
                            dstOffset, transform, NV_FALSE);
}

int NvGr2DClear(NvGrModule *ctx, NvNativeHandle *h)
{
    NvDdk2dSurface *surface = acquire_surface(ctx, h, 0, AccessWrite);

    if (surface) {
        NvError err;
        NvDdk2dBlitParameters bp;
        NvDdk2dColor fill;

        fill.Format = NvColorFormat_R8G8B8A8;
        *((NvU32*)(void*)fill.Value) = 0;

        bp.ValidFields = 0;
        NvDdk2dSetBlitFill(&bp, &fill);

        err = NvDdk2dBlit(ctx->Ddk2d, surface, NULL, NULL, NULL, &bp);
        if (err != NvSuccess) {
            ALOGE("%s: NvDdk2dBlit failed (err=%d)", __FUNCTION__, err);
        }

        release_surface(ctx, h, 0, AccessWrite);

        /* This function is called for (nearly) all surfaces at
         * creation but most won't require a ddk2dSurface later so
         * destroy it now.
         */
        NV_ASSERT(h->ddk2dSurfaces[0] == surface);
        NvDdk2dSurfaceDestroy(h->ddk2dSurfaces[0]);
        h->ddk2dSurfaces[0] = NULL;
        return 0;
    } else {
        ALOGE("%s: failed to lock surface", __FUNCTION__);
        return -1;
    }
}

int NvGr2dCopyBuffer(NvGrModule *ctx, NvNativeHandle *src, NvNativeHandle *dst)
{
    NvRect src_rect, dst_rect;

    src_rect.left   = 0;
    src_rect.top    = 0;
    src_rect.right  = src->Surf[0].Width;
    src_rect.bottom = src->Surf[0].Height;

    dst_rect.left   = 0;
    dst_rect.top    = 0;
    dst_rect.right  = dst->Surf[0].Width;
    dst_rect.bottom = dst->Surf[0].Height;

    return NvGr2dBlit(ctx, src, dst, &src_rect, &dst_rect, 0,
                      src->Type == NV_NATIVE_BUFFER_YUV &&
                      (src->Buf->Flags & NvGrBufferFlags_SourceCropValid));
}

static void CopyEdges(NvGrModule *m, NvNativeHandle *h,
                      NvDdk2dSurface *srcSurface,
                      NvDdk2dSurface *dstSurface)
{
    NvDdk2dFixedRect src_rect;
    NvRect dst_rect;
    NvError err;

    /* The border must be 2 pixels instead of 1 because the UV planes are
     * subsampled.  2 in Y is 1 in UV.
     */
    #define BORDER 2

    /* Perform the blits. */
    if (h->Buf->SourceCrop.left >= BORDER) {
        dst_rect.left   = h->Buf->SourceCrop.left - BORDER;
        dst_rect.right  = h->Buf->SourceCrop.left;
        dst_rect.top    = h->Buf->SourceCrop.top;
        dst_rect.bottom = h->Buf->SourceCrop.bottom;

        src_rect.left   = NV_SFX_WHOLE_TO_FX(dst_rect.left + BORDER);
        src_rect.right  = NV_SFX_WHOLE_TO_FX(dst_rect.right + BORDER);
        src_rect.top    = NV_SFX_WHOLE_TO_FX(dst_rect.top);
        src_rect.bottom = NV_SFX_WHOLE_TO_FX(dst_rect.bottom);

        err = NvDdk2dBlit(m->Ddk2d,
                          dstSurface, &dst_rect,
                          srcSurface, &src_rect,
                          NULL);
        if (NvSuccess != err) {
            ALOGE("%s:%d: NvDdk2dBlit failed: 0x%08x", __FILE__, __LINE__, err);
        }
    }

    if (h->Surf[0].Width - h->Buf->SourceCrop.right >= BORDER) {
        dst_rect.left   = h->Buf->SourceCrop.right;
        dst_rect.right  = h->Buf->SourceCrop.right + BORDER;
        dst_rect.top    = h->Buf->SourceCrop.top;
        dst_rect.bottom = h->Buf->SourceCrop.bottom;

        src_rect.left   = NV_SFX_WHOLE_TO_FX(dst_rect.left - BORDER);
        src_rect.right  = NV_SFX_WHOLE_TO_FX(dst_rect.right - BORDER);
        src_rect.top    = NV_SFX_WHOLE_TO_FX(dst_rect.top);
        src_rect.bottom = NV_SFX_WHOLE_TO_FX(dst_rect.bottom);

        err = NvDdk2dBlit(m->Ddk2d,
                          dstSurface, &dst_rect,
                          srcSurface, &src_rect,
                          NULL);
        if (NvSuccess != err) {
            ALOGE("%s:%d: NvDdk2dBlit failed: 0x%08x", __FILE__, __LINE__, err);
        }
    }

    if (h->Buf->SourceCrop.top >= BORDER) {
        dst_rect.left   = h->Buf->SourceCrop.left;
        dst_rect.right  = h->Buf->SourceCrop.right;
        dst_rect.top    = h->Buf->SourceCrop.top - BORDER;
        dst_rect.bottom = h->Buf->SourceCrop.top;

        if (h->Buf->SourceCrop.left >= BORDER) {
            dst_rect.left -= BORDER;
        }
        if (h->Surf[0].Width - h->Buf->SourceCrop.right >= BORDER) {
            dst_rect.right += BORDER;
        }

        src_rect.left   = NV_SFX_WHOLE_TO_FX(dst_rect.left);
        src_rect.right  = NV_SFX_WHOLE_TO_FX(dst_rect.right);
        src_rect.top    = NV_SFX_WHOLE_TO_FX(dst_rect.top + BORDER);
        src_rect.bottom = NV_SFX_WHOLE_TO_FX(dst_rect.bottom + BORDER);

        err = NvDdk2dBlit(m->Ddk2d,
                          dstSurface, &dst_rect,
                          srcSurface, &src_rect,
                          NULL);
        if (NvSuccess != err) {
            ALOGE("%s:%d: NvDdk2dBlit failed: 0x%08x", __FILE__, __LINE__, err);
        }
    }

    /* Make an exception for 1080p bottom - we've never seen a problem
     * in practice and the bottom is typically cropped.
     */
    if (!(h->Surf[0].Height == 1088 && h->Buf->SourceCrop.bottom == 1080) &&
        h->Surf[0].Height - h->Buf->SourceCrop.bottom >= BORDER) {
        dst_rect.left   = h->Buf->SourceCrop.left;
        dst_rect.right  = h->Buf->SourceCrop.right;
        dst_rect.top    = h->Buf->SourceCrop.bottom;
        dst_rect.bottom = h->Buf->SourceCrop.bottom + BORDER;

        if (h->Buf->SourceCrop.left >= BORDER) {
            dst_rect.left -= BORDER;
        }
        if (h->Surf[0].Width - h->Buf->SourceCrop.right >= BORDER) {
            dst_rect.right += BORDER;
        }

        src_rect.left   = NV_SFX_WHOLE_TO_FX(dst_rect.left);
        src_rect.right  = NV_SFX_WHOLE_TO_FX(dst_rect.right);
        src_rect.top    = NV_SFX_WHOLE_TO_FX(dst_rect.top - BORDER);
        src_rect.bottom = NV_SFX_WHOLE_TO_FX(dst_rect.bottom - BORDER);

        err = NvDdk2dBlit(m->Ddk2d,
                          dstSurface, &dst_rect,
                          srcSurface, &src_rect,
                          NULL);
        if (NvSuccess != err) {
            ALOGE("%s:%d: NvDdk2dBlit failed: 0x%08x", __FILE__, __LINE__, err);
        }
    }

}

void NvGr2dCopyEdges(NvGrModule *m, NvNativeHandle *h)
{
    NvRmFence fence;
    NvU32 numFences;
    NvDdk2dSurface *surface;

    NV_ASSERT(h->Type == NV_NATIVE_BUFFER_YUV);
    NV_ASSERT(h->SurfCount == 3);

    if ((h->Buf->Flags & NvGrBufferFlags_SourceCropValid) == 0)
        return;

    surface = get_2d_wrapper(m, h, 0);
    if (!surface)
        return;

    /* We should only be here for a read lock */
    NV_ASSERT(!(h->Buf->LockState & NVGR_WRITE_LOCK_FLAG));

    /* XXX - We're accessing the fences directly because we're
     * ignoring the usage.  The buffer is locked for read but we're
     * going to read and write.
     */
    pthread_mutex_lock(&h->Buf->FenceMutex);
    fence = h->Buf->Fence[0];
    pthread_mutex_unlock(&h->Buf->FenceMutex);

    if (fence.SyncPointID != NVRM_INVALID_SYNCPOINT_ID) {
        NvDdk2dSurfaceLock(surface,
                           NvDdk2dSurfaceAccessMode_ReadWrite,
                           NULL, NULL, NULL);
        NvDdk2dSurfaceUnlock(surface, &fence, 1);
    }

    CopyEdges(m, h, surface, surface);

    NvDdk2dSurfaceLock(surface,
                       NvDdk2dSurfaceAccessMode_ReadWrite,
                       NULL, &fence, &numFences);
    NV_ASSERT(numFences <= 1);
    NvDdk2dSurfaceUnlock(surface, NULL, 0);

    /* This is evil.  The surface is locked for read (texture) but we
     * wrote the surface, so we need to update the write fence.  This
     * should be safe because the write fence is mutex protected.
     */
    if (fence.SyncPointID != NVRM_INVALID_SYNCPOINT_ID) {
        pthread_mutex_lock(&h->Buf->FenceMutex);
        h->Buf->Fence[0] = fence;
        pthread_mutex_unlock(&h->Buf->FenceMutex);
    }
}

/* This should be used for operations where the src and dst buffers
 * are already locked or don't need to be locked (e.g. shadow buffer).
 */
int NvGr2dCopyBufferLocked(NvGrModule *ctx, NvNativeHandle *src,
                           NvNativeHandle *dst,
                           const NvRmFence *srcWriteFenceIn,
                           const NvRmFence *dstReadFencesIn, NvU32 numDstReadFencesIn,
                           NvRmFence *fenceOut)
{
    NvError err;
    NvDdk2dSurface *srcSurface = get_2d_wrapper(ctx, src, 0);
    NvDdk2dSurface *dstSurface = get_2d_wrapper(ctx, dst, 0);

    if (!srcSurface || !dstSurface) {
        ALOGE("%s: ddk2d wrappers do not exist", __FUNCTION__);
        return -1;
    }

    // Src surface may have pending writes
    if (srcWriteFenceIn) {
        NvDdk2dSurfaceLock(srcSurface, NvDdk2dSurfaceAccessMode_Write,
                           NULL, NULL, NULL);
        NvDdk2dSurfaceUnlock(srcSurface, srcWriteFenceIn, 1);
    }

    // Dst surface may have pending reads
    if (numDstReadFencesIn) {
        NvDdk2dSurfaceLock(dstSurface, NvDdk2dSurfaceAccessMode_Read,
                           NULL, NULL, NULL);
        NvDdk2dSurfaceUnlock(dstSurface, dstReadFencesIn, numDstReadFencesIn);
    }

    err = NvDdk2dBlit(ctx->Ddk2d, dstSurface, NULL, srcSurface,
                      NULL, NULL);
    if (err != NvSuccess) {
        ALOGE("%s: NvDdk2dBlit failed (err=%d)", __FUNCTION__, err);
        return -1;
    }

    if (fenceOut == NULL) {
        // Do a CPU wait
        NvDdk2dSurfaceLock(dstSurface, NvDdk2dSurfaceAccessMode_Read,
                           NULL, NULL, NULL);
        NvDdk2dSurfaceUnlock(dstSurface, NULL, 0);
    } else {
        // Extract the fence from ddk2d (there should be only one)
        NvU32 numFencesOut;
        NvRmFence fencesOutTmp[NVDDK_2D_MAX_FENCES];

        NvDdk2dSurfaceLock(dstSurface, NvDdk2dSurfaceAccessMode_Read,
                           NULL, fencesOutTmp, &numFencesOut);
        NvDdk2dSurfaceUnlock(dstSurface, NULL, 0);

        NV_ASSERT(numFencesOut == 1);
        *fenceOut = fencesOutTmp[0];
    }
    return 0;
}

