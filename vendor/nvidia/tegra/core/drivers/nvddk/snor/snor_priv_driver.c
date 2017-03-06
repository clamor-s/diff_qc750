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
 *           Private functions for the SNOR device interface</b>
 *
 * @b Description:  Implements  the private interfacing functions for the SNOR
 * hw interface.
 *
 */

#include "nvrm_drf.h"
#include "nvrm_hardware_access.h"
#include "nvassert.h"
#include "snor_priv_driver.h"

#define SNOR_RESET_TIME_MS 1

/* NVTODO : Only Spansion support for now */
static NvU16 s_ValidMainId[] = { 0x0001, };  // Spansion

/**
 * Initialize the device interface.
 */
static void InitializeDeviceInterface(SnorDeviceIntRegister *pDevIntRegs)
{

}


/**
 * Find whether the device is available on this interface or not.
 */
static NvU32
ResetDevice(
    SnorDeviceIntRegister *pDevIntRegs,
    NvU32 DeviceChipSelect)
{
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x00, 0xF0);
    // Return maximum command complete time.
    return SNOR_RESET_TIME_MS;
}

static NvBool IsDeviceReady(SnorDeviceIntRegister *pDevIntRegs)
{
    // No such status needed for SNOR
    return NV_TRUE;
}

SnorCommandStatus
SpansionGetNumBlocks(
        SnorDeviceIntRegister *pDevIntRegs,
        NvU32 *NumBlocks)
{
    NvU32 Size ;
    // Reset the Device to get it back into read array mode
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x00, 0xF0);
    // Enter CFI mode
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x55, 0x98);
    Size = SNOR_FLASH_READ16(pDevIntRegs->pNorBaseAdd,0x27);
    // CFI Query returns N. Device Size is 2^N byte
    *NumBlocks = ((1 << Size)/SPANSION_SECTOR_SIZE);
    // Reset the Device to get it back into read array mode
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x00, 0xF0);
    //FIXME: return something useful
    return 0;
}


/**
 * Get the device information.
 */
static NvBool
GetDeviceInfo(
    SnorDeviceIntRegister *pDevIntRegs,
    SnorDeviceInfo *pDevInfo,
    NvU32 DeviceChipSelect)
{
    NvU16 ManId;
    NvU16 DevId;
    NvU32 TotalValidManId = NV_ARRAY_SIZE(s_ValidMainId);
    NvU32 Index;
    NvBool IsValidDevice = NV_FALSE;

    /* NVTODO : We will add support for Spansion
     * device now. Later we need to check all
     * the devices supported
     */

    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x555, 0xAA);
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x2AA, 0x55);
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x555, 0x90);
    /* Read Manufacture ID */
    ManId = SNOR_FLASH_READ16(pDevIntRegs->pNorBaseAdd,0x0) & 0xFF;
    /* Read Device ID */
    DevId = SNOR_FLASH_READ16(pDevIntRegs->pNorBaseAdd,0x1) & 0xFFFF;
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x00, 0xF0);
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
    // NVTODO : In case of SNOR, page size = sector size
    switch(ManId)
    {
        case 0x0001: // Spansion
            pDevInfo->DeviceId = DevId;
            pDevInfo->ManufId = ManId;
            SpansionGetNumBlocks(pDevIntRegs, &pDevInfo->TotalBlocks) ;
            pDevInfo->PagesPerBlock = 16;
            pDevInfo->PageSize = 2048;
            pDevInfo->SectorPerPage = 1;
            pDevInfo->SectorSize = 2048;
            pDevInfo->BusWidth = 16;
            /* Not Sure if these are needed for SNOR */
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
    SnorDeviceIntRegister *pDevIntRegs,
    NvBool IsAsynch)
{
    return NV_TRUE;
}

static SnorInterruptReason
GetInterruptReason(
    SnorDeviceIntRegister *pDevIntRegs)
{
    return SnorInterruptReason_None;
}

/**
 * Find whether the device is available on this interface or not.
 */
