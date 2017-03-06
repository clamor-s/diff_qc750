/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SkUtils.h"
#include "cutils/log.h"

#if DSTSIZE==32
    #define DSTTYPE SkPMColor
#elif DSTSIZE==16
    #define DSTTYPE uint16_t
#else
    #error "need DSTSIZE to be 32 or 16"
#endif

#if (DSTSIZE == 32)
    #define BITMAPPROC_MEMSET(ptr, value, n) sk_memset32(ptr, value, n)
#elif (DSTSIZE == 16)
    #define BITMAPPROC_MEMSET(ptr, value, n) sk_memset16(ptr, value, n)
#else
    #error "unsupported DSTSIZE"
#endif

void MAKENAME(_opaque_clamp_transonly_nofilter_shaderproc_DX)(
            const SkBitmapProcState& s, int x, int y,
            DSTTYPE* SK_RESTRICT colors, int count) {
    SkASSERT(s.fInvType & ~SkMatrix::kTranslate_Mask == 0);
    SkASSERT(s.fInvKy == 0);
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(!s.fDoFilter);
    SkDEBUGCODE(CHECKSTATE(s);)

    //Borrowed from nofilter_trans_preamble
    SkPoint pt;
    s.fInvProc(*s.fInvMatrix, SkIntToScalar(x) + SK_ScalarHalf,
               SkIntToScalar(y) + SK_ScalarHalf, &pt);
    SkFixed fx = SkScalarToFixed(pt.fX);
    SkFixed fy = SkScalarToFixed(pt.fY);

    const unsigned w = s.fBitmap->width();
    const unsigned h = s.fBitmap->height();

    int startx = SkFixedFloor(fx);
    int starty = SkFixedFloor(fy);

    // Y out of bounds cases
    if (starty < 0) {
        starty = 0;
    }

    if (starty >= (int)h) {
        starty = h - 1;
    }

    const SRCTYPE* SK_RESTRICT startRow = (const SRCTYPE*)
        (((const char*)s.fBitmap->getPixels()) +
        starty*s.fBitmap->rowBytes());

    const SRCTYPE* SK_RESTRICT endRow = startRow + w;

#ifdef PREAMBLE
    PREAMBLE(s);
#endif

    if (startx < 0) {
        BITMAPPROC_MEMSET(colors, RETURNDST(startRow[0]), -startx);
        colors += -startx;
        count -= -startx;
        startx = 0;
    }

    const SRCTYPE* SK_RESTRICT curSrc = startRow + startx;

    if (count < 2) {
        *colors++ = RETURNDST(*startRow++);
        count--;
    }
    else
    {
        int leftInRow = endRow - curSrc;
        int innerCount = count < leftInRow ?
            count : leftInRow;
        count -= innerCount;
        const SRCTYPE* endSrc = curSrc + innerCount;
        // TODO make end bound less conservative
#ifdef UNROLLED_8X_BODY
        for (; curSrc < endSrc - 8; ) {
            UNROLLED_8X_BODY(colors, curSrc);
            colors += 8;
        }
#else
        for (; curSrc < endSrc - 4; ) {
            UNROLLED_4X_BODY(colors, curSrc);
        }
#endif // #ifdef UNROLLED_8X_BODY
        for (; curSrc < endSrc; ) {
            *colors++ = RETURNDST(*curSrc++);
        }
    }

    if (count > 0) {
        BITMAPPROC_MEMSET(colors, RETURNDST(startRow[w-1]), count);
        colors += count;
        count -= count;
    }

#ifdef POSTAMBLE
    POSTAMBLE(s);
#endif

}

void MAKENAME(_opaque_repeat_transonly_nofilter_shaderproc_DX)(
            const SkBitmapProcState& s, int x, int y,
            DSTTYPE* SK_RESTRICT colors, int count) {
    SkASSERT(s.fInvType & ~SkMatrix::kTranslate_Mask == 0);
    SkASSERT(s.fInvKy == 0);
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(!s.fDoFilter);
    SkDEBUGCODE(CHECKSTATE(s);)

    //Borrowed from nofilter_trans_preamble
    SkPoint pt;
    s.fInvProc(*s.fInvMatrix, SkIntToScalar(x) + SK_ScalarHalf,
               SkIntToScalar(y) + SK_ScalarHalf, &pt);
    SkFixed fx = SkScalarToFixed(pt.fX);
    SkFixed fy = SkScalarToFixed(pt.fY);

    const unsigned w = s.fBitmap->width();
    const unsigned h = s.fBitmap->height();

    int startx = SkFixedFloor(fx);
    int starty = SkFixedFloor(fy);
    while (startx < 0) {
        startx += w;
    }
    while (startx >= (int)w) {
        startx -= w;
    }
    while (starty < 0) {
        starty += h;
    }
    while (starty >= (int)h) {
        starty -= h;
    }

    const SRCTYPE* SK_RESTRICT startRow = (const SRCTYPE*)
        (((const char*)s.fBitmap->getPixels()) +
        starty*s.fBitmap->rowBytes());

    const SRCTYPE* SK_RESTRICT endRow = startRow + w;

    const SRCTYPE* SK_RESTRICT curSrc = startRow + startx;

#ifdef PREAMBLE
    PREAMBLE(s);
#endif

    while (count > 0) {

        int leftInRow = endRow - curSrc;
        int innerCount = count < leftInRow ?
            count : leftInRow;
        count -= innerCount;
        const SRCTYPE* endSrc = curSrc + innerCount;
        // TODO make end bound less conservative
#ifdef UNROLLED_8X_BODY
        for (; curSrc < endSrc - 8; ) {
            UNROLLED_8X_BODY(colors, curSrc);
            curSrc += 8;
            colors += 8;
        }
#else
        for (; curSrc < endSrc - 4; ) {
            UNROLLED_4X_BODY(colors, curSrc);
        }
#endif // #ifdef UNROLLED_8X_BODY
        for (; curSrc < endSrc; ) {
            *colors++ = RETURNDST(*curSrc++);
        }
        curSrc = startRow;
    }

#ifdef POSTAMBLE
    POSTAMBLE(s);
#endif

}

#ifdef PREAMBLE
    #undef PREAMBLE
#endif

#ifdef POSTAMBLE
    #undef POSTAMBLE
#endif

#undef RETURNDST
#undef UNROLLED_4X_BODY
#undef UNROLLED_8X_BODY
#undef SRCTYPE
#undef DSTSIZE
#undef DSTTYPE
#undef BITMAPPROC_MEMSET
#undef MAKENAME
#undef CHECKSTATE
