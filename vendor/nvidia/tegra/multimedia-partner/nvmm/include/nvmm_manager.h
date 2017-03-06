/*
 * Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_nvmm_manager_H
#define INCLUDED_nvmm_manager_H


#if defined(__cplusplus)
extern "C"
{
#endif

#include "nvrm_memmgr.h"
#include "nvrm_init.h"

typedef struct NvmmManagerRec *NvmmManagerHandle;

typedef struct NvmmMgrBlockRec *NvmmMgrBlockHandle;

typedef struct NvmmPowerClientRec *NvmmPowerClientHandle;

/*
 * @ingroup nvmm_manager
 * @{
 */

/**
 * @brief Initializes and opens the NvMM Manager. This function allocates the
 * handle for nvmm manager and provides it to the client.
 *
 * @retval NvSuccess Indicates that the NvmmManager has successfully opened.
 * @retval NvError_InsufficientMemory Indicates that function fails to allocate
 * the memory.
 * @retval NvError_NotInitialized Indicates the NvmmManager initialization failed.
 */
NvError NvmmManagerOpen(NvmmManagerHandle* pHandle);

/**
 * @brief Closes the NvmmManager. This function de-initializes the NvmmManager.
 * This API never fails.
 *
 * @param None
 */
void NvmmManagerClose(NvmmManagerHandle Handle, NvBool bForceClose);

typedef struct NvBlockProfileRec
{
    NvU32 StructSize;

    /* Unique id to identify the BP*/
    NvU32 BlockType;

    /* Category*/
    NvU32 BlockCategory;

    /* Locale of the block*/
    NvU32 Locale;

    /* Bitmask of NvBlockFlag values*/
    NvU32 BlockFlags;

    /* Size of stack for blockside thread*/
    NvU32 StackSize;

    /* Address of Stack*/
    NvU32 StackPtr;

    /* RM mem handle */
    NvRmMemHandle hMemHandle;

    /* Number of full output buffers,that designate the thresholdat which the */
    /* element is starving. Value of zero implies no starvation threshold. */
    NvU32 StarvationThreshold;

    /*Number of instances of the block*/
    NvU32 NumInstance;
    void* pBlockHandle;
} NvBlockProfile;

/**
 * @brief Use Case Type
 */
typedef enum
{
    NvmmUCType_Default = 0x0,
    NvmmUCType_ULP_Audio = 0x1,
    NvmmUCType_ULP_AV,
    NvmmUCType_MultiVideo,

    /* TBD: other use cases */
    NvmmUCType_Num,
    NvmmUCType_Force32 = 0x7FFFFFFF
} NvmmUCType;

/**
 * @brief BLOCK Flag
 */
typedef enum
{
    NvBlockFlag_Default = 0x1,
    NvBlockFlag_Suspend = 0x2,
    NvBlockFlag_Terminate = 0x4,
    NvBlockFlag_HWA = 0x8,
    NvBlockFlag_AVP_OK = 0x10,
    NvBlockFlag_CPU_OK = 0x20,
    NvBlockFlag_UseNewLocale = 0x40,
    NvBlockFlag_UseNewBlockType = 0x80,
    NvBlockFlag_UseCustomStack = 0x100,
    NvBlockFlag_UseGreedyIramAlloc = 0x200,
    NvmmBlockFlag_Num,
    NvmmBlockFlag_Force32 = 0x7FFFFFFF
} NvmmBlockFlag;

/**
 * @brief PowerState enums
 */
typedef enum
{
    NvmmManagerPowerState_Unspecified = -1,
    NvmmManagerPowerState_FullOn = 0,
    NvmmManagerPowerState_LowOn,
    NvmmManagerPowerState_Standby,
    NvmmManagerPowerState_Sleep,
    NvmmManagerPowerState_Off,
    NvmmManagerPowerState_Num,
    NvmmManagerPowerState_Force32 = 0x7FFFFFFF
} NvmmManagerPowerState;

/* Allocate scratch IRAM for multimedia codec type CodecType of size Size.
 * Fill the allocated IRAM handle and physical memory in pOutputStruct.
 */

NvError NvmmManagerIRAMScratchAlloc(NvmmManagerHandle Handle,
                                    void* pResponseStruct,
                                    NvU32 CodecType, NvU32 Size);

/* Free scratch IRAM for multimedia codec type CodecType.
 */
NvError NvmmManagerIRAMScratchFree(NvmmManagerHandle Handle, NvU32 CodecType);

/* Register the element with the RPM. The RPM uses the provided callback to
 * request RP changes. The RPM passes back the initial resource profile for the
 * element.
 */

NvError NvmmManagerRegisterBlock(NvmmManagerHandle Handle,
                                 NvmmMgrBlockHandle* pBlock,
                                 NvU32 BlockType, NvmmUCType UseCaseType,
                                 NvBlockProfile* pRP);

/* Update block info with the RPM */
NvError NvmmManagerUpdateBlockInfo(NvmmMgrBlockHandle hBlock,
                                   void* hActualBlock, void* hBlockRmLoader,
                                   void* hServiceAvp, void* hServiceRmLoader);

/* Unregister the element with the RPM */
NvError NvmmManagerUnregisterBlock(NvmmMgrBlockHandle hBlock, NvBool bForceClose);

/**
 * Registers NvMM power client.
 *
 * @param hEventSemaphore The client semaphore for power management event
 *  signaling. If null, no events will be signaled to the particular client.
 * @param pClientId A pointer to the storage that on entry contains client
 *  tag (optional), and on exit returns client ID, assigned by power manager.
 *
 * @retval NvSuccess if registration was successful.
 * @retval NvError_InsufficientMemory if failed to allocate memory for client
 *  registration.
 */
NvError NvmmManagerRegisterProcessClient(NvmmManagerHandle Handle,
                                         NvOsSemaphoreHandle hEventSemaphore,
                                         NvmmPowerClientHandle* pClient);

/**
 * Unregisters NvMM power client.
 *
 * @param hEventSemaphore The client semaphore for power management event
 *  signaling. If null, no events will be signaled to the particular client.
 * @param pClientId A pointer to the storage that on entry contains client
 *  tag (optional), and on exit returns client ID, assigned by power manager.
 *
 * @retval NvSuccess if registration was successful.
 * @retval NvError_InsufficientMemory if failed to allocate memory for client
 *  registration.
 */
NvError NvmmManagerUnregisterProcessClient(NvmmPowerClientHandle pClient);

/**
 * Notifies NvMM Manager to change the Power state.
 *
 * @param NewPowerState specifies the new state
 *
 * @retval NvSuccess .
 */
NvError NvmmManagerChangePowerState(NvmmManagerPowerState NewPowerState);

/**
 * Gets the NvMM Manager the Power state.
 *
 * @param pPowerState address of the PowerState
 *
 * @retval NvSuccess .
 */
NvError NvmmManagerGetPowerState(NvmmManagerHandle Handle, NvU32* pPowerState);

/**
 * Notifies NvMM Manager .
 *
 * @param none
 *
 * @retval NvSuccess .
 */
NvError NvmmManagerNotify(NvmmManagerHandle Handle);

int NvMMManagerIsUsingNewAVP(void);

/** @} */

#if defined(__cplusplus)
}
#endif

#endif
