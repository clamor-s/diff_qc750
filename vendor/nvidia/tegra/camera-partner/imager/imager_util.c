/*
 * Copyright (c) 2008-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */


#include "imager_util.h"
#include "nvodm_query_gpio.h"
#include "nvodm_query_discovery_imager.h"

#define DEBUG_PRINTS 0

#define I2C_WAIT_LIMIT (100)
#define I2CSPEED (100)
#define I2C_POWERUP_RETRIES (50)
#define I2C_POWERUP_RETRY_WAIT_MS (5)

static void AssignGpioPurpose(
    ImagerGpio *pOutGpios,
    NvU32 PinMapping,
    ImagerGpio NewGpio);

static NvU32 BuildBuffer(NvU8 *pBuf, NvU8 Width, NvU32 Value)
{
    NvU32 count = 0;
    switch (Width)
    {
        case 0:
            // possibly no address, some focusers use this
            break;

        // cascading switch
        case 32:
            pBuf[count++] = (NvU8)((Value>>24) & 0xFF);
        case 24:
            pBuf[count++] = (NvU8)((Value>>16) & 0xFF);
        case 16:
            pBuf[count++] = (NvU8)((Value>>8) & 0xFF);
        case 8:
            pBuf[count++] = (NvU8)(Value & 0xFF);
            break;

        default:
            NvOsDebugPrintf("Unsupported Bit Width %d\n", Width);
            break;
    }
    return count;
}

static NvU32 RebuildValue(NvU8 *pBuf, NvU8 Width)
{
    NvU32 Value = 0;
    NvU32 count = 0;
    switch (Width)
    {
        case 0:
            break;
        case 32:
            Value |= pBuf[count++] << 24;
        case 24:
            Value |= pBuf[count++] << 16;
        case 16:
            Value |= pBuf[count++] << 8;
        case 8:
            Value |= pBuf[count++];
            break;
    }
    return Value;
}

NvBool
WriteI2CSequence(
    NvOdmImagerI2cConnection *pI2c,
    DevCtrlReg16 *pSequence,
    NvBool FastSetMode)
{
    return WriteI2CSequenceOverride(pI2c, pSequence, FastSetMode, NULL, 0);
}

NvBool
WriteI2CSequenceOverride(
    NvOdmImagerI2cConnection *pI2c,
    DevCtrlReg16 *pSequence,
    NvBool FastSetMode,
    DevCtrlReg16 *pOverrides,
    NvU32 OverrideSize)
{
    DevCtrlReg16 *ptr = pSequence;
    NvOdmI2cStatus Status;
    NvBool Success = NV_TRUE;
    NvBool FastSetModeRegs = NV_FALSE;

#define WRITE_BUFFER_SIZE 32
    NvU8 *WriteBuffer;
    NvOdmI2cTransactionInfo *TransactionArray;
    NvU32 BufferIndex = 0;
    NvU8 BytesPerAccess = 0;

    if (!pI2c || !pI2c->hOdmI2c || !pSequence)
        return NV_FALSE;

    BytesPerAccess = (pI2c->AddressBitWidth + pI2c->DataBitWidth) / 8;

    // Malloc buffers
    WriteBuffer = NvOdmOsAlloc(WRITE_BUFFER_SIZE * BytesPerAccess);
    TransactionArray = NvOdmOsAlloc(WRITE_BUFFER_SIZE *
                                    sizeof(NvOdmI2cTransactionInfo));
    if ((TransactionArray == NULL) || (WriteBuffer == NULL))
    {
        NvOdmOsFree(TransactionArray);
        NvOdmOsFree(WriteBuffer);
        return NV_FALSE;
    }

    while (ptr->RegAddr != SEQUENCE_END)
    {
        switch (ptr->RegAddr)
        {
            case SEQUENCE_WAIT_MS:

                // Flush any accumulated transactions before sleeping
                if (BufferIndex > 0)
                {
                    Status = NvOdmI2cTransaction(pI2c->hOdmI2c,
                            TransactionArray, BufferIndex,
                            I2CSPEED, I2C_WAIT_LIMIT);
                    BufferIndex = 0;
                    if (Status != NvOdmI2cStatus_Success)
                        Success = NV_FALSE;
                }
                NvOdmOsWaitUS(ptr->RegValue * 1000);
                break;

            case SEQUENCE_FAST_SETMODE_START:
                FastSetModeRegs = NV_TRUE;
                break;

            case SEQUENCE_FAST_SETMODE_END:
                FastSetModeRegs = NV_FALSE;
                break;

            default:
                if (!FastSetMode || FastSetModeRegs)
                {
                    NvU32 DataSize = 0;
                    NvU32 Offset = BufferIndex*BytesPerAccess;
                    NvU16 OverrideValue = ptr->RegValue;
                    NvU32 i = 0;

                    if (pOverrides)
                    {
                        // search for the override list
                        for (i = 0; i < OverrideSize; i++)
                        {
                            if (ptr->RegAddr == pOverrides[i].RegAddr)
                            {
                                OverrideValue = pOverrides[i].RegValue;
                            }
                        }
                    }

                    // Stuff transaction into buffers
                    DataSize += BuildBuffer(&WriteBuffer[DataSize+Offset],
                                               pI2c->AddressBitWidth,
                                               ptr->RegAddr);
                    DataSize += BuildBuffer(&WriteBuffer[DataSize+Offset],
                                               pI2c->DataBitWidth,
                                               OverrideValue);

                    TransactionArray[BufferIndex].Address = pI2c->DeviceAddr;
                    TransactionArray[BufferIndex].Buf = &(WriteBuffer[Offset]);
                    TransactionArray[BufferIndex].Flags = NVODM_I2C_IS_WRITE;
                    TransactionArray[BufferIndex].NumBytes = DataSize;

                    BufferIndex++;

                    // Flush any accumulated transactions if buffer full
                    if (BufferIndex >= WRITE_BUFFER_SIZE)
                    {
                        Status = NvOdmI2cTransaction(pI2c->hOdmI2c,
                            TransactionArray, BufferIndex,
                            I2CSPEED, I2C_WAIT_LIMIT);
                        BufferIndex = 0;
                        if (Status != NvOdmI2cStatus_Success)
                            Success = NV_FALSE;
                    }
                }
        }

        ptr += 1;
    }

    // Flush any accumulated transactions because we're done!
    if (BufferIndex > 0)
    {
        Status = NvOdmI2cTransaction(pI2c->hOdmI2c,
            TransactionArray, BufferIndex,
            I2CSPEED, I2C_WAIT_LIMIT);
        BufferIndex = 0;
        if (Status != NvOdmI2cStatus_Success)
            Success = NV_FALSE;
    }

    // Don't forget to free the mallocs!
    NvOdmOsFree(TransactionArray);
    NvOdmOsFree(WriteBuffer);
    return Success;
}

