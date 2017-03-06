/**
 * Copyright (c) 2008-2011 Trusted Logic S.A.
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
#include "certstore_protocol.h"

#if defined(__SYMBIAN32__)
extern void symbianPrintf(const char* format, ...);
extern void symbianFprintf(FILE *stream, const char* format, ...);
extern void symbianPutc(int c, FILE *stream);
#define printf symbianPrintf
#define fprintf symbianFprintf
#define putc symbianPutc
#endif

/**
 *  The structure COMMAND_ARGUMENTS is used to save data after parsing the command line.
 *  nCommandID   : the command identifier
 *  nIdFile1     : the identifier of a file already present in the store.
 *  nIdFile2     : the identifier of a second file (used for signature checking).
 *  pFileContent : the content of a file to be installed.
 *  nFileLength  : the length of a file to be installed.
 *  pOutput      : the output for storing a certificate.
 */
typedef struct COMMAND_ARGUMENTS
{
   uint32_t    nCommandID;

   uint32_t    nIdFile1;
   uint32_t    nIdFile2;

   uint8_t*    pFileContent;
   uint32_t    nFileLength;

   uint8_t*    pOutput;
} COMMAND_ARGUMENTS;


/****************************************************************************
 ****************************************************************************
 **
 ** In this example, the client is very simple and includes the implementation
 ** of the "stub", but more complex services may decide to put the functions
 ** in a separate static or dynamic library.
 **
 ****************************************************************************
 ****************************************************************************/

/*---------------------------------------------------------------------------
                      Connection-related functions
  ---------------------------------------------------------------------------*/

/**
*  Function certStoreInitialize:
*  Description:
*           This creates a TEEC context.
*  Output : S_HANDLE*     phDevice     = points to the context handle
*
* A Device Context represents a connection to the Secure World but not
* to any particular service in the secure world.
**/
static TEEC_Result certStoreInitialize(OUT S_HANDLE* phDevice)
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
   /* Initialize TEEC context  */
   nError = TEEC_InitializeContext(NULL, pContext);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: TEEC_InitializeContext failed [0x%08x].\n", nError);
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


/**
 *  Function certStoreOpenSession:
 *  Description:
 *           Open a client session with the cerstore service.
 *  Input :  S_HANDLE      hDevice     = the device handle
 *           uint32_t       nLoginType  = the client login type (one of the constants TEEC_LOGIN_*)
 *  Output:  S_HANDLE*     phSession   = points to the session handle
 *
 * A "Client Session" is a connection between the client (this application)
 * and the service. When the client opens a session, it can specify
 * which login method it wants to use.
 **/
static TEEC_Result certStoreOpenSession(
                              IN    S_HANDLE    hDevice,
                              IN    uint32_t    nLoginType,
                              OUT   S_HANDLE*   phSession)
{
   TEEC_Operation sOperation;
   TEEC_Result    nError;
   TEEC_Session*  pSession;
   TEEC_Context*  pContext = (TEEC_Context *)hDevice;

   char*    pSignatureFile = NULL;
   uint32_t nSignatureFileLen  = 0;
   uint8_t  nParamType3 = TEEC_NONE;

   /* Identifier for the cerstore service. Note that
       SERVICE_CERSTORE_UUID is defined in the cerstore_protocol.h */
   static const TEEC_UUID SERVICE_UUID = SERVICE_CERTSTORE_UUID;

   pSession = (TEEC_Session*)malloc(sizeof(TEEC_Session));
   if (pSession == NULL)
   {
      return TEEC_ERROR_OUT_OF_MEMORY;
   }

   memset(pSession, 0, sizeof(TEEC_Session));

   if (nLoginType == TEEC_LOGIN_AUTHENTICATION)
   {
      nError = TEEC_ReadSignatureFile((void **)&pSignatureFile, &nSignatureFileLen);
      if (nError != TEEC_ERROR_ITEM_NOT_FOUND)
      {
           if (nError != TEEC_SUCCESS)
           {
              fprintf(stderr, "Error: TEEC_ReadSignatureFile failed [0x%08x]\n", nError);
              return nError;
           }

         sOperation.params[3].tmpref.buffer = pSignatureFile;
         sOperation.params[3].tmpref.size   = nSignatureFileLen;
         nParamType3 = TEEC_MEMREF_TEMP_INPUT;
      }
      else
      {
         /* No signature file found --> Fallback to identification.
          * The doc uses this as an example of how the service can filter out non-authenticated clients.
          */
         printf("Signature file not found - Fallback to login User Application\n");
         nLoginType = TEEC_LOGIN_USER_APPLICATION;
      }
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, nParamType3);
   nError = TEEC_OpenSession(pContext,
                             pSession,                   /* OUT session */
                             &SERVICE_UUID,              /* destination UUID */
                             nLoginType,                 /* connectionMethod */
                             NULL,                       /* connectionData */
                             &sOperation,                /* IN OUT operation */
                             NULL                        /* OUT returnOrigin, optional */
                             );
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: Could not perform open session [0x%08x].\n", nError);
      free(pSession);
      return nError;
   }
   else
   {
      *phSession = (S_HANDLE)pSession;
   }

   return TEEC_SUCCESS;
}

