/**
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/*
 * nvboot_nand_context.h - Definitions for the NAND context structure.
 */

#ifndef INCLUDED_NVBOOT_NAND_CONTEXT_INT_H
#define INCLUDED_NVBOOT_NAND_CONTEXT_INT_H

#include "ap15/nvboot_nand_param.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * These defines are as per nand Flash spec's.
 */
#define NVBOOT_NAND_MAKER_CODE_MASK 0xFF
#define NVBOOT_NAND_DEVICE_CODE_OFFSET 8
#define NVBOOT_NAND_DEVICE_CODE_MASK 0xFF
#define NVBOOT_NAND_PAGE_SIZE_OFFSET 24
#define NVBOOT_NAND_BLOCK_SIZE_OFFSET 28
#define NVBOOT_NAND_BUS_WIDTH_OFFSET 30
#define NVBOOT_NAND_PAGE_SIZE_MASK 0x03
#define NVBOOT_NAND_BLOCK_SIZE_MASK 0x03
#define NVBOOT_NAND_BUS_WIDTH_MASK 0x01
#define NVBOOT_NAND_COMMAND_TIMEOUT_IN_US 100000
#define NVBOOT_NAND_READ_TIMEOUT_IN_US 500000

/**
 * These defines are as per Open Nand Flash Interface spec 1.0.
 */
#define NVBOOT_NAND_ONFI_PAGE_SIZE_OFFSET 10
#define NVBOOT_NAND_ONFI_PAGES_PER_BLOCK_OFFSET 23
#define NVBOOT_NAND_ONFI_BUS_WIDTH_OFFSET 6
#define NVBOOT_NAND_ONFI_BUS_WIDTH_MASK 0x01
#define NVBOOT_NAND_ONFI_PAGES_PER_BLOCK_OFFSET 23
#define NVBOOT_NAND_ONFI_PARAM_DATA_SIZE 256
#define NVBOOT_NAND_ONFI_SIGNATURE 0x49464E4F //IFNO --> "ONFI"

/**
 * These defines hold the  fuse bits information.
 * Nand Requires 6 fuse bits and they are interpreted in the following way.
 * Bits 0,1 --> Used for Number of address cycles info. Values 00, 01, 10 and 11
 *      indicate address cycles 4, 5, 6 and 7 respectively.
 * Bit 2 --> Used for Address format type. Bit value 0 indicates address format
 *      as NvBootNandAddressFormat_ColumnRow and bit value 1 indicates
 *      address format as NvBootNandAddressFormat_ColumnPageBlock.
 * Bits 3,4 --> Used for ECC Selection Values 00, 01, 10, and 11 indicate
 *      Ecc off, RS4, RS6 and RS8 respectively.
 * Bit 5 --> Used for ONfi support On/Off. Bit value 0 indicates Onfi support
 *      is off. Bit value 1 indicates Onfo support on.
 */
#define NVBOOT_NAND_ADDRESS_CYCLE_OFFSET 0
#define NVBOOT_NAND_ADDRESS_CYCLE_MASK 0x03
#define NVBOOT_NAND_ADDRESS_FORMAT_OFFSET 2
#define NVBOOT_NAND_ADDRESS_FORMAT_MASK 0x01
#define NVBOOT_NAND_ECC_SELECT_OFFSET 3
#define NVBOOT_NAND_ECC_SELECT_MASK 0x03
#define NVBOOT_NAND_ONFI_SUPPORT_OFFSET 5
#define NVBOOT_NAND_ONFI_SUPPORT_MASK 0x01

#define NAND_DEVICE_CONFIG_0_BLOCK_SIZE_OFFSET_RANGE 7:6
#define NAND_DEVICE_CONFIG_0_PAGE_SIZE_OFFSET_RANGE  9:8

/// These defines are Nand controller's specific.
#define NVBOOT_NAND_ECC_BUFFER_ALIGNMENT_MASK 0x3F
/**
 * For page size 4KBytes(Max Page size supported),
 * with RS Correctible Symbol Count(T) = 8,
 * we need 512 bytes + 64 bytes(for buffer alignment).
 *
 * We will use the same buffer for Reading the params from ONFI Flash with
 * Ecc turned off.
 */
#define NVBOOT_NAND_ECC_VECTOR_BUFFER_SIZE (512 + 64)