NvOdmI2cStatus
NvOdmImagerI2cWrite(
   NvOdmImagerI2cConnection *pI2c,
   NvU32 Addr,
   NvU32 Data)
{
    NvU8 WriteBuffer[NVODM_IMAGER_I2C_MAX_BUFFER];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;
    NvU32 BufferSize = 0;

    BufferSize += BuildBuffer(&WriteBuffer[BufferSize],
                              pI2c->AddressBitWidth,
                              Addr);
    BufferSize += BuildBuffer(&WriteBuffer[BufferSize],
                              pI2c->DataBitWidth,
                              Data);

    TransactionInfo.Address = pI2c->DeviceAddr;
    TransactionInfo.Buf = WriteBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = BufferSize;

#if DEBUG_I2C_WRITES
    {
        NvU32 i;
        NvOsDebugPrintf("Write: ");
        for (i = 0; i < BufferSize; i++)
            NvOsDebugPrintf("0x%x ", WriteBuffer[i]);
        NvOsDebugPrintf("\n");
    }
#endif

    Error = NvOdmI2cTransaction(pI2c->hOdmI2c, &TransactionInfo, 1, I2CSPEED,
                                I2C_WAIT_LIMIT);

    if (Error != NvOdmI2cStatus_Success)
    {
        NvOdmOsDebugPrintf("NvOdmI2cTransaction Failed (error code = 0x%x)\n",
            Error);
    }

    return Error;
}

NvOdmI2cStatus NvOdmImagerI2cRead(
   NvOdmImagerI2cConnection *pI2c,
   NvU32 Addr,
   void *pData)
{
    NvU8 ReadBuffer[NVODM_IMAGER_I2C_MAX_BUFFER];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;
    NvU32 BufferSize = 0;

    // Write the offset
    BufferSize += BuildBuffer(&ReadBuffer[BufferSize],
                              pI2c->AddressBitWidth,
                              Addr);

    TransactionInfo.Address = pI2c->DeviceAddr;
    TransactionInfo.Buf = ReadBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = BufferSize;

    Error = NvOdmI2cTransaction(pI2c->hOdmI2c, &TransactionInfo, 1,
                                I2CSPEED, I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success)
    {
        NvOdmOsDebugPrintf("NvOdmImagerI2cRead: NvOdmI2cSend Failed (error code = 0x%x)\n", Error);
        return Error;
    }

    NvOdmOsMemset(ReadBuffer, 0, sizeof(ReadBuffer));
    // Read data from imager at the specified offset
    TransactionInfo.Address = (pI2c->DeviceAddr | 0x1);
    TransactionInfo.Buf = ReadBuffer;
    TransactionInfo.Flags = 0;
    TransactionInfo.NumBytes = pI2c->DataBitWidth / 8;

    Error = NvOdmI2cTransaction(pI2c->hOdmI2c, &TransactionInfo, 1,
                                I2CSPEED, I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NvOdmOsDebugPrintf("NvOdmImagerI2cRead: NvOdmI2cReceive Failed (error code = 0x%x)\n", Error);
        return Error;
    }
    *(NvU32 *)pData = RebuildValue(ReadBuffer, pI2c->DataBitWidth);
    return Error;

}

