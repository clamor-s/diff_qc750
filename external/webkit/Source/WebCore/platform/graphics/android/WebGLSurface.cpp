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
#include "WebGLSurface.h"

#include "EGLImage.h"
#include "GLUtils.h"
#include "GraphicsContext3D.h"
#include "ImageData.h"
#include <EGL/eglext_nv.h>
#include <algorithm>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/PassOwnArrayPtr.h>

#define XLOG(...) android_printLog(ANDROID_LOG_DEBUG, "WebGLSurface", __VA_ARGS__)

using WTF::makeLambda;

#define TEGRA_DRIVER_MEMORY_LIMIT 400*1024*1024

namespace WebCore {

// WebGLImageBuffer provides a texture for drawing into, and an EGL image for
// sourcing the ImageBuffer as a texture
class WebGLImageBuffer : public EGLImageBuffer {
public:
    WebGLImageBuffer(bool alpha)
        : m_size(0, 0)
    {
        GLuint oldTex;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&oldTex));
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, oldTex);

        m_colorFormat = alpha ? GL_RGBA : GL_RGB;
    }

    ~WebGLImageBuffer()
    {
        ASSERT(!m_texture);
    }

    EGLImage* eglImage() const { return m_eglImage.get(); }
    IntSize size() const { return m_size; }
    GLuint texture() const { return m_texture; }

    virtual void deleteSourceBuffer()
    {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }

    void resize(IntSize size)
    {
        m_size = size;
        GLuint oldTex;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&oldTex));
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, m_colorFormat, m_size.width(), m_size.height(), 0, m_colorFormat, GL_UNSIGNED_BYTE, 0);
        glBindTexture(GL_TEXTURE_2D, oldTex);

        if (!m_size.isEmpty())
            m_eglImage = EGLImage::createFromTexture(m_texture);
        else
            m_eglImage.clear();
    }

private:
    IntSize m_size;
    GLuint m_texture;
    GLuint m_colorFormat;
    OwnPtr<EGLImage> m_eglImage;
};

WebGLSurface::WebGLSurface(const GraphicsContext3D::Attributes& attrs)
    : m_attrs(attrs)
    , m_frameHasContent(false)
    , m_fbo(0)
    , m_depthBuffer(0)
    , m_stencilBuffer(0)
    , m_display(EGL_NO_DISPLAY)
    , m_surface(EGL_NO_SURFACE)
    , m_context(EGL_NO_CONTEXT)
    , m_fboBinding(0)
    , m_getGraphicsResetStatusEXT(0)
    , m_enabledGLOESStandardDerivatives(false)
    , m_hasLostContext(false)
    , m_hasSignalledContextLoss(false)
{
    char disableAsync[PROPERTY_VALUE_MAX];
    property_get("webkit.webgl.disableAsync", disableAsync, "0");
    if (!strtol(disableAsync, 0, 10))
        m_thread = adoptPtr(new Thread("WebGLSurface"));

    call(makeLambda(this, &WebGLSurface::initContextT)());

    XLOG("Successfully created %s WebGL context", m_thread ? "an asynchronous" : "a synchronous");
}

void WebGLSurface::initContextT()
{
    static const EGLint contextAttrs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT, EGL_LOSE_CONTEXT_ON_RESET_EXT,
        EGL_NONE
    };

    // WebGL does not support antialias in this implementation
    m_attrs.antialias = false;

    m_context = GLUtils::createBackgroundContext(EGL_NO_CONTEXT, &m_display, &m_surface, contextAttrs);

    const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (extensions && strstr(extensions, "GL_EXT_robustness"))
        m_getGraphicsResetStatusEXT = reinterpret_cast<PFNGLGETGRAPHICSRESETSTATUSEXTPROC>(eglGetProcAddress("glGetGraphicsResetStatusEXT"));

    // Create back buffer (front buffer will be created on the first swap).
    m_backBuffer = adoptPtr(new WebGLImageBuffer(m_attrs.alpha));
    m_backBuffer->lockSurface();

    // Create RBOs.
    if (m_attrs.depth) {
        glGenRenderbuffers(1, &m_depthBuffer);
        if (!m_depthBuffer)
            XLOG("Failed to create depth rbo");
    }
    if (m_attrs.stencil) {
        glGenRenderbuffers(1, &m_stencilBuffer);
        if (!m_stencilBuffer)
            XLOG("Failed to create stencil rbo");
    }
}

