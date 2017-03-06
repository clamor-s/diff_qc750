/**
 * Copyright (c) 2009 Trusted Logic S.A.
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

/* ----------------------------------------------------------------------------
*   Includes
* ---------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tee_client_api.h"
#include "aes_protocol.h"

/* ----------------------------------------------------------------------------
*   Constants or Global Data
* ---------------------------------------------------------------------------- */

/** Service Identifier **/
static const TEEC_UUID SERVICE_UUID = SERVICE_AES_UUID;


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
int parseCmdLine(
    int    nArgc,
    char*  pArgv[],
    char** ppContentPath)
{
    int   nIndex;
    int   nContentFound = 0;
    char* pArg;

    /* Check we have enough parameters on the command line */
    if (nArgc < 2)
    {
        fprintf(stderr, "Error: not enough parameters.\n");
        return 0;
    }

    /* Reset the input values to default values */
    *ppContentPath = NULL;

    /* For each parameter passed on the command line */
    for (nIndex = 1; nIndex < nArgc; nIndex++)
    {
        pArg = pArgv[nIndex];

         if (nContentFound == 0)
         {
             *ppContentPath = pArg;
             nContentFound = 1;
         }
         /* If we have a third non-option param, then error (too many args) */
         else
         {
             fprintf(stderr, "Error: Too many arguments.\n");
             return 0;
         }
    }

    /* Parse sucessful. */
    return 1;
}


/* ----------------------------------------------------------------------------
*   Basic Utility functions
* ---------------------------------------------------------------------------- */
/**
*  Function aesInitialize:
*  Description:
*           Initialize: create a device context.
*  Output : S_HANDLE*     phDevice     = points to the device handle
*
*
*  Function aesFinalize:
*  Description:
*           Finalize: delete the device context.
*  Input :  S_HANDLE      hDevice     = the device handle
*
*
*  Function aesOpenSession:
*  Description:
*           Open a client session with a specified service.
*  Input :  S_HANDLE      hDevice     = the device handle
*           uint32_t       nLoginType  = the client login type
*  Output:  S_HANDLE*     phSession   = points to the session handle
*
*
*  Function aesCloseSession:
*  Description:
*           Close the client session.
*  Input :  S_HANDLE hSession = session handle
*
**/
static TEEC_Result aesInitialize(OUT S_HANDLE* phDevice)
{
   TEEC_Result    nError;
   TEEC_Context*  pContext;

   *phDevice = S_HANDLE_NULL;

   pContext = (TEEC_Context *)malloc(sizeof(TEEC_Context));
   if (pContext == NULL)
   {
      return TEEC_ERROR_BAD_PARAMETERS;
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
      free(pContext);
   }
   else
   {
      *phDevice = (S_HANDLE)pContext;
   }
   return nError;
}

static TEEC_Result aesFinalize(IN S_HANDLE hDevice)
{
   TEEC_Context* pContext = (TEEC_Context *)hDevice;

   if (pContext == NULL)
   {
      fprintf(stderr, "Error: Device handle invalid.\n");
      return S_HANDLE_NULL;
   }

   TEEC_FinalizeContext(pContext);
   free(pContext);
   return TEEC_SUCCESS;
   }

static TEEC_Result aesOpenSession(
                              IN    S_HANDLE      hDevice,
                              OUT   S_HANDLE*     phSession)
{
   TEEC_Operation sOperation;
   TEEC_Result    nError;
   TEEC_Session*  session;
   TEEC_Context*  pContext = (TEEC_Context *)hDevice;

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
      fprintf(stderr, "Error: Could not prepare open session (%08x).\n",
         nError);
      free(session);
      return nError;
   }
   else
   {
      *phSession = (S_HANDLE)session;
   }
   return TEEC_SUCCESS;
}

static TEEC_Result aesCloseSession(IN S_HANDLE hSession)
{
   TEEC_Session*  session = (TEEC_Session *)hSession;

   if(session == NULL)
   {
      fprintf(stderr, "Error: Invalid session handle.\n");
      return S_HANDLE_NULL;
   }
   TEEC_CloseSession(session);
   free(session);
   return TEEC_SUCCESS;
   }

