/*
 * Copyright 2007 - 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_NVRM_HARDWARE_ACCESS_H
#define INCLUDED_NVRM_HARDWARE_ACCESS_H

#include "nvcommon.h"
#include "nvrm_init.h"
#include "nvos.h"

#ifdef __cplusplus
extern "C"
{
#endif  /* __cplusplus */


// By default, sim is supported on WinXP/x86 and Linux/x86 builds only.
#if !defined(NV_DEF_ENVIRONMENT_SUPPORTS_SIM)
#if NVCPU_IS_X86 && (NVOS_IS_WINDOWS || NVOS_IS_LINUX)
#define NV_DEF_ENVIRONMENT_SUPPORTS_SIM 1
#else
#define NV_DEF_ENVIRONMENT_SUPPORTS_SIM 0
#endif
#endif

/**
 * NV_WRITE* and NV_READ* - low level read/write api to hardware.
 *
 * These macros should be used to read and write registers and memory
 * in NvDDKs so that the DDK will work on simulation and real hardware
 * with no changes.
 *
 * This is for hardware modules that are NOT behind the host.  Modules that
 * are behind the host should use nvrm_channel.h.
 *
 * A DDK can obtain a mapping to its registers by using the
 * NvRmPhysicalMemMap() function.  This mapping is always uncached.  The
 * resulting pointer can then be used with NV_READ and NV_WRITE.
 */

/*
 * Maps the given physical address to the user's virtual address space.
 *
 * @param phys The physical address to map into the virtual address space
 * @param size The size of the mapping
 * @param flags Any flags for the mapping -- exactly match's NVOS_MAP_*
 * @param memType The memory mapping to use (uncached, write-combined, etc.)
 * @param ptr Output -- the resulting virtual pointer
 */
// FIXME: NvOs needs to take this up, however I think this is more
// complex than just mapping.  E.G. does it map into the kernel vaddr, or
// the current process vaddr? And how does this work on windows and
// windows-ce?
NvError NvRmPhysicalMemMap(NvRmPhysAddr phys, size_t size, NvU32 flags,
    NvOsMemAttribute memType, void **ptr );

/*
 * Unmaps the given virtual address from NvRmPhysicalMemMap.
 */
void NvRmPhysicalMemUnmap(void *ptr, size_t size);

/**
 * NV_WRITE[8|16|32|64] - Writes N data bits to hardware.
 *
 *   @param a The address to write.
 *   @param d The data to write.
 */

/**
 * NV_READ[8|16|32|64] - Reads N bits from hardware.
 *
 * @param a The address to read.
 */

void NvWrite08(void *addr, NvU8 data);
void NvWrite16(void *addr, NvU16 data);
void NvWrite32(void *addr, NvU32 data);
void NvWrite64(void *addr, NvU64 data);
NvU8 NvRead08(void *addr);
NvU16 NvRead16(void *addr);
NvU32 NvRead32(void *addr);
NvU64 NvRead64(void *addr);
void NvWriteBlk(void *dst, const void *src, NvU32 length);
void NvReadBlk(void *dst, const void *src, NvU32 length);

#if NV_DEF_ENVIRONMENT_SUPPORTS_SIM == 1

#define NV_WRITE08(a,d)     NvWrite08((void *)(a),(d))
#define NV_WRITE16(a,d)     NvWrite16((void *)(a),(d))
#define NV_WRITE32(a,d)     NvWrite32((void *)(a),(d))
#define NV_WRITE64(a,d)     NvWrite64((void *)(a),(d))
#define NV_READ8(a)         NvRead08((void *)(a))
#define NV_READ16(a)        NvRead16((void *)(a))
#define NV_READ32(a)        NvRead32((void *)(a))
#define NV_READ64(a)        NvRead64((void *)(a))
#define NV_WRITE(dst, src, len) NvWriteBlk(dst, src, len)
#define NV_READ(dst, src, len)  NvReadBlk(dst, src, len)

#else
/* connected to hardware */

#define NV_WRITE08(a,d)     *((volatile NvU8 *)(a)) = (d)
#define NV_WRITE16(a,d)     *((volatile NvU16 *)(a)) = (d)
#define NV_WRITE32(a,d)     *((volatile NvU32 *)(a)) = (d)
#define NV_WRITE64(a,d)     *((volatile NvU64 *)(a)) = (d)
#define NV_READ8(a)         *((const volatile NvU8 *)(a))
#define NV_READ16(a)        *((const volatile NvU16 *)(a))
#define NV_READ32(a)        *((const volatile NvU32 *)(a))
#define NV_READ64(a)        *((const volatile NvU64 *)(a))
#define NV_WRITE(dst, src, len)  NvOsMemcpy(dst, src, len)
#define NV_READ(dst, src, len)   NvOsMemcpy(dst, src, len)

#endif // !hardware

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // INCLUDED_NVRM_HARDWARE_ACCESS_H
