/*
 * Copyright (c) 2009 - 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "nvcommon.h"
#include "nvassert.h"
#include "nvos.h"
#include "nvidlcmd.h"
#include "nvutil.h"
#include "nvrm_memmgr.h"
#include "nvrm_power.h"
#include "nvrm_rmctrace.h"
#include "nvrm_ioctls.h"
#include "nvrm_memmgr.h"
#include "linux/nvmem_ioctl.h"
#include "nvrm_stub_helper_linux.h"

#if defined(HAVE_X11)
#include <X11/Xlib.h>
#include "tdrlib.h"
#endif

static NvRmPrivMemManager s_MM;

// This asks a possible X server to free up any optional allocations
static void NvRm_MemmgrFreeServerResources(void)
{
#if defined(HAVE_X11)
    Display* dpy;

    dpy = XOpenDisplay(":0");
    if (!dpy)
    {
        return;
    }
    tdrFreeResources(dpy, 0);
    XSync(dpy, False);
    XCloseDisplay(dpy);
#endif
}

extern NvError NvRpcStubInit(void);
extern void NvRpcStubDeInit(void);

/* Flag to control verbosity of failed operations.
 *
 * Default is to print on debug builds but not on release builds and
 * if configuration setting (NVRM_MEMMGR_VERBOSITY_KEY) exists then its value
 * overrides the default setting. */
static NvBool s_memMgrVerbose = NV_FALSE;
static int s_nvmapfd = -1;
unsigned int g_NvRmMemCameraHeapUsage =  NVMEM_HEAP_CARVEOUT_GENERIC;
const char *g_NvRmMemCameraUsageName =
    "/sys/devices/virtual/misc/nvmap/heap-camera/usage";

NV_CT_ASSERT(sizeof(unsigned long)==sizeof(NvU32));
NV_CT_ASSERT(sizeof(NvRmMemHandle)==sizeof(NvU32));

int NvRm_MemmgrGetIoctlFile(void)
{
    NV_ASSERT(s_nvmapfd >= 0);
    return s_nvmapfd;
}

NvError NvRmMemHandleCreate(
    NvRmDeviceHandle hRm,
    NvRmMemHandle *hMem,
    NvU32 Size)
{
    return s_MM.HandleCreate(hRm, hMem, Size);
}

NvError NvRmMemHandleFromId(NvU32 Id, NvRmMemHandle *hMem)
{
    return s_MM.HandleFromId(Id, hMem);
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
    return NvError_NotSupported;
}

NvError NvRmMemAllocInternal(
    NvRmMemHandle hMem,
    const NvRmHeap *Heaps,
    NvU32 NumHeaps,
    NvU32 Alignment,
    NvOsMemAttribute Coherency)
{
    return NvRmMemAllocInternalTagged(hMem, Heaps, NumHeaps,
            Alignment, Coherency, NVRM_MEM_TAG_NONE);
}

NvError NvRmMemAllocInternalTagged(
    NvRmMemHandle hMem,
    const NvRmHeap *Heaps,
    NvU32 NumHeaps,
    NvU32 Alignment,
    NvOsMemAttribute Coherency,
    NvU16 Tags)
{
    return s_MM.AllocInternalTagged(
        hMem, Heaps, NumHeaps,
        Alignment, Coherency, Tags);
}

NvError NvRmMemAlloc(
    NvRmMemHandle hMem,
    const NvRmHeap *Heaps,
    NvU32 NumHeaps,
    NvU32 Alignment,
    NvOsMemAttribute Coherency)
{
    return NvRmMemAllocTagged(hMem, Heaps, NumHeaps,
            Alignment, Coherency, NVRM_MEM_TAG_NONE);
}

