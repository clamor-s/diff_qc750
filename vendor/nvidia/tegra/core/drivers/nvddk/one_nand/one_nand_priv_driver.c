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
 *           Private functions for the one nand device interface</b>
 *
 * @b Description:  Implements  the private interfacing functions for the one
 * nand, mux one nand and flex mux one nand device hw interface.
 *
 */

#include "nvrm_drf.h"
#include "nvrm_hardware_access.h"
#include "nvassert.h"
#include "one_nand_register_def.h"
#include "one_nand_priv_driver.h"

#define ONE_NAND_READ16(pOneNandBaseAdd, RegName) \
        NV_READ16((pOneNandBaseAdd) + (ONE_NAND_##RegName##_0_ADDRESS))

#define ONE_NAND_WRITE16(pOneNandBaseAdd, RegName, Value) \
    do { \
        NV_WRITE16(((pOneNandBaseAdd) + (ONE_NAND_##RegName##_0_ADDRESS)), Value); \
    } while (0)

#define ONENAND_RESET_TIME_MS 10
static NvU16 s_ValidMainId[] = { 0x00EC}; // Samsung

/**
 * Initialize the device interface.
 */
static void InitializeDeviceInterface(OneNandDeviceIntRegister *pDevIntRegs)
{

    pDevIntRegs->BootRamMainAddress = ONE_NAND_FLEX_BOOT_MEMORY_MAIN_0_ADDRESS;
    pDevIntRegs->BootRamMainSize = 1*1024;
    pDevIntRegs->DataRamMainAddress = ONE_NAND_FLEX_DATA_MEMORY_MAIN_0_ADDRESS;
    pDevIntRegs->DataRamMainSize = 4*1094;
    
    pDevIntRegs->BootRamSpareAddress = ONE_NAND_FLEX_BOOT_MEMORY_SPARE_0_ADDRESS;
    pDevIntRegs->BootRamSpareSize = 32;
    pDevIntRegs->DataRamSpareAddress = ONE_NAND_FLEX_DATA_MEMORY_SPARE_0_ADDRESS;
    pDevIntRegs->DataRamSpareSize = 128;
}

/**
 * Find whether the device is available on this interface or not.
 */
static NvU32 
ResetDevice(
    OneNandDeviceIntRegister *pDevIntRegs, 
    NvU32 DeviceChipSelect)
{
    // Write reset command
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, COMMAND, 
                    NV_DRF_DEF(ONE_NAND, COMMAND, COMMAND, RESET_HOT));

    // Return maximum command complete time.
    return ONENAND_RESET_TIME_MS;
}

static NvBool IsDeviceReady(OneNandDeviceIntRegister *pDevIntRegs)
{
    NvU16 StatusReg;
    StatusReg = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, CONTROLLER_STATUS);
    if (StatusReg & NV_DRF_DEF(ONE_NAND, CONTROLLER_STATUS, ONGO, BUSY))
        return NV_FALSE;
    return NV_TRUE;
}

static NvBool IsDeviceOperationCompleted(OneNandDeviceIntRegister *pDevIntRegs)
{
    return NV_FALSE;
}



/**
 * Get the device information.
 */
static NvBool 
GetDeviceInfo(
    OneNandDeviceIntRegister *pDevIntRegs, 
    OneNandDeviceInfo *pDevInfo,
    NvU32 DeviceChipSelect)
{
    NvU16 ManId;
    NvU16 DevId;
    NvU32 TotalValidManId = NV_ARRAY_SIZE(s_ValidMainId);
    NvU32 Index;
    NvBool IsValidDevice = NV_FALSE;
    
    ManId = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, MAN_ID);
    DevId = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, DEV_ID);
    for (Index = 0; Index < TotalValidManId; ++Index)
    {
        if (s_ValidMainId[Index] == ManId)
        {
            IsValidDevice = NV_TRUE;
            break;
        }
    }
    if (!IsValidDevice)
        return NV_FALSE;
    switch(ManId)
    {
        case 0x00EC: // samsung
            pDevInfo->DeviceId = DevId;
            pDevInfo->ManufId = ManId;
            pDevInfo->IsDualCoreDevice = NV_FALSE;
            pDevInfo->TotalBlocks = 1000;
            pDevInfo->PagesPerBlock = 32;
            pDevInfo->PageSize = 4*1024;
            pDevInfo->SectorPerPage = 8;
            pDevInfo->SectorSize = 512;
            pDevInfo->MainAreaSizePerSector = 512;
            pDevInfo->SpareAreaSizePerSector = 32;
            pDevInfo->MainBootRamSize = 1024;
            pDevInfo->MainDataRamSize = 4096;
            pDevInfo->SpareBootRamSize = 32;
            pDevInfo->SpareDataRamSize = 128;
            pDevInfo->ResetTimingMs = 10;
            pDevInfo->PageLoadTimeUs = 100;
            pDevInfo->PageProgramTimeUs = 100;
            pDevInfo->BlockLockUnlockTimeUs = 30;
            pDevInfo->AllBlockUnlockTimeUs = 30;
            pDevInfo->BlockEraseTimeMs = 10;
            return NV_TRUE;

        default:
            break;
    }
    return NV_FALSE;
}


static NvBool 
ConfigureHostInterface(
    OneNandDeviceIntRegister *pDevIntRegs, 
    NvBool IsAsynch)
{
    // TODO: Implementation is due
    return NV_TRUE;
}

static void 
AsynchReadFromBufferRam(
    OneNandDeviceIntRegister *pDevIntRegs, 
    NvU8 *pReadMainBuffer,
    NvU8 *pReadSpareBuffer,
    NvU32 MainAreaSizeBytes,
    NvU32 SpareAreaSize)
{
    NvU16 *pMainBuffer = (NvU16 *)pReadMainBuffer;
    NvU16 *pSpareBuffer = (NvU16 *)pReadSpareBuffer;
    NvU32 Index;
    NvU32 ReadSize;

    if (pReadMainBuffer)
    {
        ReadSize = MainAreaSizeBytes >> 1;
        for (Index = 0; Index < ReadSize; ++Index)
            *pMainBuffer++ = ONE_NAND_READ16((pDevIntRegs->pNorBaseAdd + Index), 
                                                FLEX_DATA_MEMORY_MAIN);
    }
    if (pReadSpareBuffer)
    {
        ReadSize = SpareAreaSize >> 1;
        for (Index = 0; Index < ReadSize; ++Index)
            *pSpareBuffer++ = ONE_NAND_READ16((pDevIntRegs->pNorBaseAdd + Index), 
                                                FLEX_DATA_MEMORY_SPARE);
    }
}

static void AsynchWriteIntoBufferRam(
    OneNandDeviceIntRegister *pDevIntRegs, 
    NvU8 *pWriteMainBuffer,
    NvU8 *pWriteSpareBuffer,
    NvU32 MainAreaSizeBytes,
    NvU32 SpareAreaSize)
{
    NvU16 *pMainBuffer = (NvU16 *)pWriteMainBuffer;
    NvU16 *pSpareBuffer = (NvU16 *)pWriteSpareBuffer;
    NvU32 Index;
    NvU32 ReadSize;

    if (pWriteMainBuffer)
    {
        ReadSize = MainAreaSizeBytes >> 1;
        for (Index = 0; Index < ReadSize; ++Index)
        {
            ONE_NAND_WRITE16((pDevIntRegs->pNorBaseAdd + Index),
                FLEX_DATA_MEMORY_MAIN, *pMainBuffer);
            pMainBuffer++;
        }
    }
    if (pWriteSpareBuffer)
    {
        ReadSize = SpareAreaSize >> 1;
        for (Index = 0; Index < ReadSize; ++Index)
        {
            ONE_NAND_WRITE16((pDevIntRegs->pNorBaseAdd + Index),
                FLEX_DATA_MEMORY_SPARE, *pSpareBuffer);
            pSpareBuffer++;
        }
    }
}

static void 
SetBlockPageAddress(
    OneNandDeviceIntRegister *pDevIntRegs, 
    OneNandDeviceInfo *pDevInfo,
    NvU32 BlockNumber,
    NvU32 PageNumber)
{
    NvU16 StartAdd1 = 0;
    NvU16 StartAdd2 = 0;
    NvU16 StartAdd8 = 0;
    NvBool IsTopCore = NV_FALSE;
    NV_ASSERT(BlockNumber <= pDevInfo->TotalBlocks);

    // If dual core device and the block number is more than half of total 
    // block of device then it is the top core.
    if ((pDevInfo->IsDualCoreDevice) && (BlockNumber >= pDevInfo->TotalBlocks/2))
        IsTopCore = NV_TRUE;

    // Write the DFS, FBA of flash
    StartAdd1 = (NvU32)(NV_FLD_SET_DRF_NUM(ONE_NAND, START_ADD_1, FBA, BlockNumber, StartAdd1));
    if (IsTopCore)
        StartAdd1 = NV_FLD_SET_DRF_NUM(ONE_NAND, START_ADD_1, DFS, 1, StartAdd1);

    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, START_ADD_1, StartAdd1);
    
    // Select Dataram for DDP
    if (IsTopCore)
        StartAdd2 = NV_FLD_SET_DRF_NUM(ONE_NAND, START_ADD_2, DBS, 1, StartAdd2);

    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, START_ADD_2, StartAdd2);
    
    // Write the page address
    StartAdd8 = (NvU32)(NV_FLD_SET_DRF_NUM(ONE_NAND, START_ADD_8, FPA, PageNumber, StartAdd8));
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, START_ADD_8, StartAdd8);
}

