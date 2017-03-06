/*
 * Copyright (c) 2007-2012, NVIDIA CORPORATION.  All rights reserved.
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
 *           The ddk one nand API implementation</b>
 *
 * @b Description:  Implements  the public interfacing functions for the one
 * nand device driver interface.
 *
 */
#include "ap20/arsnor.h"
#include "ap20/arapb_misc.h"
#include "nvrm_hardware_access.h"
#include "nvrm_power.h"
#include "nvrm_module.h"
#include "nvrm_drf.h"
#include "nvassert.h"
#include "nvrm_memmgr.h"
#include "nvrm_interrupt.h"
#include "nvrm_pinmux.h"
#include "nvddk_one_nand.h"
#include "nvddk_nand.h"
#include "one_nand_priv_driver.h"

#define SNOR_READ32(pOneNandHwRegsVirtBaseAdd, reg) \
        NV_READ32((pOneNandHwRegsVirtBaseAdd) + ((SNOR_##reg##_0)/4))

#define SNOR_WRITE32(pOneNandHwRegsVirtBaseAdd, reg, val) \
    do \
    {  \
        NV_WRITE32((((pOneNandHwRegsVirtBaseAdd) + ((SNOR_##reg##_0)/4))), (val)); \
    } while (0)


#define DEBUG_PRINTS 0 

#if DEBUG_PRINTS
#define ONE_NAND_PRINT(x) NvOsDebugPrintf(x)
#else
#define ONE_NAND_PRINT(x)
#endif

#define MAX_CHIPSELECT 8
#define BLOCK_NUMBER(PageNumber, PagesPerBlock) ((PageNumber)/(PagesPerBlock))
#define PAGE_NUMBER(PageNumber, PagesPerBlock) ((PageNumber)%(PagesPerBlock))

#define ENABLED_PINMUX 0
typedef struct 
{
    NvU32 Config;
    NvU32 Status;
    NvU32 NorAddressPtr;
    NvU32 AhbAddrPtr;
    NvU32 Timing0;
    NvU32 Timing1;
    NvU32 MioCfg;
    NvU32 MioTiming;
    NvU32 DmaConfig;
    NvU32 ChipSelectMuxConfig;
} SnorControllerRegs;


typedef struct NvDdkOneNandRec
{
    NvRmDeviceHandle hRmDevice;

    NvU32 InstanceId;

    NvU32 OpenCount;
    
    // Physical Address of the SNOR controller instance
    NvU32 SnorControllerBaseAdd;

    // Virtual address for the SNOR controller instance
    NvU32 *pSnorVirtAdd;

    // Size of the SNOR register map
    NvU32 SnorBankSize;
    
    // Interrupt handle
    NvOsInterruptHandle hIntr;

    // Semaphore  for registering the client with the power manager.
    NvOsSemaphoreHandle hRmPowerEventSema;

    // Power client Id.
    NvU32 RmPowerClientId;

    // Command complete semaphore
    NvOsSemaphoreHandle hCommandCompleteSema;
    
    // Number of devices present
    NvU32 NumOfDevicesConnected;

    // Tells whether the device is avialble or not.
    NvU32 IsDevAvailable[MAX_CHIPSELECT];

    // Nand Device Info to the client
    NvDdkNandDeviceInfo DdkNandDevInfo;

    // One nand device access interface which contains the diffrent function 
    // pointer to access the device
    OneNandDevInterface OneNandDevInt;

    // One nand device information connected to the controller.
    OneNandDeviceInfo OneNandDevInfo;

    // One nand device interface register to access the devices.
    OneNandDeviceIntRegister OneNandDevReg;

    NvOsMutexHandle hDevAccess;
    
    NvOsMutexHandle hSnorRegAccess;

    SnorControllerRegs SnorRegs;
} NvDdkOneNand;

typedef struct 
{
    NvRmDeviceHandle hRmDevice;
    NvDdkOneNandHandle hOneNand;
    NvOsMutexHandle hOneNandOpenMutex;
} DdkOneNandInformation;

static DdkOneNandInformation *s_pOneNandInfo = NULL;

static void SnorISR (void *pArgs)
{
    NvDdkOneNandHandle hOneNand = (NvDdkOneNandHandle)pArgs;
    NvU32 StatusReg;

    StatusReg = SNOR_READ32(hOneNand->pSnorVirtAdd, DMA_CFG);
    if (NV_DRF_VAL(SNOR, DMA_CFG, IS_DMA_DONE, StatusReg))
    {
        SNOR_WRITE32(hOneNand->pSnorVirtAdd, DMA_CFG, StatusReg);
        NvOsSemaphoreSignal(hOneNand->hCommandCompleteSema);
    }

    NvRmInterruptDone(hOneNand->hIntr);
}

static void InitSnorController(NvDdkOneNandHandle hOneNand)
{
    NvU32 SnorConfig = hOneNand->SnorRegs.Config;

     // SNOR_CONFIG_0_WORDWIDE_GMI_NOR16BIT
     // SNOR_CONFIG_0_NOR_DEVICE_TYPE_SNOR
     // SNOR_CONFIG_0_MUXMODE_GMI_AD_MUX
    SnorConfig = NV_FLD_SET_DRF_DEF(SNOR, CONFIG, NOR_DEVICE_TYPE, MUXONENAND, SnorConfig);
    SnorConfig = NV_FLD_SET_DRF_DEF(SNOR, CONFIG, WORDWIDE_GMI, NOR16BIT, SnorConfig);
    SnorConfig = NV_FLD_SET_DRF_DEF(SNOR, CONFIG, MUXMODE_GMI, AD_MUX, SnorConfig);
    
    hOneNand->SnorRegs.Config = SnorConfig;
    SNOR_WRITE32(hOneNand->pSnorVirtAdd, CONFIG, SnorConfig);
}

#if 0
static void SetInterfaceTimings(NvDdkOneNandHandle hOneNand, NvU32 Timing1)
{
    
}
#endif

static void SetChipSelect(NvDdkOneNandHandle hOneNand, NvU32 ChipSelId)
{
    NvU32 SnorConfig = hOneNand->SnorRegs.Config;
    SnorConfig = NV_FLD_SET_DRF_NUM(SNOR, CONFIG, SNOR_SEL, ChipSelId, SnorConfig);

    hOneNand->SnorRegs.Config = SnorConfig;
    SNOR_WRITE32(hOneNand->pSnorVirtAdd, CONFIG, SnorConfig);
}

static NvError RegisterOneNandInterrupt(NvDdkOneNandHandle hOneNand)
{
    NvU32 IrqList;
    NvOsInterruptHandler IntHandle = SnorISR;

    IrqList = NvRmGetIrqForLogicalInterrupt(hOneNand->hRmDevice,
                        NVRM_MODULE_ID(NvRmModuleID_SyncNor, 0),
                        0);
    return NvRmInterruptRegister(hOneNand->hRmDevice, 1, &IrqList, &IntHandle,
                                    hOneNand, &hOneNand->hIntr, NV_TRUE);
}

/**
 * Initialize the one nand information.
 */
static NvError InitOneNandInformation(NvRmDeviceHandle hRmDevice)
{
    NvError Error = NvSuccess;
    DdkOneNandInformation *pOneNandInfo = NULL;

    if (!s_pOneNandInfo)
    {
        // Allocate the memory for the onenand information.
        pOneNandInfo = NvOsAlloc(sizeof(*pOneNandInfo));
        if (!pOneNandInfo)
            return NvError_InsufficientMemory;
        NvOsMemset(pOneNandInfo, 0, sizeof(*pOneNandInfo));

        // Initialize all the parameters.
        pOneNandInfo->hRmDevice = hRmDevice;
        pOneNandInfo->hOneNandOpenMutex = NULL;
        pOneNandInfo->hOneNand = NULL;

        // Create the mutex to accss the one nand information.
        Error = NvOsMutexCreate(&pOneNandInfo->hOneNandOpenMutex);
        // If error exit till then destroy all allocations.
        if (Error)
        {
            NvOsFree(pOneNandInfo);
            pOneNandInfo = NULL;
            return Error;
        }    

        if (NvOsAtomicCompareExchange32((NvS32*)&s_pOneNandInfo, 0, (NvS32)pOneNandInfo)!=0)
        {
            NvOsMutexDestroy(pOneNandInfo->hOneNandOpenMutex);
            NvOsFree(pOneNandInfo);
        }
    }
    return NvSuccess;
}

static NvError SetPowerControl(NvDdkOneNandHandle hOneNand, NvBool IsEnable)
{
    NvError Error = NvSuccess;
    NvRmModuleID ModuleId;

    ModuleId = NVRM_MODULE_ID(NvRmModuleID_SyncNor, hOneNand->InstanceId);
    if (IsEnable)
    {
        // Enable power for OneNand module
        if (!Error)
            Error = NvRmPowerVoltageControl(hOneNand->hRmDevice, ModuleId,
                            hOneNand->RmPowerClientId,
                            NvRmVoltsUnspecified, NvRmVoltsUnspecified,
                            NULL, 0, NULL);

        // Enable the clock.
        if (!Error)
            Error = NvRmPowerModuleClockControl(hOneNand->hRmDevice, ModuleId, 
                        hOneNand->RmPowerClientId, NV_TRUE);
    }
    else
    {
        // Disable the clocks.
        Error = NvRmPowerModuleClockControl(
                                    hOneNand->hRmDevice,
                                    ModuleId, hOneNand->RmPowerClientId,
                                    NV_FALSE);
        
        // Disable the power to the controller.
        if (!Error)
            Error = NvRmPowerVoltageControl(
                                    hOneNand->hRmDevice,
                                    ModuleId,
                                    hOneNand->RmPowerClientId,
                                    NvRmVoltsOff,
                                    NvRmVoltsOff,
                                    NULL,
                                    0,
                                    NULL);
        NV_ASSERT(Error == NvSuccess);
    }
    return Error;
}                


/**
 * Destroy the one nand handle.
 */
static void DestroyOneNandHandle(NvDdkOneNandHandle hOneNand)
{
#if ENABLED_PINMUX
    NvRmModuleID ModuleId;
#endif
    // If null pointer for one nand handle then return error.
    if (!hOneNand)
        return;

#if ENABLED_PINMUX        
    ModuleId = NVRM_MODULE_ID(NvRmModuleID_SyncNor, hOneNand->InstanceId);
#endif

    if (hOneNand->RmPowerClientId)
    {
        SetPowerControl(hOneNand, NV_FALSE);

        // Unregister for the power manager.
        NvRmPowerUnRegister(hOneNand->hRmDevice, hOneNand->RmPowerClientId);
        NvOsSemaphoreDestroy(hOneNand->hRmPowerEventSema);
    }
    
    if (hOneNand->hIntr)
    {
        NvRmInterruptUnregister(hOneNand->hRmDevice, hOneNand->hIntr);
        hOneNand->hIntr = NULL;
    }

    // Unmap the virtual mapping of the snor controller.
    if (hOneNand->pSnorVirtAdd)
        NvRmPhysicalMemUnmap(hOneNand->pSnorVirtAdd, hOneNand->SnorBankSize);

    // Unmap the virtual mapping of the Nor interfacing register.
    if (hOneNand->OneNandDevReg.pNorBaseAdd)
        NvRmPhysicalMemUnmap(hOneNand->OneNandDevReg.pNorBaseAdd, 
                                    hOneNand->OneNandDevReg.NorAddressSize);

#if ENABLED_PINMUX
#ifndef SET_KERNEL_PINMUX
    // Tri-State the pin-mux pins
    NV_ASSERT_SUCCESS(NvRmSetModuleTristate(hOneNand->hRmDevice, ModuleId, NV_TRUE));
#endif
#endif

    // Destroy the sync sempahores.
    NvOsSemaphoreDestroy(hOneNand->hCommandCompleteSema);
    
    NvOsMutexDestroy(hOneNand->hSnorRegAccess);

    // Free the memory of the one nand handles.
    NvOsFree(hOneNand);
}

static NvError 
CreateOneNandHandle(
    NvRmDeviceHandle hRmDevice,
    NvU32 InstanceId,
    NvDdkOneNandHandle *phOneNand)
{
    NvError Error = NvSuccess;
    NvDdkOneNandHandle hNewOneNand = NULL;
    NvRmModuleID ModuleId;
    NvU32 Index;
    OneNandDeviceType DeviceType;
    NvU32 IsResetDone;
    NvU32 ResetDelayTime;
    NvU32 MaxResetDelayTime;
    NvU32 StartTime, CurrTime, ElapsedTime;
    
    ModuleId = NVRM_MODULE_ID(NvRmModuleID_SyncNor, InstanceId);
    *phOneNand = NULL;

#if ENABLED_PINMUX
#ifndef SET_KERNEL_PINMUX
    // Enable the pin mux to normal state.
    if (NvRmSetModuleTristate(hRmDevice, ModuleId, NV_FALSE) != NvSuccess)
        return NvError_NotSupported;
#endif
#endif

    // Allcoate the memory for the OneNand handle.
    hNewOneNand = NvOsAlloc(sizeof(NvDdkOneNand));
    if (!hNewOneNand)
    {
        Error = NvError_InsufficientMemory;
        goto ErrorEnd;
    }
    
    // Reset the memory allocated for the OneNand handle.
    NvOsMemset(hNewOneNand, 0, sizeof(*hNewOneNand));

    // Set the OneNand handle parameters.
    hNewOneNand->hRmDevice = hRmDevice;
    hNewOneNand->InstanceId = InstanceId;
    hNewOneNand->OpenCount = 0;

    hNewOneNand->SnorControllerBaseAdd = 0;
    hNewOneNand->pSnorVirtAdd = NULL;
    hNewOneNand->SnorBankSize = 0;
    
    hNewOneNand->hIntr = NULL;

    hNewOneNand->hRmPowerEventSema = NULL;
    hNewOneNand->RmPowerClientId = 0;

    // Command complete semaphore
    hNewOneNand->hCommandCompleteSema = NULL;

    hNewOneNand->NumOfDevicesConnected = 0;
    for (Index = 0; Index < MAX_CHIPSELECT; ++Index)
        hNewOneNand->IsDevAvailable[Index] = 0;

    hNewOneNand->OneNandDevInfo.DeviceType = OneNandDeviceType_Unknown;
    
    hNewOneNand->OneNandDevReg.pNorBaseAdd = NULL;
    hNewOneNand->OneNandDevReg.NorAddressSize = 0;

    hNewOneNand->hDevAccess = NULL;
    hNewOneNand->hSnorRegAccess = NULL;

    hNewOneNand->SnorRegs.Config = NV_RESETVAL(SNOR, CONFIG);
    hNewOneNand->SnorRegs.Status = NV_RESETVAL(SNOR, STA);
    hNewOneNand->SnorRegs.NorAddressPtr = NV_RESETVAL(SNOR, NOR_ADDR_PTR);
    hNewOneNand->SnorRegs.AhbAddrPtr = NV_RESETVAL(SNOR, AHB_ADDR_PTR);
    hNewOneNand->SnorRegs.Timing0 = NV_RESETVAL(SNOR, TIMING0);
    hNewOneNand->SnorRegs.Timing1 = NV_RESETVAL(SNOR, TIMING1);
    hNewOneNand->SnorRegs.MioCfg = NV_RESETVAL(SNOR, MIO_CFG);
    hNewOneNand->SnorRegs.MioTiming = NV_RESETVAL(SNOR, MIO_TIMING0);
    hNewOneNand->SnorRegs.DmaConfig = NV_RESETVAL(SNOR, DMA_CFG);
    hNewOneNand->SnorRegs.ChipSelectMuxConfig = NV_RESETVAL(SNOR, CS_MUX_CFG);

    // Initialize the base addresses of snor controller
    NvRmModuleGetBaseAddress(hRmDevice, ModuleId,
                            &hNewOneNand->SnorControllerBaseAdd,
                            &hNewOneNand->SnorBankSize);
    
    Error = NvRmPhysicalMemMap(hNewOneNand->SnorControllerBaseAdd,
                               hNewOneNand->SnorBankSize,
                               NVOS_MEM_READ_WRITE, NvOsMemAttribute_Uncached,
                               (void **)&(hNewOneNand->pSnorVirtAdd));
    if (Error)
    {
        ONE_NAND_PRINT(("[NvDdkOneNandOpen] SNOR Controller register mapping failed\n"));
        goto ErrorEnd;
    }

    // Initialize the base addresses of snor controller
    NvRmModuleGetBaseAddress(hRmDevice, 
                            NVRM_MODULE_ID(NvRmModuleID_Nor, 0),
                            &hNewOneNand->OneNandDevReg.NorBaseAddress,
                            &hNewOneNand->OneNandDevReg.NorAddressSize);
    
    Error = NvRmPhysicalMemMap(hNewOneNand->OneNandDevReg.NorBaseAddress,
                               hNewOneNand->OneNandDevReg.NorAddressSize,
                               NVOS_MEM_READ_WRITE, NvOsMemAttribute_Uncached,
                               (void **)&(hNewOneNand->OneNandDevReg.pNorBaseAdd));
    if (Error)
    {
        ONE_NAND_PRINT(("[NvDdkOneNandOpen] Nor Base register mapping failed\n"));
        goto ErrorEnd;
    }

    
    // Register as the Rm power client
    Error = NvOsSemaphoreCreate(&hNewOneNand->hRmPowerEventSema, 0);
    if (!Error)
        Error = NvRmPowerRegister(hNewOneNand->hRmDevice, 
                        hNewOneNand->hRmPowerEventSema,
                        &hNewOneNand->RmPowerClientId);

    if (!Error)
        Error =  SetPowerControl(hNewOneNand, NV_TRUE);
    
    // Reset the snor controller.
    if (!Error)
        NvRmModuleReset(hRmDevice, ModuleId);
    
    if (!Error)
        Error = RegisterOneNandInterrupt(hNewOneNand);

    if (!Error)
        Error = NvOsSemaphoreCreate(&hNewOneNand->hCommandCompleteSema, 0);

    InitSnorController(hNewOneNand);

    hNewOneNand->NumOfDevicesConnected = 0;
    DeviceType = OneNandDeviceType_Unknown;
    for (Index = 0; Index < MAX_CHIPSELECT; ++Index)
    {
        // Select Chipselect 
        SetChipSelect(hNewOneNand, Index);
        
        hNewOneNand->IsDevAvailable[Index] = IsDeviceAvailable(
                                &hNewOneNand->OneNandDevReg, Index);
        if (hNewOneNand->IsDevAvailable[Index])
        {
            hNewOneNand->NumOfDevicesConnected++;
            DeviceType = GetDeviceType(&hNewOneNand->OneNandDevReg);
            // Device must be the muxonenand or flex mux one nand.
            NV_ASSERT(DeviceType != OneNandDeviceType_Unknown); 
        }    
    }

    // At this point we should have flex or mux one nand device.
    NV_ASSERT(DeviceType != OneNandDeviceType_Unknown); 
    if (DeviceType == OneNandDeviceType_Unknown)
    {
        Error = NvError_ModuleNotPresent;
        goto ErrorEnd;
    }    
    hNewOneNand->OneNandDevInfo.DeviceType = DeviceType;

    NvRmPrivOneNandDeviceInterface(&hNewOneNand->OneNandDevInt,
                                    &hNewOneNand->OneNandDevReg,
                                    &hNewOneNand->OneNandDevInfo);

    // Reset all available devices
    // Send reset command to all devices.
    MaxResetDelayTime = 0;
    for (Index = 0; Index < MAX_CHIPSELECT; ++Index)
    {
        if (hNewOneNand->IsDevAvailable[Index])
        {
            // Select Chipselect 
            SetChipSelect(hNewOneNand, Index);
            ResetDelayTime = hNewOneNand->OneNandDevInt.DevResetDevice(
                                        &hNewOneNand->OneNandDevReg, Index);
            MaxResetDelayTime = NV_MAX(MaxResetDelayTime, ResetDelayTime);                                        
        }
    }       
    
    // Now wait for the reset done.
    StartTime = NvOsGetTimeMS();
    ElapsedTime = 0;
    for (Index = 0; Index < MAX_CHIPSELECT; ++Index)
    {
        if (hNewOneNand->IsDevAvailable[Index])
        {
            // Select Chipselect 
            SetChipSelect(hNewOneNand, Index);
            do 
            {
                IsResetDone = hNewOneNand->OneNandDevInt.DevIsDeviceReady(
                                                &hNewOneNand->OneNandDevReg);
                if (IsResetDone)
                    break;
                CurrTime = NvOsGetTimeMS();
                ElapsedTime = (CurrTime >= StartTime)?(CurrTime - StartTime):
                                0xFFFFFFFF-CurrTime + StartTime;
                NvOsSleepMS(1);                                
            } while (ElapsedTime <= MaxResetDelayTime);
            
            if (ElapsedTime > MaxResetDelayTime)
            {
                NV_ASSERT(0);
                break;
            }
        }
    }       

    // Populate the device information
    for (Index = 0; Index < MAX_CHIPSELECT; ++Index)
    {
        if (hNewOneNand->IsDevAvailable[Index])
        {
            // Select Chipselect 
            SetChipSelect(hNewOneNand, Index);
            hNewOneNand->OneNandDevInt.DevGetDeviceInfo(
                                &hNewOneNand->OneNandDevReg,            
                                &hNewOneNand->OneNandDevInfo,
                                Index);            
            break;                                
        }
    }    
    
                                    
 ErrorEnd:       
    // If error then destroy all the allocation done here.
    if (Error)
    {
        DestroyOneNandHandle(hNewOneNand);
        hNewOneNand = NULL;
    }
    *phOneNand = hNewOneNand;
    return Error;
}


NvError
NvDdkOneNandOpen(
    NvRmDeviceHandle hRmDevice,
    NvU8 Instance,
    NvDdkOneNandHandle *phOneNand)
{
    NvError Error;

    NV_ASSERT(hRmDevice);

    *phOneNand = NULL;
    Error = InitOneNandInformation(hRmDevice);
    if (Error)
        return Error;

    NvOsMutexLock(s_pOneNandInfo->hOneNandOpenMutex);
    if (!s_pOneNandInfo->hOneNand)
        Error = CreateOneNandHandle(hRmDevice, Instance,  &s_pOneNandInfo->hOneNand);
        
    if (!Error)
        s_pOneNandInfo->hOneNand->OpenCount++;

    *phOneNand = s_pOneNandInfo->hOneNand;
    NvOsMutexUnlock(s_pOneNandInfo->hOneNandOpenMutex);
    return Error;    
}

void NvDdkOneNandClose(NvDdkOneNandHandle hOneNand)
{
    if (!hOneNand)
        return;

    NvOsMutexLock(s_pOneNandInfo->hOneNandOpenMutex);
    if (hOneNand->OpenCount)
        hOneNand->OpenCount--;
        
    if (!hOneNand->OpenCount)
    {
        DestroyOneNandHandle(hOneNand);
        s_pOneNandInfo->hOneNand = NULL;
    }
    NvOsMutexUnlock(s_pOneNandInfo->hOneNandOpenMutex);
}

NvError
NvDdkOneNandRead(
    NvDdkOneNandHandle hOneNand,
    NvU8 StartDeviceNum,
    NvU32* pPageNumbers,
    NvU8* const pDataBuffer,
    NvU8* const pTagBuffer,
    NvU32 *pNoOfPages,
    NvBool IsIgnoreEccError)
{
    NvError Error = NvSuccess;
    NvU32 ChipSelectId[MAX_CHIPSELECT];
    NvU32 StartPageNumber[MAX_CHIPSELECT];
    NvU32 CurrBlockNumber;
    NvU32 CurrPageNumber;
    NvU32 MaxValidIndex = 0;
    NvU32 Index;
    NvU32 StartIndex = StartDeviceNum;
    NvU32 NumberOfPagesRead = 0;
    OneNandCommandStatus OneNandStatus;
    NvU32 PagesRemaining;
    NvU32 PagesRequested;
    NvU32 CurrentDeviceLoop;
    NvBool IsLoadErrorOccured = NV_FALSE;
    NvU8 *pMainDataBuffer = (NvU8 *)pDataBuffer;
    NvU8 *pTagDataBuffer = (NvU8 *)pTagBuffer;

    NV_ASSERT(StartDeviceNum < MAX_CHIPSELECT);

    // Get the starting pages of all valid chip selects.
    for (Index = 0; Index < MAX_CHIPSELECT; ++Index)
    {
        if (pPageNumbers[StartIndex] != 0xFFFFFFFF)
        {
            ChipSelectId[MaxValidIndex] = StartIndex;
            StartPageNumber[MaxValidIndex] = pPageNumbers[StartIndex];
            MaxValidIndex++;
        }
        StartIndex = (StartIndex + 1)% MAX_CHIPSELECT;
    }

    // There should be the read request from any device.
    NV_ASSERT(MaxValidIndex);
    
    // Check whether the requested devices are available or not.
    for (Index = 0; Index < MaxValidIndex; ++Index)
    {
        if (!hOneNand->IsDevAvailable[ChipSelectId[Index]]) 
            return NvError_BadParameter;
    }

    // Enable the clock
    Error =  SetPowerControl(hOneNand, NV_TRUE);
    if (Error)
        return Error;


    // Read the pages from each devices. 
    NumberOfPagesRead = 0;
    PagesRemaining = *pNoOfPages;
    PagesRequested = *pNoOfPages;

    CurrentDeviceLoop = NV_MIN(MaxValidIndex, PagesRemaining);
    
    // Issue the load command to all selected devices
    for (Index = 0; Index < CurrentDeviceLoop; ++Index)
    {
        SetChipSelect(hOneNand, ChipSelectId[Index]);
        
        CurrBlockNumber = BLOCK_NUMBER(StartPageNumber[Index],
                                hOneNand->OneNandDevInfo.PagesPerBlock);
        CurrPageNumber  = PAGE_NUMBER(StartPageNumber[Index],
                                hOneNand->OneNandDevInfo.PagesPerBlock);
    
        // Issue the load command.
        OneNandStatus = hOneNand->OneNandDevInt.DevLoadBlockPage(
                        &hOneNand->OneNandDevReg, &hOneNand->OneNandDevInfo, 
                        CurrBlockNumber, CurrPageNumber, NV_FALSE);
        StartPageNumber[Index]++;
                                
        if (OneNandStatus)
        {
            Error = NvError_NandReadFailed;
            goto ErrorEnd;
        }
    }

    while (PagesRemaining)
    {
        CurrentDeviceLoop = NV_MIN(MaxValidIndex, PagesRemaining);

        // Read from the devices if load is completed
        for (Index = 0; Index < CurrentDeviceLoop; ++Index)
        {
            if ((Index == 0) && (IsLoadErrorOccured))
                goto ErrorEnd;
                
            SetChipSelect(hOneNand, ChipSelectId[Index]);
            // TODO: Int based ???
            do
            {
                OneNandStatus = hOneNand->OneNandDevInt.DevGetLoadStatus(
                            &hOneNand->OneNandDevReg); 
            } while (OneNandStatus == OneNandCommandStatus_Busy);
            
            // Ignore the Ecc error if it is selected.
            if ((OneNandStatus == OneNandCommandStatus_EccError) &&
                                                        (!IsIgnoreEccError))
                goto ErrorEnd;
                
            if (OneNandStatus == OneNandCommandStatus_Error) 
                goto ErrorEnd;

            // Read from device buffer ram now.
            hOneNand->OneNandDevInt.DevAsynchReadFromBufferRam(
                            &hOneNand->OneNandDevReg, pMainDataBuffer, pTagDataBuffer,
                            hOneNand->OneNandDevInfo.MainAreaSizePerPage,
                            hOneNand->OneNandDevInfo.SpareAreaSizePerPage);
            NumberOfPagesRead++;
            PagesRemaining--;
            pMainDataBuffer += hOneNand->OneNandDevInfo.MainAreaSizePerPage;
            pTagDataBuffer +=  hOneNand->OneNandDevInfo.SpareAreaSizePerPage;
            // Issue the load command again if it need to read it again.                            
            if (PagesRequested >= (NumberOfPagesRead + MaxValidIndex))
            {
                CurrBlockNumber = BLOCK_NUMBER(StartPageNumber[Index],
                                        hOneNand->OneNandDevInfo.PagesPerBlock);
                CurrPageNumber  = PAGE_NUMBER(StartPageNumber[Index],
                                        hOneNand->OneNandDevInfo.PagesPerBlock);

                StartPageNumber[Index]++;

                // Issue the load command.
                OneNandStatus = hOneNand->OneNandDevInt.DevLoadBlockPage(
                                &hOneNand->OneNandDevReg, &hOneNand->OneNandDevInfo, 
                                CurrBlockNumber, CurrPageNumber, NV_FALSE);
                if (OneNandStatus)
                {
                    IsLoadErrorOccured = NV_TRUE;
                    Error = NvError_NandReadFailed;
                }
            }
        }
    }
    // Issue the load command to all devices first.

ErrorEnd:        
    // Disble the clock
    SetPowerControl(hOneNand, NV_FALSE);
    *pNoOfPages = NumberOfPagesRead;
    return Error;
}

NvError
NvDdkOneNandWrite(
    NvDdkOneNandHandle hOneNand,
    NvU8 StartDeviceNum,
    NvU32* pPageNumbers,
    const NvU8* pDataBuffer,
    const NvU8* pTagBuffer,
    NvU32 *pNoOfPages)
{
    NvError Error = NvSuccess;
    NvU32 ChipSelectId[MAX_CHIPSELECT];
    NvU32 StartPageNumber[MAX_CHIPSELECT];
    NvU32 CurrBlockNumber;
    NvU32 CurrPageNumber;
    NvU32 MaxValidIndex = 0;
    NvU32 Index;
    NvU32 StartIndex = StartDeviceNum;
    NvU32 NumberOfPagesProgram = 0;
    OneNandCommandStatus OneNandStatus;
    NvU32 PagesRemaining;
    NvU32 PagesRequested;
    NvU32 CurrentDeviceLoop;
    NvBool IsProgramErrorOccured = NV_FALSE;
    NvU8 *pMainDataBuffer = (NvU8 *)pDataBuffer;
    NvU8 *pTagDataBuffer = (NvU8 *)pTagBuffer;

    NV_ASSERT(StartDeviceNum < MAX_CHIPSELECT);

    // Get the starting pages of all valid chip selects.
    for (Index = 0; Index < MAX_CHIPSELECT; ++Index)
    {
        if (pPageNumbers[StartIndex] != 0xFFFFFFFF)
        {
            ChipSelectId[MaxValidIndex] = StartIndex;
            StartPageNumber[MaxValidIndex] = pPageNumbers[StartIndex];
            MaxValidIndex++;
        }
        StartIndex = (StartIndex + 1)% MAX_CHIPSELECT;
    }
    
    // There should be the read request from any device.
    NV_ASSERT(MaxValidIndex);

    // Check whether the requested devices are available or not.
    for (Index = 0; Index < MaxValidIndex; ++Index)
    {
        if (!hOneNand->IsDevAvailable[ChipSelectId[Index]]) 
            return NvError_BadParameter;
    }

    // Enable the clock
    Error =  SetPowerControl(hOneNand, NV_TRUE);
    if (Error)
        return Error;

    NumberOfPagesProgram = 0;
    PagesRemaining = *pNoOfPages;
    PagesRequested = *pNoOfPages;

    CurrentDeviceLoop = NV_MIN(MaxValidIndex, PagesRemaining);
    
    // Write the data into the devices buffer ram first
    for (Index = 0; Index < CurrentDeviceLoop; ++Index)
    {
        CurrBlockNumber = BLOCK_NUMBER(StartPageNumber[Index],
                                hOneNand->OneNandDevInfo.PagesPerBlock);
        CurrPageNumber = PAGE_NUMBER(StartPageNumber[Index],
                                hOneNand->OneNandDevInfo.PagesPerBlock);

        StartPageNumber[Index]++;
        SetChipSelect(hOneNand, ChipSelectId[Index]);

        hOneNand->OneNandDevInt.DevSelectDataRamFromBlock(
                            &hOneNand->OneNandDevReg, &hOneNand->OneNandDevInfo, 
                            CurrBlockNumber);

        // Write the data now.
        hOneNand->OneNandDevInt.DevAsynchWriteIntoBufferRam(
                        &hOneNand->OneNandDevReg, pMainDataBuffer, pTagDataBuffer,
                        hOneNand->OneNandDevInfo.MainAreaSizePerPage,
                        hOneNand->OneNandDevInfo.SpareAreaSizePerPage);
        pMainDataBuffer += hOneNand->OneNandDevInfo.MainAreaSizePerPage;
        pTagDataBuffer +=  hOneNand->OneNandDevInfo.SpareAreaSizePerPage;

        // Issue the program command
        OneNandStatus = hOneNand->OneNandDevInt.DevProgramBlockPage(
                            &hOneNand->OneNandDevReg, &hOneNand->OneNandDevInfo, 
                            CurrBlockNumber, CurrPageNumber, NV_FALSE);
        if (OneNandStatus)
        {
            Error = NvError_NandWriteFailed;
            goto ErrorEnd;
        }
    }

    while (PagesRemaining)
    {
        CurrentDeviceLoop = NV_MIN(MaxValidIndex, PagesRemaining);
        
        // Check for program done
        for (Index = 0; Index < CurrentDeviceLoop; ++Index)
        {
            if ((Index == 0) && (IsProgramErrorOccured))
                goto ErrorEnd;
                
            SetChipSelect(hOneNand, ChipSelectId[Index]);
            
            // TODO: Int based ???
            do
            {
                OneNandStatus = hOneNand->OneNandDevInt.DevGetProgramStatus(
                            &hOneNand->OneNandDevReg); 
            } while (OneNandStatus == OneNandCommandStatus_Busy);
            
            if (OneNandStatus != OneNandCommandStatus_Success) 
            {
                Error = NvError_NandWriteFailed;
                goto ErrorEnd;
            }    
            NumberOfPagesProgram++;
            PagesRemaining--;

            // Copy the data and issue the program command again if it is 
            // required.
            if (PagesRequested >= (NumberOfPagesProgram + MaxValidIndex))
            {

                CurrBlockNumber = BLOCK_NUMBER(StartPageNumber[Index],
                                        hOneNand->OneNandDevInfo.PagesPerBlock);
                CurrPageNumber = PAGE_NUMBER(StartPageNumber[Index],
                                        hOneNand->OneNandDevInfo.PagesPerBlock);
                
                StartPageNumber[Index]++;
                hOneNand->OneNandDevInt.DevSelectDataRamFromBlock(
                                    &hOneNand->OneNandDevReg, &hOneNand->OneNandDevInfo, 
                                    CurrBlockNumber);
                
                // Write the data now.
                hOneNand->OneNandDevInt.DevAsynchWriteIntoBufferRam(
                                &hOneNand->OneNandDevReg, pMainDataBuffer, pTagDataBuffer,
                                hOneNand->OneNandDevInfo.MainAreaSizePerPage,
                                hOneNand->OneNandDevInfo.SpareAreaSizePerPage);
                pMainDataBuffer += hOneNand->OneNandDevInfo.MainAreaSizePerPage;
                pTagDataBuffer +=  hOneNand->OneNandDevInfo.SpareAreaSizePerPage;
                
                // Issue the program command
                OneNandStatus = hOneNand->OneNandDevInt.DevProgramBlockPage(
                                    &hOneNand->OneNandDevReg, &hOneNand->OneNandDevInfo, 
                                    CurrBlockNumber, CurrPageNumber, NV_FALSE);
                if (OneNandStatus)
                {
                    IsProgramErrorOccured = NV_TRUE;
                    Error = NvError_NandWriteFailed;
                }
            }
        }
    }

ErrorEnd:        
    SetPowerControl(hOneNand, NV_FALSE);
    *pNoOfPages = NumberOfPagesProgram;
    return Error;
}

NvError
NvDdkOneNandGetDeviceInfo(
    NvDdkOneNandHandle hOneNand,
    NvU8 DeviceNumber,
    NvDdkNandDeviceInfo* pDeviceInfo)
{
    if (!hOneNand->IsDevAvailable[DeviceNumber]) 
        return NvError_NotSupported;

    pDeviceInfo->VendorId = hOneNand->OneNandDevInfo.ManufId;
    pDeviceInfo->DeviceId = hOneNand->OneNandDevInfo.DeviceId;
    pDeviceInfo->TagSize = hOneNand->OneNandDevInfo.SpareAreaSizePerPage;
    pDeviceInfo->BusWidth = 16;
    pDeviceInfo->PageSize = hOneNand->OneNandDevInfo.MainAreaSizePerPage;
    pDeviceInfo->PagesPerBlock = hOneNand->OneNandDevInfo.PagesPerBlock;
    pDeviceInfo->NoOfBlocks = hOneNand->OneNandDevInfo.TotalBlocks;
    pDeviceInfo->ZonesPerDevice = 1;
    pDeviceInfo->DeviceCapacityInKBytes = (hOneNand->OneNandDevInfo.TotalBlocks*
                                          hOneNand->OneNandDevInfo.PagesPerBlock*
                                          hOneNand->OneNandDevInfo.MainAreaSizePerPage) >> 10;
    pDeviceInfo->InterleaveCapability = NV_TRUE;
    if (hOneNand->OneNandDevInfo.DeviceType == OneNandDeviceType_MuxOneNand)
        pDeviceInfo->NandType = NvOdmNandFlashType_Slc;
    else
        pDeviceInfo->NandType = NvOdmNandFlashType_Mlc;
    pDeviceInfo->NumberOfDevices = hOneNand->NumOfDevicesConnected;

    return NvError_Success;
}

NvError
NvDdkOneNandErase(
    NvDdkOneNandHandle hOneNand,
    NvU8 StartDeviceNum,
    NvU32* pPageNumbers,
    NvU32* pNumberOfBlocks)
{
    NvError Error = NvSuccess;
    NvU32 ChipSelectId[MAX_CHIPSELECT];
    NvU32 StartPageNumber[MAX_CHIPSELECT];
    NvU32 CurrBlockNumber;
    NvU32 MaxValidIndex = 0;
    NvU32 Index;
    NvU32 StartIndex = StartDeviceNum;
    NvU32 NumberOfBlockErased = 0;
    OneNandCommandStatus OneNandStatus;
    NvU32 BlocksRemaining;
    NvU32 BlocksRequested;
    NvU32 CurrentDeviceLoop;
    NvBool IsEraseErrorOccured = NV_FALSE;
    
    NV_ASSERT(StartDeviceNum < MAX_CHIPSELECT);
    
    // Get the starting pages of all valid chip selects.
    for (Index = 0; Index < MAX_CHIPSELECT; ++Index)
    {
        if (pPageNumbers[StartIndex] != 0xFFFFFFFF)
        {
            ChipSelectId[MaxValidIndex] = StartIndex;
            StartPageNumber[MaxValidIndex] = pPageNumbers[StartIndex];
            MaxValidIndex++;
        }
        StartIndex = (StartIndex + 1)% MAX_CHIPSELECT;
    }
    
    // Check whether the requested devices are available or not.
    for (Index = 0; Index < MaxValidIndex; ++Index)
    {
        if (!hOneNand->IsDevAvailable[ChipSelectId[Index]]) 
            return NvError_BadParameter;
    }

    // Enable the clock
    Error =  SetPowerControl(hOneNand, NV_TRUE);
    if (Error)
        return Error;
    
    NumberOfBlockErased = 0;
    BlocksRemaining = *pNumberOfBlocks;
    BlocksRequested = *pNumberOfBlocks;

    CurrentDeviceLoop = NV_MIN(MaxValidIndex, BlocksRemaining);
    
    // Issue the block erase first for all the selected devices.
    for (Index = 0; Index < CurrentDeviceLoop; ++Index)
    {
        CurrBlockNumber = BLOCK_NUMBER(StartPageNumber[Index],
                                hOneNand->OneNandDevInfo.PagesPerBlock);
        StartPageNumber[Index] += hOneNand->OneNandDevInfo.PagesPerBlock;
        SetChipSelect(hOneNand, ChipSelectId[Index]);

        hOneNand->OneNandDevInt.DevEraseBlock(
                            &hOneNand->OneNandDevReg, &hOneNand->OneNandDevInfo, 
                            CurrBlockNumber);
    }

    while (BlocksRemaining)
    {
        CurrentDeviceLoop = NV_MIN(MaxValidIndex, BlocksRemaining);
        
        // Check for erase done
        for (Index = 0; Index < CurrentDeviceLoop; ++Index)
        {
            if ((Index == 0) && (IsEraseErrorOccured))
                goto ErrorEnd;
                
            SetChipSelect(hOneNand, ChipSelectId[Index]);
            
            // TODO: Int based ???
            do
            {
                OneNandStatus = hOneNand->OneNandDevInt.DevGetEraseStatus(
                            &hOneNand->OneNandDevReg); 
            } while (OneNandStatus == OneNandCommandStatus_Busy);
            
            if (OneNandStatus != OneNandCommandStatus_Success) 
            {
                Error = NvError_NandEraseFailed;
                goto ErrorEnd;
            }    
            NumberOfBlockErased++;
            BlocksRemaining--;

            // Issue the erase command if there is more blcok erase required 
            // from this device.
            if (BlocksRequested >= (NumberOfBlockErased + MaxValidIndex))
            {

                CurrBlockNumber = BLOCK_NUMBER(StartPageNumber[Index],
                                        hOneNand->OneNandDevInfo.PagesPerBlock);
                
                StartPageNumber[Index] += hOneNand->OneNandDevInfo.PagesPerBlock;
                hOneNand->OneNandDevInt.DevEraseBlock(
                                    &hOneNand->OneNandDevReg, &hOneNand->OneNandDevInfo, 
                                    CurrBlockNumber);
            }
        }
    }

ErrorEnd:        
    SetPowerControl(hOneNand, NV_FALSE);
    *pNumberOfBlocks = NumberOfBlockErased;
    return Error;
}

void
NvDdkOneNandGetBlockInfo(
    NvDdkOneNandHandle hOneNand,
    NvU32 DeviceNumber,
    NvU32 BlockNumber,
    NandBlockInfo* pBlockInfo)
{
    NvError Error;

    OneNandCommandStatus OneNandStatus;
    
    // Check whether the requested devices are available or not.
    if (!hOneNand->IsDevAvailable[DeviceNumber]) 
        return;

    // Enable the clock
    Error =  SetPowerControl(hOneNand, NV_TRUE);
    if (Error)
    {
        NV_ASSERT(Error == NvSuccess);
        return;
    }    

    SetChipSelect(hOneNand, DeviceNumber);
    pBlockInfo->IsBlockLocked = hOneNand->OneNandDevInt.DevIsBlockLocked(
                            &hOneNand->OneNandDevReg, &hOneNand->OneNandDevInfo, 
                            BlockNumber);
    pBlockInfo->IsFactoryGoodBlock = NV_TRUE;

    // Issue the load command.
    OneNandStatus = hOneNand->OneNandDevInt.DevLoadBlockPage(
                    &hOneNand->OneNandDevReg, &hOneNand->OneNandDevInfo, 
                    BlockNumber, 0, NV_TRUE);
                            
    if (OneNandStatus == OneNandCommandStatus_Success)
    {
        // Read from device buffer ram now.
        hOneNand->OneNandDevInt.DevAsynchReadFromBufferRam(
                        &hOneNand->OneNandDevReg, NULL, pBlockInfo->pTagBuffer,
                        hOneNand->OneNandDevInfo.MainAreaSizePerPage,
                        hOneNand->OneNandDevInfo.SpareAreaSizePerPage);
    }
    SetPowerControl(hOneNand, NV_FALSE);
}

void 
NvDdkOneNandGetLockedRegions(
    NvDdkOneNandHandle hOneNand,
    LockParams* pFlashLockParams)
{

}

void 
NvDdkOneNandSetFlashLock(
    NvDdkOneNandHandle hOneNand,
    LockParams* pFlashLockParams)
{
    NvU32 StartPageNumber;
    NvU32 NumberOfPages;
    NvU32 PagesRemaining;
    NvU32 CurrBlockNumber;
    OneNandCommandStatus OneNandStatus;
    NvError Error;
    
    NV_ASSERT(pFlashLockParams);
    
    // Check whether the requested devices are available or not.
    if (!hOneNand->IsDevAvailable[pFlashLockParams->DeviceNumber]) 
        return;

    StartPageNumber = pFlashLockParams->StartPageNumber;
    NumberOfPages = pFlashLockParams->EndPageNumber - 
                            pFlashLockParams->StartPageNumber;
    NV_ASSERT(NumberOfPages);

    // Enable the clock
    Error =  SetPowerControl(hOneNand, NV_TRUE);
    if (Error)
    {
        NV_ASSERT(Error == NvSuccess);
        return;
    }    
    
    PagesRemaining = NumberOfPages;

    SetChipSelect(hOneNand, pFlashLockParams->DeviceNumber);
    while (PagesRemaining)
    {
        CurrBlockNumber = BLOCK_NUMBER(StartPageNumber,
                                hOneNand->OneNandDevInfo.PagesPerBlock);
        StartPageNumber += hOneNand->OneNandDevInfo.PagesPerBlock;
        
        OneNandStatus = hOneNand->OneNandDevInt.DevSetBlockLockStatus(
                            &hOneNand->OneNandDevReg, &hOneNand->OneNandDevInfo, 
                            CurrBlockNumber, NV_TRUE);
        if (OneNandStatus != OneNandCommandStatus_Success) 
            goto ErrorEnd;

        if (PagesRemaining >= hOneNand->OneNandDevInfo.PagesPerBlock) 
            PagesRemaining -= hOneNand->OneNandDevInfo.PagesPerBlock;
        else
            PagesRemaining = 0;
    }

ErrorEnd:        
    SetPowerControl(hOneNand, NV_FALSE);
    return;
}

void NvDdkOneNandReleaseFlashLock(NvDdkOneNandHandle hOneNand)
{
    OneNandCommandStatus OneNandStatus;
    NvU32 Index;
    NvError Error;
    
    NV_ASSERT(hOneNand);

    // Enable the clock
    Error =  SetPowerControl(hOneNand, NV_TRUE);
    if (Error)
    {
        NV_ASSERT(Error == NvSuccess);
        return;
    }    
    
    // Release the lock of all the available devices.
    for (Index = 0; Index < MAX_CHIPSELECT; ++Index)
    {
        if (!hOneNand->IsDevAvailable[Index]) 
            continue;
            
        SetChipSelect(hOneNand, Index);
            
        OneNandStatus = hOneNand->OneNandDevInt.DevUnlockAllBlockOfChip(
                            &hOneNand->OneNandDevReg, &hOneNand->OneNandDevInfo);
        if (OneNandStatus != OneNandCommandStatus_Success) 
            goto ErrorEnd;
    }

ErrorEnd:        
    SetPowerControl(hOneNand, NV_FALSE);
    return;
}

NvError NvDdkOneNandSuspend(NvDdkOneNandHandle hOneNand)
{
    return NvError_NotImplemented;
}

NvError NvDdkOneNandResume(NvDdkOneNandHandle hOneNand)
{
    return NvError_NotImplemented;
}

NvError
NvDdkOneNandBlockDevOpen(
    NvU32 Instance,
    NvU32 MinorInstance,
    NvDdkBlockDevHandle *phBlockDev)
{
    return NvError_NotImplemented;
}

NvError NvDdkOneNandBlockDevInit(NvRmDeviceHandle hDevice)
{
    return NvError_NotImplemented;
}

void NvDdkOneNandBlockDevDeinit(void)
{
}