void WebGLSurface::initCompilerT()
{
    if (m_compiler)
        return;

    m_compiler = adoptPtr(new ANGLEWebKitBridge());
    ShBuiltInResources resources;
    ShInitBuiltInResources(&resources);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &resources.MaxVertexAttribs);
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &resources.MaxVertexUniformVectors);
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &resources.MaxVaryingVectors);
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &resources.MaxVertexTextureImageUnits);
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &resources.MaxCombinedTextureImageUnits);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &resources.MaxTextureImageUnits);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &resources.MaxFragmentUniformVectors);
    resources.OES_standard_derivatives = m_enabledGLOESStandardDerivatives ? 1 : 0;
    // Always set to 1 for OpenGL ES.
    resources.MaxDrawBuffers = 1;
    m_compiler->setResources(resources);
}

WebGLSurface::~WebGLSurface()
{
    push(makeLambda(this, &WebGLSurface::destroyGLContextT)());
    m_thread.clear();
}

void WebGLSurface::copyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    call(makeLambda(this, &WebGLSurface::copyTexImage2DT)(target, level, internalformat, x, y, width, height, border));
}

void WebGLSurface::copyTexImage2DT(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    if (checkDriverOutOfMemoryT())
            return;
    glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}

void WebGLSurface::texImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels)
{
    if (pixels)
        call(makeLambda(this, &WebGLSurface::texImage2DT)(target, level, internalformat, width, height, border, format, type, pixels));
    else
        push(makeLambda(this, &WebGLSurface::texImage2DT)(target, level, internalformat, width, height, border, format, type, pixels));
}

void WebGLSurface::texImage2DT(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels)
{
    if (checkDriverOutOfMemoryT())
            return;
    if (pixels)
        glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
    else {
        glTexImage2D(target, level, internalformat, width, height, border, format, type, 0);
        if (width > 0 && height > 0) {
            GLint currentTexture;
            GLint lastFBO;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &lastFBO);

            GLuint fbo;
            glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, currentTexture, level);
            GLUtils::clearRect(GL_COLOR_BUFFER_BIT, 0, 0, width, height);

            glBindFramebuffer(GL_FRAMEBUFFER, lastFBO);
            glDeleteFramebuffers(1, &fbo);
        }
    }
}

void WebGLSurface::compressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* pixels)
{
    call(makeLambda(this, &WebGLSurface::compressedTexImage2DT)(target, level, internalformat, width, height, border, imageSize, pixels));
}

void WebGLSurface::compressedTexImage2DT(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* pixels)
{
    if (checkDriverOutOfMemoryT())
        return;
    glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, pixels);
}

bool WebGLSurface::frameHasContent() const
{
    ASSERT(isMainThread());
    return m_frameHasContent;
}

void WebGLSurface::markContextChanged()
{
    ASSERT(isMainThread());
    m_frameHasContent = true;
}

void WebGLSurface::setContextLostCallback(PassOwnPtr<GraphicsContext3D::ContextLostCallback> callback)
{
    ASSERT(isMainThread());
    m_contextLostCallback = callback;
}

bool WebGLSurface::checkDriverOutOfMemoryT()
{
    FILE* f;
    char buf[255];
    // check nvrm memory usage of browser on tegra devices. We only need to check iovmm usage,
    // carveout oom errors are signaled by GL_OUT_OF_MEMORY
    f = fopen("/d/nvmap/iovmm/clients", "r");

    if (f) {
        int ourPid = getpid();
        int statsPid = 0;
        int statsMemoryUsage = 0;
        fscanf(f, "CLIENT PROCESS PID SIZE");
        while(fscanf(f, "%254s %254s %d %d", buf, buf, &statsPid, &statsMemoryUsage) != EOF
                && statsPid != ourPid );
        fclose(f);
        if (ourPid == statsPid) {
            if (statsMemoryUsage > TEGRA_DRIVER_MEMORY_LIMIT) {
                // Send a hint to logcat what is going on.
                if (!m_hasLostContext) {
                    XLOG("Browser tries to allocate more than %d bytes of nvrm memory. "
                            "Dropping WebGL context, set OUT_OF_MEMORY \n", statsMemoryUsage);
                }
                m_hasLostContext = true;
                return true;
            }
        }
    }
    return false;
}

