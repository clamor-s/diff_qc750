/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/* 
 * nvmm/nvmm_service.c
 * This file defines the nvmm service that runs on both processors for  
 * seving multiple blocks/clients. 
 */

#include "nvos.h"
#include "nvmm_service.h"
#include "nvmm_service_common.h"
#include "nvrm_moduleloader.h"
#include "nvrm_transport.h"
#include "nvmm_debug.h"
#include "nvassert.h"
#include "nvmm_manager.h"

typedef struct NvMMServiceRec 
{
    NvRmDeviceHandle hRmDevice;
    NvRmTransportHandle hRmTransport;
    NvRmLibraryHandle hRmLoader;
    NvmmManagerHandle hNvmmMgr;
    NvOsSemaphoreHandle NvmmServiceSema;
    NvOsSemaphoreHandle NvmmServiceRcvSema;
    NvOsMutexHandle Mutex;
    NvOsThreadHandle NvmmServiceThreadHandle;
    NvU8 *RcvMsgBuffer;
    NvU32 RcvMsgBufferSize;
    NvU32 RefCount;
    NvBool bShutDown;
    NvU8 PortName[16];
    void *hAVPService;
    NvBool bInitialized;
    NvBool bCloseInProcess;
}NvMMService;

static NvMMService gs_NvmmService;
static NvOsMutexHandle gs_Mutex = 0;

void NvMMServiceThread(void *arg);

NvError NvMMServiceOpen(NvBool bLoadAxf)
{
    NvError e;
    NvMMService *pNvMMService = 0;
    NvOsMutexHandle nvmmMutex = NULL;
    NvU32 TempRefCount;

    NV_DEBUG_PRINTF(("NvMMServiceOpen +\r\n"));

    if(gs_NvmmService.bShutDown == NV_TRUE)
    {
        while(gs_NvmmService.bCloseInProcess == NV_TRUE)
        {
            NV_DEBUG_PRINTF(("NvMMServiceOpen : Delaying the Caller -\r\n"));

            /* Use delay instead of yield incase other thread is lower priority */
            NvOsSleepMS(1);
        }
    }

    if (gs_Mutex == NULL)
    {
        e = NvOsMutexCreate(&nvmmMutex);
        if (e != NvSuccess)
        {
            return e;
        }

        if (NvOsAtomicCompareExchange32((NvS32*)&gs_Mutex, 0, (NvS32)nvmmMutex) != 0)
        {
            NV_DEBUG_PRINTF(("NvMMServiceOpen : Destroying the Mutex -\r\n"));
            NvOsMutexDestroy(nvmmMutex);
        }
    }

    // >> RACE CONDITION WITH CLOSE AND DESTROYING THE MUTEX <<
    NvOsMutexLock(gs_Mutex);

    pNvMMService = &gs_NvmmService;

    if (!pNvMMService->bInitialized)
    {
        if (!bLoadAxf)
        {
            pNvMMService->RefCount++;
            NV_DEBUG_PRINTF(("NvMMServiceOpen : (!bLoadAxf) Ref Count = %d -\r\n", pNvMMService->RefCount));
            NvOsMutexUnlock(gs_Mutex);
            return NvSuccess;
        }
    }
    else
    {
        if (pNvMMService->RefCount)
        {
            pNvMMService->RefCount++;
            NV_DEBUG_PRINTF(("NvMMServiceOpen : Ref Count = %d -\r\n", pNvMMService->RefCount));
            NvOsMutexUnlock(gs_Mutex);
            return NvSuccess;
        }
    }

    TempRefCount = pNvMMService->RefCount;
    NvOsMemset(pNvMMService, 0, sizeof(NvMMService));
    gs_NvmmService.Mutex = gs_Mutex;
    gs_NvmmService.RefCount = TempRefCount;

    pNvMMService->RcvMsgBuffer = NvOsAlloc(MSG_BUFFER_SIZE);
    if (!pNvMMService->RcvMsgBuffer)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    NvOsMemset(pNvMMService->RcvMsgBuffer, 0, MSG_BUFFER_SIZE);

    NV_CHECK_ERROR_CLEANUP(
        NvmmManagerOpen(&pNvMMService->hNvmmMgr)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvOsSemaphoreCreate(&(pNvMMService->NvmmServiceSema), 0)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvOsSemaphoreCreate(&(pNvMMService->NvmmServiceRcvSema), 0)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvRmOpen(&pNvMMService->hRmDevice, 0)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvRmTransportOpen(pNvMMService->hRmDevice, 
                          NULL, 
                          pNvMMService->NvmmServiceSema,
                          &(pNvMMService->hRmTransport))
    );

    NV_CHECK_ERROR_CLEANUP(
        NvRmTransportSetQueueDepth(pNvMMService->hRmTransport,
                                   10,
                                   256)
    );

    NvRmTransportGetPortName(pNvMMService->hRmTransport,
                             pNvMMService->PortName,
                             16);

    if (!NvMMManagerIsUsingNewAVP())
    {
        e = NvRmLoadLibrary(pNvMMService->hRmDevice, 
                            "nvmm_service.axf",
                            pNvMMService->PortName,
                            16,
                            &pNvMMService->hRmLoader);
        if (e != NvSuccess)
        {
            pNvMMService->hRmLoader = NULL;
            goto fail;
        }

        NV_DEBUG_PRINTF(("NvMMServiceOpen : Loading nvmm_service.axf -\r\n"));

        NV_CHECK_ERROR_CLEANUP(
            NvRmTransportConnect(pNvMMService->hRmTransport, 
                                 NV_WAIT_INFINITE)
        );
    }
    else
    {
        pNvMMService->hRmLoader = NULL;
    }

    NV_CHECK_ERROR_CLEANUP(
        NvOsThreadCreate(NvMMServiceThread, 
                         (void *)pNvMMService, 
                         &(pNvMMService->NvmmServiceThreadHandle))
    );

    pNvMMService->RefCount += 1;
    pNvMMService->bInitialized = NV_TRUE;
    pNvMMService->bCloseInProcess = NV_FALSE;
    NV_DEBUG_PRINTF(("NvMMServiceOpen : (bInitialized) Ref Count = %d -\r\n", pNvMMService->RefCount));
    NvOsMutexUnlock(pNvMMService->Mutex);
    return NvSuccess;

fail: 
    NvOsMutexUnlock(pNvMMService->Mutex);
    NvMMServiceClose();
    return e;
}

