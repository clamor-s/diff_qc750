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

 
/*
* File            : example_sst.c
*
* Author          : Trusted Logic S.A.
*
* Created         : November 28, 2005
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sst.h"

#ifdef LIBRARY_WITH_JNI
#include <jni.h>
#endif


#if defined(__SYMBIAN32__)
extern void symbianPrintf(const char* format, ...);
#define printf symbianPrintf
#endif


/* The 32 byte content test. */
static const uint8_t example1Data[32] =
{
     0x30, 0x82, 0x20, 0xc1, 0x30, 0x82, 0x10, 0xa9,
     0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x10, 0x00,
     0x30, 0xd0, 0x62, 0x90, 0x2a, 0x86, 0x48, 0x86,
     0xf7, 0x0d, 0x01, 0x01, 0x05, 0x05, 0x00, 0x30
};

/* The 16 byte content test. */
static const uint8_t example2Data[16] =
{
   0x30, 0x82, 0x04, 0xbe, 0x02, 0x01, 0x00, 0x30,
   0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7
};

/*
 * Function:
 * printFinalStatus
 *
 * Description:
 * This function print the final status of the example: SUCCESS/FAILURE.
 *
 * Inputs:
 *   pHasErrored - A boolean indicating if there has been errors or not.
 *
 * Return Value:
 *    0 if bHasErrored is set to true, -1 otherwise
 */
static int printFinalStatus(bool bHasErrored)
{
   if (bHasErrored)
   {
      printf("\nExample Summary : FAILURE\n\n");
      return -1;
   }

   printf("\nExample Summary : SUCCESS\n\n");
   return 0;
}

/*
 * Function:
 * checkAndPrintError
 *
 * Description:
 * This function prints a human readable string associated with errorCode.
 * This may include SST_SUCCESS - not just errors.
 *
 * Inputs:
 *   errorCode - The error code to parse.
 *
 * Outputs:
 *    pHasErrored - The value pointed to will not be modified if errorCode ==
 *      SST_SUCCESS, but will be set to true if any other errorCode occurs.
 *
 * Return Value:
 *    - - -
 */
static void checkAndPrintError(SST_ERROR errorCode, bool* pHasErrored)
{
    /* If an error has occured, then set the sticky hasErrored flag to true */
    if (errorCode != SST_SUCCESS)
    {
        *pHasErrored = true;
        printf("Error: ");
    }

    /* Print error code, or success message */
    switch(errorCode)
    {
    case SST_SUCCESS:
        printf("SST_SUCCESS\n\n");
        break;
    case SST_ERROR_ACCESS_CONFLICT:
        printf("SST_ERROR_ACCESS_CONFLICT\n\n");
        break;
    case SST_ERROR_ACCESS_DENIED:
        printf("SST_ERROR_ACCESS_DENIED\n\n");
        break;
    case SST_ERROR_BAD_PARAMETERS:
        printf("SST_ERROR_BAD_PARAMETERS\n\n");
        break;
    case SST_ERROR_CORRUPTED:
        printf("SST_ERROR_CORRUPTED\n\n");
        break;
    case SST_ERROR_ITEM_NOT_FOUND:
        printf("SST_ERROR_ITEM_NOT_FOUND\n\n");
        break;
    case SST_ERROR_GENERIC:
        printf("SST_ERROR_GENERIC\n\n");
        break;
    case SST_ERROR_NO_SPACE:
        printf("SST_ERROR_NO_SPACE\n\n");
        break;
    case SST_ERROR_OUT_OF_MEMORY:
        printf("SST_ERROR_OUT_OF_MEMORY\n\n");
        break;
    case SST_ERROR_OVERFLOW:
        printf("SST_ERROR_OVERFLOW\n\n");
        break;
    default:
       printf("UNKNOWN SST ERROR: 0x%08X\n\n", errorCode);
        break;
    }
}

