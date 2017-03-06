/*
 * Copyright (c) 2009-2013, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvgralloc.h"
#include "nvgralloc_priv.h"
#include "nvgrsurfutil.h"
#include "nvassert.h"

#include <errno.h>
#include <unistd.h>
#include <cutils/ashmem.h>
#include <cutils/atomic.h>

#include <cutils/log.h>
#include <sys/mman.h>

// Google says we must clear the surface for security - bug 3131591
#define NVGR_CLEAR_SURFACES 1

static int NvGrAllocDevUnref (hw_device_t* dev);
static int NvGrAlloc         (alloc_device_t* dev, int w, int h, int format,
                              int usage, buffer_handle_t* handle, int* stride);
static int NvGrFree          (alloc_device_t* dev, buffer_handle_t handle);

static NvNativeHandle*
CreateNativeHandle (int MemId)
{
    NvNativeHandle* h = NvOsAlloc(sizeof(NvNativeHandle));
    if (!h) return NULL;
    NvOsMemset(h, 0, sizeof(NvNativeHandle));

    h->Base.version = sizeof(native_handle_t);
    h->Base.numFds = 1;
    h->Base.numInts = (sizeof(NvNativeHandle) -
                       sizeof(native_handle_t) -
                       h->Base.numFds*sizeof(int)) >> 2;

    h->Magic = NVGR_HANDLE_MAGIC;
    h->Owner = getpid();
    h->MemId = MemId;

    return h;
}

static NvError
SharedAlloc (NvU32 Size, int* FdOut, void** MapOut)
{
    int fd;
    void* addr;

    Size = ROUND_TO_PAGE(Size);

    fd = ashmem_create_region("nvgralloc-shared", Size);
    if (fd < 0)
    {
        return NvError_InsufficientMemory;
    }

    addr = mmap(0, Size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED)
    {
        close(fd);
        return NvError_InsufficientMemory;
    }

    *FdOut = fd;
    *MapOut = addr;

    return NvSuccess;
}

static NvError
RmAlloc (NvRmDeviceHandle Rm, NvU32 Align, NvU32 Size,
         int Usage, NvRmMemHandle* MemOut)
{
    NvError e;
    NvRmMemHandle Mem;
    NvRmHeap HeapArr[3];
    NvU32 NumHeaps = 0;
    NvOsMemAttribute Attr;
    int SwRead = Usage & GRALLOC_USAGE_SW_READ_MASK;
    int SwWrite = Usage & GRALLOC_USAGE_SW_WRITE_MASK;

    NV_ASSERT(MemOut);

    /* Disallow GART entirely for gralloc buffers as these are
     * typically shared between processes, and can defeat the
     * per-process accounting which tries to prevent over-commiting.
     */
    HeapArr[NumHeaps++] = NvRmHeap_ExternalCarveOut;
    HeapArr[NumHeaps++] = NvRmHeap_External;

    if (!(SwRead || SwWrite)) {
        Attr = NvOsMemAttribute_Uncached;
    } else if (SwRead == GRALLOC_USAGE_SW_READ_OFTEN) {
        Attr = NvOsMemAttribute_WriteBack;
    } else {
        Attr = NvOsMemAttribute_InnerWriteBack;
    }

    e = NvRmMemHandleCreate(Rm, &Mem, Size);
    if (NV_SHOW_ERROR(e))
    {
        return e;
    }

    e = NvRmMemAllocTagged(Mem, HeapArr, NumHeaps, Align, Attr,
            NVRM_MEM_TAG_GRALLOC_MISC);
    if (NV_SHOW_ERROR(e))
    {
        NvRmMemHandleFree(Mem);
        return e;
    }

    *MemOut = Mem;
    return NvSuccess;
}