/**
 *  Function certStoreCloseSession:
 *  Description:
 *           Close the client session.
 *  Input :  S_HANDLE hSession = session handle
 **/
static TEEC_Result certStoreCloseSession(IN S_HANDLE hSession)
{
   TEEC_Session*  pSession = (TEEC_Session *)hSession;

   if(pSession == NULL)
   {
      fprintf(stderr, "Error: Invalid session handle.\n");
      return TEEC_ERROR_BAD_PARAMETERS;
   }

   TEEC_CloseSession(pSession);
   free(pSession);
   return TEEC_SUCCESS;
}


/**
 *
 *  Function certStoreFinalize:
 *  Description:
 *           Finalize: delete the device context.
 *  Input :  S_HANDLE      hDevice     = the device handle
 *
 * Note that a Device Context can be deleted only when all
 * sessions have been closed in the device context.
 **/
static TEEC_Result certStoreFinalize(IN S_HANDLE hDevice)
{
   TEEC_Context* pContext = (TEEC_Context *)hDevice;

   if (pContext == NULL)
   {
      fprintf(stderr, "Error: TEEC handle invalid.\n");
      return TEEC_ERROR_BAD_PARAMETERS;
   }

   TEEC_FinalizeContext(pContext);
   free(pContext);
   return TEEC_SUCCESS;
}

/*---------------------------------------------------------------------------
   Each function corresponds to a command exchanged with the service.
  ---------------------------------------------------------------------------*/

/**
 *  Function clientInstallCertificate:
 *  Description:
 *           Installs a new certificate in the cerstore.
 *  Input :
 *     hSession: session handle
 *     pCertificate/nCertificateLength: content of the certificate
 *
 *  Output:
 *     TEEC_SUCCESS on success. An error code otherwise
 *     pnCertificateID: filled with the certificate identifier
 *
 *  The service parses the certificate and stores it in the certstore.
 *  This function also prints the certificate distinguished name on stdout
 *
 *  The client must be logged with manager privilege.
 **/
