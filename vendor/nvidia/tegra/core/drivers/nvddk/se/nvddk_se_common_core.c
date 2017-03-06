/*
 * Copyright (c) 2011-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvos.h"
#include "nvassert.h"
#include "nvrm_power.h"
#include "nvrm_hardware_access.h"
#include "nvrm_module.h"
#include "nvrm_interrupt.h"
#include "nvddk_blockdev.h"
#include "nvddk_se_blockdev.h"
#include "nvddk_se_core.h"
#include "nvddk_se_hw.h"
#include "t30/arse.h"

#define SE_REGR(hSE, reg) \
     NV_READ32(hSE->pVirtualAddress + ((SE_##reg##_0) / 4))

static NvBool SeAesKeySlot[SeAesHwKeySlot_NumExt];
#define KEY_SLOT_LIMIT_EXCEED_ERROR_NUM SeAesHwKeySlot_NumExt
#define KEY_SLOT_UNUSED NV_FALSE
#define KEY_SLOT_USED   NV_TRUE

/* Buffers fields*/
#define BUFFER_A (0x1)
#define BUFFER_B (0x2)

/* Max timeout for SE = 10sec */
#define SE_WAITOUT_TIME (10000)

/* Common SE context which is independent of
 * Different algorithms supported by SE Engine
 */
NvDdkSeHWCtx *s_pSeHWCtx = NULL;
NvDdkSeCoreCapabilities *gs_pSeCoreCaps = NULL;

/* Mutex to access SE HW engine */
static NvOsMutexHandle s_SeCoreEngineMutex = {0};

/* Semaphore for Ping-Pong buffer */
NvOsSemaphoreHandle s_DualBufSem = NULL;

/*  The LL & Buffer structure used in SE
   ---------------------              ---------------------
   |     IN_LL_A      |-------------->|     IN_BUF_A      |
   ---------------------              ---------------------
   |     IN_LL_B      |-------------->|     IN_BUF_B      |
   ---------------------              ---------------------
   |     OUT_LL_A     |-------------->|     OUT_BUF_A     |
   ---------------------              ---------------------
   |     OUT_LL_B     |-------------->|     OUT_BUF_B     |
   ---------------------              ---------------------
 */

/* Static Function declarations */
static NvError VerifySBKClear(NvDdkSeAesHWCtx *hSeAesHwCtx);
static NvError VerifySskLock(NvDdkSeAesHWCtx *hSeAesHwCtx);
static NvBool DataCompare(NvU8 *SrcBuffer, NvU8 *DstBuffer, NvU32 Size);

static NvError NvDdkSeGetHwInterface(NvRmDeviceHandle hRmDevice, NvDdkSeCoreCapabilities **Caps)
{
    NvError e;
    NvDdkSeHwInterface *pHwIf;
    static NvDdkSeCoreCapabilities s_Caps1;

    NvRmModuleCapability SeCaps[ ] =
    {
        {1, 0, 0, NvRmModulePlatform_Silicon, &s_Caps1}
    };

    NvOsMemset((void *)&s_Caps1, 0x0, sizeof(NvDdkSeCoreCapabilities));
    pHwIf = NULL;

    NV_ASSERT(hRmDevice);

    /* In case of T30 Sp800-90 Rng and Pkc is not supported */
    s_Caps1.IsSPRngSupported                  = NV_FALSE;
    s_Caps1.IsPkcSupported                    = NV_FALSE;

    Caps = (NvDdkSeCoreCapabilities **)NvOsAlloc(sizeof(NvDdkSeCoreCapabilities *));
    if (Caps == NULL)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    NvOsMemset(Caps, 0x0, sizeof(NvDdkSeCoreCapabilities *));
    NV_CHECK_ERROR_CLEANUP(NvRmModuleGetCapabilities
            (hRmDevice,
             NvRmModuleID_Se,
             SeCaps,
             NV_ARRAY_SIZE(SeCaps),
             (void **)Caps)
             );
    if (*Caps == &s_Caps1)
    {
        /* Assigning T30 specific Function pointers */
       if (s_Caps1.pHwIf == NULL)
       {
           s_Caps1.pHwIf = NvOsAlloc(sizeof(NvDdkSeHwInterface));
           if (s_Caps1.pHwIf == NULL)
           {
               e = NvError_InsufficientMemory;
               goto fail;
           }
           pHwIf = s_Caps1.pHwIf;
       }
       gs_pSeCoreCaps = &s_Caps1;
       pHwIf->pfNvDdkSeShaProcessBuffer        = SeSHAProcessBuffer;
       pHwIf->pfNvDdkSeShaBackUpRegs           = SeSHABackupRegisters;
       pHwIf->pfNvDdkSeAesSetKey               = SeAesSetKey;
       pHwIf->pfNvDdkSeAesProcessBuffer        = SeAesProcessBuffer;
       pHwIf->pfNvDdkSeAesCMACProcessBuffer    = SeAesCMACProcessBuffer;
       pHwIf->pfNvDdkCollectCMAC               = CollectCMAC;
       pHwIf->pfNvDdkSeAesWriteLockKeySlot     = SeAesWriteLockKeySlot;
       pHwIf->pfNvDdkSeAesSetIv                = SeAesSetIv;
    }
    else
    {
        e = NvError_NotSupported;
        goto fail;
    }

fail:
    return e;
}
NvBool NvDdkIsSeCoreInitDone(void)
{
    return (NULL != s_pSeHWCtx);
}

static void InitializeAesKeyTable(void)
{
    NvU32 KeySlotIndex;

    for (KeySlotIndex = 0; KeySlotIndex < SeAesHwKeySlot_NumExt; KeySlotIndex++)
        SeAesKeySlot[KeySlotIndex] = KEY_SLOT_UNUSED;

    SeAesKeySlot[SeAesHwKeySlot_Sbk] = KEY_SLOT_USED;
    SeAesKeySlot[SeAesHwKeySlot_Ssk] = KEY_SLOT_USED;
}

