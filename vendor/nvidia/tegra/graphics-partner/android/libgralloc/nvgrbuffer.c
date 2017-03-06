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
#include <nvassert.h>
#include <errno.h>
#include <unistd.h>
#include <cutils/ashmem.h>
#include <cutils/atomic.h>
#include <cutils/log.h>
#include <sys/mman.h>

#if NVGR_DEBUG_LOCKS

#define USE_PID 1
#if USE_PID
#define gettid getpid
#endif

static void SaveLockInfo(NvNativeHandle *h, int usage)
{
    pid_t tid = gettid();
    int ii;

    // Check if we're already holding the lock
    for (ii = 0; ii < NVGR_MAX_FENCES; ii++) {
        if (h->Buf->LockOwner[ii] == tid) {
            NV_ASSERT(NVGR_READONLY_USE(usage));
            h->Buf->LockUsage[ii] |= usage;
            NV_ASSERT(h->Buf->LockCount[ii] > 0);
            h->Buf->LockCount[ii]++;
            return;
        }
    }

    for (ii = 0; ii < NVGR_MAX_FENCES; ii++) {
        NV_ASSERT(h->Buf->LockOwner[ii] != tid);
        if (h->Buf->LockOwner[ii] == 0) {
            h->Buf->LockOwner[ii] = tid;
            h->Buf->LockUsage[ii] = usage;
            NV_ASSERT(h->Buf->LockCount[ii] == 0);
            h->Buf->LockCount[ii]++;
            break;
        }
    }

    NV_ASSERT(ii < NVGR_MAX_FENCES);
}
static void DeleteLockInfo(NvNativeHandle *h)
{
    pid_t tid = gettid();
    int ii;

    for (ii = 0; ii < NVGR_MAX_FENCES; ii++) {
        if (h->Buf->LockOwner[ii] == tid) {
            NV_ASSERT(h->Buf->LockCount[ii] > 0);
            h->Buf->LockCount[ii]--;
            if (h->Buf->LockCount[ii] == 0) {
                h->Buf->LockOwner[ii] = 0;
                h->Buf->LockUsage[ii] = 0;
            }
            break;
        }
    }

    NV_ASSERT(ii < NVGR_MAX_FENCES);
}
static void DumpLockInfo(NvNativeHandle *h)
{
    int ii, count, len;
    char buf[256], *ptr;

    len = sizeof(buf);
    ptr = buf;

    count = snprintf(ptr, len, "Buffer %x locks:", h->MemId);
    ptr += count;
    len -= count;
    for (ii = 0; ii < NVGR_MAX_FENCES; ii++) {
        if (h->Buf->LockOwner[ii]) {
            count = snprintf(ptr, len, " (tid = %d (x%d), usage = 0x%x)",
                             h->Buf->LockOwner[ii],
                             h->Buf->LockCount[ii],
                             h->Buf->LockUsage[ii]);
            ptr += count;
            len -= count;
        }
    }

    ALOGD("%s\n", buf);
}
void
NvGrClearLockUsage(buffer_handle_t handle, int usage)
{
    NvNativeHandle *h = (NvNativeHandle*)handle;
    pid_t tid = gettid();
    int ii;
    for (ii = 0; ii < NVGR_MAX_FENCES; ii++) {
        if (h->Buf->LockOwner[ii] == tid) {
            NV_ASSERT(h->Buf->LockCount[ii] > 0);
            h->Buf->LockUsage[ii] &= ~usage;
        }
    }
}
#endif

