/*
 * Copyright (c) 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef NVRM_STUB_HELPER_LINUX_H
#define NVRM_STUB_HELPER_LINUX_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct {
    NvError
    (*HandleCreate)(
        NvRmDeviceHandle hRm,
        NvRmMemHandle *hMem,
        NvU32 Size);
    NvError
    (*HandleFromId)(
        NvU32 Id,
        NvRmMemHandle *hMem);
    NvError
    (*AllocInternalTagged)(
        NvRmMemHandle hMem,
        const NvRmHeap *Heaps,
        NvU32 NumHeaps,
        NvU32 Alignment,
        NvOsMemAttribute Coherency,
        NvU16 Tags);
    void (*HandleFree)(NvRmMemHandle hMem);
    NvU32 (*GetId)(NvRmMemHandle hMem);
    void (*UnpinMult)(NvRmMemHandle *hMems, NvU32 Count);
    void (*PinMult)(NvRmMemHandle *hMems, NvU32 *addrs, NvU32 Count);
    NvError
    (*Map)(
        NvRmMemHandle hMem,
        NvU32         Offset,
        NvU32         Size,
        NvU32         Flags,
        void        **pVirtAddr);
    void (*Unmap)(NvRmMemHandle hMem, void *pVirtAddr, NvU32 Size);
    void
    (*CacheMaint)(
        NvRmMemHandle hMem,
        void         *pMapping,
        NvU32         Size,
        NvBool        Writeback,
        NvBool        Invalidate);
    void
    (*ReadStrided)(
        NvRmMemHandle hMem,
        NvU32 Offset,
        NvU32 SrcStride,
        void *pDst,
        NvU32 DstStride,
        NvU32 ElementSize,
        NvU32 Count);
    void
    (*WriteStrided)(
        NvRmMemHandle hMem,
        NvU32 Offset,
        NvU32 DstStride,
        const void *pSrc,
        NvU32 SrcStride,
        NvU32 ElementSize,
        NvU32 Count);
    NvU32 (*GetSize)(NvRmMemHandle hMem);
    NvU32 (*GetAlignment)(NvRmMemHandle hMem);
    NvU32 (*GetAddress)(NvRmMemHandle hMem, NvU32 Offset);
    NvRmHeap (*GetHeapType)(NvRmMemHandle hMem, NvU32 *BasePhysAddr);
} NvRmPrivMemManager;

extern unsigned int g_NvRmMemCameraHeapUsage;
extern const char *g_NvRmMemCameraUsageName;

NvError
NvMapMemHandleCreate(
    NvRmDeviceHandle hRm,
    NvRmMemHandle *hMem,
    NvU32 Size);

NvError NvMapMemHandleFromId(NvU32 Id, NvRmMemHandle *hMem);

NvError
NvMapMemAllocInternalTagged(
    NvRmMemHandle hMem,
    const NvRmHeap *Heaps,
    NvU32 NumHeaps,
    NvU32 Alignment,
    NvOsMemAttribute Coherency,
    NvU16 Tags);

void NvMapMemHandleFree(NvRmMemHandle hMem);

NvU32 NvMapMemGetId(NvRmMemHandle hMem);

void NvMapMemUnpinMult(NvRmMemHandle *hMems, NvU32 Count);

void NvMapMemPinMult(NvRmMemHandle *hMems, NvU32 *addrs, NvU32 Count);

NvError NvMapMemMap(
    NvRmMemHandle hMem,
    NvU32         Offset,
    NvU32         Size,
    NvU32         Flags,
    void        **pVirtAddr);

void NvMapMemCacheMaint(
    NvRmMemHandle hMem,
    void         *pMapping,
    NvU32         Size,
    NvBool        Writeback,
    NvBool        Invalidate);

void NvMapMemUnmap(NvRmMemHandle hMem, void *pVirtAddr, NvU32 Size);

void NvMapMemReadStrided(
    NvRmMemHandle hMem,
    NvU32 Offset,
    NvU32 SrcStride,
    void *pDst,
    NvU32 DstStride,
    NvU32 ElementSize,
    NvU32 Count);

void NvMapMemWriteStrided(
    NvRmMemHandle hMem,
    NvU32 Offset,
    NvU32 DstStride,
    const void *pSrc,
    NvU32 SrcStride,
    NvU32 ElementSize,
    NvU32 Count);

NvU32 NvMapMemGetSize(NvRmMemHandle hMem);

NvU32 NvMapMemGetAlignment(NvRmMemHandle hMem);

NvU32 NvMapMemGetAddress(NvRmMemHandle hMem, NvU32 Offset);

NvRmHeap NvMapMemGetHeapType(NvRmMemHandle hMem, NvU32 *BasePhysAddr);

void NvMapMemSetFileId(int fd);

/* Ion support*/
NvError IonMemHandleCreate(
    NvRmDeviceHandle hRm,
    NvRmMemHandle *hMem,
    NvU32 Size);

NvError IonMemHandleFromId(NvU32 Id, NvRmMemHandle *hMem);

NvError IonMemAllocInternalTagged(
    NvRmMemHandle hMem,
    const NvRmHeap *Heaps,
    NvU32 NumHeaps,
    NvU32 Alignment,
    NvOsMemAttribute Coherency,
    NvU16 Tags);

void IonMemHandleFree(NvRmMemHandle hMem);

NvU32 IonMemGetId(NvRmMemHandle hMem);

void IonMemUnpinMult(NvRmMemHandle *hMems, NvU32 Count);

void IonMemPinMult(NvRmMemHandle *hMems, NvU32 *addrs, NvU32 Count);

NvError IonMemMap(
    NvRmMemHandle hMem,
    NvU32         Offset,
    NvU32         Size,
    NvU32         Flags,
    void        **pVirtAddr);

void IonMemCacheMaint(
    NvRmMemHandle hMem,
    void         *pMapping,
    NvU32         Size,
    NvBool        Writeback,
    NvBool        Invalidate);

void IonMemUnmap(NvRmMemHandle hMem, void *pVirtAddr, NvU32 Size);

void IonMemReadStrided(
    NvRmMemHandle hMem,
    NvU32 Offset,
    NvU32 SrcStride,
    void *pDst,
    NvU32 DstStride,
    NvU32 ElementSize,
    NvU32 Count);

void IonMemWriteStrided(
    NvRmMemHandle hMem,
    NvU32 Offset,
    NvU32 DstStride,
    const void *pSrc,
    NvU32 SrcStride,
    NvU32 ElementSize,
    NvU32 Count);

NvU32 IonMemGetSize(NvRmMemHandle hMem);

NvU32 IonMemGetAlignment(NvRmMemHandle hMem);

NvU32 IonMemGetAddress(NvRmMemHandle hMem, NvU32 Offset);

NvRmHeap IonMemGetHeapType(NvRmMemHandle hMem, NvU32 *BasePhysAddr);

void IonMemSetFileId(int fd);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