/*  this is the place where memory for LL buffers need to be allocated */
NvError NvDdkSeCoreInit(NvRmDeviceHandle hRmDevice)
{
    NvError e = NvSuccess;
    NvU32 size = 0;
    NvU32 i = 0;
    NvU32 IrqList = 0;
    NvRmPhysAddr pBaseAddress = 0;
    NvRmPhysAddr LLDmaPhyAddr = 0;
    NvRmPhysAddr LLBufDmaPhyAddr = 0;
    NvU8 * pLLDmaVirtAddr = NULL;
    NvU8 * pLLBufDmaVirtAddr = NULL;
    NvOsInterruptHandler IntHandlers;
    NvDdkSeCoreCapabilities **SeCoreCaps = NULL;

    NV_ASSERT(hRmDevice);

    /* check if the device got initialized already */
    if (NvDdkIsSeCoreInitDone())
    {
        return NvSuccess;
    }

    /*  Allocating memory for SE context */
    s_pSeHWCtx = NvOsAlloc(sizeof(NvDdkSeHWCtx));
    if (s_pSeHWCtx == NULL)
        return NvError_InsufficientMemory;
    /* Clear the memory initially */
    NvOsMemset(s_pSeHWCtx, 0, sizeof(NvDdkSeHWCtx));

    /*  Register with Power manager */
    NV_CHECK_ERROR_CLEANUP(NvRmPowerRegister(hRmDevice,
                                             NULL,
                                             &s_pSeHWCtx->RmPowerClientId));

    NV_CHECK_ERROR(NvRmPowerVoltageControl(hRmDevice,
                                            NvRmModuleID_Se,
                                            s_pSeHWCtx->RmPowerClientId,
                                            NvRmVoltsUnspecified,
                                            NvRmVoltsUnspecified,
                                            NULL,
                                            0,
                                            NULL));
    NV_CHECK_ERROR_CLEANUP(NvRmPowerModuleClockControl(
                                            hRmDevice,
                                            NvRmModuleID_Se,
                                            s_pSeHWCtx->RmPowerClientId,
                                            NV_TRUE));

    /*  Get the SE register base (Physical) */
    NvRmModuleGetBaseAddress(hRmDevice,
        NVRM_MODULE_ID(NvRmModuleID_Se, 0),
        &pBaseAddress, &(s_pSeHWCtx->BankSize));
    /*  Map the register base to virtual  */
    NV_CHECK_ERROR_CLEANUP(NvRmPhysicalMemMap(pBaseAddress,
        s_pSeHWCtx->BankSize, NVOS_MEM_READ_WRITE, NvOsMemAttribute_Uncached,
        (void **)&(s_pSeHWCtx->pVirtualAddress)));

    /*  Create the SE engine mutex */
    NV_CHECK_ERROR_CLEANUP(NvOsMutexCreate(&s_SeCoreEngineMutex));

    /*  Create the Ping-Pong buffer Semaphore */
    NV_CHECK_ERROR_CLEANUP(NvOsSemaphoreCreate(&s_DualBufSem, 1));

    s_pSeHWCtx->hRmDevice = hRmDevice;

    /*  memory for both Input & Output LL will be reserved here */
    size = sizeof(NvDdkSeLLInfo) * NVDDK_SE_LL_NUM * 2;
    NV_CHECK_ERROR_CLEANUP(NvRmMemHandleCreate(hRmDevice,
                                               &s_pSeHWCtx->hDmaLLMem, size));

    NV_CHECK_ERROR_CLEANUP(NvRmMemAlloc(s_pSeHWCtx->hDmaLLMem,
                                        NULL,
                                        0,
                                        NVDDK_SE_HW_DMA_ADDR_ALIGNMENT,
                                        NvOsMemAttribute_Uncached));

    LLDmaPhyAddr = NvRmMemPin(s_pSeHWCtx->hDmaLLMem);

    NV_CHECK_ERROR_CLEANUP(NvRmMemMap(s_pSeHWCtx->hDmaLLMem,
                                      0,
                                      size,
                                      NVOS_MEM_READ_WRITE,
                                      (void **)&pLLDmaVirtAddr));

    /*  memory for both Input & Output LL buffers will be reserved here */
    /*  Memory for 2 input buffers and 2 output buffers */
    size = NVDDK_SE_HW_LL_BUFFER_SIZE_BYTES * NVDDK_SE_LL_NUM * 2;

    NV_CHECK_ERROR_CLEANUP(NvRmMemHandleCreate(hRmDevice,
                                               &s_pSeHWCtx->hDmaMemBuf, size));

    NV_CHECK_ERROR_CLEANUP(NvRmMemAlloc(s_pSeHWCtx->hDmaMemBuf,
                                        NULL,
                                        0,
                                        NVDDK_SE_HW_DMA_ADDR_ALIGNMENT,
                                        NvOsMemAttribute_WriteCombined));

    LLBufDmaPhyAddr = NvRmMemPin(s_pSeHWCtx->hDmaMemBuf);

    NV_CHECK_ERROR_CLEANUP(NvRmMemMap(s_pSeHWCtx->hDmaMemBuf,
                                      0,
                                      size,
                                      NVOS_MEM_READ_WRITE,
                                      (void **)&pLLBufDmaVirtAddr));

    for (i = 0; i < NVDDK_SE_LL_NUM; i++)
    {
        s_pSeHWCtx->InputLL[i].LLPhyAddr =
            (NvDdkSeLLInfo*)(LLDmaPhyAddr + i * sizeof(NvDdkSeLLInfo));
        s_pSeHWCtx->OutputLL[i].LLPhyAddr =
            (NvDdkSeLLInfo*)(LLDmaPhyAddr+ (i + NVDDK_SE_LL_NUM)* sizeof(NvDdkSeLLInfo));

        s_pSeHWCtx->InputLL[i].LLVirAddr =
            (NvDdkSeLLInfo*)(pLLDmaVirtAddr + i * sizeof(NvDdkSeLLInfo));
        s_pSeHWCtx->OutputLL[i].LLVirAddr =
            (NvDdkSeLLInfo*)(pLLDmaVirtAddr + (i + NVDDK_SE_LL_NUM)* sizeof(NvDdkSeLLInfo));

        s_pSeHWCtx->InputLL[i].LLVirAddr->LLBufPhyStartAddr =
            (NvU8*)(LLBufDmaPhyAddr + i * NVDDK_SE_HW_LL_BUFFER_SIZE_BYTES);
        s_pSeHWCtx->OutputLL[i].LLVirAddr->LLBufPhyStartAddr =
            (NvU8*)(LLBufDmaPhyAddr + (i + NVDDK_SE_LL_NUM)* NVDDK_SE_HW_LL_BUFFER_SIZE_BYTES);

        s_pSeHWCtx->InputLL[i].LLBufVirAddr =
            pLLBufDmaVirtAddr + i * NVDDK_SE_HW_LL_BUFFER_SIZE_BYTES;
        s_pSeHWCtx->OutputLL[i].LLBufVirAddr =
            pLLBufDmaVirtAddr + (i + NVDDK_SE_LL_NUM)* NVDDK_SE_HW_LL_BUFFER_SIZE_BYTES;

        s_pSeHWCtx->InputLL[i].LLVirAddr->LLBufNum = 0;
        s_pSeHWCtx->OutputLL[i].LLVirAddr->LLBufNum = 0;
    }

    s_pSeHWCtx->InterruptHandle = NULL;
    s_pSeHWCtx->ErrFlag = NV_FALSE;

    /* Disable Key schedule read */
    NV_CHECK_ERROR_CLEANUP(SeKeySchedReadDisable());

    /* Registering the SHA ISR with the system */
    IntHandlers = (NvOsInterruptHandler)SeIsr;
    IrqList = NvRmGetIrqForLogicalInterrupt(s_pSeHWCtx->hRmDevice,
                NvRmModuleID_Se, 0);
    e = NvRmInterruptRegister(s_pSeHWCtx->hRmDevice, 1, &IrqList,
                & IntHandlers, NULL, &s_pSeHWCtx->InterruptHandle, NV_FALSE);

    if (e != NvSuccess)
    {
        NvOsDebugPrintf("Failed to register the ISR \n");
        goto fail;
    }
    /* Initialize Aes Key table */
    InitializeAesKeyTable();

    NV_CHECK_ERROR_CLEANUP(NvDdkSeGetHwInterface(hRmDevice, SeCoreCaps));
    return NvSuccess;

fail:
    NvOsMutexDestroy(s_SeCoreEngineMutex);
    NvOsSemaphoreDestroy(s_DualBufSem);

    if (s_pSeHWCtx->hDmaMemBuf)
    {
        NvRmMemUnmap(s_pSeHWCtx->hDmaMemBuf,
                     s_pSeHWCtx->InputLL[0].LLBufVirAddr,
                     NVDDK_SE_HW_LL_BUFFER_SIZE_BYTES * NVDDK_SE_LL_NUM * 2);
        NvRmMemUnpin(s_pSeHWCtx->hDmaMemBuf);
        NvRmMemHandleFree(s_pSeHWCtx->hDmaMemBuf);
        s_pSeHWCtx->hDmaMemBuf = NULL;
    }

    if (s_pSeHWCtx->hDmaLLMem)
    {
        NvRmMemUnmap(s_pSeHWCtx->hDmaLLMem,
                     s_pSeHWCtx->InputLL[0].LLVirAddr,
                     sizeof(NvDdkSeLLInfo) * NVDDK_SE_LL_NUM * 2);
        NvRmMemUnpin(s_pSeHWCtx->hDmaLLMem);
        NvRmMemHandleFree(s_pSeHWCtx->hDmaLLMem);
        s_pSeHWCtx->hDmaLLMem = NULL;
    }

    NvRmPowerModuleClockControl(hRmDevice,
                                NvRmModuleID_Se,
                                s_pSeHWCtx->RmPowerClientId,
                                NV_FALSE);
    NvRmPowerVoltageControl(s_pSeHWCtx->hRmDevice,
                            NvRmModuleID_Se,
                            s_pSeHWCtx->RmPowerClientId,
                            NvRmVoltsOff,
                            NvRmVoltsOff,
                            NULL,
                            0,
                            NULL);

    NvOsFree(s_pSeHWCtx);
    s_pSeHWCtx = NULL;
    return e;
}