/*
 * Function:
 * checkAndPrintErrorFatal
 *
 * Description:
 * Same as checkAndPrintError but, in case of an error, exit the 
 * example application. This is used when an error code prevents
 * subsequent steps in the example to perform properly
 */
static void checkAndPrintErrorFatal(SST_ERROR errorCode, bool* pHasErrored)
{
   checkAndPrintError(errorCode, pHasErrored);
#ifndef LIBRARY_WITH_JNI
   if (errorCode != SST_SUCCESS)
   {
      printf("Fatal error: exit the application\n");
      exit(-2);
   }
#endif
}


/*
 * Function:
 * main
 *
 * Description:
 * The main entry point into the example application.
 *
 * Inputs:
 *   argc - The number of command line parameters (unused).
 *   argv - The values of the command line parameters (unused).
 *
 * Outputs:
 *    - - -
 *
 * Return Value:
 *    int - Return code for this application (zero for success, non-zero
 *      otherwize).
 */
#if defined (__SYMBIAN32__)
int __main__(int argc, char* argv[])
#elif defined (LIBRARY_WITH_JNI)
int runSST()
#else
int main(int argc, char* argv[])
#endif
{
    /* The error code for the SST API */
    SST_ERROR       errorCode;
    /* The file handle for the file in the secure store */
   SST_HANDLE hFile = SST_NULL_HANDLE;
   /* A temporary buffer for read back from file tests */
   uint8_t         sTestBuffer[32];
    /* number of bytes read */
    uint32_t        numBytes;
    /* File pointer position */
    uint32_t        filePtrPos;
    /* File size */
    uint32_t        fileSize = 0;
    /* The file name */
    char* filename = "myFile";

    /* Sticky flag for indicating errors */
    bool hasErrored = false;
    /* Have we reached End-Of-File */
    bool isEOF = false;

#ifndef LIBRARY_WITH_JNI
    /* remove compiler warnings about argc and argv not used */
    do{(void)(argc);}while(0);
    do{(void)(argv);}while(0);
#endif

    printf("Secure Storage Service Example Application\n");
   printf("Copyright (c) 2005-2009 Trusted Logic S.A.\n\n");

    /* Initialize the SST service, and print status message */
    printf("SST: Initialize the service :                       ");
    errorCode = SSTInit();
   checkAndPrintErrorFatal(errorCode, &hasErrored); /* If SSTInit fails, we cannot continue */

    /* Create a file in the SST, and print status message */
    printf("SST: Create a file :                                ");
    errorCode = SSTOpen(filename, SST_O_READ | SST_O_WRITE | SST_O_WRITE_META | SST_O_CREATE, 0, &hFile);
   checkAndPrintErrorFatal(errorCode, &hasErrored);

    /* Write some data into "testFile", and print status message */
    printf("SST: Write first data block :                       ");
   errorCode = SSTWrite(hFile, example1Data, sizeof(example1Data));
    checkAndPrintError(errorCode, &hasErrored);

    /* Write some more data into "testFile", and print status message */
    printf("SST: Write second data block :                      ");
   errorCode = SSTWrite(hFile, example2Data, sizeof(example2Data));
    checkAndPrintError(errorCode, &hasErrored);

    /* Check for the end-of-file, and print status */
    printf("SST: Checking the EOF :                             ");
    errorCode = SSTEof(hFile, &isEOF);
    checkAndPrintError(errorCode, &hasErrored);

    if (!isEOF)
    {
      hasErrored = true;
      printf("  Error: The file offset is not at the EOF.\n\n");
    }

    /* Seek to start of file, and print status */
    printf("SST: Seeking to the start of the file :             ");
    errorCode = SSTSeek(hFile, 0, SST_SEEK_SET);
    checkAndPrintError(errorCode, &hasErrored);

    /* Read back and check integrity of the first data block */
    printf("SST: Read back first data block :                   ");

    /* Read back data, and print status */
   errorCode = SSTRead(hFile, (uint8_t*)&sTestBuffer, sizeof(example1Data), &numBytes);
    checkAndPrintError(errorCode, &hasErrored);

    /* Check we read back enough bytes */
   if (numBytes != sizeof(example1Data))
    {
      hasErrored = true;
      printf("  Error: Not all data could be read: [%d bytes read]\n\n", numBytes);
    }

    /* Check what we read back matched what we put in */
   if (memcmp(sTestBuffer, example1Data, numBytes) != 0)
    {
      hasErrored = true;
      printf("  Error: Retrieved data does not match what was stored.\n\n");
    }

    /* Seek to start of second data block, and print status */
    printf("SST: Seek to the start of second data block :       ");
   errorCode = SSTSeek(hFile, sizeof(example1Data), SST_SEEK_SET);
    checkAndPrintError(errorCode, &hasErrored);

    /* Read the second data block */
    printf("SST: Read back second data block :                  ");

    /* Read back data and print status */
   errorCode = SSTRead(hFile, (uint8_t*)&sTestBuffer, sizeof(example2Data), &numBytes);
    checkAndPrintError(errorCode, &hasErrored);

    /* Check we read back enough bytes */
   if (numBytes != sizeof(example2Data))
    {
      hasErrored = true;
      printf("  Error: Not all data could be read: [%d bytes read]\n\n", numBytes);
    }

    /* Check what we read back matched what we put in */
   if (memcmp(sTestBuffer, example2Data, numBytes) != 0)
    {
      hasErrored = true;
      printf("  Error: Retrieved data does not match what was stored.\n\n");
    }

    /* Truncate the data file, removing the second entry, and print status */
    printf("SST: Remove second data block by truncating :       ");
   errorCode = SSTTruncate(hFile, sizeof(example1Data));
    checkAndPrintError(errorCode, &hasErrored);

    /* Get the file pointer, and print status (the file pointer position is not
     * modified by a truncate operation).
     */
    printf("SST: Get file pointer position :                    ");
    errorCode = SSTTell(hFile, &filePtrPos);
    checkAndPrintError(errorCode, &hasErrored);

   if (filePtrPos != sizeof(example1Data) + sizeof(example2Data))
    {
        hasErrored = true;
      printf("  Error: Invalid file pointer position [%d].\n\n", filePtrPos);
    }

    /* Get the file size, and print status */
    printf("SST: Get file size :                                ");
    errorCode = SSTGetSize(filename, &fileSize);
    checkAndPrintError(errorCode, &hasErrored);

   if (fileSize != sizeof(example1Data))
    {
        hasErrored = true;
      printf("  Error: Reported size is incorrect [%d].\n\n", fileSize);
    }

    /* Remove the file from the SST , and print status */
    printf("SST: Remove the file :                              ");
    errorCode = SSTCloseAndDelete(hFile);
    checkAndPrintError(errorCode, &hasErrored);

    /* Terminate the service session, and print status */
    printf("SST: Terminate the service :                        ");
    errorCode = SSTTerminate();
    checkAndPrintError(errorCode, &hasErrored);

   return printFinalStatus(hasErrored);
}

#ifdef LIBRARY_WITH_JNI

JNIEXPORT __attribute__((visibility("default"))) void JNICALL
Java_com_trustedlogic_android_examples_SST_Run
  (JNIEnv *env, jobject thiz, jstring device, jstring external_storage)
{

	jboolean iscopy;
	char* c_data=NULL;
	const char * device_name=(*env)->GetStringUTFChars(env, device, &iscopy);

	const char * storage=(*env)->GetStringUTFChars(env, external_storage, &iscopy);

	int file1 = freopen (storage,"w",stdout); // redirect stdout (currently set to /dev/null) to the logging file
	int file2 = freopen (storage,"w",stderr); // redirect stderr (currently set to /dev/null) to the logging file

	int result = runSST();
	fclose (stdout);
	fclose (stderr);

	(*env)->ReleaseStringChars(env, device, device_name);
	(*env)->ReleaseStringChars(env, external_storage, storage);

}
#endif