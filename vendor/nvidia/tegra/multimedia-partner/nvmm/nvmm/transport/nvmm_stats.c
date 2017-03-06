/* Copyright (c) 2006 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property 
 * and proprietary rights in and to this software, related documentation 
 * and any modifications thereto.  Any use, reproduction, disclosure or 
 * distribution of this software and related documentation without an 
 * express license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvmm_stats.h"
#include "nvos.h"
#include "nvmm_transport_private.h"
#include "nvrm_memmgr.h"
#include "nvmm_debug.h"

#if NVMM_KPILOG_ENABLED
/* none of this code is required unless the above flag is set to != 0 */

#undef MAX
#define MAX(a,b)    ((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a,b)    ((a) < (b) ? (a) : (b))

char *GetString(NvMMBufferType type);

char *GetBlockName(NvMMBlockType eType);

typedef struct TBFInfoRec {
    NvU64 TimeStamp;
    NvMMBufferType BufferType;
    NvU32 StreamIndex;
}TBFInfo;

typedef struct NvMMStatsRec
{
    NvU64 *pClientCallTBFToBlockList;
    TBFInfo *pClientSideSendTBFMsgList;
    TBFInfo *pBlockSideRcvTBFMsgList;
    NvU64 *pBlockSideCallTBFToBlockList;
    NvU64 *pBlockCallTBFFromBlockList;
    TBFInfo *pBlockSendTBFMsgList;
    TBFInfo *pClientSideRcvTBFMsgList;
    NvU64 *pClientSideCallTBFFromBlockList;

    NvU32 ClientCallTBFToBlockListIndex;   
    NvU32 ClientSideSendTBFMsgListIndex;   
    NvU32 BlockSideRcvTBFMsgListIndex;     
    NvU32 BlockSideCallTBFToBlockListIndex;
    NvU32 BlockCallTBFFromBlockListIndex;     
    NvU32 BlockSendTBFMsgListIndex;           
    NvU32 ClientSideRcvTBFMsgListIndex;       
    NvU32 ClientSideCallTBFFromBlockListIndex;

    NvU64 AverageLatency;
    NvU64 WorstCaseLatency;
    NvU64 BestCaseLatency;

    NvU32 MaxEntries;         
    NvOsFileHandle fPtr;      

    NvRmDeviceHandle hRmDevice;
    NvRmMemHandle hMemHandle;
    NvU32 BlockSideRcvTBFMsgListOffset;     
    NvU32 BlockSideCallTBFToBlockListOffset;
    NvU32 BlockCallTBFFromBlockListOffset;     
    NvU32 BlockSendTBFMsgListOffset;           

    NvU32 BlockSideRcvTBFMsgListSize;     
    NvU32 BlockSideCallTBFToBlockListSize;
    NvU32 BlockCallTBFFromBlockListSize;     
    NvU32 BlockSendTBFMsgListSize;           

} NvMMStats;

