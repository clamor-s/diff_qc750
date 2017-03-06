/*
 * Copyright (c) 2006-2011 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvodm_imager_i2c.h"
#define I2CSPEED 100
#define I2C_WAIT_LIMIT 100 // milliseconds, or NV_WAIT_INFINITE

//------------
// I2C Profiling
//  When enabled, will give the amount of time between transactions.
//  Place  ODMI2CPROF("string") around each transaction to be measured.
//------------
NvU32 gs_LastTime = 0; // I2C Profiling
NvU32 gs_ThisTime = 0; // I2C Profiling
#define DEBUG_I2C_WRITES 0
#if DEBUG_I2C_WRITES
#define ODMI2CPROF(_str)  \
    do { \
        gs_ThisTime = NvOdmOsGetTimeMS() % 3600000; \
        NvOdmOsDebugPrintf("<%s> Elapsed time %d\n", _str, (gs_ThisTime - gs_LastTime)); \
        gs_LastTime = gs_ThisTime; \
    } while (0);
#else
#define ODMI2CPROF(_str) ;
#endif



NvOdmI2cStatus NvOdmImagerI2cAck(
    NvOdmImagerI2cConnection *i2cHandle)
{
    NvU8 WriteBuffer[1];
    NvOdmI2cStatus Result;
    NvOdmI2cTransactionInfo TransactionInfo;

    WriteBuffer[0] = 0x0;
    TransactionInfo.Address = i2cHandle->DeviceAddr;
    TransactionInfo.Buf = WriteBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 1;

    ODMI2CPROF("Ack start");
    Result = NvOdmI2cTransaction(i2cHandle->hOdmI2c,
                                 &TransactionInfo,
                                 1, I2CSPEED,
                                 I2C_WAIT_LIMIT);
    ODMI2CPROF("Ack done");
    return Result;
}

NvOdmI2cStatus NvOdmImagerI2cWrite8(
    NvOdmImagerI2cConnection *i2cHandle,
    NvU8 Addr,
    NvU8 Data)
{
    NvU8 WriteBuffer[2];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;

    WriteBuffer[0] = Addr & 0xFF;   // imager offset
    WriteBuffer[1] = Data & 0xFF;   // written data

    TransactionInfo.Address = i2cHandle->DeviceAddr;
    TransactionInfo.Buf = WriteBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 2;

    Error = NvOdmI2cTransaction(i2cHandle->hOdmI2c, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cWrite8: NvOdmI2cSend Failed (error code =0x%x)\n", Error);
        return Error;
    }
    return Error;
}

NvOdmI2cStatus NvOdmImagerI2cRead8(
    NvOdmImagerI2cConnection *i2cHandle,
    NvU8 Addr,
    NvU8 *Data)
{
    NvU8 ReadBuffer[1];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;


    // Write the imager offset
    ReadBuffer[0] = Addr & 0xFF;

    TransactionInfo.Address = i2cHandle->DeviceAddr;
    TransactionInfo.Buf = ReadBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 1;

    Error = NvOdmI2cTransaction(i2cHandle->hOdmI2c, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cRead8: NvOdmI2cSend Failed (error code = 0x%x)\n", Error);
        return Error;
    }

    NvOdmOsMemset(ReadBuffer, 0, sizeof(ReadBuffer));

    TransactionInfo.Address = (i2cHandle->DeviceAddr | 0x1);
    TransactionInfo.Buf = ReadBuffer;
    TransactionInfo.Flags = 0;
    TransactionInfo.NumBytes = 1;

    // Read data from imager at the specified offset
    Error = NvOdmI2cTransaction(i2cHandle->hOdmI2c, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cRead8: NvOdmI2cReceive Failed (error code = 0x%x)\n", Error);
        return Error;
    }
    *Data = ReadBuffer[0];
    return Error;
}

NvOdmI2cStatus NvOdmImagerI2cWriteBuffer16(
    NvOdmImagerI2cConnection *i2cHandle,
    NvU32 *WriteBufferAddrData,
    NvU32 Length)
{
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo *TransactionArray;
    NvU32 *Buffers;
    NvU32 i;

    TransactionArray = NvOdmOsAlloc(Length * sizeof(NvOdmI2cTransactionInfo));
    Buffers = NvOdmOsAlloc(Length * sizeof(NvU32));
    if ((TransactionArray == NULL) || (Buffers == NULL))
    {
        NvOdmOsFree(TransactionArray);
        NvOdmOsFree(Buffers);
        return NvOdmI2cStatus_InternalError;
    }

    for (i = 0; i < Length; i++)
    {
        NvU16 Addr, Data;
        NvU8 *WriteBuffer = (NvU8 *)(&Buffers[i]);
        Addr = (WriteBufferAddrData[i] >> 16) & 0xFFFF;
        Data = (WriteBufferAddrData[i]) & 0xFFFF;
        WriteBuffer[0] = (NvU8)((Addr >> 8) & 0xFF);
        WriteBuffer[1] = (NvU8)(Addr & 0xFF);
        WriteBuffer[2] = (NvU8)((Data >> 8) & 0xFF);
        WriteBuffer[3] = (NvU8)(Data & 0xFF);
        TransactionArray[i].Address = i2cHandle->DeviceAddr;
        TransactionArray[i].Buf = WriteBuffer;
        TransactionArray[i].Flags = NVODM_I2C_IS_WRITE;
        TransactionArray[i].NumBytes = 4;
#if DEBUG_I2C_WRITES
        NvOdmOsDebugPrintf("i2c write(%x): <0x%.4x> = 0x%.4x\n",
                           i2cHandle->DeviceAddr, Addr, Data);
#endif
    }

    Error = NvOdmI2cTransaction(i2cHandle->hOdmI2c, TransactionArray, Length,
                                I2CSPEED, NV_WAIT_INFINITE);

    NvOdmOsFree(TransactionArray);
    NvOdmOsFree(Buffers);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cWriteBuffer: NvOdmI2cSend Failed\n");
        return Error;
    }
    return Error;
}

NvOdmI2cStatus NvOdmImagerI2cRead8_w16(
    NvOdmImagerI2cConnection *i2cHandle,
    NvU16 Addr,
    NvU16 *Data)
{
    NvU8 ReadBuffer[2];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;


    // Write the imager offset
    ReadBuffer[0] = (Addr >> 8) & 0xFF;
    ReadBuffer[1] = Addr & 0xFF;

    TransactionInfo.Address = i2cHandle->DeviceAddr;
    TransactionInfo.Buf = ReadBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 1;

    Error = NvOdmI2cTransaction(i2cHandle->hOdmI2c, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cRead8: NvOdmI2cSend Failed (error code = 0x%x)\n", Error);
        return Error;
    }

    NvOdmOsMemset(ReadBuffer, 0, sizeof(ReadBuffer));

    TransactionInfo.Address = (i2cHandle->DeviceAddr | 0x1);
    TransactionInfo.Buf = ReadBuffer;
    TransactionInfo.Flags = 0;
    TransactionInfo.NumBytes = 1;

    // Read data from imager at the specified offset
    Error = NvOdmI2cTransaction(i2cHandle->hOdmI2c, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cRead8: NvOdmI2cReceive Failed (error code = 0x%x)\n", Error);
        return Error;
    }
    *Data = (NvU16)(ReadBuffer[0]&0xff);
    return Error;
}

NvOdmI2cStatus NvOdmImagerI2cWrite16(
    NvOdmImagerI2cConnection *i2cHandle,
    NvU16 Addr,
    NvU16 Data)
{
    NvU8 WriteBuffer[4];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;

    WriteBuffer[0] = (NvU8)((Addr >> 8) & 0xFF);// high byte imager offset
    WriteBuffer[1] = (NvU8)(Addr & 0xFF);       // low byte imager offset
    WriteBuffer[2] = (NvU8)((Data >> 8) & 0xFF);
    WriteBuffer[3] = (NvU8)(Data & 0xFF);

    TransactionInfo.Address = i2cHandle->DeviceAddr;
    TransactionInfo.Buf = WriteBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 4;

#if DEBUG_I2C_WRITES
    NvOdmOsDebugPrintf("i2c write(%x): <0x%.4x> = 0x%.4x\n",
                       i2cHandle->DeviceAddr, Addr, Data);
#endif

    Error = NvOdmI2cTransaction(i2cHandle->hOdmI2c, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cWrite16: NvOdmI2cSend Failed (error code = 0x%x)\n", Error);
        return Error;
    }
    return Error;
}

NvOdmI2cStatus NvOdmImagerI2cWriteDev16(
    NvOdmImagerI2cConnection *i2cHandle,
    NvU16 Data)
{
    NvU8 WriteBuffer[4];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;

    WriteBuffer[0] = (NvU8)((Data >> 8) & 0xFF);
    WriteBuffer[1] = (NvU8)(Data & 0xFF);

    TransactionInfo.Address = i2cHandle->DeviceAddr;
    TransactionInfo.Buf = WriteBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 2;

    Error = NvOdmI2cTransaction(i2cHandle->hOdmI2c, &TransactionInfo, 1, I2CSPEED,
                                        NV_WAIT_INFINITE);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cWriteDev16: NvOdmI2cSend Failed (error code = 0x%x)\n");
    }
    return Error;
}

NvOdmI2cStatus NvOdmImagerI2cWrite16Data8(
    NvOdmImagerI2cConnection *i2cHandle,
    NvU16 Addr,
    NvU8 Data)
{
    NvU8 WriteBuffer[3];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;

    WriteBuffer[0] = (NvU8)((Addr >> 8) & 0xFF);// high byte imager offset
    WriteBuffer[1] = (NvU8)(Addr & 0xFF);       // low byte imager offset
    WriteBuffer[2] = (NvU8)(Data  & 0xFF);

    TransactionInfo.Address = i2cHandle->DeviceAddr;
    TransactionInfo.Buf = WriteBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 3;

    Error = NvOdmI2cTransaction(i2cHandle->hOdmI2c, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cWrite16Data8: NvOdmI2cSend Failed (error = 0x%x)\n", Error);
        return Error;
    }
    return Error;
}

NvOdmI2cStatus NvOdmImagerI2cWrite16Data8_W16(
    NvOdmImagerI2cConnection *i2cHandle,
    NvU16 Addr,
    NvU16 Data)
{
    NvU8 WriteBuffer[3];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;

    WriteBuffer[0] = (NvU8)((Addr >> 8) & 0xFF);// high byte imager offset
    WriteBuffer[1] = (NvU8)(Addr & 0xFF);       // low byte imager offset
    WriteBuffer[2] = (NvU8)(Data  & 0xFF);

    TransactionInfo.Address = i2cHandle->DeviceAddr;
    TransactionInfo.Buf = WriteBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 3;

    Error = NvOdmI2cTransaction(i2cHandle->hOdmI2c, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cWrite16Data8: NvOdmI2cSend Failed\n");
        return Error;
    }
    return Error;
}


NvOdmI2cStatus NvOdmImagerI2cRead16(
   NvOdmImagerI2cConnection *i2cHandle,
   NvU16 Addr,
   NvU16 *Data)
{
    NvU8 ReadBuffer[2];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;

    // Write the imager offset
    ReadBuffer[0] = (Addr >> 8) & 0xFF;
    ReadBuffer[1] = Addr & 0xFF;

    TransactionInfo.Address = i2cHandle->DeviceAddr;
    TransactionInfo.Buf = ReadBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 2;

    Error = NvOdmI2cTransaction(i2cHandle->hOdmI2c, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cRead16: NvOdmI2cSend Failed (error code = 0x%x)\n", Error);
        return Error;
    }

    NvOdmOsMemset(ReadBuffer, 0, sizeof(ReadBuffer));
    // Read data from imager at the specified offset
    TransactionInfo.Address = (i2cHandle->DeviceAddr | 0x1);
    TransactionInfo.Buf = ReadBuffer;
    TransactionInfo.Flags = 0;
    TransactionInfo.NumBytes = 2;

    Error = NvOdmI2cTransaction(i2cHandle->hOdmI2c, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cRead16: NvOdmI2cReceive Failed (error code = 0x%x)\n", Error);
        return Error;
    }
    *Data = (ReadBuffer[1]) | (ReadBuffer[0] << 8);
    return Error;
}

NvOdmI2cStatus NvOdmImagerI2cReadDev16(
   NvOdmImagerI2cConnection *i2cHandle,
   NvU16 *Data)
{
    NvU8 ReadBuffer[2];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;

    NvOdmOsMemset(ReadBuffer, 0, sizeof(ReadBuffer));
    // Read data from imager at the specified offset
    TransactionInfo.Address = (i2cHandle->DeviceAddr | 0x1);
    TransactionInfo.Buf = ReadBuffer;
    TransactionInfo.Flags = 0;
    TransactionInfo.NumBytes = 2;

    Error = NvOdmI2cTransaction(i2cHandle->hOdmI2c, &TransactionInfo, 1, I2CSPEED,
                                        NV_WAIT_INFINITE);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cReadDev16: NvOdmI2cReceive Failed (error code = 0x%x)\n", Error);
        return Error;
    }
    *Data = (ReadBuffer[1]) | (ReadBuffer[0] << 8);
    return Error;
}

NvOdmI2cStatus NvOdmImagerI2cRead16Data8(
   NvOdmImagerI2cConnection *i2cHandle,
   NvU16 Addr,
   NvU8 *Data)
{
    NvU8 ReadBuffer[2];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;

    // Write the imager offset
    ReadBuffer[0] = (Addr >> 8) & 0xFF;
    ReadBuffer[1] = Addr & 0xFF;

    TransactionInfo.Address = i2cHandle->DeviceAddr;
    TransactionInfo.Buf = ReadBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 2;

    Error = NvOdmI2cTransaction(i2cHandle->hOdmI2c, &TransactionInfo, 1, I2CSPEED,
                                        NV_WAIT_INFINITE);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cRead16Data8: NvOdmI2cSend Failed (error code = 0x%x)\n", Error);
        return Error;
    }

    NvOdmOsMemset(ReadBuffer, 0, sizeof(ReadBuffer));
    // Read data from imager at the specified offset
    TransactionInfo.Address = (i2cHandle->DeviceAddr | 0x1);
    TransactionInfo.Buf = ReadBuffer;
    TransactionInfo.Flags = 0;
    TransactionInfo.NumBytes = 1;

    Error = NvOdmI2cTransaction(i2cHandle->hOdmI2c, &TransactionInfo, 1, I2CSPEED,
                                        NV_WAIT_INFINITE);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cRead16Data8: NvOdmI2cReceive Failed (error code = 0x%x)\n", Error);
        return Error;
    }
    *Data = ReadBuffer[0];
    return Error;
}


NvOdmI2cStatus NvOdmImagerI2cWrite32(
    NvOdmImagerI2cConnection *i2cHandle,
    NvU16 Addr,
    NvU32 Data)
{
    NvU8 WriteBuffer[6];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;

    WriteBuffer[0] = (NvU8)(Addr & 0xFF);       // low byte imager offset
    WriteBuffer[1] = (NvU8)((Addr >> 8) & 0xFF);// high byte imager offset
    WriteBuffer[2] = (NvU8)(Data & 0xFF);
    WriteBuffer[3] = (NvU8)((Data >> 8) & 0xFF);
    WriteBuffer[4] = (NvU8)((Data >> 16) & 0xFF);
    WriteBuffer[5] = (NvU8)((Data >> 24) & 0xFF);

    TransactionInfo.Address = i2cHandle->DeviceAddr;
    TransactionInfo.Buf = WriteBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 6;

    Error = NvOdmI2cTransaction(i2cHandle->hOdmI2c, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cWrite32: NvOdmI2cSend Failed (error code = 0x%x)\n", Error);
        return Error;
    }
    return Error;
}

NvOdmI2cStatus NvOdmImagerI2cRead32(
   NvOdmImagerI2cConnection *i2cHandle,
   NvU16 Addr,
   NvU32 *Data)
{
    NvU8 ReadBuffer[4];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;

    // Write the imager offset
    ReadBuffer[0] = Addr & 0xFF;
    ReadBuffer[1] = (Addr >> 8) & 0xFF;

    TransactionInfo.Address = i2cHandle->DeviceAddr;
    TransactionInfo.Buf = ReadBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 2;

    Error = NvOdmI2cTransaction(i2cHandle->hOdmI2c, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cRead32: NvOdmI2cSend Failed (error code = 0x%x)\n", Error);
        return Error;
    }

    NvOdmOsMemset(ReadBuffer, 0, sizeof(ReadBuffer));
    // Read data from imager at the specified offset
    TransactionInfo.Address = (i2cHandle->DeviceAddr | 0x1);
    TransactionInfo.Buf = ReadBuffer;
    TransactionInfo.Flags = 0;
    TransactionInfo.NumBytes = 4;

    Error = NvOdmI2cTransaction(i2cHandle->hOdmI2c, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cRead32: NvOdmI2cReceive Failed (error code = 0x%x)\n", Error);
        return Error;
    }
    *Data = (ReadBuffer[3] << 24) | (ReadBuffer[2] << 16) |
            (ReadBuffer[1] << 8) | ReadBuffer[0];
    return Error;
}

void NvOdmImagerI2cGetRegisterDump(
    NvOdmImagerI2cConnection *i2cHandle,
    NvU32 *pAddresses,
    NvU32 *pValues,
    NvU32 Count)
{
    NvU32 i = 0;
    for (i = 0; i < Count; i++) {
        NvOdmImagerI2cRead8(i2cHandle, (NvU8)pAddresses[i], (NvU8*)&pValues[i]);
    }
}

