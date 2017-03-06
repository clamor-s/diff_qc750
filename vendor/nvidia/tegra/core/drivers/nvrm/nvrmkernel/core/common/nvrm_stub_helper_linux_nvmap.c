/*
 * Copyright (c) 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "nvcommon.h"
#include "nvassert.h"
#include "nvos.h"
#include "nvrm_memmgr.h"
#include "nvrm_power.h"
#include "nvrm_rmctrace.h"
#include "nvrm_ioctls.h"
#include "nvrm_memmgr.h"
#include "linux/nvmem_ioctl.h"
#include "nvrm_stub_helper_linux.h"

#if defined(ANDROID) && !defined(PLATFORM_IS_GINGERBREAD) && NV_DEBUG
#include <memcheck.h>
#define NV_USE_VALGRIND 1
#else
#define NV_USE_VALGRIND 0
#endif

static int s_nvmapfd = -1;

NvError NvMapMemHandleCreate(
    NvRmDeviceHandle hRm,
    NvRmMemHandle *hMem,
    NvU32 Size)
{
    struct nvmem_create_handle op;

    NvOsMemset(&op, 0, sizeof(op));
    op.size = Size;
    if (ioctl(s_nvmapfd, (int)NVMEM_IOC_CREATE, &op))
    {
        if (errno == EPERM)
            return NvError_AccessDenied;
        else if (errno == ENOMEM)
            return NvError_InsufficientMemory;
        else if (errno == EINVAL)
            return NvError_NotInitialized;

        return NvError_IoctlFailed;
    }
    *hMem = (NvRmMemHandle)op.handle;
    return NvSuccess;
}

NvError NvMapMemHandleFromId(NvU32 Id, NvRmMemHandle *hMem)
{
    struct nvmem_create_handle op = {{0}};

    op.id = Id;
    if (ioctl(s_nvmapfd, (int)NVMEM_IOC_FROM_ID, &op))
    {
        if (errno == EPERM)
            return NvError_AccessDenied;
        else if (errno == ENOMEM)
            return NvError_InsufficientMemory;
        else if (errno == EINVAL)
            return NvError_NotInitialized;

        return NvError_IoctlFailed;
    }
    *hMem = (NvRmMemHandle)op.handle;
    return NvSuccess;
}

NvError NvMapMemAllocInternalTagged(
    NvRmMemHandle hMem,
    const NvRmHeap *Heaps,
    NvU32 NumHeaps,
    NvU32 Alignment,
    NvOsMemAttribute Coherency,
    NvU16 Tags)
{
    struct nvmem_alloc_handle op = {0};

    if (Alignment & (Alignment-1)) {
        printf("bad alloc %08x\n", op.heap_mask);
        return NvError_BadValue;
    }

    op.handle = (__u32)hMem;
    op.align = Alignment;

    if (Coherency == NvOsMemAttribute_InnerWriteBack)
        op.flags = NVMEM_HANDLE_INNER_CACHEABLE;
    else if (Coherency == NvOsMemAttribute_WriteBack)
        op.flags = NVMEM_HANDLE_CACHEABLE;
    else if (Coherency == NvOsMemAttribute_WriteCombined)
        op.flags = NVMEM_HANDLE_WRITE_COMBINE;
    else
        op.flags = NVMEM_HANDLE_UNCACHEABLE;

    op.flags |= ((NvU32) Tags) << 16;

    if (!NumHeaps)
    {
        // GART is removed due to possible deadlocks when GART memory is pinned
        // outside NvRmChannel. Only drivers using NvRmChannel API to pin memory should use GART.
        op.heap_mask =
            NVMEM_HEAP_CARVEOUT_GENERIC;
#ifdef CONVERT_CARVEOUT_TO_GART
        if (getenv("CONVERT_CARVEOUT_TO_GART"))
            op.heap_mask = NVMEM_HEAP_IOVMM;
#endif

        if (ioctl(s_nvmapfd, NVMEM_IOC_ALLOC, &op) == 0)
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
                    op.heap_mask = NVMEM_HEAP_CARVEOUT_GENERIC;
                    break;
                } // else falls through
#else
                op.heap_mask = NVMEM_HEAP_CARVEOUT_GENERIC;
                break;
#endif
            case NvRmHeap_GART:
                op.heap_mask = NVMEM_HEAP_IOVMM; break;
            case NvRmHeap_IRam:
                op.heap_mask = NVMEM_HEAP_CARVEOUT_IRAM; break;
            case NvRmHeap_VPR:
                op.heap_mask = NVMEM_HEAP_CARVEOUT_VPR; break;
            case NvRmHeap_External:
                // NvRmHeap_External heap cannot serve contiguous memory
                // region in a long run due to fragmentation of system
                // memory. Falling down to carveout instead.
                // Single page allocations will be served from
                // system memory automatically, since
                // they are always contiguous.
            case NvRmHeap_Camera:
                op.heap_mask = g_NvRmMemCameraHeapUsage; break;
            default:
                op.heap_mask = 0; break;
            }

            if (ioctl(s_nvmapfd, NVMEM_IOC_ALLOC, &op) == 0)
                return NvSuccess;

            if (errno != ENOMEM)
                break;
        }
    }

    if (errno == EPERM)
        return NvError_AccessDenied;
    else if (errno == EINVAL)
        return NvError_BadValue;
    else
        return NvError_InsufficientMemory;
}

void NvMapMemHandleFree(NvRmMemHandle hMem)
{
    if (ioctl(s_nvmapfd, (int)NVMEM_IOC_FREE, (unsigned)hMem))
        NvOsDebugPrintf("NVMEM_IOC_FREE failed: %s\n", strerror(errno));
}

NvU32 NvMapMemGetId(NvRmMemHandle hMem)
{
    struct nvmem_create_handle op = {{0}};
    op.handle = (__u32)hMem;
    op.id = 0;

    if (ioctl(s_nvmapfd, (int)NVMEM_IOC_GET_ID, &op))
    {
        NvOsDebugPrintf("NVMEM_IOC_GET_ID failed: %s\n", strerror(errno));
        return 0;
    }
    return (NvU32)op.id;
}

void NvMapMemUnpinMult(NvRmMemHandle *hMems, NvU32 Count)
{
    struct nvmem_pin_handle op = {0};

    op.count = Count;
    if (Count == 1)
        op.handles = (unsigned long)hMems[0];
    else
        op.handles = (unsigned long)hMems;

    if (ioctl(s_nvmapfd, NVMEM_IOC_UNPIN_MULT, &op))
    {
        NvOsDebugPrintf("NVMEM_IOC_UNPIN_MULT failed: %s\n", strerror(errno));
    }
}

void NvMapMemPinMult(NvRmMemHandle *hMems, NvU32 *addrs, NvU32 Count)
{
    struct nvmem_pin_handle op = {0};

    op.count = Count;
    if (Count == 1)
    {
        op.handles = (unsigned long)hMems[0];
        op.addr = 0;
    }
    else
    {
        op.handles = (unsigned long)hMems;
        op.addr = (unsigned long)addrs;
    }

    if (ioctl(s_nvmapfd, (int)NVMEM_IOC_PIN_MULT, &op))
    {
        NvOsDebugPrintf("NVMEM_IOC_PIN_MULT failed: %s\n", strerror(errno));
    }
    else if (Count==1)
    {
        *addrs = op.addr;
    }
}

NvError NvMapMemMap(
    NvRmMemHandle hMem,
    NvU32         Offset,
    NvU32         Size,
    NvU32         Flags,
    void        **pVirtAddr)
{
    struct nvmem_map_caller op = {0};
    int prot;
    unsigned int page_size;
    size_t adjust_len;

    if (!pVirtAddr || !hMem)
        return NvError_BadParameter;

    if (s_nvmapfd < 0)
        return NvError_KernelDriverNotFound;

    prot = PROT_NONE;
    if (Flags & NVOS_MEM_READ) prot |= PROT_READ;
    if (Flags & NVOS_MEM_WRITE) prot |= PROT_WRITE;
    if (Flags & NVOS_MEM_EXECUTE) prot |= PROT_EXEC;

    page_size = (unsigned int)getpagesize();
    adjust_len = Size + (Offset & (page_size-1));
    adjust_len += (size_t)(page_size-1);
    adjust_len &= (size_t)~(page_size-1);
    *pVirtAddr = mmap(NULL, adjust_len, prot, MAP_SHARED, s_nvmapfd, 0);
    if (!*pVirtAddr)
        return NvError_InsufficientMemory;

    op.handle = (uintptr_t)hMem;
    op.addr   = (unsigned long)*pVirtAddr;
    op.offset = Offset & ~(page_size-1);
    op.length = adjust_len;
    op.flags = 0;
    if (Flags & NVOS_MEM_MAP_WRITEBACK)
        op.flags = NVMEM_HANDLE_CACHEABLE;
    else if (Flags & NVOS_MEM_MAP_INNER_WRITEBACK)
        op.flags = NVMEM_HANDLE_INNER_CACHEABLE;

    if (ioctl(s_nvmapfd, (int)NVMEM_IOC_MMAP, &op)) {
        munmap(*pVirtAddr, adjust_len);
        *pVirtAddr = NULL;
        return NvError_InvalidAddress;
    }

    *pVirtAddr = (void *)((uintptr_t)*pVirtAddr + (Offset & (page_size-1)));
    return NvSuccess;
}

void NvMapMemCacheMaint(
    NvRmMemHandle hMem,
    void         *pMapping,
    NvU32         Size,
    NvBool        Writeback,
    NvBool        Invalidate)
{
    struct nvmem_cache_op op = {0};

    if (!pMapping || !hMem || s_nvmapfd < 0 || !(Writeback || Invalidate))
        return;

    op.handle = (__u32)hMem;
    op.addr = (unsigned long)pMapping;
    op.len  = Size;
    if (Writeback && Invalidate)
        op.op = NVMEM_CACHE_OP_WB_INV;
    else if (Writeback)
        op.op = NVMEM_CACHE_OP_WB;
    else
        op.op = NVMEM_CACHE_OP_INV;

    if (ioctl(s_nvmapfd, NVMEM_IOC_CACHE, &op)) {
        NV_ASSERT(!"Mem handle cache maintenance failed\n");
    }
}

void NvMapMemUnmap(NvRmMemHandle hMem, void *pVirtAddr, NvU32 Size)
{
    unsigned int page_size;
    size_t adjust_len;
    void *adjust_addr;

    if (!hMem || !pVirtAddr || !Size)
        return;

    page_size = (unsigned int)getpagesize();

    adjust_addr = (void*)((uintptr_t)pVirtAddr & ~(page_size-1));
    adjust_len = Size;
    adjust_len += ((uintptr_t)pVirtAddr & (page_size-1));
    adjust_len += (size_t)(page_size-1);
    adjust_len &= ~(page_size-1);

    munmap(adjust_addr, adjust_len);
}

void NvMapMemReadStrided(
    NvRmMemHandle hMem,
    NvU32 Offset,
    NvU32 SrcStride,
    void *pDst,
    NvU32 DstStride,
    NvU32 ElementSize,
    NvU32 Count)
{
    struct nvmem_rw_handle op = {0};
#if NV_USE_VALGRIND
    NvU32 i = 0;
#endif

    op.handle = (__u32)hMem;
    op.addr   = (unsigned long)pDst;
    op.elem_size = ElementSize;
    op.hmem_stride = SrcStride;
    op.user_stride = DstStride;
    op.count = Count;
    op.offset = Offset;

    /* FIXME: add support for EINTR */
    if (ioctl(s_nvmapfd, NVMEM_IOC_READ, &op))
    {
        NvOsDebugPrintf("NVMEM_IOC_READ failed: %s\n", strerror(errno));
        return;
    }

