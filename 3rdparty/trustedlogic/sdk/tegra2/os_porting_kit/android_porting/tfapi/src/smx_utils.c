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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#define SM_EXPORT_IMPLEMENTATION
#include "smapi.h"
#include "smx_defs.h"

#include "smx_heap.h"

#include "smx_encoding.h"
#include "smx_utils.h"

#include "s_error.h"


/*----------------------------------------------------------------------------
 * Utility functions
 *----------------------------------------------------------------------------*/

void SMXCharItoA(
      uint32_t nValue,
      uint32_t nRadix,
      OUT uint8_t* pBuffer,
      uint32_t nBufferCapacity,
      OUT uint32_t* pnBufferSize,
      char nFill)
{
   uint32_t nResultSize = 0;

   if (nRadix < 2 || nRadix > 36)
   {
      goto return_result;
   }

   if (pBuffer == NULL)
   {
      if(nFill != 0)
      {
         nResultSize = nBufferCapacity;
         goto return_result;
      }
      nBufferCapacity = 0;
   }

   if (nValue == 0)
   {
      if (nBufferCapacity == 0)
      {
         nResultSize = 1;
         goto return_result;
      }

      if(nFill == 0)
      {
         *pBuffer = '0';
         nResultSize = 1;
      }
      else
      {
         memset(pBuffer, nFill, nBufferCapacity-1);
         *(pBuffer + nBufferCapacity - 1) = '0';
         nResultSize = nBufferCapacity;
      }

      goto return_result;
   }

   /* Write the digits right-to-left from the buffer's end. */
   /* Shortcut for nRadix = 16 */
   if(nRadix == 16)
   {
      while (nValue != 0)
      {
         if (pBuffer != NULL)
         {
            if (nResultSize < nBufferCapacity)
            {
               /* There is still room to write the digit in the buffer */
               uint32_t nDigit = nValue & 0x0F;

               pBuffer[nBufferCapacity-nResultSize-1] =
                  (uint8_t)(nDigit < 10 ? '0' + nDigit : 'A' + nDigit - 10);
            }
         }
         nResultSize++;
         nValue >>= 4;
      }
   }
   else
   {
      while (nValue != 0)
      {
         if (pBuffer != NULL)
         {
            if (nResultSize < nBufferCapacity)
            {
               /* There is still room to write the digit in the buffer */
               uint32_t nDigit;

               nDigit = nValue % nRadix;

               pBuffer[nBufferCapacity-nResultSize-1] =
                  (uint8_t)(nDigit < 10 ? '0' + nDigit : 'A' + nDigit - 10);
            }
         }
         nResultSize++;
         nValue /= nRadix;
      }
   }
   /* Now move the string to the beginning of the buffer */
   if (nResultSize < nBufferCapacity)
   {
      if(nFill == 0)
      {
         memmove(pBuffer,
                &pBuffer[nBufferCapacity-nResultSize],
                nResultSize);
      }
      else
      {
         memset(pBuffer, nFill, nBufferCapacity-nResultSize);
         nResultSize = nBufferCapacity;
      }
   }

return_result:

   if (pnBufferSize != NULL)
   {
      if (pBuffer != NULL && nResultSize > nBufferCapacity)
      {
         nResultSize = nBufferCapacity;
      }
      *pnBufferSize = nResultSize;
   }
}