/* ----------------------------------------------------------------------------
*   Main function
* ---------------------------------------------------------------------------- */
/**
*  Function aesRun:
*  Description:
*           This function just send a command CMD_AES to the service.
*  Input:   S_HANDLE*     hSession   = the session handle
**/
static void aesRun(IN S_HANDLE hSession,
                   void* pContentData,
                   void* pDecryptedData,
                   uint32_t nSize)
{
   TEEC_Result    nError;
   TEEC_Operation sOperation;
   TEEC_Session*  session = (TEEC_Session *)hSession;

   printf("Start aesRun().\n");

   memset(&sOperation, 0, sizeof(TEEC_Operation));
   sOperation.paramTypes = TEEC_PARAM_TYPES(
      TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE);
   sOperation.params[0].tmpref.buffer = pContentData;
   sOperation.params[0].tmpref.size   = nSize;
   sOperation.params[1].tmpref.buffer = pDecryptedData;
   sOperation.params[1].tmpref.size   = nSize;
   nError = TEEC_InvokeCommand(session,
      CMD_AES,
                               &sOperation,       /* IN OUT operation */
                               NULL               /* OUT returnOrigin, optional */
                              );
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: Could not prepare invoke command (%08x).\n", nError);
      return;
   }
   return;
}

size_t getFileContent(
    char* pPath,
    void** ppContent)
{
    FILE*   pStream;
    long    filesize;

   /*
    * The stat function is not used (not available in WinCE).
    */

   /* Open the file */
   pStream = fopen(pPath, "rb");
   if (pStream == NULL)
   {
      fprintf(stderr, "Error: Cannot open file: %s.\n", pPath);
      return 0;
   }

   if (fseek(pStream, 0L, SEEK_END) != 0)
   {
      fprintf(stderr, "Error: Cannot read file: %s.\n", pPath);
      goto error;
   }

   filesize = ftell(pStream);
   if (filesize < 0)
   {
      fprintf(stderr, "Error: Cannot get the file size: %s.\n", pPath);
      goto error;
   }

   if (filesize == 0)
   {
      fprintf(stderr, "Error: Empty file: %s.\n", pPath);
      goto error;
   }

   /* Set the file pointer at the beginning of the file */
   if (fseek(pStream, 0L, SEEK_SET) != 0)
   {
      fprintf(stderr, "Error: Cannot read file: %s.\n", pPath);
      goto error;
   }

   /* Allocate a buffer for the content */
   *ppContent = (unsigned char*)malloc(filesize);
   if (*ppContent == NULL)
   {
      fprintf(stderr, "Error: Cannot read file: Out of memory.\n");
      goto error;
   }

   /* Read data from the file into the buffer */
   if (fread(*ppContent, (size_t)filesize, 1, pStream) != 1)
   {
      fprintf(stderr, "Error: Cannot read file: %s.\n", pPath);
      goto error;
   }

   /* Close the file */
   fclose(pStream);

   /* Return number of bytes read */
   return (size_t)filesize;

error:
   fclose(pStream);
   return 0;

}

/* ----------------------------------------------------------------------------
*   Main
* ---------------------------------------------------------------------------- */
int main(int argc, char* argv[])
{
   TEEC_Result          nError;
   S_HANDLE         hSession;

   /* Content file */
   char*             pContentPath = NULL;
   void*             pContentData;
   uint32_t          nContentSize;

   void*             pDecryptedData;

   S_HANDLE         hDevice = S_HANDLE_NULL;

   printf("\nTrusted Logic AES Example Application\n");

   /* Parse the command line */
   if (parseCmdLine(argc, argv,&pContentPath) == false)
   {
      printf("Usage: aes_client <content path>\n");
      return 1;
   }

   nContentSize = getFileContent(pContentPath,&pContentData);

   pDecryptedData=malloc(nContentSize);
   if (pDecryptedData==NULL)
   {
      free(pContentData);
      fprintf(stderr, "Error: allocation failed.\n");
      return 1;
   }

   /* Initialize */
   nError = aesInitialize(&hDevice);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: aesInitialize() failed (%08x).\n", nError);
      return 1;
   }

   /* Open a session */
   nError = aesOpenSession(hDevice, &hSession);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: aesOpenSession() failed (%08x).\n", nError);
      return 1;
   }

   /* Send a command */
   aesRun(hSession,pContentData,pDecryptedData,nContentSize);

   fprintf(stdout,"%s\n",(char*)pDecryptedData);

   /* Close the session */
   nError = aesCloseSession(hSession);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: aesCloseSession() failed (%08x).\n", nError);
      return 1;
   }

   /* Finalize */
   nError = aesFinalize(hDevice);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: aesFinalize() failed (%08x).\n", nError);
      return 1;
   }

   printf("\nAES Example Application was successfully performed.\n");
   return 0;
}