static NvOdmI2cStatus
NvOdmImagerI2cAck(NvOdmImagerI2cConnection *pI2c)
{
    NvU8 WriteBuffer[1];
    NvOdmI2cStatus Result;
    NvOdmI2cTransactionInfo TransactionInfo;

    WriteBuffer[0] = 0x0;
    TransactionInfo.Address = pI2c->DeviceAddr;
    TransactionInfo.Buf = WriteBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 1;

    Result = NvOdmI2cTransaction(pI2c->hOdmI2c,
                                 &TransactionInfo,
                                 1, I2CSPEED,
                                 I2C_WAIT_LIMIT);
    return Result;
}


static void AssignGpioPurpose(
    ImagerGpio *pOutGpios,
    NvU32 PinMapping,
    ImagerGpio NewGpio)
{
    NewGpio.Enable = NV_TRUE;
    switch (PinMapping)
    {
        case NVODM_IMAGER_FIELD(NVODM_IMAGER_RESET):
            NewGpio.ActiveHigh = NV_TRUE;
            pOutGpios[NvOdmImagerGpio_Reset] = NewGpio;
            break;
        case NVODM_IMAGER_FIELD(NVODM_IMAGER_RESET_AL):
            NewGpio.ActiveHigh = NV_FALSE;
            pOutGpios[NvOdmImagerGpio_Reset] = NewGpio;
            break;
        case NVODM_IMAGER_FIELD(NVODM_IMAGER_POWERDOWN):
            NewGpio.ActiveHigh = NV_TRUE;
            pOutGpios[NvOdmImagerGpio_Powerdown] = NewGpio;
            break;
        case NVODM_IMAGER_FIELD(NVODM_IMAGER_POWERDOWN_AL):
            NewGpio.ActiveHigh = NV_FALSE;
            pOutGpios[NvOdmImagerGpio_Powerdown] = NewGpio;
            break;
        case NVODM_IMAGER_FIELD(NVODM_IMAGER_MCLK):
            if (NewGpio.Pin == 0)
                pOutGpios[NvOdmImagerGpio_MCLK] = NewGpio;
            else if (NewGpio.Pin == 6)
                pOutGpios[NvOdmImagerGpio_PWM] = NewGpio;
            else
                NV_ASSERT(!"Unknown GPIO config");
            break;
        case NVODM_IMAGER_FIELD(NVODM_IMAGER_UNUSED):
            break;
        default:
            NV_ASSERT(!"Undefined gpio");
    }
}


/**
 * SetPowerLevelWithPeripheralConnectivity
 *
 * If PowerLevel == NvOdmImagerPowerLevel_On, this function parses nvodm
 * peripheral connectivity information in pConnections and stores the gpio
 * config in pGpioConfig which can be used later for powering down.
 * If PowerLevel == NvOdmImagerPowerLevel_Off, this function uses the gpio
 * config in pGpioConfig to power down and releases the resources allocated
 * during powering on.
 * If PowerLevel == NvOdmImagerPowerLevel_Standby, this function uses the
 * gpio config in pGpioConfig for standby.
 *
 * @param pConnections NvOdmPeripheralConnectivity
 * @param pI2c a pointer to NvOdmImagerI2cConnection. This struct will be
 *             initialized while powering on. For standby mode, it's used
 *             as an input parameter. While powering off, this struct will
 *             be filled with 0.
 * @param pGpioConfig a pointer to ImagerGpioConfig. This struct will be
 *             initialized while powering on. For standby mode, it's used
 *             as an input parameter. While powering off, this struct will
 *             be filled with 0.
 * @param PowerLevel the power level.
 * @retval NV_TRUE on success
 *
 */

NvBool
SetPowerLevelWithPeripheralConnectivity(
    const NvOdmPeripheralConnectivity *pConnections,
    NvOdmImagerI2cConnection *pI2c,
    ImagerGpioConfig *pGpioConfig,
    NvOdmImagerPowerLevel PowerLevel)
{
    return SetPowerLevelWithPeripheralConnectivityHelper(pConnections, pI2c, NULL, pGpioConfig, PowerLevel);
}


