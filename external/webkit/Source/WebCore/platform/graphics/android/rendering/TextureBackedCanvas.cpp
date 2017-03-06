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
#include "TextureBackedCanvas.h"

#if USE(ACCELERATED_CANVAS_LAYER)

#include "AcceleratedCanvasLambdas.h"
#include "AndroidProperties.h"
#include "EGLImageLayer.h"
#include "EmojiFont.h"
#include "MappedTexture.h"
#include "GLUtils.h"
#include "MappedTexture.h"
#include "SkDevice.h"

namespace WebCore {

static const int minParallelizableHeight = 200;
static const int minParallelizableWidth = 200;
static const int minParallelizableArea = 300*300;
static const int partitioningThreshold = 300;

static int maxPartitionCount()
{
    static int processorCount = sysconf(_SC_NPROCESSORS_ONLN);
    return processorCount - 1;
}

class FlipDevice : public SkDevice {
public:
    FlipDevice(SkCanvas* canvas, const SkBitmap& bitmap)
        : SkDevice(bitmap)
        , m_pixels(0)
        , m_rowBytes(bitmap.rowBytes())
    { }

    void backBufferChanged(void* pixels, int rowBytes)
    {
        m_pixels = pixels;
        m_rowBytes = rowBytes;
    }

protected:
    const SkBitmap& onAccessBitmap(SkBitmap* bitmap)
    {
        // FIXME: this is not strictly allowed by Skia API. See TextureBackedCanvas.cpp.
        if (bitmap->rowBytes() != m_rowBytes && m_pixels)
            bitmap->setConfig(SkBitmap::kARGB_8888_Config, bitmap->width(), bitmap->height(), m_rowBytes);

        if (bitmap->getPixels() != m_pixels)
            bitmap->setPixels(m_pixels);

        return *bitmap;
    }

private:
    void* m_pixels;
    int m_rowBytes;
};

class MappedCanvasTexture
    : public MappedTexture
    , public EGLImageBuffer {
public:
    MappedCanvasTexture(const IntSize& size)
        : MappedTexture(size)
    {}
    virtual EGLImage* eglImage() const { return MappedTexture::eglImage(); }
    virtual bool canRetainTextures() const { return false; }
    virtual IntSize size() const { return MappedTexture::size(); }
    virtual void mapBuffer(SkBitmap* bitmap)
    {
        const MappedTexture::LockedBuffer* lockedBuffer = lockBuffer();
        if (!lockedBuffer)
            return;

        bitmap->setConfig(SkBitmap::kARGB_8888_Config, size().width(), size().height(), lockedBuffer->rowBytes);
        bitmap->setPixels(lockedBuffer->pixels);
    }
    virtual void unmapBuffer() { unlockBuffer(); }
};


TextureBackedCanvas::TextureBackedCanvas(const IntSize& sz, bool& success)
    : AcceleratedCanvas(sz)
    , m_partitionCount(0)
    , m_backBufferLocked(false)
    , m_saveLayerCount(0)
    , m_hasScheduledWork(false)
{
    ASSERT(!size().isEmpty());
    success = false;

    // Create back buffer here because bitmap config cannot change in onAccessBitmap.
    // FIXME: currently we assume that the rowBytes is purely function of bitmap width.
    // This should be true for now, but in the future we could want to have an
    // GraphicsBuffer flag for expressing this constraint.
    m_backBuffer.set(new MappedCanvasTexture(size()));
    const MappedTexture::LockedBuffer* buffer = m_backBuffer->lockBuffer();
    if (!buffer)
        return;

    SkBitmap bitmap;
    bitmap.setConfig(SkBitmap::kARGB_8888_Config, size().width(), size().height(), buffer->rowBytes);
    bitmap.setPixels(buffer->pixels);
    bitmap.eraseColor(0);

    m_initialRowBytes = buffer->rowBytes;

    // The m_backBufferLocked is intentionally left false, so that the backBufferChanged() would
    // be called upon first bitmap access.

    m_mainCanvas = new SkCanvas(bitmap);
    SkDevice* device = new FlipDevice(m_mainCanvas.get(), bitmap);
    m_mainCanvas->setDevice(device)->unref();

    if (shouldCreatePartitions())
        createPartitions();

    success = true;
}

TextureBackedCanvas::~TextureBackedCanvas()
{
    // Make sure there are no latent references to objects that will get deleted.
    flushDrawing();

    if (m_canvases) {
        for (size_t i = 0; i < m_partitionCount; ++i)
            delete m_canvases[i];
    }

    bufferRing()->surfaceDetached(m_backBuffer.get());
}

bool TextureBackedCanvas::shouldCreatePartitions() const
{
    if (AndroidProperties::getStringProperty("webkit.canvas.texture", "") == "noparallel")
        return false;

    if (AndroidProperties::getStringProperty("webkit.canvas.texture", "") == "forceparallel")
        return true;

    if (maxPartitionCount() <= 1)
        return false;

    return size().height() >= minParallelizableHeight
        && size().width() >= minParallelizableWidth
        && (size().height() * size().width()) >= minParallelizableArea;
}

void TextureBackedCanvas::createPartitions()
{
    const IntSize sz = size();
    const bool partitionAxisIsY = sz.height() >= sz.width();
    const int partitionAxisLength = partitionAxisIsY ? sz.height() : sz.width();

    m_partitionCount = std::max(1, std::min(partitionAxisLength / partitioningThreshold + 1, maxPartitionCount()));
    m_canvases = adoptArrayPtr(new SkCanvas*[m_partitionCount]);
    m_jobs = adoptArrayPtr(new WTF::DelegateThread<ThreadQueueCapacity>[m_partitionCount]);

    const size_t lastPartition = m_partitionCount - 1;
    const size_t partitionLength = partitionAxisLength / m_partitionCount;
    const size_t lastPartitionLength = partitionAxisLength - lastPartition * partitionLength;

    for (size_t i = 0; i < m_partitionCount; ++i) {
        size_t start = i * partitionLength;
        size_t length = partitionLength;
        if (i == lastPartition)
            length = lastPartitionLength;

        SkCanvas* canvas = SkNEW(SkCanvas);
        SkBitmap bitmap;
        bitmap.setConfig(SkBitmap::kARGB_8888_Config, sz.width(), sz.height(), initialRowBytes());

        SkDevice* device = new FlipDevice(canvas, bitmap);
        canvas->setDevice(device)->unref();

        SkRect clipRect;
        if (partitionAxisIsY)
            clipRect = SkRect::MakeXYWH(0, start, sz.width(), length);
        else
            clipRect = SkRect::MakeXYWH(start, 0, length, sz.height());

        canvas->clipRect(clipRect);
        m_canvases[i] = canvas;
    }
}

void TextureBackedCanvas::prepareForDrawing()
{
    if (m_backBufferLocked)
        return;

    SkBitmap bitmap;
    if (lockBackBuffer(&bitmap))
        backBufferChanged(bitmap.getPixels(), bitmap.rowBytes());
}

void TextureBackedCanvas::syncSoftwareCanvas()
{
    prepareForDrawing();
}

void TextureBackedCanvas::backBufferChanged(void* pixels, int rowBytes)
{
    // This will be called after flip has happened, before any job has been scheduled.
    for (unsigned i = 0; i < m_partitionCount; ++i)
        static_cast<FlipDevice*>(m_canvases[i]->getDevice())->backBufferChanged(pixels, rowBytes);
    static_cast<FlipDevice*>(m_mainCanvas->getDevice())->backBufferChanged(pixels, rowBytes);
}

// Returns true if the back buffer changed.
bool TextureBackedCanvas::lockBackBuffer(SkBitmap* bitmap)
{
    bool wasLocked = m_backBufferLocked;
    const MappedTexture::LockedBuffer* buffer = m_backBuffer->lockBuffer();

    if (buffer) {
        // FIXME: Changing the configuration of the bitmap is not strictly allowed by Skia API. This
        // function will be called from SkDevice::onAccessBitmap(). However, this is the only way to
        // change the rowBytes. The rowBytes might change between the locks or differ in front and
        // back buffer.  It is unclear what this can cause.
        if (bitmap->rowBytes() != buffer->rowBytes)
            bitmap->setConfig(SkBitmap::kARGB_8888_Config, size().width(), size().height(), buffer->rowBytes);

        if (bitmap->getPixels() != buffer->pixels)
            bitmap->setPixels(buffer->pixels);
    } else {
        // FIXME: this might not be valid as per Skia API. One would need to implement a fallback here.
        bitmap->setPixels(0);
    }

    m_backBufferLocked = true;
    return !wasLocked;
}

void TextureBackedCanvas::syncContents()
{
    // If back buffer is not locked, nothing has been drawn to the canvas. There is nothing to commit.
    if (!m_backBufferLocked)
        return;

    flushDrawing();

    m_backBuffer->unlockBuffer();
    m_backBufferLocked = false;

    OwnPtr<MappedCanvasTexture> nextBackBuffer =
        static_pointer_cast<MappedCanvasTexture>(bufferRing()->takeBufferAndLock());
    if (!nextBackBuffer)
        nextBackBuffer.set(new MappedCanvasTexture(size()));

    nextBackBuffer->lockSurface();
    nextBackBuffer->fence().finish();
    m_backBuffer->copyTo(nextBackBuffer.get());

    m_backBuffer->unlockSurface();
    bufferRing()->submitBufferAndUnlock(m_backBuffer.release());
    m_backBuffer = nextBackBuffer.release();
}

void TextureBackedCanvas::flushDrawing()
{
    if (!m_hasScheduledWork)
        return;

    for (size_t i = 0; i < m_partitionCount; ++i)
        m_jobs[i].finish();
    m_hasScheduledWork = false;
}

void TextureBackedCanvas::accessDeviceBitmap(SkBitmap* bm, bool changePixels)
{
    ASSERT(m_backBufferLocked); // Caller has called syncSoftwareCanvas().

    SkBitmap bitmap;
    if (lockBackBuffer(&bitmap))
        backBufferChanged(bitmap.getPixels(), bitmap.rowBytes());
    else
        flushDrawing();

    *bm = bitmap;
}

void TextureBackedCanvas::writePixels(const SkBitmap& bitmap, int x, int y, SkCanvas::Config8888 config8888)
{
    ASSERT(m_backBufferLocked); // caller has called syncSoftwareCanvas().
    flushDrawing();
    m_mainCanvas->writePixels(bitmap, x, y, config8888);
}

bool TextureBackedCanvas::readPixels(SkBitmap* bitmap, int x, int y, SkCanvas::Config8888 config8888)
{
    ASSERT(m_backBufferLocked); // Caller has called accessDeviceBitmap().
    ASSERT(!m_hasScheduledWork); // Caller has called accessDeviceBitmap().
    return m_mainCanvas->readPixels(bitmap, x, y, config8888);
}

#define MAKE_CALL_LATER_LAMBDA(func, args1, args2) \
    if (canParallelize()) { \
        for (size_t i = 0; i < m_partitionCount; ++i) \
            scheduleWork(m_jobs[i], new func##Lambda<LambdaAutoSync> args1); \
    } else {                                                            \
        flushDrawing(); \
        m_mainCanvas->func args2;                       \
    }

#define MAKE_CALL_LATER_LAMBDA_IF(condition, func, args1, args2) \
    if (condition) { \
        for (size_t i = 0; i < m_partitionCount; ++i) \
            scheduleWork(m_jobs[i], new func##Lambda<LambdaAutoSync> args1); \
    } else {                                                            \
        flushDrawing(); \
        m_mainCanvas->func args2;                       \
    }

void TextureBackedCanvas::save(SkCanvas::SaveFlags flags)
{
    if (canParallelize()) {
        for (size_t i = 0; i < m_partitionCount; ++i)
            scheduleWork(m_jobs[i], WTF::makeLambda(m_canvases[i], &SkCanvas::save)(flags));
    }
    m_mainCanvas->save(flags);
}

void TextureBackedCanvas::saveLayer(const SkRect* bounds, const SkPaint* paint, SkCanvas::SaveFlags flags)
{
    // We cannot parallelize saveLayer. If we did, and then ran into an operation that could not be
    // parallelized, then we would need to paint that to the m_mainCanvas. The m_mainCanvas cannot
    // contain a layer if the worker canvases already have layers.

    flushDrawing();
    int saveCount = m_mainCanvas->saveLayer(bounds, paint, flags);
    if (!m_saveLayerCount)
        m_saveLayerCount = saveCount;
}

void TextureBackedCanvas::saveLayerAlpha(const SkRect* bounds, U8CPU alpha, SkCanvas::SaveFlags flags)
{
    if (alpha == 0xFF) {
        saveLayer(bounds, 0, flags);
        return;
    }

    SkPaint tmpPaint;
    tmpPaint.setAlpha(alpha);
    saveLayer(bounds, &tmpPaint, flags);
}

void TextureBackedCanvas::restore()
{
    if (m_saveLayerCount && m_saveLayerCount == m_mainCanvas->getSaveCount() - 1) {
        ASSERT(!canParallelize());

        m_mainCanvas->restore();
        m_saveLayerCount = 0;
        return;
    }

    if (canParallelize()) {
        for (size_t i = 0; i < m_partitionCount; ++i)
            scheduleWork(m_jobs[i], WTF::makeLambda(m_canvases[i], &SkCanvas::restore)());
    }

    m_mainCanvas->restore();
}

void TextureBackedCanvas::translate(SkScalar dx, SkScalar dy)
{
    if (canParallelize()) {
        for (size_t i = 0; i < m_partitionCount; ++i)
            scheduleWork(m_jobs[i], WTF::makeLambda(m_canvases[i], &SkCanvas::translate)(dx, dy));
    }
    m_mainCanvas->translate(dx, dy);
}

void TextureBackedCanvas::scale(SkScalar sx, SkScalar sy)
{
    if (canParallelize()) {
        for (size_t i = 0; i < m_partitionCount; ++i)
            scheduleWork(m_jobs[i], WTF::makeLambda(m_canvases[i], &SkCanvas::scale)(sx, sy));
    }
    m_mainCanvas->scale(sx, sy);
}

void TextureBackedCanvas::rotate(SkScalar degrees)
{
    if (canParallelize()) {
        for (size_t i = 0; i < m_partitionCount; ++i)
            scheduleWork(m_jobs[i], WTF::makeLambda(m_canvases[i], &SkCanvas::rotate)(degrees));
    }
    m_mainCanvas->rotate(degrees);
}

void TextureBackedCanvas::concat(const SkMatrix& matrix)
{
    if (canParallelize()) {
        for (size_t i = 0; i < m_partitionCount; ++i)
            scheduleWork(m_jobs[i], new concatLambda<LambdaAutoSync>(m_canvases[i], matrix));
    }
    m_mainCanvas->concat(matrix);
}

void TextureBackedCanvas::clipRect(const SkRect& rect, SkRegion::Op op, bool doAntiAlias)
{
    if (canParallelize()) {
        for (size_t i = 0; i < m_partitionCount; ++i)
            scheduleWork(m_jobs[i], new clipRectLambda<LambdaAutoSync>(m_canvases[i], rect, op, doAntiAlias));
    }
    m_mainCanvas->clipRect(rect, op, doAntiAlias);
}


void TextureBackedCanvas::clipPath(const SkPath& path, SkRegion::Op op, bool doAntiAlias)
{
    if (canParallelize()) {
        for (size_t i = 0; i < m_partitionCount; ++i)
            scheduleWork(m_jobs[i], new clipPathLambda<LambdaAutoSync>(m_canvases[i], path, op, doAntiAlias));
    }
    m_mainCanvas->clipPath(path, op, doAntiAlias);
}

void TextureBackedCanvas::drawPoints(SkCanvas::PointMode mode, size_t count, const SkPoint pts[], const SkPaint& paint)
{
    MAKE_CALL_LATER_LAMBDA_IF(canParallelize(paint), drawPoints
                              , (m_canvases[i], mode, count, pts, paint, lockFor(paint))
                              , (mode, count, pts, paint));
}

void TextureBackedCanvas::drawRect(const SkRect& rect, const SkPaint& paint)
{
    MAKE_CALL_LATER_LAMBDA_IF(canParallelize(paint), drawRect
                              , (m_canvases[i], rect, paint, lockFor(paint))
                              , (rect, paint));
}

void TextureBackedCanvas::drawPath(const SkPath& path, const SkPaint& paint)
{
    MAKE_CALL_LATER_LAMBDA_IF(canParallelize(paint), drawPath
                              , (m_canvases[i], path, paint, lockFor(paint))
                              , (path, paint));
}

namespace {
static bool canCopy(const SkBitmap& bitmap)
{
    return bitmap.isNull() || bitmap.pixelRef();
}
}

void TextureBackedCanvas::drawBitmapRect(const SkBitmap& bitmap, const SkIRect* src, const SkRect& dst, const SkPaint* paint)
{
    MAKE_CALL_LATER_LAMBDA_IF(canCopy(bitmap) && canParallelize(paint)
                              , drawBitmapRect
                              , (m_canvases[i], bitmap, src, dst, paint, lockFor(paint))
                              , (bitmap, src, dst, paint));
}

void TextureBackedCanvas::drawText(const void* text, size_t byteLength, SkScalar x, SkScalar y, const SkPaint& paint)
{
    MAKE_CALL_LATER_LAMBDA_IF(canParallelize(paint), drawText
                              , (m_canvases[i], text, byteLength, x, y, paint, lockFor(paint))
                              , (text, byteLength, x, y, paint));
}

void TextureBackedCanvas::drawPosText(const void* text, size_t byteLength, const SkPoint pos[], const SkPaint& paint)
{
    MAKE_CALL_LATER_LAMBDA_IF(canParallelize(paint), drawPosText
                              , (m_canvases[i], text, byteLength, pos, paint, lockFor(paint))
                              , (text, byteLength, pos, paint));
}

void TextureBackedCanvas::drawPosTextH(const void* text, size_t byteLength, const SkScalar xpos[], SkScalar constY, const SkPaint& paint)
{
    MAKE_CALL_LATER_LAMBDA_IF(canParallelize(paint), drawPosTextH
                              , (m_canvases[i], text, byteLength, xpos, constY, paint, lockFor(paint))
                              , (text, byteLength, xpos, constY, paint));
}

void TextureBackedCanvas::drawTextOnPath(const void* text, size_t byteLength, const SkPath& path, const SkMatrix* matrix, const SkPaint& paint)
{
    MAKE_CALL_LATER_LAMBDA_IF(canParallelize(paint), drawTextOnPath
                              , (m_canvases[i], text, byteLength, path, matrix, paint, lockFor(paint))
                              , (text, byteLength, path, matrix, paint));
}

void TextureBackedCanvas::drawLine(SkScalar x0, SkScalar y0, SkScalar x1, SkScalar y1, const SkPaint& paint)
{
    MAKE_CALL_LATER_LAMBDA_IF(canParallelize(paint), drawLine
                              , (m_canvases[i], x0, y0, x1, y1, paint, lockFor(paint))
                              , (x0, y0, x1, y1, paint));
}

void TextureBackedCanvas::drawOval(const SkRect& oval, const SkPaint& paint)
{
    MAKE_CALL_LATER_LAMBDA_IF(canParallelize(paint), drawOval
                              , (m_canvases[i], oval, paint, lockFor(paint))
                              , (oval, paint));
}

void TextureBackedCanvas::drawEmojiFont(uint16_t index, SkScalar x, SkScalar y, const SkPaint& paint)
{
    flushDrawing();
    android::EmojiFont::Draw(m_mainCanvas.get(), index, x, y, paint);
}

const SkMatrix& TextureBackedCanvas::getTotalMatrix() const
{
    return m_mainCanvas->getTotalMatrix();
}


} // namespace WebCore

#endif // USE(ACCELERATED_CANVAS_LAYER)
