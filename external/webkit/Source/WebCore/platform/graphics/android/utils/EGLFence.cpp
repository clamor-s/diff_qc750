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
#include "EGLFence.h"

namespace WebCore {

EGLFence::EGLFence()
    : m_fence(EGL_NO_SYNC_KHR)
{
}

EGLFence::~EGLFence()
{
    clear();
}

void EGLFence::set(EGLDisplay display)
{
    clear();
    m_display = display;
    m_fence = eglCreateSyncKHR(m_display, EGL_SYNC_FENCE_KHR, 0);
}

void EGLFence::clear()
{
    if (m_fence == EGL_NO_SYNC_KHR)
        return;

    eglDestroySyncKHR(m_display, m_fence);
    m_fence = EGL_NO_SYNC_KHR;
}

void EGLFence::adopt(EGLFence& other)
{
    clear();
    m_display = other.m_display;
    m_fence = other.m_fence;
    other.m_fence = EGL_NO_SYNC_KHR;
}

bool EGLFence::hasFinished() const
{
    if (m_fence == EGL_NO_SYNC_KHR)
        return true;

    EGLint status;
    eglGetSyncAttribKHR(m_display, m_fence, EGL_SYNC_STATUS_KHR, &status);
    return status == EGL_SIGNALED_KHR;
}

bool EGLFence::finish(EGLTimeKHR nanosecondsTimeout) const
{
    if (m_fence == EGL_NO_SYNC_KHR)
        return true;

    EGLint status = eglClientWaitSyncKHR(m_display, m_fence, 0, nanosecondsTimeout);
    return status == EGL_CONDITION_SATISFIED_KHR;
}

bool EGLFence::flush() const
{
    if (m_fence == EGL_NO_SYNC_KHR)
        return true;

    EGLint status = eglClientWaitSyncKHR(m_display, m_fence, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, EGL_FOREVER_KHR);
    return status == EGL_CONDITION_SATISFIED_KHR;
}

} // namespace WebCore
