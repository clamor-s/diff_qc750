/**
 * Copyright (c) 2006-2011 Trusted Logic S.A.
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

#include "ssdi.h"
#include "drm_protocol.h"
#include "drm_service_init_data.h"

/*--------------------------------------------------------
 * Internal Types
 *-------------------------------------------------------*/

typedef struct
{
   uint32_t          nSessionType;
   CK_SESSION_HANDLE hCryptoSession;
   CK_OBJECT_HANDLE  hSessionKey;
} SESSION_CONTEXT;

#define VAR_NOT_USED(variable)  do{(void)(variable);}while(0);

/* ----------------------------------------------------------------------------
*   Client properties management Functions
* ---------------------------------------------------------------------------- */
static S_RESULT printClientPropertiesAndUUID(void)
{
   S_UUID      sClientID;
   uint32_t    nPropertyCount;
   S_PROPERTY* pPropertyArray;
   S_RESULT    nError;
   uint8_t     i;

   /* Get and print the Client UUID. */
   SSessionGetClientID(&sClientID);
   SLogTrace("CLIENT UUID: %{uuid}",&sClientID);

   /* Get and print all other client properties. */
   nError = SSessionGetAllClientProperties(&nPropertyCount, &pPropertyArray);
   if (nError != S_SUCCESS)
   {
      SLogError("SSessionGetAllClientProperties() failed (0x%08X).", nError);
      return nError;
   }
   SLogTrace("CLIENT PROPERTIES:");
   for (i = 0; i < nPropertyCount; i++)
   {
      SLogTrace(" %s: %s",pPropertyArray[i].pName, pPropertyArray[i].pValue);
   }

   SMemFree(pPropertyArray);
   return S_SUCCESS;
}


/** Determine if the client has the decryption rights */
static bool getClientEffectiveDecryptionRights(void)
{
   uint32_t    nClientLoginMode;
   bool        bAllowDecryptContent;

   /* Note that the property "sm.client.login" is always available */
   SSessionGetClientPropertyAsInt("sm.client.login", &nClientLoginMode);

   if (nClientLoginMode != S_LOGIN_AUTHENTICATION)
   {
      return false;
   }

   SSessionGetClientPropertyAsBool("example_drm.allow_decrypt_content", &bAllowDecryptContent);
   /* note that if the property does not exist, bAllowDecryptContent is set to false */

   return bAllowDecryptContent;
}


/*------------------------------------------------------------------------
 * Internal Session Management
 *-----------------------------------------------------------------------*/

S_RESULT static_openManagementSession(SESSION_CONTEXT** ppSessionContext)
{
   CK_RV ckr;
   SESSION_CONTEXT* pSessionContext;

   *ppSessionContext = NULL;
   /* allocate session context */
   if ((pSessionContext = (SESSION_CONTEXT*)SMemAlloc(sizeof(SESSION_CONTEXT))) == NULL)
   {
      return S_ERROR_OUT_OF_MEMORY;
   }
   pSessionContext->nSessionType = DRM_AGENT_MANAGEMENT_SESSION;
   /* Open a READ-WRITE session on the service keystore */
   ckr = C_OpenSession(S_CRYPTOKI_KEYSTORE_PRIVATE, CKF_SERIAL_SESSION | CKF_RW_SESSION,
                       NULL, NULL, &pSessionContext->hCryptoSession);
   if (ckr != CKR_OK)
   {
     SLogError("PKCS#11 open session failed [0x%x].\n", ckr);
     SMemFree(pSessionContext);
     return DRM_AGENT_ERROR_CRYPTO;
   }
   /* Login */
   ckr = C_Login(pSessionContext->hCryptoSession, CKU_USER, NULL, 0);
   if (ckr != CKR_OK)
   {
     SLogError("PKCS#11 login failed [0x%x].\n", ckr);
     C_CloseSession(pSessionContext->hCryptoSession);
     SMemFree(pSessionContext);
     return DRM_AGENT_ERROR_CRYPTO;
   }
   /* Save the session context */
   *ppSessionContext = pSessionContext;
   return S_SUCCESS;
}

/**
 * Opens a decryption session.
 * After this function returns, the session key is installed
 * and the decrypt operation is initialized.
 */
