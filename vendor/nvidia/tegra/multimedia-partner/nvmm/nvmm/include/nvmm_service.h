/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVMM_SERVICE_H
#define INCLUDED_NVMM_SERVICE_H

#include "nvmm.h"

 /**
 * @brief Initializes and opens the NvMM Service. 
 * @retval NvSuccess Indicates that the NvMMService has successfully opened.
 * @retval NvError_InsufficientMemory Indicates that function fails to allocate
 * the memory.
 * @retval NvError_NotInitialized Indicates the NvMMService initialization failed.
 */

 NvError NvMMServiceOpen( NvBool bLoadAxf ); // NvMMServiceHandle *pHandle );

 /** 
 * @brief Closes the NvMMService. This function de-initializes the NvMMService. 
 * This API never fails.
 *
 * @param None
 */

 void NvMMServiceClose( void ); //NvMMServiceHandle hNvMMService );

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
    void* pMessageBuffer,
    NvU32 MessageSize,
    void* pRcvMessageBuffer,
    NvU32 MaxRcvSize,
    NvU32 * pRcvMessageSize );

/** 
 * @brief Returns AVP service handle
*/
NvError GetAVPServiceHandle(void **phAVPService);

 /** 
  * @brief Returns service loader handle
 */

NvError NvMMServiceGetLoaderHandle(void **phServiceLoader);

#endif //INCLUDED_NVMM_SERVICE_H

