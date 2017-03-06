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
 *                  Spdif Driver implementation</b>
 *
 * @b Description: Implementation of the NvDdk SPDIF API.
 *
 */

#include "nvddk_spdif.h"
#include "ddkspdif_hw_private.h"
#include "nvassert.h"
#include "nvrm_hardware_access.h"
#include "t30ddkspdif_hw_private.h"

#if !NV_IS_AVP
#include "nvrm_interrupt.h"
#include "nvrm_pinmux.h"
#include "nvodm_query_pinmux.h"
#include "nvodm_query.h"
#include "nvrm_power.h"

#endif  // !NV_IS_AVP

// ------------------ STRUCTURE DECLARATIONS -----------------------
typedef struct SocSpdifCapabilityRec
{
    // Check the Chip ID
    NvU32 ChipId;

    // Holds the public capabilities.
    NvDdkSpdifDriverCapability PublicCaps;
} SocSpdifCapability;

/**
 * Combines the spdif channel information.
 */
typedef struct DdkSpdifChannelInfoRec
{
    // Nv Rm device handles.
    NvRmDeviceHandle hDevice;

    // Pointer to the head of the spdif channel handle.
    struct NvDdkSpdifRec *pHeadSpdifChannel;

    // Mutex for spdif channel information.
    NvOsMutexHandle OsMutex4SpdifInfoAccess;

    // Refcnt
    NvU32 RefCnt;

    // Soc capability for Spdif.
    SocSpdifCapability SocSpdifCaps;

} DdkSpdifChannelInfo, *DdkSpdifChannelInfoHandle;

/**
 * Functions for the hw interfaces.
 */
typedef struct
{
    /**
     * Initialize the spdif register.
     */
    NvError
    (*HwInitializeFxn)(
            NvU32 SpdifChannelId,
            SpdifHwRegisters *pSpdifHwRegs);

    /**
    * Set the the trigger level for receive/transmit fifo.
    */
    NvError
    (*SetTriggerLevelFxn)(
        SpdifHwRegisters *pHwRegs,
        SpdifHwFifo FifoType,
        NvU32 TriggerLevel);

    // Following features are enabled only for cpu side.
#if !NV_IS_AVP

    /**
     * Reset the fifo.
     */
    NvError (*ResetFifoFxn)(
            SpdifHwRegisters *pHwRegs,
            SpdifHwFifo FifoType);

    /**
     * Set the interrupt source occured from Spdif channels.
     */
    NvError (*SetInterruptSourceFxn)(
        SpdifHwRegisters *pSpdifHwRegs,
        SpdifHwInterruptSource IntSource,
        NvBool IsEnable);

    /**
     * Get the interrupt source occured from Spdif channels.
     */
    NvError (*GetInterruptSourceFxn)(
            SpdifHwRegisters *pHwRegs,
            SpdifHwInterruptSource *pIntSource);

    /**
     * Ack the interrupt source occured from Spdif channels.
     */
    NvError
    (*AckInterruptSourceFxn)(
        SpdifHwRegisters *pHwRegs,
        SpdifHwInterruptSource IntSource);

    /*
     * Save the SPDIF registers before standby entry.
     */
    void
    (*GetStandbyRegFxn)(
        SpdifHwRegisters *pSpdifHwRegs,
        SpdifStandbyRegisters *pStandbyRegs);
    /*
    * Return the SPDIF registers saved on standby entry.
    */
    void
    (*SetStandbyRegFxn)(
        SpdifHwRegisters *pSpdifHwRegs,
        SpdifStandbyRegisters *pStandbyRegs);
/*
    //
    // Sets the apbif channel used with Spdif
    //
    void
    (*SetApbifChannelFxn)(
        SpdifHwRegisters *pHwRegs,
        void *pChannelInfo);

    //
    // Sets the i2s acif register
    //
    void
    (*SetAcifRegisterFxn)(
        SpdifHwRegisters *pHwRegs,
        NvBool IsReceive,
        NvBool IsRead,
        NvU32 *pCrtlValue);
*/
#endif

} SpdifHwInterface;

/**
 * Spdif handle which combines all the information related to the given spdif
 * channels
 */
typedef struct NvDdkSpdifRec
{
    // Nv Rm device handles.
    NvRmDeviceHandle hDevice;

    // Instance Id of the spdif channels.
    NvU32 InstanceId;

    // Module Id.
    NvRmModuleID RmModuleId;

    // Number of open count.
    NvU32 ReferenceCount;

    // Spdif hw register information.
    SpdifHwRegisters SpdifHwRegs;

    // Receive Synchronous sempahore Id which need to be signalled.
    NvOsSemaphoreHandle RxSynchSempahoreId;

    // Transmit Synchronous sempahore Id which need to be signalled.
    NvOsSemaphoreHandle TxSynchSempahoreId;

    // Write dma handle.
    NvRmDmaHandle hWriteDma;

    // Pointer to the spdif information.
    DdkSpdifChannelInfo *pSpdifChannelInfo;

    // Tha spdif capabiity for this spdif channel only.
    SocSpdifCapability SocSpdifCaps;

    // Hw interface apis
    SpdifHwInterface *pHwInterface;

#if !NV_IS_AVP

    // SPDIF configuration pin-map.
    NvOdmSpdifPinMap PinMap;

    // Spdif configuration parameter.
    NvOdmQuerySpdifInterfaceProperty * SpdifInterfaceProps;

    // Spdif data property.
    NvDdkSpdifDataProperty SpdifDataProps;

    // Read dma handle.
    NvRmDmaHandle hReadDma;

    // Read Status
    NvError ReadStatus;

    // Write Status
    NvError WriteStatus;

    // Each controller will have these two fields -
    // Power Manager registration API takes a
    // semaphore. This is used to wait for power Manager events
    NvOsSemaphoreHandle hPwrEventSem;

    // Id returned from driver's registration with Power Manager
    NvU32 RmPowerClientId;

    // Saved Spdif registers before entry into standby mode
    SpdifStandbyRegisters SpdifStandbyRegs;

    NvBool IsClockEnabled;

#endif // !NV_IS_AVP

    // Double link list parameter.
    struct NvDdkSpdifRec *pNextSpdif;
    struct NvDdkSpdifRec *pPrevSpdif;
} NvDdkSpdif;