static NvU32 
EraseBlock(
    OneNandDeviceIntRegister *pDevIntRegs, 
    OneNandDeviceInfo *pDevInfo,
    NvU32 BlockNumber)
{
    SetBlockPageAddress(pDevIntRegs, pDevInfo, BlockNumber, 0);

    // Write erase command
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, COMMAND, 
                    NV_DRF_DEF(ONE_NAND, COMMAND, COMMAND, BLOCK_ERASE));
    return 6;              
 }

static OneNandCommandStatus 
GetEraseStatus(
    OneNandDeviceIntRegister *pDevIntRegs)
{
    NvU16 Status;
    
    Status = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, CONTROLLER_STATUS);
    if (NV_DRF_VAL(ONE_NAND, CONTROLLER_STATUS, ONGO, Status) == 
                                ONE_NAND_CONTROLLER_STATUS_0_ONGO_READY)
    {
        // ONE_NAND_CONTROLLER_STATUS_0_ERROR_FAIL
        if (Status & NV_DRF_DEF(ONE_NAND, CONTROLLER_STATUS, ERROR, FAIL))
            return OneNandCommandStatus_EraseError;
        return OneNandCommandStatus_Success;    
    }
    return OneNandCommandStatus_Busy;
}

static NvBool 
EraseSuspend(
    OneNandDeviceIntRegister *pDevIntRegs)
{
    // TODO: Implementation is due
    return NV_TRUE;
}
static NvBool 
EraseResume(
    OneNandDeviceIntRegister *pDevIntRegs)
{
    // TODO: Implementation is due
    return NV_TRUE;
}