NvBool
IsSnorDeviceAvailable(
    SnorDeviceIntRegister *pDevIntRegs,
    NvU32 DeviceChipSelect)
{
    NvU16 ManId=0;
    NvU16 DevId=0;
    NvU32 TotalValidManId = NV_ARRAY_SIZE(s_ValidMainId);
    NvU32 Index;
    NvBool IsValidDevice = NV_FALSE;
    volatile int nvbrk=0;

    /* NVTODO : We will add support for Spansion
     * device now. Later we need to check all
     * the devices supported
     */

    while(nvbrk==1);
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x00, 0xF0);
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x555, 0xAA);
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x2AA, 0x55);
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x555, 0x90);

    /* Read Manufacture ID */
    ManId = SNOR_FLASH_READ16(pDevIntRegs->pNorBaseAdd,0x0) & 0xFF;
    /* Read Device ID */
    DevId = SNOR_FLASH_READ16(pDevIntRegs->pNorBaseAdd,0x1) & 0xFFFF;
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x00, 0xF0);
    for (Index = 0; Index < TotalValidManId; ++Index)
    {
        if (s_ValidMainId[Index] == ManId)
        {
            IsValidDevice = NV_TRUE;
            break;
        }
    }
    return IsValidDevice;
}

SnorDeviceType GetSnorDeviceType(SnorDeviceIntRegister *pDevIntRegs)
{
    NvU16 ManId;
    NvU16 DevId;

    /* NVTODO : We will add support for Spansion
     * device now. Later we need to check all
     * the devices supported
     */

    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x555, 0xAA);
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x2AA, 0x55);
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x555, 0x90);
    /* Read Manufacture ID */
    ManId = SNOR_FLASH_READ16(pDevIntRegs->pNorBaseAdd,0x0) & 0xFF;
    /* Read Device ID */
    DevId = SNOR_FLASH_READ16(pDevIntRegs->pNorBaseAdd,0x1) & 0xFFFF;
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x00, 0xF0);
    switch(ManId)
    {
        case 0x0001: // spansion
            return SnorDeviceType_x16_NonMuxed;

        default:
            break;

    }
    return SnorDeviceType_Unknown;
}

/*
 * Spansion SNOR functions
 */
static void SpansionWaitForSNorProgramComplete(SnorDeviceIntRegister *pDevIntRegs)
{
    NvU16 Status;
    NvBool LastDQ6ToggleState;
    NvBool CurrentDQ6ToggleState;

    Status = SNOR_FLASH_READ16(pDevIntRegs->pNorBaseAdd,0x00) & 0xFFFF;
    LastDQ6ToggleState = (Status >> 6)& 0x1;
    while (1)
    {
        Status = SNOR_FLASH_READ16(pDevIntRegs->pNorBaseAdd,0x0) & 0xFF;
        CurrentDQ6ToggleState = (Status >> 6)& 0x1;
        if (LastDQ6ToggleState == CurrentDQ6ToggleState)
            break;
        LastDQ6ToggleState = CurrentDQ6ToggleState;
    }
}

#if 0
static NvBool SpansionWordProgram(SnorDeviceIntRegister *pDevIntRegs,NvU32 Address, NvU16 DataVal)
{
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x555, 0xAA);
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x2AA, 0x55);
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x555, 0xA0);
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,Address, DataVal);

    SpansionWaitForSNorProgramComplete(pDevIntRegs);
    return NV_TRUE;
}
#endif

SnorCommandStatus SpansionFormatDevice(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo,
        NvBool IsWaitForCompletion)
{
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x555, 0xAA);
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x2AA, 0x55);
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x555, 0x80);
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x555, 0xAA);
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x2AA, 0x55);
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x555, 0x10);
    // Same Status monitor suffices for Erase, Program operations
    SpansionWaitForSNorProgramComplete(pDevIntRegs);
    // Reset into read mode
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x0  , 0xF0);
    //FIXME: return something useful
    return 0;
}

SnorCommandStatus SpansionEraseSector(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo,
        NvU32 StartOffset,
        NvU32 Length,
        NvBool IsWaitForCompletion)
{
    // NVTODO : Add additional error checks
    // Assumed both parameters are aligned to 128K
    // Now taken care in the nvflash config file
    NvU32 SectorAdd = StartOffset;
    NvU32 NumOfSector = Length / SPANSION_SECTOR_SIZE ;
    while(NumOfSector)
    {
        SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x555, 0xAA);
        SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x2AA, 0x55);
        SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x555, 0x80);
        SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x555, 0xAA);
        SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x2AA, 0x55);
        SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,(SectorAdd >>1), 0x30);
        // Same Status monitor suffices for Erase, Program operations
        SpansionWaitForSNorProgramComplete(pDevIntRegs);
        // Reset into read mode
        SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x0,0xF0);
        SectorAdd += SPANSION_SECTOR_SIZE;
        NumOfSector --;
    }
    //FIXME: return something useful
    return 0;
}

