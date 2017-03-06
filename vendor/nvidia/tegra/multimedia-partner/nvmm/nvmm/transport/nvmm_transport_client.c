/*
 * Copyright (c) 2007 - 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/*
 * nvmm/transport/nvmm_transport.c
 *  This file defines the transportAPI wrappers around the NvMMblock methods and
 * callbacks.
 *  The wrappers are composed of two parts:
 *   1) ClientSide wrapper: resides on the client side. Translates client calls
 *      to the NvMMBlock handle to transport API calls and sends them to the
 *      block side wrapper. Also receives block callback messages from the
 *      BlockSide wrapper and translates them to callback calls on the client's
 *      callback functions.
 *   2) BlockSide wrapper: resides on the block side. Receives client messages,
 *      converts them to NvMMBlock calls. Translates callbacks from the block
 *      to messages
 */
#ifndef NV_ENABLE_DEBUG_PRINTS
#define NV_ENABLE_DEBUG_PRINTS (0)
#endif

#include "nvassert.h"
#include "nvmm_transport.h"
#include "nvmm_transport_private.h"
#include "nvrm_transport.h"
#include "nvrm_memmgr.h"
#include "nvmm_event.h"
#include "nvutil.h"
#include "nvrm_moduleloader.h"
#include "nvmm_util.h"
#include "nvmm_manager.h"
#include "nvmm_mixer.h"
#include "nvmm_service.h"
#include "nvmm_service_common.h"
#include "nvmm.h"
#include "nvrm_hardware_access.h"
#include "nvmm_contextmanager.h"

#if NVMM_KPILOG_ENABLED
#include "nvmm_stats.h"
#endif

typedef struct ClientSideTunnelInfoRec
{
    TransferBufferFunction pTransferBufferToClient;
    NvMMBlockHandle        hBlockOutForTransferBuffer;
    NvMMBlockHandle        hBlockInForTransferBuffer;
    NvU32                  uStreamIndexInForTransferBuffer;
    NvU32                  uStreamIndexOutForTransferBuffer;
    NvU32                  uStreamIndexOutGeneratedForTransferBuffer;
} ClientSideTunnelInfo;

/* ClientSideInfo - Information about the transport wrapper on the client side */
typedef struct ClientSideInfo
{
    NvMMBlockHandle             hActualBlock;
    NvRmDeviceHandle            hRmDevice;
    NvRmTransportHandle         hRmTransport;
    NvmmManagerHandle           hNvmmMgr;
    NvOsThreadHandle            clientThreadHandle;
    BlockSideCreationParameters params;
    volatile NvBool             bShutdown;
    NvBool                      isServiceOpened;
    NvBool                      isRmTransportConnected;
    NvBool                      IsNvmmContextManagerOpen;
    NvBool                      IsNvmmManagerOpen;
    NvBool                      IsBlockRegisteredWithContextManager;
    NvBool                      IsBlockRegisterdWithNvmmManager;
    NvBool                      IsBlockSideOpen;
    NvBool                      bIsTunnelSet[NVMMBLOCK_MAXSTREAMS];

    /* Caches client side callbacks set by the client. Route converted callback
    messages to these functions. */
    SendBlockEventFunction      pEventHandler;
    void*                       clientContext;
    NvU32                       outGoingIndex;
    NvMMState                   eShadowedBlockState;
    NvOsSemaphoreHandle         BlockEventSema;
    NvOsSemaphoreHandle         ClientEventSema;
    NvOsSemaphoreHandle         BlockInfoSema;
    NvOsSemaphoreHandle         BlockWorkerThreadCreateSema;
    NvOsSemaphoreHandle         ClientReceiveThreadCreateSema;
    NvOsMutexHandle             TransportApiLock;
    NvOsMutexHandle             BlockInfoLock;
    NvOsLibraryHandle           hLibraryHandle;

    void*                       pAttribute;
    NvError                     AttributeStatus;
    NvRmLibraryHandle           hRmLoaderHandle;
    NvError                     BlockOpenStatus;

    NvBool                      bGreedyIramAlloc;
    NvmmMgrBlockHandle          hNvmmManagerBlockHandle;

    char                        pBlockPath[NVOS_PATH_MAX];

#if NVMM_KPILOG_ENABLED
    void                        *pNvMMStat;
    void                        *pNvMMAVPProfile;
    NvU32                       profileStatus;
#endif

    ClientSideTunnelInfo        tunnelInfo[NVMMBLOCK_MAXSTREAMS];
    NvOsSemaphoreHandle         hBlockSyncSema;
    NvError                     BlockOpenSyncStatus;

    NvU32                       Align;
    NvU8                        rcvMsgBuffer[MSG_BUFFER_SIZE];
    NvU8                        sendMsgBuffer[MSG_BUFFER_SIZE];
    NvU8                        workBuffer[MSG_BUFFER_SIZE];

} ClientSideInfo;

static NvError ClientSideSetTransferBufferFunction(NvMMBlockHandle hBlock, NvU32 StreamIndex, TransferBufferFunction TransferBuffer, void *pContextForTransferBuffer, NvU32 StreamIndexForTransferBuffer);
static TransferBufferFunction ClientSideGetTransferBufferFunction(NvMMBlockHandle hBlock, NvU32 StreamIndex, void *pContextForTransferBuffer);
static NvError ClientSideSetBufferAllocator(NvMMBlockHandle hBlock, NvU32 StreamIndex, NvBool AllocateBuffers);
static void    ClientSideSetSendBlockEventFunction(NvMMBlockHandle hBlock, void *ClientContext, SendBlockEventFunction SendBlockEvent);
static NvError ClientSideSetState(NvMMBlockHandle hBlock, NvMMState State);
static NvError ClientSideGetState(NvMMBlockHandle hBlock, NvMMState *pState);
static NvError ClientSideAbortBuffers(NvMMBlockHandle hBlock, NvU32 StreamIndex);
static NvError ClientSideSetAttribute(NvMMBlockHandle hBlock, NvU32 AttributeType, NvU32 SetAttrFlag, NvU32 AttributeSize, void *pAttribute);
static NvError ClientSideGetAttribute(NvMMBlockHandle hBlock, NvU32 AttributeType, NvU32 AttributeSize, void *pAttribute);
static NvError ClientSideExtension(NvMMBlockHandle hBlock, NvU32 ExtensionIndex, NvU32 SizeInput, void *pInputData, NvU32 SizeOutput, void *pOutputData);
static NvError ClientSideTransferBufferToBlock(void *pContext, NvU32 StreamIndex, NvMMBufferType BufferType, NvU32 BufferSize, void *pBuffer);
static void    ClientSideCallbackReceiveThread(void *arg);
static NvError ClientSideSendMsg(ClientSideInfo *pClientSide, NvU8* msg, NvU32 size);
static NvError ClientSideRcvMsg(ClientSideInfo *pClientSide, NvU8* msg);
static NvError ClientSideShutDown(ClientSideInfo *pClientSide);
static NvError NvMMTransportLibraryLoad(ClientSideInfo *pClientSide);
static void     NvMMTransportLibraryUnload(ClientSideInfo *pClientSide);
static NvError  NvMMTransportLibraryGetPath(ClientSideInfo *pClientSide);
#if NVMM_KPILOG_ENABLED
static NvError ClientSideSetupProfile(ClientSideInfo *pBlockSideInfo, void *pAttribute);
#endif
static NvError NvMMClientSideOpen(ClientSideInfo *pClientSide, NvMMCreationParameters *pParams);
static void NvMMClientSideClose(ClientSideInfo *pClientSide);
static NvError NvMMClientSideSendBlockCloseMsg(ClientSideInfo *pClientSide);
static void NvMMServiceTransportCallBack(void *pClientContext, void *pEventData, NvU32 EventDataSize);

NvMMBlockType NvMMGetBlockType(NvMMBlockHandle hBlock);

