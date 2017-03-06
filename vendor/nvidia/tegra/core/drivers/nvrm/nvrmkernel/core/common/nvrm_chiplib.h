/*
 * Copyright 2007 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_NVRM_CHIPLIB_H
#define INLCUDED_NVRM_CHIPLIB_H

#include "nvcommon.h"
#include "nvrm_hardware_access.h"

#ifdef __cplusplus
extern "C"
{
#endif  /* __cplusplus */

/**
 * Chiplib interrupt handler function.
 */
typedef void (* ChiplibHandleInterrupt)( void );

#if NV_DEF_ENVIRONMENT_SUPPORTS_SIM == 1
NvBool NvRmIsSimulation(void);
#else
#define NvRmIsSimulation() NV_FALSE
#endif

/**
 * starts chiplib.
 *
 * @param lib The chiplib name
 * @param cmdline The chiplib command line
 * @param handle The interrupt handler - will be called by chiplib
 */
NvError
NvRmPrivChiplibStartup( const char *lib, const char *cmdline,
    ChiplibHandleInterrupt handler );

/**
 * stops chiplib.
 */
void
NvRmPrivChiplibShutdown( void );

/**
 * maps a bogus virtual address to a physical address.
 *
 * @param addr The physical address to map
 * @param size The size of the mapping
 */
void *
NvRmPrivChiplibMap( NvRmPhysAddr addr, size_t size );

/**
 * unmaps a previously mapped pointer from NvRmPrivChiplibMap.
 *
 * @param addr The virtual address to unmap
 */
void
NvRmPrivChiplibUnmap( void *addr );


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