SnorCommandStatus SpansionProtectSectors(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo,
        NvU32 Offset,
        NvU32 Size,
        NvBool IsLastPartition)
{
    NvError e = NvSuccess;
    volatile NvU16 Status;
    void *pNorBaseAdd = (void *)pDevIntRegs->pNorBaseAdd;
    NvU32 StartSectorAdd = Offset & ~(SPANSION_SECTOR_SIZE - 1);
    NvU32 SectorAdd, EndSectorAdd = (Offset + Size - 1) & ~(SPANSION_SECTOR_SIZE - 1);
    // Reset the device into read mode
    SNOR_FLASH_WRITE16(pNorBaseAdd,0x00, 0xF0);
    /* NVTODO : Factory reset is 'presistent' protection
     * but we should set this non-volatile bit to ensure
     * no other SW can change this to password protection
     */
    for (SectorAdd = StartSectorAdd; SectorAdd <= EndSectorAdd; SectorAdd += SPANSION_SECTOR_SIZE)
    {
        NvBool LastDQ6ToggleState;
        NvBool CurrentDQ6ToggleState, DQ5State, DQ0State;
        // Enter PPB Global Non-Volatile mode
        SNOR_FLASH_WRITE16(pNorBaseAdd, 0x555, 0xAA);
        SNOR_FLASH_WRITE16(pNorBaseAdd, 0x2AA, 0x55);
        SNOR_FLASH_WRITE16(pNorBaseAdd, 0x555, 0xC0);
        // Program the PPB bit for this sector
        SNOR_FLASH_WRITE16(pNorBaseAdd, 0x0, 0xA0);
        SNOR_FLASH_WRITE16(pNorBaseAdd, (SectorAdd >> 1), 0x00);
        // Wait until operation is complete
        Status = SNOR_FLASH_READ16(pNorBaseAdd, 0x0) & 0xFFFF;
        LastDQ6ToggleState = (Status >> 6)& 0x1;
        while (1)
        {
            // FIXME : Add a timeout.
            Status = SNOR_FLASH_READ16(pNorBaseAdd, 0x0) & 0xFF;
            CurrentDQ6ToggleState = (Status >> 6)& 0x1;
            DQ5State = (Status >> 5)& 0x1;
            // If DQ6 does not toggle
            if (LastDQ6ToggleState == CurrentDQ6ToggleState)
            {
                NvOsWaitUS(500);
                Status = SNOR_FLASH_READ16(pNorBaseAdd, (SectorAdd >> 1)) & 0xFFFF;
                DQ0State = (Status >> 0)& 0x1;
                if (DQ0State)
                {
                    e = NvError_InvalidState;
                }
                break;
            }
            // If DQ6 toggles && DQ5 is equal to 1
            else if (DQ5State)
            {
                Status = SNOR_FLASH_READ16(pNorBaseAdd, 0x0) & 0xFFFF;
                LastDQ6ToggleState = (Status >> 6)& 0x1;
                Status = SNOR_FLASH_READ16(pNorBaseAdd, 0x0) & 0xFF;
                CurrentDQ6ToggleState = (Status >> 6)& 0x1;
                // If DQ6 toggles, error out
                if (LastDQ6ToggleState != CurrentDQ6ToggleState)
                {
                    e = NvError_InvalidState;
                }
                else
                {
                    Status = SNOR_FLASH_READ16(pNorBaseAdd, (SectorAdd >> 1)) & 0xFFFF;
                    DQ0State = (Status >> 0)& 0x1;
                    if (DQ0State)
                    {
                        e = NvError_InvalidState;
                    }
                }
                break;
            }
            LastDQ6ToggleState = CurrentDQ6ToggleState;
        }
        if (e != NvSuccess)
        {
            goto fail;
        }
        // Exit PPB Global Non-Volatile mode
        SNOR_FLASH_WRITE16(pNorBaseAdd, 0x00, 0x90);
        SNOR_FLASH_WRITE16(pNorBaseAdd, 0x00, 0x00);
    }
    /* Is this is the last partition on the device
     * freeze the PPB Lock bit
     */
    if (IsLastPartition == NV_TRUE) {
        // Program Global volatile freeze bit to 0
        SNOR_FLASH_WRITE16(pNorBaseAdd, 0x555, 0xAA);
        SNOR_FLASH_WRITE16(pNorBaseAdd, 0x2AA, 0x55);
        SNOR_FLASH_WRITE16(pNorBaseAdd, 0x555, 0x50);
        // Set the the bit
        SNOR_FLASH_WRITE16(pNorBaseAdd, 0x00, 0xA0);
        SNOR_FLASH_WRITE16(pNorBaseAdd, 0x00, 0x00);
        // Wait until the Lock is set (0)
        while (SNOR_FLASH_READ16(pNorBaseAdd, 0x00));
        // Exit PPB Global volatile-freeze mode
        SNOR_FLASH_WRITE16(pNorBaseAdd, 0x00, 0x90);
        SNOR_FLASH_WRITE16(pNorBaseAdd, 0x00, 0x00);
        // Enter PPB Global Non-Volatile mode
        SNOR_FLASH_WRITE16(pNorBaseAdd, 0x555, 0xAA);
        SNOR_FLASH_WRITE16(pNorBaseAdd, 0x2AA, 0x55);
        SNOR_FLASH_WRITE16(pNorBaseAdd, 0x555, 0xC0);
        // Exit PPB Global Non-Volatile mode
        SNOR_FLASH_WRITE16(pNorBaseAdd, 0x00, 0x90);
        SNOR_FLASH_WRITE16(pNorBaseAdd, 0x00, 0x00);
    }
    SNOR_FLASH_WRITE16(pNorBaseAdd, 0x00, 0xF0);
    return (SnorCommandStatus)e;
fail :
    SNOR_FLASH_WRITE16(pNorBaseAdd, 0x00, 0xF0);
    // Exit PPB Global Non-Volatile mode
    SNOR_FLASH_WRITE16(pNorBaseAdd, 0x00, 0x90);
    SNOR_FLASH_WRITE16(pNorBaseAdd, 0x00, 0x00);
    return (SnorCommandStatus)e;
}

