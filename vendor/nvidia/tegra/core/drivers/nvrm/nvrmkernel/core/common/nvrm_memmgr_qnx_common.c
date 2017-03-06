/*
 * Copyright (c) 2011 - 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include <sys/mman.h>
#include <sys/cache.h>
#include <sys/neutrino.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <qnx/nvmap_devctls.h>
#include <nvrm_heap_simple.h>
#include "nvcommon.h"
#include "nvrm_memmgr.h"
#include "nvrm_memmgr_private.h"

extern int g_nvmapfd;

#define NVRM_MEMMGR_USEGART NV_FALSE
#undef CONVERT_CARVEOUT_TO_GART
NvRmHeapSimple      gs_GartAllocator;
NvBool              gs_GartInited = NV_FALSE;
NvU32               *gs_GartSave = NULL;
NvBool              g_useGART = NVRM_MEMMGR_USEGART;
static NvU64        gs_DummyBufferPhys;
static struct cache_ctrl gs_CacheInfo;

int NvRm_MemmgrGetIoctlFile(void)
{
    NV_ASSERT(g_nvmapfd >= 0);
    return g_nvmapfd;
}

NvError NvRmMemHandleCreate(
    NvRmDeviceHandle hRm,
    NvRmMemHandle *hMem,
    NvU32 Size)
{
    struct nvmap_create_handle op;
    int err;
    NV_ASSERT(g_nvmapfd >= 0);

    if (hMem == NULL)
        return NvError_NotInitialized;

    op.size = Size;
    if ((err = devctl(g_nvmapfd, NVMAP_IOC_CREATE, &op, sizeof(op), NULL)) != EOK)
    {
        if (err == EPERM)
            return NvError_AccessDenied;
        else if (err == ENOMEM)
            return NvError_InsufficientMemory;
        else if (err == EINVAL)
            return NvError_NotInitialized;

        return NvError_IoctlFailed;
    }
    *hMem = (NvRmMemHandle)op.handle;
    return NvSuccess;
}

NvError NvRmMemHandleFromId(NvU32 Id, NvRmMemHandle *hMem)
{
    struct nvmap_create_handle op = {{0}};
    int err;

    if (hMem == NULL)
        return NvError_NotInitialized;

    op.id = Id;
    if ((err = devctl(g_nvmapfd, NVMAP_IOC_FROM_ID, &op, sizeof(op), NULL)) != EOK)
    {
        if (err == EPERM)
            return NvError_AccessDenied;
        else if (err == ENOMEM)
            return NvError_InsufficientMemory;
        else if (err == EINVAL)
            return NvError_NotInitialized;

        return NvError_IoctlFailed;
    }
    *hMem = (NvRmMemHandle)op.handle;
    return NvSuccess;
}

NvError NvRmMemHandlePreserveHandle(NvRmMemHandle hMem, NvU32 *Key)
{
    return NvError_NotSupported;
}

NvError NvRmMemHandleClaimPreservedHandle(
    NvRmDeviceHandle hRm,
    NvU32 Key,
    NvRmMemHandle *hMem)
{
    struct nvmap_create_handle op = {{0}};
    int err;

    if (hMem == NULL)
        return NvError_NotInitialized;

    op.key = Key;
    if ((err = devctl(g_nvmapfd, NVMAP_IOC_CLAIM, &op, sizeof(op), NULL)) != EOK)
    {
        if (err == EPERM)
            return NvError_AccessDenied;
        else if (err == ENOMEM)
            return NvError_InsufficientMemory;
        else if (err == EINVAL)
            return NvError_NotInitialized;

        return NvError_IoctlFailed;
    }
    *hMem = (NvRmMemHandle)op.handle;
    return NvSuccess;
}

NvError NvRmMemAllocInternal(
    NvRmMemHandle hMem,
    const NvRmHeap *Heaps,
    NvU32 NumHeaps,
    NvU32 Alignment,
    NvOsMemAttribute Coherency)
{
    struct nvmap_alloc_handle op = {0};
    int err;

    if (Alignment & (Alignment-1)) {
        printf("bad alloc %08x\n", Alignment);
        return NvError_BadValue;
    }

    op.handle = (rmos_u32)hMem;
    op.align = Alignment;

    if (Coherency == NvOsMemAttribute_InnerWriteBack)
        op.flags = NVMAP_HANDLE_INNER_CACHEABLE;
    else if (Coherency == NvOsMemAttribute_WriteBack)
        op.flags = NVMAP_HANDLE_CACHEABLE;
    else if (Coherency == NvOsMemAttribute_WriteCombined)
        op.flags = NVMAP_HANDLE_WRITE_COMBINE;
    else
        op.flags = NVMAP_HANDLE_UNCACHEABLE;

    if (!NumHeaps)
    {
        // GART is removed due to possible deadlocks when GART memory is pinned
        // outside NvRmChannel. Only drivers using NvRmChannel API to pin memory should use GART.
        op.heap_mask =
            NVMAP_HEAP_CARVEOUT_GENERIC;
#ifdef CONVERT_CARVEOUT_TO_GART
        if (getenv("CONVERT_CARVEOUT_TO_GART"))
            op.heap_mask = NVMAP_HEAP_IOVMM;
#endif

        if (devctl(g_nvmapfd, NVMAP_IOC_ALLOC, &op, sizeof(op), NULL) == EOK)
            return NvSuccess;
    }
    else
    {
        NvU32 i;
        for (i = 0; i < NumHeaps; i++)
        {
            switch (Heaps[i])
            {
            case NvRmHeap_ExternalCarveOut:
#ifdef CONVERT_CARVEOUT_TO_GART
                if (!getenv("CONVERT_CARVEOUT_TO_GART"))
                {
                    op.heap_mask = NVMAP_HEAP_CARVEOUT_GENERIC;
                    break;
                } // else falls through
#else
                op.heap_mask = NVMAP_HEAP_CARVEOUT_GENERIC;
                break;
#endif
            case NvRmHeap_GART:
                op.heap_mask = NVMAP_HEAP_IOVMM;
                break;
            case NvRmHeap_IRam:
                op.heap_mask = NVMAP_HEAP_CARVEOUT_IRAM;
                break;
            case NvRmHeap_External:
                // NvRmHeap_External heap cannot serve contiguous memory
                // region in a long run due to fragmentation of system
                // memory. Falling down to carveout instead.
                // Single page allocations will be served from
                // system memory automatically, since
                // they are always contiguous.

                 /* need to set mask, otherwise nvmap returns EINVAL in nvmap_alloc_handle_id due to heap_mask = 0 */
                 op.heap_mask = NVMAP_HEAP_CARVEOUT_GENERIC;
                 break;
            default:
                op.heap_mask = 0;
                break;
            }

            if ((err = devctl(g_nvmapfd, NVMAP_IOC_ALLOC, &op, sizeof(op), NULL)) == EOK)
                return NvSuccess;

            if (err != ENOMEM)
                break;
        }
    }

    if (err == EPERM)
        return NvError_AccessDenied;
    else if (err == EINVAL)
        return NvError_BadValue;
    else
        return NvError_InsufficientMemory;
}

