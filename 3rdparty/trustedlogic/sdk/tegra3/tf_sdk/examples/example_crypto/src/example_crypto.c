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

#include "cryptoki.h"

#include "example_crypto.h"
#include "example_crypto_init_data.h"

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

#if defined(LINUX) || (defined ANDROID) || defined(__SYMBIAN32__)
#define STRICMP strcasecmp
#else
#define STRICMP _stricmp
#endif

/* value set by command line arg "-slotID" */
CK_SLOT_ID gSlotId = 0x00000001;


/* ===========================================================================
    See header for more information
  =========================================================================== */
void print_usage(void)
{
    printf("\n\n");
    printf("Usage: example_crypto [options] <license_path> <content_path>\n");
    printf("Decrypts contents of the content file with the license key.\n\n");
    printf("Arguments: \n");
    printf("  license_path: the path to a file containing the license.\n");
    printf("  content_path: the path to a file containing the content.\n\n");
    printf("Options: \n");
    printf("  -h, -help: display this help\n");
    printf("  -slotID: SlotID to use.\n");
    printf("  -init: initialize the keystore used by this application.\n");
    printf("         This should typically be performed once during the \n");
    printf("         platform configuration, and must be done before the \n");
    printf("         decryption of the content file. \n");
    printf("  -clean: clean the keystore used by this application.\n\n");
    printf("\n");
}

/* ===========================================================================
    See header for more information
  =========================================================================== */
int parseCmdLine(
    int    nArgc,
    char*  pArgv[],
    char** ppLicensePath,
    char** ppContentPath,
    int*   pInit,
    int*   pClean)
{
    int   nIndex;
    int   nLicenseFound = 0;
    int   nContentFound = 0;
    char* pArg;

    /* Check we have enough parameters on the command line */
    if (nArgc < 2)
    {
        fprintf(stderr, "Error: not enough parameters.\n");
        return 1;
    }

    /* Reset the input values to default values */
    *pInit = 0;
    *pClean = 0;
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
                return 1;
            }
            /* -slotID */
            else if (STRICMP(pArg, "-slotID") == 0)
            {
               nIndex++;
               pArg = pArgv[nIndex];

               printf("\nUsing SlotID [%s] from command line\n\n", pArg);
               /*
                * Reads an integer. Hexadecimal if the input string begins with "0x" or "0X",
                * octal if the string begins with "0", otherwise decimal.
               */
               if (sscanf(pArg, "%i", &gSlotId) != 1)
               {
                  fprintf(stderr, "Error: Invalid SlotID: %s\n", pArg);
                  return 1;
               }
            }
            /* If it is an init option, then set pInit reference to 1. */
            else if (strcmp(pArg, "-init") == 0)
            {
                *pInit = 1;
            }
            /* If it is a clean option, then set pClean reference to 1. */
            else if (strcmp(pArg, "-clean") == 0)
            {
                *pClean = 1;
            }
            /* Otherwise it is an invalid option. */
            else
            {
                fprintf(stderr, "Error: Invalid option: %s.\n", pArg);
                return 1;
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
                return 1;
            }
        }
    }

    /* If we have a license file path, but not a content file path then error */
    if ((nLicenseFound == 1) && (nContentFound == 0))
    {
        fprintf(stderr, "Error: No path to content file specified.\n");
        return 1;
    }

    printf("\nSlotID = 0x%X\n\n", gSlotId);
    
    /* Parse sucessful. */
    return 0;
}


/* ===========================================================================
    See header for more information
  =========================================================================== */
