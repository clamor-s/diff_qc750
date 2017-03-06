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

#include "nvmm_service_avp.h"

void NvMMServiceThread(void *arg);
NvError NvMMCommonSwiHandler(int *pRegs);

NvError NvMMServiceOpen(NvMMServiceHandle *pHandle, NvU8 *PortName) 
{
    NvError e;
    NvMMService *pNvmmService;

    NV_DEBUG_PRINTF(("In NvMMServiceOpen \n"));

    pNvmmService = NvOsAlloc(sizeof(NvMMService));
    if (!pNvmmService)
        return NvError_InsufficientMemory;

    NvOsMemset(pNvmmService, 0, sizeof(NvMMService));

    NvOsMemcpy(pNvmmService->PortName, PortName, 16);

    pNvmmService->RcvMsgBuffer = NvOsAlloc(MSG_BUFFER_SIZE);
    if (!pNvmmService->RcvMsgBuffer)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    NvOsMemset(pNvmmService->RcvMsgBuffer, 0, MSG_BUFFER_SIZE);

    NV_CHECK_ERROR_CLEANUP(
        NvRmOpen(&pNvmmService->hRmDevice, 0)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvRmAvpClientSwiHandlerRegister(AvpSwiClientType_NvMM, 
                                        NvMMCommonSwiHandler,
                                        &pNvmmService->ClientSwiIndex)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvOsMutexCreate(&pNvmmService->Mutex)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvOsSemaphoreCreate(&(pNvmmService->NvmmServiceSema), 0)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvOsSemaphoreCreate(&(pNvmmService->NvmmServiceRcvSema), 0)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvOsSemaphoreCreate(&(pNvmmService->SyncSema), 0)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvOsThreadCreate(NvMMServiceThread, 
                         (void *)pNvmmService, 
                         &(pNvmmService->NvmmServiceThreadHandle))
    );
    NvOsSemaphoreWait(pNvmmService->SyncSema);

    *pHandle = pNvmmService;
    NV_DEBUG_PRINTF(("Out NvMMServiceOpen %x\n", pNvmmService));
    return NvSuccess;

fail: 
    NvMMServiceClose(pNvmmService);
    return e;
}

void NvMMServiceClose(NvMMServiceHandle hNvMMService)
{
    NV_DEBUG_PRINTF(("Closing NvMMServiceClose::%x....\n", hNvMMService));

    if (!hNvMMService)
    {
        return;
    }

    if (hNvMMService->NvmmServiceThreadHandle)
    {
        hNvMMService->bShutDown = NV_TRUE;
        NvOsSemaphoreSignal(hNvMMService->NvmmServiceSema);
        NvOsThreadJoin(hNvMMService->NvmmServiceThreadHandle);
    }

    if (hNvMMService->SyncSema)
    {
        NvOsSemaphoreDestroy(hNvMMService->SyncSema);
    }

    if (hNvMMService->NvmmServiceRcvSema)
    {
        NvOsSemaphoreDestroy(hNvMMService->NvmmServiceRcvSema);
    }

    if (hNvMMService->NvmmServiceSema)
    {
        NvOsSemaphoreDestroy(hNvMMService->NvmmServiceSema);
    }

    if (hNvMMService->Mutex)
    {
        NvOsMutexDestroy(hNvMMService->Mutex);
    }

    NvRmAvpClientSwiHandlerUnRegister(AvpSwiClientType_NvMM, hNvMMService->ClientSwiIndex);

    if (hNvMMService->hRmDevice)
    {
        NvRmClose(hNvMMService->hRmDevice);
    }

    if (hNvMMService->RcvMsgBuffer)
    {
        NvOsFree(hNvMMService->RcvMsgBuffer);
    }

    NvOsFree(hNvMMService);

    NV_DEBUG_PRINTF(("...done\n"));
}

NvError NvMMServiceSendMessage( 
    NvMMServiceHandle hNvMMService,
    void* pMessageBuffer,
    NvU32 MessageSize )
{
    NvError Status = NvSuccess;
    Status = NvRmTransportSendMsg(hNvMMService->hRmTransport, 
                                  pMessageBuffer, 
                                  MessageSize, 
                                  NV_WAIT_INFINITE);
    return Status;
}

NvError NvMMServiceSendMessageBlocking( 
    NvMMServiceHandle hNvMMService,
    void* pMessageBuffer,
    NvU32 MessageSize,
    void* pRcvMessageBuffer,
    NvU32 MaxRcvSize,
    NvU32 * pRcvMessageSize )
{
    NvError Status = NvSuccess;
    NvOsMutexLock( hNvMMService->Mutex );
    Status = NvRmTransportSendMsg(hNvMMService->hRmTransport, 
                                  pMessageBuffer, 
                                  MessageSize, 
                                  NV_WAIT_INFINITE);
    if (Status == NvSuccess)
    {
        /* wait until the other side is done */
        NvOsSemaphoreWait(hNvMMService->NvmmServiceRcvSema);
        *pRcvMessageSize =  min(MaxRcvSize, hNvMMService->RcvMsgBufferSize);
        NvOsMemcpy(pRcvMessageBuffer, hNvMMService->RcvMsgBuffer, *pRcvMessageSize);
    }

    NvOsMutexUnlock( hNvMMService->Mutex );
    return Status;
}