NvError NvRmMemAlloc(
    NvRmMemHandle hMem,
    const NvRmHeap *Heaps,
    NvU32 NumHeaps,
    NvU32 Alignment,
    NvOsMemAttribute Coherency)
{
    return NvRmMemAllocInternal(hMem, Heaps, NumHeaps, Alignment, Coherency);
}

NvError NvRmMemAllocTagged(
    NvRmMemHandle hMem,
    const NvRmHeap *Heaps,
    NvU32 NumHeaps,
    NvU32 Alignment,
    NvOsMemAttribute Coherency,
    NvU16 Tags)
{
    return NvRmMemAllocInternal(hMem, Heaps, NumHeaps, Alignment, Coherency);
}

void NvRmMemHandleFree(NvRmMemHandle hMem)
{
    unsigned int handle = (unsigned int)hMem;
    int err;
    if ((err = devctl(g_nvmapfd, NVMAP_IOC_FREE, &handle, sizeof(unsigned), NULL)) != EOK)
        NvOsDebugPrintf("NVMAP_IOC_FREE failed: %d\n", err);
}

NvU32 NvRmMemGetId(NvRmMemHandle hMem)
{
    struct nvmap_create_handle op = {{0}};
    int err;
    op.handle = (rmos_u32)hMem;
    op.id = 0;

    if ((err = devctl(g_nvmapfd, NVMAP_IOC_GET_ID, &op, sizeof(op), NULL)) != EOK)
    {
        NvOsDebugPrintf("NVMAP_IOC_GET_ID failed: %d\n", err);
        return 0;
    }
    return (NvU32)op.id;
}

