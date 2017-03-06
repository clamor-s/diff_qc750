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
#define MODULE_NAME  "SMAPI"

#ifdef ANDROID
#include <stddef.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef LINUX
#ifndef ANDROID
#include <wchar.h>
#endif
#endif


#define SM_EXPORT_IMPLEMENTATION
#include "smapi.h"

#include "smx_defs.h"
#include "smx_encoding.h"
#include "smx_utils.h"
#include "smx_heap.h"

#include "lib_bef2_encoder.h"
#include "lib_bef2_decoder.h"

#include "tee_client_api.h"
#include "lib_char.h"

#include "s_error.h"


/*----------------------------------------------------------------------------
 * Create Device Context
 *----------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMDeviceCreateContext(
    IN   const wchar_t* pDeviceName,
         uint32_t       nReserved,
     OUT SM_HANDLE*     phDevice)
{
   SM_ERROR    nError;
   TEEC_Result nTeeError;
   SM_DEVICE_CONTEXT* pDevice = NULL;
   char*    pStrDeviceName = NULL;
   uint32_t nDeviceNameLen = 0;

   TRACE_INFO("SMDeviceCreateContext(%p, 0x%X, %p)", pDeviceName, nReserved, phDevice);

    /*
     * Validate parameters.
     */

   if (phDevice==NULL)
   {
       /* Output handle must be allocated */
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

  *phDevice = SM_HANDLE_INVALID;

   if (pDeviceName != NULL)
   {
#ifdef ANDROID
      nDeviceNameLen = strlen(pDeviceName);
#else
      nDeviceNameLen = wcslen(pDeviceName);
#endif
      nDeviceNameLen++; /* For the zero-terminating string */
      pStrDeviceName = (char*)malloc(nDeviceNameLen);
      if (pStrDeviceName == NULL)
      {
         TRACE_ERROR("SMDeviceCreateContext: Alloc device name failed (%d bytes)", nDeviceNameLen);
         nError = SM_ERROR_OUT_OF_MEMORY;
         goto error_free_ctx;
      }

      SMXWchar2Str(pStrDeviceName, (wchar_t*)pDeviceName, nDeviceNameLen);
   }

   /*
    * Allocate the device context.
    */
   pDevice = (SM_DEVICE_CONTEXT*)malloc(sizeof(SM_DEVICE_CONTEXT));
   if (pDevice == NULL)
   {
      TRACE_ERROR("SMDeviceCreateContext: Alloc device context failed");
      nError = SM_ERROR_OUT_OF_MEMORY;
      goto error_free_ctx;
   }

   memset(pDevice, 0, sizeof(SM_DEVICE_CONTEXT));

   pDevice->sNode.nType = TFAPI_OBJECT_TYPE_DEVICE_CONTEXT;
   if (nReserved == 0)
   {
      pDevice->nDeviceBufferSize = TFAPI_DEVICE_BUFFER_DEFAULT_SIZE;
   }
   else if (nReserved < TFAPI_DEVICE_BUFFER_MIN_SIZE)
   {
      pDevice->nDeviceBufferSize = TFAPI_DEVICE_BUFFER_MIN_SIZE;
   }
   else
   {
      pDevice->nDeviceBufferSize = nReserved;
   }
   if (pDevice->nDeviceBufferSize > TFAPI_SHMEM_MAX_CAPACITY)
   {
      TRACE_ERROR("SMDeviceCreateContext: Device buffer too large (%d bytes)", pDevice->nDeviceBufferSize);
      nError = SM_ERROR_OUT_OF_MEMORY;
      goto error_free_ctx;
   }

   /*
    * Create the Device context
    */
   nTeeError = TEEC_InitializeContext((const char*)pStrDeviceName, &pDevice->sTeecContext);
   if (nTeeError != TEEC_SUCCESS)
   {
      TRACE_ERROR("SMDeviceCreateContext: TEEC_InitializeContext failed (0x%08x)", nTeeError);
      /* No need to convert TEEC error into SMAPI error */
      nError = nTeeError;
      goto error_free_ctx;
   }

   /*
    * Allocate Master Shared Mem
    */
   pDevice->sSharedMem.size  = pDevice->nDeviceBufferSize;
   pDevice->sSharedMem.flags = SM_MEMORY_ACCESS_CLIENT_WRITE_SERVICE_READ |
                                SM_MEMORY_ACCESS_CLIENT_READ_SERVICE_WRITE;

   nTeeError = TEEC_AllocateSharedMemory(&pDevice->sTeecContext,
                                       &pDevice->sSharedMem);
   if (nTeeError != TEEC_SUCCESS)
   {
      TRACE_ERROR("SMDeviceCreateContext: TEEC_AllocateSharedMemory(%d) failed (0x%08x)",
                  pDevice->sSharedMem.size, nTeeError);
      /* No need to convert TEEC error into SMAPI error */
      nError = nTeeError;
      goto error_free_teec;
   }

   pDevice->pDeviceBuffer = pDevice->sSharedMem.buffer;

   /*
    * Initialize the Critical Section
    */
   nError = SMXInitCriticalSection(&pDevice->sCriticalSection);
   if (nError != SM_SUCCESS)
   {
      goto error_free_teec;
   }
   SMXLockCriticalSection(&pDevice->sCriticalSection);

   nError = SMXHeapInit(pDevice->pDeviceBuffer,pDevice->nDeviceBufferSize,&pDevice->hDeviceHeapStructure);
   if (nError != SM_SUCCESS)
   {
      goto error_free_cs;
   }
   pDevice->sDeviceAllocatorList.pFirst = NULL;

   pDevice->nReferenceCount = 1;
   pDevice->bInnerDecoderBufferUsed = false;
   pDevice->bInnerEncoderBufferUsed = false;
   pDevice->pInnerEncoderBuffer = (uint8_t*)SMXHeapAlloc(pDevice->hDeviceHeapStructure,TFAPI_DEFAULT_ENCODER_SIZE);
   if (pDevice->pInnerEncoderBuffer == NULL)
   {
      nError = SM_ERROR_OUT_OF_MEMORY;
      goto error_free_cs;
   }
   pDevice->pInnerDecoderBuffer = (uint8_t*)SMXHeapAlloc(pDevice->hDeviceHeapStructure,TFAPI_DEFAULT_DECODER_SIZE);
   if (pDevice->pInnerDecoderBuffer == NULL)
   {
      nError=SM_ERROR_OUT_OF_MEMORY;
      goto error_free_cs;
   }

   /*
    * Successful completion.
    */

   free(pStrDeviceName);

   *phDevice = SMXGetHandleFromPointer(pDevice);
   TRACE_INFO("SMDeviceCreateContext: Success (hDevice = 0x%X)", *phDevice);
   SMXUnLockCriticalSection(&pDevice->sCriticalSection);
   return SM_SUCCESS;

   /*
    * Error handling.
    */

error_free_cs:
   SMXUnLockCriticalSection(&pDevice->sCriticalSection);
   SMXTerminateCriticalSection(&pDevice->sCriticalSection);
error_free_teec:
   if (pDevice->sSharedMem.buffer != NULL)
   {
      TEEC_ReleaseSharedMemory(&pDevice->sSharedMem);
   }
   TEEC_FinalizeContext(&pDevice->sTeecContext);
error_free_ctx:
   free(pDevice);
   free(pStrDeviceName);
   TRACE_INFO("SMDeviceCreateContext: Error 0x%X", nError);
   return nError;
}


/*----------------------------------------------------------------------------
 * Delete Device Context
 *----------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMDeviceDeleteContext(
      SM_HANDLE hDevice)
{
   SM_ERROR nError = SM_SUCCESS;
   SM_DEVICE_CONTEXT* pDevice=NULL;
   uint32_t nType;

   TRACE_INFO("SMXDeviceDeleteContext(0x%X)", hDevice);

   if (hDevice == SM_HANDLE_INVALID)
   {
      return SM_SUCCESS;
   }

   /*
   * Destroy the device.
   */
   pDevice = (SM_DEVICE_CONTEXT*)SMXGetPointerFromHandle(hDevice);

   nType = pDevice->sNode.nType;

   if (nType != TFAPI_OBJECT_TYPE_DEVICE_CONTEXT)
   {
      SMAPI_ASSERT();
   }
   SMXLockCriticalSection(&pDevice->sCriticalSection);

   if (pDevice->nReferenceCount>1)
   {
      /* the only possible error code is SM_ERROR_ILLEGAL_STATE :
      the client tries to delete a context that contains pending
      operations or client sessions */
      SMXUnLockCriticalSection(&pDevice->sCriticalSection);
      nError = SM_ERROR_ILLEGAL_STATE;
      goto error;
   }
   pDevice->nReferenceCount=0;

   SMXDeviceContextRemoveAll(pDevice);

   SMXUnLockCriticalSection(&pDevice->sCriticalSection);
   SMXTerminateCriticalSection(&pDevice->sCriticalSection);

   TEEC_ReleaseSharedMemory(&pDevice->sSharedMem);
   TEEC_FinalizeContext(&pDevice->sTeecContext);

   /*
    * Completion.
    */
   memset(pDevice, 0, sizeof(SM_DEVICE_CONTEXT));
   free(pDevice);
error:
   if (nError == SM_SUCCESS)
   {
      TRACE_INFO("SMXDeviceDeleteContext(0x%X): Success", hDevice);
   }
   else
   {
      TRACE_INFO("SMXDeviceDeleteContext(0x%X): Error 0x%X", hDevice, nError);
   }

   return nError;
}

static SM_DEVICE_CONTEXT* static_SMRetrieveDeviceContextFromHandle(
   SM_HANDLE hObject)
{
   SMX_NODE* pNode;
   SM_DEVICE_CONTEXT* pDevice;

   pDevice = NULL;

   pNode=SMXGetPointerFromHandle(hObject);
   switch (pNode->nType)
   {
   case TFAPI_OBJECT_TYPE_DECODER:
   case TFAPI_OBJECT_TYPE_ENCODER:
      pNode=(SMX_NODE*)((SMX_DECODER*)pNode)->pOperation;
   case TFAPI_OBJECT_TYPE_INNER_OPERATION:
   case TFAPI_OBJECT_TYPE_OPERATION:
   case TFAPI_OBJECT_TYPE_SHARED_MEMORY:
      pNode=(SMX_NODE*)((SM_OPERATION*)pNode)->pSession;
   case TFAPI_OBJECT_TYPE_SESSION:
      pNode=(SMX_NODE*)((SM_CLIENT_SESSION*)pNode)->pDeviceContext;
   case TFAPI_OBJECT_TYPE_DEVICE_CONTEXT:
      pDevice=(SM_DEVICE_CONTEXT*)pNode;
      break;
   case TFAPI_OBJECT_TYPE_MANAGER:
      pDevice=(SM_DEVICE_CONTEXT*)((SMX_MANAGER_SESSION*)pNode)->pDeviceContext;
      break;
   default:
      SMAPI_ASSERT();
      break;
   }

   return pDevice;
}

/*----------------------------------------------------------------------------
 * Free
 *----------------------------------------------------------------------------*/
SM_EXPORT void SMFree(
      SM_HANDLE hObject,
      void* pBuffer)
{
   SMX_NODE* pNode;
   SM_DEVICE_CONTEXT* pDevice;

   if (pBuffer==NULL)
   {
      return;
   }
   if (hObject==SM_HANDLE_INVALID)
   {
      free(pBuffer);
   }
   else
   {
      pDevice = static_SMRetrieveDeviceContextFromHandle(hObject);
      SMXLockCriticalSection(&pDevice->sCriticalSection);
      pNode=(SMX_NODE*)((uint8_t*)pBuffer-offsetof(SMX_BUFFER,buffer));
      SMXDeviceContextFree(pDevice,pNode);
      SMXUnLockCriticalSection(&pDevice->sCriticalSection);
   }
}