static OneNandCommandStatus 
SetBlockLockStatus(
    OneNandDeviceIntRegister *pDevIntRegs, 
    OneNandDeviceInfo *pDevInfo,
    NvU32 BlockNumber, 
    NvBool IsLock)
{
    NvU16 IntStatus;
    NvU16 Status;

    NvU16 StartAdd1 = 0;
    NvU16 StartAdd2 = 0;
    NvU16 SbaRegister = 0;
    NvBool IsTopCore = NV_FALSE;
    NV_ASSERT(BlockNumber <= pDevInfo->TotalBlocks);
    
    // If dual core device and the block number is more than half of total 
    // block of device then it is the top core.
    if ((pDevInfo->IsDualCoreDevice) && (BlockNumber >= pDevInfo->TotalBlocks/2))
        IsTopCore = NV_TRUE;
        
    
    // Write the DFS, FBA of flash
    StartAdd1 = (NvU32)(NV_FLD_SET_DRF_NUM(ONE_NAND, START_ADD_1, FBA, BlockNumber, StartAdd1));
    if (IsTopCore)
        StartAdd1 = NV_FLD_SET_DRF_NUM(ONE_NAND, START_ADD_1, DFS, 1, StartAdd1);
    
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, START_ADD_1, StartAdd1);
    
    // Select Dataram for DDP
    if (IsTopCore)
        StartAdd2 = NV_FLD_SET_DRF_NUM(ONE_NAND, START_ADD_2, DBS, 1, StartAdd2);
    
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, START_ADD_2, StartAdd2);

    // Write SBA of flash
    SbaRegister = (NvU32)(NV_FLD_SET_DRF_NUM(ONE_NAND, START_BLOCK_ADD, SBA, BlockNumber,
                                            SbaRegister));
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, START_BLOCK_ADD, SbaRegister);
    

    // Write 0 to interrupt register
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, INT_STATUS, 0);

    // Write lock/unlock command
    if (IsLock)
        ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, COMMAND, 
                        NV_DRF_DEF(ONE_NAND, COMMAND, COMMAND, LOCK_BLOCK));
    else
        ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, COMMAND, 
                        NV_DRF_DEF(ONE_NAND, COMMAND, COMMAND, UNLOCK_BLOCK));
                        
    // Wait for INT register low to high transition
    do
    {
        IntStatus = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, INT_STATUS);
        if (NV_DRF_VAL(ONE_NAND, INT_STATUS, INT, IntStatus))
            break;
    } while(1);

    do 
    {
        Status = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, CONTROLLER_STATUS);
        if (NV_DRF_VAL(ONE_NAND, CONTROLLER_STATUS, ONGO, Status) == 
                                    ONE_NAND_CONTROLLER_STATUS_0_ONGO_READY)
        {
            // ONE_NAND_CONTROLLER_STATUS_0_ERROR_FAIL
            if (Status & NV_DRF_DEF(ONE_NAND, CONTROLLER_STATUS, ERROR, FAIL))
                return OneNandCommandStatus_Error;
            break;    
        }
    } while(1);
    
    // Check for the write protection register.
    Status = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, WRITE_PROTECT_STATUS);
    if (IsLock)
    {
        if (NV_DRF_VAL(ONE_NAND, WRITE_PROTECT_STATUS, LS, Status) ==
                            ONE_NAND_WRITE_PROTECT_STATUS_0_LS_LOCKED)
            return OneNandCommandStatus_Success;
        return OneNandCommandStatus_Error;
    }
    else
    {
        if (NV_DRF_VAL(ONE_NAND, WRITE_PROTECT_STATUS, US, Status) ==
                            ONE_NAND_WRITE_PROTECT_STATUS_0_US_UNLOCKED)
            return OneNandCommandStatus_Success;
        return OneNandCommandStatus_Error;
    }
}
static OneNandCommandStatus 
UnlockAllBlockOfChip(
    OneNandDeviceIntRegister *pDevIntRegs, 
    OneNandDeviceInfo *pDevInfo)
{
    NvU16 IntStatus;
    NvU16 Status;

    NvU16 StartAdd1 = 0;
    NvU16 StartAdd2 = 0;
    NvU16 SbaRegister = 0;
    
    // Write the DFS, FBA of flash
    StartAdd1 = NV_FLD_SET_DRF_NUM(ONE_NAND, START_ADD_1, FBA, 0, StartAdd1);
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, START_ADD_1, StartAdd1);
    
    // Select Dataram for DDP
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, START_ADD_2, StartAdd2);

    // Write SBA of flsh
    SbaRegister = NV_FLD_SET_DRF_NUM(ONE_NAND, START_BLOCK_ADD, SBA, 0,
                                            SbaRegister);
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, START_BLOCK_ADD, SbaRegister);
    

    // Write 0 to interrupt register
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, INT_STATUS, 0);

    // Write all block unlock command
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, COMMAND, 
                    NV_DRF_DEF(ONE_NAND, COMMAND, COMMAND, UNLOCK_ALL_BLOCK));

    // Wait for INT register low to high transition
    do
    {
        IntStatus = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, INT_STATUS);
        if (NV_DRF_VAL(ONE_NAND, INT_STATUS, INT, IntStatus))
            break;
    } while(1);
    
    // Check for the DQ10 from controller status register
    Status = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, CONTROLLER_STATUS);
    if (NV_DRF_VAL(ONE_NAND, CONTROLLER_STATUS, ONGO, Status) == 
                                ONE_NAND_CONTROLLER_STATUS_0_ONGO_READY)
    {
        // ONE_NAND_CONTROLLER_STATUS_0_ERROR_FAIL
        if (Status & NV_DRF_DEF(ONE_NAND, CONTROLLER_STATUS, ERROR, FAIL))
            return OneNandCommandStatus_Error;
    }

    // Check for the write protection register.
    Status = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, WRITE_PROTECT_STATUS);
    if (NV_DRF_VAL(ONE_NAND, WRITE_PROTECT_STATUS, US, Status) ==
                        ONE_NAND_WRITE_PROTECT_STATUS_0_US_UNLOCKED)
        return OneNandCommandStatus_Success;
    return OneNandCommandStatus_Error;
}