NvError NvRmMemAllocTagged(
    NvRmMemHandle hMem,
    const NvRmHeap *Heaps,
    NvU32 NumHeaps,
    NvU32 Alignment,
    NvOsMemAttribute Coherency,
    NvU16 Tags)
{
    NvError Ret;

    Ret = NvRmMemAllocInternalTagged(hMem, Heaps, NumHeaps,
            Alignment, Coherency, Tags);

    if (Ret == NvError_InsufficientMemory) {
        // If out of memory, retry once after asking the X server for memory.
        NvRm_MemmgrFreeServerResources();
        Ret = NvRmMemAllocInternalTagged(hMem, Heaps, NumHeaps, Alignment,
                                         Coherency, Tags);
    }

    if (Ret != NvSuccess && s_memMgrVerbose) {
        NvOsDebugPrintf("NvRmMemAlloc FAIL: NvError=0x%08x, size=%u, "
                        "NumHeaps=%d, Alignment=%u, Coherency=0x%x, Tags=%d\n",
                        Ret, NvRmMemGetSize(hMem),
                        NumHeaps, Alignment, Coherency, Tags);
    }

    return Ret;
}

void NvRmMemHandleFree(NvRmMemHandle hMem)
{
    s_MM.HandleFree(hMem);
}

NvU32 NvRmMemGetId(NvRmMemHandle hMem)
{
    return s_MM.GetId(hMem);
}

void NvRmMemUnpinMult(NvRmMemHandle *hMems, NvU32 Count)
{
    s_MM.UnpinMult(hMems, Count);
}

void NvRmMemPinMult(NvRmMemHandle *hMems, NvU32 *addrs, NvU32 Count)
{
    s_MM.PinMult(hMems, addrs, Count);
}

NvU32 NvRmMemPin(NvRmMemHandle hMem)
{
    NvU32 addr = ~0;
    NvRmMemPinMult(&hMem, &addr, 1);

#if 0 // NV_DEBUG - FIXME THIS ALWAYS ASSERTS AND KILLS THE RM DAEMON ON T30 !!!
    {
        NvU32 baseAddr;
        // GART memory should never be pinned from userspace
        NV_ASSERT( NvRmMemGetHeapType(hMem, &baseAddr) != NvRmHeap_GART );
    }
#endif
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
    return s_MM.Map(hMem, Offset, Size, Flags, pVirtAddr);
}

void NvRmMemCacheMaint(
    NvRmMemHandle hMem,
    void         *pMapping,
    NvU32         Size,
    NvBool        Writeback,
    NvBool        Invalidate)
{
    s_MM.CacheMaint(hMem, pMapping, Size,
        Writeback, Invalidate);
}

void NvRmMemUnmap(NvRmMemHandle hMem, void *pVirtAddr, NvU32 Size)
{
    s_MM.Unmap(hMem, pVirtAddr, Size);
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
    s_MM.ReadStrided(hMem, Offset, SrcStride, pDst,
            DstStride, ElementSize, Count);
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
    s_MM.WriteStrided(hMem, Offset, DstStride, pSrc,
            SrcStride, ElementSize, Count);
}

NvU32 NvRmMemGetSize(NvRmMemHandle hMem)
{
    return s_MM.GetSize(hMem);
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
    return s_MM.GetAlignment(hMem);
}

NvU32 NvRmMemGetCacheLineSize(void)
{
    return 32;
}

NvU32 NvRmMemGetAddress(NvRmMemHandle hMem, NvU32 Offset)
{
    return s_MM.GetAddress(hMem, Offset);
}

NvRmHeap NvRmMemGetHeapType(NvRmMemHandle hMem, NvU32 *BasePhysAddr)
{
    return s_MM.GetHeapType(hMem, BasePhysAddr);
}

NvError NvRmMemGetStat(NvRmMemStat Stat, NvS32 *Result)
{
    /* FIXME: implement */
    return NvError_NotSupported;
}

