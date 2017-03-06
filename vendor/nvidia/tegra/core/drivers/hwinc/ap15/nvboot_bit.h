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
 * NvBootInfoTable (BIT) provides information from the Boot ROM (BR)
 * to Bootloaders (BLs).
 *
 * Initially, the BIT is cleared.
 * 
 * As the BR works its way through its boot sequence, it records data in
 * the BIT.  This includes information determined as it boots and a log
 * of the boot process.  The BIT also contains a pointer to an in-memory,
 * plaintext copy of the Boot Configuration Table (BCT).
 *
 * The BIT allows BLs to determine how they were loaded, any errors that
 * occured along the way, and which of the set of BLs was finally loaded.
 *
 * The BIT also serves as a tool for diagnosing boot failures.  The cold boot
 * process is necessarily opaque, and the BIT provides a window into what
 * actually happened.  If the device is completely unable to boot, the BR
 * will enter Recovery Mode (RCM), using which one can load an applet that
 * dumps the BIT and BCT contents for analysis on the host.
 */

#ifndef INCLUDED_NVBOOT_BIT_H
#define INCLUDED_NVBOOT_BIT_H

#include "nvboot_bct.h"
#include "nvboot_config.h"
#include "nvboot_osc.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/// Specifies the amount of status data needed in the BIT.
/// One bit of status is used for each of the journal blocks,
/// and 1 additional bit is needed for the second BCT in block 0.
#define NVBOOT_BCT_STATUS_BITS  (NVBOOT_MAX_BCT_SEARCH_BLOCKS + 1)
#define NVBOOT_BCT_STATUS_BYTES ((NVBOOT_BCT_STATUS_BITS + 7) >> 3)

/**
 * Defines the type of boot.
 * Note: There is no BIT for warm boot.
 */
typedef enum
{
    /// Specifies a default (unset) value.
    NvBootType_None = 0,

    /// Specifies a cold boot
    NvBootType_Cold,

    /// Specifies the BR entered RCM
    NvBootType_Recovery,

    /// Specifies UART boot (only available internal to NVIDIA)
    NvBootType_Uart,

    NvBootType_Force32 = 0x7fffffff    
} NvBootType;

/**
 * Defines the type of device booted from.
 * Used to identify primary and secondary boot devices.
 */
typedef enum
{
    /// Specifies a default (unset) value.
    NvBootDevType_None = 0,

    /* The next four match the fuse API device values */
    /// Specifies NAND.
    NvBootDevType_Nand,

    /// Specifies an alias for 8-bit NAND.
    NvBootDevType_Nand_x8 = NvBootDevType_Nand,

    /// Specifies NOR (not supported).
    NvBootDevType_Nor,

    /// Specifies SPI NOR.
    NvBootDevType_Spi,

    /// Specifies eMMC.
    NvBootDevType_Emmc,
#ifndef BUG_430460_TO_BE_DELETED
    /// Specifies an alias for MoviNAND.
    NvBootDevType_MoviNand = NvBootDevType_Emmc, /* Deprecated */
#endif

    /// Specifies internal ROM (i.e., the BR).
    NvBootDevType_Irom,

    /// Specifies UART (only available internal to NVIDIA).
    NvBootDevType_Uart,

    /// Specifies USB (i.e., RCM)
    NvBootDevType_Usb,

    /// Specifies 16-bit NAND.
    NvBootDevType_Nand_x16,

    NvBootDevType_Force32 = 0x7fffffff    
} NvBootDevType;

/**
 * Defines the status codes for attempting to load a BL.
 */
typedef enum
{
    /// Specifies a default (unset) value.
    NvBootRdrStatus_None = 0,

    /// Specifies a successful load.
    NvBootRdrStatus_Success,

    /// Specifies validation failure.
    NvBootRdrStatus_ValidationFailure,

    /// Specifies a read error.
    NvBootRdrStatus_DeviceReadError,

    NvBootRdrStatus_Force32 = 0x7fffffff
} NvBootRdrStatus;

/**
 * Defines the information recorded about each BL.
 *
 * BLs that do not become the primary copy to load have status of None.
 * They may still experience Ecc errors if used to recover from ECC
 * errors of another copy of the BL.
 */
