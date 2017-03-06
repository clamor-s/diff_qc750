/*
 * Copyright (c) 2009-2013 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

/* TODO: [ahatala 2009-09-25] split into public/private */

#ifndef INCLUDED_NVGR_H
#define INCLUDED_NVGR_H

#include <nvrm_memmgr.h>
#include <nvrm_channel.h>
#include <nvrm_surface.h>

#include <hardware/gralloc.h>
#include <pthread.h>
#include <asm/page.h>
#include <linux/tegrafb.h>

#include "nvddk_2d_v2.h"

#define NVGR_HANDLE_MAGIC 0xDAFFCAFF
#define NVGR_BUFFER_MAGIC 0xB00BD00D
#define NVGR_WRITE_LOCK_FLAG 0x80000000
#define NVGR_SW_WRITE_LOCK_FLAG 0x40000000
#define NVGR_UNCONST(type, x) (*((type*)(&(x))))
#define NVGR_ENABLE_TRACE 0
#define NVGR_MAX_FENCES 5
#define NVGR_MAX_SURFACES 3
#define NVGR_OVERLAY_DEBUG 0
#define NVGR_DEBUG_LOCKS 0

// NVIDIA private flags
#define NVGR_USAGE_STEREO          GRALLOC_USAGE_PRIVATE_0
#define NVGR_USAGE_ONSCREEN_DISP   GRALLOC_USAGE_PRIVATE_1
#define NVGR_USAGE_CLOSED_CAP      GRALLOC_USAGE_PRIVATE_2
#define NVGR_USAGE_SECONDARY_DISP  GRALLOC_USAGE_PRIVATE_3

// NVIDIA private formats (0x100 - 0x1ff)
//
// Tokens which have two formats use the first format internally and
// the second format externally when locked for CPU access.  All
// formats currently default to YV12 external representation unless
// otherwise noted.

// Internal formats (0x100 - 0x13F)
#define NVGR_PIXEL_FORMAT_YUV420        0x100  /* planar YUV420 */
#define NVGR_PIXEL_FORMAT_YUV422        0x101  /* planar YUV422 */
#define NVGR_PIXEL_FORMAT_YUV422R       0x102  /* planar YUV422R */
#define NVGR_PIXEL_FORMAT_UYVY          0x103  /* interleaved YUV422 */
#define NVGR_PIXEL_FORMAT_YUV420_NV12   0x104  /* YUV420 / NV12 */

// Gralloc private formats (0x140 - 0x17F)
#define NVGR_PIXEL_FORMAT_YV12_EXTERNAL 0x140  /* YV12 external */
#define NVGR_PIXEL_FORMAT_NV12_EXTERNAL 0x141  /* NV12 external */

// The tiled bit modifies other formats
#define NVGR_PIXEL_FORMAT_TILED         0x080  /* tiled bit */

#define NVGR_SW_USE(u) \
    (u & (GRALLOC_USAGE_SW_READ_MASK | \
          GRALLOC_USAGE_SW_WRITE_MASK))

#define NVGR_READONLY_USE(u) \
    ((u & ~(GRALLOC_USAGE_SW_READ_MASK | \
            GRALLOC_USAGE_HW_FB | \
            GRALLOC_USAGE_HW_TEXTURE)) == 0)

#define NVGRP(x) NvOsDebugPrintf x
#if NVGR_ENABLE_TRACE
#define NVGRD(x) NVGRP(x)
#else
#define NVGRD(x)
#endif

#define ROUND_TO_PAGE(x) ((x + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1))

#define NVGR_SWAP(a, b) {                       \
    (a) ^= (b);                                 \
    (b) ^= (a);                                 \
    (a) ^= (b);                                 \
}
#define NVGR_SWAPF(a, b) {                      \
    float x = a;                                \
    a = b;                                      \
    b = x;                                      \
}

enum
{
    NvGrDevIdx_Alloc = 0,
    NvGrDevIdx_Fb0,
    NvGrDevIdx_Num,
};

#define NVGR_MAX_SCRATCH_CLIENTS 4

