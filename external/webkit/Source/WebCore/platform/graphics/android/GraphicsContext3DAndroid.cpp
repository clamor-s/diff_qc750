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
#include "GraphicsContext3D.h"

#if ENABLE(WEBGL)

#include "CanvasRenderingContext.h"
#include "EGLImageLayer.h"
#include "Extensions3DAndroid.h"
#include "FrameView.h"
#include "GLUtils.h"
#include "GraphicsContext.h"
#include "HTMLCanvasElement.h"
#include "HostWindow.h"
#include "Image.h"
#include "ImageBuffer.h"
#include "ImageData.h"
#include "ImageDecoder.h"
#include "PlatformGraphicsContext.h"
#include "SkBitmapRef.h"
#include "SkColorPriv.h"
#include "SkDevice.h"
#include "WebFrameView.h"
#include "WebGLSurface.h"
#include <GLES2/gl2.h>
#include <cutils/log.h>
#include <wtf/Closure.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/text/CString.h>

// Uncomment to enable logging. The logging the api will cause a significant drop in performance.
// #define LOG_API

#if defined(LOG_API)
#include <sstream>

class APILog {
public:
    APILog(const char* name)
        : m_divider("")
    {
        m_str << "gl." << name << '(';
    }

    ~APILog()
    {
        m_str << ')';
        android_printLog(ANDROID_LOG_DEBUG, "GraphicsContext3DAndroid", "webgl> %s\n", m_str.str().c_str());
    }

    template<typename T> APILog& operator <<(const T& t)
    {
        m_str << m_divider << t;
        m_divider = ",";
        return *this;
    }

private:
    std::ostringstream m_str;
    const char* m_divider;
};

std::ostream& operator <<(std::ostream& out, const String& str)
{
    out << str.utf8().data();
    return out;
}

#define WEBGL_LOG(code) APILog code
#else
#define WEBGL_LOG(code)
#endif

PassOwnArrayPtr<uint8_t> zeroArray(size_t size)
{
    OwnArrayPtr<uint8_t> zero = adoptArrayPtr(new uint8_t[size]);
    memset(zero.get(), 0, size);
    return zero.release();
}

using WTF::makeLambda;