PassOwnPtr<EGLImageBuffer> WebGLSurface::commitBackBuffer()
{
    ASSERT(isMainThread());
    if (m_hasLostContext) {
        if (!m_hasSignalledContextLoss && m_contextLostCallback) {
            m_contextLostCallback->onContextLost();
            m_hasSignalledContextLoss = true;
        }
        return 0;
    }

    if (!m_frameHasContent)
        return 0;

    OwnPtr<WebGLImageBuffer> previousBackBuffer = m_backBuffer.release();
    m_backBuffer = static_pointer_cast<WebGLImageBuffer>(bufferRing()->takeFreeBuffer());
    push(makeLambda(this, &WebGLSurface::setupNextBackBufferT)(previousBackBuffer.get()), 1);

    if (!m_attrs.preserveDrawingBuffer)
        m_frameHasContent = false;

    return previousBackBuffer.release();
}

void WebGLSurface::setupNextBackBufferT(WebGLImageBuffer* previousBackBuffer)
{
    previousBackBuffer->fence().set();

    if (!m_backBuffer)
        m_backBuffer = adoptPtr(new WebGLImageBuffer(m_attrs.alpha));
    // m_backBuffer can change as soon as we unlock previousBackBuffer, so we
    // make a local copy now.
    WebGLImageBuffer* localBackBuffer = m_backBuffer.get();

    localBackBuffer->lockSurface();
    previousBackBuffer->unlockSurface();

    if (m_fboBinding != m_fbo)
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    localBackBuffer->fence().finish();

    if (localBackBuffer->size() != previousBackBuffer->size())
        localBackBuffer->resize(previousBackBuffer->size());

    if (m_attrs.preserveDrawingBuffer) {
        int oldTextureBinding;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTextureBinding);

        // Copy the previous backbuffer (attached to m_fbo) to localBackBuffer.
        glBindTexture(GL_TEXTURE_2D, localBackBuffer->texture());
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, localBackBuffer->size().width(), localBackBuffer->size().height());
        glBindTexture(GL_TEXTURE_2D, oldTextureBinding);
    }

    // Attach localBackBuffer to m_fbo.
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, localBackBuffer->texture(), 0);

    if (m_fboBinding != m_fbo)
        glBindFramebuffer(GL_FRAMEBUFFER, m_fboBinding);

    if (getGraphicsResetStatusT() != GL_NO_ERROR)
        m_hasLostContext = true;
}

PassRefPtr<ImageData> WebGLSurface::readBackFramebuffer(VerticalOrientation verticalOrientation, AlphaMode alphaMode)
{
    return call(makeLambda(this, &WebGLSurface::readBackFramebufferT)(verticalOrientation, alphaMode));
}

PassRefPtr<ImageData> WebGLSurface::readBackFramebufferT(VerticalOrientation verticalOrientation, AlphaMode alphaMode)
{
    RefPtr<ImageData> image = ImageData::create(m_backBuffer->size());
    ByteArray* array = image->data()->data();
    if (array->length() != 4u * m_backBuffer->size().width() * m_backBuffer->size().height())
        return 0;

    // It's OK to use m_frameHasContent here because we're in a blocking call.
    if (!m_frameHasContent) {
        if (m_attrs.alpha)
            memset(array->data(), 0, array->length());
        else {
            const int value = makeRGB(0, 0, 0);
            int* const data = reinterpret_cast<int*>(array->data());
            const int length = array->length() / sizeof(data[0]);
            for (int i = 0; i < length; ++i)
                data[i] = value;
        }
        return image;
    }

    GLUtils::AlphaOp alphaOp;
    if (!m_attrs.premultipliedAlpha && alphaMode == AlphaPremultiplied)
        alphaOp = GLUtils::AlphaDoPremultiply;
    else if (m_attrs.premultipliedAlpha && alphaMode == AlphaNotPremultiplied)
        alphaOp = GLUtils::AlphaDoUnmultiply;
    else
        alphaOp = GLUtils::AlphaDoNothing;

    if (m_fboBinding != m_fbo)
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    GLuint textureBinding;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, reinterpret_cast<GLint*>(&textureBinding));
    if (textureBinding != m_backBuffer->texture())
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_backBuffer->texture(), 0);

    GLUtils::readPixels(0, 0, image->width(), image->height(), array->data(),
                        verticalOrientation == BottomToTop ? GLUtils::BottomToTop : GLUtils::TopToBottom,
                        alphaOp);

    if (textureBinding != m_backBuffer->texture())
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureBinding, 0);

    if (m_fboBinding != m_fbo)
        glBindFramebuffer(GL_FRAMEBUFFER, m_fboBinding);

    return image.release();
}

