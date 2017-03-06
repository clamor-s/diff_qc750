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
#include "GaneshCanvas.h"

#if USE(ACCELERATED_CANVAS_LAYER)
#include "AcceleratedCanvasLambdas.h"
#include "AndroidProperties.h"
#include "EGLFence.h"
#include "EGLImage.h"
#include "EmojiFont.h"
#include "GLUtils.h"
#include "GrContext.h"
#include "PushReadbackContext.h"
#include "SkGpuDevice.h"

#define LOG_TAG "GaneshCanvas"
#include <cutils/log.h>

#include <wtf/OwnArrayPtr.h>
#include <wtf/PassOwnArrayPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

static const int maxCachedTextures = 256;
static const int maxCachedTextureBytes = 64 * 1024 * 1024;
static const int minGaneshCanvasHeight = 200;
static const int minGaneshCanvasWidth = 200;
static const int minGaneshCanvasArea = 300*300;

class GaneshCanvasBuffer : public EGLImageBuffer {
public:
    // The OpenGL context must be current for these.
    GaneshCanvasBuffer(const IntSize& size)
        : m_textureId(0)
        , m_size(size)
    {
        glGenTextures(1, &m_textureId);
        glBindTexture(GL_TEXTURE_2D, m_textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width(), size.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        m_eglImage = EGLImage::createFromTexture(m_textureId);
    }

    ~GaneshCanvasBuffer()
    {
        ASSERT(!m_textureId);
    }

    GLuint textureId() const { return m_textureId; }
    virtual EGLImage* eglImage() const { return m_eglImage.get(); }
    virtual IntSize size() const { return m_size; }

    virtual void deleteSourceBuffer()
    {
        ASSERT(m_textureId);
        glDeleteTextures(1, &m_textureId);
        m_textureId = 0;
    }

private:
    GLuint m_textureId;
    OwnPtr<EGLImage> m_eglImage;
    IntSize m_size;
};


GaneshCanvas::GaneshCanvas(const IntSize& canvasSize, bool& success)
    : AcceleratedCanvas(canvasSize)
    , m_display(EGL_NO_DISPLAY)
    , m_surface(EGL_NO_SURFACE)
    , m_context(EGL_NO_CONTEXT)
    , m_renderTargetFBO(0)
    , m_stencilBuffer(0)
{
    ASSERT(!size().isEmpty());
    ASSERT(WTF::isMainThread());

    m_workerCanvas = new SkCanvas;

    if (!AndroidProperties::getStringProperty("webkit.canvas.ganesh", "").contains("noparallel")) {
        m_thread = adoptPtr(new Thread("GaneshCanvas"));
        success = m_thread->call(WTF::makeLambda(this, &GaneshCanvas::init)());
    } else
        success = init();
}

GaneshCanvas::~GaneshCanvas()
{
    if (m_thread)
        m_thread->call(WTF::makeLambda(this, &GaneshCanvas::destroy)());
    else
        destroy();
}

bool GaneshCanvas::isSuitableFor(const IntSize& size)
{
    const bool forceGanesh = AndroidProperties::getStringProperty("webkit.canvas.ganesh", "") == "force";

    if (forceGanesh)
        return true;

    return size.height() >= minGaneshCanvasHeight
        && size.width() >= minGaneshCanvasWidth
        && (size.height() * size.width()) >= minGaneshCanvasArea;
}

// FIXME: http://nvbugs/1007696 Race condition (?) causes assert in the driver during destruction.
// The test-case is a harness that creates and deletes huge amount of canvas contexts.
static WTF::Mutex& eglBugMutex()
{
    DEFINE_STATIC_LOCAL(WTF::Mutex, mutex, ());
    return mutex;
}

bool GaneshCanvas::init()
{
    if (m_thread)
        eglBugMutex().lock();

    m_context = GLUtils::createBackgroundContext(EGL_NO_CONTEXT, &m_display, &m_surface);
    if (m_context == EGL_NO_CONTEXT) {
        ALOGV("Initializing Ganesh failed: failed to create an OpenGL context.");

        if (m_thread)
            eglBugMutex().unlock();

        return false;
    }

    m_backBuffer = adoptPtr(new GaneshCanvasBuffer(size()));
    m_backBuffer->lockSurface();
    glGenFramebuffers(1, &m_renderTargetFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_renderTargetFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_backBuffer->textureId(), 0);

    glGenRenderbuffers(1, &m_stencilBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_stencilBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, size().width(), size().height());
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_stencilBuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        ALOGV("Initializing Ganesh failed: glCheckFramebufferStatus() did not return GL_FRAMEBUFFER_COMPLETE.");

        if (m_thread)
            eglBugMutex().unlock();

        return false;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    GrContext* ganeshContext = GrContext::Create(kOpenGL_Shaders_GrEngine, 0);
    ganeshContext->setTextureCacheLimits(maxCachedTextures, maxCachedTextureBytes);

    GrPlatformRenderTargetDesc targetDesc;
    targetDesc.fWidth = size().width();
    targetDesc.fHeight = size().height();
    targetDesc.fConfig = kSkia8888_PM_GrPixelConfig;
    targetDesc.fSampleCnt = 0;
    targetDesc.fStencilBits = 8;
    targetDesc.fRenderTargetHandle = m_renderTargetFBO;
    GrRenderTarget* renderTarget = ganeshContext->createPlatformRenderTarget(targetDesc);

    SkDevice* device = new SkGpuDevice(ganeshContext, renderTarget);
    ganeshContext->unref();
    renderTarget->unref();
    m_workerCanvas->setDevice(device)->unref();

    if (m_thread)
        eglBugMutex().unlock();

    return true;
}

void GaneshCanvas::destroy()
{
#if DEBUG
    assertInGrThread();
#endif

    // If this is not constructed, don't destroy
    if (m_context == EGL_NO_CONTEXT) {
        delete m_workerCanvas;
        return;
    }

    if (m_thread)
        eglBugMutex().lock();

    makeContextCurrent();

    // Delete everybody that uses GL before destroying our context.
    m_workerCanvas->setDevice(0);

    if (m_stencilBuffer)
        glDeleteRenderbuffers(1, &m_stencilBuffer);

    if (m_renderTargetFBO)
        glDeleteFramebuffers(1, &m_renderTargetFBO);

    bufferRing()->surfaceDetached(m_backBuffer.get());

    eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    eglDestroySurface(m_display, m_surface);
    eglDestroyContext(m_display, m_context);

    if (m_thread) {
        eglReleaseThread();
        eglBugMutex().unlock();
    }
}

#if DEBUG
void GaneshCanvas::assertInGrThread()
{
    if (m_thread)
        ASSERT(currentThread() == m_thread->id());
    else
        ASSERT(WTF::isMainThread());
}
#endif

PassOwnPtr<EGLImageBuffer> GaneshCanvas::commitBackBuffer()
{
    OwnPtr<GaneshCanvasBuffer> previousBackBuffer = m_backBuffer.release();
    m_backBuffer = static_pointer_cast<GaneshCanvasBuffer>(bufferRing()->takeFreeBuffer());

    if (m_thread)
        m_thread->callLater(WTF::makeLambda(this, &GaneshCanvas::setupNextBackBufferT)(previousBackBuffer.get()));
    else
        setupNextBackBufferT(previousBackBuffer.get());

    return previousBackBuffer.release();
}

void GaneshCanvas::setupNextBackBufferT(GaneshCanvasBuffer* previousBackBuffer)
{
#if DEBUG
    assertInGrThread();
#endif

    makeContextCurrent();

    m_workerCanvas->flush();
    previousBackBuffer->fence().set();

    GLuint oldTextureBinding, oldFboBinding;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&oldTextureBinding));
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&oldFboBinding));

    if (!m_backBuffer)
        m_backBuffer = adoptPtr(new GaneshCanvasBuffer(size()));
    // m_backBuffer can change as soon as we unlock previousBackBuffer, so we
    // make a local copy now.
    GaneshCanvasBuffer* localBackBuffer = m_backBuffer.get();

    localBackBuffer->lockSurface();
    previousBackBuffer->unlockSurface();

    if (oldFboBinding != m_renderTargetFBO)
        glBindFramebuffer(GL_FRAMEBUFFER, m_renderTargetFBO);

    // Copy the previous backbuffer (attached to m_renderTargetFBO) to localBackBuffer.
    localBackBuffer->fence().finish();
    glBindTexture(GL_TEXTURE_2D, localBackBuffer->textureId());
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, size().width(), size().height());
    glBindTexture(GL_TEXTURE_2D, oldTextureBinding);

    // Point Ganesh's rendering target at localBackBuffer.
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, localBackBuffer->textureId(), 0);

    if (oldFboBinding != m_renderTargetFBO)
        glBindFramebuffer(GL_FRAMEBUFFER, oldFboBinding);
}