namespace WebCore {

PassRefPtr<GraphicsContext3D> GraphicsContext3D::create(GraphicsContext3D::Attributes attrs, HostWindow* hostWindow, GraphicsContext3D::RenderStyle renderStyle)
{
    return adoptRef(new GraphicsContext3D(attrs, hostWindow, true));
}

GraphicsContext3D::GraphicsContext3D(Attributes attrs, HostWindow* hostWindow, bool renderToWindow)
    : m_currentWidth(0)
    , m_currentHeight(0)
{
    m_surface = adoptRef(new WebGLSurface(attrs));
    m_layer = new EGLImageLayer(m_surface, "webgl");
    m_layer->unref();
}

GraphicsContext3D::~GraphicsContext3D()
{
}

IntSize GraphicsContext3D::getInternalFramebufferSize() const
{
    return IntSize(m_currentWidth, m_currentHeight);
}

void GraphicsContext3D::reshape(int width, int height)
{
    if (width == m_currentWidth && height == m_currentHeight)
        return;

    m_currentWidth = width;
    m_currentHeight = height;

    m_surface->reshape(width, height);
}

PlatformLayer* GraphicsContext3D::platformLayer() const
{
    return m_layer.get();
}

bool GraphicsContext3D::layerComposited() const
{
    // Since the surface is double-buffered, "layerComposited" really isn't
    // what WebGLRenderingContext wants to know. What it's asking is if the
    // frame is brand new (it clears each new frame).
    return !m_surface->frameHasContent();
}

void GraphicsContext3D::markLayerComposited()
{
    // This will only be reached if we aren't doing accelerated compositing
    // into a layer (so never). However, if we commit the back buffer here
    // WebGL will still have the correct behavior (with just a little bit of
    // unnecessary extra work) if somebody does do compositing that way.
    m_surface->commitBackBuffer();
}

void GraphicsContext3D::markContextChanged()
{
    m_surface->markContextChanged();
}

void GraphicsContext3D::setContextLostCallback(WTF::PassOwnPtr<WebCore::GraphicsContext3D::ContextLostCallback> callback)
{
    m_surface->setContextLostCallback(callback);
}

PlatformGraphicsContext3D GraphicsContext3D::platformGraphicsContext3D() const
{
    return 0;
}

GLuint GraphicsContext3D::platformTexture() const
{
    return 0;
}

GraphicsContext3D::Attributes GraphicsContext3D::getContextAttributes()
{
    return m_surface->getContextAttributes();
}

Extensions3D* GraphicsContext3D::getExtensions()
{
    if (!m_extensions)
        m_extensions = adoptPtr(new Extensions3DAndroid(m_surface.get()));
    return m_extensions.get();
}

bool GraphicsContext3D::paintsIntoCanvasBuffer() const
{
    return false;
}

PassRefPtr<ImageData> GraphicsContext3D::paintRenderingResultsToImageData()
{
    return m_surface->readBackFramebuffer(WebGLSurface::TopToBottom, WebGLSurface::AlphaNotPremultiplied);
}

void GraphicsContext3D::paintRenderingResultsToCanvas(CanvasRenderingContext* context)
{
    RefPtr<ImageData> image = m_surface->readBackFramebuffer(WebGLSurface::TopToBottom, WebGLSurface::AlphaPremultiplied);

    if (!image)
        return;

    SkBitmap sourceBitmap;
    sourceBitmap.setConfig(SkBitmap::kARGB_8888_Config, image->width(), image->height(), 4 * image->width());
    sourceBitmap.setPixels(image->data()->data()->data());
    sourceBitmap.setIsOpaque(!m_surface->hasAlpha());

    context->canvas()->buffer()->context()->platformContext()->prepareForDrawing();
    PlatformGraphicsContext* canvas = context->canvas()->buffer()->context()->platformContext();
    canvas->writePixels(sourceBitmap, 0, 0, SkCanvas::kNative_Premul_Config8888);
}

bool GraphicsContext3D::isGLES2Compliant() const
{
    return true;
}

void GraphicsContext3D::makeContextCurrent()
{
    // The context lives in its own thread, so this call is a NOP.
}

GLuint GraphicsContext3D::createShader(GLenum type)
{
    WEBGL_LOG(("createShader") << type);
    return m_surface->call(makeLambda(glCreateShader)(type));
}

void GraphicsContext3D::deleteShader(GLuint shader)
{
    WEBGL_LOG(("deleteShader") << shader);
    m_surface->push(makeLambda(glDeleteShader)(shader));
}

void GraphicsContext3D::shaderSource(GLuint shader, const String& source)
{
    WEBGL_LOG(("shaderSource") << shader << source);
    m_surface->shaderSource(shader, source);
}

void GraphicsContext3D::compileShader(GLuint shader)
{
    WEBGL_LOG(("compileShader") << shader);
    m_surface->compileShader(shader);
}

String GraphicsContext3D::getShaderSource(GLuint shader)
{
    WEBGL_LOG(("getShaderSource") << shader);
    return m_surface->getShaderSource(shader);
}

String GraphicsContext3D::getShaderInfoLog(GLuint shader)
{
    WEBGL_LOG(("getShaderInfoLog") << shader);
    return m_surface->getShaderInfoLog(shader);
}

void GraphicsContext3D::getShaderiv(GLuint shader, GLuint pname, GLint* value)
{
    WEBGL_LOG(("getShaderiv") << shader << pname << value);
    m_surface->getShaderiv(this, shader, pname, value);
}

void GraphicsContext3D::releaseShaderCompiler()
{
    WEBGL_LOG(("releaseShaderCompiler"));
    m_surface->releaseShaderCompiler();
}

static void webglGetProgramInfoLog(GLuint program, String& retValue)
{
    GLint logSize = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
    if (logSize > 1) {
        GLsizei returnedLength;
        OwnArrayPtr<GLchar> log = adoptArrayPtr(new GLchar[logSize]);
        glGetProgramInfoLog(program, logSize, &returnedLength, log.get());
        ASSERT(logSize == 1 + returnedLength);
        retValue = String(log.get()).crossThreadString();
    }
}

String GraphicsContext3D::getProgramInfoLog(GLuint program)
{
    WEBGL_LOG(("getProgramInfoLog") << program);
    String retValue;
    m_surface->call(makeLambda(webglGetProgramInfoLog)(program, retValue));
    if (retValue.isNull())
        return "";
    return retValue;
}

long GraphicsContext3D::getVertexAttribOffset(GLuint index, GLenum pname)
{
    WEBGL_LOG(("getVertexAttribOffset") << index << pname);
    void* ret = 0;
    m_surface->call(makeLambda(glGetVertexAttribPointerv)(index, pname, &ret));
    return static_cast<long>(reinterpret_cast<intptr_t>(ret));
}

void GraphicsContext3D::vertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLintptr offset)
{
    WEBGL_LOG(("vertexAttribPointer") << index << size << type << normalized << stride << offset);
    m_surface->push(makeLambda(glVertexAttribPointer)(index, size, type, normalized, stride,
                                                     reinterpret_cast<void*>(static_cast<intptr_t>(offset))));
}

bool GraphicsContext3D::validateShaderLocation(const String& string)
{
    static const size_t MaxLocationStringLength = 256;
    if (string.length() > MaxLocationStringLength) {
        synthesizeGLError(INVALID_VALUE);
        return false;
    }
    return true;
}

GLint GraphicsContext3D::getUniformLocation(GLuint program, const String& name)
{
    WEBGL_LOG(("getUniformLocation") << program << name);
    if (!validateShaderLocation(name))
        return -1;
    CString utf8 = name.utf8();
    return m_surface->call(makeLambda(glGetUniformLocation)(program, utf8.data()));
}