size_t getFileContent(
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
int decryptAndDisplayContent(
    char* pLicensePath,
    char* pContentPath )
{
    /* =======================================================================
       General purpose variables
       =======================================================================*/
    /* Application session handle for PKCS#11 library */
    CK_SESSION_HANDLE hPKCS11Session;
    /* Error code for any PKCS#11 commands */
    CK_RV             errorCode;

    /* =======================================================================
       Variables for device key and related PKCS#11 operations
       =======================================================================*/
    /* Handle for the device key */
    CK_OBJECT_HANDLE  hDeviceKey;
    /* Object ID for the device key */
    static const CK_BYTE           pObjId1[] = {0x00, 0x01};
    /* Number of objects we are trying to match */
    CK_ULONG          objectCount;
    /* Create a mechanism for RSA X509 certificates */
    static const CK_MECHANISM RSA_mechanism =
    {
        CKM_RSA_X_509, NULL, 0
    };
    /* Create a template object to use when searching for the device key in
       the key store */
    static const CK_ATTRIBUTE findTemplate[] =
    {
      { CKA_ID, (void*)pObjId1, sizeof(pObjId1) }
    };

    /* =======================================================================
       Variables for license information
       =======================================================================*/
    /* Byte array representing the license information */
    CK_BYTE*          pCipherLicenseData = NULL;
    /* Length of the license data */
    CK_ULONG          cipherLicenseLen;
    /* Decoded license length */
    CK_BYTE           pClearLicenseData[128];
    /* Decoded license */
    CK_ULONG          clearLicenseLen = 128;

    /* =======================================================================
       Variables for content decode and related PKCS#11 operations
       =======================================================================*/
    /* Content file */
    FILE*             pContentFile;
    /* Loop counter */
    CK_ULONG          nIndex;
    /* Number of bytes read from content file */
    size_t            numBytesRead;
    /* Binary block holding the current working set of 256 bytes of data */
    CK_BYTE           pContentBlock[256];
    /* The actual input block size when decrypting content */
    CK_ULONG          nCurrentBlockLen;
    /* The target input block size when decrypting content */
    CK_ULONG          nContentBlockLen = 256;
    /* The target output block size when decrypting content */
    CK_ULONG          nClearTextBlockLen = 256;

    /* =======================================================================
       Variables for AES key and related PKCS#11 operations
       =======================================================================*/
    /* AES key handle in the key store */
    CK_OBJECT_HANDLE  hAESKey;
    /* Raw AES key */
    CK_BYTE           pAESKey[16];
    /* AES's key class (secret key) */
    static const CK_OBJECT_CLASS   aesKeyClass  = CKO_SECRET_KEY;
    /* AES key type (AES) */
    static const CK_KEY_TYPE       aesKeyType   = CKK_AES;
    /* Dummy variable which we can use for &"TRUE" */
    static const CK_BBOOL          bTrue = TRUE;
    /* AES initialization vector (Used for AES in any block chaining mode) */
    static const CK_BYTE           pAESInitVector[] =
    {
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    };
    /* Object ID for this key for this session */
    static const CK_BYTE           pObjId2[] = {0x00, 0x02};

    /* Define our AES mechanism as CBC with an IV */
    static const CK_MECHANISM AES_CBC_mechanism =
    {
        CKM_AES_CBC, (void*)pAESInitVector, sizeof(pAESInitVector)
    };
    
    /* Create a key template for the AES key. */
    CK_ATTRIBUTE aesKeyTemplate[5];
    aesKeyTemplate[0].type = CKA_CLASS;
    aesKeyTemplate[0].pValue = (void*)&aesKeyClass;
    aesKeyTemplate[0].ulValueLen = sizeof(aesKeyClass);
    aesKeyTemplate[1].type = CKA_ID;
    aesKeyTemplate[1].pValue = (void*)pObjId2;
    aesKeyTemplate[1].ulValueLen = sizeof(pObjId2);
    aesKeyTemplate[2].type = CKA_KEY_TYPE;
    aesKeyTemplate[2].pValue = (void*)&aesKeyType;
    aesKeyTemplate[2].ulValueLen = sizeof(aesKeyType);
    aesKeyTemplate[3].type = CKA_DECRYPT;
    aesKeyTemplate[3].pValue = (void*)&bTrue;
    aesKeyTemplate[3].ulValueLen = sizeof(bTrue);
    aesKeyTemplate[4].type = CKA_VALUE;
    aesKeyTemplate[4].pValue = (void*)pAESKey;
    aesKeyTemplate[4].ulValueLen = sizeof(pAESKey);

    /* Initialize the PKCS#11 interface */
    if ((errorCode = C_Initialize(NULL)) != CKR_OK)
    {
        fprintf(stderr, "Error: PKCS#11 library initialization failed [0x%x].\n",
                  (unsigned int)errorCode);
        return 1;
    }

    /* Open a session within PKCS#11 */
    if ((errorCode = C_OpenSession(gSlotId, CKF_SERIAL_SESSION,
                                   NULL, NULL, &hPKCS11Session)) != CKR_OK)
    {
        fprintf(stderr, "Error: PKCS#11 open session failed [0x%x].\n",
                  (unsigned int)errorCode);
        goto disconnect;
    }

    /* Present login credentials to PKCS#11 - user login information is taken
       from the absolute file path for the user executable. This stops other
       applications accessing this application's key store. */
    if ((errorCode = C_Login(hPKCS11Session, CKU_USER, NULL, 0)) != CKR_OK)
    {
        fprintf(stderr, "Error: PKCS#11 login failed [0x%x].\n",
                  (unsigned int)errorCode);
        goto disconnect;
    }

    /* Initialize a search within PKCS#11, using our find template to match
     * the device key */
    if ((errorCode = C_FindObjectsInit(
                        hPKCS11Session,
                        findTemplate,
                        sizeof(findTemplate)/sizeof(CK_ATTRIBUTE)))
        != CKR_OK)
    {
        fprintf(stderr, "Error: PKCS#11 cannot initialize key search [0x%x].\n",
                  (unsigned int)errorCode);
        goto disconnect;
    }

    /* Perform the search to match one object (device key), or fail */
    errorCode = C_FindObjects(hPKCS11Session, &hDeviceKey, 1, &objectCount);
    if (errorCode != CKR_OK)
    {
        fprintf (stderr, "Error: PKCS#11 cannot find the key [0x%x].\n",
                  (unsigned int)errorCode);
        goto disconnect;
    }
    if (objectCount == 0)
    {
       fprintf (stderr, "Error: the device key is not installed. Please install the device key.\n");
       errorCode = CKR_ARGUMENTS_BAD;
       goto disconnect;
    }

    /* If we have found the device key, we can stop the search. */
    if ((errorCode = C_FindObjectsFinal(hPKCS11Session)) != CKR_OK)
    {
        fprintf(stderr, "Error: PKCS#11 cannot terminate search [0x%x].\n",
                  (unsigned int)errorCode);
        goto disconnect;
    }

    /* Load the license file contents from pLicensePath into a buffer pointed
       to by pLicenseData (getFileContent will allocate buffer). */
    cipherLicenseLen = (CK_ULONG)getFileContent(pLicensePath,
                                                &pCipherLicenseData);
    if (cipherLicenseLen == 0)
    {
        fprintf(stderr, "Error: license file read failed.\n");
        errorCode = CKR_ARGUMENTS_BAD;
        goto disconnect;
    }

    /* Decrypt the license data using the device key. This will recover the
       plain text of the AES key used to encrypt the content file.
       The first stage of this operation is to init a decrypt operation. */
    if ((errorCode = C_DecryptInit(hPKCS11Session,
                                   &RSA_mechanism,
                                   hDeviceKey)) != CKR_OK)
    {
        fprintf(stderr, "Error: PKCS#11 cannot initialize decryption [0x%x].\n",
                  (unsigned int)errorCode);
        goto disconnect;
    }

    /* The second stage of this operation is to actually do the decrypt */
    if ((errorCode = C_Decrypt(hPKCS11Session,
                               pCipherLicenseData,
                               cipherLicenseLen,
                               pClearLicenseData,
                               &clearLicenseLen)) != CKR_OK)
    {
        fprintf(stderr, "Error: PKCS#11 license key decryption failed [0x%x].\n",
                  (unsigned int)errorCode);
        goto disconnect;
    }

    /* We can now free the license data */
    free(pCipherLicenseData);
    pCipherLicenseData = NULL;

    /* The data in the license file is padded to 128-byte length. The actual
       AES key value is in the 16 last bytes. */
    if (clearLicenseLen != 128)
    {
        fprintf(stderr, "Error: PKCS#11 decrypted license is invalid.\n");
        errorCode = CKR_ARGUMENTS_BAD;
        goto disconnect;
    }
    /* Copy actual key data into our key template */
    memcpy(pAESKey, pClearLicenseData+128-sizeof(pAESKey), sizeof(pAESKey));

    /* Import the AES key into this application's PKCS#11 key store */
    if ((errorCode = C_CreateObject(hPKCS11Session,
                                    aesKeyTemplate, sizeof(aesKeyTemplate) / sizeof(CK_ATTRIBUTE),
                                    &hAESKey)) != CKR_OK)
    {
        fprintf(stderr, "Error: PKCS#11 license key import failed [0x%x].\n",
                  (unsigned int)errorCode);
        goto disconnect;
    }

    /* Open the content file */
    pContentFile = fopen(pContentPath, "rb");
    if (pContentFile == NULL)
    {
        fprintf(stderr, "Error: PKCS#11 cannot open content file.\n");
        errorCode = CKR_ARGUMENTS_BAD;
        goto destroy_key;
    }

    /* We will decrypt the content in blocks - so the first step is initialize
       a decrypt session */
    if ((errorCode = C_DecryptInit(hPKCS11Session,
                                   &AES_CBC_mechanism,
                                   hAESKey)) != CKR_OK)
    {
        fprintf(stderr, "Error: PKCS#11 decryption init failed [0x%x].\n",
                  (unsigned int)errorCode);
        goto destroy_key;
    }

    printf("Decrypted content:\n\n");
    printf("--- BEGIN ---\n");

    /* While not at end of file, then process blocks of file data */
    while (feof(pContentFile) == 0)
    {
        nCurrentBlockLen = 0;
        memset(pContentBlock, 0, nContentBlockLen);

        /* Read a block of 256 bytes */
        while (nCurrentBlockLen < nContentBlockLen)
        {
            /* Try to read enough bytes to fill current block */
            numBytesRead = fread(pContentBlock+nCurrentBlockLen, 1,
                                 nContentBlockLen-nCurrentBlockLen,
                                 pContentFile);
            /* Add number of bytes we have read to current block count */
            nCurrentBlockLen += (CK_ULONG)numBytesRead;
            /* If we read no bytes, then we have reached end of last block */
            if (numBytesRead == 0)
            {
                break;
            }
        }

        /* Call PKCS#11 to decrypt this block */
        if ((errorCode = C_DecryptUpdate(hPKCS11Session,
                                         pContentBlock,
                                         nContentBlockLen,
                                         pContentBlock,
                                         &nClearTextBlockLen)) != CKR_OK)
        {
            fprintf(stderr, "Error: PKCS#11 content decryption failed [0x%x].\n",
                  (unsigned int)errorCode);
            fclose(pContentFile);
            goto destroy_key;
        }

        /* If clear text is not a full block length, then we don't want to
           print the padding zeros. */
        if (nClearTextBlockLen < nCurrentBlockLen)
        {
            nCurrentBlockLen = nClearTextBlockLen;
        }
        /* Print the block to stdout. */
        for (nIndex = 0; nIndex < nCurrentBlockLen; nIndex++)
        {
            putc(pContentBlock[nIndex], stdout);
        }
        /* Reset expected block size for next loop */
        nClearTextBlockLen = nContentBlockLen;
    }

    /* Close off the decrypt session (we don't actually decrypt anything here)
       Note: to close off the session, we must pass a non-NULL buffer in the
       pLastPart parameter. Otherwise, the call only returns the return length
       (zero in this case) and does not close the session. See section 4.2 of
       the PKCS#11 specification.
     */
    nContentBlockLen = 0;
    (void)C_DecryptFinal(hPKCS11Session, pContentBlock, &nContentBlockLen);

    fclose(pContentFile);
    printf("--- END ---\n");

    /* Discard errors while closing */
destroy_key:
    (void)C_DestroyObject(hPKCS11Session, hAESKey);
disconnect:
    (void)C_Logout(hPKCS11Session);
    (void)C_CloseSession(hPKCS11Session);
    (void)C_Finalize(NULL);

    /* Ensure cipher'd license data is destroyed, even in fail cases. */
    if(pCipherLicenseData != NULL)
    {
        free(pCipherLicenseData);
    }
    pCipherLicenseData = NULL;
    if (errorCode == CKR_OK)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/* ===========================================================================
    See header for more information
   =========================================================================== */
int init(void)
{
    /* Error code for PKCS#11 operations */
    CK_RV             errorCode;
    /* Session handle for PKCS11 library */
    CK_SESSION_HANDLE hPKCS11Session;
    /* Variable for holding device key class (private key) */
    static const CK_OBJECT_CLASS   devKeyClass  = CKO_PRIVATE_KEY;
    /* Variable for holding device key type (RSA key) */
    static const CK_KEY_TYPE       devKeyType  = CKK_RSA;
    /* Object ID of the device key */
    static const CK_BYTE           pObjId[] = {0x00, 0x01};
    /* Dummy varaible so we can pass "TRUE" by reference. */
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
    {CKA_MODULUS,          (void*)sDevKeyModulus,  sizeof(sDevKeyModulus) },
    {CKA_PUBLIC_EXPONENT,  (void*)sDevKeyPublic_e, sizeof(sDevKeyPublic_e) },
    {CKA_PRIVATE_EXPONENT, (void*)sDevKeyPriv_e,   sizeof(sDevKeyPriv_e)   },
    {CKA_PRIME_1,          (void*)sDevKeyPrime_P,  sizeof(sDevKeyPrime_P)  },
    {CKA_PRIME_2,          (void*)sDevKeyPrime_Q,  sizeof(sDevKeyPrime_Q)  },
    {CKA_EXPONENT_1,       (void*)sDevKey_eP,      sizeof(sDevKey_eP)      },
    {CKA_EXPONENT_2,       (void*)sDevKey_eQ,      sizeof(sDevKey_eQ)      },
    {CKA_COEFFICIENT,      (void*)sDevKey_qInv,    sizeof(sDevKey_qInv)    }
    };

    /* Init the PKCS#11 library for this application */
    if ((errorCode = C_Initialize(NULL)) != CKR_OK)
    {
       fprintf(stderr, "Error: PKCS#11 library initialization failed [0x%x].\n",
               (unsigned int)errorCode);
        return 1;
    }

    /* Open a session within the PKCS#11 library for this application */
    if ((errorCode = C_OpenSession(gSlotId,
                                CKF_RW_SESSION | CKF_SERIAL_SESSION,
                                NULL,
                                NULL,
                                &hPKCS11Session)) != CKR_OK)
    {
        fprintf(stderr, "Error: PKCS#11 open session failed [0x%x].\n",
               (unsigned int)errorCode);
        (void)C_Finalize(NULL);
        return 1;
    }

    /* Present login credentials to PKCS#11 - user login information is taken
       from the absolute file path for the user executable. This stops other
       applications accessing this application's key store. */
    if ((errorCode = C_Login(hPKCS11Session,
                             CKU_USER,
                             NULL,
                             0)) != CKR_OK)
    {
        fprintf(stderr, "Error: PKCS#11 login failed [0x%x].\n",
                  (unsigned int)errorCode);
        (void)C_CloseSession(hPKCS11Session);
        (void)C_Finalize(NULL);
        return 1;
    }

    /* Import the device key into the PKCS#11 key store for this application. */
    if ((errorCode = C_CreateObject(hPKCS11Session,
                                    devPrivateKeyTemplate,
                                    sizeof(devPrivateKeyTemplate)/sizeof(CK_ATTRIBUTE),
                                    &hDevKey)) != CKR_OK)
    {
        fprintf(stderr, "Warning: Device key may be already initialized.\n");
        (void)C_Logout(hPKCS11Session);
        (void)C_CloseSession(hPKCS11Session);
        (void)C_Finalize(NULL);
        return 2;
    }

    /* Discard errors while exiting */
   (void)C_Logout(hPKCS11Session);
   (void)C_CloseSession(hPKCS11Session);
   (void)C_Finalize(NULL);

   fprintf(stdout, "Initialization succeed.\n");
   return 0;
}

/* ===========================================================================
    See header for more information
   =========================================================================== */
int clean(void)
{
    /* Error code for PKCS#11 operations */
    CK_RV             errorCode;
    /* Session handle for PKCS11 library */
    CK_SESSION_HANDLE hPKCS11Session;
    /* Handle for a key */
    CK_OBJECT_HANDLE  hKey;
    /* The number of key found at each step */
    CK_ULONG ulKeyCount;

   /* Init the PKCS#11 library for this application */
    if ((errorCode = C_Initialize(NULL)) != CKR_OK)
    {
       fprintf(stderr, "Error: PKCS#11 library initialization failed [0x%x].\n",
               (unsigned int)errorCode);
        return 1;
    }

    /* Open a session within the PKCS#11 library for this application */
    if ((errorCode = C_OpenSession(gSlotId,
                                CKF_RW_SESSION | CKF_SERIAL_SESSION,
                                NULL,
                                NULL,
                                &hPKCS11Session)) != CKR_OK)
    {
        fprintf(stderr, "Error: PKCS#11 open session failed [0x%x].\n",
               (unsigned int)errorCode);
        (void)C_Finalize(NULL);
        return 1;
    }

    /* Present login credentials to PKCS#11 - user login information is taken
       from the absolute file path for the user executable. This stops other
       applications accessing this application's key store. */
    if ((errorCode = C_Login(hPKCS11Session,
                             CKU_USER,
                             NULL,
                             0)) != CKR_OK)
    {
        fprintf(stderr, "Error: PKCS#11 login failed [0x%x].\n",
                  (unsigned int)errorCode);
        (void)C_CloseSession(hPKCS11Session);
        (void)C_Finalize(NULL);
        return 1;
    }

    while (1)
    {
       /* Initialize object search. We look for all objects. */
       errorCode = C_FindObjectsInit(hPKCS11Session, NULL, 0);
       if (errorCode != CKR_OK)
       {
          fprintf(stderr, "Error: PKCS#11 find object init failed [0x%x].\n",
                     (unsigned int)errorCode);
          goto disconnect;
       }

       /* Find the next object in the keystore. */
       errorCode = C_FindObjects(hPKCS11Session,
                                 &hKey,
                                 1,
                                 &ulKeyCount);
       if (errorCode != CKR_OK)
       {
          fprintf(stderr, "Error: PKCS#11 find object failed [0x%x].\n",
                     (unsigned int)errorCode);
          goto disconnect;
       }

       /* Finalize the object search. */
       errorCode = C_FindObjectsFinal(hPKCS11Session);
       if (errorCode != CKR_OK)
       {
           fprintf(stderr, "Error: PKCS#11 find object final stage failed [0x%x].\n",
                     (unsigned int)errorCode);
           goto disconnect;
       }

       if (ulKeyCount == 0)
         break;

       /* Destroy the object found. */
       errorCode = C_DestroyObject(hPKCS11Session, hKey);
       if (errorCode != CKR_OK)
       {
          fprintf(stderr, "Error: PKCS#11 destroy object failed [0x%x].\n",
                  (unsigned int)errorCode);
          goto disconnect;
       }
    }

disconnect:
    /* Discard errors while exiting */
   (void)C_Logout(hPKCS11Session);
   (void)C_CloseSession(hPKCS11Session);
   (void)C_Finalize(NULL);

   if (errorCode == CKR_OK)
   {
      fprintf(stdout, "Cleanup succeed.\n");
      return 0;
   }
   else
   {
      fprintf(stdout, "Cleanup failure.\n");
      return 1;
   }
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
    int   nStatus;

    printf("Trusted Logic PKCS#11 Crypto API Example Application\n\n");

    /* Parse the command line sent by the user */
    nStatus = parseCmdLine(argc, argv, &pLicensePath, &pContentPath, &nDoInit, &nDoClean);
    if (nStatus != 0)
    {
        print_usage();
        return nStatus;
    }
    /* If "-clean" was specified on the command line, clean the key store */
    if (nDoClean)
    {
        /* Clean the key store */
        nStatus = clean();
        if (nStatus != 0)
        {
            fprintf(stderr, "Error: Key store cleanup failed.\n");
            return nStatus;
        }
    }

    /* If "-init" was specified on the command line, then try to install the
       device private key */
    if (nDoInit)
    {
        /* Initialize the device key */
        nStatus = init();
        if (nStatus != 0)
        {
            fprintf(stderr, "Error: Device key initialization failed.\n");
            return nStatus;
        }
    }

    /* If both content and license information was provided, then decrypt and
       display content on the screen. */
    if ((pLicensePath != NULL) && (pContentPath != NULL))
    {
        nStatus = decryptAndDisplayContent(pLicensePath, pContentPath);
        if (nStatus != 0)
        {
            fprintf(stderr, "Error: decryption failed.\n");
            return nStatus;
        }
    }

    return 0;
}

#ifdef LIBRARY_WITH_JNI
int argumentsRead (char** datablock,char* javaData)
{
	int i=1;

	char*p = NULL;
	int len;
	p=strtok(javaData," ");
	
	while(p!=NULL)
	{
		strcpy(datablock[i],p);
		len=strlen(p);
		javaData+=len+1;
		i++;
		p=strtok(javaData," ");
	}
	return i;
}

JNIEXPORT __attribute__((visibility("default"))) void JNICALL
Java_com_trustedlogic_android_examples_Crypto_CallMain
  (JNIEnv *env, jobject this, jstring device, jstring external_storage, jstring main_arguments)
{
	jboolean iscopy;

	////argv initialisation 5 params max, 128 char each
	char** arguments= malloc(5 * sizeof(char*));
	int i,nb_arg;
	for (i=0;i<5;i++)
	{		
		arguments[i]=malloc(128*sizeof(char));
	}

	const char * device_name=(*env)->GetStringUTFChars(env, device, &iscopy);
	const char * storage=(*env)->GetStringUTFChars(env, external_storage, &iscopy);
		
	char* javaData =(*env)->GetStringUTFChars(env, main_arguments, &iscopy);

		(*env)->ReleaseStringChars(env, main_arguments, javaData);
	
		nb_arg=argumentsRead(arguments,javaData);

	int file1 = freopen (storage,"w",stdout); // redirect stdout (currently set to /dev/null) to the logging file
	int file2 = freopen (storage,"w",stderr); // redirect stderr (currently set to /dev/null) to the logging file

	// call main...
	int result = main(nb_arg, arguments);
	fclose (stdout);
	fclose (stderr);

	(*env)->ReleaseStringChars(env, device, device_name);
	(*env)->ReleaseStringChars(env, external_storage, storage);

	
	//free operation
	for (i=0;i<5;i++)
	{
		free(arguments[i]);
	}
	free(arguments);

}
#endif

