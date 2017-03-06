//
// Copyright (c) 2007-2009 NVIDIA Corporation.  All Rights Reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation.  Any
// use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation
// is strictly prohibited.
//

/**
 * @file
 * <b>NVIDIA Tegra Boot Loader API</b>
 *
 */

#ifndef INCLUDED_NVBL_CACHE_H
#define INCLUDED_NVBL_CACHE_H

/**
 * @defgroup nvbl_cache_group NvBL Cache API
 *
 *
 *
 * @ingroup nvbl_group
 * @{
 */

#include "nvbl_common.h"

#ifdef  __cplusplus
extern "C" {
#endif

// Dcache size
#define NV_BL_D_CACHE_SIZE          0x8000
// Cache line size
#define NV_BL_CACHE_LINE_SIZE       32

//==========================================================================
/**  @name Cache L1 (Coprocessor) Functions
 */
/*@{*/

/**
 * @brief   Enables/disables the ARM L1 data cache.
 * @pre     Must be called from a privileged processor mode.
 * @param   enable Specify NV_TRUE to enable the cache, NV_FALSE to disable.
 * @returns The previous state of ARM L1 data cache.
 */
NvBool  NvBlEnableDataCache(NvBool enable);


/**
 * @brief   Enables/disables ARM L1 instruction cache.
 * @pre     Must be called from a privileged processor mode.
 * @param   enable Specify NV_TRUE to enable the cache, NV_FALSE to disable.
 * @returns The previous state of ARM L1 instruction cache.
 */
NvBool  NvBlEnableInstructionCache(NvBool enable);


/**
 * @brief   Enables/disables broadcast ARM L1 cache operations.
 * @pre     Must be called from a privileged processor mode.
 * @note    This function has no effect in single CPU configurations.
 * @param   enable Specify NV_TRUE to enable broadcast cache operations,
 * NV_FALSE to disable.
 * @returns The previous state of ARM L1 broadcast cache operations.
 */
NvBool  NvBlEnableBroadcastCacheOps(NvBool enable);


/**
 * @brief   Invalidates the L1 instruction cache.
 * @pre     Must be called from a privileged processor mode.
 */
void    NvBlInvalidateInstructionCache(void);


/**
 * @brief   Invalidates all caches, L1 data, L1 instruction, and
 *          branch target address cache (BCAT).
 * @pre     Must be called from a privileged processor mode.
 * @pre     Must not be called if the L1 data cache is enabled.
 */
void    NvBlInvalidateAllCaches(void);


/**
 * @brief   Cleans (writes back) the L1 data cache.
 * @pre     Must be called from a privileged processor mode.
 */
void    NvBlCleanDataCache(void);


/**
 * @brief   Invalidates the L1 data cache.
 * @pre     Must be called from a privileged processor mode.
 * @pre     Must not be called if the L1 data cache is enabled.
*/
void    NvBlInvalidateDataCache(void);


/**
 * @brief   Flushes and invalidates the L1 data cache.
 * @pre     Must be called from a privileged processor mode.
 */
void    NvBlCleanAndInvalidateDataCache(void);


/**
 * @brief   Performs data synchronization barrier cache operation.
 */
void    NvBlDataSynchronizationBarrier(void);


/**
 * @brief   Performs data memory barrier cache operation.
 */
void    NvBlDataMemoryBarrier(void);


/**
 * @brief   Flushes branch branch target address cache (BTAC).
 * @pre     Must be called from a privileged processor mode.
 */
void    NvBlFlushBranchTargetCache(void);


/**
 * @brief   Writes back and invalidates a range of addresses L1 data cache
 * @pre     Must be called from a privileged processor mode.
 * @param   start Starting address range.
 * @param   TotalLength  The number of bytes to invalidate.
 */
void    NvBlDataCacheWritebackInvalidateRange(void* start, NvU32 TotalLength);

/**
 * @brief   Writes back a range of addresses L1 data cache.
 * @pre     Must be called from a privileged processor mode.
 * @param   start A pointer to the starting address range.
 * @param   TotalLength  The number of bytes to invalidate.
 */
void    NvBlDataCacheWritebackRange(void* start, NvU32 TotalLength);

/*@}*/


//==========================================================================
// Cache L2 Functions
//==========================================================================

/**
 * Defines L2 cache flags.
 */
typedef enum
{
    /// Specifies no L2 cache attribute flags are set.
    NvBlL2CacheFlags_None = 0x000000000,

    /// Specifies Unified (set) vs. Harvard (clear) L2 cache.
    NvBlL2CacheFlags_Unified = 0x000000001,

    /// Specifies a Write Through (set) vs. Write Back (clear) L2 cache.
    NvBlL2CacheFlags_WriteThrough = 0x000000002,

    /// Ignore -- Forces compilers to make 32-bit enums.
    NvBlL2CacheFlags__Force32 = 0x7FFFFFFF

} NvBlL2CacheFlags;

/**
 * Holds L2 cache attributes.
 */
typedef struct  NvBlL2CacheAttributesRec
{
    /// Holds the L2 cache flags.
    NvBlL2CacheFlags Flags;

    /// Holds the L2 instruction cache bytes per cache line.
    NvU32   ILineSize;

    /// Holds the L2 instruction cache number of cache ways.
    NvU32   IWays;

    /// Holds the L2 instruction cache total cache size.
    NvU32   ISize;

    /// Holds the L2 instruction cache sets per way.
    NvU32   ISetsPerWay;

    /// Holds the L2 data cache bytes per cache line.
    NvU32   DLineSize;

    /// Holds the L2 data cache number of cache ways.
    NvU32   DWays;

    /// Holds the L2 data cache total cache size.
    NvU32   DSize;

    /// Holds the L2 data cache sets per way.
    NvU32   DSetsPerWay;

} NvBlL2CacheAttributes;

//==========================================================================
/**  @name Cache L2 Functions
 */
/*@{*/

/**
 * @brief   Tests if L2 cache is enabled.
 * @returns  NV_TRUE if L2 cache is present and enabled, NV_FALSE if not present or disabled.
 */
NvBool    NvBlL2CacheIsEnabled(void);

/**
 * @brief   Tests if L2 cache is present.
 * @returns  NV_TRUE if L2 cache is present, NV_FALSE if not present.
 */
NvBool    NvBlL2CacheIsPresent(void);

/**
 * @brief   Initializes L2 cache controller and invalidates the cache contents.
 * If an L2 cache is not present in the SOC, this function is effectively a
 * no-operation.
 */
void    NvBlL2CacheInit(void);

/**
 * @brief   Enables the L2 Cache. If an L2 cache is not present in the SOC,
 * this function is effectively a no-operation.
 * @param   enable  Specify NV_TRUE to enable the cache, NV_FALSE to disable.
 * @returns  NV_TRUE if L2 cache is enabled, NV_FALSE if disabled.
 */
NvBool  NvBlL2CacheEnable(NvBool enable);

/**
 * @brief   Retrieves the L2 cache attributes. If an L2 cache is not present
 * in the SOC, the attributes structure is cleared.
 * @param   pAttributes A pointer to the output structure to receive
 * the cache attributes.
 */
void    NvBlL2CacheGetAttributes(NvBlL2CacheAttributes* pAttributes);

/**
 * @brief   Invalidates the entire L2 cache without writing back. If an L2
 * cache is not present in the SOC, this function is effectively a
 * no-operation.
 */
void    NvBlL2CacheInvalidate(void);

/**
 * @brief   Writes back (clean) the entire L2 cache without invalidating.
 * If an L2 cache is not present in the SOC, this function is effectively a
 * no-operation.
 */
void    NvBlL2CacheWriteback(void);

/**
 * @brief   Writes back (clean) and invalidates the entire L2 cache. If an L2
 * cache is not present in the SOC, this function is effectively a
 * no-operation.
 */
void    NvBlL2CacheWritebackInvalidate(void);

/**
 * @brief   Invalidates without writing back a range of addresses. If an L2
 * cache is not present in the SOC, this function is effectively a
 * no-operation. If the MMU is enabled, any part of the range that is
 * not mapped by a valid page table entry will be ignored.
 * @param   Start The starting address.
 * @param   TotalLength  The number of bytes to invalidate.
 */
void    NvBlL2CacheInvalidateRange(void* Start, NvU32 TotalLength);

/**
 * @brief   Writes back but does not invalidate a range of addresses. If an L2
 * cache is not present in the SOC, this function is effectively a
 * no-operation. If the MMU is enabled, any part of the range that is
 * not mapped by a valid page table entry will be ignored.
 * @param   Start The starting address.
 * @param   TotalLength  The number of bytes.
 */
void    NvBlL2CacheWritebackRange(void* Start, NvU32 TotalLength);

/**
 * @brief   Writes back and invalidates a range of addresses. If an L2
 * cache is not present in the SOC, this function is effectively a
 * no-operation. If the MMU is enabled, any part of the range that is
 * not mapped by a valid page table entry will be ignored.
 * @param   Start The starting address.
 * @param   TotalLength  The number of bytes.
 */
void    NvBlL2CacheWritebackInvalidateRange(void* Start, NvU32 TotalLength);
/*@}*/


#ifdef  __cplusplus
}
#endif

/** @} */

#endif // INCLUDED_NVBL_CACHE_H
