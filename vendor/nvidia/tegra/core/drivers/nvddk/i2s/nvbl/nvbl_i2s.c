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
 * @brief <b>NVIDIA Driver Development Kit: 
 *                  I2s Driver implementation for the bootloader</b>
 *
 * @b Description: Implementation of the NvBl I2S API.
 * 
 */

#include "nvbl_i2s.h"
#include "nvodm_query.h"
#include "../nvddk_i2s_ac97_hw_private.h"
#include "nvrm_power.h"
#include "nvrm_dma.h"
#include "nvrm_interrupt.h"
#include "nvassert.h"
#include "nvrm_hardware_access.h"
#include "nvodm_query_pinmux.h"
#include "nvrm_pinmux.h"
#include "nvodm_query.h"

// Constants used to size arrays.  Must be >= the max of all chips.
#define MAX_I2S_CONTROLLERS 4
#define MAX_AC97_CONTROLLERS 4

#define MAX_I2S_AC97_CONTROLLERS (MAX_I2S_CONTROLLERS + MAX_AC97_CONTROLLERS) 
#define INSTANCE_MASK 0xFFFF
#define AC97_START_INSTANCE_ID 0x80000000
#define I2S_BCLKDATABITS_LRCLK 32

typedef struct BlI2sInfoRec *BlI2sInfoHandle;

/**
 * Functions for the hw interfaces.
 */
typedef struct 
{
    /**
     * Initialize the i2s register.
     */
    void (*HwInitializeFxn)(NvU32 InstanceId, I2sAc97HwRegisters *pHwRegs);

    /**
     * Get the clock source frequency for desired sampling rate.
     */
    NvU32 (*GetClockSourceFreqFxn)(NvU32 SamplingRate, NvU32 DatabitsPerLRCLK);

    /**
     * Set the timing ragister based on the clock source frequency.
     */
    void 
    (*SetSamplingFreqFxn)(
        I2sAc97HwRegisters *pHwRegs, 
        NvU32 SamplingRate,
        NvU32 DatabitsPerLRCLK,
        NvU32 ClockSourceFreq);

    /**
     * Enable/Disable the data flow.
     */
    void 
    (*SetDataFlowFxn)(
        I2sAc97HwRegisters *pHwRegs, 
        I2sAc97Direction Direction, 
        NvBool IsEnable);

    /**
     * Reset the fifo.
     */
    void (*ResetFifoFxn)(I2sAc97HwRegisters *pHwRegs, I2sAc97Direction Direction);

    /**
     * Set the fifo format.
     */
    void (*SetFifoFormatFxn)(
        I2sAc97HwRegisters *pHwRegs,
        NvBlI2sDataWordValidBits DataFifoFormat, 
        NvU32 DataSize);

    /**
     * Set the I2s left right control polarity.
     */
    void (*SetInterfacePropertyFxn)(
        I2sAc97HwRegisters *pHwRegs,
        void *pInterface);


    /**
     * Get the interrupt source occured from i2s channels.
     */
    I2sHwInterruptSource (*GetInterruptSourceFxn)(I2sAc97HwRegisters *pHwRegs);

    /**
     * Ack the interrupt source occured from i2s channels.
     */
    void
    (*AckInterruptSourceFxn)(
        I2sAc97HwRegisters *pHwRegs, 
        I2sHwInterruptSource IntSource);

    /**
     * Set the the trigger level for receive/transmit fifo.
     */
    void
    (*SetTriggerLevelFxn)(
        I2sAc97HwRegisters *pHwRegs, 
        I2sAc97Direction FifoType, 
        NvU32 TriggerLevel);

    /**
     * Enable the loop back in the i2s channels.
     */
    void (*SetLoopbackTestFxn)(I2sAc97HwRegisters *pHwRegs, NvBool IsEnable);
} I2sAc97HwInterface;

/**
 * I2s handle which combines all the information related to the given i2s 
 * channels
 */