void NvMMServiceClose( void )
{
    NvMMService *pNvMMService = &gs_NvmmService;

    NV_DEBUG_PRINTF(("NvMMServiceClose +\r\n", pNvMMService->RefCount));

    if(!gs_Mutex)
    {
        NV_DEBUG_PRINTF(("NvMMServiceClose : No Global Mutex : Ref Count = %d -\r\n", pNvMMService->RefCount));
        pNvMMService->bCloseInProcess = NV_FALSE;
        return;
    }
    pNvMMService->bCloseInProcess = NV_TRUE;

    NvOsMutexLock(gs_Mutex);
    pNvMMService->RefCount--;
    if (pNvMMService->RefCount != 0)
    {
        NV_DEBUG_PRINTF(("NvMMServiceClose : Ref Count = %d -\r\n", pNvMMService->RefCount));
        pNvMMService->bCloseInProcess = NV_FALSE;
        NvOsMutexUnlock(gs_Mutex);
        return;
    }

    pNvMMService->bShutDown = NV_TRUE;
    if (pNvMMService->NvmmServiceThreadHandle)
    {
        NvOsSemaphoreSignal(pNvMMService->NvmmServiceSema);
        NvOsThreadJoin(pNvMMService->NvmmServiceThreadHandle);
    }

    // Close the transport before unloading the library.
    if (pNvMMService->hRmTransport)
    {
        NvRmTransportClose(pNvMMService->hRmTransport);
    }

    if (pNvMMService->hRmLoader)
    {
        NV_DEBUG_PRINTF(("NvMMServiceClose : Unloading nvmm_service.axf -\r\n"));
        NvRmFreeLibrary(pNvMMService->hRmLoader);
    }

    if (pNvMMService->hNvmmMgr)
    {
        NvmmManagerClose(pNvMMService->hNvmmMgr, NV_FALSE);
    }

    if (pNvMMService->hRmDevice)
    {
        NvRmClose(pNvMMService->hRmDevice);
    }

    if (pNvMMService->NvmmServiceRcvSema)
    {
        NvOsSemaphoreDestroy(pNvMMService->NvmmServiceRcvSema);
    }

    if (pNvMMService->NvmmServiceSema)
    {
        NvOsSemaphoreDestroy(pNvMMService->NvmmServiceSema);
    }

    if (pNvMMService->RcvMsgBuffer)
    {
        NvOsFree(pNvMMService->RcvMsgBuffer);
    }

    {
        NvOsMutexHandle mutex = gs_Mutex;
        gs_Mutex = 0;
        NvOsMutexUnlock(mutex);
        NvOsMutexDestroy(mutex);

        /* This takes care of bCloseInProcess = NV_FALSE flag */
        NvOsMemset(pNvMMService, 0, sizeof(NvMMService));
    }

    NV_DEBUG_PRINTF(("NvMMServiceClose -\r\n"));

    return;
}

