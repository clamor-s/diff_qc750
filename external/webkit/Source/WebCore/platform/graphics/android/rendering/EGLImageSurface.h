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

#ifndef EGLImageSurface_h
#define EGLImageSurface_h

#if USE(ACCELERATED_COMPOSITING)

#include "IntSize.h"
#include "EGLFence.h"
#include <GLES2/gl2.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Threading.h>
#include <wtf/ThreadSafeRefCounted.h>

class SkBitmap;

namespace WebCore {

class EGLImage;
class FPSTimer;

class EGLImageBuffer {
public:
    virtual ~EGLImageBuffer() {}

    virtual EGLImage* eglImage() const = 0;
    virtual bool canRetainTextures() const { return true; }
    virtual IntSize size() const = 0;
    virtual void deleteSourceBuffer() {}

    EGLFence& fence() { return m_fence; }
    void lockSurface() { m_mutex.lock(); }
    void unlockSurface() { m_mutex.unlock(); }

    virtual void mapBuffer(SkBitmap*, bool premultiplyAlpha);
    virtual void unmapBuffer() {}

private:
    WTF::Mutex m_mutex;
    EGLFence m_fence;
};

class EGLImageBufferRing : public ThreadSafeRefCounted<EGLImageBufferRing> {
public:
    EGLImageBufferRing();

    void surfaceDetached(EGLImageBuffer* currentBackBuffer);

    PassOwnPtr<EGLImageBuffer> takeBufferAndLock();
    void submitBufferAndUnlock(PassOwnPtr<EGLImageBuffer>);

    PassOwnPtr<EGLImageBuffer> takeFreeBuffer();
    void submitBuffer(PassOwnPtr<EGLImageBuffer>);

    GLuint beginReadingFrontBuffer(EGLImageBuffer** frontTextureBuffer);
    void didFinishReading(EGLImageBuffer* frontTextureBuffer);
    void referenceFrontTexture();
    void releaseFrontTexture();

    bool mapFrontBuffer(SkBitmap*, bool premultiplyAlpha);
    void unmapFrontBuffer();

private:
    WTF::Mutex m_mutex;
    OwnPtr<EGLImageBuffer> m_freeBuffer;
    OwnPtr<EGLImageBuffer> m_frontBuffer;
    GLuint m_frontTexture;
    volatile int m_frontTextureRefCount;
};

class EGLImageSurface : public RefCounted<EGLImageSurface> {
public:
    EGLImageSurface()
        : m_bufferRing(adoptRef(new EGLImageBufferRing()))
    {}
    virtual ~EGLImageSurface() {}

    RefPtr<EGLImageBufferRing> bufferRing() const { return m_bufferRing; }

    virtual bool isInverted() const { return false; }
    virtual bool hasAlpha() const { return true; }
    virtual bool hasPremultipliedAlpha() const { return true; }
    virtual bool requiresFenceWaitAfterPainting() const { return false; }
    virtual PassOwnPtr<EGLImageBuffer> commitBackBuffer() { return 0; }
    virtual void syncContents() {}

private:
    RefPtr<EGLImageBufferRing> m_bufferRing;
};

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)

#endif // EGLImageSurface_h
