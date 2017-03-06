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
 * This is the implementation of the smx_os.h interface for the
 * Linux OS family
 */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <linux/limits.h>

#include "smx_utils.h"
#include "smx_os.h"
#include "s_error.h"

/*----------------------------------------------------------------------------
 * Critical sections
 *----------------------------------------------------------------------------*/
/* 
 * Implementation of the critical section operations for Linux 
 *
 * We use a mutex to represent a critical section
 */
/* 
 * Implementation of the critical section operations for Linux 
 *
 * We use a mutex for each critical section
 */
SM_ERROR SMXInitCriticalSection(SMX_CRITICAL_SECTION* pCriticalSection)
{
   SM_ERROR nError=SM_SUCCESS;
   pthread_mutexattr_t  mtxAttr;

   assert(pCriticalSection!=NULL);

   if (pthread_mutexattr_init(&mtxAttr) != 0)
   {
      return S_ERROR_UNDERLYING_OS;
   }

   if (pthread_mutexattr_settype(&mtxAttr, PTHREAD_MUTEX_RECURSIVE) == 0)
   {
      nError = (pthread_mutex_init(pCriticalSection, &mtxAttr) == 0 ?
                  S_SUCCESS : S_ERROR_UNDERLYING_OS);
   }
   else
   {
      nError = S_ERROR_UNDERLYING_OS;
   }

   pthread_mutexattr_destroy(&mtxAttr);

   return nError;
}

void SMXTerminateCriticalSection(SMX_CRITICAL_SECTION* pCriticalSection)
{
   /* Note that pthread_mutex_destroy may fail only when the 
      critical section is not in the correct state, which can never
      happen */
   pthread_mutex_destroy(pCriticalSection);
}

SM_ERROR SMXLockCriticalSection(SMX_CRITICAL_SECTION* pCriticalSection)
{
   if (pthread_mutex_lock(pCriticalSection) == 0)
   {
      return SM_SUCCESS;
   }
   else
   {
      return S_ERROR_UNDERLYING_OS;
   }
}

void SMXUnLockCriticalSection(SMX_CRITICAL_SECTION* pCriticalSection)
{
   /* Note that pthread_mutex_destroy may fail only when the 
      critical section is not in the correct state, which can never
      happen */
   pthread_mutex_unlock(pCriticalSection);
}

/*----------------------------------------------------------------------------
 * Time
 *----------------------------------------------------------------------------*/
void SMXMilliSecondSleep(uint32_t nMilliSeconds)
{
   struct timespec time;
   time.tv_sec = nMilliSeconds / 1000;
   time.tv_nsec = (nMilliSeconds % 1000) * 1000000;
   while (nanosleep(&time, &time) == -1) continue;
}

uint64_t SMXGetCurrentTime(void)
{
   uint64_t currentTime=0;
   struct timeval now;

   gettimeofday(&now,NULL);
   currentTime = now.tv_sec;
   currentTime = (currentTime * 1000) + (now.tv_usec / 1000);

   return currentTime;
}

/*----------------------------------------------------------------------------
 * Signature files
 *----------------------------------------------------------------------------*/
S_RESULT SMXGetSignatureFile(
                 uint8_t** ppSignature,
                 uint32_t*  pnSignatureSize)
{
   S_RESULT nErrorCode = S_SUCCESS;

   uint32_t      nBytesRead;
   uint32_t      nSignatureSize = 0;
   uint8_t*     pSignature = NULL;
   FILE*    pSignatureFile = NULL;
   char     sFileName[PATH_MAX + 1 + 5];  /* Allocate room for the signature extension */
   long     nFileSize;

   *pnSignatureSize = 0;
   *ppSignature = NULL;

   if (realpath("/proc/self/exe", sFileName) == NULL)
   {
      TRACE_ERROR("SMXGetSignatureFile: realpath failed [%d]", errno);
      return S_ERROR_UNDERLYING_OS;
   }

   /* Work out the signature file name */
   strcat(sFileName, ".ssig");

   pSignatureFile = fopen(sFileName, "rb");
   if (pSignatureFile == NULL)
   {
      /* Signature doesn't exist - Fair enough ! */
      return S_SUCCESS;
   }

   if (fseek(pSignatureFile, 0, SEEK_END) != 0)
   {
      TRACE_ERROR("SMXGetSignatureFile: fseek(%s) failed [%d]",
                     sFileName, errno);
      nErrorCode = S_ERROR_UNDERLYING_OS;
      goto error;
   }

   nFileSize = ftell(pSignatureFile);
   if (nFileSize < 0)
   {
      TRACE_ERROR("SMXGetSignatureFile: ftell(%s) failed [%d]",
                     sFileName, errno);
      nErrorCode = S_ERROR_UNDERLYING_OS;
      goto error;
   }

   nSignatureSize = (uint32_t)nFileSize;

   if (nSignatureSize == 0)
   {
      TRACE_ERROR("SMXGetSignatureFile: Empty signature file");
      nErrorCode = S_ERROR_NO_DATA;
      goto error;
   }

   pSignature = malloc(nSignatureSize);
   if (pSignature == NULL)
   {
      TRACE_ERROR("SMXGetSignatureFile: Heap - Out of memory for %u bytes",
                     nSignatureSize);
      nErrorCode = S_ERROR_OUT_OF_MEMORY;
      goto error;
   }

   rewind(pSignatureFile);

   nBytesRead = fread(pSignature, 1, nSignatureSize, pSignatureFile);
   if (nBytesRead < nSignatureSize)
   {
      TRACE_ERROR("SMXGetSignatureFile: fread failed [%d]", errno);
      nErrorCode = S_ERROR_UNDERLYING_OS;
      goto error;
   }

   fclose(pSignatureFile);

   *pnSignatureSize = nSignatureSize;
   *ppSignature = pSignature;

   return S_SUCCESS;

error:
   fclose(pSignatureFile);
   free(pSignature);

   return nErrorCode;
}
