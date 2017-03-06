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
#ifndef AcceleratedCanvasLambdas_h
#define AcceleratedCanvasLambdas_h

#include <wtf/Lambda.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/Threading.h>

namespace WebCore {

class LambdaAutoSync {
public:
    typedef WTF::Mutex* Type;
    static const int defaultValue = 0;

    LambdaAutoSync(WTF::Mutex* mutex)
        : m_mutex(mutex)
    {
        if (m_mutex)
            m_mutex->lock();
    }

    ~LambdaAutoSync()
    {
        if (m_mutex)
            m_mutex->unlock();
    }

private:
    WTF::Mutex* m_mutex;
};

class LambdaNoSync {
public:
    typedef bool Type;
    static const bool defaultValue = false;

    LambdaNoSync(bool)
    {
    }
};


template<typename T>
class OptionalCopy {
public:
    OptionalCopy(const T* value)
        : m_value(value ? *value : T())
        , m_exists(value)
    {
    }

    T* ptr() { return m_exists ?  &m_value : 0; }
    const T* ptr() const { return m_exists ?  &m_value : 0; }

private:
    T m_value;
    bool m_exists;
};

template<typename Locker = LambdaNoSync>
class saveLayerLambda : public WTF::Lambda {
public:
    saveLayerLambda(SkCanvas* canvas, const SkRect* bounds, const SkPaint* paint, SkCanvas::SaveFlags flags, typename Locker::Type locker = Locker::defaultValue)
        : m_canvas(canvas)
        , m_bounds(bounds)
        , m_paint(paint)
        , m_flags(flags)
        , m_locker(locker)
    {
    }

    void call() { Locker lock(m_locker); m_canvas->saveLayer(m_bounds.ptr(), m_paint.ptr(), m_flags); }

private:
    SkCanvas* m_canvas;
    OptionalCopy<SkRect> m_bounds;
    OptionalCopy<SkPaint> m_paint;
    SkCanvas::SaveFlags m_flags;
    typename Locker::Type m_locker;
};

template<typename Locker = LambdaNoSync>
class concatLambda : public WTF::Lambda {
public:
    concatLambda(SkCanvas* canvas, const SkMatrix& matrix)
        : m_canvas(canvas)
        , m_matrix(matrix)
    {
    }

    void call() { m_canvas->concat(m_matrix); }

private:
    SkCanvas* m_canvas;
    SkMatrix m_matrix;
};

template<typename Locker = LambdaNoSync>
class clipRectLambda : public WTF::Lambda {
public:
    clipRectLambda(SkCanvas* canvas, const SkRect& rect, SkRegion::Op op, bool doAntiAlias)
        : m_canvas(canvas)
        , m_rect(rect)
        , m_op(op)
        , m_doAntiAlias(doAntiAlias)
    {
    }

    void call() { m_canvas->clipRect(m_rect, m_op, m_doAntiAlias); }

private:
    SkCanvas* m_canvas;
    SkRect m_rect;
    SkRegion::Op m_op;
    bool m_doAntiAlias;
};

template<typename Locker = LambdaNoSync>
class clipPathLambda : public WTF::Lambda {
public:
    clipPathLambda(SkCanvas* canvas, const SkPath& path, SkRegion::Op op, bool doAntiAlias)
        : m_canvas(canvas)
        , m_path(path)
        , m_op(op)
        , m_doAntiAlias(doAntiAlias)
    { }

    void call() { m_canvas->clipPath(m_path, m_op, m_doAntiAlias); }

private:
    SkCanvas* m_canvas;
    SkPath m_path;
    SkRegion::Op m_op;
    bool m_doAntiAlias;
};

template<typename Locker = LambdaNoSync>
class drawPointsLambda : public WTF::Lambda {
public:
    drawPointsLambda(SkCanvas* canvas, SkCanvas::PointMode mode, size_t count, const SkPoint pts[], const SkPaint& paint, typename Locker::Type locker = Locker::defaultValue)
        : m_canvas(canvas)
        , m_mode(mode)
        , m_count(count)
        , m_pts(adoptArrayPtr(pts ? new SkPoint[count] : 0))
        , m_paint(paint)
        , m_locker(locker)
    {
        if (m_pts)
            memcpy(m_pts.get(), pts, count * sizeof(pts[0]));
    }