typedef struct NvBootBlStateRec
{
    /// Specifies the outcome of attempting to load this BL.
    NvBootRdrStatus Status;

    /// Specifies the first block that experienced an ECC error, if any.
    /// 0 otherwise.
    NvU32           FirstEccBlock;

    /// Specifies the first page that experienced an ECC error, if any.
    /// 0 otherwise.
    NvU32           FirstEccPage;

    /// Specifies if the BL experienced any ECC errors.
    NvBool          HadEccError;

    /// Specifies if the BL provided data for another BL that experienced an
    /// ECC error.
    NvBool          UsedForEccRecovery;
} NvBootBlState;

/**
 * Defines the BIT.
 *
 * Notes:
 * * SecondaryDevice: This is set by cold boot (and soon UART) processing.
 *   Recovery mode does not alter its value.
 * * BctStatus[] is a bit vector representing the cause of BCT read failures.
 *       A 0 bit indicates a validation failure
 *       A 1 bit indicates a device read error
 *       Bit 0 contains the status for the BCT at block 0, slot 0.
 *       Bit 1 contains the status for the BCT at block 0, slot 1.
 *       Bit N contains the status for the BCT at block (N-1), slot 0, which
 *       is a failed attempt to locate the journal block at block N.
 *       (1 <= N < NVBOOT_MAX_BCT_SEARCH_BLOCKS)
 * * BctLastJournalRead contains the cause of the BCT search within the
 *   journal block. Success indicates the search ended with the successful
 *   reading of the BCT in the last slot.  CRC and ECC failures are indicated
 *   appropriately. 
 */
typedef struct NvBootInfoTableRec
{
    ///
    /// Version information
    ///

    /// Specifies the version number of the BR code.
    NvU32               BootRomVersion;

    /// Specifies the version number of the BR data structure.
    NvU32               DataVersion;

    /// Specifies the version number of the RCM protocol.
    NvU32               RcmVersion;

    /// Specifies the type of boot.
    NvBootType          BootType;

    /// Specifies the primary boot device.
    NvBootDevType       PrimaryDevice;

    /// Specifies the secondary boot device.
    NvBootDevType       SecondaryDevice;

    ///
    /// Hardware status information
    ///

    /// Specifies the measured oscillator frequency.
    NvBootClocksOscFreq OscFrequency;

    /// Specifies whether the device was initialized.
    NvBool              DevInitialized;

    /// Specifies whether SDRAM was initialized.
    NvBool              SdramInitialized;

    /// Specifies whether the ForceRecovery AO bit was cleared.
    NvBool              ClearedForceRecovery;

    /// Specifies whether the FailBack AO bit was cleared.
    NvBool              ClearedFailBack;

    ///
    /// BCT information
    ///

    /// Specifies if a valid BCT was found.
    NvBool              BctValid;

    /// Specifies the status of attempting to read BCTs during the
    /// BCT search process.  See the notes above for more details.
    NvU8                BctStatus[NVBOOT_BCT_STATUS_BYTES];

    /// Specifies the status of the last journal block read.
    NvBootRdrStatus     BctLastJournalRead;

    /// Specifies the block number in which the BCT was found.
    NvU32               BctBlock;

    /// Specifies the page number of the start of the BCT that was found.
    NvU32               BctPage; 

    /// Specifies the size of the BCT in bytes.  It is 0 until BCT loading
    /// is attempted.
    NvU32               BctSize;  /* 0 until BCT loading is attempted */

    /// Specifies a pointer to the BCT in memory.  It is NULL until BCT
    /// loading is attempted.  The BCT in memory is the last BCT that
    /// the BR tried to load, regardless of whether the operation was
    /// successful.
    NvBootConfigTable  *BctPtr;

    /// Specifies the state of attempting to load each of the BLs.
    NvBootBlState       BlState[NVBOOT_MAX_BOOTLOADERS];

    /// Specifies the lowest iRAM address that preserves communicated data.
    /// SafeStartAddr starts out with the address of memory following
    /// the BIT.  When BCT loading starts, it is bumped up to the 
    /// memory following the BCT.
    NvU32               SafeStartAddr;

} NvBootInfoTable;

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_NVBOOT_BIT_H */