void GaneshCanvas::makeContextCurrent()
{
    if (m_thread)
        return;

    ASSERT(m_context != EGL_NO_CONTEXT);
    eglMakeCurrent(m_display, m_surface, m_surface, m_context);
}

void GaneshCanvas::accessDeviceBitmap(SkBitmap* bitmap, bool changePixels)
{
    if (m_thread) {
        // We have to return SkBitmap with no pixel ref. This call cannot be forwarded to the
        // worker thread, because the result of m_workerCanvas->accessBitmap contains a SkBitmapRef
        // which is valid only in the worker thread.
        SkBitmap tmp;
        tmp.setConfig(m_workerCanvas->getDevice()->config(), size().width(), size().height());
        *bitmap = tmp;
        return;
    }
    ASSERT(eglGetCurrentContext() == m_context);
    *bitmap = m_workerCanvas->getDevice()->accessBitmap(changePixels);
}

void GaneshCanvas::writePixels(const SkBitmap& bitmap, int x, int y, SkCanvas::Config8888 config8888)
{
    if (m_thread) {
        m_thread->call(WTF::makeLambda(m_workerCanvas, &SkCanvas::writePixels)(bitmap, x, y, config8888));
        return;
    }
    ASSERT(eglGetCurrentContext() == m_context);
    m_workerCanvas->writePixels(bitmap, x, y, config8888);
}

