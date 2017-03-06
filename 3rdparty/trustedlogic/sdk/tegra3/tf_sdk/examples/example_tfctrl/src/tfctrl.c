/*
* Copyright (c) 2004-2008 Trusted Logic S.A.
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

/* ===========================================================================
    Includes
  =========================================================================== */
#if defined(ANDROID)
#include <stddef.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef LIBRARY_WITH_JNI
#include <jni.h>
#include <android/log.h>

static void logCatInfo(const char *format, ...)
{
   va_list ap;
   va_start(ap, format);
   __android_log_vprint(ANDROID_LOG_INFO   , "TFCTRL", format, ap);
   va_end(ap);
}

static void logCatError(const char *format, ...)
{
   va_list ap;
   va_start(ap, format);
   __android_log_vprint(ANDROID_LOG_ERROR   , "TFCTRL", format, ap);
   va_end(ap);
}

static void (*traceInfo)(const char *format, ...) = logCatInfo;
static void (*traceError)(const char *format, ...) = logCatError;

#endif

#include "service_manager_protocol.h"

#include "tee_client_api.h"

#if defined(__SYMBIAN32__)
extern void symbianPrintf(const char* format, ...);
extern void symbianFprintf(FILE *stream, const char* format, ...);
#define printf symbianPrintf
#define fprintf symbianFprintf
#endif

/*
 * Function:
 * printUsage
 *
 * Description:
 * This function prints the usage instructions on stderr and exits with code 2
 *
 * Inputs:
 *    - - -
 *
 * Outputs:
 *    - - -
 *
 * Return Value:
 *    - - -
 */
void print_usage(void)
{
    fprintf(stderr, "Usage: tfctrl <command>\n");
    fprintf(stderr, "  Supported commands are:\n");
    fprintf(stderr, "  > list: list all the implementation properties, the services and the list of their properties\n");
    exit(2);
}

/*
 * Function:
 * tfctrl_list
 *
 * Description:
 * This function execute the list command
 *
 * Inputs:
 *    - - -
 *
 * Outputs:
 *    - - -
 *
 * Return Value:
 *    - - -
 */