#if NV_LOGGER_ENABLED
static const char *BlockTypeString(NvMMBlockType eType)
{
    switch (eType)
    {
        case NvMMBlockType_DecMPEG2VideoVld: return "MPEG2VDecVldBlock";
        case NvMMBlockType_DecAAC:
        case NvMMBlockType_DecEAACPLUS: return "AacDec";
        case NvMMBlockType_DecADTS: return "AdtsDec";
        case NvMMBlockType_DecMPEG4:
        case NvMMBlockType_DecMPEG4AVP: return "MPEG4DecBlock";
        case NvMMBlockType_EncAMRNB: return "AmrNBEnc";
        case NvMMBlockType_EnciLBC: return "iLBCEnc";
        case NvMMBlockType_DecAMRNB: return "AmrNBDec";
        case NvMMBlockType_DecMP3:
        case NvMMBlockType_DecMP3SW: return "MP3Dec";
        case NvMMBlockType_DecMP2: return "MP2Dec";
        case NvMMBlockType_DecJPEG: return "JpegDecBlock";
        case NvMMBlockType_DecSuperJPEG: return "SuperJpegDecBlock";
        case NvMMBlockType_DecJPEGProgressive: return "JpegProgressiveDecBlock";
        case NvMMBlockType_DecVC1: return "Vc1DecBlock";
        case NvMMBlockType_DecVC1_2x: return "Vc1Dec2xBlock";
        case NvMMBlockType_DecH264:
        case NvMMBlockType_DecH264AVP: return "H264DecBlock";
        case NvMMBlockType_DecH264_2x: return "H264Dec2xBlock";
        case NvMMBlockType_DecMPEG2: return "MPEG2DecBlock";
        case NvMMBlockType_EncJPEG: return "JPEGEncBlock";
        case NvMMBlockType_EncAMRWB: return "AmrWBEnc";
        case NvMMBlockType_DecAMRWB: return "AmrWBDec";
        case NvMMBlockType_DecILBC: return "iLBCDec";
        case NvMMBlockType_DecWAV: return "WavDec";
        case NvMMBlockType_EncWAV: return "WavEnc";
        case NvMMBlockType_AudioMixer: return "Mixer";
        case NvMMBlockType_SrcCamera: return "Camera";
        case NvMMBlockType_SrcUSBCamera: return "USBCamera";
        case NvMMBlockType_RingTone: return "RingTone";
        case NvMMBlockType_DigitalZoom: return "DigitalZoom";
        case NvMMBlockType_EncAAC: return "AacPlusEnc";
        case NvMMBlockType_DecWMA: return "WmaDec";
        case NvMMBlockType_DecWMAPRO: return "WmaproDec";
        case NvMMBlockType_DecWMALSL: return "WmalslDec";
        case NvMMBlockType_DecOGG: return "OGGDec";
        case NvMMBlockType_DecBSAC: return "BsacDec";
        case NvMMBlockType_SuperParser: return "SuperParserBlock";
        case NvMMBlockType_3gppWriter: return "3gpWriterBlock";
        case NvMMBlockType_DecMPEG2VideoRecon: return "Mpeg2VDecReconBlock";
        default: break;
    }

    return "UnKnownBlock";
}

const char *DecipherMsgCode(NvU8 *msg)
{
    static const char *strMsg[] =
    {
        "Unknown msg",
        "NVMM_MSG_SetTransferBufferFunction",
        "NVMM_MSG_GetTransferBufferFunction",
        "NVMM_MSG_SetBufferAllocator",
        "NVMM_MSG_SetSendBlockEventFunction",
        "NVMM_MSG_SetState",
        "NVMM_MSG_GetState",
        "NVMM_MSG_AbortBuffers",
        "NVMM_MSG_SetAttribute",
        "NVMM_MSG_GetAttribute",
        "NVMM_MSG_Extension",
        "NVMM_MSG_TransferBufferFromBlock",
        "NVMM_MSG_TransferBufferToBlock",
        "NVMM_MSG_Close",
        "NVMM_MSG_SetBlockEventHandler",
        "NVMM_MSG_SendEvent",
        "NVMM_MSG_GetAttributeInfo",
        "NVMM_MSG_WorkerThreadCreated",
        "NVMM_MSG_BlockKPI",
        "NVMM_MSG_BlockAVPPROF"
    };

    NvU32 code = *((NvU32*)&msg[0]);
    if (code >= sizeof(strMsg)/sizeof(char*))
        code = 0;

    return strMsg[code];
}
#endif

static void NvMMServiceTransportCallBack(void *pClientContext, void *pEventData, NvU32 EventDataSize)
{
    NvMMTransCommEventType eEventType;
    NvMMTransportBlockOpenInfo *pBlockOpenInfo;
    ClientSideInfo *pClientSide;

    if(pClientContext && pEventData)
    {
        pClientSide = (ClientSideInfo *)pClientContext;
        eEventType = *((NvMMTransCommEventType*)pEventData);
        switch (eEventType)
        {
            case NvMMTransCommEventType_BlockOpenInfo:
            {
                pBlockOpenInfo = (NvMMTransportBlockOpenInfo *)pEventData;
                pClientSide->BlockOpenSyncStatus = pBlockOpenInfo->BlockOpenSyncStatus;
                if(pClientSide->hBlockSyncSema)
                    NvOsSemaphoreSignal(pClientSide->hBlockSyncSema);
            }
            break;

            case NvMMTransCommEventType_BlockSetTBFCompleteInfo:
            {
                if(pClientSide->BlockInfoSema)
                    NvOsSemaphoreSignal(pClientSide->BlockInfoSema);
            }
            break;

            default:
                break;
        }
    }
}

static NvError NvMMClientSideSendBlockCloseMsg(ClientSideInfo *pClientSide)
{
    NvError status = NvSuccess;
    NvU8* msg;
    NvU32 size = 0;
    if(pClientSide->isRmTransportConnected == NV_TRUE)
    {
        NvOsMutexLock(pClientSide->TransportApiLock);
        msg = pClientSide->sendMsgBuffer;
        *((NvU32 *)&msg[size]) = NVMM_MSG_Close;
        size += sizeof(NvU32);
        *((NvBool *)&msg[size]) = NV_FALSE;
        size += sizeof(NvBool);
        ClientSideSendMsg(pClientSide, msg, size);
        NvOsMutexUnlock(pClientSide->TransportApiLock);
    }
    return status;
}

/** Constructor for the block which is used by NvMMOpenMMBlock. */
NvError NvMMTransOpenBlock(NvMMBlockHandle *phBlock, NvMMCreationParameters *pParams)
{
    NvError status = NvSuccess;
    NvMMBlock *pBlock;
    ClientSideInfo *pClientSide = NULL;

    pBlock = NvOsAlloc(sizeof (NvMMBlock));
    if (!pBlock)
    {
        status = NvError_InsufficientMemory;
        goto cleanup;
    }

    pClientSide = NvOsAlloc(sizeof (ClientSideInfo));
    if (!pClientSide)
    {
        status = NvError_InsufficientMemory;
        goto cleanup;
    }
    NvOsMemset(pClientSide, 0, sizeof(ClientSideInfo));

    // ManagerOpen should be moved to system start??
    pClientSide->IsNvmmManagerOpen = NV_FALSE;
    status = NvmmManagerOpen(&pClientSide->hNvmmMgr);
    if (status != NvSuccess) goto cleanup;
    pClientSide->IsNvmmManagerOpen = NV_TRUE;

    pClientSide->IsNvmmContextManagerOpen = NV_FALSE;

    status = NvmmContextManagerOpen(pClientSide->hNvmmMgr);
    if (status != NvSuccess) goto cleanup;
    pClientSide->IsNvmmContextManagerOpen = NV_TRUE;

    status = NvOsSemaphoreCreate(&(pClientSide->ClientEventSema), 0);
    if (status != NvSuccess) goto cleanup;

    status = NvOsSemaphoreCreate(&(pClientSide->hBlockSyncSema), 0);
    if (status != NvSuccess) goto cleanup;

    status = NvOsSemaphoreCreate(&(pClientSide->BlockInfoSema), 0);
    if (status != NvSuccess) goto cleanup;

    status = NvOsSemaphoreCreate(&(pClientSide->BlockWorkerThreadCreateSema), 0);
    if (status != NvSuccess) goto cleanup;
    pClientSide->params.BlockWorkerThreadCreateSema = pClientSide->BlockWorkerThreadCreateSema;

    status = NvOsSemaphoreCreate(&(pClientSide->ClientReceiveThreadCreateSema), 0);
    if (status != NvSuccess) goto cleanup;

    status = NvOsMutexCreate(&pClientSide->TransportApiLock);
    if (status != NvSuccess) goto cleanup;

    status = NvOsMutexCreate(&pClientSide->BlockInfoLock);
    if (status != NvSuccess) goto cleanup;

    // Open rm device handle.
    status = NvRmOpen(&pClientSide->hRmDevice, 0);
    if (status != NvSuccess) goto cleanup;

    status = NvMMClientSideOpen(pClientSide, pParams);
    if (status != NvSuccess) goto cleanup;

    pBlock->StructSize                  = sizeof(NvMMBlock);
    pBlock->pContext                    = pClientSide;
    pBlock->AbortBuffers                = ClientSideAbortBuffers;
    pBlock->GetAttribute                = ClientSideGetAttribute;
    pBlock->SetAttribute                = ClientSideSetAttribute;
    pBlock->Extension                   = ClientSideExtension;
    pBlock->GetState                    = ClientSideGetState;
    pBlock->SetState                    = ClientSideSetState;
    pBlock->SetBufferAllocator          = ClientSideSetBufferAllocator;
    pBlock->SetSendBlockEventFunction   = ClientSideSetSendBlockEventFunction;
    pBlock->SetTransferBufferFunction   = ClientSideSetTransferBufferFunction;
    pBlock->GetTransferBufferFunction   = ClientSideGetTransferBufferFunction;

    pClientSide->IsBlockRegisteredWithContextManager = NV_FALSE;
    status = NvmmContextManagerRegisterBlock(pBlock);
    if (status != NvSuccess) goto cleanup;
    pClientSide->IsBlockRegisteredWithContextManager = NV_TRUE;

    *phBlock = pBlock;
    return NvSuccess;

cleanup:
    if (pClientSide)
    {
        pClientSide->bShutdown = NV_TRUE;
        if(pClientSide->IsBlockSideOpen == NV_TRUE)
            NvMMClientSideSendBlockCloseMsg(pClientSide);
        if (pClientSide->ClientEventSema)
            NvOsSemaphoreSignal(pClientSide->ClientEventSema);
        NvMMClientSideClose(pClientSide);
        if(pClientSide->IsBlockRegisteredWithContextManager == NV_TRUE)
            NvmmContextManagerUnregisterBlock(pBlock);
        if(pClientSide->IsNvmmContextManagerOpen == NV_TRUE)
            NvmmContextManagerClose();
        if(pClientSide->IsNvmmManagerOpen == NV_TRUE)
            NvmmManagerClose(pClientSide->hNvmmMgr, NV_FALSE);
        NvOsSemaphoreDestroy(pClientSide->ClientEventSema);
        NvOsSemaphoreDestroy(pClientSide->hBlockSyncSema);
        NvOsSemaphoreDestroy(pClientSide->BlockInfoSema);
        NvOsSemaphoreDestroy(pClientSide->BlockWorkerThreadCreateSema);
        NvOsSemaphoreDestroy(pClientSide->ClientReceiveThreadCreateSema);
        NvOsMutexDestroy(pClientSide->TransportApiLock);
        NvOsMutexDestroy(pClientSide->BlockInfoLock);
    }
    NvOsFree(pClientSide);
    NvOsFree(pBlock);
    return status;
}


