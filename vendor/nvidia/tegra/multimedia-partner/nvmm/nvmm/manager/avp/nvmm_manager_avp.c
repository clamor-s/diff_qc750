/* Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an
 * express license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvos.h"
#include "nvassert.h"

#include "nvrm_transport.h"
#include "nvrm_moduleloader.h"
#include "nvrm_avp_swi_registry.h"

#include "nvmm_logger.h"
#include "nvmm.h"
#include "nvmm_manager_common.h"
#include "nvmm_manager_avp.h"

/** Maximum number of  clients */
#define NVMM_MAX_CLIENTS 64

typedef struct NvmmManagerClientRec
{
    NvmmManagerClientType eClientType;
    NvMMBlockHandle hBlock;
    NvmmManagerClientCallBack pfCallBack;
    void *Args;
    NvU32 ArgsSize;
    void *hUniqueHandle;
    NvBool bAbnormalTerminationSent;
} NvmmManagerClient;

typedef struct NvmmManagerRec
{
    NvOsSemaphoreHandle   hSema;
    NvOsSemaphoreHandle   hSyncSema;
    NvOsMutexHandle       hMutex;
    NvRmDeviceHandle      hRmDevice;
    NvOsThreadHandle      hThread;
    NvRmTransportHandle   hRmTransport;
    NvBool                bShutDown;
    NvU8                  *pMsgBuffer;
    NvU32                 nMsgBufferSize;
    NvU32                 nNvmmManagerFxnTableId;
    NvmmManagerClient     Client[NVMM_MAX_CLIENTS];
    NvmmManagerFxnTable   AvpFxnTable;

} NvmmManager;

static NvmmManager gs_NvmmManager;

NvError NvmmManagerOpen(void);

void NvmmManagerClose(void);

void NvmmManagerThread(void *arg);

NvError NvmmManagerRegisterBlock(NvMMBlockHandle hBlock, 
                                 NvmmManagerClientCallBack pfBlockCallBack,
                                 void *Args, 
                                 NvU32 ArgsSize,
                                 void *hUniqueHandle);

NvError NvmmManagerUnregisterBlock(NvMMBlockHandle hBlock, 
                                   void *hUniqueHandle);

NvError NvmmManagerHandleAbnormalTermiantion(NvMMBlockHandle hBlock);

NvError NvmmManagerOpen(void)
{
    NvError e = NvSuccess;
    NvmmManagerClient *pClient;
    NvU32 i;
    NvmmManager *pNvmmManager = &gs_NvmmManager;

    NV_LOGGER_PRINT((NVLOG_NVMM_MANAGER, NVLOG_DEBUG, "NvmmManagerOpen+\n"));

    NvOsMemset(pNvmmManager, 0, sizeof(NvmmManager));

    NV_CHECK_ERROR_CLEANUP(NvRmOpen(&pNvmmManager->hRmDevice, 0));

    NV_CHECK_ERROR_CLEANUP(NvOsSemaphoreCreate(&pNvmmManager->hSema, 0));

    NV_CHECK_ERROR_CLEANUP(NvOsSemaphoreCreate(&pNvmmManager->hSyncSema, 0));

    NV_CHECK_ERROR_CLEANUP(NvOsMutexCreate(&pNvmmManager->hMutex));

    pNvmmManager->pMsgBuffer = NvOsAlloc(MSG_BUFFER_SIZE);
    if(!pNvmmManager->pMsgBuffer)
    {
        e = NvError_InsufficientMemory;
        NV_CHECK_ERROR_CLEANUP(e);
    }
    NvOsMemset(pNvmmManager->pMsgBuffer, 0, MSG_BUFFER_SIZE);

    for(i = 0; i < NVMM_MAX_CLIENTS; i++)
    {
        pClient = &pNvmmManager->Client[i];
        pClient->eClientType = NvmmManagerClientType_Force32;
        pClient->hBlock = NULL;
        pClient->pfCallBack = NULL;
        pClient->Args = NULL;
        pClient->ArgsSize = 0;
        pClient->hUniqueHandle = NULL;
        pClient->bAbnormalTerminationSent = NV_FALSE;
    }
    pNvmmManager->bShutDown = NV_FALSE;

    pNvmmManager->AvpFxnTable.NvmmManagerRegisterBlock    = NvmmManagerRegisterBlock;
    pNvmmManager->AvpFxnTable.NvmmManagerUnregisterBlock  = NvmmManagerUnregisterBlock;

    e = NvRmRegisterLibraryCall(NvmmManagerFxnTableId, 
                                &pNvmmManager->AvpFxnTable, 
                                &pNvmmManager->nNvmmManagerFxnTableId);
    NV_CHECK_ERROR_CLEANUP(e);

    e = NvOsThreadCreate(NvmmManagerThread, (void *)pNvmmManager, &(pNvmmManager->hThread));
    NV_CHECK_ERROR_CLEANUP(e);
    NvOsSemaphoreWait(pNvmmManager->hSyncSema);

    NV_LOGGER_PRINT((NVLOG_NVMM_MANAGER, NVLOG_DEBUG, "NvmmManagerOpen-\n"));

    return e;

fail:
    NvmmManagerClose();

    return e;
}