void SMXWcharItoA(
      uint32_t nValue,
      uint32_t nRadix,
      wchar_t *pBuffer,
      uint32_t nBufferCapacity,
      uint32_t *pnBufferSize)
{
   uint32_t nBufferSize;

   if (pnBufferSize != NULL)
   {
      *pnBufferSize = 0;
   }

   if (nRadix < 2 || nRadix > 36)
   {
      return;
   }

   if (pBuffer == NULL)
   {
      nBufferCapacity = 0;
   }

   nBufferSize = 0;

   if (nValue == 0)
   {
      if (nBufferCapacity > 0)
      {
         *pBuffer = '0';
      }
      nBufferSize++;
   }
   else
   {
      /* Write the digits right-to-left from the buffer's end. */
      while (nValue != 0)
      {
         if (pBuffer != NULL)
         {
            if (nBufferSize < nBufferCapacity)
            {
               /* There is still room to write the digit in the buffer */
               uint32_t nDigit;

               nDigit = nValue % nRadix;

               pBuffer[nBufferCapacity-nBufferSize-1] =
                  (wchar_t)(nDigit < 10 ? '0' + nDigit : 'A' + nDigit - 10);
            }
         }
         nBufferSize++;
         nValue /= nRadix;
      }
      /* Now move the string to the beginning of the buffer */
      if (nBufferSize < nBufferCapacity)
      {
         memmove(pBuffer,
                   &pBuffer[nBufferCapacity-nBufferSize],
                   nBufferSize*sizeof(wchar_t));
      }
   }

   if (pnBufferSize != NULL)
   {
      if (pBuffer != NULL && nBufferSize > nBufferCapacity)
      {
         nBufferSize = nBufferCapacity;
      }
      *pnBufferSize = nBufferSize;
   }
}

uint32_t SMXGetWcharZStringLength(
      IN const wchar_t *pWcharString)
{
   const wchar_t *pWcharStringEnd;

   if (pWcharString == NULL)
   {
      return 0;
   }

   pWcharStringEnd = pWcharString;
   while (*pWcharStringEnd++ != 0)
   {
      /* do nothing */
   }

   return (pWcharStringEnd - pWcharString) - 1;
}

bool SMXCompareWcharZString(
      const wchar_t *pString1,
      const wchar_t *pString2)
{
   uint32_t nString1Length;
   uint32_t nString2Length;

   if (pString1 == NULL && pString2 == NULL)
   {
      return true;
   }
   if (pString1 == NULL || pString2 == NULL)
   {
      return false;
   }

   nString1Length = SMXGetWcharZStringLength(pString1);
   nString2Length = SMXGetWcharZStringLength(pString2);
   if (nString1Length != nString2Length)
   {
      return false;
   }

   return (memcmp(pString1, pString2, nString1Length * sizeof(wchar_t)) == 0);
}

void SMXStr2Wchar(wchar_t* pWchar, char* pStr, uint32_t nlength)
{
   uint32_t i;

   for (i = 0; i < nlength; i++)
   {
      pWchar[i] = (wchar_t)pStr[i];
   }
}

void SMXWchar2Str(char* pStr, wchar_t* pWchar, uint32_t nlength)
{
   uint32_t i;

   for (i = 0; i < nlength; i++)
   {
      pStr[i] = (char)pWchar[i];
   }
}


/* ----------------------------------------------
   Traces and logs

   The difference between traces and logs is that traces are compiled out
   in release builds whereas logs are visible even in release builds.

   -----------------------------------------------*/
#ifndef NDEBUG
#if defined ANDROID
#define LOG_TAG "Smapi"
#include <android/log.h>
#define _TRACE_INFO(format, ...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, format, __VA_ARGS__)
#define _TRACE_ERROR(format, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, format, __VA_ARGS__)
#define _TRACE_WARNING(format, ...) __android_log_print(TRACE_WARNING, LOG_TAG, format, __VA_ARGS__)
#else
void _TRACE_ERROR(const char* format, ...)
{
   va_list ap;
   va_start(ap, format);
   fprintf(stderr, "TRACE: ERROR: ");
   vfprintf(stderr, format, ap);
   fprintf(stderr, "\n");
   va_end(ap);
}

void _TRACE_WARNING(const char* format, ...)
{
   va_list ap;
   va_start(ap, format);
   fprintf(stderr, "TRACE: WARNING: ");
   vfprintf(stderr, format, ap);
   fprintf(stderr, "\n");
   va_end(ap);
}

void _TRACE_INFO(const char* format, ...)
{
   va_list ap;
   va_start(ap, format);
   fprintf(stderr, "TRACE: ");
   vfprintf(stderr, format, ap);
   fprintf(stderr, "\n");
   va_end(ap);
}
#endif
#endif  /* !defined(__TRACES__) */


SM_HANDLE SMXGetHandleFromPointer(void* pObject)
{
   return (SM_HANDLE)((uint32_t)pObject ^ TFAPI_HANDLE_SCRAMBLER);
}