/*----------------------------------------------------------------------------
 * Stub Get Time Limit
 *----------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMStubGetTimeLimit(
      SM_HANDLE      hElement,
      uint32_t       nTimeout,
      SM_TIME_LIMIT* pTimeLimit)
{
   uint64_t timeLimit=0;
   SM_DEVICE_CONTEXT* pDevice;

   MD_VAR_NOT_USED(hElement)

   TRACE_INFO("SMStubGetTimeLimit(0x%X, %u ms)", hElement, nTimeout);

   if (pTimeLimit==NULL)
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   pDevice = static_SMRetrieveDeviceContextFromHandle(hElement);

   if (nTimeout == SM_INFINITE_TIMEOUT)
   {
      /* Infinite timeout */
      timeLimit = 0xFFFFFFFFFFFFFFFFULL;
   }
   else
   {
      timeLimit = SMXGetCurrentTime() + nTimeout;
   }
   TRACE_INFO("SMStubGetTimeLimit: %ld",timeLimit);
   memcpy(pTimeLimit,&timeLimit,sizeof(SM_TIME_LIMIT));
   return SM_SUCCESS;
}

/*----------------------------------------------------------------------------
 * Stub Prepare Open Operation
 *----------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMStubPrepareOpenOperation(
      SM_HANDLE            hDevice,
      uint32_t             nLoginType,
      const void*          pLoginInfo,
      const SM_UUID*       pidService,
      uint32_t             nControlMode,
      const SM_TIME_LIMIT* pTimeLimit,
      uint32_t             nReserved1,
      uint32_t             nReserved2,
      SM_HANDLE*           phClientSession,
      SM_HANDLE*           phParameterEncoder,
      SM_HANDLE*           phOperation)
{
   SM_ERROR nError;
   SM_DEVICE_CONTEXT* pDevice=NULL;
   SM_CLIENT_SESSION* pClientSession = NULL;
   SM_OPERATION* pInnerOperation = NULL;
   SM_HANDLE hEncoder;
   LIB_BEF_ENCODER* pBEFEncoder;

   uint32_t nIndex;

   TRACE_INFO("SMStubPrepareOpenOperation(0x%X, 0x%X, %p, 0x%X, 0x%X, %p, 0x%X, 0x%x, %p, %p, %p)",
         hDevice, nLoginType, pLoginInfo, pidService, nControlMode, pTimeLimit,
         nReserved1, nReserved2, phClientSession, phParameterEncoder,
         phOperation);

   MD_VAR_NOT_USED(nControlMode)

   /*
    * Validate parameters.
    */
   if (phClientSession != NULL)
   {
      *phClientSession = SM_HANDLE_INVALID;
   }
   if (phParameterEncoder != NULL)
   {
      *phParameterEncoder = SM_HANDLE_INVALID;
   }
   if (phOperation!=NULL)
   {
      *phOperation = SM_HANDLE_INVALID;
   }

   if((pidService == NULL)||
      (phClientSession == NULL)||
      (phParameterEncoder == NULL)||
      (phOperation == NULL)||
      (pLoginInfo != NULL))
   {
      TRACE_ERROR("SMStubPrepareOpenOperation: Invalid parameters");
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   switch (nLoginType)
   {
      case SM_LOGIN_PUBLIC:
          TRACE_INFO("SMStubPrepareOpenOperation: LoginType is SM_LOGIN_PUBLIC");
          break;
      case SM_LOGIN_OS_IDENTIFICATION:
          TRACE_INFO("SMStubPrepareOpenOperation: LoginType is SM_LOGIN_OS_IDENTIFICATION");
          break;
      case SM_LOGIN_PRIVILEGED:
          TRACE_INFO("SMStubPrepareOpenOperation: LoginType is SM_LOGIN_PRIVILEGED");
          break;
      case SM_LOGIN_AUTHENTICATION:
          TRACE_INFO("SMStubPrepareOpenOperation: LoginType is SM_LOGIN_AUTHENTICATION");
          break;
      case SM_LOGIN_AUTHENTICATION_FALLBACK_OS_IDENTIFICATION:
          TRACE_INFO("SMStubPrepareOpenOperation: LoginType is FALLBACK_OS_IDENTIFICATION");
          break;
      default:
         TRACE_ERROR("SMStubPrepareOpenOperation: Invalid nLoginType parameter (0x%X)",
               nLoginType);
         return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   nError=SMXLockObjectFromHandle(
      hDevice,
      (void*)&pDevice,
      TFAPI_OBJECT_TYPE_DEVICE_CONTEXT);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubPrepareOpenOperation: SMXLockObjectFromHandle error");
      goto error;
   }
   if (pDevice==NULL)
   {
      TRACE_ERROR("SMStubPrepareOpenOperation: Device context error");
      goto error;
   }

   /*
    * Create the client session.
    */

   pClientSession=(SM_CLIENT_SESSION*)SMXDeviceContextAlloc(pDevice,sizeof(SM_CLIENT_SESSION));
   if (pClientSession==NULL)
   {
      TRACE_ERROR("SMStubPrepareOpenOperation: SMXDeviceContextAlloc error");
      nError=SM_ERROR_OUT_OF_MEMORY;
      goto error;
   }
   pClientSession->pDeviceContext = pDevice;
   pClientSession->sNode.nType=TFAPI_OBJECT_TYPE_SESSION;
   pClientSession->nReferenceCount=2;
   pClientSession->nLoginType = nLoginType;
   memcpy(&pClientSession->sDestinationUUID,
          pidService,
          sizeof(SM_UUID));

   for(nIndex=0;nIndex<TFAPI_INNER_OPERATION_COUNT;nIndex++)
   {
      pClientSession->sInnerOperation[nIndex].sNode.nType=TFAPI_OBJECT_TYPE_INNER_OPERATION;
      pClientSession->sInnerOperation[nIndex].nOperationState=TFAPI_OPERATION_STATE_UNUSED;
      pClientSession->sInnerOperation[nIndex].pSession=pClientSession;
      memset(&pClientSession->sInnerOperation[nIndex].sEncoder, 0, sizeof(SMX_ENCODER));
      memset(&pClientSession->sInnerOperation[nIndex].sDecoder, 0, sizeof(SMX_DECODER));
   }

   /*
    * Create the operation.
    */

   pInnerOperation=&pClientSession->sInnerOperation[1];
   pInnerOperation->nOperationState=TFAPI_OPERATION_STATE_PREPARE;
   pInnerOperation->nMessageType=TFAPI_OPEN_CLIENT_SESSION;
   pInnerOperation->nOperationError=S_SUCCESS;
   if(pTimeLimit == NULL)
   {
      pInnerOperation->sTimeOut = (uint64_t)-1;
   }
   else
   {
      memcpy(&(pInnerOperation->sTimeOut),pTimeLimit,sizeof(uint64_t));
   }

   nError=SMXEncoderInit(pInnerOperation, nReserved1);
   if (nError!=SM_SUCCESS)
   {
      goto error;
   }
   nError=SMXDecoderCreate(
            pInnerOperation,
            NULL,
            nReserved2,
            NULL);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubPrepareOpenOperation: SMXDecoderCreate error");
      goto error;
   }

   /*
    * Encode the operation-specific parameter data.
    */

   hEncoder=SMXGetHandleFromPointer(&pInnerOperation->sEncoder);

   pBEFEncoder = &(pInnerOperation->sEncoder.sBEFEncoder);

   if (pInnerOperation->sEncoder.sBEFEncoder.nError!=S_SUCCESS)
   {
      nError=pInnerOperation->sEncoder.sBEFEncoder.nError;
      goto error;
   }

   /*
    * Successful completion.
    */

   *phClientSession = SMXGetHandleFromPointer(pClientSession);
   *phOperation = SMXGetHandleFromPointer(pInnerOperation);
   *phParameterEncoder = hEncoder;

   TRACE_INFO("SMStubPrepareOpenOperation(0x%X): Success",
         hDevice);

   SMXUnLockCriticalSection(&pDevice->sCriticalSection);
   return SM_SUCCESS;

   /*
    * Error handling.
    */

error:
   if (pInnerOperation!=NULL)
   {
      SMXEncoderUninit(&pInnerOperation->sEncoder);
      SMXDecoderDestroy(&pInnerOperation->sDecoder, false);
   }
   SMXDeviceContextFree(pDevice,(SMX_NODE*)pClientSession);
   SMXUnLockObject((SMX_NODE*)pDevice);

   TRACE_INFO("SMStubPrepareOpenOperation(0x%X): Error 0x%X",
         hDevice, nError);
   return nError;
}


/*----------------------------------------------------------------------------
 * Stub Prepare Invoke Operation
 *----------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMStubPrepareInvokeOperation(
      SM_HANDLE            hClientSession,
      uint32_t             nCommandIdentifier,
      const SM_TIME_LIMIT* pTimeLimit,
      uint32_t             nReserved1,
      uint32_t             nReserved2,
      SM_HANDLE*           phParameterEncoder,
      SM_HANDLE*           phOperation)
{
   SM_ERROR nError;
   SM_DEVICE_CONTEXT* pDevice=NULL;
   SM_CLIENT_SESSION* pClientSession;
   SM_OPERATION* pOperation = NULL;

   TRACE_INFO("SMStubPrepareInvokeOperation(0x%X, 0x%X, %p, 0x%X, 0x%X, %p, %p)",
         hClientSession, nCommandIdentifier, pTimeLimit, nReserved1, nReserved2,
         phParameterEncoder, phOperation);

   /*
    * Validate parameters.
    */
   if (phParameterEncoder!=NULL)
   {
      *phParameterEncoder = SM_HANDLE_INVALID;
   }
   if (phOperation!=NULL)
   {
      *phOperation = SM_HANDLE_INVALID;
   }

   if (phParameterEncoder==NULL
      || phOperation==NULL)
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   nError=SMXLockObjectFromHandle(
      hClientSession,
      (void*)&pClientSession,
      TFAPI_OBJECT_TYPE_SESSION);
   if (nError!=SM_SUCCESS)
   {
      goto error;
   }
   if (pClientSession==NULL)
   {
      nError=SM_ERROR_ILLEGAL_ARGUMENT;
      goto error;
   }
   pDevice=pClientSession->pDeviceContext;

   /*
    * Create the operation.
    */
   if (pClientSession->sInnerOperation[1].nOperationState==TFAPI_OPERATION_STATE_UNUSED)
   {
      pOperation=&(pClientSession->sInnerOperation[1]);
      pOperation->nOperationState=TFAPI_OPERATION_STATE_PREPARE;
   }

   if (pOperation==NULL)
   {
      pOperation=(SM_OPERATION*)SMXDeviceContextAlloc(pDevice,sizeof(SM_OPERATION));
      if (pOperation==NULL)
      {
         nError=SM_ERROR_OUT_OF_MEMORY;
         goto error;
      }
      pOperation->sNode.nType=TFAPI_OBJECT_TYPE_OPERATION;
      pOperation->pSession=pClientSession;
      pOperation->nOperationState=TFAPI_OPERATION_STATE_PREPARE;
   }

   pOperation->nMessageType=TFAPI_INVOKE_CLIENT_COMMAND;
   pOperation->nOperationError=S_SUCCESS;
   if(pTimeLimit == NULL)
   {
      pOperation->sTimeOut = (uint64_t)-1;
   }
   else
   {
      pOperation->sTimeOut = *(uint64_t*)pTimeLimit;
   }

   nError=SMXEncoderInit(pOperation, nReserved1);
   if (nError!=SM_SUCCESS)
   {
      goto error;
   }
   nError=SMXDecoderCreate(
            pOperation,
            NULL,
            nReserved2,
            NULL);
   if (nError!=SM_SUCCESS)
   {
      goto error;
   }

   pOperation->nCommandIdentifier=nCommandIdentifier;

   /*
    * Successful completion.
    */

   *phOperation = SMXGetHandleFromPointer(pOperation);
   *phParameterEncoder = SMXGetHandleFromPointer(&pOperation->sEncoder);
   TRACE_INFO("SMStubPrepareInvokeOperation(0x%X): Success",
         hClientSession);

   /* release critical section but not the session object */
   SMXUnLockCriticalSection(&pDevice->sCriticalSection);
   return SM_SUCCESS;

   /*
    * Error handling.
    */

