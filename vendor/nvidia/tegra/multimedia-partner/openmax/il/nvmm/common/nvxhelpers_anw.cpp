/* Copyright (c) 2010-2012 NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an
 * express license agreement from NVIDIA Corporation is strictly prohibited.
 */

extern "C"
{
#include "NvxHelpers.h"
#include "nvxhelpers_int.h"
#include "nvassert.h"
#include <nvgralloc.h>
}

#include <unistd.h>

#include <hardware/gralloc.h>


static NvGrModule const* GrModule = NULL;

NvError NvxAllocateOverlayANW(
    NvU32 *pWidth,
    NvU32 *pHeight,
    NvColorFormat eFormat,
    NvxOverlay *pOverlay,
    ENvxVidRendType eType,
    NvBool bDisplayAtRequestRect,
    NvRect *pDisplayRect,
    NvU32 nLayout)
{
    NvU32 width, height;
    NvError status;
    NvxSurface *pSurface = NULL;
    ANativeWindow *anw = (ANativeWindow *)pOverlay->pANW;
    int minUndequeuedBufs = 0;

    width = *pWidth;
    height = *pHeight;

    pOverlay->srcX = pOverlay->srcY = 0;
    pOverlay->srcW = width;
    pOverlay->srcH = height;

    if (NvSuccess != NvxSurfaceAlloc(&pSurface, width, height,
                                     eFormat, nLayout))
    {
        goto fail;
    }

    pOverlay->pSurface = pSurface;
    ClearYUVSurface(pOverlay->pSurface);


    native_window_api_connect(anw, NATIVE_WINDOW_API_CAMERA);
    native_window_set_scaling_mode(anw,
                    NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW);

    //native_window_connect?
    anw->query(anw, NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS, &minUndequeuedBufs);
    native_window_set_buffer_count(anw, 1 + minUndequeuedBufs);
    native_window_set_buffers_geometry(anw, width, height,
                                       HAL_PIXEL_FORMAT_YV12);

    if (NvSuccess != NvxUpdateOverlayANW(pDisplayRect, pOverlay, NV_TRUE))
        goto fail;


    return NvSuccess;

fail:
    if (pOverlay->pSurface)
        NvxSurfaceFree(&pOverlay->pSurface);

    pOverlay->pSurface = NULL;

    return NvError_BadParameter;
}

NvError NvxUpdateOverlayANW(NvRect *pNewDestRect, NvxOverlay *pOverlay,
                            NvBool bSetSurface)
{
    ANativeWindow *anw = (ANativeWindow *)pOverlay->pANW;

    native_window_set_buffers_geometry(anw, pOverlay->srcW, pOverlay->srcH,
                                       HAL_PIXEL_FORMAT_YV12);

    if (bSetSurface)
    {
        NvxRenderSurfaceToOverlayANW(pOverlay, pOverlay->pSurface, OMX_FALSE);
    }

    return NvSuccess;
}

void NvxReleaseOverlayANW(NvxOverlay *pOverlay)
{
    NvxSurfaceFree(&pOverlay->pSurface);
    pOverlay->pSurface = NULL;
}

