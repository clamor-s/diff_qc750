/*
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
/*
 * This header file defines the interface between the portable implementation
 * of the SMAPI library and the OS API. When porting to a new OS, you must
 * provide an implementation of the functions defined in this API.
 * See also the file schannel_client.h for a specification of the SChannel
 * OS-dependent functions that you must also implement
 */

#ifndef   __SMAPI_OS_H__
#define   __SMAPI_OS_H__

#include "smapi.h"

/*----------------------------------------------------------------------------
 * Critical sections
 *----------------------------------------------------------------------------*/
#if defined (LINUX)
#include <pthread.h>
#include <semaphore.h>
typedef pthread_mutex_t SMX_CRITICAL_SECTION;
#endif

#ifdef WIN32
#include <signal.h>
#include <windows.h>
typedef CRITICAL_SECTION SMX_CRITICAL_SECTION;
#endif

/*
 * When porting to a different OS, you must define the type
 * SMX_CRITICAL_SECTION and implement the following functions
 */
SM_ERROR SMXInitCriticalSection(SMX_CRITICAL_SECTION* pCriticalSection);
void SMXTerminateCriticalSection(SMX_CRITICAL_SECTION* pCriticalSection);
SM_ERROR SMXLockCriticalSection(SMX_CRITICAL_SECTION* pCriticalSection);
void SMXUnLockCriticalSection(SMX_CRITICAL_SECTION* pCriticalSection);

/*----------------------------------------------------------------------------
 * Assertions
 *----------------------------------------------------------------------------*/
#if defined (ANDROID)
#include <signal.h>
#include <unistd.h>
#define SMAPI_ASSERT() kill(getpid(),SIGKILL)
#elif defined (LINUX)
#include <signal.h>
#include <unistd.h>
#define SMAPI_ASSERT() kill(getpid(),SIGSEGV)
#endif

#ifdef WIN32
#define SMAPI_ASSERT() raise(SIGSEGV)
// the following code displays a popup which can be useful in debug
//#define SMAPI_ASSERT() (_wassert(_CRT_WIDE("SMAPI Assertion"), _CRT_WIDE(__FILE__), __LINE__), 0)
#endif


/* The macro SMAPI_ASSERT() must terminate the current process */

/*----------------------------------------------------------------------------
 * Time
 *----------------------------------------------------------------------------*/
void SMXMilliSecondSleep(uint32_t nMilliSeconds);

uint64_t SMXGetCurrentTime(void);

/*----------------------------------------------------------------------------
 * Signature files
 *----------------------------------------------------------------------------*/
/**
 * This function must return the content of the signature file
 * associated with the current process. This is typically
 * a file located in the same directory as the current
 * executable and whose name is the name of the executable
 * with the ".ssig" extension appended.
 * You may choose a different way to find the signature file
 * depending on your OS
 *
 * @param ppSignature on success, filled with a pointer to the content
 *    of the signature file. This pointer must be freed by the caller
 *
 * @param pnSignatureSize on success, filled with the length in bytes
 *    of the signature file
 *
 * @return S_SUCCESS or an error code
 **/

S_RESULT SMXGetSignatureFile(
                 OUT uint8_t** ppSignature,
                 OUT uint32_t*  pnSignatureSize);


#endif /* __SMAPI_OS_H__ */
