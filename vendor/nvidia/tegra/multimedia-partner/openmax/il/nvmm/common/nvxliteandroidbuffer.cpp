/* Copyright (c) 2010-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an
 * express license agreement from NVIDIA Corporation is strictly prohibited.
 */

extern "C" {
#include "NvxComponent.h"
#include "NvxHelpers.h"
#include "nvmmlitetransformbase.h"
#include <nvgralloc.h>
#include <nvassert.h>
}

#include "nvxliteandroidbuffer.h"

#include <hardware/gralloc.h>
#include <media/hardware/HardwareAPI.h>
#include <hardware/hardware.h>

using namespace android;

static NvGrModule const *GrModule = NULL;

OMX_ERRORTYPE HandleEnableANBLite(NvxComponent *pNvComp, OMX_U32 portallowed,
                              void *ANBParam)
{
    EnableAndroidNativeBuffersParams *eanbp;
    eanbp = (EnableAndroidNativeBuffersParams *)ANBParam;

    if (eanbp->nPortIndex != portallowed)
    {
        return OMX_ErrorBadParameter;
    }

    OMX_COLOR_FORMATTYPE *pColorFormat =
        &pNvComp->pPorts[portallowed].oPortDef.format.video.eColorFormat;

    if (eanbp->enable)
    {
        int format = *pColorFormat & ~NVGR_PIXEL_FORMAT_TILED;
        int layout = *pColorFormat & NVGR_PIXEL_FORMAT_TILED;
        switch(format)
        {
            case OMX_COLOR_FormatYUV420Planar:
                format = NVGR_PIXEL_FORMAT_YUV420 | layout;
                break;
            case OMX_COLOR_FormatYUV420SemiPlanar:
                format = NVGR_PIXEL_FORMAT_YUV420_NV12 | layout;
                break;
            default:
                format = HAL_PIXEL_FORMAT_YV12;
                break;
        }
        *pColorFormat = (OMX_COLOR_FORMATTYPE)format;
    }
    else
    {
        *pColorFormat = OMX_COLOR_FormatYUV420Planar;
    }

    NvOsDebugPrintf("%s:%d internal format eColorFormat=0x%x", __FUNCTION__, __LINE__, *pColorFormat);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE HandleUseANBLite(NvxComponent *pNvComp, void *ANBParam)
{
    UseAndroidNativeBufferParams *uanbp;
    uanbp = (UseAndroidNativeBufferParams *)ANBParam;

    return NvxUseBufferInt(pNvComp->hBaseComponent,
                           uanbp->bufferHeader, uanbp->nPortIndex,
                           uanbp->pAppPrivate, 0,
                           (OMX_U8 *)uanbp->nativeBuffer.get(),
                           NVX_BUFFERTYPE_ANDROID_NATIVE_BUFFER_T);
}

OMX_ERRORTYPE HandleUseNBHLite(NvxComponent *pNvComp, void *NBHParam)
{
    NVX_PARAM_USENATIVEBUFFERHANDLE *unbhp;
    unbhp = (NVX_PARAM_USENATIVEBUFFERHANDLE *)NBHParam;

    return NvxUseBufferInt(pNvComp->hBaseComponent,
                           unbhp->bufferHeader, unbhp->nPortIndex,
                           NULL, 0,
                           (OMX_U8 *)unbhp->oNativeBufferHandlePtr,
                           NVX_BUFFERTYPE_ANDROID_BUFFER_HANDLE_T);
}

OMX_ERRORTYPE ImportAllANBsLite(NvxComponent *pNvComp,
                            SNvxNvMMLiteTransformData *pData,
                            OMX_U32 streamIndex,
                            OMX_U32 matchWidth,
                            OMX_U32 matchHeight)
{
    SNvxNvMMLitePort *pPort = &(pData->oPorts[streamIndex]);
    NvxPort *pPortOut = pPort->pOMXPort;
    OMX_BUFFERHEADERTYPE *pBuffer;
    NvxList *pOutBufList = pPortOut->pEmptyBuffers;
    NvxListItem *cur = NULL;

    NvxMutexLock(pPortOut->hMutex);

    pBuffer = pPortOut->pCurrentBufferHdr;
    if (pBuffer)
    {
        ImportANBLite(pNvComp, pData, streamIndex, pBuffer,
                  matchWidth, matchHeight);
    }

    NvOsMutexLock(pOutBufList->oLock);
    cur = pOutBufList->pHead;
    while (cur)
    {
        pBuffer = (OMX_BUFFERHEADERTYPE *)cur->pElement;
        ImportANBLite(pNvComp, pData, streamIndex, pBuffer,
                  matchWidth, matchHeight);
        cur = cur->pNext;
    }
    NvOsMutexUnlock(pOutBufList->oLock);

    NvxMutexUnlock(pPortOut->hMutex);

    NvOsDebugPrintf("imported: %d buffers\n", pPort->nBufsTot);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE ImportANBLite(NvxComponent *pNvComp,
                        SNvxNvMMLiteTransformData *pData,
                        OMX_U32 streamIndex,
                        OMX_BUFFERHEADERTYPE *buffer,
                        OMX_U32 matchWidth,
                        OMX_U32 matchHeight)
{
    android_native_buffer_t *anbuffer = NULL;
    NvMMBuffer *nvmmbuf = NULL;
    NvNativeHandle *handle;
    OMX_U32 width, height;
    NvRmDeviceHandle hRm = NvxGetRmDevice();
    NvRmFence fences[NVGR_MAX_FENCES];
    NvU32 i, numFences;
    NvMMSurfaceDescriptor *pSurfaces;
    SNvxNvMMLitePort *pPort;
    NvxBufferPlatformPrivate *pPrivateData = NULL;

    if (!GrModule)
    {
        hw_module_t const *hwMod;
        hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hwMod);
        GrModule = (NvGrModule *)hwMod;
    }

    if (pData->oPorts[streamIndex].bUsesNBHs)
    {
        buffer_handle_t *hBuffer = (buffer_handle_t *)(buffer->pBuffer);
        if (!(buffer->pBuffer))
            return OMX_ErrorBadParameter;

        handle = (NvNativeHandle *)(*hBuffer);
    }
    else
    {
        anbuffer = (android_native_buffer_t *)(buffer->pBuffer);
        if (!anbuffer)
        {
            return OMX_ErrorBadParameter;
        }

        handle = (NvNativeHandle *)anbuffer->handle;
    }

    width = handle->Surf[0].Width;
    height = handle->Surf[0].Height;

    pPort = &(pData->oPorts[streamIndex]);
    if (pPort->nWidth > width || pPort->nHeight > height)
    {
        NvOsDebugPrintf("ANB's don't match: %d %d %d %d\n", pPort->nWidth, width, pPort->nHeight, height);
        return OMX_ErrorBadParameter;
    }

    if (matchWidth > 0 && matchHeight > 0 &&
        (width != matchWidth || height != matchHeight))
    {
        NvOsDebugPrintf("ANBs don't match 2: %d %d %d 5d\n", matchWidth, width, matchHeight, height);
        return OMX_ErrorBadParameter;
    }

    if (0 != GrModule->Base.lock(&GrModule->Base, (const native_handle_t*)handle,
                                 GRALLOC_USAGE_HW_RENDER, 0, 0,
                                 width, height, NULL))
    {
        return OMX_ErrorBadParameter;
    }

    pPrivateData = (NvxBufferPlatformPrivate *)buffer->pPlatformPrivate;
    if (!pPrivateData->nvmmBuffer)
    {
        NvMMSurfaceFences *fencestruct;

        nvmmbuf = (NvMMBuffer *)NvOsAlloc(sizeof(NvMMBuffer));
        if (!nvmmbuf)
        {
            GrModule->Base.unlock(&GrModule->Base, (const native_handle_t*)handle);
            return OMX_ErrorInsufficientResources;
        }

        fencestruct = (NvMMSurfaceFences *)NvOsAlloc(sizeof(NvMMSurfaceFences));
        if (!fencestruct)
        {
            NvOsFree(nvmmbuf);
            GrModule->Base.unlock(&GrModule->Base, (const native_handle_t*)handle);
            return OMX_ErrorInsufficientResources;
        }

        NvOsMemset(nvmmbuf, 0, sizeof(NvMMBuffer));
        NvOsMemset(fencestruct, 0, sizeof(NvMMSurfaceFences));

        nvmmbuf->StructSize = sizeof(NvMMBuffer);
        nvmmbuf->BufferID = (pData->oPorts[streamIndex].nBufsANB)++;
        nvmmbuf->PayloadType = NvMMPayloadType_SurfaceArray;
        nvmmbuf->Payload.Surfaces.fences = fencestruct;
        NvOsMemset(&nvmmbuf->PayloadInfo, 0, sizeof(nvmmbuf->PayloadInfo));

        pData->oPorts[streamIndex].pBuffers[nvmmbuf->BufferID] = nvmmbuf;
        pPrivateData->nvmmBuffer = nvmmbuf;
        pData->oPorts[streamIndex].nBufsTot++;
    }

    nvmmbuf = (NvMMBuffer *)(pPrivateData->nvmmBuffer);

    // Fetches the SurfaceFlinger fences from the gralloc buffer
    numFences = NVGR_MAX_FENCES;
    GrModule->getfences(&GrModule->Base, (const native_handle_t*)handle, fences,
                        (int*)&numFences);

    pSurfaces = &(nvmmbuf->Payload.Surfaces);
    pSurfaces->fences->numFences = numFences;
    pSurfaces->fences->outFence.SyncPointID = NVRM_INVALID_SYNCPOINT_ID;

    pSurfaces->fences->useOutFence = NV_TRUE;

    if (pData->bWaitOnFence == NV_TRUE) {
         pSurfaces->fences->useOutFence = NV_FALSE;
    }

    NvOsMemcpy(&(pSurfaces->fences->inFence[0]), fences, sizeof(NvRmFence) * numFences);

    GrModule->Base.unlock(&GrModule->Base, (const native_handle_t*)handle);

    if ((pSurfaces->Surfaces[0].hMem != handle->Surf[0].hMem) &&
        (OMX_TRUE == pPrivateData->nvmmBufIsPinned))
    {
        if (pSurfaces->Surfaces[0].hMem != 0)
        {
            NvRmMemUnpin(pSurfaces->Surfaces[0].hMem);
        }
        pPrivateData->nvmmBufIsPinned = OMX_FALSE;
    }

    NvOsMemcpy(pSurfaces->Surfaces, handle->Surf, sizeof(NvRmSurface) * 3);

    if (!pPrivateData->nvmmBufIsPinned)
    {
        pPrivateData->nvmmBufIsPinned = OMX_TRUE;
        pSurfaces->PhysicalAddress[0] = NvRmMemPin(pSurfaces->Surfaces[0].hMem);
        pSurfaces->PhysicalAddress[1] = pSurfaces->PhysicalAddress[0] +
                                        pSurfaces->Surfaces[1].Offset;
        pSurfaces->PhysicalAddress[2] = pSurfaces->PhysicalAddress[0] +
                                        pSurfaces->Surfaces[2].Offset;
    }

    pSurfaces->Empty = NV_TRUE;

    pData->TransferBufferToBlock(pData->hBlock, streamIndex,
                                 NvMMBufferType_Payload, sizeof(NvMMBuffer),
                                 nvmmbuf);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE ExportANBLite(NvxComponent *pNvComp,
                        NvMMBuffer *nvmmbuf,
                        OMX_BUFFERHEADERTYPE **pOutBuf,
                        NvxPort *pPortOut,
                        OMX_BOOL bFreeBacking)
{
    // find the matching OMX buffer
    NvxList *pOutBufList = pPortOut->pEmptyBuffers;
    NvxListItem *cur = NULL;
    OMX_BUFFERHEADERTYPE *omxbuf = NULL;
    NvMMBuffer *buf = NULL;
    android_native_buffer_t *anbuffer;
    NvxBufferPlatformPrivate *pPrivateData = NULL;
    NvNativeHandle *handle = NULL;

    if (!GrModule)
    {
        hw_module_t const *hwMod;
        hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hwMod);
        GrModule = (NvGrModule *)hwMod;
    }

    omxbuf = *pOutBuf;
    pPrivateData = (NvxBufferPlatformPrivate *)omxbuf->pPlatformPrivate;
    buf = (NvMMBuffer *)pPrivateData->nvmmBuffer;

    if (buf->BufferID != nvmmbuf->BufferID)
    {
        NvOsMutexLock(pOutBufList->oLock);
        cur = pOutBufList->pHead;
        while (cur)
        {
            omxbuf = (OMX_BUFFERHEADERTYPE *)cur->pElement;
            pPrivateData = (NvxBufferPlatformPrivate *)omxbuf->pPlatformPrivate;
            buf = (NvMMBuffer *)pPrivateData->nvmmBuffer;
            if (buf && buf->BufferID == nvmmbuf->BufferID)
            {
                break;
            }
            buf = NULL;
            cur = cur->pNext;
        }
        NvOsMutexUnlock(pOutBufList->oLock);

        if (cur)
            NvxListRemoveEntry(pOutBufList, omxbuf);
        else
        {
            NvOsDebugPrintf("couldn't find matching buffer\n");
            return OMX_ErrorBadParameter;
        }
    }

    *pOutBuf = omxbuf;

    if (pPrivateData->eType == NVX_BUFFERTYPE_ANDROID_NATIVE_BUFFER_T)
    {
        anbuffer = (android_native_buffer_t *)(omxbuf->pBuffer);
        if (anbuffer)
        {
            handle = (NvNativeHandle *)(anbuffer->handle);
        }
    }
    else if (pPrivateData->eType == NVX_BUFFERTYPE_ANDROID_BUFFER_HANDLE_T)
    {
        buffer_handle_t *hBuffer = (buffer_handle_t *)(omxbuf->pBuffer);
        handle = (NvNativeHandle *)(*hBuffer);
    }
    else
    {
        NvOsDebugPrintf("Unsupported android buffer type: eType = 0x%x\n",
                        pPrivateData->eType);
        return OMX_ErrorBadParameter;
    }

    if (handle)
    {
        NvU32 StereoInfo = nvmmbuf->PayloadInfo.BufferFlags &
            ( NvMMBufferFlag_StereoEnable |
              NvMMBufferFlag_Stereo_SEI_FPType_Mask |
              NvMMBufferFlag_Stereo_SEI_ContentType_Mask );
        OMX_U32 width, height;

        width = handle->Surf[0].Width;
        height = handle->Surf[0].Height;

        if (nvmmbuf->Payload.Surfaces.fences &&
            0 == GrModule->Base.lock(&GrModule->Base, (const native_handle_t*)handle,
                                     GRALLOC_USAGE_HW_RENDER, 0, 0,
                                     width, height, NULL))
        {
            if (nvmmbuf->Payload.Surfaces.fences->outFence.SyncPointID !=
                NVRM_INVALID_SYNCPOINT_ID)
            {
                GrModule->addfence(&GrModule->Base, (const native_handle_t*)handle,
                                   &(nvmmbuf->Payload.Surfaces.fences->outFence));
            }

            GrModule->Base.unlock(&GrModule->Base, (const native_handle_t*)handle);
        }

        GrModule->set_stereo_info(&GrModule->Base, (const native_handle_t*)handle,
                                  StereoInfo);
        /* WAR bug 827707 */
        GrModule->set_source_crop(&GrModule->Base, (const native_handle_t*)handle,
                                  &nvmmbuf->Payload.Surfaces.CropRect);
    }

    if (bFreeBacking)
    {
        NvOsFree(buf);
        pPrivateData->nvmmBuffer = NULL;
    }

    omxbuf->nFilledLen = sizeof(NvNativeHandle);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE FreeANBLite(NvxComponent *pNvComp,
                      SNvxNvMMLiteTransformData *pData,
                      OMX_U32 streamIndex,
                      OMX_BUFFERHEADERTYPE *buffer)

{
    NvxBufferPlatformPrivate *pPrivateData = NULL;
    NvMMBuffer *nvmmbuf;

    pPrivateData = (NvxBufferPlatformPrivate *)buffer->pPlatformPrivate;
    nvmmbuf = (NvMMBuffer *)pPrivateData->nvmmBuffer;

    if (!nvmmbuf)
        return OMX_ErrorNone;

    // This assumes that we'll be freeing _all_ buffers at once

    pData->oPorts[streamIndex].nBufsANB--;
    pData->oPorts[streamIndex].nBufsTot--;
    pData->oPorts[streamIndex].pBuffers[nvmmbuf->BufferID] = NULL;

    if (pPrivateData->nvmmBufIsPinned)
        NvRmMemUnpin(nvmmbuf->Payload.Surfaces.Surfaces[0].hMem);
    pPrivateData->nvmmBufIsPinned = OMX_FALSE;

    NvOsFree(nvmmbuf);
    pPrivateData->nvmmBuffer = NULL;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE HandleGetANBUsageLite(NvxComponent *pNvComp,
                                void *ANBParam,
                                OMX_U32 SWAccess)
{
    GetAndroidNativeBufferUsageParams *ganbup;
    ganbup = (GetAndroidNativeBufferUsageParams *)ANBParam;

    if(SWAccess)
        ganbup->nUsage = GRALLOC_USAGE_SW_READ_OFTEN;
    else
        ganbup->nUsage = 0;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE CopyNvxSurfaceToANBLite(NvxComponent *pNvComp,
                                  NvMMBuffer *nvmmbuf,
                                  OMX_BUFFERHEADERTYPE *buffer)
{
    NvxSurface *pSrcSurface = &nvmmbuf->Payload.Surfaces;
    android_native_buffer_t *anbuffer;
    NvNativeHandle *handle;
    NvError Err;
    NvDdk2dBlitParameters TexParam;
    NvDdk2dSurface *pSrcDdk2dSurface = NULL;
    NvDdk2dSurface *pDstDdk2dSurface = NULL;
    NvDdk2dFixedRect SrcRectLocal;
    NvRect SrcRect, DstRect;
    NvDdk2dHandle h2d = NvxGet2d();
    NvU32 width, height;
    NvRmFence fences[NVGR_MAX_FENCES];
    NvU32 numFences;

    if (!GrModule)
    {
        hw_module_t const *hwMod;
        hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hwMod);
        GrModule = (NvGrModule *)hwMod;
    }

    if (!h2d || !GrModule)
        return OMX_ErrorBadParameter;

    NvOsMemset(&TexParam, 0, sizeof(NvDdk2dBlitParameters));

    anbuffer = (android_native_buffer_t *)(buffer->pBuffer);
    if (!anbuffer)
        return OMX_ErrorBadParameter;

    handle = (NvNativeHandle *)anbuffer->handle;

    if (handle->Buf)
    {
        NvU32 StereoInfo = nvmmbuf->PayloadInfo.BufferFlags &
            ( NvMMBufferFlag_StereoEnable |
              NvMMBufferFlag_Stereo_SEI_FPType_Mask |
              NvMMBufferFlag_Stereo_SEI_ContentType_Mask );

        GrModule->set_stereo_info(&GrModule->Base, anbuffer->handle,
                                  StereoInfo);
        /* WAR bug 827707 */
        GrModule->set_source_crop(&GrModule->Base, anbuffer->handle,
                                  &nvmmbuf->Payload.Surfaces.CropRect);
    }
    else
    {
        // NULL-GraphicBuffer here?
        return OMX_ErrorBadParameter;
    }

    width = NV_MIN(pSrcSurface->Surfaces[0].Width, handle->Surf[0].Width);
    height = NV_MIN(pSrcSurface->Surfaces[0].Height, handle->Surf[0].Height);

    if (0 != GrModule->Base.lock(&GrModule->Base, anbuffer->handle,
                                 GRALLOC_USAGE_HW_RENDER, 0, 0,
                                 width, height, NULL))
        return OMX_ErrorBadParameter;

    // Fetches the SurfaceFlinger fences from the gralloc buffer
    numFences = NVGR_MAX_FENCES;
    GrModule->getfences(&GrModule->Base, anbuffer->handle, fences,
                        (int*)&numFences);

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
    NvDdk2dSurfaceLock(pDstDdk2dSurface, NvDdk2dSurfaceAccessMode_Read,
                       NULL, NULL, NULL);
    NvDdk2dSurfaceUnlock(pDstDdk2dSurface, fences, numFences);

    NvOsMemset(&SrcRect, 0, sizeof(NvRect));
    NvOsMemset(&DstRect, 0, sizeof(NvRect));

    SrcRect.right = DstRect.right = width;
    SrcRect.bottom = DstRect.bottom = height;

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

/*  This should replace the Nvx2dFlush above, but doesn't appear to work.

    // Fetch the blit fence
    NvDdk2dSurfaceLock(pDstDdk2dSurface, NvDdk2dSurfaceAccessMode_ReadWrite,
                       NULL, fences, &numFences);
    NvDdk2dSurfaceUnlock(pDstDdk2dSurface, NULL, 0);

    // Add the blit fence to the gralloc buffer.  Gralloc accepts only
    // a single write fence, and calling addfence overwrites any
    // previous fences, so we must ensure there is only one here.
    if (numFences)
    {
        NV_ASSERT(numFences == 1);
        GrModule->addfence(&GrModule->Base, anbuffer->handle, &fences[0]);
    }
*/

L_cleanup:
    if (pSrcDdk2dSurface)
        NvDdk2dSurfaceDestroy(pSrcDdk2dSurface);
    if (pDstDdk2dSurface)
        NvDdk2dSurfaceDestroy(pDstDdk2dSurface);

    NvxUnlock2d();

    GrModule->Base.unlock(&GrModule->Base, anbuffer->handle);

    buffer->nFilledLen = sizeof(NvNativeHandle);

    return OMX_ErrorNone;
}


OMX_ERRORTYPE CopyANBToNvxSurfaceLite(NvxComponent *pNvComp,
                                  buffer_handle_t buffer,
                                  NvxSurface *pDestSurface,
                                  OMX_U32 portWidth,
                                  OMX_U32 portHeight)
{
    NvNativeHandle *handle;
    NvError Err;
    NvDdk2dBlitParameters TexParam;
    NvDdk2dSurface *pSrcDdk2dSurface = NULL;
    NvDdk2dSurface *pDstDdk2dSurface = NULL;
    NvDdk2dFixedRect SrcRectLocal;
    NvRect SrcRect, DstRect;
    NvDdk2dHandle h2d;
    NvU32 width, height;
    NvDdk2dSurfaceType SurfaceType;
    NvBool bSkip2DBlit;
    NvU32 numFences;
    NvU32 c;
    NvRmFence fences[NVGR_MAX_FENCES];
    NvRmDeviceHandle hRm = NvxGetRmDevice();

    handle = (NvNativeHandle *)buffer;
    if (!handle)
        return OMX_ErrorBadParameter;

    bSkip2DBlit = (handle->Type == NV_NATIVE_BUFFER_YUV) &&
                  (handle->Surf[0].Width == portWidth) &&
                  (handle->Surf[0].Height == portHeight);

    if (!GrModule)
    {
        hw_module_t const *hwMod;
        hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hwMod);
        GrModule = (NvGrModule *)hwMod;
    }
    if (0 != GrModule->Base.lock(&GrModule->Base, buffer,
                                 GRALLOC_USAGE_HW_TEXTURE, 0, 0,
                                 handle->Surf[0].Width,
                                 handle->Surf[0].Height,
                                 NULL))
    {
        return OMX_ErrorBadParameter;
    }
    // Fetches the SurfaceFlinger fences from the gralloc buffer
    numFences = NVGR_MAX_FENCES;
    GrModule->getfences(&GrModule->Base, buffer, fences, (int*)&numFences);

    if (bSkip2DBlit)
    {
        // Wait on the fences
        for (NvU32 i = 0; i < numFences; i++)
        {
            NvRmFenceWait(hRm, &fences[i], NV_WAIT_INFINITE);
        }
        GrModule->Base.unlock(&GrModule->Base, buffer);

        pDestSurface->SurfaceCount = handle->SurfCount;
        NvOsMemcpy(pDestSurface->Surfaces, handle->Surf,
                    sizeof(NvRmSurface) * NVMMSURFACEDESCRIPTOR_MAX_SURFACES);
        return OMX_ErrorNone;
    }

    h2d = NvxGet2d();
    if (!h2d)
        return OMX_ErrorBadParameter;

    NvOsMemset(&TexParam, 0, sizeof(NvDdk2dBlitParameters));

    NvxLock2d();
    Err = NvDdk2dSurfaceCreate(h2d, NvDdk2dSurfaceType_Y_U_V,
                               &(pDestSurface->Surfaces[0]), &pDstDdk2dSurface);
    if (NvSuccess != Err)
    {
        goto L_cleanup;
    }
    if (handle->SurfCount == 1)
        SurfaceType = NvDdk2dSurfaceType_Single;
    else if (handle->SurfCount == 3)
        SurfaceType = NvDdk2dSurfaceType_Y_U_V;
    else
        goto L_cleanup;
    Err = NvDdk2dSurfaceCreate(h2d, SurfaceType,
                               &(handle->Surf[0]), &pSrcDdk2dSurface);
    if (NvSuccess != Err)
    {
        goto L_cleanup;
    }

    /* transfer gralloc fences into ddk2d:
     * do a CPU wait on any pending nvddk2d fences (there should be
     * none), then unlock with the fences from gralloc.
     *
     * Note that ddk2d access mode is the opposite of the gralloc
     * usage.  We locked the gralloc buffer for read access, so
     * we are giving ddk2d write fences.
     * This step is necessary to avoid 2D reading before any possible
     * HW access (for example 3D) was done rendering.
     */
    NvDdk2dSurfaceLock(pSrcDdk2dSurface,
                       NvDdk2dSurfaceAccessMode_Write,
                       NULL, NULL, NULL);
    NvDdk2dSurfaceUnlock(pSrcDdk2dSurface, fences, numFences);

    NvOsMemset(&SrcRect, 0, sizeof(NvRect));
    NvOsMemset(&DstRect, 0, sizeof(NvRect));

    width = NV_MIN(handle->Surf[0].Width, pDestSurface->Surfaces[0].Width);
    height = NV_MIN(handle->Surf[0].Height, pDestSurface->Surfaces[0].Height);

    SrcRect.right = DstRect.right = width;
    SrcRect.bottom = DstRect.bottom = height;

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

    /* Lock the Src surface to get 2D read fences, meaning we need
     * to ask for AccessMode_Write.
     */
    NvDdk2dSurfaceLock(pSrcDdk2dSurface,
                       NvDdk2dSurfaceAccessMode_Write,
                       NULL, fences, &numFences);
    /* unlock without any fences */
    NvDdk2dSurfaceUnlock(pSrcDdk2dSurface, fences, 0);

    /* transfer 2d fences into gralloc, filtering invalid syncpoint IDs
     * This is to ensure that the ANB does not get overwritten before we
     * are done with reading from it in 2D.
     * This is a necessary step.
     */
    for(c = 0; c < numFences; c++) {
        if (fences[c].SyncPointID != NVRM_INVALID_SYNCPOINT_ID) {
            GrModule->addfence(&GrModule->Base, (buffer_handle_t) buffer, &fences[c]);
        }
    }

    // Now flush the Dest surface, which essentially does a CPU wait.
    // Otherwise the NvxSurface might be read before the blit is completed
    Nvx2dFlush(pDstDdk2dSurface);
L_cleanup:
    if (pSrcDdk2dSurface)
        NvDdk2dSurfaceDestroy(pSrcDdk2dSurface);
    if (pDstDdk2dSurface)
        NvDdk2dSurfaceDestroy(pDstDdk2dSurface);

    NvxUnlock2d();

    GrModule->Base.unlock(&GrModule->Base, buffer);

    return OMX_ErrorNone;
}


OMX_ERRORTYPE HandleStoreMetaDataInBuffersParamLite(void *pParam,
                                                SNvxNvMMLitePort *pPort,
                                                OMX_U32 PortAllowed)
{
    StoreMetaDataInBuffersParams *pStoreMetaData =
        (StoreMetaDataInBuffersParams*)pParam;

    if (!pStoreMetaData || pStoreMetaData->nPortIndex != PortAllowed)
        return OMX_ErrorBadParameter;

    pPort[PortAllowed].bUsesAndroidMetaDataBuffers =
        pStoreMetaData->bStoreMetaData;

    pPort[PortAllowed].bEmbedNvMMBuffer = pStoreMetaData->bStoreMetaData;

    return OMX_ErrorNone;
}