static void
GetBufferLock (NvNativeHandle* h, int usage)
{
    int need_exclusive = !NVGR_READONLY_USE(usage);

    pthread_mutex_lock(&h->Buf->LockMutex);

    while ((h->Buf->LockState & NVGR_WRITE_LOCK_FLAG) ||
           (need_exclusive && h->Buf->LockState))
    {
        struct timespec timeout = {2, 0};

        if (pthread_cond_timedwait_relative_np(&h->Buf->LockExclusive,
                &h->Buf->LockMutex, &timeout) == ETIMEDOUT) {
            ALOGE("GetBufferLock timed out for thread %d buffer 0x%x usage 0x%x LockState %d\n",
                 gettid(), h->MemId, usage, h->Buf->LockState);
#if NVGR_DEBUG_LOCKS
            DumpLockInfo(h);
#endif
        }
    }

#if NVGR_DEBUG_LOCKS
    SaveLockInfo(h, usage);
#endif

    if (need_exclusive)
    {
        h->Buf->LockState |= NVGR_WRITE_LOCK_FLAG;
        if (NVGR_SW_USE(usage))
            h->Buf->LockState |= NVGR_SW_WRITE_LOCK_FLAG;
    }
    else
    {
        h->Buf->LockState++;
        pthread_mutex_unlock(&h->Buf->LockMutex);
    }
}

static void
ReleaseBufferLock (NvNativeHandle* h)
{
#if NVGR_DEBUG_LOCKS
    DeleteLockInfo(h);
#endif

    if (h->Buf->LockState & NVGR_WRITE_LOCK_FLAG)
    {
        // exclusive lock held
        h->Buf->LockState = 0;
        pthread_cond_broadcast(&h->Buf->LockExclusive);
        pthread_mutex_unlock(&h->Buf->LockMutex);
    }
    else
    {
        pthread_mutex_lock(&h->Buf->LockMutex);
        h->Buf->LockState--;
        if (!h->Buf->LockState)
            pthread_cond_signal(&h->Buf->LockExclusive);
        pthread_mutex_unlock(&h->Buf->LockMutex);
    }
}

int NvGrRegisterBuffer (gralloc_module_t const* module,
                        buffer_handle_t handle)

{
    NvGrModule*         m = (NvGrModule*)module;
    NvNativeHandle*     h = (NvNativeHandle*)handle;
    NvError             e = NvSuccess;

    NV_ASSERT(h->Magic == NVGR_HANDLE_MAGIC);

    // If buffer is owned by me, just inc refcount

    if (h->Owner == getpid())
    {
        android_atomic_inc(&h->RefCount);
        return 0;
    }

    NVGRD(("NvGrRegisterBuffer [0x%08x]: Attaching", h->MemId));

    // Clear the owner.  It doesn't matter to this process anymore,
    // but if passed back to the original owner via binder, it will
    // cause an unintended early return above.  Google bug 7114990.
    h->Owner = 0;

    // Get a reference on the module

    if (NvGrModuleRef(m) != NvSuccess)
        return -ENOMEM;

    // Start with pixels not mapped

    h->Pixels = NULL;
    h->hShadow = NULL;
    h->RefCount = 1;
    h->ddk2dSurface = NULL;
    pthread_mutex_init(&h->MapMutex, NULL);

    // Map buffer memory

    h->Buf = mmap(0, ROUND_TO_PAGE(sizeof(NvGrBuffer)),
                  PROT_READ|PROT_WRITE, MAP_SHARED, h->MemId, 0);
    if (h->Buf == MAP_FAILED)
        return -ENOMEM;

    // Get a surface memory handle for this process

    e = NvRmMemHandleFromId(h->Buf->SurfMemId, &h->Surf[0].hMem);
    if (NV_SHOW_ERROR(e))
        return -ENOMEM;

    // Clear any surface pointers which came from the originating
    // process.
    memset(h->ddk2dSurfaces, 0, sizeof(h->ddk2dSurfaces));

    return 0;
}

