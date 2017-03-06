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
#ifndef GraphicsContextFunctions_h
#define GraphicsContextFunctions_h

#define FOR_EACH_GFX_CTX_VOID_FUNCTION(MACRO) \
    MACRO(save, (SkCanvas::SaveFlags flags), (flags))                   \
    MACRO(saveLayer, (const SkRect* bounds, const SkPaint* paint, SkCanvas::SaveFlags flags), (bounds, paint, flags)) \
    MACRO(saveLayerAlpha, (const SkRect* bounds, U8CPU alpha, SkCanvas::SaveFlags flags), (bounds, alpha, flags)) \
    MACRO(restore, (), ()) \
    MACRO(translate, (SkScalar dx, SkScalar dy), (dx, dy)) \
    MACRO(scale, (SkScalar sx, SkScalar sy), (sx, sy)) \
    MACRO(rotate, (SkScalar degrees), (degrees)) \
    MACRO(concat, (const SkMatrix& matrix), (matrix))           \
    MACRO(clipRect, (const SkRect& rect, SkRegion::Op op, bool doAntiAlias), (rect, op)) \
    MACRO(clipPath, (const SkPath& path, SkRegion::Op op, bool doAntiAlias), (path, op)) \
    MACRO(drawPoints, (SkCanvas::PointMode mode, size_t count, const SkPoint pts[], const SkPaint& paint), (mode, count, pts, paint)) \
    MACRO(drawRect, (const SkRect& rect, const SkPaint& paint), (rect, paint)) \
    MACRO(drawPath, (const SkPath& path, const SkPaint& paint), (path, paint)) \
    MACRO(drawBitmapRect, (const SkBitmap& bitmap, const SkIRect* src, const SkRect& dst, const SkPaint* paint), (bitmap, src, dst, paint)) \
    MACRO(drawText, (const void* text, size_t byteLength, SkScalar x, SkScalar y, const SkPaint& paint), (text, byteLength, x, y, paint)) \
    MACRO(drawPosText, (const void* text, size_t byteLength, const SkPoint pos[], const SkPaint& paint), (text, byteLength, pos, paint)) \
    MACRO(drawPosTextH, (const void* text, size_t byteLength, const SkScalar xpos[], SkScalar constY, const SkPaint& paint), (text, byteLength, xpos, constY, paint)) \
    MACRO(drawTextOnPath, (const void* text, size_t byteLength, const SkPath& path, const SkMatrix* matrix, const SkPaint& paint), (text, byteLength, path, matrix, paint)) \
    MACRO(drawLine, (SkScalar x0, SkScalar y0, SkScalar x1, SkScalar y1, const SkPaint& paint), (x0, y0, x1, y1, paint)) \
    MACRO(drawOval, (const SkRect& oval, const SkPaint& paint), (oval, paint))



#endif
