
/*
 * Copyright (C) 2009 The Android Open Source Project
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#if defined(__ARM_HAVE_NEON)
    #include <arm_neon.h>
#endif

#include "SkBitmapProcState.h"
#include "SkColorPriv.h"
#include "SkUtils.h"

#if __ARM_ARCH__ >= 6 && !defined(SK_CPU_BENDIAN)
void SI8_D16_nofilter_DX_arm(
    const SkBitmapProcState& s,
    const uint32_t* SK_RESTRICT xy,
    int count,
    uint16_t* SK_RESTRICT colors) __attribute__((optimize("O1")));

void SI8_D16_nofilter_DX_arm(const SkBitmapProcState& s,
                             const uint32_t* SK_RESTRICT xy,
                             int count, uint16_t* SK_RESTRICT colors) {
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(s.fInvType <= (SkMatrix::kTranslate_Mask | SkMatrix::kScale_Mask));
    SkASSERT(s.fDoFilter == false);
    
    const uint16_t* SK_RESTRICT table = s.fBitmap->getColorTable()->lock16BitCache();
    const uint8_t* SK_RESTRICT srcAddr = (const uint8_t*)s.fBitmap->getPixels();
    
    // buffer is y32, x16, x16, x16, x16, x16
    // bump srcAddr to the proper row, since we're told Y never changes
    SkASSERT((unsigned)xy[0] < (unsigned)s.fBitmap->height());
    srcAddr = (const uint8_t*)((const char*)srcAddr +
                               xy[0] * s.fBitmap->rowBytes());
    
    uint8_t src;
    
    if (1 == s.fBitmap->width()) {
        src = srcAddr[0];
        uint16_t dstValue = table[src];
        sk_memset16(colors, dstValue, count);
    } else {
        int i;
        int count8 = count >> 3;
        const uint16_t* SK_RESTRICT xx = (const uint16_t*)(xy + 1);
        
        asm volatile (
                      "cmp        %[count8], #0                   \n\t"   // compare loop counter with 0
                      "beq        2f                              \n\t"   // if loop counter == 0, exit
                      "1:                                             \n\t"
                      "ldmia      %[xx]!, {r5, r7, r9, r11}       \n\t"   // load ptrs to pixels 0-7
                      "subs       %[count8], %[count8], #1        \n\t"   // decrement loop counter
                      "uxth       r4, r5                          \n\t"   // extract ptr 0
                      "mov        r5, r5, lsr #16                 \n\t"   // extract ptr 1
                      "uxth       r6, r7                          \n\t"   // extract ptr 2
                      "mov        r7, r7, lsr #16                 \n\t"   // extract ptr 3
                      "ldrb       r4, [%[srcAddr], r4]            \n\t"   // load pixel 0 from image
                      "uxth       r8, r9                          \n\t"   // extract ptr 4
                      "ldrb       r5, [%[srcAddr], r5]            \n\t"   // load pixel 1 from image
                      "mov        r9, r9, lsr #16                 \n\t"   // extract ptr 5
                      "ldrb       r6, [%[srcAddr], r6]            \n\t"   // load pixel 2 from image
                      "uxth       r10, r11                        \n\t"   // extract ptr 6
                      "ldrb       r7, [%[srcAddr], r7]            \n\t"   // load pixel 3 from image
                      "mov        r11, r11, lsr #16               \n\t"   // extract ptr 7
                      "ldrb       r8, [%[srcAddr], r8]            \n\t"   // load pixel 4 from image
                      "add        r4, r4, r4                      \n\t"   // double pixel 0 for RGB565 lookup
                      "ldrb       r9, [%[srcAddr], r9]            \n\t"   // load pixel 5 from image
                      "add        r5, r5, r5                      \n\t"   // double pixel 1 for RGB565 lookup
                      "ldrb       r10, [%[srcAddr], r10]          \n\t"   // load pixel 6 from image
                      "add        r6, r6, r6                      \n\t"   // double pixel 2 for RGB565 lookup
                      "ldrb       r11, [%[srcAddr], r11]          \n\t"   // load pixel 7 from image
                      "add        r7, r7, r7                      \n\t"   // double pixel 3 for RGB565 lookup
                      "ldrh       r4, [%[table], r4]              \n\t"   // load pixel 0 RGB565 from colmap
                      "add        r8, r8, r8                      \n\t"   // double pixel 4 for RGB565 lookup
                      "ldrh       r5, [%[table], r5]              \n\t"   // load pixel 1 RGB565 from colmap
                      "add        r9, r9, r9                      \n\t"   // double pixel 5 for RGB565 lookup
                      "ldrh       r6, [%[table], r6]              \n\t"   // load pixel 2 RGB565 from colmap
                      "add        r10, r10, r10                   \n\t"   // double pixel 6 for RGB565 lookup
                      "ldrh       r7, [%[table], r7]              \n\t"   // load pixel 3 RGB565 from colmap
                      "add        r11, r11, r11                   \n\t"   // double pixel 7 for RGB565 lookup
                      "ldrh       r8, [%[table], r8]              \n\t"   // load pixel 4 RGB565 from colmap
                      "ldrh       r9, [%[table], r9]              \n\t"   // load pixel 5 RGB565 from colmap
                      "ldrh       r10, [%[table], r10]            \n\t"   // load pixel 6 RGB565 from colmap
                      "ldrh       r11, [%[table], r11]            \n\t"   // load pixel 7 RGB565 from colmap
                      "pkhbt      r5, r4, r5, lsl #16             \n\t"   // pack pixels 0 and 1
                      "pkhbt      r6, r6, r7, lsl #16             \n\t"   // pack pixels 2 and 3
                      "pkhbt      r8, r8, r9, lsl #16             \n\t"   // pack pixels 4 and 5
                      "pkhbt      r10, r10, r11, lsl #16          \n\t"   // pack pixels 6 and 7
                      "stmia      %[colors]!, {r5, r6, r8, r10}   \n\t"   // store last 8 pixels
                      "bgt        1b                              \n\t"   // loop if counter > 0
                      "2:                                             \n\t"
                      : [xx] "+r" (xx), [count8] "+r" (count8), [colors] "+r" (colors)
                      : [table] "r" (table), [srcAddr] "r" (srcAddr)
                      : "memory", "cc", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11"
                      );
        
        for (i = (count & 7); i > 0; --i) {
            src = srcAddr[*xx++]; *colors++ = table[src];
        }
    }

    s.fBitmap->getColorTable()->unlock16BitCache();
}

void SI8_opaque_D32_nofilter_DX_arm(
    const SkBitmapProcState& s,
    const uint32_t* SK_RESTRICT xy,
    int count,
    SkPMColor* SK_RESTRICT colors) __attribute__((optimize("O1")));

void SI8_opaque_D32_nofilter_DX_arm(const SkBitmapProcState& s,
                                    const uint32_t* SK_RESTRICT xy,
                                    int count, SkPMColor* SK_RESTRICT colors) {
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(s.fInvType <= (SkMatrix::kTranslate_Mask | SkMatrix::kScale_Mask));
    SkASSERT(s.fDoFilter == false);

    const SkPMColor* SK_RESTRICT table = s.fBitmap->getColorTable()->lockColors();
    const uint8_t* SK_RESTRICT srcAddr = (const uint8_t*)s.fBitmap->getPixels();

    // buffer is y32, x16, x16, x16, x16, x16
    // bump srcAddr to the proper row, since we're told Y never changes
    SkASSERT((unsigned)xy[0] < (unsigned)s.fBitmap->height());
    srcAddr = (const uint8_t*)((const char*)srcAddr + xy[0] * s.fBitmap->rowBytes());

    if (1 == s.fBitmap->width()) {
        uint8_t src = srcAddr[0];
        SkPMColor dstValue = table[src];
        sk_memset32(colors, dstValue, count);
    } else {
        const uint16_t* xx = (const uint16_t*)(xy + 1);

        asm volatile (
                      "subs       %[count], %[count], #8          \n\t"   // decrement count by 8, set flags
                      "blt        2f                              \n\t"   // if count < 0, branch to singles
                      "1:                                             \n\t"   // eights loop
                      "ldmia      %[xx]!, {r5, r7, r9, r11}       \n\t"   // load ptrs to pixels 0-7
                      "uxth       r4, r5                          \n\t"   // extract ptr 0
                      "mov        r5, r5, lsr #16                 \n\t"   // extract ptr 1
                      "uxth       r6, r7                          \n\t"   // extract ptr 2
                      "mov        r7, r7, lsr #16                 \n\t"   // extract ptr 3
                      "ldrb       r4, [%[srcAddr], r4]            \n\t"   // load pixel 0 from image
                      "uxth       r8, r9                          \n\t"   // extract ptr 4
                      "ldrb       r5, [%[srcAddr], r5]            \n\t"   // load pixel 1 from image
                      "mov        r9, r9, lsr #16                 \n\t"   // extract ptr 5
                      "ldrb       r6, [%[srcAddr], r6]            \n\t"   // load pixel 2 from image
                      "uxth       r10, r11                        \n\t"   // extract ptr 6
                      "ldrb       r7, [%[srcAddr], r7]            \n\t"   // load pixel 3 from image
                      "mov        r11, r11, lsr #16               \n\t"   // extract ptr 7
                      "ldrb       r8, [%[srcAddr], r8]            \n\t"   // load pixel 4 from image
                      "ldrb       r9, [%[srcAddr], r9]            \n\t"   // load pixel 5 from image
                      "ldrb       r10, [%[srcAddr], r10]          \n\t"   // load pixel 6 from image
                      "ldrb       r11, [%[srcAddr], r11]          \n\t"   // load pixel 7 from image
                      "ldr        r4, [%[table], r4, lsl #2]      \n\t"   // load pixel 0 SkPMColor from colmap
                      "ldr        r5, [%[table], r5, lsl #2]      \n\t"   // load pixel 1 SkPMColor from colmap
                      "ldr        r6, [%[table], r6, lsl #2]      \n\t"   // load pixel 2 SkPMColor from colmap
                      "ldr        r7, [%[table], r7, lsl #2]      \n\t"   // load pixel 3 SkPMColor from colmap
                      "ldr        r8, [%[table], r8, lsl #2]      \n\t"   // load pixel 4 SkPMColor from colmap
                      "ldr        r9, [%[table], r9, lsl #2]      \n\t"   // load pixel 5 SkPMColor from colmap
                      "ldr        r10, [%[table], r10, lsl #2]    \n\t"   // load pixel 6 SkPMColor from colmap
                      "ldr        r11, [%[table], r11, lsl #2]    \n\t"   // load pixel 7 SkPMColor from colmap
                      "subs       %[count], %[count], #8          \n\t"   // decrement loop counter
                      "stmia      %[colors]!, {r4-r11}            \n\t"   // store 8 pixels
                      "bge        1b                              \n\t"   // loop if counter >= 0
                      "2:                                             \n\t"
                      "adds       %[count], %[count], #8          \n\t"   // fix up counter, set flags
                      "beq        4f                              \n\t"   // if count == 0, branch to exit
                      "3:                                             \n\t"   // singles loop
                      "ldrh       r4, [%[xx]], #2                 \n\t"   // load pixel ptr
                      "subs       %[count], %[count], #1          \n\t"   // decrement loop counter
                      "ldrb       r5, [%[srcAddr], r4]            \n\t"   // load pixel from image
                      "ldr        r6, [%[table], r5, lsl #2]      \n\t"   // load SkPMColor from colmap
                      "str        r6, [%[colors]], #4             \n\t"   // store pixel, update ptr
                      "bne        3b                              \n\t"   // loop if counter != 0
                      "4:                                             \n\t"   // exit
                      : [xx] "+r" (xx), [count] "+r" (count), [colors] "+r" (colors)
                      : [table] "r" (table), [srcAddr] "r" (srcAddr)
                      : "memory", "cc", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11"
                      );
    }

    s.fBitmap->getColorTable()->unlockColors(false);
}
#endif //__ARM_ARCH__ >= 6 && !defined(SK_CPU_BENDIAN)

#if defined(SK_CPU_LENDIAN) && defined(__ARM_HAVE_NEON)
// Convert RGBA4444 to ABGR8888
void S4444_opaque_D32_nofilter_DX_neon(const SkBitmapProcState& s,
                                      const uint32_t* SK_RESTRICT xy,
                                      int count, SkPMColor* SK_RESTRICT colors) {
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(s.fInvType <= (SkMatrix::kTranslate_Mask | SkMatrix::kScale_Mask));
    SkASSERT(s.fDoFilter == false);
    SkDEBUGCODE(CHECKSTATE(s);)

    typedef SkPMColor16 SRCTYPE;

    const SRCTYPE* SK_RESTRICT srcAddr = (const SRCTYPE*)s.fBitmap->getPixels();

    // buffer is y32, x16, x16, x16, x16, x16
    // bump srcAddr to the proper row, since we're told Y never changes
    SkASSERT((unsigned)xy[0] < (unsigned)s.fBitmap->height());
    srcAddr = (const SRCTYPE* SK_RESTRICT)((const char*)srcAddr +
                                                xy[0] * s.fBitmap->rowBytes());
    xy += 1;

    SRCTYPE src;

    if (1 == s.fBitmap->width()) {
        src = srcAddr[0];
        SkPMColor dstValue = SkPixel4444ToPixel32(src);
        sk_memset32(colors, dstValue, count);
    } else {
        int i;
        for (i = (count >> 2); i > 0; --i) {
            uint32_t xx0 = *xy++;
            uint32_t xx1 = *xy++;
            uint32_t i0 = UNPACK_PRIMARY_SHORT(xx0);
            uint32_t i1 = UNPACK_SECONDARY_SHORT(xx0);
            uint32_t i2 = UNPACK_PRIMARY_SHORT(xx1);
            uint32_t i3 = UNPACK_SECONDARY_SHORT(xx1);

            // Use ARM NEON instructions to convert 4444 to 8888
            // TODO (sjaved): look into unrolling twice

            // load 4 pixels of 16 bits each into one 16x4 vector
            uint16x4_t pixels16;
            pixels16 = vld1_lane_u16((const uint16_t*)srcAddr + i0, pixels16, 0);
            pixels16 = vld1_lane_u16((const uint16_t*)srcAddr + i1, pixels16, 1);
            pixels16 = vld1_lane_u16((const uint16_t*)srcAddr + i2, pixels16, 2);
            pixels16 = vld1_lane_u16((const uint16_t*)srcAddr + i3, pixels16, 3);

            // expand 16x4 to 32x4
            uint32x4_t pixels32 = vmovl_u16(pixels16);

            // shift left insert
            pixels32 = vsliq_n_u32(pixels32, pixels32, 8);
            pixels32 = (uint32x4_t)vsliq_n_u16((uint16x8_t)pixels32, (uint16x8_t)pixels32, 4);
            pixels32 = (uint32x4_t)vsliq_n_u8((uint8x16_t)pixels32, (uint8x16_t)pixels32, 4);

            // reverse
            uint32x4_t colors32 = (uint32x4_t)vrev32q_u8((uint8x16_t)pixels32);

            // store into colors
            vst1q_u32((uint32_t*)colors, colors32);
            colors+=4;
        }
        const uint16_t* SK_RESTRICT xx = (const uint16_t*)(xy);
        for (i = (count & 3); i > 0; --i) {
            SkASSERT(*xx < (unsigned)s.fBitmap->width());

            src = srcAddr[*xx++]; *colors++ = SkPixel4444ToPixel32(src);
        }
    }
}

// Convert RGBA4444 to RGB565
// TODO (sjaved)
// Currently only works for opaque cases
void S4444_D16_nofilter_DX_neon(const SkBitmapProcState& s,
                                      const uint32_t* SK_RESTRICT xy,
                                      int count, uint16_t* SK_RESTRICT colors) {
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(s.fInvType <= (SkMatrix::kTranslate_Mask | SkMatrix::kScale_Mask));
    SkASSERT(s.fDoFilter == false);
    SkDEBUGCODE(CHECKSTATE(s);)

    typedef SkPMColor16 SRCTYPE;

    const SRCTYPE* SK_RESTRICT srcAddr = (const SRCTYPE*)s.fBitmap->getPixels();

    // buffer is y32, x16, x16, x16, x16, x16
    // bump srcAddr to the proper row, since we're told Y never changes
    SkASSERT((unsigned)xy[0] < (unsigned)s.fBitmap->height());
    srcAddr = (const SRCTYPE* SK_RESTRICT)((const char*)srcAddr +
                                                xy[0] * s.fBitmap->rowBytes());
    xy += 1;

    SRCTYPE src;

    if (1 == s.fBitmap->width()) {
        src = srcAddr[0];
        uint16_t dstValue = SkPixel4444ToPixel16(src);
        sk_memset16(colors, dstValue, count);
    } else {
        int i;
        // Use ARM NEON instructions to convert 4444 to 565
        // TODO (sjaved): write direct asm instructions
        uint16_t* input16 = (uint16_t*)malloc(8 * sizeof(uint16_t));
        for (i = (count >> 2); i > 0; i -= 2) {
            uint32_t xx0 = *xy++;
            uint32_t xx1 = *xy++;
            uint32_t xx2 = *xy++;
            uint32_t xx3 = *xy++;

            uint32_t i0 = UNPACK_PRIMARY_SHORT(xx0);
            uint32_t i1 = UNPACK_SECONDARY_SHORT(xx0);
            uint32_t i2 = UNPACK_PRIMARY_SHORT(xx1);
            uint32_t i3 = UNPACK_SECONDARY_SHORT(xx1);

            uint32_t i4 = UNPACK_PRIMARY_SHORT(xx2);
            uint32_t i5 = UNPACK_SECONDARY_SHORT(xx2);
            uint32_t i6 = UNPACK_PRIMARY_SHORT(xx3);
            uint32_t i7 = UNPACK_SECONDARY_SHORT(xx3);

            // load 8 pixels deinterleaved
            input16[0] = srcAddr[i0];
            input16[1] = srcAddr[i1];
            input16[2] = srcAddr[i2];
            input16[3] = srcAddr[i3];
            input16[4] = srcAddr[i4];
            input16[5] = srcAddr[i5];
            input16[6] = srcAddr[i6];
            input16[7] = srcAddr[i7];
            uint8x8x2_t deinterleaved8 = vld2_u8((const uint8_t*)input16);

            // get rg and ba pixels separately
            uint8x8_t ba = deinterleaved8.val[0]; // lower 8
            uint8x8_t rg = deinterleaved8.val[1];

            // get red
            uint8x8_t r = vsri_n_u8(rg, rg, 4);

            // get green
            // shift left insert by 4
            uint8x8_t g = vsli_n_u8(rg, rg, 4);

            // get blue
            // shift-right insert by 4
            uint8x8_t b = vsri_n_u8(ba, ba, 4);

            // insert top 3 green bits into red
            r = vsri_n_u8(r, g, 5);
            // insert blue bits into green
            g = vshl_n_u8(g, 3);
            b = vsri_n_u8(g, b, 3);

            // expand to 16 bits
            uint16x8_t r16 = vshlq_n_u16(vmovl_u8(r), 8);
            uint16x8_t b16 = vmovl_u8(b);
            uint16x8_t result = vorrq_u16(r16, b16);

            // store into colors
            vst1q_u16((uint16_t*)colors, result);
            colors+=8;
        }
    }
}


// Convert RGB565 to ABGR8888
void S16_opaque_D32_nofilter_DX_neon(const SkBitmapProcState& s,
                                      const uint32_t* SK_RESTRICT xy,
                                      int count, SkPMColor* SK_RESTRICT colors) {
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(s.fInvType <= (SkMatrix::kTranslate_Mask | SkMatrix::kScale_Mask));
    SkASSERT(s.fDoFilter == false);
    SkDEBUGCODE(CHECKSTATE(s);)

    typedef uint16_t SRCTYPE;

    const SRCTYPE* SK_RESTRICT srcAddr = (const SRCTYPE*)s.fBitmap->getPixels();

    // buffer is y32, x16, x16, x16, x16, x16
    // bump srcAddr to the proper row, since we're told Y never changes
    SkASSERT((unsigned)xy[0] < (unsigned)s.fBitmap->height());
    srcAddr = (const SRCTYPE*)((const char*)srcAddr +
                                                xy[0] * s.fBitmap->rowBytes());
    xy += 1;

    SRCTYPE src;

    if (1 == s.fBitmap->width()) {
        src = srcAddr[0];
        SkPMColor dstValue = SkPixel16ToPixel32(src);
        sk_memset32(colors, dstValue, count);
    } else {
        int i;

        // constant vectors used for shifting in NEON CODE
        // require these shifts because of little endian addressing
        int16x8_t shiftGreen = (int16x8_t)vmovq_n_u32(2 << 16);
        int8x16_t shiftRed = (int8x16_t)vmovq_n_u32(3 << 24);
        int8x16_t shiftLsb = {0,-5,-6,-5,0,-5,-6,-5,0,-5,-6,-5,0,-5,-6,-5};
        uint32x4_t orAlpha = vmovq_n_u32(0x000000FF);

        // TODO (sjaved): look into doing an extra unroll
        for (i = (count >> 2); i > 0; --i) {
            uint32_t xx0 = *xy++;
            uint32_t xx1 = *xy++;

            uint32_t i0 = UNPACK_PRIMARY_SHORT(xx0);
            uint32_t i1 = UNPACK_SECONDARY_SHORT(xx0);
            uint32_t i2 = UNPACK_PRIMARY_SHORT(xx1);
            uint32_t i3 = UNPACK_SECONDARY_SHORT(xx1);

            // use NEON instructions to unpack 565 to 8888

            // Load 4 pixels into a 64-bit vector
            uint16x4_t f_pixels16;
            f_pixels16 = vld1_lane_u16((const uint16_t*)srcAddr + i0, f_pixels16, 0);
            f_pixels16 = vld1_lane_u16((const uint16_t*)srcAddr + i1, f_pixels16, 1);
            f_pixels16 = vld1_lane_u16((const uint16_t*)srcAddr + i2, f_pixels16, 2);
            f_pixels16 = vld1_lane_u16((const uint16_t*)srcAddr + i3, f_pixels16, 3);

            // expand 16x4 to 32x4
            uint32x4_t f_pixels32 = vmovl_u16(f_pixels16);

            // shift left to put blue into it's proper position
            f_pixels32 = vshlq_n_u32(f_pixels32, 11);

            // shift left by vector to put green into it's proper position (per word)
            f_pixels32 = (uint32x4_t)vshlq_u16((uint16x8_t)f_pixels32, shiftGreen);
            // shift left by vector again to put red into it's proper position (per byte)
            f_pixels32 = (uint32x4_t)vshlq_u8((uint8x16_t)f_pixels32, shiftRed);

            // moving msbs to lsbs and insert
            uint8x16_t tempReg1 = vshlq_u8((uint8x16_t)f_pixels32, shiftLsb);
            f_pixels32 = (uint32x4_t)vbslq_u8((uint8x16_t)f_pixels32, (uint8x16_t)f_pixels32, tempReg1);

            // alpha
            f_pixels32 = (uint32x4_t)vorrq_u8((uint8x16_t)f_pixels32, (uint8x16_t)orAlpha);

            // reverse
            uint32x4_t f_colors32 = (uint32x4_t)vrev32q_u8((uint8x16_t)f_pixels32);

            // store into colors
            vst1q_u32((uint32_t*)colors, f_colors32);
            colors+=4;
        }
        const uint16_t* SK_RESTRICT xx = (const uint16_t*)(xy);
        for (i = (count & 3); i > 0; --i) {
            SkASSERT(*xx < (unsigned)s.fBitmap->width());

            src = srcAddr[*xx++]; *colors++ = SkPixel16ToPixel32(src);
        }
    }
}

// Bilinear filter for S32_opaque_D32_DX
void S32_opaque_D32_filter_DX_neon(const SkBitmapProcState& s,
                                   const uint32_t* SK_RESTRICT xy,
                                   int count, uint32_t* SK_RESTRICT colors) {
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(s.fDoFilter);
    SkASSERT(s.fBitmap->config() == SkBitmap::kARGB_8888_Config);
    SkASSERT(s.fBitmap->isOpaque());

    const char* SK_RESTRICT srcAddr = (const char*)s.fBitmap->getPixels();
    unsigned rb = s.fBitmap->rowBytes();
    unsigned subY;

    const SkPMColor* SK_RESTRICT row0;
    const SkPMColor* SK_RESTRICT row1;

    // setup row ptrs and update proc_table
    {
        uint32_t XY = *xy++;
        unsigned y0 = XY >> 14;
        row0 = (const SkPMColor*)(srcAddr + (y0 >> 4) * rb);
        row1 = (const SkPMColor*)(srcAddr + (XY & 0x3FFF) * rb);
        subY = y0 & 0xF;
    }

    // we calculate at least 3 pixels (1 in the first pass, at least 1 in the middle and 1 at the end)
    if (count >= 3) {
        // first pass: load 8 pixels
        uint32_t XX = *xy++;    // x0:14 | 4 | x1:14
        unsigned x0 = XX >> 14;
        unsigned x1 = XX & 0x3FFF;
        unsigned subX = x0 & 0xF;
        x0 >>= 4;
        uint32_t XX2 = *xy++;    // x0:14 | 4 | x1:14
        unsigned x02 = XX2 >> 14; // TODO: calculate x02 and x12 using NEON
        unsigned x12 = XX2 & 0x3FFF;
        unsigned subX2 = x02 & 0xF;
        x02 >>= 4;

        x0 *= 4;
        x1 *= 4;

        x02 *= 4;
        x12 *= 4;
        // First pass:
        // We interleave vld ops with other arithmetic operations (loading y, calculating 16-y, multiply ops, etc.)
        // we do this because we want to take advantage of the dual-issue pipeline
        // instruction timing for multiplies is high, and we want to take advantage of it by
        // also doing loads at the same time. The store for this pass will be done in the next iteration.
        // Indented ops: we load the pixels needed for the next iteration
        asm volatile(
                // d0 = y, d1 = 16-y (will be used by all iterations)
                "add            r2, %[row0], %[x0]             \n\t"   // row0 pixel1
                "vdup.8         d0, %[y]                       \n\t"   // duplicate y into d0
                "vld1.32        {d4[0]}, [r2]                  \n\t"   // lane load pixel1
                "vmov.u8        d16, #16                       \n\t"   // set up constant in d16
                "add            r3, %[row0], %[x1]             \n\t"   // row0 pixel2
                "vsub.u8        d1, d16, d0                    \n\t"   // d1 = 16-y
                "vld1.32        {d4[1]}, [r3]                  \n\t"   // lane load pixel2
                "vmov.u16       d16, #16                       \n\t"   // set up constant in d16 (for 16 - x)

                "add            r2, %[row1], %[x0]             \n\t"   // row1 pixel1
                "vld1.32        {d5[0]}, [r2]                  \n\t"   // lane load pixel3
                "add            r3, %[row1], %[x1]             \n\t"   // row1 pixel2
                "vmull.u8       q3, d4, d1                     \n\t"   // q3 [d7|d6] = [a01|a00] * (16-y)
                "vld1.32        {d5[1]}, [r3]                  \n\t"   // lane load pixel4

                    "add            r2, %[row0], %[x02]        \n\t"   // row0 pixel3
                "vmull.u8       q4, d5, d0                     \n\t"   // q4 [d9|d8] = [a11|a10] * y

                "vdup.16        d10, %[x]                      \n\t"   // duplicate x into d10
                "vsub.u16       d11, d16, d10                  \n\t"   // d11 = 16-x
                    "vld1.32        {d4[0]}, [r2]              \n\t"   // lane load pixel5
                "vmul.i16       d12, d7, d10                   \n\t"   // d12  = a01 * x
                    "add            r3, %[row0], %[x12]        \n\t"   // row0 pixel4
                "vmla.i16       d12, d9, d10                   \n\t"   // d12 += a11 * x
                    "vld1.32        {d4[1]}, [r3]              \n\t"   // lane load pixel6
                "vmla.i16       d12, d6, d11                   \n\t"   // d12 += a00 * (16-x)
                    "add            r4, %[row1], %[x02]        \n\t"   // row1 pixel3
                "vmla.i16       d12, d8, d11                   \n\t"   // d12 += a10 * (16-x)
                    "vld1.32        {d5[0]}, [r4]              \n\t"   // lane load pixel7
                    "add            r1, %[row1], %[x12]        \n\t"   // row1 pixel4
                "vshrn.i16      d2, q6, #8                     \n\t"   // shift down result by 8
                    "vld1.32        {d5[1]}, [r1]              \n\t"   // lane load pixel8
                :
                : [x] "r" (subX),[y] "r" (subY), [dst] "r" (colors),
                  [row0] "r" (row0),
                  [row1] "r" (row1),
                  [x0] "r" (x0),
                  [x1] "r" (x1),
                  [x02] "r" (x02),
                  [x12] "r" (x12)
                : "cc", "memory", "r1", "r2", "r3", "r4", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d16"
                    );
        subX = subX2;
        --count;

        // middle iterations
        do {
            XX2 = *xy++;    // x0:14 | 4 | x1:14
            x02 = XX2 >> 14;
            x12 = XX2 & 0x3FFF;
            subX2 = x02 & 0xF;
            x02 >>= 4;

            x02 *= 4;
            x12 *= 4;

            asm volatile(
                    // d0 = y, d1 = 16-y

                    // multiply with interleaved loads
                    // we do a store from the previous iteration in the middle since it produces the best result
                    // indented code is for loading the pixels for the next iteration.
                    // we do this because we want to take advantage of the dual-issue pipeline.
                    // instruction timing for multiplies is high, and we want to take advantage of it by
                    // also doing loads at the same time
                        "add            r2, %[row0], %[x0]         \n\t"   // row0 pixel1
                    "vmull.u8       q3, d4, d1                     \n\t"   // q3 [d7|d6] = [a01|a00] * (16-y)
                        "vld1.32        {d4[0]}, [r2]              \n\t"   // lane load pixel1
                    "vmull.u8       q4, d5, d0                     \n\t"   // q4 [d9|d8] = [a11|a10] * y
                        "add            r3, %[row0], %[x1]         \n\t"   // row0 pixel2

                    "vdup.16        d10, %[x]                      \n\t"   // duplicate x into d10
                        "vld1.32        {d4[1]}, [r3]              \n\t"   // lane load pixel2
                    "vsub.u16       d11, d16, d10                  \n\t"   // d11 = 16-x
                        "vst1.32        {d2[0]}, [%[dst]] !        \n\t"   // store result of previous iteration
                    "vmul.i16       d12, d7, d10                   \n\t"   // d12  = a01 * x
                        "add            r2, %[row1], %[x0]         \n\t"   // row1 pixel1
                    "vmla.i16       d12, d9, d10                   \n\t"   // d12 += a11 * x
                        "vld1.32        {d5[0]}, [r2]              \n\t"   // lane load pixel3
                    "vmla.i16       d12, d6, d11                   \n\t"   // d12 += a00 * (16-x)
                        "add            r3, %[row1], %[x1]         \n\t"   // row1 pixel2
                    "vmla.i16       d12, d8, d11                   \n\t"   // d12 += a10 * (16-x)
                        "vld1.32        {d5[1]}, [r3]              \n\t"   // lane load pixel4
                    "vshrn.i16      d2, q6, #8                     \n\t"   // shift down result by 8
                    :
                    : [x] "r" (subX), [dst] "r" (colors),
                      [row0] "r" (row0),
                      [row1] "r" (row1),
                      [x0] "r" (x02),
                      [x1] "r" (x12)
                    : "cc", "memory", "r2", "r3", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d16"
                        );

            subX = subX2;
        } while (--count > 1);

        // last case: no more loads, just do a bilinear filter
        asm volatile(
                // d0 = y, d1 = 16-y
                "vmull.u8       q3, d4, d1                     \n\t"   // q3 [d7|d6] = [a01|a00] * (16-y)
                    "vst1.32        {d2[0]}, [%[dst]] !        \n\t"   // store result of previous iteration
                "vmull.u8       q4, d5, d0                     \n\t"   // q4 [d9|d8] = [a11|a10] * y

                "vdup.16        d10, %[x]                      \n\t"   // duplicate x into d10
                "vsub.u16       d11, d16, d10                  \n\t"   // d11 = 16-x

                "vmul.i16       d12, d7, d10                   \n\t"   // d12  = a01 * x
                "vmla.i16       d12, d9, d10                   \n\t"   // d12 += a11 * x
                "vmla.i16       d12, d6, d11                   \n\t"   // d12 += a00 * (16-x)
                "vmla.i16       d12, d8, d11                   \n\t"   // d12 += a10 * (16-x)
                "vshrn.i16      d2, q6, #8                     \n\t"   // shift down result by 8
                "vst1.32        {d2[0]}, [%[dst]]              \n\t"   // store result
                :
                : [x] "r" (subX), [dst] "r" (colors)
                : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d16"
                );
    }
    else { // only need to handle a few pixels, so use the older version
        do {
            uint32_t XX = *xy++;    // x0:14 | 4 | x1:14
            unsigned x0 = XX >> 14;
            unsigned x1 = XX & 0x3FFF;
            unsigned subX = x0 & 0xF;
            x0 >>= 4;
            asm volatile(
                    "vdup.8         d0, %[y]                \n\t"   // duplicate y into d0
                    "vmov.u8        d16, #16                \n\t"   // set up constant in d16
                    "vsub.u8        d1, d16, d0             \n\t"   // d1 = 16-y

                    "vdup.32        d4, %[a00]              \n\t"   // duplicate a00 into d4
                    "vdup.32        d5, %[a10]              \n\t"   // duplicate a10 into d5
                    "vmov.32        d4[1], %[a01]           \n\t"   // set top of d4 to a01
                    "vmov.32        d5[1], %[a11]           \n\t"   // set top of d5 to a11

                    "vmull.u8       q3, d4, d1              \n\t"   // q3 = [a01|a00] * (16-y)
                    "vmull.u8       q0, d5, d0              \n\t"   // q0 = [a11|a10] * y

                    "vdup.16        d5, %[x]                \n\t"   // duplicate x into d5
                    "vmov.u16       d16, #16                \n\t"   // set up constant in d16
                    "vsub.u16       d3, d16, d5             \n\t"   // d3 = 16-x

                    "vmul.i16       d4, d7, d5              \n\t"   // d4  = a01 * x
                    "vmla.i16       d4, d1, d5              \n\t"   // d4 += a11 * x
                    "vmla.i16       d4, d6, d3              \n\t"   // d4 += a00 * (16-x)
                    "vmla.i16       d4, d0, d3              \n\t"   // d4 += a10 * (16-x)
                    "vshrn.i16      d0, q2, #8              \n\t"   // shift down result by 8
                    "vst1.32        {d0[0]}, [%[dst]] !     \n\t"   // store result
                    :
                    : [x] "r" (subX), [y] "r" (subY), [a00] "r" (row0[x0]), [a01] "r" (row0[x1]), [a10] "r" (row1[x0]), [a11] "r" (row1[x1]), [dst] "r" (colors)
                    : "cc", "memory", "r4", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d16"
                );
        } while (--count > 0);
    }
}

// Convert ABGR8888 to RGB565
void S32_opaque_D16_nofilter_DX_neon(const SkBitmapProcState& s,
                                      const uint32_t* SK_RESTRICT xy,
                                      int count, uint16_t* SK_RESTRICT colors) {
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(s.fInvType <= (SkMatrix::kTranslate_Mask | SkMatrix::kScale_Mask));
    SkASSERT(s.fDoFilter == false);
    SkDEBUGCODE(CHECKSTATE(s);)

    typedef SkPMColor SRCTYPE;

    const SRCTYPE* SK_RESTRICT srcAddr = (const SRCTYPE*)s.fBitmap->getPixels();

    // buffer is y32, x16, x16, x16, x16, x16
    // bump srcAddr to the proper row, since we're told Y never changes
    SkASSERT((unsigned)xy[0] < (unsigned)s.fBitmap->height());
    srcAddr = (const SRCTYPE*)((const char*)srcAddr +
                                                xy[0] * s.fBitmap->rowBytes());
    xy += 1;

    SRCTYPE src;

    if (1 == s.fBitmap->width()) {
        src = srcAddr[0];
        uint16_t dstValue = (uint16_t)SkPixel32ToPixel16(src);
        sk_memset16(colors, dstValue, count);
    } else {
        int i;

        // declare variables
// masks used for bit select
#define MASK_GREEN (0x7E0)
#define MASK_RED   (0xF800)
        const uint32x4_t maskGreen = {MASK_GREEN, MASK_GREEN, MASK_GREEN, MASK_GREEN};
        const uint32x4_t maskRed = {MASK_RED, MASK_RED, MASK_RED, MASK_RED};
        uint32x4_t pixels32;

        for (i = (count >> 2); i > 0; --i) {
            uint32_t xx0 = *xy++;
            uint32_t xx1 = *xy++;

            uint32_t i0 = UNPACK_PRIMARY_SHORT(xx0);
            uint32_t i1 = UNPACK_SECONDARY_SHORT(xx0);
            uint32_t i2 = UNPACK_PRIMARY_SHORT(xx1);
            uint32_t i3 = UNPACK_SECONDARY_SHORT(xx1);

            // Load 4 pixels into a 128-bit vector using intrinsics
            // We use intrinsics because lane loads were not properly working in asm
            pixels32 = vld1q_lane_u32((const uint32_t*)srcAddr + i0, pixels32, 0);
            pixels32 = vld1q_lane_u32((const uint32_t*)srcAddr + i1, pixels32, 1);
            pixels32 = vld1q_lane_u32((const uint32_t*)srcAddr + i2, pixels32, 2);
            pixels32 = vld1q_lane_u32((const uint32_t*)srcAddr + i3, pixels32, 3);

            // NEON assembly
            asm volatile (
                "vrev32.u8      q0, %q[read]            \n\t"    // reverse: ABGR -> RGBA
                "vshr.u32       q0, q0, #11             \n\t"    // shift out alpha bits, put B in proper position
                "vshr.u32       q1, q0, #2              \n\t"    // move G into proper position, put in another register
                "vshr.u32       q2, q0, #5              \n\t"    // move R into proper position, put in another register
                "vbit           q0, q1, %q[maskGreen]   \n\t"    // put G into destination
                "vbit           q0, q2, %q[maskRed]     \n\t"    // put R into destination
                "vmovn.i32      d8, q0                  \n\t"    // narrow down to 16-bit vector
                "vst1.16        {d8}, [%[dstPtrReg]] !  \n\t"    // store and automatically increment
                : [dstPtrReg] "+r" ( colors )
                : [maskGreen] "w"  ( maskGreen ),
                  [maskRed]   "w"  ( maskRed ),
                  [read]      "w"  ( pixels32 )
                : "memory", "r1", "r2", "r3", "r4", "d0", "d6", "d7", "d1", "d8", "q0", "q1", "q2", "q3"
            );
        }
        const uint16_t* SK_RESTRICT xx = (const uint16_t*)(xy);
        for (i = (count & 3); i > 0; --i) {
            SkASSERT(*xx < (unsigned)s.fBitmap->width());

            src = srcAddr[*xx++]; *colors++ = SkPixel32ToPixel16(src);
        }
    }
}

// Bilinear filter for S32_alpha_D32_DX
void S32_alpha_D32_filter_DX_neon(const SkBitmapProcState& s,
                                   const uint32_t* SK_RESTRICT xy,
                                   int count, uint32_t* SK_RESTRICT colors) {
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(s.fDoFilter);
    SkASSERT(s.fBitmap->config() == SkBitmap::kARGB_8888_Config);
    SkASSERT(!s.fBitmap->isOpaque());

    // alpha
    SkASSERT(s.fAlphaScale < 256);
    unsigned alphaScale = s.fAlphaScale;

    const char* SK_RESTRICT srcAddr = (const char*)s.fBitmap->getPixels();
    unsigned rb = s.fBitmap->rowBytes();
    unsigned subY;

    const SkPMColor* SK_RESTRICT row0;
    const SkPMColor* SK_RESTRICT row1;

    // setup row ptrs and update proc_table
    {
        uint32_t XY = *xy++;
        unsigned y0 = XY >> 14;
        row0 = (const SkPMColor*)(srcAddr + (y0 >> 4) * rb);
        row1 = (const SkPMColor*)(srcAddr + (XY & 0x3FFF) * rb);
        subY = y0 & 0xF;
    }

    // we calculate at least 3 pixels (1 in the first pass, at least 1 in the middle and 1 at the end)
    if (count >= 3) {
        // first pass: load 8 pixels
        uint32_t XX = *xy++;    // x0:14 | 4 | x1:14
        unsigned x0 = XX >> 14;
        unsigned x1 = XX & 0x3FFF;
        unsigned subX = x0 & 0xF;
        x0 >>= 4;
        uint32_t XX2 = *xy++;    // x0:14 | 4 | x1:14
        unsigned x02 = XX2 >> 14; // TODO: calculate x02 and x12 using NEON
        unsigned x12 = XX2 & 0x3FFF;
        unsigned subX2 = x02 & 0xF;
        x02 >>= 4;

        x0 *= 4;
        x1 *= 4;

        x02 *= 4;
        x12 *= 4;
        // First pass:
        // We interleave vld ops with other arithmetic operations (loading y, calculating 16-y, multiply ops, etc.)
        // we do this because we want to take advantage of the dual-issue pipeline
        // instruction timing for multiplies is high, and we want to take advantage of it by
        // also doing loads at the same time. The store for this pass will be done in the next iteration.
        // Indented ops: we load the pixels needed for the next iteration
        asm volatile(
                // d0 = y, d1 = 16-y (will be used by all iterations)
                "add            r2, %[row0], %[x0]             \n\t"   // row0 pixel1
                "vdup.8         d0, %[y]                       \n\t"   // duplicate y into d0
                "vld1.32        {d4[0]}, [r2]                  \n\t"   // lane load pixel1
                "vmov.u8        d16, #16                       \n\t"   // set up constant in d16
                "add            r3, %[row0], %[x1]             \n\t"   // row0 pixel2
                "vsub.u8        d1, d16, d0                    \n\t"   // d1 = 16-y
                "vld1.32        {d4[1]}, [r3]                  \n\t"   // lane load pixel2
                "vmov.u16       d16, #16                       \n\t"   // set up constant in d16 (for 16 - x)

                "add            r2, %[row1], %[x0]             \n\t"   // row1 pixel1
                "vld1.32        {d5[0]}, [r2]                  \n\t"   // lane load pixel3
                "add            r3, %[row1], %[x1]             \n\t"   // row1 pixel2
                "vmull.u8       q3, d4, d1                     \n\t"   // q3 [d7|d6] = [a01|a00] * (16-y)
                "vld1.32        {d5[1]}, [r3]                  \n\t"   // lane load pixel4

                    "add            r2, %[row0], %[x02]        \n\t"   // row0 pixel3
                "vmull.u8       q4, d5, d0                     \n\t"   // q4 [d9|d8] = [a11|a10] * y

                "vdup.16        d10, %[x]                      \n\t"   // duplicate x into d10
                "vsub.u16       d11, d16, d10                  \n\t"   // d11 = 16-x
                    "vld1.32        {d4[0]}, [r2]              \n\t"   // lane load pixel5
                "vmul.i16       d12, d7, d10                   \n\t"   // d12  = a01 * x
                    "add            r3, %[row0], %[x12]        \n\t"   // row0 pixel4
                "vdup.16        d3, %[scale]                   \n\t"   // duplicate scale into d3
                "vmla.i16       d12, d9, d10                   \n\t"   // d12 += a11 * x
                    "vld1.32        {d4[1]}, [r3]              \n\t"   // lane load pixel6
                "vmla.i16       d12, d6, d11                   \n\t"   // d12 += a00 * (16-x)
                    "add            r4, %[row1], %[x02]        \n\t"   // row1 pixel3
                "vmla.i16       d12, d8, d11                   \n\t"   // d12 += a10 * (16-x)
                    "vld1.32        {d5[0]}, [r4]              \n\t"   // lane load pixel7
                "vshr.u16       d12, d12, #8                   \n\t"   // shift down result by 8
                    "add            r2, %[row1], %[x12]        \n\t"   // row1 pixel4
                "vmul.i16       d12, d12, d3                   \n\t"   // multiply result by scale
                    "vld1.32        {d5[1]}, [r2]              \n\t"   // lane load pixel8
                :
                : [x] "r" (subX),[y] "r" (subY), [dst] "r" (colors), [scale] "r" (alphaScale),
                  [row0] "r" (row0),
                  [row1] "r" (row1),
                  [x0] "r" (x0),
                  [x1] "r" (x1),
                  [x02] "r" (x02),
                  [x12] "r" (x12)
                : "cc", "memory", "r2", "r3", "r4", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d12"
                );
        subX = subX2;
        --count;

        // middle iterations
        do {
            XX2 = *xy++;    // x0:14 | 4 | x1:14
            x02 = XX2 >> 14;
            x12 = XX2 & 0x3FFF;
            subX2 = x02 & 0xF;
            x02 >>= 4;

            x02 *= 4;
            x12 *= 4;

            asm volatile(
                    // d0 = y, d1 = 16-y

                    // multiply with interleaved loads
                    // we do a store from the previous iteration in the middle since it produces the best result
                    // indented code is for loading the pixels for the next iteration.
                    // we do this because we want to take advantage of the dual-issue pipeline.
                    // instruction timing for multiplies is high, and we want to take advantage of it by
                    // also doing loads at the same time
                        "add            r2, %[row0], %[x0]         \n\t"   // row0 pixel1
                    "vshrn.i16      d2, q6, #8                     \n\t"   // shift down result by 8
                        "add            r3, %[row0], %[x1]         \n\t"   // row0 pixel2
                        "pld            [r3]                       \n\t"
                    "vmull.u8       q3, d4, d1                     \n\t"   // q3 [d7|d6] = [a01|a00] * (16-y)
                        "vld1.32        {d4[0]}, [r2]              \n\t"   // lane load pixel1
                    "vmull.u8       q4, d5, d0                     \n\t"   // q4 [d9|d8] = [a11|a10] * y

                    "vdup.16        d10, %[x]                      \n\t"   // duplicate x into d10
                        "add            r2, %[row1], %[x0]         \n\t"   // row1 pixel1
                        "pld            [r2]                       \n\t"
                    "vsub.u16       d11, d16, d10                  \n\t"   // d11 = 16-x
                        "vst1.32        {d2[0]}, [%[dst]] !        \n\t"   // store result of previous iteration
                    "vmul.i16       d12, d7, d10                   \n\t"   // d12  = a01 * x
                        "vld1.32        {d4[1]}, [r3]              \n\t"   // lane load pixel2
                    "vdup.16        d3, %[scale]                   \n\t"   // duplicate scale into d3
                    "vmla.i16       d12, d9, d10                   \n\t"   // d12 += a11 * x
                    "vmla.i16       d12, d6, d11                   \n\t"   // d12 += a00 * (16-x)
                        "add            r3, %[row1], %[x1]         \n\t"   // row1 pixel2
                        "pld            [r3]                       \n\t"
                    "vmla.i16       d12, d8, d11                   \n\t"   // d12 += a10 * (16-x)
                        "vld1.32        {d5[0]}, [r2]              \n\t"   // lane load pixel3
                    "vshr.u16       d12, d12, #8                   \n\t"   // shift down result by 8
                    "vmul.i16       d12, d12, d3                   \n\t"   // multiply result by scale
                        "vld1.32        {d5[1]}, [r3]              \n\t"   // lane load pixel4
                    :
                    : [x] "r" (subX), [dst] "r" (colors), [scale] "r" (alphaScale),
                      [row0] "r" (row0),
                      [row1] "r" (row1),
                      [x0] "r" (x02),
                      [x1] "r" (x12)
                    : "cc", "memory", "r2", "r3", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d12", "d16"
                        );

            subX = subX2;
        } while (--count > 1);

        // last case: no more loads, just do a bilinear filter
        asm volatile(
                // d0 = y, d1 = 16-y
                    "vshrn.i16      d2, q6, #8                     \n\t"   // shift down result by 8
                "vmull.u8       q3, d4, d1                     \n\t"   // q3 [d7|d6] = [a01|a00] * (16-y)
                    "vst1.32        {d2[0]}, [%[dst]] !        \n\t"   // store result of previous iteration
                "vmull.u8       q4, d5, d0                     \n\t"   // q4 [d9|d8] = [a11|a10] * y

                "vdup.16        d10, %[x]                      \n\t"   // duplicate x into d10
                "vsub.u16       d11, d16, d10                  \n\t"   // d11 = 16-x

                "vmul.i16       d12, d7, d10                   \n\t"   // d12  = a01 * x
                "vmla.i16       d12, d9, d10                   \n\t"   // d12 += a11 * x
                "vmla.i16       d12, d6, d11                   \n\t"   // d12 += a00 * (16-x)
                "vmla.i16       d12, d8, d11                   \n\t"   // d12 += a10 * (16-x)
                "vdup.16        d3, %[scale]                   \n\t"   // duplicate scale into d3
                "vshr.u16       d12, d12, #8                   \n\t"   // shift down result by 8
                "vmul.i16       d12, d12, d3                   \n\t"   // multiply result by scale
                "vshrn.i16      d2, q6, #8                     \n\t"   // shift down result by 8
                "vst1.32        {d2[0]}, [%[dst]]              \n\t"   // store result
                :
                : [x] "r" (subX), [dst] "r" (colors), [scale] "r" (alphaScale)
                : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11", "d12", "d16"
                );
    }
    else { // only need to handle a few pixels, so use the older version
        do {
            uint32_t XX = *xy++;    // x0:14 | 4 | x1:14
            unsigned x0 = XX >> 14;
            unsigned x1 = XX & 0x3FFF;
            unsigned subX = x0 & 0xF;
            x0 >>= 4;
            asm volatile(
                    "vdup.8         d0, %[y]                \n\t"   // duplicate y into d0
                    "vmov.u8        d16, #16                \n\t"   // set up constant in d16
                    "vsub.u8        d1, d16, d0             \n\t"   // d1 = 16-y

                    "vdup.32        d4, %[a00]              \n\t"   // duplicate a00 into d4
                    "vdup.32        d5, %[a10]              \n\t"   // duplicate a10 into d5
                    "vmov.32        d4[1], %[a01]           \n\t"   // set top of d4 to a01
                    "vmov.32        d5[1], %[a11]           \n\t"   // set top of d5 to a11

                    "vmull.u8       q3, d4, d1              \n\t"   // q3 = [a01|a00] * (16-y)
                    "vmull.u8       q0, d5, d0              \n\t"   // q0 = [a11|a10] * y

                    "vdup.16        d5, %[x]                \n\t"   // duplicate x into d5
                    "vmov.u16       d16, #16                \n\t"   // set up constant in d16
                    "vsub.u16       d3, d16, d5             \n\t"   // d3 = 16-x

                    "vmul.i16       d4, d7, d5              \n\t"   // d4  = a01 * x
                    "vmla.i16       d4, d1, d5              \n\t"   // d4 += a11 * x
                    "vmla.i16       d4, d6, d3              \n\t"   // d4 += a00 * (16-x)
                    "vmla.i16       d4, d0, d3              \n\t"   // d4 += a10 * (16-x)
                    "vdup.16        d3, %[scale]            \n\t"   // duplicate scale into d3
                    "vshr.u16       d4, d4, #8              \n\t"   // shift down result by 8
                    "vmul.i16       d4, d4, d3              \n\t"   // multiply result by scale
                    "vshrn.i16      d0, q2, #8              \n\t"   // shift down result by 8
                    "vst1.32        {d0[0]}, [%[dst]] !     \n\t"   // store result
                    :
                    : [x] "r" (subX), [y] "r" (subY), [a00] "r" (row0[x0]), [a01] "r" (row0[x1]), [a10] "r" (row1[x0]), [a11] "r" (row1[x1]), [dst] "r" (colors), [scale] "r" (alphaScale)
                    : "cc", "memory", "r4", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d16"
                );
        } while (--count > 0);
    }
}

// These are NEON optimisations of non-indexed shaderprocs for
// translation-only, opaque, clamp-clamp and repeat-repeat cases.  Each NEON
// body is 4 pixels unrolled (except for 4444 -> 565 where it's 8 pixels)

#define MAKENAME(suffix)        S32_D32_opt_neon ## suffix
#define DSTSIZE                 32
#define SRCTYPE                 SkPMColor
#define CHECKSTATE(state)       SkASSERT(state.fBitmap->config() == SkBitmap::kARGB_8888_Config); \
                                SkASSERT(state.fAlphaScale == 256)
#define RETURNDST(src)          src
#define UNROLLED_4X_BODY(dstPtr, srcPtr) \
    asm volatile( \
        "vld1.32 {d0}, [%[srcPtrReg]] !        \n\t" \
        "vst1.32 {d0}, [%[dstPtrReg]] !        \n\t" \
        : [dstPtrReg] "+r" ( dstPtr ), [srcPtrReg] "+r" ( srcPtr ) \
        : \
        : "memory", "q0" \
        )
#include "SkBitmapProcState_transonly_nofilter_shaderproc.h"

#define MAKENAME(suffix)        S4444_D32_opt_neon ## suffix
#define DSTSIZE                 32
#define SRCTYPE                 SkPMColor16
#define CHECKSTATE(state)       SkASSERT(state.fBitmap->config() == SkBitmap::kARGB_4444_Config); \
                                SkASSERT(state.fAlphaScale == 256)
#define RETURNDST(src)          SkPixel4444ToPixel32(src)
#define UNROLLED_4X_BODY(dstPtr, srcPtr) \
    asm volatile( \
        "vld1.16 {d0}, [%[srcPtrReg]] !        \n\t" \
        "vmovl.u16 q0, d0                      \n\t" \
        "vsliq.u32 q0, q0, #8                  \n\t" \
        "vsliq.u16 q0, q0, #4                  \n\t" \
        "vsliq.u8 q0, q0, #4                   \n\t" \
        "vrev32q.u8 q0, q0                     \n\t" \
        "vst1.32 {q0}, [%[dstPtrReg]] !        \n\t" \
        : [dstPtrReg] "+r" ( dstPtr ), [srcPtrReg] "+r" ( srcPtr ) \
        : \
        : "memory", "q0" \
        )
#include "SkBitmapProcState_transonly_nofilter_shaderproc.h"

#define MAKENAME(suffix)        S16_D32_opt_neon ## suffix
#define DSTSIZE                 32
#define SRCTYPE                 SkPMColor16
#define RETURNDST(src)          SkPixel16ToPixel32(src)
#define CHECKSTATE(state)       SkASSERT(state.fBitmap->config() == SkBitmap::kRGB_565_Config); \
                                SkASSERT(state.fAlphaScale == 256)
// Setup masks
#define PREAMBLE(s) \
    int16x8_t shiftGreen = (int16x8_t)vmovq_n_u32(2 << 16); \
    int8x16_t shiftRed = (int8x16_t)vmovq_n_u32(3 << 24); \
    int8x16_t shiftLsb = {0,-5,-6,-5,0,-5,-6,-5,0,-5,-6,-5,0,-5,-6,-5}; \
    uint32x4_t orAlpha = vmovq_n_u32(0x000000FF);
#define UNROLLED_4X_BODY(dstPtr, srcPtr) \
    asm volatile( \
        "vld1.16 {d0}, [%[srcPtrReg]] !        \n\t" \
        "vmovl.u16 q0, d0                      \n\t" \
        "vshlq.u32 q0, q0, #11                 \n\t" \
        "vshlq.u16 q0, q0, %q[shiftGreen]      \n\t" \
        "vshlq.u8 q0, q0, %q[shiftRed]         \n\t" \
        "vshlq.u8 q1, q0, %q[shiftLsb]         \n\t" \
        "vbslq.u8 q0, q0, q1                   \n\t" \
        "vorrq.u8 q0, q0, %q[orAlpha]          \n\t" \
        "vrev32.u8 q0, q0                      \n\t" \
        "vst1.32 {q0}, [%[dstPtrReg]] !        \n\t" \
        : [dstPtrReg] "+r" ( dstPtr ), [srcPtrReg] "+r" ( srcPtr ) \
        : [shiftGreen] "w" ( shiftGreen ), \
            [shiftRed] "w" ( shiftRed ), \
            [shiftLsb] "w" ( shiftLsb ), \
            [orAlpha] "w" ( orAlpha ) \
        : "memory", "q0", "q1" \
        )
#include "SkBitmapProcState_transonly_nofilter_shaderproc.h"

// TODO jmccaffrey & sjaved
// 8888 -> 565 fastpath
#define MAKENAME(suffix)        S32_D16_opt_neon ## suffix
#define DSTSIZE                 16
#define SRCTYPE                 SkPMColor
#define RETURNDST(src)          SkPixel32ToPixel16(src)
#define CHECKSTATE(state)       SkASSERT(state.fBitmap->config() == SkBitmap::kRGB_8888_Config)
#define MASK_GREEN (0x7E0)
#define MASK_RED   (0xF800)
#define PREAMBLE(s) \
        const uint32x4_t maskGreen = {MASK_GREEN, MASK_GREEN, MASK_GREEN, MASK_GREEN}; \
        const uint32x4_t maskRed = {MASK_RED, MASK_RED, MASK_RED, MASK_RED};
#define UNROLLED_4X_BODY(dstPtr, srcPtr) \
    asm volatile ( \
        "vld1.32        {d6-d7}, [%[srcPtrReg]]!\n\t" \
        "vrev32.u8      q0, q3                  \n\t" \
        "vshr.u32       q0, q0, #11             \n\t" \
        "vshr.u32       q1, q0, #2              \n\t" \
        "vshr.u32       q2, q0, #5              \n\t" \
        "vbit           q0, q1, %q[maskGreen]   \n\t" \
        "vbit           q0, q2, %q[maskRed]     \n\t" \
        "vmovn.i32      d8, q0                  \n\t" \
        "vst1.16        {d8}, [%[dstPtrReg]] !  \n\t" \
        : [dstPtrReg] "+r" ( colors ), [srcPtrReg] "+r" ( srcPtr ) \
        : [maskGreen] "w"  ( maskGreen ), \
          [maskRed]   "w"  ( maskRed ) \
        : "memory", "d0", "d1", "d6", "d7", "d8", "q0", "q1", "q2", "q3" \
        )
#include "SkBitmapProcState_transonly_nofilter_shaderproc.h"

// TODO sjaved
// 4444 -> 565 fastpath
#define MAKENAME(suffix)        S4444_D16_opt_neon ## suffix
#define DSTSIZE                 16
#define SRCTYPE                 SkPMColor16
#define RETURNDST(src)          SkPixel4444ToPixel16(src)
#define CHECKSTATE(state)       SkASSERT(state.fBitmap->config() == SkBitmap::kRGB_4444_Config)
#define UNROLLED_8X_BODY(dstPtr, srcPtr) \
    asm volatile ( \
        "vld2.8         {d0-d1}, [%[srcPtrReg]]  \n\t" \
        "vshl.i8        d2, d1, #0               \n\t" \
        "vsri.8         d2, d2, #4               \n\t" \
        "vshl.i8        d3, d1, #0               \n\t" \
        "vsli.8         d3, d3, #4               \n\t" \
        "vshl.i8        d4, d0, #0               \n\t" \
        "vsri.8         d4, d4, #4               \n\t" \
        "vsri.8         d2, d3, #5               \n\t" \
        "vshl.i8        d3, d3, #3               \n\t" \
        "vsri.8         d3, d4, #3               \n\t" \
        "vmovl.u8       q0, d2                   \n\t" \
        "vshl.i16       q0, q0, #8               \n\t" \
        "vmovl.u8       q1, d4                   \n\t" \
        "vorr           q2, q0, q1               \n\t" \
        "vst1.16        {d4-d5}, [%[dstPtrReg]]  \n\t" \
        : [dstPtrReg] "+r" ( colors ), [srcPtrReg] "+r" ( srcPtr ) \
        : \
        : "memory", "d0", "d1", "d2", "d3", "d4", "d5", "q1", "q2", "q0" \
        )
#include "SkBitmapProcState_transonly_nofilter_shaderproc.h"

#define MAKENAME(suffix)        S16_D16_opt_neon ## suffix
#define DSTSIZE                 16
#define SRCTYPE                 uint16_t
#define RETURNDST(src)          src
#define CHECKSTATE(state)       SkASSERT(state.fBitmap->config() == SkBitmap::kRGB_565_Config)
#define UNROLLED_4X_BODY(dstPtr, srcPtr) \
    asm volatile( \
        "vld1.16 {d0}, [%[srcPtrReg]] !        \n\t" \
        "vst1.16 {d0}, [%[dstPtrReg]] !        \n\t" \
        : [dstPtrReg] "+r" ( dstPtr ), [srcPtrReg] "+r" ( srcPtr ) \
        : \
        : "memory", "d0" \
        )
#include "SkBitmapProcState_transonly_nofilter_shaderproc.h"

#endif // #if defined(SK_CPU_LENDIAN) && defined(__ARM_HAVE_NEON)

///////////////////////////////////////////////////////////////////////////////

/*  If we replace a sampleproc, then we null-out the associated shaderproc,
    otherwise the shader won't even look at the matrix/sampler
 */
