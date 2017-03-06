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

#include <stdlib.h>
#include <string.h>

#ifdef LINUX
#ifndef ANDROID
#include <wchar.h>
#endif
#endif

#define SM_EXPORT_IMPLEMENTATION
#include "smapi.h"

#include "service_manager_protocol.h"

#include "smx_heap.h"
#include "smx_utils.h"

/*---------------------------------------------------------------------------
* Internal Structures and Constants
*---------------------------------------------------------------------------*/

/**
* The prototype of the implementation property getter functions.
*/
typedef SM_ERROR (*PSMX_IMPL_PROPERTY_VALUE_GETTER_FUNC)(
   SM_DEVICE_CONTEXT* pDevice,
   wchar_t **ppPropertyValue);


/**
* Describes an implementation property handler.
*/
typedef struct
{
   /**
   * The name of the implementation property.
   */
   const wchar_t *pPropertyName;

   /**
   * A pointer to the function to invoke to get the property value.
   */
   PSMX_IMPL_PROPERTY_VALUE_GETTER_FUNC pGetterFunc;

} SMX_IMPL_PROPERTY_HANDLER;



/*---------------------------------------------------------------------------
 * Forward Declarations
 *---------------------------------------------------------------------------*/

static SM_ERROR static_SMXGetApiVersionImplProperty (
   SM_DEVICE_CONTEXT* pDevice,
   wchar_t **ppPropertyValue);

static SM_ERROR static_SMXGetApiDescriptionImplProperty (
   SM_DEVICE_CONTEXT* pDevice,
   wchar_t **ppPropertyValue);

static SM_ERROR static_SMXGetDeviceNameImplProperty (
   SM_DEVICE_CONTEXT* pDevice,
   wchar_t **ppPropertyValue);

static SM_ERROR static_SMXGetDeviceDescriptionImplProperty (
   SM_DEVICE_CONTEXT* pDevice,
   wchar_t **ppPropertyValue);

static SM_ERROR static_SMXGetDriverDescriptionImplProperty (
   SM_DEVICE_CONTEXT* pDevice,
   wchar_t **ppPropertyValue);


/*---------------------------------------------------------------------------
 * Internal Variables
 *---------------------------------------------------------------------------*/

/**
 * Defines the implementation property handlers.
 */
static const SMX_IMPL_PROPERTY_HANDLER g_smxImplPropertyHandlers[] =
{
   { L"sm.apiversion", static_SMXGetApiVersionImplProperty },
   { L"sm.apidescription", static_SMXGetApiDescriptionImplProperty },
   { L"sm.devicename", static_SMXGetDeviceNameImplProperty },
   { L"sm.devicedescription", static_SMXGetDeviceDescriptionImplProperty },
   { L"sm.driverdescription", static_SMXGetDriverDescriptionImplProperty },
   { NULL, NULL }
};


/*---------------------------------------------------------------------------
 * Manager Open
 *---------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMManagerOpen(
                                 SM_HANDLE   hDevice,
                                 uint32_t    nLoginType,
                                 const void* pLoginInfo,
                                 uint32_t    nControlMode,
                                 SM_HANDLE*  phServiceManager)
{
   const SM_UUID        idService = SERVICE_MANAGER_UUID;
   SM_ERROR             nError;
   TEEC_Result          nTeeError;
   SM_DEVICE_CONTEXT*   pDevice = NULL;

   SMX_MANAGER_SESSION* pManagerSession = NULL;
   TEEC_Operation       sOperation;
   uint32_t             nErrorOrigin;
   uint32_t             nTeecLoginType = nLoginType;
   char*                pSignatureFile = NULL;
   uint32_t             nSignatureFileLen = 0;
   uint32_t             nParamType3 = TEEC_NONE;

   TRACE_INFO("SMManagerOpen(0x%X, 0x%X, %p, 0x%X, %p)",
      hDevice, nLoginType, pLoginInfo, nControlMode, phServiceManager);

   /*
    * Validate parameters.
    */
   if (  (hDevice == SM_HANDLE_INVALID) ||
         (phServiceManager == NULL))
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   /* nControlMode is checked by the Service Manager in the secure side */

   switch (nLoginType)
   {
   case SM_LOGIN_PUBLIC:
      if (pLoginInfo != NULL)
      {
         TRACE_ERROR("SMManagerOpen(0x%X): SM_LOGIN_PUBLIC: pLoginInfo not NULL",
            hDevice);
         return SM_ERROR_ILLEGAL_ARGUMENT;
      }
      break;

   case SM_LOGIN_OS_IDENTIFICATION:
   case SM_LOGIN_AUTHENTICATION:
   case SM_LOGIN_PRIVILEGED:
      break;

   case SM_LOGIN_AUTHENTICATION_FALLBACK_OS_IDENTIFICATION:
      nTeecLoginType = TEEC_LOGIN_AUTHENTICATION;
      break;

   default:
      TRACE_ERROR("SMManagerOpen(0x%X): Invalid nLoginType parameter (0x%X)",
         hDevice, nLoginType);
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   if (nTeecLoginType == TEEC_LOGIN_AUTHENTICATION)
   {
      nTeeError = TEEC_ReadSignatureFile((void **)&pSignatureFile, &nSignatureFileLen);
      if (  (nTeeError == TEEC_ERROR_ITEM_NOT_FOUND) &&
            (nLoginType == SM_LOGIN_AUTHENTICATION))
      {
         TRACE_ERROR("SMManagerOpen: Error - Missing signature!");
         return SM_ERROR_ACCESS_DENIED;
      }
      
      if (  (nTeeError == TEEC_ERROR_ITEM_NOT_FOUND) &&
            (nLoginType == SM_LOGIN_AUTHENTICATION_FALLBACK_OS_IDENTIFICATION))
      { 
         /* No signature needed */
         nTeecLoginType = TEEC_LOGIN_APPLICATION;
      }
      else
      {
         if (nTeeError != TEEC_SUCCESS)
         {
            TRACE_ERROR("SMManagerOpen:TEEC_ReadSignatureFile returned an error 0x%x",nTeeError);
            /* No need to convert TEEC error into SMAPI error */
            return nTeeError;
         }

         /* Send signature file in param3 */
         sOperation.params[3].tmpref.buffer = pSignatureFile;
         sOperation.params[3].tmpref.size   = nSignatureFileLen;
         nParamType3 = TEEC_MEMREF_TEMP_INPUT;
      }
   }

   *phServiceManager = SM_HANDLE_INVALID;

   /* Get the device context */
   nError = SMXLockObjectFromHandle(
      hDevice,
      (void*)&pDevice,
      TFAPI_OBJECT_TYPE_DEVICE_CONTEXT);
   if (nError!=SM_SUCCESS)
   {
      TRACE_ERROR("SMManagerOpen: SMXLockObjectFromHandle error");
      goto error;
   }
   if (pDevice==NULL)
   {
      TRACE_ERROR("SMManagerOpen: Device context error");
      nError = SM_ERROR_ILLEGAL_ARGUMENT;
      goto error;
   }

   /* Allocate pSession */
   pManagerSession = (SMX_MANAGER_SESSION*)SMXDeviceContextAlloc(pDevice,sizeof(SMX_MANAGER_SESSION));
   if (pManagerSession == NULL)
   {
      TRACE_ERROR("SMManagerOpen: out of memory for allocating TEE Session");
      nError = SM_ERROR_OUT_OF_MEMORY;
      goto error;
   }

   pManagerSession->sNode.nType = TFAPI_OBJECT_TYPE_MANAGER;
   pManagerSession->pDeviceContext = pDevice;
   pManagerSession->hDevice = hDevice;

   memset(&sOperation, 0, sizeof(TEEC_Operation));
   sOperation.paramTypes = TEEC_PARAM_TYPES(
      TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, nParamType3);
   sOperation.params[0].value.a = nControlMode;

   nTeeError = TEEC_OpenSession(&pDevice->sTeecContext,
      &pManagerSession->sTeecSession,  /* OUT session */
      &idService,                      /* destination UUID */
      nTeecLoginType,                  /* connectionMethod */
      (void*)pLoginInfo,               /* connectionData */
      &sOperation,                     /* IN OUT operation */
      &nErrorOrigin                    /* OUT errorOrigin */
      );

   if (nTeeError != TEEC_SUCCESS)
   {
      TRACE_INFO("SMManagerOpen: TEEC_OpenSession returned error 0x%08X\n", nTeeError);
      /* No need to convert TEEC error into SMAPI error */
      nError = nTeeError;
      goto error;
   }

   /*
   * Successful completion.
   */

   *phServiceManager = SMXGetHandleFromPointer(pManagerSession);

   SMXUnLockCriticalSection(&pDevice->sCriticalSection);

   TRACE_INFO("SMManagerOpen(0x%X): Success (hServiceManager = 0x%X)",
      hDevice, *phServiceManager);

   return SM_SUCCESS;

   /*
   * Error handling.
   */

error:
   SMXDeviceContextFree(pDevice, (SMX_NODE*)pManagerSession);
   SMXUnLockObject((SMX_NODE*)pDevice);
   TRACE_INFO("SMManagerOpen(0x%X): Error 0x%X", hDevice, nError);
   return nError;
}