static NvError NvMMClientSideOpen(ClientSideInfo *pClientSide, NvMMCreationParameters *pParams)
{
    NvError status = NvSuccess;
    NvBlockProfile BP;
    NvBlockProfile *pBP = &BP;
    void *hServiceRmLoader;
    NvBool loadServiceAxf = NV_FALSE;

    pClientSide->IsBlockSideOpen        = NV_FALSE;
    pClientSide->isServiceOpened        = NV_FALSE;
    pClientSide->isServiceOpened        = NV_FALSE;
    pClientSide->isRmTransportConnected = NV_FALSE;
    pClientSide->IsBlockRegisterdWithNvmmManager = NV_FALSE;
    pClientSide->BlockOpenSyncStatus = NvSuccess;
    status = NvRmTransportOpen(pClientSide->hRmDevice, NULL, pClientSide->ClientEventSema, &pClientSide->hRmTransport);
    if (status != NvSuccess) goto cleanup;

    status = NvmmManagerRegisterBlock(
        pClientSide->hNvmmMgr,
        &pClientSide->hNvmmManagerBlockHandle,
        pParams->Type,
        NvmmUCType_Default,
        pBP);
    if (status != NvSuccess) goto cleanup;

    pClientSide->IsBlockRegisterdWithNvmmManager = NV_TRUE;

    pClientSide->bGreedyIramAlloc = NV_FALSE;
    pClientSide->params.eType = pParams->Type;

    pClientSide->params.eLocale = pBP->Locale;

    if(NvBlockFlag_UseNewBlockType & pBP->BlockFlags)
    {
        pClientSide->params.eType = pBP->BlockType;
    }

    if (pClientSide->params.eLocale == NvMMLocale_AVP ||
        pClientSide->params.eType == NvMMBlockType_DecH264)
    {
        loadServiceAxf = NV_TRUE;
    }

#if !NV_DEF_ENVIRONMENT_SUPPORTS_SIM
    status = NvMMServiceOpen(loadServiceAxf);
    if (status == NvSuccess)
        pClientSide->isServiceOpened = NV_TRUE;
    else
        goto cleanup;
#endif

    pClientSide->params.BlockSpecific = pParams->BlockSpecific;
    pClientSide->params.SetULPMode = pParams->SetULPMode;
    pClientSide->params.LogLevel = NV_LOGGER_GETLEVEL(BlockTypeToLogClientType(pClientSide->params.eType));
    pClientSide->params.StackPtr = pBP->StackPtr;
    pClientSide->params.StackSize = pBP->StackSize;
    pClientSide->params.pTransCallback = NvMMServiceTransportCallBack;
    pClientSide->params.pClientSide = (void*)pClientSide;
    pClientSide->params.BlockInfoSema = pClientSide->BlockInfoSema;
    if((pParams->SetULPMode != NV_FALSE) && (NvBlockFlag_UseGreedyIramAlloc & pBP->BlockFlags))
    {
        pClientSide->bGreedyIramAlloc = NV_TRUE;
    }

    NvRmTransportGetPortName(pClientSide->hRmTransport, (NvU8 *)pClientSide->params.blockName, NVMM_BLOCKNAME_LENGTH);
    status =  NvRmTransportSetQueueDepth(pClientSide->hRmTransport, 30, 256);
    if (status != NvSuccess) goto cleanup;

    if (pClientSide->params.eLocale == NvMMLocale_AVP)
    {
        status = GetAVPServiceHandle(&pClientSide->params.hAVPService);
        if (status != NvSuccess) goto cleanup;
    }

    status = NvMMTransportLibraryLoad(pClientSide);
    if (status != NvSuccess) goto cleanup;

    if (pClientSide->params.eLocale == NvMMLocale_CPU)
    {
        status = NvOsSemaphoreCreate(&(pClientSide->BlockEventSema), 0);
        if (status != NvSuccess) goto cleanup;

        pClientSide->params.BlockEventSema = pClientSide->BlockEventSema;
        /* create a blockSide thread and connect to the client side */
        status = NvOsThreadCreate(
            (NvOsThreadFunction)BlockSideWorkerThread,
            &pClientSide->params,
            &pClientSide->params.blockSideWorkerHandle);
        if (status != NvSuccess) goto cleanup;
    }
    /* Make sure that block can connect(no failures before connect) */
    NvOsSemaphoreWait(pClientSide->hBlockSyncSema);
    if(pClientSide->BlockOpenSyncStatus != NvSuccess)
    {
        status = pClientSide->BlockOpenSyncStatus;
        goto cleanup;
    }
    status = NvRmTransportConnect(pClientSide->hRmTransport, 60000);
    if (status != NvSuccess) goto cleanup;
    pClientSide->isRmTransportConnected = NV_TRUE;

    status = NvOsThreadCreate(ClientSideCallbackReceiveThread, pClientSide, &pClientSide->clientThreadHandle );
    if (status != NvSuccess) goto cleanup;
    NvOsSemaphoreWait(pClientSide->ClientReceiveThreadCreateSema);

    pClientSide->eShadowedBlockState = NvMMState_Stopped;
    NvOsSemaphoreWait(pClientSide->BlockWorkerThreadCreateSema);
    if(pClientSide->BlockOpenStatus != NvSuccess)
    {
        status = pClientSide->BlockOpenStatus;
        goto cleanup;
    }

    pClientSide->IsBlockSideOpen = NV_TRUE;
    if (pClientSide->params.eLocale == NvMMLocale_AVP)
    {
        NvMMServiceGetLoaderHandle(&hServiceRmLoader);
        status = NvmmManagerUpdateBlockInfo(pClientSide->hNvmmManagerBlockHandle,
                                            (void*)pClientSide->hActualBlock,
                                            (void*)pClientSide->hRmLoaderHandle,
                                            (void*)pClientSide->params.hAVPService,
                                            (void*)hServiceRmLoader);
        if (status != NvSuccess) goto cleanup;
    }
    return NvSuccess;

cleanup:
    return status;
}

static void NvMMClientSideClose(ClientSideInfo* pClientSide)
{
    NvMMLocale locale = pClientSide->params.eLocale;

    if(pClientSide->clientThreadHandle)
    {
        NvOsThreadJoin(pClientSide->clientThreadHandle);
    }

    if((locale == NvMMLocale_CPU) && (pClientSide->params.blockSideWorkerHandle))
    {
        NvOsThreadJoin(pClientSide->params.blockSideWorkerHandle);
    }

#if NVMM_KPILOG_ENABLED
    if (pClientSide->pNvMMStat)
    {
        NvMMStats_ComputeStatistics(pClientSide->pNvMMStat);
        NvMMStatsDestroy(pClientSide->pNvMMStat);
    }
    if (pClientSide->pNvMMAVPProfile)
    {
        NvMMAVPProfileDumpToDisk(pClientSide->pNvMMAVPProfile);
        NvMMAVPProfileDestroy(pClientSide->pNvMMAVPProfile);
    }
#endif
    if(pClientSide->IsBlockRegisterdWithNvmmManager == NV_TRUE)
    {
        NvmmManagerUnregisterBlock(pClientSide->hNvmmManagerBlockHandle, NV_FALSE);
    }
    if (pClientSide->hRmTransport)
    {
        NvRmTransportClose(pClientSide->hRmTransport);
        pClientSide->hRmTransport = 0;
    }

    NvMMTransportLibraryUnload(pClientSide);

#if !NV_DEF_ENVIRONMENT_SUPPORTS_SIM
    if(pClientSide->isServiceOpened == NV_TRUE)
        NvMMServiceClose();
#endif
}

