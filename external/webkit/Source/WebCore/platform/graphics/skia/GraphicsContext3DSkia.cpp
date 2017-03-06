/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if ENABLE(WEBGL)

#include "GraphicsContext3D.h"

#include "BitmapImage.h"
#include "Image.h"
#include "ImageSource.h"
#if !PLATFORM(ANDROID)
#include "NativeImageSkia.h"
#endif
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#if PLATFORM(ANDROID)
#include "SkBitmapRef.h"
#endif
#include <algorithm>

namespace WebCore {

bool GraphicsContext3D::getImageData(Image* image,
                                     GC3Denum format,
                                     GC3Denum type,
                                     bool premultiplyAlpha,
                                     bool ignoreGammaAndColorProfile,
                                     Vector<uint8_t>& outputVector)
{
    if (!image)
        return false;
#if PLATFORM(ANDROID)
    SkRefPtr<SkBitmapRef> pixels;
#else
    OwnPtr<NativeImageSkia> pixels;
#endif
    NativeImagePtr skiaImage = image->nativeImageForCurrentFrame();

    AlphaOp neededAlphaOp = AlphaDoNothing;
    bool hasAlpha = skiaImage ? !skiaImage->bitmap().isOpaque() : true;
    if ((!skiaImage || ignoreGammaAndColorProfile || (hasAlpha && !premultiplyAlpha)) && image->data()) {
        ImageSource decoder(ImageSource::AlphaNotPremultiplied,
                            ignoreGammaAndColorProfile ? ImageSource::GammaAndColorProfileIgnored : ImageSource::GammaAndColorProfileApplied);
        // Attempt to get raw unpremultiplied image data 
        decoder.setData(image->data(), true);
        if (!decoder.frameCount() || !decoder.frameIsCompleteAtIndex(0))
            return false;
        hasAlpha = decoder.frameHasAlphaAtIndex(0);
#if PLATFORM(ANDROID)
        pixels = decoder.createFrameAtIndex(0);
        pixels->unref();
#else
        pixels = adoptPtr(decoder.createFrameAtIndex(0));
#endif
        if (!pixels.get() || !pixels->isDataComplete() || !pixels->bitmap().width() || !pixels->bitmap().height())
            return false;
#if !PLATFORM(ANDROID)
        SkBitmap::Config skiaConfig = pixels->bitmap().config();
        if (skiaConfig != SkBitmap::kARGB_8888_Config)
            return false;
#endif
        skiaImage = pixels.get();
        if (hasAlpha && premultiplyAlpha)
            neededAlphaOp = AlphaDoPremultiply;
    } else if (!premultiplyAlpha && hasAlpha)
        neededAlphaOp = AlphaDoUnmultiply;
    if (!skiaImage)
        return false;

#if PLATFORM(ANDROID)
    if (skiaImage->bitmap().isNull())
        return false;

    SkBitmap::Config skiaConfig = skiaImage->bitmap().config();
    if (skiaConfig != SkBitmap::kARGB_8888_Config) {
        SkBitmap dst;
        if (!skiaImage->bitmap().copyTo(&dst, SkBitmap::kARGB_8888_Config))
            return false;
        pixels = new SkBitmapRef(dst);
        pixels->unref();
        skiaImage = pixels.get();
    }
#endif

    const SkBitmap& skiaImageRef = skiaImage->bitmap();
    // A mismatch in size can occur if the image was subsampled due to its large size.
    // TODO: what should we do here?? Upsample? simply fail for now.
    if (image->width() != skiaImageRef.width() || image->height() != skiaImageRef.height())
        return false;

    SkAutoLockPixels lock(skiaImageRef);
    ASSERT(skiaImageRef.rowBytes() == skiaImageRef.width() * 4);
    outputVector.resize(skiaImageRef.rowBytes() * skiaImageRef.height());
    return packPixels(reinterpret_cast<const uint8_t*>(skiaImageRef.getPixels()),
                      SK_B32_SHIFT ? SourceFormatRGBA8 : SourceFormatBGRA8,
                      skiaImageRef.width(), skiaImageRef.height(), 0,
                      format, type, neededAlphaOp, outputVector.data());
}

} // namespace WebCore

#endif // ENABLE(WEBGL)