/*---------------------------------------------------------------------------
* Manager Close
*---------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMManagerClose(SM_HANDLE hServiceManager)
{
   SMX_MANAGER_SESSION* pManagerSession;
   SM_DEVICE_CONTEXT*   pDevice;
   SM_ERROR             nError = SM_SUCCESS;

   TRACE_INFO("SMManagerClose(0x%X)", hServiceManager);

   if (hServiceManager == SM_HANDLE_INVALID)
   {
      return SM_SUCCESS;
   }

   pManagerSession = (SMX_MANAGER_SESSION*)SMXGetPointerFromHandle(hServiceManager);
   if (pManagerSession == NULL)
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   pDevice = pManagerSession->pDeviceContext;
   if (pDevice == NULL)
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   SMXLockCriticalSection(&pDevice->sCriticalSection);

   TEEC_CloseSession(&pManagerSession->sTeecSession);

   SMXDeviceContextFree(pDevice,(SMX_NODE*)pManagerSession);

   /* SMXUnLockObject also unlocks the critical section */
   nError = SMXUnLockObject((SMX_NODE*)pDevice);
   if (nError != SM_SUCCESS)
   {
      TRACE_ERROR("SMManagerClose(0x%X): Error 0x%X\n",hServiceManager, nError);
   }

   return nError;
}


/*---------------------------------------------------------------------------
* Get All Services
*---------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMManagerGetAllServices(
   SM_HANDLE hServiceManager,
   SM_UUID** pServiceIdentifierList,
   uint32_t* pnListLength)
{
   SM_ERROR    nError = SM_SUCCESS;
   TEEC_Result nTeeError;

   uint32_t    nIdentifierLen;
   SM_UUID*    pIdentifiers = NULL;

   TEEC_Operation sOperation;
   uint32_t       nErrorOrigin;
   SMX_MANAGER_SESSION* pManagerSession;

   TRACE_INFO("SMManagerGetAllServices(0x%X, %p, %p)",
      hServiceManager, pServiceIdentifierList, pnListLength);

   /*
   * Validate parameters.
   */
   if (  (hServiceManager == SM_HANDLE_INVALID) ||
         (pServiceIdentifierList == NULL) ||
         (pnListLength == NULL))
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   *pServiceIdentifierList = NULL;
   *pnListLength = 0;

   /*
   * Get the TEEC session handle 
   */
   pManagerSession = (SMX_MANAGER_SESSION*)SMXGetPointerFromHandle(hServiceManager);
   if (pManagerSession == SM_HANDLE_INVALID)
   {
      TRACE_ERROR("SMManagerGetAllServices(0x%X): Invalid hServiceManager parameter (0x%X)",
         hServiceManager, hServiceManager);
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   if (pManagerSession->pDeviceContext == NULL)
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   SMXLockCriticalSection(&pManagerSession->pDeviceContext->sCriticalSection);

   nIdentifierLen = sizeof(SM_UUID);
   pIdentifiers = (SM_UUID*)SMXAlloc(pManagerSession->hDevice, nIdentifierLen, false);
   if (pIdentifiers == NULL)
   {
      nError = SM_ERROR_OUT_OF_MEMORY;
      goto error;
   }

   memset(pIdentifiers,0, nIdentifierLen);

send_invoke:
   memset(&sOperation, 0, sizeof(TEEC_Operation));

   sOperation.paramTypes = TEEC_PARAM_TYPES(
      TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].tmpref.buffer = pIdentifiers;
   sOperation.params[0].tmpref.size   = nIdentifierLen;
   nTeeError = TEEC_InvokeCommand(&pManagerSession->sTeecSession,
      SERVICE_MANAGER_COMMAND_ID_GET_ALL_SERVICES,         /* commandID : 0 for GetAllServices */
      &sOperation,                                         /* IN OUT operation */
      &nErrorOrigin                                        /* OUT errorOrigin */
      );
   if ((nTeeError != TEEC_SUCCESS) && (nTeeError != TEEC_ERROR_SHORT_BUFFER))
   {
      /* No need to convert TEEC error into SMAPI error */
      nError = nTeeError;
      goto error;
   }

   nIdentifierLen = sOperation.params[0].tmpref.size ;
   if (nTeeError == TEEC_ERROR_SHORT_BUFFER)
   {
      void *pTemp;
      /*
      * TempRef size is updated depending on the number of services.
      * Need to reallocate the buffer.
      */
      pTemp = (SM_UUID*)SMXReAlloc(pManagerSession->hDevice, pIdentifiers, nIdentifierLen, true);
      if (pTemp == NULL)
      {
         nError = SM_ERROR_OUT_OF_MEMORY;
         goto error;
      }
      pIdentifiers = pTemp;
      goto send_invoke;
   }

   /*
   * Successful completion.
   */

   *pServiceIdentifierList = pIdentifiers;
   *pnListLength = nIdentifierLen/sizeof(SM_UUID);
   SMXUnLockCriticalSection(&pManagerSession->pDeviceContext->sCriticalSection);

   TRACE_INFO("SMManagerGetAllServices(0x%X): Success (nIdentifierCount = 0x%X, pIdentifiers = %p)",
      hServiceManager, *pnListLength, pIdentifiers);
   return SM_SUCCESS;

   /*
   * Error handling.
   */

error:
   TRACE_INFO("SMManagerGetAllServices(0x%X): Error 0x%X",
      hServiceManager, nError);
   SMXUnLockCriticalSection(&pManagerSession->pDeviceContext->sCriticalSection);
   SMFree(pManagerSession->hDevice, pIdentifiers);
   return nError;
}