error:
   if (pOperation!=NULL)
   {
      SMXEncoderUninit(&pOperation->sEncoder);
      SMXDecoderDestroy(&pOperation->sDecoder, false);

      if (pOperation->sNode.nType!=TFAPI_OBJECT_TYPE_INNER_OPERATION)
      {
         SMXDeviceContextFree(pDevice,(SMX_NODE*)pOperation);
      }
   }
   SMXUnLockObject((SMX_NODE*)pClientSession);
   TRACE_INFO("SMStubPrepareInvokeOperation(0x%X): Error 0x%X",
         hClientSession, nError);
   return nError;
}


/*----------------------------------------------------------------------------
 * Stub Prepare Close Operation
 *----------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMStubPrepareCloseOperation(
      SM_HANDLE  hClientSession,
      uint32_t   nReserved1,
      uint32_t   nReserved2,
      SM_HANDLE* phParameterEncoder,
      SM_HANDLE* phOperation)
{
   SM_ERROR nError;
   SM_DEVICE_CONTEXT* pDevice=NULL;
   SM_CLIENT_SESSION* pClientSession=NULL;
   SM_OPERATION* pOperation = NULL;

   TRACE_INFO("SMStubPrepareCloseOperation(0x%X, 0x%X, 0x%X, %p, %p)",
         hClientSession, nReserved1, nReserved2, phParameterEncoder,
         phOperation);

   MD_VAR_NOT_USED(nReserved1)
   MD_VAR_NOT_USED(nReserved2)

   /*
    * Validate parameters.
    */
   if (phParameterEncoder!=NULL)
   {
      *phParameterEncoder = SM_HANDLE_INVALID;
   }
   if (phOperation!=NULL)
   {
      *phOperation = SM_HANDLE_INVALID;
   }

   if((phParameterEncoder == NULL)||
      (phOperation == NULL))
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   nError=SMXLockObjectFromHandle(
      hClientSession,
      (void*)&pClientSession,
      TFAPI_OBJECT_TYPE_SESSION);
   if (nError!=SM_SUCCESS)
   {
      goto error;
   }
   if (pClientSession==NULL)
   {
      nError=SM_ERROR_ILLEGAL_ARGUMENT;
      goto error;
   }
   pDevice=pClientSession->pDeviceContext;

   pOperation=pClientSession->sInnerOperation;

   pOperation->nOperationState=TFAPI_OPERATION_STATE_PREPARE;
   pOperation->nMessageType=TFAPI_CLOSE_CLIENT_SESSION;
   pOperation->nOperationError=S_SUCCESS;
   pOperation->sTimeOut=(uint64_t)-1;

   nError=SMXEncoderInit(pOperation, 0xffffffff);
   if (nError!=SM_SUCCESS)
   {
      goto error;
   }
   nError=SMXDecoderCreate(
            pOperation,
            NULL,
            0xffffffff,
            NULL);
   if (nError!=SM_SUCCESS)
   {
      goto error;
   }

   /*
    * Successful completion.
    */

   *phParameterEncoder = SMXGetHandleFromPointer(&pOperation->sEncoder);
   *phOperation = SMXGetHandleFromPointer(pOperation);


   TRACE_INFO("SMStubPrepareCloseOperation(0x%X): Success",
         hClientSession);
   SMXUnLockCriticalSection(&pDevice->sCriticalSection);
   return SM_SUCCESS;

   /*
    * Error handling.
    */

error:
   if (pOperation!=NULL)
   {
      SMXEncoderUninit(&pOperation->sEncoder);
      SMXDecoderDestroy(&pOperation->sDecoder, false);
   }
   SMXUnLockObject((SMX_NODE*)pClientSession);
   TRACE_INFO("SMStubPrepareCloseOperation(0x%X): Error 0x%X",
         hClientSession, nError);
   return nError;
}


/*----------------------------------------------------------------------------
 * Stub Perform Operation
 *----------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMStubPerformOperation(
      SM_HANDLE  hOperation,
      uint32_t   nReserved,
      SM_ERROR*  pnServiceErrorCode,
      SM_HANDLE* phAnswerDecoder)
{
    SM_ERROR           nError;
    TEEC_Result        nTeeError;
    TEEC_TimeLimit*    pTimeLimit = NULL;
    TEEC_TimeLimit     timeLimit;
    SM_DEVICE_CONTEXT* pDevice;
    SM_CLIENT_SESSION* pSession;
    SM_OPERATION*      pOperation;
    uint8_t            i;
    uint32_t           nErrorOrigin;
    uint32_t           nParamType2 = TEEC_NONE;
    uint32_t           nParamType3 = TEEC_NONE;

   TRACE_INFO("SMStubPerformOperation(0x%X, 0x%X, %p, %p)",
         hOperation, nReserved, pnServiceErrorCode, phAnswerDecoder);

   /*
    * Implementation Note:
    *    The TEEC_TimeLimit structure must be declared in the stack,
    *    and not dynamically allocated.
    *    The SMStubPerformOperation must not allocate any memory.
    */

   /*
    * Validate parameters.
    */
   if (pnServiceErrorCode!=NULL)
   {
      *pnServiceErrorCode = SM_ERROR_GENERIC;
   }
   if (phAnswerDecoder!=NULL)
   {
      *phAnswerDecoder = SM_HANDLE_INVALID;
   }

   nError=SMXLockOperationFromHandle(
      hOperation,
      &pOperation,
      SM_ACTION_PERFORM);
   if (nError!=SM_SUCCESS)
   {
      goto error;
   }

   if (pOperation->nMessageType!=TFAPI_CLOSE_CLIENT_SESSION)
   {
      if((pnServiceErrorCode == NULL)||
         (phAnswerDecoder == NULL)||
         (nReserved != 0))
      {
         nError = SM_ERROR_ILLEGAL_ARGUMENT;
         goto error;
      }
   }

   pSession = pOperation->pSession;
   pDevice  = pSession->pDeviceContext;

   nError = SMXEncoderGetError(&pOperation->sEncoder);
   if (nError!=SM_SUCCESS)
   {
      /* don't fail on a perform close operation */
      if (pOperation->nMessageType==TFAPI_CLOSE_CLIENT_SESSION)
      {
         if (pOperation->sEncoder.sBEFEncoder.pBuffer != pDevice->pInnerEncoderBuffer)
         {
            SMXHeapFree(pDevice->hDeviceHeapStructure, pOperation->sEncoder.sBEFEncoder.pBuffer);
         }
         else
         {
            pDevice->bInnerEncoderBufferUsed=false;
         }
         pOperation->sEncoder.sBEFEncoder.nError=SM_SUCCESS;
         pOperation->sEncoder.sBEFEncoder.nCapacity=0;
         pOperation->sEncoder.sBEFEncoder.pBuffer=NULL;
         libBEFEncoderInit(&pOperation->sEncoder.sBEFEncoder);
      }
      else
      {
         goto error;
      }
   }

   SMXEncoderAutoCloseSequences(&pOperation->sEncoder);