NvBool
SetPowerLevelWithPeripheralConnectivityHelper(
    const NvOdmPeripheralConnectivity *pConnections,
    NvOdmImagerI2cConnection *pI2c,
    NvOdmImagerI2cConnection *pI2cPmu,
    ImagerGpioConfig *pGpioConfig,
    NvOdmImagerPowerLevel PowerLevel)
{
    NvOdmServicesPmuHandle hPmu = NULL;
    NvU32 i = 0;
#ifndef SET_KERNEL_PINMUX
    NvU32 NumI2cConfigs;
    const NvU32 *pI2cConfigs;
#endif
    NvOdmI2cStatus I2cResult;
    NvOdmImagerI2cConnection *pI2cRight = NULL;
    NvU32 I2CDeviceCount = 0;

    if (!pConnections || !pI2c || !pGpioConfig)
        return NV_FALSE;

    switch(PowerLevel)
    {
        case NvOdmImagerPowerLevel_On:

            NvOdmOsMemset(pGpioConfig, 0, sizeof(ImagerGpioConfig));

            hPmu = NvOdmServicesPmuOpen();
            if (!hPmu)
                return NV_FALSE;

            // start parsing the connectivity information
            for (i = 0; i < pConnections->NumAddress; i++)
            {
                switch(pConnections->AddressList[i].Interface)
                {
                    case NvOdmIoModule_Vdd:
                        {
                            NvOdmServicesPmuVddRailCapabilities Cap;
                            NvU32 Settle_us;
                            // address is the vdd rail id
                            NvOdmServicesPmuGetCapabilities(hPmu,
                               pConnections->AddressList[i].Address,
                               &Cap);
                            // set the rail voltage to the recommended
                            NvOdmServicesPmuSetVoltage(hPmu,
                               pConnections->AddressList[i].Address,
                               Cap.requestMilliVolts,
                               &Settle_us);
                            // wait for rail to settle
                            NvOdmOsWaitUS(Settle_us);
                        }
                        break;
                    case NvOdmIoModule_I2c_Pmu:
                        if (pI2cPmu)
                        {
                            pI2cPmu->DeviceAddr =
                                pConnections->AddressList[i].Address;
                            pI2cPmu->I2cPort =
                                pConnections->AddressList[i].Instance;
                         }
                        break;
                    case NvOdmIoModule_I2c:
                        if (I2CDeviceCount == 0)
                        {
                            pI2c->DeviceAddr =
                                pConnections->AddressList[i].Address;
                            pI2c->I2cPort =
                                pConnections->AddressList[i].Instance;
                            I2CDeviceCount ++;
                        }
                        else if ((I2CDeviceCount == 1) && (pI2cPmu))
                        {
                            pI2cRight = pI2cPmu;
                            pI2cRight->DeviceAddr =
                                pConnections->AddressList[i].Address;
                            pI2cRight->I2cPort =
                                pConnections->AddressList[i].Instance;
                            I2CDeviceCount ++;

                        }
                        break;
                    case NvOdmIoModule_Gpio:
                        {
                            NvU32 Address = pConnections->AddressList[i].Address;
                            if (NVODM_IMAGER_IS_SET(Address))
                            {
                                ImagerGpio NewGpio;

                                NvOdmOsMemset(&NewGpio, 0, sizeof(ImagerGpio));
                                pGpioConfig->PinMappings =
                                    NVODM_IMAGER_FIELD(Address);
                                NewGpio.Port =
                                    pConnections->AddressList[i].Instance;
                                NewGpio.Pin = NVODM_IMAGER_CLEAR(Address);
                                AssignGpioPurpose(pGpioConfig->Gpios,
                                                  pGpioConfig->PinMappings,
                                                  NewGpio);
                            }
                            else
                            {
                                // No purpose assigned for this GPIO
                            }
                        }
                        break;
                    case NvOdmIoModule_VideoInput:
                        {
                            // For future use, currently not used.
                            NvU32 Code;
                            pGpioConfig->PinMappings =
                                pConnections->AddressList[i].Address;
                            // Check the data connection
                            Code = ((pGpioConfig->PinMappings >>
                                     NVODM_CAMERA_DATA_PIN_SHIFT)
                                    & NVODM_CAMERA_DATA_PIN_MASK);
                            switch (Code)
                            {
                                case NVODM_CAMERA_PARALLEL_VD0_TO_VD9:
                                case NVODM_CAMERA_PARALLEL_VD0_TO_VD7:
                                case NVODM_CAMERA_SERIAL_CSI_D1A:
                                case NVODM_CAMERA_SERIAL_CSI_D2A:
                                case NVODM_CAMERA_SERIAL_CSI_D1A_D2A:
                                case NVODM_CAMERA_SERIAL_CSI_D1B:
                                default:
                                    break;
                            }
                        }
                        break;
                    case  NvOdmIoModule_ExternalClock:
                        // ignore external clock
                        break;
                    default:
                        NV_ASSERT(!"Unknown NvOdmIoAddress used");
                        break;
                }
            }

            NvOdmServicesPmuClose(hPmu);

            if (pGpioConfig->Gpios[NvOdmImagerGpio_Powerdown].Enable)
            {
                NvU32 Port =
                    pGpioConfig->Gpios[NvOdmImagerGpio_Powerdown].Port;
                NvU32 Pin =
                    pGpioConfig->Gpios[NvOdmImagerGpio_Powerdown].Pin;
                NvOdmGpioPinActiveState Level;
                Level = NvOdmGpioPinActiveState_Low;
                if (pGpioConfig->Gpios[NvOdmImagerGpio_Powerdown].ActiveHigh)
                {
                    Level = NvOdmGpioPinActiveState_High;
                }
                if (pGpioConfig->hGpio == NULL)
                    pGpioConfig->hGpio = NvOdmGpioOpen();
                if (pGpioConfig->PinsUsed >= MAX_GPIO_PINS)
                {
                    NV_ASSERT(
                        !"Not enough handles in the context for this pin");
                    return NV_FALSE;
                }
                pGpioConfig->hPin[pGpioConfig->PinsUsed] =
                    NvOdmGpioAcquirePinHandle(pGpioConfig->hGpio, Port, Pin);
                pGpioConfig->Gpios[NvOdmImagerGpio_Powerdown].HandleIndex = pGpioConfig->PinsUsed;
                NvOdmGpioConfig(pGpioConfig->hGpio,
                                pGpioConfig->hPin[pGpioConfig->PinsUsed],
                                NvOdmGpioPinMode_Output);

                NvOdmGpioSetState(pGpioConfig->hGpio,
                    pGpioConfig->hPin[pGpioConfig->PinsUsed], Level);
                pGpioConfig->PinsUsed++;
            }

            if (pGpioConfig->Gpios[NvOdmImagerGpio_Reset].Enable)
            {
                NvU32 Port = pGpioConfig->Gpios[NvOdmImagerGpio_Reset].Port;
                NvU32 Pin = pGpioConfig->Gpios[NvOdmImagerGpio_Reset].Pin;
                NvOdmGpioPinActiveState LevelA, LevelB;
                LevelA = NvOdmGpioPinActiveState_Low;
                LevelB = NvOdmGpioPinActiveState_High;
                if (!pGpioConfig->Gpios[NvOdmImagerGpio_Reset].ActiveHigh)
                {
                    LevelA = NvOdmGpioPinActiveState_High;
                    LevelB = NvOdmGpioPinActiveState_Low;
                }

                if (pGpioConfig->hGpio == NULL)
                    pGpioConfig->hGpio = NvOdmGpioOpen();
                if (pGpioConfig->PinsUsed >= MAX_GPIO_PINS)
                {
                    NV_ASSERT(
                        !"Not enough handles in the context for this pin");
                    return NV_FALSE;
                }
                pGpioConfig->hPin[pGpioConfig->PinsUsed] =
                    NvOdmGpioAcquirePinHandle(pGpioConfig->hGpio, Port, Pin);
                pGpioConfig->Gpios[NvOdmImagerGpio_Reset].HandleIndex =
                    pGpioConfig->PinsUsed;
                NvOdmGpioConfig(pGpioConfig->hGpio,
                                pGpioConfig->hPin[pGpioConfig->PinsUsed],
                                NvOdmGpioPinMode_Output);
                NvOdmGpioSetState(pGpioConfig->hGpio,
                    pGpioConfig->hPin[pGpioConfig->PinsUsed], LevelA);
                NvOdmOsWaitUS(1000); // wait 1ms
                NvOdmGpioSetState(pGpioConfig->hGpio,
                    pGpioConfig->hPin[pGpioConfig->PinsUsed], LevelB);
                NvOdmOsWaitUS(1000); // wait 1ms
                NvOdmGpioSetState(pGpioConfig->hGpio,
                    pGpioConfig->hPin[pGpioConfig->PinsUsed], LevelA);
                pGpioConfig->PinsUsed++;
            }

            // Setup I2c
#ifndef SET_KERNEL_PINMUX
            NvOdmQueryPinMux(NvOdmIoModule_I2c, &pI2cConfigs, &NumI2cConfigs);
            if (pI2cConfigs[pI2c->I2cPort]==NvOdmI2cPinMap_Multiplexed)
                pI2c->hOdmI2c = NvOdmI2cPinMuxOpen(NvOdmIoModule_I2c, pI2c->I2cPort, NvOdmI2cPinMap_Config1);
            else
#endif
                pI2c->hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, pI2c->I2cPort);

            if (!pI2c->hOdmI2c)
                return NV_FALSE;

            NvOdmOsWaitUS(1000); // allow pin changes to settle

            /// Test that the device is responsive
            /// Make several attempts to give device time to finish powerup:
            for (i = 0; i < I2C_POWERUP_RETRIES; i++)
            {
                I2cResult = NvOdmImagerI2cAck(pI2c);
                if (I2cResult == NvOdmI2cStatus_Success)
                {
                    break;
                }
                NvOdmOsWaitUS(I2C_POWERUP_RETRY_WAIT_MS * 1000);
            }
            if (i >= I2C_POWERUP_RETRIES)
            {
                NvOdmOsDebugPrintf("NvOdmImagerSetPowerLevel: Device failed Ack after %d retries\n",i);
                return NV_FALSE;
            }

            if(pI2cRight && pI2cRight->DeviceAddr > 0)
            {
#ifndef SET_KERNEL_PINMUX
                NvOdmQueryPinMux(NvOdmIoModule_I2c, &pI2cConfigs, &NumI2cConfigs);
                if (pI2cConfigs[pI2cRight->I2cPort]==NvOdmI2cPinMap_Multiplexed)
                    pI2cRight->hOdmI2c =
                        NvOdmI2cPinMuxOpen(NvOdmIoModule_I2c, pI2cRight->I2cPort, NvOdmI2cPinMap_Config1);
                else
#endif
                    pI2cRight->hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, pI2cRight->I2cPort);

                if (!pI2cRight->hOdmI2c)
                    return NV_FALSE;

                NvOdmOsWaitUS(1000); // allow pin changes to settle

                /// Test that the device is responsive
                I2cResult = NvOdmImagerI2cAck(pI2cRight);
                if (I2cResult != NvOdmI2cStatus_Success)
                {
                    NvOsDebugPrintf("NvOdmImagerSetPowerLevel: Device failed Ack\n");
                    return NV_FALSE;
                }

                pI2cRight->AddressBitWidth = 16;
                pI2cRight->DataBitWidth = 8;
            }
            else
            {
            ////
            // TODO: Need more thought here. How do we program pmu_i2c mux?
            ////
                if(pI2cPmu && pI2cPmu->DeviceAddr > 0)
                {
#ifndef SET_KERNEL_PINMUX
                    NvOdmQueryPinMux(NvOdmIoModule_I2c_Pmu, &pI2cConfigs, &NumI2cConfigs);
#endif
                    pI2cPmu->hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c_Pmu, pI2cPmu->I2cPort);
                    if (!pI2cPmu->hOdmI2c) {
                        return NV_FALSE;
                    }
                    NvOdmOsWaitUS(1000); // allow pin changes to settle

                    if(pI2cPmu->I2cPort == 0) // for 3D camera board.
                    {
                        NvU32 SensorI2CAddr;
                        NvOdmI2cStatus status;

                        SensorI2CAddr = pI2cPmu->DeviceAddr;
                        pI2cPmu->DeviceAddr = 0x42;
                        pI2cPmu->AddressBitWidth = 8;
                        pI2cPmu->DataBitWidth = 8;

                        status = NvOdmImagerI2cWrite(pI2cPmu, 0x03, 0xfc);
                        if (status != NvOdmI2cStatus_Success)
                        {
                            return NV_FALSE;
                        }

                        status = NvOdmImagerI2cWrite(pI2cPmu, 0x01, 0x01);
                        if (status != NvOdmI2cStatus_Success)
                        {
                            return NV_FALSE;
                        }
                        pI2cPmu->DeviceAddr = SensorI2CAddr;
                        pI2cPmu->AddressBitWidth = 16;
                        pI2cPmu->DataBitWidth = 8;
                    }
                }
            }
            break;

        case NvOdmImagerPowerLevel_Standby:
            if (pGpioConfig->Gpios[NvOdmImagerGpio_Powerdown].Enable)
            {
                NvOdmGpioPinActiveState Level;
                NvOdmGpioGetState(pGpioConfig->hGpio, pGpioConfig->hPin[0],
                    (NvU32 *)&Level);
                NvOdmGpioSetState(pGpioConfig->hGpio, pGpioConfig->hPin[0],
                    !Level);
            }

            break;
        case NvOdmImagerPowerLevel_Off:
            // Turn off PMU voltages
            hPmu = NvOdmServicesPmuOpen();
            if (!hPmu)
                return NV_FALSE;

            // Turn off VDD
            for (i = 0; i < pConnections->NumAddress; i++)
            {
                switch (pConnections->AddressList[i].Interface)
                {
                    case NvOdmIoModule_Vdd:
                        {
                            NvU32 Settle_us;
                            NvU32 Millivolts;
                            /* set the rail voltage to the recommended */
                            NvOdmServicesPmuSetVoltage(hPmu,
                               pConnections->AddressList[i].Address,
                               NVODM_VOLTAGE_OFF,
                               &Settle_us);
                            /* wait for rail to settle */
                            NvOdmOsWaitUS(Settle_us);
                            /* make sure it is off */
                            NvOdmServicesPmuGetVoltage(hPmu,
                               pConnections->AddressList[i].Address,
                               &Millivolts);
                        }
                        break;
                    default:
                        break;
                }
            }
            NvOdmServicesPmuClose(hPmu);

            // Reset the sensor
            for (i = 0; i < pGpioConfig->PinsUsed; i++)
            {
                if (pGpioConfig->hPin[i])
                {
                    NvOdmGpioPinActiveState Level;
                    NvOdmGpioGetState(pGpioConfig->hGpio, pGpioConfig->hPin[i],
                        (NvU32 *)&Level);
                    NvOdmGpioSetState(pGpioConfig->hGpio, pGpioConfig->hPin[i],
                        !Level);
                }
                NvOdmGpioReleasePinHandle(pGpioConfig->hGpio,
                    pGpioConfig->hPin[i]);
            }

            // release the resources
            NvOdmGpioClose(pGpioConfig->hGpio);
            pGpioConfig->hGpio = NULL;
            NvOdmI2cClose(pI2c->hOdmI2c);
            pI2c->hOdmI2c = NULL;
            if(pI2cPmu)
            {
                NvOdmI2cClose(pI2cPmu->hOdmI2c);
                pI2cPmu->hOdmI2c = NULL;
            }

            break;
        default:
            return NV_FALSE;
    }

    return NV_TRUE;
}