/*---------------------------------------------------------------------------
* Get Property
*---------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMManagerGetServiceProperty(
   SM_HANDLE      hServiceManager,
   const SM_UUID* pidService,
   wchar_t*       pPropertyName,
   wchar_t**      ppPropertyValue)
{
   SM_ERROR    nError = SM_SUCCESS;
   TEEC_Result nTeeError;
   wchar_t *pPropertyValue = NULL;
   uint32_t nPropertyValueLen;
   char*    pStrPropertyValue = NULL;
   char*    pStrPropertyName = NULL;
   uint32_t nPropertyNameLen;

   SMX_MANAGER_SESSION* pManagerSession;
   TEEC_Operation sOperation;
   uint32_t       nErrorOrigin;

   TRACE_INFO("SMManagerGetServiceProperty(0x%X, %p, %p, %p)",
      hServiceManager, pidService, pPropertyName, ppPropertyValue);

  /*
   * Validate parameters.
   */
   if (  (hServiceManager == SM_HANDLE_INVALID) ||
         (ppPropertyValue == NULL) ||
         (pPropertyName == NULL))
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   *ppPropertyValue = NULL;

   /*
   * Get the TEEC session handle.
   */
   pManagerSession = (SMX_MANAGER_SESSION*)SMXGetPointerFromHandle(hServiceManager);
   if (pManagerSession == SM_HANDLE_INVALID)
   {
      TRACE_ERROR("SMManagerGetServiceProperty(0x%X): Invalid hServiceManager parameter",
         hServiceManager);
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   if (pManagerSession->pDeviceContext == NULL)
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   SMXLockCriticalSection(&pManagerSession->pDeviceContext->sCriticalSection);

#ifdef ANDROID
   nPropertyNameLen = strlen(pPropertyName);
#else
   nPropertyNameLen = wcslen(pPropertyName);
#endif
   nPropertyNameLen++;  /* For the zero-terminating string */
   pStrPropertyName = malloc(nPropertyNameLen);

   nPropertyValueLen = 64;
   /* Allocate one more wchar for the null-terminating string */
   pPropertyValue = (wchar_t*)SMXAlloc(pManagerSession->hDevice, (nPropertyValueLen + 1) * sizeof(wchar_t), false);
   pStrPropertyValue = malloc(nPropertyValueLen);

   if (  (pPropertyValue == NULL) ||
         (pStrPropertyName == NULL) ||
         (pStrPropertyValue == NULL))
   {
      nError = SM_ERROR_OUT_OF_MEMORY;
      goto error;
   }

   SMXWchar2Str(pStrPropertyName, pPropertyName, nPropertyNameLen);

send_invoke:
   memset(&sOperation, 0, sizeof(TEEC_Operation));

   sOperation.paramTypes = TEEC_PARAM_TYPES(
      TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE);
   sOperation.params[0].tmpref.buffer = (void*)pidService;
   sOperation.params[0].tmpref.size   = sizeof(TEEC_UUID);
   sOperation.params[1].tmpref.buffer = pStrPropertyName;
   sOperation.params[1].tmpref.size   = nPropertyNameLen;
   sOperation.params[2].tmpref.buffer = pStrPropertyValue;
   sOperation.params[2].tmpref.size   = nPropertyValueLen;
   nTeeError = TEEC_InvokeCommand(&pManagerSession->sTeecSession,
      SERVICE_MANAGER_COMMAND_ID_GET_PROPERTY,
      &sOperation,
      &nErrorOrigin
      );
   if ((nTeeError != TEEC_SUCCESS) && (nTeeError != TEEC_ERROR_SHORT_BUFFER))
   {
      /* No need to convert TEEC error into SMAPI error */
      nError = nTeeError;
      goto error;
   }

   nPropertyValueLen = sOperation.params[2].tmpref.size;
   if (nTeeError == TEEC_ERROR_SHORT_BUFFER)
   {
      void* pTemp;
      /*
      * TempRef size is updated depending on the property length
      * Need to reallocate the buffer.
      * Allocate one more wchar for the null-terminating string.
      */
      pTemp = SMXReAlloc(pManagerSession->hDevice, pPropertyValue, (nPropertyValueLen + 1) * sizeof(wchar_t), false);
      if (pTemp == NULL)
      {
         nError = SM_ERROR_OUT_OF_MEMORY;
         goto error;
      }
      pPropertyValue = (wchar_t*)pTemp;

      pTemp = realloc(pStrPropertyValue, nPropertyValueLen);
      if (pTemp == NULL)
      {
         nError = SM_ERROR_OUT_OF_MEMORY;
         goto error;
      }
      pStrPropertyValue = pTemp;
      goto send_invoke;
   }

   /*
    * Successful completion.
    */

   SMXStr2Wchar(pPropertyValue, pStrPropertyValue, nPropertyValueLen);
   /* Sets the null-terminating char of the string */
   pPropertyValue[nPropertyValueLen] = 0;

   *ppPropertyValue = pPropertyValue;

   SMXUnLockCriticalSection(&pManagerSession->pDeviceContext->sCriticalSection);
   free(pStrPropertyName);
   free(pStrPropertyValue);
   TRACE_INFO("SMManagerGetServiceProperty(0x%X): Success (pPropertyValue = %p)",
      hServiceManager, pPropertyValue);
   return SM_SUCCESS;

   /*
    * Error handling.
    */

error:
   TRACE_INFO("SMManagerGetServiceProperty(0x%X): Error 0x%X",
      hServiceManager, nError);
   SMXUnLockCriticalSection(&pManagerSession->pDeviceContext->sCriticalSection);
   SMFree(pManagerSession->hDevice, pPropertyValue);
   free(pStrPropertyName);
   free(pStrPropertyValue);
   return nError;
}