/*  This is the place where memory for LL buffers need to be freed */
void NvDdkSeCoreDeInit(void)
{
    // Destroy mutex
    NvOsMutexDestroy(s_SeCoreEngineMutex);
    NvOsSemaphoreDestroy(s_DualBufSem);

    /* Clear All Interupts */
    SeClearInterrupts();
    /* Unregister the ISR */
    if (s_pSeHWCtx->InterruptHandle)
    {
        NvRmInterruptUnregister(s_pSeHWCtx->hRmDevice, s_pSeHWCtx->InterruptHandle);
        s_pSeHWCtx->InterruptHandle = NULL;
    }

    if (s_pSeHWCtx->hDmaMemBuf)
    {
        NvRmMemUnmap(s_pSeHWCtx->hDmaMemBuf,
                     s_pSeHWCtx->InputLL[0].LLBufVirAddr,
                     NVDDK_SE_HW_LL_BUFFER_SIZE_BYTES * NVDDK_SE_LL_NUM * 2);
        NvRmMemUnpin(s_pSeHWCtx->hDmaMemBuf);
        NvRmMemHandleFree(s_pSeHWCtx->hDmaMemBuf);
        s_pSeHWCtx->hDmaMemBuf = NULL;
    }

    if (s_pSeHWCtx->hDmaLLMem)
    {
        NvRmMemUnmap(s_pSeHWCtx->hDmaLLMem,
                     s_pSeHWCtx->InputLL[0].LLVirAddr,
                     sizeof(NvDdkSeLLInfo) * NVDDK_SE_LL_NUM * 2);
        NvRmMemUnpin(s_pSeHWCtx->hDmaLLMem);
        NvRmMemHandleFree(s_pSeHWCtx->hDmaLLMem);
        s_pSeHWCtx->hDmaLLMem = NULL;
    }

    NvRmPowerModuleClockControl(s_pSeHWCtx->hRmDevice,
                                NvRmModuleID_Se,
                                s_pSeHWCtx->RmPowerClientId,
                                NV_FALSE);

    NvOsFree(s_pSeHWCtx);
    s_pSeHWCtx = NULL;
}

NvError NvDdkSeShaInit(NvDdkSeShaHWCtx *hSeShaHwCtx, const NvDdkSeShaInitInfo *pInitInfo)
{

    NvError e = NvSuccess;
    NV_ASSERT(hSeShaHwCtx);
    NV_ASSERT(pInitInfo);

    if (!pInitInfo->TotalMsgSizeInBytes)
        return NvError_InvalidSize;

    hSeShaHwCtx->Mode = pInitInfo->SeSHAEncMode;
    hSeShaHwCtx->MsgLength = (NvU64)IN_BITS(pInitInfo->TotalMsgSizeInBytes);
    hSeShaHwCtx->MsgLeft = (NvU64)(hSeShaHwCtx->MsgLength);
    hSeShaHwCtx->IsSHAInitHwHash = 1;
    hSeShaHwCtx->LLAddr = NULL;

    switch (hSeShaHwCtx->Mode)
    {
        case NvDdkSeShaOperatingMode_Sha1:
            hSeShaHwCtx->ShaPktMode = SE_MODE_PKT_SHAMODE_SHA1;
            hSeShaHwCtx->ShaFinalHashLen = IN_BYTES(ARSE_SHA1_HASH_SIZE);
            hSeShaHwCtx->ShaBlockSize = SHA1_BLOCK_SIZE;
            break;
        case NvDdkSeShaOperatingMode_Sha224:
            hSeShaHwCtx->ShaPktMode = SE_MODE_PKT_SHAMODE_SHA224;
            hSeShaHwCtx->ShaFinalHashLen = IN_BYTES(ARSE_SHA224_HASH_SIZE);
            hSeShaHwCtx->ShaBlockSize = SHA224_BLOCK_SIZE;
            break;
        case NvDdkSeShaOperatingMode_Sha256:
            hSeShaHwCtx->ShaPktMode = SE_MODE_PKT_SHAMODE_SHA256;
            hSeShaHwCtx->ShaFinalHashLen = IN_BYTES(ARSE_SHA256_HASH_SIZE);
            hSeShaHwCtx->ShaBlockSize = SHA256_BLOCK_SIZE;
            break;
        case NvDdkSeShaOperatingMode_Sha384:
            hSeShaHwCtx->ShaPktMode = SE_MODE_PKT_SHAMODE_SHA384;
            hSeShaHwCtx->ShaFinalHashLen = IN_BYTES(ARSE_SHA384_HASH_SIZE);
            hSeShaHwCtx->ShaBlockSize = SHA384_BLOCK_SIZE;
            break;
        case NvDdkSeShaOperatingMode_Sha512:
            hSeShaHwCtx->ShaPktMode = SE_MODE_PKT_SHAMODE_SHA512;
            hSeShaHwCtx->ShaFinalHashLen = IN_BYTES(ARSE_SHA512_HASH_SIZE);
            hSeShaHwCtx->ShaBlockSize = SHA512_BLOCK_SIZE;
            break;
        default:
            e = NvError_BadParameter;
            break;
    }
    return e;
}

NvError NvDdkSeShaUpdate(NvDdkSeShaHWCtx *hSeShaHwCtx,
                         const NvDdkSeShaUpdateArgs *pProcessBufInfo)
{
    NvError e = NvSuccess;
    NvU8 *DstBuf = NULL;
    NvU32 BytesToBeProcessed = 0;
    NvU32 CurLLBufSize = 0;
    NvU32 sofar = 0;
    NvU32 BufferToUse = 0;
    NvDdkSeLLInfo *LLtoSHA = NULL;
    NvBool IsFirst = 1;

    NV_ASSERT(hSeShaHwCtx);
    NV_ASSERT(pProcessBufInfo);
    NV_ASSERT(s_pSeHWCtx);

    NvOsMutexLock(s_SeCoreEngineMutex);

    IsFirst = 1;
    s_pSeHWCtx->BufferInUse = BUFFER_A;
    BytesToBeProcessed = pProcessBufInfo->InputBufSize;
    while (BytesToBeProcessed)
    {
        /* Get the Buffer to fill data */
        if (s_pSeHWCtx->BufferInUse == BUFFER_A)
            BufferToUse = BUFFER_B;
        else
            BufferToUse = BUFFER_A;

        /* Get the current buffer size that can be processed */
        if (BytesToBeProcessed >= NVDDK_SE_HW_LL_BUFFER_SIZE_BYTES)
            CurLLBufSize = NVDDK_SE_HW_LL_BUFFER_SIZE_BYTES;
        else
            CurLLBufSize = BytesToBeProcessed;

        if (BufferToUse == BUFFER_A)
        {
            LLtoSHA = s_pSeHWCtx->InputLL[BUFFER_A-1].LLPhyAddr;
            DstBuf = s_pSeHWCtx->InputLL[BUFFER_A-1].LLBufVirAddr;
            s_pSeHWCtx->InputLL[BUFFER_A-1].LLVirAddr->LLBufSize = CurLLBufSize;
        }
        else if (BufferToUse == BUFFER_B)
        {
            LLtoSHA = s_pSeHWCtx->InputLL[BUFFER_B-1].LLPhyAddr;
            DstBuf = s_pSeHWCtx->InputLL[BUFFER_B-1].LLBufVirAddr;
            s_pSeHWCtx->InputLL[BUFFER_B-1].LLVirAddr->LLBufSize = CurLLBufSize;
        }

        /*  Copy the data to Buffer */
        NvOsMemcpy(DstBuf,
                   (pProcessBufInfo->SrcBuf)+sofar,
                   CurLLBufSize);
        NvOsFlushWriteCombineBuffer();

        /* Wait for SHA Engine to become free, which will be signalled in ISR */
        e = NvOsSemaphoreWaitTimeout(s_DualBufSem, SE_WAITOUT_TIME);
        if (e == NvError_Timeout)
            goto fail;

        /*  Do not take the HASH register backup for the first time */
        if (IsFirst)
            IsFirst = 0;
        else
            NV_CHECK_ERROR_CLEANUP(gs_pSeCoreCaps->pHwIf->pfNvDdkSeShaBackUpRegs(hSeShaHwCtx));

        /*  Enabling the interrupt */
        e = NvRmInterruptEnable(s_pSeHWCtx->hRmDevice, s_pSeHWCtx->InterruptHandle);
        if (e != NvSuccess)
            goto fail;

        if (s_pSeHWCtx->ErrFlag)
        {
            e = NvError_InvalidState;
            goto fail;
        }

        /* If it is not the last Chunk, it should always be multiple of block size */
        if (IN_BITS(CurLLBufSize) % (hSeShaHwCtx->ShaBlockSize))
        {
            if (IN_BYTES(hSeShaHwCtx->MsgLeft) != (NvU64)CurLLBufSize)
            {
                e = NvError_BadParameter;
                goto fail;
            }
        }
        /*  Set the Buffer in Use */
        s_pSeHWCtx->BufferInUse = BufferToUse;

        /*  Set the LL to be used by SHA */
        hSeShaHwCtx->LLAddr = LLtoSHA;

        NV_CHECK_ERROR_CLEANUP(gs_pSeCoreCaps->pHwIf->pfNvDdkSeShaProcessBuffer(hSeShaHwCtx));

        sofar += CurLLBufSize;
        BytesToBeProcessed -= CurLLBufSize;
    }

fail:
    /* Check whether the last buffer submission is processed or not */
    NvOsSemaphoreWaitTimeout(s_DualBufSem, SE_WAITOUT_TIME);
    e = gs_pSeCoreCaps->pHwIf->pfNvDdkSeShaBackUpRegs(hSeShaHwCtx);
    NvOsSemaphoreSignal(s_DualBufSem);

    /*  Release the lock for SE */
    NvOsMutexUnlock(s_SeCoreEngineMutex);
    return e;
}