static TEEC_Result clientInstallCertificate(
                                         IN  S_HANDLE hSession,
                                         IN  uint8_t*  pCertificate,
                                         IN  uint32_t  nCertificateLength,
                                         OUT uint32_t* pnCertificateID)
{
   TEEC_Result    nError;
   TEEC_Operation sOperation;
   TEEC_Session*  pSession = (TEEC_Session *)hSession;
   uint8_t sDistinguishedNameField[1024];
   uint32_t nRemainingSize;
   uint32_t nStrlLen;
   uint8_t* pCurrentStr;

   fprintf(stderr,"Start clientInstallCertificate().\n");

   sOperation.paramTypes = TEEC_PARAM_TYPES(
      TEEC_VALUE_OUTPUT,
      TEEC_MEMREF_TEMP_OUTPUT,
      TEEC_MEMREF_TEMP_INPUT,
      TEEC_NONE);

   memset(sDistinguishedNameField, 0, 1024);
   sOperation.params[1].tmpref.buffer = sDistinguishedNameField;
   sOperation.params[1].tmpref.size   = 1024;
   sOperation.params[2].tmpref.buffer = pCertificate;
   sOperation.params[2].tmpref.size   = nCertificateLength;

   nError = TEEC_InvokeCommand(pSession,
                               CMD_INSTALL_CERT,
                               &sOperation,       /* IN OUT operation */
                               NULL               /* OUT returnOrigin, optional */
                              );
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: Could not invoke command [0x%08x].\n", nError);
      return nError;
   }

   /* Get output data */

   /* 1) Identifier */
   *pnCertificateID = sOperation.params[0].value.a;
   fprintf(stderr,"CERTIFICATE IDENTIFIER: 0x%08X.\n", *pnCertificateID);

   /* 2) Print the Distinguished Name */
   nRemainingSize = sOperation.params[1].tmpref.size;
   pCurrentStr = sDistinguishedNameField;
   while(nRemainingSize != 0)
   {
      nStrlLen = strlen((const char*)pCurrentStr);
      if (nStrlLen == 0)
         break;
      printf("DISTINGUISHED NAME:\n");
      printf("   %s\n", pCurrentStr);
      if (nRemainingSize <= nStrlLen)
         break;
      nRemainingSize -= nStrlLen;
      pCurrentStr = &pCurrentStr[nStrlLen+1];
   }

   return TEEC_SUCCESS;
}

/**
 *  Function clientReadAndPrintCertificateDN:
 *  Description:
 *           Get the Distinguished Name of a certificate in the store
 *           and print it on stdout
 *  Input:
 *       hSession: handle on a session opened on the certstore service
 *       nCertificateID: identifier of the certificate to print
 **/

static void clientReadAndPrintCertificateDN(IN S_HANDLE hSession,
                                     IN uint32_t nCertificateID)
{
   TEEC_Result    nError;
   TEEC_Operation sOperation;
   TEEC_Session*  pSession = (TEEC_Session *)hSession;
   uint8_t sDistinguishedNameField[1024];

   printf("Start clientPrintCertificateDN().\n");

   sOperation.paramTypes = TEEC_PARAM_TYPES(
      TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE);

   sOperation.params[0].value.a = nCertificateID;
   sOperation.params[1].tmpref.buffer = sDistinguishedNameField;
   sOperation.params[1].tmpref.size   = 1024;

   nError = TEEC_InvokeCommand(pSession,
                               CMD_READ_DN,
                               &sOperation,       /* IN OUT operation */
                               NULL               /* OUT returnOrigin, optional */
                              );
   if(nError != TEEC_SUCCESS)
   {
       fprintf(stderr, "Error: Could not invoke command [0x%08x].\n", nError);
       return;
   }

   printf("The Certificate 0x%08X has the following DN:\n",nCertificateID);

   /* Decode Output data (DN) */
   printf("DISTINGUISHED NAME:\n");
   printf("   %s\n", sDistinguishedNameField);
}


/**
 *  Function clientListAllCertificates:
 *  Description:
 *           Lists all the certificates available in the store.
 *  Input :  S_HANDLE   hSession  = session handle
 **/
static void clientListAllCertificates(IN S_HANDLE hSession)
{
   TEEC_Result    nError;
   TEEC_Operation sOperation;
   TEEC_Session*  pSession = (TEEC_Session *)hSession;
   uint32_t       sCertIdentifiers[25];
   uint32_t       i;

   printf("Start clientListAllCertificates().\n");

   sOperation.paramTypes = TEEC_PARAM_TYPES(
      TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

   memset(sCertIdentifiers, 0, sizeof(uint32_t)*25);
   sOperation.params[0].tmpref.buffer = sCertIdentifiers;
   sOperation.params[0].tmpref.size   = sizeof(uint32_t)*25;

   nError = TEEC_InvokeCommand(pSession,
                               CMD_LIST_ALL,
                               &sOperation,       /* IN OUT operation */
                               NULL               /* OUT returnOrigin, optional */
                              );
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: Could not invoke command [0x%08x].\n", nError);
      return;
   }

   printf("LIST OF CERTIFICATE IDENTIFIERS:\n");
   for (i = 0; i < sOperation.params[0].tmpref.size/4; i++)
   {
      printf("   0x%08X\n", sCertIdentifiers[i]);
   }
}