void NvRmMemUnpinMult(NvRmMemHandle *hMems, NvU32 Count)
{
    int err;
    if (hMems == NULL)
        return;

    if (Count == 1)
    {
        struct nvmap_pin_handle op = {0};
        op.count = Count;
        op.handles = (unsigned long)hMems[0];

        if ((err = devctl(g_nvmapfd, NVMAP_IOC_UNPIN_MULT, &op, sizeof(op), NULL)) != EOK)
            NvOsDebugPrintf("NVMAP_IOC_UNPIN_MULT failed: %d\n", err);
    }
    else
    {
        NvU8 *buff;
        NvU32 len;
        struct nvmap_pin_handle *op;
        unsigned long *ptr;
        int err;

        len = Count * (sizeof(op->handles));
        buff = NvOsAlloc(sizeof(*op) + len);
        if (buff == NULL)
        {
            NvOsDebugPrintf("%s: insufficient memory\n", __func__);
            return;
        }

        NvOsMemset(buff, 0, sizeof(*op) + len);

        op = (struct nvmap_pin_handle *)buff;
        op->count = Count;

        ptr = buff + sizeof(*op);
        NvOsMemcpy(ptr, hMems, Count * (sizeof(op->handles)));

        if ((err = devctl(g_nvmapfd, NVMAP_IOC_UNPIN_MULT, buff, sizeof(*op) + len, NULL)) != EOK)
            NvOsDebugPrintf("NVMAP_IOC_UNPIN_MULT failed: %d\n", err);
        NvOsFree(buff);
    }
}

void NvRmMemPinMult(NvRmMemHandle *hMems, NvU32 *addrs, NvU32 Count)
{
    if (hMems == NULL)
        return;

    if (Count == 1)
    {
        struct nvmap_pin_handle op = {0};
        int err;
        op.count = Count;
        op.handles = (unsigned long)hMems[0];
        op.addr = 0;

        if ((err = devctl(g_nvmapfd, NVMAP_IOC_PIN_MULT, &op, sizeof(op), NULL)) != EOK)
            NvOsDebugPrintf("NVMAP_IOC_PIN_MULT failed: %d\n", err);
        *addrs = op.addr;
    }
    else
    {
        NvU8 *buff;
        NvU32 len;
        struct nvmap_pin_handle *op;
        unsigned long *ptr;
        int err;

        len = Count * (sizeof(op->handles) + sizeof(op->addr));
        buff = NvOsAlloc(sizeof(*op) + len);
        if (buff == NULL)
        {
            NvOsDebugPrintf("%s: insufficient memory\n", __func__);
            return;
        }

        NvOsMemset(buff, 0, sizeof(*op) + len);

        op = (struct nvmap_pin_handle *)buff;
        op->count = Count;

        ptr = buff + sizeof(*op);
        NvOsMemcpy(ptr, hMems, Count * (sizeof(op->handles)));

        if ((err = devctl(g_nvmapfd, NVMAP_IOC_PIN_MULT, buff, sizeof(*op) + len, NULL)) != EOK) {
            NvOsDebugPrintf("NVMAP_IOC_PIN_MULT failed: %d\n", err);
            NvOsFree(buff);
            return;
        }

        ptr = buff + sizeof(*op) + op->count * sizeof(op->handles);
        NvOsMemcpy(addrs, ptr, Count * (sizeof(op->addr)));
        NvOsFree(buff);
    }
}

NvU32 NvRmMemPin(NvRmMemHandle hMem)
{
    NvU32 addr = ~0;
    NvRmMemPinMult(&hMem, &addr, 1);
    return addr;
}

void NvRmMemUnpin(NvRmMemHandle hMem)
{
    NvRmMemUnpinMult(&hMem, 1);
}