SnorCommandStatus SpansionUnprotectAllSectors(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo)
{
    NvU16 Status = 0;
    void *pNorBaseAdd = (void*)pDevIntRegs->pNorBaseAdd;
    // Enter PPB Global Non-Volatile mode
    SNOR_FLASH_WRITE16(pNorBaseAdd, 0x555, 0xAA);
    SNOR_FLASH_WRITE16(pNorBaseAdd, 0x2AA, 0x55);
    SNOR_FLASH_WRITE16(pNorBaseAdd, 0x555, 0xC0);
    // Erasure of PPB bits
    SNOR_FLASH_WRITE16(pNorBaseAdd, 0x0, 0x80);
    SNOR_FLASH_WRITE16(pNorBaseAdd, 0x0, 0x30);
    // FIXME : Wait for status.
    NvOsWaitUS(500000);
    // Exit PPB Global Non-Volatile mode
    SNOR_FLASH_WRITE16(pNorBaseAdd, 0x00, 0x90);
    SNOR_FLASH_WRITE16(pNorBaseAdd, 0x00, 0x00);
    return Status;
}

#define SPANSION_BUFFER_SIZE 16
#define SPANSION_WRITE_SECTOR_SIZE 16
#define SECTOR_ADDRESS_MASK  0xfffffff0
SnorCommandStatus SpansionProgram(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo,
        NvU32 ByteOffset,
        NvU32 SizeInBytes,
        const NvU8* Buffer,
        NvBool IsWaitForCompletion)
{
    // 16-bit A/D split SNOR.
    // Programming can happen only in 16 bit words
    NvU32 WordOffset = ByteOffset >> 1, NumWords, i;
    NvU32 SizeInWords = SizeInBytes >> 1;
    NvU32 CurrentSectorAddress; //A_max - A_4
    NvU32 CurrentSectorEndAddress;
    NvU16 WriteData[SPANSION_BUFFER_SIZE];
    /* Spansion device supports 16 words buffer. Following conditions
     * are to be met for buffered writes
     * Max of 16 words can be programmed at once.
     * The sector address A_max - A_4 of all addresses should be the same
     * i.e; Only words in the same sector can be in the buffer
     */
    while(SizeInWords > 0)
    {
        CurrentSectorAddress = WordOffset & 0xfffffff0;
        CurrentSectorEndAddress = CurrentSectorAddress + SPANSION_WRITE_SECTOR_SIZE;
        // Calculate the number of words in current sector
        NumWords = CurrentSectorEndAddress - WordOffset;
        NumWords = (NumWords > SizeInWords) ? SizeInWords : NumWords;
        if(!NumWords)
            break;

        NvOsMemcpy((void *)WriteData,(void *)Buffer, NumWords << 1);
        Buffer += (NumWords << 1);

        // a. Unlock cycles
        SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x555, 0xAA);
        SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x2AA, 0x55);

        // b. Program Address, 0x25
        SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,WordOffset,0x25);

        // c. Sector Adress, (Word Count - 1)
        SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,CurrentSectorAddress,NumWords-1);

        // d. Fill up the buffer
        for( i=0; i < NumWords; WordOffset++, i++)
            SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,WordOffset,WriteData[i]);

        // e. Program to Flash
        SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,CurrentSectorAddress,0x29);

        // f. Wait for Completion
        SpansionWaitForSNorProgramComplete(pDevIntRegs);

        SizeInWords -= NumWords;
    }
    return NV_TRUE;
}

