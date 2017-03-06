/**
 * Copyright (c) 2006-2009 Trusted Logic S.A.
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

/* This example depends on the Advanced profile of the Trusted Foundations */

#include <stdlib.h>
#include <string.h>

#include "tee_client_api.h"
#include "drm_protocol.h"
#include "drm_stub.h"

#define DRM_EXPORT_API S_DLL_EXPORT

static const TEEC_UUID SERVICE_UUID = SERVICE_DRM_UUID;
static const TEEC_UUID STIPLET_UUID = STIPLET_DRM_UUID;

/* There is one global device context */
static TEEC_Context g_sContext;
static bool         g_bInitialized = false;

/* This structure contains the data associated with a decryption session */
typedef struct
{
   TEEC_Session      sSession;
   TEEC_SharedMemory sSharedMem;
   bool      bUseStiplet;
}
DECRYPT_SESSION_CONTEXT;

/* Note that for a management session, the session handle is a direct pointer
   on the TEEC_Session structure */

/* ------------------------------------------------------------------------
                          Internal error management
------------------------------------------------------------------------- */

static DRM_ERROR static_convertErrorCode(TEEC_Result nError)
{
   switch (nError)
   {
      case TEEC_SUCCESS:
         return DRM_SUCCESS;
      case TEEC_ERROR_BAD_PARAMETERS:
         return DRM_ERROR_ILLEGAL_ARGUMENT;
      case TEEC_ERROR_BAD_STATE:
         return DRM_ERROR_ILLEGAL_STATE;
      case TEEC_ERROR_OUT_OF_MEMORY:
         return DRM_ERROR_OUT_OF_MEMORY;
      case DRM_AGENT_ERROR_CRYPTO:
         return DRM_ERROR_CRYPTO;
      case DRM_AGENT_ERROR_ALREADY_INITIALIZED:
         return DRM_ERROR_KEY_ALREADY_INITIALIZED;
      default:
         return DRM_ERROR_GENERIC;
   }
}

/* ------------------------------------------------------------------------
                          Public Functions
------------------------------------------------------------------------- */


DRM_ERROR DRM_EXPORT_API drm_initialize( void )
{
   TEEC_Result    nTeeError;

   nTeeError = TEEC_InitializeContext(NULL, &g_sContext);
   if (nTeeError != TEEC_SUCCESS)
   {
      return static_convertErrorCode(nTeeError);
   }
   g_bInitialized = true;
   return DRM_SUCCESS;
}

DRM_ERROR DRM_EXPORT_API drm_finalize( void )
{
   if (!g_bInitialized)
   {
      return DRM_ERROR_NOT_INITIALIZED;
   }

   TEEC_FinalizeContext(&g_sContext);
   g_bInitialized = false;
   return DRM_SUCCESS;
}

DRM_ERROR DRM_EXPORT_API drm_open_management_session ( DRM_SESSION_HANDLE* pSession, bool bUseStiplet )
{
   TEEC_Result              nTeeError;
   TEEC_Operation           sOperation;
   const TEEC_UUID*         pServiceUUID;
   TEEC_Session*            session;

   /* Check drm agent is initialized */
   if (!g_bInitialized)
   {
      return DRM_ERROR_NOT_INITIALIZED;
   }

   if (bUseStiplet)
   {
      pServiceUUID = &STIPLET_UUID;
   }
   else
   {
      pServiceUUID = &SERVICE_UUID;
   }

   session = (TEEC_Session*)malloc(sizeof(TEEC_Session));
   if (session == NULL)
   {
      return DRM_ERROR_OUT_OF_MEMORY;
   }
   sOperation.paramTypes = TEEC_PARAM_TYPES(
      TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].value.a = (uint32_t)DRM_AGENT_MANAGEMENT_SESSION;
   sOperation.params[0].value.b = 0;
   nTeeError = TEEC_OpenSession(&g_sContext,
                             session,                    /* OUT session */
                             pServiceUUID,              /* destination UUID */
                             TEEC_LOGIN_APPLICATION,          /* connectionMethod */
                             NULL,                       /* connectionData */
                             &sOperation,                /* IN OUT operation */
                             NULL                        /* OUT returnOrigin, optional */
                             );
   if (nTeeError == TEEC_SUCCESS)
   {
      *pSession = (DRM_SESSION_HANDLE)session;
   }
   else
   {
      *pSession = DRM_SESSION_HANDLE_INVALID;
   }
   return static_convertErrorCode(nTeeError);
}