NvError NvRmMemMap(
    NvRmMemHandle hMem,
    NvU32         Offset,
    NvU32         Size,
    NvU32         Flags,
    void        **pVirtAddr)
{
    struct nvmap_map_caller op = {0};
    int prot;
    unsigned int page_size;
    size_t adjust_len;

    if (!pVirtAddr || !hMem)
        return NvError_BadParameter;

    if (g_nvmapfd < 0)
        return NvError_KernelDriverNotFound;

    page_size = (unsigned int)getpagesize();
    adjust_len = Size + (Offset & (page_size - 1));
    adjust_len += (size_t)(page_size - 1);
    adjust_len &= (size_t)~(page_size - 1);

    op.handle = (uintptr_t)hMem;
    op.addr   = (unsigned long)*pVirtAddr;
    op.offset = Offset & ~(page_size - 1);
    op.length = adjust_len;
    op.flags = 0;

    if (devctl(g_nvmapfd, NVMAP_IOC_MMAP, &op, sizeof(op), NULL) != EOK) {
        *pVirtAddr = NULL;
        return NvError_InvalidAddress;
    }

    prot = PROT_NONE;
    if (Flags & NVOS_MEM_READ)
        prot |= PROT_READ;
    if (Flags & NVOS_MEM_WRITE)
        prot |= PROT_WRITE;
    if (Flags & NVOS_MEM_EXECUTE)
        prot |= PROT_EXEC;
    prot |= PROT_NOCACHE;
    if (op.flags & NVMAP_HEAP_CARVEOUT_IRAM ||
            op.flags & NVMAP_HEAP_CARVEOUT_GENERIC) {
        /*
         * NVMAP-NOTE: Returned handle contains the physical address
         */
        /*
         * FIXME-NVMAP: Add correct cache settings
         */
         *pVirtAddr = mmap(0, adjust_len, prot, MAP_SHARED | MAP_PHYS, NOFD,
                 (rmos_u32)op.handle + (Offset & ~(page_size - 1)));
         if (*pVirtAddr == MAP_FAILED)
             return NvError_InsufficientMemory;
    }
    else if (op.flags & NVMAP_HEAP_SYSMEM) {
        /*
         * NVMAP-NOTE: Returned handle is id for shared memory
         */
        rmos_u8 shm_path[NVMAP_SHM_NAME_MAX_LENGTH];
        int shm_fd;

        snprintf(shm_path, NVMAP_SHM_NAME_MAX_LENGTH, NVMAP_SHM_FORMAT_STRING, op.handle);
        shm_fd = shm_open(shm_path, O_RDWR, 0777); // already exists.
        if (shm_fd == -1)
            return NvError_InsufficientMemory;

         *pVirtAddr = mmap(0, adjust_len, prot, MAP_SHARED, shm_fd,
                Offset & ~(page_size - 1));
         close(shm_fd);
         if (*pVirtAddr == MAP_FAILED)
             return NvError_InsufficientMemory;
    }
    else
        return NvError_InsufficientMemory;
    *pVirtAddr = (void *)((uintptr_t)*pVirtAddr + (Offset & (page_size - 1)));
    return NvSuccess;
}

void NvRmMemCacheMaint(
    NvRmMemHandle hMem,
    void         *pMapping,
    NvU32         Size,
    NvBool        Writeback,
    NvBool        Invalidate)
{
    struct nvmap_cache_op op = {0};
    int err;

    if ((pMapping == NULL) || (hMem == NULL))
        return;

    op.handle = (rmos_u32)hMem;
    if ((err = devctl(g_nvmapfd, NVMAP_IOC_CACHE, &op, sizeof(op), NULL))
            != EOK)
    {
        NvOsDebugPrintf("NVMAP_IOC_CACHE failed: %d\n", err);
        return;
    }

    if (op.cache_flags == NVMAP_HANDLE_CACHEABLE ||
        op.cache_flags == NVMAP_HANDLE_INNER_CACHEABLE)
    {
        int flags = 0;

        if (Writeback)
            flags |= MS_SYNC;

        if (Invalidate)
            flags |= MS_INVALIDATE;

        if (!flags)
            return;

        msync(pMapping, Size, flags);
    }
    else if (op.cache_flags == NVMAP_HANDLE_WRITE_COMBINE)
    {
        /* flush dummy buffer to ensure that store buffers are drained */
        CACHE_FLUSH(&gs_CacheInfo, &gs_DummyBufferPhys, gs_DummyBufferPhys,
            sizeof(gs_DummyBufferPhys));
    }
}