SnorCommandStatus SpansionAsyncRead(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo,
        NvU32 ByteOffset,
        NvU32 SizeInBytes,
        NvU8* Buffer,
        NvBool IsWaitForCompletion)
{
    NvU32 WordOffset = ByteOffset >> 1;
    NvU32 SizeInWords = SizeInBytes >> 1, Index;
    NvU16* pReadData = (NvU16*)Buffer;
    volatile int nvbrk=0;
    while(nvbrk==1);
    // Reset the Device to get it back into read array mode
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x00, 0xF0);
    for(Index=0;
        Index < SizeInWords;
        Index++, WordOffset++, pReadData++)
    {
        *pReadData = SNOR_FLASH_READ16(pDevIntRegs->pNorBaseAdd,WordOffset);
    }
    return NV_TRUE;
}

SnorCommandStatus SpansionReadMode(
        SnorDeviceIntRegister *pDevIntRegs,
        SnorDeviceInfo *pDevInfo,
        NvBool IsWaitForCompletion)
{
    // Reset the Device to get it back into read array mode
    SNOR_FLASH_WRITE16(pDevIntRegs->pNorBaseAdd,0x00, 0xF0);
    return NV_TRUE;
}

/**
 * Initialize the snor device interface.
 */
void
NvRmPrivSnorDeviceInterface(
    SnorDevInterface *pDevInterface,
    SnorDeviceIntRegister *pDevIntReg,
    SnorDeviceInfo *pDevInfo)
{
    // NVTODO : This should be based on Manufacture ID
    pDevInterface->DevInitializeDeviceInterface = InitializeDeviceInterface;
    pDevInterface->DevResetDevice = ResetDevice;
    pDevInterface->DevIsDeviceReady = IsDeviceReady;
    pDevInterface->DevGetDeviceInfo = GetDeviceInfo;
    pDevInterface->DevConfigureHostInterface = ConfigureHostInterface;
    pDevInterface->DevConfigureHostInterface = ConfigureHostInterface;
    pDevInterface->DevGetInterruptReason = GetInterruptReason;
    // This can be made as generic CFI Query
    pDevInterface->DevProgram = SpansionProgram;
    pDevInterface->DevFormatDevice = SpansionFormatDevice;
    pDevInterface->DevEraseSector = SpansionEraseSector;
    pDevInterface->DevProtectSectors = SpansionProtectSectors;
    pDevInterface->DevUnprotectAllSectors = SpansionUnprotectAllSectors;
    pDevInterface->DevRead = SpansionAsyncRead;
    pDevInterface->DevReadMode = SpansionReadMode ;
}