NvError NvDdkSeShaFinal(NvDdkSeShaHWCtx *hSeShaHwCtx, NvDdkSeShaFinalInfo *pFinalInfo)
{
    NvError e = NvSuccess;
    NvU32 i = 0;
    NvU32 temp = 0;
    NvU32 bytesToCopy = 0;

    NV_ASSERT(hSeShaHwCtx);
    NV_ASSERT(pFinalInfo);

    switch (hSeShaHwCtx->Mode)
    {
        case NvDdkSeShaOperatingMode_Sha1:
            bytesToCopy = IN_BYTES(NvDdkSeShaResultSize_Sha1);
            break;
        case NvDdkSeShaOperatingMode_Sha224:
            bytesToCopy = IN_BYTES(NvDdkSeShaResultSize_Sha224);
            break;
        case NvDdkSeShaOperatingMode_Sha256:
            bytesToCopy = IN_BYTES(NvDdkSeShaResultSize_Sha256);
            break;
        case NvDdkSeShaOperatingMode_Sha384:
            bytesToCopy = IN_BYTES(NvDdkSeShaResultSize_Sha384);
            break;
        case NvDdkSeShaOperatingMode_Sha512:
            bytesToCopy = IN_BYTES(NvDdkSeShaResultSize_Sha512);
            break;
        default:
            e = NvError_BadParameter;
            goto fail;
    }

    if (pFinalInfo->HashSize != bytesToCopy)
    {
        e = NvError_BadParameter;
        goto fail;
    }

    for (i = 0; i < bytesToCopy; i++)
    {
        temp = hSeShaHwCtx->HashResult[i / 4];
        pFinalInfo->OutBuf[i] = temp >> (3 - (i % 4)) * 8;
    }

    if ((hSeShaHwCtx->Mode == NvDdkSeShaOperatingMode_Sha384) ||
        (hSeShaHwCtx->Mode == NvDdkSeShaOperatingMode_Sha512))
    {
        NvU32 *p = (NvU32*)pFinalInfo->OutBuf;
        i = 0;
        while (i < (bytesToCopy >> 2))
        {
            temp = *(p + i);
            *(p + i) = *(p + i + 1);
            *(p + i + 1) = temp;
            i += 2;
        }
    }

fail:
    return e;
}

/* AES Specific */

NvError
NvDdkSeAesSelectOperation(NvDdkSeAesHWCtx *hSeAesHwCtx, const NvDdkSeAesOperation *pOperation)
{
    NV_ASSERT(hSeAesHwCtx);
    NV_ASSERT(pOperation);

    switch (pOperation->OpMode)
    {
        case NvDdkSeAesOperationalMode_Cbc:
        case NvDdkSeAesOperationalMode_Ecb:
            hSeAesHwCtx->IsEncryption = pOperation->IsEncrypt;
            break;
        default:
            return NvError_NotSupported;
    }
    hSeAesHwCtx->OpMode = pOperation->OpMode;
    return NvSuccess;
}

static NvU32 SeAesGetFreeKeySlot(void)
{
    NvU32 KeySlotIndex;

    for (KeySlotIndex = 0; KeySlotIndex < SeAesHwKeySlot_NumExt; KeySlotIndex++)
        if (SeAesKeySlot[KeySlotIndex] == KEY_SLOT_UNUSED)
            return KeySlotIndex;

    return KEY_SLOT_LIMIT_EXCEED_ERROR_NUM;
}

NvError
NvDdkSeAesGetInitialVector(NvDdkSeAesHWCtx *hSeAesHwCtx, NvDdkSeAesGetIvArgs *pIvInfo)
{
    NV_ASSERT(hSeAesHwCtx);
    NV_ASSERT(pIvInfo);
    NV_ASSERT(pIvInfo->Iv);

    if (pIvInfo->VectorSize < NvDdkSeAesConst_IVLengthBytes)
        return NvError_BadParameter;

    NvOsMemcpy(pIvInfo->Iv, hSeAesHwCtx->IV, NvDdkSeAesConst_IVLengthBytes);
    return NvSuccess;
}

NvError
NvDdkSeAesClearSecureBootKey(NvDdkSeAesHWCtx *hSeAesHwCtx)
{
    NvError e = NvSuccess;
    NvU8 ZeroVector[16];
    NvBool SetKeyDone = NV_FALSE;

    /* Lock the engine */
    NvOsMutexLock(s_SeCoreEngineMutex);

    NV_ASSERT(hSeAesHwCtx);
    /* Set Zero key & IV in SBK */
    NvOsMemset(ZeroVector, 0x0, sizeof(ZeroVector));

    hSeAesHwCtx->KeySlotNum    = SeAesHwKeySlot_Sbk;
    NvOsMemcpy(hSeAesHwCtx->Key, ZeroVector, NvDdkSeAesKeySize_128Bit);
    hSeAesHwCtx->KeyLenInBytes = NvDdkSeAesKeySize_128Bit;

    /* Clear SBK */
    NV_CHECK_ERROR_CLEANUP(gs_pSeCoreCaps->pHwIf->pfNvDdkSeAesSetKey(hSeAesHwCtx));
    NvOsMutexUnlock(s_SeCoreEngineMutex);

    SetKeyDone = NV_TRUE;
    /*Verify SBK clear */
    NV_CHECK_ERROR_CLEANUP(VerifySBKClear(hSeAesHwCtx));
fail:
    if (!SetKeyDone)
    {
        /* Unlock the engine */
        NvOsMutexUnlock(s_SeCoreEngineMutex);
    }
    return e;
}

