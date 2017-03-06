
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
#include "nvrm_i2c.h"

NvError NvRmI2cTransaction(
    NvRmI2cHandle hI2c,
    NvU32 I2cPinMap,
    NvU32 WaitTimeoutInMilliSeconds,
    NvU32 ClockSpeedKHz,
    NvU8 *Data,
    NvU32 DataLen,
    NvRmI2cTransactionInfo *Transaction,
    NvU32 NumOfTransactions)
{
    return NvSuccess;
}

void NvRmI2cClose(NvRmI2cHandle hI2c)
{
}

NvError NvRmI2cOpen(NvRmDeviceHandle hDevice,
    NvU32 IoModule,
    NvU32 instance,
    NvRmI2cHandle *phI2c)
{
    if (!phI2c)
        return NvError_BadParameter;

    *phI2c = (void *)0xdeadbeef;
    return NvSuccess;
}

NvError NvRmI2cSlaveOpen(NvRmDeviceHandle hDevice,
    NvU32 IoModule,
    NvU32 instance,
    NvRmI2cHandle *phI2c)
{
    if (!phI2c)
        return NvError_BadParameter;

    *phI2c = (void *)0xdeadbeef;
    return NvSuccess;
}
NvError
NvRmI2cSlaveStart(
    NvRmI2cHandle hI2c,
    NvU32 I2cPinMap,
    NvU32 ClockSpeedKHz,
    NvU32 Address,
    NvBool IsTenBitAdd,
    NvU32 maxRecvPacketSize,
    NvU8 DummyCharTx)
{
    return NvError_NotSupported;
}

void NvRmI2cSlaveStop(NvRmI2cHandle hI2c)
{

}

NvError
NvRmI2cSlaveRead(
    NvRmI2cHandle hI2c,
    NvU8 *pRxBuffer,
    NvU32 BytesRequested,
    NvU32 *pBytesRead,
    NvU8 MinBytesRead,
    NvU32 Timeoutms)
{
    return NvError_NotSupported;
}


NvError
NvRmI2cSlaveWriteAsynch(
    NvRmI2cHandle hI2c,
    NvU8 *pTxBuffer,
    NvU32 BytesRequested,
    NvU32 *pBytesWritten)
{
    return NvError_NotSupported;
}

NvError NvRmI2cSlaveGetWriteStatus(NvRmI2cHandle hI2c, NvU32 TimeoutMs, NvU32 *pBytesRemaining)
{
    return NvError_NotSupported;
}

void NvRmI2cSlaveFlushWriteBuffer(NvRmI2cHandle hI2c)
{
}

NvU32 NvRmI2cSlaveGetNACKCount(NvRmI2cHandle hI2c, NvBool IsResetCount)
{
    return 0;
}