int NvGrUnregisterBuffer (gralloc_module_t const* module,
                          buffer_handle_t handle)
{
    NvNativeHandle* h = (NvNativeHandle*)handle;
    NvGrModule*     m = (NvGrModule*)module;
    int i;

    NV_ASSERT(h->Magic == NVGR_HANDLE_MAGIC);
    NV_ASSERT(h->Buf->Magic == NVGR_BUFFER_MAGIC);

    NVGRD(("NvGrUnregisterBuffer [0x%08x]: Refcount %d", h->MemId, h->RefCount));

    if (android_atomic_dec(&h->RefCount) > 1)
        return 0;

    // Cpu sync before deallocating
    for (i = 0; i < NVGR_MAX_FENCES; i++)
    {
        NvRmFence* f = &h->Buf->Fence[i];
        if (f->SyncPointID != NVRM_INVALID_SYNCPOINT_ID)
            NvRmFenceWait(m->Rm, f, NV_WAIT_INFINITE);
    }

    if (h->hShadow) {
        NvNativeHandle *hShadow = h->hShadow;

        if (hShadow->Pixels) {
            NvRmMemUnmap(hShadow->Surf[0].hMem, hShadow->Pixels,
                         hShadow->Buf->SurfSize);
        }

        NvRmMemHandleFree(hShadow->Surf[0].hMem);

        for (i = 0; i < NVGR_MAX_DDK2D_SURFACES; i++) {
            if (hShadow->ddk2dSurfaces[i] != NULL) {
                NvDdk2dSurfaceDestroy(hShadow->ddk2dSurfaces[i]);
            }
        }

        pthread_mutex_destroy(&hShadow->MapMutex);
        munmap(hShadow->Buf, ROUND_TO_PAGE(sizeof(NvGrBuffer)));

        // Shadow buffers are not shared between processes
        NV_ASSERT(hShadow->Owner == getpid());
        close(hShadow->MemId);
        NvOsFree(hShadow);
    }

    if (h->Pixels)
        NvRmMemUnmap(h->Surf[0].hMem, h->Pixels, h->Buf->SurfSize);

    NvRmMemHandleFree(h->Surf[0].hMem);

    for (i = 0; i < NVGR_MAX_DDK2D_SURFACES; i++) {
        if (h->ddk2dSurfaces[i] != NULL) {
            NvDdk2dSurfaceDestroy(h->ddk2dSurfaces[i]);
        }
    }

    pthread_mutex_destroy(&h->MapMutex);
    if (h->Owner == getpid()) {
#if NVGR_DEBUG_LOCKS
        if (h->Buf->LockState) {
            // This is not an error.  Typically it will occur if a
            // client dies while holding a gralloc lock.
            // SurfaceFlinger will clean up when it detects the client
            // is gone.
            ALOGD("Attempting to delete a locked buffer!\n");
            DumpLockInfo(h);
        }
#endif
        // Bug 782321
        // Don't delete these mutexes because a client may still
        // reference this buffer and will hang if the mutex is
        // de-inited.  Since pthread mutexes on android use
        // pre-allocated memory, there is no leak here.
        //
        // pthread_mutex_destroy(&h->Buf->FenceMutex);
        // pthread_mutex_destroy(&h->Buf->LockMutex);
        // pthread_cond_destroy(&h->Buf->LockExclusive);
    }

    munmap(h->Buf, ROUND_TO_PAGE(sizeof(NvGrBuffer)));

    if (h->Owner == getpid()) {
        close(h->MemId);
        NvOsFree(h);
    }

    NvGrModuleUnref(m);
    return 0;
}

static inline NvBool
IsSubRect(NvNativeHandle *h, int l, int t, int width, int height)
{
    return l || t ||
        width < (int) h->Surf[0].Width ||
        height < (int) h->Surf[0].Height;
}

static inline NvBool UseShadow(NvNativeHandle *h)
{
    return h->Format != h->ExtFormat ||
        h->Surf[0].Layout != NvRmSurfaceLayout_Pitch;
}