/*---------------------------------------------------------------------------
* Get All Service Properties
*---------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMManagerGetAllServiceProperties(
   SM_HANDLE      hServiceManager,
   const SM_UUID* pidService,
   SM_PROPERTY**  ppProperties,
   uint32_t*      pnPropertiesLength)
{
   SMX_MANAGER_SESSION* pManagerSession;
   SM_ERROR             nError = SM_SUCCESS;
   TEEC_Result          nTeeError;

   uint8_t*       pPropertyBuffer = NULL;
   uint32_t       nPropertyBufferLen;
   TEEC_Operation sOperation;
   uint32_t       nErrorOrigin;

   uint32_t i,nPropertiesCount;
   uint8_t* pBuffer = NULL;
   uint32_t nBufferSize;

   TRACE_INFO("SMManagerGetAllServiceProperties(0x%X, %p, %p, %p)",
      hServiceManager, pidService, ppProperties, pnPropertiesLength);

  /*
   * Validate parameters.
   */
   if (  (hServiceManager == SM_HANDLE_INVALID) ||
         (ppProperties == NULL) ||
         (pnPropertiesLength == NULL))
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   /*
   * Get the TEEC session handle.
   */
   pManagerSession = (SMX_MANAGER_SESSION*)SMXGetPointerFromHandle(hServiceManager);
   if (pManagerSession == SM_HANDLE_INVALID)
   {
      TRACE_ERROR("SMManagerGetServiceProperty(0x%X): Invalid hServiceManager parameter",
         hServiceManager);
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   if (pManagerSession->pDeviceContext == NULL)
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   SMXLockCriticalSection(&pManagerSession->pDeviceContext->sCriticalSection);

   *ppProperties = NULL;
   *pnPropertiesLength = 0;

   nPropertyBufferLen = 64;
   pPropertyBuffer = (uint8_t*)SMXAlloc(pManagerSession->hDevice, nPropertyBufferLen, false);
   if (pPropertyBuffer == NULL)
   {
      nError = SM_ERROR_OUT_OF_MEMORY;
      goto error;
   }
   
   memset(pPropertyBuffer, 0, nPropertyBufferLen);

send_invoke:
   memset(&sOperation, 0, sizeof(TEEC_Operation));

   sOperation.paramTypes = TEEC_PARAM_TYPES(
      TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].tmpref.buffer = (void*)pidService;
   sOperation.params[0].tmpref.size   = sizeof(TEEC_UUID);
   sOperation.params[1].tmpref.buffer = pPropertyBuffer;
   sOperation.params[1].tmpref.size   = nPropertyBufferLen;
   nTeeError = TEEC_InvokeCommand(&pManagerSession->sTeecSession,
      SERVICE_MANAGER_COMMAND_ID_GET_ALL_PROPERTIES,
      &sOperation,
      &nErrorOrigin
      );
   if ((nTeeError != TEEC_SUCCESS) && (nTeeError != TEEC_ERROR_SHORT_BUFFER))
   {
      /* No need to convert TEEC error into SMAPI error */
      nError = nTeeError;
      goto error;
   }

   nPropertyBufferLen = sOperation.params[1].tmpref.size ;
   if (nTeeError == TEEC_ERROR_SHORT_BUFFER)
   {
      void* pTemp;
      /*
      * TempRef size is updated depending on the property length
      * Need to reallocate the buffer.
      */
      pTemp = SMXReAlloc(pManagerSession->hDevice, pPropertyBuffer, nPropertyBufferLen, false);
      if (pTemp == NULL)
      {
         nError = SM_ERROR_OUT_OF_MEMORY;
         goto error;
      }
      pPropertyBuffer = (uint8_t*)pTemp;
      memset(pPropertyBuffer, 0 , nPropertyBufferLen);
      goto send_invoke;
   }

   nBufferSize = 0;
   pBuffer = SMXAlloc(pManagerSession->hDevice, nBufferSize, false);
   if (pBuffer == NULL)
   {
      nError = SM_ERROR_OUT_OF_MEMORY;
      goto error;
   }

   i = 0;
   nPropertiesCount = 0;
   while (i < nPropertyBufferLen)
   {
      uint32_t nPropertyNameLength;
      uint32_t nPropertyValueLength;
      uint32_t nNewBufferSize;
      void*    pTemp;

      /*
       * Read property name and value buffer length.
       */

      if (nPropertyBufferLen - i >= 4)
      {
         /* read PropertyNameLength */
         nPropertyNameLength = *((uint32_t *)(&pPropertyBuffer[i]));
         i += 4;
      }
      else
      {
         /* Buffer overflow */
         nError = SM_ERROR_GENERIC;
         goto error;
      }

      if (nPropertyBufferLen - i >= 4)
      {
         /* read PropertyValueLen */
         nPropertyValueLength = *((uint32_t *)(&pPropertyBuffer[i]));
         i += 4;
      }
      else
      {
         /* Buffer overflow */
         nError = SM_ERROR_GENERIC;
         goto error;
      }

      /* Read first string: Property name */
      nNewBufferSize = nBufferSize + 4;
      /* Allocate room for a null-terminated *wchar_t* string */
      nNewBufferSize += (nPropertyNameLength + 1) * sizeof(wchar_t);
      pTemp = SMXReAlloc(pManagerSession->hDevice, pBuffer, nNewBufferSize, false);
      if (pTemp == NULL)
      {
         nError = SM_ERROR_OUT_OF_MEMORY;
         goto error;
      }

      pBuffer = pTemp;
      memmove(pBuffer + 4, pBuffer, nBufferSize);
      *(uint32_t*)pBuffer = nBufferSize + 4;

      if (nPropertyBufferLen - i >= nPropertyNameLength)
      {
         /* copy property name... */
         SMXStr2Wchar(
            (wchar_t*)(pBuffer + 4 + nBufferSize),
            (char*)&pPropertyBuffer[i],
            nPropertyNameLength);
         /* ... and set the null-terminated character */
         *(wchar_t*)(pBuffer + 4 + nBufferSize + (nPropertyNameLength * sizeof(wchar_t))) = 0;
         i += nPropertyNameLength;
      }
      else
      {
         /* Buffer overflow */
         nError = SM_ERROR_GENERIC;
         goto error;
      }

      nBufferSize = nNewBufferSize;
      nPropertiesCount++;

      /* Read second string: Property value */
      nNewBufferSize += 4;
      /* Allocate room for a null-terminated *wchar_t* string */
      nNewBufferSize += (nPropertyValueLength + 1) * sizeof(wchar_t);
      pTemp = SMXReAlloc(pManagerSession->hDevice, pBuffer, nNewBufferSize, false);
      if (pTemp == NULL)
      {
         nError = SM_ERROR_OUT_OF_MEMORY;
         goto error;
      }
      pBuffer = pTemp;

      memmove(pBuffer + 4, pBuffer, nBufferSize);
      *(uint32_t*)pBuffer = nBufferSize + 4;
      if (nPropertyBufferLen - i >= nPropertyValueLength)
      {
         /* copy property value... */
         SMXStr2Wchar(
            (wchar_t*)(pBuffer + 4 + nBufferSize), 
            (char*)&pPropertyBuffer[i],
            nPropertyValueLength);
         /* ... and set the null-terminated character */
         *(wchar_t*)(pBuffer + 4 + nBufferSize + (nPropertyValueLength * sizeof(wchar_t))) = 0;
         i += nPropertyValueLength;
      }
      else
      {
         /* Buffer overflow */
         nError = SM_ERROR_GENERIC;
         goto error;
      }

      nBufferSize = nNewBufferSize;
      nPropertiesCount++;

       /* Alignement for the next property */
      i = (i + 3) & (~3);
   }

   /*
    * Update relative offsets (in the buffer) into absolute pointers.
    * And re-order properties.
    */
   {
      wchar_t** ppStringArray;

      ppStringArray = (wchar_t**)pBuffer;

      for (i = 0; i < nPropertiesCount; i++)
      {
         uint32_t nValue;
         nValue = (uint32_t)ppStringArray[i];   /* Offset */
         nValue += (uint32_t)pBuffer;
         nValue += i * 4;
         ppStringArray[i] = (wchar_t*)(nValue);
      }

      for (i = 0; i < nPropertiesCount / 2; i++)
      {
         wchar_t* pTemp;
         pTemp = ppStringArray[i];
         ppStringArray[i] = ppStringArray[nPropertiesCount-i-1];
         ppStringArray[nPropertiesCount-i-1] = pTemp;
      }
   }

   /*
   * Successful completion.
   */

   SMXUnLockCriticalSection(&pManagerSession->pDeviceContext->sCriticalSection);

   SMFree(pManagerSession->hDevice, pPropertyBuffer);

   if (nPropertyBufferLen == 0)
   {
      SMFree(pManagerSession->hDevice, pBuffer);
   }
   else
   {
      *ppProperties = (SM_PROPERTY*)pBuffer;
      *pnPropertiesLength = nPropertiesCount/2;
   }

   TRACE_INFO("SMManagerGetAllServiceProperties(0x%X): Success"
      " (*ppProperties = %p, *pnPropertiesLength = %u)",
      hServiceManager, *ppProperties, *pnPropertiesLength);
   return SM_SUCCESS;

   /*
   * Error handling.
   */

error:
   TRACE_INFO("SMManagerGetAllServiceProperties(0x%X): Error 0x%X",
      hServiceManager, nError);
   SMXUnLockCriticalSection(&pManagerSession->pDeviceContext->sCriticalSection);
   SMFree(pManagerSession->hDevice, pPropertyBuffer);
   SMFree(pManagerSession->hDevice, pBuffer);
   return nError;
}