/**
 *  Function clientDeleteCertificate:
 *  Description:
 *           Send the certificate identifier to the service and
 *           a request to delete the certificate.
 *  Input :  S_HANDLE   hSession  = session handle
 *           uint32_t nCertIdentifier = the certificate identifier
 *
 **/
static void clientDeleteCertificate(IN S_HANDLE hSession,
                                    IN uint32_t nCertIdentifier)
{
   TEEC_Result    nError;
   TEEC_Operation sOperation;
   TEEC_Session*  pSession = (TEEC_Session *)hSession;

   fprintf(stderr,"Start clientDeleteCertificate().\n");

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

   sOperation.params[0].value.a = nCertIdentifier;

   nError = TEEC_InvokeCommand(pSession,
                               CMD_DELETE_CERT,
                               &sOperation,       /* IN OUT operation */
                               NULL               /* OUT returnOrigin, optional */
                              );
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: Could not invoke command [0x%08x].\n", nError);
      return;
   }
}

/**
 *  Function clientDeleteAllCertificates:
 *  Description:
 *           Delete all certificates in the store
 *
 *  Input :  S_HANDLE   hSession  = session handle
 *  Output:  TEEC_SUCCESS on success. An error code otherwise.
 *
 *  The client must be connected with manager privilege
 **/
static TEEC_Result clientDeleteAllCertificates(IN S_HANDLE hSession)
{
   TEEC_Result    nError;
   TEEC_Operation sOperation;
   TEEC_Session*  pSession = (TEEC_Session *)hSession;

   printf("Start clientDeleteAllCertificates().\n");

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);

   nError = TEEC_InvokeCommand(pSession,
                               CMD_DELETE_ALL,
                               &sOperation,       /* IN OUT operation */
                               NULL               /* OUT returnOrigin, optional */
                              );
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: Could not invoke command [0x%08x].\n", nError);
      return nError;
   }

   return TEEC_SUCCESS;
}


/**
 *  Function clientGetCertificate:
 *  Description:
 *     Retrieve a certificate given its identifier.
 *  Input:
 *     hSession: the session handle
 *     nCertificateID: the certificate identifier
 *  Output:
 *     *pnCertificateLength: filled with the length of the certificate
 *     *ppCertificate:       filled with the content of the certificate
 *  Return:
 *     TEEC_SUCCESS on success, an error code otherwise.
 *
 *  The certificate content must be freed with SMFree when no longer used
 **/
static void clientGetCertificate( IN  S_HANDLE   hSession,
                                  IN  uint32_t    nCertificateID,
                                  OUT uint8_t**   ppCertificate,
                                  OUT uint32_t*   pnCertificateLength)
{
   TEEC_Result    nError;
   TEEC_Operation sOperation;
   TEEC_Session*  pSession = (TEEC_Session *)hSession;
   uint8_t *      pCertificate;

   fprintf(stderr,"Start clientGetCertificate().\n");

   pCertificate = (uint8_t*)malloc(4*1024); /* Maximum size of the answer buffer: large enough to hold a certificate */
   if (pCertificate == NULL)
   {
      return; // TEEC_ERROR_OUT_OF_MEMORY
   }

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE);

   sOperation.params[0].value.a = nCertificateID;
   sOperation.params[1].tmpref.buffer = pCertificate;
   sOperation.params[1].tmpref.size   = 4*1024;

   nError = TEEC_InvokeCommand(pSession,
                               CMD_GET_CERT,
                               &sOperation,       /* IN OUT operation */
                               NULL               /* OUT returnOrigin, optional */
                              );
   if(nError != TEEC_SUCCESS)
   {
       fprintf(stderr, "Error: Could not invoke command [0x%08x].\n", nError);
       return;
   }

   /* Decode the certificate */
   *ppCertificate       = pCertificate;
   *pnCertificateLength = sOperation.params[1].tmpref.size;
}