// ------------------ GLOBAL DEFINITIONS -----------------------

// Spdif channel info.
static DdkSpdifChannelInfo *s_pSpdifChannelInfo = NULL;
static SpdifHwInterface s_SpdifHwInterface;

#define T20_CHIPID_NUM   0x20
#define T30_CHIPID_NUM   0x30

// ------------------ FUNCTION PROTOTYPES -----------------------
#if !NV_IS_AVP
/**
 * Get the spdif soc capability.
 *
 */
static NvError
SpdifGetSocCapabilities(
    NvRmDeviceHandle hDevice,
    NvU32 ChannelId,
    SocSpdifCapability *pSpdifSocCaps);

/**
 * Initialize the spdif data property.
 */
static NvError
InitSpdifDataProperty(
    NvU32 CommChannelId,
    NvDdkSpdifDataProperty *pSpdifDataProps);

static NvError SetInterfaceProperty(NvDdkSpdifHandle hSpdif);

#endif // !NV_IS_AVP

/**
 * Initialize the spdif information.
 * Thread safety: Caller responsibity.
 */
static NvError InitSpdifInformation(NvRmDeviceHandle hDevice);

/**
 * Destroy all the spdif information. It free all the allocated resource.
 * Thread safety: Caller responsibity.
 */
static void DeInitSpdifInformation(DdkSpdifChannelInfo *pSpdifChannelInfo);

/**
 * Create the handle for the spdif channel.
 * Thread safety: Caller responsibity.
 */
static NvError SpdifChannelHandleCreate(
    NvRmDeviceHandle hDevice,
    NvU32 ChannelId,
    NvDdkSpdifHandle *phSpdif);

/**
 * Destroy the handle of spdif channel and free all the allocation done for it.
 * Thread safety: Caller responsibity.
 */
static void SpdifChannelHandleDestroy(NvDdkSpdifHandle hSpdif);

// ------------------ FUNCTION DEFINITIONS -----------------------

/**
 * Get the spdif soc capability.
 *
 */
static NvError
SpdifGetSocCapabilities(
    NvRmDeviceHandle hDevice,
    NvU32 ChannelId,
    SocSpdifCapability *pSpdifSocCaps)
{
    static SocSpdifCapability SocSpdifCapsList[2];
    NvRmModuleCapability SpdifModuleCapability[] =
        {
            {1, 0, 0, NvRmModulePlatform_Silicon, &SocSpdifCapsList[0]},
            {2, 0, 0, NvRmModulePlatform_Silicon, &SocSpdifCapsList[1]},
        };
    SocSpdifCapability *pSpdifCaps = NULL;
    NvU32 i = 0;
    NvRmModuleID ModuleId;
    NvError  Error = NvSuccess;

    NV_ASSERT(pSpdifSocCaps);

    for (i=0; i < NV_ARRAY_SIZE(SpdifModuleCapability); i++)
    {
        SocSpdifCapsList[i].PublicCaps.TotalNumberOfSpdifChannel = 1;
        SocSpdifCapsList[i].PublicCaps.SpdifChannelId = ChannelId;
        SocSpdifCapsList[i].PublicCaps.IsValidBitsInPackedModeSupported
                                                                 = NV_TRUE;
        SocSpdifCapsList[i].PublicCaps.IsValidBitsStartsFromMsbSupported
                                                                 = NV_FALSE;
        SocSpdifCapsList[i].PublicCaps.IsValidBitsStartsFromLsbSupported
                                                                 = NV_TRUE;
        SocSpdifCapsList[i].PublicCaps.IsMonoDataFormatSupported = NV_FALSE;
        SocSpdifCapsList[i].PublicCaps.IsLoopbackSupported = NV_TRUE;
        SocSpdifCapsList[i].PublicCaps.IsChannelDataSupported = NV_TRUE;
        SocSpdifCapsList[i].PublicCaps.IsUserDataSupported = NV_TRUE;
        SocSpdifCapsList[i].PublicCaps.SpdifDataWordSizeInBits = 32;
    }

    // Ap20
    SocSpdifCapsList[0].ChipId = T20_CHIPID_NUM;
    // T30
    SocSpdifCapsList[1].ChipId = T30_CHIPID_NUM;

    // setting as 0 - as only one spdif device
    ModuleId = NVRM_MODULE_ID(NvRmModuleID_Spdif, 0);

    // Get the capabity from modules files.
    Error = NvRmModuleGetCapabilities(
                        hDevice, ModuleId,
                        &SpdifModuleCapability[0],
                        NV_ARRAY_SIZE(SpdifModuleCapability),
                        (void **)&pSpdifCaps);

    if ((Error != NvSuccess) || (pSpdifCaps == NULL))
    {
        NVDDK_SPDIF_POWERLOG(("SpdifGetSocCapabilities Failed \n"));
        NV_ASSERT_SUCCESS(Error);
        return Error;
    }

    NvOsMemcpy(pSpdifSocCaps, pSpdifCaps, sizeof(SocSpdifCapability));
    return Error;
}