static void webglTexImage2DResourceSafe(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, unsigned imageSize)
{
    if (!width || !height)
        return glTexImage2D(target, level, internalformat, width, height, border, format, type, 0);

    if (level) {
        // FBO's can't render to non-zero levels.
        OwnArrayPtr<uint8_t> zero = zeroArray(imageSize);
        return glTexImage2D(target, level, internalformat, width, height, border, format, type, zero.get());
    }

    glTexImage2D(target, level, internalformat, width, height, border, format, type, 0);

    GLint boundTexture;
    GLint lastFBO;
    GLenum bindingName = (target == GL_TEXTURE_2D) ? GL_TEXTURE_BINDING_2D : GL_TEXTURE_BINDING_CUBE_MAP;
    glGetIntegerv(bindingName, &boundTexture);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &lastFBO);

    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, boundTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        ASSERT_NOT_REACHED();
        OwnArrayPtr<uint8_t> zero = zeroArray(imageSize);
        glTexSubImage2D(target, 0, 0, 0, width, height, format, type, zero.get());
    } else
        GLUtils::clearRect(GL_COLOR_BUFFER_BIT, 0, 0, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, lastFBO);
    glDeleteFramebuffers(1, &fbo);
}

bool GraphicsContext3D::texImage2DResourceSafe(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLint unpackAlignment)
{
    WEBGL_LOG(("texImage2DResourceSafe") << target << level << internalformat << width << height << border << format << type << unpackAlignment);

    unsigned imageSize;
    GLenum error = computeImageSizeInBytes(format, type, width, height, unpackAlignment, &imageSize, 0);
    if (error != GraphicsContext3D::NO_ERROR) {
        synthesizeGLError(error);
        return false;
    }
    m_surface->push(makeLambda(webglTexImage2DResourceSafe)(target, level, internalformat, width, height, border, format, type, imageSize));

    return true;
}

bool GraphicsContext3D::texImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels)
{
    WEBGL_LOG(("texImage2D") << target << level << internalformat << width << height << border << format << type << pixels);
    ASSERT(pixels);
    m_surface->texImage2D(target, level, internalformat, width, height, border, format, type, pixels);
    return true;
}

void GraphicsContext3D::copyTexImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Dint border)
{
    WEBGL_LOG(("copyTexImage2D") << target << level << internalformat << x << y << width << height << border);
    m_surface->copyTexImage2D(target, level, internalformat, x, y, width, height, border);
}

void GraphicsContext3D::texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels)
{
    WEBGL_LOG(("texSubImage2D") << target << level << xoffset << yoffset << width << height << format << type << pixels);
    // We could copy the array here and not have to block, but this will be faster as the image gets
    // larger.
    if (pixels)
        m_surface->call(makeLambda(glTexSubImage2D)(target, level, xoffset, yoffset, width, height, format, type, pixels));
}

void GraphicsContext3D::compressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* pixels)
{
    WEBGL_LOG(("compressedTexImage2D") << target << level << internalformat << width << height << border << imageSize << pixels);

    // The pixel data pointer is required for this function call. WebGLRenderingContext checks it as
    // part of its function data validation.
    ASSERT(pixels);
    // We could copy the array here and not have to block, but this will be faster as the image gets
    // larger.
    m_surface->compressedTexImage2D(target, level, internalformat, width, height, border, imageSize, pixels);
}

void GraphicsContext3D::compressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* pixels)
{
    WEBGL_LOG(("compressedTexSubImage2D") << target << level << xoffset << yoffset << width << height << format << imageSize << pixels);
    if (pixels)
        m_surface->call(makeLambda(glCompressedTexSubImage2D)(target, level, xoffset, yoffset, width, height, format, imageSize, pixels));
}

String GraphicsContext3D::getString(GLenum name)
{
    WEBGL_LOG(("getString") << name);
    // might want to consider returning our own strings in the future
    return reinterpret_cast<const char*>(m_surface->call(makeLambda(glGetString)(name)));
}

void GraphicsContext3D::synthesizeGLError(GLenum error)
{
    WEBGL_LOG(("synthesizeGLError") << error);
    m_syntheticErrors.add(error);
}

GLenum GraphicsContext3D::getError()
{
    WEBGL_LOG(("getError"));
    if (m_syntheticErrors.size() > 0) {
        ListHashSet<unsigned long>::iterator iter = m_syntheticErrors.begin();
        GLenum err = *iter;
        m_syntheticErrors.remove(iter);
        return err;
    }
    return m_surface->call(makeLambda(glGetError)());
}

void GraphicsContext3D::drawElements(GLenum mode, GLsizei count, GLenum type, GLintptr offset)
{
    WEBGL_LOG(("drawElements") << mode << count << type << offset);
    // Send "1" to make sure the GL thread wakes up and starts drawing immediately.
    m_surface->push(makeLambda(glDrawElements)(mode, count, type,
            reinterpret_cast<void*>(static_cast<intptr_t>(offset))), 1);
}

void GraphicsContext3D::drawArrays(GLenum mode, GLint first, GLsizei count)
{
    WEBGL_LOG(("drawArrays") << mode << first << count);
    // Send "1" to make sure the GL thread wakes up and starts drawing immediately.
    m_surface->push(makeLambda(glDrawArrays)(mode, first, count), 1);
}