static GLuint renderbufferStorage(GLuint rbo, GLenum format, int width, int height)
{
    GLuint oldRbo;
    glGetIntegerv(GL_RENDERBUFFER_BINDING, reinterpret_cast<GLint*>(&oldRbo));
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, oldRbo);
    return rbo;
}

void WebGLSurface::reshape(int width, int height)
{
    ASSERT(isMainThread());
    call(makeLambda(this, &WebGLSurface::reshapeT)(width, height));
    m_frameHasContent = false;
}

void WebGLSurface::reshapeT(int width, int height)
{
    GLenum clearBuffers = GL_COLOR_BUFFER_BIT;

    if (m_depthBuffer) {
        renderbufferStorage(m_depthBuffer, GL_DEPTH_COMPONENT16, width, height);
        clearBuffers |= GL_DEPTH_BUFFER_BIT;
    }
    if (m_stencilBuffer) {
        renderbufferStorage(m_stencilBuffer, GL_STENCIL_INDEX8, width, height);
        clearBuffers |= GL_STENCIL_BUFFER_BIT;
    }

    m_backBuffer->resize(IntSize(width, height));

    GLuint oldFbo;
    getIntegervT(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&oldFbo));
    if (!m_fbo && width > 0 && height > 0) {
        // Create FBO and bind back buffer on first reshape since FBO creation
        // fails if uninitialized buffers are attached.
        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_backBuffer->texture(), 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBuffer);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_stencilBuffer);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            XLOG("Failed to create FBO");
    }

    // The WebGL spec requires us to clear the buffer on resize. However,
    // WebGLRenderingContext explicitly performs a clear before every draw
    // when !preserveDrawingBuffer and !m_backBuffer->hasContent().
    if (m_attrs.preserveDrawingBuffer) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        GLUtils::clearRect(clearBuffers, 0, 0, width, height);
    }
    bindFramebufferT(oldFbo);
}

void WebGLSurface::destroyGLContextT()
{
    m_compiler.clear();

    // Unbind fbo before destruction.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteRenderbuffers(1, &m_stencilBuffer);
    glDeleteRenderbuffers(1, &m_depthBuffer);
    glDeleteFramebuffers(1, &m_fbo);

    bufferRing()->surfaceDetached(m_backBuffer.get());

    eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (m_surface != EGL_NO_SURFACE)
        eglDestroySurface(m_display, m_surface);
    if (m_context != EGL_NO_CONTEXT)
        eglDestroyContext(m_display, m_context);
}

GLuint WebGLSurface::getGraphicsResetStatus()
{
    if (m_getGraphicsResetStatusEXT)
        return call(makeLambda(this, &WebGLSurface::getGraphicsResetStatusT)());

    return GL_NO_ERROR;
}

GLuint WebGLSurface::getGraphicsResetStatusT()
{
    if (m_getGraphicsResetStatusEXT)
        return m_getGraphicsResetStatusEXT();

    return GL_NO_ERROR;
}


// Note for the FBO functions below: FBO 0 is a special case: The default FBO
// in WebGL is actually m_backBuffer->fbo(). So if m_backBuffer->fbo()
// is bound, we wrap FBO ops to behave as if FBO 0 were bound.

void WebGLSurface::bindFramebuffer(GLuint fbo)
{
    ASSERT(WTF::isMainThread());
    push(makeLambda(this, &WebGLSurface::bindFramebufferT)(fbo));
}

void WebGLSurface::bindFramebufferT(GLuint fbo)
{
    // This is mostly just a forward to GL with whatever FBO caller wants. The only difference is
    // that if caller asks for 0, we actually bind m_fbo.
    if (!fbo)
        fbo = m_fbo;

    if (m_fboBinding != fbo) {
        m_fboBinding = fbo;
        glBindFramebuffer(GL_FRAMEBUFFER, m_fboBinding);
    }
}

void WebGLSurface::framebufferTexture2D(GLenum attachment, GLuint textarget, GLuint texture, GLint level)
{
    ASSERT(WTF::isMainThread());
    push(makeLambda(this, &WebGLSurface::framebufferTexture2DT)(attachment, textarget, texture, level));
}

void WebGLSurface::framebufferTexture2DT(GLenum attachment, GLuint textarget, GLuint texture, GLint level)
{
    if (m_fboBinding == m_fbo)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, textarget, texture, level);

    if (m_fboBinding == m_fbo)
        glBindFramebuffer(GL_FRAMEBUFFER, m_fboBinding);
}

