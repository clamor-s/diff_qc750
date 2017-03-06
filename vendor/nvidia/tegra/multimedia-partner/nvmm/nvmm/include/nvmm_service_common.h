/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVMM_SERVICE_COMMON_H
#define INCLUDED_NVMM_SERVICE_COMMON_H

#include "nvrm_memmgr.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#define MSG_BUFFER_SIZE 256
#define EVENT_BUFFER_SIZE 128
#define min(a,b) (((a) < (b)) ? (a) : (b))

/**
 * @brief NvMM Service Message enumerations.
 * Following enum are used by NvMM Service for setting the Message type.
 */
typedef enum
{
    NVMM_SERVICE_MSG_UnmapBuffer = 1,
    NVMM_SERVICE_MSG_UnmapBuffer_Response,
    NVMM_SERVICE_MSG_GetInfo,
    NVMM_SERVICE_MSG_GetInfo_Response,
    NVMM_SERVICE_MSG_AllocScratchIRAM,
    NVMM_SERVICE_MSG_AllocScratchIRAM_Response,
    NVMM_SERVICE_MSG_FreeScratchIRAM,
    NVMM_SERVICE_MSG_FreeScratchIRAM_Response,
    NvMMServiceMsgType_Num,
    NvMMServiceMsgType_NvmmTransCommInfo,

    NvMMServiceMsgType_Force32 = 0x7FFFFFFF
} NvMMServiceMsgType;


/**
 * @brief IRAM scratch space enumerations.
 * Following enum are used for IRAM scratch space allocation for different multimedia codecs.
 */
typedef enum
{
    NvMMIRAMScratchType_Video = 1,
    NvMMIRAMScratchType_Audio,
    NvMMIRAMScratchType_Num,
    NvMMIRAMScratchType_Force32 = 0x7FFFFFFF
} NvMMIRAMScratchType;

/**
 * @struct NvMMServiceMsgInfo_UnmapBuffer - Holds the information for 
 * UnmapBuffer Message
 * 
 */
typedef struct NvMMServiceMsgInfo_UnmapBufferRec
{
    NvMMServiceMsgType MsgType;
    NvRmMemHandle hMem;
    void* pVirtAddr;
    NvU32 Length;
} NvMMServiceMsgInfo_UnmapBuffer;

/**
 * @struct NvMMServiceMsgInfo_UnmapBufferResponse - Holds the information 
 * for UnmapBufferResponse Message
 * 
 */
typedef struct NvMMServiceMsgInfo_UnmapBufferResponseRec
{
    NvMMServiceMsgType MsgType;
} NvMMServiceMsgInfo_UnmapBufferResponse;


/**
 * @struct NvMMServiceMsgInfo_AllocScratchIRAM - Holds the information for 
 * AllocScratchIRAM Message
 * 
 */
typedef struct NvMMServiceMsgInfo_AllocScratchIRAMRec
{
    NvMMServiceMsgType MsgType;
    NvMMIRAMScratchType IRAMScratchType;
    NvU32 Size;

} NvMMServiceMsgInfo_AllocScratchIRAM;


/**
 * @struct NvMMServiceMsgInfo_AllocScratchIRAMResponse - Holds the information 
 * for AllocScratchIRAMResponse Message
 * 
 */
typedef struct NvMMServiceMsgInfo_AllocScratchIRAMResponseRec
{
    NvMMServiceMsgType MsgType;
    NvRmMemHandle hMem;
    NvU32 pVirtAddr;
    NvU32 pPhyAddr;
    NvU32 Length;

} NvMMServiceMsgInfo_AllocScratchIRAMResponse;

/**
 * @struct NvMMServiceMsgInfo_FreeScratchIRAM - Holds the information for 
 * FreeScratchIRAM Message
 * 
 */
typedef struct NvMMServiceMsgInfo_FreeScratchIRAMRec
{
    NvMMServiceMsgType MsgType;
    NvMMIRAMScratchType IRAMScratchType;

} NvMMServiceMsgInfo_FreeScratchIRAM;

/**
 * @struct NvMMServiceMsgInfo_FreeScratchIRAMResponse - Holds the information 
 * for FreeScratchIRAMResponse Message
 * 
 */
typedef struct NvMMServiceMsgInfo_FreeScratchIRAMResponseRec
{
    NvMMServiceMsgType MsgType;

} NvMMServiceMsgInfo_FreeScratchIRAMResponse;

/**
 * @struct 
 * 
 */
typedef struct NvMMServiceMsgInfo_GetInfoRec
{
    NvMMServiceMsgType MsgType;
} NvMMServiceMsgInfo_GetInfo;

/**
 * @struct 
 * 
 */
typedef struct NvMMServiceMsgInfo_GetInfoResponseRec
{
    NvMMServiceMsgType MsgType;
    void *hAVPService;
} NvMMServiceMsgInfo_GetInfoResponse;

 /**
 * @brief A callback which can be used by NvMM service clients. This can be
 * used for notifying clients when any message arrives in service
 */
typedef void (*NvMMServiceCallBack)(void *pContext, void *pEventData, NvU32 EventDataSize);

/**
 * @struct for commicating between NVMM service on CPU and AVP for NVMM Transport
 */
typedef struct NvMMServiceMsgInfo_NvmmTransCommInfoRec
{
    /* Message  used by NvMM serice */
    NvMMServiceMsgType MsgType;
    /* Client call back , NvMM service will call this when message is received */
    NvMMServiceCallBack pCallBack;
    /* Clients private context */
    void *pContext; 
    /* Send max of 128 bytes of data between CPU and AVP, if bugger buffer is neeeded 
     * send shared buffer pointer, mem handle etc inside this and handle those caes on both sides
     */
    NvU8 pEventData[EVENT_BUFFER_SIZE];

}NvMMServiceMsgInfo_NvmmTransCommInfo;

#if defined(__cplusplus)
}
#endif

#endif // INCLUDED_NVMM_SERVICE_COMMON_H
