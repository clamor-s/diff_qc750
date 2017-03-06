/**
 * Copyright (c) 2004-2009 Trusted Logic S.A.
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

/* ===========================================================================
Includes
=========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drm_stub.h"

#if defined(__SYMBIAN32__)
extern void symbianPrintf(const char* format, ...);
extern void symbianFprintf(FILE *stream, const char* format, ...);
extern void symbianPutc(int c, FILE *stream);
#define printf symbianPrintf
#define fprintf symbianFprintf
#define putc symbianPutc
#endif

/* ===========================================================================
See header for more information
=========================================================================== */
static void print_usage(void)
{
   printf("Usage: drm_client [options] <license_path> <content_path>\n");
   printf("Decrypts contents of the content file with the license key.\n\n");
   printf("Arguments: \n");
   printf("  license_path: the path to a file containing the license.\n");
   printf("  content_path: the path to a file containing the content.\n\n");
   printf("Options: \n");
   printf("  -h, -help: display this help\n");
   printf("  -init: initialize the drm root key.\n");
   printf("         This should typically be performed once during the \n");
   printf("         platform configuration, and must be done before the \n");
   printf("         decryption of the content file. \n");
   printf("  -clean: clean the keystore used by this application.\n");
   printf("  -use-stiplet: use the DRM STIPlet agent.\n\n");
}

/* ===========================================================================
See header for more information
=========================================================================== */
static DRM_ERROR parseCmdLine(
                 int    nArgc,
                 char*  pArgv[],
                 char** ppLicensePath,
                 char** ppContentPath,
                 int*   pInit,
                 int*   pClean,
                 bool*  pbUseStiplet)
{
   int   nIndex;
   int   nLicenseFound = 0;
   int   nContentFound = 0;
   char* pArg;

   /* Check we have enough parameters on the command line */
   if (nArgc < 2)
   {
      fprintf(stderr, "Error: not enough parameters.\n");
      return DRM_ERROR_ILLEGAL_ARGUMENT;
   }

   /* Reset the input values to default values */
   *pInit = 0;
   *pClean = 0;
   *pbUseStiplet = false;
   *ppLicensePath = NULL;
   *ppContentPath = NULL;

   /* For each parameter passed on the command line */
   for (nIndex = 1; nIndex < nArgc; nIndex++)
   {
      pArg = pArgv[nIndex];

      /* If first character is a '-' then this param is an option. */
      if (pArg[0] == '-')
      {
         /* If it is a help option, then return 0 (main will print
         * usage).
         */
         if ((strcmp(pArg, "-h") == 0) || (strcmp(pArg, "-help") == 0))
         {
            return DRM_ERROR_ILLEGAL_ARGUMENT;
         }
         /* If it is an init option, then set pInit reference to 1. */
         else if (strcmp(pArg, "-init") == 0)
         {
            *pInit = 1;
         }
         /* If it is a clean option, then set pRemove reference to 1. */
         else if (strcmp(pArg, "-clean") == 0)
         {
             *pClean = 1;
         }
         else if (strcmp(pArg, "-use-stiplet") == 0)
         {
             *pbUseStiplet = true;
         }
         /* Otherwise it is an invalid option. */
         else
         {
            fprintf(stderr, "Error: Invalid option: %s.\n", pArg);
            return DRM_ERROR_ILLEGAL_ARGUMENT;
         }
      }
      /* Else, must be a parameter to licenseFound or contentFound. */
      else
      {
         /* First non-option param specifies the licenseFound file path */
         if (nLicenseFound == 0)
         {
            *ppLicensePath = pArg;
            nLicenseFound = 1;
         }
         /* Second non-option param specifies the contentFound file path */
         else if (nContentFound == 0)
         {
            *ppContentPath = pArg;
            nContentFound = 1;
         }
         /* If we have a third non-option param, then error (too many args) */
         else
         {
            fprintf(stderr, "Error: Too many arguments.\n");
            return DRM_ERROR_ILLEGAL_ARGUMENT;
         }
      }
   }

   /* If we have a license file path, but not a content file path then error */
   if ((nLicenseFound == 1) && (nContentFound == 0))
   {
      fprintf(stderr, "Error: No path to contentFound file specified.\n");
      return DRM_ERROR_ILLEGAL_ARGUMENT;
   }

   /* Parse sucessful. */
   return DRM_SUCCESS;
}


