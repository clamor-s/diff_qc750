/**
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

/**
 * Implementation of the mutex library over the Linux API
 **/
#include <pthread.h>
#include "lib_mutex.h"

void libMutexInit(LIB_MUTEX* pMutex)
{
   pthread_mutex_init(pMutex, NULL);
   /* Error ignored. Is that OK? */
}

void libMutexLock(LIB_MUTEX* pMutex)
{
   pthread_mutex_lock(pMutex);
}

void libMutexUnlock(LIB_MUTEX* pMutex)
{
   pthread_mutex_unlock(pMutex);
}

void libMutexDestroy(LIB_MUTEX* pMutex)
{
   pthread_mutex_destroy(pMutex);
}
