/*
 * Copyright (c) 2009 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include <windows.h>
#include "nvcommon.h"
#include "nvassert.h"
#include "nvos.h"
#include "nvidlcmd.h"
#include "nvrm_memmgr.h"
#include "nvrm_power.h"
#include "nvrm_rmctrace.h"
#include "nvrm_ioctls.h"
#include "nvrm_memmgr_private.h"
#include "nvrm_hwintf.h"
#include "nvrm_power_private.h"
#include "nvrm_interrupt.h"

typedef struct NvRmPrivCarveoutInfoRec
{
    NvU32 PhysBase;
    void  *pVirtBase;
    NvU32 CarveoutSize;
} NvRmPrivCarveoutInfo;

NvOsFileHandle s_DeviceFile;
static NvRmPrivCarveoutInfo gs_CarveoutInfo;

NvBool NvRmPrivGetCarveoutInfo(void);

BOOL WINAPI DllMain(HANDLE hinstDLL, 
                    DWORD dwReason, 
                    LPVOID lpvReserved)
{
    switch (dwReason) {
    case DLL_PROCESS_ATTACH: 
        DisableThreadLibraryCalls(hinstDLL);

        if (NvOsFopen("NRM1:", NVOS_OPEN_READ | NVOS_OPEN_WRITE,
            &s_DeviceFile) != NvSuccess)
        {
            return FALSE;
        }
        if( NvRmPrivGetCarveoutInfo() == NV_FALSE )
        {
            return NV_FALSE;
        }

        break;

    case DLL_PROCESS_DETACH: 
        NvOsFclose(s_DeviceFile);
        s_DeviceFile = NULL;
        break;

    case DLL_THREAD_DETACH:
    case DLL_THREAD_ATTACH:
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

int NvRm_MemmgrGetIoctlFile(void)
{
    NV_ASSERT(!"Not supported");
    return 0;
}

static NvError DoNvRmMemMapIntoCallerPtr(
    NvRmMemHandle  hMem,
    const NvU32    Offset,
    const NvU32    PhysSize,
    const NvU32    PhysAddress, // this is the size of the virtual space to
                                // allocate
    NvU32          Flags,
    void          **pVirtAddr)
{
    NvU32 args[5];
    void  *pNewVirtSpace = NULL;
    NvU32 TotalLength;
    NvU32 PageOffset;         // offset within the first page
    NvU32 TmpFlags;
    NvError err = NvSuccess;

    // This is very confusing, so let me try to explain.
    // The mapping is 2 parts.  First we allocate a number of virtual pages.
    // TotalLength, is the number of pages * the page size.
    // PageOffset is the byte offset into the first page the user has requested

    // offset in the first page.
    PageOffset = (PhysAddress + Offset) & (NVCPU_MIN_PAGE_SIZE-1);

    // adjust size up to a multiple of pages
    TotalLength = (PhysSize + PageOffset + (NVCPU_MIN_PAGE_SIZE-1) ) &
        ~(NVCPU_MIN_PAGE_SIZE-1);

    /* either have no permissions, or global addr */ 
    TmpFlags = NVOS_MEM_NONE;
    if (Flags & NVOS_MEM_GLOBAL_ADDR )
        TmpFlags |= NVOS_MEM_GLOBAL_ADDR;

    // Allocate virtual memory.
    err = NvOsPhysicalMemMap(0, TotalLength, NvOsMemAttribute_Uncached, 
       TmpFlags, &pNewVirtSpace);
    if (err != NvSuccess)
    {
        pNewVirtSpace = NULL;
        goto exit_gracefully;
    }

    // FIXME: MapIntoCaller should take the Flags argument!
    args[0] = (NvU32)hMem;
    args[1] = (NvU32)pNewVirtSpace;
    args[2] = Offset;
    args[3] = TotalLength;

    err = NvOsIoctl(NvRm_NvIdlGetIoctlFile(),
            NvRmIoctls_NvRmMemMapIntoCallerPtr, args, sizeof(args[0]) * 4,
            0, sizeof(args[4]));
    if (err != NvSuccess)
        goto exit_gracefully;

    err = args[4];
    if (err == NvSuccess)
    {
        // adjust up the pointer
        pNewVirtSpace = (void *)( ((NvUPtr)pNewVirtSpace) + PageOffset );
    }

exit_gracefully:
    if (err != NvSuccess)
    {
        NvOsPhysicalMemUnmap(pNewVirtSpace, TotalLength);
        pNewVirtSpace = NULL;
    }
    *pVirtAddr = pNewVirtSpace;
    return err;
}