static NV_INLINE void NvGrClearSurface(NvGrModule *ctx, NvNativeHandle *h)
{
#if NVGR_CLEAR_SURFACES
    NvError err = NvGr2DClear(ctx, h);

    if (NvSuccess != err) {
        NvU8* scanline = NvOsAlloc(h->Surf[0].Pitch);
        NvU32 ii;

        ALOGE("NvGr2DClear failed (%d), falling back to surface write", err);

        if (scanline) {
            // clear to black
            NvOsMemset(scanline, 0, h->Surf[0].Pitch);

            for (ii = 0; ii < h->SurfCount; ii++) {
                NvRmSurface *s = h->Surf + ii;

                // ensure the alloc is big enough
                NV_ASSERT(h->Surf[ii].Pitch <= h->Surf[0].Pitch);

                // For a YUV surface, the UV planes must be 0x7f
                if (ii == 1 && h->Type == NV_NATIVE_BUFFER_YUV) {
                    NvOsMemset(scanline, 0x7f, h->Surf[ii].Pitch);
                }

                NvRmMemWriteStrided(s->hMem, s->Offset, s->Pitch,
                                    scanline, 0, s->Pitch, s->Height);
            }

            NvOsFree(scanline);
        } else {
            // slower fallback
            for (ii = 0; ii < h->SurfCount; ii++) {
                NvRmSurface *s = h->Surf + ii;
                NvU32 size, val;

                // Clear to black.  For a YUV surface, the UV planes must be 127
                if (ii && h->Type == NV_NATIVE_BUFFER_YUV) {
                    val = 0x7f7f7f7f;
                } else {
                    val = 0;
                }

                size = s->Width * s->Height * (NV_COLOR_GET_BPP(s->ColorFormat)>>3);
                NV_ASSERT(s->Offset + size <= h->Buf->SurfSize);

                NvRmMemWriteStrided(s->hMem, s->Offset, sizeof(val),
                                    &val, 0, sizeof(val), size/sizeof(val));
            }
        }
    }

    /* HACK for bug 827483 -
     *
     * Locking the buffer for write access bumps the writeCount,
     * breaking the SurfaceView optimization.  Reset the writeCount
     * here since there have been no "real" writes yet.
     */
    h->Buf->writeCount = 0;
#endif
}

static int
GetShadowBufferStrideInPixels(NvGrModule* ctx, int external_format, int width)
{
    NvRmSurface s;
    NvColorFormat format;

    switch (external_format) {
        case NVGR_PIXEL_FORMAT_YV12_EXTERNAL:
        case NVGR_PIXEL_FORMAT_NV12_EXTERNAL:
            return NvGrAlignUp(width, 16);
        default:
            format = NvGrGetNvFormat(external_format);
            if (format == NvColorFormat_Unspecified) {
                // Locking for SW use not supported (unknown shadow format)
                return 0;
            }
            s.Layout = NvRmSurfaceLayout_Pitch;
            s.ColorFormat = format;
            s.Width = width;
            NvRmSurfaceComputePitch(ctx->Rm, 0, &s);
            return s.Pitch / (NV_COLOR_GET_BPP(format) >> 3);
    }
}