NvError AxfModuleMain(NvRmModuleLoaderReason Reason, void *Args, NvU32 ArgsSize)
{
    NvError Status = NvSuccess;
    static NvMMServiceHandle s_hNvMMService = NULL;
    switch(Reason) 
    {
    case NvRmModuleLoaderReason_Attach:
        NvMMServiceOpen(&s_hNvMMService, Args);
    break;
    case NvRmModuleLoaderReason_Detach:
        NvMMServiceClose(s_hNvMMService);
        s_hNvMMService = NULL;
    break;
    }
    return Status;
}

NvError NvMMCommonSwiHandler(int *pRegs)
{
    NvError Status = NvSuccess;

    NvU32 size = 0;
    NvMMServiceApiType ApiType = *((NvMMServiceApiType *)&pRegs[size]);
    switch(ApiType)
    {
        case NvMMServiceApiType_SendMessageBlocking:
        {
            NvMMServiceApiInfo_SendMessageBlocking *pApiInfo = 
                (NvMMServiceApiInfo_SendMessageBlocking *)&pRegs[0];

            Status =  NvMMServiceSendMessageBlocking(pApiInfo->hNvMMService,
                                                     pApiInfo->pMessageBuffer,
                                                     pApiInfo->MessageSize,
                                                     pApiInfo->pRcvMessageBuffer,
                                                     pApiInfo->MaxRcvSize,
                                                     pApiInfo->pRcvMessageSize);
        }
            break;
        case NvMMServiceApiType_SendMessage:
        {
            NvMMServiceApiInfo_SendMessage *pApiInfo = 
                (NvMMServiceApiInfo_SendMessage *)&pRegs[0];
    
            Status =  NvMMServiceSendMessage(pApiInfo->hNvMMService,
                                             pApiInfo->pMessageBuffer,
                                             pApiInfo->MessageSize);
        }
            break;
        default:
            break;
    }
    return Status;
}

void NvMMServiceThread(void *arg)
{
    NvError e;
    NvU32 Message;
    NvMMService *pNvMMService = (NvMMService *)arg;
    NvU32 SignalSent = 0;

    NV_CHECK_ERROR_CLEANUP(
        NvRmTransportOpen(pNvMMService->hRmDevice, 
                          (char *)pNvMMService->PortName, 
                          pNvMMService->NvmmServiceSema,
                          &pNvMMService->hRmTransport)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvRmTransportSetQueueDepth(
        pNvMMService->hRmTransport,
        10,
        256)
    );

    NvOsSemaphoreSignal(pNvMMService->SyncSema);
    SignalSent = 1;

    NV_CHECK_ERROR_CLEANUP(
        NvRmTransportWaitForConnect(pNvMMService->hRmTransport, 
                                    NV_WAIT_INFINITE)
    );

    while (!pNvMMService->bShutDown)
    {
        NvOsSemaphoreWait(pNvMMService->NvmmServiceSema);
        if (pNvMMService->bShutDown)
        {
            break;
        }

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

                    pUnmapBufferInfo = 
                        ((NvMMServiceMsgInfo_UnmapBuffer *)&pNvMMService->RcvMsgBuffer[0]);
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
                    NvMMServiceSendMessage(pNvMMService,
                                           (void *)&Response, 
                                           sizeof(NvMMServiceMsgInfo_UnmapBufferResponse));
                }
                break;
                case NVMM_SERVICE_MSG_UnmapBuffer_Response:
                {
                    NvOsSemaphoreSignal(pNvMMService->NvmmServiceRcvSema);
                }
                break;
                case NVMM_SERVICE_MSG_AllocScratchIRAM_Response:
                {
                    NvOsSemaphoreSignal(pNvMMService->NvmmServiceRcvSema);
                }
                case NVMM_SERVICE_MSG_FreeScratchIRAM_Response:
                {
                    NvOsSemaphoreSignal(pNvMMService->NvmmServiceRcvSema);
                }
                case NVMM_SERVICE_MSG_GetInfo:
                {
                    NvMMServiceMsgInfo_GetInfoResponse Response;
                    //Send the ack 
                    Response.MsgType = NVMM_SERVICE_MSG_GetInfo_Response;
                    Response.hAVPService = pNvMMService;
                    NvMMServiceSendMessage(pNvMMService,
                                           (void *)&Response, 
                                           sizeof(NvMMServiceMsgInfo_GetInfoResponse));
                }
                break;

                case NvMMServiceMsgType_NvmmTransCommInfo:
                {
                    NvMMServiceSendMessage(pNvMMService,
                                           (void *)pNvMMService->RcvMsgBuffer, 
                                           pNvMMService->RcvMsgBufferSize);
                }
                break;

                default: 
                    break;
            }
        }
    }
fail:

    if (!SignalSent)
    {
        NvOsSemaphoreSignal(pNvMMService->SyncSema);
    }
    if (pNvMMService->hRmTransport)
    {
        NvRmTransportClose(pNvMMService->hRmTransport);
    }
}