static bool webglGetActiveAttrib(GLuint program, GLuint index, ActiveInfo& info)
{
    GLint maxAttributeSize = 0;
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxAttributeSize);
    GLchar name[maxAttributeSize]; // GL_ACTIVE_ATTRIBUTE_MAX_LENGTH includes null termination.
    GLsizei nameLength = 0;
    GLint size = 0;
    GLenum type = 0;
    glGetActiveAttrib(program, index, maxAttributeSize, &nameLength, &size, &type, name);
    if (!nameLength)
        return false;
    info.name = String(name, nameLength).crossThreadString();
    info.type = type;
    info.size = size;
    return true;
}

bool GraphicsContext3D::getActiveAttrib(GLuint program, GLuint index, ActiveInfo& info)
{
    WEBGL_LOG(("getActiveAttrib") << program << index);
    if (!program) {
        synthesizeGLError(INVALID_VALUE);
        return false;
    }
    return m_surface->call(makeLambda(webglGetActiveAttrib)(program, index, info));
}

static bool webglGetActiveUniform(GLuint program, GLuint index, ActiveInfo& info)
{
    GLint maxUniformSize = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformSize);
    GLchar name[maxUniformSize]; // GL_ACTIVE_UNIFORM_MAX_LENGTH includes null termination.
    GLsizei nameLength = 0;
    GLint size = 0;
    GLenum type = 0;
    glGetActiveUniform(program, index, maxUniformSize, &nameLength, &size, &type, name);
    if (!nameLength)
        return false;
    info.name = String(name, nameLength).crossThreadString();
    info.type = type;
    info.size = size;
    return true;
}

bool GraphicsContext3D::getActiveUniform(GLuint program, GLuint index, ActiveInfo& info)
{
    WEBGL_LOG(("getActiveUniform") << program << index);
    if (!program) {
        synthesizeGLError(INVALID_VALUE);
        return false;
    }
    return m_surface->call(makeLambda(webglGetActiveUniform)(program, index, info));
}

int GraphicsContext3D::getAttribLocation(GLuint program, const String& name)
{
    WEBGL_LOG(("getAttribLocation") << program << name);
    if (!validateShaderLocation(name))
        return -1;
    CString utf8 = name.utf8();
    return m_surface->call(makeLambda(glGetAttribLocation)(program, utf8.data()));
}

static void webglBindAttribLocation(GLuint program, GLuint index, PassOwnArrayPtr<char> name)
{
    OwnArrayPtr<char> ownName = name;
    glBindAttribLocation(program, index, ownName.get());
}

void GraphicsContext3D::bindAttribLocation(GLuint program, GLuint index, const String& name)
{
    WEBGL_LOG(("bindAttribLocation") << program << index << name);
    if (!validateShaderLocation(name))
        return;

    CString utf8 = name.utf8();
    m_surface->push(makeLambda(webglBindAttribLocation)(program, index, WTF::makeLambdaArrayArg(utf8.data(), utf8.length() + 1)));
}

void GraphicsContext3D::bindFramebuffer(GLenum target, GLuint name)
{
    WEBGL_LOG(("bindFramebuffer") << target << name);
    if (target != GL_FRAMEBUFFER) {
        synthesizeGLError(INVALID_ENUM);
        return;
    }
    m_surface->bindFramebuffer(name);
}

void GraphicsContext3D::framebufferTexture2D(GLenum target, GLenum attachment, GLuint textarget, GLuint texture, GLint level)
{
    WEBGL_LOG(("framebufferTexture2D") << target << attachment << textarget << texture << level);
    if (target != GL_FRAMEBUFFER) {
        synthesizeGLError(INVALID_ENUM);
        return;
    }
    m_surface->framebufferTexture2D(attachment, textarget, texture, level);
}

void GraphicsContext3D::framebufferRenderbuffer(GLenum target, GLenum attachment, GLuint renderbuffertarget, GLuint rbo)
{
    WEBGL_LOG(("framebufferRenderbuffer") << target << attachment << renderbuffertarget << rbo);
    if (target != GL_FRAMEBUFFER) {
        synthesizeGLError(INVALID_ENUM);
        return;
    }
    m_surface->framebufferRenderbuffer(attachment, renderbuffertarget, rbo);
}

void GraphicsContext3D::getFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, int* value)
{
    WEBGL_LOG(("getFramebufferAttachmentParameteriv") << target << attachment << pname);
    if (target != GL_FRAMEBUFFER) {
        synthesizeGLError(INVALID_ENUM);
        return;
    }
    m_surface->getFramebufferAttachmentParameteriv(attachment, pname, value);
}

GLenum GraphicsContext3D::checkFramebufferStatus(GLenum target)
{
    WEBGL_LOG(("checkFramebufferStatus") << target);
    if (target != GL_FRAMEBUFFER) {
        synthesizeGLError(INVALID_ENUM);
        return NONE;
    }
    return m_surface->checkFramebufferStatus();
}

