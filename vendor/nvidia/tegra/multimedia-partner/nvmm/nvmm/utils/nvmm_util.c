/*
 * Copyright (c) 2007 - 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvassert.h"
#include "nvmm_util.h"
#include "nvrm_hardware_access.h"
#include "nvmm_debug.h"
#include "nvmm_service_common.h"
#if !NV_IS_AVP
#include "nvmm_service.h"
#else
#include "nvmm_service_avp.h"
#include "nvrm_avp_swi_registry.h"
#include "nvmm_manager_avp.h"
#endif

#include "nvmm_bufferprofile.h"
#include "nvmm_camera.h"
#include "nvmm_digitalzoom.h"
#include "nvmm_block.h"

#if NV_USE_NVAVP
#include "nvavp.h"
#endif

#if defined(HAVE_X11)
#include <X11/Xlib.h>
#include "tdrlib.h"
#endif

#define MSG_BUFFER_SIZE 256
NvError NvMMUtilDeallocateBufferPrivate(NvMMBuffer *pBuffer, 
                                        void *hService, 
                                        NvBool bAbnormalTermination);

// This asks a possible X server to free up any optional allocations
static void NvMMUtilFreeServerResources(void)
{
#if defined(HAVE_X11)
    Display* dpy;

    dpy = XOpenDisplay(":0");
    if (!dpy)
    {
        return;
    }
    tdrFreeResources(dpy, 0);
    XSync(dpy, 0);
    XCloseDisplay(dpy);
#endif
}

NvError NvMMUtilAllocateBuffer(
    NvRmDeviceHandle hRmDevice,
    NvU32 size,
    NvU32 align,
    NvMMMemoryType memoryType,
    NvBool bInSharedMemory,
    NvMMBuffer **pBuf)
{
    NvRmMemHandle hMemHandle;
    NvMMBuffer *pBuffer = *pBuf;
    NvError status = NvSuccess;
    NvBool retry = NV_TRUE;
    NvOsMemAttribute  coherency;

    NvOsMemset(pBuffer, 0, sizeof(NvMMBuffer));
    pBuffer->StructSize = sizeof(NvMMBuffer);
    pBuffer->Payload.Ref.PhyAddress = NV_RM_INVALID_PHYS_ADDRESS;

    if (bInSharedMemory == NV_TRUE)
    {
        switch (memoryType)
        {
            case NvMMMemoryType_Uncached :
                coherency = NvOsMemAttribute_Uncached;
                break;
            case NvMMMemoryType_WriteBack :
                coherency = NvOsMemAttribute_WriteBack;
                break;
            case NvMMMemoryType_WriteCombined :
                coherency = NvOsMemAttribute_WriteCombined;
                break;
            case NvMMMemoryType_InnerWriteBack :
                coherency = NvOsMemAttribute_InnerWriteBack;
                break;
            default :
                coherency = NvOsMemAttribute_Uncached;
                break;
        }

        status = NvRmMemHandleCreate(hRmDevice, &hMemHandle, size);
        if (status != NvSuccess)
        {
            return status;
        }
        while (NV_TRUE)
        {
            status = NvRmMemAlloc(hMemHandle,
                                NULL,
                                0,
                                align, 
                                coherency);
            if (status != NvSuccess)
            {
                if (retry)
                {
                    NvMMUtilFreeServerResources();
                    retry = NV_FALSE;
                    continue;
                }

                NvRmMemHandleFree(hMemHandle);
                return status;
            }
            else
            {
                break;
            }
        }

        NvRmMemPin(hMemHandle);

        // the payload on the external carved out
        pBuffer->Payload.Ref.MemoryType = memoryType;
        pBuffer->Payload.Ref.sizeOfBufferInBytes = size;
        pBuffer->Payload.Ref.hMem = hMemHandle;
#if NV_IS_AVP
        pBuffer->Payload.Ref.hMemAvp = hMemHandle;
#else
        pBuffer->Payload.Ref.hMemCpu = hMemHandle;
#endif

        pBuffer->Payload.Ref.id = NvRmMemGetId(hMemHandle);

        if (!size)
        {
            pBuffer->PayloadType = NvMMPayloadType_None;
        }
        else
        {
            pBuffer->PayloadType = NvMMPayloadType_MemHandle;
            pBuffer->Payload.Ref.PhyAddress = NvRmMemGetAddress(pBuffer->Payload.Ref.hMem, pBuffer->Payload.Ref.Offset);

#if NV_IS_AVP 
            status = NvRmMemMap(pBuffer->Payload.Ref.hMem, 
                                pBuffer->Payload.Ref.Offset,
                                size,
                                NVOS_MEM_READ_WRITE,
                                &pBuffer->Payload.Ref.pMemAvp);
                                
            pBuffer->Payload.Ref.pMem = pBuffer->Payload.Ref.pMemAvp;
#else
            status = NvRmMemMap(pBuffer->Payload.Ref.hMem, 
                                pBuffer->Payload.Ref.Offset,
                                size,
                                NVOS_MEM_READ_WRITE,
                                &pBuffer->Payload.Ref.pMemCpu);
            if (status != NvSuccess)
            {
                pBuffer->PayloadInfo.BufferFlags |= NvMMBufferFlag_MemMapError;
                status = NvSuccess;
            }
            else
            {
                pBuffer->Payload.Ref.pMem = pBuffer->Payload.Ref.pMemCpu;
            }
#endif
        }

        return status;
    }
    else 
    {
        pBuffer->Payload.Ref.MemoryType = memoryType;
        pBuffer->Payload.Ref.sizeOfBufferInBytes = size;
        
        if (!size)
        {
            pBuffer->PayloadType = NvMMPayloadType_None;
        }
        else
        {
            pBuffer->PayloadType = NvMMPayloadType_MemPointer;
            pBuffer->Payload.Ref.pMem = NvOsAlloc(size); 
            if (!pBuffer->Payload.Ref.pMem)
                return NvError_InsufficientMemory;
        }
    }
    return NvSuccess;
}

NvError NvMMUtilAllocateVideoBuffer(
    NvRmDeviceHandle hRmDevice,
    NvMMVideoFormat VideoFormat,
    NvMMBuffer **pBuf)
{
    NvMMBuffer *pBuffer;
    NvError status = NvSuccess;


    pBuffer = NvOsAlloc(sizeof(NvMMBuffer));
    NvOsMemset(pBuffer, 0, sizeof(NvMMBuffer));
    pBuffer->StructSize = sizeof(NvMMBuffer);
    pBuffer->PayloadType = NvMMPayloadType_SurfaceArray;


    pBuffer->Payload.Surfaces.SurfaceCount = VideoFormat.NumberOfSurfaces;
    NvOsMemcpy(&pBuffer->Payload.Surfaces.Surfaces, VideoFormat.SurfaceDescription,sizeof(NvRmSurface) *
            NVMMSURFACEDESCRIPTOR_MAX_SURFACES);  
    
#if !NV_IS_AVP
    NvMMUtilAllocateSurfaces(hRmDevice, &pBuffer->Payload.Surfaces);     
#endif
   
    *pBuf = pBuffer;
    return status;
}    


NvError 
NvMMUtilDeallocateBuffer(
    NvMMBuffer *pBuffer)
{
    NvError status = NvSuccess;

#if NV_IS_AVP 
    NV_ASSERT(!"No block on AVP should use this API to Deallocate Buffers \n");
#else
    status = NvMMUtilDeallocateBufferPrivate(pBuffer, NULL, NV_FALSE);
#endif

    return status;
}

NvError 
NvMMUtilDeallocateBlockSideBuffer(
    NvMMBuffer *pBuffer, 
    void *hService,
    NvBool bAbnormalTermination)
{
    NvError status = NvSuccess;

#if NV_IS_AVP 
    status = NvMMUtilDeallocateBufferPrivate(pBuffer, hService, bAbnormalTermination);
#else
    status = NvMMUtilDeallocateBufferPrivate(pBuffer, NULL, bAbnormalTermination);
#endif

    return status;
}

NvError 
NvMMUtilDeallocateBufferPrivate(
    NvMMBuffer *pBuffer, 
    void *hService,
    NvBool bAbnormalTermination)
{
    NvError status = NvSuccess;
    NvU32 RcvdMsgSize;
    NvMMServiceMsgInfo_UnmapBuffer UnmapBuffInfo;
    NvMMServiceMsgInfo_UnmapBufferResponse UnmapBufferResponse;

    if (!pBuffer)
        return status;

    if (pBuffer->PayloadType == NvMMPayloadType_MemHandle)
    {
#if NV_IS_AVP
        pBuffer->Payload.Ref.hMem = pBuffer->Payload.Ref.hMemAvp;
        NvRmMemUnmap(pBuffer->Payload.Ref.hMemAvp, pBuffer->Payload.Ref.pMemAvp, pBuffer->Payload.Ref.sizeOfBufferInBytes);
        if(bAbnormalTermination == NV_FALSE)
        {
            if (pBuffer->Payload.Ref.pMemCpu)
            {
                UnmapBuffInfo.MsgType = NVMM_SERVICE_MSG_UnmapBuffer;
                UnmapBuffInfo.hMem = pBuffer->Payload.Ref.hMemCpu;
                UnmapBuffInfo.pVirtAddr = pBuffer->Payload.Ref.pMemCpu;
                UnmapBuffInfo.Length = pBuffer->Payload.Ref.sizeOfBufferInBytes;
                status = NvMMServiceSendMessageBlocking(hService,
                                                        (void *)&UnmapBuffInfo, 
                                                        sizeof(NvMMServiceMsgInfo_UnmapBuffer),
                                                        (void *)&UnmapBufferResponse, 
                                                        sizeof(NvMMServiceMsgInfo_UnmapBufferResponse),
                                                        &RcvdMsgSize);
                NV_ASSERT(status == NvSuccess);
            }
        }
#else
        pBuffer->Payload.Ref.hMem = pBuffer->Payload.Ref.hMemCpu;
        NvRmMemUnmap(pBuffer->Payload.Ref.hMemCpu, pBuffer->Payload.Ref.pMemCpu, pBuffer->Payload.Ref.sizeOfBufferInBytes);
        if (pBuffer->Payload.Ref.pMemAvp)
        {
            UnmapBuffInfo.MsgType = NVMM_SERVICE_MSG_UnmapBuffer;
            UnmapBuffInfo.hMem = pBuffer->Payload.Ref.hMemAvp;
            UnmapBuffInfo.pVirtAddr = pBuffer->Payload.Ref.pMemAvp;
            UnmapBuffInfo.Length = pBuffer->Payload.Ref.sizeOfBufferInBytes;
            status = NvMMServiceSendMessageBlocking((void *)&UnmapBuffInfo, 
                                                    sizeof(NvMMServiceMsgInfo_UnmapBuffer),
                                                    (void *)&UnmapBufferResponse, 
                                                    sizeof(NvMMServiceMsgInfo_UnmapBufferResponse),
                                                    &RcvdMsgSize);
            NV_ASSERT(status == NvSuccess);
        }
#endif
        NvRmMemUnpin(pBuffer->Payload.Ref.hMem);
        NvRmMemHandleFree(pBuffer->Payload.Ref.hMem);
        pBuffer->Payload.Ref.pMem = NULL;
        pBuffer->Payload.Ref.hMemCpu = NULL;
        pBuffer->Payload.Ref.hMemAvp = NULL;
        pBuffer->Payload.Ref.pMemCpu = NULL;
        pBuffer->Payload.Ref.pMemAvp = NULL;
        pBuffer->Payload.Ref.PhyAddress = 0;
    }
    else if (pBuffer->PayloadType == NvMMPayloadType_MemPointer)
    {
        NvOsFree(pBuffer->Payload.Ref.pMem);
        pBuffer->Payload.Ref.pMem = NULL;
    }

    return status;
}

NvError NvMMUtilDeallocateVideoBuffer(NvMMBuffer *pBuffer)
{
    NvError status = NvSuccess;

    if (!pBuffer)
    {
        return NvSuccess;
    }

#if !NV_IS_AVP
    NvMMUtilDestroySurfaces(&pBuffer->Payload.Surfaces);
#endif
    NvOsFree(pBuffer);
    pBuffer = NULL;

    return status;
}


#if !NV_IS_AVP

NvError NvMemAlloc(NvRmDeviceHandle hRmDev,
                          NvRmMemHandle* phMem,
                          NvU32 Size,
                          NvU32 Alignment,
                          NvU32 *PhysicalAddress)
{
    NvError e;
    NvRmMemHandle hMem;
    NvBool retry = NV_TRUE;

    e = NvRmMemHandleCreate(hRmDev, &hMem, Size);
    if (e != NvSuccess)
        goto fail;

retry:
    e = NvRmMemAlloc(hMem, NULL, 0, Alignment, NvOsMemAttribute_Uncached);
    if (e != NvSuccess)
    {
        if (retry)
        {
            NvMMUtilFreeServerResources();
            retry = NV_FALSE;
            goto retry;
        }
        goto fail;
    }

    *phMem = hMem;
    *PhysicalAddress = NvRmMemPin(hMem);
    return NvSuccess;

fail:
    NvRmMemHandleFree(hMem);
    return e;
} // end of NvMemAlloc


NvError
NvMMUtilAllocateSurfaces(
    NvRmDeviceHandle hRmDev,
    NvMMSurfaceDescriptor *pSurfaceDesc)
{
    NvError err = NvSuccess;
    NvS32 ComponentSize, SurfaceAlignment, i;

    for (i=0; i<pSurfaceDesc->SurfaceCount; i++)
    {
        // calculating Y-Pitch based on chroma format instead of calling NvRmSurfaceComputePitch
        SurfaceAlignment = NvRmSurfaceComputeAlignment(hRmDev, &pSurfaceDesc->Surfaces[i]);
        ComponentSize = NvRmSurfaceComputeSize(&pSurfaceDesc->Surfaces[i]);
        if (ComponentSize)
        {
            err = NvMemAlloc(hRmDev, &pSurfaceDesc->Surfaces[i].hMem, ComponentSize, SurfaceAlignment, &pSurfaceDesc->PhysicalAddress[i]);
            if (err != NvSuccess)
            {
                 goto fail;
            }
        }
    }

    return NvSuccess;

fail:   
    for (i=0; i<pSurfaceDesc->SurfaceCount - 1; i++)
    {
        NvMemFree(pSurfaceDesc->Surfaces[i].hMem);
        pSurfaceDesc->Surfaces[i].hMem = NULL;
    }   
    return err;
}


void NvMemFree(NvRmMemHandle hMem)
{
    NvRmMemUnpin(hMem);
    NvRmMemHandleFree(hMem);
} // end of NvMemFree


void NvMMUtilDestroySurfaces(NvMMSurfaceDescriptor *pSurfaceDesc)
{
    NvS32 i, j, surfCnt;
    NvRmMemHandle hMem = NULL;

    surfCnt = pSurfaceDesc->SurfaceCount;
    for (i = 0; i < surfCnt; i++)
    {
        hMem = pSurfaceDesc->Surfaces[i].hMem;
        if (hMem != NULL)
        {
            NvMemFree(hMem);
            pSurfaceDesc->Surfaces[i].hMem = NULL;
            // If our hMem handle is shared by several surfaces,
            // clean it out so we don't attempt to free it again:
            for (j = i+1; j < surfCnt; j++)
            {
                if (pSurfaceDesc->Surfaces[j].hMem == hMem)
                    pSurfaceDesc->Surfaces[j].hMem = NULL;
            }
        }
    }   
}

#endif //!NV_IS_AVP

NvError
NvMMUtilAddBufferProfilingEntry(
    NvMMBufferProfilingData* pProfilingData,
    NvMMBufferProfilingEvent Event,
    NvU32 StreamIndex,
    NvU32 FrameId)
{
    NvU32 i = pProfilingData->NumEntries;

    if (pProfilingData->NumEntries >= MAX_BUFFER_PROFILING_ENTRIES)
    {
        return NvError_InvalidState;
    }

    pProfilingData->Entries[i].Event = Event;
    pProfilingData->Entries[i].FrameId = FrameId;
    pProfilingData->Entries[i].StreamIndex = StreamIndex;
    pProfilingData->Entries[i].TimeStamp = NvOsGetTimeUS() * 10;

    pProfilingData->NumEntries++;

    return NvSuccess;
}

NvError
NvMMUtilDumpBufferProfilingData(
    NvMMBufferProfilingData* pProfilingData,
    NvOsFileHandle hFile)
{
    NvU32 i;

    for (i = 0; i < pProfilingData->NumEntries; i++)
    {
        // StreamIndex
        switch(pProfilingData->BlockType)
        {

            case NvMMBlockType_SrcCamera:
                NvOsFprintf(hFile, "Camera Block, ");
                switch(pProfilingData->Entries[i].StreamIndex)
                {
                    case NvMMCameraStreamIndex_OutputPreview:
                        NvOsFprintf(hFile, "Preview Output, ");
                        break;
                    case NvMMCameraStreamIndex_Output:
                        NvOsFprintf(hFile, "Still/Video Output, ");
                        break;
                    default:
                        NvOsFprintf(hFile, "Unknown Stream, ");
                        break;

                }
                break;
            case NvMMBlockType_DigitalZoom:
                NvOsFprintf(hFile, "DZ Block, ");
                switch(pProfilingData->Entries[i].StreamIndex)
                {
                    case NvMMDigitalZoomStreamIndex_InputPreview:
                        NvOsFprintf(hFile, "Preview Input, ");
                        break;
                    case NvMMDigitalZoomStreamIndex_Input:
                        NvOsFprintf(hFile, "Still/Video Input, ");
                        break;
                    case NvMMDigitalZoomStreamIndex_OutputPreview:
                        NvOsFprintf(hFile, "Preview Output, ");
                        break;
                    case NvMMDigitalZoomStreamIndex_OutputStill:
                        NvOsFprintf(hFile, "Still Output, ");
                        break;
                    case NvMMDigitalZoomStreamIndex_OutputVideo:
                        NvOsFprintf(hFile, "Video Output, ");
                        break;
                    default:
                        NvOsFprintf(hFile, "Unknown Stream, ");
                        break;
                }
                break;
            case NvMMBlockType_EncAAC:
                NvOsFprintf(hFile, "EncAAC, ");
                switch(pProfilingData->Entries[i].StreamIndex)
                {
                    case NvMMStreamDir_In:
                        NvOsFprintf(hFile, "Audio Input, ");
                        break;
                    case NvMMStreamDir_Out:
                        NvOsFprintf(hFile, "Audio Output, ");
                        break;
                    default:
                        NvOsFprintf(hFile, "Unknown Stream, ");
                        break;
                }
                break;


            case NvMMBlockTypeForBufferProfiling_3gpAudio:
                NvOsFprintf(hFile, "3GP Audio, ");
                switch(pProfilingData->Entries[i].StreamIndex)
                {
                    case NvMMStreamDir_In:
                        NvOsFprintf(hFile, "Audio Input, ");
                        break;
                    case NvMMStreamDir_Out:
                        NvOsFprintf(hFile, "Audio Output, ");
                        break;
                    default:
                        NvOsFprintf(hFile, "Unknown Stream, ");
                        break;
                }
                break;

            case NvMMBlockTypeForBufferProfiling_3gpVideo:
                NvOsFprintf(hFile, "3GP Video, ");
                switch(pProfilingData->Entries[i].StreamIndex)
                {
                    case NvMMStreamDir_In:
                        NvOsFprintf(hFile, "Video Input, ");
                        break;
                    case NvMMStreamDir_Out:
                        NvOsFprintf(hFile, "Video Output, ");
                        break;
                    default:
                        NvOsFprintf(hFile, "Unknown Stream, ");
                        break;
                }
                break;
            default:
                NvOsFprintf(hFile, "Unknown Block, Unknown Stream, ");
                        break;
        }

        // Event
        switch(pProfilingData->Entries[i].Event)
        {
            case NvMMBufferProfilingEvent_ReceiveBuffer:
                NvOsFprintf(hFile, "ReceiveBuffer, ");
                break;
            case NvMMBufferProfilingEvent_SendBuffer:
                NvOsFprintf(hFile, "SendBuffer, ");
                break;
            case NvMMBufferProfilingEvent_StartProcessing:
                NvOsFprintf(hFile, "StartProcessing, ");
                break;
            default:
                NvOsFprintf(hFile, "UnknownEvent, ");
        }


        
        // FrameID and TimeStamps (converted to macroseconds)
        NvOsFprintf(hFile, "%d, %u\n", pProfilingData->Entries[i].FrameId,
                    (NvU32)pProfilingData->Entries[i].TimeStamp/10);
    }

    return NvSuccess;

}

#if !NV_IS_AVP

// Concurrancy doesn't matter, this is harmless if done twice..
int NvMMIsUsingNewAVP(void)
{
#if NV_USE_NVAVP
    static int snewavp = -1;
    NvAvpHandle hAVP = NULL;

    if (snewavp != -1)
        return snewavp;

    if (NvAvpOpen(&hAVP) != NvSuccess || hAVP == NULL)
        snewavp = 0;
    else
    {
        snewavp = 1;
        NvAvpClose(hAVP);
    }

    return snewavp;
#else
    return 0;
#endif
}

#endif

