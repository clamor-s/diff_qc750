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
 *           The ddk SNOR API implementation</b>
 *
 * @b Description:  Implements  the public interfacing functions for the SNOR
 * device driver interface.
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
#include "nvddk_snor.h"
#include "snor_priv_driver.h"

#define DEBUG_PRINTS 0
#define SPANSION_TIMING0 0xA0400273
#define SPANSION_TIMING1 0x00030402

#if DEBUG_PRINTS
#define SNOR_PRINT(fmt,args...) NvOsDebugPrintf(fmt, ##args)
#else
#define SNOR_PRINT(fmt,args...)
#endif

#define MAX_CHIPSELECT 8
#define BLOCK_NUMBER(PageNumber, PagesPerBlock) ((PageNumber)/(PagesPerBlock))
#define PAGE_NUMBER(PageNumber, PagesPerBlock) ((PageNumber)%(PagesPerBlock))

NvError
SnorGetPhysAdd(
    NvRmDeviceHandle hRmDevice,
    NvRmMemHandle* hRmMemHandle,
    void** pVirtBuffer,
    NvU32 size,
    NvU32* pPhysBuffer);

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

typedef struct NvDdkSnorRec
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

    // Nor Device Info to the client
    // NvDdkSnorDeviceInfo DdkSnorDevInfo;

    // Add pointers to SNOR device specific functions
    SnorDevInterface SnorDevInt;

    // SNOR device information connected to the controller.
    SnorDeviceInfo SnorDevInfo;

    // SNOR device interface register to access the devices.
    SnorDeviceIntRegister SnorDevReg;

    NvOsMutexHandle hDevAccess;

    NvOsMutexHandle hSnorRegAccess;

    SnorControllerRegs SnorRegs;
    // Rmmemory Handle of the buffer allocated
    NvRmMemHandle hRmMemHandle;
    // Physical Buffer Address of the memory
    NvU32 pPhysBuffer;
    // Virtual Buffer Address
    void* pVirtBuffer;
} NvDdkSnor;

typedef struct
{
    NvRmDeviceHandle hRmDevice;
    NvDdkSnorHandle hSnor ;
    NvOsMutexHandle hSnorOpenMutex;
} DdkSnorInformation;

static DdkSnorInformation *s_pSnorInfo = NULL;

static void SnorISR(void *pArgs)
{
    NvDdkSnorHandle hSnor = (NvDdkSnorHandle)pArgs;
    volatile NvU32 DmaConfig, SnorConfig;
    DmaConfig = SNOR_READ32(hSnor->pSnorVirtAdd, DMA_CFG);
    SnorConfig = SNOR_READ32(hSnor->pSnorVirtAdd, CONFIG);
    if (NV_DRF_VAL(SNOR, DMA_CFG, IS_DMA_DONE, DmaConfig))
    {
        // Disable NOR GO and DMA GO
        DmaConfig = NV_FLD_SET_DRF_DEF(SNOR, DMA_CFG, DMA_GO, DISABLE, DmaConfig);
        DmaConfig = NV_FLD_SET_DRF_DEF(SNOR, DMA_CFG, IS_DMA_DONE, ENABLE, DmaConfig);
        DmaConfig = NV_FLD_SET_DRF_DEF(SNOR, DMA_CFG, IE_DMA_DONE, DISABLE, DmaConfig);
        SnorConfig = NV_FLD_SET_DRF_DEF(SNOR, CONFIG, GO_NOR, DISABLE, SnorConfig);
        hSnor->SnorRegs.Config = SnorConfig;
        hSnor->SnorRegs.DmaConfig = DmaConfig;
        SNOR_WRITE32(hSnor->pSnorVirtAdd, DMA_CFG, DmaConfig);
        SNOR_WRITE32(hSnor->pSnorVirtAdd, CONFIG, SnorConfig);
        NvOsSemaphoreSignal(hSnor->hCommandCompleteSema);
    }

    NvRmInterruptDone(hSnor->hIntr);
}

static void InitSnorController(NvDdkSnorHandle hSnor)
{
    NvU32 SnorConfig = SNOR_READ32(hSnor->pSnorVirtAdd, CONFIG);

     // SNOR_CONFIG_0_WORDWIDE_GMI_NOR16BIT
     // SNOR_CONFIG_0_NOR_DEVICE_TYPE_SNOR
     // SNOR_CONFIG_0_MUXMODE_GMI_AD_MUX
     // NVTODO : Changing initial state to 16 bit, non-muxed
     // NVTODO : Can this be changed later ?
    SnorConfig = NV_FLD_SET_DRF_DEF(SNOR, CONFIG, NOR_DEVICE_TYPE, SNOR, SnorConfig);
    SnorConfig = NV_FLD_SET_DRF_DEF(SNOR, CONFIG, WORDWIDE_GMI, NOR16BIT, SnorConfig);
    SnorConfig = NV_FLD_SET_DRF_DEF(SNOR, CONFIG, MUXMODE_GMI, AD_NONMUX, SnorConfig);

    hSnor->SnorRegs.Config = SnorConfig;
    SNOR_WRITE32(hSnor->pSnorVirtAdd, CONFIG, SnorConfig);
    SNOR_WRITE32(hSnor->pSnorVirtAdd, TIMING0, SPANSION_TIMING0);
    SNOR_WRITE32(hSnor->pSnorVirtAdd, TIMING1, SPANSION_TIMING1);
}