void NvMMTransCloseBlock(NvMMBlockHandle hBlock)
{
    ClientSideInfo* pClientSide = hBlock->pContext;
    NvU8* msg;
    NvU32 size = 0;
    NvU32 i;
    NvMMAttrib_BlockInfo BlockInfo;
    NvMMBlockCloseInfo BlockCloseInfo;

    NvOsMemset(&BlockCloseInfo, 0, sizeof(NvMMBlockCloseInfo));
    NvOsMemset(&BlockInfo, 0, sizeof(NvMMAttrib_BlockInfo));

    ClientSideGetAttribute(hBlock,
                           NvMMAttribute_GetBlockInfo,
                           sizeof(NvMMAttrib_BlockInfo),
                           &BlockInfo);

    NvOsMutexLock(pClientSide->TransportApiLock);
    msg = pClientSide->sendMsgBuffer;
    *((NvU32 *)&msg[size]) = NVMM_MSG_Close;
    size += sizeof(NvU32);
     *((NvBool *)&msg[size]) = NV_FALSE; // is format change request
    size += sizeof(NvBool);
    ClientSideSendMsg(pClientSide, msg, size);
    NvOsMutexUnlock(pClientSide->TransportApiLock);

    /* wait for buffer flow if tunnel (Set TBF) is set */
    for(i = 0; i < BlockInfo.NumStreams; i++)
    {
        if(pClientSide->bIsTunnelSet[i] == NV_TRUE)
        {
            /**
             * Video block on AVP does't seem like complete block, even though it
             * uses NvMM Transport layer
             *
             * Camera and DZ don't need to wait for this semaphore for now.
             * See bug 627808 for detail
             */
            if(!((((pClientSide->params.eType == NvMMBlockType_DecMPEG4AVP) ||
                   (pClientSide->params.eType == NvMMBlockType_DecH264AVP)) &&
                   (pClientSide->params.eLocale == NvMMLocale_AVP)) ||
                  ((pClientSide->params.eType == NvMMBlockType_SrcCamera ||
                    pClientSide->params.eType == NvMMBlockType_SrcUSBCamera ||
                    pClientSide->params.eType == NvMMBlockType_DigitalZoom) &&
                   (pClientSide->params.eLocale == NvMMLocale_CPU))))
            {
                NvOsSemaphoreWait(pClientSide->hBlockSyncSema);
            }
        }
    }

    NvMMClientSideClose(pClientSide);
    if(pClientSide->IsBlockRegisteredWithContextManager == NV_TRUE)
        NvmmContextManagerUnregisterBlock(hBlock);
    ClientSideShutDown(pClientSide);
    NvmmContextManagerClose();
    NvmmManagerClose(pClientSide->hNvmmMgr, NV_FALSE);
    BlockCloseInfo.structSize = sizeof(NvMMBlockCloseInfo);
    BlockCloseInfo.blockClosed = NV_TRUE;
    if(pClientSide->pEventHandler)
    {
        pClientSide->pEventHandler(pClientSide->clientContext,
                                   NvMMEvent_BlockClose,
                                   BlockCloseInfo.structSize,
                                   &BlockCloseInfo);
    }
    // Free the main context (which holds the mutex) last
    NvOsFree(pClientSide);
    NvOsFree(hBlock);

}

NvError ClientSideShutDown(ClientSideInfo *pClientSide)
{
    NvError status = NvSuccess;

    NvRmClose(pClientSide->hRmDevice);
    NvOsSemaphoreDestroy(pClientSide->ClientEventSema);
    NvOsSemaphoreDestroy(pClientSide->hBlockSyncSema);
    NvOsSemaphoreDestroy(pClientSide->BlockInfoSema);
    NvOsSemaphoreDestroy(pClientSide->BlockWorkerThreadCreateSema);
    NvOsSemaphoreDestroy(pClientSide->ClientReceiveThreadCreateSema);
    NvOsMutexDestroy(pClientSide->TransportApiLock);
    NvOsMutexDestroy(pClientSide->BlockInfoLock);

    return status;
}

static NvError NvMMTransportLibraryLoad(ClientSideInfo *pClientSide)
{
    NvError status = NvSuccess;
    status = NvMMTransportLibraryGetPath(pClientSide);
    if (status != NvSuccess)
        return status;

    switch(pClientSide->params.eLocale)
    {
    case NvMMLocale_AVP:
        status =  NvRmLoadLibraryEx(pClientSide->hRmDevice,
                                    pClientSide->pBlockPath,
                                    &pClientSide->params,
                                    sizeof(BlockSideCreationParameters),
                                    pClientSide->bGreedyIramAlloc,
                                    &pClientSide->hRmLoaderHandle);
        if (status != NvSuccess)
        {
            pClientSide->hRmLoaderHandle = NULL;
        }
        break;

    case NvMMLocale_CPU:
#if (NVMM_BUILT_DYNAMIC)
        status = NvOsLibraryLoad(pClientSide->pBlockPath,
                                 &pClientSide->hLibraryHandle);
        if (status != NvSuccess)
        {
            pClientSide->hLibraryHandle = NULL;
        }
        pClientSide->params.hLibraryHandle = pClientSide->hLibraryHandle;
#endif
        break;

    default:
        status = NvError_NotSupported;
        break;
    }
    return status;
}


static void NvMMTransportLibraryUnload(ClientSideInfo *pClientSide)
{
    switch(pClientSide->params.eLocale)
    {
    case NvMMLocale_AVP:
        if (pClientSide->hRmLoaderHandle)
            NvRmFreeLibrary(pClientSide->hRmLoaderHandle);
        break;
    case NvMMLocale_CPU:
#if (NVMM_BUILT_DYNAMIC)
        if (pClientSide->hLibraryHandle)
            NvOsLibraryUnload(pClientSide->hLibraryHandle);
#endif
        break;
    default:
        break;
    }
}

static NvError NvMMTransportLibraryGetPath(ClientSideInfo *pClientSide)
{
    NvError status = NvSuccess;
    NvU32 PathLen = 0;

    NvOsMemset(&pClientSide->pBlockPath[0], 0, NVOS_PATH_MAX);
    switch(pClientSide->params.eType)
    {
        case NvMMBlockType_DecAAC:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_aacdec.axf", NvOsStrlen("nvmm_aacdec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;
        case NvMMBlockType_DecADTS:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_adtsdec.axf", NvOsStrlen("nvmm_adtsdec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_DecMP2:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_mp2dec.axf", NvOsStrlen("nvmm_mp2dec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_DecMP3:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_mp3dec.axf", NvOsStrlen("nvmm_mp3dec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_DecMP3SW:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_sw_mp3dec.axf", NvOsStrlen("nvmm_sw_mp3dec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_DecMPEG4:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_mpeg4dec.axf", NvOsStrlen("nvmm_mpeg4dec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_video", NvOsStrlen("libnvmm_video"));
            break;

        case NvMMBlockType_DecMPEG4AVP:
            NV_ASSERT(pClientSide->params.eLocale == NvMMLocale_AVP);
            NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_mpeg4dec.axf", NvOsStrlen("nvmm_mpeg4dec.axf"));
            break;

        case NvMMBlockType_DecH264:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_h264dec.axf", NvOsStrlen("nvmm_h264dec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_video", NvOsStrlen("libnvmm_video"));
            break;

        case NvMMBlockType_DecH264AVP:
            NV_ASSERT(pClientSide->params.eLocale == NvMMLocale_AVP);
            NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_h264dec.axf", NvOsStrlen("nvmm_h264dec.axf"));
            break;

        case NvMMBlockType_DecH264_2x:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
            NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_h264dec2x.axf", NvOsStrlen("nvmm_h264dec2x.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_video", NvOsStrlen("libnvmm_video"));
            break;

        case NvMMBlockType_DecMPEG2:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_mpeg2dec.axf", NvOsStrlen("nvmm_mpeg2dec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_video", NvOsStrlen("libnvmm_video"));
            break;

        case NvMMBlockType_DecJPEG:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_jpegdec.axf", NvOsStrlen("nvmm_jpegdec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_image", NvOsStrlen("libnvmm_image"));
            break;

        case NvMMBlockType_DecSuperJPEG:
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_image", NvOsStrlen("libnvmm_image"));
            break;

        case NvMMBlockType_DecJPEGProgressive:
            NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_image", NvOsStrlen("libnvmm_image"));
            break;

        case NvMMBlockType_DecVC1:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_vc1dec.axf", NvOsStrlen("nvmm_vc1dec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_vc1_video", NvOsStrlen("libnvmm_vc1_video"));
            break;

        case NvMMBlockType_DecVC1_2x:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_vc1dec_2x.axf", NvOsStrlen("nvmm_vc1dec_2x.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_vc1_video", NvOsStrlen("libnvmm_vc1_video"));
            break;

            case NvMMBlockType_DecAMRNB:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_amrnbdec.axf", NvOsStrlen("nvmm_amrnbdec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_EncAMRNB:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_amrnbenc.axf", NvOsStrlen("nvmm_amrnbenc.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_EnciLBC:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_ilbcenc.axf", NvOsStrlen("nvmm_ilbcenc.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_SuperParser:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_superparser.axf", NvOsStrlen("nvmm_superparser.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_parser", NvOsStrlen("libnvmm_parser"));
            break;

        case NvMMBlockType_DecOGG:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_oggdec.axf", NvOsStrlen("nvmm_oggdec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

         case NvMMBlockType_DecBSAC:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_bsacdec.axf", NvOsStrlen("nvmm_bsacdec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_DecAMRWB:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_amrwbdec.axf", NvOsStrlen("nvmm_amrwbdec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_DecILBC:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_ilbcdec.axf", NvOsStrlen("nvmm_ilbcdec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_EncAMRWB:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_amrwbenc.axf", NvOsStrlen("nvmm_amrwbenc.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_DecEAACPLUS:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_aacdec.axf", NvOsStrlen("nvmm_aacdec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_DecWAV:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_wavdec.axf", NvOsStrlen("nvmm_wavdec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_EncWAV:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_wavenc.axf", NvOsStrlen("nvmm_wavenc.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_EncAAC:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_encaac.axf", NvOsStrlen("nvmm_encaac.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_DecWMA:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_wmadec.axf", NvOsStrlen("nvmm_wmadec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_msaudio", NvOsStrlen("libnvmm_msaudio"));
            break;

        case NvMMBlockType_DecWMAPRO:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_wmaprodec.axf", NvOsStrlen("nvmm_wmaprodec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_msaudio", NvOsStrlen("libnvmm_msaudio"));
            break;

        case NvMMBlockType_DecWMALSL:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_wmaLsldec.axf", NvOsStrlen("nvmm_wmaLsldec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_msaudio", NvOsStrlen("libnvmm_msaudio"));
            break;

        case NvMMBlockType_DecWMASUPER:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_superdec.axf", NvOsStrlen("nvmm_superdec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_msaudio", NvOsStrlen("libnvmm_msaudio"));
            break;
        case NvMMBlockType_DecSUPERAUDIO:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_superaudiodec.axf", NvOsStrlen("nvmm_superaudiodec.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_DecMPEG2VideoVld:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_vdecmpeg2vld.axf", NvOsStrlen("nvmm_vdecmpeg2vld.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_video", NvOsStrlen("libnvmm_video"));
            break;

        case NvMMBlockType_EncJPEG:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_jpegenc.axf", NvOsStrlen("nvmm_jpegenc.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_image", NvOsStrlen("libnvmm_image"));
            break;

        case NvMMBlockType_AudioMixer:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_audiomixer.axf", NvOsStrlen("nvmm_audiomixer.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_SinkAudio:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_sinkaudio.axf", NvOsStrlen("nvmm_sinkaudio.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_SourceAudio:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_sourceaudio.axf", NvOsStrlen("nvmm_sourceaudio.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_RingTone:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_RingTone.axf", NvOsStrlen("nvmm_RingTone.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_audio", NvOsStrlen("libnvmm_audio"));
            break;

        case NvMMBlockType_SrcCamera:
        case NvMMBlockType_SrcUSBCamera:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_srccamera.axf", NvOsStrlen("nvmm_srccamera.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_camera", NvOsStrlen("libnvmm_camera"));
            break;

        case NvMMBlockType_DigitalZoom:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_digitalzoom.axf", NvOsStrlen("nvmm_digitalzoom.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_camera", NvOsStrlen("libnvmm_camera"));
            break;

       case NvMMBlockType_3gppWriter:
           if(pClientSide->params.eLocale == NvMMLocale_AVP)
               NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_writer.axf", NvOsStrlen("nvmm_writer.axf"));
           else
               NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_writer", NvOsStrlen("libnvmm_writer"));
           break;

       case NvMMBlockType_DecMPEG2VideoRecon:
            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"nvmm_mpeg2vdec_recon.axf", NvOsStrlen("nvmm_mpeg2vdec_recon.axf"));
            else
                NvOsMemcpy((char *)&pClientSide->pBlockPath[PathLen], (const char *)"libnvmm_video", NvOsStrlen("libnvmm_video"));
            break;

        default:
           status = NvError_NotSupported;
    }

    return status;
}