NvError NvMMStatsCreate(void **phNvMMStat, 
                        NvMMBlockType eType,
                        NvU32 uMaxEntries) 
{
    NvError status = NvSuccess;
    if (eType)  // client side
    {
#if !NV_IS_AVP
        NvMMStats *pNvMMStats = NULL;
        NvU32 Size = 0;
        NvU8 NvMMStatsFileName[256];

        if (phNvMMStat == NULL)
            return NvError_BadParameter;

        pNvMMStats  = NvOsAlloc(sizeof(NvMMStats));
        if (!pNvMMStats)
        {
            status = NvError_InsufficientMemory;
            goto nvmm_stats_exit;
        }
        NvOsMemset(pNvMMStats, 0, sizeof(NvMMStats));

        pNvMMStats->MaxEntries  = (uMaxEntries)?uMaxEntries:NVMM_MAX_STAT_ENTRIES; 

        Size = NvOsStrlen((const char *)GetBlockName(eType)) + 
               NvOsStrlen("TBFCallStatistics_") + 
               NvOsStrlen(".txt") + 1;
        NvOsSnprintf((char *)NvMMStatsFileName, 
                     Size, 
                     "TBFCallStatistics_%s.txt", 
                     GetBlockName(eType));
        status = NvOsFopen((const char *)NvMMStatsFileName, 
                           NVOS_OPEN_CREATE|NVOS_OPEN_WRITE, 
                           &pNvMMStats->fPtr);  
        if (NvSuccess !=  status || !pNvMMStats->fPtr)
        {
            status = NvError_FileOperationFailed;
            goto nvmm_stats_exit; 
        }
        /*
        NvOsFprintf(pNvMMStats->fPtr, "NvMM TBF latency\n\n");
        NvOsFprintf(pNvMMStats->fPtr, "Block Name: %s\n\n", pFileName);
        NvOsFprintf(pNvMMStats->fPtr, "1)   TBTBCount: TransferBufferFunction Count\n"); 
        NvOsFprintf(pNvMMStats->fPtr, "2)   TS_ClientCall: timestamp at the instant when the client calls NvMM transport's TransferBufferFunction\n");
        NvOsFprintf(pNvMMStats->fPtr, "3)   TS_MsgSent: timestamp at the instant when the client-side NvMM transport call NvRmTransportSendMessage\n");
        NvOsFprintf(pNvMMStats->fPtr, "4)   TS_MsgSent-TS_ClientCall: Client-side latency of TransferBufferFunction\n");
        NvOsFprintf(pNvMMStats->fPtr, "5)   TS_MsgRecv: timestamp at the instant when the block-side NvMM transport receives the message via NvRmTransportRecvMessage\n");
        NvOsFprintf(pNvMMStats->fPtr, "6)   TS_BlockCall: timestamp at the instant when the NvMM transport calls the block's TransferBufferFunction.\n");
        NvOsFprintf(pNvMMStats->fPtr, "7)   TS_BlockCall-TS_MsgRcv: Block-side latency of TransferBufferFunction\n");
        NvOsFprintf(pNvMMStats->fPtr, "8)   TS_BlockCall-TS_ClientCall: the total latency of a TransferBufferFunction\n");
        NvOsFprintf(pNvMMStats->fPtr, "9)   NvMM_TBTB_Latency: NvMM transport latency = (TSBlockCall - TS_MsgRev) + (TS_MsgSent - TS_ClientCall)\n\n");
        */
        pNvMMStats->pClientCallTBFToBlockList = NvOsAlloc( uMaxEntries * sizeof(NvU64));
        if (!pNvMMStats->pClientCallTBFToBlockList)
        {
            status = NvError_InsufficientMemory;
            goto nvmm_stats_exit; 
        }
        NvOsMemset(pNvMMStats->pClientCallTBFToBlockList, 0, uMaxEntries * sizeof(NvU64));
        pNvMMStats->ClientCallTBFToBlockListIndex = 0;

        pNvMMStats->pClientSideSendTBFMsgList = NvOsAlloc( uMaxEntries * sizeof(TBFInfo));
        if (!pNvMMStats->pClientSideSendTBFMsgList)
        {
            status = NvError_InsufficientMemory;
            goto nvmm_stats_exit; 
        }
        NvOsMemset(pNvMMStats->pClientSideSendTBFMsgList, 0, uMaxEntries * sizeof(TBFInfo));
        pNvMMStats->ClientSideSendTBFMsgListIndex = 0;

        pNvMMStats->pBlockSideRcvTBFMsgList = NvOsAlloc( uMaxEntries * sizeof(TBFInfo));
        if (!pNvMMStats->pBlockSideRcvTBFMsgList)
        {
            status = NvError_InsufficientMemory;
            goto nvmm_stats_exit; 
        }
        NvOsMemset(pNvMMStats->pBlockSideRcvTBFMsgList, 0, uMaxEntries * sizeof(TBFInfo));
        pNvMMStats->BlockSideRcvTBFMsgListIndex = 0;

        pNvMMStats->pBlockSideCallTBFToBlockList = NvOsAlloc( uMaxEntries * sizeof(NvU64));
        if (!pNvMMStats->pBlockSideCallTBFToBlockList)
        {
            status = NvError_InsufficientMemory;
            goto nvmm_stats_exit; 
        }
        NvOsMemset(pNvMMStats->pBlockSideCallTBFToBlockList, 0, uMaxEntries * sizeof(NvU64));
        pNvMMStats->BlockSideCallTBFToBlockListIndex = 0;

        pNvMMStats->pBlockCallTBFFromBlockList = NvOsAlloc( uMaxEntries * sizeof(NvU64));
        if (!pNvMMStats->pBlockCallTBFFromBlockList)
        {
            status = NvError_InsufficientMemory;
            goto nvmm_stats_exit; 
        }
        NvOsMemset(pNvMMStats->pBlockCallTBFFromBlockList, 0, uMaxEntries * sizeof(NvU64));
        pNvMMStats->BlockCallTBFFromBlockListIndex = 0;

        pNvMMStats->pBlockSendTBFMsgList = NvOsAlloc( uMaxEntries * sizeof(TBFInfo));
        if (!pNvMMStats->pBlockSendTBFMsgList)
        {
            status = NvError_InsufficientMemory;
            goto nvmm_stats_exit; 
        }
        NvOsMemset(pNvMMStats->pBlockSendTBFMsgList, 0, uMaxEntries * sizeof(TBFInfo));
        pNvMMStats->BlockSendTBFMsgListIndex = 0;

        pNvMMStats->pClientSideRcvTBFMsgList = NvOsAlloc( uMaxEntries * sizeof(TBFInfo));
        if (!pNvMMStats->pClientSideRcvTBFMsgList)
        {
            status = NvError_InsufficientMemory;
            goto nvmm_stats_exit; 
        }
        NvOsMemset(pNvMMStats->pClientSideRcvTBFMsgList, 0, uMaxEntries * sizeof(TBFInfo));
        pNvMMStats->ClientSideRcvTBFMsgListIndex = 0;

        pNvMMStats->pClientSideCallTBFFromBlockList = NvOsAlloc( uMaxEntries * sizeof(NvU64));
        if (!pNvMMStats->pClientSideCallTBFFromBlockList)
        {
            status = NvError_InsufficientMemory;
            goto nvmm_stats_exit; 
        }
        NvOsMemset(pNvMMStats->pClientSideCallTBFFromBlockList, 0, uMaxEntries * sizeof(NvU64));
        pNvMMStats->ClientSideCallTBFFromBlockListIndex = 0;

        *phNvMMStat = (void *)pNvMMStats;
        return status;
    
nvmm_stats_exit:
        if (pNvMMStats)
        {
            NvOsFree(pNvMMStats->pClientCallTBFToBlockList);
            NvOsFree(pNvMMStats->pClientSideSendTBFMsgList);
            NvOsFree(pNvMMStats->pBlockSideRcvTBFMsgList);
            NvOsFree(pNvMMStats->pBlockSideCallTBFToBlockList);
            NvOsFree(pNvMMStats->pBlockCallTBFFromBlockList);
            NvOsFree(pNvMMStats->pBlockSendTBFMsgList);
            NvOsFree(pNvMMStats->pClientSideRcvTBFMsgList);
            NvOsFree(pNvMMStats->pClientSideCallTBFFromBlockList);
            NvOsFclose(pNvMMStats->fPtr);
        }
        NvOsFree(pNvMMStats);
#endif
        return status;
    }
    else  //  block-side
    {
        NvMMStats *pNvMMStats = NULL;
        void *temp;

        if (phNvMMStat == NULL)
            return NvError_BadParameter;

        pNvMMStats  = NvOsAlloc(sizeof(NvMMStats));
        if (!pNvMMStats)
            return NvError_InsufficientMemory; 
        NvOsMemset(pNvMMStats, 0, sizeof(NvMMStats));

        pNvMMStats->MaxEntries  = (uMaxEntries)?uMaxEntries:NVMM_MAX_STAT_ENTRIES; 
        status = NvRmOpen(&pNvMMStats->hRmDevice, 0);
        if (status != NvSuccess) goto nvmm_stats_avp_exit;

        status = NvRmMemHandleCreate(pNvMMStats->hRmDevice, 
                                     &pNvMMStats->hMemHandle, 
                                     ((sizeof(NvU64) * pNvMMStats->MaxEntries * 2) + (2 * sizeof(TBFInfo) * pNvMMStats->MaxEntries)));
        if (status != NvSuccess) goto nvmm_stats_avp_exit;

        status = NvRmMemAlloc(pNvMMStats->hMemHandle,
                              NULL,
                              0,
                              32, 
                              NvOsMemAttribute_Uncached);
        if (status != NvSuccess) goto nvmm_stats_avp_exit;

        NvRmMemPin(pNvMMStats->hMemHandle);
        pNvMMStats->BlockSideRcvTBFMsgListOffset = 0;
        pNvMMStats->BlockSideRcvTBFMsgListSize = pNvMMStats->MaxEntries * sizeof(TBFInfo);
        status = NvRmMemMap(pNvMMStats->hMemHandle,
                            pNvMMStats->BlockSideRcvTBFMsgListOffset,
                            pNvMMStats->BlockSideRcvTBFMsgListSize,
                            NVOS_MEM_READ_WRITE,
                            &temp);
        if (status != NvSuccess) goto nvmm_stats_avp_exit;
        pNvMMStats->pBlockSideRcvTBFMsgList = temp;
        pNvMMStats->BlockSideRcvTBFMsgListIndex = 0;

        pNvMMStats->BlockSideCallTBFToBlockListOffset = 
            (pNvMMStats->BlockSideRcvTBFMsgListOffset + pNvMMStats->BlockSideRcvTBFMsgListSize);
        pNvMMStats->BlockSideCallTBFToBlockListSize = (sizeof(NvU64) * pNvMMStats->MaxEntries);
        status = NvRmMemMap(pNvMMStats->hMemHandle,
                            pNvMMStats->BlockSideCallTBFToBlockListOffset,
                            pNvMMStats->BlockSideCallTBFToBlockListSize,
                            NVOS_MEM_READ_WRITE,
                            &temp);
        if (status != NvSuccess) goto nvmm_stats_avp_exit;
        pNvMMStats->pBlockSideCallTBFToBlockList = temp;
        pNvMMStats->BlockSideCallTBFToBlockListIndex = 0;

        pNvMMStats->BlockCallTBFFromBlockListOffset = 
            (pNvMMStats->BlockSideCallTBFToBlockListOffset + pNvMMStats->BlockSideCallTBFToBlockListSize);
        pNvMMStats->BlockCallTBFFromBlockListSize = pNvMMStats->MaxEntries * sizeof(NvU64);
        status = NvRmMemMap(pNvMMStats->hMemHandle,
                            pNvMMStats->BlockCallTBFFromBlockListOffset,
                            pNvMMStats->BlockCallTBFFromBlockListSize,
                            NVOS_MEM_READ_WRITE,
                            &temp);
        if (status != NvSuccess) goto nvmm_stats_avp_exit;
        pNvMMStats->pBlockCallTBFFromBlockList = temp;
        pNvMMStats->BlockCallTBFFromBlockListIndex = 0;

        pNvMMStats->BlockSendTBFMsgListOffset = 
            (pNvMMStats->BlockCallTBFFromBlockListOffset + pNvMMStats->BlockCallTBFFromBlockListSize);
        pNvMMStats->BlockSendTBFMsgListSize = pNvMMStats->MaxEntries * sizeof(TBFInfo);
        status = NvRmMemMap(pNvMMStats->hMemHandle,
                            pNvMMStats->BlockSendTBFMsgListOffset,
                            pNvMMStats->BlockSendTBFMsgListSize,
                            NVOS_MEM_READ_WRITE,
                            &temp);
        if (status != NvSuccess) goto nvmm_stats_avp_exit;
        pNvMMStats->pBlockSendTBFMsgList = temp;
        pNvMMStats->BlockSendTBFMsgListIndex = 0;

        *phNvMMStat = (void *)pNvMMStats;
        return status;

nvmm_stats_avp_exit:
        if (pNvMMStats)
        {
            NvRmMemUnmap(pNvMMStats->hMemHandle, pNvMMStats->pBlockSideRcvTBFMsgList, pNvMMStats->BlockSideRcvTBFMsgListSize);
            NvRmMemUnmap(pNvMMStats->hMemHandle, pNvMMStats->pBlockSideCallTBFToBlockList, pNvMMStats->BlockSideCallTBFToBlockListSize);
            NvRmMemUnmap(pNvMMStats->hMemHandle, pNvMMStats->pBlockCallTBFFromBlockList, pNvMMStats->BlockCallTBFFromBlockListSize);
            NvRmMemUnmap(pNvMMStats->hMemHandle, pNvMMStats->pBlockSendTBFMsgList, pNvMMStats->BlockCallTBFFromBlockListSize);
            NvRmMemUnpin(pNvMMStats->hMemHandle);
            NvRmMemHandleFree(pNvMMStats->hMemHandle);
            NvRmClose(pNvMMStats->hRmDevice);
        }
        NvOsFree(pNvMMStats);
        return status;
    }
}

