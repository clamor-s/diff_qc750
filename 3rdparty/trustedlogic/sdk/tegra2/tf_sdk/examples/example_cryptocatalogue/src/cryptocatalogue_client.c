/**
 * Copyright (c) 2008-2009 Trusted Logic S.A.
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

/* ----------------------------------------------------------------------------
*   Includes
* ---------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tee_client_api.h"
#include "cryptocatalogue_protocol.h"

#ifdef LIBRARY_WITH_JNI
#include <jni.h>
#endif

#if defined(__SYMBIAN32__)
extern void symbianPrintf(const char* format, ...);
extern void symbianFprintf(FILE *stream, const char* format, ...);
extern void symbianPutc(int c, FILE *stream);
#define printf symbianPrintf
#define fprintf symbianFprintf
#define putc symbianPutc
#endif

/* ----------------------------------------------------------------------------
*   Constants or Global Data
* ---------------------------------------------------------------------------- */

/** Service Identifier **/
static const TEEC_UUID SERVICE_UUID = SERVICE_CRYPTOCATALOGUE_UUID;


/* ----------------------------------------------------------------------------
*   Command line Utility functions
* ---------------------------------------------------------------------------- */
/**
*  Function parseCmdLine:
*  Description:
*           Parse the client command line.
*  Input :  int         nArgc             = input arguments counts
*           char*       pArgv[]           = points on input arguments
*  Return value: bool  = true if parseCmdLine succeeds
*
**/
static bool parseCmdLine(IN    int         nArgc,
                         IN    char*       pArgv[])
{
   (void)pArgv;

   /* Check we have enough parameters on the command line */
   if (nArgc < 1)
   {
      fprintf(stderr, "Error: not enough parameters.\n");
      return false;
   }

   /* Check we do not have too many parameters on the command line */
   if (nArgc > 1)
   {
      fprintf(stderr, "Error: too many parameters.\n");
      return false;
   }

   /* Parse sucessful. */
   return true;
}

/* ----------------------------------------------------------------------------
*   Basic Utility functions
* ---------------------------------------------------------------------------- */
/**
*  Function cryptoCatalogueInitialize:
*  Description:
*           Initialize: create a device context.
*  Output : S_HANDLE*     phDevice     = points to the device handle
*
*
*  Function cryptoCatalogueFinalize:
*  Description:
*           Finalize: delete the device context.
*  Input :  S_HANDLE      hDevice     = the device handle
*
*
*  Function cryptoCatalogueOpenSession:
*  Description:
*           Open a client session with a specified service.
*  Input :  S_HANDLE      hDevice     = the device handle
*           uint32_t       nLoginType  = the client login type
*  Output:  S_HANDLE*     phSession   = points to the session handle
*
*
*  Function cryptoCatalogueCloseSession:
*  Description:
*           Close the client session.
*  Input :  S_HANDLE hSession = session handle
*
**/
static TEEC_Result cryptoCatalogueInitialize(OUT S_HANDLE* phDevice)
{
   TEEC_Result    nError;
   TEEC_Context * pContext;

   *phDevice = S_HANDLE_NULL;

   pContext = (TEEC_Context*)malloc(sizeof(TEEC_Context));
   if (pContext == NULL)
   {
      return TEEC_ERROR_OUT_OF_MEMORY;
   }
   memset(pContext, 0, sizeof(TEEC_Context));
   /* Create Device context  */
   nError = TEEC_InitializeContext(NULL, pContext);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: TEEC_InitializeContext failed (%08x).\n",
         nError);
      if(nError == TEEC_ERROR_COMMUNICATION)
      {
         fprintf(stderr, "Error: The client could not communicate with the service.\n");
      }
   }
   else
   {
      *phDevice = (S_HANDLE)pContext;
   }
   return nError;
}

static TEEC_Result cryptoCatalogueFinalize(IN S_HANDLE hDevice)
{
   TEEC_Context * pContext = (TEEC_Context *)hDevice;

   if (pContext == NULL)
   {
      fprintf(stderr, "Error: Device handle invalid.\n");
      return S_HANDLE_NULL;
   }

   TEEC_FinalizeContext(pContext);
   return TEEC_SUCCESS;
   }

