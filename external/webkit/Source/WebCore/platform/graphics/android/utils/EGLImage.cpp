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
#include "EGLImage.h"

#include "GLUtils.h"

namespace WebCore {

PassOwnPtr<EGLImage> EGLImage::createFromTexture(GLuint textureID)
{
    EGLClientBuffer buffer = reinterpret_cast<EGLClientBuffer>(textureID);
    static const EGLint attr[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};
    EGLImageKHR image = eglCreateImageKHR(eglGetCurrentDisplay(), eglGetCurrentContext(),
                                          EGL_GL_TEXTURE_2D_KHR, buffer, attr);
    GLUtils::checkEglError("eglCreateImage", (image != EGL_NO_IMAGE_KHR));

    return adopt(image, eglGetCurrentDisplay());
}

EGLImage::~EGLImage()
{
    if (m_image != EGL_NO_IMAGE_KHR)
        eglDestroyImageKHR(m_imageDisplay, m_image);
}

GLuint EGLImage::createTexture(GLint filter, GLint wrap) const
{
    if (m_image == EGL_NO_IMAGE_KHR)
        return 0;

    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    writeToTexture2D(textureID);
    return textureID;

}

void EGLImage::writeToTexture2D(GLuint textureID) const
{
    if (m_image == EGL_NO_IMAGE_KHR)
        return;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, reinterpret_cast<GLeglImageOES>(m_image));
}

} // namespace WebCore