void* SMXGetPointerFromHandle(SM_HANDLE hObject)
{
   return (void*)((uint32_t)hObject ^ TFAPI_HANDLE_SCRAMBLER);
}

/*----------------------------------------------------------------------------
 * SMX_NODE management
 *----------------------------------------------------------------------------*/

void SMXListAdd(SMX_LIST* pList,SMX_NODE* pNode)
{
   assert(pList!=NULL && pNode!=NULL);

   if(pList->pFirst != NULL)
   {
      pList->pFirst->pPrevious=pNode;
   }
   pNode->pNext=pList->pFirst;
   pNode->pPrevious=NULL;

   pList->pFirst=pNode;
}

void SMXListRemove(SMX_LIST* pList,SMX_NODE* pNode)
{
   assert(pList!=NULL);

   if (pNode!=NULL)
   {
      SMX_NODE* pNext=pNode->pNext;
      SMX_NODE* pPrevious=pNode->pPrevious;

      if (pNode==pList->pFirst)
      {
         pList->pFirst=pNext;
      }
      if (pNext!=NULL)
      {
         pNext->pPrevious=pPrevious;
      }
      if (pPrevious!=NULL)
      {
         pPrevious->pNext=pNext;
      }
   }
}

SMX_NODE* SMXDeviceContextAlloc(
                            SM_DEVICE_CONTEXT *pDevice,
                            uint32_t nSize)
{
   SMX_NODE* pNode=NULL;

   /* nSize-1 because of the char already present in SMX_DEVICE_OBJECT */
   pNode=(SMX_NODE*)malloc(nSize);
   if (pNode==NULL)
   {
      goto error;
   }
   memset(pNode, 0, nSize);

   SMXListAdd(&(pDevice->sDeviceAllocatorList),pNode);

error:
   return pNode;
}

void* SMXAlloc(
               SM_HANDLE hObject,
               uint32_t nSize,
               bool bUseDeviceLock)
{
   SMX_NODE *pNode;
   SM_DEVICE_CONTEXT* pDevice;
   void* pBuffer=NULL;

   if (hObject==SM_HANDLE_INVALID)
   {
      return malloc(nSize);
   }
   else
   {

      pNode=SMXGetPointerFromHandle(hObject);
      switch (pNode->nType)
      {
      case TFAPI_OBJECT_TYPE_DECODER:
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
      default:
         pDevice=NULL;
         break;
      }
      if (pDevice!=NULL)
      {
         if (bUseDeviceLock) SMXLockCriticalSection(&pDevice->sCriticalSection);
         pNode=SMXDeviceContextAlloc(pDevice,nSize+sizeof(SMX_NODE));
         if (bUseDeviceLock) SMXUnLockCriticalSection(&pDevice->sCriticalSection);
         if (pNode!=NULL)
         {
            pBuffer=(void*)((SMX_BUFFER*)pNode)->buffer;
         }
      }
   }
   return pBuffer;
}

SMX_NODE* SMXDeviceContextReAlloc(
                            SM_DEVICE_CONTEXT *pDevice,
                            SMX_NODE* pNode,
                            uint32_t nSize)
{
   SMX_NODE* pOldNode=pNode;
   SMX_NODE* pPrevious;
   SMX_NODE* pNext;

   if (pNode==NULL)
   {
      goto error;
   }

   pNode=(SMX_NODE*)realloc(pNode,nSize);
   if (pNode==NULL)
   {
      goto error;
   }
   if(pNode != pOldNode)
   {
      /* be careful pNode has been freed
         the linked list previous and next nodes must be updated */

      pPrevious = pNode->pPrevious;
      pNext = pNode->pNext;

      if(pPrevious != NULL)
      {
         pPrevious->pNext = pNode;
      }
      if(pNext != NULL)
      {
         pNext->pPrevious = pNode;
      }
      if (pOldNode==pDevice->sDeviceAllocatorList.pFirst)
      {
         pDevice->sDeviceAllocatorList.pFirst=pNode;
      }
   }

error:
   return pNode;
}