static NvBool 
IsBlockLocked(
    OneNandDeviceIntRegister *pDevIntRegs, 
    OneNandDeviceInfo *pDevInfo,
    NvU32 BlockNumber)
{
    NvU16 Status;
    
    SetBlockPageAddress(pDevIntRegs, pDevInfo, BlockNumber, 0);
    // Check for the write protection register.
    Status = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, WRITE_PROTECT_STATUS);
    if ((NV_DRF_VAL(ONE_NAND, WRITE_PROTECT_STATUS, LS, Status) ==
                        ONE_NAND_WRITE_PROTECT_STATUS_0_LS_LOCKED) ||
        (NV_DRF_VAL(ONE_NAND, WRITE_PROTECT_STATUS, LTS, Status) ==
                        ONE_NAND_WRITE_PROTECT_STATUS_0_LTS_LOCK_TIGHT))
        return NV_TRUE;
        
    return NV_FALSE;        
}


static OneNandCommandStatus 
MuxOneNandGetLoadStatus(
    OneNandDeviceIntRegister *pDevIntRegs)
{
    NvU16 IntStatus;
    NvU16 StatusReg;
    
    // Wait for INT register low to high transition
    IntStatus = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, INT_STATUS);
    if (!NV_DRF_VAL(ONE_NAND, INT_STATUS, INT, IntStatus))
        return OneNandCommandStatus_Busy;

    // check for the DQ10 from controller status register
    StatusReg = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, CONTROLLER_STATUS);
    if (NV_DRF_VAL(ONE_NAND, CONTROLLER_STATUS, ERROR, StatusReg) == 
                    ONE_NAND_CONTROLLER_STATUS_0_ERROR_FAIL)
        return OneNandCommandStatus_Error;
    else    
        return OneNandCommandStatus_Success;
}

