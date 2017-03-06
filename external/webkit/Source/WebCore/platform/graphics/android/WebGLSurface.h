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

#ifndef WebGLSurface_h
#define WebGLSurface_h

#include "ANGLEWebKitBridge.h"
#include "EGLImageSurface.h"
#include "GraphicsContext3D.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2ext_nv.h>
#include <utils/threads.h>
#include <wtf/DelegateThread.h>
#include <wtf/Lambda.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class ImageData;
class WebGLImageBuffer;

typedef struct {
    String source;
    String log;
    bool isValid;
} ShaderSourceEntry;
typedef HashMap<Platform3DObject, ShaderSourceEntry> ShaderSourceMap;

class WebGLSurface : public EGLImageSurface {
    static const unsigned ThreadQueueCapacity = 2048;
    typedef WTF::DelegateThread<ThreadQueueCapacity> Thread;

public:
    WebGLSurface(const GraphicsContext3D::Attributes& attrs);
    virtual ~WebGLSurface();

    GraphicsContext3D::Attributes getContextAttributes() const { return m_attrs; }

    // EGLImageSurface overrrides.
    virtual bool isInverted() const { return true; }
    virtual bool hasAlpha() const { return m_attrs.alpha; }
    virtual bool hasPremultipliedAlpha() const { return m_attrs.premultipliedAlpha; }
    virtual bool requiresFenceWaitAfterPainting() const { return true; }
    virtual PassOwnPtr<EGLImageBuffer> commitBackBuffer();

    bool frameHasContent() const;
    void markContextChanged();
    void setContextLostCallback(PassOwnPtr<GraphicsContext3D::ContextLostCallback>);

    enum VerticalOrientation {BottomToTop, TopToBottom};
    enum AlphaMode {AlphaPremultiplied, AlphaNotPremultiplied};
    PassRefPtr<ImageData> readBackFramebuffer(VerticalOrientation, AlphaMode);
    void reshape(int width, int height);
    GLuint getGraphicsResetStatus();

    // Wrapper methods for wrapped GL calls.
    void bindFramebuffer(GLuint fbo);
    void framebufferTexture2D(GLenum attachment, GLuint textarget, GLuint texture, GLint level);
    void framebufferRenderbuffer(GLenum attachment, GLuint renderbuffertarget, GLuint rbo);
    void getFramebufferAttachmentParameteriv(GLenum attachment, GLenum pname, int* value);
    GLenum checkFramebufferStatus();
    void getIntegerv(GLenum pname, GLint* params);

    bool checkDriverOutOfMemoryT();

    // These calls checks for browser GL memory consumption and trigger context loss if limit is reached
    void texImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);
    void texImage2DT(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);

    void compressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* pixels);
    void compressedTexImage2DT(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* pixels);

    void copyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
    void copyTexImage2DT(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);

    void shaderSource(GLuint shader, const String& source);
    void enableGLOESStandardDerivatives();
    void compileShader(GLuint shader);
    String getShaderSource(GLuint shader);
    String getShaderInfoLog(GLuint shader);
    void getShaderiv(GraphicsContext3D*, GLuint shader, GLuint pname, GLint* value);
    void clear(GLbitfield mask);
    void releaseShaderCompiler();

    // Methods used for calling unwrapped GL calls.
    inline void push(PassOwnPtr<WTF::Lambda> func, unsigned minJobsToWakeThread = 8)
    {
        if (m_thread)
            return m_thread->callLater(func, minJobsToWakeThread);

        OwnPtr<WTF::Lambda> ownFunc = func;
        if (m_context != EGL_NO_CONTEXT)
            eglMakeCurrent(m_display, m_surface, m_surface, m_context);
        ownFunc->call();
    }
    inline void call(PassOwnPtr<WTF::Lambda> func)
    {
        if (m_thread)
            return m_thread->call(func);

        OwnPtr<WTF::Lambda> ownFunc = func;
        if (m_context != EGL_NO_CONTEXT)
            eglMakeCurrent(m_display, m_surface, m_surface, m_context);
        ownFunc->call();
    }
    template<typename R> inline R call(PassOwnPtr<WTF::ReturnLambda<R> > func)
    {
        if (m_thread)
            return m_thread->call(func);

        OwnPtr<WTF::ReturnLambda<R> > ownFunc = func;
        if (m_context != EGL_NO_CONTEXT)
            eglMakeCurrent(m_display, m_surface, m_surface, m_context);
        ownFunc->call();
        return ownFunc->ret();
    }

private:
    // "T" means these methods are only meant to be called on the delegate thread.
    void initContextT();
    void initCompilerT();
    void setupNextBackBufferT(WebGLImageBuffer* previousBackBuffer);
    void reshapeT(int width, int height);
    PassRefPtr<ImageData> readBackFramebufferT(VerticalOrientation, AlphaMode);
    void destroyGLContextT();
    GLuint getGraphicsResetStatusT();

    void bindFramebufferT(GLuint fbo);
    void framebufferTexture2DT(GLenum attachment, GLuint textarget, GLuint texture, GLint level);
    void framebufferRenderbufferT(GLenum attachment, GLuint renderbuffertarget, GLuint rbo);
    void getFramebufferAttachmentParameterivT(GLenum attachment, GLenum pname, int* value);
    GLenum checkFramebufferStatusT();
    void getIntegervT(GLenum pname, GLint* params);

    void shaderSourceT(GLuint shader, String source);
    void enableGLOESStandardDerivativesT();
    void compileShaderT(GLuint shader);
    void getShaderSourceT(GLuint shader, String& retValue);
    void getShaderInfoLogT(GLuint shade, String& retValuer);
    bool getShaderivT(GLuint shader, GLuint pname, GLint* value);

    void releaseShaderCompilerT();

    inline void makeContextCurrent()
    {
        if (m_context != EGL_NO_CONTEXT)
            eglMakeCurrent(m_display, m_surface, m_surface, m_context);
    }

    OwnPtr<Thread> m_thread;
    GraphicsContext3D::Attributes m_attrs;
    bool m_frameHasContent;
    OwnPtr<WebGLImageBuffer> m_backBuffer;
    GLuint m_fbo;
    GLuint m_depthBuffer;
    GLuint m_stencilBuffer;
    EGLDisplay m_display;
    EGLSurface m_surface;
    EGLContext m_context;
    GLuint m_fboBinding;
    PFNGLGETGRAPHICSRESETSTATUSEXTPROC m_getGraphicsResetStatusEXT;
    OwnPtr<GraphicsContext3D::ContextLostCallback> m_contextLostCallback;
    bool m_enabledGLOESStandardDerivatives;
    ShaderSourceMap m_shaderSourceMap;
    OwnPtr<ANGLEWebKitBridge> m_compiler;
    bool m_hasLostContext;
    bool m_hasSignalledContextLoss;
};

} // namespace WebCore

#endif // WebGLSurface_h
