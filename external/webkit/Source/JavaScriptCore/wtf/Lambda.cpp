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

#include "config.h"
#include "Lambda.h"

#include "MainThread.h"

namespace WTF {

struct LambdaAllocation {
    static const size_t DataSize = sizeof(int) * 16;
    int data[DataSize / sizeof(int)];
};

class LambdaAllocator {
    static const int TotalAllocations = 8192;
public:
    inline static LambdaAllocator* instance()
    {
        if (!m_instance)
            m_instance = new LambdaAllocator;
        return m_instance;
    }

    inline LambdaAllocation* alloc()
    {
        if (!m_freeIndexCount)
            return 0;
        return m_freeAllocations[--m_freeIndexCount];
    }

    inline bool free(LambdaAllocation* allocation)
    {
        if (allocation < &m_allocations[0] || allocation >= &m_allocations[TotalAllocations])
            return false;

        m_freeAllocations[m_freeIndexCount++] = allocation;
        return true;
    }

private:
    LambdaAllocator()
    {
        for (int i = 0; i < TotalAllocations; i++)
            m_freeAllocations[i] = &m_allocations[i];
        m_freeIndexCount = TotalAllocations;
    }

    static LambdaAllocator* m_instance;
    LambdaAllocation m_allocations[TotalAllocations];
    LambdaAllocation* m_freeAllocations[TotalAllocations];
    int m_freeIndexCount;
};

LambdaAllocator* LambdaAllocator::m_instance = 0;

void* Lambda::operator new(size_t size)
{
    ASSERT(WTF::isMainThread());
    LambdaAllocation* allocation;
    if (size > LambdaAllocation::DataSize
        || !(allocation = LambdaAllocator::instance()->alloc()))
        return ::operator new(size);

    return &allocation->data[0];
}

void Lambda::operator delete(void* address)
{
    ASSERT(WTF::isMainThread());
    if (!LambdaAllocator::instance()->free(reinterpret_cast<LambdaAllocation*>(address)))
        return ::operator delete(address);
}

}