/*---------------------------------------------------------------------------
* Download Service
*---------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMManagerDownloadService(
   SM_HANDLE      hServiceManager,
   const uint8_t* pServiceCode,
   uint32_t       nServiceCodeSize,
   SM_UUID*       pidService)
{
   TEEC_Result  nTeeError=TEEC_SUCCESS;
   SM_ERROR    nError;
   TEEC_Operation sOperation;
   SMX_MANAGER_SESSION* pManagerSession = NULL;

   TRACE_INFO("SMManagerDownloadService(0x%X, %p, %u, %p)",
      hServiceManager, pServiceCode, nServiceCodeSize, pidService);
   
  /*
   * Validate parameters.
   */
   if (  (hServiceManager == SM_HANDLE_INVALID) ||
         (pidService == NULL) ||
         (pServiceCode == NULL) ||
         (nServiceCodeSize == 0))
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   /*
   * Get the TEEC session handle.
   */
   pManagerSession = (SMX_MANAGER_SESSION*)SMXGetPointerFromHandle(hServiceManager);
   if (pManagerSession == SM_HANDLE_INVALID)
   {
      TRACE_ERROR("SMManagerDownloadService(0x%X): Invalid hServiceManager parameter (0x%X)",
         hServiceManager, hServiceManager);
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }
   
   if (pManagerSession->pDeviceContext == NULL)
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   SMXLockCriticalSection(&pManagerSession->pDeviceContext->sCriticalSection);

   /* Download the service */
   memset(&sOperation, 0, sizeof(TEEC_Operation));

   sOperation.paramTypes = TEEC_PARAM_TYPES(
      TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE);

   sOperation.params[0].tmpref.buffer = (void*)pServiceCode;
   sOperation.params[0].tmpref.size   = nServiceCodeSize;
   sOperation.params[1].tmpref.buffer = pidService;
   sOperation.params[1].tmpref.size   = sizeof(*pidService);

   nTeeError = TEEC_InvokeCommand(&pManagerSession->sTeecSession,
                               SERVICE_MANAGER_COMMAND_ID_DOWNLOAD_SERVICE,
                               &sOperation,     /* IN OUT operation */
                               NULL             /* OUT errorOrigin, optional */
                              );

   if (nTeeError != TEEC_SUCCESS)
   {
      /* No need to convert TEEC error into SMAPI error */
      nError = nTeeError;
      goto error;
   }

   TRACE_INFO("SMManagerDownloadService(0x%X): Success (%p, %u, %p)",
      hServiceManager, pServiceCode, nServiceCodeSize, pidService);

   SMXUnLockCriticalSection(&pManagerSession->pDeviceContext->sCriticalSection);
   return SM_SUCCESS;

error:
   TRACE_ERROR("SMManagerDownloadService(0x%X): Error 0x%X",
      hServiceManager, nError);

   SMXUnLockCriticalSection(&pManagerSession->pDeviceContext->sCriticalSection);

   return nError;
}