void NvMMStatsDestroy(void *pStats) 
{
    NvMMStats *pNvMMStats = (NvMMStats *)pStats;
    if (pStats == NULL)
        return;

    if (pNvMMStats->fPtr)
    {
#if !NV_IS_AVP
        NvOsFclose(pNvMMStats->fPtr);
        NvOsFree(pNvMMStats->pClientCallTBFToBlockList);
        NvOsFree(pNvMMStats->pClientSideSendTBFMsgList);
        NvOsFree(pNvMMStats->pBlockSideRcvTBFMsgList);
        NvOsFree(pNvMMStats->pBlockSideCallTBFToBlockList);
        NvOsFree(pNvMMStats->pBlockCallTBFFromBlockList);
        NvOsFree(pNvMMStats->pBlockSendTBFMsgList);
        NvOsFree(pNvMMStats->pClientSideRcvTBFMsgList);
        NvOsFree(pNvMMStats->pClientSideCallTBFFromBlockList);
#endif
    }
    else
    {
        NvRmMemUnmap(pNvMMStats->hMemHandle, 
                     pNvMMStats->pBlockSideRcvTBFMsgList,
                     pNvMMStats->BlockSideRcvTBFMsgListSize);
        NvRmMemUnmap(pNvMMStats->hMemHandle, 
                     pNvMMStats->pBlockSideCallTBFToBlockList,
                     pNvMMStats->BlockSideCallTBFToBlockListSize);
        NvRmMemUnmap(pNvMMStats->hMemHandle, 
                     pNvMMStats->pBlockCallTBFFromBlockList,
                     pNvMMStats->BlockCallTBFFromBlockListSize);
        NvRmMemUnmap(pNvMMStats->hMemHandle, 
                     pNvMMStats->pBlockSendTBFMsgList,
                     pNvMMStats->BlockSendTBFMsgListSize);
        NvRmMemUnpin(pNvMMStats->hMemHandle);
        NvRmMemHandleFree(pNvMMStats->hMemHandle);
        NvRmClose(pNvMMStats->hRmDevice);
    }
    NvOsFree(pNvMMStats);
}