typedef struct NvBlI2sRec
{
    // Nv Rm device handles.
    NvRmDeviceHandle hDevice;    
    
    // Instance Id of the combined i2s/AC97 channels passed by client.
    NvU32 InstanceId;

    // Module Id.
    NvRmModuleID RmModuleId;

    // Dma module Id
    NvRmDmaModuleID BlDmaModuleId;

    // Channel reference count.
    NvU32 OpenCount;

    // I2s configuration parameter.
    NvOdmQueryI2sInterfaceProperty *pI2sInterfaceProps;

    // I2s data property.
    NvBlI2sDataProperty I2sDataProps;

    // I2s/Ac97 hw register information.
    I2sAc97HwRegisters I2sAc97HwRegs;

    // Transmit rm bootloader dma.
    NvRmDmaHandle hTxRmDma;
    
    // Pointer to the i2s information.
    BlI2sInfoHandle pI2sInfo;

    // Hw interface apis
    I2sAc97HwInterface *pHwInterface;

    NvU32 RmPowerClientId;

} NvBlI2s ;



/**
 * Combines the i2s/ac97 channel information.
 */
typedef struct BlI2sInfoRec
{
    // Reference count of how many times the I2S has been opened.
    NvU32 OpenCount;

    // Nv Rm device handles.
    NvRmDeviceHandle hDevice;    

    // Pointer to the head of the i2s channel handle.
    NvBlI2s I2sChannelList[MAX_I2S_AC97_CONTROLLERS];

    NvU32 TotalI2sChannel;
    
} BlI2sInfo;


static BlI2sInfo *s_pI2sInfo = NULL;
static I2sAc97HwInterface s_I2sHwInterface;

/***************** Static function prototype Ends here *********************/

/**
 * init the i2s interface.
 */
static void InitI2sInterface(void)
{
    s_I2sHwInterface.HwInitializeFxn = NvDdkPrivI2sHwInitialize;
    s_I2sHwInterface.GetClockSourceFreqFxn = NvDdkPrivI2sGetClockSourceFreq;
    s_I2sHwInterface.SetSamplingFreqFxn = NvDdkPrivI2sSetSamplingFreq;
    s_I2sHwInterface.SetDataFlowFxn = NvDdkPrivI2sSetDataFlow;
    s_I2sHwInterface.ResetFifoFxn = NvDdkPrivI2sResetFifo;
    s_I2sHwInterface.SetFifoFormatFxn = NvDdkPrivI2sSetFifoFormat;
    s_I2sHwInterface.SetInterfacePropertyFxn = NvDdkPrivI2sSetInterfaceProperty;
    s_I2sHwInterface.GetInterruptSourceFxn = NvDdkPrivI2sGetInterruptSource;
    s_I2sHwInterface.AckInterruptSourceFxn = NvDdkPrivI2sAckInterruptSource;
    s_I2sHwInterface.SetTriggerLevelFxn = NvDdkPrivI2sSetTriggerLevel;
    s_I2sHwInterface.SetLoopbackTestFxn = NvDdkPrivI2sSetLoopbackTest;
}

/**
 * Get the total number of i2s channels.
 */
static NvU32 GetTotalI2sChannel(NvRmDeviceHandle hDevice)
{
    return NvRmModuleGetNumInstances(hDevice, NvRmModuleID_I2s);
}

/**
 * Initialize the i2s data property.
 */
static void 
InitI2sDataProperty(
    NvU32 CommInstanceId,
    NvBlI2sDataProperty *pI2sDataProps)
{
    // Initialize the i2s configuration parameters.
    pI2sDataProps->IsMonoDataFormat = NV_FALSE;
    pI2sDataProps->I2sDataWordFormat = NvBlI2sDataWordValidBits_StartFromLsb;
    pI2sDataProps->ValidBitsInI2sDataWord = 16;
    pI2sDataProps->SamplingRate = 0;
}


/**
 * Destroy all the i2s information. It free all the allocated resource.
 * Thread safety: Caller responsibity.
 */
static void DeInitI2sInformation(void)
{
    BlI2sInfo* local = s_pI2sInfo;
    s_pI2sInfo = NULL;
    NvOsFree(local);
}

/**
 * Initialize the i2s information.
 * Thread safety: Caller responsibity.
 */
static NvBool InitI2sInformation(NvRmDeviceHandle hDevice)
{
    BlI2sInfo *pI2sInfo;
    
    NV_ASSERT(NvRmModuleGetNumInstances(hDevice, NvRmModuleID_I2s) <= MAX_I2S_CONTROLLERS);

    // Allocate the memory for the i2s information.
    pI2sInfo = NvOsAlloc(sizeof(*pI2sInfo));
    if (!pI2sInfo)
        return NV_FALSE;

    NvOsMemset(pI2sInfo, 0, sizeof(*pI2sInfo));

    // Initialize all the parameters.
    pI2sInfo->hDevice = hDevice;
    pI2sInfo->TotalI2sChannel = GetTotalI2sChannel(hDevice);
    InitI2sInterface();

    s_pI2sInfo = pI2sInfo;
    return NV_TRUE;
}