/*
    In this case the clientside simply tells that blockside that there some
    non-null transferbuffer function. The blockside will register a
    blockside-local transfer buffer function and route calls to that function
    via a message to the clientside. The client side will tranlate the message
    based callback to a call on the function registered here (which is cached
    for just this purpose)
*/
static NvError ClientSideSetTransferBufferFunction(
    NvMMBlockHandle hBlock,
    NvU32 StreamIndex,
    TransferBufferFunction TransferBuffer,
    void *pContextForTranferBuffer,
    NvU32 StreamIndexForTransferBuffer)
{
    NvError status = NvSuccess;
    ClientSideInfo *pClientSide = ((NvMMBlock *)hBlock)->pContext;
    ClientSideInfo *pClientSideOutGoing = NULL;
    NvU8* msg;
    NvU32 size = 0;
    NvU32 StreamIndexOut = 0;
    NvU32 i;
    NvU32 TunnelledMode = 0;
    NvBool bResetTunnel = NV_FALSE;
    NvU32 uResetTunnelIndexOut = 0;

    TransferBufferFunction pfTransferBufferOut;
    ClientSideTunnelInfo *pTunnelInfo = NULL;

    NvOsMutexLock(pClientSide->TransportApiLock);

    NV_ASSERT(pClientSide->outGoingIndex < NVMMBLOCK_MAXSTREAMS);
    /* Find whether client is trying to override existing tunnel */
    for(i = 0; i < NVMMBLOCK_MAXSTREAMS; i++)
    {
        pTunnelInfo = &pClientSide->tunnelInfo[i];
        if((pTunnelInfo->uStreamIndexInForTransferBuffer == StreamIndex) &&
           (pTunnelInfo->hBlockInForTransferBuffer       == hBlock))
        {
            uResetTunnelIndexOut = pTunnelInfo->uStreamIndexOutGeneratedForTransferBuffer;
            bResetTunnel         =  NV_TRUE;
            break;
        }
    }

    if(TransferBuffer == ClientSideTransferBufferToBlock)
    {
        pClientSideOutGoing = ((NvMMBlock *)pContextForTranferBuffer)->pContext;
        StreamIndexOut      = StreamIndexForTransferBuffer; /* Use client passed ouput index */

        if(pClientSide->params.eLocale == pClientSideOutGoing->params.eLocale)
        {
            /* nothing to cache tunnel is set up on block side directly */
            TunnelledMode = NVMM_TUNNEL_MODE_DIRECT;
        }
        else
        {
            if(pClientSideOutGoing->params.eLocale == NvMMLocale_CPU)
            {
                /* Use uResetTunnelIndexOut output index if TBF is being reset */
                if(bResetTunnel == NV_TRUE)
                    StreamIndexOut = uResetTunnelIndexOut;
                else
                    StreamIndexOut = pClientSide->outGoingIndex;  /* Use generated index */

                pfTransferBufferOut = pClientSideOutGoing->hActualBlock->GetTransferBufferFunction(
                                                           pClientSideOutGoing->hActualBlock,
                                                           StreamIndexForTransferBuffer,
                                                           NULL);
                /* cache this info in clientsideinfo */
                pTunnelInfo = &pClientSide->tunnelInfo[StreamIndexOut];
                pTunnelInfo->hBlockInForTransferBuffer        = hBlock;
                pTunnelInfo->hBlockOutForTransferBuffer       = pClientSideOutGoing->hActualBlock;
                pTunnelInfo->pTransferBufferToClient          = pfTransferBufferOut;
                pTunnelInfo->uStreamIndexInForTransferBuffer  = StreamIndex;
                pTunnelInfo->uStreamIndexOutForTransferBuffer = StreamIndexForTransferBuffer;
                pTunnelInfo->uStreamIndexOutGeneratedForTransferBuffer = StreamIndexOut;
                /* Increment generated ouput stream index in tunnel "non-reset" case */
                if(bResetTunnel == NV_FALSE)
                    pClientSide->outGoingIndex ++;
            }
            else
            {
                /* nothing to cache tunnel is set up on block side directly */
                if (pClientSide->params.eLocale == NvMMLocale_CPU)
                    TunnelledMode = NVMM_TUNNEL_MODE_INDIRECT;
            }
        }
    }
    else
    {
        /* Use uResetTunnelIndexOut output index if TBF is being reset */
        if(bResetTunnel == NV_TRUE)
            StreamIndexOut = uResetTunnelIndexOut;
        else
            StreamIndexOut = pClientSide->outGoingIndex;  /* Use generated index */

        /* cache this info in clientsideinfo */
        pTunnelInfo = &pClientSide->tunnelInfo[StreamIndexOut];
        pTunnelInfo->hBlockInForTransferBuffer        = hBlock;
        pTunnelInfo->hBlockOutForTransferBuffer       = pContextForTranferBuffer;
        pTunnelInfo->pTransferBufferToClient          = TransferBuffer;
        pTunnelInfo->uStreamIndexInForTransferBuffer  = StreamIndex;
        pTunnelInfo->uStreamIndexOutForTransferBuffer = StreamIndexForTransferBuffer;
        pTunnelInfo->uStreamIndexOutGeneratedForTransferBuffer = StreamIndexOut;
        /* Increment generated ouput stream index in tunnel non-reset case */
        if(bResetTunnel == NV_FALSE)
            pClientSide->outGoingIndex ++;
    }
    msg = pClientSide->sendMsgBuffer;
    /* the blockside only needs to know whether the callback is non-null
    (see note above) */
    *((NvU32 *)&msg[size]) = NVMM_MSG_SetTransferBufferFunction;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = StreamIndex;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = StreamIndexOut;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = TunnelledMode;
    size += sizeof(NvU32);
    if(TunnelledMode == NVMM_TUNNEL_MODE_DIRECT)
    {
        *((NvMMBlockHandle *)&msg[size]) = (NvMMBlockHandle)pClientSideOutGoing->hActualBlock;
        size += sizeof(NvMMBlockHandle);
    }
    else if (TunnelledMode == NVMM_TUNNEL_MODE_INDIRECT)
    {
        *((TransferBufferFunction *)&msg[size]) = ClientSideTransferBufferToBlock;
        size += sizeof(TransferBufferFunction);
        *((NvMMBlockHandle *)&msg[size]) = (NvMMBlockHandle)pContextForTranferBuffer;
        size += sizeof(NvMMBlockHandle);
    }
    status = ClientSideSendMsg(pClientSide, msg, size);
    NvOsMutexUnlock(pClientSide->TransportApiLock);
    if (status == NvSuccess)
    {
        /* Block the client until ClientSideSetTransferBufferFunction is done */
        NvOsSemaphoreWait(pClientSide->BlockInfoSema);
    }
    if((TunnelledMode == NVMM_TUNNEL_MODE_DIRECT) ||
       (TunnelledMode == NVMM_TUNNEL_MODE_INDIRECT))
        pClientSide->bIsTunnelSet[StreamIndex] = NV_TRUE;

    return status;
}