void* SMXReAlloc(
                 SM_HANDLE hObject,
                 void* pBuffer,
                 uint32_t nSize,
                 bool bUseDeviceLock)
{
   SMX_NODE *pNode;
   SM_DEVICE_CONTEXT* pDevice;
   void* pReallocatedBuffer=NULL;

   if (hObject!=SM_HANDLE_INVALID)
   {

      pNode=SMXGetPointerFromHandle(hObject);
      switch (pNode->nType)
      {
      case TFAPI_OBJECT_TYPE_DECODER:
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
      default:
         pDevice=NULL;
         break;
      }
      if (pDevice!=NULL)
      {
         pNode=(SMX_NODE*)((uint8_t*)pBuffer-offsetof(SMX_BUFFER,buffer));
         if (bUseDeviceLock) SMXLockCriticalSection(&pDevice->sCriticalSection);
         pNode=SMXDeviceContextReAlloc(pDevice,pNode,nSize+sizeof(SMX_NODE));
         if (bUseDeviceLock) SMXUnLockCriticalSection(&pDevice->sCriticalSection);
         if (pNode!=NULL)
         {
            pReallocatedBuffer=(void*)((SMX_BUFFER*)pNode)->buffer;
         }
      }
   }
   return pReallocatedBuffer;
}

void SMXDeviceContextFree(
                              SM_DEVICE_CONTEXT *pDevice,
                              SMX_NODE *pNode)
{
   assert(pDevice!=NULL);

   if (pNode!=NULL)
   {
      SMXListRemove(&(pDevice->sDeviceAllocatorList),pNode);
      memset(pNode, 0, sizeof(SMX_NODE));
      free(pNode);
   }
}

void SMXDeviceContextRemoveAll(
                               SM_DEVICE_CONTEXT *pDevice)
{
   SMX_NODE* pRoot;
   SMX_NODE* pNext;

   assert(pDevice!=NULL);

   pRoot=pDevice->sDeviceAllocatorList.pFirst;

   while (pRoot!=NULL)
   {
      pNext=pRoot->pNext;
      memset(pRoot, 0, sizeof(SMX_NODE));
      free(pRoot);
      pRoot=pNext;
   }
   pDevice->sDeviceAllocatorList.pFirst=NULL;
}

/*----------------------------------------------------------------------------
 * Synchronization
 *----------------------------------------------------------------------------*/