/**
*   Initialize the spdif Interfaces
*/
static void InitSpdifInterface(NvU32 ChipId)
{
    if (ChipId == T30_CHIPID_NUM)
    {
        s_SpdifHwInterface.HwInitializeFxn = NvDdkPrivT30SpdifHwInitialize;
        s_SpdifHwInterface.SetTriggerLevelFxn =
                                        NvDdkPrivT30SpdifSetTriggerLevel;
#if !NV_IS_AVP
        s_SpdifHwInterface.ResetFifoFxn = NvDdkPrivT30SpdifResetFifo;
        s_SpdifHwInterface.SetInterruptSourceFxn =
                                        NvDdkPrivT30SpdifSetInteruptSource;
        s_SpdifHwInterface.GetInterruptSourceFxn =
                                        NvDdkPrivT30SpdifGetInteruptSource;
        s_SpdifHwInterface.AckInterruptSourceFxn =
                                        NvDdkPrivT30SpdifAckInteruptSource;
        s_SpdifHwInterface.GetStandbyRegFxn =
                                        NvDdkPrivT30SpdifGetStandbyRegisters;
        s_SpdifHwInterface.SetStandbyRegFxn =
                                        NvDdkPrivT30SpdifSetStandbyRegisters;
#endif
    }
    else
    {
        s_SpdifHwInterface.HwInitializeFxn = NvDdkPrivSpdifHwInitialize;
        s_SpdifHwInterface.SetTriggerLevelFxn =
                                        NvDdkPrivSpdifSetTriggerLevel;
#if !NV_IS_AVP
        s_SpdifHwInterface.ResetFifoFxn = NvDdkPrivSpdifResetFifo;
        s_SpdifHwInterface.SetInterruptSourceFxn =
                                        NvDdkPrivSpdifSetInteruptSource;
        s_SpdifHwInterface.GetInterruptSourceFxn =
                                        NvDdkPrivSpdifGetInteruptSource;
        s_SpdifHwInterface.AckInterruptSourceFxn =
                                        NvDdkPrivSpdifAckInteruptSource;
        s_SpdifHwInterface.GetStandbyRegFxn =
                                        NvDdkPrivSpdifGetStandbyRegisters;
        s_SpdifHwInterface.SetStandbyRegFxn =
                                        NvDdkPrivSpdifSetStandbyRegisters;
#endif
    }
}

/**
 * Initialize the spdif information.
 * Thread safety: Caller responsibity.
 */
static NvError InitSpdifInformation(NvRmDeviceHandle hDevice)
{
    NvError Error = NvSuccess;
    DdkSpdifChannelInfo *pSpdifChannelInfo = NULL;

    // If spdif information is created then return.
    if (s_pSpdifChannelInfo != NULL)
    {
        return NvSuccess;
    }

    // Allocate the memory for the spdif information.
    pSpdifChannelInfo = NvOsAlloc(sizeof(DdkSpdifChannelInfo));
    if (pSpdifChannelInfo == NULL)
    {
        return NvError_InsufficientMemory;
    }

    // Reset the memory allocated for the spdif
    NvOsMemset(pSpdifChannelInfo, 0, sizeof(DdkSpdifChannelInfo));

    // Initialize all the parameters.
    pSpdifChannelInfo->hDevice = hDevice;

    // Create the mutex to accss the spdif information.
    Error = NvOsMutexCreate(&pSpdifChannelInfo->OsMutex4SpdifInfoAccess);

    // Initialize the spdif soc capabilities.
    if (Error == NvSuccess)
    {
        Error = SpdifGetSocCapabilities(
                             hDevice, 0,
                             &pSpdifChannelInfo->SocSpdifCaps);
    }

    // If error exit till then destroy allallocations.
    if (Error != NvSuccess)
    {
        DeInitSpdifInformation(pSpdifChannelInfo);
        pSpdifChannelInfo = NULL;
    }

    // Set the interface call.
    InitSpdifInterface(pSpdifChannelInfo->SocSpdifCaps.ChipId);

    s_pSpdifChannelInfo = pSpdifChannelInfo;

    return Error;
}

/**
 * Destroy all the spdif information. It free all the allocated resource.
 * Thread safety: Caller responsibity.
 */
static void DeInitSpdifInformation(DdkSpdifChannelInfo *pSpdifChannelInfo)
{
    // If null parameter then return.
    if (pSpdifChannelInfo == NULL)
    {
        return;
    }

    if (pSpdifChannelInfo->RefCnt > 0)
    {
        pSpdifChannelInfo->RefCnt--;
    }

    if (!pSpdifChannelInfo->RefCnt)
    {
        // Free all allocations.
        if (pSpdifChannelInfo->OsMutex4SpdifInfoAccess)
            NvOsMutexDestroy(pSpdifChannelInfo->OsMutex4SpdifInfoAccess);

        NvOsFree(pSpdifChannelInfo);
        s_pSpdifChannelInfo = NULL;
    }
}

#if !NV_IS_AVP

/**
 * Initialize the spdif data property.
 */
static NvError
InitSpdifDataProperty(
    NvU32 CommChannelId,
    NvDdkSpdifDataProperty *pSpdifDataProps)
{
    NV_ASSERT(pSpdifDataProps);

    // Initialize the spdif configuration parameters.
    pSpdifDataProps->IsMonoDataFormat = NV_FALSE;
    pSpdifDataProps->SpdifDataWordFormat =
                        NvDdkSpdifDataWordValidBits_StartFromLsb;
    pSpdifDataProps->ValidBitsInSpdifDataWord = 0;
    pSpdifDataProps->SamplingRate = 11025;
    return NvSuccess;
}

static NvError SetInterfaceProperty(NvDdkSpdifHandle hSpdif)
{
    NvError Error = NvSuccess;

    NV_ASSERT(hSpdif);

    if (hSpdif->SpdifInterfaceProps)
    {
        Error = NvDdkPrivSpdifSetDataCaptureControl(&hSpdif->SpdifHwRegs,
                  hSpdif->SpdifInterfaceProps->SpdifDataCaptureControl);
    }
    else
    {
        return NvError_BadParameter;
    }

    return Error;
}


/**
 * Get the spdif capabilies at ddk level.
 */