retry:
   /*
    * Initiate the operation.
    */
   if ((SMXGetCurrentTime()>=pOperation->sTimeOut) &&
         (pOperation->nMessageType!=TFAPI_CLOSE_CLIENT_SESSION))
   {
      TRACE_INFO("SMStubPerformOperation: Timeout!");
      nError = SM_ERROR_CANCEL;
      goto error;
   }

   for (i=0; i<TFAPI_SHARED_MEMORY_REFERENCE_MAX_COUNT; i++)
   {
      if (pOperation->sPreallocatedSharedMemoryReference[i].pShMem != NULL)
      {
         SMX_SHARED_MEMORY_REF* pSharedMemoryReference;
         SM_SHARED_MEMORY* pShMem;
         pSharedMemoryReference = &pOperation->sPreallocatedSharedMemoryReference[i];
         pShMem = pSharedMemoryReference->pShMem;

         if ((pShMem->nFlags & SM_MEMORY_ACCESS_CLIENT_WRITE_SERVICE_READ)!=0
            && (pShMem->nFlags & (SMX_MEMORY_ACCESS_DIRECT | SMX_MEMORY_ACCESS_DIRECT_FORCE))==0)
         {
            // update registered shared memory
            if (pShMem->pClientBuffer!=NULL && pShMem->pClientBuffer!=pShMem->pBuffer)
            {
               memcpy(pShMem->pBuffer,pShMem->pClientBuffer,pShMem->nLength);
            }
         }
      }
   }

   /* Copy time limit if required */
   if (pOperation->sTimeOut != (uint64_t)-1)
   {
      memcpy(&timeLimit, &pOperation->sTimeOut, sizeof(pOperation->sTimeOut));
      pTimeLimit = &timeLimit;
   }

  /*
   * Perform the ioctl call
   */
   switch(pOperation->nMessageType)
   {
      case TFAPI_OPEN_CLIENT_SESSION:
      {
         uint8_t*        pBuffer;
         uint32_t        nTEECLoginType = TEEC_LOGIN_PUBLIC;
         char*           pSignatureFile = NULL;
         uint32_t        nSignatureFileLen = 0;

         switch (pSession->nLoginType)
         {
         case SM_LOGIN_PUBLIC:
         case SM_LOGIN_OS_IDENTIFICATION:
         case SM_LOGIN_AUTHENTICATION:
         case SM_LOGIN_PRIVILEGED:
            /* The SMAPI login types are mapped on the TEEC login types */
            nTEECLoginType = pSession->nLoginType;
            break;
         case SM_LOGIN_AUTHENTICATION_FALLBACK_OS_IDENTIFICATION:
            nTEECLoginType = TEEC_LOGIN_AUTHENTICATION;
            break;
         default:
            SMAPI_ASSERT();
            break;
         }

         if (nTEECLoginType == TEEC_LOGIN_AUTHENTICATION)
         {
            nTeeError = TEEC_ReadSignatureFile((void **)&pSignatureFile, &nSignatureFileLen);
            if ((nTeeError == TEEC_ERROR_ITEM_NOT_FOUND) && (pSession->nLoginType == SM_LOGIN_AUTHENTICATION))
            {
                /* Should find a signature */
                TRACE_ERROR("SMStubPerformOperation: Missing signature error");
                nError = TEEC_ERROR_ACCESS_DENIED;
                goto error;
            }
            else if ((nTeeError != TEEC_SUCCESS) &&
                     (pSession->nLoginType == SM_LOGIN_AUTHENTICATION_FALLBACK_OS_IDENTIFICATION))
            { 
                /* No signature needed */
                nTEECLoginType = TEEC_LOGIN_USER_APPLICATION;
            }
            else
            {
                if (nTeeError != TEEC_SUCCESS)
                {
                    TRACE_ERROR("SMStubPerformOperation:TEEC_ReadSignatureFile returned an error 0x%x",nTeeError);
                    goto error;
                }
                /* Send signature file in param3 */
                pOperation->sTeecOperation.params[3].tmpref.buffer = pSignatureFile;
                pOperation->sTeecOperation.params[3].tmpref.size   = nSignatureFileLen;
                nParamType3 = TEEC_MEMREF_TEMP_INPUT;
            }
         }

         pBuffer = pOperation->sEncoder.sBEFEncoder.pBuffer;
         pOperation->sTeecOperation.params[0].memref.parent = &pDevice->sSharedMem;
         pOperation->sTeecOperation.params[0].memref.offset = (pBuffer!=NULL?pBuffer-pDevice->pDeviceBuffer:0);
         pOperation->sTeecOperation.params[0].memref.size   = pOperation->sEncoder.sBEFEncoder.nSize;

         pBuffer = pOperation->sDecoder.sBEFDecoder.pEncodedData;
         pOperation->sTeecOperation.params[1].memref.parent = &pDevice->sSharedMem;
         pOperation->sTeecOperation.params[1].memref.offset = (pBuffer!=NULL?pBuffer-pDevice->pDeviceBuffer:0);
         pOperation->sTeecOperation.params[1].memref.size   = pOperation->sDecoder.sBEFDecoder.nEncodedSize;

         pOperation->sTeecOperation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_MEMREF_PARTIAL_INPUT,
            TEEC_MEMREF_PARTIAL_OUTPUT,
            TEEC_NONE,
            nParamType3);

         SMXUnLockCriticalSection(&pOperation->pSession->pDeviceContext->sCriticalSection);
         nTeeError = TEEC_OpenSessionEx(&pDevice->sTeecContext,      /* IN context */
            &pSession->sTeecSession,                                 /* OUT session */
            pTimeLimit,                                              /* IN timelimit */
            (const TEEC_UUID*)&pSession->sDestinationUUID,           /* destination UUID */
            nTEECLoginType,                                          /* connectionMethod */
            NULL,                                                    /* connectionData */
            &pOperation->sTeecOperation,                             /* IN OUT operation */
            &nErrorOrigin);                                          /* OUT returnOrigin, optional */
         SMXLockCriticalSection(&pOperation->pSession->pDeviceContext->sCriticalSection);
         nError = TeecErrorToSmapiError(nTeeError, nErrorOrigin, pnServiceErrorCode);
         TRACE_INFO("SMStubPerformOperation: TEEC_OpenSession: 0x%08X,0x%08X -> 0x%08X,0x%08X\n",
                     nTeeError, nErrorOrigin, nError, *pnServiceErrorCode);
         if ((nError == SM_SUCCESS) || (nError == SM_ERROR_SERVICE))
         {
            pOperation->sDecoder.sBEFDecoder.nEncodedSize = pOperation->sTeecOperation.params[1].tmpref.size;
         }
         break;
      }

      case TFAPI_INVOKE_CLIENT_COMMAND:
      {
         /* Set param0 with the Encoder data
          * Set param1 with the Decoder data
          * Set param2 and param3 with data previously registered by the client
          */
         uint8_t* pBuffer;

         memset(&pOperation->sTeecOperation, 0, sizeof(TEEC_Operation));
         pBuffer = pOperation->sEncoder.sBEFEncoder.pBuffer;

         pOperation->sTeecOperation.params[0].memref.parent = &pDevice->sSharedMem;
         pOperation->sTeecOperation.params[0].memref.offset = (pBuffer!=NULL?pBuffer-pDevice->pDeviceBuffer:0);
         pOperation->sTeecOperation.params[0].memref.size   = pOperation->sEncoder.sBEFEncoder.nSize;

         pBuffer = pOperation->sDecoder.sBEFDecoder.pEncodedData;
         pOperation->sTeecOperation.params[1].memref.parent = &pDevice->sSharedMem;
         pOperation->sTeecOperation.params[1].memref.offset = (pBuffer!=NULL?pBuffer-pDevice->pDeviceBuffer:0);
         pOperation->sTeecOperation.params[1].memref.size   = pOperation->sDecoder.sBEFDecoder.nEncodedSize;

         for (i=0; i<TFAPI_SHARED_MEMORY_REFERENCE_MAX_COUNT; i++)
         {
            if (pOperation->sPreallocatedSharedMemoryReference[i].pShMem != NULL)
            {
                pOperation->sTeecOperation.params[2+i].memref.parent = &pOperation->sPreallocatedSharedMemoryReference[i].pShMem->sTeecSharedMem;
                pOperation->sTeecOperation.params[2+i].memref.size   = pOperation->sPreallocatedSharedMemoryReference[i].nLength;
                pOperation->sTeecOperation.params[2+i].memref.offset = pOperation->sPreallocatedSharedMemoryReference[i].nOffset;
                if (i==0)
                {
                    nParamType2 = TEEC_MEMREF_WHOLE | pOperation->sPreallocatedSharedMemoryReference[i].pShMem->nFlags;
                }
                else 
                {
                    nParamType3 = TEEC_MEMREF_WHOLE | pOperation->sPreallocatedSharedMemoryReference[i].pShMem->nFlags;
                }
            }
         }

         pOperation->sTeecOperation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_MEMREF_PARTIAL_INPUT, 
            TEEC_MEMREF_PARTIAL_OUTPUT,
            nParamType2,
            nParamType3);

         SMXUnLockCriticalSection(&pOperation->pSession->pDeviceContext->sCriticalSection);
         nTeeError = TEEC_InvokeCommandEx(&pOperation->pSession->sTeecSession,
            pTimeLimit,
            pOperation->nCommandIdentifier,
            &pOperation->sTeecOperation,          /* IN OUT operation */
            &nErrorOrigin             /* OUT errorOrigin, optional */
            );
         SMXLockCriticalSection(&pOperation->pSession->pDeviceContext->sCriticalSection);
         nError = TeecErrorToSmapiError(nTeeError, nErrorOrigin, pnServiceErrorCode);
         TRACE_INFO("SMStubPerformOperation: TEEC_InvokeCommand: 0x%08X,0x%08X -> 0x%08X,0x%08X\n",
                     nTeeError, nErrorOrigin, nError, *pnServiceErrorCode);
         if ((nError == SM_SUCCESS) || (nError == SM_ERROR_SERVICE))
         {
            pOperation->sDecoder.sBEFDecoder.nEncodedSize = pOperation->sTeecOperation.params[1].tmpref.size;
         }
         break;
      }

      case TFAPI_CLOSE_CLIENT_SESSION:
      {
         SMXUnLockCriticalSection(&pOperation->pSession->pDeviceContext->sCriticalSection);
         TEEC_CloseSession(&pOperation->pSession->sTeecSession);
         SMXLockCriticalSection(&pOperation->pSession->pDeviceContext->sCriticalSection);

         pOperation->sDecoder.sBEFDecoder.nEncodedSize = 0;
         break;
      }
   }

   libBEFDecoderInit(&pOperation->sDecoder.sBEFDecoder);

   if (pOperation->nMessageType==TFAPI_CLOSE_CLIENT_SESSION)
   {
      if((pnServiceErrorCode == NULL)||
         (phAnswerDecoder == NULL)||
         (nReserved != 0))
      {
         nError = SM_ERROR_ILLEGAL_ARGUMENT;
         goto error;
      }
      *phAnswerDecoder = SMXGetHandleFromPointer(&pOperation->sDecoder);
      *pnServiceErrorCode = SM_SUCCESS;
   }
   else
   {
       switch (nError)
       {
       case SM_SUCCESS:
       case SM_ERROR_SERVICE:
           *phAnswerDecoder = SMXGetHandleFromPointer(&pOperation->sDecoder);
           break;

       case SM_ERROR_NOT_SUPPORTED:
           if ((pOperation->nMessageType == TFAPI_OPEN_CLIENT_SESSION) &&
               (pSession->nLoginType == SM_LOGIN_AUTHENTICATION_FALLBACK_OS_IDENTIFICATION))
           {
               /* We could not open a session with the login SM_LOGIN_AUTHENTICATION */
               /* If it is not supported by the product, */
               /* retry with fallback to SM_LOGIN_OS_IDENTIFICATION */
               pSession->nLoginType = SM_LOGIN_OS_IDENTIFICATION;
               goto retry;
           }
           break;

       default:
           break;
       }
   }

   /*
    * Error handling.
    */

error:
   if (pOperation != NULL)
   {
      pOperation->nOperationError=nError;
      SMXUnLockObject((SMX_NODE*)pOperation);
   }
   if (nError==SM_SUCCESS)
   {
      TRACE_INFO("SMStubPerformOperation(0x%X): Success",
         hOperation);
   }
   else
   {
      TRACE_INFO("SMStubPerformOperation(0x%X): Error 0x%X",
            hOperation, nError);
   }

   return nError;
}


/*----------------------------------------------------------------------------
 * Stub Start Operation Async
 *----------------------------------------------------------------------------*/

SM_EXPORT void SMStubStartOperationAsync(
      SM_HANDLE  hOperation,
      uint32_t   nReserved,
      void*      pSyncObj)
{
   SM_ERROR nError;
   SM_OPERATION* pOperation;

   /* This function does nothing because asynchronous operations are not supported */

   MD_VAR_NOT_USED(nReserved)
   MD_VAR_NOT_USED(pSyncObj)

   TRACE_INFO("SMStubStartOperationAsync(0x%X, 0x%X, %p)",
         hOperation, nReserved, pSyncObj);

   nError=SMXLockOperationFromHandle(
      hOperation,
      &pOperation,
      SM_ACTION_PERFORM);
   if (nError!=SM_SUCCESS)
   {
      goto error;
   }
   pOperation->nOperationState = TFAPI_OPERATION_STATE_RECEIVED;
   pOperation->nOperationError = SM_ERROR_ASYNC_OPERATIONS_NOT_SUPPORTED;

error:
   SMXUnLockObject((SMX_NODE*)pOperation);
   if (nError==SM_SUCCESS)
   {
      TRACE_INFO("SMStubStartOperationAsync(0x%X): Success",
         hOperation);
   }
   else
   {
      TRACE_INFO("SMStubStartOperationAsync(0x%X): Error 0x%X",
            hOperation, nError);
   }
}


/*----------------------------------------------------------------------------
 * Stub Get Operation Async Result
 *----------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMStubGetOperationAsyncResult(
      SM_HANDLE  hOperation,
      SM_ERROR*  pnServiceErrorCode,
      SM_HANDLE* phAnswerDecoder)
{
   MD_VAR_NOT_USED(hOperation)
   MD_VAR_NOT_USED(pnServiceErrorCode)
   MD_VAR_NOT_USED(phAnswerDecoder)
   return SM_ERROR_ASYNC_OPERATIONS_NOT_SUPPORTED;
}


/*----------------------------------------------------------------------------
 * Stub Cancel Operation
 *----------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMStubCancelOperation(
      SM_HANDLE hOperation)
{
   SM_ERROR nError=SM_SUCCESS;
   SM_DEVICE_CONTEXT* pDevice;
   SM_CLIENT_SESSION* pSession;
   SM_OPERATION* pOperation;

   TRACE_INFO("SMStubCancelOperation(0x%X)", hOperation);

   nError=SMXLockOperationFromHandle(hOperation,&pOperation,SM_ACTION_CANCEL);
   if (nError!=SM_SUCCESS)
   {
      goto error;
   }
   pSession = pOperation->pSession;
   pDevice  = pSession->pDeviceContext;
   if (pOperation->nMessageType==TFAPI_CLOSE_CLIENT_SESSION)
   {
      /* Do nothing */
      goto error;
   }

   if (pOperation->nOperationState==TFAPI_OPERATION_STATE_INVOKE)
   {
      SMXUnLockCriticalSection(&pDevice->sCriticalSection);

      TEEC_RequestCancellation(&pOperation->sTeecOperation);

      SMXLockCriticalSection(&pDevice->sCriticalSection);
   }
error:
   SMXUnLockObject((SMX_NODE*)pOperation);
   if (nError==SM_SUCCESS)
   {
      TRACE_INFO("SMStubCancelOperation(0x%X): Success",
         hOperation);
   }
   else
   {
      TRACE_INFO("SMStubCancelOperation(0x%X): Error 0x%X",
         hOperation, nError);
   }
   return nError;
}


