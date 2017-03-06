/*
 * Copyright (c) 2011-2012, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef PushReadbackContext_h
#define PushReadbackContext_h

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <wtf/Noncopyable.h>

namespace WebCore {

class PushReadbackContext {
    WTF_MAKE_NONCOPYABLE(PushReadbackContext);

public:
    PushReadbackContext()
        : m_oldDisplay(eglGetCurrentDisplay())
        , m_oldDrawSurface(eglGetCurrentSurface(EGL_DRAW))
        , m_oldReadSurface(eglGetCurrentSurface(EGL_READ))
        , m_oldContext(eglGetCurrentContext())
    {
        m_isUsable = makeReadbackContextCurrent();
    }

    static bool makeReadbackContextCurrent();

    bool isUsable() const { return m_isUsable; }

    ~PushReadbackContext()
    {
        if (m_isUsable)
            eglMakeCurrent(m_oldDisplay, m_oldDrawSurface, m_oldReadSurface, m_oldContext);
    }

private:
    EGLDisplay m_oldDisplay;
    EGLSurface m_oldDrawSurface;
    EGLSurface m_oldReadSurface;
    EGLContext m_oldContext;
    bool m_isUsable;
};

} // namespace WebCore

#endif // ReadbackContext_h