int NvGrLock (gralloc_module_t const* module,
              buffer_handle_t handle,
              int usage, int l, int t, int width, int height,
              void** vaddr)
{
    NvGrModule*     m = (NvGrModule*)module;
    NvNativeHandle* h = (NvNativeHandle*)handle;

    NV_ASSERT(h->Magic == NVGR_HANDLE_MAGIC);
    NV_ASSERT(h->Buf->Magic == NVGR_BUFFER_MAGIC);
    NV_ASSERT(m->RefCount);

    NVGRD(("NvGrLock [0x%08x]: rect (%d,%d,%d,%d) usage %x",
           h->MemId, l, t, width, height, usage));

    if (NVGR_SW_USE(usage) &&
        (usage & (GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE)))
    {
        NV_ASSERT(!"Both SW and HW access requested, can't deal with this");
        return -EINVAL;
    }

    // Get a lock based on usage. If this is a write lock, we'll have exclusive
    // access to both shared and process local state from here on.

    GetBufferLock(h, usage);

    // Deal with SW access

    if (NVGR_SW_USE(usage))
    {
        NvRmFence *fences, safef;
        NvU32 numFences;

        if (NVGR_READONLY_USE(usage)) {
            pthread_mutex_lock(&h->Buf->FenceMutex);
            // Copy the fence so it can be used outside the mutex
            safef = h->Buf->Fence[0];
            fences = &safef;
            numFences = 1;
            pthread_mutex_unlock(&h->Buf->FenceMutex);
        } else {
            // exclusive buffer lock held, no need for fencemutex
            fences = h->Buf->Fence;
            numFences = NVGR_MAX_FENCES;
        }

        pthread_mutex_lock(&h->MapMutex);

        if (UseShadow(h)) {
            int ret;

            if (!h->hShadow) {
                NvError e;
                // Alloc Shadow Buffer
                ret = NvGrAllocInternal(m,
                                        h->Surf[0].Width,
                                        h->Surf[0].Height,
                                        h->ExtFormat,
                                        h->Usage,
                                        NvRmSurfaceLayout_Pitch,
                                        &h->hShadow);
                if (ret != 0) {
                    pthread_mutex_unlock(&h->MapMutex);
                    ReleaseBufferLock(h);
                    return -ENOMEM;
                }

                // Assert that the shadow buffer stride matches the one that
                // we returned on alloc()
                NV_ASSERT(h->LockedPitchInPixels == 0 ||
                    h->hShadow->Surf[0].Pitch == h->LockedPitchInPixels *
                    (NV_COLOR_GET_BPP(h->hShadow->Surf[0].ColorFormat) >> 3));

                // MemMap the shadow for CPU access
                e = NvRmMemMap(h->hShadow->Surf[0].hMem, 0,
                               h->hShadow->Buf->SurfSize,
                               NVOS_MEM_READ_WRITE, (void**)&h->hShadow->Pixels);
                if (NV_SHOW_ERROR(e)) {
                    pthread_mutex_unlock(&h->MapMutex);
                    ReleaseBufferLock(h);
                    return -ENOMEM;
                }

                // Invalidate the cache to clear pending writebacks.
                NvRmMemCacheMaint(h->hShadow->Surf[0].hMem, h->hShadow->Pixels,
                                  h->hShadow->Buf->SurfSize, NV_TRUE, NV_TRUE);
            } else if (!h->hShadow->Pixels)  {
                pthread_mutex_unlock(&h->MapMutex);
                ReleaseBufferLock(h);
                return -ENOMEM;
            }

            // Update the shadow copy if read access is requested or
            // locking a subrect, in which case the region outside the
            // rect is preserved.
            if (h->Buf->writeCount != h->hShadow->Buf->writeCount &&
                ((usage & GRALLOC_USAGE_SW_READ_MASK) ||
                 IsSubRect(h, l, t, width, height))) {
                // Src write fences: Wait for any writes on the
                // primary buffer.  Dst read fences: Read fences
                // on the shadow buffer can only come from the
                // unlock operation. All such fences have been
                // added as write fences to the primary buffer, so
                // waiting on fences[0] is sufficient.
                ret = NvGr2dCopyBufferLocked(m, h, h->hShadow,
                                             &fences[0], // Src write
                                             NULL, 0, // Dst reads
                                             NULL); // CPU wait

                if (ret != 0) {
                    pthread_mutex_unlock(&h->MapMutex);
                    ReleaseBufferLock(h);
                    return -ENOMEM;
                }

                // Avoid duplicate blits
                h->hShadow->Buf->writeCount = h->Buf->writeCount;

                // Since we wrote to the buffer with hardware above,
                // always invalidate the cache here.  Note that
                // writeback+invalidate is faster than invalidate
                // alone, and writeback is a nop because the cache
                // entries should be clean.
                NvRmMemCacheMaint(h->hShadow->Surf[0].hMem,
                                  h->hShadow->Pixels,
                                  h->hShadow->Buf->SurfSize, NV_TRUE,
                                  NV_TRUE);
            }
        }
        else
        {
            // CPU wait for fences
            while (numFences--) {
                NvRmFence* f = &fences[numFences];
                if (f->SyncPointID != NVRM_INVALID_SYNCPOINT_ID) {
                    NVGRD(("NvGrLock [0x%08x]: CPU waiting for ID %d value %d",
                           h->MemId, f->SyncPointID, f->Value));
                    NvRmFenceWait(m->Rm, f, NV_WAIT_INFINITE);
                }
            }

            NV_ASSERT(h->Surf[0].Layout == NvRmSurfaceLayout_Pitch);

            /* Map if necessary. We keep the mapping open until the handle
             * is closed. */

            if (!h->Pixels)
            {
                NvError e;
                e = NvRmMemMap(h->Surf[0].hMem, 0, h->Buf->SurfSize,
                               NVOS_MEM_READ_WRITE, (void**)&h->Pixels);

                if (NV_SHOW_ERROR(e))
                {
                    pthread_mutex_unlock(&h->MapMutex);
                    ReleaseBufferLock(h);
                    return -ENOMEM;
                }
            }

            if (h->Buf->Flags & NvGrBufferFlags_NeedsCacheMaint) {
                if ((usage & GRALLOC_USAGE_SW_READ_MASK) ||
                    IsSubRect(h, l, t, width, height)) {
                    NvRmMemCacheMaint(h->Surf[0].hMem,
                                      h->Pixels,
                                      h->Buf->SurfSize, NV_TRUE,
                                      NV_TRUE);
                }
                // We only skip cache maintenance on a full buffer
                // write lock which always flushes and invalidates on
                // unlock, so this flag is no longer required even if
                // we skipped maintenance above.
                h->Buf->Flags &= ~NvGrBufferFlags_NeedsCacheMaint;
            }
        }

        pthread_mutex_unlock(&h->MapMutex);
    } else if (!UseShadow(h) && !NVGR_READONLY_USE(usage)) {
        // Cache entries may be stale following a hardware write.
        // Flag the buffer for cache maintenance on the next CPU read
        // or preserve lock.
        h->Buf->Flags |= NvGrBufferFlags_NeedsCacheMaint;
    }

    // Save LockRect
    if (!NVGR_READONLY_USE(usage)) {
        h->LockRect.left = l;
        h->LockRect.top = t;
        h->LockRect.right = l + width;
        h->LockRect.bottom = t + height;
    }

    if (vaddr)
        *vaddr = h->hShadow ? h->hShadow->Pixels : h->Pixels;

    if (!NVGR_READONLY_USE(usage)) {
        /* HACK for bug 827483 -
         * If the counter is about to wrap, jump to 2 to preserve the
         * fact the buffer has been written more than once.
         */
        if (h->Buf->writeCount == (NvU32)~0) {
            h->Buf->writeCount = 2;
        } else {
            h->Buf->writeCount++;
        }
    }

    android_atomic_inc(&h->RefCount);

    return 0;
}