/*----------------------------------------------------------------------------
 * Stub Release Operation
 *----------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMStubReleaseOperation(
      SM_HANDLE hOperation)
{
   SM_ERROR nError=SM_SUCCESS;
   SM_DEVICE_CONTEXT* pDevice=NULL;
   SM_CLIENT_SESSION* pSession;
   SM_OPERATION* pOperation;
   uint8_t i;

   /**
    * hOperation == SM_HANDLE_INVALID is allowed.
    */
   if(hOperation == SM_HANDLE_INVALID)
   {
      return SM_SUCCESS;
   }

   nError=SMXLockOperationFromHandle(hOperation,&pOperation,SM_ACTION_RELEASE);
   if (nError!=SM_SUCCESS)
   {
      goto error;
   }
   if (pOperation==NULL)
   {
      nError=SM_ERROR_ILLEGAL_ARGUMENT;
      goto error;
   }
   pSession=pOperation->pSession;
   pDevice=pSession->pDeviceContext;

   SMXEncoderUninit(&pOperation->sEncoder);
   SMXDecoderDestroy(&pOperation->sDecoder, false);

   /* all shared memory associated with the operation are dereferenced */
   for (i=0; i<TFAPI_SHARED_MEMORY_REFERENCE_MAX_COUNT; i++)
   {
      if (pOperation->sPreallocatedSharedMemoryReference[i].pShMem != NULL)
      {
         SMX_SHARED_MEMORY_REF* pSharedMemoryReference;
         pSharedMemoryReference = &pOperation->sPreallocatedSharedMemoryReference[i];

         if (pSharedMemoryReference->pShMem->nReferenceCount>1)
         {
            pSharedMemoryReference->pShMem->nReferenceCount--;
         }
         memset(pSharedMemoryReference, 0, sizeof(SMX_SHARED_MEMORY_REF));
      }
   }

   if (pOperation->sNode.nType!=TFAPI_OBJECT_TYPE_INNER_OPERATION)
   {
      SMXDeviceContextFree(pDevice,(SMX_NODE*)pOperation);
      pOperation=NULL;
   }
   else
   {
      /* make inner operation available */
      pOperation->nOperationState=TFAPI_OPERATION_STATE_UNUSED;
   }
   if (pSession->nReferenceCount==0)
   {
      SMXDeviceContextFree(pDevice,(SMX_NODE*)pSession);
   }
   SMXUnLockCriticalSection(&pDevice->sCriticalSection);
error:
   return nError;
}


/*----------------------------------------------------------------------------
 * Encoder
 *----------------------------------------------------------------------------*/

SM_EXPORT void SMStubEncoderWriteUint8(
      SM_HANDLE hEncoder,
      uint8_t   nValue)
{
   SM_ERROR nError;
   SMX_ENCODER* pEncoder;
   LIB_BEF_ENCODER* pBEFEncoder;

   nError = SMXLockObjectFromHandle(
         hEncoder,
         (void*)&pEncoder,
         TFAPI_OBJECT_TYPE_ENCODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }
   pBEFEncoder=&pEncoder->sBEFEncoder;

   libBEFEncoderWriteUint8(pBEFEncoder, nValue);
   if (SMXEncoderBufferReAllocated(pEncoder))
   {
      libBEFEncoderWriteUint8(pBEFEncoder, nValue);
   }

error:
   SMXUnLockObject((SMX_NODE*)pEncoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubEncoderWriteUint8(0x%X): failed (error 0x%X)",
         hEncoder, nError);
   }
}

/* ------------------------------------------------------------------------ */

SM_EXPORT void SMStubEncoderWriteUint16(
      SM_HANDLE hEncoder,
      uint16_t  nValue)
{
   SM_ERROR nError;
   SMX_ENCODER* pEncoder;
   LIB_BEF_ENCODER* pBEFEncoder;

   nError = SMXLockObjectFromHandle(
         hEncoder,
         (void*)&pEncoder,
         TFAPI_OBJECT_TYPE_ENCODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }
   pBEFEncoder=&pEncoder->sBEFEncoder;

   libBEFEncoderWriteUint16(pBEFEncoder, nValue);
   if (SMXEncoderBufferReAllocated(pEncoder))
   {
      libBEFEncoderWriteUint16(pBEFEncoder, nValue);
   }

error:
   SMXUnLockObject((SMX_NODE*)pEncoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubEncoderWriteUint16(0x%X): failed (error 0x%X)",
         hEncoder, nError);
   }
}

/* ------------------------------------------------------------------------ */

SM_EXPORT void SMStubEncoderWriteUint32(
      SM_HANDLE hEncoder,
      uint32_t  nValue)
{
   SM_ERROR nError;
   SMX_ENCODER* pEncoder;
   LIB_BEF_ENCODER* pBEFEncoder;

   nError = SMXLockObjectFromHandle(
         hEncoder,
         (void*)&pEncoder,
         TFAPI_OBJECT_TYPE_ENCODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   pBEFEncoder=&pEncoder->sBEFEncoder;

   libBEFEncoderWriteUint32(pBEFEncoder, nValue);
   if (SMXEncoderBufferReAllocated(pEncoder))
   {
      libBEFEncoderWriteUint32(pBEFEncoder, nValue);
   }

error:
   SMXUnLockObject((SMX_NODE*)pEncoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubEncoderWriteUint32(0x%X): failed (error 0x%X)",
         hEncoder, nError);
   }
}

/* ------------------------------------------------------------------------ */

SM_EXPORT void SMStubEncoderWriteBoolean(
      SM_HANDLE hEncoder,
      bool      nValue)
{
   SM_ERROR nError;
   SMX_ENCODER* pEncoder;
   LIB_BEF_ENCODER* pBEFEncoder;

   nError = SMXLockObjectFromHandle(
         hEncoder,
         (void*)&pEncoder,
         TFAPI_OBJECT_TYPE_ENCODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   pBEFEncoder=&pEncoder->sBEFEncoder;

   libBEFEncoderWriteBoolean(pBEFEncoder, nValue);
   if (SMXEncoderBufferReAllocated(pEncoder))
   {
      libBEFEncoderWriteBoolean(pBEFEncoder, nValue);
   }

error:
   SMXUnLockObject((SMX_NODE*)pEncoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubEncoderWriteBoolean(0x%X): failed (error 0x%X)",
         hEncoder, nError);
   }
}

/* ------------------------------------------------------------------------ */

SM_EXPORT void SMStubEncoderWriteHandle(
      SM_HANDLE hEncoder,
      SM_HANDLE hValue)
{
   SM_ERROR nError;
   SMX_ENCODER* pEncoder;
   LIB_BEF_ENCODER* pBEFEncoder;

   nError = SMXLockObjectFromHandle(
         hEncoder,
         (void*)&pEncoder,
         TFAPI_OBJECT_TYPE_ENCODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   pBEFEncoder=&pEncoder->sBEFEncoder;

   libBEFEncoderWriteHandle(pBEFEncoder, hValue);
   if (SMXEncoderBufferReAllocated(pEncoder))
   {
      libBEFEncoderWriteHandle(pBEFEncoder, hValue);
   }

error:
   SMXUnLockObject((SMX_NODE*)pEncoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubEncoderWriteHandle(0x%X): failed (error 0x%X)",
         hEncoder, nError);
   }
}

/* ------------------------------------------------------------------------ */

SM_EXPORT void SMStubEncoderWriteString(
      SM_HANDLE      hEncoder,
      const wchar_t* pValue)
{
   SM_ERROR nError;
   SMX_ENCODER* pEncoder;
   LIB_BEF_ENCODER* pBEFEncoder;

   nError = SMXLockObjectFromHandle(
         hEncoder,
         (void*)&pEncoder,
         TFAPI_OBJECT_TYPE_ENCODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   pBEFEncoder=&pEncoder->sBEFEncoder;

   libBEFEncoderWriteWString(pBEFEncoder, pValue);
   if (SMXEncoderBufferReAllocated(pEncoder))
   {
      libBEFEncoderWriteWString(pBEFEncoder, pValue);
   }

error:
   SMXUnLockObject((SMX_NODE*)pEncoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubEncoderWriteString(0x%X): failed (error 0x%X)",
         hEncoder, nError);
   }
}

/* ------------------------------------------------------------------------ */

SM_EXPORT void SMStubEncoderWriteUint8Array(
      SM_HANDLE      hEncoder,
      uint32_t       nArrayLength,
      const uint8_t* pnArray)
{
   SM_ERROR nError;
   SMX_ENCODER* pEncoder;
   LIB_BEF_ENCODER* pBEFEncoder;

   nError = SMXLockObjectFromHandle(
         hEncoder,
         (void*)&pEncoder,
         TFAPI_OBJECT_TYPE_ENCODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   pBEFEncoder=&pEncoder->sBEFEncoder;

   libBEFEncoderWriteUint8Array(pBEFEncoder, nArrayLength, pnArray);
   if (SMXEncoderBufferReAllocated(pEncoder))
   {
      libBEFEncoderWriteUint8Array(pBEFEncoder, nArrayLength, pnArray);
   }

error:
   SMXUnLockObject((SMX_NODE*)pEncoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubEncoderWriteUint8Array(0x%X): failed (error 0x%X)",
         hEncoder, nError);
   }
}

/* ------------------------------------------------------------------------ */

SM_EXPORT void SMStubEncoderWriteUint16Array(
      SM_HANDLE       hEncoder,
      uint32_t        nArrayLength,
      const uint16_t* pnArray)
{
   SM_ERROR nError;
   SMX_ENCODER* pEncoder;
   LIB_BEF_ENCODER* pBEFEncoder;

   nError = SMXLockObjectFromHandle(
         hEncoder,
         (void*)&pEncoder,
         TFAPI_OBJECT_TYPE_ENCODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   pBEFEncoder=&pEncoder->sBEFEncoder;

   libBEFEncoderWriteUint16Array(pBEFEncoder, nArrayLength, pnArray);
   if (SMXEncoderBufferReAllocated(pEncoder))
   {
      libBEFEncoderWriteUint16Array(pBEFEncoder, nArrayLength, pnArray);
   }

error:
   SMXUnLockObject((SMX_NODE*)pEncoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubEncoderWriteUint16Array(0x%X): failed (error 0x%X)",
         hEncoder, nError);
   }
}

/* ------------------------------------------------------------------------ */

SM_EXPORT void SMStubEncoderWriteUint32Array(
      SM_HANDLE       hEncoder,
      uint32_t        nArrayLength,
      const uint32_t* pnArray)
{
   SM_ERROR nError;
   SMX_ENCODER* pEncoder;
   LIB_BEF_ENCODER* pBEFEncoder;

   nError = SMXLockObjectFromHandle(
         hEncoder,
         (void*)&pEncoder,
         TFAPI_OBJECT_TYPE_ENCODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   pBEFEncoder=&pEncoder->sBEFEncoder;

   libBEFEncoderWriteUint32Array(pBEFEncoder, nArrayLength, pnArray);
   if (SMXEncoderBufferReAllocated(pEncoder))
   {
      libBEFEncoderWriteUint32Array(pBEFEncoder, nArrayLength, pnArray);
   }

error:
   SMXUnLockObject((SMX_NODE*)pEncoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubEncoderWriteUint32Array(0x%X): failed (error 0x%X)",
         hEncoder, nError);
   }
}

/* ------------------------------------------------------------------------ */

SM_EXPORT void SMStubEncoderWriteHandleArray(
      SM_HANDLE        hEncoder,
      uint32_t         nArrayLength,
      const SM_HANDLE* pnArray)
{
   SM_ERROR nError;
   SMX_ENCODER* pEncoder;
   LIB_BEF_ENCODER* pBEFEncoder;

   nError = SMXLockObjectFromHandle(
         hEncoder,
         (void*)&pEncoder,
         TFAPI_OBJECT_TYPE_ENCODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   pBEFEncoder=&pEncoder->sBEFEncoder;

   libBEFEncoderWriteHandleArray(pBEFEncoder, nArrayLength, pnArray);
   if (SMXEncoderBufferReAllocated(pEncoder))
   {
      libBEFEncoderWriteHandleArray(pBEFEncoder, nArrayLength, pnArray);
   }

error:
   SMXUnLockObject((SMX_NODE*)pEncoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubEncoderWriteHandleArray(0x%X): failed (error 0x%X)",
         hEncoder, nError);
   }
}

