/**
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * nvboot_config.h provides constants that parameterize operations in the
 * Boot ROM.
 *
 * IMPORTANT: These constants can ONLY be changed during Boot ROM 
 *            development.  For a particular Tegra product, their
 *            values are frozen and provided here for reference by
 *            humans and bootloader code alike.
 */

#ifndef INCLUDED_NVBOOT_CONFIG_H
#define INCLUDED_NVBOOT_CONFIG_H

#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * Defines the maximum number of device parameter sets in the BCT.
 * The value must be equal to (1 << # of device straps)
 */
#define NVBOOT_BCT_MAX_PARAM_SETS      4

/**
 * Defines the maximum number of SDRAM parameter sets in the BCT.
 * The value must be equal to (1 << # of SDRAM straps)
 */
#define NVBOOT_BCT_MAX_SDRAM_SETS      4

/**
 * Defines the number of bytes provided for the CustomerData field in
 * the BCT.
 * Note that some of this data has been allocated by other NVIDIA tools
 * and components for their own use.  Please see TBD for further details.
 */
#define NVBOOT_BCT_CUSTOMER_DATA_SIZE 48

/**
 * Defines the number of bytes in the Reserved area of the BCT.
 */
#define NVBOOT_BCT_RESERVED_SIZE      11

/**
 * Defines the maximum number of bootloader descriptions in the BCT.
 */
#define NVBOOT_MAX_BOOTLOADERS         4

/**
 * Defines the minimum size of a block of storage in the secondary boot
 * device in log2(bytes) units.  Thus, a value of 8 == 256 bytes.
 */
#define NVBOOT_MIN_BLOCK_SIZE_LOG2     8

/**
 * Defines the maximum size of a block of storage in the secondary boot
 * device in log2(bytes) units.  Thus, a value of 23 == 8 mebibytes.
 */
#define NVBOOT_MAX_BLOCK_SIZE_LOG2    23

/**
 * Defines the minimum size of a page of storage in the secondary boot
 * device in log2(bytes) units.  Thus, a value of 8 == 256 bytes.
 */
#define NVBOOT_MIN_PAGE_SIZE_LOG2      8

/**
 * Defines the maximum size of a page of storage in the secondary boot
 * device in log2(bytes) units.  Thus, a value of 12 == 4096 bytes.
 */
#define NVBOOT_MAX_PAGE_SIZE_LOG2     12

/**
 * Defines the maximum page size in bytes (for convenience).
 */
#define NVBOOT_MAX_PAGE_SIZE  (1 << (NVBOOT_MAX_PAGE_SIZE_LOG2))

/**
 * Defines the maximum number of blocks to search for BCTs.
 *
 * This value covers the initial block and a set of journal blocks.
 *
 * Ideally, this number will span several erase units for reliable updates
 * and tolerance for blocks to become bad with use.  Safe updates require
 * a minimum of 2 erase units in which BCTs can appear.
 *
 * To ensure that the BCT search spans a sufficient range of configurations,
 * the search block count has been set to 64. This allows for redundancy with
 * a wide range of parts and provides room for greater problems in this
 * region of the device.
 */
#define NVBOOT_MAX_BCT_SEARCH_BLOCKS   64

/**
 * Defines the number of entries (bits) in the bad block table.
 * The consequences of changing its value are as follows.  Using P as the
 * # of physical blocks in the boot loader and B as the value of this
 * constant:
 *    B > P: There will be unused storage in the bad block table.
 *    B < P: The virtual block size will be greater than the physical block
 *           size, so the granularity of the bad block table will be less than
 *           one bit per physical block.
 *
 * 4096 bits is enough to represent an 8MiB partition of 2KiB blocks with one
 * bit per block (1 virtual block = 1 physical block).  This occupies 512 bytes
 * of storage.
 */
#define NVBOOT_BAD_BLOCK_TABLE_SIZE 4096

/**
 * Selection of engines & key slots for AES operations.
 *
 * The SBK key tables are stored in key slots for which read access can 
 * be disabled, but write access cannot be disabled.  Key slots 0 and 1
 * have these characteristics.
 *
 * The SBK key slots are configured for write-only access by the Boot ROM.
 *
 * The bootloader is required to over-write the SBK key slots before
 * passing control to any other code.
 */

/**
 * Defines the engine to use for the SBK.  The value is an enumerated
 * constant.
 */
#define NVBOOT_SBK_ENGINE NvBootAesEngine_A
    
/**
 * Defines the key slot to use for encryption with the SBK.  The value is an
 * enumerated constant.
 */
#define NVBOOT_SBK_ENCRYPT_SLOT   NvBootAesKeySlot_0

/**
 * Defines the key slot to use for decryption with the SBK.  The value is an
 * enumerated constant.
 */
#define NVBOOT_SBK_DECRYPT_SLOT   NvBootAesKeySlot_1
    
/**
 * The SSK key tables are stored in key slots for which read and/or
 * write access can be disabled.  Key slots 2 and 3 have these
 * characteristics.
 *
 * The SSK key slots are configured for write-only access by the Boot ROM.
 *
 * The SSK key slots can optionally be over-written by the bootloader with
 * any user-defined values.  Regardless, though, the bootloader must ensure
 * that write-access is disabled for these slots (at which time both read-
 * and write-access will be disabled).
 */

/**
 * Defines the engine to use for the SSK.  The value is an enumerated
 * constant.
 */
#define NVBOOT_SSK_ENGINE NvBootAesEngine_B

/**
 * Defines the key slot to use for encryption with the SSK.  The value is an
 * enumerated constant.
 */
#define NVBOOT_SSK_ENCRYPT_SLOT   NvBootAesKeySlot_2

/**
 * Defines the key slot to use for decryption with the SSK.  The value is an
 * enumerated constant.
 */
#define NVBOOT_SSK_DECRYPT_SLOT   NvBootAesKeySlot_3

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_NVBOOT_CONFIG_H */