NvError NvMMServiceSendMessage( 
   void* pMessageBuffer,
   NvU32 MessageSize )
{
    NvError Status = NvSuccess;
    NvMMService *pNvMMService = &gs_NvmmService;

    Status = NvRmTransportSendMsg(pNvMMService->hRmTransport, 
                                  pMessageBuffer, 
                                  MessageSize, 
                                  NV_WAIT_INFINITE);
    return Status;
}

NvError NvMMServiceSendMessageBlocking( 
    void* pMessageBuffer,
    NvU32 MessageSize,
    void* pRcvMessageBuffer,
    NvU32 MaxRcvSize,
    NvU32 * pRcvMessageSize )
{
    NvError Status = NvSuccess;
    NvMMService *pNvMMService = &gs_NvmmService;
    NvOsMutexLock( pNvMMService->Mutex );
    Status = NvRmTransportSendMsg(pNvMMService->hRmTransport, 
                                  pMessageBuffer, 
                                  MessageSize, 
                                  NV_WAIT_INFINITE);
    if (Status == NvSuccess)
    {
        /* wait until the other side is done */
        NvOsSemaphoreWait(pNvMMService->NvmmServiceRcvSema);
        *pRcvMessageSize =  min(MaxRcvSize, pNvMMService->RcvMsgBufferSize);
        NvOsMemcpy(pRcvMessageBuffer, pNvMMService->RcvMsgBuffer, *pRcvMessageSize);
    }
    NvOsMutexUnlock( pNvMMService->Mutex );
    return Status;
}

NvError GetAVPServiceHandle(void **phAVPService) 
{
    NvError Status = NvSuccess;
    NvU32 RcvdMsgSize;
    NvMMServiceMsgInfo_GetInfo AVPServiceInfo;
    NvMMServiceMsgInfo_GetInfoResponse AVPServiceInfoResponse;
    NvMMService *pNvMMService = &gs_NvmmService;

    if (!pNvMMService->hAVPService)
    {
        AVPServiceInfo.MsgType = NVMM_SERVICE_MSG_GetInfo;
        Status = NvMMServiceSendMessageBlocking((void *)&AVPServiceInfo, 
                                                sizeof(NvMMServiceMsgInfo_GetInfo),
                                                (void *)&AVPServiceInfoResponse, 
                                                sizeof(NvMMServiceMsgInfo_GetInfoResponse),
                                                &RcvdMsgSize);
        if (Status == NvSuccess && 
            RcvdMsgSize > 0)
        {
            pNvMMService->hAVPService = AVPServiceInfoResponse.hAVPService;
        }
    }
    *phAVPService = pNvMMService->hAVPService;
    NV_DEBUG_PRINTF(("GetAVPServiceHandle:: %x\n", *phAVPService));
    return Status;
}

NvError NvMMServiceGetLoaderHandle(void **phServiceLoader) 
{
    NvError Status = NvSuccess;
    NvMMService *pNvMMService = &gs_NvmmService;

    *phServiceLoader = (void*)pNvMMService->hRmLoader;

    return Status;
}