NvError
NvDdkSpdifGetCapabilities(
    NvRmDeviceHandle hDevice,
    NvU32 ChannelId,
    NvDdkSpdifDriverCapability* const pSpdifDriverCaps)
{
    NvError Error = NvSuccess;
    SocSpdifCapability SocSpdifCaps;

    NV_ASSERT(pSpdifDriverCaps);

    // Get the soc spdif capabilities or return error if failed
    Error = SpdifGetSocCapabilities(hDevice, ChannelId, &SocSpdifCaps);
    if (Error)
        return Error;

    // validate the ChannelId.
    // It starts from 1. zero indicates that client has queried for
    // number of channels available.
    NV_ASSERT(ChannelId < SocSpdifCaps.PublicCaps.TotalNumberOfSpdifChannel);

    NvOsMemcpy(pSpdifDriverCaps,
               &SocSpdifCaps.PublicCaps,
               sizeof(NvDdkSpdifDriverCapability));
    return NvSuccess;
}

static NvError EnableSpdifPower(NvDdkSpdifHandle hSpdif)
{
    NvError Error = NvSuccess;

    if (hSpdif->IsClockEnabled)
        return Error;

    // request power
    Error = NvRmPowerVoltageControl(hSpdif->hDevice,
                                    hSpdif->RmModuleId,
                                    hSpdif->RmPowerClientId,
                                    NvRmVoltsUnspecified,
                                    NvRmVoltsUnspecified,
                                    NULL,
                                    0,
                                    NULL);

    // now enable clock to spdif controller
    if (!Error)
    {
        Error = NvRmPowerModuleClockControl(hSpdif->hDevice,
                                            hSpdif->RmModuleId,
                                            hSpdif->RmPowerClientId,
                                            NV_TRUE);
    }

    if (!Error)
    {
        // Reset the module.
        NvRmModuleReset(hSpdif->hDevice, hSpdif->RmModuleId);
        // Reset the FIFOs
        hSpdif->pHwInterface->ResetFifoFxn(&hSpdif->SpdifHwRegs,
                                            SpdifHwFifo_Both);

        hSpdif->IsClockEnabled   = NV_TRUE;
    }

    return Error;
}

static NvError DisableSpdifPower(NvDdkSpdifHandle hSpdif)
{
    NvError Error = NvSuccess;

    if (!hSpdif->IsClockEnabled)
        return Error;

    // Disable the clock
    Error = NvRmPowerModuleClockControl(hSpdif->hDevice, hSpdif->RmModuleId,
                                    hSpdif->RmPowerClientId, NV_FALSE);

    // Disable power
    if (!Error)
        Error = NvRmPowerVoltageControl(hSpdif->hDevice, hSpdif->RmModuleId,
                    hSpdif->RmPowerClientId, NvRmVoltsOff, NvRmVoltsOff,
                    NULL, 0, NULL);

    if (!Error)
        hSpdif->IsClockEnabled   = NV_FALSE;

    return Error;
}

#endif // !NV_IS_AVP

/**
 * Create the handle for the spdif channel.
 * Thread safety: Caller responsibity.
 */
static NvError SpdifChannelHandleCreate(
    NvRmDeviceHandle hDevice,
    NvU32 ChannelId,
    NvDdkSpdifHandle *phSpdif)
{
    NvError e = NvSuccess;
    NvDdkSpdifHandle hSpdif = NULL;

    NV_ASSERT(phSpdif);
    NV_ASSERT(hDevice);

    *phSpdif = NULL;

    // Allocate the memory for the spdif handle or return Error if failes
    hSpdif = NvOsAlloc(sizeof(NvDdkSpdif));
    if (hSpdif == NULL)
    {
        return NvError_InsufficientMemory;
    }

    // Reset the memory allocated for the spdif handle.
    NvOsMemset(hSpdif, 0, sizeof(NvDdkSpdif));

    // Set the spdif handle parameters.
    hSpdif->hDevice = hDevice;
    hSpdif->InstanceId = 0; // only one spdif instance
    hSpdif->RmModuleId =
            NVRM_MODULE_ID(NvRmModuleID_Spdif, 0);

    // Get the soc capabilities and if error return here now.
    NV_CHECK_ERROR_CLEANUP(
        SpdifGetSocCapabilities(hDevice, ChannelId, &hSpdif->SocSpdifCaps));

    hSpdif->pHwInterface = &s_SpdifHwInterface;

    // Create the synchrnous semaphores.
    NV_CHECK_ERROR_CLEANUP(
        NvOsSemaphoreCreate(&hSpdif->RxSynchSempahoreId, 0));

    NV_CHECK_ERROR_CLEANUP(
        NvOsSemaphoreCreate(&hSpdif->TxSynchSempahoreId, 0));

#if !NV_IS_AVP

    hSpdif->SpdifInterfaceProps =
        (NvOdmQuerySpdifInterfaceProperty *)
        NvOdmQuerySpdifGetInterfaceProperty(hSpdif->InstanceId);

#ifndef SET_KERNEL_PINMUX
    //  Error is intentionally ignored from this function, since SPDIF may
    //  be output by HDMI, rather than TOSLink
    (void) NvRmSetModuleTristate(hDevice,
                                 hSpdif->RmModuleId,
                                 NV_FALSE);
#endif

    NV_CHECK_ERROR_CLEANUP(
        InitSpdifDataProperty(ChannelId, &hSpdif->SpdifDataProps));

#endif // !NV_IS_AVP


    // Get the spdif hw physical base address and map in virtual memory space.
    NvRmModuleGetBaseAddress(hDevice, hSpdif->RmModuleId,
        &hSpdif->SpdifHwRegs.RegsPhyBaseAdd,
        &hSpdif->SpdifHwRegs.BankSize);

    NV_CHECK_ERROR_CLEANUP(
        NvRmPhysicalMemMap(
                hSpdif->SpdifHwRegs.RegsPhyBaseAdd,
                hSpdif->SpdifHwRegs.BankSize, NVOS_MEM_READ_WRITE,
                NvOsMemAttribute_Uncached,
                (void **)&hSpdif->SpdifHwRegs.pRegsVirtBaseAdd));

    NV_CHECK_ERROR_CLEANUP(
        hSpdif->pHwInterface->HwInitializeFxn(
                            ChannelId,
                            &hSpdif->SpdifHwRegs));

#if !NV_IS_AVP
    // One time registration with Power Manager
    NV_CHECK_ERROR_CLEANUP(NvOsSemaphoreCreate(&(hSpdif->hPwrEventSem), 0));

    hSpdif->RmPowerClientId = NVRM_POWER_CLIENT_TAG('S','P','D','F');
    NV_CHECK_ERROR_CLEANUP(NvRmPowerRegister(hDevice,
                                             hSpdif->hPwrEventSem,
                                             &(hSpdif->RmPowerClientId)));

    NV_CHECK_ERROR_CLEANUP(EnableSpdifPower(hSpdif));

    NV_CHECK_ERROR_CLEANUP(
        SetInterfaceProperty(hSpdif));

#endif // !NV_IS_AVP

    *phSpdif = hSpdif;
    return e;

fail:
    // If error then destroy all the allocation done here.
    SpdifChannelHandleDestroy(hSpdif);
    return e;
}