void
NvDdkSeAesReleaseKeySlot(NvDdkSeAesHWCtx *hSeAesHwCtx)
{
    /* Lock the engine */
    NvOsMutexLock(s_SeCoreEngineMutex);
    NV_ASSERT(hSeAesHwCtx);

    if (hSeAesHwCtx->KeySlotNum >= SeAesHwKeySlot_NumExt ||
            hSeAesHwCtx->KeySlotNum == SeAesHwKeySlot_Sbk ||
            hSeAesHwCtx->KeySlotNum == SeAesHwKeySlot_Ssk)
    {
        goto Done;
    }

    /* Freeing up key slot */
    SeAesKeySlot[hSeAesHwCtx->KeySlotNum] = KEY_SLOT_UNUSED;
Done:
    NvOsMutexUnlock(s_SeCoreEngineMutex);
}

static NvError
SetUserSpecifiedKey(NvDdkSeAesHWCtx *hSeAesHwCtx, const NvDdkSeAesKeyInfo *pKeyInfo)
{
    NvError e = NvSuccess;
    NvU32 FreeKeySlot;

    if (hSeAesHwCtx == NULL)
    {
        e = NvError_BadParameter;
        goto fail;
    }
    FreeKeySlot = SeAesGetFreeKeySlot();
    if (FreeKeySlot == KEY_SLOT_LIMIT_EXCEED_ERROR_NUM)
    {
        e =  NvError_AlreadyAllocated;
        goto fail;
    }
    /* Mark Keyslot as used */
    SeAesKeySlot[FreeKeySlot] = KEY_SLOT_USED;
    hSeAesHwCtx->KeySlotNum = FreeKeySlot;
    /* Populate key properties */
    hSeAesHwCtx->KeyLenInBytes = pKeyInfo->KeyLength;
    NvOsMemcpy(hSeAesHwCtx->Key, pKeyInfo->Key, hSeAesHwCtx->KeyLenInBytes);
    NV_CHECK_ERROR_CLEANUP(gs_pSeCoreCaps->pHwIf->pfNvDdkSeAesSetKey(hSeAesHwCtx));
fail:
    return e;
}

NvError
NvDdkSeAesSelectKey(NvDdkSeAesHWCtx *hSeAesHwCtx, const NvDdkSeAesKeyInfo *pKeyInfo)
{
    NvError e = NvSuccess;

    NV_ASSERT(hSeAesHwCtx);
    NV_ASSERT(pKeyInfo);
    NV_ASSERT(pKeyInfo->Key);

    /* Lock the Engne */
    NvOsMutexLock(s_SeCoreEngineMutex);

    switch (pKeyInfo->KeyLength)
    {
        case NvDdkSeAesKeySize_128Bit:
        case NvDdkSeAesKeySize_192Bit:
        case NvDdkSeAesKeySize_256Bit:
            break;
        default:
           e =  NvError_BadParameter;
           goto fail;
    }

    if (pKeyInfo->KeyLength == NvDdkSeAesKeySize_128Bit)
    {
        switch (pKeyInfo->KeyType)
        {
            case NvDdkSeAesKeyType_SecureBootKey:
                {
                    hSeAesHwCtx->KeySlotNum = SeAesHwKeySlot_Sbk;
                    hSeAesHwCtx->KeyLenInBytes = NvDdkSeAesKeySize_128Bit;
                    break;
                }
            case NvDdkSeAesKeyType_SecureStorageKey:
                {
                    hSeAesHwCtx->KeySlotNum = SeAesHwKeySlot_Ssk;
                    hSeAesHwCtx->KeyLenInBytes = NvDdkSeAesKeySize_128Bit;
                    break;
                }
            case NvDdkSeAesKeyType_UserSpecified:
                {
                    NV_CHECK_ERROR_CLEANUP(SetUserSpecifiedKey(hSeAesHwCtx, pKeyInfo));
                    break;
                }
            default:
                e  = NvError_BadParameter;
                goto fail;
        }
    }
    else
    {
       if (pKeyInfo->KeyType != NvDdkSeAesKeyType_UserSpecified)
        {
            e = NvError_NotSupported;
            goto fail;
        }
       NV_CHECK_ERROR_CLEANUP(SetUserSpecifiedKey(hSeAesHwCtx, pKeyInfo));
    }
    hSeAesHwCtx->KeyType = pKeyInfo->KeyType;
fail:
    /* Unlock the engine */
    NvOsMutexUnlock(s_SeCoreEngineMutex);
    return e;
}

NvError
NvDdkSeAesProcessBuffer(
    NvDdkSeAesHWCtx *hSeAesHwCtx,
    const NvDdkSeAesProcessBufferInfo *pProcessBufInfo)
{
    NvError e = NvSuccess;
    NvU32 BytesToBeProcessed;
    NvU32 CurBufPos;
    NvU32 CurBufSize;
    NvU8  *pDmaBuf;

    /* Lock the Engine */
    NvOsMutexLock(s_SeCoreEngineMutex);

    NV_ASSERT(hSeAesHwCtx);
    NV_ASSERT(pProcessBufInfo->pSrcBuffer);
    NV_ASSERT(pProcessBufInfo->pDstBuffer);

    switch (hSeAesHwCtx->OpMode)
    {
        case NvDdkSeAesOperationalMode_Cbc:
        case NvDdkSeAesOperationalMode_Ecb:
        case NvDdkSeAesOperationalMode_Ofb:
        case NvDdkSeAesOperationalMode_Ctr:
            break;
        default:
            return NvError_InvalidState;
    }

    if (pProcessBufInfo->SrcBufSize % NvDdkSeAesConst_BlockLengthBytes)
        return NvError_BadParameter;

    BytesToBeProcessed = pProcessBufInfo->SrcBufSize;
    CurBufPos = 0;

    while (BytesToBeProcessed)
    {
        if (BytesToBeProcessed  >= NVDDK_SE_HW_LL_BUFFER_SIZE_BYTES)
            CurBufSize = NVDDK_SE_HW_LL_BUFFER_SIZE_BYTES;
        else
            CurBufSize = BytesToBeProcessed;

        /* Copy Source data into DMA Buffer */
        pDmaBuf = s_pSeHWCtx->InputLL[BUFFER_A-1].LLBufVirAddr;
        NvOsMemcpy(pDmaBuf, pProcessBufInfo->pSrcBuffer + CurBufPos, CurBufSize);

        /* Fill the Input Linked List */
        s_pSeHWCtx->InputLL[BUFFER_A-1].LLVirAddr->LLBufSize = CurBufSize;
        s_pSeHWCtx->InputLL[BUFFER_A-1].LLVirAddr->LLBufNum  = 0;

        /* Fill the  Output Linked List */
        s_pSeHWCtx->OutputLL[BUFFER_A-1].LLVirAddr->LLBufSize = CurBufSize;
        s_pSeHWCtx->OutputLL[BUFFER_A-1].LLVirAddr->LLBufNum  = 0;

        /* Fill source Buffer Size */
        hSeAesHwCtx->SrcBufSize =  CurBufSize;

        hSeAesHwCtx->InLLAddr  = (NvU32)s_pSeHWCtx->InputLL[BUFFER_A-1].LLPhyAddr;
        hSeAesHwCtx->OutLLAddr = (NvU32)s_pSeHWCtx->OutputLL[BUFFER_A-1].LLPhyAddr;

        /* Aes Process Buffer */
        NV_CHECK_ERROR_CLEANUP(
                gs_pSeCoreCaps->pHwIf->pfNvDdkSeAesProcessBuffer(hSeAesHwCtx)
                );

        /* Read Output back to buffer */
        NvOsMemcpy(pProcessBufInfo->pDstBuffer + CurBufPos, s_pSeHWCtx->OutputLL[BUFFER_A-1].LLBufVirAddr, CurBufSize);

        /* Updating The Loop Control variables */
        BytesToBeProcessed -= CurBufSize;
        CurBufPos          += CurBufSize;
    }

fail:
    /* Unclok the Engine */
    NvOsMutexUnlock(s_SeCoreEngineMutex);
    return e;
}

static void LeftShiftByOne(NvU8 Array[], NvU32 Size)
{
    NvU8 carry;
    NvU32 i;

    Array[0] <<= 1;
    for(carry = 0, i = 1; i < Size; i++)
    {
        carry = (Array[i] >> 7) & 0x01;
        Array[i - 1] |= carry;
        Array[i] <<= 1;
    }
}

