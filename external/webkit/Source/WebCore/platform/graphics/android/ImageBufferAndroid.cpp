/*
 * Copyright 2007, The Android Open Source Project
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
#include "ImageBuffer.h"

#include "AndroidProperties.h"
#include "Base64.h"
#include "BitmapImage.h"
#include "ColorSpace.h"
#include "GraphicsContext.h"
#include "JPEGImageEncoder.h"
#include "NotImplemented.h"
#include "MIMETypeRegistry.h"
#include "PlatformBridge.h"
#include "PlatformGraphicsContextSkia.h"
#include "PlatformGraphicsContext.h"
#include "PlatformGraphicsContextSkia.h"
#include "PNGImageEncoder.h"
#include "SkBitmapRef.h"
#include "SkCanvas.h"
#include "SkColorPriv.h"
#include "SkData.h"
#include "SkDevice.h"
#include "SkImageEncoder.h"
#include "SkStream.h"
#include "SkUnPreMultiply.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/text/StringConcatenate.h>
#if USE(ACCELERATED_CANVAS_LAYER)
#include "GaneshCanvas.h"
#include "TextureBackedCanvas.h"
#endif

using namespace std;

namespace WebCore {

static const int minTextureArea = 50 * 100;
static const int maxTextureDimension = 2048;

ImageBufferData::ImageBufferData(const IntSize&)
{
}

ImageBuffer::ImageBuffer(const IntSize& size, ColorSpace, RenderingMode renderingMode, bool& success)
    : m_data(size)
    , m_size(size)
    , m_accelerateRendering(false)
{
    success = false;

    // GraphicsContext creates a 32bpp SkBitmap, so 4 bytes per pixel.
    if (!PlatformBridge::canSatisfyMemoryAllocation(size.width() * size.height() * 4))
        return;

    if (size.width() <= 0 || size.height() <= 0)
        return;

#if USE(ACCELERATED_CANVAS_LAYER)
    // Only accelerate if the canvas is of a reasonable size.
    if (size.width() <= maxTextureDimension && size.height() <= maxTextureDimension
        && size.width() * size.height() >= minTextureArea) {
        RefPtr<AcceleratedCanvas> canvas;
        if (renderingMode == Accelerated && GaneshCanvas::isSuitableFor(size))
            canvas = GaneshCanvas::create(size);
        else
            canvas = TextureBackedCanvas::create(size);

        if (canvas) {
            m_data.m_platformContext = PlatformGraphicsContextSkia::createWithAcceleratedCanvas(canvas.get());
            m_data.m_acceleratedCanvas = canvas.release();
        }
    }
#endif

#if USE(ACCELERATED_CANVAS_LAYER)
    if (!m_data.m_acceleratedCanvas)
#endif
    {
        SkBitmap bitmap;
        bitmap.setConfig(SkBitmap::kARGB_8888_Config, size.width(), size.height());
        bitmap.allocPixels();
        bitmap.eraseColor(0);
        m_data.m_canvas = new SkCanvas(bitmap);
        m_data.m_canvas->unref();

        m_data.m_platformContext = new PlatformGraphicsContextSkia(m_data.m_canvas.get());
    }

    if (!m_data.m_platformContext)
        return;

    m_context.set(new GraphicsContext(m_data.m_platformContext.get()));

    success = true;
}

ImageBuffer::~ImageBuffer()
{
    m_context.clear();
}

GraphicsContext* ImageBuffer::context() const
{
    return m_context.get();
}

bool ImageBuffer::drawsUsingCopy() const
{
    return true;
}

PassRefPtr<Image> ImageBuffer::copyImage() const
{
    ASSERT(context());

    SkBitmap copy;
    m_data.m_platformContext->copyImageTo(&copy);

    SkBitmapRef* ref = new SkBitmapRef(copy);
    RefPtr<Image> image = BitmapImage::create(ref, 0);
    ref->unref();
    return image;
}

void ImageBuffer::clip(GraphicsContext* context, const FloatRect& rect) const
{
    SkDebugf("xxxxxxxxxxxxxxxxxx clip not implemented\n");
}

void ImageBuffer::draw(GraphicsContext* context, ColorSpace styleColorSpace, const FloatRect& destRect, const FloatRect& srcRect, CompositeOperator op, bool useLowQualityScale)
{
    RefPtr<Image> imageCopy = copyImage();
    context->drawImage(imageCopy.get(), styleColorSpace, destRect, srcRect, op, useLowQualityScale);
}

void ImageBuffer::drawPattern(GraphicsContext* context, const FloatRect& srcRect, const AffineTransform& patternTransform, const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect)
{
    RefPtr<Image> imageCopy = copyImage();
    imageCopy->drawPattern(context, srcRect, patternTransform, phase, styleColorSpace, op, destRect);
}

// Borrowed from ImageBufferSkia.cpp.

template <Multiply multiplied>
PassRefPtr<ByteArray> getImageData(const IntRect& rect, PlatformGraphicsContextSkia* pctx, const IntSize& size)
{
    float area = 4.0f * rect.width() * rect.height();
    if (area > static_cast<float>(std::numeric_limits<int>::max()))
        return 0;

    RefPtr<ByteArray> result = ByteArray::create(rect.width() * rect.height() * 4);

    SkBitmap::Config srcConfig;
    {
        SkBitmap bitmap;
        pctx->accessCanvasBitmap(&bitmap, false);
        srcConfig = bitmap.config();
    }

    if (srcConfig == SkBitmap::kNo_Config) {
        // This is an empty SkBitmap that could not be configured.
        ASSERT(!size.width() || !size.height());
        return result.release();
    }

    unsigned char* data = result->data();

    if (rect.x() < 0
        || rect.y() < 0
        || rect.maxX() > size.width()
        || rect.maxY() > size.height())
        memset(data, 0, result->length());

    int originX = rect.x();
    int destX = 0;
    if (originX < 0) {
        destX = -originX;
        originX = 0;
    }
    int endX = rect.maxX();
    if (endX > size.width())
        endX = size.width();
    int numColumns = endX - originX;

    if (numColumns <= 0)
        return result.release();

    int originY = rect.y();
    int destY = 0;
    if (originY < 0) {
        destY = -originY;
        originY = 0;
    }
    int endY = rect.maxY();
    if (endY > size.height())
        endY = size.height();
    int numRows = endY - originY;

    if (numRows <= 0)
        return result.release();

    ASSERT(srcConfig == SkBitmap::kARGB_8888_Config);

    unsigned destBytesPerRow = 4 * rect.width();
    SkBitmap destBitmap;
    destBitmap.setConfig(SkBitmap::kARGB_8888_Config, rect.width(), rect.height(), destBytesPerRow);
    destBitmap.setPixels(data);

    SkCanvas::Config8888 config8888;
    if (multiplied == Premultiplied)
        config8888 = SkCanvas::kRGBA_Premul_Config8888;
    else
        config8888 = SkCanvas::kRGBA_Unpremul_Config8888;

    pctx->readPixels(&destBitmap, rect.x(), rect.y(), config8888);

    return result.release();
}

PassRefPtr<ByteArray> ImageBuffer::getUnmultipliedImageData(const IntRect& rect) const
{
    m_data.m_platformContext->syncSoftwareCanvas();
    return getImageData<Unmultiplied>(rect, m_data.m_platformContext.get(), m_size);
}

PassRefPtr<ByteArray> ImageBuffer::getPremultipliedImageData(const IntRect& rect) const
{
    m_data.m_platformContext->syncSoftwareCanvas();
    return getImageData<Premultiplied>(rect, m_data.m_platformContext.get(), m_size);
}

template <Multiply multiplied>
void putImageData(ByteArray*& source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint,
                  PlatformGraphicsContextSkia* pctx, const IntSize& size)
{
    ASSERT(sourceRect.width() > 0);
    ASSERT(sourceRect.height() > 0);

    int originX = sourceRect.x();
    int destX = destPoint.x() + sourceRect.x();
    ASSERT(destX >= 0);
    ASSERT(destX < size.width());
    ASSERT(originX >= 0);
    ASSERT(originX < sourceRect.maxX());

    int endX = destPoint.x() + sourceRect.maxX();
    ASSERT(endX <= size.width());

    int numColumns = endX - destX;

    int originY = sourceRect.y();
    int destY = destPoint.y() + sourceRect.y();
    ASSERT(destY >= 0);
    ASSERT(destY < size.height());
    ASSERT(originY >= 0);
    ASSERT(originY < sourceRect.maxY());

    int endY = destPoint.y() + sourceRect.maxY();
    ASSERT(endY <= size.height());
    int numRows = endY - destY;

    unsigned srcBytesPerRow = 4 * sourceSize.width();
    SkBitmap srcBitmap;
    srcBitmap.setConfig(SkBitmap::kARGB_8888_Config, numColumns, numRows, srcBytesPerRow);
    srcBitmap.setPixels(source->data() + originY * srcBytesPerRow + originX * 4);

    SkCanvas::Config8888 config8888;
    if (multiplied == Premultiplied)
        config8888 = SkCanvas::kRGBA_Premul_Config8888;
    else
        config8888 = SkCanvas::kRGBA_Unpremul_Config8888;

    pctx->writePixels(srcBitmap, destX, destY, config8888);
}
void ImageBuffer::putUnmultipliedImageData(ByteArray* source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint)
{
    context()->platformContext()->syncSoftwareCanvas();
    putImageData<Unmultiplied>(source, sourceSize, sourceRect, destPoint, m_data.m_platformContext.get(), m_size);
}

void ImageBuffer::putPremultipliedImageData(ByteArray* source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint)
{
    context()->platformContext()->syncSoftwareCanvas();
    putImageData<Premultiplied>(source, sourceSize, sourceRect, destPoint, m_data.m_platformContext.get(), m_size);
}

// Borrowed from ImageBufferSkia.cpp.
template <typename T>
static String ImageToDataURL(const T& source, const String& mimeType, const double* quality)
{
    String lowercaseMimeType = mimeType.lower();
    ASSERT(MIMETypeRegistry::isSupportedImageMIMETypeForEncoding(lowercaseMimeType));

    Vector<unsigned char> encodedImage;
    if (lowercaseMimeType == "image/jpeg") {
        int compressionQuality = JPEGImageEncoder::DefaultCompressionQuality;
        if (quality && *quality >= 0.0 && *quality <= 1.0)
            compressionQuality = static_cast<int>(*quality * 100 + 0.5);
        if (!JPEGImageEncoder::encode(source, compressionQuality, &encodedImage))
            return "data:,";
    } else {
        ASSERT(lowercaseMimeType == "image/png");
        if (!PNGImageEncoder::encode(source, &encodedImage))
            return "data:,";
    }

    Vector<char> base64Data;
    base64Encode(*reinterpret_cast<Vector<char>*>(&encodedImage), base64Data);

    return makeString("data:", lowercaseMimeType, ";base64,", base64Data);
}

// Borrowed from ImageBufferSkia.cpp.
String ImageDataToDataURL(const ImageData& source, const String& mimeType, const double* quality)
{
    return ImageToDataURL(source, mimeType, quality);
}

String ImageBuffer::toDataURL(const String& mimeType, const double* quality) const
{
    SkBitmap bitmap;
    // The lock in refPixelsWithLock is balanced by a call to unlockPixels() in the SkBitmap destructor.
    if (!m_data.m_platformContext->refPixelsWithLock(&bitmap))
        return "data:,";

    return ImageToDataURL(bitmap, mimeType, quality);
}

void ImageBuffer::platformTransformColorSpace(const Vector<int>& lookupTable)
{
    notImplemented();
}

size_t ImageBuffer::dataSize() const
{
    return m_size.width() * m_size.height() * 4;
}

#if USE(ACCELERATED_CANVAS_LAYER)
bool ImageBuffer::requestAccelerationForChangingCanvas()
{
    if (!m_accelerateRendering && m_data.m_acceleratedCanvas) {
        m_data.m_platformLayer = m_data.m_acceleratedCanvas->createPlatformLayer();
        m_data.m_platformLayer->unref();
        m_accelerateRendering = true;

        // Make sure the layer has something to draw from before adding it to
        // the tree. Otherwise we might get a flicker.
        m_data.m_platformLayer->syncContents();
        return true;
    }

    return false;
}

PlatformLayer* ImageBuffer::platformLayer() const
{
    return m_data.m_platformLayer.get();
}
#endif

}