SM_ERROR SMXLockOperationFromHandle(
      SM_HANDLE hOperation,
      SM_OPERATION** ppOperation,
      SM_ACTION_TYPE nActionType)
{
   SM_ERROR nError=SM_SUCCESS;
   SM_OPERATION* pOperation=NULL;
   uint32_t nType;

   assert(ppOperation != NULL);

   *ppOperation=NULL;

   if(hOperation == SM_HANDLE_INVALID)
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   pOperation = (SM_OPERATION*)SMXGetPointerFromHandle(hOperation);

   nType=pOperation->sNode.nType;
   if (nType==TFAPI_OBJECT_TYPE_OPERATION || nType==TFAPI_OBJECT_TYPE_INNER_OPERATION)
   {
      SMXLockCriticalSection(&pOperation->pSession->pDeviceContext->sCriticalSection);
   }
   else
   {
      SMAPI_ASSERT();
   }

   switch (nType)
   {
   case TFAPI_OBJECT_TYPE_INNER_OPERATION:
      if (pOperation->nOperationState==TFAPI_OPERATION_STATE_UNUSED)
      {
         SMAPI_ASSERT();
         break;
      }
      /* fall through */
   case TFAPI_OBJECT_TYPE_OPERATION:
      switch (nActionType)
      {
      case SM_ACTION_PERFORM:
         if (pOperation->nMessageType!=TFAPI_CLOSE_CLIENT_SESSION &&
            pOperation->nOperationError!=SM_SUCCESS)
         {
            if (pOperation->nOperationError==SM_ERROR_CANCEL)
            {
               pOperation->nOperationState=TFAPI_OPERATION_STATE_RECEIVED;
            }
            nError=pOperation->nOperationError;
            goto error;
         }
         if (pOperation->nOperationState!=TFAPI_OPERATION_STATE_PREPARE)
         {
            nError=SM_ERROR_ILLEGAL_STATE;
            goto error;
         }
         pOperation->nOperationState=TFAPI_OPERATION_STATE_INVOKE;
         pOperation->pSession->nReferenceCount++;
         break;
      case SM_ACTION_CANCEL:
         if (pOperation->nMessageType!=TFAPI_CLOSE_CLIENT_SESSION)
         {
            switch (pOperation->nOperationState)
            {
            case TFAPI_OPERATION_STATE_PREPARE:
            case TFAPI_OPERATION_STATE_ENCODE:
               pOperation->nOperationError=SM_ERROR_CANCEL;
               break;
            default:
               break;
            }
         }
         pOperation->pSession->nReferenceCount++;
         break;
      case SM_ACTION_RELEASE:
         /* perform must be done before released the operation */
         if (pOperation->nOperationState!=TFAPI_OPERATION_STATE_RECEIVED)
         {
            nError=SM_ERROR_ILLEGAL_STATE;
            goto error;
         }
         if ((pOperation->nMessageType==TFAPI_CLOSE_CLIENT_SESSION)
            ||
            (pOperation->nMessageType==TFAPI_OPEN_CLIENT_SESSION &&
            (pOperation->nOperationError!=SM_SUCCESS)))
         {
            pOperation->pSession->nReferenceCount-=2;
         }
         else
         {
            pOperation->pSession->nReferenceCount--;
         }
         if (pOperation->pSession->nReferenceCount==0)
         {
            pOperation->pSession->pDeviceContext->nReferenceCount--;
         }
         break;
      default:
         nError=SM_ERROR_ILLEGAL_ARGUMENT;
         break;
      }
      if (nError!=SM_SUCCESS)
      {
         goto error;
      }
      break;
   default:
      SMAPI_ASSERT();
      break;
   }
   *ppOperation = pOperation;

error:
   if (nError!=SM_SUCCESS)
   {
      SMXUnLockCriticalSection(&pOperation->pSession->pDeviceContext->sCriticalSection);
   }
   return nError;
}

