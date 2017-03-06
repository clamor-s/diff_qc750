/*
 * Copyright (c) 2007 - 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/* Chip specific parts of surface manager. */

#include "nvrm_surface.h"
#include "ap15rm_private.h"
#include "nvassert.h"

NvU32 NvRmSurfaceComputeSize(NvRmSurface *pSurf)
{
    // HW reads groups of 4 pixels and may read memory outside
    // surface area causing errors when reading memory mapped
    // into the GART. Adding extra row and
    // a pixel fixes GART read errors.
    int extraReadRow = (pSurf->Height > 0) ? 1 : 0;
    int extraReadPixel;

    if((pSurf->Pitch == pSurf->Width) &&
       (pSurf->Width != 0) && (pSurf->Height != 0))
    {
        extraReadPixel = 1;
    }
    else
    {
        extraReadPixel = 0;
    }

    if (pSurf->Layout == NvRmSurfaceLayout_Pitch)
    {
        return pSurf->Pitch * (pSurf->Height + extraReadRow) + extraReadPixel;
    }
    else if (pSurf->Layout == NvRmSurfaceLayout_Tiled)
    {
        // Round height up to a multiple of NVRM_SURFACE_TILE_HEIGHT lines
        NvU32 AlignedHeight =
            (pSurf->Height + extraReadRow + (NVRM_SURFACE_TILE_HEIGHT-1)) &
                            ~(NVRM_SURFACE_TILE_HEIGHT-1);

        // Pitch must be a multiple of NVRM_SURFACE_TILE_WIDTH bytes
        NV_ASSERT(!(pSurf->Pitch & (NVRM_SURFACE_TILE_WIDTH-1)));

        return pSurf->Pitch * AlignedHeight + extraReadPixel;
    }
    else
    {
        NV_ASSERT(!"Invalid Layout");
        return 0;
    }
}

// Values pulled from AP15's project.h file.  If these values change,
// we will either need to update them or create a new tiling format.
#define NV_MC_TILE_BALIGN       256

// Recommended linear surface alignment (bytes) in FDC
// (see SURFADDR in defs/ar3d.spec)
// TODO: shouldn't there be a h/w constant for this???
#define NVRM_SURFACE_PITCH_ALIGN        32

#define NVRM_SURFACE_YUV_PLANAR_ALIGN   256

NvU32 NvRmSurfaceComputeAlignment(NvRmDeviceHandle hDevice, NvRmSurface *pSurf)
{
    NvU32 Alignment = 0;

    if (pSurf->Layout == NvRmSurfaceLayout_Pitch)
    {
        if ((pSurf->ColorFormat == NvColorFormat_Y8) ||
            (pSurf->ColorFormat == NvColorFormat_U8) ||
            (pSurf->ColorFormat == NvColorFormat_V8) ||
            (pSurf->ColorFormat == NvColorFormat_U8_V8))
        {
            Alignment = NVRM_SURFACE_YUV_PLANAR_ALIGN;
        }
        else
        {
                Alignment = NVRM_SURFACE_PITCH_ALIGN;
        }
    }
    else if (pSurf->Layout == NvRmSurfaceLayout_Tiled)
    {
        /** 
         * Recommended tiled surface alignment (bytes) in MC
         * (see "Programmers Guide to Tiling in AP15")
         */
        Alignment = NV_MC_TILE_BALIGN;
    }
    return Alignment;
}

// Values pulled from AP15's project.h file.  If these values change,
// we will either need to update them or create a new tiling format.
#define NV_MC_TILE_MAXK         15
#define NV_MC_TILE_MAXN         6
#define NV_MC_TILE_PCONSTLOG2   6
#define NV_MC_TILE_SIZE         1024

// derived values
#define NVRM_SURFACE_TILE_MAX_PITCH \
            (NV_MC_TILE_MAXK << (NV_MC_TILE_PCONSTLOG2 + NV_MC_TILE_MAXN))

// Pad pitch to 32-byte boundary. T20/T30 hw's minimum requirement
// is 16-byte but 32-byte alignment is needed for performance.
#define NVRM_SURFACE_LINEAR_PITCH_ALIGNMENT 32

static NvU32 getTiledPitch(NvU32 pitch, NvU32 flags)
{
    NvU32 k = (pitch + (NVRM_SURFACE_TILE_WIDTH-1)) / NVRM_SURFACE_TILE_WIDTH;
    NvU32 n = NV_MC_TILE_PCONSTLOG2;
    NvU32 maxn = NV_MC_TILE_PCONSTLOG2 + NV_MC_TILE_MAXN; 
    NvBool k_is_valid = NV_FALSE;
    NvU32 mask;
    
    if (pitch == 0)
        return pitch;       // allow a pitch of 0 to be specified

    NV_ASSERT(pitch <= NVRM_SURFACE_TILE_MAX_PITCH);

    if (!(flags & NV_RM_SURFACE_TILE_PITCH_NON_RESTRICTIVE)) {

        while (!k_is_valid)
        {
            // k needs to be an odd number
            if (!(k & 1))
            {
                if (k > NV_MC_TILE_MAXK)
                {
                    k >>= 1;
                    n++;
                }
                else
                {
                    // remaining bits that can be shifted out before n exceeds maxn
                    mask = (1 << ((maxn + 1) - n)) - 1;
                    if (k & mask)
                    {
                        // factor out all powers of 2, since an odd k will be reached
                        // before exceeding maxn.
                        while (!(k & 1))
                        {
                            k >>= 1;
                            n++;
                        }
                    }
                    else
                    {
                        // can't factor out all powers of 2, before n exceeds maxn
                        // so just add 1 to k to make it odd (yields the smallest
                        // needed padding to get to an odd pitch).
                        k++;
                    }
                }
            }
            else if (k > NV_MC_TILE_MAXK)
            {
                k = (k + 1) >> 1;
                n++;
            }
            else
                k_is_valid = NV_TRUE;
        }

        if ((pitch > (k << n)) || !(k & 1) || (k > NV_MC_TILE_MAXK) || (n > NV_MC_TILE_PCONSTLOG2 + NV_MC_TILE_MAXN))
        {
            NV_ASSERT( !"Error: failed to find tiled pitch" );
        }
        pitch = (k << n);
    }
    else
    {
        // round up to nearest tile boundary
        pitch = (pitch + (NV_MC_TILE_SIZE-1)) & ~(NV_MC_TILE_SIZE-1);
    }

    NV_ASSERT(pitch <= NVRM_SURFACE_TILE_MAX_PITCH);
    return pitch;
}


void NvRmSurfaceComputePitch(
    NvRmDeviceHandle hDevice,
    NvU32 flags,
    NvRmSurface *pSurf)
{
    NvU32 Pitch;

    NV_ASSERT(pSurf);
    /* Calculate Pitch & align (by adding pad bytes) */
    if (pSurf->Layout == NvRmSurfaceLayout_Pitch)
    {
        Pitch = pSurf->Width * NV_COLOR_GET_BPP(pSurf->ColorFormat);
        Pitch = (Pitch + 7) >> 3;
        Pitch = (Pitch + (NVRM_SURFACE_LINEAR_PITCH_ALIGNMENT-1)) &
                        ~(NVRM_SURFACE_LINEAR_PITCH_ALIGNMENT-1);
        pSurf->Pitch = Pitch;
    }
    else if (pSurf->Layout == NvRmSurfaceLayout_Tiled)
    {
        Pitch = pSurf->Width * NV_COLOR_GET_BPP(pSurf->ColorFormat);
        Pitch = (Pitch + 7) >> 3;
        pSurf->Pitch = getTiledPitch(Pitch, flags);
    }
    return;
}

