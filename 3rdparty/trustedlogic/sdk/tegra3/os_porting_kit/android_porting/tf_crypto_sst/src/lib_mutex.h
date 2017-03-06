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

#ifndef __LIB_MUTEX_H__
#define __LIB_MUTEX_H__

/**
 * This library defines a type of simple non-recursive mutex that can 
 * be statically initialized. The interface is very similar to pthread
 * mutexes.
 **/

#ifdef WIN32

#include <windows.h>
#include "s_type.h"

/* Windows API: use a critical section with a state describing 
   whether it has been initialized or not or is being initialized.
   We use interlocked operations on the state to synchronize between
   multiple threads competing to initialize the critical section */
typedef struct
{
   volatile uint32_t nInitState;
   CRITICAL_SECTION sCriticalSection;
}
LIB_MUTEX;

#define LIB_MUTEX_WIN32_STATE_UNINITIALIZED 0
#define LIB_MUTEX_WIN32_STATE_INITIALIZING  1
#define LIB_MUTEX_WIN32_STATE_INITIALIZED   2

#define LIB_MUTEX_INITIALIZER { 0 }

#endif  /* WIN32 */

#ifdef LINUX

#include <pthread.h>
#include "s_type.h"

/* Linux: directly use a pthread mutex. */
typedef pthread_mutex_t LIB_MUTEX;
#define LIB_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

#endif /* LINUX */

/**
 * Initialize a mutex. Note that thie function cannot fail. If there
 * is not enough resource to initialize the mutex, the implementation
 * resort to active-wait 
 **/
void libMutexInit(LIB_MUTEX* pMutex);

/**
 * Lock the mutex 
 */
void libMutexLock(LIB_MUTEX* pMutex);

/**
 * Unlock the mutex
 */
void libMutexUnlock(LIB_MUTEX* pMutex);

/**
 * Destroy the mutex
 */
void libMutexDestroy(LIB_MUTEX* pMutex);

#endif /* __LIB_MUTEX_H__ */