/**
 * LoadOverridesFile searches for and loads the contents of a
 * camera overrides file. If not found or empty, NULL is returned
 * and the error is logged. If found, the file is read into an
 * allocated buffer, and the buffer is returned. The caller owns
 * any memory returned.
 */
char*
LoadOverridesFile(const char *pFiles[], NvU32 len)
{
    NvOdmOsStatType Stat;
    NvOdmOsFileHandle hFile = NULL;
    char *pTempString = NULL;
    NvU32 i;

    for (i = 0; i < len; i++)
    {
#if DEBUG_PRINTS
        NvOsDebugPrintf("Imager: looking for override file [%s] %d/%d\n",
            pFiles[i], i+1, len);
#endif
        if (NvOdmOsStat(pFiles[i], &Stat) && Stat.size > 0) // found one
        {
            pTempString = NvOdmOsAlloc((size_t)Stat.size + 1);
            if (pTempString == NULL)
            {
                NV_ASSERT(pTempString != NULL &&
                          "Couldn't alloc memory to read config file");
                break;
            }
            if (!NvOdmOsFopen(pFiles[i], NVODMOS_OPEN_READ, &hFile))
            {
                NV_ASSERT(!"Failed to open a file that fstatted just fine");
                NvOdmOsFree(pTempString);
                pTempString = NULL;
                break;
            }
            NvOdmOsFread(hFile, pTempString, (size_t)Stat.size, NULL);
            pTempString[Stat.size] = '\0';
            NvOdmOsFclose(hFile);

            break;    // only handle the first file
        }
    }

#if DEBUG_PRINTS
    if(pTempString == NULL)
    {
        NvOsDebugPrintf("Imager: No override file found.\n");
    }
    else
    {
        NvOsDebugPrintf("Imager: Found override file data [%s]\n",
            pFiles[i]);
    }
#endif

    return pTempString;
}

