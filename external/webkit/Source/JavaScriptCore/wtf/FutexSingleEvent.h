/*
 * Copyright (C) 2011, 2012, NVIDIA CORPORATION
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *    * Neither the name of NVIDIA CORPORATION nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.    IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WTF_FutexSingleEvent_h
#define WTF_FutexSingleEvent_h

#include <wtf/Atomics.h>
#include <wtf/FastAllocBase.h>
#include <wtf/Futex.h>
#include <wtf/Noncopyable.h>

namespace WTF {

// FutexSingleEvent allows a single thread to wait for a one-time event to
// occur. It uses futexes so it's very light-weight to create and use.
class FutexSingleEvent {
    WTF_MAKE_NONCOPYABLE(FutexSingleEvent);
public:
    FutexSingleEvent()
        : m_hasHappened(0)
        , m_isWaiting(0)
    { }

    bool hasHappened() const { return m_hasHappened; }

    void wait()
    {
        if (atomicRead(&m_hasHappened))
            return;

        atomicWrite(&m_isWaiting, 1);
        while (!atomicRead(&m_hasHappened))
            futexWaitPrivate(&m_hasHappened, 0);
    }

    void trigger()
    {
        atomicWrite(&m_hasHappened, 1);
        if (atomicRead(&m_isWaiting))
            futexWakePrivate(&m_hasHappened, 1);
    }

private:
    // Values used for futexes and atomics must be aligned.
    int volatile m_hasHappened __attribute__((aligned(sizeof(int))));
    int volatile m_isWaiting __attribute__((aligned(sizeof(int))));
};

} // namespace WTF

#endif
