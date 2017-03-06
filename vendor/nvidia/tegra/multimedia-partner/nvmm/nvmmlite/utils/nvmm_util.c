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
#include "nvmmlite_util.h"
#include "nvrm_hardware_access.h"

#if defined(HAVE_X11)
#include <X11/Xlib.h>
#include "tdrlib.h"
#endif

#define MSG_BUFFER_SIZE 256
NvError NvMMLiteUtilDeallocateBufferPrivate(NvMMBuffer *pBuffer);

// This asks a possible X server to free up any optional allocations
static void NvMMLiteUtilFreeServerResources(void)
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

NvError NvMMLiteUtilAllocateBuffer(
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
                    NvMMLiteUtilFreeServerResources();
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
        pBuffer->Payload.Ref.hMemCpu = hMemHandle;

        pBuffer->Payload.Ref.id = NvRmMemGetId(hMemHandle);

        if (!size)
        {
            pBuffer->PayloadType = NvMMPayloadType_None;
        }
        else
        {
            pBuffer->PayloadType = NvMMPayloadType_MemHandle;
            pBuffer->Payload.Ref.PhyAddress = NvRmMemGetAddress(pBuffer->Payload.Ref.hMem, pBuffer->Payload.Ref.Offset);

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

NvError NvMMLiteUtilAllocateVideoBuffer(
    NvRmDeviceHandle hRmDevice,
    NvMMLiteVideoFormat VideoFormat,
    NvMMBuffer **pBuf)
{
    NvMMBuffer *pBuffer;
    NvError status = NvSuccess;
    

    pBuffer = NvOsAlloc(sizeof(NvMMBuffer));
    NvOsMemset(pBuffer, 0, sizeof(NvMMBuffer));
    pBuffer->StructSize = sizeof(NvMMBuffer);
    pBuffer->PayloadType = NvMMPayloadType_SurfaceArray;


    pBuffer->Payload.Surfaces.SurfaceCount = VideoFormat.NumberOfSurfaces;
    NvOsMemcpy(&pBuffer->Payload.Surfaces.Surfaces,
               VideoFormat.SurfaceDescription,
               sizeof(NvRmSurface) * NVMMSURFACEDESCRIPTOR_MAX_SURFACES);  
    
    NvMMLiteUtilAllocateSurfaces(hRmDevice, &pBuffer->Payload.Surfaces);     
   
    *pBuf = pBuffer;
    return status;
}    


NvError 
NvMMLiteUtilDeallocateBuffer(
    NvMMBuffer *pBuffer)
{
    NvError status = NvSuccess;

    if (!pBuffer)
        return status;

    if (pBuffer->PayloadType == NvMMPayloadType_MemHandle)
    {
        pBuffer->Payload.Ref.hMem = pBuffer->Payload.Ref.hMemCpu;
        NvRmMemUnmap(pBuffer->Payload.Ref.hMemCpu, pBuffer->Payload.Ref.pMemCpu, pBuffer->Payload.Ref.sizeOfBufferInBytes);

        NvRmMemUnpin(pBuffer->Payload.Ref.hMem);
        NvRmMemHandleFree(pBuffer->Payload.Ref.hMem);
        pBuffer->Payload.Ref.pMem = NULL;
        pBuffer->Payload.Ref.hMemCpu = NULL;
        pBuffer->Payload.Ref.pMemCpu = NULL;
        pBuffer->Payload.Ref.PhyAddress = 0;
    }
    else if (pBuffer->PayloadType == NvMMPayloadType_MemPointer)
    {
        NvOsFree(pBuffer->Payload.Ref.pMem);
        pBuffer->Payload.Ref.pMem = NULL;
    }

    return status;
}

NvError NvMMLiteUtilDeallocateVideoBuffer(NvMMBuffer *pBuffer)
{
    NvError status = NvSuccess;

    NvMMLiteUtilDestroySurfaces(&pBuffer->Payload.Surfaces);
    NvOsFree(pBuffer);
    pBuffer = NULL;
    
    return status;
}

static NvError NvMemAlloc(NvRmDeviceHandle hRmDev,
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
            NvMMLiteUtilFreeServerResources();
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

static void NvMemFree(NvRmMemHandle hMem)
{
    NvRmMemUnpin(hMem);
    NvRmMemHandleFree(hMem);
} // end of NvMemFree

NvError
NvMMLiteUtilAllocateSurfaces(
    NvRmDeviceHandle hRmDev,
    NvMMSurfaceDescriptor *pSurfaceDesc)
{
    NvError err = NvSuccess;
    NvS32 ComponentSize, SurfaceAlignment, i;

    for (i=0; i<pSurfaceDesc->SurfaceCount; i++)
    {
        // calculating Y-Pitch based on chroma format instead of calling NvRmSurfaceComputePitch
        SurfaceAlignment = NvRmSurfaceComputeAlignment(hRmDev,
                               &pSurfaceDesc->Surfaces[i]);
        ComponentSize = NvRmSurfaceComputeSize(&pSurfaceDesc->Surfaces[i]);
        if (ComponentSize)
        {
            err = NvMemAlloc(hRmDev, &pSurfaceDesc->Surfaces[i].hMem,
                             ComponentSize, SurfaceAlignment,
                             &pSurfaceDesc->PhysicalAddress[i]);
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

void NvMMLiteUtilDestroySurfaces(NvMMSurfaceDescriptor *pSurfaceDesc)
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