static void SetChipSelect(NvDdkSnorHandle hSnor, NvU32 ChipSelId)
{
    NvU32 SnorConfig = hSnor->SnorRegs.Config;
    SnorConfig = NV_FLD_SET_DRF_NUM(SNOR, CONFIG, SNOR_SEL, ChipSelId, SnorConfig);

    hSnor->SnorRegs.Config = SnorConfig;
    SNOR_WRITE32(hSnor->pSnorVirtAdd, CONFIG, SnorConfig);
}

static NvError RegisterSnorInterrupt(NvDdkSnorHandle hSnor)
{
    NvU32 IrqList;
    NvOsInterruptHandler IntHandle = SnorISR;

    IrqList = NvRmGetIrqForLogicalInterrupt(hSnor->hRmDevice,
                  NVRM_MODULE_ID(NvRmModuleID_SyncNor, 0),
                  0);
    return NvRmInterruptRegister(hSnor->hRmDevice,
               1,
               &IrqList,
               &IntHandle,
               hSnor,
               &hSnor->hIntr,
               NV_TRUE);
}

static void
NvDdkPrivSnorSlaveMode(
    NvDdkSnorHandle hSnor)
{
    volatile NvU32 SnorConfig = hSnor->SnorRegs.Config;
    // Controller in Slave Mode. As all the reads are in DMA mode it puts
    // the controller into Master Mode. Hence, this switch is needed for
    // all writes.
    SnorConfig = NV_FLD_SET_DRF_DEF(SNOR, CONFIG, DEVICE_MODE, ASYNC, SnorConfig);
    SnorConfig = NV_FLD_SET_DRF_DEF(SNOR, CONFIG, MST_ENB, DISABLE, SnorConfig);
    // Enable Writes
    SnorConfig = NV_FLD_SET_DRF_DEF(SNOR, CONFIG, NOR_WP ,ENABLE, SnorConfig);
    SNOR_WRITE32(hSnor->pSnorVirtAdd, CONFIG, SnorConfig);
}

/**
 * Initialize the SNOR information.
 */
static NvError InitSnorInformation(NvRmDeviceHandle hRmDevice)
{
    NvError Error = NvSuccess;
    DdkSnorInformation *pSnorInfo = NULL;

    if (!s_pSnorInfo)
    {
        // Allocate the memory for the snor information.
        pSnorInfo = NvOsAlloc(sizeof(*pSnorInfo));
        if (!pSnorInfo)
            return NvError_InsufficientMemory;
        NvOsMemset(pSnorInfo, 0, sizeof(*pSnorInfo));

        // Initialize all the parameters.
        pSnorInfo->hRmDevice = hRmDevice;
        pSnorInfo->hSnorOpenMutex = NULL;
        pSnorInfo->hSnor = NULL;

        // Create the mutex to access the Nor information.
        Error = NvOsMutexCreate(&pSnorInfo->hSnorOpenMutex);
        // If error exit till then destroy all allocations.
        if (Error)
        {
            NvOsFree(pSnorInfo);
            pSnorInfo = NULL;
            return Error;
        }

        if (NvOsAtomicCompareExchange32((NvS32*)&s_pSnorInfo, 0, (NvS32)pSnorInfo)!=0)
        {
            NvOsMutexDestroy(pSnorInfo->hSnorOpenMutex);
            NvOsFree(pSnorInfo);
        }
    }
    return NvSuccess;
}