static void webglRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    glRenderbufferStorage(target, internalformat, width, height);

    // WebGL security dictates that we clear the buffer after allocation.
    if (!width || !height)
        return;

    GLuint oldFbo, tempFbo, rbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&oldFbo));
    glGetIntegerv(GL_RENDERBUFFER_BINDING, reinterpret_cast<GLint*>(&rbo));
    glGenFramebuffers(1, &tempFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, tempFbo);

    GLenum attachment = 0, clearBuffer = 0;
    switch (internalformat) {
    case GL_DEPTH_COMPONENT16:
        attachment = GL_DEPTH_ATTACHMENT;
        clearBuffer = GL_DEPTH_BUFFER_BIT;
        break;
    case GL_STENCIL_INDEX8:
        attachment = GL_STENCIL_ATTACHMENT;
        clearBuffer = GL_STENCIL_BUFFER_BIT;
        break;
    case GL_RGBA4:
    case GL_RGB565:
    case GL_RGB5_A1:
        attachment = GL_COLOR_ATTACHMENT0;
        clearBuffer = GL_COLOR_BUFFER_BIT;
        break;
    }
    ASSERT(attachment && clearBuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, rbo);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
        GLUtils::clearRect(clearBuffer, 0, 0, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, oldFbo);
    glDeleteFramebuffers(1, &tempFbo);
}

void GraphicsContext3D::renderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    WEBGL_LOG(("renderbufferStorage") << target << internalformat << width << height);
    m_surface->push(makeLambda(webglRenderbufferStorage)(target, internalformat, width, height));
}

void GraphicsContext3D::getIntegerv(GLenum pname, GLint* params)
{
    WEBGL_LOG(("getIntegerv") << pname << params);
    m_surface->getIntegerv(pname, params);
}

static void webglBufferDataResourceSafe(GLenum target, GLsizeiptr size, GLenum usage)
{
    OwnArrayPtr<uint8_t> zero = zeroArray(size);
    glBufferData(target, size, zero.get(), usage);
}

void GraphicsContext3D::bufferData(GLenum target, GLsizeiptr size, GLenum usage)
{
    WEBGL_LOG(("bufferData") << target << size << usage);
    m_surface->push(makeLambda(webglBufferDataResourceSafe)(target, size, usage));
}

static void webglBufferData(GLenum target, GLsizeiptr size, PassOwnArrayPtr<char> data, GLenum usage)
{
    OwnArrayPtr<char> ownData = data;
    glBufferData(target, size, ownData.get(), usage);
}

void GraphicsContext3D::bufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage)
{
    WEBGL_LOG(("bufferData") << target << size << data << usage);
    m_surface->push(makeLambda(webglBufferData)(target, size, WTF::makeLambdaArrayArg(data, size), usage));
}

static void webglBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, PassOwnArrayPtr<char> data)
{
    OwnArrayPtr<char> ownData = data;
    glBufferSubData(target, offset, size, ownData.get());
}

void GraphicsContext3D::bufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data)
{
    WEBGL_LOG(("bufferSubData") << target << offset << size << data);
    m_surface->push(makeLambda(webglBufferSubData)(target, offset, size, WTF::makeLambdaArrayArg(data, size)));
}

void GraphicsContext3D::vertexAttrib1fv(GLuint index, GLfloat* v)
{
    WEBGL_LOG(("vertexAttrib1fv") << index << v);
    vertexAttrib1f(index, v[0]);
}

void GraphicsContext3D::vertexAttrib2fv(GLuint index, GLfloat* v)
{
    WEBGL_LOG(("vertexAttrib2fv") << index << v);
    vertexAttrib2f(index, v[0], v[1]);
}

void GraphicsContext3D::vertexAttrib3fv(GLuint index, GLfloat* v)
{
    WEBGL_LOG(("vertexAttrib3fv") << index << v);
    vertexAttrib3f(index, v[0], v[1], v[2]);
}

void GraphicsContext3D::vertexAttrib4fv(GLuint index, GLfloat* v)
{
    WEBGL_LOG(("vertexAttrib4fv") << index << v);
    vertexAttrib4f(index, v[0], v[1], v[2], v[3]);
}

void GraphicsContext3D::clear(GLbitfield mask)
{
    WEBGL_LOG(("clear") << mask);
    m_surface->push(makeLambda(glClear)(mask));
}