void NvMMServiceThread(void *arg)
{
    NvError e;
    NvU32 Message;
    NvMMService *pNvMMService = (NvMMService *)arg;

    while (!pNvMMService->bShutDown)
    {
        NvOsSemaphoreWait(pNvMMService->NvmmServiceSema);
        e = NvRmTransportRecvMsg(pNvMMService->hRmTransport, 
                                 pNvMMService->RcvMsgBuffer, 
                                 MSG_BUFFER_SIZE, 
                                 &pNvMMService->RcvMsgBufferSize);

        if( (NvSuccess == e) && (pNvMMService->RcvMsgBufferSize != 0))
        {
            Message = *((NvU32 *)&pNvMMService->RcvMsgBuffer[0]);
            switch(Message) 
            {
                case NVMM_SERVICE_MSG_UnmapBuffer:
                {
                    NvMMServiceMsgInfo_UnmapBuffer *pUnmapBufferInfo;
                    NvMMServiceMsgInfo_UnmapBufferResponse Response;

                    pUnmapBufferInfo = (NvMMServiceMsgInfo_UnmapBuffer *)&pNvMMService->RcvMsgBuffer[0];
                    NV_DEBUG_PRINTF((" NVMM_SERVICE_MSG_UnmapBuffer %x\t%x\t%x\n", 
                                     pUnmapBufferInfo->hMem, 
                                     pUnmapBufferInfo->pVirtAddr, 
                                     pUnmapBufferInfo->Length));
                    NvRmMemUnmap(pUnmapBufferInfo->hMem, 
                                 pUnmapBufferInfo->pVirtAddr, 
                                 pUnmapBufferInfo->Length);
                    NvRmMemHandleFree(pUnmapBufferInfo->hMem);

                    //Send the ack 
                    Response.MsgType = NVMM_SERVICE_MSG_UnmapBuffer_Response;
                    NvMMServiceSendMessage(
                        (void *)&Response, 
                        sizeof(NvMMServiceMsgInfo_UnmapBufferResponse));
                }
                break;
                case NVMM_SERVICE_MSG_UnmapBuffer_Response:
                {
                    NvOsSemaphoreSignal(pNvMMService->NvmmServiceRcvSema);
                }
                break;
                case NVMM_SERVICE_MSG_AllocScratchIRAM:
                {
                    NvMMServiceMsgInfo_AllocScratchIRAM *pScratchIRAMInfo;
                    NvMMServiceMsgInfo_AllocScratchIRAMResponse Response;

                    NvOsMutexLock( pNvMMService->Mutex );
                    pScratchIRAMInfo = (NvMMServiceMsgInfo_AllocScratchIRAM *)&pNvMMService->RcvMsgBuffer[0];

                    NvmmManagerIRAMScratchAlloc( 
                        pNvMMService->hNvmmMgr, 
                        (void *)&Response, 
                        (NvU32)pScratchIRAMInfo->IRAMScratchType,
                        pScratchIRAMInfo->Size);

                    //NvOsDebugPrintf("code Exits here 0x%x 0x%x 0x%x\n", Response.pPhyAddr, Response.hMem, Response.Length);

                    //Send the ack 
                    Response.MsgType = NVMM_SERVICE_MSG_AllocScratchIRAM_Response;
                    NvMMServiceSendMessage( (void *)&Response, sizeof(NvMMServiceMsgInfo_AllocScratchIRAMResponse));

                    NvOsMutexUnlock( pNvMMService->Mutex );

                }
                break;
                case NVMM_SERVICE_MSG_FreeScratchIRAM:
                {
                    NvMMServiceMsgInfo_FreeScratchIRAM *pScratchIRAMInfo;
                    NvMMServiceMsgInfo_FreeScratchIRAMResponse Response;

                    NvOsMutexLock( pNvMMService->Mutex );
                    pScratchIRAMInfo = (NvMMServiceMsgInfo_FreeScratchIRAM *)&pNvMMService->RcvMsgBuffer[0];

                    NvmmManagerIRAMScratchFree( 
                        pNvMMService->hNvmmMgr, 
                        (NvU32)pScratchIRAMInfo->IRAMScratchType);

                    //Send the ack 
                    Response.MsgType = NVMM_SERVICE_MSG_FreeScratchIRAM_Response;
                    NvMMServiceSendMessage(
                        (void *)&Response, 
                        sizeof(NvMMServiceMsgInfo_FreeScratchIRAMResponse));

                    NvOsMutexUnlock( pNvMMService->Mutex );
                }
                break;
                case NVMM_SERVICE_MSG_GetInfo_Response:
                {
                    NvOsSemaphoreSignal(pNvMMService->NvmmServiceRcvSema);
                }
                break;

                case NvMMServiceMsgType_NvmmTransCommInfo:
                {
                    NvMMServiceMsgInfo_NvmmTransCommInfo *pNvmmTransCommInfo;
                    pNvmmTransCommInfo = (NvMMServiceMsgInfo_NvmmTransCommInfo *)&pNvMMService->RcvMsgBuffer[0];
                    if(pNvmmTransCommInfo->pCallBack)
                    {
                        pNvmmTransCommInfo->pCallBack(pNvmmTransCommInfo->pContext,
                                                      pNvmmTransCommInfo->pEventData, 
                                                      EVENT_BUFFER_SIZE);
                    }
                }
                break;

                default: 
                    break;
            }

        }
    }
}