static OneNandCommandStatus 
MuxOneNandLoadBlockPage(
    OneNandDeviceIntRegister *pDevIntRegs,
    OneNandDeviceInfo *pDevInfo,
    NvU32 BlockNumber,
    NvU32 PageNumber,
    NvBool IsWaitForCompletion)
{
    NvU16 SectorCountReg = 0;
    NvU16 SysConfig1 = 0;
    OneNandCommandStatus CommandStatus;
    
    SetBlockPageAddress(pDevIntRegs, pDevInfo, BlockNumber, PageNumber);

    // Write BSA/BSC of the data ram
    SectorCountReg = NV_FLD_SET_DRF_DEF(ONE_NAND, SECTOR_COUNT, BSA3, 
                                            DATARAM, SectorCountReg); 
    SectorCountReg = NV_FLD_SET_DRF_DEF(ONE_NAND, SECTOR_COUNT, BSA2, 
                                            DATARAM0, SectorCountReg); 
    SectorCountReg = NV_FLD_SET_DRF_DEF(ONE_NAND, SECTOR_COUNT, BSC_COUNT, 
                                            ALL_SECTOR, SectorCountReg); 
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, SECTOR_COUNT, SectorCountReg);

    // Enable Ecc
    SysConfig1 = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, SYSTEM_CONFIG_1);
    SysConfig1 = NV_FLD_SET_DRF_DEF(ONE_NAND, SYSTEM_CONFIG_1, ECC, ENABLE, 
                                                                 SysConfig1);
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, SYSTEM_CONFIG_1, SysConfig1);
    
    // Clear interrupt register
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, INT_STATUS, 0);

    // Write Load command
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, COMMAND, 
                NV_DRF_DEF(ONE_NAND, COMMAND, COMMAND, LOAD_PAGE_INTO_BUFFER));

    if (!IsWaitForCompletion)
        return OneNandCommandStatus_Success;

    do
    {
        CommandStatus = MuxOneNandGetLoadStatus(pDevIntRegs);
    } while (CommandStatus == OneNandCommandStatus_Busy);

    return CommandStatus;
}

static OneNandCommandStatus 
FlexMuxOneNandGetLoadStatus(
    OneNandDeviceIntRegister *pDevIntRegs)
{
    NvU16 IntStatus;
    NvU16 EccStatusReg;
    NvU32 Index;

    // Wait for INT register low to high transition
    IntStatus = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, INT_STATUS);
    if (!NV_DRF_VAL(ONE_NAND, INT_STATUS, INT, IntStatus))
        return OneNandCommandStatus_Busy;

    // Check for Ecc error
    for (Index = 0; Index < 4; ++Index)
    {
        EccStatusReg = ONE_NAND_READ16((pDevIntRegs->pNorBaseAdd + Index), ECC_1);
        // ONE_NAND_ECC_1_0_FLEX_SECTOR0_ERROR_MAX_ERROR
        if (NV_DRF_VAL(ONE_NAND, ECC_1, FLEX_SECTOR0_ERROR, EccStatusReg) >= 
                    ONE_NAND_ECC_1_0_FLEX_SECTOR0_ERROR_MAX_ERROR)
            return OneNandCommandStatus_EccError;

        if (NV_DRF_VAL(ONE_NAND, ECC_1, FLEX_SECTOR1_ERROR, EccStatusReg) >= 
                    ONE_NAND_ECC_1_0_FLEX_SECTOR1_ERROR_MAX_ERROR)
            return OneNandCommandStatus_EccError;
    }
    return OneNandCommandStatus_Success;
}

