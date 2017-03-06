/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
#include "nvos.h"
#include "nvddk_dsi_block_driver_test.h"
#include "nvassert.h"
#include "nvddk_disp.h"

#define NVDDK_DISP_MAX_CONTROLLERS 2
#define FRAMEBUFFER_DOUBLE 0x1
#define FRAMEBUFFER_32BIT  0x2
#define FRAMEBUFFER_CLEAR  0x4

#define VERIFY_SURFACE(s) \
    if( !(s)->Height || !(s)->Width || !(s)->Pitch || !(s)->hMem ) \
    { \
        return NvError_BadParameter; \
    }

/**
 * This macro extracts the number of bits per pixel out of an NvColorFormat or
 * NvColorComponentPacking.
 */
#define NV_COLOR_GET_BPP(fmt) (((NvU32)(fmt)) >> 24)

static void fill_colorbar( void *data, NvColorFormat format, NvU32 width, NvU32 height )
{
    NvU32 i;
    NvU32 j;
    NvU32 wr, wg, wb;
    NvU32 color;
    NvU32 r_pos, g_pos, b_pos;
    NvU32 r_width, g_width, b_width;
    NvU32 scanlines_left;
    NvU32 bpp;

    bpp = NV_COLOR_GET_BPP(format);
    switch( format ) {
    case NvColorFormat_R5G6B5:
        r_width = 0x1f;
        b_width = 0x1f;
        g_width = 0x3f;
        r_pos = 11;
        g_pos = 5;
        b_pos = 0;
        break;
    case NvColorFormat_A8B8G8R8:
        r_width = 0xff;
        b_width = 0xff;
        g_width = 0xff;
        r_pos = 0;
        g_pos = 8;
        b_pos = 16;
        break;
    case NvColorFormat_X8R8G8B8:
    case NvColorFormat_A8R8G8B8:
        r_width = 0xff;
        b_width = 0xff;
        g_width = 0xff;
        r_pos = 16;
        g_pos = 8;
        b_pos = 0;
        break;
    case NvColorFormat_A4R4G4B4:
        r_width = 0x0f;
        b_width = 0x0f;
        g_width = 0x0f;
        r_pos = 8;
        g_pos = 4;
        b_pos = 0;
        break;
    case NvColorFormat_A1R5G5B5:
        r_width = 0x1f;
        b_width = 0x1f;
        g_width = 0x1f;
        r_pos = 10;
        g_pos = 5;
        b_pos = 0;
        break;
    case NvColorFormat_R8G8B8A8:
        r_width = 0xff;
        b_width = 0xff;
        g_width = 0xff;
        r_pos = 16;
        g_pos = 8;
        b_pos = 0;
        break;
    default:
        NV_ASSERT( !"unsupported format" );
        return;
    }

    /* use 15.16 format for color calcualtions */
    wr = (r_width << 16) / width;
    wb = (b_width << 16) / width;
    wg = (g_width << 16) / width;

    for( i = 0; i < height / 3; i++ )
    {
        for( j = 0; j < width; j++ )
        {
            color = ((((wr * j) >> 16) & r_width) << r_pos);
            if( bpp == 16 )
            {
                NvU16 *d = (NvU16 *)data;
                *d = (NvU16)color;
                data = d + 1;
            }
            else
            {
                NvU32 *d = (NvU32 *)data;
                *d = color;
                data = d + 1;
            }
        }
    }
    for( i = 0; i < height / 3; i++ )
    {
        for( j = 0; j < width; j++ )
        {
            color = ((g_width) - (((wg * j) >> 16) & g_width)) << g_pos;
            if( bpp == 16 )
            {
                NvU16 *d = (NvU16 *)data;
                *d = (NvU16)color;
                data = d + 1;
            }
            else
            {
                NvU32 *d = (NvU32 *)data;
                *d = color;
                data = d + 1;
            }
        }
    }

    scanlines_left = height - (((NvU32)(height/3)) * 3);
    for( i = 0; i < height / 3  + scanlines_left; i++ )
    {
        for( j = 0; j < width; j++ )
        {
            color = ((((wb * j) >> 16) & b_width) << b_pos);
            if( bpp == 16 )
            {
                NvU16 *d = (NvU16 *)data;
                *d = (NvU16)color;
                data = d + 1;
            }
            else
            {
                NvU32 *d = (NvU32 *)data;
                *d = color;
                data = d + 1;
            }
        }
    }
}

static NvError testdisp_fill_primarycolor_bars( NvRmDeviceHandle rm, NvRmSurface *surf )
{
    void *pixels;

    VERIFY_SURFACE( surf );

    pixels = NvOsAlloc( NvRmSurfaceComputeSize( surf ) );
    if( !pixels )
    {
        return NvError_InsufficientMemory;
    }

    fill_colorbar( pixels, surf->ColorFormat, surf->Width, surf->Height );

    NvRmSurfaceWrite( surf, 0, 0, surf->Width, surf->Height, pixels );

    NvOsFree( pixels );
    return NvSuccess;
}