NvError 
NvMMStats_TimeStamp(void *pStats, 
                    NvU64 TimeStamp, 
                    NvMMBufferType BufferType,
                    NvU32  StreamIndex,
                    NvMMStat_TimeStampType Type)
{   
    NvError status  = NvSuccess;
    NvMMStats *pNvMMStats = (NvMMStats *)pStats;

    if (pStats == NULL)
        return NvError_BadParameter;

    switch (Type)
    {
    case CLIENT_TBF_TO_BLOCK:
        if (!pNvMMStats->pClientCallTBFToBlockList ||
            pNvMMStats->ClientCallTBFToBlockListIndex >= pNvMMStats->MaxEntries)
            return NvError_InsufficientMemory;
        pNvMMStats->pClientCallTBFToBlockList[pNvMMStats->ClientCallTBFToBlockListIndex] = TimeStamp;
        pNvMMStats->ClientCallTBFToBlockListIndex++;
        break;
    case CLIENTSIDE_SEND_TBF_MSG:
        if (!pNvMMStats->pClientSideSendTBFMsgList ||
            pNvMMStats->ClientSideSendTBFMsgListIndex >= pNvMMStats->MaxEntries)
            return NvError_InsufficientMemory;
        pNvMMStats->pClientSideSendTBFMsgList[pNvMMStats->ClientSideSendTBFMsgListIndex].TimeStamp = TimeStamp;
        pNvMMStats->pClientSideSendTBFMsgList[pNvMMStats->ClientSideSendTBFMsgListIndex].BufferType = BufferType;
        pNvMMStats->pClientSideSendTBFMsgList[pNvMMStats->ClientSideSendTBFMsgListIndex].StreamIndex = StreamIndex;
        pNvMMStats->ClientSideSendTBFMsgListIndex++;
        break;
    case BLOCKSIDE_RCV_TBF_MSG:
        if (!pNvMMStats->pBlockSideRcvTBFMsgList ||
            pNvMMStats->BlockSideRcvTBFMsgListIndex >= pNvMMStats->MaxEntries)
            return NvError_InsufficientMemory;
        pNvMMStats->pBlockSideRcvTBFMsgList[pNvMMStats->BlockSideRcvTBFMsgListIndex].TimeStamp = TimeStamp;
        pNvMMStats->pBlockSideRcvTBFMsgList[pNvMMStats->BlockSideRcvTBFMsgListIndex].BufferType = BufferType;
        pNvMMStats->pBlockSideRcvTBFMsgList[pNvMMStats->BlockSideRcvTBFMsgListIndex].StreamIndex = StreamIndex;
        pNvMMStats->BlockSideRcvTBFMsgListIndex++;
        break;
    case BLOCKSIDE_CALL_TBF_TO_BLOCK:
        if (!pNvMMStats->pBlockSideCallTBFToBlockList ||
            pNvMMStats->BlockSideCallTBFToBlockListIndex >= pNvMMStats->MaxEntries)
            return NvError_InsufficientMemory;
        pNvMMStats->pBlockSideCallTBFToBlockList[pNvMMStats->BlockSideCallTBFToBlockListIndex] = TimeStamp;
        pNvMMStats->BlockSideCallTBFToBlockListIndex++;
        break;
    case BLOCK_TBF_FROM_BLOCK:
        if (!pNvMMStats->pBlockCallTBFFromBlockList ||
            pNvMMStats->BlockCallTBFFromBlockListIndex >= pNvMMStats->MaxEntries)
            return NvError_InsufficientMemory;
        pNvMMStats->pBlockCallTBFFromBlockList[pNvMMStats->BlockCallTBFFromBlockListIndex] = TimeStamp;
        pNvMMStats->BlockCallTBFFromBlockListIndex++;
        break;
    case BLOCKSIDE_SEND_TBF_MSG:
        if (!pNvMMStats->pBlockSendTBFMsgList ||
            pNvMMStats->BlockSendTBFMsgListIndex >= pNvMMStats->MaxEntries)
            return NvError_InsufficientMemory;
        pNvMMStats->pBlockSendTBFMsgList[pNvMMStats->BlockSendTBFMsgListIndex].TimeStamp = TimeStamp;
        pNvMMStats->pBlockSendTBFMsgList[pNvMMStats->BlockSendTBFMsgListIndex].BufferType = BufferType;
        pNvMMStats->pBlockSendTBFMsgList[pNvMMStats->BlockSendTBFMsgListIndex].StreamIndex = StreamIndex;
        pNvMMStats->BlockSendTBFMsgListIndex++;
        break;
    case CLIENTSIDE_RCV_TBF_MSG:
        if (!pNvMMStats->pClientSideRcvTBFMsgList ||
            pNvMMStats->ClientSideRcvTBFMsgListIndex >= pNvMMStats->MaxEntries)
            return NvError_InsufficientMemory;
        pNvMMStats->pClientSideRcvTBFMsgList[pNvMMStats->ClientSideRcvTBFMsgListIndex].TimeStamp = TimeStamp;
        pNvMMStats->pClientSideRcvTBFMsgList[pNvMMStats->ClientSideRcvTBFMsgListIndex].BufferType = BufferType;
        pNvMMStats->pClientSideRcvTBFMsgList[pNvMMStats->ClientSideRcvTBFMsgListIndex].StreamIndex = StreamIndex;
        pNvMMStats->ClientSideRcvTBFMsgListIndex++;
        break;
    case CLIENTSIDE_CALL_TBF_FROM_BLOCK:
        if (!pNvMMStats->pClientSideCallTBFFromBlockList ||
            pNvMMStats->ClientSideCallTBFFromBlockListIndex >= pNvMMStats->MaxEntries)
            return NvError_InsufficientMemory;
        pNvMMStats->pClientSideCallTBFFromBlockList[pNvMMStats->ClientSideCallTBFFromBlockListIndex] = TimeStamp;
        pNvMMStats->ClientSideCallTBFFromBlockListIndex++;
        break;
    }
    return status; 
}