static OneNandCommandStatus 
FlexMuxOneNandLoadBlockPage(
    OneNandDeviceIntRegister *pDevIntRegs,
    OneNandDeviceInfo *pDevInfo,
    NvU32 BlockNumber,
    NvU32 PageNumber,
    NvBool IsWaitForCompletion)
{
    NvU16 SectorCountReg = 0;
    NvU16 SysConfig1 = 0;
    OneNandCommandStatus CommandStatus;

    // Set block/DBS/FPA 
    SetBlockPageAddress(pDevIntRegs, pDevInfo, BlockNumber, PageNumber);

    // Write BSA/BSC of the data ram
    SectorCountReg = NV_FLD_SET_DRF_DEF(ONE_NAND, SECTOR_COUNT, BSA3, 
                                            DATARAM, SectorCountReg); 
    SectorCountReg = NV_FLD_SET_DRF_DEF(ONE_NAND, SECTOR_COUNT, BSA2, 
                                            DATARAM0, SectorCountReg); 
    SectorCountReg = NV_FLD_SET_DRF_DEF(ONE_NAND, SECTOR_COUNT, BSC_COUNT, 
                                            ALL_SECTOR, SectorCountReg); 
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, SECTOR_COUNT, SectorCountReg);

    // Write System Configuration Register:Enable Ecc
    SysConfig1 = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, SYSTEM_CONFIG_1);
    SysConfig1 = NV_FLD_SET_DRF_DEF(ONE_NAND, SYSTEM_CONFIG_1, ECC, ENABLE, 
                                                                 SysConfig1);
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, SYSTEM_CONFIG_1, SysConfig1);
    
    // Clear interrupt register
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, INT_STATUS, 0);

    // Write Load command
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, COMMAND, 
                    NV_DRF_DEF(ONE_NAND, COMMAND, COMMAND, LOAD_PAGE_INTO_BUFFER));

    if (!IsWaitForCompletion)
        return OneNandCommandStatus_Success;

    do
    {
        CommandStatus = FlexMuxOneNandGetLoadStatus(pDevIntRegs);
    } while (CommandStatus == OneNandCommandStatus_Busy);

    return CommandStatus;
}

static void 
SelectDataRamFromBlock(
    OneNandDeviceIntRegister *pDevIntRegs, 
    OneNandDeviceInfo *pDevInfo,
    NvU32 BlockNumber)
{
    NvBool IsTopCore = NV_FALSE;
    NvU16 StartAdd2 = 0;
    
    NV_ASSERT(BlockNumber <= pDevInfo->TotalBlocks);

    // If dual core device and the block number is more than half of total 
    // block of device then it is the top core.
    if ((pDevInfo->IsDualCoreDevice) && (BlockNumber >= pDevInfo->TotalBlocks/2))
        IsTopCore = NV_TRUE;

    // Select Dataram for DDP
    if (IsTopCore)
        StartAdd2 = NV_FLD_SET_DRF_NUM(ONE_NAND, START_ADD_2, DBS, 1, StartAdd2);

    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, START_ADD_2, StartAdd2);
}

static OneNandCommandStatus 
MuxOneNandGetProgramStatus(
    OneNandDeviceIntRegister *pDevIntRegs)
{
    NvU16 IntStatus;
    NvU16 StatusReg;
    
    // Wait for INT register low to high transition
    IntStatus = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, INT_STATUS);
    if (!NV_DRF_VAL(ONE_NAND, INT_STATUS, INT, IntStatus))
        return OneNandCommandStatus_Busy;

    StatusReg = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, CONTROLLER_STATUS);

    // Check for the Program lock error
    // DQ[6] should be 1 for success
    if (!NV_DRF_VAL(ONE_NAND, INT_STATUS, WI, IntStatus))
    {
        // ONE_NAND_CONTROLLER_STATUS_0_LOCK_LOCKED
        if (NV_DRF_VAL(ONE_NAND, CONTROLLER_STATUS, LOCK, StatusReg) ==
                ONE_NAND_CONTROLLER_STATUS_0_LOCK_LOCKED)
            return OneNandCommandStatus_LockError;
        else    
            return OneNandCommandStatus_Error;
    }

    // check for the DQ10 from controller status register
    if (NV_DRF_VAL(ONE_NAND, CONTROLLER_STATUS, ERROR, StatusReg) ==
                ONE_NAND_CONTROLLER_STATUS_0_ERROR_FAIL)
        return OneNandCommandStatus_Error;
    else    
        return OneNandCommandStatus_Success;
}

