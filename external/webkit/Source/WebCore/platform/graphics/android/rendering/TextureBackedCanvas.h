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

#ifndef TextureBackedCanvas_h
#define TextureBackedCanvas_h

#if USE(ACCELERATED_CANVAS_LAYER)

#include "AcceleratedCanvas.h"
#include <wtf/DelegateThread.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/OwnPtr.h>
#include <wtf/OwnPtrCommon.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Threading.h>

namespace WebCore {
class GraphicsContext;
class MappedCanvasTexture;

inline bool isThreadSafe(const SkPaint& p)
{
    // Of complex objects contained in SkPaint:
    // All subclasses of SkXfermode implemented in Skia sources seem to be thread-safe.
    // SkTypeface is thread-safe.
    return !(p.getPathEffect() || p.getShader() || p.getMaskFilter() || p.getColorFilter() || p.getRasterizer() || p.getLooper());
}

class TextureBackedCanvas : public AcceleratedCanvas {
public:
    static const size_t ThreadQueueCapacity = 2048;
    typedef WTF::DelegateThread<ThreadQueueCapacity> Thread;

    static PassRefPtr<TextureBackedCanvas> create(const IntSize& size)
    {
        bool success = false;
        TextureBackedCanvas* canvas = new TextureBackedCanvas(size, success);
        if (!success) {
            delete canvas;
            return 0;
        }
        return adoptRef(canvas);
    }
    ~TextureBackedCanvas();

    // Accelerated canvas methods.
    virtual void accessDeviceBitmap(SkBitmap*, bool changePixels);
    virtual void writePixels(const SkBitmap&, int x, int y, SkCanvas::Config8888);
    virtual bool readPixels(SkBitmap*, int x, int y, SkCanvas::Config8888);

#define DECLARE_FWD_FUNCTION(NAME, PARAMS, CALL_ARGS) \
    virtual void NAME PARAMS;

    FOR_EACH_GFX_CTX_VOID_FUNCTION(DECLARE_FWD_FUNCTION)

#undef DECLARE_FWD_FUNCTION

    virtual void drawEmojiFont(uint16_t index, SkScalar x, SkScalar y, const SkPaint&);
    virtual const SkMatrix& getTotalMatrix() const;
    virtual void syncContents();
    virtual void prepareForDrawing();
    virtual void syncSoftwareCanvas();

private:
    TextureBackedCanvas(const IntSize&, bool& success);
    void backBufferChanged(void* pixels, int rowBytes);
    void flushDrawing();

    void updateBackBufferIfNeeded();
    int initialRowBytes() const { return m_initialRowBytes; }

    bool shouldCreatePartitions() const;
    void createPartitions();

    bool lockBackBuffer(SkBitmap* target);

    // We cannot parallelize paints that contain loopers, because the SkBlurDrawLooper results will
    // differ depending on whether the blur intersects a clip region or not.
    bool canParallelize(const SkPaint& paint) const
    {
        return canParallelize() && !paint.getLooper();
    }

    bool canParallelize(const SkPaint* paint) const
    {
        return canParallelize() && (!paint || !paint->getLooper());
    }

    bool canParallelize() const { return m_partitionCount && !m_saveLayerCount; }

    WTF::Mutex* lockFor(const SkPaint& paint) { return !isThreadSafe(paint) ? &m_paintMutex : 0; }
    WTF::Mutex* lockFor(const SkPaint* paint) { return paint ? lockFor(*paint) : 0; }

    void scheduleWork(Thread& thread, PassOwnPtr<WTF::Lambda> operation)
    {
        static const size_t minJobsToWakeThread = 8;
        thread.callLater(operation, minJobsToWakeThread);
        m_hasScheduledWork = true;
    }

    OwnArrayPtr<Thread> m_jobs;
    OwnPtr<SkCanvas> m_mainCanvas;
    OwnArrayPtr<SkCanvas*> m_canvases;
    WTF::Mutex m_paintMutex;
    OwnPtr<MappedCanvasTexture> m_backBuffer;

    size_t m_partitionCount;
    bool m_backBufferLocked;
    int m_initialRowBytes;
    int m_saveLayerCount;
    bool m_hasScheduledWork;
};

} // namespace WebCore

#endif // USE(ACCELERATED_CANVAS_LAYER)

#endif // TextureBackedCanvas_h