void execute_list(void)
{
   TEEC_Result    nError;
   TEEC_Context   sContext;
   TEEC_Session   sSession;
   TEEC_Operation sOperation;
   TEEC_ImplementationInfo sDescription;
   const TEEC_UUID service_manager_uuid = SERVICE_MANAGER_UUID;

   fprintf(stderr, "Trusted Logic TFCTRL\n\n");

   /* Create the TEE Context */

   nError = TEEC_InitializeContext(NULL,  /* const char * name */
                                   &sContext);   /* TEEC_Context* context */
   if (nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "TEEC_InitializeContext returned error 0x%08X\n", nError);
      exit(nError);
   }

   TEEC_GetImplementationInfo(&sContext, &sDescription);

    /* Dump all implementation info */
   printf("[Implementation Properties]\n");
   printf(" apiDescription  : %s\n", sDescription.apiDescription);
   printf(" commsDescription: %s\n", sDescription.commsDescription);
   printf(" TEEDescription  : %s\n", sDescription.TEEDescription);
   printf("\n");

   /* Open the TEE Session */

   memset(&sOperation, 0, sizeof(TEEC_Operation));
   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
                                              TEEC_NONE,
                                              TEEC_NONE,
                                              TEEC_NONE);
   sOperation.params[0].value.a = 0x02;   /* SM_CONTROL_MODE_USER */
   nError = TEEC_OpenSession(&sContext,
                             &sSession,               /* OUT session */
                             &service_manager_uuid,   /* destination UUID */
                             TEEC_LOGIN_PUBLIC,       /* connectionMethod */
                             NULL,                    /* connectionData */
                             &sOperation,             /* IN OUT operation */
                             NULL                     /* OUT errorOrigin, optional */
                             );

   if (nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "TEEC_OpenSession returned error 0x%08X\n", nError);
      exit(nError);
   }

   /* Get all the services */
   {
      TEEC_UUID*  pServiceIdentifierList = NULL;
      uint32_t    nServiceIdentifierListLen = 8 * sizeof(TEEC_UUID);
      uint32_t    nListLength;
      uint32_t    i;

      pServiceIdentifierList = (TEEC_UUID*)malloc(nServiceIdentifierListLen);
      if (pServiceIdentifierList == NULL)
      {
         fprintf(stderr, "Out Of memory for Service Identifiers [%d bytes]\n", nServiceIdentifierListLen);
         exit(TEEC_ERROR_OUT_OF_MEMORY);
      }

send_invoke_get_all_services:

      memset(pServiceIdentifierList, 0, nServiceIdentifierListLen);
      memset(&sOperation, 0, sizeof(TEEC_Operation));

      sOperation.paramTypes = TEEC_PARAM_TYPES(
         TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

      sOperation.params[0].tmpref.buffer = pServiceIdentifierList;
      sOperation.params[0].tmpref.size   = nServiceIdentifierListLen;

      nError = TEEC_InvokeCommand(&sSession,
                                  SERVICE_MANAGER_COMMAND_ID_GET_ALL_SERVICES,
                                  &sOperation,     /* IN OUT operation */
                                  NULL             /* OUT errorOrigin, optional */
                                 );
      if ((nError != TEEC_SUCCESS) && (nError != TEEC_ERROR_SHORT_BUFFER))
      {
         fprintf(stderr, "TEEC_InvokeCommand(GET_ALL_SERVICES) returned error 0x%08X\n", nError);
         exit(nError);
      }

      nServiceIdentifierListLen = (uint32_t)sOperation.params[0].tmpref.size;
      if (nError == TEEC_ERROR_SHORT_BUFFER)
      { 
         /*
         * TempRef size is updated depending on the number of properties.
         * Need to reallocate the buffer.
         */
         pServiceIdentifierList = (TEEC_UUID*)realloc(pServiceIdentifierList, nServiceIdentifierListLen);
         if (pServiceIdentifierList == NULL)
         {
            fprintf(stderr, "Out Of memory for Service Identifiers [%d bytes]\n", nServiceIdentifierListLen);
            exit(TEEC_ERROR_OUT_OF_MEMORY);
         }
         goto send_invoke_get_all_services;
      }

      nListLength = nServiceIdentifierListLen / sizeof(TEEC_UUID);

      /* Iterate through the service list */
      for (i = 0; i < nListLength; i++)
      {
         TEEC_UUID* pIdentifier = &pServiceIdentifierList[i];
         uint32_t j;
         uint8_t* pPropertiesBuffer = NULL;
         uint32_t nPropertiesBufferLen = 64;

         /* Dump the service UUID */
         printf("[%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X]\n",
            pIdentifier->time_low,
            pIdentifier->time_mid,
            pIdentifier->time_hi_and_version,
            pIdentifier->clock_seq_and_node[0],
            pIdentifier->clock_seq_and_node[1],
            pIdentifier->clock_seq_and_node[2],
            pIdentifier->clock_seq_and_node[3],
            pIdentifier->clock_seq_and_node[4],
            pIdentifier->clock_seq_and_node[5],
            pIdentifier->clock_seq_and_node[6],
            pIdentifier->clock_seq_and_node[7]);

         pPropertiesBuffer = (uint8_t*)malloc(nPropertiesBufferLen);
         if (pPropertiesBuffer == NULL)
         {
            fprintf(stderr, "Out Of memory for Properties Buffer [%d bytes]\n", nPropertiesBufferLen);
            exit(TEEC_ERROR_OUT_OF_MEMORY);
         }

send_invoke_get_all_properties:

         /*
          * Dump all the service properties
          */

         memset(pPropertiesBuffer, 0, nPropertiesBufferLen);
         memset(&sOperation, 0, sizeof(TEEC_Operation));

         sOperation.paramTypes = TEEC_PARAM_TYPES(
            TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT,
            TEEC_NONE, TEEC_NONE);

         sOperation.params[0].tmpref.buffer = pIdentifier;
         sOperation.params[0].tmpref.size   = sizeof(TEEC_UUID);
         sOperation.params[1].tmpref.buffer = pPropertiesBuffer;
         sOperation.params[1].tmpref.size   = nPropertiesBufferLen;

         nError = TEEC_InvokeCommand(&sSession,
                                     SERVICE_MANAGER_COMMAND_ID_GET_ALL_PROPERTIES,
                                     &sOperation,     /* IN OUT operation */
                                     NULL             /* OUT errorOrigin, optional */
                                    );
         if ((nError != TEEC_SUCCESS) && (nError != TEEC_ERROR_SHORT_BUFFER))
         {
            fprintf(stderr, "TEEC_InvokeCommand(GET_ALL_PROPERTIES) returned error 0x%08X\n", nError);
            exit(nError);
         }

         nPropertiesBufferLen = (uint32_t)sOperation.params[1].tmpref.size;
         if (nError == TEEC_ERROR_SHORT_BUFFER)
         { 
            /*
            * TempRef size is updated depending on the number of properties.
            * Need to reallocate the buffer.
            */
            pPropertiesBuffer = (uint8_t*)realloc(pPropertiesBuffer, nPropertiesBufferLen);
            if (pPropertiesBuffer == NULL)
            {
               fprintf(stderr, "Out Of memory for Properties Buffer [%d bytes]\n", nPropertiesBufferLen);
               exit(TEEC_ERROR_OUT_OF_MEMORY);
            }
            goto send_invoke_get_all_properties;
         }

         j = 0;
         while (j < sOperation.params[1].tmpref.size)
         {
            uint32_t nPropertyNameLength = 0;
            uint32_t nPropertyValueLength = 0;

            uint8_t* pPropertyName = NULL;
            uint8_t* pPropertyValue = NULL;

            if (sOperation.params[1].tmpref.size - j >= 4)
            {
               nPropertyNameLength = *((uint32_t *)(&pPropertiesBuffer[j]));
               j += 4;
            }
            else
            {
               j = (uint32_t)sOperation.params[1].tmpref.size;
            }
            if (sOperation.params[1].tmpref.size - j >= 4)
            {
               nPropertyValueLength = *((uint32_t *)(&pPropertiesBuffer[j]));
               j += 4;
            }
            else
            {
               /* End of the parsing */
               break;
            }

            if (sOperation.params[1].tmpref.size - j >= nPropertyNameLength)
            {
               /* CAUTION: This is not a null-terminated string */
               pPropertyName = &pPropertiesBuffer[j];
               j += nPropertyNameLength;
            }
            else
            {
               /* End of the parsing */
               break;
            }

            if (sOperation.params[1].tmpref.size - j >= nPropertyValueLength)
            {
               /* CAUTION: This is not a null-terminated string */
               pPropertyValue = &pPropertiesBuffer[j];
               j += nPropertyValueLength;
            }
            else
            {
               /* End of the parsing */
               break;
            }

            /* Alignement for the next property */
            j = (j + 3) & ~3;

            /* CAUTION: Print a string without the null-terminated character */
            printf("%.*s: %.*s\n", nPropertyNameLength, pPropertyName, nPropertyValueLength, pPropertyValue);
         }

         /* We're done with this service */
         printf("\n");
         free(pPropertiesBuffer);
      }

      /* We're all set */
      free(pServiceIdentifierList);
   }

   /* Close the TEE Session */
   TEEC_CloseSession(&sSession);

   /* Close the TEE Context */
   TEEC_FinalizeContext(&sContext);
}