S_RESULT static_openDecryptSession(
            SESSION_CONTEXT** ppSessionContext,
            const uint8_t* pCipherKey,
            uint32_t nCipherKeyLen)
{
   SESSION_CONTEXT* pSessionContext;
   S_RESULT errorCode = S_SUCCESS;
   CK_RV ckr;
   CK_BYTE* pClearData = NULL;
   CK_ULONG nClearDataLen = 128;
   CK_ULONG nSessionKeyLen = 16;

   /* =======================================================================
    Variables used to find the device key
   =======================================================================*/
   /* Handle for the device key */
   CK_OBJECT_HANDLE  hDeviceKey;
   /* Device Key ID */
   static const CK_BYTE           pObjId1[] = {0x00, 0x01};
   /* Number of objects we are trying to match */
   CK_ULONG          objectCount;
   /* Create a mechanism for RSA X509 certificates */
   static const CK_MECHANISM RSA_mechanism =
   {
       CKM_RSA_X_509, NULL, 0
   };
   /* Find Template */
   static const CK_ATTRIBUTE findTemplate[] =
   {
      { CKA_ID, (void*)pObjId1, sizeof(pObjId1) }
   };
   /* =======================================================================
    Variables used to for the AES session key
   =======================================================================*/
   /* AES's key class (secret key) */
   static const CK_OBJECT_CLASS   aesKeyClass  = CKO_SECRET_KEY;
   /* AES key type (AES) */
   static const CK_KEY_TYPE       aesKeyType   = CKK_AES;
   /* Dummy variable which we can use for &"TRUE" */
   static const CK_BBOOL          bAesTrue = TRUE;
   /* Object ID for this key for this session. */
   static const CK_BYTE           pObjId2[] = {0x00, 0x02};

   /* Create a key template for the AES key. The attribute CKA_VALUE will be
      filled with the key material later on */
   CK_ATTRIBUTE aesKeyTemplate[] = {
     {CKA_CLASS,    (void*)&aesKeyClass, sizeof(aesKeyClass)},
     {CKA_ID,       (void*)pObjId2,      sizeof(pObjId2)},
     {CKA_KEY_TYPE, (void*)&aesKeyType,  sizeof(aesKeyType)},
     {CKA_DECRYPT,  (void*)&bAesTrue,    sizeof(bAesTrue)},
     {CKA_VALUE,    (void*)NULL,         0}
   };

   /* =======================================================================
    Variables used to for the AES decryption
   =======================================================================*/
   /* AES IV */
   static const CK_BYTE pAESInitVector[] =
   {
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
   };
   /* Define our AES mechanism as CBC with an IV */
   static const CK_MECHANISM AES_CBC_mechanism =
   {
     CKM_AES_CBC, (void*)pAESInitVector, sizeof(pAESInitVector)
   };

   *ppSessionContext = NULL;

   /* if the client has the effective Decryption rigths, continue */
   if (!getClientEffectiveDecryptionRights())
   {
      SLogError("The client must have decryption rights and be authenticated.");
      return S_ERROR_ACCESS_DENIED;
   }


   /* allocate session context */
   pSessionContext = SMemAlloc(sizeof(SESSION_CONTEXT));
   if (pSessionContext == NULL)
   {
      return S_ERROR_OUT_OF_MEMORY;
   }
   pSessionContext->nSessionType = DRM_AGENT_DECRYPT_SESSION;

   /* Open a READ-ONLY session on the service key store */
   if ((ckr = C_OpenSession(S_CRYPTOKI_KEYSTORE_PRIVATE, CKF_SERIAL_SESSION,
                            NULL, NULL, &pSessionContext->hCryptoSession)) != CKR_OK)
   {
     SLogError("PKCS#11 open session failed [0x%x].\n", ckr);
     SMemFree(pSessionContext);
     return DRM_AGENT_ERROR_CRYPTO;
   }
   /* Login */
   if ((ckr = C_Login(pSessionContext->hCryptoSession, CKU_USER, NULL, 0)) != CKR_OK)
   {
     SLogError("PKCS#11 login failed [0x%x].\n", ckr);
     errorCode = DRM_AGENT_ERROR_CRYPTO;
     goto disconnect;
   }
   /* Initialize a search to find the device key */
   if ((ckr = C_FindObjectsInit(
      pSessionContext->hCryptoSession,
      findTemplate,
      sizeof(findTemplate)/sizeof(CK_ATTRIBUTE)))!= CKR_OK)
   {
     SLogError("PKCS#11 cannot initialize key search [0x%x].\n", ckr);
     errorCode = DRM_AGENT_ERROR_CRYPTO;
     goto disconnect;
   }
   /* Perform the search to match one object (device key), or fail */
   if ((ckr = C_FindObjects(pSessionContext->hCryptoSession,
                          &hDeviceKey,
                          1, &objectCount)) != CKR_OK)
   {
     SLogError ("PKCS#11 cannot find the device key [0x%x].\n", ckr);
     errorCode = DRM_AGENT_ERROR_CRYPTO;
     goto disconnect;
   }
   /* If we have found the device key, we can stop the search. */
   if ((ckr = C_FindObjectsFinal(pSessionContext->hCryptoSession)) != CKR_OK)
   {
     SLogError("PKCS#11 cannot terminate search [0x%x].\n", ckr);
     errorCode = DRM_AGENT_ERROR_CRYPTO;
     goto disconnect;
   }

   if (objectCount == 0)
   {
      /* device key not found */
      errorCode = DRM_AGENT_ERROR_CRYPTO;
      SLogError("Device not found!");
      goto disconnect;
   }

   /* Decrypt the license data using the device key. */
   if ((ckr = C_DecryptInit(pSessionContext->hCryptoSession,
                          &RSA_mechanism,
                          hDeviceKey)) != CKR_OK)
   {
     SLogError("PKCS#11 cannot initialize decryption [0x%x].\n", ckr);
     errorCode = DRM_AGENT_ERROR_CRYPTO;
     goto disconnect;
   }
   pClearData = (CK_BYTE_PTR)SMemAlloc(nClearDataLen);
   if (pClearData == NULL)
   {
      SLogError("PKCS#11 allocate result buffer\n");
      errorCode = S_ERROR_OUT_OF_MEMORY;
      goto disconnect;
   }
   /* The second stage of this operation is to actually do the decryption */
   if ((ckr = C_Decrypt(pSessionContext->hCryptoSession,
                      pCipherKey,
                      nCipherKeyLen,
                      pClearData,
                      &nClearDataLen)) != CKR_OK)
   {
      SLogError("PKCS#11 license key decryption failed [0x%x].\n", ckr);
      errorCode = DRM_AGENT_ERROR_CRYPTO;
      goto disconnect;
   }
   /* The data in the license file is padded to nCipherKeyLen. The actual
      key value is in the nSessionKeyLen last bytes. */
   if (nClearDataLen < nSessionKeyLen)
   {
      SLogError("PKCS#11 decrypted license is invalid.\n");
      errorCode = DRM_AGENT_ERROR_CRYPTO;
      goto disconnect;
   }

   /* Import the session key into the key store */
   aesKeyTemplate[4].pValue = pClearData+nClearDataLen-nSessionKeyLen;
   aesKeyTemplate[4].ulValueLen = nSessionKeyLen;
   if ((ckr = C_CreateObject(pSessionContext->hCryptoSession,
                             aesKeyTemplate,
                             sizeof(aesKeyTemplate)/sizeof(CK_ATTRIBUTE),
                             &pSessionContext->hSessionKey)) != CKR_OK)
   {
     SLogError( "PKCS#11 license key import failed [0x%x].\n", ckr);
     errorCode = DRM_AGENT_ERROR_CRYPTO;
     goto disconnect;
   }
   SMemFree(pClearData);
   pClearData = NULL;
   /* Initialize the Decryption Operation */
   if ((ckr = C_DecryptInit(pSessionContext->hCryptoSession,
                            &AES_CBC_mechanism,
                            pSessionContext->hSessionKey)) != CKR_OK)
   {
      SLogError("PKCS#11 cannot initialize decryption [0x%x].\n", ckr);
      errorCode = DRM_AGENT_ERROR_CRYPTO;
      /* Destroy the session key on error */
      C_DestroyObject(pSessionContext->hCryptoSession, pSessionContext->hSessionKey);
      goto disconnect;
   }

   /* We use a "Cryptoki Update Shortcut" to decrypt the stream. The decryption itself
      takes place outside of the service. The service merely installs a shortcut
      for subsequent decrypt updates. Refer to the Developer Reference Manual for
      more information about the Cryptoki Update Shortcut feature. */
   ckr = CV_ActivateUpdateShortcut2(
      pSessionContext->hCryptoSession,
      DRM_AGENT_CMD_DECRYPT, /* Shortcut is activated on the DRM_AGENT_CMD_DECRYPT command */
      S_UPDATE_SHORTCUT_FLAG_AGGRESSIVE,
      0);
   if (ckr != CKR_OK)
   {
      SLogError("PKCS#11 cannot activate update shortcut [0x%x].\n", ckr);
      errorCode = DRM_AGENT_ERROR_CRYPTO;
      /* Destroy the session key on error */
      C_DestroyObject(pSessionContext->hCryptoSession, pSessionContext->hSessionKey);
      goto disconnect;
   }


   /* Save the session context */
   *ppSessionContext = pSessionContext;
   return S_SUCCESS;

disconnect:
   (void)C_Logout(pSessionContext->hCryptoSession);
   (void)C_CloseSession(pSessionContext->hCryptoSession);
   SMemFree(pSessionContext);
   SMemFree(pClearData);
   return errorCode;
}