/* ===========================================================================
See header for more information
=========================================================================== */
static size_t getFileContent(
                      char* pPath,
                      unsigned char** ppContent)
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

/* ===========================================================================
See header for more information
=========================================================================== */
static DRM_ERROR decryptAndDisplayContent(
                              char* pLicensePath,
                              char* pContentPath,
                              bool bUseStiplet)
{
   DRM_SESSION_HANDLE hSession;
   DRM_ERROR nError = DRM_SUCCESS;
   uint8_t* pEncryptedKey;
   uint32_t nEncryptedKeyLen;
   uint32_t nBlockSize = 256;
   uint32_t nCurrentBlockLen;
   uint8_t* pBlock = NULL;
   FILE* pContentFile;
   uint32_t numBytesRead;
   uint32_t nIndex;

   /* Load the license file contents from pLicensePath into a buffer pointed
   to by pLicenseData (getFileContent will allocate buffer). */
   nEncryptedKeyLen = getFileContent(pLicensePath,
      &pEncryptedKey);
   if (nEncryptedKeyLen == 0)
   {
      fprintf(stderr, "Error: license file read failed.\n");
      nError = DRM_ERROR_ILLEGAL_ARGUMENT;
      return nError;
   }
   /* open decryption session. Let the TFAPI allocate a block for us */
   nError = drm_open_decrypt_session(pEncryptedKey,
      nEncryptedKeyLen,
      (uint8_t**)&pBlock,
      nBlockSize,
      &hSession,
      bUseStiplet);
   if (nError != DRM_SUCCESS)
   {
      fprintf(stderr, "Error opening decryption session: %d\n", nError);
      free(pBlock);
      return nError;
   }

   /* Open the content file */
   pContentFile = fopen(pContentPath, "rb");
   if (pContentFile == NULL)
   {
      fprintf(stderr, "Error: cannot open content file.\n");
      nError = DRM_ERROR_ILLEGAL_ARGUMENT;
      goto close_session;
   }

   printf("Decrypted content:\n\n");
   printf("--- BEGIN ---\n");

   /* While not at end of file, then process blocks of file data */
   while (feof(pContentFile) == 0)
   {
      nCurrentBlockLen = 0;
      memset(pBlock, 0, nBlockSize);

      /* Read a block of 256 bytes */
      while (nCurrentBlockLen < nBlockSize)
      {
         /* Try to read enough bytes to fill current block */
         numBytesRead = fread(pBlock+nCurrentBlockLen, 1,
            nBlockSize-nCurrentBlockLen,
            pContentFile);
         /* Add number of bytes we have read to current block count */
         nCurrentBlockLen += numBytesRead;
         /* If we read no bytes, then we have reached end of last block */
         if (numBytesRead == 0)
         {
            break;
         }
      }
      nError = drm_decrypt(hSession, 0, nBlockSize);
      if (nError != DRM_SUCCESS)
      {
         fprintf(stderr, "Error while decrypting content [0x%x].\n", nError);
         goto close_session;
      }

      /* Print the block to stdout. Discard padding zeros. */
      for (nIndex = 0; nIndex < nCurrentBlockLen; nIndex++)
      {
         putc(pBlock[nIndex], stdout);
      }
   }
   fclose(pContentFile);
   printf("\n--- END ---\n");

close_session:
   (void)drm_close_decrypt_session(hSession);
   if(pEncryptedKey != NULL)
   {
      free(pEncryptedKey);
   }
   pEncryptedKey = NULL;
   return nError;
}