SM_ERROR SMXLockObjectFromHandle(
      SM_HANDLE hService,
      SMX_NODE** ppObject,
      uint32_t nType)
{
   SM_ERROR nError=SM_SUCCESS;
   SMX_NODE* pObject=NULL;
   SM_DEVICE_CONTEXT* pDevice=NULL;
   uint32_t nFoundType;

   assert(ppObject != NULL);

   *ppObject=NULL;

   if(hService == SM_HANDLE_INVALID)
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   pObject = (SMX_NODE*)SMXGetPointerFromHandle(hService);

   nFoundType=pObject->nType;

   if (nFoundType!=nType)
   {
      SMAPI_ASSERT();
   }

   switch (nFoundType)
   {
   case TFAPI_OBJECT_TYPE_ENCODER:
      {
         SM_OPERATION* pOperation=((SMX_ENCODER*)pObject)->pOperation;
         pDevice=pOperation->pSession->pDeviceContext;
         SMXLockCriticalSection(&pDevice->sCriticalSection);

         if (pOperation->nOperationState!=TFAPI_OPERATION_STATE_PREPARE &&
            pOperation->nOperationState!=TFAPI_OPERATION_STATE_ENCODE)
         {
            goto error;
         }
         if (pOperation->nOperationState==TFAPI_OPERATION_STATE_ENCODE)
         {
            nError=SM_ERROR_ILLEGAL_STATE;
            goto error;
         }
         if (pOperation->nOperationState==TFAPI_OPERATION_STATE_PREPARE)
         {
            pOperation->nOperationState=TFAPI_OPERATION_STATE_ENCODE;
         }
      }
      break;
   case TFAPI_OBJECT_TYPE_DECODER:
      {
         SM_OPERATION* pOperation=((SMX_DECODER*)pObject)->pOperation;
         pDevice=pOperation->pSession->pDeviceContext;
         SMXLockCriticalSection(&pDevice->sCriticalSection);

         if (pOperation->nOperationState!=TFAPI_OPERATION_STATE_RECEIVED &&
            pOperation->nOperationState!=TFAPI_OPERATION_STATE_DECODE)
         {
            goto error;
         }
         if (pOperation->nOperationState==TFAPI_OPERATION_STATE_DECODE)
         {
            nError=SM_ERROR_ILLEGAL_STATE;
            goto error;
         }
         if (pOperation->nOperationState==TFAPI_OPERATION_STATE_RECEIVED)
         {
            pOperation->nOperationState=TFAPI_OPERATION_STATE_DECODE;
         }
      }
      break;
   case TFAPI_OBJECT_TYPE_INNER_OPERATION:
   case TFAPI_OBJECT_TYPE_OPERATION:
      /* should not occur ! */
      SMAPI_ASSERT();
      break;
   case TFAPI_OBJECT_TYPE_SESSION:
      {
         SM_CLIENT_SESSION* pClientSession=(SM_CLIENT_SESSION*)pObject;
         pDevice=pClientSession->pDeviceContext;
         SMXLockCriticalSection(&pDevice->sCriticalSection);

         if (pClientSession->nReferenceCount==0)
         {
            nError=SM_ERROR_ILLEGAL_STATE;
            goto error;
         }
         pClientSession->nReferenceCount++;
      }
      break;
   case TFAPI_OBJECT_TYPE_SHARED_MEMORY:
      {
         SM_SHARED_MEMORY* pSharedMemory=(SM_SHARED_MEMORY*)pObject;
         /* critical section already locked by the encoder */

         if (pSharedMemory->nReferenceCount==0)
         {
            nError=SM_ERROR_ILLEGAL_STATE;
            goto error;
         }
         pSharedMemory->nReferenceCount++;
      }
      break;
   case TFAPI_OBJECT_TYPE_DEVICE_CONTEXT:
      {
         pDevice=(SM_DEVICE_CONTEXT*)pObject;
         SMXLockCriticalSection(&pDevice->sCriticalSection);

         if (pDevice->nReferenceCount==0)
         {
            nError=SM_ERROR_ILLEGAL_STATE;
            goto error;
         }
         pDevice->nReferenceCount++;
      }
      break;
   default:
      nError=SM_ERROR_ILLEGAL_ARGUMENT;
      break;
   }
   if (nError==SM_SUCCESS)
   {
      *ppObject=pObject;
   }
error:
   if (nError!=SM_SUCCESS)
   {
      if (pDevice!=NULL)
      {
         SMXUnLockCriticalSection(&pDevice->sCriticalSection);
      }
   }
   return nError;
}

