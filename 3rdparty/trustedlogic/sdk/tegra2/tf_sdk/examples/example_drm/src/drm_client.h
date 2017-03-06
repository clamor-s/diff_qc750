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

#ifndef __DRM_CLIENT_H__
#define __DRM_CLIENT_H__

/* ===========================================================================
    Function prototypes
=========================================================================== */
/*
 * Function:
 * printUsage
 *
 * Description:
 * This function prints the usage instructions for the example application.
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
static void printUsage( void );

/*
 * Function:
 * parseCmdLine
 *
 * Description:
 * This function parses the command line given to the application.
 *
 * Inputs:
 *   nArgc - The number of arguements passed in the pArgV array.
 *   pArgv - Ptr to array of ptrs to input parameter strings.
 *
 * Outputs:
 *   ppLicensePath - The pointer pointed to by ppLicensePath will be updated
 *     to point to the value of the license file's path. If the license path
 *     is not specified on the command line, then the pointer will be set to
 *     NULL on return from this function.
 *   ppContentPath - The pointer pointed to by ppContentPath will be updated
 *     to point to the value of the content file's path. If the content path
 *     is not specified on the command line, then the pointer will be set to
 *     NULL on return from this function.
 *   pInit - The integer pointed to by this variable will be set to 1 if the
 *     command line specified "-init" mode, and will be 0 otherwise.
 *
 * Return Value:
 *    Returns 1 on successful parsing of the input.
 */
static int parseCmdLine(
   int    nArgc,
   char*  pArgv[],
   char** ppLicensePath,
   char** ppContentPath,
   int*   pInit);

/*
 * Function:
 * getFileContent
 *
 * Description:
 * This function loads the data from the specified file into a memory buffer.
 *
 * Inputs:
 *   pPath - Pointer to character string containing the file path.
 *
 * Outputs:
 *   ppContent - This function will allocate a buffer, and fill it with the
 *     content from the file. It is the responsibilty of the calling code to
 *     free the memory associated with the buffer when it is no longer
 *     required.
 *
 * Return Value:
 *    Returns the number of bytes of data stored in the buffer, or 0 on
 *    failure.
 */
static size_t getFileContent(
   char*           pPath,
   unsigned char** ppContent);

/*
 * Function:
 * decryptAndDisplayContent
 *
 * Description:
 * The passed license contains and key which is encrypted using the device
 * key. The passed content file contains some content that is encrypted, and
 * which can be decrypted using the key in the license file. This function
 * first decrypts the license key using the device key, and then uses the
 * decrypted license key to decrypt the content stream before displaying it
 * on the screen.
 *
 * This function requires that the device key is already installed on the host
 * platform. This can be achieved by passing the "-init" parameter on the
 * command line.
 *
 * Inputs:
 *   pLicensePath - Pointer to character string containing the license file
 *     path.
 *   pContentPath - Pointer to character string containing the content file
 *     path.
 *
 * Outputs:
 *   - - -
 *
 * Return Value:
 *    - - -
 */
static void decryptAndDisplayContent(
   char* pLicensePath,
   char* pContentPath
);

/*
 * Function:
 * init
 *
 * Description:
 * Installs the device key into the application specific key store. For the
 * purposes of this demo, this device key is specified in the
 * "example_crypto_init_data.h" header file. In a real deployment the
 * installation of the device specific key depends on the implementation, but
 * ideally it should be insterted into the device in a secure fashion during
 * manufacture.
 *
 * Inputs:
 *   - - -
 *
 * Outputs:
 *   - - -
 *
 * Return Value:
 *   1 on success, 0 otherwise.
 */
static int init( void );

#endif /* __DRM_CLIENT_H__ */