/*---------------------------------------------------------------------------
* Remove Service
*---------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMManagerRemoveService(
   SM_HANDLE      hServiceManager,
   const SM_UUID* pidService)
{
   TEEC_Result  nTeeError=TEEC_SUCCESS;
   SM_ERROR    nError;
   TEEC_Operation sOperation;
   SMX_MANAGER_SESSION* pManagerSession = NULL;

   TRACE_INFO("SMManagerRemoveService(0x%X, %p)",
      hServiceManager, pidService);
   
  /*
   * Validate parameters.
   */
   if (  (hServiceManager == SM_HANDLE_INVALID) ||
         (pidService == NULL))
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   /*
   * Get the TEEC session handle.
   */
   pManagerSession = (SMX_MANAGER_SESSION*)SMXGetPointerFromHandle(hServiceManager);
   if (pManagerSession == SM_HANDLE_INVALID)
   {
      TRACE_ERROR("SMManagerRemoveService(0x%X): Invalid hServiceManager parameter (0x%X)",
         hServiceManager, hServiceManager);
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }
   
   if (pManagerSession->pDeviceContext == NULL)
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   SMXLockCriticalSection(&pManagerSession->pDeviceContext->sCriticalSection);

   /* Remove the service */
   memset(&sOperation, 0, sizeof(TEEC_Operation));

   sOperation.paramTypes = TEEC_PARAM_TYPES(
      TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

   sOperation.params[0].tmpref.buffer = (void*)pidService;
   sOperation.params[0].tmpref.size   = sizeof(*pidService);

   nTeeError = TEEC_InvokeCommand(&pManagerSession->sTeecSession,
                               SERVICE_MANAGER_COMMAND_ID_REMOVE_SERVICE,
                               &sOperation,     /* IN OUT operation */
                               NULL             /* OUT errorOrigin, optional */
                              );

   if (nTeeError != TEEC_SUCCESS)
   {
      /* No need to convert TEEC error into SMAPI error */
      nError = nTeeError;
      goto error;
   }

   TRACE_INFO("SMManagerRemoveService(0x%X): Success (%p)",
      hServiceManager, pidService);

   SMXUnLockCriticalSection(&pManagerSession->pDeviceContext->sCriticalSection);
   return SM_SUCCESS;

error:
   TRACE_ERROR("SMManagerRemoveService(0x%X): Error 0x%X",
      hServiceManager, nError);

   SMXUnLockCriticalSection(&pManagerSession->pDeviceContext->sCriticalSection);

   return nError;
}


/*---------------------------------------------------------------------------
* Implementation Property Management
*---------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMGetImplementationProperty(
   SM_HANDLE       hDevice,
   const wchar_t*  pPropertyName,
   wchar_t**       ppPropertyValue)
{
   SM_ERROR nError = SM_SUCCESS;
   uint32_t i;
   bool bFound = false;
   SM_DEVICE_CONTEXT* pDevice = NULL;

   /*
   * Check parameters.
   */
   if(ppPropertyValue != NULL)
   {
      *ppPropertyValue = NULL;
   }

   if((pPropertyName == NULL)||
      (ppPropertyValue == NULL))
   {
      TRACE_ERROR("SMGetImplementationProperty(0x%X): Invalid pPropertyName parameter (%p)",
         hDevice, pPropertyName);
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   if (hDevice != SM_HANDLE_INVALID)
   {
      /*
      * Retrieve the device.
      */
      nError = SMXLockObjectFromHandle(
         hDevice,
         (void*)&pDevice,
         TFAPI_OBJECT_TYPE_DEVICE_CONTEXT);
      if (nError != SM_SUCCESS)
      {
         TRACE_ERROR("SMGetImplementationProperty(0x%X): SMXDeviceFromHandleEnterCS failed (error 0x%X)",
            hDevice, nError);
         goto end;
      }
   }

   /*
   * Look for the property value.
   */

   for (i = 0; g_smxImplPropertyHandlers[i].pPropertyName != NULL; i++)
   {
      const SMX_IMPL_PROPERTY_HANDLER *pHandler;

      pHandler = &(g_smxImplPropertyHandlers[i]);
      if (SMXCompareWcharZString(pHandler->pPropertyName, pPropertyName) != false)
      {
         bFound = true;
         nError = pHandler->pGetterFunc(pDevice, ppPropertyValue);
         if (nError != SM_SUCCESS)
         {
            TRACE_ERROR("SMGetImplementationProperty(0x%X): Getter failed (error 0x%X)",
               hDevice, nError);
            goto end;
         }

         goto end;
      }
   }

end:
   SMXUnLockObject((SMX_NODE*)pDevice);

   if ((nError == SM_SUCCESS) && !bFound)
   {
      /*
      * Unknown property.
      */

      TRACE_WARNING("SMGetImplementationProperty(0x%X): Unknown property",
         hDevice);
      return SM_ERROR_ITEM_NOT_FOUND;
   }

   return nError;
}

/*---------------------------------------------------------------------------*/