NvError
NvDdkSeAesComputeCMAC(
        NvDdkSeAesHWCtx *hSeAesHwCtx,
        const NvDdkSeAesComputeCmacInInfo *pInInfo,
        NvDdkSeAesComputeCmacOutInfo *pOutInfo)
{
    NvError e = NvSuccess;
    NvU8 TmpBuf[16], L[16], K1[16], K2[16], *pDmaBuf;
    NvDdkSeAesKeyInfo KeyInfo;
    NvDdkSeAesOperation OpInfo;
    NvDdkSeAesSetIvInfo SetIvInfo;
    NvDdkSeAesProcessBufferInfo PbInfo;
    NvS16 i;
    NvU32 BytesToBeProcessed, CurBufPos, NumOfBlocks, LastBlockBytes = 0;
    NvU32 CurBufSize, KeySlotIndexStore;
    NvBool PadData = NV_FALSE;

    /* Lock the Engine */
    NvOsMutexLock(s_SeCoreEngineMutex);
    if (!pInInfo)
    {
        e = NvError_BadParameter;
        goto fail;
    }
    if (!pInInfo->IsLastChunk)
    {
        if (pInInfo->BufSize % 16)
        {
            e = NvError_BadParameter;
            goto fail;
        }
    }
    NvOsMemset(TmpBuf, 0x0, 16);

    /* Set Key And Iv For First Time */
    if (pInInfo->IsFirstChunk)
    {
        OpInfo.OpMode    = NvDdkSeAesOperationalMode_Cbc;
        OpInfo.IsEncrypt = NV_TRUE;

        KeyInfo.KeyLength          = pInInfo->KeyLen;
        NvOsMemcpy(KeyInfo.Key, pInInfo->Key, pInInfo->KeyLen);
        KeyInfo.KeyType            = NvDdkSeAesKeyType_UserSpecified;

        /* Select operation */
        NV_CHECK_ERROR_CLEANUP(NvDdkSeAesSelectOperation(hSeAesHwCtx,
                    (const NvDdkSeAesOperation *)&OpInfo));

        /* Set key*/
        NV_CHECK_ERROR_CLEANUP(NvDdkSeAesSelectKey(hSeAesHwCtx,
                    (const NvDdkSeAesKeyInfo *)&KeyInfo));

        SetIvInfo.pIV = TmpBuf;
        SetIvInfo.VectorSize = NvDdkSeAesConst_IVLengthBytes;
        /* Set Iv */
        NV_CHECK_ERROR_CLEANUP(NvDdkSeAesSetIv(hSeAesHwCtx,
                     (const NvDdkSeAesSetIvInfo *)&SetIvInfo));
    }

    /* Fill temporary buffer with zeros */
    NumOfBlocks = (pInInfo->BufSize / 16);
    if (pInInfo->IsLastChunk)
    {
        if (!pOutInfo)
        {
            e = NvError_BadParameter;
            goto fail;
        }
        /* Frist process n-1 blocks of data */
        if (!NumOfBlocks || (pInInfo->BufSize % 16))
        {
            PadData = NV_TRUE;
            LastBlockBytes = (pInInfo->BufSize % 16);
        }
        else
        {
            NumOfBlocks--;
            PadData = NV_FALSE;
            LastBlockBytes = 16;
        }
    }

    BytesToBeProcessed = NumOfBlocks * 16;
    CurBufPos = 0;
    pDmaBuf = s_pSeHWCtx->InputLL[BUFFER_A-1].LLBufVirAddr;
    while (BytesToBeProcessed)
    {
        if (BytesToBeProcessed  <= NVDDK_SE_HW_LL_BUFFER_SIZE_BYTES)
            CurBufSize = BytesToBeProcessed;
        else
            CurBufSize = NVDDK_SE_HW_LL_BUFFER_SIZE_BYTES;

        NvOsMemset(pDmaBuf, 0x0, NVDDK_SE_HW_LL_BUFFER_SIZE_BYTES);
        NvOsMemcpy(pDmaBuf,  pInInfo->pBuffer + CurBufPos, CurBufSize);

        s_pSeHWCtx->InputLL[BUFFER_A-1].LLVirAddr->LLBufSize = CurBufSize;
        s_pSeHWCtx->InputLL[BUFFER_A-1].LLVirAddr->LLBufNum  = 0;

        hSeAesHwCtx->SrcBufSize =  CurBufSize;
        hSeAesHwCtx->InLLAddr  = (NvU32)s_pSeHWCtx->InputLL[BUFFER_A-1].LLPhyAddr;

        NV_CHECK_ERROR_CLEANUP(gs_pSeCoreCaps->pHwIf->pfNvDdkSeAesCMACProcessBuffer(hSeAesHwCtx));
        BytesToBeProcessed  -= CurBufSize;
        CurBufPos           += CurBufPos;
    }

    /*
     * For processing last chunk, do the following :
     * compute K1 and K2.
     * Pad the data appropriatly.
     * Process 16 bytes of data (includes data + pad).
     * Get CMAC output.
     */
    if (pInInfo->IsLastChunk)
    {
       /* Store the Key Slot We Are Using in a Temporary variable */
       KeySlotIndexStore = hSeAesHwCtx->KeySlotNum;
       NvOsMemset(TmpBuf, 0x0, 16);
       /* Compute L  = CBC Operation with Zero Iv and Zero Ipdata and key K */
       KeyInfo.KeyLength          = pInInfo->KeyLen;
       NvOsMemcpy(KeyInfo.Key, pInInfo->Key, pInInfo->KeyLen);
       KeyInfo.KeyType            = NvDdkSeAesKeyType_UserSpecified;

       SetIvInfo.pIV    = TmpBuf;
       SetIvInfo.VectorSize = NvDdkSeAesConst_IVLengthBytes;

       PbInfo.pSrcBuffer = TmpBuf;
       PbInfo.pDstBuffer = L;
       PbInfo.SrcBufSize = 16;

       /* Select operation */
       NV_CHECK_ERROR_CLEANUP(NvDdkSeAesSelectOperation(hSeAesHwCtx,
                   (const NvDdkSeAesOperation *)&OpInfo));

       /* Select Key */
       NV_CHECK_ERROR_CLEANUP(NvDdkSeAesSelectKey(hSeAesHwCtx,
                   (const NvDdkSeAesKeyInfo *)&KeyInfo));
       /* Select IV */
       NV_CHECK_ERROR_CLEANUP(NvDdkSeAesSetIv(hSeAesHwCtx,
                   ( NvDdkSeAesSetIvInfo*)&SetIvInfo));
       /* Process buffer */
       NV_CHECK_ERROR_CLEANUP(NvDdkSeAesProcessBuffer(
                   hSeAesHwCtx,
                   (const NvDdkSeAesProcessBufferInfo *)&PbInfo));

       /*
        * Free the keyslot Used for computing L.
        * Get the old keyslot into the picture.
        */
       SeAesKeySlot[hSeAesHwCtx->KeySlotNum] = KEY_SLOT_UNUSED;
       hSeAesHwCtx->KeySlotNum = KeySlotIndexStore;

       /**
        * K1 = L << 1 if MSB of K1 == 0, else K1 = L ^ 0x87
        *
        * K2 = K1 << 1 if MSB of K1 == 0, else K2 = K1 ^ 0x87
        */
       NvOsMemcpy(K1, L, 16);
       LeftShiftByOne(K1, 16);
       if ((L[0] >> 7) & 0x1)
       {
            K1[15] = K1[15] ^ 0x87;
       }
       NvOsMemcpy(K2, K1, 16);
       LeftShiftByOne(K2, 16);
       if ((K1[0] >> 7) & 0x1)
       {
            K2[15] = K2[15] ^ 0x87;
       }
       /* Process Last Block */
       NvOsMemcpy(TmpBuf, pInInfo->pBuffer + NumOfBlocks * 16, LastBlockBytes);
       if (PadData)
       {
           TmpBuf[LastBlockBytes] = 0x80;

           for (i = LastBlockBytes + 1; i < 16; i++)
                TmpBuf[i] = 0x00;
           for (i = 0; i < 16; i++)
                TmpBuf[i] ^= K2[i];
       }
       else
       {
            /* XOR with K1 */
            for (i = 0; i < 16; i++)
                 TmpBuf[i] ^= K1[i];
       }

       NvOsMemset(pDmaBuf, 0x0, NVDDK_SE_HW_LL_BUFFER_SIZE_BYTES);
       NvOsMemcpy(pDmaBuf,  TmpBuf, 16);

       s_pSeHWCtx->InputLL[BUFFER_A-1].LLVirAddr->LLBufSize = 16;
       s_pSeHWCtx->InputLL[BUFFER_A-1].LLVirAddr->LLBufNum  = 0;

       hSeAesHwCtx->SrcBufSize = 16;
       hSeAesHwCtx->InLLAddr  = (NvU32)s_pSeHWCtx->InputLL[BUFFER_A-1].LLPhyAddr;
       NV_CHECK_ERROR_CLEANUP(gs_pSeCoreCaps->pHwIf->pfNvDdkSeAesCMACProcessBuffer(hSeAesHwCtx));
       if (pOutInfo->CMACLen < 16)
       {
            e = NvError_InvalidSize;
            goto fail;
       }
       NV_CHECK_ERROR_CLEANUP(
               gs_pSeCoreCaps->pHwIf->pfNvDdkCollectCMAC((NvU32 *)pOutInfo->pCMAC)
                );
    }
fail:
    /* Unlock the engine */
    NvOsMutexUnlock(s_SeCoreEngineMutex);
    return e;
}