/// Minimum number of address cycles supported.
#define NVBOOT_NAND_MIN_ADDRESS_CYCLES_SUPPORTED 4
/// Maximum number of address cycles supported.
#define NVBOOT_NAND_MAX_ADDRESS_CYCLES_SUPPORTED 7
/// Max clock divider supported.
#define NVBOOT_NAND_MAX_CLOCK_DIVIDER_SUPPORTED 127

/// These defines are needed for Nand Addressing format.
#define NVBOOT_NAND_ADDR_REG_1_PAGE_OFFSET 16
#define NVBOOT_NAND_ADDR_REG_1_PAGE_MASK 0xFFFF
#define NVBOOT_NAND_ADDR_REG_2_PAGE_OFFSET 16
#define NVBOOT_NAND_BITS_PER_ADDRESS_REGISTER 32

/// Defines various nand commands as per nand spec.
typedef enum
{
    NvBootNandCommands_Reset = 0xFF,
    NvBootNandCommands_ReadId = 0x90,
    NvBootNandCommands_ReadId_Address = 0x20,
    NvBootNandCommands_Read = 0x00,
    NvBootNandCommands_ReadStart = 0x30,
    NvBootNandCommands_ReadParamPage = 0xEC,
    NvBootNandCommands_Status = 0x70,
    NvBootNandCommands_CommandNone = 0,
    NvBootNandCommands_Force32 = 0x7FFFFFFF
} NvBootNandCommands;

/// Defines Nand command structure.
typedef struct
{
    NvBootNandCommands Command1;
    NvBootNandCommands Command2;
    NvU16 ColumnNumber;
    NvU16 PageNumber;
    NvU16 BlockNumber;
    NvU8 ChipNumber;
} NvBootNandCommand;

/// Defines Nand Page sizes that are supported in Boot ROM.
typedef enum
{
    NvBootNandPageSize_2KBLog2 = 0xB,
    NvBootNandPageSize_4KBLog2 = 0xC,

    NvBootNandPageSize_Min = NvBootNandPageSize_2KBLog2,
    NvBootNandPageSize_Max = NvBootNandPageSize_4KBLog2,

    NvBootNandPageSize_Force32 = 0x7FFFFFFF
} NvBootNandPageSize;

/// Defines Nand Block sizes in bytes that are supported in Boot ROM.
typedef enum
{
    NvBootNandBlockSize_64KBLog2 = 0x10,
    NvBootNandBlockSize_128KBLog2 = 0x11,
    NvBootNandBlockSize_256KBLog2 = 0x12,
    NvBootNandBlockSize_512KBLog2 = 0x13,
    NvBootNandBlockSize_Force32 = 0x7FFFFFFF
} NvBootNandBlockSize;

typedef enum
{
    NvBootNandBusWidth_8Bit = 1,
    NvBootNandBusWidth_16Bit = 2,
    NvBootNandBusWidth_Force32 = 0x7FFFFFFF
} NvBootNandBusWidth;

/*
 * NvBootNandContext - The context structure for the NAND driver.
 * A pointer to this structure is passed into the driver's Init() routine.
 * This pointer is the only data that can be kept in a global variable for
 * later reference.
 */
typedef struct NvBootNandContextRec
{
    /// Maker Code.
    NvU8 MakerCode;
    /// Device Code.
    NvU8 DeviceCode;
    /// Block size in Log2 scale.
    NvU8 BlockSizeLog2;
    /// Page size in Log2 scale.
    NvU8 PageSizeLog2;
    /// No of pages per block in Log 2 scale;
    NvU8 PagesPerBlockLog2;
    /// Number of address cycles.
    NvU8 NumOfAddressCycles;
    /// Address format.
    NvBootNandAddressFormat AddressFormat;
    /// Bus width.
    NvBootNandBusWidth BusWidth;
    /// Nand Command params.
    NvBootNandCommand Command;
    /// Device status.
    NvBootDeviceStatus DeviceStatus;
    /// Holds the Nand Read start time.
    NvU64 ReadStartTime;
    /// Indicates whether the ONFI support is enabled/disabled.
    NvBool EnableOnfiSupport;
    /// Holds the selected ECC.
    NvBootNandEccSelection EccSelection;
    /// Ecc Error vector buffer.
    NvU8 EccErrorVector[NVBOOT_NAND_ECC_VECTOR_BUFFER_SIZE];
} NvBootNandContext;

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_NVBOOT_NAND_CONTEXT_INT_H */