void NvxRenderSurfaceToOverlayANW(NvxOverlay *pOverlay,
                                  NvxSurface *pSrcSurface,
                                  OMX_BOOL bWait)
{
    ANativeWindow *anw = (ANativeWindow *)pOverlay->pANW;
    android_native_buffer_t *anbuffer;
    NvNativeHandle *handle;
    NvError Err;
    NvDdk2dBlitParameters TexParam;
    NvDdk2dSurface *pSrcDdk2dSurface = NULL;
    NvDdk2dSurface *pDstDdk2dSurface = NULL;
    NvDdk2dFixedRect SrcRectLocal;
    NvRect SrcRect, DstRect;
    NvDdk2dHandle h2d = NvxGet2d();
    NvRmFence fences[NVGR_MAX_FENCES];
    NvU32 numFences;
    int preFence;

    if (!h2d || !GrModule)
        return;

    NvOsMemset(&TexParam, 0, sizeof(NvDdk2dBlitParameters));

    if (0 != anw->dequeueBuffer(anw, &anbuffer, &preFence))
        return;
    /* XXX - until sync framework is implemented */
    NV_ASSERT(preFence == -1);

    if (0 != GrModule->Base.lock(&GrModule->Base, anbuffer->handle, GRALLOC_USAGE_HW_RENDER,
                                 0, 0, pOverlay->srcW, pOverlay->srcH, NULL)) {
        anw->cancelBuffer(anw, anbuffer, preFence);
        return;
    }

    handle = (NvNativeHandle *)anbuffer->handle;

    // Fetches the SurfaceFlinger fences from the gralloc buffer
    numFences = NVGR_MAX_FENCES;
    GrModule->getfences(&GrModule->Base, anbuffer->handle, fences, (int*)&numFences);

    NV_ASSERT(numFences <= NVDDK_2D_MAX_FENCES);

    NvxLock2d();
    Err = NvDdk2dSurfaceCreate(h2d, NvDdk2dSurfaceType_Y_U_V,
                               &(pSrcSurface->Surfaces[0]), &pSrcDdk2dSurface);
    if (NvSuccess != Err)
    {
        goto L_cleanup;
    }
    Err = NvDdk2dSurfaceCreate(h2d, NvDdk2dSurfaceType_Y_U_V,
                               handle->Surf, &pDstDdk2dSurface);
    if (NvSuccess != Err)
    {
        goto L_cleanup;
    }

    // Add the fences to the destination surface
    NvDdk2dSurfaceLock(pDstDdk2dSurface,
                       NvDdk2dSurfaceAccessMode_Read,
                       NULL,
                       NULL,
                       NULL);
    NvDdk2dSurfaceUnlock(pDstDdk2dSurface, fences, numFences);

    //
    // set the source and destiantion rectangles for the blit. take into account any cropping
    // rectangle that is in effect on the source. the blit destination is always top-left within
    // the android native window.
    //
    DstRect.left   = 0;
    DstRect.top    = 0;
    DstRect.right  = pOverlay->srcW;
    DstRect.bottom = pOverlay->srcH;

    SrcRect.left   = pOverlay->srcX;
    SrcRect.top    = pOverlay->srcY;
    SrcRect.right  = SrcRect.left + pOverlay->srcW;
    SrcRect.bottom = SrcRect.top  + pOverlay->srcH;

    // Set attributes
    NvDdk2dSetBlitFilter(&TexParam, NvDdk2dStretchFilter_Nicest);

    NvDdk2dSetBlitTransform(&TexParam, NvDdk2dTransform_None);

    ConvertRect2Fx(&SrcRect, &SrcRectLocal);
    Err = NvDdk2dBlit(h2d, pDstDdk2dSurface, &DstRect,
                      pSrcDdk2dSurface, &SrcRectLocal, &TexParam);
    if (NvSuccess != Err)
    {
        goto L_cleanup;
    }

    Nvx2dFlush(pDstDdk2dSurface);

#if 0
    // Fetch the blit fence
    NvDdk2dSurfaceLock(pDstDdk2dSurface,
                       NvDdk2dSurfaceAccessMode_Write,
                       NULL,
                       fences,
                       &numFences);
    NvDdk2dSurfaceUnlock(pDstDdk2dSurface, NULL, 0);

    // This is needed only for traditional OMX path (not IOMX),
    // for example when using omxplayer.
    if (pOverlay->StereoOverlayModeFlag)
    {
        handle->Buf->StereoInfo |= pOverlay->StereoOverlayModeFlag;
    }

    // Add the blit fence to the gralloc buffer.  Gralloc accepts only
    // a single write fence, and calling addfence overwrites any
    // previous fences, so we must ensure there is only one here.
    if (numFences) {
        NV_ASSERT(numFences == 1);
        GrModule->addfence(&GrModule->Base, anbuffer->handle, &fences[0]);
    }
#endif

L_cleanup:
    if (pSrcDdk2dSurface)
        NvDdk2dSurfaceDestroy(pSrcDdk2dSurface);
    if (pDstDdk2dSurface)
        NvDdk2dSurfaceDestroy(pDstDdk2dSurface);

    NvxUnlock2d();

    GrModule->Base.unlock(&GrModule->Base, anbuffer->handle);

    if (NvSuccess != Err) {
        anw->cancelBuffer(anw, anbuffer, preFence);
    } else {
        anw->queueBuffer(anw, anbuffer, -1);
    }
}

OMX_U32 NvxGetRotationANW(void)
{
    return 0;
}

void NvxInitANW(void)
{
    hw_module_t const* hwMod;
    hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hwMod);
    GrModule = (NvGrModule*) hwMod;
}

void NvxShutdownANW(void)
{
    // There is currently no API to release the module.
}

void NvxSmartDimmerEnableANW(NvxOverlay *pOverlay, NvBool enable)
{
}