#define CALL_CREATE_TO_GL(GLType) \
unsigned GraphicsContext3D::create##GLType() \
{ \
    WEBGL_LOG(("create"#GLType)); \
    GLuint o; \
    m_surface->call(makeLambda(gl##Gen##GLType##s)(1, &o)); \
    return o; \
}

CALL_CREATE_TO_GL(Buffer)
CALL_CREATE_TO_GL(Framebuffer)
CALL_CREATE_TO_GL(Renderbuffer)
CALL_CREATE_TO_GL(Texture)

#undef CALL_CREATE_TO_GL

#define PUSH_DELETE_TO_GL(GLType) \
void webglDelete##GLType(GLuint o) \
{ \
    glDelete##GLType##s(1, &o); \
} \
void GraphicsContext3D::delete##GLType(unsigned o) \
{ \
    WEBGL_LOG(("delete"#GLType)); \
    m_surface->push(makeLambda(webglDelete##GLType)(o)); \
}

PUSH_DELETE_TO_GL(Buffer)
// Checking for the caller deleting the WebGL FBO is unneeded. WebGL doesn't support deleting a GL
// object by name, and we protect the WebGL FBO in getIntegerv.
PUSH_DELETE_TO_GL(Framebuffer)
PUSH_DELETE_TO_GL(Renderbuffer)
PUSH_DELETE_TO_GL(Texture)

#undef PUSH_DELETE_TO_GL

#define PUSH_UNIFORM_TO_GL(name, size, GLType) \
static void webglUniform##name(GLint location, GLsizei count, PassOwnArrayPtr<GLType> v) \
{ \
    OwnArrayPtr<GLType> ownV = v; \
    glUniform##name(location, count, ownV.get()); \
} \
void GraphicsContext3D::uniform##name(GLint location, GLType* v, GLsizei count) \
{ \
    WEBGL_LOG(("uniform"#name) << location << v << count); \
    m_surface->push(makeLambda(webglUniform##name)(location, count, WTF::makeLambdaArrayArg(v, size * count))); \
}

PUSH_UNIFORM_TO_GL(1fv, 1, GLfloat)
PUSH_UNIFORM_TO_GL(1iv, 1, GLint)
PUSH_UNIFORM_TO_GL(2fv, 2, GLfloat)
PUSH_UNIFORM_TO_GL(2iv, 2, GLint)
PUSH_UNIFORM_TO_GL(3fv, 3, GLfloat)
PUSH_UNIFORM_TO_GL(3iv, 3, GLint)
PUSH_UNIFORM_TO_GL(4fv, 4, GLfloat)
PUSH_UNIFORM_TO_GL(4iv, 4, GLint)

#undef PUSH_UNIFORM_TO_GL

#define PUSH_UNIFORM_MATRIX_TO_GL(name, size) \
static void webglUniformMatrix##name(GLint location, GLsizei count, GLboolean transpose, PassOwnArrayPtr<GLfloat> v) \
{ \
    OwnArrayPtr<GLfloat> ownV = v; \
    glUniformMatrix##name(location, count, transpose, ownV.get()); \
} \
void GraphicsContext3D::uniformMatrix##name(GLint location, GLboolean transpose, GLfloat* v, GLsizei count) \
{ \
    WEBGL_LOG(("uniformMatrix"#name) << location << transpose << v << count); \
    m_surface->push(makeLambda(webglUniformMatrix##name)(location, count, transpose, WTF::makeLambdaArrayArg(v, size * size * count))); \
}

PUSH_UNIFORM_MATRIX_TO_GL(2fv, 2)
PUSH_UNIFORM_MATRIX_TO_GL(3fv, 3)
PUSH_UNIFORM_MATRIX_TO_GL(4fv, 4)

#undef PUSH_UNIFORM_MATRIX_TO_GL

// Exmaple: ARGLIST5(=,+) becomes "= arg1 + arg2 + arg3 + arg4 + arg5"
// use __VA_ARGS__ instead of "dividerN" so we can pass in a comma
#define ARGLIST0(divider0, ...)
#define ARGLIST1(divider0, ...) ARGLIST0(divider0, __VA_ARGS__) divider0 arg1
#define ARGLIST2(divider0, ...) ARGLIST1(divider0, __VA_ARGS__) __VA_ARGS__ arg2
#define ARGLIST3(divider0, ...) ARGLIST2(divider0, __VA_ARGS__) __VA_ARGS__ arg3
#define ARGLIST4(divider0, ...) ARGLIST3(divider0, __VA_ARGS__) __VA_ARGS__ arg4
#define ARGLIST5(divider0, ...) ARGLIST4(divider0, __VA_ARGS__) __VA_ARGS__ arg5
#define ARGLIST6(divider0, ...) ARGLIST5(divider0, __VA_ARGS__) __VA_ARGS__ arg6
#define ARGLIST7(divider0, ...) ARGLIST6(divider0, __VA_ARGS__) __VA_ARGS__ arg7
#define ARGLIST8(divider0, ...) ARGLIST7(divider0, __VA_ARGS__) __VA_ARGS__ arg8

#define PUSH_TO_GL(argCount, name, glFuncName, params) \
void GraphicsContext3D::name(params) \
{ \
    WEBGL_LOG((#name) ARGLIST##argCount(<< , <<)); \
    m_surface->push(makeLambda(gl##glFuncName)(ARGLIST##argCount(, ,))); \
}

#define CALL_TO_GL(argCount, GLReturnType, name, glFuncName, params) \
GLReturnType GraphicsContext3D::name(params) \
{ \
    WEBGL_LOG((#name) ARGLIST##argCount(<< , <<)); \
    return m_surface->call(makeLambda(gl##glFuncName)(ARGLIST##argCount(, ,))); \
}

#define PARAMS0()
#define PARAMS1(T1) T1 arg1
#define PARAMS2(T1, T2) PARAMS1(T1), T2 arg2
#define PARAMS3(T1, T2, T3) PARAMS2(T1, T2), T3 arg3
#define PARAMS4(T1, T2, T3, T4) PARAMS3(T1, T2, T3), T4 arg4
#define PARAMS5(T1, T2, T3, T4, T5) PARAMS4(T1, T2, T3, T4), T5 arg5
#define PARAMS6(T1, T2, T3, T4, T5, T6) PARAMS5(T1, T2, T3, T4, T5), T6 arg6
#define PARAMS7(T1, T2, T3, T4, T5, T6, T7) PARAMS6(T1, T2, T3, T4, T5, T6), T7 arg7
#define PARAMS8(T1, T2, T3, T4, T5, T6, T7, T8) PARAMS7(T1, T2, T3, T4, T5, T6, T7), T8 arg8

PUSH_TO_GL(2, attachShader, AttachShader, PARAMS2(GLuint, GLuint))
PUSH_TO_GL(2, bindRenderbuffer, BindRenderbuffer, PARAMS2(GLuint, GLuint))
PUSH_TO_GL(2, bindTexture, BindTexture, PARAMS2(GLenum, GLuint))
PUSH_TO_GL(4, blendColor, BlendColor, PARAMS4(GLclampf, GLclampf, GLclampf, GLclampf))
PUSH_TO_GL(1, blendEquation, BlendEquation, PARAMS1(GLenum))
PUSH_TO_GL(2, blendEquationSeparate, BlendEquationSeparate, PARAMS2(GLenum, GLenum))
PUSH_TO_GL(2, blendFunc, BlendFunc, PARAMS2(GLenum, GLenum))
PUSH_TO_GL(4, blendFuncSeparate, BlendFuncSeparate, PARAMS4(GLenum, GLenum, GLenum, GLenum))
PUSH_TO_GL(4, clearColor, ClearColor, PARAMS4(GLclampf, GLclampf, GLclampf, GLclampf))
PUSH_TO_GL(1, clearDepth, ClearDepthf, PARAMS1(GLclampf))
PUSH_TO_GL(1, clearStencil, ClearStencil, PARAMS1(GLint))
PUSH_TO_GL(4, colorMask, ColorMask, PARAMS4(GLboolean, GLboolean, GLboolean, GLboolean))
PUSH_TO_GL(1, cullFace, CullFace, PARAMS1(GLenum))
PUSH_TO_GL(1, depthFunc, DepthFunc, PARAMS1(GLenum))
PUSH_TO_GL(1, depthMask, DepthMask, PARAMS1(GLboolean))
PUSH_TO_GL(2, depthRange, DepthRangef, PARAMS2(GLclampf, GLclampf))
PUSH_TO_GL(2, detachShader, DetachShader, PARAMS2(GLuint, GLuint))
PUSH_TO_GL(1, disable, Disable, PARAMS1(GLenum))
PUSH_TO_GL(1, enable, Enable, PARAMS1(GLenum))
PUSH_TO_GL(1, frontFace, FrontFace, PARAMS1(GLenum))
PUSH_TO_GL(2, hint, Hint, PARAMS2(GLenum, GLenum))
PUSH_TO_GL(1, lineWidth, LineWidth, PARAMS1(GLfloat))
PUSH_TO_GL(1, linkProgram, LinkProgram, PARAMS1(GLuint))
PUSH_TO_GL(2, pixelStorei, PixelStorei, PARAMS2(GLenum, GLint))
PUSH_TO_GL(2, polygonOffset, PolygonOffset, PARAMS2(GLfloat, GLfloat))
PUSH_TO_GL(2, sampleCoverage, SampleCoverage, PARAMS2(GLclampf, GLboolean))
PUSH_TO_GL(4, scissor, Scissor, PARAMS4(GLint, GLint, GLsizei, GLsizei))
PUSH_TO_GL(3, stencilFunc, StencilFunc, PARAMS3(GLenum, GLint, GLuint))
PUSH_TO_GL(4, stencilFuncSeparate, StencilFuncSeparate, PARAMS4(GLenum, GLenum, GLint, GLuint))
PUSH_TO_GL(1, stencilMask, StencilMask, PARAMS1(GLuint))
PUSH_TO_GL(2, stencilMaskSeparate, StencilMaskSeparate, PARAMS2(GLenum, GLuint))
PUSH_TO_GL(3, stencilOp, StencilOp, PARAMS3(GLenum, GLenum, GLenum))
PUSH_TO_GL(4, stencilOpSeparate, StencilOpSeparate, PARAMS4(GLenum, GLenum, GLenum, GLenum))
PUSH_TO_GL(3, texParameterf, TexParameterf, PARAMS3(GLenum, GLenum, GLfloat))
PUSH_TO_GL(3, texParameteri, TexParameteri, PARAMS3(GLenum, GLenum, GLint))
PUSH_TO_GL(2, uniform1f, Uniform1f, PARAMS2(GLint, GLfloat))
PUSH_TO_GL(2, uniform1i, Uniform1i, PARAMS2(GLint, GLint))
PUSH_TO_GL(3, uniform2f, Uniform2f, PARAMS3(GLint, GLfloat, GLfloat))
PUSH_TO_GL(3, uniform2i, Uniform2i, PARAMS3(GLint, GLint, GLint))
PUSH_TO_GL(4, uniform3f, Uniform3f, PARAMS4(GLint, GLfloat, GLfloat, GLfloat))
PUSH_TO_GL(4, uniform3i, Uniform3i, PARAMS4(GLint, GLint, GLint, GLint))
PUSH_TO_GL(5, uniform4f, Uniform4f, PARAMS5(GLint, GLfloat, GLfloat, GLfloat, GLfloat))
PUSH_TO_GL(5, uniform4i, Uniform4i, PARAMS5(GLint, GLint, GLint, GLint, GLint))
PUSH_TO_GL(1, useProgram, UseProgram, PARAMS1(GLuint))
PUSH_TO_GL(1, validateProgram, ValidateProgram, PARAMS1(GLuint))
PUSH_TO_GL(2, vertexAttrib1f, VertexAttrib1f, PARAMS2(GLuint, GLfloat))
PUSH_TO_GL(3, vertexAttrib2f, VertexAttrib2f, PARAMS3(GLuint, GLfloat, GLfloat))
PUSH_TO_GL(4, vertexAttrib3f, VertexAttrib3f, PARAMS4(GLuint, GLfloat, GLfloat, GLfloat))
PUSH_TO_GL(5, vertexAttrib4f, VertexAttrib4f, PARAMS5(GLuint, GLfloat, GLfloat, GLfloat, GLfloat))
PUSH_TO_GL(4, viewport, Viewport, PARAMS4(GLint, GLint, GLsizei, GLsizei))
PUSH_TO_GL(8, copyTexSubImage2D, CopyTexSubImage2D, PARAMS8(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei))
PUSH_TO_GL(1, generateMipmap, GenerateMipmap, PARAMS1(GLenum))
PUSH_TO_GL(2, bindBuffer, BindBuffer, PARAMS2(GLenum, GLuint))
PUSH_TO_GL(1, disableVertexAttribArray, DisableVertexAttribArray, PARAMS1(GLuint))
PUSH_TO_GL(1, enableVertexAttribArray, EnableVertexAttribArray, PARAMS1(GLuint))
PUSH_TO_GL(1, activeTexture, ActiveTexture, PARAMS1(GLenum))
PUSH_TO_GL(1, deleteProgram, DeleteProgram, PARAMS1(GLuint))

CALL_TO_GL(0, void, finish, Finish, PARAMS0())
CALL_TO_GL(0, void, flush, Flush, PARAMS0())
CALL_TO_GL(7, void, readPixels, ReadPixels, PARAMS7(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*))
CALL_TO_GL(4, void, getAttachedShaders, GetAttachedShaders, PARAMS4(GLuint, GLsizei, GLsizei*, GLuint*))
CALL_TO_GL(2, void, getBooleanv, GetBooleanv, PARAMS2(GLenum, GLboolean*))
CALL_TO_GL(3, void, getBufferParameteriv, GetBufferParameteriv, PARAMS3(GLenum, GLenum, GLint*))
CALL_TO_GL(2, void, getFloatv, GetFloatv, PARAMS2(GLenum, GLfloat*))
CALL_TO_GL(3, void, getProgramiv, GetProgramiv, PARAMS3(GLuint, GLenum, GLint*))
CALL_TO_GL(3, void, getRenderbufferParameteriv, GetRenderbufferParameteriv, PARAMS3(GLenum, GLenum, GLint*))
CALL_TO_GL(3, void, getTexParameterfv, GetTexParameterfv, PARAMS3(GLenum, GLenum, GLfloat*))
CALL_TO_GL(3, void, getTexParameteriv, GetTexParameteriv, PARAMS3(GLenum, GLenum, GLint*))
CALL_TO_GL(3, void, getUniformfv, GetUniformfv, PARAMS3(GLuint, GLint, GLfloat*))
CALL_TO_GL(3, void, getUniformiv, GetUniformiv, PARAMS3(GLuint, GLint, GLint*))
CALL_TO_GL(3, void, getVertexAttribfv, GetVertexAttribfv, PARAMS3(GLuint, GLenum, GLfloat*))
CALL_TO_GL(3, void, getVertexAttribiv, GetVertexAttribiv, PARAMS3(GLuint, GLenum, GLint*))
CALL_TO_GL(1, GLboolean, isBuffer, IsBuffer, PARAMS1(GLuint))
CALL_TO_GL(1, GLboolean, isEnabled, IsEnabled, PARAMS1(GLenum))
CALL_TO_GL(1, GLboolean, isFramebuffer, IsFramebuffer, PARAMS1(GLuint))
CALL_TO_GL(1, GLboolean, isProgram, IsProgram, PARAMS1(GLuint))
CALL_TO_GL(1, GLboolean, isRenderbuffer, IsRenderbuffer, PARAMS1(GLuint))
CALL_TO_GL(1, GLboolean, isShader, IsShader, PARAMS1(GLuint))
CALL_TO_GL(1, GLboolean, isTexture, IsTexture, PARAMS1(GLuint))
CALL_TO_GL(0, GLenum, createProgram, CreateProgram, PARAMS0())

} // namespace WebCore

#endif // ENABLE(WEBGL)
