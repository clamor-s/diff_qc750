/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef _NVMM_STATS_H_
#define _NVMM_STATS_H_

#include "nvmm.h"
#include "nvrm_transport.h"

#define NVMM_MAX_STAT_ENTRIES 0x5000

typedef enum
{
    CLIENT_TBF_TO_BLOCK = 1,
    CLIENTSIDE_SEND_TBF_MSG,
    BLOCKSIDE_RCV_TBF_MSG,
    BLOCKSIDE_CALL_TBF_TO_BLOCK,
    BLOCK_TBF_FROM_BLOCK,
    BLOCKSIDE_SEND_TBF_MSG,
    CLIENTSIDE_RCV_TBF_MSG,
    CLIENTSIDE_CALL_TBF_FROM_BLOCK,
}NvMMStat_TimeStampType;

NvError 
NvMMStatsCreate(
    void **phNvMMStat, 
    NvMMBlockType eType,
    NvU32 uMaxEntries);

void NvMMStatsDestroy(void *pStats);

NvError 
NvMMStats_TimeStamp(
    void *pStats, 
    NvU64 TimeStamp, 
    NvMMBufferType BufferType,
    NvU32 StreamIndex,
    NvMMStat_TimeStampType Type);

NvError 
NvMMStats_ComputeStatistics(
    void *pStats);

NvError
NvMMStatsSendBlockKPI(void *pStats,
                       NvRmTransportHandle hRmTransport);

NvError
NvMMStatsStoreBlockKPI(void *pStats, char *msg);


NvError 
NvMMAVPProfileCreate(void **phNvMMProf);

NvError NvMMAVPProfileDestroy(void *pProf);

NvError 
NvMMAVPProfileDumpToDisk(void *pProf);

NvError
NvMMSendBlockAVPProf(void *pProf,
                     NvRmTransportHandle hRmTransport);

NvError
NvMMStoreBlockAVPProf(void *pProf, char *msg);

#endif  /* _NVMM_STAT_H_ */