static NvError
NvDdkSeAesWriteLockKeySlot(NvDdkSeAesHWCtx *hSeAesHwCtx, NvU32 *pKeySlotIndex)
{
    NvError e = NvSuccess;

    /* Lock the engine */
    NvOsMutexLock(s_SeCoreEngineMutex);

    NV_ASSERT(hSeAesHwCtx);
    if (*pKeySlotIndex >= SeAesHwKeySlot_NumExt)
    {
        e = NvError_NotSupported;
        goto fail;
    }

    gs_pSeCoreCaps->pHwIf->pfNvDdkSeAesWriteLockKeySlot(*pKeySlotIndex);
fail:
    /* Unlock The engine */
    NvOsMutexUnlock(s_SeCoreEngineMutex);
    return e;
}

NvError
NvDdkSeAesLockSecureStorageKey(NvDdkSeAesHWCtx *hSeAesHwCtx)
{
    NvU32 KeySlotIndex;
    NvError e = NvSuccess;
    NV_ASSERT(hSeAesHwCtx);

    KeySlotIndex = SeAesHwKeySlot_Ssk;
    NV_CHECK_ERROR_CLEANUP(NvDdkSeAesWriteLockKeySlot(hSeAesHwCtx, &KeySlotIndex));

    /* Verify Ssk lock */
    NV_CHECK_ERROR_CLEANUP(VerifySskLock(hSeAesHwCtx));
fail:
    return e;
}

NvError
NvDdkSeAesSetAndLockSecureStorageKey(
        NvDdkSeAesHWCtx *hSeAesHwCtx,
         const NvDdkSeAesSsk *SskInfo)
{
    NvError e = NvSuccess;
    NvBool SetKeyDone = NV_FALSE;

    /* Lock the Engine */
    NvOsMutexLock(s_SeCoreEngineMutex);

    NV_ASSERT(hSeAesHwCtx);
    NV_ASSERT(SskInfo);
    NV_ASSERT(SskInfo->pSsk);

    NvOsMemcpy(hSeAesHwCtx->Key, SskInfo->pSsk, NvDdkSeAesKeySize_128Bit);
    hSeAesHwCtx->KeyLenInBytes = NvDdkSeAesKeySize_128Bit;
    hSeAesHwCtx->KeySlotNum = SeAesHwKeySlot_Ssk;

    /**
     * Write  SSk in to key slot
     */
    NV_CHECK_ERROR_CLEANUP(gs_pSeCoreCaps->pHwIf->pfNvDdkSeAesSetKey(hSeAesHwCtx));
    /* Unlock the Engine */
    NvOsMutexUnlock(s_SeCoreEngineMutex);

    SetKeyDone = NV_TRUE;
    /**
     * Write Lock SSK
     */
    NV_CHECK_ERROR_CLEANUP(NvDdkSeAesWriteLockKeySlot(hSeAesHwCtx, &hSeAesHwCtx->KeySlotNum));

    /* Verify Ssk lock */
    NV_CHECK_ERROR_CLEANUP(VerifySskLock(hSeAesHwCtx));
fail:
    if (!SetKeyDone)
    {
        /* Unlock the engine */
        NvOsMutexUnlock(s_SeCoreEngineMutex);
    }
    return e;
}

NvError
NvDdkSeAesSetIv(NvDdkSeAesHWCtx *hSeAesHwCtx, const NvDdkSeAesSetIvInfo *pSetIvInfo)
{
    NvError e = NvSuccess;

    /* Lock the engine */
    NvOsMutexLock(s_SeCoreEngineMutex);
    NV_ASSERT(hSeAesHwCtx);
    NV_ASSERT(pSetIvInfo);

    if (hSeAesHwCtx->KeySlotNum >= SeAesHwKeySlot_NumExt)
    {
        e = NvError_NotSupported;
        goto fail;
    }
    if (!pSetIvInfo->pIV)
    {
        e = NvError_BadParameter;
        goto fail;
    }
    switch (hSeAesHwCtx->OpMode)
    {
        case NvDdkSeAesOperationalMode_Cbc:
        case NvDdkSeAesOperationalMode_Ecb:
            break;
        default:
            e = NvError_NotSupported;
            goto fail;
    }
    if (pSetIvInfo->VectorSize < NvDdkSeAesConst_IVLengthBytes)
    {
        e = NvError_BadParameter;
        goto fail;
    }

    NvOsMemcpy(hSeAesHwCtx->IV, pSetIvInfo->pIV, NvDdkSeAesConst_IVLengthBytes);
    /* Update the Iv in the current key slot */
    gs_pSeCoreCaps->pHwIf->pfNvDdkSeAesSetIv(hSeAesHwCtx);
fail:
    NvOsMutexUnlock(s_SeCoreEngineMutex);
    return e;
}

/**
 * Verification Algorithams for Clear SBK and lock SSK
 */
static NvBool DataCompare(NvU8 *SrcBuffer, NvU8 *DstBuffer, NvU32 Size)
{
    NvU32 i = 0;

    while ((i < Size) && (SrcBuffer[i] == DstBuffer[i]))
    {
        i++;
    }
    if (i == Size)
        return NV_FALSE;
    else
        return NV_TRUE;
}

