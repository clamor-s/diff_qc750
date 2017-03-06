/*
 * Copyright (C) 2011, 2012, NVIDIA CORPORATION
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
#include "EGLImageLayer.h"

#if USE(ACCELERATED_COMPOSITING)

#include "DrawQuadData.h"
#include "EGLImage.h"
#include "FPSTimer.h"
#include "LayerContent.h"
#include "SkCanvas.h"
#include "SkPaint.h"
#include "TilesManager.h"
#include "Timer.h"
#include "WebViewCore.h"
#include <JNIUtility.h>
#include <wtf/DelegateThread.h>

namespace WebCore {

static void attachCurrentThreadToJavaVM()
{
    JavaVM* jvm = JSC::Bindings::getJavaVM();
    JNIEnv* env = JSC::Bindings::getJNIEnv();
    jvm->AttachCurrentThread(&env, 0);
}

static void detachCurrentThreadFromJavaVM()
{
    JavaVM* jvm = JSC::Bindings::getJavaVM();
    jvm->DetachCurrentThread();
}


EGLImageLayer::EGLImageLayer(RefPtr<EGLImageSurface> surface, const char* name)
    : LayerAndroid(reinterpret_cast<RenderLayer*>(0))
    , m_surface(surface)
    , m_fpsTimer(FPSTimer::createIfNeeded(name))
    , m_isInverted(surface->isInverted())
    , m_hasAlpha(surface->hasAlpha())
    , m_hasPremultipliedAlpha(surface->hasPremultipliedAlpha())
{
}

EGLImageLayer::EGLImageLayer(const EGLImageLayer& layer)
    : LayerAndroid(layer)
    , m_bufferRing(layer.m_surface->bufferRing())
    , m_isInverted(layer.m_isInverted)
    , m_hasAlpha(layer.m_hasAlpha)
    , m_hasPremultipliedAlpha(layer.m_hasPremultipliedAlpha)
{
    m_bufferRing->referenceFrontTexture();
}

EGLImageLayer::~EGLImageLayer()
{
    if (m_fenceWaitThread) {
        // Finish off the thread before we try to delete or release anything.
        m_fenceWaitThread->callLater(WTF::makeLambda(detachCurrentThreadFromJavaVM)());
        m_fenceWaitThread.clear();
    }

    if (m_bufferRing)
        m_bufferRing->releaseFrontTexture();
}

bool EGLImageLayer::needsBlendingLayer(float opacity) const
{
    if (opacity == 1)
        return false;

    if (!content() || content()->isEmpty())
        return false;

    return true;
}

bool EGLImageLayer::drawGL(bool layerTilesDisabled)
{
    EGLImageBuffer* frontTextureBuffer;
    if (GLuint textureID = m_bufferRing->beginReadingFrontBuffer(&frontTextureBuffer)) {
        SkRect geometry = SkRect::MakeSize(getSize());
        if (m_isInverted)
            std::swap(geometry.fTop, geometry.fBottom);

        if (m_hasAlpha && !m_hasPremultipliedAlpha)
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        TextureQuadData data(textureID, GL_TEXTURE_2D, GL_LINEAR, LayerQuad,
                             drawTransform(), &geometry, drawOpacity(), m_hasAlpha);
        TilesManager::instance()->shader()->drawQuad(&data);

        m_bufferRing->didFinishReading(frontTextureBuffer);

        if (m_hasAlpha && !m_hasPremultipliedAlpha)
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

    return LayerAndroid::drawGL(layerTilesDisabled);
}

void EGLImageLayer::onDraw(SkCanvas* canvas, SkScalar opacity, android::DrawExtra* extra, PaintStyle style)
{
    bool usingBlendingLayer = needsBlendingLayer(opacity);
    if (usingBlendingLayer) {
        const SkRect layerBounds = SkRect::MakeSize(getSize());
        canvas->saveLayerAlpha(&layerBounds, SkScalarRound(opacity * 255));
        opacity = SK_Scalar1;
    }

    LayerAndroid::onDraw(canvas, opacity, extra, style);

    SkBitmap bitmap;
    bool premultiplyAlpha = m_hasAlpha && !m_hasPremultipliedAlpha;
    if (m_bufferRing->mapFrontBuffer(&bitmap, premultiplyAlpha)) {
        const int surfaceOpacity = SkScalarRound(opacity * 255);
        SkPaint paint;
        if (surfaceOpacity < 255)
            paint.setAlpha(surfaceOpacity);

        // Draw the bitmap onto the screen upside-down.
        SkRect sourceRect = SkRect::MakeWH(bitmap.width(), bitmap.height());
        SkRect destRect = SkRect::MakeSize(getSize());
        SkMatrix matrix;
        matrix.setRectToRect(sourceRect, destRect, SkMatrix::kFill_ScaleToFit);
        if (m_isInverted) {
            matrix.postScale(1, -1);
            matrix.postTranslate(0, destRect.height());
        }
        canvas->drawBitmapMatrix(bitmap, matrix, &paint);

        m_bufferRing->unmapFrontBuffer();
    }

    if (usingBlendingLayer)
        canvas->restore();
}

void EGLImageLayer::didAttachToView(android::WebViewCore* webViewCore)
{
    if (!m_surface->requiresFenceWaitAfterPainting())
        return;
    m_webViewCore = webViewCore;
}

void EGLImageLayer::didDetachFromView()
{
    if (m_fenceWaitThread)
        m_fenceWaitThread->finish();
    m_webViewCore = 0;
}

bool EGLImageLayer::handleNeedsDisplay()
{
    if (!m_surface->requiresFenceWaitAfterPainting())
        return false;

    if (!m_syncTimer)
        m_syncTimer = new Timer<EGLImageLayer>(this, &EGLImageLayer::commitBackBuffer);

    if (!m_syncTimer->isActive())
        m_syncTimer->startOneShot(0);

    return true;
}

void EGLImageLayer::commitBackBuffer(Timer<EGLImageLayer>*)
{
    if (!m_webViewCore)
        return;

    if (!m_fenceWaitThread) {
        m_fenceWaitThread.set(new FenceWaitThread("FenceWait"));
        m_fenceWaitThread->callLater(WTF::makeLambda(attachCurrentThreadToJavaVM)());
    } else {
        // Ensure the previous buffer has finished and is back in the ring
        // before EGLImageSurface::commitBackBuffer tries to grab another one.
        m_fenceWaitThread->finish();
    }

    if (OwnPtr<EGLImageBuffer> buffer = m_surface->commitBackBuffer())
        m_fenceWaitThread->callLater(WTF::makeLambda(this, &EGLImageLayer::submitBufferForComposition)(m_surface->bufferRing(), buffer.release()));
}

void EGLImageLayer::submitBufferForComposition(PassRefPtr<EGLImageBufferRing> bufferRing, PassOwnPtr<EGLImageBuffer> buffer)
{
    buffer->lockSurface();
    buffer->fence().finish();
    buffer->unlockSurface();
    bufferRing->submitBuffer(buffer);
    m_webViewCore->viewInvalidateLayer(uniqueId());

    if (m_fpsTimer)
        m_fpsTimer->frameComplete();
}

void EGLImageLayer::syncContents()
{
    if (OwnPtr<EGLImageBuffer> buffer = m_surface->commitBackBuffer()) {
        buffer->lockSurface();
        buffer->fence().finish();
        buffer->unlockSurface();
        m_surface->bufferRing()->submitBuffer(buffer.release());
    } else
        m_surface->syncContents();

    if (m_fpsTimer)
        m_fpsTimer->frameComplete();
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