/**
 * Destroy the handle of spdif channel and free all the allocation done for it.
 * Thread safety: Caller responsibity.
 */
static void SpdifChannelHandleDestroy(NvDdkSpdifHandle hSpdif)
{
    // If null pointer for spdif handle then return error.
    if (hSpdif == NULL)
    {
        return;
    }

    // Unmap the virtual mapping of the spdif hw register.
    NvRmPhysicalMemUnmap(hSpdif->SpdifHwRegs.pRegsVirtBaseAdd,
                         hSpdif->SpdifHwRegs.BankSize);

#if !NV_IS_AVP

    DisableSpdifPower(hSpdif);

    // Unregister driver from Power Manager
    NvRmPowerUnRegister(hSpdif->hDevice, hSpdif->RmPowerClientId);
    // Destroy semaphore created
    NvOsSemaphoreDestroy(hSpdif->hPwrEventSem);

#ifndef SET_KERNEL_PINMUX
    // Tri-State the pin-mux pins
    (void)NvRmSetModuleTristate(hSpdif->hDevice,
                                hSpdif->RmModuleId,
                                NV_TRUE);
#endif

#endif // !NV_IS_AVP

    // Destroy the sync sempahores.
    NvOsSemaphoreDestroy(hSpdif->RxSynchSempahoreId);
    NvOsSemaphoreDestroy(hSpdif->TxSynchSempahoreId);

    // Free the memory of the spdif handles.
    NvOsFree(hSpdif);
}

/**
 * Open the spdif handle.
 */
NvError
NvDdkSpdifOpen(
    NvRmDeviceHandle hDevice,
    NvU32 SpdifChannelId,
    NvDdkSpdifHandle *phSpdif)
{
    NvError Error = NvSuccess;
    DdkSpdifChannelInfo *pSpdifChannelInfo = NULL;
    NvDdkSpdifHandle hSpdif = NULL;

    NV_ASSERT(phSpdif);
    NV_ASSERT(hDevice);

    NVDDK_SPDIF_POWERLOG(("NvDdkSpdifOpen ++ \n"));

    // If global spdif info is null then create the spdif information.
    if (s_pSpdifChannelInfo == NULL)
    {
        Error = InitSpdifInformation(hDevice);
        if (Error)
            return Error;
    }

    pSpdifChannelInfo = s_pSpdifChannelInfo;

    // If spdif info is null then return error.
    if (pSpdifChannelInfo == NULL)
    {
        return NvError_NotInitialized;
    }

    // Increase the refcnt
    pSpdifChannelInfo->RefCnt++;

    NV_ASSERT(SpdifChannelId <=
        pSpdifChannelInfo->SocSpdifCaps.PublicCaps.TotalNumberOfSpdifChannel);

    // Lock the spdif info mutex access.
    NvOsMutexLock(pSpdifChannelInfo->OsMutex4SpdifInfoAccess);

    // Check for the open spdif handle to find out whether same instance port
    // name exit or not.
    // Get the head pointer of spdif channel
    hSpdif = pSpdifChannelInfo->pHeadSpdifChannel;
    while (hSpdif != NULL)
    {
        // if instance id match then found the handle of spdif channel.
        if (hSpdif->InstanceId == SpdifChannelId)
        {
            break;
        }

        // Advance the handle pointer ot next.
        hSpdif = hSpdif->pNextSpdif;
    }

    // If the spdif handle does not exist then create it.
    if (hSpdif == NULL)
    {
        Error = SpdifChannelHandleCreate(hDevice, SpdifChannelId, &hSpdif);

        // If no error the add in the head of spdif handle list.
        if (Error != NvSuccess)
        {
            goto fail;
        }
        // make the allocated handle point to head.
        hSpdif->pNextSpdif = pSpdifChannelInfo->pHeadSpdifChannel;
        // make the heads prev as allocated.
        if (pSpdifChannelInfo->pHeadSpdifChannel != NULL)
            pSpdifChannelInfo->pHeadSpdifChannel->pPrevSpdif = hSpdif;
        // make the allocated as head.
        pSpdifChannelInfo->pHeadSpdifChannel = hSpdif;

        hSpdif->pSpdifChannelInfo = pSpdifChannelInfo;
    }

    // Increase the open count if there is no error.
    hSpdif->ReferenceCount++;

    // If no error then update the passed pointers.
    *phSpdif = hSpdif;

    // Unlock the spdif info access.
    NvOsMutexUnlock(pSpdifChannelInfo->OsMutex4SpdifInfoAccess);
    NVDDK_SPDIF_POWERLOG(("NvDdkSpdifOpen -- \n"));
    return Error;

fail:
    // Unlock the spdif info access.
    NvOsMutexUnlock(pSpdifChannelInfo->OsMutex4SpdifInfoAccess);
    SpdifChannelHandleDestroy(hSpdif);
    DeInitSpdifInformation(pSpdifChannelInfo);
    NVDDK_SPDIF_POWERLOG(("NvDdkSpdifOpen Failed -- \n"));
    return Error;
}

/**
 * Close the spdif handle.
 */