static int NvGrAllocExt(NvGrAllocDev* dev, NvGrAllocParameters *params,
                        NvNativeHandle** handle)
{
    NvGrModule* ctx = (NvGrModule*) dev->Base.common.module;
    int ret;
    NvNativeHandle *h;
    int width = params->Width;
    int height = params->Height;
    NvRmSurfaceLayout layout = params->Layout;
    int usage = params->Usage;
    int format = params->Format;
    int external_format = format;
    int internal_usage = usage;

    // Tiled buffers should not be used as textures
    if (layout == NvRmSurfaceLayout_Tiled) {
        NV_ASSERT(!(usage & GRALLOC_USAGE_HW_TEXTURE));
    }

    // Remap formats
    switch (format) {
    case HAL_PIXEL_FORMAT_YV12:
    case NVGR_PIXEL_FORMAT_YUV420:
        format = NVGR_PIXEL_FORMAT_YUV420;
        external_format = NVGR_PIXEL_FORMAT_YV12_EXTERNAL;
        break;
    case NVGR_PIXEL_FORMAT_YUV420_NV12:
        format = NVGR_PIXEL_FORMAT_YUV420;
        external_format = NVGR_PIXEL_FORMAT_NV12_EXTERNAL;
        break;
    default:
        break;
    }

    if (format != external_format || layout != NvRmSurfaceLayout_Pitch) {
        // The internal buffer will never be mapped for CPU
        internal_usage &= ~(GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK);
    }

    ret = NvGrAllocInternal(ctx, width, height, format, internal_usage, layout, &h);
    if (ret != 0) {
        return ret;
    }

    h->Usage     = usage;
    h->ExtFormat = external_format;

    if (format != external_format || layout != NvRmSurfaceLayout_Pitch) {
        // These formats will be copied to a shadow buffer on Lock, so
        // return the stride of the shadow buffer.
        h->LockedPitchInPixels =
            GetShadowBufferStrideInPixels(ctx, external_format, width);
    } else {
        h->LockedPitchInPixels =
            (h->Surf[0].Pitch / (NV_COLOR_GET_BPP(h->Surf[0].ColorFormat) >> 3));
    }

    // Clear surface
    NvGrClearSurface(ctx, h);

    *handle = h;

    NVGRD(("NvGrAlloc [0x%08x]: Allocated %dx%d format %x usage %x stride %d",
           "external_format %x layout %d",
           h->MemId, width, height, format, usage, h->LockedPitchInPixels,
           layout, external_format));

    return 0;
}

static int NvGrAlloc (alloc_device_t* dev, int width, int height, int format,
                      int usage, buffer_handle_t* handle, int* stride)
{
    int ret;
    NvNativeHandle *h;
    NvRmSurfaceLayout layout = NvRmSurfaceLayout_Pitch;
    NvGrAllocParameters params;

    // Vendor formats can be requested to be tiled
    if (0x100 <= format && format <= 0x1FF) {
        if (format & NVGR_PIXEL_FORMAT_TILED) {
            layout = NvRmSurfaceLayout_Tiled;
            format &= ~NVGR_PIXEL_FORMAT_TILED;
        }
    }

    // Tiled buffers should not be used as textures
    if (layout == NvRmSurfaceLayout_Tiled) {
        usage &= ~GRALLOC_USAGE_HW_TEXTURE;
    }

    params.Width = width;
    params.Height = height;
    params.Format = format;
    params.Usage = usage;
    params.Layout = layout;

    ret = NvGrAllocExt((NvGrAllocDev*)dev, &params, &h);
    if (ret != 0) {
        return ret;
    }

    // Note: Android expects stride in pixels, not bytes
    *stride = h->LockedPitchInPixels;
    *handle = &h->Base;
    return 0;
}

