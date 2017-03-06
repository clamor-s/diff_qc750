/*
 * Copyright (C) 2011, 2012, NVIDIA CORPORATION
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WTF_Futex_h
#define WTF_Futex_h

#include <cutils/atomic.h>
#include <errno.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/time.h>

#ifndef FUTEX_PRIVATE_FLAG
#define FUTEX_PRIVATE_FLAG  128
#define FUTEX_WAIT_PRIVATE  (FUTEX_WAIT | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_PRIVATE  (FUTEX_WAKE | FUTEX_PRIVATE_FLAG)
#endif

namespace WTF {

inline int futex(void volatile* uaddr, int op, int val, timespec* timeout, void volatile* uaddr2, int val3)
{
    return syscall(__NR_futex, const_cast<void*>(uaddr), op, val, timeout, const_cast<void*>(uaddr2), val3);
}

// Cross-process, slower futex calls.
inline int futexWake(volatile void *ftx, int val)
{
    return futex(ftx, FUTEX_WAKE, val, 0, 0, 0);
}

inline int futexWait(volatile void *ftx, int val, const timespec* timeout = 0)
{
    return futex(ftx, FUTEX_WAIT, val, const_cast<timespec*>(timeout), 0, 0);
}

// Single-process, faster futex calls.
inline int futexWakePrivate(volatile void *ftx, int val)
{
    return futex(ftx, FUTEX_WAKE_PRIVATE, val, 0, 0, 0);
}

inline int futexWaitPrivate(volatile void *ftx, int val, const timespec* timeout = 0)
{
    return futex(ftx, FUTEX_WAIT_PRIVATE, val, const_cast<timespec*>(timeout), 0, 0);
}

// Atomics for dealing with futex variables.
template<typename T> inline void atomicWrite(volatile T* dest, T value)
{
    COMPILE_ASSERT(sizeof(T) == sizeof(int), wrong_size_for_atomic_value);
    android_atomic_release_store(static_cast<int>(value), reinterpret_cast<volatile int*>(dest));
}

template<typename T> inline T atomicRead(volatile T* source)
{
    COMPILE_ASSERT(sizeof(T) == sizeof(int), wrong_size_for_atomic_value);
    return static_cast<T>(android_atomic_acquire_load(reinterpret_cast<volatile int*>(source)));
}

} // namespace WTF

#endif