typedef struct NvGrScratchSetRec NvGrScratchSet;
typedef struct NvGrScratchClientRec NvGrScratchClient;
typedef struct NvNativeHandleRec NvNativeHandle;

/* Composite interface */

#define NVGR_COMPOSITE_LIST_MAX 5

typedef enum {
    NVGR_COMPOSITE_BLEND_NONE = 0,
    NVGR_COMPOSITE_BLEND_PREMULT,
    NVGR_COMPOSITE_BLEND_COVERAGE,
} NvGrCompositeBlend;

typedef struct NvGrCompositeLayerRec {
    NvNativeHandle *buffer;
    NvRect src;
    NvRect dst;
    NvGrCompositeBlend blend;
    int transform;
} NvGrCompositeLayer;

typedef struct NvGrCompositeListRec {
    int nLayers;
    NvRect clip;
    struct NvGrCompositeLayerRec layers[NVGR_COMPOSITE_LIST_MAX];
    int geometry_changed;
} NvGrCompositeList;

typedef struct NvGrModuleRec {
    gralloc_module_t    Base;

    // NV gralloc API extensions
    void (*addfence) (gralloc_module_t const*, buffer_handle_t, const NvRmFence*);
    void (*getfences) (gralloc_module_t const*, buffer_handle_t, NvRmFence*, int*);
    void (*set_stereo_info) (gralloc_module_t const*, buffer_handle_t, NvU32);
    void (*set_source_crop) (gralloc_module_t const*, buffer_handle_t, NvRect *);
#if NVGR_DEBUG_LOCKS
    void (*clear_lock_usage)(buffer_handle_t, int);
#endif

    int (*alloc_scratch)(struct NvGrModuleRec *ctx, int width, int height,
                         int format, int usage, NvRmSurfaceLayout layout,
                         NvNativeHandle **handle);
    int (*free_scratch)(struct NvGrModuleRec *ctx, NvNativeHandle *handle);
    int (*copy_buffer)(struct NvGrModuleRec *ctx, NvNativeHandle *src, NvNativeHandle *dst);
    void (*copy_edges)(struct NvGrModuleRec *ctx, NvNativeHandle *h);
    int  (*blit)(struct NvGrModuleRec *ctx, NvNativeHandle *src, NvNativeHandle *dst,
                 NvRect *srcRect, NvRect *dstRect, int transform, int edges);

    int  (*clear)(struct NvGrModuleRec *ctx, NvNativeHandle *h);
    int  (*composite)(struct NvGrModuleRec *ctx, NvNativeHandle *dst, struct NvGrCompositeListRec *list);

    NvError (*module_ref)(struct NvGrModuleRec *m);
    void (*module_unref)(struct NvGrModuleRec *m);
    void (*dump_buffer)(gralloc_module_t const*, buffer_handle_t, const char *filename);
    int (*scratch_open)(struct NvGrModuleRec *ctx, size_t count, NvGrScratchClient **scratch);
    void (*scratch_close)(struct NvGrModuleRec *ctx, NvGrScratchClient *sc);

    // Module private state
    pthread_mutex_t     Lock;
    NvS32               RefCount;
    NvRmDeviceHandle    Rm;
    NvDdk2dHandle       Ddk2d;
    NvGrScratchClient  *scratch_clients[NVGR_MAX_SCRATCH_CLIENTS];

    // Devices
    hw_device_t*        Dev[NvGrDevIdx_Num];
    int                 DevRef[NvGrDevIdx_Num];
} NvGrModule;

// nvgrmodule.c

NvError NvGrModuleRef    (NvGrModule* m);
void    NvGrModuleUnref  (NvGrModule* m);
NvBool  NvGrDevUnref     (NvGrModule* m, int idx);

// nvgralloc.c

int NvGrAllocInternal (NvGrModule *m, int width, int height, int format,
                       int usage, NvRmSurfaceLayout layout,
                       NvNativeHandle **handle);
int NvGrFreeInternal (NvGrModule *m, NvNativeHandle *handle);
int  NvGrAllocDevOpen    (NvGrModule* mod, hw_device_t** dev);

