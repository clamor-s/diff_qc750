/*
 * Copyright (c) 2011 Trusted Logic S.A.
 * All Rights Reserved.
 *
 * This software is the confidential and proprietary information of
 * Trusted Logic S.A. ("Confidential Information"). You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered
 * into with Trusted Logic S.A.
 *
 * TRUSTED LOGIC S.A. MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. TRUSTED LOGIC S.A. SHALL
 * NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
 * MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */
#ifndef __SMX_DEFS_H__
#define __SMX_DEFS_H__

#include "s_version.h"
#include "smapi.h"
#include "tee_client_api.h"

#include "lib_bef2_encoder.h"
#include "lib_bef2_decoder.h"
#include "smx_os.h"

#define SM_DEFAULT_DEVICE_NAME_W L"smodule"

/*----------------------------------------------------------------------------
 * DEVICE_OBJECT Type
 *----------------------------------------------------------------------------*/
#define TFAPI_DEVICE_BUFFER_DEFAULT_SIZE  0x10000
#define TFAPI_DEVICE_BUFFER_MIN_SIZE      0x1000
#define TFAPI_OBJECT_TYPE_DEVICE_CONTEXT  0x15DE68CA
#define TFAPI_OBJECT_TYPE_SESSION         0xD5FF6C89
#define TFAPI_OBJECT_TYPE_OPERATION       0xA9FC64D0
#define TFAPI_OBJECT_TYPE_SHARED_MEMORY   0x55FE67A3
#define TFAPI_OBJECT_TYPE_INNER_OPERATION 0x3458FDE4
#define TFAPI_OBJECT_TYPE_ENCODER         0xA5A5A5A5
#define TFAPI_OBJECT_TYPE_DECODER         0xF5F5F5F5
#define TFAPI_OBJECT_TYPE_MANAGER         0xC5C5C5C5
#define TFAPI_OPERATION_STATE_UNUSED      0
#define TFAPI_OPERATION_STATE_PREPARE     1
#define TFAPI_OPERATION_STATE_INVOKE      2
#define TFAPI_OPERATION_STATE_RECEIVED    3
#define TFAPI_OPERATION_STATE_ENCODE      4
#define TFAPI_OPERATION_STATE_DECODE      5
#define TFAPI_OPEN_CLIENT_SESSION         1
#define TFAPI_INVOKE_CLIENT_COMMAND       2
#define TFAPI_CLOSE_CLIENT_SESSION        3
#define TFAPI_DEFAULT_ENCODER_SIZE        0x400
#define TFAPI_DEFAULT_DECODER_SIZE        0x400
#define TFAPI_ENCODER_INCREMENT           0x80
#define TFAPI_HANDLE_SCRAMBLER            0x375FAC6D
#define TFAPI_BUSY_TIMEOUT                50
#define TFAPI_TIME_IMMEDIATE              0
#define TFAPI_TIME_INFINITE               0xFFFFFFFFFFFFFFFF
#define TFAPI_INNER_OPERATION_COUNT       2
#define TFAPI_SHMEM_MAX_CAPACITY          0x800000
#define TFAPI_SHMEM_INITIAL_CAPACITY      16
#define TFAPI_SHMEM_CAPACITY_INC          8

typedef struct SMX_NODE {
   uint32_t nType;

   struct SMX_NODE* pNext;
   struct SMX_NODE* pPrevious;
} SMX_NODE;

typedef struct SMX_LIST {
   SMX_NODE* pFirst;
} SMX_LIST;


#if defined(WIN32)
typedef HANDLE   SMX_EVENT;
#elif defined(LINUX)
typedef sem_t    SMX_EVENT;
#endif

typedef struct SM_DEVICE_CONTEXT
{
   SMX_NODE sNode;

   uint32_t nReferenceCount;
   SM_HANDLE hDeviceHeapStructure;
   struct SMX_LIST sDeviceAllocatorList;
   uint32_t nDeviceBufferSize;
   uint8_t* pInnerEncoderBuffer;
   uint8_t* pInnerDecoderBuffer;
   bool bInnerEncoderBufferUsed;
   bool bInnerDecoderBufferUsed;
   uint8_t* pDeviceBuffer;
   SMX_CRITICAL_SECTION sCriticalSection;

   TEEC_Context      sTeecContext;
   TEEC_SharedMemory sSharedMem;
} SM_DEVICE_CONTEXT;