int NvGrUnlock (gralloc_module_t const* module,
                buffer_handle_t handle)
{
    NvGrModule*     m = (NvGrModule*)module;
    NvNativeHandle* h = (NvNativeHandle*)handle;

    NV_ASSERT(h->Magic == NVGR_HANDLE_MAGIC);
    NV_ASSERT(h->Buf->Magic == NVGR_BUFFER_MAGIC);

    NVGRD(("NvGrUnlock [0x%08x]", h->MemId));

    if (h->Buf->LockState & NVGR_SW_WRITE_LOCK_FLAG)
    {
        // Exclusive lock held
        if (h->hShadow && h->hShadow->Pixels)
        {
            NvRmFence fence;
            int ret;

            NV_ASSERT(h->Format != h->ExtFormat ||
                      h->Surf[0].Layout != NvRmSurfaceLayout_Pitch);

            // Writeback the locked region and invalidate the cache
            NvRmMemCacheMaint(h->hShadow->Surf[0].hMem, h->hShadow->Pixels,
                              h->hShadow->Buf->SurfSize, NV_TRUE, NV_TRUE);

            // 2D copy
            // Src write fences: We did a CPU wait when locking, so we already
            // waited on all writes to the shadow buffer.
            // Dst read fences: Wait for any reads on the primary buffer.
            ret = NvGr2dCopyBufferLocked(m, h->hShadow, h,
                                         NULL,
                                         h->Buf->Fence + 1, NVGR_MAX_FENCES - 1,
                                         &fence);
            if (ret != 0) {
                ReleaseBufferLock(h);
                NvGrUnregisterBuffer(module, handle);
                return ret;
            }

            NvGrAddFence(module, handle, &fence);
        }
        else if (h->Pixels)
        {
            /* Writeback the locked region and invalidate the cache */

            NvU32 i;
            NvU8 *start;
            NvU32 size;
            NvU32 y = h->LockRect.top;
            NvU32 height = h->LockRect.bottom - h->LockRect.top;

            #define CACHE_MAINT(surface, y, height)                             \
                start = h->Pixels + (surface)->Offset + (y * (surface)->Pitch); \
                size = height * (surface)->Pitch;                               \
                NvRmMemCacheMaint((surface)->hMem, start, size, NV_TRUE, NV_TRUE);

            CACHE_MAINT(&h->Surf[0], y, height);

            for (i=1; i<h->SurfCount; ++i)
            {
                NvRmSurface *surface = &h->Surf[i];

                int py = (y * surface->Height) / h->Surf[0].Height;
                int pheight = (height * surface->Height) / h->Surf[0].Height;

                CACHE_MAINT(surface, py, pheight);
            }

            #undef CACHE_MAINT
        }
    }

    ReleaseBufferLock(h);

    return NvGrUnregisterBuffer(module, handle);
}