void static_closeManagementSession(SESSION_CONTEXT* pSessionContext)
{
   /* close PKCS11 session */
   (void)C_Logout(pSessionContext->hCryptoSession);
   (void)C_CloseSession(pSessionContext->hCryptoSession);
   /* free session context */
   SMemFree(pSessionContext);
}

void static_closeDecryptSession(SESSION_CONTEXT* pSessionContext)
{
   CK_RV ckr;
   uint8_t dummy;
   uint32_t nDummyLen = 0;

   /* finalize decryption */
   (void)C_DecryptFinal(pSessionContext->hCryptoSession, &dummy, &nDummyLen);
   /* destroy session key */
   ckr = C_DestroyObject(pSessionContext->hCryptoSession, pSessionContext->hSessionKey);
   if (ckr != CKR_OK)
   {
      SLogError("PKCS#11 cannot destroy session key [0x%x].\n", ckr);
      /* This should never happen */
      SAssert(false);
   }
   /* close PKCS11 session */
   (void)C_Logout(pSessionContext->hCryptoSession);
   (void)C_CloseSession(pSessionContext->hCryptoSession);
   /* free session conytext */
   SMemFree(pSessionContext);
}


/*------------------------------------------------------------------------
 * Service Private Key Initialization
 *-----------------------------------------------------------------------*/

