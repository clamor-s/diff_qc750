/**
 * Copyright (c) 2008-2010 Trusted Logic S.A.
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
 *
 */

#ifndef ___MTC_H_INC___
#define ___MTC_H_INC___

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
   includes
------------------------------------------------------------------------------*/
#include "s_type.h"
#include "s_error.h"

/* Define MTC_EXPORTS during the build of mtc libraries. Do
 * not define it in applications.
 */

#ifdef MTC_EXPORTS
#define MTC_EXPORT S_DLL_EXPORT
#else
#define MTC_EXPORT S_DLL_IMPORT
#endif

/*------------------------------------------------------------------------------
   typedefs
------------------------------------------------------------------------------*/

typedef struct
{
   uint32_t nLow;
   uint32_t nHigh;
}
S_MONOTONIC_COUNTER_VALUE;

/*------------------------------------------------------------------------------
   defines
------------------------------------------------------------------------------*/

#define S_MONOTONIC_COUNTER_GLOBAL        0x00000000

/*------------------------------------------------------------------------------
   API
------------------------------------------------------------------------------*/

S_RESULT MTC_EXPORT SMonotonicCounterInit(void);

void MTC_EXPORT SMonotonicCounterTerminate(void);

S_RESULT MTC_EXPORT SMonotonicCounterOpen(
                 uint32_t nCounterIdentifier,
                 S_HANDLE* phCounter);

void MTC_EXPORT SMonotonicCounterClose(S_HANDLE hCounter);

S_RESULT MTC_EXPORT SMonotonicCounterGet(
                 S_HANDLE hCounter,
                 S_MONOTONIC_COUNTER_VALUE* psCurrentValue);

S_RESULT MTC_EXPORT SMonotonicCounterIncrement(
                 S_HANDLE hCounter,
                 S_MONOTONIC_COUNTER_VALUE* psNewValue);

#ifdef __cplusplus
}
#endif

#endif /*___MTC_H_INC___*/