NvError DsiBasicTest(void)
{
    NvDdkDispControllerHandle hControllers[NVDDK_DISP_MAX_CONTROLLERS];
    NvDdkDispControllerHandle hController;
    NvDdkDispControllerHandle hController_temp;
    NvDdkDispWindowHandle     hWindow;
    NvDdkDispMode             Mode;
    NvRmSurface               Surf;
    void                      *ptr;
    NvDdkDispWindowAttribute  Attrs[] =
    {
        NvDdkDispWindowAttribute_DestRect_Left,
        NvDdkDispWindowAttribute_DestRect_Top,
        NvDdkDispWindowAttribute_DestRect_Right,
        NvDdkDispWindowAttribute_DestRect_Bottom,
        NvDdkDispWindowAttribute_SourceRect_Left,
        NvDdkDispWindowAttribute_SourceRect_Top,
        NvDdkDispWindowAttribute_SourceRect_Right,
        NvDdkDispWindowAttribute_SourceRect_Bottom,
    };
    NvU32                     Vals[NV_ARRAY_SIZE(Attrs)];
    NvU32                     Size;
    NvU32                     Align;
    NvU32                     n;
    NvU32                     nControllers;
    NvError                   e = NvSuccess;
    NvU32                     cntCont;
    NvU32                     type;
    static NvRmDeviceHandle       s_hRm      = NULL;
    static NvDdkDispHandle        s_hDdkDisp = NULL;
    static NvDdkDispDisplayHandle s_hDisplay = NULL;
    static NvRmSurface            s_Frontbuffer;

    NvU32  Flags = FRAMEBUFFER_DOUBLE | FRAMEBUFFER_CLEAR | FRAMEBUFFER_32BIT;

    if (s_hDisplay)
        return NV_TRUE;

    NV_CHECK_ERROR_CLEANUP(NvRmOpenNew(&s_hRm));
    NV_CHECK_ERROR_CLEANUP(NvDdkDispOpen(s_hRm, &s_hDdkDisp, 0));
    nControllers = 0;
    NV_CHECK_ERROR_CLEANUP(
        NvDdkDispListControllers(s_hDdkDisp, &nControllers, 0)
    );
    NV_CHECK_ERROR_CLEANUP(
        NvDdkDispListControllers(s_hDdkDisp, &nControllers, hControllers)
    );

    for (cntCont=0; cntCont<nControllers; cntCont++)
    {
        hController_temp = hControllers[cntCont];
        NV_CHECK_ERROR_CLEANUP(
            NvDdkDispGetDisplayByUsage(hController_temp,
                NvDdkDispDisplayUsage_Primary, &s_hDisplay)
        );
        NV_CHECK_ERROR_CLEANUP(
            NvDdkDispGetDisplayAttribute(s_hDisplay,
                NvDdkDispDisplayAttribute_Type, &type)
        );
    }
    hController = hControllers[0];
    NV_CHECK_ERROR_CLEANUP(
           NvDdkDispGetDisplayByUsage(hController,
               NvDdkDispDisplayUsage_Primary, &s_hDisplay)
    );

    n = 1;
    NV_CHECK_ERROR_CLEANUP(NvDdkDispListWindows(hController, &n, &hWindow));
    //Display already attatched
    NV_CHECK_ERROR_CLEANUP(
        NvDdkDispGetBestMode(hController, &Mode, 0)
    );
    NV_CHECK_ERROR_CLEANUP(NvDdkDispSetMode(hController, &Mode, 0));
    Vals[0] = Vals[1] = Vals[4] = Vals[5] = 0;
    Vals[2] = Vals[6] = Mode.width;
    Vals[3] = Vals[7] = Mode.height;
    NV_CHECK_ERROR_CLEANUP(
        NvDdkDispSetWindowAttributes(hWindow, Attrs, Vals,
            NV_ARRAY_SIZE(Attrs), 0)
    );

    NvOsMemset(&Surf, 0, sizeof(Surf));
    Surf.Width = Mode.width;
    Surf.Height = Mode.height;
    if (Flags & FRAMEBUFFER_DOUBLE)
        Surf.Height*=2;
    if (Flags & FRAMEBUFFER_32BIT)
        Surf.ColorFormat = NvColorFormat_R8G8B8A8;
    else
        Surf.ColorFormat = NvColorFormat_R5G6B5;
    Surf.Layout = NvRmSurfaceLayout_Pitch;

    //  dev/fb expects that the framebuffer surface is 4K-aligned, so
    //  allocate with the maximum of the RM's computed alignment and 4K
    NvRmSurfaceComputePitch(s_hRm, 0, &Surf);
    Size = NvRmSurfaceComputeSize(&Surf);
    Align = NvRmSurfaceComputeAlignment(s_hRm, &Surf);
    Align = NV_MAX(4096, Align);

    NV_CHECK_ERROR_CLEANUP(NvRmMemHandleCreate(s_hRm, &Surf.hMem, Size));
    NV_CHECK_ERROR_CLEANUP(
        NvRmMemAlloc(Surf.hMem, NULL, 0, Align, NvOsMemAttribute_Uncached)
    );

    NV_CHECK_ERROR_CLEANUP(NvRmMemMap(Surf.hMem, 0, Size, 0, &ptr));

    if (Flags & FRAMEBUFFER_CLEAR)
    {
        testdisp_fill_primarycolor_bars( s_hRm, &Surf );
    }

    s_Frontbuffer = Surf;
    if (Flags & FRAMEBUFFER_DOUBLE)
        s_Frontbuffer.Height /= 2;

    NV_CHECK_ERROR_CLEANUP(
        NvDdkDispSetWindowSurface(hWindow, &s_Frontbuffer, 1, 0)
    );
fail:
    return e;
}