static TransferBufferFunction ClientSideGetTransferBufferFunction(
    NvMMBlockHandle hBlock,
    NvU32 StreamIndex,
    void *pContextForTransferBuffer)
{
    return ClientSideTransferBufferToBlock;
}

static NvError ClientSideSetBufferAllocator(
    NvMMBlockHandle hBlock,
    NvU32 StreamIndex,
    NvBool AllocateBuffers)
{
    NvError status = NvSuccess;
    ClientSideInfo *pClientSide = ((NvMMBlock *)hBlock)->pContext;
    NvU8* msg;
    NvU32 size = 0;

    NvOsMutexLock(pClientSide->TransportApiLock);

    msg = pClientSide->sendMsgBuffer;
    *((NvU32 *)&msg[size]) = NVMM_MSG_SetBufferAllocator;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = StreamIndex;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = AllocateBuffers;
    size += sizeof(NvBool);
    status = ClientSideSendMsg(pClientSide, msg, size);

    NvOsMutexUnlock(pClientSide->TransportApiLock);
    return status;
}

/*
    In this case the clientside simply tells that blockside that there some
    non-null event handler. The blockside will register a blockside-local
    event handler and route calls to that handler via a message to the
    clientside. The client side will tranlate the message based callback to a
    call on the function registered here (which is cached for just this
    purpose
*/
static void ClientSideSetSendBlockEventFunction(
    NvMMBlockHandle hBlock,
    void *ClientContext,
    SendBlockEventFunction SendBlockEvent)
{
    NvError status;
    ClientSideInfo *pClientSide = ((NvMMBlock *)hBlock)->pContext;
    NvU8* msg;
    NvU32 size = 0;

    //
    // Tell blockside that a new callback was set on clientside.
    // the blockside only needs to know whether the callback is non-null (see note above).
    //
    // The blockside event callback function could be set up
    // when the blockside NvMMTransport wrapper is created. In this case
    // the following message does not need to be sent.
    NvOsMutexLock(pClientSide->TransportApiLock);

    //
    // Store the client's callback info in the clientside block meta data
    //
    pClientSide->pEventHandler = SendBlockEvent;
    pClientSide->clientContext = ClientContext;

    msg = pClientSide->sendMsgBuffer;
    *((NvU32 *)&msg[size]) = NVMM_MSG_SetSendBlockEventFunction;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = SendBlockEvent ? NV_TRUE : NV_FALSE;
    size += sizeof(NvBool);
    status = ClientSideSendMsg(pClientSide, msg, size);

    if (status != NvSuccess)
        NV_ASSERT(!"ClientSideSetSendBlockEventFunction Failed.\n");

    NvOsMutexUnlock(pClientSide->TransportApiLock);
 }

static NvError ClientSideSetState(NvMMBlockHandle hBlock, NvMMState State)
{
    NvError status = NvSuccess;
    ClientSideInfo *pClientSide = ((NvMMBlock *)hBlock)->pContext;
    NvU8* msg;
    NvU32 size = 0;

    NvOsMutexLock(pClientSide->TransportApiLock);
    /* shadow block's state on the client side */
    pClientSide->eShadowedBlockState = State;
    msg = pClientSide->sendMsgBuffer;
    *((NvU32 *)&msg[size]) = NVMM_MSG_SetState;
    size += sizeof(NvU32);
    *((NvMMState *)&msg[size]) = State;
    size += sizeof(NvMMState);
     *((NvBool *)&msg[size]) = NV_FALSE; // is format change request
    size += sizeof(NvBool);
    status = ClientSideSendMsg(pClientSide, msg, size);
    NvOsMutexUnlock(pClientSide->TransportApiLock);

    return status;
}

static NvError ClientSideGetState(NvMMBlockHandle hBlock, NvMMState *pState)
{
    NvError status = NvSuccess;

    ClientSideInfo *pClientSide = ((NvMMBlock *)hBlock)->pContext;

    /* populate return value with shadowed block state -
    there's no need to ask the block */
    *pState = pClientSide->eShadowedBlockState;

    return status;
}

static NvError ClientSideAbortBuffers(NvMMBlockHandle hBlock, NvU32 StreamIndex)
{
    NvError status = NvSuccess;
    ClientSideInfo*pClientSide = ((NvMMBlock *)hBlock)->pContext;
    NvU8* msg;
    NvU32 size = 0;
    NvOsMutexLock(pClientSide->TransportApiLock);
    msg = pClientSide->sendMsgBuffer;
    *((NvU32 *)&msg[size]) = NVMM_MSG_AbortBuffers;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = StreamIndex;
    size += sizeof(NvU32);
    *((NvBool *)&msg[size]) = NV_FALSE; // is format change request
    size += sizeof(NvBool);
    status = ClientSideSendMsg(pClientSide, msg, size);
    NvOsMutexUnlock(pClientSide->TransportApiLock);

    return status;
}

static NvError ClientSideSetAttribute(
    NvMMBlockHandle hBlock,
    NvU32 AttributeType,
    NvU32 SetAttrFlag,
    NvU32 AttributeSize,
    void *pAttribute)
{
    NvError status = NvSuccess;
    ClientSideInfo *pClientSide = ((NvMMBlock *)hBlock)->pContext;
    NvU8* msg;
    NvU32 size = 0;

    NvOsMutexLock(pClientSide->TransportApiLock);

#if NVMM_KPILOG_ENABLED
    /* Enable profiling if necessary */
    if (AttributeType == NvMMAttribute_Profile)
        ClientSideSetupProfile(pClientSide, pAttribute);
#endif

    msg = pClientSide->sendMsgBuffer;
    *((NvU32 *)&msg[size]) = NVMM_MSG_SetAttribute;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = AttributeType;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = SetAttrFlag;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = AttributeSize;
    size += sizeof(NvU32);
    if (AttributeSize && pAttribute)
    {
        NvOsMemcpy(&msg[size], pAttribute, AttributeSize);
        size += AttributeSize;
    } else
    {
        *((NvU32 *)&msg[size]) = 0;
        size += sizeof(NvU32);
    }
    *((NvBool *)&msg[size]) = NV_FALSE; // is format change request
    size += sizeof(NvBool);

    status = ClientSideSendMsg(pClientSide, msg, size);
    NvOsMutexUnlock(pClientSide->TransportApiLock);

    return status;
}

/*
 * The Attribute size is limited by the Size of the message that can be sent
 * across the RM transport. And so the max size of the attribute that can be
 *  extracted is 256-4-4-8 = 240 bytes.
 */

static NvError ClientSideGetAttribute(
    NvMMBlockHandle hBlock,
    NvU32 AttributeType,
    NvU32 AttributeSize,
    void *pAttribute)
{
    NvError status = NvSuccess;
    ClientSideInfo *pClientSide = ((NvMMBlock *)hBlock)->pContext;

    NvOsMutexLock(pClientSide->BlockInfoLock);

    if (pClientSide->params.eLocale == NvMMLocale_CPU)
    {
        status = pClientSide->hActualBlock->GetAttribute(
                            pClientSide->hActualBlock,
                            AttributeType,
                            AttributeSize,
                            pAttribute);
    }
    else
    {
        NvU32 size = 0;
        NvU8 msg[MSG_BUFFER_SIZE];

        *((NvU32 *)&msg[size]) = NVMM_MSG_GetAttribute;
        size += sizeof(NvU32);
        *((NvU32 *)&msg[size]) = AttributeType;
        size += sizeof(NvU32);
        *((NvU32 *)&msg[size]) = AttributeSize;
        size += sizeof(NvU32);
        if (AttributeSize && pAttribute)
        {
            NvOsMemcpy(&msg[size], pAttribute, AttributeSize);
            size += AttributeSize;
        }
        else
        {
            *((NvU32 *)&msg[size]) = 0;
            size += sizeof(NvU32);
        }

        pClientSide->pAttribute = pAttribute;
        if ((status = ClientSideSendMsg(pClientSide, msg, size)) == NvSuccess)
        {
            // Block the client until we get the Attribute back from the NvMMblock
            NvOsSemaphoreWait(pClientSide->BlockInfoSema);
            // Get return status from block side call
            status = pClientSide->AttributeStatus;
        }
    }

    NvOsMutexUnlock(pClientSide->BlockInfoLock);

    if (status != NvSuccess)
    {
        NV_LOGGER_PRINT((NVLOG_NVMM_TRANSPORT, NVLOG_ERROR, "NvMM%s [Locale = %s] GetAttribute = 0x%x Failed! NvError = 0x%08x",
            BlockTypeString(pClientSide->params.eType),
            (pClientSide->params.eLocale==NvMMLocale_CPU)?"CPU":(pClientSide->params.eLocale==NvMMLocale_AVP)?"AVP":"???",
           AttributeType, status));
    }
    else
    {
        NV_LOGGER_PRINT((NVLOG_NVMM_TRANSPORT, NVLOG_DEBUG, "NvMM%s [Locale = %s] GetAttribute = 0x%x Success",
            BlockTypeString(pClientSide->params.eType),
            (pClientSide->params.eLocale==NvMMLocale_CPU)?"CPU":(pClientSide->params.eLocale==NvMMLocale_AVP)?"AVP":"???",
           AttributeType));

    }

    return status;
}