    void call() { Locker lock(m_locker); m_canvas->drawPoints(m_mode, m_count, m_pts.get(), m_paint); }

private:
    SkCanvas* m_canvas;
    SkCanvas::PointMode m_mode;
    size_t m_count;
    OwnArrayPtr<SkPoint> m_pts;
    SkPaint m_paint;
    typename Locker::Type m_locker;
};

template<typename Locker = LambdaNoSync>
class drawRectLambda : public WTF::Lambda {
public:
    drawRectLambda(SkCanvas* canvas, const SkRect& rect, const SkPaint& paint, typename Locker::Type locker = Locker::defaultValue)
        : m_canvas(canvas)
        , m_rect(rect)
        , m_paint(paint)
        , m_locker(locker)
    { }

    void call() { Locker lock(m_locker); m_canvas->drawRect(m_rect, m_paint); }

private:
    SkCanvas* m_canvas;
    SkRect m_rect;
    SkPaint m_paint;
    typename Locker::Type m_locker;
};

template<typename Locker = LambdaNoSync>
class drawPathLambda : public WTF::Lambda {
public:
    drawPathLambda(SkCanvas* canvas, const SkPath& path, const SkPaint& paint, typename Locker::Type locker = Locker::defaultValue)
        : m_canvas(canvas)
        , m_path(path)
        , m_paint(paint)
        , m_locker(locker)
    { }

    void call() { Locker lock(m_locker); m_canvas->drawPath(m_path, m_paint); }

private:
    SkCanvas* m_canvas;
    SkPath m_path;
    SkPaint m_paint;
    typename Locker::Type m_locker;
};

template<typename Locker = LambdaNoSync>
class drawBitmapRectLambda : public WTF::Lambda {
public:
    drawBitmapRectLambda(SkCanvas* canvas, const SkBitmap& bitmap, const SkIRect* src, const SkRect& dst, const SkPaint* paint, typename Locker::Type locker = Locker::defaultValue)
        : m_canvas(canvas)
        , m_bitmap(bitmap)
        , m_src(src)
        , m_dst(dst)
        , m_paint(paint)
        , m_locker(locker)
    { }

    void call() { Locker lock(m_locker); m_canvas->drawBitmapRect(m_bitmap, m_src.ptr(), m_dst, m_paint.ptr()); }

private:
    SkCanvas* m_canvas;
    SkBitmap m_bitmap;
    OptionalCopy<SkIRect> m_src;
    SkRect m_dst;
    OptionalCopy<SkPaint> m_paint;
    typename Locker::Type m_locker;
};

template<typename Locker = LambdaNoSync>
class drawTextLambda : public WTF::Lambda {
public:
    drawTextLambda(SkCanvas* canvas, const void* text, size_t byteLength, SkScalar x, SkScalar y, const SkPaint& paint, typename Locker::Type locker = Locker::defaultValue)
        : m_canvas(canvas)
        , m_text(adoptArrayPtr(text ? new char[byteLength] : 0))
        , m_byteLength(byteLength)
        , m_x(x)
        , m_y(y)
        , m_paint(paint)
        , m_locker(locker)
    {
        if (m_text)
            memcpy(m_text.get(), text, byteLength);
    }

    void call() { Locker lock(m_locker); m_canvas->drawText(m_text.get(), m_byteLength, m_x, m_y, m_paint); }

private:
    SkCanvas* m_canvas;
    OwnArrayPtr<char> m_text;
    size_t m_byteLength;
    SkScalar m_x;
    SkScalar m_y;
    SkPaint m_paint;
    typename Locker::Type m_locker;
};

template<typename Locker = LambdaNoSync>
class drawPosTextLambda : public WTF::Lambda {
public:
    drawPosTextLambda(SkCanvas* canvas, const void* text, size_t byteLength, const SkPoint pos[], const SkPaint& paint, typename Locker::Type locker = Locker::defaultValue)
        : m_canvas(canvas)
        , m_text(adoptArrayPtr(text ? new char[byteLength] : 0))
        , m_byteLength(byteLength)
        , m_paint(paint)
        , m_locker(locker)
    {
        if (m_text)
            memcpy(m_text.get(), text, byteLength);

        size_t points = m_paint.countText(text, byteLength);
        m_pos = adoptArrayPtr(pos ? new SkPoint[points] : 0);
        if (m_pos)
            memcpy(m_pos.get(), pos, points * sizeof(pos[0]));
    }

