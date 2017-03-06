
#define NV_IDL_IS_STUB

/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvos.h"
#include "nvassert.h"
#include "nvidlcmd.h"
#include "nvrm_dma.h"

NvBool NvRmDmaIsDmaTransferCompletes(
    NvRmDmaHandle hDma, NvBool IsFirstHalfBuffer)
{
    return NV_FALSE;
}

NvError NvRmDmaGetTransferredCount(
    NvRmDmaHandle hDma,
    NvU32 *pTransferCount,
    NvBool IsTransferStop)
{
    return NvError_NotSupported;
}

void NvRmDmaAbort(NvRmDmaHandle hDma)
{
}

NvError NvRmDmaStartDmaTransfer(
    NvRmDmaHandle hDma,
    NvRmDmaClientBuffer *pClientBuffer,
    NvRmDmaDirection DmaDirection,
    NvU32 WaitTimeoutInMilliSecond,
    NvOsSemaphoreHandle AsynchSemaphoreId)
{
    return NvError_NotSupported;
}

void NvRmDmaFree(NvRmDmaHandle hDma)
{
}

NvError NvRmDmaAllocate(
    NvRmDeviceHandle hRmDevice,
    NvRmDmaHandle *phDma,
    NvBool Enable32bitSwap,
    NvRmDmaPriority Priority,
    NvRmDmaModuleID DmaRequestorModuleId,
    NvU32 DmaRequestorInstanceId)
{
    return NvError_NotSupported;
}

NvError NvRmDmaGetCapabilities(
    NvRmDeviceHandle hDevice,
    NvRmDmaCapabilities *pRmDmaCaps)
{
    return NvError_NotSupported;
}