NvError NvMMStats_ComputeStatistics(void *pStats)
{
    NvError status  = NvSuccess;
#if !NV_IS_AVP
    NvMMStats *pNvMMStats = (NvMMStats *)pStats;
    NvU64 startTime = 0, displayTime = 0, rcvdTime = 0, sentTime = 0;
    NvU32 nFullInputBuffer;
    NvU32 nEmptyInputBuffer;
    NvU32 nFullOutputBuffer;
    NvU32 nEmptyOutputBuffer;
    NvU32 rcvdIndex;    
    NvU32 sentIndex;    
    NvBool bDone = NV_FALSE;
    NvU32 i = 0;
    char *str;

    NV_DEBUG_PRINTF(("In NvMMStats_ComputeStatistics \n"));

    if (pStats == NULL || !pNvMMStats->fPtr)
        return NvError_BadParameter;

    
    NvOsFprintf(pNvMMStats->fPtr, "\nBlockSide:\n\n");
    NvOsFprintf(pNvMMStats->fPtr,"Count\t\tTime(ms)\t\tInputStream\t\tOutputStream\n");
    NvOsFprintf(pNvMMStats->fPtr,"     \t\t        \t\t  Up:Down  \t\t  Up:Down   \n");
    NvOsFprintf(pNvMMStats->fPtr,"\n\n");

    startTime = pNvMMStats->pBlockSideRcvTBFMsgList[0].TimeStamp;
    nFullInputBuffer = 0;
    nEmptyInputBuffer = 0;
    nFullOutputBuffer = 0;
    nEmptyOutputBuffer = 0;
    rcvdIndex = 0;
    sentIndex = 0;

    str = "BBBBBBBBBB";

    while (!bDone)
    {
        rcvdTime = pNvMMStats->pBlockSideRcvTBFMsgList[rcvdIndex].TimeStamp - startTime;
        sentTime = pNvMMStats->pBlockSendTBFMsgList[sentIndex].TimeStamp - startTime;
        //if (pNvMMStats->pBlockSideRcvTBFMsgList[rcvdIndex].TimeStamp < pNvMMStats->pBlockSendTBFMsgList[sentIndex].TimeStamp)
        if (rcvdTime < sentTime)
        {
            if (pNvMMStats->pBlockSideRcvTBFMsgList[rcvdIndex].StreamIndex == 0)
            {
                nFullInputBuffer++;
                if (nEmptyInputBuffer) nEmptyInputBuffer--;
            }
            else if (pNvMMStats->pBlockSideRcvTBFMsgList[rcvdIndex].StreamIndex == 1)
            {
                nEmptyOutputBuffer++;
                if (nFullOutputBuffer) nFullOutputBuffer--;
            }
            displayTime = pNvMMStats->pBlockSideRcvTBFMsgList[rcvdIndex].TimeStamp - startTime;
            rcvdIndex++;
        }
        else
        {
            if (pNvMMStats->pBlockSendTBFMsgList[sentIndex].StreamIndex == 1)
            {
                nEmptyInputBuffer++;
                if (nFullInputBuffer) nFullInputBuffer--;
            }
            else if (pNvMMStats->pBlockSendTBFMsgList[sentIndex].StreamIndex == 0)
            {
                nFullOutputBuffer++;
                if (nEmptyOutputBuffer) nEmptyOutputBuffer--;
            }
            displayTime = pNvMMStats->pBlockSendTBFMsgList[sentIndex].TimeStamp - startTime;
            sentIndex++;
        }
        NvOsFprintf(pNvMMStats->fPtr, "%d\t\t%f\t\t\t%-.*s:%.*s\t\t%-.*s:%.*s\n",
                    i++,
                    displayTime/1000.0,
                    nEmptyInputBuffer,str,
                    nFullInputBuffer, str,
                    nEmptyOutputBuffer,str,
                    nFullOutputBuffer,str);
        /*
        NvOsFprintf(pNvMMStats->fPtr, "%d\t%f\t%d\t%d\t%d\t%d\n",
                    i++,
                    displayTime/1000.0,
                    nFullInputBuffer,
                    nEmptyInputBuffer,
                    nEmptyOutputBuffer,
                    nFullOutputBuffer); */

        if ((rcvdIndex > pNvMMStats->BlockSideRcvTBFMsgListIndex) || (sentIndex > pNvMMStats->BlockSendTBFMsgListIndex))
        {
            bDone = NV_TRUE;
        }
    }
#endif
    return status;

#if 0
    NvError status  = NvSuccess;
#if !NV_IS_AVP
    NvMMStats *pNvMMStats = (NvMMStats *)pStats;
    NvU32 i = 0;
    NvU32 TotalEntries = 0;
    NvU64 divisor = 1000000;
    NV_DEBUG_PRINTF(("In NvMMStats_ComputeStatistics \n"));

    if (pStats == NULL || !pNvMMStats->fPtr)
        return NvError_BadParameter;

    TotalEntries = MIN(pNvMMStats->ClientSideSendTBFMsgListIndex,
                       pNvMMStats->BlockSideRcvTBFMsgListIndex); 
    if (TotalEntries)
    {
        NvOsFprintf(pNvMMStats->fPtr, "\nClient To Block:\n\n");
        NvOsFprintf(pNvMMStats->fPtr,"Count\tBufferType\t\tSendTBFMsg\t\tRcvTBFMsg\n");
        NvOsFprintf(pNvMMStats->fPtr,"\n\n");
        for (i = 0; i < TotalEntries; i++)
        {
            NvOsFprintf(pNvMMStats->fPtr,"%d\t%10s\t\t%d.%06d\t\t%d.%06d\n",
                        i,
                        GetString(pNvMMStats->pClientSideSendTBFMsgList[i].BufferType),
                       (NvU32)(pNvMMStats->pClientSideSendTBFMsgList[i].TimeStamp/divisor),(NvU32)(pNvMMStats->pClientSideSendTBFMsgList[i].TimeStamp % divisor),
                       (NvU32)(pNvMMStats->pBlockSideRcvTBFMsgList[i].TimeStamp/divisor),(NvU32)(pNvMMStats->pBlockSideRcvTBFMsgList[i].TimeStamp%divisor)); 

        }
    } else
    {
        NvOsFprintf(pNvMMStats->fPtr,"No Entries\n");
    }

    TotalEntries = MIN(pNvMMStats->BlockSendTBFMsgListIndex,
                       pNvMMStats->ClientSideRcvTBFMsgListIndex); 
    if (TotalEntries)
    {
        NvOsFprintf(pNvMMStats->fPtr, "\nBlock To Client:\n");
        NvOsFprintf(pNvMMStats->fPtr,"Count\tBufferType\t\tSendTBFMsg\t\tRcvTBFMsg\n");
        NvOsFprintf(pNvMMStats->fPtr,"\n\n");
        for (i = 0; i < TotalEntries; i++)
        {
            NvOsFprintf(pNvMMStats->fPtr,"%d\t%10s\t\t%d.%06d\t\t%d.%06d\n",
                        i,
                        GetString(pNvMMStats->pBlockSendTBFMsgList[i].BufferType),
                       (NvU32)(pNvMMStats->pBlockSendTBFMsgList[i].TimeStamp/divisor),(NvU32)(pNvMMStats->pBlockSendTBFMsgList[i].TimeStamp % divisor),
                       (NvU32)(pNvMMStats->pClientSideRcvTBFMsgList[i].TimeStamp/divisor),(NvU32)(pNvMMStats->pClientSideRcvTBFMsgList[i].TimeStamp%divisor)); 

        }
    } else
    {
        NvOsFprintf(pNvMMStats->fPtr,"No Entries\n");
    }
#endif
    return status;
#endif
}

NvError
NvMMStatsSendBlockKPI(void *pStats,
                       NvRmTransportHandle hRmTransport)
{
    NvError status = NvSuccess;
    NvU32 size = 0;
    NvMMStats *pNvMMStats = (NvMMStats *)pStats;
    NvU8 *msg = NvOsAlloc(256);
    if (!msg)
        return NvError_InsufficientMemory;

    /* marshal parameters */
    *((NvU32 *)&msg[size]) = NVMM_MSG_BlockKPI;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = pNvMMStats->BlockSideRcvTBFMsgListIndex;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = pNvMMStats->BlockSideCallTBFToBlockListIndex;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = pNvMMStats->BlockCallTBFFromBlockListIndex;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = pNvMMStats->BlockSendTBFMsgListIndex;
    size += sizeof(NvU32);
    *((NvRmMemHandle *)&msg[size]) = pNvMMStats->hMemHandle;
    size += sizeof(NvRmMemHandle);
    *((NvU32 *)&msg[size]) = (NvU32)pNvMMStats->BlockSideRcvTBFMsgListOffset;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = (NvU32)pNvMMStats->BlockSideCallTBFToBlockListOffset;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = (NvU32)pNvMMStats->BlockCallTBFFromBlockListOffset;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = (NvU32)pNvMMStats->BlockSendTBFMsgListOffset;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = (NvU32)pNvMMStats->BlockSideRcvTBFMsgListSize;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = (NvU32)pNvMMStats->BlockSideCallTBFToBlockListSize;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = (NvU32)pNvMMStats->BlockCallTBFFromBlockListSize;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = (NvU32)pNvMMStats->BlockSendTBFMsgListSize;
    size += sizeof(NvU32);

    status = NvRmTransportSendMsg(hRmTransport,
                                  (void *)msg,
                                  size,
                                  NV_WAIT_INFINITE);
    NvOsFree(msg);
    return status;
}