void NvmmManagerClose(void)
{
    NV_LOGGER_PRINT((NVLOG_NVMM_MANAGER, NVLOG_DEBUG, "NvmmManagerClose+\n"));

    gs_NvmmManager.bShutDown = NV_TRUE;
    if(gs_NvmmManager.hThread)
    {
        NvOsSemaphoreSignal(gs_NvmmManager.hSema);
        NvOsThreadJoin(gs_NvmmManager.hThread);
    }
    if(gs_NvmmManager.hRmTransport)
        NvRmTransportClose(gs_NvmmManager.hRmTransport);

    if (gs_NvmmManager.pMsgBuffer)
        NvOsFree(gs_NvmmManager.pMsgBuffer);

    NvRmUnregisterLibraryCall(NvmmManagerFxnTableId, gs_NvmmManager.nNvmmManagerFxnTableId);
    NvOsMutexDestroy(gs_NvmmManager.hMutex);
    NvOsSemaphoreDestroy(gs_NvmmManager.hSyncSema);
    NvOsSemaphoreDestroy(gs_NvmmManager.hSema);
    NvRmClose(gs_NvmmManager.hRmDevice);
    NV_LOGGER_PRINT((NVLOG_NVMM_MANAGER, NVLOG_DEBUG, "NvmmManagerClose-\n"));

    return;
}

NvError AxfModuleMain(NvRmModuleLoaderReason Reason, void *Args, NvU32 ArgsSize)
{
    NvError Status = NvSuccess;
    switch(Reason) 
    {
        case NvRmModuleLoaderReason_Attach:
            NvmmManagerOpen();
        break;

        case NvRmModuleLoaderReason_Detach:
            NvmmManagerClose();
        break;
    }
    return Status;
}

void NvmmManagerThread(void *arg)
{
    NvmmManager *pNvmmManager = &gs_NvmmManager;
    NvError e = NvSuccess;
    NvmmManagerMsgType eMessageType;
    NvmmManagerAbnormalTermMsg *pAbrnormalTerm;
    NvMMBlockHandle hBlock;
    NvU32 SignalSent = 0;

    e = NvRmTransportOpen(pNvmmManager->hRmDevice, 
                          "NVMM_MANAGER_SRV", 
                          pNvmmManager->hSema,
                          &pNvmmManager->hRmTransport);
    NV_CHECK_ERROR_CLEANUP(e);

    e = NvRmTransportSetQueueDepth(pNvmmManager->hRmTransport, 10, 256);
    NV_CHECK_ERROR_CLEANUP(e);

    NvOsSemaphoreSignal(pNvmmManager->hSyncSema);
    SignalSent = 1;

    e = NvRmTransportWaitForConnect(pNvmmManager->hRmTransport, NV_WAIT_INFINITE);
    NV_CHECK_ERROR_CLEANUP(e);

    while(!pNvmmManager->bShutDown)
    {
        NvOsSemaphoreWait(pNvmmManager->hSema);
        if(pNvmmManager->bShutDown)
            break;
        e = NvRmTransportRecvMsg(pNvmmManager->hRmTransport, 
                                 pNvmmManager->pMsgBuffer, 
                                 MSG_BUFFER_SIZE, 
                                 &pNvmmManager->nMsgBufferSize);

        if((e == NvSuccess) && (pNvmmManager->nMsgBufferSize != 0))
        {
            eMessageType = *((NvmmManagerMsgType *)&pNvmmManager->pMsgBuffer[0]);
            switch(eMessageType) 
            {
                case NvmmManagerCallBack_AbnormalTerm:
                {
                    pAbrnormalTerm = ((NvmmManagerAbnormalTermMsg *)&pNvmmManager->pMsgBuffer[0]);
                    hBlock = pAbrnormalTerm->hBlock;
                    NV_LOGGER_PRINT((NVLOG_NVMM_MANAGER, NVLOG_DEBUG, "NvmmManagerCallBack_AbnormalTerm MSG\n"));
                    NvmmManagerHandleAbnormalTermiantion(hBlock);
                }
                break;
            }
        }
    }
fail:
    if (!SignalSent)
    {
        NvOsSemaphoreSignal(pNvmmManager->hSyncSema);
    }
    return;
}