static void SetInterfaceProperty(NvBlI2sHandle hDdkI2s)
{
    hDdkI2s->pHwInterface->SetInterfacePropertyFxn(&hDdkI2s->I2sAc97HwRegs, 
                        (void *)hDdkI2s->pI2sInterfaceProps);
}

// Configure the pinmux for Cdev Sources
static NvBool CdevConfigurePinMux(NvBlI2sHandle hI2s)
{
    return NV_TRUE;
}

static void ConfigurePinMux(NvBlI2sHandle hI2s)
{
    const NvOdmQueryDapPortProperty *pDapProp = NULL;
    NvU32 DapIndex = 1;

    while(1) 
    {
        pDapProp = NvOdmQueryDapPortGetProperty(DapIndex);
        if (pDapProp == NULL)
            break;
        if (pDapProp->DapDestination == NvOdmDapPort_HifiCodecType)
        {
#ifndef SET_KERNEL_PINMUX
            NV_ASSERT_SUCCESS(NvRmSetOdmModuleTristate(hI2s->hDevice, 
                                NvOdmIoModule_Dap, DapIndex-1, NV_FALSE));
#endif
            return;                    
        }
        DapIndex++;
    } 
}

/**
 * Destroy the handle of i2s channel and free all the allocation done for it.
 * Thread safety: Caller responsibity.
 */
static void DestroyChannelHandle(NvBlI2sHandle hI2s)
{
    // If null pointer for i2s handle then return error.
    if (hI2s == NULL)
        return;

    // Unmap the virtual mapping of the i2s hw register.
    NvRmPhysicalMemUnmap(hI2s->I2sAc97HwRegs.pRegsVirtBaseAdd,
                         hI2s->I2sAc97HwRegs.BankSize);

    // Disable the clocks.
    NV_ASSERT_SUCCESS(NvRmPowerModuleClockControl(
                            hI2s->hDevice,
                            hI2s->RmModuleId,
                            hI2s->RmPowerClientId, 
                            NV_FALSE));

            
    // Reset the memory allocated for the i2s handle.
    NvOsMemset(hI2s ,0,sizeof(NvBlI2s ));

}
/**
 * Create the handle for the i2s channel.
 * Thread safety: Caller responsibity.
 */