/**
 *  Function clientVerifyCertificate:
 *  Description:
 *     Verify one certificate against another.
 *  Input:
 *     hSession: the session handle
 *     nCertificateID: the certificate identifier
 *     nIssuerCertificateID: the issuer certificate identifier
 *  Output:
 *     TEEC_SUCCESS on success, an error code otherwise.
 *
 *  The verification will be done by using the public key of the issuer certificate to check
 *  the signature on the first certificate.
 **/

void clientVerifyCertificate(IN S_HANDLE hSession,
                             IN uint32_t nCertificateID,
                             IN uint32_t nIssuerCertificateID)
{
   TEEC_Result    nError;
   TEEC_Operation sOperation;
   TEEC_Session*  pSession = (TEEC_Session *)hSession;

   fprintf(stderr,"Start clientVerifyCertificate().\n");

   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_OUTPUT, TEEC_NONE, TEEC_NONE);

   sOperation.params[0].value.a = nCertificateID;
   sOperation.params[0].value.b = nIssuerCertificateID;

   nError = TEEC_InvokeCommand(pSession,
                               CMD_VERIFY_CERT,
                               &sOperation,       /* IN OUT operation */
                               NULL               /* OUT returnOrigin, optional */
                              );
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: Could not invoke command [0x%08x].\n", nError);
      return;
   }

   /* Get the answer */
   nError = sOperation.params[1].value.a;
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr,"Verification failed [0x%08x]\n", nError);
   }
   else
   {
      fprintf(stderr,"Verification succeeded !\n");
   }
}

/****************************************************************************
 ****************************************************************************
 **
 **   The remaining of the source contains the command-line driver for the
 **   cerstore API
 **
 ****************************************************************************
 ****************************************************************************/

/*
*  Function createFile:
*  Description:
*           Create a file with the name pPath and the content pContent
*  Input :  char*    pPath          = a path
*           uint8_t* pContent       = a content
*           uint32_t nContentLength = the content length
*  Output:  returns bool  = true if the file is succesfully created
*
**/
static bool createFile( IN char*    pPath,
                        IN uint8_t* pContent,
                        IN uint32_t nContentLength)
{
   FILE*       pStream;

   /* Open the file */
   pStream = fopen( pPath, "wb");
   if (pStream == NULL)
   {
      fprintf(stderr, "Error: Cannot open file: %s.\n", pPath);
      return false;
   }

   /* Write pContent in the file */
   if (fwrite(pContent, nContentLength, 1, pStream) != 1)
   {
      fprintf(stderr, "Error: Cannot write in file: %s.\n", pPath);
      fclose(pStream);
      return false;
   }

   /* Close the file */
   fclose(pStream);
   return true;
}

/**
*  Function getFileContent:
*  Description:
*           Recovers the content of  a file from its path.
*  Input :  char* pPath                = the path to a file
*  Output:  unsigned char** ppContent  = points to the content of the file
*           returns uint32_t           = the file length in bytes
*/
static size_t getFileContent( IN   char* pPath,
                              OUT  unsigned char** ppContent)
{
   FILE*   pStream;
   long    filesize;

   *ppContent = NULL;

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
      goto cleanup;
   }

   filesize = ftell(pStream);
   if (filesize < 0)
   {
      fprintf(stderr, "Error: Cannot get the file size: %s.\n", pPath);
      goto cleanup;
   }

   if (filesize == 0)
   {
      fprintf(stderr, "Error: Empty file: %s.\n", pPath);
      goto cleanup;
   }

   /* Set the file pointer at the beginning of the file */
   if (fseek(pStream, 0L, SEEK_SET) != 0)
   {
      fprintf(stderr, "Error: Cannot read file: %s.\n", pPath);
      goto cleanup;
   }

   /* Allocate a buffer for the content */
   *ppContent = (unsigned char*)malloc(filesize);
   if (*ppContent == NULL)
   {
      fprintf(stderr, "Error: Cannot read file: Out of memory.\n");
      goto cleanup;
   }

   /* Read data from the file into the buffer */
   if (fread(*ppContent, (size_t)filesize, 1, pStream) != 1)
   {
      fprintf(stderr, "Error: Cannot read file: %s.\n", pPath);
      goto cleanup;
   }

   /* Close the file */
   fclose(pStream);

   /* Return number of bytes read */
   return (size_t)filesize;