namespace {
static bool localReadPixels(SkCanvas* canvas, SkBitmap* bitmap, int x, int y, SkCanvas::Config8888 config8888)
{
    return canvas->readPixels(bitmap, x, y, config8888);
}
}

bool GaneshCanvas::readPixels(SkBitmap* bitmap, int x, int y, SkCanvas::Config8888 config8888)
{
    if (m_thread)
        return m_thread->call(WTF::makeLambda(localReadPixels)(m_workerCanvas, bitmap, x, y, config8888));
    ASSERT(eglGetCurrentContext() == m_context);
    return m_workerCanvas->readPixels(bitmap, x, y, config8888);
}

#define MAKE_CALL(func, args) \
    if (m_thread) { \
        m_thread->call(WTF::makeLambda(m_workerCanvas, &SkCanvas::func) args); \
        return; \
    } \
    ASSERT(eglGetCurrentContext() == m_context); \
    m_workerCanvas->func args;

#define MAKE_CALL_LATER(func, args) \
    if (m_thread) { \
        m_thread->callLater(WTF::makeLambda(m_workerCanvas, &SkCanvas::func) args); \
        return; \
    } \
    ASSERT(eglGetCurrentContext() == m_context); \
    m_workerCanvas->func args;

#define MAKE_CALL_LATER_LAMBDA(func, args1, args2) \
    if (m_thread) { \
        m_thread->callLater(new func##Lambda<> args1 ); \
        return; \
    } \
    ASSERT(eglGetCurrentContext() == m_context); \
    m_workerCanvas->func args2;

void GaneshCanvas::save(SkCanvas::SaveFlags flags)
{
    MAKE_CALL_LATER(save, (flags));
}

void GaneshCanvas::saveLayer(const SkRect* bounds, const SkPaint* paint, SkCanvas::SaveFlags flags)
{
    MAKE_CALL_LATER_LAMBDA(saveLayer, (m_workerCanvas, bounds, paint, flags), (bounds, paint, flags));
}

void GaneshCanvas::saveLayerAlpha(const SkRect* bounds, U8CPU alpha, SkCanvas::SaveFlags flags)
{
    if (alpha == 0xFF) {
        saveLayer(bounds, 0, flags);
        return;
    }

    SkPaint tmpPaint;
    tmpPaint.setAlpha(alpha);
    saveLayer(bounds, &tmpPaint, flags);
}

void GaneshCanvas::restore()
{
    MAKE_CALL_LATER(restore, ());
}

void GaneshCanvas::translate(SkScalar dx, SkScalar dy)
{
    MAKE_CALL_LATER(translate, (dx, dy));
}

void GaneshCanvas::scale(SkScalar sx, SkScalar sy)
{
    MAKE_CALL_LATER(scale, (sx, sy));
}

void GaneshCanvas::rotate(SkScalar degrees)
{
    MAKE_CALL_LATER(rotate, (degrees));
}

void GaneshCanvas::concat(const SkMatrix& matrix)
{
    MAKE_CALL_LATER_LAMBDA(concat, (m_workerCanvas, matrix), (matrix));
}

void GaneshCanvas::clipRect(const SkRect& rect, SkRegion::Op op, bool doAntiAlias)
{
    MAKE_CALL_LATER_LAMBDA(clipRect, (m_workerCanvas, rect, op, doAntiAlias), (rect, op, doAntiAlias));
}

void GaneshCanvas::clipPath(const SkPath& path, SkRegion::Op op, bool doAntiAlias)
{
    MAKE_CALL_LATER_LAMBDA(clipPath, (m_workerCanvas, path, op, doAntiAlias), (path, op, doAntiAlias));
}

void GaneshCanvas::drawPoints(SkCanvas::PointMode mode, size_t count, const SkPoint pts[], const SkPaint& paint)
{
    MAKE_CALL(drawPoints, (mode, count, pts, paint));
}

void GaneshCanvas::drawRect(const SkRect& rect, const SkPaint& paint)
{
    MAKE_CALL_LATER_LAMBDA(drawRect, (m_workerCanvas, rect, paint), (rect, paint));
}

void GaneshCanvas::drawPath(const SkPath& path, const SkPaint& paint)
{
    MAKE_CALL_LATER_LAMBDA(drawPath, (m_workerCanvas, path, paint), (path, paint));
}

namespace {
static bool canCopy(const SkBitmap& bitmap)
{
    return bitmap.isNull() || bitmap.pixelRef();
}
}

void GaneshCanvas::drawBitmapRect(const SkBitmap& bitmap, const SkIRect* src, const SkRect& dst, const SkPaint* paint)
{
    if (m_thread) {
        if (canCopy(bitmap))
            m_thread->callLater(new drawBitmapRectLambda<>(m_workerCanvas, bitmap, src, dst, paint));
        else
            m_thread->call(WTF::makeLambda(m_workerCanvas, &SkCanvas::drawBitmapRect)(bitmap, src, dst, paint));
        return;
    }
    ASSERT(eglGetCurrentContext() == m_context);
    m_workerCanvas->drawBitmapRect(bitmap, src, dst, paint);
}

void GaneshCanvas::drawText(const void* text, size_t byteLength, SkScalar x, SkScalar y, const SkPaint& paint)
{
    MAKE_CALL_LATER_LAMBDA(drawText, (m_workerCanvas, text, byteLength, x, y, paint), (text, byteLength, x, y, paint));
}

void GaneshCanvas::drawPosText(const void* text, size_t byteLength, const SkPoint pos[], const SkPaint& paint)
{
    MAKE_CALL_LATER_LAMBDA(drawPosText, (m_workerCanvas, text, byteLength, pos, paint), (text, byteLength, pos, paint));
}

void GaneshCanvas::drawPosTextH(const void* text, size_t byteLength, const SkScalar xpos[], SkScalar constY, const SkPaint& paint)
{
    MAKE_CALL_LATER_LAMBDA(drawPosTextH, (m_workerCanvas, text, byteLength, xpos, constY, paint), (text, byteLength, xpos, constY, paint));
}

void GaneshCanvas::drawTextOnPath(const void* text, size_t byteLength, const SkPath& path, const SkMatrix* matrix, const SkPaint& paint)
{
    MAKE_CALL_LATER_LAMBDA(drawTextOnPath, (m_workerCanvas, text, byteLength, path, matrix, paint), (text, byteLength, path, matrix, paint));
}

void GaneshCanvas::drawLine(SkScalar x0, SkScalar y0, SkScalar x1, SkScalar y1, const SkPaint& paint)
{
    MAKE_CALL_LATER_LAMBDA(drawLine, (m_workerCanvas, x0, y0, x1, y1, paint), (x0, y0, x1, y1, paint));
}

void GaneshCanvas::drawOval(const SkRect& oval, const SkPaint& paint)
{
    MAKE_CALL_LATER_LAMBDA(drawOval, (m_workerCanvas, oval, paint), (oval, paint));
}

void GaneshCanvas::drawEmojiFont(uint16_t index, SkScalar x, SkScalar y, const SkPaint& paint)
{
    if (m_thread) {
        m_thread->call(WTF::makeLambda(&android::EmojiFont::Draw)(m_workerCanvas, index, x, y, paint));
        return;
    }
    ASSERT(eglGetCurrentContext() == m_context);
    android::EmojiFont::Draw(m_workerCanvas, index, x, y, paint);
}

namespace {
static void localGetTotalMatrix(const SkCanvas* canvas, SkMatrix* matrix)
{
    *matrix = canvas->getTotalMatrix();
}
}

const SkMatrix& GaneshCanvas::getTotalMatrix() const
{
    if (m_thread) {
        m_thread->call(WTF::makeLambda(&localGetTotalMatrix)(m_workerCanvas, const_cast<SkMatrix*>(&m_retMatrix)));
        return m_retMatrix;
    }
    return m_workerCanvas->getTotalMatrix();
}

} // namespace WebCore

#endif // USE(ACCELERATED_CANVAS_LAYER)