S_RESULT static_init( SESSION_CONTEXT* pSessionContext )
{
   CK_RV       ckr;
   /* Variable for holding device key class (private key) */
   static const CK_OBJECT_CLASS   devKeyClass  = CKO_PRIVATE_KEY;
   /* Variable for holding device key type (RSA key) */
   static const CK_KEY_TYPE       devKeyType  = CKK_RSA;
   /* Object ID of the device key */
   static const CK_BYTE           pObjId[] = {0x00, 0x01};
   /* Dummy variable so we can pass "TRUE" by reference. */
   static const CK_BBOOL          bTrue = TRUE;
   /* Handle for the device key */
   CK_OBJECT_HANDLE  hDevKey;
   /* Template structure for the device private key */
   static const CK_ATTRIBUTE devPrivateKeyTemplate[] =
   {
      {CKA_CLASS,            (void*)&devKeyClass,    sizeof(devKeyClass)},
      {CKA_ID,               (void*)pObjId,          sizeof(pObjId)     },
      {CKA_TOKEN,            (void*)&bTrue,          sizeof(bTrue)      },
      {CKA_KEY_TYPE,         (void*)&devKeyType,     sizeof(devKeyType) },
      {CKA_DECRYPT,          (void*)&bTrue,          sizeof(bTrue)      },
      {CKA_MODULUS,          (void*)sDevKeyModulus,  sizeof(sDevKeyModulus)  },
      {CKA_PUBLIC_EXPONENT,  (void*)sDevKeyPublic_e, sizeof(sDevKeyPublic_e) },
      {CKA_PRIVATE_EXPONENT, (void*)sDevKeyPriv_e,   sizeof(sDevKeyPriv_e)   },
      {CKA_PRIME_1,          (void*)sDevKeyPrime_P,  sizeof(sDevKeyPrime_P)  },
      {CKA_PRIME_2,          (void*)sDevKeyPrime_Q,  sizeof(sDevKeyPrime_Q)  },
      {CKA_EXPONENT_1,       (void*)sDevKey_eP,      sizeof(sDevKey_eP)      },
      {CKA_EXPONENT_2,       (void*)sDevKey_eQ,      sizeof(sDevKey_eQ)      },
      {CKA_COEFFICIENT,      (void*)sDevKey_qInv,    sizeof(sDevKey_qInv)    }
   };

   /* Import the device key into the service key store for this application. */
   if ((ckr = C_CreateObject(pSessionContext->hCryptoSession,
                             devPrivateKeyTemplate,
                             sizeof(devPrivateKeyTemplate)/sizeof(CK_ATTRIBUTE),
                             &hDevKey)) != CKR_OK)
   {
      if (ckr == CKR_ATTRIBUTE_VALUE_INVALID)
      {
         SLogWarning("Warning: Device key already initialized.");
         return DRM_AGENT_ERROR_ALREADY_INITIALIZED;
      }
      else
      {
         SLogError("PKCS11 error: 0x%x.", ckr);
         return DRM_AGENT_ERROR_CRYPTO;
      }
   }
   return S_SUCCESS;
}