void NvRmMemUnmap(NvRmMemHandle hMem, void *pVirtAddr, NvU32 Size)
{
    unsigned int page_size;
    size_t adjust_len;
    void *adjust_addr;
    unsigned int handle = (unsigned int)hMem;
    int err;

    if (!hMem || !pVirtAddr || !Size)
        return;

    page_size = (unsigned int)getpagesize();

    adjust_addr = (void *)((uintptr_t)pVirtAddr & ~(page_size - 1));
    adjust_len = Size;
    adjust_len += ((uintptr_t)pVirtAddr & (page_size - 1));
    adjust_len += (size_t)(page_size - 1);
    adjust_len &= ~(page_size - 1);

    munmap(adjust_addr, adjust_len);
    if ((err = devctl(g_nvmapfd, NVMAP_IOC_MUNMAP, &handle, sizeof(unsigned int), NULL))
            != EOK)
        NvOsDebugPrintf("NVMAP_IOC_UNMAP failed: %d\n", err);
}

void NvRmMemWrite(NvRmMemHandle hMem,NvU32 Offset,const void *pSrc, NvU32 Size)
{
    NvRmMemWriteStrided(hMem, Offset, Size, pSrc, Size, Size, 1);
}

void NvRmMemRead(NvRmMemHandle hMem, NvU32 Offset, void *pDst, NvU32 Size)
{
    NvRmMemReadStrided(hMem, Offset, Size, pDst, Size, Size, 1);
}

void NvRmMemReadStrided(
    NvRmMemHandle hMem,
    NvU32 Offset,
    NvU32 SrcStride,
    void *pDst,
    NvU32 DstStride,
    NvU32 ElementSize,
    NvU32 Count)
{
    iov_t sv[1];
    iov_t rv[2];
    struct nvmap_rw_handle op;
    int ret;

    op.handle = (rmos_u32)hMem;
    op.elem_size = ElementSize;
    op.hmem_stride = SrcStride;
    op.user_stride = DstStride;
    op.count = Count;
    op.offset = Offset;

    SETIOV(&sv[0], &op, sizeof(op));
    SETIOV(&rv[0], &op, sizeof(op));
    SETIOV(&rv[1], pDst, (DstStride * (Count - 1) + ElementSize));
    /* FIXME: add support for EINTR */
    ret = devctlv(g_nvmapfd, NVMAP_IOC_READ, NV_ARRAY_SIZE(sv),
                  NV_ARRAY_SIZE(rv), sv, rv, NULL);
    if (ret != EOK)
        NvOsDebugPrintf("NVMAP_IOC_READ failed: %d\n", ret);
}

void NvRmMemWriteStrided(
    NvRmMemHandle hMem,
    NvU32 Offset,
    NvU32 DstStride,
    const void *pSrc,
    NvU32 SrcStride,
    NvU32 ElementSize,
    NvU32 Count)
{
    iov_t sv[2];
    struct nvmap_rw_handle op;
    int ret;

    op.handle = (rmos_u32)hMem;
    op.elem_size = ElementSize;
    op.hmem_stride = DstStride;
    op.user_stride = SrcStride;
    op.count = Count;
    op.offset = Offset;

    SETIOV(&sv[0], &op, sizeof(op));
    SETIOV(&sv[1], pSrc, (SrcStride * (Count - 1) + ElementSize));
    ret = devctlv(g_nvmapfd, NVMAP_IOC_WRITE, NV_ARRAY_SIZE(sv), 0, sv, NULL,
                  NULL);
    if (ret != EOK)
        NvOsDebugPrintf("NVMAP_IOC_WRITE failed: %d\n", ret);
}

NvU32 NvRmMemGetSize(NvRmMemHandle hMem)
{
    struct nvmap_handle_param op = {0};
    int err;

    op.handle = (unsigned long)hMem;
    op.param = NVMAP_HANDLE_PARAM_SIZE;
    err = devctl(g_nvmapfd, NVMAP_IOC_PARAM, &op, sizeof(op), NULL);
    if (err != EOK) {
        NvOsDebugPrintf("NVMAP_IOC_PARAM failed: %d\n", err);
        return 0;
    }
    return (NvU32)op.result;
}

void NvRmMemMove(NvRmMemHandle hDstMem, NvU32 DstOffset,
    NvRmMemHandle hSrcMem, NvU32 SrcOffset, NvU32 Size)
{
    NvU8 Buffer[8192];

    while (Size)
    {
        NvU32 Count = NV_MIN(sizeof(Buffer), Size);
        NvRmMemRead(hSrcMem, SrcOffset, Buffer, Count);
        NvRmMemWrite(hDstMem, DstOffset, Buffer, Count);
        Size -= Count;
        DstOffset += Count;
        SrcOffset += Count;
    }
}

