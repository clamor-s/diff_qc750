/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVMM_SERVICE_AVP_H
#define INCLUDED_NVMM_SERVICE_AVP_H

#include "nvos.h"
#include "nvrm_transport.h"
#include "nvmm_service_common.h"
#include "nvrm_moduleloader.h"
#include "nvrm_transport.h"
#include "nvassert.h"
#include "nvrm_avp_swi_registry.h"

typedef struct NvMMServiceRec *NvMMServiceHandle;

typedef struct NvMMServiceRec 
{
    NvRmDeviceHandle hRmDevice;
    NvRmTransportHandle hRmTransport;
    NvOsSemaphoreHandle NvmmServiceSema;
    NvOsSemaphoreHandle NvmmServiceRcvSema;
    NvOsSemaphoreHandle SyncSema;
    NvOsMutexHandle Mutex;
    NvOsThreadHandle NvmmServiceThreadHandle;
    NvU8 *RcvMsgBuffer;
    NvU32 RcvMsgBufferSize;
    NvBool bShutDown;
    NvU8 PortName[16];
    NvU32 ClientSwiIndex;
}NvMMService;

/**
 * @brief NvMM Service API type enumerations.
 * Following enum are used by NvMM Service for setting the API type.
 */

typedef enum
{
    NvMMServiceApiType_SendMessage = 1,
    NvMMServiceApiType_SendMessageBlocking,
    NvMMServiceApiType_Num,
    NvMMServiceApiType_Force32 = 0x7FFFFFFF
} NvMMServiceApiType;

/**
 * @struct NvMMServiceApiInfo_SendMessage - Holds the information 
 * for SendMessage API
 * 
 */
typedef struct NvMMServiceApiInfo_SendMessageRec
{
    NvMMServiceApiType Type;
    NvMMServiceHandle hNvMMService;
    void* pMessageBuffer;
    NvU32 MessageSize;
} NvMMServiceApiInfo_SendMessage;

/**
 * @struct NvMMServiceApiInfo_SendMessageBlocking - Holds the information 
 * for SendMessageBlocking API
 * 
 */
typedef struct NvMMServiceApiInfo_SendMessageBlockingRec
{
    NvMMServiceApiType Type;
    NvMMServiceHandle hNvMMService;
    void* pMessageBuffer;
    NvU32 MessageSize;
    void* pRcvMessageBuffer;
    NvU32 MaxRcvSize;
    void* pRcvMessageSize;
} NvMMServiceApiInfo_SendMessageBlocking;

 /**
 * @brief Initializes and opens the NvMM Service. 
 * @retval NvSuccess Indicates that the NvMMService has successfully opened.
 * @retval NvError_InsufficientMemory Indicates that function fails to allocate
 * the memory.
 * @retval NvError_NotInitialized Indicates the NvMMService initialization failed.
 */

 NvError NvMMServiceOpen(NvMMServiceHandle *pHandle, NvU8 *PortName );

 /** 
 * @brief Closes the NvMMService. This function de-initializes the NvMMService. 
 * This API never fails.
 *
 * @param None
 */

 void NvMMServiceClose( NvMMServiceHandle hNvMMService );

/** 
 * @brief Sends a message to the Service running on the other processor
 *
 * @param hNvMMService Handle to the NvMMService.
 * @param pMessageBuffer The pointer to the message buffer where message which 
 * need to be send is available.
 * @param MessageSize Specifies the size of the message.
 * 
 * @retval NvSuccess Indicates the operation succeeded.
 * @retval NvError_BadValue Indicates the arguments are not proper. 
 */
 NvError NvMMServiceSendMessage( 
    NvMMServiceHandle hNvMMService,
    void* pMessageBuffer,
    NvU32 MessageSize );

/** 
 * @brief Sends a message to the Service running on the other processor and waits for the response
 *
 * @param hNvMMService Handle to the NvMMService.
 * @param pMessageBuffer The pointer to the message buffer where message which 
 * need to be send is available.
 * @param MessageSize Specifies the size of the message.
 * @param pRcvMessageBuffer The pointer to the receive message buffer where the
 * received message will be copied.
 * @param MaxRcvSize The maximum size in bytes that may be copied to the receive buffer
 * @param pRcvMessageSize Pointer to the variable where the length of the received message 
 * will be stored.
 * 
 * @retval NvSuccess Indicates the operation succeeded.
 * @retval NvError_BadValue Indicates the arguments are not proper. 
 */

 NvError NvMMServiceSendMessageBlocking( 
    NvMMServiceHandle hNvMMService,
    void* pMessageBuffer,
    NvU32 MessageSize,
    void* pRcvMessageBuffer,
    NvU32 MaxRcvSize,
    NvU32 * pRcvMessageSize );

#endif //INCLUDED_NVMM_SERVICE_H