static OneNandCommandStatus 
MuxOneNandProgramBlockPage(
    OneNandDeviceIntRegister *pDevIntRegs,
    OneNandDeviceInfo *pDevInfo,
    NvU32 BlockNumber,
    NvU32 PageNumber,
    NvBool IsWaitForCompletion)
{
    NvU16 SectorCountReg = 0;
    NvU16 SysConfig1 = 0;
    OneNandCommandStatus CommandStatus;

    // Set block/Page/DBS.
    SetBlockPageAddress(pDevIntRegs, pDevInfo, BlockNumber, PageNumber);

    // Write BSA/BSC of the data ram
    SectorCountReg = NV_FLD_SET_DRF_DEF(ONE_NAND, SECTOR_COUNT, BSA3, 
                                            DATARAM, SectorCountReg); 
    SectorCountReg = NV_FLD_SET_DRF_DEF(ONE_NAND, SECTOR_COUNT, BSA2, 
                                            DATARAM0, SectorCountReg); 
    SectorCountReg = NV_FLD_SET_DRF_DEF(ONE_NAND, SECTOR_COUNT, BSC_COUNT, 
                                            ALL_SECTOR, SectorCountReg); 
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, SECTOR_COUNT, SectorCountReg);

    // Enable Ecc
    SysConfig1 = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, SYSTEM_CONFIG_1);
    SysConfig1 = NV_FLD_SET_DRF_DEF(ONE_NAND, SYSTEM_CONFIG_1, ECC, ENABLE, 
                                                                 SysConfig1);
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, SYSTEM_CONFIG_1, SysConfig1);
    
    // Clear interrupt register
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, INT_STATUS, 0);

    // Write program command
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, COMMAND, 
                    NV_DRF_DEF(ONE_NAND, COMMAND, COMMAND, PROGRAM_PAGE));

    if (!IsWaitForCompletion)
        return OneNandCommandStatus_Success;

    do
    {
        CommandStatus = MuxOneNandGetProgramStatus(pDevIntRegs);
    } while (CommandStatus == OneNandCommandStatus_Busy);

    return CommandStatus;
}

static OneNandCommandStatus 
FlexMuxOneNandGetProgramStatus(
    OneNandDeviceIntRegister *pDevIntRegs)
{
    NvU16 IntStatus;
    NvU16 StatusReg;

    // Wait for INT register low to high transition
    IntStatus = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, INT_STATUS);
    if (!NV_DRF_VAL(ONE_NAND, INT_STATUS, INT, IntStatus))
        return OneNandCommandStatus_Busy;

    // Read controller status register and check for the DQ10
    StatusReg = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, CONTROLLER_STATUS);
    if (NV_DRF_VAL(ONE_NAND, CONTROLLER_STATUS, ERROR, StatusReg))
        return OneNandCommandStatus_Error;
    else    
        return OneNandCommandStatus_Success;
}

static OneNandCommandStatus 
FlexMuxOneNandProgramBlockPage(
    OneNandDeviceIntRegister *pDevIntRegs,
    OneNandDeviceInfo *pDevInfo,
    NvU32 BlockNumber,
    NvU32 PageNumber,
    NvBool IsWaitForCompletion)
{
    NvU16 SectorCountReg = 0;
    NvU16 SysConfig1 = 0;
    OneNandCommandStatus CommandStatus;
    NvU32 WProtectionStatus;

    // Set block/Page/DBS.
    SetBlockPageAddress(pDevIntRegs, pDevInfo, BlockNumber, PageNumber);
    
    // Read Write Protection Status
    WProtectionStatus = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, WRITE_PROTECT_STATUS);
    if ((NV_DRF_VAL(ONE_NAND, WRITE_PROTECT_STATUS, STATUS, WProtectionStatus) !=
                ONE_NAND_WRITE_PROTECT_STATUS_0_STATUS_PROGRAM_ALLOWED))
        return OneNandCommandStatus_LockError;           
    
    // Write BSA/BSC of the data ram
    SectorCountReg = NV_FLD_SET_DRF_DEF(ONE_NAND, SECTOR_COUNT, BSA3, 
                                            DATARAM, SectorCountReg); 
    SectorCountReg = NV_FLD_SET_DRF_DEF(ONE_NAND, SECTOR_COUNT, BSA2, 
                                            DATARAM0, SectorCountReg); 
    SectorCountReg = NV_FLD_SET_DRF_DEF(ONE_NAND, SECTOR_COUNT, BSC_COUNT, 
                                            ALL_SECTOR, SectorCountReg); 
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, SECTOR_COUNT, SectorCountReg);

    // Write System Configuration Register:Enable Ecc
    SysConfig1 = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, SYSTEM_CONFIG_1);
    SysConfig1 = NV_FLD_SET_DRF_DEF(ONE_NAND, SYSTEM_CONFIG_1, ECC, ENABLE, 
                                                                 SysConfig1);
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, SYSTEM_CONFIG_1, SysConfig1);
    
    // Clear interrupt register
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, INT_STATUS, 0);

    // Write program command
    ONE_NAND_WRITE16(pDevIntRegs->pNorBaseAdd, COMMAND, 
                    NV_DRF_DEF(ONE_NAND, COMMAND, COMMAND, PROGRAM_PAGE));

    if (!IsWaitForCompletion)
        return OneNandCommandStatus_Success;

    do
    {
        CommandStatus = FlexMuxOneNandGetProgramStatus(pDevIntRegs);
    } while (CommandStatus == OneNandCommandStatus_Busy);

    return CommandStatus;
}


