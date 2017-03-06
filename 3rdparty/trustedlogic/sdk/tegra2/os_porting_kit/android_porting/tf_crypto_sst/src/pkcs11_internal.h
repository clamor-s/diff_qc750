/**
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

#ifndef __PKCS11_INTERNAL_H__
#define __PKCS11_INTERNAL_H__

#define CRYPTOKI_EXPORTS
#include "cryptoki.h"
#include "service_system_protocol.h"

#include "lib_object.h"
#include "lib_mutex.h"
#include "tee_client_api.h"

#include <stdlib.h>
#include <string.h>


/**
 * The magic word.
 */
#define PKCS11_SESSION_MAGIC  ( (uint32_t)0x45EF683B )

/**
 * Computes required size to fit in a 4-bytes aligned buffer (at the end)
 * If the size is a multiple of 4, just returns the size
 * Otherwise, return the size so that the (end of the buffer)+1 is 4-bytes aligned.
 */
#define PKCS11_GET_SIZE_WITH_ALIGNMENT(a)  (uint32_t)(((uint32_t)a+3) & ~3)


/**
 * The System Service UUID used by the library
 */
extern const TEEC_UUID SERVICE_UUID;

/**
 * g_sContext: the global TEEC context used by the library
 */
extern TEEC_Context  g_sContext;

void stubMutexLock(void);
void stubMutexUnlock(void);
TEEC_Result stubInitializeContext(void);
void stubFinalizeContext(void);

/** Whether the cryptoki library is initialized or not */
extern bool g_bCryptokiInitialized;

CK_RV ckInternalTeeErrorToCKError(TEEC_Result nError);

#define PKCS11_PRIMARY_SESSION_TAG    1
#define PKCS11_SECONDARY_SESSION_TAG  2

typedef struct
{
   /*
    * Magic word, must be set to {PKCS11_SESSION_MAGIC}.
    */
   uint32_t    nMagicWord;

   /* nSessionTag must be set to {PKCS11_PRIMARY_SESSION_TAG} for primary session 
   *                          to {PKCS11_SECONDARY_SESSION_TAG} for secondary session */
   uint32_t    nSessionTag;

}PKCS11_SESSION_CONTEXT_HEADER, * PPKCS11_SESSION_CONTEXT_HEADER;

/** 
 * The PKCS11 Primary session context
 */
typedef struct
{
   /* sHeader must be the first field of this structure */
   PKCS11_SESSION_CONTEXT_HEADER sHeader;

   /* TEEC session used for this cryptoki primary session.
      Each primary session has its own TEEC session */
   TEEC_Session sSession;
   uint32_t     hCryptoSession;

   /* Mutex to protect the table of secondary sessions */
   LIB_MUTEX sSecondarySessionTableMutex;
      
   /* Table of secondary sessions */
   LIB_OBJECT_TABLE_HANDLE16 sSecondarySessionTable;

} PKCS11_PRIMARY_SESSION_CONTEXT, * PPKCS11_PRIMARY_SESSION_CONTEXT;

/** 
 * The PKCS11 Secondary session context
 */
typedef struct 
{
   /* sHeader must be the first field of this structure */
   PKCS11_SESSION_CONTEXT_HEADER sHeader;

   /* Secondary session handle as returned by pkcs11 */
   uint32_t       hSecondaryCryptoSession;

   /* A node of the table of secondary sessions */
   LIB_OBJECT_NODE_HANDLE16 sSecondarySessionNode;

   /* pointer to the primary session */
   PKCS11_PRIMARY_SESSION_CONTEXT* pPrimarySession;

} PKCS11_SECONDARY_SESSION_CONTEXT, *PPKCS11_SECONDARY_SESSION_CONTEXT;

bool ckInternalSessionIsOpenedEx(S_HANDLE hSession, bool* pBoolIsPrimarySession);

#endif /* __PKCS11_INTERNAL_H__ */