SM_EXPORT SM_ERROR SMGetAllImplementationProperties(
   SM_HANDLE     hDevice,
   SM_PROPERTY** ppProperties,
   uint32_t*     pnPropertiesLength)
{
   SM_ERROR nError = SM_SUCCESS;
   SM_PROPERTY *pProperties = NULL;
   uint32_t nMaxPropertyCount;
   uint32_t nPropertyCount=0;
   uint32_t nBlockSize;
   uint32_t i;
   SM_DEVICE_CONTEXT* pDevice = NULL;
   SM_PROPERTY *pReturnedProperties = NULL;
   uint8_t* pOffset;
   uint32_t length;

   TRACE_INFO("SMGetAllImplementationProperties");

   /*
   * Check parameters.
   */

   if(ppProperties != NULL)
   {
      *ppProperties = NULL;
   }

   if(pnPropertiesLength != NULL)
   {
      *pnPropertiesLength = 0;
   }

   if((ppProperties == NULL)||
      (pnPropertiesLength == NULL))
   {
      return SM_ERROR_ILLEGAL_ARGUMENT;
   }

   /*
   * Count the total number of properties defined by the implementation.
   */

   nMaxPropertyCount = 0;
   while (g_smxImplPropertyHandlers[nMaxPropertyCount].pPropertyName != NULL)
   {
      nMaxPropertyCount++;
   }

   if (nMaxPropertyCount == 0)
   {
      return SM_SUCCESS;
   }

   if (hDevice != SM_HANDLE_INVALID)
   {
      /*
      * Retrieve the device.
      */
      nError = SMXLockObjectFromHandle(
         hDevice,
         (void*)&pDevice,
         TFAPI_OBJECT_TYPE_DEVICE_CONTEXT);
      if (nError != SM_SUCCESS)
      {
         TRACE_ERROR("SMGetAllImplementationProperties(0x%X): SMXDeviceFromHandleEnterCS failed (error 0x%X)",
            hDevice, nError);
         goto end;
      }
   }

   /*
   * Allocate the initial property array.
   */

   nBlockSize = nMaxPropertyCount * sizeof(SM_PROPERTY);
   if (pDevice!=NULL)
   {
      SMX_NODE* pNode;
      pNode=SMXDeviceContextAlloc(pDevice,nBlockSize+sizeof(SMX_NODE));
      if (pNode!=NULL)
      {
         pProperties=(SM_PROPERTY*)((SMX_BUFFER*)pNode)->buffer;
      }
   }
   else
   {
      pProperties=(SM_PROPERTY*)malloc(nBlockSize);
   }
   if (pProperties == NULL)
   {
      TRACE_ERROR("SMGetAllImplementationProperties(0x%X): Out of memory for initial block (%u bytes)",
         hDevice, nBlockSize);
      nError = SM_ERROR_OUT_OF_MEMORY;
      goto end;
   }
   memset(pProperties, 0, nBlockSize);

   /*
   * Retrieve the implementation properties.
   */

   nBlockSize = 0;
   nPropertyCount = 0;
   for (i = 0; i < nMaxPropertyCount; i++)
   {
      const SMX_IMPL_PROPERTY_HANDLER *pHandler;
      wchar_t *pPropertyValue;

      pHandler = g_smxImplPropertyHandlers+i;

      pPropertyValue = NULL;
      nError = pHandler->pGetterFunc(pDevice, &pPropertyValue);
      if (nError != SM_SUCCESS)
      {
         /* SM_ERROR_ITEM_NOT_FOUND means the property is device-dependent and cannot
         be retrieved. We reset the error and go to the next property */
         if (nError == SM_ERROR_ITEM_NOT_FOUND)
         {
            nError = SM_SUCCESS;
         }
         else
         {
            TRACE_ERROR("SMGetAllImplementationProperties(0x%X): Getter failed (error 0x%X)",
               hDevice, nError);
            goto end;
         }
      }
      if(pPropertyValue != NULL)
      {
         TRACE_INFO("SMGetAllImplementationProperties: %ls : %ls",pHandler->pPropertyName,pPropertyValue);
         pProperties[nPropertyCount].pName = (wchar_t*)pHandler->pPropertyName;
         nBlockSize+= (SMXGetWcharZStringLength(pHandler->pPropertyName) + 1) * sizeof(wchar_t);
         pProperties[nPropertyCount].pValue = pPropertyValue;
         nBlockSize+= (SMXGetWcharZStringLength(pPropertyValue) + 1) * sizeof(wchar_t);
         nPropertyCount++;
      }
   }
   if(nPropertyCount != 0)
   {
      nBlockSize += nPropertyCount * sizeof(SM_PROPERTY);
      if (pDevice!=NULL)
      {
         SMX_NODE* pNode;
         pNode=SMXDeviceContextAlloc(pDevice,nBlockSize+sizeof(SMX_NODE));
         if (pNode!=NULL)
         {
            pReturnedProperties=(SM_PROPERTY*)((SMX_BUFFER*)pNode)->buffer;
         }
      }
      else
      {
         pReturnedProperties=(SM_PROPERTY*)malloc(nBlockSize);
      }
      if (pReturnedProperties == NULL)
      {
         TRACE_ERROR("SMGetAllImplementationProperties(0x%X): Out of memory for initial block (%u bytes)",
            hDevice, nBlockSize);
         nError = SM_ERROR_OUT_OF_MEMORY;
         goto end;
      }
   }
   *ppProperties = pReturnedProperties;
   *pnPropertiesLength = nPropertyCount;
   TRACE_INFO("SMGetAllImplementationProperties: New table of properties (size=%d)",nBlockSize);
   if(pReturnedProperties != NULL)
   {
      pOffset = ((uint8_t*)pReturnedProperties) + (nPropertyCount * sizeof(SM_PROPERTY));
      for (i = 0; i < nPropertyCount; i++)
      {
         length = (SMXGetWcharZStringLength(pProperties[i].pName) + 1) * sizeof(wchar_t);
         if (pProperties[i].pName != NULL)
         {
            memcpy(
               pOffset,
               pProperties[i].pName,
               length);
            pReturnedProperties->pName = (wchar_t*)pOffset;
         }
         pOffset+= length;

         length = (SMXGetWcharZStringLength(pProperties[i].pValue) + 1) * sizeof(wchar_t);
         if (pProperties[i].pValue != NULL)
         {
            memcpy(
               pOffset,
               pProperties[i].pValue,
               length);
            pReturnedProperties->pValue = (wchar_t*)pOffset;
         }
         pOffset+= length;

         pReturnedProperties++;
      }
   }

end:
   TRACE_INFO("SMGetAllImplementationProperties: Cleanup");
   if(pProperties != NULL)
   {
      /*
      * DO NOT CALL THE SMAPI SMFree to avoid mutex "recursivity"
      * as the mutex used may not support this feature.
      */
      if (pDevice!=NULL)
      {
         SMX_NODE* pNode;
         for (i = 0; i < nPropertyCount; i++)
         {
            if (pProperties[i].pValue != NULL)
            {
               pNode=(SMX_NODE*)((uint8_t*)(pProperties[i].pValue)-offsetof(SMX_BUFFER,buffer));
               SMXDeviceContextFree(pDevice,pNode);
            }
         }
         pNode=(SMX_NODE*)((uint8_t*)(pProperties)-offsetof(SMX_BUFFER,buffer));
         SMXDeviceContextFree(pDevice,pNode);
      }
      else
      {
         for (i = 0; i < nPropertyCount; i++)
         {
            free(pProperties[i].pValue);
         }
         free(pProperties);
      }
      pProperties = NULL;
   }

   if (pDevice!=NULL)
   {
      SMXUnLockObject((SMX_NODE*)pDevice);
   }

   if (nError != SM_SUCCESS)
   {
      /*
      * Error handling.
      */
      TRACE_ERROR("SMGetAllImplementationProperties(0x%X): Error 0x%X",
         hDevice, nError);
   }
   return nError;
}

/*---------------------------------------------------------------------------*/