NvError
NvMMStatsStoreBlockKPI(void *pStats, char *msg) 
{
    NvU32 size = 0;
    NvError status = NvSuccess;
    NvRmMemHandle hMemHandle;
    void *temp;
    NvU32 BlockSideRcvTBFMsgListOffset;     
    NvU32 BlockSideCallTBFToBlockListOffset;
    NvU32 BlockCallTBFFromBlockListOffset;     
    NvU32 BlockSendTBFMsgListOffset;           

    NvU32 BlockSideRcvTBFMsgListSize;     
    NvU32 BlockSideCallTBFToBlockListSize;
    NvU32 BlockCallTBFFromBlockListSize;     
    NvU32 BlockSendTBFMsgListSize;       

    NvMMStats *pNvMMStats = (NvMMStats *)pStats;

    /* unmarshal parameters */    
    size += sizeof(NvU32);
    pNvMMStats->BlockSideRcvTBFMsgListIndex = *((NvU32 *)&msg[size]);
    size += sizeof(NvU32);
    pNvMMStats->BlockSideCallTBFToBlockListIndex = *((NvU32 *)&msg[size]);
    size += sizeof(NvU32);
    pNvMMStats->BlockCallTBFFromBlockListIndex = *((NvU32 *)&msg[size]);
    size += sizeof(NvU32);
    pNvMMStats->BlockSendTBFMsgListIndex = *((NvU32 *)&msg[size]);
    size += sizeof(NvU32);

    hMemHandle = *((NvRmMemHandle *)&msg[size]);
    size += sizeof(NvRmMemHandle);

    BlockSideRcvTBFMsgListOffset = *((NvU32 *)&msg[size]);
    size += sizeof(NvU32);
    BlockSideCallTBFToBlockListOffset = *((NvU32 *)&msg[size]);
    size += sizeof(NvU32);
    BlockCallTBFFromBlockListOffset = *((NvU32 *)&msg[size]);
    size += sizeof(NvU32);
    BlockSendTBFMsgListOffset = *((NvU32 *)&msg[size]);
    size += sizeof(NvU32);

    BlockSideRcvTBFMsgListSize = *((NvU32 *)&msg[size]);
    size += sizeof(NvU32);
    BlockSideCallTBFToBlockListSize = *((NvU32 *)&msg[size]);
    size += sizeof(NvU32);
    BlockCallTBFFromBlockListSize = *((NvU32 *)&msg[size]);
    size += sizeof(NvU32);
    BlockSendTBFMsgListSize = *((NvU32 *)&msg[size]);
    size += sizeof(NvU32);

    status = NvRmMemMap(hMemHandle, 
                        BlockSideRcvTBFMsgListOffset,
                        BlockSideRcvTBFMsgListSize,
                        NVOS_MEM_READ_WRITE,
                        &temp);
    if (status != NvSuccess)
        return status;
    NvOsMemcpy(pNvMMStats->pBlockSideRcvTBFMsgList, 
               temp, 
               sizeof(TBFInfo)*pNvMMStats->BlockSideRcvTBFMsgListIndex);
    NvRmMemUnmap(hMemHandle, temp, BlockSideRcvTBFMsgListSize);

    status = NvRmMemMap(hMemHandle, 
                        BlockSideCallTBFToBlockListOffset,
                        BlockSideCallTBFToBlockListSize,
                        NVOS_MEM_READ_WRITE,
                        &temp);
    if (status != NvSuccess)
        return status;
    NvOsMemcpy(pNvMMStats->pBlockSideCallTBFToBlockList, 
               temp, 
               sizeof(NvU64)*pNvMMStats->BlockSideCallTBFToBlockListIndex);
    NvRmMemUnmap(hMemHandle, temp, BlockSideCallTBFToBlockListSize);

    status = NvRmMemMap(hMemHandle, 
                        BlockCallTBFFromBlockListOffset,
                        BlockCallTBFFromBlockListSize,
                        NVOS_MEM_READ_WRITE,
                        &temp);
    if (status != NvSuccess)
        return status;
    NvOsMemcpy(pNvMMStats->pBlockCallTBFFromBlockList, 
               temp, 
               sizeof(NvU64)*pNvMMStats->BlockCallTBFFromBlockListIndex);
    NvRmMemUnmap(hMemHandle, temp, BlockCallTBFFromBlockListSize);

    status = NvRmMemMap(hMemHandle, 
                        BlockSendTBFMsgListOffset,
                        BlockSendTBFMsgListSize,
                        NVOS_MEM_READ_WRITE,
                        &temp);
    if (status != NvSuccess)
        return status;
    NvOsMemcpy(pNvMMStats->pBlockSendTBFMsgList, 
               temp, 
               sizeof(TBFInfo)*pNvMMStats->BlockSendTBFMsgListIndex);
    NvRmMemUnmap(hMemHandle, temp, BlockSendTBFMsgListSize);


    return status;
}

char *GetString(NvMMBufferType type) 
{
    char *str;
    switch (type)
    {
    case NvMMBufferType_Payload:
         str = "Payload";
        break;   
    case NvMMBufferType_Custom:
        str = "Custom";
        break;   
    case NvMMBufferType_StreamEvent:
        str = "Event";
        break;   
    default: 
        str = "Unknown";
        break;
    }
    return str;
}