static NvError ClientSideExtension(
    NvMMBlockHandle hBlock,
    NvU32 ExtensionIndex,
    NvU32 SizeInput,
    void *pInputData,
    NvU32 SizeOutput,
    void *pOutputData)
{
    NvError status = NvSuccess;
    ClientSideInfo *pClientSide = ((NvMMBlock *)hBlock)->pContext;
    NvU32 size = 0;
    NvU8 msg[MSG_BUFFER_SIZE];

    NvOsMutexLock(pClientSide->BlockInfoLock);
    *((NvU32 *)&msg[size]) = NVMM_MSG_Extension;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = ExtensionIndex;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = SizeInput;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = SizeOutput;
    size += sizeof(NvU32);
    if (SizeInput && pInputData)
    {
        NvOsMemcpy(&msg[size], pInputData, SizeInput);
        size += SizeInput;
    }
    else
    {
        *((NvU32 *)&msg[size]) = 0;
        size += sizeof(NvU32);
    }
    pClientSide->pAttribute = pOutputData;
    if ((status = ClientSideSendMsg(pClientSide, msg, size)) == NvSuccess)
    {
        NvOsSemaphoreWait(pClientSide->BlockInfoSema);
        // Get return status from block side call
        status = pClientSide->AttributeStatus;
    }
    NvOsMutexUnlock(pClientSide->BlockInfoLock);

    return status;
}

static NvError ClientSideTransferBufferToBlock(
    void *pContext,
    NvU32 StreamIndex,
    NvMMBufferType BufferType,
    NvU32 BufferSize,
    void *pBuffer)
{
    NvError status = NvSuccess;
    ClientSideInfo *pClientSide = ((NvMMBlock *)pContext)->pContext;
    NvU8* msg;
    NvU32 size = 0;

    if (BufferType == NvMMBufferType_StreamEvent)
    {
        NvU32 sEventType = (NvU32)(((NvMMStreamEventBase*)pBuffer)->event);
        if (sEventType == NvMMEvent_StreamShutdown)
        {
            NvU32 uResetTunnelIndexOut = 0;
            NvU32 i = 0;
            for(i = 0; i < NVMMBLOCK_MAXSTREAMS; i++)
            {
                ClientSideTunnelInfo *pTunnelInfo = &pClientSide->tunnelInfo[i];
                if((pTunnelInfo->uStreamIndexInForTransferBuffer == StreamIndex) &&
                   (pTunnelInfo->hBlockInForTransferBuffer       == pContext))
                {
                    uResetTunnelIndexOut = pTunnelInfo->uStreamIndexOutGeneratedForTransferBuffer;
                    break;
                }
            }
            pClientSide->tunnelInfo[uResetTunnelIndexOut].pTransferBufferToClient = NULL;
        }
    }

    NvOsMutexLock(pClientSide->TransportApiLock);

    msg = pClientSide->sendMsgBuffer;
    size = 0;

    *((NvU32 *)&msg[size]) = NVMM_MSG_TransferBufferToBlock;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = StreamIndex;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = BufferType;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = BufferSize;
    size += sizeof(NvU32);
    // this is to ensure the buffer size send should not be more than 256 bytes
    NV_ASSERT(BufferSize <= MSG_BUFFER_SIZE);
    if (BufferSize && pBuffer)
    {
        NvOsMemcpy(&msg[size], pBuffer,BufferSize);
        size += BufferSize;
    }
    else
    {
        *((NvU32 *)&msg[size]) = 0;
        size += sizeof(NvU32);
    }
    status = ClientSideSendMsg(pClientSide, msg, size);

    NvOsMutexUnlock(pClientSide->TransportApiLock);

    return status;
}

static void MapClientSideBuffer(NvMMBuffer *pBuffer)
{
    NvError status;

    if (pBuffer->PayloadType != NvMMPayloadType_MemHandle)
        return;

    if (pBuffer->Payload.Ref.pMemCpu == 0)
    {
        if (pBuffer->Payload.Ref.hMemCpu == 0)
        {
            status = NvRmMemHandleFromId(pBuffer->Payload.Ref.id,
                                         &pBuffer->Payload.Ref.hMemCpu);
            if (status != NvSuccess)
            {
                pBuffer->PayloadInfo.BufferFlags |= NvMMBufferFlag_MemMapError;
                return;
            }
        }

        status = NvRmMemMap(pBuffer->Payload.Ref.hMemCpu,
                            pBuffer->Payload.Ref.Offset,
                            pBuffer->Payload.Ref.sizeOfBufferInBytes,
                            NVOS_MEM_READ_WRITE,
                            &pBuffer->Payload.Ref.pMemCpu);
        if (status != NvSuccess)
        {
            pBuffer->PayloadInfo.BufferFlags |= NvMMBufferFlag_MemMapError;
            return;
        }
    }

    pBuffer->Payload.Ref.pMem = pBuffer->Payload.Ref.pMemCpu;
    pBuffer->Payload.Ref.hMem = pBuffer->Payload.Ref.hMemCpu;
}

