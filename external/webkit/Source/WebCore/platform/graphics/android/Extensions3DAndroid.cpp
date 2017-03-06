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
#include "Extensions3DAndroid.h"

#include "NotImplemented.h"
#include "WebGLSurface.h"

using WTF::makeLambda;

namespace WebCore {

Extensions3DAndroid::Extensions3DAndroid(WebGLSurface* surface)
    : m_initializedAvailableExtensions(false)
    , m_surface(surface)
{
}

Extensions3DAndroid::~Extensions3DAndroid()
{
}

bool Extensions3DAndroid::supports(const String& name)
{
    if (!m_initializedAvailableExtensions) {
        String extensionsString = reinterpret_cast<const char*>(m_surface->call(makeLambda(glGetString)(GL_EXTENSIONS)));
        Vector<String> availableExtensions;
        extensionsString.split(" ", availableExtensions);
        for (size_t i = 0; i < availableExtensions.size(); ++i)
            m_availableExtensions.add(availableExtensions[i]);
        m_initializedAvailableExtensions = true;
    }

    return m_availableExtensions.contains(name);
}

void Extensions3DAndroid::ensureEnabled(const String& name)
{
    if (name == "GL_OES_standard_derivatives" && supports(name)) {
        // Enable support in ANGLE (if not enabled already)
        m_surface->enableGLOESStandardDerivatives();
    }
}

bool Extensions3DAndroid::isEnabled(const String& name)
{
    return supports(name);
}

int Extensions3DAndroid::getGraphicsResetStatusARB()
{
    return m_surface->getGraphicsResetStatus();
}

void Extensions3DAndroid::blitFramebuffer(long srcX0, long srcY0, long srcX1, long srcY1, long dstX0, long dstY0, long dstX1, long dstY1, unsigned long mask, unsigned long filter)
{
    notImplemented();
}

void Extensions3DAndroid::renderbufferStorageMultisample(unsigned long target, unsigned long samples, unsigned long internalformat, unsigned long width, unsigned long height)
{
    notImplemented();
}

GLuint Extensions3DAndroid::createVertexArrayOES()
{
    GLuint array;
    m_surface->call(makeLambda(glGenVertexArraysOES)(1, &array));
    return array;
}

void Extensions3DAndroid::deleteVertexArrayOES(GLuint array)
{
    m_surface->push(makeLambda(glDeleteVertexArraysOES)(1, &array));
}

GLboolean Extensions3DAndroid::isVertexArrayOES(GLuint array)
{
    return m_surface->call(makeLambda(glIsVertexArrayOES)(array));
}

void Extensions3DAndroid::bindVertexArrayOES(GLuint array)
{
    m_surface->push(makeLambda(glBindVertexArrayOES)(array));
}

} // namespace WebCore
