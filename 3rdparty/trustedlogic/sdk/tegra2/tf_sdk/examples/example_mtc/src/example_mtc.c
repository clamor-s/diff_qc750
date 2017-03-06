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

/* ===========================================================================
    Includes
  =========================================================================== */

#include <stdio.h>

#include "mtc.h"
#include "s_error.h"

#if defined(__SYMBIAN32__)
extern void symbianPrintf(const char* format, ...);
#define printf symbianPrintf
#endif

#if defined (__SYMBIAN32__)
int __main__(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
   S_RESULT nError;
   S_HANDLE hCounter;
   S_MONOTONIC_COUNTER_VALUE sMC;

   nError = SMonotonicCounterInit();
   if (nError != S_SUCCESS)
   {
      printf("Unable to initialize the monotonic counter, error %X\n", nError);
      return nError;
   }

   nError = SMonotonicCounterOpen(S_MONOTONIC_COUNTER_GLOBAL, &hCounter);
   if (nError != S_SUCCESS)
   {
      printf("Unable to open the monotonic counter, error %X\n", nError);
      SMonotonicCounterTerminate();
      return nError;
   }

   nError = SMonotonicCounterGet(hCounter, &sMC);
   if (nError != S_SUCCESS)
   {
      printf("Unable to get the monotonic counter value, error %X\n", nError);
      goto do_end;
   }
   printf("Monotonic Counter Value is : %08X%08X\n", sMC.nHigh, sMC.nLow);

   nError = SMonotonicCounterIncrement(hCounter, &sMC);
   if (nError != S_SUCCESS)
   {
      printf("Unable to increment the monotonic counter value, error %X\n", nError);
      goto do_end;
   }
   printf("Monotonic Counter Value is : %08X%08X\n", sMC.nHigh, sMC.nLow);

do_end:
   SMonotonicCounterClose(hCounter);
   SMonotonicCounterTerminate();
   if (nError == S_SUCCESS)
   {
      printf("SUCCESS\n\n");
   }
   else
   {
      printf("FAILURE\n\n");
   }
   return nError;
}