/**
 * LoadBlobFile searches factory calibration blob file.
 * If not found or empty, NV_FALSE is returned.
 * If found, the blob is read into an pre-allocated buffer.
 */
NvBool
LoadBlobFile(const char *pFiles[], NvU32 len, NvU8 *pBlob, NvU32 blobSize)
{
    NvOdmOsStatType Stat;
    NvOdmOsFileHandle hFile = NULL;
    NvU32 i;

    for (i = 0; i < len; i++)
    {
#if DEBUG_PRINTS
        NvOsDebugPrintf("Imager: looking for override file [%s] %d/%d\n",
            pFiles[i], i+1, len);
#endif
        if (NvOdmOsStat(pFiles[i], &Stat) && Stat.size > 0) // found one
        {
            if ((NvU64) blobSize < Stat.size) // not enough space to store the blob
            {
                NvOsDebugPrintf("Insufficient memory for reading blob\n");
                continue;   // keep searching for blob files
            }

            if (!NvOdmOsFopen(pFiles[i], NVODMOS_OPEN_READ, &hFile))
            {
                NV_ASSERT(!"Failed to open a file that fstatted just fine");
                break;
            }
            NvOdmOsFread(hFile, pBlob, (size_t)Stat.size, NULL);
            NvOdmOsFclose(hFile);
            return NV_TRUE;    // only handle the first valid blob file
        }
    }

    return NV_FALSE;
}