#if NV_USE_VALGRIND
    // Tell Valgrind about memory that is now assumed to have valid data
    for(i = 0; i < Count; i++)
    {
        VALGRIND_MAKE_MEM_DEFINED(pDst + i*DstStride, ElementSize);
    }
#endif
}

void NvMapMemWriteStrided(
    NvRmMemHandle hMem,
    NvU32 Offset,
    NvU32 DstStride,
    const void *pSrc,
    NvU32 SrcStride,
    NvU32 ElementSize,
    NvU32 Count)
{
    struct nvmem_rw_handle op = {0};

    op.handle = (__u32)hMem;
    op.addr   = (unsigned long)pSrc;
    op.elem_size = ElementSize;
    op.hmem_stride = DstStride;
    op.user_stride = SrcStride;
    op.count = Count;
    op.offset = Offset;

    /* FIXME: add support for EINTR */
    if (ioctl(s_nvmapfd, NVMEM_IOC_WRITE, &op))
        NvOsDebugPrintf("NVMEM_IOC_WRITE failed: %s\n", strerror(errno));
}

NvU32 NvMapMemGetSize(NvRmMemHandle hMem)
{
    struct nvmem_handle_param op = {0};
    int e;

    op.handle = (unsigned long)hMem;
    op.param = NVMEM_HANDLE_PARAM_SIZE;
    e = ioctl(s_nvmapfd, (int)NVMEM_IOC_PARAM, &op);
    if (e) {
        NvOsDebugPrintf("NVMEM_IOC_PARAM failed: %s\n", strerror(errno));
        return 0;
    }
    return (NvU32)op.result;
}