void NvDdkSpdifClose(NvDdkSpdifHandle hSpdif)
{
    DdkSpdifChannelInfo *pSpdifInformation = NULL;
    NvDdkSpdif *pPrevSpdif = NULL;
    NvDdkSpdif *pNextSpdif = NULL;

    NVDDK_SPDIF_POWERLOG(("NvDdkSpdifClose++ \n"));
    // if null parametre then do nothing.
    if (hSpdif == NULL)
    {
        return;
    }

    // Get the spdif information pointer.
    pSpdifInformation = hSpdif->pSpdifChannelInfo;

    // Lock the spdif information access.
    NvOsMutexLock(pSpdifInformation->OsMutex4SpdifInfoAccess);

    // decremenr the open count and it becomes 0 then release all the
    // allocation done for this handle.
    hSpdif->ReferenceCount--;

    // If the open count become zero then remove from the list of handles and
    // free..
    if (hSpdif->ReferenceCount == 0)
    {
        // Get the prev and next pointers.
        pPrevSpdif = hSpdif->pPrevSpdif;
        pNextSpdif = hSpdif->pNextSpdif;

        // If next is not the null then link the next pointer to prev pointer.
        if (pNextSpdif != NULL)
        {
            pNextSpdif->pPrevSpdif = pPrevSpdif;
        }

        // If prev pointer is null then this is the head pointer so move the
        // head pointer otherwise link prev pointer to the next pointer.
        if (pPrevSpdif == NULL)
        {
            pSpdifInformation->pHeadSpdifChannel = pNextSpdif;
        }
        else
        {
            pPrevSpdif->pNextSpdif = pNextSpdif;
        }

        // Disable transmit and receive.
        NvDdkPrivSpdifSetTransmit(&hSpdif->SpdifHwRegs, NV_FALSE);
        NvDdkPrivSpdifSetReceive(&hSpdif->SpdifHwRegs, NV_FALSE);

        // Now destroy the handles.
        SpdifChannelHandleDestroy(hSpdif);
    }

    // Unlock the spdif information access.
    NvOsMutexUnlock(pSpdifInformation->OsMutex4SpdifInfoAccess);

    DeInitSpdifInformation(pSpdifInformation);
    NVDDK_SPDIF_POWERLOG(("NvDdkSpdifClose-- \n"));
}

/**
 * Transmit the data from spdif channel.
 * Thread safety: Provided in the function.
 */
NvError
NvDdkSpdifWrite(
    NvDdkSpdifHandle hSpdif,
    NvRmPhysAddr TransmitBufferPhyAddress,
    NvU32 *pBytesWritten,
    NvU32 WaitTimeoutInMilliSeconds,
    NvOsSemaphoreHandle AsynchSemaphoreId)
{
    NvError Error = NvSuccess;
    NvRmDmaClientBuffer DmaClientBuff;

    NV_ASSERT(hSpdif);
    NV_ASSERT(pBytesWritten);

    // Allocate the dma if it is not done yet.
    if (hSpdif->hWriteDma == NULL)
    {
        Error = NvRmDmaAllocate(hSpdif->hDevice, &hSpdif->hWriteDma,
                        NV_FALSE, NvRmDmaPriority_High,
                        hSpdif->SpdifHwRegs.RmDmaModuleId,
                        hSpdif->SpdifHwRegs.ChannelId);
        if (Error)
        {
            return Error;
        }
        hSpdif->pHwInterface->SetTriggerLevelFxn(&hSpdif->SpdifHwRegs,
                                                 SpdifHwFifo_Transmit,
                                                 16);
    }

    NvDdkPrivSpdifSetTransmit(&hSpdif->SpdifHwRegs, NV_TRUE);

    // Now do the dma transfer
    DmaClientBuff.SourceBufferPhyAddress = TransmitBufferPhyAddress;
    DmaClientBuff.SourceAddressWrapSize  = 0;
    DmaClientBuff.DestinationBufferPhyAddress =
                                    hSpdif->SpdifHwRegs.TxFifoAddress;
    DmaClientBuff.DestinationAddressWrapSize = 4;
    DmaClientBuff.TransferSize = *pBytesWritten;

    if (WaitTimeoutInMilliSeconds == 0)
    {
        Error = NvRmDmaStartDmaTransfer(hSpdif->hWriteDma, &DmaClientBuff,
                    NvRmDmaDirection_Forward, 0, AsynchSemaphoreId);
        return Error;
    }

    Error = NvRmDmaStartDmaTransfer(hSpdif->hWriteDma, &DmaClientBuff,
                NvRmDmaDirection_Forward, 0, hSpdif->TxSynchSempahoreId);

    if (!Error)
    {
        Error = NvOsSemaphoreWaitTimeout(hSpdif->TxSynchSempahoreId,
                                                WaitTimeoutInMilliSeconds);
        if (Error == NvError_Timeout)
        {
            NvRmDmaAbort(hSpdif->hWriteDma);
        }
    }

    NvDdkPrivSpdifSetTransmit(&hSpdif->SpdifHwRegs, NV_FALSE);
    return Error;
}

/**
 * Stop the write opeartion.
 */
void NvDdkSpdifWriteAbort(NvDdkSpdifHandle hSpdif)
{
    NV_ASSERT(hSpdif);

    if (hSpdif->hWriteDma != NULL)
    {
        NvRmDmaAbort(hSpdif->hWriteDma);
    }
    NvDdkPrivSpdifSetTransmit(&hSpdif->SpdifHwRegs, NV_FALSE);
}

#if !NV_IS_AVP

NvError
NvDdkSpdifGetDataProperty(
    NvDdkSpdifHandle hSpdif,
    NvDdkSpdifDataProperty * const pSpdifDataProperty)
{
    NV_ASSERT(hSpdif);
    NV_ASSERT(pSpdifDataProperty);

    // Copy the current configured parameter for the spdif communication.
    NvOsMemcpy(pSpdifDataProperty,
               &hSpdif->SpdifDataProps,
               sizeof(*pSpdifDataProperty));
    return NvSuccess;
}

