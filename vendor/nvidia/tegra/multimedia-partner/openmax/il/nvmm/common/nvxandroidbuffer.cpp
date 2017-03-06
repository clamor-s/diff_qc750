/* Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
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
#include "nvmmtransformbase.h"
#include <nvgralloc.h>
#include <nvassert.h>
}

#include "nvxandroidbuffer.h"

#include <media/hardware/HardwareAPI.h>
#include <hardware/hardware.h>

using namespace android;

static NvGrModule const *GrModule = NULL;

OMX_ERRORTYPE HandleEnableANB(NvxComponent *pNvComp, OMX_U32 portallowed,
                              void *ANBParam)
{
    EnableAndroidNativeBuffersParams *eanbp;
    eanbp = (EnableAndroidNativeBuffersParams *)ANBParam;

    if (eanbp->nPortIndex != portallowed)
    {
        return OMX_ErrorBadParameter;
    }

    NvxPort *pPort = &(pNvComp->pPorts[portallowed]);

    if (eanbp->enable)
    {
        // TODO: Disable usebuffer/etc, store old format, mark as allowed
        pPort->oPortDef.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)
                                                    (HAL_PIXEL_FORMAT_YV12);
    }
    else
    {
        pPort->oPortDef.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE HandleUseANB(NvxComponent *pNvComp, void *ANBParam)
{
    UseAndroidNativeBufferParams *uanbp;
    uanbp = (UseAndroidNativeBufferParams *)ANBParam;

    return NvxUseBufferInt(pNvComp->hBaseComponent,
                           uanbp->bufferHeader, uanbp->nPortIndex,
                           uanbp->pAppPrivate, 0,
                           (OMX_U8 *)uanbp->nativeBuffer.get(),
                           NVX_BUFFERTYPE_ANDROID_NATIVE_BUFFER_T);
}

OMX_ERRORTYPE HandleUseNBH(NvxComponent *pNvComp, void *NBHParam)
{
    NVX_PARAM_USENATIVEBUFFERHANDLE *unbhp;
    unbhp = (NVX_PARAM_USENATIVEBUFFERHANDLE *)NBHParam;

    return NvxUseBufferInt(pNvComp->hBaseComponent,
                           unbhp->bufferHeader, unbhp->nPortIndex,
                           NULL, 0,
                           (OMX_U8 *)unbhp->oNativeBufferHandlePtr,
                           NVX_BUFFERTYPE_ANDROID_BUFFER_HANDLE_T);
}

OMX_ERRORTYPE ImportAllANBs(NvxComponent *pNvComp,
                            SNvxNvMMTransformData *pData,
                            OMX_U32 streamIndex,
                            OMX_U32 matchWidth,
                            OMX_U32 matchHeight)
{
    SNvxNvMMPort *pPort = &(pData->oPorts[streamIndex]);
    NvxPort *pPortOut = pPort->pOMXPort;
    OMX_BUFFERHEADERTYPE *pBuffer;
    NvxList *pOutBufList = pPortOut->pEmptyBuffers;
    NvxListItem *cur = NULL;

    NvxMutexLock(pPortOut->hMutex);

    pBuffer = pPortOut->pCurrentBufferHdr;
    if (pBuffer)
    {
        ImportANB(pNvComp, pData, streamIndex, pBuffer,
                  matchWidth, matchHeight);
    }

    NvOsMutexLock(pOutBufList->oLock);
    cur = pOutBufList->pHead;
    while (cur)
    {
        pBuffer = (OMX_BUFFERHEADERTYPE *)cur->pElement;
        ImportANB(pNvComp, pData, streamIndex, pBuffer,
                  matchWidth, matchHeight);
        cur = cur->pNext;
    }
    NvOsMutexUnlock(pOutBufList->oLock);

    NvxMutexUnlock(pPortOut->hMutex);

    NvOsDebugPrintf("imported: %d buffers\n", pPort->nBufsTot);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE ImportANB(NvxComponent *pNvComp,
                        SNvxNvMMTransformData *pData,
                        OMX_U32 streamIndex,
                        OMX_BUFFERHEADERTYPE *buffer,
                        OMX_U32 matchWidth,
                        OMX_U32 matchHeight)
{
    android_native_buffer_t *anbuffer;
    NvMMBuffer *nvmmbuf = NULL;
    NvNativeHandle *handle;
    OMX_U32 width, height;
    NvRmDeviceHandle hRm = NvxGetRmDevice();
    NvRmFence fences[NVGR_MAX_FENCES];
    NvU32 i, numFences;
    NvMMSurfaceDescriptor *pSurfaces;
    SNvxNvMMPort *pPort;
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
        nvmmbuf = (NvMMBuffer *)NvOsAlloc(sizeof(NvMMBuffer));
        if (!nvmmbuf)
        {
            GrModule->Base.unlock(&GrModule->Base, (const native_handle_t*)handle);
            return OMX_ErrorInsufficientResources;
        }

        NvOsMemset(nvmmbuf, 0, sizeof(NvMMBuffer));

        nvmmbuf->StructSize = sizeof(NvMMBuffer);
        nvmmbuf->BufferID = (pData->oPorts[streamIndex].nBufsANB)++;
        nvmmbuf->PayloadType = NvMMPayloadType_SurfaceArray;
        NvOsMemset(&nvmmbuf->PayloadInfo, 0, sizeof(nvmmbuf->PayloadInfo));

        if (pData->InterlaceStream != OMX_TRUE)
        {
            pData->oPorts[streamIndex].pBuffers[nvmmbuf->BufferID] = nvmmbuf;
            pData->oPorts[streamIndex].nBufsTot++;
        }
        pPrivateData->nvmmBuffer = nvmmbuf;
    }

    nvmmbuf = (NvMMBuffer *)(pPrivateData->nvmmBuffer);

    // Fetches the SurfaceFlinger fences from the gralloc buffer
    numFences = NVGR_MAX_FENCES;
    GrModule->getfences(&GrModule->Base, (const native_handle_t*)handle, fences,
                        (int*)&numFences);

    // Wait on the fences
    for (i = 0; i < numFences; i++)
    {
        NvRmFenceWait(hRm, &fences[i], NV_WAIT_INFINITE);
    }

    GrModule->Base.unlock(&GrModule->Base, (const native_handle_t*)handle);

    pSurfaces = &(nvmmbuf->Payload.Surfaces);

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
    if (OMX_FALSE == pPrivateData->nvmmBufIsPinned)
    {
        pPrivateData->nvmmBufIsPinned = OMX_TRUE;
        pSurfaces->PhysicalAddress[0] = NvRmMemPin(pSurfaces->Surfaces[0].hMem);
        pSurfaces->PhysicalAddress[1] = pSurfaces->PhysicalAddress[0] +
                                        pSurfaces->Surfaces[1].Offset;
        pSurfaces->PhysicalAddress[2] = pSurfaces->PhysicalAddress[0] +
                                        pSurfaces->Surfaces[2].Offset;
    }
    pSurfaces->SurfaceCount = handle->SurfCount;
    pSurfaces->Empty = NV_TRUE;

    if (pData->InterlaceStream == OMX_TRUE)
    {
        pData->vmr.Renderstatus[nvmmbuf->BufferID] = FRAME_VMR_FREE;
        pData->vmr.pVideoSurf[nvmmbuf->BufferID] = nvmmbuf;
        NvOsSemaphoreSignal(pData->InpOutAvail);
    }
    else
    {
        pData->TransferBufferToBlock(pData->hBlock, streamIndex,
                                     NvMMBufferType_Payload, sizeof(NvMMBuffer),
                                     nvmmbuf);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE ExportANB(NvxComponent *pNvComp,
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

    if (buf->PayloadInfo.BufferMetaDataType ==
            NvMMBufferMetadataType_DigitalZoom)
    {
        if (buf->PayloadInfo.BufferMetaData.DigitalZoomBufferMetadata.KeepFrame)
        {
            omxbuf->nFlags |= OMX_BUFFERFLAG_POSTVIEW;
        }
        else
        {
            omxbuf->nFlags &= !OMX_BUFFERFLAG_POSTVIEW;
        }
    }

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

        GrModule->set_stereo_info(&GrModule->Base, (const native_handle_t*)handle,
                                  StereoInfo);
    }
/*
    anbuffer = (android_native_buffer_t *)(omxbuf->pBuffer);
    if (!anbuffer)
        return OMX_ErrorBadParameter;

    GrModule->Base.unlock(&GrModule->Base, anbuffer->handle);

    if (pPrivateData->nvmmBufIsPinned)
        NvRmMemUnpin(buf->Payload.Surfaces.Surfaces[0].hMem);
    pPrivateData->nvmmBufIsPinned = OMX_FALSE;
*/

    if (bFreeBacking)
    {
        NvOsFree(buf);
        pPrivateData->nvmmBuffer = NULL;
    }

    omxbuf->nFilledLen = sizeof(NvNativeHandle);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE FreeANB(NvxComponent *pNvComp,
                      SNvxNvMMTransformData *pData,
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
    if(pData->InterlaceStream != OMX_TRUE)
    {
        pData->oPorts[streamIndex].nBufsTot--;
        pData->oPorts[streamIndex].pBuffers[nvmmbuf->BufferID] = NULL;
    }
    if (pPrivateData->nvmmBufIsPinned)
        NvRmMemUnpin(nvmmbuf->Payload.Surfaces.Surfaces[0].hMem);
    pPrivateData->nvmmBufIsPinned = OMX_FALSE;

    NvOsFree(nvmmbuf);
    pPrivateData->nvmmBuffer = NULL;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE HandleGetANBUsage(NvxComponent *pNvComp,
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

OMX_ERRORTYPE CopyNvxSurfaceToANB(NvxComponent *pNvComp,
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


OMX_ERRORTYPE CopyANBToNvxSurface(NvxComponent *pNvComp,
                                  buffer_handle_t buffer,
                                  NvxSurface *pDestSurface)
{
    NvNativeHandle *handle;
    NvError Err;
    NvDdk2dBlitParameters TexParam;
    NvDdk2dSurface *pSrcDdk2dSurface = NULL;
    NvDdk2dSurface *pDstDdk2dSurface = NULL;
    NvDdk2dFixedRect SrcRectLocal;
    NvRect SrcRect, DstRect;
    NvDdk2dHandle h2d = NvxGet2d();
    NvU32 width, height;
    NvDdk2dSurfaceType SurfaceType;
    NvRmFence fences[NVGR_MAX_FENCES];
    NvU32 numFences;

    if (!GrModule)
    {
        hw_module_t const *hwMod;
        hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hwMod);
        GrModule = (NvGrModule *)hwMod;
    }

    if (!h2d)
        return OMX_ErrorBadParameter;

    NvOsMemset(&TexParam, 0, sizeof(NvDdk2dBlitParameters));

    handle = (NvNativeHandle *)buffer;
    if (!handle)
       return OMX_ErrorBadParameter;

    width = NV_MIN(handle->Surf[0].Width, pDestSurface->Surfaces[0].Width);
    height = NV_MIN(handle->Surf[0].Height, pDestSurface->Surfaces[0].Height);

    if (0 != GrModule->Base.lock(&GrModule->Base, buffer,
                                 GRALLOC_USAGE_HW_TEXTURE, 0, 0,
                                 width, height, NULL))
    {
        return OMX_ErrorBadParameter;
    }

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

    // Fetches the SurfaceFlinger fences from the gralloc buffer
    numFences = NVGR_MAX_FENCES;
    GrModule->getfences(&GrModule->Base, buffer, fences,
                        (int*)&numFences);

    // Add the fences to the source surface
    NvDdk2dSurfaceLock(pSrcDdk2dSurface, NvDdk2dSurfaceAccessMode_Write,
                       NULL, NULL, NULL);
    NvDdk2dSurfaceUnlock(pSrcDdk2dSurface, fences, numFences);

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

    // Wait for the blit to complete.
    //
    // Note that unlocking with no fences forces a CPU wait for the
    // operation to complete. Ideally we'd pass the fences back into
    // the ANB, and on to the encoder.
    NvDdk2dSurfaceLock(pSrcDdk2dSurface, NvDdk2dSurfaceAccessMode_Write,
                       NULL, 0, NULL);
    NvDdk2dSurfaceUnlock(pSrcDdk2dSurface, NULL, 0);

L_cleanup:
    GrModule->Base.unlock(&GrModule->Base, buffer);

    if (pSrcDdk2dSurface)
        NvDdk2dSurfaceDestroy(pSrcDdk2dSurface);
    if (pDstDdk2dSurface)
        NvDdk2dSurfaceDestroy(pDstDdk2dSurface);

    NvxUnlock2d();

    return OMX_ErrorNone;
}


OMX_ERRORTYPE HandleStoreMetaDataInBuffersParam(void *pParam,
                                                SNvxNvMMPort *pPort,
                                                OMX_U32 PortAllowed)
{
    StoreMetaDataInBuffersParams *pStoreMetaData =
        (StoreMetaDataInBuffersParams*)pParam;

    if (!pStoreMetaData || pStoreMetaData->nPortIndex != PortAllowed)
        return OMX_ErrorBadParameter;

    pPort[PortAllowed].bUsesAndroidMetaDataBuffers =
        pStoreMetaData->bStoreMetaData;


    return OMX_ErrorNone;
}