static NvBool CreateChannelHandle(
    NvRmDeviceHandle hDevice,
    NvU32 InstanceId, 
    BlI2sInfo *pI2sInfo)
{
    NvError Error = NvSuccess;
    NvBlI2s *pI2sChannel = NULL;

    // Allcoate the memory for the i2s handle. 
    pI2sChannel = &pI2sInfo->I2sChannelList[InstanceId];

    // Reset the memory allocated for the i2s handle.
    NvOsMemset(pI2sChannel,0,sizeof(*pI2sChannel));

    // Set the i2s handle parameters.
    pI2sChannel->hDevice = hDevice;
    pI2sChannel->InstanceId = (InstanceId & INSTANCE_MASK);
    pI2sChannel->OpenCount= 0;


    pI2sChannel->pI2sInfo = pI2sInfo;
    pI2sChannel->pHwInterface = NULL;
    pI2sChannel->RmPowerClientId = 0;
    

    pI2sChannel->RmModuleId = NvRmModuleID_I2s;
    pI2sChannel->RmModuleId =  NVRM_MODULE_ID(pI2sChannel->RmModuleId, pI2sChannel->InstanceId),

    pI2sChannel->BlDmaModuleId = NvRmDmaModuleID_I2s;
    pI2sChannel->pHwInterface = &s_I2sHwInterface;
    pI2sChannel->pHwInterface->HwInitializeFxn(pI2sChannel->InstanceId, 
            &pI2sChannel->I2sAc97HwRegs);

    pI2sChannel->pI2sInterfaceProps = (NvOdmQueryI2sInterfaceProperty *)NvOdmQueryI2sGetInterfaceProperty(pI2sChannel->InstanceId);
    if(pI2sChannel->pI2sInterfaceProps)
        InitI2sDataProperty(InstanceId, &pI2sChannel->I2sDataProps);
    else
        Error = NvError_NotSupported;

    // Get the i2s hw physical base address and map in virtual memory space.
    if (!Error)
    {
        NvRmModuleGetBaseAddress(hDevice, pI2sChannel->RmModuleId,
            &pI2sChannel->I2sAc97HwRegs.RegsPhyBaseAdd,
            &pI2sChannel->I2sAc97HwRegs.BankSize);

        pI2sChannel->I2sAc97HwRegs.RxFifoAddress += 
                                pI2sChannel->I2sAc97HwRegs.RegsPhyBaseAdd;
        pI2sChannel->I2sAc97HwRegs.TxFifoAddress += 
                                pI2sChannel->I2sAc97HwRegs.RegsPhyBaseAdd;
        pI2sChannel->I2sAc97HwRegs.RxFifo2Address += 
                                pI2sChannel->I2sAc97HwRegs.RegsPhyBaseAdd;
        pI2sChannel->I2sAc97HwRegs.TxFifo2Address += 
                                pI2sChannel->I2sAc97HwRegs.RegsPhyBaseAdd;

        Error = NvRmPhysicalMemMap(
                    pI2sChannel->I2sAc97HwRegs.RegsPhyBaseAdd, 
                    pI2sChannel->I2sAc97HwRegs.BankSize, NVOS_MEM_READ_WRITE,
                    NvOsMemAttribute_Uncached,
                    (void **)&pI2sChannel->I2sAc97HwRegs.pRegsVirtBaseAdd);
    }

    // Enable the clock.
    if (!Error)
    {
        Error = NvRmPowerModuleClockControl(hDevice, pI2sChannel->RmModuleId, 
                    pI2sChannel->RmPowerClientId,  NV_TRUE);
    }

    // Reset the module.
    if (!Error)
    {
        NvRmModuleReset(hDevice, pI2sChannel->RmModuleId);
        SetInterfaceProperty(pI2sChannel);
        ConfigurePinMux(pI2sChannel);
    }   
    
    // If error then destroy all the allocation done here.
    if (Error)
    {
        DestroyChannelHandle(pI2sChannel);
        return NV_FALSE;
    }    
    return NV_TRUE;
}

/**
 * Open the i2s handle.
 */
NvBool 
NvBlI2sOpen(
    NvRmDeviceHandle hDevice,
    NvU32 InstanceId,
    NvBlI2sHandle *phI2s)
{
    BlI2sInfo *pI2sInfo = NULL;
    NvBlI2s *pI2sChannel = NULL;
    NvU32 TotalChannel =0 ;
    NvBool IsSuccess = NV_TRUE;

    NV_ASSERT(phI2s);
    NV_ASSERT(hDevice);

    *phI2s = NULL;

    pI2sInfo = s_pI2sInfo;
    if(!pI2sInfo)
    {
        IsSuccess = InitI2sInformation(hDevice);
        if(!IsSuccess)
            return IsSuccess;
        pI2sInfo = s_pI2sInfo;
    }

    TotalChannel = pI2sInfo->TotalI2sChannel;

    // Validate the channel Id parameter.
    if (InstanceId >= TotalChannel)
    {
        if(!pI2sInfo->OpenCount)
            DeInitI2sInformation();
        return NV_FALSE;
    }

    // Check for the open i2s handle to find out whether same instance port 
    // name exit or not.   
    // Get the head pointer of i2s channel
    pI2sChannel = &pI2sInfo->I2sChannelList[InstanceId];

    // If the i2s handle does not exist then cretae it.
    if (!pI2sChannel->OpenCount)
    {
        IsSuccess = CreateChannelHandle(hDevice, InstanceId, pI2sInfo);
        if(!IsSuccess)
        {
            if(!pI2sInfo->OpenCount)
                DeInitI2sInformation();

            return IsSuccess;
        }
        pI2sChannel->pI2sInfo = pI2sInfo;
    }

    // Incerease the open count.
    pI2sChannel->OpenCount++;
    *phI2s = pI2sChannel;

    if(!pI2sInfo->OpenCount)
        pI2sInfo->OpenCount++;
    else
        pI2sInfo->OpenCount++;

    return IsSuccess;
}

/**
 * Close the i2s handle.
 */