static void ClientSideCallbackReceiveThread(void *arg)
{
    NvError status = NvSuccess;
    ClientSideInfo *pClientSide = (ClientSideInfo *)arg;
    NvU8* msg;
    NvU32 size;
    NvU32 message;
    NvU32 streamIndex;
    NvMMBufferType BufferType;
    NvU32 BufferSize;
    void* pBuffer;
    NvU32 EventType, EventInfoSize;
    void* pEventInfo;
    NvBool bRet;
    ClientSideTunnelInfo *pTunnelInfo = NULL;

    NvOsSemaphoreSignal(pClientSide->ClientReceiveThreadCreateSema);

    while (!pClientSide->bShutdown)
    {
        bRet = NV_FALSE;
        pBuffer = NULL;
        BufferType = 0;
        BufferSize = 0;
        EventType = 0;
        pEventInfo = NULL;

        msg = pClientSide->rcvMsgBuffer;
        status = ClientSideRcvMsg(pClientSide, msg);
        if (status == NvSuccess)
        {
            size = sizeof(NvU32);
            message = *((NvU32 *)&msg[0]);

            switch(message)
            {
                case NVMM_MSG_TransferBufferFromBlock:
                    pBuffer = pClientSide->workBuffer;
                    /* unmarshal parameters */
                    streamIndex = *((NvU32 *)&msg[size]);
                    size += sizeof(NvU32);
                    BufferType = *((NvU32 *)&msg[size]);
                    size += sizeof(NvU32);
                    BufferSize = *((NvU32 *)&msg[size]);
                    size += sizeof(NvU32);
                    if (BufferSize)
                    {
                        NvOsMemcpy(pBuffer, &msg[size], BufferSize);
                        size += BufferSize;
                    }

                    NV_ASSERT(streamIndex < NVMMBLOCK_MAXSTREAMS);

                    if (BufferType == NvMMBufferType_StreamEvent)
                    {
                        NvU32 sEventType = (NvU32)(((NvMMStreamEventBase*)pBuffer)->event);
                        if(sEventType ==  NvMMEvent_StreamEnd)
                        {
                            NV_LOGGER_PRINT((NVLOG_NVMM_TRANSPORT, NVLOG_INFO, "NvMM%s [Locale = %s] NvMMEvent_StreamEnd(EOS) Received",
                                BlockTypeString(pClientSide->params.eType),
                                (pClientSide->params.eLocale==NvMMLocale_CPU)?"CPU":(pClientSide->params.eLocale==NvMMLocale_AVP)?"AVP":"???"));
                        }
                     }

                    pTunnelInfo = &pClientSide->tunnelInfo[streamIndex];
                    if (pTunnelInfo->pTransferBufferToClient)
                    {
                        if (BufferType == NvMMBufferType_Payload)
                        {
                            NvMMBuffer *PayloadBuffer = (NvMMBuffer *)pBuffer;
                            if (!(PayloadBuffer->PayloadInfo.BufferFlags & NvMMBufferFlag_MemMapError))
                            {
                                MapClientSideBuffer(PayloadBuffer);
                            }
                        }

                        pTunnelInfo = &pClientSide->tunnelInfo[streamIndex];
                        status = pTunnelInfo->pTransferBufferToClient(
                                     pTunnelInfo->hBlockOutForTransferBuffer,
                                     pTunnelInfo->uStreamIndexOutForTransferBuffer,
                                     BufferType,
                                     BufferSize,
                                     pBuffer);
                    }
                    break;

                case NVMM_MSG_SendEvent:
                    /* unmarshal parameters */
                    EventType = *((NvU32 *)&msg[size]);
                    size += sizeof(NvU32);
                    EventInfoSize = *((NvU32 *)&msg[size]);
                    size += sizeof(NvU32);
                    if (EventInfoSize)
                    {
                        pEventInfo = pClientSide->workBuffer;
                        NvOsMemcpy(pEventInfo, &msg[size], EventInfoSize);
                        size+= EventInfoSize;
                    }

                    if (EventType == NvMMEvent_BlockError)
                    {
                        if (((NvMMBlockErrorInfo*)pEventInfo)->error != NvSuccess)
                        {
                            NV_LOGGER_PRINT((NVLOG_NVMM_TRANSPORT, NVLOG_ERROR, "NvMM%s [Locale = %s] Failed! NvError = 0x%08x",
                                BlockTypeString(pClientSide->params.eType),
                                (pClientSide->params.eLocale==NvMMLocale_CPU)?"CPU":(pClientSide->params.eLocale==NvMMLocale_AVP)?"AVP":"???",
                                ((NvMMBlockErrorInfo*)pEventInfo)->error));
                        }
                    }
                    else if (EventType == NvMMEvent_SetAttributeError)
                    {
                        NV_LOGGER_PRINT((NVLOG_NVMM_TRANSPORT, NVLOG_ERROR, "NvMM%s [Locale = %s] SetAttribute = 0x%x Failed! NvError = 0x%08x",
                            BlockTypeString(pClientSide->params.eType),
                            (pClientSide->params.eLocale==NvMMLocale_CPU)?"CPU":(pClientSide->params.eLocale==NvMMLocale_AVP)?"AVP":"???",
                           ((NvMMSetAttributeErrorInfo*)pEventInfo)->AttributeType, ((NvMMSetAttributeErrorInfo*)pEventInfo)->error));
                    }
                    else if (EventType == NvMMEvent_BlockBufferNegotiationComplete)
                    {
                        // This semaphore is used to synchronize block open between Client side-Block side and
                        // for buffer negotiation complete event.
                        NvOsSemaphoreSignal(pClientSide->hBlockSyncSema);
                        if(pClientSide->params.eType != NvMMBlockType_SrcUSBCamera)
                            break;
                    }
                    else if (EventType == NvMMEvent_SetAttributeComplete)
                    {
                        bRet = NvmmContextManagerNotify();
                        if (bRet)
                            break;
                    }

                    if (status == NvSuccess)
                    {
                        if (EventType == NvMMEvent_BlockClose)
                        {
                            NV_LOGGER_PRINT((NVLOG_NVMM_TRANSPORT, NVLOG_INFO, "NvMM%s [Locale = %s] Closed",
                                BlockTypeString(pClientSide->params.eType),
                                (pClientSide->params.eLocale==NvMMLocale_CPU)?"CPU":(pClientSide->params.eLocale==NvMMLocale_AVP)?"AVP":"???"));

                            pClientSide->bShutdown = NV_TRUE;
                            break;
                        }
                    }

                    if (pClientSide->pEventHandler) // client might not have set this
                    {
                        status = pClientSide->pEventHandler(pClientSide->clientContext,
                                                            EventType,
                                                            EventInfoSize,
                                                            pEventInfo);
                    }


                    break;

                case NVMM_MSG_GetAttributeInfo:
                    pClientSide->AttributeStatus = (NvError) *((NvU32 *)&msg[size]);
                    size += sizeof(NvU32);
                    if (pClientSide->pAttribute)
                    {
                        NvU32 AttributeSize;
                        AttributeSize = *((NvU32 *)&msg[size]);
                        size += sizeof(NvU32);
                        NvOsMemcpy(pClientSide->pAttribute, &msg[size], AttributeSize);
                        NvOsSemaphoreSignal(pClientSide->BlockInfoSema);
                    }
                    else
                    {
                        NvOsSemaphoreSignal(pClientSide->BlockInfoSema);
                    }
                    break;

                case NVMM_MSG_GetTransferBufferFunction:
                    break;

                case NVMM_MSG_WorkerThreadCreated:
                    {
                        pClientSide->BlockOpenStatus = *(NvU32*)&msg[size];
                        size += sizeof(NvU32);
                        if (pClientSide->BlockOpenStatus == NvSuccess)
                        {
                            if(pClientSide->params.eLocale == NvMMLocale_AVP)
                                pClientSide->params.blockSideWorkerHandle = *(NvOsThreadHandle*)&msg[size];
                            size += sizeof(NvOsThreadHandle);
                            NvOsMemcpy(&pClientSide->hActualBlock, &msg[size], sizeof(NvMMBlockHandle));
                            size += sizeof(NvMMBlockHandle);

                            NV_LOGGER_PRINT((NVLOG_NVMM_TRANSPORT, NVLOG_INFO, "NvMM%s [Locale = %s] Opened",
                                BlockTypeString(pClientSide->params.eType),
                                (pClientSide->params.eLocale==NvMMLocale_CPU)?"CPU":(pClientSide->params.eLocale==NvMMLocale_AVP)?"AVP":"???"));

                        }
                        else
                        {
                            NV_LOGGER_PRINT((NVLOG_NVMM_TRANSPORT, NVLOG_ERROR, "NvMM%s [Locale = %s] Open Failed! NvError = 0x%08x",
                                BlockTypeString(pClientSide->params.eType),
                                (pClientSide->params.eLocale==NvMMLocale_CPU)?"CPU":(pClientSide->params.eLocale==NvMMLocale_AVP)?"AVP":"???",
                                pClientSide->BlockOpenStatus));

                            pClientSide->bShutdown = NV_TRUE;
                        }
                        NvOsSemaphoreSignal(pClientSide->BlockWorkerThreadCreateSema);
                    }
                    break;

#if NVMM_KPILOG_ENABLED
                case NVMM_MSG_BlockKPI:
                    if (pClientSide->pNvMMStat &&
                       pClientSide->profileStatus & NvMMProfile_BlockTransport)
                    {
                        NvMMStatsStoreBlockKPI(pClientSide->pNvMMStat, (char*)msg);
                    }
                    break;
#endif
            }
        }
    }
}

#if NVMM_KPILOG_ENABLED
NvError ClientSideSetupProfile(ClientSideInfo *pClientSideInfo, void *pAttribute)
{
    NvMMAttrib_ProfileInfo *pProfileInfo = (NvMMAttrib_ProfileInfo *)pAttribute;
    NvError status = NvSuccess;

    pClientSideInfo->profileStatus = pProfileInfo->ProfileType;

    if (!pClientSideInfo->pNvMMStat &&
        pClientSideInfo->profileStatus & NvMMProfile_BlockTransport)
    {
        status = NvMMStatsCreate(&pClientSideInfo->pNvMMStat,
            pClientSideInfo->params.eType,
            NVMM_MAX_STAT_ENTRIES);
        if (status != NvSuccess)
            NV_ASSERT(!"ClientSideWorkerThread: NvMMStatsCreate failed\n");
        pClientSideInfo->params.pNvMMStat = pClientSideInfo->pNvMMStat;
    }

    if (!pClientSideInfo->pNvMMAVPProfile &&
        pClientSideInfo->profileStatus & NvMMProfile_AVPKernel)
    {
        status = NvMMAVPProfileCreate(&pClientSideInfo->pNvMMAVPProfile);
        if (status != NvSuccess)
            NV_ASSERT(!"ClientSideWorkerThread: NvMMAVPProfCreate failed\n");
    }

    return status;
}
#endif

NvError ClientSideSendMsg(ClientSideInfo *pClientSide, NvU8* msg, NvU32 size)
{
    NvError status;

    NV_LOGGER_PRINT((NVLOG_NVMM_TRANSPORT, NVLOG_DEBUG, "ClientSideSendMsg - %p from %s of %s\n", msg,
        pClientSide->params.blockName, DecipherMsgCode(msg)));

    status = NvRmTransportSendMsg(pClientSide->hRmTransport,
                                  msg,
                                  size,
                                  NV_WAIT_INFINITE);

    if (status != NvSuccess)
    {
        NV_ASSERT(!"Message dropped. NvMMQueue overflowed.\n");
        NV_LOGGER_PRINT((NVLOG_NVMM_TRANSPORT, NVLOG_ERROR, "ClientSideSendMsg - Message dropped. NvMMQueue overflowed"));
    }

    return status;
}

NvError ClientSideRcvMsg(ClientSideInfo *pClientSide, NvU8* msg)
{
    NvError status;
    NvU32 size = MSG_BUFFER_SIZE;

    NvOsSemaphoreWait(pClientSide->ClientEventSema);
    if(pClientSide->bShutdown)
    {
        return NvError_InvalidState;
    }

    status = NvRmTransportRecvMsg(
                pClientSide->hRmTransport,
                msg,
                MSG_BUFFER_SIZE,
                &size);

    NV_LOGGER_PRINT((NVLOG_NVMM_TRANSPORT, NVLOG_DEBUG, "ClientSideRecvMsg - %p from %s of %s\n", msg,
        pClientSide->params.blockName, DecipherMsgCode(msg)));

    return status;
}

NvMMBlockType NvMMGetBlockType(NvMMBlockHandle hBlock)
{
    return ((ClientSideInfo *)(hBlock->pContext))->params.eType;
}

