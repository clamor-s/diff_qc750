/*
 * Copyright (C) 2011, NVIDIA CORPORATION
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
#include "MappedTexture.h"

#include "GLUtils.h"
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>

namespace WebCore {

static bool makeCopyContextCurrentContext()
{
    static bool hasInitialized = false;
    static EGLDisplay copyDisplay;
    static EGLSurface copySurface;
    static EGLContext copyContext;

    if (!hasInitialized) {
        copyContext = GLUtils::createBackgroundContext(EGL_NO_CONTEXT, &copyDisplay, &copySurface);
        if (copyContext == EGL_NO_CONTEXT)
            return false;

        hasInitialized = true;
    }

    if (eglGetCurrentContext() == copyContext)
        return true;

    EGLBoolean returnValue = eglMakeCurrent(copyDisplay, copySurface, copySurface, copyContext);
    GLUtils::checkEglError("eglMakeCurrent", returnValue);
    return returnValue;
}


MappedTexture::MappedTexture(const IntSize& size)
    : m_size(size)
{
    m_buffer.pixels = 0;

    // Initialize EGL to be able to create EGLImages.
    static bool hasInitialized = false;
    if (!hasInitialized) {
        EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY) {
            GLUtils::checkEglError("eglGetDisplay");
            CRASH();
        }

        if (!eglInitialize(display, 0, 0)) {
            GLUtils::checkEglError("eglInitialize");
            CRASH();
        }

        hasInitialized = true;
    }

    m_graphicBuffer = new android::GraphicBuffer(m_size.width(), m_size.height(), android::PIXEL_FORMAT_RGBA_8888,
                                                 GRALLOC_USAGE_SW_READ_OFTEN
                                                 | GRALLOC_USAGE_SW_WRITE_OFTEN
                                                 | GRALLOC_USAGE_HW_TEXTURE);

    EGLDisplay eglImageDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    GLUtils::checkEglError("eglGetDisplay", eglImageDisplay != EGL_NO_DISPLAY);

    static const EGLint attrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
    EGLClientBuffer clientBuffer = reinterpret_cast<EGLClientBuffer>(m_graphicBuffer->getNativeBuffer());
    EGLImageKHR eglImage = eglCreateImageKHR(eglImageDisplay, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuffer, attrs);
    GLUtils::checkEglError("eglCreateImageKHR", eglImage != EGL_NO_IMAGE_KHR);
    m_eglImage = EGLImage::adopt(eglImage, eglImageDisplay);

    // Map the buffer to CPU memory once. This works around an apparent gralloc
    // issue where random noise can appear during the first mapping.
    lockBuffer();
    unlockBuffer();
}

MappedTexture::~MappedTexture()
{
    if (m_buffer.pixels)
        m_graphicBuffer->unlock();
}

const MappedTexture::LockedBuffer* MappedTexture::lockBuffer()
{
    if (m_buffer.pixels)
        return &m_buffer;

    if (m_graphicBuffer->lock(GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN,
                              reinterpret_cast<void**>(&m_buffer.pixels)) == android::OK) {
        m_buffer.rowBytes = m_graphicBuffer->getStride() * android::bytesPerPixel(m_graphicBuffer->getPixelFormat());
        return &m_buffer;
    }

    m_buffer.pixels = 0;
    return 0;

}

void MappedTexture::unlockBuffer()
{
    if (!m_buffer.pixels)
        return;

    m_graphicBuffer->unlock();
    m_buffer.pixels = 0;
}

void MappedTexture::copyTo(MappedTexture* dest)
{
    ASSERT(dest);

    if (!m_eglImage->isValid() || !dest->m_eglImage->isValid())
        return;

    if (!makeCopyContextCurrentContext())
        return;

    GLuint sourceTextureId = m_eglImage->createTexture();
    if (!sourceTextureId)
        return;

    GLuint destTextureId = dest->m_eglImage->createTexture();
    if (!destTextureId) {
        glDeleteTextures(1, &sourceTextureId);
        return;
    }

    GLuint copyFbo;
    glGenFramebuffers(1, &copyFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, copyFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sourceTextureId, 0);

    glBindTexture(GL_TEXTURE_2D, destTextureId);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, m_size.width(), m_size.height());

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &copyFbo);

    glDeleteTextures(1, &destTextureId);
    glDeleteTextures(1, &sourceTextureId);
}

} // namespace WebCore
