/*
 * Copyright (c) 2007-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * @brief <b>nVIDIA Driver Development Kit:
 *           Private functions for SNOR device driver
 *
 * @b Description:  Defines the private interfacing functions for the SNOR
 * device driver.
 *
 */

#ifndef INCLUDED_SNOR_PRIV_DRIVER_H
#define INCLUDED_SNOR_PRIV_DRIVER_H

#include "nvcommon.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define SPANSION_SECTOR_SIZE (128 * 1024)

/* SNOR APB registers */

#define SNOR_READ32(pSnorHwRegsVirtBaseAdd, reg) \
        NV_READ32((pSnorHwRegsVirtBaseAdd) + ((SNOR_##reg##_0)/4))

#define SNOR_WRITE32(pSnorHwRegsVirtBaseAdd, reg, val) \
    do \
    {  \
        NV_WRITE32((((pSnorHwRegsVirtBaseAdd) + ((SNOR_##reg##_0)/4))), (val)); \
    } while (0)

/* Flash Read Write */

#define SNOR_FLASH_WRITE16(pSnorBaseAdd, Address, Val) \
        do \
        { \
                NV_WRITE16(((NvU32)(pSnorBaseAdd) + (Address <<1)),Val) ; \
        } while(0)

#define SNOR_FLASH_READ16(pSnorBaseAdd, Address) NV_READ16((NvU32)(pSnorBaseAdd) + (Address << 1))

#define SNOR_FLASH_WRITE32(pSnorBaseAdd, Address, Val) \
        do\
        { \
            NV_WRITE32 (((NvU32)(pSnorBaseAdd) + (Address << 2)), Val); \
        } while (0)

#define FLASH_READ_ADDRESS32(pSnorBaseAdd,Address) NV_READ32((NvU32)(pSnorBaseAdd) + (Address << 2))


typedef enum
{
    SnorDeviceType_x16_NonMuxed         = 0x0,
    SnorDeviceType_x32_NonMuxed         = 0x1,
    SnorDeviceType_x16_Muxed            = 0x2,
    SnorDeviceType_x32_Muxed         = 0x3,
    SnorDeviceType_Unknown              = 0x10,
    SnorDeviceType_Force32           = 0x7FFFFFFF
} SnorDeviceType;

/**
 * SNOR command status
 */
typedef enum
{
    SnorCommandStatus_Ready          = 0x0,
    SnorCommandStatus_Success        = 0x0,
    SnorCommandStatus_Busy           = 0x1,
    SnorCommandStatus_Error          = 0x2,
    SnorCommandStatus_LockError      = 0x3,
    SnorCommandStatus_EraseError     = 0x4,
    SnorCommandStatus_EccError       = 0x5,
    SnorCommandStatus_Force32        = 0x7FFFFFFF
} SnorCommandStatus;

/**
 * SNOR device information.
 */
typedef struct
{
    NvU32 DeviceId;
    NvU32 ManufId;

    SnorDeviceType DeviceType;
    // NVTODO : Redundant. Should derive this
    // from DeviceType
    NvU32 BusWidth ;
    // NVTODO : Verify which of these
    // are needed by bootrom
    NvU32 TotalBlocks;
    NvU32 PagesPerBlock;
    NvU32 PageSize;
    NvU32 SectorPerPage;
    NvU32 SectorSize;
    // NVTODO : Which timings are we
    // interested in ??
    NvU32 ResetTimingMs;
    NvU32 PageLoadTimeUs;
    NvU32 PageProgramTimeUs;
    NvU32 BlockLockUnlockTimeUs;
    NvU32 AllBlockUnlockTimeUs;
    NvU32 BlockEraseTimeMs;
} SnorDeviceInfo;

typedef struct
{
    NvRmPhysAddr NorBaseAddress;
    NvU16 *pNorBaseAdd;
    NvU32 NorAddressSize;
} SnorDeviceIntRegister;


/**
 * SNOR interrupt reason.
 */
typedef enum
{
  SnorInterruptReason_None = 0x0,
  SnorInterruptReason_Force32 = 0x7FFFFFFF
} SnorInterruptReason;


// The structure of the function pointers to provide the interface to
// SNOR device driver
typedef struct
{
    /**
     * Initialize the device interface.
     */
    void (*DevInitializeDeviceInterface)(SnorDeviceIntRegister *pDevIntRegs);


    /**
     * Find whether the device is available on this interface or not.
     */
    NvU32 (*DevResetDevice)(SnorDeviceIntRegister *pDevIntRegs, NvU32 DeviceChipSelect);


    NvBool (*DevIsDeviceReady)(SnorDeviceIntRegister *pDevIntRegs);

    /**
     * Get the device information.
     */
    NvBool
    (*DevGetDeviceInfo)(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo,
        NvU32 DeviceChipSelect);

    NvBool
    (*DevConfigureHostInterface)(
        SnorDeviceIntRegister *pDevIntRegs,
        NvBool IsAsynch);

    void
    (*DevAsynchReadFromBufferRam)(
        SnorDeviceIntRegister *pDevIntRegs,
        NvU8 *pReadMainBuffer,
        NvU8 *pReadSpareBuffer,
        NvU32 MainAreaSizeBytes,
        NvU32 SpareAreaSize);

    void
    (*DevAsynchWriteIntoBufferRam)(
        SnorDeviceIntRegister *pDevIntRegs,
        NvU8 *pWriteMainBuffer,
        NvU8 *pWriteSpareBuffer,
        NvU32 MainAreaSizeBytes,
        NvU32 SpareAreaSize);

    NvU32
    (*DevEraseBlock)(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo,
        NvU32 BlockNumber);

    SnorCommandStatus (*DevGetEraseStatus)(SnorDeviceIntRegister *pDevIntRegs);

    NvBool (*DevEraseSuspend)(SnorDeviceIntRegister *pDevIntRegs);

    NvBool (*DevEraseResume)(SnorDeviceIntRegister *pDevIntRegs);

    SnorCommandStatus
    (*DevSetBlockLockStatus)(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo,
        NvU32 BlockNumber,
        NvBool IsLock);

    SnorCommandStatus
    (*DevUnlockAllBlockOfChip)(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo);

    NvBool
    (*DevIsBlockLocked)(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo,
        NvU32 BlockNumber);

    SnorCommandStatus
    (*DevGetLoadStatus)(
        SnorDeviceIntRegister *pDevIntRegs);

    SnorCommandStatus
    (*DevLoadBlockPage)(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo,
        NvU32 BlockNumber,
        NvU32 PageNumber,
        NvBool IsWaitForCompletion);

    void
    (*DevSelectDataRamFromBlock)(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo,
        NvU32 BlockNumber);

    SnorCommandStatus
    (*DevGetProgramStatus)(
        SnorDeviceIntRegister *pDevIntRegs);

    SnorCommandStatus
    (*DevFormatDevice)(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo,
        NvBool IsWaitForCompletion);

    SnorCommandStatus
    (*DevEraseSector)(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo,
        NvU32 StartOffset,
        NvU32 Length,
        NvBool IsWaitForCompletion);

    SnorCommandStatus
    (*DevProgram)(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo,
        NvU32 ByteOffset,
        NvU32 SizeInBytes,
	const NvU8* Buffer,
        NvBool IsWaitForCompletion);

    SnorCommandStatus
    (*DevRead)(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo,
        NvU32 ByteOffset,
        NvU32 SizeInBytes,
	NvU8* Buffer,
        NvBool IsWaitForCompletion);

    SnorInterruptReason
    (*DevGetInterruptReason)(
        SnorDeviceIntRegister *pDevIntRegs);

    SnorCommandStatus
    (*DevReadMode)(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo,
        NvBool IsWaitForCompletion);

    SnorCommandStatus
    (*DevProtectSectors)(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo,
        NvU32 Offset,
        NvU32 Size,
        NvBool IsLastPartition);

    SnorCommandStatus
    (*DevUnprotectAllSectors)(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo);

} SnorDevInterface, *SnorDevInterfaceHandle;


NvBool
IsDeviceAvailable(
    SnorDeviceIntRegister *pDevIntRegs,
    NvU32 DeviceChipSelect);

SnorDeviceType GetDeviceType(SnorDeviceIntRegister *pDevIntRegs);

/**
 * Initialize the SNOR device interface.
 */
void
NvRmPrivSnorDeviceInterface(
    SnorDevInterface *pDevInterface,
    SnorDeviceIntRegister *pDevIntReg,
    SnorDeviceInfo *pDevInfo);

NvBool
IsSnorDeviceAvailable(
    SnorDeviceIntRegister *pDevIntRegs,
    NvU32 DeviceChipSelect);

SnorDeviceType GetSnorDeviceType(SnorDeviceIntRegister *pDevIntRegs);

SnorCommandStatus SpansionGetNumBlocks(
    SnorDeviceIntRegister *pDevIntRegs,
    NvU32 *NumBlocks);

SnorCommandStatus SpansionFormatDevice(
    SnorDeviceIntRegister *pDevIntRegs,
    SnorDeviceInfo *pDevInfo,
    NvBool IsWaitForCompletion);

SnorCommandStatus SpansionEraseSector(
    SnorDeviceIntRegister *pDevIntRegs,
    SnorDeviceInfo *pDevInfo,
    NvU32 StartOffset,
    NvU32 Length,
    NvBool IsWaitForCompletion);

SnorCommandStatus SpansionProtectSectors(
    SnorDeviceIntRegister *pDevIntRegs,
    SnorDeviceInfo *pDevInfo,
    NvU32 Offset,
    NvU32 Size,
    NvBool IsLastPartition);

SnorCommandStatus SpansionUnprotectAllSectors(
    SnorDeviceIntRegister *pDevIntRegs,
    SnorDeviceInfo *pDevInfo);

SnorCommandStatus SpansionProgram(
    SnorDeviceIntRegister *pDevIntRegs,
    SnorDeviceInfo *pDevInfo,
    NvU32 ByteOffset,
    NvU32 SizeInBytes,
    const NvU8* Buffer,
    NvBool IsWaitForCompletion);

SnorCommandStatus SpansionAsyncRead(
    SnorDeviceIntRegister *pDevIntRegs,
    SnorDeviceInfo *pDevInfo,
    NvU32 ByteOffset,
    NvU32 SizeInBytes,
    NvU8* Buffer,
    NvBool IsWaitForCompletion);

SnorCommandStatus SpansionReadMode(
    SnorDeviceIntRegister *pDevIntRegs,
    SnorDeviceInfo *pDevInfo,
    NvBool IsWaitForCompletion);

/** @}*/

#if defined(__cplusplus)
}
#endif

#endif  // INCLUDED_SNOR_PRIV_DRIVER_H