/*
 * Function:
 * smctrl_download
 *
 * Description:
 *
 *
 * Inputs:
 *    - - -
 *
 * Outputs:
 *    - - -
 *
 * Return Value:
 *    - - -
 */
void execute_download(char *pFileName)
{
   FILE *pFile;
   int nResult;
   long nFileSize;
   void *buffer;
   size_t nReadSize;

   TEEC_Result    nError;
   TEEC_Context   sContext;
   TEEC_Session   sSession;
   TEEC_Operation sOperation;
   const TEEC_UUID service_manager_uuid = SERVICE_MANAGER_UUID;
   TEEC_UUID sServiceUUID;

   fprintf(stderr, "Trusted Logic TFCTRL\n\n");
   fprintf(stderr, "Downloading '%s'\n", pFileName);

   pFile = fopen(pFileName, "rb");
   if (pFile == NULL)
   {
      fprintf(stderr, "fopen failed to open '%s'\n", pFileName);
      exit(1);
   }
   nResult = fseek(pFile, 0, SEEK_END);
   if (nResult != 0)
   {
      fprintf(stderr, "fseek returned error 0x%08X\n", nResult);
      exit(nResult);
   }
   nFileSize = ftell(pFile);
   nResult = fseek(pFile, 0, SEEK_SET);
   if (nResult != 0)
   {
      fprintf(stderr, "fseek returned error 0x%08X\n", nResult);
      exit(nResult);
   }

   buffer = malloc(nFileSize);
   if (buffer == NULL)
   {
      fprintf(stderr, "malloc failed\n");
      exit(1);
   }

   nReadSize = fread(buffer, 1, nFileSize, pFile);
   if ((long)nReadSize != nFileSize)
   {
      fprintf(stderr, "fread failed\n");
      exit(1);
   }

   fclose(pFile);

   /* Create the TEE Context */

   nError = TEEC_InitializeContext(NULL,  /* const char * name */
                                   &sContext);   /* TEEC_Context* context */
   if (nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "TEEC_InitializeContext returned error 0x%08X\n", nError);
      exit(nError);
   }

   /* Open the TEE Session */

   memset(&sOperation, 0, sizeof(TEEC_Operation));
   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
                                              TEEC_NONE,
                                              TEEC_NONE,
                                              TEEC_NONE);
   sOperation.params[0].value.a = 0x08;   /* SM_CONTROL_MODE_MANAGER */
   nError = TEEC_OpenSession(&sContext,
                             &sSession,               /* OUT session */
                             &service_manager_uuid,   /* destination UUID */
                             TEEC_LOGIN_PRIVILEGED,   /* connectionMethod */
                             NULL,                    /* connectionData */
                             &sOperation,             /* IN OUT operation */
                             NULL                     /* OUT errorOrigin, optional */
                             );

   if (nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "TEEC_OpenSession returned error 0x%08X\n", nError);
      exit(nError);
   }

   /* Download the service */
   memset(&sOperation, 0, sizeof(TEEC_Operation));

   sOperation.paramTypes = TEEC_PARAM_TYPES(
      TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE, TEEC_NONE);

   sOperation.params[0].tmpref.buffer = buffer;
   sOperation.params[0].tmpref.size   = nFileSize;
   sOperation.params[1].tmpref.buffer = &sServiceUUID;
   sOperation.params[1].tmpref.size   = sizeof(sServiceUUID);

   nError = TEEC_InvokeCommand(&sSession,
                               SERVICE_MANAGER_COMMAND_ID_DOWNLOAD_SERVICE,
                               &sOperation,     /* IN OUT operation */
                               NULL             /* OUT errorOrigin, optional */
                              );
   free(buffer);

   if ((nError != TEEC_SUCCESS) && (nError != TEEC_ERROR_SHORT_BUFFER))
   {
      fprintf(stderr, "TEEC_InvokeCommand(DOWNLOAD_SERVICE) returned error 0x%08X\n", nError);
      exit(nError);
   }

   /* Dump the service UUID */
   printf("[%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X]\n",
      sServiceUUID.time_low,
      sServiceUUID.time_mid,
      sServiceUUID.time_hi_and_version,
      sServiceUUID.clock_seq_and_node[0],
      sServiceUUID.clock_seq_and_node[1],
      sServiceUUID.clock_seq_and_node[2],
      sServiceUUID.clock_seq_and_node[3],
      sServiceUUID.clock_seq_and_node[4],
      sServiceUUID.clock_seq_and_node[5],
      sServiceUUID.clock_seq_and_node[6],
      sServiceUUID.clock_seq_and_node[7]);
   
   /* Close the TEE Session */
   TEEC_CloseSession(&sSession);

   /* Close the TEE Context */
   TEEC_FinalizeContext(&sContext);

   printf("SUCCESS\n");
}

