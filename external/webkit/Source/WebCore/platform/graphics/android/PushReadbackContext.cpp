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

#include "config.h"
#include "PushReadbackContext.h"

#include "GLUtils.h"

#include <wtf/StdLibExtras.h>
#include <wtf/ThreadSpecific.h>
#include <wtf/Threading.h>

namespace WebCore {

struct ReadBackContext {
    ReadBackContext()
    {
        m_context = GLUtils::createBackgroundContext(EGL_NO_CONTEXT, &m_display, &m_surface);
        if (m_context == EGL_NO_CONTEXT)
            return;

        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    }

    EGLContext m_context;
    EGLSurface m_surface;
    EGLDisplay m_display;
    GLuint m_fbo;
};

bool PushReadbackContext::makeReadbackContextCurrent()
{
    DEFINE_STATIC_LOCAL(WTF::ThreadSpecific<ReadBackContext>, readBackContext, ());

    if (readBackContext->m_context == EGL_NO_CONTEXT)
        return false;

    return eglMakeCurrent(readBackContext->m_display, readBackContext->m_surface, readBackContext->m_surface, readBackContext->m_context) == EGL_TRUE;
}

} // namespace WebCore