/* ------------------------------------------------------------------------ */


SM_EXPORT void SMStubEncoderWriteMemoryReference(
      SM_HANDLE hEncoder,
      SM_HANDLE hBlock,
      uint32_t  nOffset,
      uint32_t  nLength,
      uint32_t  nFlags)
{
   SM_ERROR nError=SM_SUCCESS;
   SMX_ENCODER* pEncoder=NULL;
   SM_SHARED_MEMORY* pShMem=NULL;
   SM_OPERATION* pOperation=NULL;
   SM_DEVICE_CONTEXT* pDevice=NULL;
   LIB_BEF_ENCODER* pBEFEncoder=NULL;
   uint8_t i;
   uint32_t nSlot=0;

   nError = SMXLockObjectFromHandle(
         hEncoder,
         (void*)&pEncoder,
         TFAPI_OBJECT_TYPE_ENCODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   if (hBlock == SM_HANDLE_INVALID)
   {
      TRACE_ERROR("SMStubEncoderWriteHandleArray(0x%X): Invalid block handle",
            hEncoder);
      nError=SM_ERROR_ILLEGAL_ARGUMENT;
      goto error;
   }

   nError = SMXLockObjectFromHandle(
         hBlock,
         (void*)&pShMem,
         TFAPI_OBJECT_TYPE_SHARED_MEMORY);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   switch (nFlags)
   {
      case SM_MEMORY_ACCESS_CLIENT_WRITE_SERVICE_READ:
      case SM_MEMORY_ACCESS_CLIENT_READ_SERVICE_WRITE:
      case SM_MEMORY_ACCESS_CLIENT_WRITE_SERVICE_READ
            | SM_MEMORY_ACCESS_CLIENT_READ_SERVICE_WRITE:
         if ((pShMem->nFlags & nFlags)==0)
         {
            TRACE_ERROR("SMStubEncoderWriteMemoryReference(0x%X): Invalid flags",
                  hEncoder);
            nError=SM_ERROR_ILLEGAL_ARGUMENT;
            goto error;
         }
         break;

      default:
         TRACE_ERROR("SMStubEncoderWriteMemoryReference(0x%X): Invalid flags",
               hEncoder);
         nError=SM_ERROR_ILLEGAL_ARGUMENT;
         goto error;
   }
   pOperation=pEncoder->pOperation;
   pDevice=pOperation->pSession->pDeviceContext;

   if (pShMem->pSession != pOperation->pSession
      || nLength > pShMem->nLength
      || nOffset > pShMem->nLength
      || nOffset+nLength > pShMem->nLength)
   {
      nError=SM_ERROR_ILLEGAL_ARGUMENT;
      goto error;
   }

   /* Look for a free mem ref */
   for (i=0; i<TFAPI_SHARED_MEMORY_REFERENCE_MAX_COUNT; i++)
   {
      if (pOperation->sPreallocatedSharedMemoryReference[i].pShMem == NULL)
      {
         SMX_SHARED_MEMORY_REF* pSharedMemoryReference;
         pSharedMemoryReference = &pOperation->sPreallocatedSharedMemoryReference[i];

         pSharedMemoryReference->nFlags=nFlags;
         pSharedMemoryReference->nLength=nLength;
         pSharedMemoryReference->nOffset=nOffset;
         pSharedMemoryReference->pShMem=pShMem;
         nSlot = i+2; /* i can be 0 or 1, nSlot can be 2 or 3 */
         break;
      }
   }
   if (i == TFAPI_SHARED_MEMORY_REFERENCE_MAX_COUNT)
   {
      nError = SM_ERROR_OUT_OF_MEMORY;
      goto error;
   }

   pBEFEncoder=&pEncoder->sBEFEncoder;

   libBEFEncoderWriteMemoryReference(pBEFEncoder,
      nSlot,
      0,
      nLength,
      nFlags);
   if (SMXEncoderBufferReAllocated(pEncoder))
   {
      libBEFEncoderWriteMemoryReference(pBEFEncoder,
         nSlot,
         0,
         nLength,
         nFlags);
   }

error:
   if (nError!=SM_SUCCESS)
   {
      SMXEncoderSetError(pEncoder, nError);
      if (pShMem!=NULL)
      {
         SMXUnLockObject((SMX_NODE*)pShMem);
      }
      TRACE_ERROR("SMStubEncoderWriteMemoryReference(0x%X): failed (error 0x%X)",
         hEncoder, nError);
   }
   SMXUnLockObject((SMX_NODE*)pEncoder);
}

/* ------------------------------------------------------------------------ */

SM_EXPORT void SMStubEncoderOpenSequence(
      SM_HANDLE hEncoder)
{
   SM_ERROR nError;
   SMX_ENCODER* pEncoder;

   nError = SMXLockObjectFromHandle(
         hEncoder,
         (void*)&pEncoder,
         TFAPI_OBJECT_TYPE_ENCODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   libBEFEncoderOpenSequence(&pEncoder->sBEFEncoder);
   if (SMXEncoderBufferReAllocated(pEncoder))
   {
      libBEFEncoderOpenSequence(&pEncoder->sBEFEncoder);
   }

error:
   SMXUnLockObject((SMX_NODE*)pEncoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubEncoderOpenSequence(0x%X): failed (error 0x%X)",
         hEncoder, nError);
   }
}

/* ------------------------------------------------------------------------ */

SM_EXPORT void SMStubEncoderCloseSequence(
      SM_HANDLE hEncoder)
{
   SM_ERROR nError;
   SMX_ENCODER* pEncoder;

   nError = SMXLockObjectFromHandle(
         hEncoder,
         (void*)&pEncoder,
         TFAPI_OBJECT_TYPE_ENCODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   libBEFEncoderCloseSequence(&pEncoder->sBEFEncoder);

error:
   SMXUnLockObject((SMX_NODE*)pEncoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubEncoderCloseSequence(0x%X): failed (error 0x%X)",
         hEncoder, nError);
   }
}


/*----------------------------------------------------------------------------
 * Decoder
 *----------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMStubDecoderGetError(
      SM_HANDLE hDecoder)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   nError = SMXDecoderGetError(pDecoder);

   SMXUnLockObject((SMX_NODE*)pDecoder);

   return nError;

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   TRACE_ERROR("SMStubEncoderOpenSequence(0x%X): failed (error 0x%X)",
      hDecoder, nError);
   return nError;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT bool SMStubDecoderHasData(
      SM_HANDLE hDecoder)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;
   bool bHasData=false;

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   bHasData = libBEFDecoderHasData(&pDecoder->sBEFDecoder);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderHasData(0x%X): failed (error 0x%X)",
            hDecoder, nError);
   }
   return bHasData;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT uint8_t SMStubDecoderReadUint8(
      SM_HANDLE hDecoder)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;
   uint8_t nValue=0;

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   nValue = libBEFDecoderReadUint8(&pDecoder->sBEFDecoder);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderReadUint8(0x%X): failed (error 0x%X)",
            hDecoder, nError);
   }
   return nValue;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT uint16_t SMStubDecoderReadUint16(
      SM_HANDLE hDecoder)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;
   uint16_t nValue=0;

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   nValue = libBEFDecoderReadUint16(&pDecoder->sBEFDecoder);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderReadUint16(0x%X): failed (error 0x%X)",
            hDecoder, nError);
   }
   return nValue;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT uint32_t SMStubDecoderReadUint32(
      SM_HANDLE hDecoder)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;
   uint32_t nValue=0;

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   nValue = libBEFDecoderReadUint32(&pDecoder->sBEFDecoder);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderReadUint32(0x%X): failed (error 0x%X)",
            hDecoder, nError);
   }
   return nValue;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT bool SMStubDecoderReadBoolean(
      SM_HANDLE hDecoder)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;
   bool bValue=false;

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   bValue = libBEFDecoderReadBoolean(&pDecoder->sBEFDecoder);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderReadBoolean(0x%X): failed (error 0x%X)",
            hDecoder, nError);
   }
   return bValue;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT SM_HANDLE SMStubDecoderReadHandle(
      SM_HANDLE hDecoder)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;
   SM_HANDLE hValue=SM_HANDLE_INVALID;

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   hValue = libBEFDecoderReadHandle(&pDecoder->sBEFDecoder);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderReadHandle(0x%X): failed (error 0x%X)",
            hDecoder, nError);
   }
   return hValue;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT wchar_t* SMStubDecoderReadString(
      SM_HANDLE hDecoder)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;
   SMX_NODE* pNode;
   wchar_t* pValue=NULL;
   SM_DEVICE_CONTEXT *pDevice;
   uint32_t nLength;

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }
   pDevice = pDecoder->pOperation->pSession->pDeviceContext;

   nLength = libBEFDecoderReadWStringLength(&pDecoder->sBEFDecoder);
   if(pDecoder->sBEFDecoder.nError != S_SUCCESS)
   {
      nError = pDecoder->sBEFDecoder.nError;
      goto error;
   }
   if (nLength != LIB_BEF_NULL_ELEMENT)
   {
      pNode = (SMX_NODE*)SMXDeviceContextAlloc(pDevice,
         nLength*sizeof(wchar_t)+sizeof(SMX_NODE));
      if (pNode==NULL)
      {
         nError = SM_ERROR_OUT_OF_MEMORY;
         pDecoder->sBEFDecoder.nError = nError;
         goto error;
      }
      pValue=(wchar_t*)((SMX_BUFFER*)pNode)->buffer;

      libBEFDecoderCopyStringAsWchar(
            &pDecoder->sBEFDecoder,
            pValue);
      if(pDecoder->sBEFDecoder.nError != S_SUCCESS)
      {
         nError = pDecoder->sBEFDecoder.nError;
         pValue=NULL;
         SMXDeviceContextFree(pDevice,pNode);
         goto error;
      }
   }
   libBEFDecoderSkip(&pDecoder->sBEFDecoder);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);

   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderReadString(0x%X): failed (error 0x%X)",
            hDecoder, nError);
   }
   return pValue;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT uint32_t SMStubDecoderReadArrayLength(
      SM_HANDLE hDecoder)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;
   uint32_t nValue=0;

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   nValue = libBEFDecoderReadArrayLength(&pDecoder->sBEFDecoder);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderReadArrayLength(0x%X): failed (error 0x%X)",
            hDecoder, nError);
   }
   return nValue;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT uint32_t SMStubDecoderCopyUint8Array(
      SM_HANDLE hDecoder,
      uint32_t  nIndex,
      uint32_t  nMaxLength,
      uint8_t*  pArray)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;
   SM_DEVICE_CONTEXT* pDevice;
   uint32_t nCopyLength;

   if (pArray==NULL)
   {
      return 0;
   }

   nCopyLength = SM_NULL_ELEMENT;

   if (nMaxLength == LIB_BEF_NULL_ELEMENT)
   {
      return 0;
   }

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   pDevice = pDecoder->pOperation->pSession->pDeviceContext;

   nCopyLength = libBEFDecoderCopyUint8Array(
      &pDecoder->sBEFDecoder,
      nIndex,
      nMaxLength,
      pArray);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError != SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderCopyUint8Array(0x%X): failed (error 0x%X)",
         hDecoder, nError);
   }
   return nCopyLength;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT uint32_t SMStubDecoderCopyUint16Array(
      SM_HANDLE hDecoder,
      uint32_t  nIndex,
      uint32_t  nMaxLength,
      uint16_t* pArray)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;
   SM_DEVICE_CONTEXT* pDevice;
   uint32_t nCopyLength;

   if (pArray==NULL)
   {
      return 0;
   }

   nCopyLength = SM_NULL_ELEMENT;

   if (nMaxLength == LIB_BEF_NULL_ELEMENT)
   {
      return 0;
   }

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   pDevice = pDecoder->pOperation->pSession->pDeviceContext;

   nCopyLength = libBEFDecoderCopyUint16Array(
      &pDecoder->sBEFDecoder,
      nIndex,
      nMaxLength,
      pArray);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError != SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderCopyUint16Array(0x%X): failed (error 0x%X)",
         hDecoder, nError);
   }

   return nCopyLength;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT uint32_t SMStubDecoderCopyUint32Array(
      SM_HANDLE hDecoder,
      uint32_t  nIndex,
      uint32_t  nMaxLength,
      uint32_t* pArray)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;
   SM_DEVICE_CONTEXT* pDevice;
   uint32_t nCopyLength;

   if (pArray==NULL)
   {
      return 0;
   }

   nCopyLength = SM_NULL_ELEMENT;

   if (nMaxLength == LIB_BEF_NULL_ELEMENT)
   {
      return 0;
   }

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   pDevice = pDecoder->pOperation->pSession->pDeviceContext;

   nCopyLength = libBEFDecoderCopyUint32Array(
      &pDecoder->sBEFDecoder,
      nIndex,
      nMaxLength,
      pArray);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError != SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderCopyUint32Array(0x%X): failed (error 0x%X)",
         hDecoder, nError);
   }
   return nCopyLength;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT uint32_t SMStubDecoderCopyHandleArray(
      SM_HANDLE  hDecoder,
      uint32_t   nIndex,
      uint32_t   nMaxLength,
      SM_HANDLE* pArray)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;
   SM_DEVICE_CONTEXT* pDevice;
   uint32_t nCopyLength;

   if (pArray==NULL)
   {
      return 0;
   }

   nCopyLength = SM_NULL_ELEMENT;

   if (nMaxLength == LIB_BEF_NULL_ELEMENT)
   {
      return 0;
   }

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   pDevice = pDecoder->pOperation->pSession->pDeviceContext;

   nCopyLength = libBEFDecoderCopyHandleArray(
      &pDecoder->sBEFDecoder,
      nIndex,
      nMaxLength,
      pArray);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError != SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderCopyHandleArray(0x%X): failed (error 0x%X)",
         hDecoder, nError);
   }
   return nCopyLength;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT uint8_t* SMStubDecoderReadUint8Array(
      SM_HANDLE hDecoder,
      uint32_t* pnArrayLength)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;
   SM_DEVICE_CONTEXT* pDevice;
   SMX_NODE* pNode;
   uint8_t* pArray;
   uint32_t nArrayLength;

   assert(pnArrayLength!=NULL);

   *pnArrayLength = SM_NULL_ELEMENT;
   pArray = NULL;

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   pDevice = pDecoder->pOperation->pSession->pDeviceContext;

   nArrayLength=libBEFDecoderReadArrayLength(&pDecoder->sBEFDecoder);
   if (nArrayLength != LIB_BEF_NULL_ELEMENT)
   {
      pNode= SMXDeviceContextAlloc(pDevice,nArrayLength+sizeof(SMX_NODE));
      if (pNode==NULL)
      {
         nError=SM_ERROR_OUT_OF_MEMORY;
         pDecoder->sBEFDecoder.nError = nError;
         goto error;
      }
      pArray=(uint8_t*)((SMX_BUFFER*)pNode)->buffer;

      libBEFDecoderCopyUint8Array(
         &pDecoder->sBEFDecoder,
         0,
         nArrayLength,
         pArray);

      if(pDecoder->sBEFDecoder.nError == S_SUCCESS)
      {
         *pnArrayLength=nArrayLength;
      }
      else
      {
         nError = pDecoder->sBEFDecoder.nError;
         SMXDeviceContextFree(pDevice, pNode);
         pArray = NULL;
      }
   }

   libBEFDecoderSkip(&pDecoder->sBEFDecoder);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError != SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderReadUint8Array(0x%X): failed (error 0x%X)",
         hDecoder, nError);
   }
   return pArray;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT uint16_t* SMStubDecoderReadUint16Array(
      SM_HANDLE hDecoder,
      uint32_t* pnArrayLength)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;
   SM_DEVICE_CONTEXT* pDevice;
   SMX_NODE* pNode;
   uint16_t* pArray;
   uint32_t nArrayLength;

   assert(pnArrayLength!=NULL);

   *pnArrayLength = SM_NULL_ELEMENT;
   pArray = NULL;

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   pDevice = pDecoder->pOperation->pSession->pDeviceContext;

   nArrayLength=libBEFDecoderReadArrayLength(&pDecoder->sBEFDecoder);
   if (nArrayLength != LIB_BEF_NULL_ELEMENT)
   {
      pNode = (SMX_NODE*)SMXDeviceContextAlloc(pDevice,(nArrayLength<<1)+sizeof(SMX_NODE));
      if (pNode==NULL)
      {
         nError=SM_ERROR_OUT_OF_MEMORY;
         pDecoder->sBEFDecoder.nError = nError;
         goto error;
      }
      pArray=(uint16_t*)((SMX_BUFFER*)pNode)->buffer;

      libBEFDecoderCopyUint16Array(
         &pDecoder->sBEFDecoder,
         0,
         nArrayLength,
         pArray);
      if(pDecoder->sBEFDecoder.nError == S_SUCCESS)
      {
         *pnArrayLength=nArrayLength;
      }
      else
      {
         nError = pDecoder->sBEFDecoder.nError;
         SMXDeviceContextFree(pDevice, pNode);
         pArray = NULL;
      }
   }

   libBEFDecoderSkip(&pDecoder->sBEFDecoder);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderReadUint16Array(0x%X): failed (error 0x%X)",
         hDecoder, nError);
   }
   return pArray;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT uint32_t* SMStubDecoderReadUint32Array(
      SM_HANDLE hDecoder,
      uint32_t* pnArrayLength)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;
   SM_DEVICE_CONTEXT* pDevice;
   SMX_NODE* pNode;
   uint32_t* pArray;
   uint32_t nArrayLength;

   assert(pnArrayLength!=NULL);

   *pnArrayLength = SM_NULL_ELEMENT;
   pArray = NULL;

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   pDevice = pDecoder->pOperation->pSession->pDeviceContext;

   nArrayLength=libBEFDecoderReadArrayLength(&pDecoder->sBEFDecoder);
   if (nArrayLength != LIB_BEF_NULL_ELEMENT)
   {
      pNode = (SMX_NODE*)SMXDeviceContextAlloc(pDevice,(nArrayLength<<2)+sizeof(SMX_NODE));
      if (pNode==NULL)
      {
         nError=SM_ERROR_OUT_OF_MEMORY;
         pDecoder->sBEFDecoder.nError = nError;
         goto error;
      }
      pArray=(uint32_t*)((SMX_BUFFER*)pNode)->buffer;

      libBEFDecoderCopyUint32Array(
         &pDecoder->sBEFDecoder,
         0,
         nArrayLength,
         pArray);
      if(pDecoder->sBEFDecoder.nError == S_SUCCESS)
      {
         *pnArrayLength=nArrayLength;
      }
      else
      {
         nError = pDecoder->sBEFDecoder.nError;
         SMXDeviceContextFree(pDevice, pNode);
         pArray = NULL;
      }
   }

   libBEFDecoderSkip(&pDecoder->sBEFDecoder);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderReadUint32Array(0x%X): failed (error 0x%X)",
         hDecoder, nError);
   }
   return pArray;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT SM_HANDLE* SMStubDecoderReadHandleArray(
      SM_HANDLE hDecoder,
      uint32_t* pnArrayLength)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;
   SM_DEVICE_CONTEXT* pDevice;
   SMX_NODE* pNode;
   SM_HANDLE* pArray;
   uint32_t nArrayLength;

   assert(pnArrayLength!=NULL);

   *pnArrayLength = SM_NULL_ELEMENT;
   pArray = NULL;

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   pDevice = pDecoder->pOperation->pSession->pDeviceContext;

   nArrayLength=libBEFDecoderReadArrayLength(&pDecoder->sBEFDecoder);
   if (nArrayLength != LIB_BEF_NULL_ELEMENT)
   {
      pNode = (SMX_NODE*)SMXDeviceContextAlloc(pDevice,(nArrayLength<<2)+sizeof(SMX_NODE));
      if (pNode==NULL)
      {
         nError=SM_ERROR_OUT_OF_MEMORY;
         goto error;
      }
      pArray=(SM_HANDLE*)((SMX_BUFFER*)pNode)->buffer;

      libBEFDecoderCopyHandleArray(
         &pDecoder->sBEFDecoder,
         0,
         nArrayLength,
         pArray);

      if(pDecoder->sBEFDecoder.nError == S_SUCCESS)
      {
         *pnArrayLength=nArrayLength;
      }
      else
      {
         nError = pDecoder->sBEFDecoder.nError;
         SMXDeviceContextFree(pDevice, pNode);
         pArray = NULL;
      }
   }

   libBEFDecoderSkip(&pDecoder->sBEFDecoder);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderReadHandleArray(0x%X): failed (error 0x%X)",
         hDecoder, nError);
   }
   return pArray;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT void SMStubDecoderReadSequence(
      SM_HANDLE  hDecoder,
      SM_HANDLE* phSequenceDecoder)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder = NULL;
   SM_DEVICE_CONTEXT* pDevice;
   SMX_DECODER* pSequenceDecoder=NULL;

   if (phSequenceDecoder==NULL)
   {
      nError=SM_ERROR_ILLEGAL_ARGUMENT;
      goto error;
   }

   *phSequenceDecoder = S_HANDLE_NULL;

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   if (pDecoder->sBEFDecoder.nError != SM_SUCCESS)
   {
      /* Error state set on entry: do nothing */
      nError = pDecoder->sBEFDecoder.nError;
      goto error;
   }

   /*
    * Allocate and initialize the decoder structure.
    */
   pDevice = pDecoder->pOperation->pSession->pDeviceContext;

   nError = SMXDecoderCreate(
               pDecoder->pOperation,
               pDecoder->sBEFDecoder.pEncodedData,
               pDecoder->sBEFDecoder.nEncodedSize,
               &pSequenceDecoder);
   if (nError != SM_SUCCESS)
   {
      SMXDecoderSetError(pDecoder, nError);
      goto error;
   }

   pSequenceDecoder->pNextSibling=pDecoder->pNextSibling;
   if (pDecoder->pNextSibling!=NULL)
   {
      pDecoder->pNextSibling->pParentDecoder=pSequenceDecoder;
   }
   pSequenceDecoder->pParentDecoder=pDecoder;
   pDecoder->pNextSibling=pSequenceDecoder;

   libBEFDecoderReadSequence(
      &pDecoder->sBEFDecoder,
      &pSequenceDecoder->sBEFDecoder);
   if(pDecoder->sBEFDecoder.nError == S_SUCCESS)
   {
      *phSequenceDecoder = SMXGetHandleFromPointer(pSequenceDecoder);
   }
   else
   {
      TRACE_ERROR("SMStubDecoderReadSequence(0x%X): failed (error 0x%X)",
            hDecoder, pDecoder->sBEFDecoder.nError);
      SMXDecoderDestroy(pSequenceDecoder, true);
   }

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError != SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderReadSequence(0x%X): failed (error 0x%X)",
            hDecoder, nError);
   }
}

