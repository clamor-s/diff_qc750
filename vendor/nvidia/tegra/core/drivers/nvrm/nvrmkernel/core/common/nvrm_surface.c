/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvrm_surface.h"
#include "nvassert.h"

// Size of a subtile in bytes
#define SUBTILE_BYTES (NVRM_SURFACE_SUB_TILE_WIDTH * NVRM_SURFACE_SUB_TILE_HEIGHT)

void NvRmSurfaceInitNonRmPitch(
    NvRmSurface *pSurf,
    NvU32 Width,
    NvU32 Height,
    NvColorFormat ColorFormat,
    NvU32 Pitch,
    void *pBase)
{
    NvOsMemset(pSurf, 0, sizeof(*pSurf));

    pSurf->Width = Width;
    pSurf->Height = Height;
    pSurf->ColorFormat = ColorFormat;
    pSurf->Layout = NvRmSurfaceLayout_Pitch;
    pSurf->Pitch = Pitch;
    pSurf->pBase = pBase;

    /* hMem and Offset do not apply for non-RM alloc'd memory */
    pSurf->hMem = NULL;

    /*
     * Surface base address must be aligned to
     * no less than the word size of ColorFormat.
     */
    NV_ASSERT( !( (NvU32)pBase % ((NV_COLOR_GET_BPP(pSurf->ColorFormat)+7) >> 3) ) );
}

void NvRmSurfaceInitRmPitch(
    NvRmSurface *pSurf,
    NvU32 Width,
    NvU32 Height,
    NvColorFormat ColorFormat,
    NvU32 Pitch,
    NvRmMemHandle hMem,
    NvU32 Offset)
{
    NvOsMemset(pSurf, 0, sizeof(*pSurf));

    pSurf->Width = Width;
    pSurf->Height = Height;
    pSurf->ColorFormat = ColorFormat;
    pSurf->Layout = NvRmSurfaceLayout_Pitch;
    pSurf->Pitch = Pitch;
    pSurf->hMem = hMem;
    pSurf->Offset = Offset;

    /* pBase does not apply for RM alloc'd memory */
    pSurf->pBase = NULL;
}

void NvRmSurfaceFree(NvRmSurface *pSurf)
{
    if (pSurf->hMem)
    {
        /* Free surface from RM alloc'd memory */
        NvRmMemHandleFree(pSurf->hMem);
        pSurf->hMem = NULL;
    }
    else
    {
        /* Free surface from non-RM alloc'd memory */
        NvOsFree(pSurf->pBase);
        pSurf->pBase = NULL;
    }
}

// Simplified version of ComputeOffset that...
// 1. Only works for tiled surfaces
// 2. Takes x in units of bytes rather than pixels
static NvU32 TiledAddr(NvRmSurface *pSurf, NvU32 x, NvU32 y)
{
    NvU32 Offset;

    // Compute offset to the pixel within the subtile
    Offset  =  x & (NVRM_SURFACE_SUB_TILE_WIDTH-1);
    Offset += (y & (NVRM_SURFACE_SUB_TILE_HEIGHT-1)) << NVRM_SURFACE_SUB_TILE_WIDTH_LOG2;

    // Throw away low bits of x and y to get location at subtile granularity
    x &= ~(NVRM_SURFACE_SUB_TILE_WIDTH-1);
    y &= ~(NVRM_SURFACE_SUB_TILE_HEIGHT-1);

    // Add horizontal offset to subtile within its row of subtiles
    Offset += x << NVRM_SURFACE_SUB_TILE_HEIGHT_LOG2;

    // Add vertical offset to row of subtiles
    Offset += y*pSurf->Pitch;

    return Offset;
}

NvU32 NvRmSurfaceComputeOffset(NvRmSurface *pSurf, NvU32 x, NvU32 y)
{
    NvU32 BitsPerPixel = NV_COLOR_GET_BPP(pSurf->ColorFormat);

    // Make sure we don't fall off the end of the surface
    NV_ASSERT(x < pSurf->Width);
    NV_ASSERT(y < pSurf->Height);

    // Convert x from units of pixels to units of bytes
    // We assert here that the pixel is aligned to a byte boundary
    x *= BitsPerPixel;
    NV_ASSERT(!(x & 7));
    x >>= 3;

    if (pSurf->Layout == NvRmSurfaceLayout_Pitch)
    {
        return y*pSurf->Pitch + x;
    }
    else if (pSurf->Layout == NvRmSurfaceLayout_Tiled)
    {
        return TiledAddr(pSurf, x, y);
    }
    else
    {
        NV_ASSERT(!"Invalid Layout");
        return 0;
    }
}