void NvGrAddFence (gralloc_module_t const* module,
                   buffer_handle_t handle,
                   const NvRmFence* fencein)
{
    NvNativeHandle* h = (NvNativeHandle*)handle;
    NvRmFence safef;

    if (fencein)
        safef = *fencein;
    else {
        safef.SyncPointID = NVRM_INVALID_SYNCPOINT_ID;
        safef.Value = 0;
    }

    NV_ASSERT(h->Magic == NVGR_HANDLE_MAGIC);
    NV_ASSERT(h->Buf->Magic == NVGR_BUFFER_MAGIC);

    NV_ASSERT(h->Buf->LockState != 0);

    if (h->Buf->LockState & NVGR_WRITE_LOCK_FLAG)
    {
        // Exclusive lock held, can retire read fences
        int i;
        for (i = 1; i < NVGR_MAX_FENCES; i++)
        {
            NvRmFence* f = &h->Buf->Fence[i];
            f->SyncPointID = NVRM_INVALID_SYNCPOINT_ID;
        }
        // The write fence always goes in index 0
        h->Buf->Fence[0] = safef;
    }
    else if (safef.SyncPointID != NVRM_INVALID_SYNCPOINT_ID)
    {
        // Read fences start in index 1
        int i = 1;
        pthread_mutex_lock(&h->Buf->FenceMutex);
        for (; i < NVGR_MAX_FENCES; i++)
        {
            NvRmFence* f = &h->Buf->Fence[i];

            if ((f->SyncPointID == safef.SyncPointID) ||
                (f->SyncPointID == NVRM_INVALID_SYNCPOINT_ID))
            {
                *f = safef;
                break;
            }
        }
        pthread_mutex_unlock(&h->Buf->FenceMutex);
        if (i == NVGR_MAX_FENCES) {
            ALOGE("NvGrAddFence: array overflow, dropping fence %d",
                 safef.SyncPointID);
            NV_ASSERT(i != NVGR_MAX_FENCES);
        }
    }
}