SM_EXPORT void SMStubDecoderOpenSequence(
      SM_HANDLE hDecoder)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   libBEFDecoderOpenSequence(&pDecoder->sBEFDecoder);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderOpenSequence(0x%X): failed (error 0x%X)",
         hDecoder, nError);
   }
}

SM_EXPORT void SMStubDecoderCloseSequence(
      SM_HANDLE hDecoder)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   libBEFDecoderCloseSequence(&pDecoder->sBEFDecoder);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("libBEFDecoderCloseSequence(0x%X): failed (error 0x%X)",
         hDecoder, nError);
   }
}

/* ------------------------------------------------------------------------ */

SM_EXPORT void SMStubDecoderSkip(
      SM_HANDLE hDecoder)
{
   SM_ERROR nError;
   SMX_DECODER* pDecoder;

   nError = SMXLockObjectFromHandle(
         hDecoder,
         (void*)&pDecoder,
         TFAPI_OBJECT_TYPE_DECODER);
   if (nError != SM_SUCCESS)
   {
      goto error;
   }

   libBEFDecoderSkip(&pDecoder->sBEFDecoder);

error:
   SMXUnLockObject((SMX_NODE*)pDecoder);
   if (nError != SM_SUCCESS)
   {
      TRACE_ERROR("SMStubDecoderSkip(0x%X): failed (error 0x%X)",
            hDecoder, nError);
   }
}