cleanup:
   fclose(pStream);
   return 0;
}

/**
*  Function printUsage:
*  Description:
*           Display the usage of certstore_client commands and command arguments.
*
**/
static void printUsage(void)
{
   printf("Usage: certstore_client <command>\n");
   printf("Supported commands are: \n");
   printf("  help or -help: Display this help\n");
   printf("  installCert <certificate path>: Install a certificate.\n");
   printf("       This option must be followed by a certificate path.\n");
   printf("       The service returns a certificate identifier, and the DN\n");
   printf("  deleteCert <certificate identifier>: Delete a certificate.\n");
   printf("       This option must be followed by a certificate identifier\n");
   printf("  listAll: List all certificates already installed.\n");
   printf("       (returns all certificates identifiers).\n");
   printf("  readCertDN <certificate identifier>: Read a certificate DN (i.e. read and interprete). \n");
   printf("        This option must be followed by a certificate identifier.\n");
   printf("        The service returns the Cert DN formatted as a list of strings.\n");
   printf("  getCert <certificate identifier> [<output_path>]: Get a certificate. \n");
   printf("         This option must be followed by a certificate identifier.\n");
   printf("         The service returns the whole Certificate.\n");
   printf("         An output can be specified if necessary.Otherwise a default output is used to store the file.\n");
   printf("  verifyCert <certificate identifier> <issuer_certificate identifier>: \n");
   printf("               Verify that the first certificate is signed by the second one.\n");
   printf("  deleteAll: Delete all certificates.\n");
   printf("Remark: \n");
   printf("  <certificate path>: the path to a file containing a certificate at DER format.\n");
   printf("  <certificate identifier>: the identifier of a certificate already installed.\n");
   printf("  <output_path>: the path to a file for storing the certificate.\n");
}

/**
 *  Function parseCmdLine:
 *  Description:
 *           Parse the client command line.
 *  Input :  int         nArgc               = input arguments counts
 *           char*       pArgv[]             = points on input arguments
 *  Output:  COMMAND_ARGUMENTS* pCommandArgs = structure containing the command arguments
 *           returns bool                    = true if parseCmdLine succeed
 *
 **/