void NvRmSurfaceWrite(
    NvRmSurface *pSurf,
    NvU32 x,
    NvU32 y,
    NvU32 Width,
    NvU32 Height,
    const void *pSrcPixels)
{
    const NvU8 *pSrc = pSrcPixels;
    NvU32 BitsPerPixel = NV_COLOR_GET_BPP(pSurf->ColorFormat);
    NvU32 Offset;
    NvU8 *pDst;

    // Make sure we don't fall off the end of the surface
    NV_ASSERT(x + Width  <= pSurf->Width);
    NV_ASSERT(y + Height <= pSurf->Height);

    // Convert x and Width from units of pixels to units of bytes
    // We assert here that the rectangle is aligned to byte boundaries
    x *= BitsPerPixel;
    NV_ASSERT(!(x & 7));
    x >>= 3;

    Width *= BitsPerPixel;
    NV_ASSERT(!(Width & 7));
    Width >>= 3;

    if (pSurf->Layout == NvRmSurfaceLayout_Pitch)
    {
        if (pSurf->hMem)
        {
            // RM alloc'd memory
            Offset = pSurf->Offset + y*pSurf->Pitch + x;
            NvRmMemWriteStrided(
                pSurf->hMem, Offset, pSurf->Pitch, // destination
                pSrc, Width, // source
                Width, Height); // elementsize, count
        }
        else
        {
            // non-RM alloc'd memory
            pDst = (NvU8 *)pSurf->pBase + y*pSurf->Pitch + x;
            while (Height--)
            {
                NvOsMemcpy(pDst, pSrc, Width);
                pSrc += Width;
                pDst += pSurf->Pitch;
            }
        }
    }
    else if (pSurf->Layout == NvRmSurfaceLayout_Tiled)
    {
        NvU32 WidthStart, MiddleTiles, WidthEnd;

        // Determine width of initial chunk to get us to a subtile-aligned spot
        WidthStart = 0;
        if (x & (NVRM_SURFACE_SUB_TILE_WIDTH-1))
        {
            WidthStart = NV_MIN(Width,
                NVRM_SURFACE_SUB_TILE_WIDTH - (x & (NVRM_SURFACE_SUB_TILE_WIDTH-1)));
            Width -= WidthStart;
        }
        NV_ASSERT(WidthStart < NVRM_SURFACE_SUB_TILE_WIDTH);

        // Determine number of full middle tiles and extra pixels at the end
        MiddleTiles = Width >> NVRM_SURFACE_SUB_TILE_WIDTH_LOG2;
        WidthEnd = Width & (NVRM_SURFACE_SUB_TILE_WIDTH-1);

        if (pSurf->hMem)
        {
            // RM alloc'd memory
            while (Height--)
            {
                // Get address of first pixel in row
                Offset = TiledAddr(pSurf, x, y) + pSurf->Offset;
                y++;

                // Write fractional part of first tile (if any)
                if (WidthStart)
                {
                    NvRmMemWrite(pSurf->hMem, Offset, pSrc, WidthStart);
                    pSrc += WidthStart;
                    Offset += WidthStart - NVRM_SURFACE_SUB_TILE_WIDTH + SUBTILE_BYTES;
                }

                if (MiddleTiles)
                {
                    // Write the middle tiles
                    NvRmMemWriteStrided(pSurf->hMem,
                                        Offset,
                                        SUBTILE_BYTES, // dest stride
                                        pSrc,
                                        NVRM_SURFACE_SUB_TILE_WIDTH, // src stride
                                        NVRM_SURFACE_SUB_TILE_WIDTH, // size
                                        MiddleTiles); // count
                    Offset += MiddleTiles * SUBTILE_BYTES;
                    pSrc += MiddleTiles * NVRM_SURFACE_SUB_TILE_WIDTH;
                }

                // Write fractional part of ending tile (if any)
                if (WidthEnd)
                {
                    NvRmMemWrite(pSurf->hMem, Offset, pSrc, WidthEnd);
                    pSrc += WidthEnd;
                }
            }
        }
        else
        {
            NvU32 n;
            
            // non-RM alloc'd memory
            while (Height--)
            {
                // Get address of first pixel in row
                pDst = (NvU8 *)pSurf->pBase + TiledAddr(pSurf, x, y);
                y++;

                // Write fractional part of first tile (if any)
                if (WidthStart)
                {
                    NvOsMemcpy(pDst, pSrc, WidthStart);
                    pSrc += WidthStart;
                    pDst += WidthStart - NVRM_SURFACE_SUB_TILE_WIDTH + SUBTILE_BYTES;
                }

                // Write the middle tiles
                n = MiddleTiles;
                while (n--)
                {
                    NvOsMemcpy(pDst, pSrc, NVRM_SURFACE_SUB_TILE_WIDTH);
                    pSrc += NVRM_SURFACE_SUB_TILE_WIDTH;
                    pDst += SUBTILE_BYTES;
                }

                // Write fractional part of ending tile (if any)
                if (WidthEnd)
                {
                    NvOsMemcpy(pDst, pSrc, WidthEnd);
                    pSrc += WidthEnd;
                }
            }
        }
    }
    else
    {
        NV_ASSERT(!"Invalid Layout");
    }
}