/*------------------------------------------------------------------------
 * Service Key Store Cleanup
 *-----------------------------------------------------------------------*/

S_RESULT static_clean( SESSION_CONTEXT* pSessionContext )
{
    /* Error code for PKCS#11 operations */
    CK_RV             ckr;
    /* Handle for a key */
    CK_OBJECT_HANDLE  hKey;
    /* The number of key found at each step */
    CK_ULONG ulKeyCount;

    while (1)
    {
       /* Initialize object search. We look for all objects. */
       ckr = C_FindObjectsInit(pSessionContext->hCryptoSession, NULL, 0);
       if (ckr != CKR_OK)
       {
          SLogError("PKCS#11 find object init failed [0x%x]", ckr);
          return ckr;
       }

       /* Find the next object in the keystore. */
       ckr = C_FindObjects(pSessionContext->hCryptoSession,
                           &hKey,
                           1,
                           &ulKeyCount);
       if (ckr != CKR_OK)
       {
          SLogError("PKCS#11 find object failed [0x%x]", ckr);
          return ckr;
       }

       /* Finalize the object search. */
       ckr = C_FindObjectsFinal(pSessionContext->hCryptoSession);
       if (ckr != CKR_OK)
       {
           SLogError("PKCS#11 find object final stage failed [0x%x]", ckr);
           return ckr;
       }
       if (ulKeyCount == 0)
       {
         break;
       }

       /* Destroy the object found. */
       ckr = C_DestroyObject(pSessionContext->hCryptoSession, hKey);
       if (ckr != CKR_OK)
       {
          SLogError("PKCS#11 destroy object failed [0x%x]", ckr);
          return ckr;
       }
    }

    return ckr;
}

/*--------------------------------------------------------
 * Service Entry Points
 *-------------------------------------------------------*/

S_RESULT SRVX_EXPORT SRVXCreate(void)
{
   CK_RV ckr;

   SLogTrace("SRVXCreate");

   /* Initialize the PKCS#11 interface */
   ckr = C_Initialize(NULL);
   if (ckr != CKR_OK)
   {
     SLogError("PKCS#11 library initialization failed [0x%x].\n", ckr);
     return DRM_AGENT_ERROR_CRYPTO;
   }
   return S_SUCCESS;
}

void SRVX_EXPORT SRVXDestroy(void)
{
   SLogTrace("SRVXDestroy");

   (void)C_Finalize(NULL);
   return;
}