SM_ERROR SMXUnLockObject(
      SMX_NODE* pObject)
{
   SM_ERROR nError=SM_SUCCESS;
   SM_DEVICE_CONTEXT *pDevice=NULL;
   uint32_t nType;

   if (pObject==NULL)
   {
      nError=SM_ERROR_ILLEGAL_ARGUMENT;
      goto error;
   }

   nType=pObject->nType;

   switch (nType)
   {
   case TFAPI_OBJECT_TYPE_ENCODER:
      {
         SM_OPERATION* pOperation=((SMX_ENCODER*)pObject)->pOperation;
         pDevice=pOperation->pSession->pDeviceContext;
         if (pOperation->nOperationState==TFAPI_OPERATION_STATE_ENCODE)
         {
            pOperation->nOperationState=TFAPI_OPERATION_STATE_PREPARE;
         }
         else
         {
            nError=SM_ERROR_ILLEGAL_STATE;
            goto error;
         }
      }
      break;
   case TFAPI_OBJECT_TYPE_DECODER:
      {
         SM_OPERATION* pOperation=((SMX_DECODER*)pObject)->pOperation;
         pDevice=pOperation->pSession->pDeviceContext;
         if (pOperation->nOperationState==TFAPI_OPERATION_STATE_DECODE)
         {
            pOperation->nOperationState=TFAPI_OPERATION_STATE_RECEIVED;
         }
         else
         {
            nError=SM_ERROR_ILLEGAL_STATE;
            goto error;
         }
      }
      break;
   case TFAPI_OBJECT_TYPE_INNER_OPERATION:
   case TFAPI_OBJECT_TYPE_OPERATION:
      if (((SM_OPERATION*)pObject)->nOperationState==TFAPI_OPERATION_STATE_INVOKE)
      {
         ((SM_OPERATION*)pObject)->nOperationState=TFAPI_OPERATION_STATE_RECEIVED;
      }
      pObject=(SMX_NODE*)(((SM_OPERATION*)pObject)->pSession);
      /* fall through */
   case TFAPI_OBJECT_TYPE_SESSION:
      pDevice=((SM_CLIENT_SESSION*)pObject)->pDeviceContext;
      if (((SM_CLIENT_SESSION*)pObject)->nReferenceCount<=1)
      {
         nError=SM_ERROR_ILLEGAL_STATE;
         goto error;
      }
      ((SM_CLIENT_SESSION*)pObject)->nReferenceCount--;
      break;
   case TFAPI_OBJECT_TYPE_DEVICE_CONTEXT:
      pDevice=(SM_DEVICE_CONTEXT*)pObject;
      if (pDevice->nReferenceCount<=1)
      {
         nError=SM_ERROR_ILLEGAL_STATE;
         goto error;
      }
      pDevice->nReferenceCount--;
      break;
   case TFAPI_OBJECT_TYPE_SHARED_MEMORY:
      {
         SM_SHARED_MEMORY* pSharedMemory=(SM_SHARED_MEMORY*)pObject;
         /* unlock critical section done by the encoder */
         if (pSharedMemory->nReferenceCount<=1)
         {
            nError=SM_ERROR_ILLEGAL_STATE;
            goto error;
         }
         pSharedMemory->nReferenceCount--;
      }
      break;
   default:
      nError=SM_ERROR_ILLEGAL_ARGUMENT;
      goto error;
   }

error:
   if (pDevice!=NULL)
   {
      SMXUnLockCriticalSection(&pDevice->sCriticalSection);
   }

   return nError;
}

bool SMXEncoderBufferReAllocated(SMX_ENCODER* pEncoder)
{
   SM_ERROR nError;
   LIB_BEF_ENCODER* pBEFEncoder;
   SM_DEVICE_CONTEXT *pDevice;
   void* pBuffer;

   pBEFEncoder=&pEncoder->sBEFEncoder;
   nError=SMXEncoderGetError(pEncoder);
   if (nError==S_ERROR_SHORT_BUFFER)
   {
      pDevice=pEncoder->pOperation->pSession->pDeviceContext;

      if (pBEFEncoder->pBuffer==pDevice->pInnerEncoderBuffer)
      {
         pBuffer=SMXHeapAlloc(pDevice->hDeviceHeapStructure,pBEFEncoder->nRequiredCapacity);
         if (pBuffer!=NULL)
         {
            memcpy(pBuffer,pDevice->pInnerEncoderBuffer,pBEFEncoder->nCapacity);
            pDevice->bInnerEncoderBufferUsed=false;
         }
      }
      else
      {
         pBuffer = SMXHeapRealloc(pDevice->hDeviceHeapStructure,pBEFEncoder->pBuffer,pBEFEncoder->nRequiredCapacity);
      }
      if (pBuffer==NULL)
      {
         pEncoder->sBEFEncoder.nError=SM_ERROR_OUT_OF_MEMORY;
      }
      else
      {
         pBEFEncoder->pBuffer = pBuffer;
         pBEFEncoder->nCapacity=pBEFEncoder->nRequiredCapacity;
         pBEFEncoder->nError = SM_SUCCESS;
         return true;
      }
   }
   return false;
}

void SMXEncoderAutoCloseSequences(SMX_ENCODER* pEncoder)
{
   LIB_BEF_ENCODER* pBEFEncoder;

   pBEFEncoder=&pEncoder->sBEFEncoder;
   /**
    * Reset the encoder error in order to be able to close any opened
    * sequence.
    */
   pBEFEncoder->nError = S_SUCCESS;
   do
   {
      libBEFEncoderCloseSequence(pBEFEncoder);
   }

   while(pBEFEncoder->nError == S_SUCCESS);
   pBEFEncoder->nError = S_SUCCESS;
}

/*---------------------------------------------------------------------------*/

