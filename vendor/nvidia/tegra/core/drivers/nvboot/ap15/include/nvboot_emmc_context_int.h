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
 * nvboot_emmc_context_int.h - Definitions for the eMMC context structure.
 */

#ifndef INCLUDED_NVBOOT_EMMC_CONTEXT_INT_H
#define INCLUDED_NVBOOT_EMMC_CONTEXT_INT_H

#include "ap15/nvboot_emmc_param.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/// Emmc Response buffer size. It must be multiple of 4 bytes.
#define NVBOOT_EMMC_RESPONSE_BUFFER_SIZE_IN_BYTES 20
/// This needs to be more than the BCT struct info.
#define NVBOOT_EMMC_BLOCK_SIZE_LOG2 12

/// Defines Command Responses of Emmc.
typedef enum
{
    MmcResponseType_NoResponse = 0,
    MmcResponseType_R1,
    MmcResponseType_R2,
    MmcResponseType_R3,
    MmcResponseType_R4,
    MmcResponseType_R5,
    MmcResponseType_R6,
    MmcResponseType_R7,
    MmcResponseType_R1B,
    MmcResponseType_Num,
    MmcResponseType_Force32 = 0x7FFFFFFF
} MmcResponseType;

/* Defines Emmc Address modes.
 * If the Emmc cards capacity is <= 2B, Byte addressing is used.
 * If the Emmc cards capacity is > 2B, Sector addressing is used.
 */
typedef enum
{
    MmcAddressMode_Sector = 0x40ff8080,
    MmcAddressMode_Byte = 0x00ff8080,
    MmcAddressMode_Force32 = 0x7FFFFFFF
} MmcAddressMode;

/// Defines Various Emmc Commands as per spec.
typedef enum
{
    MmcCommand_GoIdleState = 0,
    MmcCommand_SendOperatingConditions = 1,
    MmcCommand_AllSendCid = 2,
    MmcCommand_SetRelativeAddress = 3,
    MmcCommand_Switch = 6,
    MmcCommand_SelectDeselectCard = 7,
    MmcCommand_SendCsd = 9,
    MmcCommand_SendStatus = 13,
    MmcCommand_SetBlockLength = 16,
    MmcCommand_ReadSingle = 17,
    MmcCommand_Force32 = 0x7FFFFFFF
} MmcCommands;

/// Defines Emmc card states.
typedef enum
{
    MmcState_Idle = 0,
    MmcState_Ready,
    MmcState_Ident,
    MmcState_Stby,
    MmcState_Tran,
    MmcState_Data,
    MmcState_Rcv,
    MmcState_Prg,
    MmcState_Force32 = 0x7FFFFFFF
} MmcState;

/*
 * NvBootEmmcContext - The context structure for the EMMC driver.
 * A pointer to this structure is passed into the driver's Init() routine.
 * This pointer is the only data that can be kept in a global variable for
 * later reference.
 */
typedef struct NvBootEmmcContextRec
{
    /// Block size.
    NvU8 BlockSizeLog2;
    /// Page size.
    NvU8 PageSizeLog2;
    /// No of pages per block;
    NvU8 PagesPerBlockLog2;
    /// Card's Relative Card Address.
    NvU32 CardRca;
    /// Data bus width.
    NvBootEmmcDataWidth DataWidth;
    /**
     * Clock divider for the HsmmcController clock source.
     * Hsmmc Controller gets PLL_P clock, which is 432MHz, as a clock source.
     * If it is set to 18, then clock Frequency at which Hsmmc controller runs
     * is 432/18 = 24MHz.
     */
    NvU8 ClockDivider;
    /// Response buffer
    NvU32 MmcResponse[NVBOOT_EMMC_RESPONSE_BUFFER_SIZE_IN_BYTES /
                      sizeof(NvU32)];
    /// Address mode(sector/byte).
    MmcAddressMode AddressMode;
    /// Mmc Card State.
    MmcState CardState;
    /// Device status.
    NvBootDeviceStatus DeviceStatus;
    /// Holds the Movi Nand Read start time.
    NvU32 ReadStartTime;
    /**
     * This sub divides the clock that goes to card from controller.
     * If the controller clock is 24 Mhz and if this set to 2, then
     * the clock that goes to card is 12MHz.
     */
    NvU8 CardClockDivider;
    /// Indicates whether to access the card in High speed mode.
    NvBool HighSpeedMode;
} NvBootEmmcContext;


#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_NVBOOT_EMMC_CONTEXT_INT_H */