S_RESULT SRVX_EXPORT SRVXOpenClientSession(
   uint32_t nParamTypes,
   IN OUT S_PARAM pParams[4],
   OUT void** ppSessionContext )
{
   S_RESULT errorCode;
   uint32_t nSessionType;

   SLogTrace("SRVXOpenClientSession");

   /* Print client properties */
   errorCode = printClientPropertiesAndUUID();
   if (errorCode != S_SUCCESS)
   {
      SLogError("printClientPropertiesAndUUID() failed (0x%08x).", errorCode);
      return errorCode;
   }

   /* Check the parameter type. Start with the param#0. It must be a VALUE_INPUT
      parameter that contains a session tag, used to distinguish between a
      management session and a decryption session */
   if (S_PARAM_TYPE_GET(nParamTypes, 0) != S_PARAM_TYPE_VALUE_INPUT)
   {
      SLogError("0x%08X: wrong type for parameter #0 (VALUE_INPUT expected)", nParamTypes);
      return S_ERROR_BAD_PARAMETERS;
   }

   /* get the session type */
   nSessionType = pParams[0].value.a;

   switch(nSessionType)
   {
   case DRM_AGENT_MANAGEMENT_SESSION:
      /* Check the parameter types: there must be no additional parameters */
      if (nParamTypes != S_PARAM_TYPES(S_PARAM_TYPE_VALUE_INPUT, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE))
      {
         SLogError("0x%08X: wrong parameter types for a management open-session", nParamTypes);
         return S_ERROR_BAD_PARAMETERS;
      }
      return static_openManagementSession((SESSION_CONTEXT**)ppSessionContext);
   case DRM_AGENT_DECRYPT_SESSION:
      /* For a decryption session, param #1 must be an input memref */
      if (nParamTypes != S_PARAM_TYPES(S_PARAM_TYPE_VALUE_INPUT, S_PARAM_TYPE_MEMREF_INPUT, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE))
      {
         SLogError("0x%08X: wrong parameter types for a decryption open-session", nParamTypes);
         return S_ERROR_BAD_PARAMETERS;
      }
      /* The encrypted key must not be NULL */
      if (pParams[1].memref.pBuffer == NULL)
      {
         SLogError("NULL param#1");
         return S_ERROR_BAD_PARAMETERS;
      }
      return static_openDecryptSession((SESSION_CONTEXT**)ppSessionContext, pParams[1].memref.pBuffer, pParams[1].memref.nSize);
   default:
      SLogError("invalid session type: 0x%X", nSessionType);
      return S_ERROR_BAD_PARAMETERS;
   }
}

void SRVX_EXPORT SRVXCloseClientSession(IN OUT void* pSessionContext)
{
   SLogTrace("SRVXCloseClientSession");

   switch(((SESSION_CONTEXT*)pSessionContext)->nSessionType)
   {
   case DRM_AGENT_MANAGEMENT_SESSION:
      static_closeManagementSession((SESSION_CONTEXT*)pSessionContext);
      break;
   default:
   case DRM_AGENT_DECRYPT_SESSION:
      static_closeDecryptSession((SESSION_CONTEXT*)pSessionContext);
      break;
   }
}

S_RESULT SRVX_EXPORT SRVXInvokeCommand(IN OUT void* pSessionContext,
                                       uint32_t nCommandID,
                                       uint32_t nParamTypes,
                                       IN OUT S_PARAM pParams[4])
{
   SLogTrace("SRVXInvokeCommand");

   SAssert(((SESSION_CONTEXT*)pSessionContext) != NULL);
   VAR_NOT_USED(pParams);

   switch(nCommandID)
   {
   case DRM_AGENT_CMD_DECRYPT:
      SLogError("The command 0x%X should not arrive to the service (Update Shortcut failed).", nCommandID);
      return S_ERROR_BAD_STATE;
   case DRM_AGENT_CMD_INIT:
      if (nParamTypes != S_PARAM_TYPES(S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE))
      {
         SLogError("DRM_AGENT_CMD_INIT: bad parameter types (expected all none)");
         return S_ERROR_BAD_PARAMETERS;
      }
      if (((SESSION_CONTEXT*)pSessionContext)->nSessionType != DRM_AGENT_MANAGEMENT_SESSION)
      {
         SLogError("invalid session type for management command");
         return S_ERROR_BAD_STATE;
      }
      return static_init((SESSION_CONTEXT*)pSessionContext);
   case DRM_AGENT_CMD_CLEAN:
      if (nParamTypes != S_PARAM_TYPES(S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE, S_PARAM_TYPE_NONE))
      {
         SLogError("DRM_AGENT_CMD_CLEAN: bad parameter types (expected all none)");
         return S_ERROR_BAD_PARAMETERS;
      }
      if (((SESSION_CONTEXT*)pSessionContext)->nSessionType != DRM_AGENT_MANAGEMENT_SESSION)
      {
         SLogError("invalid session type for management command");
         return S_ERROR_BAD_STATE;
      }
      return static_clean((SESSION_CONTEXT*)pSessionContext);
   default:
      SLogError("invalid command ID: 0x%X", nCommandID);
      return S_ERROR_BAD_PARAMETERS;
   }
}