/*----------------------------------------------------------------------------
 * Shared Memory
 *----------------------------------------------------------------------------*/

/* Create or Register a shared memory for a Client Session */
static SM_ERROR static_SMXStubCreateSharedMemory(
      SM_HANDLE  hClientSession,
      void*      pBuffer,
      uint32_t   nLength,
      uint32_t   nFlags,
      SM_HANDLE* phBlockHandle,
      void**     ppBlock)
{
   SM_ERROR nError;
   TEEC_Result nTeeError;
   bool bBadParameters = false;
   SM_DEVICE_CONTEXT* pDevice=NULL;
   SM_CLIENT_SESSION* pClientSession;
   SM_SHARED_MEMORY* pShMem = NULL;

   TRACE_INFO("static_SMXStubCreateSharedMemory(0x%X, %p, %u, 0x%X, %p)",
         hClientSession, pBuffer, nLength, nFlags, phBlockHandle);

   if (phBlockHandle==NULL)
   {
      TRACE_ERROR("SMXStubCreateSharedMemory(0x%X): NULL phBlockHandle",
                     hClientSession);
      bBadParameters = true;
   }

   if (pBuffer == NULL)
   {
      /* Memory Allocation */
      if (ppBlock == NULL)
      {
         TRACE_ERROR("SMXStubCreateSharedMemory(0x%X): NULL pBuffer/ppBlock",
                        hClientSession);
         bBadParameters = true;
      }
   }

   if (ppBlock != NULL)
   {
      *ppBlock = NULL;
   }

   if (nLength == 0)
   {
      TRACE_ERROR("SMXStubCreateSharedMemory(0x%X): Invalid nLength parameter (%u)",
            hClientSession, nLength);
      bBadParameters = true;
   }

   if ((nFlags &
      ~(SM_MEMORY_ACCESS_CLIENT_WRITE_SERVICE_READ |
      SM_MEMORY_ACCESS_CLIENT_READ_SERVICE_WRITE |
      SMX_MEMORY_ACCESS_DIRECT |
      SMX_MEMORY_ACCESS_DIRECT_FORCE))!=0)
   {
      TRACE_ERROR("SMXStubCreateSharedMemory(0x%X): Invalid nFlags parameter (%u)",
         hClientSession, nFlags);
      bBadParameters = true;
   }
   if ((nFlags &
      (SM_MEMORY_ACCESS_CLIENT_WRITE_SERVICE_READ |
      SM_MEMORY_ACCESS_CLIENT_READ_SERVICE_WRITE))==0)
   {
      TRACE_ERROR("SMXStubCreateSharedMemory(0x%X): Invalid nFlags parameter (%u)",
         hClientSession, nFlags);
      bBadParameters = true;
   }

#ifndef LINUX
   /* The flags SMX_MEMORY_ACCESS_DIRECT and SMX_MEMORY_ACCESS_DIRECT_FORCE
      are meaningful only on Linux. For all other implementations, they can
      be ignored */
   nFlags &= ~(SMX_MEMORY_ACCESS_DIRECT | SMX_MEMORY_ACCESS_DIRECT_FORCE);
#endif

   if (bBadParameters != false)
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   nError=SMXLockObjectFromHandle(
      hClientSession,
      (void*)&pClientSession,
      TFAPI_OBJECT_TYPE_SESSION);
   if (nError!=SM_SUCCESS)
   {
      goto error;
   }
   if (pClientSession==NULL)
   {
      nError=SM_ERROR_ILLEGAL_ARGUMENT;
      goto error;
   }
   pDevice=pClientSession->pDeviceContext;

   /*
    * Create the shared memory
    */

   pShMem=(SM_SHARED_MEMORY*)SMXDeviceContextAlloc(pDevice,sizeof(SM_SHARED_MEMORY));
   if (pShMem==NULL)
   {
      nError=SM_ERROR_OUT_OF_MEMORY;
      goto error;
   }
   pShMem->sNode.nType=TFAPI_OBJECT_TYPE_SHARED_MEMORY;
   pShMem->pSession=pClientSession;
   pShMem->nReferenceCount=1;
   pShMem->nFlags=nFlags;
   pShMem->nLength=nLength;
   pShMem->pClientBuffer=pBuffer;

   pShMem->sTeecSharedMem.flags = nFlags;
   pShMem->sTeecSharedMem.size  = nLength;

   if(ppBlock == NULL)
   {
       pShMem->sTeecSharedMem.buffer = pBuffer;
       /* don't allocate, use user sent buffer */
       nTeeError = TEEC_RegisterSharedMemory(&pDevice->sTeecContext,
           &pShMem->sTeecSharedMem);
   }
   else
   {
       nTeeError = TEEC_AllocateSharedMemory(&pDevice->sTeecContext,
           &pShMem->sTeecSharedMem);
   }
   if (nTeeError != TEEC_SUCCESS)
   {
      /* No need to convert TEEC error into SMAPI error */
      nError = nTeeError;
      goto error;
   }

   /*
    * Successful completion.
    */
   pShMem->pBuffer = pShMem->sTeecSharedMem.buffer;

   if (ppBlock!=NULL)
   {
      *ppBlock = pShMem->pBuffer;
   }
   *phBlockHandle = SMXGetHandleFromPointer(pShMem);

   SMXUnLockCriticalSection(&pDevice->sCriticalSection);

   TRACE_INFO("SMXStubCreateSharedMemory(0x%X): Success",
         hClientSession);
   return SM_SUCCESS;

   /*
    * Error handling.
    */

error:
   if (pShMem!=NULL)
   {
      if (pShMem->pBuffer!=NULL && pShMem->pClientBuffer != pShMem->pBuffer)
      {
         /* pShMem->pBuffer was allocated using static_SMXSharedMemAlloc, it must now
            be deallocated. Note that if pShMem->pClientBuffer == pShMem->pBuffer,
            then pBuffer must not be deallocated because it was passed by the client */
         TEEC_ReleaseSharedMemory (&pShMem->sTeecSharedMem);
      }
      SMXDeviceContextFree(pDevice,(SMX_NODE*)pShMem);
   }
   SMXUnLockObject((SMX_NODE*)pClientSession);
   TRACE_INFO("SMXStubCreateSharedMemory(0x%X): Error 0x%X",
         hClientSession, nError);
   return nError;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT SM_ERROR SMStubAllocateSharedMemory(
      SM_HANDLE  hClientSession,
      uint32_t   nLength,
      uint32_t   nFlags,
      uint32_t   nReserved,
      void**     ppBlock,
      SM_HANDLE* phBlockHandle)
{
   SM_ERROR nResult;

   TRACE_INFO("SMStubAllocateSharedMemory(0x%X, %u, 0x%X, %p, %p)",
         hClientSession, nLength, nFlags, ppBlock, phBlockHandle);

   if(nReserved != 0
      || phBlockHandle==NULL
      || ppBlock==NULL)
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   if (nLength>TFAPI_SHMEM_MAX_CAPACITY)
   {
      return SM_ERROR_OUT_OF_MEMORY;
   }

   nResult = static_SMXStubCreateSharedMemory(hClientSession,
                                          NULL, nLength, nFlags,
                                          phBlockHandle, ppBlock);
   return nResult;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT SM_ERROR SMStubRegisterSharedMemory(
      SM_HANDLE  hClientSession,
      void*      pBuffer,
      uint32_t   nBufferLength,
      uint32_t   nFlags,
      uint32_t   nReserved,
      SM_HANDLE* phBlockHandle)
{
   SM_ERROR nResult;

   TRACE_INFO("SMStubRegisterSharedMemory(0x%X, %p, %u, 0x%X, %p)",
         hClientSession, pBuffer, nBufferLength, nFlags, phBlockHandle);

   if(nReserved != 0
      || phBlockHandle==NULL
      || pBuffer==NULL)
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }
   if (nBufferLength>TFAPI_SHMEM_MAX_CAPACITY)
   {
      return SM_ERROR_OUT_OF_MEMORY;
   }

   nResult = static_SMXStubCreateSharedMemory(hClientSession,
                                          pBuffer, nBufferLength, nFlags,
                                          phBlockHandle, NULL);
   return nResult;
}

/* ------------------------------------------------------------------------ */

SM_EXPORT SM_ERROR SMStubReleaseSharedMemory(
      SM_HANDLE hBlockHandle)
{
   SM_ERROR nError = SM_SUCCESS;
   SM_DEVICE_CONTEXT* pDevice=NULL;
   SM_CLIENT_SESSION* pSession;
   SM_SHARED_MEMORY* pShMem;
   TEEC_SharedMemory* pTeecShMem;

   TRACE_INFO("SMStubReleaseSharedMemory(0x%X)", hBlockHandle);

   if(hBlockHandle == SM_HANDLE_INVALID)
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   pShMem=SMXGetPointerFromHandle(hBlockHandle);
   if (pShMem->sNode.nType != TFAPI_OBJECT_TYPE_SHARED_MEMORY)
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }
   pSession = pShMem->pSession;
   pDevice = pSession->pDeviceContext;
   pTeecShMem = &pShMem->sTeecSharedMem;

   SMXLockCriticalSection(&pDevice->sCriticalSection);

   if (pShMem->nReferenceCount != 1)
   {
      nError=SM_ERROR_ILLEGAL_STATE;
      goto error;
   }
   pShMem->nReferenceCount--;

   SMXUnLockCriticalSection(&pDevice->sCriticalSection);
   TEEC_ReleaseSharedMemory(pTeecShMem);
   SMXLockCriticalSection(&pDevice->sCriticalSection);

   pSession->nReferenceCount--;
   if (pSession->nReferenceCount==0)
   {
      pDevice->nReferenceCount--;
   }

   //if (pShMem->pBuffer!=NULL && pShMem->pClientBuffer != pShMem->pBuffer)
   //{
   //    /* pShMem->pBuffer was allocated using static_SMXSharedMemAlloc, it must now
   //    be deallocated. Note that if pShMem->pClientBuffer == pShMem->pBuffer,
   //    then pBuffer must not be deallocated because it was passed by the client */
   //    scxReleaseSharedMemory(pShMem->pSCXContext, pShMem->pBuffer, pShMem->nLength);
   //}
   SMXDeviceContextFree(pDevice,(SMX_NODE*)pShMem);
   if (pSession->nReferenceCount==0)
   {
      SMXDeviceContextFree(pDevice,(SMX_NODE*)pSession);
   }

   /*
    * Completion.
    */

error:
   if (nError == SM_SUCCESS)
   {
      TRACE_INFO("SMStubReleaseSharedMemory(0x%X): Success", hBlockHandle);
   }
   else
   {
      TRACE_INFO("SMStubReleaseSharedMemory(0x%X): Error 0x%X",
            hBlockHandle, nError);
   }
   if (pDevice!=NULL)
   {
      SMXUnLockCriticalSection(&pDevice->sCriticalSection);
   }
   return nError;
}