int NvGrAllocInternal (NvGrModule *Ctx, int width, int height, int format,
                       int usage, NvRmSurfaceLayout layout,
                       NvNativeHandle **handle)
{
    NvGrBuffer*         Obj;
    int                 ObjMem;
    NvRmSurface         Surf[NVGR_MAX_SURFACES];
    NvU32               SurfAlign;
    NvU32               SurfCount;
    NvNativeBufferType  BufferType;
    NvNativeHandle*     h = NULL;
    NvError             e;
    int                 i;
    pthread_mutexattr_t m_attr;
    pthread_condattr_t  c_attr;

    // Check input

    if (width <= 0 || height <= 0)
    {
        NV_SHOW_ERROR(NvError_BadParameter);
        return -EINVAL;
    }

    NvOsMemset(Surf, 0, sizeof(Surf));

    switch (format) {
    case NVGR_PIXEL_FORMAT_YUV420:
    case NVGR_PIXEL_FORMAT_YV12_EXTERNAL:
        // This format requires an even width and height
        if ((width | height) & 1)
        {
            NV_SHOW_ERROR(NvError_BadParameter);
            ALOGI("%s %d", __func__, __LINE__);
            return -EINVAL;
        }

        SurfCount  = 3;
        BufferType = NV_NATIVE_BUFFER_YUV;

        Surf[0].Width       = width;
        Surf[1].Width       = width / 2;
        Surf[2].Width       = width / 2;

        Surf[0].Height      = height;
        Surf[1].Height      = height / 2;
        Surf[2].Height      = height / 2;

        Surf[0].Layout      = layout;
        Surf[1].Layout      = layout;
        Surf[2].Layout      = layout;

        Surf[0].ColorFormat = NvColorFormat_Y8;
        Surf[1].ColorFormat = NvColorFormat_U8;
        Surf[2].ColorFormat = NvColorFormat_V8;
        break;

    case NVGR_PIXEL_FORMAT_YUV422:
        // This format requires an even width and height
        if ((width | height) & 1)
        {
            NV_SHOW_ERROR(NvError_BadParameter);
            ALOGI("%s %d", __func__, __LINE__);
            return -EINVAL;
        }

        SurfCount  = 3;
        BufferType = NV_NATIVE_BUFFER_YUV;

        Surf[0].Width       = width;
        Surf[1].Width       = width / 2;
        Surf[2].Width       = width / 2;

        Surf[0].Height      = height;
        Surf[1].Height      = height;
        Surf[2].Height      = height;

        Surf[0].Layout      = layout;
        Surf[1].Layout      = layout;
        Surf[2].Layout      = layout;

        Surf[0].ColorFormat = NvColorFormat_Y8;
        Surf[1].ColorFormat = NvColorFormat_U8;
        Surf[2].ColorFormat = NvColorFormat_V8;
        break;

    case NVGR_PIXEL_FORMAT_YUV422R:
        // This format requires an even width and height
        if ((width | height) & 1)
        {
            NV_SHOW_ERROR(NvError_BadParameter);
            ALOGI("%s %d", __func__, __LINE__);
            return -EINVAL;
        }

        SurfCount  = 3;
        BufferType = NV_NATIVE_BUFFER_YUV;

        Surf[0].Width       = width;
        Surf[1].Width       = width;
        Surf[2].Width       = width;

        Surf[0].Height      = height;
        Surf[1].Height      = height / 2;
        Surf[2].Height      = height / 2;

        Surf[0].Layout      = layout;
        Surf[1].Layout      = layout;
        Surf[2].Layout      = layout;

        Surf[0].ColorFormat = NvColorFormat_Y8;
        Surf[1].ColorFormat = NvColorFormat_U8;
        Surf[2].ColorFormat = NvColorFormat_V8;
        break;

    case NVGR_PIXEL_FORMAT_NV12_EXTERNAL:
        // This format requires an even width and height
        if ((width | height) & 1)
        {
            NV_SHOW_ERROR(NvError_BadParameter);
            ALOGI("%s %d", __func__, __LINE__);
            return -EINVAL;
        }

        SurfCount  = 2;
        BufferType = NV_NATIVE_BUFFER_YUV;

        Surf[0].Width       = width;
        Surf[1].Width       = width / 2;

        Surf[0].Height      = height;
        Surf[1].Height      = height / 2;

        Surf[0].Layout      = layout;
        Surf[1].Layout      = layout;

        Surf[0].ColorFormat = NvColorFormat_Y8;
        Surf[1].ColorFormat = NvColorFormat_U8_V8;
        break;

    default:
        SurfCount  = 1;
        BufferType = NV_NATIVE_BUFFER_SINGLE;

        Surf[0].Width       = width;
        Surf[0].Height      = height;
        Surf[0].Layout      = layout;
        Surf[0].ColorFormat = NvGrGetNvFormat(format);

        if  (usage & NVGR_USAGE_STEREO) {
            // TODO: We could check h/w capability and set
            // both frames to half width here.
            SurfCount  = 2;
            BufferType = NV_NATIVE_BUFFER_STEREO;

            Surf[1].Width       = width;
            Surf[1].Height      = height;
            Surf[1].Layout      = layout;
            Surf[1].ColorFormat = NvGrGetNvFormat(format);
        }

        if (Surf[0].ColorFormat == NvColorFormat_Unspecified)
        {
            NV_SHOW_ERROR(NvError_BadParameter);
            ALOGI("%s %d", __func__, __LINE__);
            return -EINVAL;
        }
        break;
    }

    // Get module ref

    if (NvGrModuleRef(Ctx) != NvSuccess)
        return -ENOMEM;

    // Allocate and map a kernel buffer object

    e = SharedAlloc(sizeof(NvGrBuffer), &ObjMem, (void**)&Obj);
    if (NV_SHOW_ERROR(e))
        return -ENOMEM;

    // Fill defaults

    NvOsMemset(Obj, 0, sizeof(NvGrBuffer));
    Obj->Magic = NVGR_BUFFER_MAGIC;
    Obj->Flags = 0;
    for (i = 0; i < NVGR_MAX_FENCES; i++)
        Obj->Fence[i].SyncPointID = NVRM_INVALID_SYNCPOINT_ID;
    Obj->writeCount = -1;

    // Calculate plane pitch and offsets, and total size
    switch (format) {
    case NVGR_PIXEL_FORMAT_YV12_EXTERNAL:
        SurfAlign = 16;
        Surf[0].Pitch = NvGrAlignUp(Surf[0].Width, SurfAlign);
        Surf[1].Pitch = NvGrAlignUp(Surf[0].Pitch / 2, SurfAlign);
        Surf[2].Pitch = Surf[1].Pitch;

        Surf[0].Offset = 0;
        Surf[1].Offset = Surf[0].Offset + Surf[0].Pitch * Surf[0].Height;
        Surf[2].Offset = Surf[1].Offset + Surf[1].Pitch * Surf[1].Height;
        Obj->SurfSize  = Surf[2].Offset + Surf[2].Pitch * Surf[2].Height;
        // YV12 is YVU order, swap the offsets for U and V
        NVGR_SWAP(Surf[1].Offset, Surf[2].Offset);
        break;

    case NVGR_PIXEL_FORMAT_NV12_EXTERNAL:
        SurfAlign = 16;
        Surf[0].Pitch = NvGrAlignUp(Surf[0].Width, SurfAlign);
        Surf[1].Pitch = Surf[0].Pitch;

        Surf[0].Offset = 0;
        Surf[1].Offset = Surf[0].Offset + Surf[0].Pitch * NvGrAlignUp(Surf[0].Height, SurfAlign);
        Obj->SurfSize  = Surf[1].Offset + Surf[1].Pitch * Surf[1].Height;
        break;

    default:
        // Get allocation params
        NvGrGetAllocParams(Ctx->Rm, &Surf[0], &SurfAlign, &Obj->SurfSize,
                           (usage & GRALLOC_USAGE_HW_TEXTURE), usage);

        for (i = 1; i < (int) SurfCount; i++) {
            NvU32 align, size;

            NvGrGetAllocParams(Ctx->Rm, &Surf[i], &align, &size,
                               (usage & GRALLOC_USAGE_HW_TEXTURE), usage);
            NV_ASSERT(align <= SurfAlign);

            // Align the surface offset
            Surf[i].Offset = (Obj->SurfSize + align-1) & ~(align-1);
            Obj->SurfSize = Surf[i].Offset + size;
        }
        break;
    }

    // Allocate the surface

    e = RmAlloc(Ctx->Rm, SurfAlign, Obj->SurfSize,
                usage, &Surf[0].hMem);
    if (NV_SHOW_ERROR(e))
        return -ENOMEM;

    Obj->SurfMemId = NvRmMemGetId(Surf[0].hMem);
    for (i = 1; i < (int) SurfCount; i++) {
        Surf[i].hMem = Surf[0].hMem;
    }

    // Create the handle

    h = CreateNativeHandle(ObjMem);
    if (!h)
    {
        /* TODO: [ahatala 2009-09-24] cleanup */
        NV_SHOW_ERROR(NvError_InsufficientMemory);
        return -ENOMEM;
    }

    // Fill process local info

    h->Buf       = Obj;
    h->SurfCount = SurfCount;
    h->Type      = BufferType;
    memcpy(h->Surf, Surf, SurfCount * sizeof(Surf[0]));
    h->Pixels    = NULL;
    h->hShadow   = NULL;
    h->RefCount  = 1;
    h->Usage     = usage;
    h->Format    = format;
    h->ExtFormat = format;
    pthread_mutex_init(&h->MapMutex, NULL);
    memset(h->ddk2dSurfaces, 0, sizeof(h->ddk2dSurfaces));

    // Create sync objects

    pthread_mutexattr_init(&m_attr);
    pthread_mutexattr_setpshared(&m_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&Obj->LockMutex, &m_attr);
    pthread_mutex_init(&Obj->FenceMutex, &m_attr);
    pthread_mutexattr_destroy(&m_attr);
    pthread_condattr_init(&c_attr);
    pthread_condattr_setpshared(&c_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&Obj->LockExclusive, &c_attr);
    pthread_condattr_destroy(&c_attr);

    if  (usage & NVGR_USAGE_STEREO) {
        h->Buf->StereoInfo = NV_STEREO_SEPARATELR | NV_STEREO_ENABLE_MASK;
    }

    *handle = h;

    return 0;
}