NvBool NvRmPrivGetCarveoutInfo(void)
{
    NvU32 args[3];
    void *ptr = NULL;

    NvOsIoctl(NvRm_NvIdlGetIoctlFile(), NvRmIoctls_NvRmGetCarveoutInfo,
       args, 0, 0, 3 * sizeof(NvU32) );

    /* Windows CE maps all of carveout into the process' virtual address
     * space when the RM stub is opened, primarily to work-around shortcomings
     * of the CE5 (Windows Mobile 6.1 / 6.5) kernel's 32MB / process
     * limitation by mapping the carveout into the 1GB global space.  Linux
     * doesn't have this limitation, and calling an NvOs kernel function
     * from inside the RM stub's _init section is a bad idea. */
    ptr = (void *)args[1];

    gs_CarveoutInfo.PhysBase = args[0];
    gs_CarveoutInfo.pVirtBase = ptr;
    gs_CarveoutInfo.CarveoutSize = args[2];

    return NV_TRUE;
}

NvError NvRmMemMap(
    NvRmMemHandle  hMem,
    NvU32          Offset,
    NvU32          Size,
    NvU32          Flags,
    void          **pVirtAddr)
{
    NvU32 address;
    NvRmHeap heap;
    NvError e;

    heap = NvRmMemGetHeapType(hMem, &address);

    if (heap == NvRmHeap_IRam)
    {
        return NvError_NotSupported;
    }

    // check if this region is in the shared space.
    if ( (address+Offset) >= gs_CarveoutInfo.PhysBase &&
         (address+Offset) < (gs_CarveoutInfo.PhysBase +
                             gs_CarveoutInfo.CarveoutSize))
    {
        NvU8 *pSharedPtr = gs_CarveoutInfo.pVirtBase;
        pSharedPtr += ((address+Offset) - gs_CarveoutInfo.PhysBase);
        *pVirtAddr = pSharedPtr;

        return NvSuccess;
    }

    e = DoNvRmMemMapIntoCallerPtr(hMem, Offset, Size, address, Flags,
            pVirtAddr);

    return e;
}

void NvRmMemUnmap(NvRmMemHandle hMem, void *pVirtAddr, NvU32 size)
{
    NvU8   *pSharedPtr = gs_CarveoutInfo.pVirtBase;

    // check if this region is in the shared space.
    if ( (NvU8*)pVirtAddr >= pSharedPtr &&
         (NvU8*)pVirtAddr < (pSharedPtr + gs_CarveoutInfo.CarveoutSize))
    {
        return;  // nothing to do
    }

    /* NvOs implementations handle VA alignment internally, just pass back
     * the pointer that was returned by PhysicalMemMap */
    NvOsPhysicalMemUnmap(pVirtAddr, size);

    return;
}

void NvRmMemCacheMaint(
    NvRmMemHandle hMem,
    void         *pMapping,
    NvU32         Size,
    NvBool        Writeback,
    NvBool        Invalidate) {

    if (!(Writeback || Invalidate))
        return;

    if (Writeback && Invalidate)
        NvOsDataCacheWritebackInvalidateRange(pMapping, Size);
    else if (Writeback)
        NvOsDataCacheWritebackRange(pMapping, Size);
    else
    {
        NV_ASSERT(!"No support for invalidate-only on WinCE");
    }   
}


/** Custom ioctl marshaler to make these functions usably fast on ce */
void NvRmMemRead(NvRmMemHandle hMem, NvU32 Offset, void *pDst, NvU32 Size)
{
    NvU32 args[4];

    args[0] = (NvU32)hMem;
    args[1] = Offset;
    args[2] = (NvU32)pDst;
    args[3] = Size;

    NvOsIoctl(NvRm_NvIdlGetIoctlFile(),
              NvRmIoctls_NvRmMemRead,
              args,
              sizeof(args),
              0,
              0 );    
}

void NvRmMemWrite(NvRmMemHandle hMem,NvU32 Offset,const void *pSrc, NvU32 Size)
{
    NvU32 args[4];

    args[0] = (NvU32)hMem;
    args[1] = Offset;
    args[2] = (NvU32)pSrc;
    args[3] = Size;

    NvOsIoctl(NvRm_NvIdlGetIoctlFile(),
              NvRmIoctls_NvRmMemWrite,
              args,
              sizeof(args),
              0,
              0);
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
    NvU32 args[7];

    args[0] = (NvU32)hMem;
    args[1] = Offset;
    args[2] = SrcStride;
    args[3] = (NvU32)pDst;
    args[4] = DstStride;
    args[5] = ElementSize;
    args[6] = Count;

    NvOsIoctl(NvRm_NvIdlGetIoctlFile(),
              NvRmIoctls_NvRmMemReadStrided,
              args,
              sizeof(args),
              0,
              0);
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
    NvU32 args[7];

    args[0] = (NvU32)hMem;
    args[1] = Offset;
    args[2] = DstStride;
    args[3] = (NvU32)pSrc;
    args[4] = SrcStride;
    args[5] = ElementSize;
    args[6] = Count;

    NvOsIoctl(NvRm_NvIdlGetIoctlFile(),
              NvRmIoctls_NvRmMemWriteStrided,
              args,
              sizeof(args),
              0,
              0);
}