void SkBitmapProcState::platformProcs() {
    bool doFilter = fDoFilter;
    bool isOpaque = 256 == fAlphaScale;
    bool justDx = false;

    if (fInvType <= (SkMatrix::kTranslate_Mask | SkMatrix::kScale_Mask)) {
        justDx = true;
    }

    switch (fBitmap->config()) {
        case SkBitmap::kIndex8_Config:
#if __ARM_ARCH__ >= 6 && defined(SK_CPU_LENDIAN)
            if (justDx && !doFilter) {
#if 0   /* crashing on android device */
                fSampleProc16 = SI8_D16_nofilter_DX_arm;
                fShaderProc16 = NULL;
#endif
                if (isOpaque) {
                    // this one is only very slighty faster than the C version
                    fSampleProc32 = SI8_opaque_D32_nofilter_DX_arm;
                    fShaderProc32 = NULL;
                }
            }
#endif
            break;
#if defined(SK_CPU_LENDIAN) && defined(__ARM_HAVE_NEON)
        case SkBitmap::kARGB_4444_Config:       // RGBA 4444
            if (justDx && !doFilter) {
                if (isOpaque) {
                    fSampleProc32 = S4444_opaque_D32_nofilter_DX_neon;

                    // Take the accelerated shaderproc if it's applicable
                    // else use the accelerated sampler proc
                    if (fShaderProc32 == S4444_D32_opaque_repeat_transonly_nofilter_shaderproc_DX) {
                        fShaderProc32 = S4444_D32_opt_neon_opaque_repeat_transonly_nofilter_shaderproc_DX;
                    } else if (fShaderProc32 == S4444_D32_opaque_clamp_transonly_nofilter_shaderproc_DX) {
                        fShaderProc32 = S4444_D32_opt_neon_opaque_clamp_transonly_nofilter_shaderproc_DX;
                    } else {
                        fShaderProc32 = NULL;
                    }

                    fSampleProc16 = S4444_D16_nofilter_DX_neon;
                    if (fShaderProc16 == S4444_D16_opaque_repeat_transonly_nofilter_shaderproc_DX) {
                        fShaderProc16 = S4444_D16_opt_neon_opaque_repeat_transonly_nofilter_shaderproc_DX;
                    } else if (fShaderProc16 == S4444_D16_opaque_clamp_transonly_nofilter_shaderproc_DX) {
                        fShaderProc16 = S4444_D16_opt_neon_opaque_clamp_transonly_nofilter_shaderproc_DX;
                    } else {
                        fShaderProc16 = NULL;
                    }
                }
            }
            break;
        case SkBitmap::kRGB_565_Config:         // RGB 565
            if (justDx && !doFilter) {
                if (isOpaque) {
                    fSampleProc32 = S16_opaque_D32_nofilter_DX_neon;
                    if (fShaderProc32 == S16_D32_opaque_repeat_transonly_nofilter_shaderproc_DX) {
                        fShaderProc32 = S16_D32_opt_neon_opaque_repeat_transonly_nofilter_shaderproc_DX;
                    } else if (fShaderProc32 == S16_D32_opaque_clamp_transonly_nofilter_shaderproc_DX) {
                        fShaderProc32 = S16_D32_opt_neon_opaque_clamp_transonly_nofilter_shaderproc_DX;
                    } else {
                        fShaderProc32 = NULL;
                    }

                    if (fShaderProc16 == S16_D16_opaque_repeat_transonly_nofilter_shaderproc_DX) {
                        fShaderProc16 = S16_D16_opt_neon_opaque_repeat_transonly_nofilter_shaderproc_DX;
                    } else if (fShaderProc16 == S16_D16_opaque_clamp_transonly_nofilter_shaderproc_DX) {
                        fShaderProc16 = S16_D16_opt_neon_opaque_clamp_transonly_nofilter_shaderproc_DX;
                    }
                }
            }
            break;
        case SkBitmap::kARGB_8888_Config:      // RGBA 8888
            if (justDx) {
                if (doFilter) {
                    if (isOpaque) {
                        fSampleProc32 = S32_opaque_D32_filter_DX_neon;
                    }
                    else {
                        fSampleProc32 = S32_alpha_D32_filter_DX_neon;
                    }
                }
                else {
                    if (isOpaque) {
                        if (fShaderProc32 == S32_D32_opaque_repeat_transonly_nofilter_shaderproc_DX) {
                            fShaderProc32 = S32_D32_opt_neon_opaque_repeat_transonly_nofilter_shaderproc_DX;
                        } else if (fShaderProc32 == S32_D32_opaque_clamp_transonly_nofilter_shaderproc_DX) {
                            fShaderProc32 = S32_D32_opt_neon_opaque_clamp_transonly_nofilter_shaderproc_DX;
                        }
                        else {
                            fShaderProc32 = NULL;
                        }

                        fSampleProc16 = S32_opaque_D16_nofilter_DX_neon; // convert to RGB565
                        if (fShaderProc16 == S32_D16_opaque_repeat_transonly_nofilter_shaderproc_DX) {
                            fShaderProc16 = S32_D16_opt_neon_opaque_repeat_transonly_nofilter_shaderproc_DX;
                        }
                        else if (fShaderProc16 == S32_D16_opaque_clamp_transonly_nofilter_shaderproc_DX) {
                            fShaderProc16 = S32_D16_opt_neon_opaque_clamp_transonly_nofilter_shaderproc_DX;
                        }
                    }
                }
            }
            break;
#endif // defined(SK_CPU_LENDIAN)) && defined(__ARM_HAVE_NEON)
        default:
            break;
    }
}