DRM_ERROR DRM_EXPORT_API drm_close_management_session( DRM_SESSION_HANDLE hSession )
{
   TEEC_Session*           session;

   /* Check DRM Agent is initialized */
   if (!g_bInitialized)
   {
      return DRM_ERROR_NOT_INITIALIZED;
   }
   if (hSession == DRM_SESSION_HANDLE_INVALID)
   {
      return DRM_ERROR_SESSION_NOT_OPEN;
   }
   session = (TEEC_Session*)hSession;
   TEEC_CloseSession(session);
   free(session);
   return DRM_SUCCESS;
}

DRM_ERROR DRM_EXPORT_API drm_management_init( DRM_SESSION_HANDLE hSession )
{
   TEEC_Result              teecErr;
   TEEC_Session*            session;

   /* Check hSession is valid */
   if (hSession == DRM_SESSION_HANDLE_INVALID)
   {
      return DRM_ERROR_SESSION_NOT_OPEN;
   }

   session = (TEEC_Session*)hSession;
   teecErr = TEEC_InvokeCommand(session,
      DRM_AGENT_CMD_INIT,
      NULL,              /* IN OUT operation: no payload */
      NULL               /* OUT returnOrigin, optional */
   );
   return static_convertErrorCode(teecErr);
}

DRM_ERROR DRM_EXPORT_API drm_management_clean( DRM_SESSION_HANDLE hSession )
{
   TEEC_Result              teecErr;
   TEEC_Session*            session;

   /* Check hSession is valid */
   if (hSession == DRM_SESSION_HANDLE_INVALID)
   {
      return DRM_ERROR_SESSION_NOT_OPEN;
   }

   session = (TEEC_Session*)hSession;
   teecErr = TEEC_InvokeCommand(session,
      DRM_AGENT_CMD_CLEAN,
      NULL,              /* IN OUT operation: no payload */
      NULL               /* OUT returnOrigin, optional */
   );
   return static_convertErrorCode(teecErr);
}


DRM_ERROR DRM_EXPORT_API drm_open_decrypt_session (unsigned char* pEncryptedKey,
                                                   unsigned int nEncryptedKeyLen,
                                                   unsigned char** ppBlock,
                                                   unsigned int nBlockLen,
                                                   DRM_SESSION_HANDLE* phSession,
                                                   bool bUseStiplet)
{
   TEEC_Result              nTeeError;
   TEEC_Operation           sOperation;
   DECRYPT_SESSION_CONTEXT* pDecryptSessionContext;
   const TEEC_UUID*         pServiceUUID;
   char*                    pSignatureFile = NULL;
   uint32_t                 nSignatureFileLen  = 0;

   /* Check drm agent is initialized */
   if (!g_bInitialized)
   {
      return DRM_ERROR_NOT_INITIALIZED;
   }

   if (bUseStiplet)
   {
      pServiceUUID = &STIPLET_UUID;
   }
   else
   {
      pServiceUUID = &SERVICE_UUID;
   }

   /* Check drm agent is initialized */
   if (!g_bInitialized)
   {
      return DRM_ERROR_NOT_INITIALIZED;
   }

   if (nBlockLen < DRM_BLOCK_SIZE)
   {
      return DRM_ERROR_ILLEGAL_ARGUMENT;
   }

   /* Allocate the decrypt session context */
   pDecryptSessionContext = malloc(sizeof(DECRYPT_SESSION_CONTEXT));
   if (pDecryptSessionContext == NULL)
   {
      return DRM_ERROR_OUT_OF_MEMORY;
   }
   memset(pDecryptSessionContext, 0, sizeof(DECRYPT_SESSION_CONTEXT));

   sOperation.paramTypes = TEEC_PARAM_TYPES(
       TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_MEMREF_TEMP_INPUT);
   sOperation.params[0].value.a = (uint32_t)DRM_AGENT_DECRYPT_SESSION; /* session tag */
   sOperation.params[0].value.b = 0;
   sOperation.params[1].tmpref.buffer = pEncryptedKey;
   sOperation.params[1].tmpref.size   = nEncryptedKeyLen; /* encrypted key */

   /* Get signature file  and send it in param3 of sOperation*/
   nTeeError = TEEC_ReadSignatureFile((void **)&pSignatureFile, &nSignatureFileLen);
   if (nTeeError != TEEC_SUCCESS)
   {
       goto error_free_context;
   }
   sOperation.params[3].tmpref.buffer = pSignatureFile;
   sOperation.params[3].tmpref.size   = nSignatureFileLen;

   nTeeError = TEEC_OpenSession(&g_sContext,
         &pDecryptSessionContext->sSession,  /* OUT session */
         pServiceUUID,                       /* destination UUID */
         TEEC_LOGIN_AUTHENTICATION,         /* connectionMethod */
         NULL,                               /* connectionData */
         &sOperation,                        /* IN OUT operation */
         NULL                                /* OUT returnOrigin, optional */
         );
   if (nTeeError != TEEC_SUCCESS)
   {
      goto error_free_context;
   }

   /* The data is always exchanged using a shared memory block */
   /* Allocate or register shared memory block */
   if (*ppBlock == NULL)
   {
      pDecryptSessionContext->sSharedMem.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
      pDecryptSessionContext->sSharedMem.size  = nBlockLen;
         /* Allocate shared memory buffer */
      nTeeError = TEEC_AllocateSharedMemory(&g_sContext, &pDecryptSessionContext->sSharedMem);
      if (nTeeError != TEEC_SUCCESS)
      {
         goto error_close_session;
      }
      *ppBlock = pDecryptSessionContext->sSharedMem.buffer;
   }
   else
   {
      pDecryptSessionContext->sSharedMem.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
      pDecryptSessionContext->sSharedMem.size  = nBlockLen;
      pDecryptSessionContext->sSharedMem.buffer = *ppBlock;
         /* Register shared memory buffers */
      nTeeError = TEEC_RegisterSharedMemory(&g_sContext, &pDecryptSessionContext->sSharedMem);
      if (nTeeError != TEEC_SUCCESS)
      {
         goto error_close_session;
      }
   }

   pDecryptSessionContext->bUseStiplet = bUseStiplet;

   *phSession = (DRM_SESSION_HANDLE)pDecryptSessionContext;

   return DRM_SUCCESS;

error_close_session:
   TEEC_CloseSession(&pDecryptSessionContext->sSession);
error_free_context:
   free(pDecryptSessionContext);
   *phSession = DRM_SESSION_HANDLE_INVALID;
   return static_convertErrorCode(nTeeError);
}