char *GetBlockName(NvMMBlockType eType) 
{
    char *str;
    switch (eType)
    {
    case NvMMBlockType_UnKnown:
        str = "UnKnown";
        break;
    case NvMMBlockType_EncJPEG:
        str = "EncJPEG";
        break;
    case NvMMBlockType_DecJPEG:
        str = "DecJPEG";
        break;
    case NvMMBlockType_DecH263:
        str = "DecH263";
        break;
    case NvMMBlockType_DecMPEG4:
    case NvMMBlockType_DecMPEG4AVP:
        str = "DecMPEG4";
        break;
    case NvMMBlockType_DecH264:
    case NvMMBlockType_DecH264AVP:
        str = "DecH264";
        break;
    case NvMMBlockType_DecMPEG2:
        str = "DecMPEG2";
        break;
    case NvMMBlockType_DecSuperJPEG:
        str = "DecSuperJPEG";
        break;
    case NvMMBlockType_DecJPEGProgressive:
        str = "DecJPEGProgressive";
        break;
    case NvMMBlockType_DecVC1:
        str = "DecVC1";
        break;
    case NvMMBlockType_DecMPEG2VideoVld:
        str = "DecMPEG2VideoVld";
        break;
    case NvMMBlockType_DecMPEG2VideoRecon:
        str = "DecMPEG2VideoRecon";
        break;
    case NvMMBlockType_DecJPEG2000:
        str = "DecJPEG2000";
        break;
    case NvMMBlockType_EncAMRNB:
        str = "EncAMRNB";
        break;
    case NvMMBlockType_EnciLBC:
        str = "EnciLBC";
        break;
    case NvMMBlockType_EncAMRWB:
        str = "EncAMRWB";
        break;
    case NvMMBlockType_EncAAC:
        str = "EncAAC";
        break;
    case NvMMBlockType_EncMP3:
        str = "EncMP3";
        break;
    case NvMMBlockType_EncSBC:
        str = "EncSBC";
        break;
    case NvMMBlockType_DecAMRNB:
        str = "DecAMRNB";
        break;
    case NvMMBlockType_DecAMRWB:
        str = "DecAMRWB";
        break;
    case NvMMBlockType_DecAAC:
        str = "DecAAC";
        break;
    case NvMMBlockType_DecEAACPLUS:
        str = "DecEAACPLUS";
        break;
    case NvMMBlockType_DecMP3:
        str = "DecMP3";
        break;
    case NvMMBlockType_DecMP2:
        str = "DecMP2";
        break;
    case NvMMBlockType_DecWMA:
        str = "DecWMA";
        break;
    case NvMMBlockType_DecWMAPRO:
        str = "DecWMAPRO";
        break;
    case NvMMBlockType_DecWMALSL:
        str = "DecWMALSL";
        break;
    case NvMMBlockType_DecRA8:
        str = "DecRA8";
        break;
    case NvMMBlockType_DecWAV:
        str = "DecWAV";
        break;
    case NvMMBlockType_EncWAV:
        str = "EncWAV";
        break;
    case NvMMBlockType_DecSBC:
        str = "DecSBC";
        break;
    case NvMMBlockType_DecOGG:
        str = "DecOGG";
        break;
    case NvMMBlockType_DecBSAC:
        str = "DecBSAC";
        break;
    case NvMMBlockType_DecADTS:
        str = "DecADTS";
        break;
    case NvMMBlockType_RingTone:
        str = "RingTone";
        break;
    case  NvMMBlockType_SrcCamera:
        str = "SrcCamera";
        break;
    case  NvMMBlockType_SrcUSBCamera:
        str = "SrcUSBCamera";
        break;
    case NvMMBlockType_SourceAudio:
        str = "SourceAudio";
        break;
    case NvMMBlockType_Mux:
        str = "Mux";
        break;
    case NvMMBlockType_Demux:
        str = "Demux";
        break;
    case NvMMBlockType_FileReader:
        str = "FileReader";
        break;
    case NvMMBlockType_3gpFileReader:
        str = "3gpFileReader";
        break;
    case NvMMBlockType_FileWriter:
        str = "FileWriter";
        break;
    case NvMMBlockType_AudioMixer:
        str = "AudioMixer";
        break;
    case NvMMBlockType_AudioFX:
        str = "AudioFX";
        break;
    case NvMMBlockType_DigitalZoom:
        str = "DigitalZoom";
        break;
    case NvMMBlockType_Sink:
        str = "Sink";
        break;
    case NvMMBlockType_SinkAudio:
        str = "SinkAudio";
        break;
    case NvMMBlockType_SinkVideo:
        str = "SinkVideo";
        break;
    case NvMMBlockType_SuperParser:
        str = "SuperParser";
        break;
    case NvMMBlockType_3gppWriter:
        str = "3gppWriter";
        break;
    case NvMMBlockType_VideoRenderer:
        str = "VideoRenderer";
        break;
    case NvMMBlockType_Force32:
    default:
        str = "Unknown";
        break;
    }
    return str;
}

typedef struct NvMMAVPProfRec
{
#if NV_IS_AVP
    NvRmDeviceHandle hRmDevice;
    NvRmMemHandle hMemHandle;
    NvU32 NumApertures;
    NvU32 Offsets[NVOS_MAX_PROFILE_APERTURES];
    NvU32 Sizes[NVOS_MAX_PROFILE_APERTURES];
    void *pData[NVOS_MAX_PROFILE_APERTURES];
#else
    NvU32 NumApertures;
    NvU32 Sizes[NVOS_MAX_PROFILE_APERTURES];
    void *pData[NVOS_MAX_PROFILE_APERTURES];
#endif
} NvMMAVPProf;

NvError NvMMAVPProfileCreate(void **phNvMMProf) 
{
#if NV_IS_AVP
    NvMMAVPProf *pProf = NULL;
    NvError status = NvSuccess;
    NvU32 nPos, nTotSize = 0, i;
    void *temp;

    if (phNvMMProf == NULL)
        return NvError_BadParameter;

    pProf  = NvOsAlloc(sizeof(NvMMAVPProf));
    if (!pProf)
        return NvError_InsufficientMemory; 
    NvOsMemset(pProf, 0, sizeof(NvMMAVPProf));

    NvOsProfileApertureSizes(&pProf->NumApertures, &pProf->Sizes[0]);

    if (pProf->NumApertures == 0)
        return status;

    for (i = 0; i < pProf->NumApertures; i++)
        nTotSize += pProf->Sizes[i];

    status = NvRmOpen(&pProf->hRmDevice, 0);
    if (status != NvSuccess)
    {
        NvMMAVPProfileDestroy(pProf);
        return status;
    }

    status = NvRmMemHandleCreate(pProf->hRmDevice, 
                                 &pProf->hMemHandle, 
                                 nTotSize);
    if (status != NvSuccess)
    {
        NvMMAVPProfileDestroy(pProf);
        return status;
    }

    status = NvRmMemAlloc(pProf->hMemHandle,
                          NULL, 
                          0,
                          32, 
                          NvOsMemAttribute_Uncached);
    if (status != NvSuccess)
    {
        NvMMAVPProfileDestroy(pProf);
        return status;
    }

    NvRmMemPin(pProf->hMemHandle);

    nPos = 0;
    for (i = 0; i < pProf->NumApertures; i++)
    {
        pProf->Offsets[i] = nPos;
        status = NvRmMemMap(pProf->hMemHandle,
                            pProf->Offsets[i],
                            pProf->Sizes[i],
                            NVOS_MEM_READ_WRITE,
                            &temp);
        if (status != NvSuccess)
        {
            NvMMAVPProfileDestroy(pProf);
            return status;
        }

        pProf->pData[i] = temp;
        nPos += pProf->Sizes[i];
    }

    NvOsProfileStart((void**)(pProf->pData));

    *phNvMMProf = (void *)pProf;
    return status;
#else
    NvMMAVPProf *pProf = NULL;

    if (phNvMMProf == NULL)
        return NvError_BadParameter;

    pProf  = NvOsAlloc(sizeof(NvMMAVPProf));
    if (!pProf)
        return NvError_InsufficientMemory; 
    NvOsMemset(pProf, 0, sizeof(NvMMAVPProf));

    *phNvMMProf = (void *)pProf;
    return NvSuccess;
#endif
}


NvError NvMMAVPProfileDestroy(void *pProf)
{
    NvMMAVPProf *pNvProf = (NvMMAVPProf *)pProf;
    NvU32 i = 0;
    if (!pNvProf)
        return NvError_BadParameter;

#if !NV_IS_AVP
    for (i = 0; i < pNvProf->NumApertures; i++)
    {
        if (pNvProf->pData[i])
        {
            NvOsFree(pNvProf->pData[i]);
        }
    }
#else
    for (i = 0; i < pNvProf->NumApertures; i++)
    {
        if (pNvProf->pData[i])
        {
            NvRmMemUnmap(pNvProf->hMemHandle, 
                         pNvProf->pData[i],
                         pNvProf->Sizes[i]);
        }
    }

    if (pNvProf->hMemHandle)
    {
        NvRmMemUnpin(pNvProf->hMemHandle);
        NvRmMemHandleFree(pNvProf->hMemHandle);
    }

    if (pNvProf->hRmDevice)
    {
        NvRmClose(pNvProf->hRmDevice);
    }
#endif

    NvOsFree(pNvProf);
    return NvSuccess;
}