void NvBlI2sClose(NvBlI2sHandle hI2s)
{
    BlI2sInfo *pI2sInfo = NULL;
    // if null parametre then do nothing. 
    if (!hI2s)
        return;

    if (!hI2s->pI2sInfo)
        return;

    pI2sInfo = hI2s->pI2sInfo;
    
    // decremenr the open count and it becomes 0 then release all the allocation
    // done for this handle.
    hI2s->OpenCount--;

    // If the open count become zero then remove from the list of handles and 
    // free..
    if (!hI2s->OpenCount)
    {
        hI2s->pI2sInfo->I2sChannelList[hI2s->InstanceId].OpenCount = 0;
        if (hI2s->hTxRmDma)
            NvRmDmaFree(hI2s->hTxRmDma);

        // Now destroy the handles.
        DestroyChannelHandle(hI2s);
    }    

    pI2sInfo->OpenCount--;
    if(!pI2sInfo->OpenCount)
        DeInitI2sInformation();
        
}

NvBool 
NvBlI2sGetDataProperty(
    NvBlI2sHandle hI2s, 
    NvBlI2sDataProperty * const pI2SDataProperty)
{
    NV_ASSERT(hI2s);
    NV_ASSERT(pI2SDataProperty);

    // Copy the current configured parameter for the i2s communication.
    NvOsMemcpy(pI2SDataProperty, &hI2s->I2sDataProps, sizeof(*pI2SDataProperty));
    return NV_TRUE;
}

/**
 * Set the i2s configuration which is configured currently.
 */
NvBool 
NvBlI2sSetDataProperty(
    NvBlI2sHandle hI2s, 
    const NvBlI2sDataProperty *pI2SNewDataProp)
{
    NvError Error = NvSuccess;
    NvBlI2sDataProperty *pI2sCurrDataProps = NULL;
    NvU32 PrefClockSource;
    NvU32 ClockFreqReqd = 0;
    NvU32 ConfiguredClockFreq = 0;
    NvBool IsSuccess = NV_TRUE;

    NV_ASSERT(hI2s);
    NV_ASSERT(pI2SNewDataProp);

    // Get the current i2s data configuration pointers.
    pI2sCurrDataProps = &hI2s->I2sDataProps;

    // Enable the CDEV source_clk based on codec Mode
    if (hI2s->pI2sInterfaceProps->Mode == NvOdmQueryI2sMode_Master)
    {
        IsSuccess = CdevConfigurePinMux(hI2s);
        if (!IsSuccess)
            return IsSuccess;
    }

    if ((pI2SNewDataProp->IsMonoDataFormat != pI2sCurrDataProps->IsMonoDataFormat))
    {
        if (pI2SNewDataProp->IsMonoDataFormat)
        {
            return NV_FALSE;
        }    
    }

    // Set the data word format and valid bits in the data word.
    if ((pI2SNewDataProp->I2sDataWordFormat != pI2sCurrDataProps->I2sDataWordFormat) ||
         (pI2SNewDataProp->ValidBitsInI2sDataWord !=
                                        pI2sCurrDataProps->ValidBitsInI2sDataWord))
    {
        hI2s->pHwInterface->SetFifoFormatFxn(&hI2s->I2sAc97HwRegs, 
                                  pI2SNewDataProp->I2sDataWordFormat, 
                                  pI2SNewDataProp->ValidBitsInI2sDataWord);
        pI2sCurrDataProps->I2sDataWordFormat = 
                                    pI2SNewDataProp->I2sDataWordFormat;
        pI2sCurrDataProps->ValidBitsInI2sDataWord = 
                                    pI2SNewDataProp->ValidBitsInI2sDataWord;
    }


    if (pI2SNewDataProp->SamplingRate != pI2sCurrDataProps->SamplingRate)
    {
        ConfiguredClockFreq = 0;
        // Set the sampling rate only when it is master mode
        if (hI2s->pI2sInterfaceProps->Mode != NvOdmQueryI2sMode_Master)
            return NV_TRUE;

        ClockFreqReqd = hI2s->pHwInterface->GetClockSourceFreqFxn(
                                    pI2SNewDataProp->SamplingRate, I2S_BCLKDATABITS_LRCLK); 

        // Got the clock frequency for i2s source so configured it.                            
        PrefClockSource = ClockFreqReqd;
        Error = NvRmPowerModuleClockConfig(hI2s->hDevice, 
                                      hI2s->RmModuleId, 0, ClockFreqReqd,
                                      NvRmFreqUnspecified, &PrefClockSource,
                                      1, &ConfiguredClockFreq, 
                                      NvRmClockConfig_InternalClockForCore);


        // If no error then check for configured clock frequency and verify.
        if (!Error)
        {
            // Now set the timing register based on the new clock source
            // and sampling rate.
            hI2s->pHwInterface->SetSamplingFreqFxn(&hI2s->I2sAc97HwRegs,
                pI2SNewDataProp->SamplingRate, I2S_BCLKDATABITS_LRCLK, ConfiguredClockFreq);
            pI2sCurrDataProps->SamplingRate = pI2SNewDataProp->SamplingRate;
        }    
        if (Error)
            return NV_FALSE;
    }
    return IsSuccess;
}
    