DRM_ERROR DRM_EXPORT_API drm_close_decrypt_session( DRM_SESSION_HANDLE hSession )
{
   DECRYPT_SESSION_CONTEXT* pDecryptSessionContext;

   /* Check DRM Agent is initialized */
   if (!g_bInitialized)
   {
      return DRM_ERROR_NOT_INITIALIZED;
   }

   pDecryptSessionContext = (DECRYPT_SESSION_CONTEXT*)hSession;
   if (pDecryptSessionContext == NULL)
   {
      return DRM_SUCCESS;
   }

   TEEC_ReleaseSharedMemory(&pDecryptSessionContext->sSharedMem);
   TEEC_CloseSession(&pDecryptSessionContext->sSession);
   free(pDecryptSessionContext);
   return DRM_SUCCESS;
}

DRM_ERROR DRM_EXPORT_API drm_decrypt(DRM_SESSION_HANDLE  hSession,
                                     uint32_t            nOffset,
                                     uint32_t            nLength)
{
   TEEC_Result              teecErr;
   TEEC_Operation           sOperation;
   DECRYPT_SESSION_CONTEXT* pDecryptSessionContext;

   pDecryptSessionContext = (DECRYPT_SESSION_CONTEXT*)hSession;
   if (pDecryptSessionContext == NULL)
   {
      return DRM_ERROR_SESSION_NOT_OPEN;
   }

   /* Check buffer length */
   if (nOffset + nLength > pDecryptSessionContext->sSharedMem.size)
   {
      return DRM_ERROR_ILLEGAL_ARGUMENT;
   }

   /* The data were copied by the caller in the shared memory block */
   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_INPUT, TEEC_MEMREF_PARTIAL_OUTPUT, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].memref.parent = &pDecryptSessionContext->sSharedMem;
   sOperation.params[0].memref.offset  = 0;
   sOperation.params[0].memref.size   = nLength;
   sOperation.params[1].memref.parent = &pDecryptSessionContext->sSharedMem;
   sOperation.params[1].memref.offset  = 0;
   sOperation.params[1].memref.size   = nLength;

   teecErr = TEEC_InvokeCommand(&pDecryptSessionContext->sSession,
                               DRM_AGENT_CMD_DECRYPT,
                               &sOperation,       /* IN OUT operation */
                               NULL               /* OUT returnOrigin, optional */
                              );
   return static_convertErrorCode(teecErr);
}
