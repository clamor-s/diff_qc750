/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
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
 *           Private functions for device driver of mux onenand and
 *           Flex mux one nand device.
 *
 * @b Description:  Defines the private interfacing functions for the device
 * driver of the mux one nand and flex mux one nand device.
 *
 */

#ifndef INCLUDED_ONE_NAND_PRIV_DRIVER_H
#define INCLUDED_ONE_NAND_PRIV_DRIVER_H

/**
 * @defgroup nvddk_one_nand Mux one nand adn flex mux one nand 
 * Controller hw interface API
 *
 * This is the mux one nand and flex mux one nand driver API.
 *
 * @ingroup nvddk_modules
 * @{
 *
 */

#include "nvcommon.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Combines the type of device whether it is mux one nand or flex mux one nand.
 */
typedef enum
{
    OneNandDeviceType_Unknown           = 0x0,
    OneNandDeviceType_MuxOneNand        = 0x1,
    OneNandDeviceType_FlexMuxOneNand    = 0x2,
    OneNandDeviceType_Force32 = 0x7FFFFFFF
} OneNandDeviceType;

/**
 * One Nand command status
 */
typedef enum
{
    OneNandCommandStatus_Ready          = 0x0,
    OneNandCommandStatus_Success        = 0x0,
    OneNandCommandStatus_Busy           = 0x1,
    OneNandCommandStatus_Error          = 0x2,
    OneNandCommandStatus_LockError      = 0x3,
    OneNandCommandStatus_EraseError     = 0x4,
    OneNandCommandStatus_EccError       = 0x5,
    OneNandCommandStatus_Force32        = 0x7FFFFFFF
} OneNandCommandStatus;

/**
 * One Nand device information.
 */
typedef struct
{
    NvU32 DeviceId;
    NvU32 ManufId;

    OneNandDeviceType DeviceType;

    NvBool IsDualCoreDevice;
    NvU32 TotalBlocks;
    NvU32 PagesPerBlock;
    NvU32 PageSize;
    NvU32 SectorPerPage;
    NvU32 SectorSize;
    NvU32 MainAreaSizePerSector;
    NvU32 SpareAreaSizePerSector;
    NvU32 MainAreaSizePerPage;
    NvU32 SpareAreaSizePerPage;
    NvU32 MainBootRamSize;
    NvU32 MainDataRamSize;
    NvU32 SpareBootRamSize;
    NvU32 SpareDataRamSize;
    
    NvU32 ResetTimingMs;
    NvU32 PageLoadTimeUs;
    NvU32 PageProgramTimeUs;
    NvU32 BlockLockUnlockTimeUs;
    NvU32 AllBlockUnlockTimeUs;
    NvU32 BlockEraseTimeMs;
} OneNandDeviceInfo;

/**
 * One Nand device interface details.
 */
typedef struct
{
    NvRmPhysAddr NorBaseAddress;
    NvU16 *pNorBaseAdd;
    NvU32 NorAddressSize;
    NvU32 DataRamMainAddress;
    NvU32 DataRamMainSize;
    NvU32 BootRamMainAddress;
    NvU32 BootRamMainSize;
    NvU32 DataRamSpareAddress;
    NvU32 DataRamSpareSize;
    NvU32 BootRamSpareAddress;
    NvU32 BootRamSpareSize;
} OneNandDeviceIntRegister;


/**
 * One Nand interrupt reason.
 */
typedef enum
{
  OneNandInterruptReason_None = 0x0,

  OneNandInterruptReason_Force32 = 0x7FFFFFFF
} OneNandInterruptReason;