/* Verify SBK clear */
static NvError VerifySBKClear(NvDdkSeAesHWCtx *hSeAesHwCtx)
{
    NvError e = NvSuccess;
    NvDdkSeAesHWCtx *TmpAesCtx = NULL;
    NvDdkSeAesOperation OpInfo;
    NvDdkSeAesSetIvInfo SetIvInfo;
    NvDdkSeAesKeyInfo   KeyInfo;
    NvDdkSeAesProcessBufferInfo PbInfo;
    NvU8 ZeroVec[16];
    NvU8 PlainText[16] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    NvU8 ExpCipherText[16] = {
         0x7A, 0xCA, 0x0F, 0xD9, 0xBC, 0xD6, 0xEC, 0x7C,
         0x9F, 0x97, 0x46, 0x66, 0x16, 0xE6, 0xA2, 0x82
    };
    NvU8 ObtainedCiphertext[16];

    NV_ASSERT(hSeAesHwCtx);
    TmpAesCtx = NvOsAlloc(sizeof(NvDdkSeAesHWCtx));
    if (TmpAesCtx == NULL)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }
    /* Take a backup of current AesContext */
    NvOsMemcpy(TmpAesCtx, hSeAesHwCtx, sizeof(NvDdkSeAesHWCtx));
    /* Zero the Zero vector */
    NvOsMemset(ZeroVec, 0x0, 16);

    /* Select Operation */
    OpInfo.OpMode = NvDdkSeAesOperationalMode_Cbc;
    OpInfo.IsEncrypt = NV_TRUE;
    NV_CHECK_ERROR_CLEANUP(NvDdkSeAesSelectOperation(hSeAesHwCtx,
                (const NvDdkSeAesOperation *)&OpInfo));
    /* Select Iv */
    SetIvInfo.pIV = ZeroVec;
    SetIvInfo.VectorSize = NvDdkSeAesConst_IVLengthBytes;
    NV_CHECK_ERROR_CLEANUP(NvDdkSeAesSetIv(hSeAesHwCtx,
                (const NvDdkSeAesSetIvInfo *)&SetIvInfo));
    /* Select the key */
    KeyInfo.KeyType = NvDdkSeAesKeyType_SecureBootKey;
    KeyInfo.KeyLength = NvDdkSeAesKeySize_128Bit;
    NV_CHECK_ERROR_CLEANUP(NvDdkSeAesSelectKey(hSeAesHwCtx,
                (const NvDdkSeAesKeyInfo *)&KeyInfo));
    /* Process buffer */
    PbInfo.pSrcBuffer = PlainText;
    PbInfo.pDstBuffer = ObtainedCiphertext;
    PbInfo.SrcBufSize = 16;
    NV_CHECK_ERROR_CLEANUP(NvDdkSeAesProcessBuffer(hSeAesHwCtx,
                (const NvDdkSeAesProcessBufferInfo *)&PbInfo));

    if (DataCompare(ExpCipherText, ObtainedCiphertext, 16))
    {
        e = NvError_BadValue;
        goto fail;
    }
fail:
    /* Copy back the previous Aes context */
    NvOsMemcpy(hSeAesHwCtx, TmpAesCtx, sizeof(NvDdkSeAesHWCtx));
    return e;
}

static NvError VerifySskLock(NvDdkSeAesHWCtx *hSeAesHwCtx)
{
    NvError e = NvSuccess;
    NvU32 i = 0;
    NvDdkSeAesHWCtx *TmpAesCtx = NULL;
    NvDdkSeAesOperation OpInfo;
    NvDdkSeAesSetIvInfo SetIvInfo;
    NvDdkSeAesKeyInfo   KeyInfo;
    NvDdkSeAesProcessBufferInfo PbInfo;
    NvU8 ZeroVec[16], Key[16], SrcBuffer[16];
    NvU8 Buffer1[16], Buffer2[16];

    NV_ASSERT(hSeAesHwCtx);
    TmpAesCtx = NvOsAlloc(sizeof(NvDdkSeAesHWCtx));
    if (TmpAesCtx == NULL)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }
    /* Take a backup of current AesContext */
    NvOsMemcpy(TmpAesCtx, hSeAesHwCtx, sizeof(NvDdkSeAesHWCtx));

    /* Zero the Zero vector */
    NvOsMemset(ZeroVec, 0x0, 16);
    /* Zero out the key */
    NvOsMemset(Key, 0x0, 16);

    /* Fill the source buffer */
    for (i = 0; i < 16; i++)
        SrcBuffer[i] = i;

    /* Select Operation */
    OpInfo.OpMode = NvDdkSeAesOperationalMode_Cbc;
    OpInfo.IsEncrypt = NV_TRUE;
    NV_CHECK_ERROR_CLEANUP(NvDdkSeAesSelectOperation(hSeAesHwCtx,
                (const NvDdkSeAesOperation *)&OpInfo));
    /* Set key */
    KeyInfo.KeyType =   NvDdkSeAesKeyType_SecureStorageKey;
    KeyInfo.KeyLength = NvDdkSeAesKeySize_128Bit;
    NV_CHECK_ERROR_CLEANUP(NvDdkSeAesSelectKey(hSeAesHwCtx,
                (const NvDdkSeAesKeyInfo *)&KeyInfo));
    /* Set Iv */
    SetIvInfo.pIV = ZeroVec;
    SetIvInfo.VectorSize = NvDdkSeAesConst_IVLengthBytes;
    NV_CHECK_ERROR_CLEANUP(NvDdkSeAesSetIv(hSeAesHwCtx,
                (const NvDdkSeAesSetIvInfo *)&SetIvInfo));

    /* process buffer */
    PbInfo.pSrcBuffer = SrcBuffer;
    PbInfo.pDstBuffer = Buffer1;
    PbInfo.SrcBufSize = 16;
    NV_CHECK_ERROR_CLEANUP(NvDdkSeAesProcessBuffer(hSeAesHwCtx,
                (const NvDdkSeAesProcessBufferInfo *)&PbInfo));

    /* Get the Output with Zero key And Zero Iv*/
    /* Select IV */
    SetIvInfo.pIV = ZeroVec;
    SetIvInfo.VectorSize = NvDdkSeAesConst_IVLengthBytes;
    NV_CHECK_ERROR_CLEANUP(NvDdkSeAesSetIv(hSeAesHwCtx,
                (const NvDdkSeAesSetIvInfo *)&SetIvInfo));
    /* Set Key */
    KeyInfo.KeyType =   NvDdkSeAesKeyType_UserSpecified;
    KeyInfo.KeyLength = NvDdkSeAesKeySize_128Bit;
    NvOsMemcpy(KeyInfo.Key, ZeroVec, NvDdkSeAesKeySize_128Bit);
    NV_CHECK_ERROR_CLEANUP(NvDdkSeAesSelectKey(hSeAesHwCtx,
                (const NvDdkSeAesKeyInfo *)&KeyInfo));
    /* Process buffer */
    PbInfo.pDstBuffer = Buffer2;
    NV_CHECK_ERROR_CLEANUP(NvDdkSeAesProcessBuffer(hSeAesHwCtx,
                (const NvDdkSeAesProcessBufferInfo *)&PbInfo));
    if (!DataCompare(Buffer1, Buffer2, 16))
    {
        /* change the key */
        for (i = 0; i < NvDdkSeAesKeySize_128Bit; i++)
        {
            Key[i] = ~Key[i];
        }
    }
    /* Select IV */
    SetIvInfo.pIV = ZeroVec;
    SetIvInfo.VectorSize = NvDdkSeAesConst_IVLengthBytes;
    NV_CHECK_ERROR_CLEANUP(NvDdkSeAesSetIv(hSeAesHwCtx,
                (const NvDdkSeAesSetIvInfo *)&SetIvInfo));
    /* Select Ssk keyslot */
    KeyInfo.KeyType =   NvDdkSeAesKeyType_SecureStorageKey;
    NV_CHECK_ERROR_CLEANUP(NvDdkSeAesSelectKey(hSeAesHwCtx,
                (const NvDdkSeAesKeyInfo *)&KeyInfo));
    /* Set Ssk into the key slot */
    NvOsMemcpy(hSeAesHwCtx->Key, Key, NvDdkSeAesKeySize_128Bit);
    NV_CHECK_ERROR_CLEANUP(gs_pSeCoreCaps->pHwIf->pfNvDdkSeAesSetKey(hSeAesHwCtx));
    /* Process buffer */
    PbInfo.pDstBuffer = Buffer2;
    NV_CHECK_ERROR_CLEANUP(NvDdkSeAesProcessBuffer(hSeAesHwCtx,
                (const NvDdkSeAesProcessBufferInfo *)&PbInfo));
    if (DataCompare(Buffer1, Buffer2, 16))
    {
        e = NvError_BadValue;
        goto fail;
    }
fail:
    /* Copy back the previous Aes context */
    NvOsMemcpy(hSeAesHwCtx, TmpAesCtx, sizeof(NvDdkSeAesHWCtx));
    return e;
}