NvU32 NvRmMemGetAlignment(NvRmMemHandle hMem)
{
    struct nvmap_handle_param op = {0};
    int err;

    op.handle = (unsigned long)hMem;
    op.param = NVMAP_HANDLE_PARAM_ALIGNMENT;
    if ((err = devctl(g_nvmapfd, NVMAP_IOC_PARAM, &op, sizeof(op), NULL))
            != EOK)
    {
        NvOsDebugPrintf("NVMAP_IOC_PARAM failed: %d\n", err);
        return 0;
    }
    return (NvU32)op.result;
}

NvU32 NvRmMemGetCacheLineSize(void)
{
    return 32;
}

NvU32 NvRmMemGetAddress(NvRmMemHandle hMem, NvU32 Offset)
{
    struct nvmap_handle_param op = {0};
    NvU32 addr;
    int err;

    op.handle = (unsigned long)hMem;
    op.param = NVMAP_HANDLE_PARAM_BASE;

    if ((err = devctl(g_nvmapfd, NVMAP_IOC_PARAM, &op, sizeof(op), NULL))
            != EOK)
    {
        NvOsDebugPrintf("NVMAP_IOC_PARAM failed: %d\n", err);
        return ~0ul;
    }

    addr = (NvU32)op.result;

    if (addr == ~0ul)
        return addr;

    return addr + Offset;
}

NvRmHeap NvRmMemGetHeapType(NvRmMemHandle hMem, NvU32 *BasePhysAddr)
{
    struct nvmap_handle_param op = {0};
    int err;

    op.handle = (unsigned long)hMem;
    op.param = NVMAP_HANDLE_PARAM_BASE;

    if ((err = devctl(g_nvmapfd, NVMAP_IOC_PARAM, &op, sizeof(op), NULL))
            != EOK)
    {
        NvOsDebugPrintf("NVMAP_IOC_PARAM failed: %d\n", err);
        return (NvRmHeap) 0;
    }

    *BasePhysAddr = (NvU32)op.result;

    op.handle = (unsigned long)hMem;
    op.param = NVMAP_HANDLE_PARAM_HEAP;

    if ((err = devctl(g_nvmapfd, NVMAP_IOC_PARAM, &op, sizeof(op), NULL))
            != EOK)
    {
        NvOsDebugPrintf("NVMAP_IOC_PARAM failed: %d\n", err);
        return (NvRmHeap) 0;
    }

    if (op.result & NVMAP_HEAP_SYSMEM)
        return NvRmHeap_External;
    else if (op.result & NVMAP_HEAP_IOVMM)
        return NvRmHeap_GART;
    else if (op.result & NVMAP_HEAP_CARVEOUT_IRAM)
        return NvRmHeap_IRam;
    else
        return NvRmHeap_ExternalCarveOut;
}

NvError NvRmMemGetStat(NvRmMemStat Stat, NvS32 *Result)
{
    /* FIXME: implement */
    return NvError_NotSupported;
}

void __attribute__((constructor)) nvmap_constructor()
{
    g_nvmapfd = open(NVMAP_DEV_NODE, O_RDWR);
    if (g_nvmapfd < 0)
        NvOsDebugPrintf("\n\n\n****nvmap device open failed****\n\n\n");

    if (ThreadCtl(_NTO_TCTL_IO, 0) == -1)
        NvOsDebugPrintf("%s: ThreadCtl failed (%d)", __func__, errno);

    gs_CacheInfo.fd = NOFD;
    if (cache_init(0, &gs_CacheInfo, NULL) == -1)
        NvOsDebugPrintf("%s: cache_init failed (%d)", __func__, errno);

    if (mem_offset64(&gs_DummyBufferPhys, NOFD, sizeof(gs_DummyBufferPhys),
            &gs_DummyBufferPhys, NULL) == -1)
        NvOsDebugPrintf("%s: mem_offset64 failed (%d)", __func__, errno);
}

void __attribute__((destructor)) nvmap_destructor()
{
    if (g_nvmapfd >= 0)
        close(g_nvmapfd);

    if (gs_CacheInfo.cache_line_size != 0)
        cache_fini(&gs_CacheInfo);
}
