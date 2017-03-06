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

#ifndef WTF_DelegateThread_h
#define WTF_DelegateThread_h

#include <wtf/Closure.h>
#include <wtf/FutexSingleEvent.h>
#include <wtf/Lambda.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/ProducerConsumerQueue.h>
#include <wtf/Threading.h>

namespace WTF {

// Make ProducerConsumerQueue<> delete Lambda*'s on cleanup.
inline void cleanupQueueSlot(Lambda* value)
{
    if (value)
        delete value;
}

// A class for delegating function calls from any number of threads
// to be executed (in order) on a separate single worker thread;
// for parallel processing, use parallel.h instead
//
// example usage:
//
//     DelegateThread thread;
//     thread.callLater(makeLambda(glBindBuffer)(GL_ARRAY_BUFFER, buf));
//     thread.callLater(makeLambda(glDrawArrays)(GL_TRIANGLES, 0, 3));
//     renderer = thread.call(makeLambda(glGetString)(GL_RENDERER));

template<unsigned MinQueueCapacity>
class DelegateThread {
public:
    DelegateThread(const char* name = "")
        : m_thread(createThread(threadLoop, &m_queue, name))
    { }

    ~DelegateThread()
    {
        m_queue.push(0);
        waitForThreadCompletion(m_thread, 0);
        detachThread(m_thread);
    }

    ThreadIdentifier id() const { return m_thread; }

    // Issue an asynchronous function call.
    void callLater(PassOwnPtr<Lambda> operation, unsigned wakeThreshold = 0)
    {
        m_queue.push(operation.leakPtr(), wakeThreshold);
    }

    // Issue a synchronous function call and send back the return value.
    template<typename T> T call(PassOwnPtr<ReturnLambda<T> > operation)
    {
        FutexSingleEvent functionCall;
        callLater(WTF::makeLambda(callWorker)(*operation.get(), functionCall));
        functionCall.wait();
        m_queue.cleanup();
        return operation->ret();
    }

    // Issue a synchronous function call.
    void call(PassOwnPtr<Lambda> operation)
    {
        FutexSingleEvent functionCall;
        callLater(WTF::makeLambda(callWorker)(*operation.get(), functionCall));
        functionCall.wait();
        m_queue.cleanup();
    }

    // Wait for all queued work to finish.
    void finish()
    {
        FutexSingleEvent functionCall;
        callLater(WTF::makeLambda(finishWorker)(functionCall));
        functionCall.wait();
        m_queue.cleanup();
    }

private:
    static void callWorker(Lambda& operation, FutexSingleEvent& functionCall)
    {
        operation.call();
        functionCall.trigger();
    }

    static void finishWorker(FutexSingleEvent& functionCall)
    {
        functionCall.trigger();
    }

    static void* threadLoop(void* param)
    {
        ProducerConsumerQueue<Lambda*, MinQueueCapacity>* queue =
            reinterpret_cast<ProducerConsumerQueue<Lambda*, MinQueueCapacity>*>(param);

        while (Lambda* operation = queue->front()) {
            operation->call();
            queue->pop();
        }
        queue->pop();

        return 0;
    }

    ThreadIdentifier m_thread;
    ProducerConsumerQueue<Lambda*, MinQueueCapacity> m_queue;
};

} // namespace WTF

#endif