/*
 * Function:
 * smctrl_remove
 *
 * Description:
 *
 *
 * Inputs:
 *    - - -
 *
 * Outputs:
 *    - - -
 *
 * Return Value:
 *    - - -
 */
void execute_remove(char *pUUID)
{
   TEEC_Result nError;
   TEEC_UUID sServiceUUID;
   int sValue[10];

   TEEC_Context   sContext;
   TEEC_Session   sSession;
   TEEC_Operation sOperation;
   const TEEC_UUID service_manager_uuid = SERVICE_MANAGER_UUID;

   fprintf(stderr, "Trusted Logic TFCTRL\n\n");

   if(sscanf(
         pUUID,
         "%8lx-%4x-%4x-%2x%2x-%2x%2x%2x%2x%2x%2x",
         (long*)&sServiceUUID.time_low,
         &sValue[0],
         &sValue[1],
         &sValue[2],
         &sValue[3],
         &sValue[4],
         &sValue[5],
         &sValue[6],
         &sValue[7],
         &sValue[8],
         &sValue[9]) != 11)
   {
      fprintf(stderr, "Ill-formed UUID\n");
      exit(1);
   }
   sServiceUUID.time_mid              = (uint16_t)sValue[0];
   sServiceUUID.time_hi_and_version   = (uint16_t)sValue[1];
   sServiceUUID.clock_seq_and_node[0] = (uint8_t)sValue[2];
   sServiceUUID.clock_seq_and_node[1] = (uint8_t)sValue[3];
   sServiceUUID.clock_seq_and_node[2] = (uint8_t)sValue[4];
   sServiceUUID.clock_seq_and_node[3] = (uint8_t)sValue[5];
   sServiceUUID.clock_seq_and_node[4] = (uint8_t)sValue[6];
   sServiceUUID.clock_seq_and_node[5] = (uint8_t)sValue[7];
   sServiceUUID.clock_seq_and_node[6] = (uint8_t)sValue[8];
   sServiceUUID.clock_seq_and_node[7] = (uint8_t)sValue[9];

   /* Create the TEE Context */
   nError = TEEC_InitializeContext(NULL,  /* const char * name */
                                   &sContext);   /* TEEC_Context* context */
   if (nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "TEEC_InitializeContext returned error 0x%08X\n", nError);
      exit(nError);
   }

   /* Open the TEE Session */
   memset(&sOperation, 0, sizeof(TEEC_Operation));
   sOperation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
                                              TEEC_NONE,
                                              TEEC_NONE,
                                              TEEC_NONE);
   sOperation.params[0].value.a = 0x08;   /* SM_CONTROL_MODE_MANAGER */
   nError = TEEC_OpenSession(&sContext,
                             &sSession,               /* OUT session */
                             &service_manager_uuid,   /* destination UUID */
                             TEEC_LOGIN_PRIVILEGED,   /* connectionMethod */
                             NULL,                    /* connectionData */
                             &sOperation,             /* IN OUT operation */
                             NULL                     /* OUT errorOrigin, optional */
                             );

   if (nError != TEEC_SUCCESS)
   {
      fprintf(stderr, "TEEC_OpenSession returned error 0x%08X\n", nError);
      exit(nError);
   }

   /* Remove the service */
   memset(&sOperation, 0, sizeof(TEEC_Operation));

   sOperation.paramTypes = TEEC_PARAM_TYPES(
      TEEC_MEMREF_TEMP_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

   sOperation.params[0].tmpref.buffer = &sServiceUUID;
   sOperation.params[0].tmpref.size   = sizeof(sServiceUUID);

   nError = TEEC_InvokeCommand(&sSession,
                               SERVICE_MANAGER_COMMAND_ID_REMOVE_SERVICE,
                               &sOperation,     /* IN OUT operation */
                               NULL             /* OUT errorOrigin, optional */
                              );
   if ((nError != TEEC_SUCCESS))
   {
      fprintf(stderr, "TEEC_InvokeCommand(REMOVE_SERVICE) returned error 0x%08X\n", nError);
      exit(nError);
   }

   /* Close the TEE Session */
   TEEC_CloseSession(&sSession);

   /* Close the TEE Context */
   TEEC_FinalizeContext(&sContext);

   printf("SUCCESS\n");
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
   char* pCommand;
   if (argc < 2)
   {
      print_usage();
   }
   pCommand = argv[1];
   if (strcmp(pCommand, "list") == 0)
   {
      if (argc != 2)
      {
         print_usage();
      }
      execute_list();
   }
   else if (strcmp(pCommand, "download") == 0)
   {
      if (argc != 3)
      {
         print_usage();
      }
      execute_download(argv[2]);
   }
   else if (strcmp(pCommand, "remove") == 0)
   {
      if (argc != 3)
      {
         print_usage();
      }
      execute_remove(argv[2]);
   }
   else
   {
      print_usage();
   }
   return 0;
}

#ifdef LIBRARY_WITH_JNI

JNIEXPORT __attribute__((visibility("default"))) void JNICALL
Java_com_trustedlogic_android_tfctrl_TFCTRL_ListServices
  (JNIEnv *env, jobject thiz, jstring device, jstring external_storage)
{

	jboolean iscopy;
	char* c_data=NULL;
	const char * device_name=(*env)->GetStringUTFChars(env, device, &iscopy);

	const char * storage=(*env)->GetStringUTFChars(env, external_storage, &iscopy);

	int file1 = freopen (storage,"w",stdout); // redirect stdout (currently set to /dev/null) to the logging file
	int file2 = freopen (storage,"w",stderr); // redirect stderr (currently set to /dev/null) to the logging file

	execute_list();
	fclose (stdout);
	fclose (stderr);

	(*env)->ReleaseStringChars(env, device, device_name);
	(*env)->ReleaseStringChars(env, external_storage, storage);

}
#endif