NvU32 NvMapMemGetAlignment(NvRmMemHandle hMem)
{
    struct nvmem_handle_param op = {0};

    op.handle = (unsigned long)hMem;
    op.param = NVMEM_HANDLE_PARAM_ALIGNMENT;
    if (ioctl(s_nvmapfd, (int)NVMEM_IOC_PARAM, &op))
    {
        NvOsDebugPrintf("NVMEM_IOC_PARAM failed: %s\n", strerror(errno));
        return 0;
    }
    return (NvU32)op.result;
}

NvU32 NvMapMemGetAddress(NvRmMemHandle hMem, NvU32 Offset)
{
    struct nvmem_handle_param op = {0};
    NvU32 addr;

    op.handle = (unsigned long)hMem;
    op.param = NVMEM_HANDLE_PARAM_BASE;

    if (ioctl(s_nvmapfd, (int)NVMEM_IOC_PARAM, &op))
    {
        NvOsDebugPrintf("NVMEM_IOC_PARAM failed: %s\n", strerror(errno));
        return ~0ul;
    }

    addr = (NvU32)op.result;

    if (addr == ~0ul)
        return addr;

    return addr + Offset;
}

NvRmHeap NvMapMemGetHeapType(NvRmMemHandle hMem, NvU32 *BasePhysAddr)
{
    struct nvmem_handle_param op = {0};

    op.handle = (unsigned long)hMem;
    op.param = NVMEM_HANDLE_PARAM_BASE;

    if (ioctl(s_nvmapfd, (int)NVMEM_IOC_PARAM, &op))
    {
        NvOsDebugPrintf("NVMEM_IOC_PARAM failed: %s\n", strerror(errno));
        return (NvRmHeap) 0;
    }

    *BasePhysAddr = (NvU32)op.result;

    op.handle = (unsigned long)hMem;
    op.param = NVMEM_HANDLE_PARAM_HEAP;

    if (ioctl(s_nvmapfd, (int)NVMEM_IOC_PARAM, &op))
    {
        NvOsDebugPrintf("NVMEM_IOC_PARAM failed: %s\n", strerror(errno));
        return (NvRmHeap) 0;
    }

    if (op.result & NVMEM_HEAP_SYSMEM)
        return NvRmHeap_External;
    else if (op.result & NVMEM_HEAP_IOVMM)
        return NvRmHeap_GART;
    else if (op.result & NVMEM_HEAP_CARVEOUT_IRAM)
        return NvRmHeap_IRam;
    else
        return NvRmHeap_ExternalCarveOut;
}

void NvMapMemSetFileId(int fd)
{
    s_nvmapfd = fd;
}