static bool parseCmdLine(IN    int         nArgc,
                         IN    char*       pArgv[],
                         OUT   COMMAND_ARGUMENTS* pCommandArgs)
{
   char* pArg;

   /* Check we have enough parameters in the command line */
   if (nArgc < 2)
   {
      fprintf(stderr, "Error: not enough parameters.\n");
      return false;
   }

   /* Check we do not have too many parameters in the command line */
   if (nArgc > 4)
   {
      fprintf(stderr, "Error: too many parameters.\n");
      return false;
   }

   /* Reset the input values to default values */
   pCommandArgs->nCommandID = 0;
   pCommandArgs->pOutput = NULL;

   pCommandArgs->pFileContent = NULL;
   pCommandArgs->nFileLength = 0;

   pCommandArgs->nIdFile1 = 0;
   pCommandArgs->nIdFile2 = 0;


   /* Check Command (i.e. pArgv[1]) */
   pArg = pArgv[1];
   if ((strcmp(pArg, "help") == 0) || (strcmp(pArg, "-help") == 0))
   {
      /* If it is a help command, then return false (main will print usage)*/
      return false;
   }
   else if (strcmp(pArg, "installCert") == 0)
   {
      pCommandArgs->nCommandID = CMD_INSTALL_CERT;
   }
   else if (strcmp(pArg, "deleteCert") == 0)
   {
      pCommandArgs->nCommandID = CMD_DELETE_CERT;
   }
   else if (strcmp(pArg, "deleteAll") == 0)
   {
      pCommandArgs->nCommandID = CMD_DELETE_ALL;
   }
   else if (strcmp(pArg, "listAll") == 0)
   {
     pCommandArgs->nCommandID = CMD_LIST_ALL;
   }
   else if (strcmp(pArg, "getCert") == 0)
   {
     pCommandArgs->nCommandID = CMD_GET_CERT;
   }
   else if (strcmp(pArg, "readCertDN") == 0)
   {
      pCommandArgs->nCommandID = CMD_READ_DN;
   }
    else if (strcmp(pArg, "verifyCert") == 0)
   {
     pCommandArgs->nCommandID = CMD_VERIFY_CERT;
   }
   /* Otherwise it is an invalid command. */
   else
   {
      fprintf(stderr, "Error: Invalid command: %s.\n", pArg);
      return false;
   }

   /* Check Command Argument (i.e. pArgv[2]) */
   switch(pCommandArgs->nCommandID)
   {
   case CMD_INSTALL_CERT:
      if (nArgc == 3)
      {
          /*
           * User should have typed the path to the file.
           * Get the certificate content from the certificate file path.
           */
          pCommandArgs->nFileLength = getFileContent(pArgv[2]/* path */, &pCommandArgs->pFileContent);
          if (pCommandArgs->nFileLength == 0)
          {
              fprintf(stderr, "Error: getFileContent() failed.\n");
              return false;
          }
      }
      else
      {
         fprintf(stderr, "Error: Argument missing.\n");
         return false;
      }
      break;

   case CMD_DELETE_CERT:
   case CMD_READ_DN:
      if (nArgc == 3)
      {
          /*
           * User should have typed the certificate's ID.
           * Transform  command string argument into integer.
           */
          if (sscanf(pArgv[2], "0x%x", &pCommandArgs->nIdFile1) == 0)
          {
              fprintf(stderr, "Error: bad Certificate Identifier format (must be hexadecimal format).\n");
              return false;
          }
      }
      else
      {
         fprintf(stderr, "Error: Argument missing.\n");
         return false;
      }
      break;

   case CMD_GET_CERT:
      if (nArgc > 2)
      {
          /*
           * User should have typed the certificate's ID.
           * Transform  command string argument into integer.
           */
          if (sscanf(pArgv[2], "0x%x", &pCommandArgs->nIdFile1) == 0)
          {
              fprintf(stderr, "Error: bad Certificate Identifier format (must be hexadecimal format).\n");
              return false;
          }
          switch(nArgc)
          {
          case 3:
              {
                  /* set default output to ./certificate<identifier> */
                  uint32_t nOutputLength;

                  nOutputLength = strlen("./certificate") + strlen(pArgv[2]);
                  pCommandArgs->pOutput = malloc(nOutputLength + 1);
                  if(pCommandArgs->pOutput == NULL)
                  {
                      fprintf(stderr, "Error: Cannot allocate pPath.\n");
                      return false;
                  }
                  strcpy((char*)pCommandArgs->pOutput, "./certificate");
                  strcat((char*)pCommandArgs->pOutput, pArgv[2]);
              }
              break;
          case 4:
              /* output was provided */
              pCommandArgs->pOutput = malloc(strlen(pArgv[3]));
              strcpy((char*)pCommandArgs->pOutput, pArgv[3]);
              break;
          default:
              fprintf(stderr, "Not enough parameters. \n");
              return false;
          }
      }
      else
      {
         fprintf(stderr, "Error: Argument missing.\n");
         return false;
      }
      break;

   case CMD_DELETE_ALL:
   case CMD_LIST_ALL:
      if (nArgc != 2)
      {
         fprintf(stderr, "Error: Too many arguments.\n");
         return false;
      }
      break;

   case CMD_VERIFY_CERT:
      if (nArgc == 4)
      {
          /*
           * User should have typed the certificate's ID followed by the issuer certificate ID.
           * Transform  command string argument into integer.
           */
          if (sscanf(pArgv[2], "0x%x", &pCommandArgs->nIdFile1) == 0)
          {
              fprintf(stderr, "Error: bad first Certificate Identifier format (must be hexadecimal format).\n");
              return false;
          }
          if (sscanf(pArgv[3], "0x%x", &pCommandArgs->nIdFile2) == 0)
          {
              fprintf(stderr, "Error: bad second Certificate Identifier format (must be hexadecimal format).\n");
              return false;
          }
      }
      else
      {
         fprintf(stderr, "Error: Argument missing.\n");
         return false;
      }
      break;

   default:
      fprintf(stderr, "Error: Invalid argument: %s.\n", pArg);
      return false;
   }

   /* Parse sucessful. */
   return true;
}