/* This flag is defined in kernel's fcntl.h. But not in android's fcntl.h. */
#ifndef O_CLOEXEC
#define O_CLOEXEC 02000000
#endif
void __attribute__ ((constructor)) nvrm_init(void);
void __attribute__ ((constructor)) nvrm_init(void)
{
    NvError e = NvSuccess;
    int camera_fd;
    char camera_usage[16] = {0};

    /* check if a special camera heap exists; if so, use its usage mask
     * for the CameraHeapUsage mask, rather than just the generic heap */
    camera_fd = open(g_NvRmMemCameraUsageName, O_RDONLY);
    if (camera_fd >= 0)
    {
        int cnt = read(camera_fd, camera_usage, sizeof(camera_usage));
        if (cnt)
        {
            NvOsDebugPrintf("[nvrm] custom camera heap in use\n");
            g_NvRmMemCameraHeapUsage = NvUStrtoul(camera_usage, NULL, 16) |
                NVMEM_HEAP_CARVEOUT_GENERIC;
        }
        close(camera_fd);
    }

    s_nvmapfd = open("/dev/knvmap", O_RDWR | O_SYNC | O_CLOEXEC);
    if (s_nvmapfd<0)
        s_nvmapfd = open("/dev/nvmap", O_RDWR | O_SYNC | O_CLOEXEC);

    if (s_nvmapfd<0)
    {
        e = NvError_KernelDriverNotFound;
    }
    else
    {
        s_MM.HandleCreate = NvMapMemHandleCreate;
        s_MM.HandleFromId = NvMapMemHandleFromId;
        s_MM.AllocInternalTagged = NvMapMemAllocInternalTagged;
        s_MM.HandleFree = NvMapMemHandleFree;
        s_MM.GetId = NvMapMemGetId;
        s_MM.UnpinMult = NvMapMemUnpinMult;
        s_MM.PinMult = NvMapMemPinMult;
        s_MM.Map = NvMapMemMap;
        s_MM.Unmap = NvMapMemUnmap;
        s_MM.CacheMaint = NvMapMemCacheMaint;
        s_MM.ReadStrided = NvMapMemReadStrided;
        s_MM.WriteStrided = NvMapMemWriteStrided;
        s_MM.GetSize = NvMapMemGetSize;
        s_MM.GetAlignment = NvMapMemGetAlignment;
        s_MM.GetAddress = NvMapMemGetAddress;
        s_MM.GetHeapType = NvMapMemGetHeapType;
        NvMapMemSetFileId(s_nvmapfd);
    }

    if (e != NvSuccess)
    {
        s_nvmapfd = open("/dev/ion", O_RDWR | O_SYNC | O_CLOEXEC);
        if (s_nvmapfd<0)
        {
            e = NvError_KernelDriverNotFound;
            goto fail;
        }
        else
        {
            e = NvSuccess;
            s_MM.HandleCreate = IonMemHandleCreate;
            s_MM.HandleFromId = IonMemHandleFromId;
            s_MM.AllocInternalTagged = IonMemAllocInternalTagged;
            s_MM.HandleFree = IonMemHandleFree;
            s_MM.GetId = IonMemGetId;
            s_MM.UnpinMult = IonMemUnpinMult;
            s_MM.PinMult = IonMemPinMult;
            s_MM.Map = IonMemMap;
            s_MM.Unmap = IonMemUnmap;
            s_MM.CacheMaint = IonMemCacheMaint;
            s_MM.ReadStrided = IonMemReadStrided;
            s_MM.WriteStrided = IonMemWriteStrided;
            s_MM.GetSize = IonMemGetSize;
            s_MM.GetAlignment = IonMemGetAlignment;
            s_MM.GetAddress = IonMemGetAddress;
            s_MM.GetHeapType = IonMemGetHeapType;
            IonMemSetFileId(s_nvmapfd);
        }
    }

    if (NvRpcStubInit()!=NvSuccess)
        goto fail;


    /* Check verbosity flag setting. */
    {
        NvU32 verbose = 0;

        /* Set default value. */
        s_memMgrVerbose = NV_DEBUG ? NV_TRUE : NV_FALSE;

        /* Override with config value, if one exists. */
        if (NvOsGetConfigU32(NVRM_MEMMGR_VERBOSITY_KEY, &verbose) == NvSuccess)
            s_memMgrVerbose = verbose != 0;
    }

 fail:
    if (e != NvSuccess)
    {
        NvOsDebugPrintf("\n\n\n****nvrm_init failed****\n\n\n");
        if (s_nvmapfd >= 0)
        {
            close(s_nvmapfd);
            s_nvmapfd = -1;
        }
        NvRpcStubDeInit();
    }
}

void __attribute__ ((destructor)) nvrm_fini(void);
void __attribute__ ((destructor)) nvrm_fini(void)
{
    if (s_nvmapfd >= 0)
    {
        close(s_nvmapfd);
        s_nvmapfd = -1;
    }

    NvRpcStubDeInit();
}