NvError NvmmManagerHandleAbnormalTermiantion(NvMMBlockHandle hBlock)
{
    NvError Status = NvError_InsufficientMemory;
    NvmmManager *pNvmmManager = &gs_NvmmManager;
    NvmmManagerClient *pClient;
    NvMMAttrib_AbnormalTermInfo AbnormalTermInfo;
    void *hUniqueHandle = NULL;
    NvU32 i;

    AbnormalTermInfo.bAbnormalTermination = NV_TRUE;

    /* Find the process specific handle */
    for(i = 0; i < NVMM_MAX_CLIENTS; i++)
    {
        pClient = &pNvmmManager->Client[i];
        if(pClient->hBlock == hBlock)
        {
            hUniqueHandle = pClient->hUniqueHandle;
            break;
        }
    }

    for(i = 0; i < NVMM_MAX_CLIENTS; i++)
    {
        pClient = &pNvmmManager->Client[i];
        if((pClient->hBlock != NULL) && 
           (pClient->hUniqueHandle == hUniqueHandle) &&
           (pClient->eClientType == NvmmManagerClientType_NvmmBlock) &&
           (pClient->bAbnormalTerminationSent == NV_FALSE))
        {
            pClient->hBlock->SetAttribute(pClient->hBlock, 
                                          NvMMAttribute_AbnormalTermination,
                                          NV_FALSE, 
                                          sizeof(NvMMAttrib_AbnormalTermInfo), 
                                          &AbnormalTermInfo);
            pClient->hBlock->SetState(pClient->hBlock, NvMMState_Stopped);
            pClient->bAbnormalTerminationSent = NV_TRUE;
        }
    }

    /* do call back on all clients */
    for(i = 0; i < NVMM_MAX_CLIENTS; i++)
    {
        pClient = &pNvmmManager->Client[i];
        switch(pClient->eClientType) 
        {
            case NvmmManagerClientType_NvmmBlock:
            {
                if((pClient->hBlock != NULL) && (pClient->bAbnormalTerminationSent == NV_TRUE))
                {
                    pClient->pfCallBack(NvmmManagerCallBack_AbnormalTerm, 
                                        pClient->Args, 
                                        pClient->ArgsSize);

                    pClient->hBlock = NULL;
                    pClient->hUniqueHandle = NULL;
                    pClient->eClientType = NvmmManagerClientType_Force32;
                    pClient->pfCallBack = NULL;
                    pClient->Args = NULL;
                    pClient->ArgsSize = 0;
                    pClient->bAbnormalTerminationSent = NV_FALSE;
                }
            }
            break;
        }
    }

    return Status;
}

NvError NvmmManagerRegisterBlock(NvMMBlockHandle hBlock, 
                                 NvmmManagerClientCallBack pfBlockCallBack,
                                 void *Args, 
                                 NvU32 ArgsSize,
                                 void *hUniqueHandle)
{
    NvError Status = NvError_InsufficientMemory;
    NvmmManager *pNvmmManager = &gs_NvmmManager;
    NvmmManagerClient *pClient;
    NvU32 i;

    NV_LOGGER_PRINT((NVLOG_NVMM_MANAGER, NVLOG_DEBUG, "NvmmManagerRegisterBlock+\n"));
    NvOsMutexLock(pNvmmManager->hMutex);
    for(i = 0; i < NVMM_MAX_CLIENTS; i++)
    {
        pClient = &pNvmmManager->Client[i];
        if(pClient->hBlock == NULL)
        {
            pClient->eClientType = NvmmManagerClientType_NvmmBlock;
            pClient->hBlock = hBlock;
            pClient->pfCallBack = pfBlockCallBack;
            pClient->Args = Args;
            pClient->ArgsSize = ArgsSize;
            pClient->hUniqueHandle = hUniqueHandle;
            pClient->bAbnormalTerminationSent = NV_FALSE;
            Status = NvError_Success;
            NV_LOGGER_PRINT((NVLOG_NVMM_MANAGER, NVLOG_DEBUG, "NvmmManagerRegisterBlock-Success\n"));
            break;
        }
    }
    NvOsMutexUnlock(pNvmmManager->hMutex);
    NV_LOGGER_PRINT((NVLOG_NVMM_MANAGER, NVLOG_DEBUG, "NvmmManagerRegisterBlock-\n"));
    return Status;
}


NvError NvmmManagerUnregisterBlock(NvMMBlockHandle hBlock, void *hUniqueHandle)
{
    NvError Status = NvError_InsufficientMemory;
    NvmmManager *pNvmmManager = &gs_NvmmManager;
    NvmmManagerClient *pClient;
    NvU32 i;

    NV_LOGGER_PRINT((NVLOG_NVMM_MANAGER, NVLOG_DEBUG, "NvmmManagerUnregisterBlock+\n"));
    NvOsMutexLock(pNvmmManager->hMutex);
    for(i = 0; i < NVMM_MAX_CLIENTS; i++)
    {
        pClient = &pNvmmManager->Client[i];
        if(pClient->hBlock == hBlock)
        {
            pClient = &pNvmmManager->Client[i];
            pClient->eClientType = NvmmManagerClientType_Force32;
            pClient->hBlock = NULL;
            pClient->hUniqueHandle = NULL;
            pClient->bAbnormalTerminationSent = NV_FALSE;
            Status = NvError_Success;
            NV_LOGGER_PRINT((NVLOG_NVMM_MANAGER, NVLOG_DEBUG, "NvmmManagerUnregisterBlock-Success\n"));
            break;
        }
    }
    NvOsMutexUnlock(pNvmmManager->hMutex);
    NV_LOGGER_PRINT((NVLOG_NVMM_MANAGER, NVLOG_DEBUG, "NvmmManagerUnregisterBlock-\n"));
    return Status;
}