/**
 * Transmit the data from i2s channel.
 * Thread safety: Provided in the function.
 */
NvBool 
NvBlI2sWrite(
    NvBlI2sHandle hI2s, 
    NvU32 *pWriteBuffer,
    NvU32 BytesRequested,
    NvU32 *pBytesWritten,
    NvBool IsAsynch)
{
    NvError Error = NvSuccess;
    NvRmDmaClientBuffer DmaClientBuff;
    
    NV_ASSERT(hI2s);
    NV_ASSERT(pBytesWritten);

    // Allocate the dma if it is not done yet.
    if (!hI2s->hTxRmDma)
    {
        Error = NvRmDmaAllocate(hI2s->hDevice, &hI2s->hTxRmDma,
                        NV_FALSE, NvRmDmaPriority_High, hI2s->BlDmaModuleId, hI2s->InstanceId);
        if (Error)
            return NV_FALSE;
        hI2s->pHwInterface->SetTriggerLevelFxn(&hI2s->I2sAc97HwRegs, 
                            I2sAc97Direction_Transmit, 16);
    }

    hI2s->pHwInterface->SetDataFlowFxn(&hI2s->I2sAc97HwRegs, I2sAc97Direction_Transmit, NV_TRUE);

    // Now do the dma transfer
    DmaClientBuff.SourceBufferPhyAddress = (NvU32)pWriteBuffer;
    DmaClientBuff.SourceAddressWrapSize = 0;
    DmaClientBuff.DestinationBufferPhyAddress = hI2s->I2sAc97HwRegs.TxFifoAddress;
    DmaClientBuff.DestinationAddressWrapSize = 4;
    DmaClientBuff.TransferSize = BytesRequested;

    Error = NvRmDmaStartDmaTransfer(hI2s->hTxRmDma, &DmaClientBuff, NvRmDmaDirection_Forward, 
                    0, NULL);

    if (Error)
    {
        hI2s->pHwInterface->SetDataFlowFxn(&hI2s->I2sAc97HwRegs, 
                                I2sAc97Direction_Transmit, NV_FALSE);
        return NV_FALSE;                                
    }

    if (!IsAsynch)
    {
        while (NvRmDmaIsDmaTransferCompletes(hI2s->hTxRmDma, NV_FALSE));

        hI2s->pHwInterface->SetDataFlowFxn(&hI2s->I2sAc97HwRegs, 
                                I2sAc97Direction_Transmit, NV_FALSE);
        NvRmDmaAbort(hI2s->hTxRmDma);
        hI2s->pHwInterface->SetDataFlowFxn(&hI2s->I2sAc97HwRegs, I2sAc97Direction_Transmit, NV_FALSE);
    }
                                
    return NV_TRUE;
}

/**
 * Stop the write opeartion.
 */
void NvBlI2sWriteAbort(NvBlI2sHandle hI2s) 
{
    // Required parameter validation
    if (hI2s)
    {
        NvRmDmaAbort(hI2s->hTxRmDma);
        hI2s->pHwInterface->SetDataFlowFxn(&hI2s->I2sAc97HwRegs, I2sAc97Direction_Transmit, NV_FALSE);
    }
}

/**
 * Stop the write opeartion.
 */
NvBool NvBlI2sIsHalfBufferTransferCompletes(NvBlI2sHandle hI2s, NvBool IsFirstHalfBuffer) 
{
    return NvRmDmaIsDmaTransferCompletes(hI2s->hTxRmDma, IsFirstHalfBuffer);
}