    void call() { Locker lock(m_locker); m_canvas->drawPosText(m_text.get(), m_byteLength, m_pos.get(), m_paint); }

private:
    SkCanvas* m_canvas;
    OwnArrayPtr<char> m_text;
    size_t m_byteLength;
    OwnArrayPtr<SkPoint> m_pos;
    SkPaint m_paint;
    typename Locker::Type m_locker;
};


template<typename Locker = LambdaNoSync>
class drawPosTextHLambda : public WTF::Lambda {
public:
    drawPosTextHLambda(SkCanvas* canvas, const void* text, size_t byteLength, const SkScalar xpos[],  SkScalar constY, const SkPaint& paint, typename Locker::Type locker = Locker::defaultValue)
        : m_canvas(canvas)
        , m_text(adoptArrayPtr(text ? new char[byteLength] : 0))
        , m_byteLength(byteLength)
        , m_constY(constY)
        , m_paint(paint)
        , m_locker(locker)
    {
        if (m_text)
            memcpy(m_text.get(), text, byteLength);

        size_t points = m_paint.countText(text, byteLength);
        m_xpos = adoptArrayPtr(xpos ? new SkScalar[points] : 0);
        if (m_xpos)
            memcpy(m_xpos.get(), xpos, points * sizeof(xpos[0]));
    }

    void call() { Locker lock(m_locker); m_canvas->drawPosTextH(m_text.get(), m_byteLength, m_xpos.get(), m_constY, m_paint); }

private:
    SkCanvas* m_canvas;
    OwnArrayPtr<char> m_text;
    size_t m_byteLength;
    OwnArrayPtr<SkScalar> m_xpos;
    SkScalar m_constY;
    SkPaint m_paint;
    typename Locker::Type m_locker;
};

template<typename Locker = LambdaNoSync>
class drawTextOnPathLambda : public WTF::Lambda {
public:
    drawTextOnPathLambda(SkCanvas* canvas, const void* text, size_t byteLength,  const SkPath& path, const SkMatrix* matrix, const SkPaint& paint, typename Locker::Type locker = Locker::defaultValue)
        : m_canvas(canvas)
        , m_text(adoptArrayPtr(text ? new char[byteLength] : 0))
        , m_byteLength(byteLength)
        , m_path(path)
        , m_matrix(matrix)
        , m_paint(paint)
        , m_locker(locker)
    {
        if (m_text)
            memcpy(m_text.get(), text, byteLength);
    }

    void call() { Locker lock(m_locker); m_canvas->drawTextOnPath(m_text.get(), m_byteLength, m_path, m_matrix.ptr(), m_paint); }

private:
    SkCanvas* m_canvas;
    OwnArrayPtr<char> m_text;
    size_t m_byteLength;
    SkPath m_path;
    OptionalCopy<SkMatrix> m_matrix;
    SkPaint m_paint;
    typename Locker::Type m_locker;
};

template<typename Locker = LambdaNoSync>
class drawLineLambda : public WTF::Lambda {
public:
    drawLineLambda(SkCanvas* canvas, SkScalar x0, SkScalar y0, SkScalar x1, SkScalar y1, const SkPaint& paint, typename Locker::Type locker = Locker::defaultValue)
        : m_canvas(canvas)
        , m_x0(x0)
        , m_y0(y0)
        , m_x1(x1)
        , m_y1(y1)
        , m_paint(paint)
        , m_locker(locker)
    { }

    void call() { Locker lock(m_locker); m_canvas->drawLine(m_x0, m_y0, m_x1, m_y1, m_paint); }

private:
    SkCanvas* m_canvas;
    SkScalar m_x0;
    SkScalar m_y0;
    SkScalar m_x1;
    SkScalar m_y1;
    SkPaint m_paint;
    typename Locker::Type m_locker;
};

template<typename Locker = LambdaNoSync>
class drawOvalLambda : public WTF::Lambda {
public:
    drawOvalLambda(SkCanvas* canvas, const SkRect& oval, const SkPaint& paint, typename Locker::Type locker = Locker::defaultValue)
        : m_canvas(canvas)
        , m_oval(oval)
        , m_paint(paint)
        , m_locker(locker)
    { }

    void call() { Locker lock(m_locker); m_canvas->drawOval(m_oval, m_paint); }

private:
    SkCanvas* m_canvas;
    SkRect m_oval;
    SkPaint m_paint;
    typename Locker::Type m_locker;
};

} // namespace WebCore

#endif
