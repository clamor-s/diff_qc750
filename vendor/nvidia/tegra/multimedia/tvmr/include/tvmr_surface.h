/*
 * Copyright 2010 by NVIDIA Corporation.  All rights reserved.  All
 * information contained herein is proprietary and confidential to NVIDIA
 * Corporation.  Any use, reproduction, or disclosure without the written
 * permission of NVIDIA Corporation is prohibited.
 */

#ifndef _TVMR_SURFACE_H
#define _TVMR_SURFACE_H

#include "nvcommon.h"

typedef enum {
  TVMRSurfaceType_YV12,
  TVMRSurfaceType_NV12,
  TVMRSurfaceType_R8G8B8A8,
  TVMRSurfaceType_YV16x2,
  TVMRSurfaceType_YV16,
  TVMRSurfaceType_YV24,
  TVMRSurfaceType_YV16_Transposed, /* chroma subsampled vertically */
  TVMRSurfaceType_B5G6R5
} TVMRSurfaceType;

typedef struct {
   NvU32 pitch;
   void * mapping;
   void * priv;
} TVMRSurface;

typedef struct {
   TVMRSurfaceType type;
   NvU32 width;
   NvU32 height;
   TVMRSurface * surface;
} TVMROutputSurface;

typedef struct {
   TVMRSurfaceType type;
   NvU32 width;
   NvU32 height;
   TVMRSurface * surfaces[6];
} TVMRVideoSurface;

typedef void* TVMRFence;

#endif /* _TVMR_SURFACE_H */