NvBool 
NvBlI2sGetInterfaceProperty(
    NvBlI2sHandle hI2s, 
    void *pI2SInterfaceProperty)
{
    NvOsMemcpy(pI2SInterfaceProperty, &hI2s->pI2sInterfaceProps, sizeof(NvOdmQueryI2sInterfaceProperty));
    return NV_TRUE;

}


// Configure the I2s Clock accordingly
static NvError ConfigureI2sClock(NvBlI2sHandle hI2s, NvU32 NewSampleRate)
{
    NvError Error = NvSuccess;
    NvU32 PrefClockSource;
    NvU32 ClockFreqReqd = 0;
    NvU32 ConfiguredClockFreq = 0;

    // Set the sampling rate only when it is master mode
    if (hI2s->pI2sInterfaceProps->Mode != NvOdmQueryI2sMode_Master)
    {
        // As POR value for i2s is Master. Calling the RmModuleClockConfig with 
        // External Clock and Preferred Clock as OScillator clock to set the 
        // i2s as Slave.
        ClockFreqReqd    = NvRmPowerGetPrimaryFrequency(hI2s->hDevice);
        PrefClockSource  = ClockFreqReqd;
        Error = NvRmPowerModuleClockConfig(hI2s->hDevice, 
                                  hI2s->RmModuleId, 0, ClockFreqReqd,
                                  NvRmFreqUnspecified, &PrefClockSource,
                                  1, &ConfiguredClockFreq, 
                                  NvRmClockConfig_ExternalClockForCore);
        if (!Error)
            hI2s->I2sDataProps.SamplingRate = NewSampleRate;
        return NvSuccess;
    }

    ClockFreqReqd = hI2s->pHwInterface->GetClockSourceFreqFxn(NewSampleRate, I2S_BCLKDATABITS_LRCLK); 

    // Got the clock frequency for i2s source so configured it.                            
    PrefClockSource = ClockFreqReqd;
    Error = NvRmPowerModuleClockConfig(hI2s->hDevice, 
                                  hI2s->RmModuleId, 0, ClockFreqReqd,
                                  NvRmFreqUnspecified, &PrefClockSource,
                                  1, &ConfiguredClockFreq, 
                                  NvRmClockConfig_InternalClockForCore);


    // If no error then check for configured clock frequency and verify.
    if (!Error)
    {
        // Now set the timing register based on the new clock source
        // and sampling rate.
        hI2s->pHwInterface->SetSamplingFreqFxn(&hI2s->I2sAc97HwRegs,
                                                NewSampleRate, I2S_BCLKDATABITS_LRCLK, ConfiguredClockFreq);
        hI2s->I2sDataProps.SamplingRate = NewSampleRate;
    }    
    return Error;
}

NvBool 
NvBlI2sSetInterfaceProperty(
    NvBlI2sHandle hI2s, 
    void *pI2SInterfaceProperty)
{
    NvError Error   = NvSuccess;
    NvBool IsUpdate = NV_FALSE;
    
    NvOdmQueryI2sInterfaceProperty CurIntProperty, *pNewIntProperty; 
    NvBlI2sGetInterfaceProperty(hI2s, &CurIntProperty);
    pNewIntProperty = (NvOdmQueryI2sInterfaceProperty*)pI2SInterfaceProperty;
    
    // check for mode and comm format
    if ((CurIntProperty.Mode != pNewIntProperty->Mode) ||
       (CurIntProperty.I2sDataCommunicationFormat != pNewIntProperty->I2sDataCommunicationFormat) ||
       (CurIntProperty.I2sLRLineControl != pNewIntProperty->I2sLRLineControl))    
    {
        IsUpdate = NV_TRUE;    
        NvOsMemcpy(&hI2s->pI2sInterfaceProps, pI2SInterfaceProperty, sizeof(NvOdmQueryI2sInterfaceProperty));
    }


    // Set only if the property are different from Current one
    if (IsUpdate)
    {
        hI2s->pHwInterface->SetInterfacePropertyFxn(&hI2s->I2sAc97HwRegs, 
                       pI2SInterfaceProperty);
        Error = ConfigureI2sClock(hI2s, hI2s->I2sDataProps.SamplingRate);
    }
    if (Error)
        return NV_FALSE;
    return NV_TRUE;
}