NvError
NvMMSendBlockAVPProf(void *pProf,
                      NvRmTransportHandle hRmTransport)
{
#if NV_IS_AVP
    NvError status = NvSuccess;
    NvU32 size = 0, i;
    NvMMAVPProf *pNvProf = (NvMMAVPProf *)pProf;
    NvU8 *msg = NvOsAlloc(256);
    if (!msg)
        return NvError_InsufficientMemory;

    if (!pNvProf)
        return status;

    NvOsProfileStop(&pNvProf->pData[0]);

    /* marshal parameters */
    *((NvU32 *)&msg[size]) = NVMM_MSG_BlockAVPPROF;
    size += sizeof(NvU32);
    *((NvU32 *)&msg[size]) = pNvProf->NumApertures;
    size += sizeof(NvU32);
    *((NvRmMemHandle *)&msg[size]) = pNvProf->hMemHandle;
    size += sizeof(NvRmMemHandle);
    for (i = 0; i < pNvProf->NumApertures; i++)
    {
        *((NvU32 *)&msg[size]) = (NvU32)pNvProf->Offsets[i];
        size += sizeof(NvU32);
        *((NvU32 *)&msg[size]) = (NvU32)pNvProf->Sizes[i];
        size += sizeof(NvU32);
    }

    status = NvRmTransportSendMsg(hRmTransport,
                                  (void *)msg,
                                  size,
                                  NV_WAIT_INFINITE);

    NvOsFree(msg);
    return status;
#else
    return NvSuccess;
#endif
}

NvError
NvMMStoreBlockAVPProf(void *pProf, char *msg) 
{
    NvU32 size = 0;
    NvError status = NvSuccess;
    NvRmMemHandle hMemHandle;
    void *temp;
    NvU32 i;
    NvU32 Offsets[NVOS_MAX_PROFILE_APERTURES];
    NvMMAVPProf *pNvProf = (NvMMAVPProf *)pProf;

    /* unmarshal parameters */    
    size += sizeof(NvU32);
    pNvProf->NumApertures = *((NvU32 *)&msg[size]);
    size += sizeof(NvU32);
    hMemHandle = *((NvRmMemHandle *)&msg[size]);
    size += sizeof(NvRmMemHandle);

    pNvProf->NumApertures = MIN(NVOS_MAX_PROFILE_APERTURES, pNvProf->NumApertures);

    for (i = 0; i < pNvProf->NumApertures; i++)
    {
        Offsets[i] = *((NvU32 *)&msg[size]);
        size += sizeof(NvU32);
        pNvProf->Sizes[i] = *((NvU32 *)&msg[size]);
        size += sizeof(NvU32);
    }

    for (i = 0; i < pNvProf->NumApertures; i++)
    {
        pNvProf->pData[i] = NvOsAlloc(pNvProf->Sizes[i]);
        if (!pNvProf->pData[i])
            break;

        status = NvRmMemMap(hMemHandle, 
                            Offsets[i],
                            pNvProf->Sizes[i],
                            NVOS_MEM_READ_WRITE,
                            &temp);
        if (status != NvSuccess)
            return status;
    
        NvOsMemcpy(pNvProf->pData[i], temp, pNvProf->Sizes[i]); 
        NvRmMemUnmap(hMemHandle, temp, pNvProf->Sizes[i]);
    }

    return status;
}

/* This needs to go away.  Soon. */
#define USE_TEMP_FIX 1

#if USE_TEMP_FIX
/* This is copied from the gmon_out.h header file */
#define GMON_MAGIC      "gmon"  /* magic cookie */
#define GMON_VERSION    1       /* version number */

struct gmon_hdr
{
    char cookie[4];
    char version[4];
    char spare[3 * 4];
};

/* types of records in this file: */
typedef enum
{
    GMON_TAG_TIME_HIST = 0,
    GMON_TAG_CG_ARC = 1,
    GMON_TAG_BB_COUNT = 2
} GMON_Record_Tag;

struct gmon_hist_hdr
{
    char low_pc[sizeof (char *)];  /* base pc address of sample buffer */
    char high_pc[sizeof (char *)]; /* max pc address of sampled buffer */
    char hist_size[4];             /* size of sample buffer */
    char prof_rate[4];             /* profiling clock rate */
    char dimen[15];                /* phys. dim., usually "seconds" */
    char dimen_abbrev;             /* usually 's' for "seconds" */
};

#define NVRTXC_IRAM_SAMPLE_POINTS (128*1024)
#define NVRTXC_SDRAM_SAMPLE_POINTS (512*1024)

typedef struct RtxcProfileRec
{
    struct gmon_hdr hdr;
    char tagTime;
    struct gmon_hist_hdr hist_hdr;
    NvU16 prof_data[1];
} RtxcProfile;
#if !NV_IS_AVP
static NvError
profile_write( NvOsFileHandle file, NvU32 index, void *aperture )
{
    NvError e;
    RtxcProfile *prof;
    NvU32 data_size;

    prof = (RtxcProfile *)aperture;

    if( index == 0 )
    {
        data_size = (NVRTXC_IRAM_SAMPLE_POINTS * 2);
    }
    else
    {
        data_size = (NVRTXC_SDRAM_SAMPLE_POINTS * 2);
    }

    NV_CHECK_ERROR_CLEANUP(
        NvOsFwrite( file, &prof->hdr, sizeof(prof->hdr) )
    );

    NV_CHECK_ERROR_CLEANUP(
        NvOsFwrite( file, &prof->tagTime, sizeof(prof->tagTime) )
    );

    NV_CHECK_ERROR_CLEANUP(
        NvOsFwrite( file, &prof->hist_hdr, sizeof(prof->hist_hdr) )
    );

    NV_CHECK_ERROR_CLEANUP(
        NvOsFwrite( file, prof->prof_data, data_size )
    );

fail:
    return e;
}
#endif
#endif

NvError 
NvMMAVPProfileDumpToDisk(void *pProf)
{
#if !NV_IS_AVP
    NvMMAVPProf *pNvProf = (NvMMAVPProf *)pProf;
    NvU32 i = 0;
    NvError status = NvSuccess;

    if (!pNvProf)
        return NvError_BadParameter;

    for (i = 0; i < pNvProf->NumApertures; i++)
    {
        NvOsFileHandle fPtr; 
        NvU8 oFilename[256];

        if (!pNvProf->pData[i] || pNvProf->Sizes[i] == 0)
            continue;

        NvOsSnprintf((char *)oFilename, 256, "gmon%d.out", i);

        status = NvOsFopen((const char *)oFilename, 
                           NVOS_OPEN_CREATE|NVOS_OPEN_WRITE, 
                           &fPtr);
        if (NvSuccess != status || !fPtr)
            continue;

#if USE_TEMP_FIX
        profile_write(fPtr, i, pNvProf->pData[i]);
#else
        NvOsFwrite(fPtr, pNvProf->pData[i], pNvProf->Sizes[i]);
#endif
        NvOsFclose(fPtr);
    }
#endif

    return NvSuccess;
}

#endif /* NVMM_KPILOG_ENABLED != 0 */