static TEEC_Result cryptoCatalogueOpenSession(
                              IN    S_HANDLE      hDevice,
                              OUT   S_HANDLE*     phSession)
{
   TEEC_Operation sOperation;
   TEEC_Result    nError;
   TEEC_Session*  session;
   TEEC_Context * pContext = (TEEC_Context *)hDevice;

   session = (TEEC_Session*)malloc(sizeof(TEEC_Session));
   if (session == NULL)
   {
      return TEEC_ERROR_OUT_OF_MEMORY;
   }
   memset(session, 0, sizeof(TEEC_Session));
   memset(&sOperation, 0, sizeof(TEEC_Operation));
   sOperation.paramTypes = 0;
   nError = TEEC_OpenSession(pContext,
                             session,                    /* OUT session */
                             &SERVICE_UUID,              /* destination UUID */
                             TEEC_LOGIN_PUBLIC,          /* connectionMethod */
                             NULL,                       /* connectionData */
                             &sOperation,                /* IN OUT operation */
                             NULL                        /* OUT returnOrigin, optional */
                             );
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: TEEC_OpenSession() failed (%08x).\n",
         nError);
      return nError;
   }
   *phSession = (S_HANDLE)session;
      return nError;
   }

static TEEC_Result cryptoCatalogueCloseSession(IN S_HANDLE hSession)
{
   TEEC_Session*  session = (TEEC_Session *)hSession;

   if(session == NULL)
   {
      fprintf(stderr, "Error: Invalid session handle.\n");
      return S_HANDLE_NULL;
   }
   TEEC_CloseSession(session);
   return TEEC_SUCCESS;
   }

/* ----------------------------------------------------------------------------
*   Main function
* ---------------------------------------------------------------------------- */
/**
*  Function cryptoCatalogueClient:
*  Description:
*           This function just send a command CMD_CRYPTOCATALOGUE to the service.
*  Input:   S_HANDLE*     hSession   = the session handle
**/
static TEEC_Result cryptoCatalogueClient(IN S_HANDLE hSession)
{
   TEEC_Result    nError;
   TEEC_Operation sOperation;
   TEEC_Session*  session = (TEEC_Session *)hSession;

   printf("Start cryptoCatalogueClient().\n");

   session = (TEEC_Session*)hSession;
   memset(&sOperation, 0, sizeof(TEEC_Operation));
   nError = TEEC_InvokeCommand(session,
      CMD_CRYPTOCATALOGUE,
                               &sOperation,       /* IN OUT operation */
                               NULL               /* OUT returnOrigin, optional */
                              );
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: TEEC_InvokeCommand() failed (%08x).\n", nError);
      return nError;
   }
   return TEEC_SUCCESS;
   }

/* ----------------------------------------------------------------------------
*   Main
* ---------------------------------------------------------------------------- */
#if defined(__SYMBIAN32__)
int _main(int argc, char* argv[])
#elif defined (LIBRARY_WITH_JNI)
int runCryptoCatalogue(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
   TEEC_Result          nError;
   S_HANDLE         hSession;

   S_HANDLE         hDevice = S_HANDLE_NULL;

   printf("\nTrusted Logic cryptoCatalogue Example Application\n");

   /* Parse the command line */
   if (parseCmdLine(argc, argv) == false)
   {
      printf("Usage: cryptocatalogue_client\n");
      return 1;
   }

   /* Initialize */
   nError = cryptoCatalogueInitialize(&hDevice);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: cryptoCatalogueInitialize() failed (%08x).\n", nError);
      return 1;
   }

   /* Open a session */
   nError = cryptoCatalogueOpenSession(hDevice, &hSession);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: cryptoCatalogueOpenSession() failed (%08x).\n", nError);
      return 1;
   }

   /* Send a command */
   nError = cryptoCatalogueClient(hSession);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: cryptoCatalogueClient() failed (%08x).\n", nError);
      return 1;
   }

   /* Close the session */
   nError = cryptoCatalogueCloseSession(hSession);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: cryptoCatalogueCloseSession() failed (%08x).\n", nError);
      return 1;
   }

   /* Finalize */
   nError = cryptoCatalogueFinalize(hDevice);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: cryptoCatalogueFinalize() failed (%08x).\n", nError);
      return 1;
   }

   printf("\nCryptocatalogue_example was successfully performed.\n");
   return 0;
}

#ifdef LIBRARY_WITH_JNI

JNIEXPORT __attribute__((visibility("default"))) void JNICALL
Java_com_trustedlogic_android_examples_CryptoCatalogue_Run
  (JNIEnv *env, jobject this, jstring device, jstring external_storage)
{
	jboolean iscopy;

	const char * device_name=(*env)->GetStringUTFChars(env, device, &iscopy);
	const char * storage=(*env)->GetStringUTFChars(env, external_storage, &iscopy);

	int file1 = freopen (storage,"w",stdout); // redirect stdout (currently set to /dev/null) to the logging file
	int file2 = freopen (storage,"w",stderr); // redirect stderr (currently set to /dev/null) to the logging file

	// call main...
	int result = runCryptoCatalogue(1, NULL);
	fclose (stdout);
	fclose (stderr);

	(*env)->ReleaseStringChars(env, device, device_name);
	(*env)->ReleaseStringChars(env, external_storage, storage);
}
#endif
