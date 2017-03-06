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

#include "nvassert.h"
#include "nvrm_avp_swi_registry.h"
#include "nvmm_service_avp.h"


__swi(AvpSwiClientSwiNum_NvMM) NvError NvMMServiceSwi( NvU32 ClientSwiIndex, void *ApiInfo);

NvError NvMMServiceSendMessageBlocking(
    NvMMServiceHandle hNvMMService,
    void* pMessageBuffer,
    NvU32 MessageSize,
    void* pRcvMessageBuffer,
    NvU32 MaxRcvSize,
    NvU32 * pRcvMessageSize)
{
    NvError Status = NvSuccess;

    NvMMServiceApiInfo_SendMessageBlocking ApiInfo;
    ApiInfo.Type = NvMMServiceApiType_SendMessageBlocking;
    ApiInfo.hNvMMService = hNvMMService;
    ApiInfo.pMessageBuffer = pMessageBuffer;
    ApiInfo.MessageSize = MessageSize;
    ApiInfo.pRcvMessageBuffer = pRcvMessageBuffer;
    ApiInfo.MaxRcvSize = MaxRcvSize;
    ApiInfo.pRcvMessageSize = pRcvMessageSize;

    Status = NvMMServiceSwi(hNvMMService->ClientSwiIndex, &ApiInfo);

    return Status;
}

NvError NvMMServiceSendMessage( 
    NvMMServiceHandle hNvMMService,
    void* pMessageBuffer,
    NvU32 MessageSize ) 
{
    NvError Status = NvSuccess;

    NvMMServiceApiInfo_SendMessage ApiInfo;
    ApiInfo.Type = NvMMServiceApiType_SendMessage;
    ApiInfo.hNvMMService = hNvMMService;
    ApiInfo.pMessageBuffer = pMessageBuffer;
    ApiInfo.MessageSize = MessageSize;

    Status = NvMMServiceSwi(hNvMMService->ClientSwiIndex, &ApiInfo);

    return Status;
}