static int NvGrFree (alloc_device_t* dev, buffer_handle_t handle)
{
    return NvGrFreeInternal((NvGrModule *) dev->common.module,
                            (NvNativeHandle *) handle);
}

int NvGrFreeInternal (NvGrModule *ctx, NvNativeHandle *handle)
{
    return NvGrUnregisterBuffer((gralloc_module_t*) ctx,
                                (buffer_handle_t) handle);
}

static void NvGrDump(struct alloc_device_t* dev, char *buff, int buff_len)
{
    snprintf(buff, buff_len, "Nvidia Gralloc\n");
}

int NvGrAllocDevOpen (NvGrModule* mod, hw_device_t** out)
{
    NvGrAllocDev* dev;

    dev = NvOsAlloc(sizeof(NvGrAllocDev));
    if (!dev) return -ENOMEM;
    NvOsMemset(dev, 0, sizeof(NvGrAllocDev));

    if (NvGrModuleRef(mod) != NvSuccess)
    {
        NvOsFree(dev);
        return -EINVAL;
    }

    dev->Base.common.tag     = HARDWARE_DEVICE_TAG;
#ifdef GRALLOC_API_VERSION
    dev->Base.common.version = GRALLOC_API_VERSION;
#else
    dev->Base.common.version = 0;
#endif
    dev->Base.common.module  = &mod->Base.common;
    dev->Base.common.close   = NvGrAllocDevUnref;

    dev->Base.alloc          = NvGrAlloc;
    dev->Base.free           = NvGrFree;

#ifdef GRALLOC_API_VERSION
#if  (GRALLOC_API_VERSION >= 1)
    dev->Base.dump           = NvGrDump;
#endif
#endif

    dev->alloc_ext = NvGrAllocExt;

    *out = &dev->Base.common;
    return 0;
}

int NvGrAllocDevUnref (hw_device_t* hwdev)
{
    NvGrAllocDev* dev = (NvGrAllocDev*)hwdev;
    NvGrModule* m = (NvGrModule*)dev->Base.common.module;
    if (NvGrDevUnref(m, NvGrDevIdx_Alloc))
    {
        NvGrModuleUnref(m);
        NvOsFree(dev);
    }
    return 0;
}