void NvGrGetFences (gralloc_module_t const* module,
                    buffer_handle_t handle,
                    NvRmFence* fences,
                    int* len)
{
    NvNativeHandle* h = (NvNativeHandle*)handle;

    NV_ASSERT(h->Magic == NVGR_HANDLE_MAGIC);
    NV_ASSERT(h->Buf->Magic == NVGR_BUFFER_MAGIC);
    NV_ASSERT(fences && len);
    NV_ASSERT(h->Buf->LockState != 0);

    if (h->Buf->LockState & NVGR_WRITE_LOCK_FLAG)
    {
        int i, wrote = 0;

        // Exclusive lock held
        // If there are read fences, the write fence can be ignored.
        // If there are no read fences, the previous write must be considered
        // to avoid out of order writes
        for (i = 1; i < NVGR_MAX_FENCES; i++)
        {
            NvRmFence* f = &h->Buf->Fence[i];
            if (f->SyncPointID == NVRM_INVALID_SYNCPOINT_ID)
                break;
            if (wrote == *len)
            {
                NV_ASSERT(!"Caller provided too short fence array");
                break;
            }
            fences[wrote++] = *f;
        }

        // No read fences, return the write fence.
        if (wrote == 0) {
            NvRmFence* f = &h->Buf->Fence[0];
            if (f->SyncPointID != NVRM_INVALID_SYNCPOINT_ID) {
                fences[wrote++] = *f;
            }
        }

        *len = wrote;
    }
    else
    {
        pthread_mutex_lock(&h->Buf->FenceMutex);
        if (h->Buf->Fence[0].SyncPointID != NVRM_INVALID_SYNCPOINT_ID)
        {
            NV_ASSERT(*len >= 1);
            // Only index 0 is a write fence.  The others are read
            // fences and other readers don't need to wait for them.
            fences[0] = h->Buf->Fence[0];
            *len = 1;
        }
        else
            *len = 0;
        pthread_mutex_unlock(&h->Buf->FenceMutex);
    }
}

void NvGrSetStereoInfo (gralloc_module_t const* module,
                        buffer_handle_t handle,
                        NvU32 stereoInfo)
{
    NvNativeHandle* h = (NvNativeHandle*)handle;

    NV_ASSERT(h->Magic == NVGR_HANDLE_MAGIC);
    NV_ASSERT(h->Buf->Magic == NVGR_BUFFER_MAGIC);

    h->Buf->StereoInfo = stereoInfo;
}

/* WAR bug 827707.  If the buffer is locked for texture and the crop
 * is smaller than the texture, we need to replicate the edge texels
 * to avoid filtering artifacts.
 */
void NvGrSetSourceCrop (gralloc_module_t const* module,
                        buffer_handle_t handle,
                        NvRect *cropRect)
{
    NvNativeHandle* h = (NvNativeHandle*)handle;

    NV_ASSERT(h->Magic == NVGR_HANDLE_MAGIC);
    NV_ASSERT(h->Buf->Magic == NVGR_BUFFER_MAGIC);

    if (cropRect->right <= cropRect->left ||
        cropRect->bottom <= cropRect->top) {
        h->Buf->Flags &= ~NvGrBufferFlags_SourceCropValid;
        return;
    }

    h->Buf->SourceCrop = *cropRect;
    h->Buf->Flags |= NvGrBufferFlags_SourceCropValid;
}

void NvGrDumpBuffer(gralloc_module_t const *module,
                    buffer_handle_t handle,
                    const char *filename)
{
    NvError err;
    NvNativeHandle *h;
    int result;
    void *vaddr;

    h = (NvNativeHandle *) handle;

    result = NvGrLock(module, handle, GRALLOC_USAGE_SW_READ_RARELY,
                      0, 0, h->Surf[0].Width, h->Surf[0].Height, &vaddr);

    if (result == 0)
    {
        err = NvRmSurfaceDump(h->Surf, h->SurfCount, filename);

        NvGrUnlock(module, handle);
    }
}
