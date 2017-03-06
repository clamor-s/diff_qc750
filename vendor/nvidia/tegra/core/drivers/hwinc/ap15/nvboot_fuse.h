/*
 * Copyright (c) 2006 - 2007 NVIDIA Corporation.  All rights reserved.
 * 
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file nvboot_fuse.h
 *
 * Defines the parameters and data structures related to fuses.
 */

#ifndef INCLUDED_NVBOOT_FUSE_H
#define INCLUDED_NVBOOT_FUSE_H

#define NVBOOT_SECURE_BOOT_KEY_BYTES (16) 
#define NVBOOT_DEVICE_KEY_BYTES (4)

#if defined(__cplusplus)
extern "C"
{
#endif

/*
 * NvBootFuseBootDevice -- Peripheral device where Boot Loader is stored
 *
 * A02 split the original NAND encoding in NAND x8 and NAND x16
 * for backwards compatibility the original enum was maintained and 
 * is implicitly x8 and aliased with the explicit x8 enum
 */
typedef enum
{
    NvBootFuseBootDevice_None = 0,
    NvBootFuseBootDevice_NandFlash,
    NvBootFuseBootDevice_NandFlash_x8 = NvBootFuseBootDevice_NandFlash,
    NvBootFuseBootDevice_NorFlash,
    NvBootFuseBootDevice_SpiFlash,
    NvBootFuseBootDevice_Emmc,
#ifndef BUG_430460_TO_BE_DELETED
    NvBootFuseBootDevice_HsmmcMovinand = NvBootFuseBootDevice_Emmc,
#endif
    NvBootFuseBootDevice_NandFlash_x16,
    NvBootFuseBootDevice_Max, /* Must appear after the last legal item */
    NvBootFuseBootDevice_Undefined,
    NvBootFuseBootDevice_Force32 = 0x7fffffff
} NvBootFuseBootDevice;

/**
 * NvBootFuseBootDeviceConfiguration -- Characteristics of the boot device
 *
 * The configuration settings for each boot device type are as follows --
 * 
 * * NandFlash_x8 or NandFlash_x16
 *   - Block size offset - 0 (most NAND), 1 (42nm NAND), 2, or 3
 *   - Page  size offset - 0 (most NAND), 1 (42nm NAND), 2, or 3
 *   - ECC selection -- none (ECC off), RS4, RS6, or RS8
 *   - Number of address cycles -- 4, 5, 6, or 7
 * * Emmc
 *   - voltage range selection - High, Dual, or Low
 *   - data bus width -- 4-bit or 8-bit
 * * NorFlash 
 *   - none
 * * SpiFlash
 *   - none
 * 
 * Note: Any bits not specifically defined below are reserved and must be
 * programmed to zero.
 *
 * These #define's are compatible with the DRF macros (see nvrm_drf.h).
 *
 * For example, to generate the configuration setting for NAND flash with
 * 5 address cycles, ECC=RS4, and Block & Page size offsets for 42nm NAND
 * (each == 1):
 *
 * config = NV_DRF_DEF(NVBOOT_FUSE, NAND_CONFIG, ADDRESS_CYCLES,    5_CYCLES) |
 *          NV_DRF_DEF(NVBOOT_FUSE, NAND_CONFIG, ECC_SELECT,        RS4)      |
 *          NV_DRF_DEF(NVBOOT_FUSE, NAND_CONFIG, PAGE_SIZE_OFFSET,  0)        |
 *          NV_DRF_DEF(NVBOOT_FUSE, NAND_CONFIG, BLOCK_SIZE_OFFSET, 0);
 *
 * Similarly, for EMMC with 8-bit data width and high voltage range --
 *
 * config = NV_DRF_DEF(NVBOOT_FUSE, EMMC_CONFIG, VOLTAGE_RANGE, HIGH) |
 *          NV_DRF_DEF(NVBOOT_FUSE, EMMC_CONFIG, BUS_WIDTH,     8_BIT);
 */

// NAND config

/*
 * Block size offset
 *   - Introduced in AP16 A03.
 *   - All versions of AP15 and all prior versions of AP16 used a 
 *     block offset of 0
 *   - 42nm NAND devices typically require a block offset of 1
 *   - Earlier   devices typically require a block offset of 0
 */
#define NVBOOT_FUSE_NAND_CONFIG_0_BLOCK_SIZE_OFFSET_RANGE  9:8
#define NVBOOT_FUSE_NAND_CONFIG_0_BLOCK_SIZE_OFFSET_0      0x0UL
#define NVBOOT_FUSE_NAND_CONFIG_0_BLOCK_SIZE_OFFSET_1      0x1UL
#define NVBOOT_FUSE_NAND_CONFIG_0_BLOCK_SIZE_OFFSET_2      0x2UL
#define NVBOOT_FUSE_NAND_CONFIG_0_BLOCK_SIZE_OFFSET_3      0x3UL

/*
 * Page size offset
 *   - Introduced in AP16 A03.
 *   - All versions of AP15 and all prior versions of AP16 used a 
 *     page offset of 0
 *   - 42nm NAND devices typically require a block offset of 1
 *   - Earlier   devices typically require a block offset of 0
 */
#define NVBOOT_FUSE_NAND_CONFIG_0_PAGE_SIZE_OFFSET_RANGE   7:6
#define NVBOOT_FUSE_NAND_CONFIG_0_PAGE_SIZE_OFFSET_0       0x0UL
#define NVBOOT_FUSE_NAND_CONFIG_0_PAGE_SIZE_OFFSET_1       0x1UL
#define NVBOOT_FUSE_NAND_CONFIG_0_PAGE_SIZE_OFFSET_2       0x2UL
#define NVBOOT_FUSE_NAND_CONFIG_0_PAGE_SIZE_OFFSET_3       0x3UL

#define NVBOOT_FUSE_NAND_CONFIG_0_ECC_SELECT_RANGE         4:3
#define NVBOOT_FUSE_NAND_CONFIG_0_ECC_SELECT_NONE          0x0UL
#define NVBOOT_FUSE_NAND_CONFIG_0_ECC_SELECT_RS4           0x1UL
#define NVBOOT_FUSE_NAND_CONFIG_0_ECC_SELECT_RS6           0x2UL
#define NVBOOT_FUSE_NAND_CONFIG_0_ECC_SELECT_RS8           0x3UL

#define NVBOOT_FUSE_NAND_CONFIG_0_ADDRESS_CYCLES_RANGE     1:0
#define NVBOOT_FUSE_NAND_CONFIG_0_ADDRESS_CYCLES_4_CYCLES  0x0UL
#define NVBOOT_FUSE_NAND_CONFIG_0_ADDRESS_CYCLES_5_CYCLES  0x1UL
#define NVBOOT_FUSE_NAND_CONFIG_0_ADDRESS_CYCLES_6_CYCLES  0x2UL
#define NVBOOT_FUSE_NAND_CONFIG_0_ADDRESS_CYCLES_7_CYCLES  0x3UL

// EMMC config

/*
 * Voltage range
 *   - Introduced in AP16 A01.
 *   - All versions of AP15 used dual voltage.
 */
#define NVBOOT_FUSE_EMMC_CONFIG_0_VOLTAGE_RANGE_RANGE      2:1
#define NVBOOT_FUSE_EMMC_CONFIG_0_VOLTAGE_RANGE_HIGH       0x0UL
#define NVBOOT_FUSE_EMMC_CONFIG_0_VOLTAGE_RANGE_DUAL       0x1UL
#define NVBOOT_FUSE_EMMC_CONFIG_0_VOLTAGE_RANGE_LOW        0x2UL

#define NVBOOT_FUSE_EMMC_CONFIG_0_BUS_WIDTH_RANGE          0:0
#define NVBOOT_FUSE_EMMC_CONFIG_0_BUS_WIDTH_4_BIT          0x0UL
#define NVBOOT_FUSE_EMMC_CONFIG_0_BUS_WIDTH_8_BIT          0x1UL

#if defined(__cplusplus)
}
#endif

#endif // INCLUDED_NVBOOT_FUSE_H