static NvError SetPowerControl(NvDdkSnorHandle hSnor, NvBool IsEnable)
{
    NvError Error = NvSuccess;
    NvRmModuleID ModuleId;
    const NvRmFreqKHz PrefFreqList[] = {200000, 150000, 100000, 86000, NvRmFreqUnspecified};
    NvU32 ListCount = NV_ARRAY_SIZE(PrefFreqList);
    NvRmFreqKHz CurrentFreq = 0;

    ModuleId = NVRM_MODULE_ID(NvRmModuleID_SyncNor, hSnor->InstanceId);
    if (IsEnable)
    {
        // Enable power for Snor module
        Error = NvRmPowerVoltageControl(hSnor->hRmDevice,
                    ModuleId,
                    hSnor->RmPowerClientId,
                    NvRmVoltsUnspecified,
                    NvRmVoltsUnspecified,
                    NULL, 0, NULL);
        if (Error)
            return Error;

        // Enable the clock.
        Error = NvRmPowerModuleClockControl(hSnor->hRmDevice,
                    ModuleId,
                    hSnor->RmPowerClientId,
                    NV_TRUE);
        if (Error)
            return Error;

        // Configure Clock
        Error = NvRmPowerModuleClockConfig(hSnor->hRmDevice,
                    NvRmModuleID_SyncNor,
                    hSnor->RmPowerClientId,
                    PrefFreqList[ListCount-1],
                    PrefFreqList[0],
                    PrefFreqList,
                    ListCount,
                    &CurrentFreq, 0);
    }
    else
    {
        // Disable the clocks.
        Error = NvRmPowerModuleClockControl(
                    hSnor->hRmDevice,
                    ModuleId,
                    hSnor->RmPowerClientId,
                    NV_FALSE);

        // Disable the power to the controller.
        if (!Error)
            Error = NvRmPowerVoltageControl(
                        hSnor->hRmDevice,
                        ModuleId,
                        hSnor->RmPowerClientId,
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
 * Destroy the SNOR handle.
 */
static void DestroySnorHandle(NvDdkSnorHandle hSnor)
{
    NvRmModuleID ModuleId;
    // If null pointer for SNOR handle then return error.
    if (!hSnor)
        return;

    ModuleId = NVRM_MODULE_ID(NvRmModuleID_SyncNor, hSnor->InstanceId);

    if (hSnor->RmPowerClientId)
    {
        SetPowerControl(hSnor, NV_FALSE);

        // Unregister for the power manager.
        NvRmPowerUnRegister(hSnor->hRmDevice, hSnor->RmPowerClientId);
        NvOsSemaphoreDestroy(hSnor->hRmPowerEventSema);
    }

    if (hSnor->hIntr)
    {
        NvRmInterruptUnregister(hSnor->hRmDevice, hSnor->hIntr);
        hSnor->hIntr = NULL;
    }

    // Unmap the virtual mapping of the snor controller.
    if (hSnor->pSnorVirtAdd)
        NvRmPhysicalMemUnmap(hSnor->pSnorVirtAdd, hSnor->SnorBankSize);

    // Unmap the virtual mapping of the Nor interfacing register.
    if (hSnor->SnorDevReg.pNorBaseAdd)
        NvRmPhysicalMemUnmap(hSnor->SnorDevReg.pNorBaseAdd,
            hSnor->SnorDevReg.NorAddressSize);

#ifndef SET_KERNEL_PINMUX
    // Tri-State the pin-mux pins
    NV_ASSERT_SUCCESS(NvRmSetModuleTristate(hSnor->hRmDevice, ModuleId, NV_TRUE));
#endif
    // Destroy the sync sempahores.
    NvOsSemaphoreDestroy(hSnor->hCommandCompleteSema);

    NvOsMutexDestroy(hSnor->hSnorRegAccess);

    // Free the memory of the SNOR handles.
    NvOsFree(hSnor);
}

NvError
SnorGetPhysAdd(
    NvRmDeviceHandle hRmDevice,
    NvRmMemHandle* hRmMemHandle,
    void** pVirtBuffer,
    NvU32 size,
    NvU32* pPhysBuffer)
{
    NvU32 SNOR_ALIGNMENT_SIZE = 4*1024*4; // NVTODO : Make this an enum. 4K 32 bit words
    NvError Error = NvError_InvalidAddress;

    // Initialise the handle to NULL
    *pPhysBuffer = 0;
    *hRmMemHandle = NULL;

    // Create the Memory Handle
    Error = NvRmMemHandleCreate(hRmDevice, hRmMemHandle, size);
    if (Error != NvSuccess)
    {
        return Error;
    }

    // Allocate the memory
    Error = NvRmMemAlloc(*hRmMemHandle,
                NULL,
                0,
                SNOR_ALIGNMENT_SIZE,
                NvOsMemAttribute_Uncached);
    if (Error != NvSuccess)
    {
        NvRmMemHandleFree(*hRmMemHandle);
        return Error;
    }

    // Pin the memory and Get Physical Address
    *pPhysBuffer = NvRmMemPin(*hRmMemHandle);

    Error = NvRmMemMap(*hRmMemHandle, 0, size, NVOS_MEM_READ_WRITE, pVirtBuffer);
    if (Error != NvSuccess)
    {
        NvRmMemUnpin(*hRmMemHandle);
        NvRmMemHandleFree(*hRmMemHandle);
        return Error;
    }

    return NvSuccess;
}

static NvError
CreateSnorHandle(
    NvRmDeviceHandle hRmDevice,
    NvU32 InstanceId,
    NvDdkSnorHandle *pSnor)
{
    NvError Error = NvSuccess;
    NvDdkSnorHandle hNewSnor = NULL;
    NvRmModuleID ModuleId;
    NvU32 Index;
    SnorDeviceType DeviceType;
    NvU32 ResetDelayTime;
    NvU32 MaxResetDelayTime;

    ModuleId = NVRM_MODULE_ID(NvRmModuleID_SyncNor, InstanceId);
    *pSnor = NULL;

#ifndef SET_KERNEL_PINMUX
    // Enable the pin mux to normal state.
    if (NvRmSetModuleTristate(hRmDevice, ModuleId, NV_FALSE) != NvSuccess)
    {
        return NvError_NotSupported;
    }
#endif

    // Allcoate the memory for the Snor handle.
    hNewSnor = NvOsAlloc(sizeof(NvDdkSnor));
    if (!hNewSnor)
    {
        return NvError_InsufficientMemory;
    }

    // Reset the memory allocated for the Snor handle.
    NvOsMemset(hNewSnor, 0, sizeof(*hNewSnor));

    // Set the Snor handle parameters.
    hNewSnor->hRmDevice = hRmDevice;
    hNewSnor->InstanceId = InstanceId;
    hNewSnor->OpenCount = 0;

    hNewSnor->SnorControllerBaseAdd = 0;
    hNewSnor->pSnorVirtAdd = NULL;
    hNewSnor->SnorBankSize = 0;

    hNewSnor->hIntr = NULL;

    hNewSnor->hRmPowerEventSema = NULL;
    hNewSnor->RmPowerClientId = 0;

    // Command complete semaphore
    hNewSnor->hCommandCompleteSema = NULL;

    hNewSnor->NumOfDevicesConnected = 0;
    for (Index = 0; Index < MAX_CHIPSELECT; ++Index)
        hNewSnor->IsDevAvailable[Index] = 0;


    // Default to x16 Non muxed
    hNewSnor->SnorDevInfo.DeviceType = SnorDeviceType_x16_NonMuxed;

    hNewSnor->SnorDevReg.pNorBaseAdd = NULL;
    hNewSnor->SnorDevReg.NorAddressSize = 0;

    hNewSnor->hDevAccess = NULL;
    hNewSnor->hSnorRegAccess = NULL;

    hNewSnor->SnorRegs.Config = NV_RESETVAL(SNOR, CONFIG);
    hNewSnor->SnorRegs.Status = NV_RESETVAL(SNOR, STA);
    hNewSnor->SnorRegs.NorAddressPtr = NV_RESETVAL(SNOR, NOR_ADDR_PTR);
    hNewSnor->SnorRegs.AhbAddrPtr = NV_RESETVAL(SNOR, AHB_ADDR_PTR);
    hNewSnor->SnorRegs.Timing0 = NV_RESETVAL(SNOR, TIMING0);
    hNewSnor->SnorRegs.Timing1 = NV_RESETVAL(SNOR, TIMING1);
    hNewSnor->SnorRegs.MioCfg = NV_RESETVAL(SNOR, MIO_CFG);
    hNewSnor->SnorRegs.MioTiming = NV_RESETVAL(SNOR, MIO_TIMING0);
    hNewSnor->SnorRegs.DmaConfig = NV_RESETVAL(SNOR, DMA_CFG);
    hNewSnor->SnorRegs.ChipSelectMuxConfig = NV_RESETVAL(SNOR, CS_MUX_CFG);

    // Initialize the base addresses of snor controller
    NvRmModuleGetBaseAddress(hRmDevice,
        ModuleId,
        &hNewSnor->SnorControllerBaseAdd,
        &hNewSnor->SnorBankSize);

    Error = NvRmPhysicalMemMap(hNewSnor->SnorControllerBaseAdd,
                hNewSnor->SnorBankSize,
                NVOS_MEM_READ_WRITE,
                NvOsMemAttribute_Uncached,
                (void **)&(hNewSnor->pSnorVirtAdd));
    SNOR_PRINT("(hNewSnor->SnorControllerBaseAdd) 0x%08x\n",(hNewSnor->SnorControllerBaseAdd));
    SNOR_PRINT("(hNewSnor->pSnorVirtAdd) 0x%08x\n",(hNewSnor->pSnorVirtAdd));
    if (Error)
    {
        SNOR_PRINT(("[NvDdkSnorOpen] SNOR Controller register mapping failed\n"));
        goto ErrorEnd;
    }

    // Initialize the base addresses of snor controller
    NvRmModuleGetBaseAddress(hRmDevice,
        NVRM_MODULE_ID(NvRmModuleID_Nor, 0),
        &hNewSnor->SnorDevReg.NorBaseAddress,
        &hNewSnor->SnorDevReg.NorAddressSize);

    Error = NvRmPhysicalMemMap(hNewSnor->SnorDevReg.NorBaseAddress,
                hNewSnor->SnorDevReg.NorAddressSize,
                NVOS_MEM_READ_WRITE, NvOsMemAttribute_Uncached,
                (void **)&(hNewSnor->SnorDevReg.pNorBaseAdd));
    SNOR_PRINT("(hNewSnor->SnorDevReg.pNorBaseAdd) 0x%08x",(hNewSnor->SnorDevReg.pNorBaseAdd));
    if (Error)
    {
        SNOR_PRINT(("[NvDdkSnorOpen] Nor Base register mapping failed\n"));
        goto ErrorEnd;
    }

    // Register as the Rm power client
    Error = NvOsSemaphoreCreate(&hNewSnor->hRmPowerEventSema, 0);
    if (Error)
        goto ErrorEnd;

    NV_ASSERT_SUCCESS(NvRmPowerRegister(hNewSnor->hRmDevice,
                    hNewSnor->hRmPowerEventSema,
                    &hNewSnor->RmPowerClientId));

    NV_ASSERT_SUCCESS(SetPowerControl(hNewSnor, NV_TRUE));

    // Reset the snor controller.
    NvRmModuleReset(hRmDevice, ModuleId);

    NV_ASSERT_SUCCESS(RegisterSnorInterrupt(hNewSnor));

    NV_ASSERT_SUCCESS(NvOsSemaphoreCreate(&hNewSnor->hCommandCompleteSema, 0));

    InitSnorController(hNewSnor);

    hNewSnor->NumOfDevicesConnected = 0;
    DeviceType = SnorDeviceType_Unknown;
    for (Index = 0; Index < MAX_CHIPSELECT; ++Index)
    {
        // Select Chipselect
        SetChipSelect(hNewSnor, Index);

        hNewSnor->IsDevAvailable[Index] = IsSnorDeviceAvailable(
                                &hNewSnor->SnorDevReg, Index);
        if (hNewSnor->IsDevAvailable[Index])
        {
            hNewSnor->NumOfDevicesConnected++;
            DeviceType = GetSnorDeviceType(&hNewSnor->SnorDevReg);
            NV_ASSERT(DeviceType != SnorDeviceType_Unknown);
        }
    }

    // At this point we should have Snor Device
    NV_ASSERT(DeviceType != SnorDeviceType_Unknown);
    if (DeviceType == SnorDeviceType_Unknown)
    {
        Error = NvError_ModuleNotPresent;
        goto ErrorEnd;
    }

    hNewSnor->SnorDevInfo.DeviceType = DeviceType;

    NvRmPrivSnorDeviceInterface(&hNewSnor->SnorDevInt,
        &hNewSnor->SnorDevReg,
        &hNewSnor->SnorDevInfo);

    // Reset all available devices
    // Send reset command to all devices.
    MaxResetDelayTime = 0;
    for (Index = 0; Index < MAX_CHIPSELECT; ++Index)
    {
        if (hNewSnor->IsDevAvailable[Index])
        {
            // Select Chipselect
            SetChipSelect(hNewSnor, Index);
            ResetDelayTime = hNewSnor->SnorDevInt.DevResetDevice(
                                        &hNewSnor->SnorDevReg, Index);
            MaxResetDelayTime = NV_MAX(MaxResetDelayTime, ResetDelayTime);
        }
    }

    // Populate the device information
    for (Index = 0; Index < MAX_CHIPSELECT; ++Index)
    {
        if (hNewSnor->IsDevAvailable[Index])
        {
            // Select Chipselect
            SetChipSelect(hNewSnor, Index);
            hNewSnor->SnorDevInt.DevGetDeviceInfo(
                                &hNewSnor->SnorDevReg,
                                &hNewSnor->SnorDevInfo,
                                Index);
            break;
        }
    }

    Error = SnorGetPhysAdd(hNewSnor->hRmDevice, &hNewSnor->hRmMemHandle,
            &hNewSnor->pVirtBuffer,4096*4, &hNewSnor->pPhysBuffer); // NVTODO : Make size as macro
    if (Error)
    {
        SNOR_PRINT(("[NvDdkSnorOpen] Buffer Creation Failed\n"));
        goto ErrorEnd;
    }


 ErrorEnd:
    // If error then destroy all the allocation done here.
    if (Error)
    {
        DestroySnorHandle(hNewSnor);
        hNewSnor = NULL;
    }
    *pSnor = hNewSnor;
    return Error;
}


NvError
NvDdkSnorOpen(
    NvRmDeviceHandle hRmDevice,
    NvU8 Instance,
    NvDdkSnorHandle *phSnor)
{
    NvError Error;

    NV_ASSERT(hRmDevice);

    *phSnor = NULL;
    Error = InitSnorInformation(hRmDevice);
    if (Error)
        return Error;

    NvOsMutexLock(s_pSnorInfo->hSnorOpenMutex);
    if (!s_pSnorInfo->hSnor)
        Error = CreateSnorHandle(hRmDevice, Instance,  &s_pSnorInfo->hSnor);

    if (!Error)
        s_pSnorInfo->hSnor->OpenCount++;

    *phSnor = s_pSnorInfo->hSnor;
    NvOsMutexUnlock(s_pSnorInfo->hSnorOpenMutex);
    return Error;
}

void NvDdkSnorClose(NvDdkSnorHandle hSnor)
{
    if (!hSnor)
        return;

    NvOsMutexLock(s_pSnorInfo->hSnorOpenMutex);
    if (hSnor->OpenCount)
        hSnor->OpenCount--;

    if (!hSnor->OpenCount)
    {
        DestroySnorHandle(hSnor);
        s_pSnorInfo->hSnor = NULL;
    }
    NvOsMutexUnlock(s_pSnorInfo->hSnorOpenMutex);
}

NvError
NvDdkSnorGetNumBlocks(NvDdkSnorHandle hSnor, NvU32 *Size)
{
    *Size = hSnor->SnorDevInfo.TotalBlocks;
    return NvSuccess;
}

NvError
NvDdkSnorFormatDevice(
    NvDdkSnorHandle hSnor,
    NvU32 *SnorStatus)
{
    NvDdkPrivSnorSlaveMode(hSnor);
    hSnor->SnorDevInt.DevFormatDevice(&(hSnor->SnorDevReg),
                                      &(hSnor->SnorDevInfo),
                                      NV_TRUE);
    return NvSuccess;
}


NvError
NvDdkSnorEraseSectors(
    NvDdkSnorHandle hSnor,
    NvU32 StartOffset,
    NvU32 Length)
{
    NvDdkPrivSnorSlaveMode(hSnor);
    hSnor->SnorDevInt.DevEraseSector(&(hSnor->SnorDevReg),
                                     &(hSnor->SnorDevInfo),
                                     StartOffset,
                                     Length,
                                     NV_TRUE);
    return NvSuccess;
}

NvError
NvDdkSnorProtectSectors(
    NvDdkSnorHandle hSnor,
    NvU32 StartOffset,
    NvU32 Length,
    NvBool IsLastPartition)
{
    NvError Error = NvSuccess;
    NvDdkPrivSnorSlaveMode(hSnor);
    if (! hSnor->SnorDevInt.DevProtectSectors )
    {
        Error = NvError_NotSupported;
        return Error;
    }
    Error = hSnor->SnorDevInt.DevProtectSectors(&(hSnor->SnorDevReg),
                                                &(hSnor->SnorDevInfo),
                                                StartOffset,
                                                Length,
                                                IsLastPartition);
    return Error;
}

NvError
NvDdkSnorUnprotectAllSectors(
    NvDdkSnorHandle hSnor)
{
    NvError Error = NvSuccess;
    NvDdkPrivSnorSlaveMode(hSnor);
    if (! hSnor->SnorDevInt.DevUnprotectAllSectors)
    {
        Error = NvError_NotSupported;
        return Error;
    }
    Error = hSnor->SnorDevInt.DevUnprotectAllSectors(&(hSnor->SnorDevReg),
                                                     &(hSnor->SnorDevInfo));
    return Error;
}

extern int RunMemCmpTest;
NvError
NvDdkSnorRead(
    NvDdkSnorHandle hSnor,
    NvU8 ChipNum,
    NvU32 StartByteOffset,
    NvU32 SizeInBytes,
    NvU8* pBuffer,
    NvU32* SnorStatus)
{
    NvError Error = NvSuccess;
    NvU32 *NorAddress = (NvU32*)((NvU32)(hSnor->SnorDevReg.pNorBaseAdd) + StartByteOffset);
    NvU32 PhysAddr = hSnor->pPhysBuffer;
    volatile NvU32 SnorConfig = hSnor->SnorRegs.Config;
    volatile NvU32 DmaConfig = hSnor->SnorRegs.DmaConfig;

    // AHB address can be Ext Mem or IRAM
    NvU32 *AHBAddress = (NvU32*)pBuffer;
    NvU32 *TempAHBAddress, *TempNorAddress;
    // Irrespective of the chip used, NOR DMA always
    // is in terms of 32-bit words (AHB Word Length).
    NvS32 NumAHBWords = (NvS32)(SizeInBytes >> 2) ;
    NvS32 CurrentTransferLength =0 ;
    NV_ASSERT(ChipNum < MAX_CHIPSELECT);

    // Check whether the requested devices are available or not.
    if(!hSnor->IsDevAvailable[ChipNum])
        return NvError_BadParameter;

    // Enable the clock
    Error =  SetPowerControl(hSnor, NV_TRUE);
    if (Error)
        return Error;
    // For now the requests are only for chipselect 0
    SetChipSelect(hSnor,ChipNum);
    for( TempAHBAddress = AHBAddress, TempNorAddress = NorAddress ;
            NumAHBWords > 0 ;
            TempAHBAddress += (CurrentTransferLength), TempNorAddress += (CurrentTransferLength),
            NumAHBWords -= CurrentTransferLength)
    {
        // NVTODO : Do we need to fallback for PIO reads ?
        // NVTODO : While using DMA, check if page/burst mode is supported. ODM ?
        // Controller in Master Mode. For all writes, NOR controller is in Slave Mode
        // Hence, this switch is needed for all writes.
        SnorConfig = NV_FLD_SET_DRF_DEF(SNOR, CONFIG, MST_ENB, ENABLE, SnorConfig);
        SnorConfig = NV_FLD_SET_DRF_DEF(SNOR, CONFIG, DEVICE_MODE, PAGE, SnorConfig);
        SnorConfig = NV_FLD_SET_DRF_DEF(SNOR, CONFIG, PAGE_SIZE, PG8WORD, SnorConfig);
        hSnor->SnorRegs.Config = SnorConfig;
        SNOR_WRITE32(hSnor->pSnorVirtAdd, CONFIG, SnorConfig);
        // AHB Burst Length, Interrupt Enable, Direction of Transfer (DMA)
        DmaConfig = NV_FLD_SET_DRF_DEF(SNOR, DMA_CFG, BURST_SIZE, BS1WORD, DmaConfig);
        DmaConfig = NV_FLD_SET_DRF_DEF(SNOR, DMA_CFG, IE_DMA_DONE, ENABLE, DmaConfig);
        DmaConfig = NV_FLD_SET_DRF_DEF(SNOR, DMA_CFG, DIR, NOR2AHB, DmaConfig);
        CurrentTransferLength = (NumAHBWords > 0x400) ? 0x400 : NumAHBWords;
        // Setup DMA
        // a. NOR (From) and AHB (To) Addresses.
        SNOR_WRITE32(hSnor->pSnorVirtAdd, NOR_ADDR_PTR, (NvU32)TempNorAddress);
        SNOR_WRITE32(hSnor->pSnorVirtAdd, AHB_ADDR_PTR, (NvU32)PhysAddr);
        // b. DMA Word Count. Specified as (#Words-1)
        DmaConfig = NV_FLD_SET_DRF_NUM(SNOR, DMA_CFG, WORD_COUNT, CurrentTransferLength-1, DmaConfig);
        // c. Enable DMA GO and GO SNOR
        DmaConfig = NV_FLD_SET_DRF_DEF(SNOR, DMA_CFG, DMA_GO, ENABLE, DmaConfig);
        SnorConfig = NV_FLD_SET_DRF_DEF(SNOR, CONFIG, GO_NOR, ENABLE, SnorConfig);
        // d. Set the correct timing
        SNOR_WRITE32(hSnor->pSnorVirtAdd, TIMING0, SPANSION_TIMING0);
        SNOR_WRITE32(hSnor->pSnorVirtAdd, TIMING1, SPANSION_TIMING1);
        /* Read Back all the registers */
        hSnor->SnorRegs.Config = SnorConfig;
        hSnor->SnorRegs.DmaConfig = DmaConfig;
        SNOR_WRITE32(hSnor->pSnorVirtAdd, CONFIG , SnorConfig);
        SNOR_WRITE32(hSnor->pSnorVirtAdd, DMA_CFG, DmaConfig);
        // Wait for Completion
        // NVTODO : Do a timed wait and handle error conditions
        NvOsSemaphoreWait(hSnor->hCommandCompleteSema);
        NvOsMemcpy((NvU8*)TempAHBAddress, hSnor->pVirtBuffer, CurrentTransferLength <<2);
    }
    SetPowerControl(hSnor, NV_FALSE);
    return Error;
}

NvError
NvDdkSnorWrite(
    NvDdkSnorHandle hSnor,
    NvU8 ChipNum,
    NvU32 StartByteOffset,
    NvU32 SizeInBytes,
    const NvU8* pBuffer,
    NvU32* SnorStatus)

{
    NvError Error = NvSuccess;
    NV_ASSERT(ChipNum < MAX_CHIPSELECT);

    // Check whether the requested devices are available or not.
    if(!hSnor->IsDevAvailable[ChipNum])
        return NvError_BadParameter;

    // Enable the clock
    Error =  SetPowerControl(hSnor, NV_TRUE);
    if (Error)
        return Error;
    SetChipSelect(hSnor,ChipNum);

    NvDdkPrivSnorSlaveMode(hSnor);
    // Get the device into read array mode
    hSnor->SnorDevInt.DevReadMode(&(hSnor->SnorDevReg),
                                  &(hSnor->SnorDevInfo),
                                  NV_TRUE);
    hSnor->SnorDevInt.DevProgram( &(hSnor->SnorDevReg),
                                  &(hSnor->SnorDevInfo),
                                  StartByteOffset,
                                  SizeInBytes,
                                  pBuffer,
                                  NV_TRUE);

    Error = SetPowerControl(hSnor, NV_FALSE);
    return Error;
}

NvError
NvDdkSnorGetDeviceInfo(
    NvDdkSnorHandle hSnor,
    NvU8 DeviceNumber,
    NvDdkNorDeviceInfo* pDeviceInfo)
{
    if (!hSnor->IsDevAvailable[DeviceNumber])
        return NvError_NotSupported;

    pDeviceInfo->VendorId = hSnor->SnorDevInfo.ManufId;
    pDeviceInfo->DeviceId = hSnor->SnorDevInfo.DeviceId;
    pDeviceInfo->BusWidth = 16;
    pDeviceInfo->PagesPerBlock = hSnor->SnorDevInfo.PagesPerBlock;
    pDeviceInfo->NoOfBlocks = hSnor->SnorDevInfo.TotalBlocks;
    pDeviceInfo->ZonesPerDevice = 1;
    return NvError_Success;
}

NvError
NvDdkSnorErase(
    NvDdkSnorHandle hSnor,
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
    SnorCommandStatus SnorStatus;
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
        if (!hSnor->IsDevAvailable[ChipSelectId[Index]])
            return NvError_BadParameter;
    }

    // Enable the clock
    Error =  SetPowerControl(hSnor, NV_TRUE);
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
                                hSnor->SnorDevInfo.PagesPerBlock);
        StartPageNumber[Index] += hSnor->SnorDevInfo.PagesPerBlock;
        SetChipSelect(hSnor, ChipSelectId[Index]);

        hSnor->SnorDevInt.DevEraseBlock(
                            &hSnor->SnorDevReg, &hSnor->SnorDevInfo,
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

            SetChipSelect(hSnor, ChipSelectId[Index]);

            // TODO: Int based ???
            do
            {
                SnorStatus = hSnor->SnorDevInt.DevGetEraseStatus(
                            &hSnor->SnorDevReg);
            } while (SnorStatus == SnorCommandStatus_Busy);

            if (SnorStatus != SnorCommandStatus_Success)
            {
                Error = NvError_NorEraseFailed;
                goto ErrorEnd;
            }
            NumberOfBlockErased++;
            BlocksRemaining--;

            // Issue the erase command if there is more blcok erase required
            // from this device.
            if (BlocksRequested >= (NumberOfBlockErased + MaxValidIndex))
            {

                CurrBlockNumber = BLOCK_NUMBER(StartPageNumber[Index],
                                        hSnor->SnorDevInfo.PagesPerBlock);

                StartPageNumber[Index] += hSnor->SnorDevInfo.PagesPerBlock;
                hSnor->SnorDevInt.DevEraseBlock(
                                    &hSnor->SnorDevReg, &hSnor->SnorDevInfo,
                                    CurrBlockNumber);
            }
        }
    }

ErrorEnd:
    SetPowerControl(hSnor, NV_FALSE);
    *pNumberOfBlocks = NumberOfBlockErased;
    return Error;
}