static OneNandInterruptReason 
GetInterruptReason(
    OneNandDeviceIntRegister *pDevIntRegs)
{
    return OneNandInterruptReason_None;
}

/**
 * Find whether the device is available on this interface or not.
 */
NvBool 
IsDeviceAvailable(
    OneNandDeviceIntRegister *pDevIntRegs, 
    NvU32 DeviceChipSelect)
{
    NvU32 ManId;
    NvU32 TotalValidManId = NV_ARRAY_SIZE(s_ValidMainId);
    NvU32 Index;
    
    ManId = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, MAN_ID);
    for (Index = 0; Index < TotalValidManId; ++Index)
    {
        if (s_ValidMainId[Index] == ManId)
            return NV_TRUE;
    }
    return NV_FALSE;
}

OneNandDeviceType GetDeviceType(OneNandDeviceIntRegister *pDevIntRegs)
{
    NvU32 ManId;
    NvU32 DevId;

    ManId = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, MAN_ID);
    DevId = ONE_NAND_READ16(pDevIntRegs->pNorBaseAdd, DEV_ID);

    switch(ManId)
    {
        case 0xEC: // samsung
        
            // ONE_NAND_DEV_ID_0_SEPERATION_FLEX
            if (NV_DRF_VAL(ONE_NAND, DEV_ID, SEPERATION, DevId) ==
                            ONE_NAND_DEV_ID_0_SEPERATION_FLEX)
                return OneNandDeviceType_FlexMuxOneNand;

            return OneNandDeviceType_MuxOneNand;
            
        default:
            break;
            
    }
    return OneNandDeviceType_Unknown;
}

/**
 * Initialize the one nand device interface.
 */
void 
NvRmPrivOneNandDeviceInterface(
    OneNandDevInterface *pDevInterface, 
    OneNandDeviceIntRegister *pDevIntReg,
    OneNandDeviceInfo *pDevInfo)
{
    pDevInterface->DevInitializeDeviceInterface = InitializeDeviceInterface;
    pDevInterface->DevResetDevice = ResetDevice;
    pDevInterface->DevIsDeviceReady = IsDeviceReady;
    pDevInterface->DevIsDeviceOperationCompleted = IsDeviceOperationCompleted;
    pDevInterface->DevGetDeviceInfo = GetDeviceInfo;
    pDevInterface->DevConfigureHostInterface = ConfigureHostInterface;
    pDevInterface->DevConfigureHostInterface = ConfigureHostInterface;
    pDevInterface->DevAsynchReadFromBufferRam = AsynchReadFromBufferRam;
    pDevInterface->DevAsynchWriteIntoBufferRam = AsynchWriteIntoBufferRam;
    pDevInterface->DevEraseBlock = EraseBlock;
    pDevInterface->DevGetEraseStatus = GetEraseStatus;
    pDevInterface->DevEraseSuspend = EraseSuspend;
    pDevInterface->DevEraseResume = EraseResume;
    pDevInterface->DevSetBlockLockStatus = SetBlockLockStatus;
    pDevInterface->DevSetBlockLockStatus = SetBlockLockStatus;
    pDevInterface->DevUnlockAllBlockOfChip = UnlockAllBlockOfChip;
    pDevInterface->DevIsBlockLocked = IsBlockLocked;
    pDevInterface->DevSelectDataRamFromBlock = SelectDataRamFromBlock;
    pDevInterface->DevGetInterruptReason = GetInterruptReason;
    if (pDevInfo->DeviceType == OneNandDeviceType_MuxOneNand)
    {
        pDevInterface->DevGetLoadStatus = MuxOneNandGetLoadStatus;
        pDevInterface->DevLoadBlockPage = MuxOneNandLoadBlockPage;
        pDevInterface->DevGetProgramStatus = MuxOneNandGetProgramStatus;
        pDevInterface->DevProgramBlockPage = MuxOneNandProgramBlockPage;
    }
    else 
    {
        pDevInterface->DevGetLoadStatus = FlexMuxOneNandGetLoadStatus;
        pDevInterface->DevLoadBlockPage = FlexMuxOneNandLoadBlockPage;
        pDevInterface->DevGetProgramStatus = FlexMuxOneNandGetProgramStatus;
        pDevInterface->DevProgramBlockPage = FlexMuxOneNandProgramBlockPage;
    }
}