SM_ERROR TeecErrorToSmapiError(  TEEC_Result nTeecResult,
                                 int32_t nErrorOrigin,
                                 SM_ERROR* pnServiceErrorCode)
{
   if (nTeecResult == TEEC_SUCCESS)
   {
      if (pnServiceErrorCode != NULL)
      {
         *pnServiceErrorCode = SM_SUCCESS;
      }
      return SM_SUCCESS;
   } 
   
   if (nErrorOrigin == TEEC_ORIGIN_TRUSTED_APP)
   {
      /* A service error code */
      if (pnServiceErrorCode != NULL)
      {
         *pnServiceErrorCode = nTeecResult;
      }
      return SM_ERROR_SERVICE;
   }

   /* A framework error code */
   if (pnServiceErrorCode != NULL)
   {
      *pnServiceErrorCode = SM_ERROR_GENERIC;
   }

   return nTeecResult;
}

/*----------------------------------------------------------------------------
 * TEEC Contexts Access - Exported, but non-official API
 *----------------------------------------------------------------------------*/

SM_EXPORT TEEC_Context* _SMGetTEEContext(SM_HANDLE hDevice)
{
   SM_DEVICE_CONTEXT* pDevice = NULL;

   /* TRACE_INFO("_SMGetTEEContext(0x%x)", hDevice); */

   if (hDevice == SM_HANDLE_INVALID)
   {
      TRACE_ERROR("_SMGetTEEContext(0x%x): Error - NULL handle", hDevice);
      return NULL;
   }

   pDevice = (SM_DEVICE_CONTEXT*)SMXGetPointerFromHandle(hDevice);

   if (pDevice->sNode.nType != TFAPI_OBJECT_TYPE_DEVICE_CONTEXT)
   {
      TRACE_ERROR("_SMGetTEEContext(0x%x): Error - Invalid handle", hDevice);
      return NULL;
   }

   /* TRACE_INFO("_SMGetTEEContext(0x%x) --> 0x%x", hDevice, &pDevice->sTeecContext); */

   return &pDevice->sTeecContext;
}

SM_EXPORT TEEC_Session* _SMGetTEESession(SM_HANDLE hSession)
{
   SM_CLIENT_SESSION* pClientSession;

   /* TRACE_INFO("_SMGetTEEContext(0x%x)", hSession); */

   if (hSession == SM_HANDLE_INVALID)
   {
      TRACE_ERROR("_SMGetTEESession(0x%x): Error - NULL handle", hSession);
      return NULL;
   }

   pClientSession = (SM_CLIENT_SESSION*)SMXGetPointerFromHandle(hSession);

   if (pClientSession->sNode.nType != TFAPI_OBJECT_TYPE_SESSION)
   {
      TRACE_ERROR("_SMGetTEESession(0x%x): Error - Invalid handle", hSession);
      return NULL;
   }

   /* TRACE_INFO("_SMGetTEESession(0x%x) --> 0x%x", hSession, &pClientSession->sTeecSession); */

   return &pClientSession->sTeecSession;
}

SM_EXPORT TEEC_SharedMemory* _SMGetTEESharedMemory(SM_HANDLE hSharedMemory)
{
   SM_SHARED_MEMORY* pShMem;

   /* TRACE_INFO("_SMGetTEESharedMemory(0x%x)", hSharedMemory); */

   if (hSharedMemory == SM_HANDLE_INVALID)
   {
      TRACE_ERROR("_SMGetTEESharedMemory(0x%x): Error - NULL handle", hSharedMemory);
      return NULL;
   }

   pShMem = (SM_SHARED_MEMORY*)SMXGetPointerFromHandle(hSharedMemory);

   if (pShMem->sNode.nType != TFAPI_OBJECT_TYPE_SHARED_MEMORY)
   {
      TRACE_ERROR("_SMGetTEESharedMemory(0x%x): Error - Invalid handle", hSharedMemory);
      return NULL;
   }

   /* TRACE_INFO("_SMGetTEESharedMemory(0x%x) --> 0x%x", hSharedMemory, &pShMem->sTeecSharedMem); */

   return &pShMem->sTeecSharedMem;
}