void
NvDdkSnorGetLockedRegions(
    NvDdkSnorHandle hSnor,
    LockParams* pFlashLockParams)
{

}

void
NvDdkSnorSetFlashLock(
    NvDdkSnorHandle hSnor,
    LockParams* pFlashLockParams)
{
    NvU32 StartPageNumber;
    NvU32 NumberOfPages;
    NvU32 PagesRemaining;
    NvU32 CurrBlockNumber;
    SnorCommandStatus SnorStatus;
    NvError Error;

    NV_ASSERT(pFlashLockParams);

    // Check whether the requested devices are available or not.
    if (!hSnor->IsDevAvailable[pFlashLockParams->DeviceNumber])
        return;

    StartPageNumber = pFlashLockParams->StartPageNumber;
    NumberOfPages = pFlashLockParams->EndPageNumber -
                            pFlashLockParams->StartPageNumber;
    NV_ASSERT(NumberOfPages);

    // Enable the clock
    Error =  SetPowerControl(hSnor, NV_TRUE);
    if (Error)
    {
        NV_ASSERT(Error == NvSuccess);
        return;
    }

    PagesRemaining = NumberOfPages;

    SetChipSelect(hSnor, pFlashLockParams->DeviceNumber);
    while (PagesRemaining)
    {
        CurrBlockNumber = BLOCK_NUMBER(StartPageNumber,
                                hSnor->SnorDevInfo.PagesPerBlock);
        StartPageNumber += hSnor->SnorDevInfo.PagesPerBlock;

        SnorStatus = hSnor->SnorDevInt.DevSetBlockLockStatus(
                            &hSnor->SnorDevReg, &hSnor->SnorDevInfo,
                            CurrBlockNumber, NV_TRUE);
        if (SnorStatus != SnorCommandStatus_Success)
            goto ErrorEnd;

        if (PagesRemaining >= hSnor->SnorDevInfo.PagesPerBlock)
            PagesRemaining -= hSnor->SnorDevInfo.PagesPerBlock;
        else
            PagesRemaining = 0;
    }

ErrorEnd:
    SetPowerControl(hSnor, NV_FALSE);
    return;
}

void NvDdkSnorReleaseFlashLock(NvDdkSnorHandle hSnor)
{
    SnorCommandStatus SnorStatus;
    NvU32 Index;
    NvError Error;

    NV_ASSERT(hSnor);

    // Enable the clock
    Error =  SetPowerControl(hSnor, NV_TRUE);
    if (Error)
    {
        NV_ASSERT(Error == NvSuccess);
        return;
    }

    // Release the lock of all the available devices.
    for (Index = 0; Index < MAX_CHIPSELECT; ++Index)
    {
        if (!hSnor->IsDevAvailable[Index])
            continue;

        SetChipSelect(hSnor, Index);

        SnorStatus = hSnor->SnorDevInt.DevUnlockAllBlockOfChip(
                            &hSnor->SnorDevReg, &hSnor->SnorDevInfo);
        if (SnorStatus != SnorCommandStatus_Success)
            goto ErrorEnd;
    }

ErrorEnd:
    SetPowerControl(hSnor, NV_FALSE);
    return;
}

NvError NvDdkSnorSuspend(NvDdkSnorHandle hSnor)
{
    return NvError_NotImplemented;
}

NvError NvDdkSnorResume(NvDdkSnorHandle hSnor)
{
    return NvError_NotImplemented;
}