static void
GetPixelAspectRatioCorrectedModeDimension(
    const NvOdmImagerSensorMode *pMode,
    NvSize *pDimension)
{
    NV_ASSERT(pMode->PixelAspectRatio >= 0.0);

    if (pMode->PixelAspectRatio > 1.0)
    {
        pDimension->width = pMode->ActiveDimensions.width;
        pDimension->height = (NvS32)(pMode->ActiveDimensions.height /
                                      pMode->PixelAspectRatio);
    }
    else if ((pMode->PixelAspectRatio > 0.0) && (pMode->PixelAspectRatio < 1.0))
    {
        pDimension->width = (NvS32)(pMode->ActiveDimensions.width *
                                    pMode->PixelAspectRatio);
        pDimension->height = pMode->ActiveDimensions.height;
    }
    else
    {
        *pDimension = pMode->ActiveDimensions;
    }

    // round to 2-pixel
    pDimension->width = (pDimension->width + 1) & ~1;
    pDimension->height = (pDimension->height + 1) & ~1;
}

static NvBool
IsAspectRatioSame(
    const NvSize ASize,
    const NvSize BSize)
{
    // 16/9 = 1.777 or 1.11000111 in binary
    // 4/3 = 1.333 or 1.01010101 in binary
    // So to check one digit accuracy after
    // decimal i.e. 0.1, floating point operation
    // is used and difference between the two ratio
    // is compared with 0.1.
    // This code is used couple of times only so
    // using floating point does effect performance.

    NvF32 ARatio, BRatio, diff;
    ARatio = ((NvF32)ASize.width/ASize.height);
    BRatio = ((NvF32)BSize.width/BSize.height);
    diff = (ARatio > BRatio) ? ARatio - BRatio : BRatio - ARatio;
    if (diff<0.1)
        return NV_TRUE;
    else
        return NV_FALSE;
}