typedef struct SM_SHARED_MEMORY
{
   SMX_NODE sNode;

   struct SM_CLIENT_SESSION* pSession;
   uint32_t nReferenceCount;
   uint32_t nFlags;
   uint8_t* pClientBuffer; /* AFY: why not use sTeecSharedMem fields? */
   uint8_t* pBuffer;
   uint32_t nLength;
   TEEC_SharedMemory sTeecSharedMem;
} SM_SHARED_MEMORY;

typedef struct SMX_SHARED_MEMORY_REF
{
   SM_SHARED_MEMORY* pShMem;
   uint32_t  nOffset;
   uint32_t  nLength;
   uint32_t  nFlags;
} SMX_SHARED_MEMORY_REF;

/**
 * Describes the current state of an encoder.
 */
typedef struct SMX_ENCODER
{
   SMX_NODE sNode;

   /**
    * A pointer to the operation the encoder is attached to.
    */
   struct SM_OPERATION* pOperation;

   /**
    * The structure of the BEF encoder.
    */
   LIB_BEF_ENCODER sBEFEncoder;

} SMX_ENCODER;


/**
 * Describes the current state of an decoder.
 */
typedef struct SMX_DECODER
{
   SMX_NODE sNode;

   /**
    * A pointer to the operation the decoder is attached to.
    */
   struct SM_OPERATION* pOperation;

   /**
    * A pointer to the parent decoder of this decoder, or NULL if none.
    */
   struct SMX_DECODER* pParentDecoder;

   /**
    * A pointer to the next element in the linked list of the children decoders
    * this decoder belongs to, or NULL if there is no such element or list.
    */
   struct SMX_DECODER* pNextSibling;

   /**
    * The handle of the underlying BEF decoder.
    */
   LIB_BEF_DECODER sBEFDecoder;
} SMX_DECODER;

/*
* Struct for the TEEC_API used in PerformOperation
* For storing the info sent in a PrepareXXXOperation
*/
typedef struct USER_DATA
{
    /* PrepareOpenOpearion */
    void*          pLoginInfo;
    uint8_t*       pIdentifierString;
}USER_DATA;

#define TFAPI_SHARED_MEMORY_REFERENCE_MAX_COUNT 2

typedef struct SM_OPERATION
{
   SMX_NODE sNode;

   struct SM_CLIENT_SESSION*  pSession;
   uint32_t                   nOperationState;
   SM_ERROR                   nOperationError;
   uint32_t                   nMessageType;
   uint64_t                   sTimeOut;
   struct SMX_ENCODER         sEncoder;
   struct SMX_DECODER         sDecoder;
   uint32_t                   nCommandIdentifier;
   SMX_SHARED_MEMORY_REF      sPreallocatedSharedMemoryReference[TFAPI_SHARED_MEMORY_REFERENCE_MAX_COUNT];
   TEEC_Operation             sTeecOperation;
} SM_OPERATION;

typedef struct SM_CLIENT_SESSION
{
   SMX_NODE sNode;

   uint32_t           nReferenceCount;
   SM_DEVICE_CONTEXT* pDeviceContext;
   SM_OPERATION       sInnerOperation[TFAPI_INNER_OPERATION_COUNT];
   TEEC_Session       sTeecSession;
   uint32_t           nLoginType;
   S_UUID             sDestinationUUID;
} SM_CLIENT_SESSION;

typedef struct SMX_MANAGER_SESSION
{
   SMX_NODE sNode;

   /**
    * The TEE Client Api Session.
    */
   TEEC_Session sTeecSession ;

   /**
    * The Device Context handle
    */
   S_HANDLE hDevice;

   SM_DEVICE_CONTEXT* pDeviceContext;

} SMX_MANAGER_SESSION;


typedef struct SMX_BUFFER
{
   SMX_NODE sNode;
   uint8_t buffer[1];
} SMX_BUFFER;


#endif  /* !defined(__SMX_DEFS_H__) */