// The structure of the function pointers to provide the interface to device 
// driver for the mux one nand and flex mux one nand driver.
typedef struct
{
    /**
     * Initialize the device interface.
     */
    void (*DevInitializeDeviceInterface)(OneNandDeviceIntRegister *pDevIntRegs);
    
    
    /**
     * Find whether the device is available on this interface or not.
     */
    NvU32 (*DevResetDevice)(OneNandDeviceIntRegister *pDevIntRegs, NvU32 DeviceChipSelect);
    
    
    NvBool (*DevIsDeviceReady)(OneNandDeviceIntRegister *pDevIntRegs);
        
    NvBool (*DevIsDeviceOperationCompleted)(OneNandDeviceIntRegister *pDevIntRegs);
    
    /**
     * Get the device information.
     */
    NvBool 
    (*DevGetDeviceInfo)(
        OneNandDeviceIntRegister *pDevIntRegs, 
        OneNandDeviceInfo *pDevInfo,
        NvU32 DeviceChipSelect);
    
    
    
    NvBool 
    (*DevConfigureHostInterface)(
        OneNandDeviceIntRegister *pDevIntRegs, 
        NvBool IsAsynch);
    
    
    void 
    (*DevAsynchReadFromBufferRam)(
        OneNandDeviceIntRegister *pDevIntRegs, 
        NvU8 *pReadMainBuffer,
        NvU8 *pReadSpareBuffer,
        NvU32 MainAreaSizeBytes,
        NvU32 SpareAreaSize);
    
    
    void 
    (*DevAsynchWriteIntoBufferRam)(
        OneNandDeviceIntRegister *pDevIntRegs, 
        NvU8 *pWriteMainBuffer,
        NvU8 *pWriteSpareBuffer,
        NvU32 MainAreaSizeBytes,
        NvU32 SpareAreaSize);
    
    NvU32 
    (*DevEraseBlock)(
        OneNandDeviceIntRegister *pDevIntRegs, 
        OneNandDeviceInfo *pDevInfo,
        NvU32 BlockNumber);
    
    
    OneNandCommandStatus (*DevGetEraseStatus)(OneNandDeviceIntRegister *pDevIntRegs);
    
    
    NvBool (*DevEraseSuspend)(OneNandDeviceIntRegister *pDevIntRegs);
    
    NvBool (*DevEraseResume)(OneNandDeviceIntRegister *pDevIntRegs);
    
    
    OneNandCommandStatus 
    (*DevSetBlockLockStatus)(
        OneNandDeviceIntRegister *pDevIntRegs, 
        OneNandDeviceInfo *pDevInfo,
        NvU32 BlockNumber, 
        NvBool IsLock);
    
    OneNandCommandStatus 
    (*DevUnlockAllBlockOfChip)(
        OneNandDeviceIntRegister *pDevIntRegs, 
        OneNandDeviceInfo *pDevInfo);

    NvBool 
    (*DevIsBlockLocked)(
        OneNandDeviceIntRegister *pDevIntRegs, 
        OneNandDeviceInfo *pDevInfo,
        NvU32 BlockNumber);

    
    OneNandCommandStatus 
    (*DevGetLoadStatus)(
        OneNandDeviceIntRegister *pDevIntRegs);
    
    
    OneNandCommandStatus 
    (*DevLoadBlockPage)(
        OneNandDeviceIntRegister *pDevIntRegs,
        OneNandDeviceInfo *pDevInfo,
        NvU32 BlockNumber,
        NvU32 PageNumber,
        NvBool IsWaitForCompletion);
       
    void 
    (*DevSelectDataRamFromBlock)(
        OneNandDeviceIntRegister *pDevIntRegs, 
        OneNandDeviceInfo *pDevInfo,
        NvU32 BlockNumber);
    
    
    OneNandCommandStatus 
    (*DevGetProgramStatus)(
        OneNandDeviceIntRegister *pDevIntRegs);
    
    
    OneNandCommandStatus 
    (*DevProgramBlockPage)(
        OneNandDeviceIntRegister *pDevIntRegs,
        OneNandDeviceInfo *pDevInfo,
        NvU32 BlockNumber,
        NvU32 PageNumber,
        NvBool IsWaitForCompletion);
    
   
    OneNandInterruptReason 
    (*DevGetInterruptReason)(
        OneNandDeviceIntRegister *pDevIntRegs);
    
} OneNandDevInterface, *OneNandDevInterfaceHandle;


NvBool 
IsDeviceAvailable(
    OneNandDeviceIntRegister *pDevIntRegs, 
    NvU32 DeviceChipSelect);

OneNandDeviceType GetDeviceType(OneNandDeviceIntRegister *pDevIntRegs);

/**
 * Initialize the one nand device interface.
 */
void 
NvRmPrivOneNandDeviceInterface(
    OneNandDevInterface *pDevInterface, 
    OneNandDeviceIntRegister *pDevIntReg,
    OneNandDeviceInfo *pDevInfo);

/** @}*/

#if defined(__cplusplus)
}
#endif

#endif  // INCLUDED_ONE_NAND_PRIV_DRIVER_H