void NvRmSurfaceRead(
    NvRmSurface *pSurf,
    NvU32 x,
    NvU32 y,
    NvU32 Width,
    NvU32 Height,
    void *pDstPixels)
{
    NvU8 *pDst = pDstPixels;
    NvU32 BitsPerPixel = NV_COLOR_GET_BPP(pSurf->ColorFormat);
    NvU32 Offset;
    const NvU8 *pSrc;

    // Make sure that the surface has data associated with it
    NV_ASSERT(pSurf->hMem || pSurf->pBase);

    // Make sure we don't fall off the end of the surface
    NV_ASSERT(x + Width  <= pSurf->Width);
    NV_ASSERT(y + Height <= pSurf->Height);

    // Convert x and Width from units of pixels to units of bytes
    // We assert here that the rectangle is aligned to byte boundaries
    x *= BitsPerPixel;
    NV_ASSERT(!(x & 7));
    x >>= 3;

    Width *= BitsPerPixel;
    NV_ASSERT(!(Width & 7));
    Width >>= 3;

    if (pSurf->Layout == NvRmSurfaceLayout_Pitch)
    {
        if (pSurf->hMem)
        {
            // RM alloc'd memory
            Offset = pSurf->Offset + y*pSurf->Pitch + x;
            NvRmMemReadStrided(
                pSurf->hMem, Offset, pSurf->Pitch, // source
                pDst, Width, // destination
                Width, Height); // elementsize, count
        }
        else
        {
            // non-RM alloc'd memory
            pSrc = (NvU8 *)pSurf->pBase + y*pSurf->Pitch + x;
            while (Height--)
            {
                NvOsMemcpy(pDst, pSrc, Width);
                pDst += Width;
                pSrc += pSurf->Pitch;
            }
        }
    }
    else if (pSurf->Layout == NvRmSurfaceLayout_Tiled)
    {
        NvU32 WidthStart, MiddleTiles, WidthEnd;
        void* UserPtr = pSurf->pBase;
        
        // Determine width of initial chunk to get us to a subtile-aligned spot
        WidthStart = 0;
        if (x & (NVRM_SURFACE_SUB_TILE_WIDTH-1))
        {
            WidthStart = NV_MIN(Width,
                NVRM_SURFACE_SUB_TILE_WIDTH - (x & (NVRM_SURFACE_SUB_TILE_WIDTH-1)));
            Width -= WidthStart;
        }
        NV_ASSERT(WidthStart < NVRM_SURFACE_SUB_TILE_WIDTH);

        // Determine number of full middle tiles and extra pixels at the end
        MiddleTiles = Width >> NVRM_SURFACE_SUB_TILE_WIDTH_LOG2;
        WidthEnd = Width & (NVRM_SURFACE_SUB_TILE_WIDTH-1);

#if NVOS_IS_LINUX
        // On Android NvRmRead calls have a considerable overhead. For large surfaces
        // repeated calls kill performance. It is faster to just do two kernel calls,
        // map and unmap, and un-tile using memcpy. If mapping fails, we fall back to
        // mem read.
        // For WinCE 6, mapping memory is much more expensive, so do it for 
        // linux only.
        if (!UserPtr)
        {
            NvError err = NvRmMemMap(pSurf->hMem, 
                                     pSurf->Offset, 
                                     NvRmSurfaceComputeSize(pSurf),
                                     NVOS_MEM_READ, 
                                     &UserPtr);
            if (err != NvSuccess)
                UserPtr = NULL;
        }
#endif // NVOS_IS_LINUX

        if (!UserPtr)
        {
            // RM alloc'd memory
            while (Height--)
            {
                // Get address of first pixel in row
                Offset = pSurf->Offset + TiledAddr(pSurf, x, y);
                y++;

                // Read fractional part of first tile (if any)
                if (WidthStart)
                {
                    NvRmMemRead(pSurf->hMem, Offset, pDst, WidthStart);
                    pDst += WidthStart;
                    Offset += WidthStart - NVRM_SURFACE_SUB_TILE_WIDTH + SUBTILE_BYTES;
                }

                // Read the middle tiles
                NvRmMemReadStrided(pSurf->hMem, 
                                   Offset,
                                   SUBTILE_BYTES, // src stride
                                   pDst,
                                   NVRM_SURFACE_SUB_TILE_WIDTH, // dest stride
                                   NVRM_SURFACE_SUB_TILE_WIDTH, // element size
                                   MiddleTiles); // count
                Offset += MiddleTiles * SUBTILE_BYTES;
                pDst += MiddleTiles * NVRM_SURFACE_SUB_TILE_WIDTH;

                // Read fractional part of ending tile (if any)
                if (WidthEnd)
                {
                    NvRmMemRead(pSurf->hMem, Offset, pDst, WidthEnd);
                    pDst += WidthEnd;
                }
            }
        }
        else
        {
            NvU32 n;

            // non-RM alloc'd memory
            while (Height--)
            {
                // Get address of first pixel in row
                pSrc = (const NvU8 *)UserPtr + TiledAddr(pSurf, x, y);
                y++;

                // Read fractional part of first tile (if any)
                if (WidthStart)
                {
                    NvOsMemcpy(pDst, pSrc, WidthStart);
                    pDst += WidthStart;
                    pSrc += WidthStart - NVRM_SURFACE_SUB_TILE_WIDTH + SUBTILE_BYTES;
                }

                // Read the middle tiles
                n = MiddleTiles;
                while (n--)
                {
                    // Compute middle tile (x,y) offset address, then read segment
                    NvOsMemcpy(pDst, pSrc, NVRM_SURFACE_SUB_TILE_WIDTH);
                    pDst += NVRM_SURFACE_SUB_TILE_WIDTH;
                    pSrc += SUBTILE_BYTES;
                }

                // Read fractional part of ending tile (if any)
                if (WidthEnd)
                {
                    NvOsMemcpy(pDst, pSrc, WidthEnd);
                    pDst += WidthEnd;
                }
            }
        }
#if NVOS_IS_LINUX
        // We mapped pointer into user process. Unmap it.
        if (UserPtr && pSurf->hMem)
            NvRmMemUnmap(pSurf->hMem, UserPtr, NvRmSurfaceComputeSize(pSurf));
#endif // NVOS_IS_LINUX
    }
    else
    {
        NV_ASSERT(!"Invalid Layout");
    }
}