/**
 *  Function main:
 *  Description:
 *     Main entry point of the command-line application
 **/
#if defined(__SYMBIAN32__)
int __main__(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
   TEEC_Result nError;
   S_HANDLE    hSession;
   uint32_t    nLoginType;

   S_HANDLE          hDevice = S_HANDLE_NULL;
   COMMAND_ARGUMENTS sCommandArguments;

   printf("\nTrusted Logic CERTSTORE Example Application\n");

   /* Parse the command line */
   if (parseCmdLine(argc, argv, &sCommandArguments) == false)
   {
      printUsage();
      return 1;
   }

   /* Setup the login type */
   switch(sCommandArguments.nCommandID)
   {
   case CMD_INSTALL_CERT:
   case CMD_DELETE_CERT:
   case CMD_DELETE_ALL:
      nLoginType = TEEC_LOGIN_AUTHENTICATION; /* need to send the ssig file in param3 of operation, in OpenSession */
      break;

   case CMD_LIST_ALL:
   case CMD_READ_DN:
   case CMD_GET_CERT:
   case CMD_VERIFY_CERT:
      nLoginType = TEEC_LOGIN_PUBLIC;
      break;

   default:
      fprintf(stderr, "Error: Invalid Command.\n");
      return 1;
   }

   /* Initialize */
   nError = certStoreInitialize(&hDevice);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: certStoreInitialize() failed [0x%08x].\n", nError);
      return 1;
   }

   /* Open a session */
   nError = certStoreOpenSession(hDevice, nLoginType, &hSession);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: certStoreOpenSession() failed [0x%08x].\n", nError);
      return 1;
   }

   /* Command */
   switch(sCommandArguments.nCommandID)
   {
   case CMD_INSTALL_CERT:
      clientInstallCertificate(hSession,
                                sCommandArguments.pFileContent,
                                sCommandArguments.nFileLength,
                                &sCommandArguments.nIdFile1);
      break;

   case CMD_DELETE_CERT:
      clientDeleteCertificate(hSession,sCommandArguments.nIdFile1);
      break;

   case CMD_DELETE_ALL:
      clientDeleteAllCertificates(hSession);
      break;

   case CMD_LIST_ALL:
      clientListAllCertificates(hSession);
      break;

   case CMD_READ_DN:
      clientReadAndPrintCertificateDN(hSession,sCommandArguments.nIdFile1);
      break;

   case CMD_GET_CERT:
      {
         uint8_t* pCertificateBuffer = NULL;
         uint32_t nCertificateLength = 0;
         bool bError;

         clientGetCertificate(hSession,sCommandArguments.nIdFile1,&pCertificateBuffer,&nCertificateLength);

         /* copy certificate to output */
         bError = createFile((char*)sCommandArguments.pOutput,pCertificateBuffer, nCertificateLength);
         if (bError == false)
         {
             fprintf(stderr, "Error: createFile() failed.\n");
             return 1;
         }
         else
         {
             printf("The Certificate 0x%08X is successfully loaded in the file %s.\n", sCommandArguments.nIdFile1,
                                                                                       sCommandArguments.pOutput);
         }
         free(pCertificateBuffer);
      }
      break;

   case CMD_VERIFY_CERT:
      clientVerifyCertificate(hSession,sCommandArguments.nIdFile1, sCommandArguments.nIdFile2 );
      break;

   default:
      fprintf(stderr, "Error: Invalid Command.\n");
      return 1;
   }

   /* Free the command argument structure */
   free(sCommandArguments.pOutput);
   free(sCommandArguments.pFileContent);

   /* Close the session */
   nError = certStoreCloseSession(hSession);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: certStoreCloseSession() failed [0x%08x].\n", nError);
      return 1;
   }

   /* Finalize */
   nError = certStoreFinalize(hDevice);
   if(nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "Error: certStoreFinalize() failed [0x%08x].\n", nError);
      return 1;
   }

   return 0;
}
