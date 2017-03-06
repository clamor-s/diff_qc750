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

#ifndef MappedTexture_h
#define MappedTexture_h

#include "EGLImage.h"
#include "IntSize.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <ui/GraphicBuffer.h>
#include <utils/threads.h>
#include <wtf/Noncopyable.h>

namespace android {

class GraphicBuffer;

};

namespace WebCore {

/**
 * MappedTexture is a class that encapsulates code for allocating a memory region in WebKit
 * thread and using that memory as an OpenGL texture in compositor thread. It is invalid to make
 * calls on the OpenGL texture that would reallocate (like glTexImage2D()) but calls like
 * glTexSubImage2D() are OK.
 * The MappedTexture instance is created, destroyed and manipulated in WebKit thread.
 * Caller should manage the mutual exclusion to the instance.
 */
class MappedTexture {
    WTF_MAKE_NONCOPYABLE(MappedTexture);

public:
    MappedTexture(const IntSize&);
    ~MappedTexture();

    IntSize size() const { return m_size; }

    struct LockedBuffer {
        uint8_t* pixels;
        int rowBytes;
    };

    const LockedBuffer* lockBuffer();
    void unlockBuffer();

    void copyTo(MappedTexture* dest);

    // Any texture should be destroyed before buffer is locked again.
    EGLImage* eglImage() const { return m_eglImage.get(); }

private:
    IntSize m_size;
    android::sp<android::GraphicBuffer> m_graphicBuffer;
    LockedBuffer m_buffer;
    OwnPtr<EGLImage> m_eglImage;
};

} // namespace WebCore

#endif // MappedTexture_h
