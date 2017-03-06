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
#include "EGLImageSurface.h"

#if USE(ACCELERATED_COMPOSITING)

#include "EGLImage.h"
#include "GLUtils.h"

namespace WebCore {

void EGLImageBuffer::mapBuffer(SkBitmap* bitmap, bool premultiplyAlpha)
{
    // The compositor GL context is still active during the SW draw path.
    int oldFramebufferBinding;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFramebufferBinding);

    bitmap->setConfig(SkBitmap::kARGB_8888_Config, size().width(), size().height(), 4 * size().width());
    bitmap->allocPixels();

    GLuint framebufferID;
    glGenFramebuffers(1, &framebufferID);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferID);
    GLuint textureID = eglImage()->createTexture();

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureID, 0);
    GLUtils::AlphaOp alphaOp = premultiplyAlpha ? GLUtils::AlphaDoPremultiply : GLUtils::AlphaDoNothing;
    GLUtils::readPixels(0, 0, size().width(), size().height(), bitmap->getPixels(), GLUtils::BottomToTop, alphaOp);

    glBindFramebuffer(GL_FRAMEBUFFER, oldFramebufferBinding);
    glDeleteFramebuffers(1, &framebufferID);
    glDeleteTextures(1, &textureID);
}


EGLImageBufferRing::EGLImageBufferRing()
    : m_frontTexture(0)
    , m_frontTextureRefCount(0)
{}

void EGLImageBufferRing::surfaceDetached(EGLImageBuffer* currentBackBuffer)
{
    MutexLocker lock(m_mutex);
    if (m_freeBuffer)
        m_freeBuffer->deleteSourceBuffer();
    if (m_frontBuffer)
        m_frontBuffer->deleteSourceBuffer();
    if (currentBackBuffer)
        currentBackBuffer->deleteSourceBuffer();
}

PassOwnPtr<EGLImageBuffer> EGLImageBufferRing::takeBufferAndLock()
{
    m_mutex.lock();
    return m_frontBuffer.release();
}

void EGLImageBufferRing::submitBufferAndUnlock(PassOwnPtr<EGLImageBuffer> buffer)
{
    ASSERT(!m_frontBuffer);
    m_frontBuffer = buffer;
    m_mutex.unlock();
}

PassOwnPtr<EGLImageBuffer> EGLImageBufferRing::takeFreeBuffer()
{
    MutexLocker lock(m_mutex);
    return m_freeBuffer.release();
}

void EGLImageBufferRing::submitBuffer(PassOwnPtr<EGLImageBuffer> buffer)
{
    MutexLocker lock(m_mutex);
    ASSERT(!m_freeBuffer);
    if (m_frontBuffer)
        m_freeBuffer = m_frontBuffer.release();
    m_frontBuffer = buffer;
}

GLuint EGLImageBufferRing::beginReadingFrontBuffer(EGLImageBuffer** outFrontTextureBuffer)
{
    EGLImageBuffer* frontTextureBuffer;
    {
        MutexLocker lock(m_mutex);
        if (!m_frontBuffer)
            return 0;
        frontTextureBuffer = m_frontBuffer.get();
        frontTextureBuffer->lockSurface();
    }

    if (!m_frontTexture) {
        m_frontTexture = frontTextureBuffer->eglImage()->createTexture();
        if (!m_frontTexture) {
            frontTextureBuffer->unlockSurface();
            return 0;
        }
    } else
        frontTextureBuffer->eglImage()->writeToTexture2D(m_frontTexture);

    *outFrontTextureBuffer = frontTextureBuffer;
    return m_frontTexture;
}

void EGLImageBufferRing::didFinishReading(EGLImageBuffer* frontTextureBuffer)
{
    ASSERT(frontTextureBuffer);
    if (!frontTextureBuffer->canRetainTextures()) {
        glDeleteTextures(1, &m_frontTexture);
        m_frontTexture = 0;
    }
    frontTextureBuffer->fence().set();
    frontTextureBuffer->unlockSurface();
}

void EGLImageBufferRing::referenceFrontTexture()
{
    WTF::atomicIncrement(&m_frontTextureRefCount);
}

void EGLImageBufferRing::releaseFrontTexture()
{
    if (WTF::atomicDecrement(&m_frontTextureRefCount)
        || !m_frontTexture)
        return;

    glDeleteTextures(1, &m_frontTexture);
    m_frontTexture = 0;
}

bool EGLImageBufferRing::mapFrontBuffer(SkBitmap* bitmap, bool premultiplyAlpha)
{
    m_mutex.lock();
    if (!m_frontBuffer) {
        m_mutex.unlock();
        return false;
    }
    m_frontBuffer->mapBuffer(bitmap, premultiplyAlpha);
    return true;
}

void EGLImageBufferRing::unmapFrontBuffer()
{
    ASSERT(m_frontBuffer);
    m_frontBuffer->unmapBuffer();
    m_mutex.unlock();
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