void WebGLSurface::framebufferRenderbuffer(GLenum attachment, GLuint renderbuffertarget, GLuint rbo)
{
    ASSERT(WTF::isMainThread());
    push(makeLambda(this, &WebGLSurface::framebufferRenderbufferT)(attachment, renderbuffertarget, rbo));
}

void WebGLSurface::framebufferRenderbufferT(GLenum attachment, GLuint renderbuffertarget, GLuint rbo)
{
    if (m_fboBinding == m_fbo)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, renderbuffertarget, rbo);

    if (m_fboBinding == m_fbo)
        glBindFramebuffer(GL_FRAMEBUFFER, m_fboBinding);
}

void WebGLSurface::getFramebufferAttachmentParameteriv(GLenum attachment, GLenum pname, int* value)
{
    ASSERT(WTF::isMainThread());
    call(makeLambda(this, &WebGLSurface::getFramebufferAttachmentParameterivT)(attachment, pname, value));
}

void WebGLSurface::getFramebufferAttachmentParameterivT(GLenum attachment, GLenum pname, int* value)
{
    if (m_fboBinding == m_fbo)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment, pname, value);

    if (m_fboBinding == m_fbo)
        glBindFramebuffer(GL_FRAMEBUFFER, m_fboBinding);
}

GLenum WebGLSurface::checkFramebufferStatus()
{
    ASSERT(WTF::isMainThread());
    return call(makeLambda(this, &WebGLSurface::checkFramebufferStatusT)());
}

GLenum WebGLSurface::checkFramebufferStatusT()
{
    if (m_fboBinding == m_fbo)
        return GL_FRAMEBUFFER_COMPLETE;

    return glCheckFramebufferStatus(GL_FRAMEBUFFER);
}

void WebGLSurface::getIntegerv(GLenum pname, GLint* params)
{
    ASSERT(WTF::isMainThread());
    call(makeLambda(this, &WebGLSurface::getIntegervT)(pname, params));
}

void WebGLSurface::getIntegervT(GLenum pname, GLint* params)
{
    glGetIntegerv(pname, params);
    if (pname == GL_FRAMEBUFFER_BINDING && static_cast<GLuint>(*params) == m_fbo)
        *params = 0;
}


void WebGLSurface::shaderSource(GLuint shader, const String& source)
{
    ASSERT(WTF::isMainThread());
    push(makeLambda(this, &WebGLSurface::shaderSourceT)(shader, source.crossThreadString()));
}

// Use 'String' instead of 'const String&' to get a copy as the invocation is parallel.
void WebGLSurface::shaderSourceT(GLuint shader, String source)
{
    ShaderSourceEntry entry;
    entry.source = source;
    m_shaderSourceMap.set(shader, entry);
}

void WebGLSurface::enableGLOESStandardDerivatives()
{
    ASSERT(WTF::isMainThread());
    push(makeLambda(this, &WebGLSurface::enableGLOESStandardDerivativesT)());
}

void WebGLSurface::enableGLOESStandardDerivativesT()
{
    if (m_compiler && !m_enabledGLOESStandardDerivatives) {
        m_enabledGLOESStandardDerivatives = true;
        ShBuiltInResources ANGLEResources = m_compiler->getResources();
        if (!ANGLEResources.OES_standard_derivatives) {
            ANGLEResources.OES_standard_derivatives = 1;
            m_compiler->setResources(ANGLEResources);
        }
    }
}

void WebGLSurface::compileShader(GLuint shader)
{
    ASSERT(WTF::isMainThread());
    push(makeLambda(this, &WebGLSurface::compileShaderT)(shader));
}