/**
 * Set the spdif configuration which is configured currently.
 */
NvError
NvDdkSpdifSetDataProperty(
    NvDdkSpdifHandle hSpdif,
    const NvDdkSpdifDataProperty *pSpdifNewDataProp)
{
    NvError Error = NvSuccess;
    NvDdkSpdifDataProperty *pSpdifCurrDataProps = NULL;
    NvU32 ClockFreqReqd = 0;
    NvU32 ConfiguredClockFreq = 0;
    NvU32 PrefClockSource[4];
    NvRmModuleID ModuleId = NvRmModuleID_Invalid;

    NV_ASSERT(hSpdif);
    NV_ASSERT(pSpdifNewDataProp);

    NVDDK_SPDIF_POWERLOG(("NvDdkSpdifSetDataProperty++ \n"));

    // Get the current spdif data configuration pointers.
    pSpdifCurrDataProps = &hSpdif->SpdifDataProps;

    if (pSpdifNewDataProp->IsMonoDataFormat)
    {
        return NvError_NotSupported;
    }

    Error = NvDdkPrivSpdifSetDataBitSize(&hSpdif->SpdifHwRegs,
                                pSpdifNewDataProp->ValidBitsInSpdifDataWord);
    if (Error == NvSuccess)
    {
        pSpdifCurrDataProps->SpdifDataWordFormat =
                                 pSpdifNewDataProp->SpdifDataWordFormat;
        pSpdifCurrDataProps->ValidBitsInSpdifDataWord =
                                 pSpdifNewDataProp->ValidBitsInSpdifDataWord;

        // pack bit is invalid for t30
        if (hSpdif->SocSpdifCaps.ChipId != T30_CHIPID_NUM)
        {
            NvDdkPrivSpdifSetPackedDataMode(&hSpdif->SpdifHwRegs,
                                pSpdifNewDataProp->SpdifDataWordFormat,
                                pSpdifNewDataProp->ValidBitsInSpdifDataWord);
        }
    }

    if (Error)
    {
        NVDDK_SPDIF_POWERLOG(("NvDdkSpdifSetDataProperty failed 0x%x\n", Error));
        return Error;
    }

    // Set the sampling rate only when it is master mode
    Error = NvDdkPrivSpdifSetSamplingFreq(&hSpdif->SpdifHwRegs,
                        pSpdifNewDataProp->SamplingRate, &ClockFreqReqd);

    // Got the clock frequency for spdif source so configure it.
    if (Error == NvSuccess)
    {
        PrefClockSource[0] = ClockFreqReqd;
        PrefClockSource[1] = ClockFreqReqd;
        PrefClockSource[2] = ClockFreqReqd;
        PrefClockSource[3] = ClockFreqReqd;
        ModuleId = NVRM_MODULE_ID(NvRmModuleID_Spdif, hSpdif->InstanceId);
        Error = NvRmPowerModuleClockConfig(hSpdif->hDevice,
                                      ModuleId,
                                      hSpdif->RmPowerClientId,
                                      NvRmFreqUnspecified,
                                      NvRmFreqUnspecified,
                                      PrefClockSource,
                                      1,
                                      &ConfiguredClockFreq,
                                      NvRmClockConfig_AudioAdjust);
        // If not error then check for configured clock frequency and
        // verify.
        if (Error == NvSuccess)
        {
            // If not what we requested then error.
            if (ClockFreqReqd != ConfiguredClockFreq)
            {
                Error = NvError_NotSupported;
                NVDDK_SPDIF_POWERLOG(("NvDdkSpdifSetDataProperty ClockFreqReqd not same\n"));
                // Verify CurrentFreq is in init freq.
                if (pSpdifCurrDataProps->SamplingRate == 11025)
                {
                    // set the ClockFreq as OscFrequency
                    // return  success with fixed clock
                    ClockFreqReqd =
                            NvRmPowerGetPrimaryFrequency(hSpdif->hDevice);
                    return NvSuccess;
                }
                else
                {
                    // revert back the sampling freq.
                    Error = NvDdkPrivSpdifSetSamplingFreq(&hSpdif->SpdifHwRegs,
                            pSpdifCurrDataProps->SamplingRate, &ClockFreqReqd);
                }

                if (Error == NvSuccess)
                {
                    Error = NvError_NotSupported;
                }
                else
                {
                    return Error;
                }
            }
            else
            {
                pSpdifCurrDataProps->SamplingRate =
                                     pSpdifNewDataProp->SamplingRate;
            }
            hSpdif->pHwInterface->ResetFifoFxn(&hSpdif->SpdifHwRegs,
                                                SpdifHwFifo_Both);
        }
    }
    NVDDK_SPDIF_POWERLOG(("NvDdkSpdifSetDataProperty--\n"));
    return Error;
}

/**
 * Read the data from com channel.
 * Thread safety: Provided in the function.
 */
