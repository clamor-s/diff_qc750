/*
 * Copyright (c) 2007 - 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * nvboot_nand_context.h - Definitions for the Nand context structure.
 */

#ifndef INCLUDED_NVBOOT_NAND_CONTEXT_H
#define INCLUDED_NVBOOT_NAND_CONTEXT_H

#include "ap20/nvboot_nand_param.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * Buffer size for Reading the params from Onfi Flash.
 */
#define NVBOOT_NAND_ONFI_PARAM_BUF_SIZE_IN_BYTES 256

/// Defines various nand commands as per Nand spec.
typedef enum
{
    NvBootNandCommands_Reset = 0xFF,
    NvBootNandCommands_ReadId = 0x90,
    NvBootNandCommands_ReadId_Address = 0x20,
    NvBootNandCommands_Read = 0x00,
    NvBootNandCommands_ReadStart = 0x30,
    NvBootNandCommands_ReadParamPage = 0xEC,
    NvBootNandCommands_ReadParamPageStart = 0xAB,
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

/// Defines Data widths that are supported in BootRom.
typedef enum
{
    NvBootNandDataWidth_Discovey = 0,
    NvBootNandDataWidth_8Bit,
    NvBootNandDataWidth_16Bit,
    NvBootNandDataWidth_Num,
    NvBootNandDataWidth_Force32 = 0x7FFFFFFF
} NvBootNandDataWidth;

/**
 * Defines HW Ecc options that can be selected.
 * Ecc Discovery option lets the driver to discover the Ecc used.
 * Rs4 Ecc can correct upto 4 symbol per 512 bytes.
 * Bch8 and Bch16 Ecc can correct upto 8 and 16 bits per 512 bytes respectively.
 *
 */
typedef enum
{
    NvBootNandEccSelection_Discovery = 0,
    NvBootNandEccSelection_Rs4,
    NvBootNandEccSelection_Bch8,
    NvBootNandEccSelection_Bch16,
    NvBootNandEccSelection_Off,
    NvBootNandEccSelection_Num,
    NvBootNandEccSelection_Force32 = 0x7FFFFFFF
} NvBootNandEccSelection;

/**
 * NvBootNandContext - The context structure for the Nand driver.
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
    /// Nand Data width.
    NvBootNandDataWidth DataWidth;
    /// Nand Command params.
    NvBootNandCommand Command;
    /// Device status.
    NvBootDeviceStatus DeviceStatus;
    /// Holds the Nand Read start time.
    NvU64 ReadStartTime;
    /// Holds the selected ECC.
    NvBootNandEccSelection EccSelection;
    /// Buffer for reading Onfi params. Define as NvU32 to make it word aligned.
    NvU32 OnfiParamsBuffer[NVBOOT_NAND_ONFI_PARAM_BUF_SIZE_IN_BYTES / sizeof(NvU32)];
    /// Onfi Nand part or not.
    NvBool IsOnfi;
} NvBootNandContext;

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_NVBOOT_NAND_CONTEXT_H */