static SM_ERROR static_SMXGetApiVersionImplProperty(
   SM_DEVICE_CONTEXT* pDevice,
   wchar_t **ppPropertyValue)
{
   /*
   * Do not get the device lock in SMXAlloc
   * as it is already taken by the caller function.
   * This is to avoid mutex "recursivity",
   * as the mutex used may not support this feature.
   */
   wchar_t *pApiVersion;
   wchar_t *pBuffer;
   uint32_t nChunkLength;

   *ppPropertyValue = NULL;

   /* Max is "255.255\0" */
   if(pDevice == NULL)
   {
      pApiVersion = (wchar_t *)SMXAlloc(
         SM_HANDLE_INVALID,
         8 * sizeof(wchar_t),
         false);
   }
   else
   {
      pApiVersion = (wchar_t *)SMXAlloc(
         SMXGetHandleFromPointer(pDevice),
         8 * sizeof(wchar_t),
         false);
   }
   if (pApiVersion == NULL)
   {
      TRACE_ERROR("SMXGetApiVersionImplProperty(%p): Out of memory",
         pDevice);
      return SM_ERROR_OUT_OF_MEMORY;
   }

   pBuffer = pApiVersion;

   SMXWcharItoA((SM_API_VERSION & 0xFF000000) >> 24, 10, pBuffer, 3, &nChunkLength);
   pBuffer += nChunkLength;

   *pBuffer++ = (wchar_t)'.';

   SMXWcharItoA((SM_API_VERSION & 0x00FF0000) >> 16, 10, pBuffer, 3, &nChunkLength);
   pBuffer += nChunkLength;

   *pBuffer++ = 0;

   *ppPropertyValue = pApiVersion;

   return SM_SUCCESS;
}

/*---------------------------------------------------------------------------*/

static SM_ERROR static_SMXGetApiDescriptionImplProperty(
   SM_DEVICE_CONTEXT* pDevice,
   wchar_t **ppPropertyValue)
{
   /*
   * Do not get the device lock in SMXAlloc
   * as it is already taken by the caller function.
   * This is to avoid mutex "recursivity",
   * as the mutex used may not support this feature.
   */
   wchar_t *pApiDescription;
   uint32_t length;

   *ppPropertyValue = NULL;

   length = strlen(S_VERSION_STRING);

   if(pDevice == NULL)
   {
      pApiDescription = (wchar_t *)SMXAlloc(
         SM_HANDLE_INVALID,
         (length+1)*sizeof(wchar_t),
         false);
   }
   else
   {
      pApiDescription = (wchar_t *)SMXAlloc(
         SMXGetHandleFromPointer(pDevice),
         (length+1)*sizeof(wchar_t),
         false);
   }
   if (pApiDescription == NULL)
   {
      TRACE_ERROR("SMXGetApiDescriptionImplProperty(%p): Out of memory",
         pDevice);
      return SM_ERROR_OUT_OF_MEMORY;
   }

   SMXStr2Wchar(
      pApiDescription,
      S_VERSION_STRING,
      length+1);

   *ppPropertyValue = pApiDescription;

   return SM_SUCCESS;
}

/*---------------------------------------------------------------------------*/

static SM_ERROR static_SMXGetDeviceNameImplProperty(
   SM_DEVICE_CONTEXT* pDevice,
   wchar_t **ppPropertyValue)
{

   MD_VAR_NOT_USED(pDevice);
   MD_VAR_NOT_USED(ppPropertyValue);
   /* This property is no longer supported */
   return SM_ERROR_ITEM_NOT_FOUND;
}

/*---------------------------------------------------------------------------*/

static SM_ERROR static_SMXGetDeviceDescriptionImplProperty(
   SM_DEVICE_CONTEXT* pDevice,
   wchar_t **ppPropertyValue)
{
   /*
   * Do not get the device lock in SMXAlloc
   * as it is already taken by the caller function.
   * This is to avoid mutex "recursivity",
   * as the mutex used may not support this feature.
   */
   TEEC_Result    nError;
   TEEC_Context   sContext;
   TEEC_ImplementationInfo sDescription;
   wchar_t*       pVersionInfo;

   *ppPropertyValue = NULL;

   if (pDevice == NULL)
   {
      /* we need a device context */
      return SM_ERROR_ITEM_NOT_FOUND;
   }
   pVersionInfo = (wchar_t *)SMXAlloc(
      SMXGetHandleFromPointer(pDevice), sizeof(sDescription.TEEDescription) * sizeof(wchar_t), false);
   if (pVersionInfo == NULL)
   {
      TRACE_ERROR("SMXGetDeviceDescriptionImplProperty(%p): Out of memory",
         pDevice);
      return SM_ERROR_OUT_OF_MEMORY;
   }

   nError = TEEC_InitializeContext(NULL,  /* const char * name */
                                   &sContext);   /* TEEC_Context* context */
   if (nError != TEEC_SUCCESS)
   {
      TRACE_ERROR("SMXGetDeviceDescriptionImplProperty(%p): TEEC_InitializeContext error [0x%x]",
         pDevice, nError);
      /* No need to convert TEEC error into SMAPI error */
      return nError;
   }

   TEEC_GetImplementationInfo(&sContext, &sDescription);
   TEEC_FinalizeContext(&sContext);

   SMXStr2Wchar(
      pVersionInfo,
      sDescription.TEEDescription,
      sizeof(sDescription.TEEDescription));

   *ppPropertyValue = pVersionInfo;

   return SM_SUCCESS;
}

/*---------------------------------------------------------------------------*/

static SM_ERROR static_SMXGetDriverDescriptionImplProperty(
   SM_DEVICE_CONTEXT* pDevice,
   wchar_t **ppPropertyValue)
{
   /*
   * Do not get the device lock in SMXAlloc
   * as it is already taken by the caller function.
   * This is to avoid mutex "recursivity",
   * as the mutex used may not support this feature.
   */
   TEEC_Result    nError;
   TEEC_Context   sContext;
   TEEC_ImplementationInfo sDescription;
   wchar_t* pVersionInfo;

   *ppPropertyValue = NULL;

   if (pDevice == NULL)
   {
      /* we need a device context */
      return SM_ERROR_ITEM_NOT_FOUND;
   }
   pVersionInfo = (wchar_t *)SMXAlloc(
      SMXGetHandleFromPointer(pDevice), sizeof(sDescription.commsDescription) * sizeof(wchar_t), false);
   if (pVersionInfo == NULL)
   {
      TRACE_ERROR("SMXGetDriverDescriptionImplProperty(%p): Out of memory",
         pDevice);
      return SM_ERROR_OUT_OF_MEMORY;
   }

   nError = TEEC_InitializeContext(NULL,  /* const char * name */
                                   &sContext);   /* TEEC_Context* context */
   if (nError != TEEC_SUCCESS)
   {
      TRACE_ERROR("SMXGetDriverDescriptionImplProperty(%p): TEEC_InitializeContext error [0x%x]",
         pDevice, nError);
      /* No need to convert TEEC error into SMAPI error */
      return nError;
   }

   TEEC_GetImplementationInfo(&sContext, &sDescription);
   TEEC_FinalizeContext(&sContext);

   SMXStr2Wchar(
      pVersionInfo,
      sDescription.commsDescription,
      sizeof(sDescription.commsDescription));

   *ppPropertyValue = pVersionInfo;

   return SM_SUCCESS;
}