void WebGLSurface::compileShaderT(GLuint shader)
{
    int GLshaderType;
    ANGLEShaderType shaderType;

    glGetShaderiv(shader, GL_SHADER_TYPE, &GLshaderType);
    if (GLshaderType == GL_VERTEX_SHADER)
        shaderType = SHADER_TYPE_VERTEX;
    else if (GLshaderType == GL_FRAGMENT_SHADER)
        shaderType = SHADER_TYPE_FRAGMENT;
    else
        return; // Invalid shader type.

    HashMap<Platform3DObject, ShaderSourceEntry>::iterator result = m_shaderSourceMap.find(shader);

    if (result == m_shaderSourceMap.end())
        return;

    ShaderSourceEntry& entry = result->second;
    String translatedShaderSource;
    String shaderInfoLog;

    initCompilerT();
    bool isValid = m_compiler->validateShaderSource(entry.source.utf8().data(), shaderType, translatedShaderSource, shaderInfoLog, SH_ESSL_OUTPUT);

    entry.log = shaderInfoLog;
    entry.isValid = isValid;

    if (!isValid)
        return; // Shader didn't validate, don't move forward with compiling translated source

    int translatedShaderLength = translatedShaderSource.length();

    const CString& translatedShaderCString = translatedShaderSource.utf8();
    const char* translatedShaderPtr = translatedShaderCString.data();

    glShaderSource(shader, 1, &translatedShaderPtr, &translatedShaderLength);

    glCompileShader(shader);

    int GLCompileSuccess;

    glGetShaderiv(shader, GL_COMPILE_STATUS, &GLCompileSuccess);

    // OpenGL might not accept the shader even though it was validated by ANGLE, probably
    // due to usage of functionality not supported by the hardware.
    if (GLCompileSuccess != GL_TRUE)
        XLOG("OpenGL shader compilation failed for an ANGLE validated %s shader", (shaderType == SHADER_TYPE_VERTEX) ? "vertex" : "fragment");
}

String WebGLSurface::getShaderSource(GLuint shader)
{
    ASSERT(WTF::isMainThread());
    String retValue;
    call(makeLambda(this, &WebGLSurface::getShaderSourceT)(shader, retValue));
    if (retValue.isNull())
        return "";
    return retValue;
}

void WebGLSurface::getShaderSourceT(GLuint shader, String& retValue)
{
    HashMap<Platform3DObject, ShaderSourceEntry>::iterator result = m_shaderSourceMap.find(shader);
    if (result == m_shaderSourceMap.end())
        return;

    retValue = result->second.source.crossThreadString();
}

String WebGLSurface::getShaderInfoLog(GLuint shader)
{
    ASSERT(WTF::isMainThread());
    String retValue;
    call(makeLambda(this, &WebGLSurface::getShaderInfoLogT)(shader, retValue));
    if (retValue.isNull())
        return "";
    return retValue;
}

void WebGLSurface::getShaderInfoLogT(GLuint shader, String& retValue)
{
    HashMap<Platform3DObject, ShaderSourceEntry>::iterator result = m_shaderSourceMap.find(shader);
    if (result == m_shaderSourceMap.end())
        return;

    ShaderSourceEntry entry = result->second;
    if (!entry.isValid) {
        retValue = entry.log.crossThreadString();
        return;
    }

    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if (!length)
        return;

    GLsizei size = 0;
    OwnArrayPtr<GLchar> info = adoptArrayPtr(new GLchar[length]);
    glGetShaderInfoLog(shader, length, &size, info.get());

    retValue = String(info.get()).crossThreadString();
}

void WebGLSurface::getShaderiv(GraphicsContext3D* context, GLuint shader, GLuint pname, GLint* value)
{
    ASSERT(WTF::isMainThread());
    if (!call(makeLambda(this, &WebGLSurface::getShaderivT)(shader, pname, value)))
        context->synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
}

bool WebGLSurface::getShaderivT(GLuint shader, GLenum pname, GLint* value)
{
    HashMap<Platform3DObject, ShaderSourceEntry>::iterator result = m_shaderSourceMap.find(shader);

    switch (pname) {
    case GraphicsContext3D::DELETE_STATUS:
    case GraphicsContext3D::SHADER_TYPE:
        glGetShaderiv(shader, pname, value);
        break;

    case GraphicsContext3D::COMPILE_STATUS:
        if (result == m_shaderSourceMap.end()) {
            *value = static_cast<int>(false);
            break;
        }

        *value = static_cast<int>(result->second.isValid);
        break;

    case GraphicsContext3D::INFO_LOG_LENGTH: {
        if (result == m_shaderSourceMap.end()) {
            *value = 0;
            break;
        }
        String retValue;
        getShaderInfoLogT(shader, retValue);
        *value = retValue.length();
        break;
    }

    case GraphicsContext3D::SHADER_SOURCE_LENGTH: {
        String retValue;
        getShaderSourceT(shader, retValue);
        *value = retValue.length();
        break;
    }

    default:
        return false;
    }

    return true;
}

void WebGLSurface::releaseShaderCompiler()
{
    ASSERT(WTF::isMainThread());
    push(makeLambda(this, &WebGLSurface::releaseShaderCompilerT)());
}

void WebGLSurface::releaseShaderCompilerT()
{
    m_compiler.clear();
    glReleaseShaderCompiler();
}


} // namespace WebCore