NvYuvColorFormat NvRmSurfaceGetYuvColorFormat(NvRmSurface **surf, NvU32 numSurfaces)
{
    NV_ASSERT(surf[0]->ColorFormat == NvColorFormat_Y8);

    if (numSurfaces == 3)
    {
        NvU32 chromaWidth = surf[1]->Width;
        NvU32 chromaHeight = surf[1]->Height;
        NvU32 lumaWidth = surf[0]->Width;
        NvU32 lumaHeight = surf[0]->Height;

        /* YUV:420  - chroma width and height will be half the width and height of luma surface */
        if (((((lumaWidth + 0x1) & ~0x1) >> 1) == chromaWidth) && 
            ((((lumaHeight + 0x1) & ~0x1) >> 1) == chromaHeight))
        {
            return NvYuvColorFormat_YUV420;
        } else 
        /* YUV:422  - chroma width be half the width of luma surface, 
         * height being the same  */
        if (((((lumaWidth + 0x1) & ~0x1) >> 1) == chromaWidth) && 
            (lumaHeight  == chromaHeight))
        {
            return NvYuvColorFormat_YUV422;
        } else
        /* YUV:422R  - chroma height be half the height of luma surface, 
         * width being the same  */
        if (((((lumaHeight + 0x1) & ~0x1) >> 1) == chromaHeight) && 
            (lumaWidth  == chromaWidth))
        {
            return NvYuvColorFormat_YUV422R;
        } else
        /* YUV:444  - chroma height and width are same as luma width and height
         * */
        if ((lumaHeight == chromaHeight) && (lumaWidth == chromaWidth))
        {
            return NvYuvColorFormat_YUV444;
        } else
        {
            return NvYuvColorFormat_Unspecified;
        }
    }
    return NvYuvColorFormat_Unspecified;
}
