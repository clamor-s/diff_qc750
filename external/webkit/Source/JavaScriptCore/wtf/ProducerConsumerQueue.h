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

#ifndef WTF_ProducerConsumerQueue_h
#define WTF_ProducerConsumerQueue_h

#include <algorithm>
#include <wtf/Futex.h>
#include <wtf/Noncopyable.h>

// Single producer, single consumer lockless queue.
namespace WTF {

// Overload this method to add producer-thread cleanup to
// ProducerConsumerQueue<> (like deleting pointers).
template<typename T> inline void cleanupQueueSlot(volatile const T& value)
{
}

template<unsigned N, unsigned Shift = 0> struct RoundUpToPowerOf2 {
    // We have to duplicate N as M to work around a compiler bug.
    template<unsigned M, unsigned X> struct GetValue {
        static const unsigned Value = RoundUpToPowerOf2<X, 1>::Value;
    };
    template<unsigned M> struct GetValue<M, 0> {
        static const unsigned Value = M << Shift;
    };
    static const unsigned Value = GetValue<N, N & (N - 1)>::Value;
};

// ProducerConsumerQueue<> is a lockless single-producer, single-consumer queue.
template<typename T, unsigned MinCapacity>
class ProducerConsumerQueue {
    WTF_MAKE_NONCOPYABLE(ProducerConsumerQueue);

    // Round Capacity up to the nearest power of 2. We do math modulo Capacity
    // to compute queue indices. But unsigned ints also wrap, so it's important
    // for Capacity to be a power of 2. That way when the unsigned ints wrap,
    // the value is still congruent modulo Capacity to what it would have been
    // without wrapping.
    static const unsigned Capacity = RoundUpToPowerOf2<MinCapacity>::Value;

public:
    ProducerConsumerQueue<T, MinCapacity>()
        : m_nextCleanupIndex(0)
    { }

    ~ProducerConsumerQueue<T, MinCapacity>()
    {
        ASSERT(m_front == m_back);
        cleanup();
    }

    // Producer thread only.
    void push(const T& value, unsigned wakeThreshold = 0)
    {
        // Wait until the consumer isn't using m_queue[m_back % Capacity] anymore.
        m_front.waitUntilNotEqual(m_back - Capacity);
        cleanup();

        m_queue[m_back % Capacity] = value;

        wakeThreshold = std::min(wakeThreshold, Capacity / 2);
        bool shouldWakeConsumer = (1 + m_back - m_front >= wakeThreshold);
        m_back.increment(shouldWakeConsumer);
    }

    // Producer thread only.
    inline void cleanup()
    {
        for (; m_nextCleanupIndex != m_front; m_nextCleanupIndex++)
            cleanupQueueSlot(m_queue[m_nextCleanupIndex % Capacity]);
    }

    // Consumer thread only.
    inline volatile const T& front()
    {
        // Wait until there's something on the queue.
        m_back.waitUntilNotEqual(m_front);
        return m_queue[m_front % Capacity];
    }

    // Consumer thread only.
    inline void pop()
    {
        // Wait until there's something on the queue.
        m_back.waitUntilNotEqual(m_front);

        // If the producer slept because the queue was full, don't wake it back
        // up until the queue half empty again.
        bool shouldWakeProducer = (m_back - m_front <= (1 + Capacity) / 2);
        m_front.increment(shouldWakeProducer);
    }

    class Index {
        WTF_MAKE_NONCOPYABLE(Index);
    public:
        Index()
            : m_index(0)
            , m_isWaiting(0)
        { }

        inline operator unsigned() { return m_index; }

        inline void increment(bool shouldWakeFutex = true)
        {
            atomicWrite(&m_index, 1 + m_index);
            if (shouldWakeFutex && atomicRead(&m_isWaiting))
                futexWakePrivate(&m_index, 1);
        }

        inline void waitUntilNotEqual(unsigned value)
        {
            if (atomicRead(&m_index) != value)
                return;

            atomicWrite(&m_isWaiting, 1);
            while (atomicRead(&m_index) == value) {
                // futexWaitPrivate() will atomically sleep this thread if m_index == value.
                futexWaitPrivate(&m_index, value);
            }

            m_isWaiting = 0;
        }

    private:
        // Values used for futexes and atomics must be aligned.
        unsigned volatile m_index __attribute__((aligned(sizeof(int))));
        int volatile m_isWaiting __attribute__((aligned(sizeof(int))));
    };

private:
    T volatile m_queue[Capacity];
    Index m_front;
    Index m_back;
    unsigned m_nextCleanupIndex;
};

} // namespace WTF

#endif