// nvgrdebug.c

// nvgrbuffer.c

typedef enum
{
    NvGrBufferFlags_Posted          = 1 << 0,
    NvGrBufferFlags_SourceCropValid = 1 << 1,
    NvGrBufferFlags_NeedsCacheMaint = 1 << 2,
} NvGrBufferFlags;

//
// The buffer data stored kernel-side. This can be accessed
// via the .BufMem member of NvNativeHandle and it is mapped
// into using processes to the address pointed to by .Buffer.
//

typedef struct
{
    NvU32               Magic;
    NvU32               SurfMemId;
    NvU32               SurfSize;
    NvU32               Flags;
    NvRmFence           Fence[NVGR_MAX_FENCES];
    pthread_mutex_t     FenceMutex;
#if NVGR_DEBUG_LOCKS
    pid_t               LockOwner[NVGR_MAX_FENCES];
    pid_t               LockCount[NVGR_MAX_FENCES];
    int                 LockUsage[NVGR_MAX_FENCES];
#endif
    NvU32               LockState;
    pthread_mutex_t     LockMutex;
    pthread_cond_t      LockExclusive;
    // Check Stereo Layout details above
    NvU32               StereoInfo;
    // WAR for bug 827707
    NvRect              SourceCrop;
    // HACK for bug 827483
    NvU32 writeCount;
} NvGrBuffer;


typedef enum NvNativeBufferTypeEnum
{
    NV_NATIVE_BUFFER_SINGLE,
    NV_NATIVE_BUFFER_STEREO,
    NV_NATIVE_BUFFER_YUV
} NvNativeBufferType;

//
// The NVIDIA native buffer type
//

#define NVGR_MAX_DDK2D_SURFACES 2

struct NvNativeHandleRec {
    native_handle_t     Base;

    // shared memory handle
    int                 MemId;

    // immutable global data
    NvU32               Magic;
    pid_t               Owner;

    // per process data
    NvNativeBufferType  Type;
    NvU32               SurfCount;
    NvRmSurface         Surf[NVGR_MAX_SURFACES];
    NvGrBuffer*         Buf;
    NvU8*               Pixels;
    NvRect              LockRect;
    int                 RefCount;
    int                 Usage;
    int                 Format;
    int                 ExtFormat;
    pthread_mutex_t     MapMutex;

    /* nvddk2d wrapper around the buffer, used for rotation blits */
    /* this is lazily allocated by hwc */
    union {
        /* XXX - union is for compatibility with nvcap until changes propagate */
        NvDdk2dSurface *ddk2dSurface;
        NvDdk2dSurface *ddk2dSurfaces[NVGR_MAX_DDK2D_SURFACES];
    };
    /* Shadow buffer, used for improving lock/unlock performance */
    /* this is lazily allocated */
    struct NvNativeHandleRec *hShadow;
    NvU32               LockedPitchInPixels;
};

int  NvGrRegisterBuffer  (gralloc_module_t const* module,
                          buffer_handle_t handle);
int  NvGrUnregisterBuffer(gralloc_module_t const* module,
                          buffer_handle_t handle);
int  NvGrLock            (gralloc_module_t const* module,
                          buffer_handle_t handle,
                          int usage, int l, int t, int w, int h,
                          void** vaddr);
int  NvGrUnlock          (gralloc_module_t const* module,
                          buffer_handle_t handle);
void NvGrAddFence        (gralloc_module_t const* module,
                          buffer_handle_t handle,
                          const NvRmFence* fence);
void NvGrGetFences       (gralloc_module_t const* module,
                          buffer_handle_t handle,
                          NvRmFence* fences,
                          int* numFences);
void NvGrSetStereoInfo   (gralloc_module_t const* module,
                          buffer_handle_t handle,
                          NvU32 stereoInfo);
void NvGrSetSourceCrop   (gralloc_module_t const* module,
                          buffer_handle_t handle,
                          NvRect *cropRect);

#if NVGR_DEBUG_LOCKS
void NvGrClearLockUsage  (buffer_handle_t handle, int usage);
#endif

