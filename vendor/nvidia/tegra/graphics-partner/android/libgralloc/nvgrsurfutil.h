/*
 * Copyright (c) 2009 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_NVGRSURFUTIL_H
#define INCLUDED_NVGRSURFUTIL_H

#include <hardware/gralloc.h>
#include <nvrm_surface.h>
#include <nvassert.h>

static NV_INLINE NvBool
NvGrIsNpot(NvU32 val)
{
    return val != (val & (NvU32)-(NvS32)val);
}

/*
 * Mapping of HAL formats to NvColor formats.
 * This has to be a 1-to-1 mapping.
 */
#define NVGR_HAL_NV_FORMAT_MAP()                            \
    MAP(HAL_PIXEL_FORMAT_RGBA_8888, NvColorFormat_A8B8G8R8) \
    MAP(HAL_PIXEL_FORMAT_RGBX_8888, NvColorFormat_X8B8G8R8) \
    MAP(HAL_PIXEL_FORMAT_RGB_888,   NvColorFormat_R8_G8_B8) \
    MAP(HAL_PIXEL_FORMAT_RGB_565,   NvColorFormat_R5G6B5)   \
    MAP(HAL_PIXEL_FORMAT_BGRA_8888, NvColorFormat_A8R8G8B8) \
    MAP(HAL_PIXEL_FORMAT_RGBA_5551, NvColorFormat_R5G5B5A1) \
    MAP(HAL_PIXEL_FORMAT_RGBA_4444, NvColorFormat_A4R4G4B4) \
    MAP(NVGR_PIXEL_FORMAT_YUV420,   NvColorFormat_Y8)       \
    MAP(NVGR_PIXEL_FORMAT_UYVY,     NvColorFormat_UYVY)

static NV_INLINE NvColorFormat
NvGrGetNvFormat (int f)
{
#define MAP(HalFormat, NvFormat) \
    case HalFormat:              \
        return NvFormat;

    switch (f)
    {
        NVGR_HAL_NV_FORMAT_MAP();

        /* TODO: [ahatala 2009-09-16] YUV formats */
        default:
            return NvColorFormat_Unspecified;
    }

#undef MAP
}

static NV_INLINE int
NvGrGetHalFormat (NvColorFormat f)
{
#define MAP(HalFormat, NvFormat) \
    case NvFormat:              \
        return HalFormat;

    switch (f)
    {
        NVGR_HAL_NV_FORMAT_MAP();

        default:
            // There's no invalid HAL format, but 0 is not attributed
            // and works fine for EGL's NATIVE_VISUAL_ID.
            return 0;
    }

#undef MAP
}

// Rounds up to the nearest multiple of align, assuming align is a power of 2.
static NV_INLINE int NvGrAlignUp(const NvU32 x, const NvU32 align) {
    NV_ASSERT(!NvGrIsNpot(align));
    return ((x)+align-1) & (~(align-1));
}

/* TODO: [ahatala 2009-09-16] tiledness?! */
static NV_INLINE void
NvGrGet3dAllocParams (NvRmSurface* s,
                      NvU32* align,
                      NvU32* size)
{
    NvU32 w = s->Width;
    NvU32 h = s->Height;
    NvBool npot = NvGrIsNpot(w) || NvGrIsNpot(h);
    NvU32 w_align = 16;
    NvU32 h_align = 16;

    NV_ASSERT(align && size);

    // height alignment
    if (npot) h = NvGrAlignUp(h, h_align);

    // width alignment
    if (npot) w_align *= 4;
    w *= ((NV_COLOR_GET_BPP(s->ColorFormat)+7) >> 3);
    w = NvGrAlignUp(w, w_align);

    *align = 1024;
    *size = NvGrAlignUp(w*h, *align);
    s->Pitch = w;
}

// For T20 and T30, the fast rotate path is hit when the width and
// height are multiples of 128 bits.  If other chips require different
// alignment, we need a way to get this info out of ddk2d.
static NV_INLINE NvU32 NvGrGetFastRotateAlign(NvColorFormat format)
{
    NvU32 bpp = NV_COLOR_GET_BPP(format);
    NvU32 align;

    switch (bpp) {
    case 8:
        align = 128 >> 3;
        break;
    case 16:
        align = 128 >> 4;
        break;
    case 32:
        align = 128 >> 5;
        break;
    default:
        NV_ASSERT(!"Unexpected size");
        align = 1;
    }

    if (NV_COLOR_GET_COLOR_SPACE(format) != NvColorSpace_LinearRGBA) {
        align *= 2;
    }

    return align;
}

static NV_INLINE void
NvGrGetNormalAllocParams (NvRmDeviceHandle rm,
                          NvRmSurface* s,
                          NvU32* align,
                          NvU32* size,
                          int usage)
{
    // Make sure we calculate pitch, align, and size with extra width/height
    // padding for display surfaces.  This is so that when we use extended
    // source rects for fast rotate blits, we are still reading memory that
    // belongs to the surface.
    const NvU32 w_orig = s->Width;
    const NvU32 h_orig = s->Height;

    if (usage & GRALLOC_USAGE_HW_FB) {
        const NvU32 fast_rotate_blit_align = NvGrGetFastRotateAlign(s->ColorFormat);
        s->Width = NvGrAlignUp(s->Width, fast_rotate_blit_align);
        s->Height = NvGrAlignUp(s->Height, fast_rotate_blit_align);
    }

    //Calculate pitch, align, and size
    NvRmSurfaceComputePitch(rm, 0, s);
    *align = NvRmSurfaceComputeAlignment(rm, s);
    *size  = NvRmSurfaceComputeSize(s);

    // Reset width and height to original values
    s->Width = w_orig;
    s->Height = h_orig;
}

static NV_INLINE void
NvGrGetAllocParams (NvRmDeviceHandle rm,
                    NvRmSurface* s,
                    NvU32* align,
                    NvU32* size,
                    int use3D,
                    int usage)
{
    if (use3D)
        NvGrGet3dAllocParams(s, align, size);
    else
        NvGrGetNormalAllocParams(rm, s, align, size, usage);
}

#endif