NvError
NvDdkSpdifRead(
    NvDdkSpdifHandle hSpdif,
    NvRmPhysAddr ReceiveBufferPhyAddress,
    NvU32 *pBytesRead,
    NvU32 WaitTimeoutInMilliSeconds,
    NvOsSemaphoreHandle AsynchSemaphoreId)
{
    NvError Error = NvSuccess;
    NvRmDmaClientBuffer DmaClientBuff;

    NV_ASSERT(hSpdif);
    NV_ASSERT(pBytesRead != 0);

    // Allocate the dma if it is not done yet.
    if (hSpdif->hReadDma == NULL)
    {
        Error = NvRmDmaAllocate(hSpdif->hDevice, &hSpdif->hReadDma,
                        NV_FALSE, NvRmDmaPriority_High,
                        hSpdif->SpdifHwRegs.RmDmaModuleId,
                        hSpdif->SpdifHwRegs.ChannelId);
        if (Error)
        {
            return Error;
        }
        hSpdif->pHwInterface->SetTriggerLevelFxn(&hSpdif->SpdifHwRegs,
                                              SpdifHwFifo_Receive,
                                              16);
    }

    NvDdkPrivSpdifSetReceive(&hSpdif->SpdifHwRegs, NV_TRUE);

    // Now do the dma transfer
    DmaClientBuff.SourceBufferPhyAddress = ReceiveBufferPhyAddress;
    DmaClientBuff.SourceAddressWrapSize = 0;
    DmaClientBuff.DestinationBufferPhyAddress =
                                hSpdif->SpdifHwRegs.RxFifoAddress;
    DmaClientBuff.DestinationAddressWrapSize = 4;
    DmaClientBuff.TransferSize = *pBytesRead;

    if (WaitTimeoutInMilliSeconds == 0)
    {
        Error = NvRmDmaStartDmaTransfer(hSpdif->hReadDma, &DmaClientBuff,
                    NvRmDmaDirection_Reverse, 0, AsynchSemaphoreId);
        return Error;
    }

    Error = NvRmDmaStartDmaTransfer(hSpdif->hReadDma, &DmaClientBuff,
                NvRmDmaDirection_Reverse, 0, hSpdif->RxSynchSempahoreId);

    if (Error == NvSuccess)
    {
        Error = NvOsSemaphoreWaitTimeout(hSpdif->RxSynchSempahoreId,
                                                WaitTimeoutInMilliSeconds);
        if (Error == NvError_Timeout)
        {
            NvRmDmaAbort(hSpdif->hReadDma);
        }
    }

    NvDdkPrivSpdifSetReceive(&hSpdif->SpdifHwRegs, NV_FALSE);
    return Error;
}

/**
 * Stop the write opeartion.
 */
void NvDdkSpdifReadAbort(NvDdkSpdifHandle hSpdif)
{
    // Required parameter validation
    if (hSpdif == NULL)
    {
        return;
    }

    if (hSpdif->hReadDma != NULL)
    {
        NvRmDmaAbort(hSpdif->hReadDma);
    }
    NvDdkPrivSpdifSetReceive(&hSpdif->SpdifHwRegs, NV_FALSE);
}

/**
 * Get the asynchrnous read transfer information for last transaction.
 */
NvError
NvDdkSpdifGetAsynchReadTransferInfo(
    NvDdkSpdifHandle hSpdif,
    NvDdkSpdifClientBuffer *pSPDIFReceivedBufferInfo)
{
    NV_ASSERT(hSpdif);
    NV_ASSERT(pSPDIFReceivedBufferInfo);

    pSPDIFReceivedBufferInfo->TransferStatus = hSpdif->ReadStatus;
    return NvSuccess;
}

/**
 * Get the asynchrnous write transfer information.
 */
NvError
NvDdkSpdifGetAsynchWriteTransferInfo(
    NvDdkSpdifHandle hSpdif,
    NvDdkSpdifClientBuffer *pSPDIFSentBufferInfo)

{
    NV_ASSERT(hSpdif);
    NV_ASSERT(pSPDIFSentBufferInfo);

    pSPDIFSentBufferInfo->TransferStatus = hSpdif->WriteStatus;
    return NvSuccess;
}

NvError NvDdkSpdifWritePause(NvDdkSpdifHandle hSpdif)
{
    SpdifHwRegisters *pHwRegister = NULL;
    NvError Error = NvSuccess;

    NV_ASSERT(hSpdif);
    pHwRegister = &hSpdif->SpdifHwRegs;
    Error = NvDdkPrivSpdifSetTransmit(pHwRegister, NV_FALSE);
    return Error;
}

NvError NvDdkSpdifWriteResume(NvDdkSpdifHandle hSpdif)
{
    SpdifHwRegisters *pHwRegister = NULL;
    NvError Error = NvSuccess;

    NV_ASSERT(hSpdif);

    pHwRegister = &hSpdif->SpdifHwRegs;
    Error = NvDdkPrivSpdifSetTransmit(pHwRegister, NV_TRUE);

    return Error;
}

NvError NvDdkSpdifSetLoopbackMode(NvDdkSpdifHandle hSpdif, NvBool IsEnable)
{
    NvError Error = NvSuccess;

    NV_ASSERT(hSpdif);
    if (hSpdif->SocSpdifCaps.PublicCaps.IsLoopbackSupported == NV_TRUE)
    {
        Error = NvDdkPrivSpdifSetLoopbackTest(&hSpdif->SpdifHwRegs, IsEnable);
    }
    else
    {
        Error = NvError_NotSupported;
    }

    return Error;
}

NvError
NvDdkSpdifSetContinuousDoubleBufferingMode(
    NvDdkSpdifHandle hSpdif,
    NvBool IsEnable,
    NvDdkSpdifClientBuffer* pTransmitBuffer,
    NvOsSemaphoreHandle AsynchSemaphoreId1,
    NvOsSemaphoreHandle AsynchSemaphoreId2)
{
    return NvError_NotImplemented;
}

NvError NvDdkSpdifSuspend(NvDdkSpdifHandle hSpdif)
{
    NvError Error = NvSuccess;
    NV_ASSERT(hSpdif);

    // Spdif registers saved as in LP0 state register contents
    // are not intact.
    hSpdif->pHwInterface->GetStandbyRegFxn(&(hSpdif->SpdifHwRegs),
                                      &(hSpdif->SpdifStandbyRegs));

    Error = DisableSpdifPower(hSpdif);

    return Error;
}

NvError NvDdkSpdifResume(NvDdkSpdifHandle hSpdif)
{
    NvError Error = NvSuccess;
    NV_ASSERT(hSpdif);

    Error = EnableSpdifPower(hSpdif);

    // Spdif registers restored as in LP0 state register contents
    // are not intact.
    if(!Error)
    {
        hSpdif->pHwInterface->SetStandbyRegFxn(&(hSpdif->SpdifHwRegs),
                                          &(hSpdif->SpdifStandbyRegs));
    }

    return Error;
}

#endif // !NV_IS_AVP