void NvGrDumpBuffer(gralloc_module_t const *module, buffer_handle_t handle,
                    const char *filename);


// nvgr_2d.c
int  NvGr2dInit(NvGrModule *ctx);
void NvGr2dDeInit(NvGrModule *ctx);
int  NvGr2dBlit(NvGrModule *ctx, NvNativeHandle *src, NvNativeHandle *dst,
                NvRect *srcRect, NvRect *dstRect, int transform, int edges);
int  NvGr2dBlitExt(NvGrModule *ctx, NvNativeHandle *src, int srcIndex,
                   NvNativeHandle *dst, int dstIndex,
                   NvRect *srcRect, NvRect *dstRect, int transform,
                   NvPoint *dstOffset);
int  NvGr2DClear(NvGrModule *ctx, NvNativeHandle *h);
int  NvGr2dCopyBuffer(NvGrModule *ctx, NvNativeHandle *src, NvNativeHandle *dst);
int  NvGr2dCopyBufferLocked(NvGrModule *ctx, NvNativeHandle *src,
                            NvNativeHandle *dst,
                            const NvRmFence *srcWriteFenceIn,
                            const NvRmFence *dstReadFencesIn,
                            NvU32 numDstReadFencesIn,
                            NvRmFence *fenceOut);
void NvGr2dCopyEdges(NvGrModule *ctx, NvNativeHandle *h);
int  NvGr2dComposite(NvGrModule *ctx, NvNativeHandle *dst,
                     struct NvGrCompositeListRec *list);

// Convert the NvColorFormat to the equivalent TEGRA_FB format as used by
// gralloc (see format list in tegrafb.h). Returns -1 if the format is not
// supported by gralloc.
static inline int NvGrTranslateFmt (NvColorFormat c)
{
    switch (c) {
    case NvColorFormat_A8B8G8R8:
    case NvColorFormat_X8B8G8R8:
        return TEGRA_FB_WIN_FMT_R8G8B8A8;
    case NvColorFormat_R5G6B5:
        return TEGRA_FB_WIN_FMT_B5G6R5;
    case NvColorFormat_A8R8G8B8:
        return TEGRA_FB_WIN_FMT_B8G8R8A8;
    case NvColorFormat_R5G5B5A1:
        return TEGRA_FB_WIN_FMT_AB5G5R5;
    case NvColorFormat_A4R4G4B4:
        return TEGRA_FB_WIN_FMT_B4G4R4A4;
    case NvColorFormat_Y8: /* we expect this to be the first plane of planar YCbCr set */
        return TEGRA_FB_WIN_FMT_YCbCr420P;
    default:
        return -1;
    }
}

// Convert the HAL pixel format to the equivalent TEGRA_FB format as used by
// gralloc (see format list in tegrafb.h). Returns -1 if the format is not
// supported by gralloc.
static inline int NvGrTranslateHalFmt (int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
        return TEGRA_FB_WIN_FMT_R8G8B8A8;
    case HAL_PIXEL_FORMAT_RGB_565:
        return TEGRA_FB_WIN_FMT_B5G6R5;
    case HAL_PIXEL_FORMAT_BGRA_8888:
        return TEGRA_FB_WIN_FMT_B8G8R8A8;
    case HAL_PIXEL_FORMAT_RGBA_5551:
        return TEGRA_FB_WIN_FMT_AB5G5R5;
    case HAL_PIXEL_FORMAT_RGBA_4444:
        return TEGRA_FB_WIN_FMT_B4G4R4A4;
    case NVGR_PIXEL_FORMAT_YUV420:
        return TEGRA_FB_WIN_FMT_YCbCr420P;
    case NVGR_PIXEL_FORMAT_YUV422:
        return TEGRA_FB_WIN_FMT_YCbCr422P;
    case NVGR_PIXEL_FORMAT_YUV422R:
        return TEGRA_FB_WIN_FMT_YCbCr422R;
    case NVGR_PIXEL_FORMAT_UYVY:
        return TEGRA_FB_WIN_FMT_YCbCr422;
    default:
        return -1;
    }
}

#endif