/* ===========================================================================
See header for more information
=========================================================================== */
static DRM_ERROR init(bool bUseStiplet)
{
   DRM_SESSION_HANDLE hSession;
   DRM_ERROR nError;

   nError = drm_open_management_session(&hSession, bUseStiplet);
   if (nError != DRM_SUCCESS)
   {
      fprintf(stderr, "Error: cannot open management session (error %u).\n", nError);
      return nError;
   }

   nError = drm_management_init(hSession);
   if (nError != DRM_SUCCESS)
   {
      fprintf(stderr, "Error: cannot initialize drm key (error %u).\n", nError);
      return nError;
   }

   nError = drm_close_management_session (hSession);
   if (nError != DRM_SUCCESS)
   {
      fprintf(stderr, "Error: cannot close management session (error %u).\n", nError);
      /* discard this error */
      return DRM_SUCCESS;
   }

   fprintf(stdout, "Initialization succeed.\n");
   return DRM_SUCCESS;
}


/* ===========================================================================
    See header for more information
   =========================================================================== */
static DRM_ERROR clean(bool bUseStiplet)
{
   DRM_SESSION_HANDLE hSession;
   DRM_ERROR nError;

   nError = drm_open_management_session(&hSession, bUseStiplet);
   if (nError != DRM_SUCCESS)
   {
      fprintf(stderr, "Error: cannot open management session (error %u).\n", nError);
      return nError;
   }

   nError = drm_management_clean(hSession);
   if (nError != DRM_SUCCESS)
   {
      fprintf(stderr, "Error: cannot clean key store (error %u).\n", nError);
      return nError;
   }

   nError = drm_close_management_session(hSession);
   if (nError != DRM_SUCCESS)
   {
      fprintf(stderr, "Error: cannot close management session (error %u).\n", nError);
      /* discard this error */
      return DRM_SUCCESS;
   }

   fprintf(stdout, "Cleanup succeed.\n");
   return DRM_SUCCESS;
}

/* ===========================================================================
The main entry point for this application
=========================================================================== */

#if defined (__SYMBIAN32__)
int __main__(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
   char* pContentPath = NULL;
   char* pLicensePath = NULL;
   int   nDoInit = 0;
   int   nDoClean = 0;
   bool  bUseStiplet = false;
   DRM_ERROR nError = DRM_SUCCESS;

   printf("Trusted Logic DRM Example Application\n\n");

   /* Parse the command line sent by the user */
   nError = parseCmdLine(argc, argv, &pLicensePath, &pContentPath, &nDoInit, &nDoClean, &bUseStiplet);
   if (nError != DRM_SUCCESS)
   {
      print_usage();
      return nError;
   }

   nError = drm_initialize();
   if (nError != DRM_SUCCESS)
   {
      fprintf(stderr, "Error initializing the DRM Agent.\n");
      return nError;
   }

   /* If "-init" was specified on the command line, then try to install the
   device private key */
   if (nDoInit)
   {
      /* Initialize the device key */
      nError = init(bUseStiplet);
      if (nError != DRM_SUCCESS)
      {
         fprintf(stderr, "Error: Device key initialization failed.\n");
         return nError;
      }
   }

   /* If "-clean" was specified on the command line, then try to clean the
      key store. */
   if (nDoClean)
   {
      /* Initialize the device key */
      nError = clean(bUseStiplet);
      if (nError != DRM_SUCCESS)
      {
         fprintf(stderr, "Error: Clean up failed.\n");
         return nError;
      }
   }

   /* If both content and license information was provided, then decrypt and
   display content on the screen. */
   if ((pLicensePath != NULL) && (pContentPath != NULL))
   {
      nError = decryptAndDisplayContent(pLicensePath, pContentPath, bUseStiplet);
   }
   else
   {
      nError = DRM_ERROR_ILLEGAL_ARGUMENT;
   }

   (void)drm_finalize();

   return nError;
}