NvU32
NvOdmImagerGetBestSensorMode(
    const NvOdmImagerSensorMode* pRequest,
    const SensorSetModeSequence* pModeList,
    NvU32 NumSensorModes)
{
    NvU32 i;
    NvU32 BestMode = 0;
    NvS32 CurrentMatchScore = -1;

    // Match Score bit for Preference Parameter is set in the following order:
    #define CropModeBit 0x1 // Bit 1: Crop Mode
    #define AspectRatioBit 0x2 // Bit 2: Aspect Ratio, only for video mode
    #define FrameRateBit 0x4 // Bit 3: Frame Rate
    #define ResolutionBit 0x8 // Bit 4: Resolution

    for (i = 0; i < (NvU32)NumSensorModes; i++)
    {
        const NvOdmImagerSensorMode* pMode = &pModeList[i].Mode;
        NvS32 MatchScore = 0;
        NvSize CorrectedModeResolution = {0, 0};

        GetPixelAspectRatioCorrectedModeDimension(pMode, &CorrectedModeResolution);

        //Give Ist Preference to resolution while selecting a mode
        if (pRequest->ActiveDimensions.width <= CorrectedModeResolution.width &&
        pRequest->ActiveDimensions.height <= CorrectedModeResolution.height)
        {
            MatchScore = (MatchScore) | (ResolutionBit);
        }

        //Give IInd Preference to frame rate
        if (pRequest->PeakFrameRate <= pMode->PeakFrameRate)
        {
            MatchScore = (MatchScore) | (FrameRateBit);
        }

        //In case of Video Capture IIIrd preference is given to aspect ratio
        // while in case of Still capture it is ignored.
        if (IsAspectRatioSame(pMode->ActiveDimensions, pRequest->ActiveDimensions) &&
        ((pRequest->Type == NvOdmImagerModeType_Movie) ||
        (pRequest->Type == NvOdmImagerModeType_Preview)))
        {
            MatchScore = (MatchScore) | (AspectRatioBit);
        }

        //Give Final Preference Crop Mode, currently
        //it could be partial or No Crop Mode
        if (pMode->CropMode == NvOdmImagerNoCropMode)
        {
            MatchScore = (MatchScore) | (CropModeBit);
        }

        if (MatchScore > CurrentMatchScore)
        {
            //This mode is selected over previous mode
            BestMode = i;
            CurrentMatchScore = MatchScore;
        }
        else if (MatchScore == CurrentMatchScore)
        {
            // There is a tie between two modes
            //further we have to choose between these two
            switch(MatchScore>>2)
            {
#define MODE_WIDTH_SCALING(_m) \
    (((_m).PixelAspectRatio == 0.0f || (_m).PixelAspectRatio >= 1.0f) ? \
    (1.0f) : ((_m).PixelAspectRatio))

#define MODE_HEIGHT_SCALING(_m) \
    (((_m).PixelAspectRatio <= 1.0f) ? \
    (1.0f) : (1.0f / (_m).PixelAspectRatio))

#define BIGGER_RESOLUTION_MODE(_a, _b) \
    ((NvF32)_a.ActiveDimensions.width * MODE_WIDTH_SCALING(_a) < \
    (NvF32)_b.ActiveDimensions.width * MODE_WIDTH_SCALING(_b) || \
    (NvF32)_a.ActiveDimensions.height * MODE_HEIGHT_SCALING(_a) < \
    (NvF32)_b.ActiveDimensions.height * MODE_HEIGHT_SCALING(_b))

#define SMALLER_RESOLUTION_MODE(_a, _b) \
    ((NvF32)_a.ActiveDimensions.width * MODE_WIDTH_SCALING(_a) > \
    (NvF32)_b.ActiveDimensions.width * MODE_WIDTH_SCALING(_b) || \
    (NvF32)_a.ActiveDimensions.height * MODE_HEIGHT_SCALING(_a) > \
    (NvF32)_b.ActiveDimensions.height * MODE_HEIGHT_SCALING(_b))

            //0: Resolution doesn't match, frame rate doesn't match
            case 0:
            //1: Resolution doesn't match, frame rate matches
            case 1:
                // choose the one with a bigger resolution
                if (BIGGER_RESOLUTION_MODE(pModeList[BestMode].Mode, pModeList[i].Mode))
                BestMode = i;
            break;

            // 2: Resolution matches, frame rate doesn't match
            case 2:
                // choose the one with a higher frame rate
                if (pModeList[BestMode].Mode.PeakFrameRate < pModeList[i].Mode.PeakFrameRate)
                {
                    BestMode = i;
                }
                // choose the one with a smaller resolution if frame rates are the same
                else if (pModeList[BestMode].Mode.PeakFrameRate == pModeList[i].Mode.PeakFrameRate)
                {
                    if (SMALLER_RESOLUTION_MODE(pModeList[BestMode].Mode, pModeList[i].Mode))
                        BestMode = i;
                }
                break;
            // 3: Resolution matches, frame rate matches.
            case 3:
            {
                // choose the one with a smaller resolution
                if (SMALLER_RESOLUTION_MODE(pModeList[BestMode].Mode, pModeList[i].Mode))
                BestMode = i;
                break;
            }
        }
        }
    }

#if DEBUG_PRINTS
    NvOsDebugPrintf("NvOdmImagerGetBestSensorMode: requested %dx%d, %3.1fFPS, type %u selected %dx%d, %3.1fFPS (%u)\n",
                    pRequest->ActiveDimensions.width,
                    pRequest->ActiveDimensions.height,
                    pRequest->PeakFrameRate,
                    pRequest->Type,
                    pModeList[BestMode].Mode.ActiveDimensions.width,
                    pModeList[BestMode].Mode.ActiveDimensions.height,
                    pModeList[BestMode].Mode.PeakFrameRate,
                    BestMode);
#endif
    return BestMode;
}
