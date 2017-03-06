/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_touch_synaptics.h"
#include "synaptics_reg.h"
#include "nvodm_query_discovery.h"

static const
NvOdmTouchCapabilities Synaptics_Capabilities =
{
    1,      // IsMultiTouchSupported
    2,      // MaxNumberOfFingerCoordReported;
    0,      // IsRelativeDataSupported
    1,      // MaxNumberOfRelativeCoordReported
    1,      // MaxNumberOfWidthReported
    1,      // MaxNumberOfPressureReported
    (NvU32)NvOdmTouchGesture_Not_Supported, // Gesture
    1,      // IsWidthSupported
    1,      // IsPressureSupported
    1,      // IsFingersSupported
    X_MIN,   // XMinPosition
    Y_MIN,   // YMinPosition
    X_MAX,   // XMaxPosition
    Y_MAX,   // YMaxPosition
    (NvU32)(NvOdmTouchOrientation_XY_SWAP |
            NvOdmTouchOrientation_H_FLIP |
            NvOdmTouchOrientation_V_FLIP)
};

static NvBool
Synaptics_WriteRegister(Synaptics_TouchDevice* hTouch, NvU8 reg, NvU8 val)
{
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;
    NvU8 arr[2] = {0};

    arr[0] = reg;
    arr[1] = val;

    TransactionInfo.Address = hTouch->DeviceAddr;
    TransactionInfo.Buf = arr;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 2;

    Error = NvOdmI2cTransaction(hTouch->hOdmI2c,
                                &TransactionInfo,
                                1,
                                hTouch->I2cClockSpeedKHz,
                                SYNAPTICS_I2C_TIMEOUT);
    if (Error != NvOdmI2cStatus_Success)
    {
        NVODMTOUCH_PRINTF(("I2C Write Failure = %d (addr=0x%x, reg=0x%x, val=0x%0x)\n",
                            Error, hTouch->DeviceAddr, reg, val));
        return NV_FALSE;
    }

    return NV_TRUE;
}

static NvBool Synaptics_ReadRegisterOnce (Synaptics_TouchDevice* hTouch, NvU8 reg, NvU8* buffer, NvU32 len)
{
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo[2 * SYNAPTICS_MAX_READS];
    NvU32 reads = (len + (SYNAPTICS_MAX_PACKET_SIZE - 1)) / SYNAPTICS_MAX_PACKET_SIZE;
    NvU32 left = len;
    NvU16 i = 0;

    NV_ASSERT(len <= SYNAPTICS_MAX_READ_BYTES);

    ////////////////////////////////////////////////////////////////////////////
    // For multi-byte reads, the TP panel supports just sending the first
    // address and then keep reading registers.
    // The limit for I2C packets is 8 bytes, so we read up to 8 bytes per
    // multi-byte read transaction.
    ////////////////////////////////////////////////////////////////////////////

    for (i = 0; i < reads; i++)
    {
        NvU16 ind = i * 2;

        TransactionInfo[ind].Address = hTouch->DeviceAddr;
        TransactionInfo[ind].Buf = &reg;
        TransactionInfo[ind].Flags = NVODM_I2C_IS_WRITE;
        TransactionInfo[ind].NumBytes = 1;

        ind++;

        TransactionInfo[ind].Address = hTouch->DeviceAddr | 0x1;
        TransactionInfo[ind].Buf = buffer + i * SYNAPTICS_MAX_PACKET_SIZE;
        TransactionInfo[ind].Flags = 0;
        TransactionInfo[ind].NumBytes =
            left > SYNAPTICS_MAX_PACKET_SIZE ? SYNAPTICS_MAX_PACKET_SIZE : left;

        left -= SYNAPTICS_MAX_PACKET_SIZE;
    }

    Error = NvOdmI2cTransaction(hTouch->hOdmI2c,
                                TransactionInfo,
                                reads * 2,
                                hTouch->I2cClockSpeedKHz,
                                SYNAPTICS_I2C_TIMEOUT);

    if (Error != NvOdmI2cStatus_Success)
    {
        NVODMTOUCH_PRINTF(("I2C Read Failure = %d (addr=0x%x, reg=0x%x)\n", Error,
                           hTouch->DeviceAddr, reg));
        return NV_FALSE;
    }

    return NV_TRUE;
}

static NvBool Synaptics_ReadRegisterSafe (Synaptics_TouchDevice* hTouch, NvU8 reg, NvU8* buffer, NvU32 len)
{
    if (!Synaptics_ReadRegisterOnce(hTouch, reg, buffer, len))
        return NV_FALSE;

    return NV_TRUE;
}

static NvBool Synaptics_Configure(Synaptics_TouchDevice* hTouch)
{
    NvU32 temp = 0;

    /* disable interrupts */
    if (!SYNAPTICS_WRITE(hTouch, TP_GESTURE_ENABLES_1, 0))
        return NV_FALSE;

    // set the device control register
    temp = (SYNAPTICS_LOW_SAMPLE_RATE & DEVICE_CTRL_REPORT_RATE_BIT_MASK) || // sample rate = 80 samples per sec
           (DEVICE_CTRL_CONFIGURED_BIT_MASK) || // configured bit is set
           (0 & DEVICE_CTRL_NO_SLEEP_BIT_MASK) || // use the current sleep mode
           (SLEEP_MODE_NORMAL & DEVICE_CTRL_SLEEP_MODE_BIT_MASK); // use normal operation

    /* write the Configured bit, and the sample rate */
    if (!SYNAPTICS_WRITE(hTouch, RMI_DEVICE_CONTROL, temp))
        return NV_FALSE;

    hTouch->ChipRevisionId = 0; // not used as of now
    NVODMTOUCH_PRINTF(("Touch controller Revision ID = %d\n", hTouch->ChipRevisionId));

    /* Configure the 2D general control register */
    temp = (0x0 & TP_REPORT_MODE_REPORTING_MODE_BIT_MASK) ||
           (TP_REPORT_MODE_ABS_POS_FILTER_BIT_MASK);

    if (!SYNAPTICS_WRITE(hTouch, TP_REPORT_MODE, temp))
        return NV_FALSE;

    hTouch->SleepMode = 0x0;
    hTouch->SampleRate = 0; /* this forces register write */
    return NV_TRUE;
}

static NvBool Synaptics_GetSample(Synaptics_TouchDevice* hTouch, NvOdmTouchCoordinateInfo* coord)
{
    NvU8 Finger0[X_Y_COORDINATE_DATA_LENGTH] = {0};
    NvU8 Finger1[X_Y_COORDINATE_DATA_LENGTH] = {0};
    NvU16 status = 0;
    NvU8 FingerState = 0, Gesture = 0;

    NVODMTOUCH_PRINTF(("Synaptics_GetSample+\n"));

    if (!hTouch || !coord)
        return NV_FALSE;

    if (!SYNAPTICS_READ(hTouch, TP_FINGER_STATE, &FingerState, 1))
        return NV_FALSE;

    /* tell windows to ignore transitional finger count samples */
    coord->fingerstate = (FingerState == 0x1) ? NvOdmTouchSampleValidFlag : NvOdmTouchSampleIgnore;

    if (coord->fingerstate == NvOdmTouchSampleIgnore)
        return NV_TRUE;

    // get the finger count
    coord->additionalInfo.Fingers = 0;
    if (!SYNAPTICS_READ(hTouch, TP_GESTURE_FLAGS_1, &coord->additionalInfo.Fingers, 1))
       return NV_FALSE;

    // extract the number of fingers on touchpad
    coord->additionalInfo.Fingers &= TP_GESTURE_1_FINGER_COUNT_BIT_MASK;

    // check the gesture event occured
    if (!SYNAPTICS_READ(hTouch, TP_GESTURE_FLAGS_0, &Gesture, 1))
       return NV_FALSE;

    if (Gesture & TP_GESTURE_0_SINGLE_TAP_BIT_MASK)
        // Bit 3 of status indicates a tap. Driver still doesn't expose
        // gesture capabilities. This is added more for testing of the support
        // in the hardware for gesture support.
    {
        coord->additionalInfo.Gesture = NvOdmTouchGesture_Tap;
        // NvOdmOsDebugPrintf("Detected the Tap gesture\n");
    }

    /* get co-ordinates for both the fingers */
    SYNAPTICS_READ(hTouch, TP_X_POS_11_4_FINGER_0, Finger0, X_Y_COORDINATE_DATA_LENGTH);
    SYNAPTICS_READ(hTouch, TP_X_POS_11_4_FINGER_1, Finger1, X_Y_COORDINATE_DATA_LENGTH);

    if (status)
    {
        /* always read first finger data, even if transitional */
        coord->fingerstate |= NvOdmTouchSampleDownFlag;

        if ((hTouch->PrevFingers == 2) && (coord->additionalInfo.Fingers == 1))
        {
            coord->xcoord =
            coord->additionalInfo.multi_XYCoords[0][0] =
                (NvU32)((Finger1[2] & 0xF) | (Finger1[0] << 4));

            coord->ycoord =
            coord->additionalInfo.multi_XYCoords[0][1] =
                (NvU32)(((Finger1[2] & 0xF0) >> 4) | (Finger1[1] << 4));

            coord->additionalInfo.width[0] = 0;
            coord->additionalInfo.Pressure[0] = 0;
        }
        else
        {
            coord->xcoord =
            coord->additionalInfo.multi_XYCoords[0][0] =
                (NvU32)((Finger0[2] & 0xF) | (Finger0[0] << 4));

            coord->ycoord =
            coord->additionalInfo.multi_XYCoords[0][1] =
                (NvU32)(((Finger0[2] & 0xF0) >> 4) | (Finger0[1] << 4));

            coord->additionalInfo.width[0] = 0;
            coord->additionalInfo.Pressure[0] = 0;
        }

        /* only read second finger data if reported */
        if (coord->additionalInfo.Fingers == 2)
        {
            coord->additionalInfo.multi_XYCoords[1][0] =
                (NvU32)((Finger1[2] & 0xF) | (Finger1[0] << 4));

            coord->additionalInfo.multi_XYCoords[1][1] =
                (NvU32)(((Finger1[2] & 0xF0) >> 4) | (Finger1[1] << 4));

            /* these are not supported, zero out just in case */
            coord->additionalInfo.width[1] = 0;
            coord->additionalInfo.Pressure[1] = 0;
            if (coord->additionalInfo.multi_XYCoords[1][0] <= 0 ||
                coord->additionalInfo.multi_XYCoords[1][0] >= hTouch->Caps.XMaxPosition ||
                coord->additionalInfo.multi_XYCoords[1][1] <= 0 ||
                coord->additionalInfo.multi_XYCoords[1][1] >= hTouch->Caps.YMaxPosition ||
                coord->additionalInfo.width[0] == 0xF)
                coord->fingerstate = NvOdmTouchSampleIgnore;
#if SYNAPTICS_REPORT_2ND_FINGER_DATA
            else
                NvOdmOsDebugPrintf("catch 2 fingers width=0x%x, X=%d, Y=%d, DeltaX=%d, DeltaY=%d\n",
                                    coord->additionalInfo.width[0],
                                    coord->additionalInfo.multi_XYCoords[1][0],
                                    coord->additionalInfo.multi_XYCoords[1][1],
                                    Relative[0], Relative[1]);
#endif
        }
    }

    hTouch->PrevFingers = coord->additionalInfo.Fingers;

    NVODMTOUCH_PRINTF(("Synaptics_GetSample-\n"));
    return NV_TRUE;
}

static void InitOdmTouch (NvOdmTouchDevice *pDev)
{
    if (pDev)
    {
        pDev->Close              = Synaptics_Close;
        pDev->GetCapabilities    = Synaptics_GetCapabilities;
        pDev->ReadCoordinate     = Synaptics_ReadCoordinate;
        pDev->EnableInterrupt    = Synaptics_EnableInterrupt;
        pDev->HandleInterrupt    = Synaptics_HandleInterrupt;
        pDev->GetSampleRate      = Synaptics_GetSampleRate;
        pDev->SetSampleRate      = Synaptics_SetSampleRate;
        pDev->PowerControl       = Synaptics_PowerControl;
        pDev->PowerOnOff         = Synaptics_PowerOnOff;
        pDev->GetCalibrationData = Synaptics_GetCalibrationData;
        pDev->OutputDebugMessage = NV_FALSE;
    }
}

static void Synaptics_GpioIsr(void *arg)
{
    Synaptics_TouchDevice *hTouch = (Synaptics_TouchDevice *)arg;
    NvU8 IntStatus = 0;

    SYNAPTICS_READ(hTouch, RMI_INTERRUPT_STATUS, &IntStatus, 1);

    if (IntStatus & INT_STATUS_STATUS_BIT_MASK)
    {
        /* reconfigure panel, if failed start again */
        Synaptics_Configure(hTouch);
    }

    if (IntStatus & INT_STATUS_GPIO_BIT_MASK)
    {
        // report L/R click
    }

    /* Signal the touch thread to read the sample. After it is done reading the
     * sample it should re-enable the interrupt. */
    NvOdmOsSemaphoreSignal(hTouch->hIntSema);
}

NvBool Synaptics_ReadCoordinate(NvOdmTouchDeviceHandle hDevice, NvOdmTouchCoordinateInfo* coord)
{
    Synaptics_TouchDevice *hTouch = (Synaptics_TouchDevice *)hDevice;

#if SYNAPTICS_BENCHMARK_SAMPLE
    NvU32 time = NvOdmOsGetTimeMS();
#endif
    NVODMTOUCH_PRINTF(("GpioIst+\n"));

    for (;;)
    {
        if (Synaptics_GetSample(hTouch, coord))
        {
            break;
        }

        /* If reading the data failed, the panel may be resetting itself.
           Poll device status register to find when panel is back up
           and reconfigure */
        for (;;)
        {
            NvU8 DevStatus = 0;

            while (!SYNAPTICS_READ(hTouch, RMI_DEVICE_STATUS, &DevStatus, 1))
            {
                NvOdmOsSleepMS(10);
            }

            /* if we have a panel error, force reset and wait for status again */
            if (DevStatus & DEVICE_STATUS_UNCONFIGURED_BIT_MASK)
            {
                SYNAPTICS_WRITE(hTouch, RMI_DEVICE_COMMAND, DEVICE_RESET);
                continue;
            }

            /* reconfigure panel, if failed start again */
            if (!Synaptics_Configure(hTouch))
                continue;

            /* enable interrupts -- only for absolute data */
            if (!SYNAPTICS_WRITE(hTouch, TP_GESTURE_ENABLES_1, TP_GESTURE_INT_ENABLE_SINGLE_TAP_BIT_MASK))
            {
                continue;
            }

            break;
        }
    }

#if SYNAPTICS_BENCHMARK_SAMPLE
    NvOdmOsDebugPrintf("Touch sample time %d\n", NvOdmOsGetTimeMS() - time);
#endif

    return NV_TRUE;
}

void Synaptics_GetCapabilities(NvOdmTouchDeviceHandle hDevice, NvOdmTouchCapabilities* pCapabilities)
{
    Synaptics_TouchDevice *hTouch = (Synaptics_TouchDevice *)hDevice;

    if (hTouch && pCapabilities)
        *pCapabilities = hTouch->Caps;
}

NvBool Synaptics_PowerOnOff(NvOdmTouchDeviceHandle hDevice, NvBool OnOff)
{
    if (!hDevice)
        return NV_FALSE;

    if (OnOff)
        return Synaptics_PowerControl(hDevice, NvOdmTouch_PowerMode_0); // power ON
    else
        return Synaptics_PowerControl(hDevice, NvOdmTouch_PowerMode_3); // power OFF
}

NvBool Synaptics_Open(NvOdmTouchDeviceHandle *hDevice)
{
    Synaptics_TouchDevice *hTouch = NULL;
    NvU32 i = 0;
    NvU32 found = 0;
    NvU32 GpioPort = 0;
    NvU32 GpioPin = 0;
    NvU32 I2cInstance = 0;
    NvU8 Buf[4] = {0};
    const NvOdmPeripheralConnectivity *pConnectivity = NULL;
    NvU32 MaxX = 0, MaxY = 0;

    // allocate memory to be used for the handle
    hTouch = NvOdmOsAlloc(sizeof(Synaptics_TouchDevice));
    if (!hTouch)
        return NV_FALSE;

    NvOdmOsMemset(hTouch, 0, sizeof(Synaptics_TouchDevice));

    /* set function pointers */
    InitOdmTouch(&hTouch->OdmTouch);

    // read the query database
    pConnectivity = NvOdmPeripheralGetGuid(SYNAPTICS_TOUCH_DEVICE_GUID);
    if (!pConnectivity)
    {
        NVODMTOUCH_PRINTF(("NvOdm Touch : pConnectivity is NULL Error \n"));
        goto fail;
    }

    if (pConnectivity->Class != NvOdmPeripheralClass_HCI)
    {
        NVODMTOUCH_PRINTF(("NvOdm Touch : didn't find any periperal in discovery query for touch device Error \n"));
        goto fail;
    }

    // reset VDD value
    hTouch->VddId = 0xFF;

    for (i = 0; i < pConnectivity->NumAddress; i++)
    {
        switch (pConnectivity->AddressList[i].Interface)
        {
            case NvOdmIoModule_I2c:
                hTouch->DeviceAddr = (pConnectivity->AddressList[i].Address << 1);
                I2cInstance = pConnectivity->AddressList[i].Instance;
                found |= 1;
                break;

            case NvOdmIoModule_Gpio:
                GpioPort = pConnectivity->AddressList[i].Instance;
                GpioPin = pConnectivity->AddressList[i].Address;
                found |= 2;
                break;

            default:
                break;
        }
    }

    // see if we found the bus and GPIO used by the hardware
    if ((found & 3) != 3)
    {
        NVODMTOUCH_PRINTF(("NvOdm Touch : peripheral connectivity problem \n"));
        goto fail;
    }

    // allocate I2C instance
    hTouch->hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, I2cInstance);
    if (!hTouch->hOdmI2c)
    {
        NVODMTOUCH_PRINTF(("NvOdm Touch : NvOdmI2cOpen Error \n"));
        goto fail;
    }

    // get the handle to the pin used as ATTN
    hTouch->hGpio = (NvOdmServicesGpioHandle)NvOdmGpioOpen();
    if (!hTouch->hGpio)
    {
        NVODMTOUCH_PRINTF(("NvOdm Touch : NvOdmGpioOpen Error \n"));
        goto fail;
    }

    hTouch->hPin = NvOdmGpioAcquirePinHandle(hTouch->hGpio, GpioPort, GpioPin);
    if (!hTouch->hPin)
    {
        NVODMTOUCH_PRINTF(("NvOdm Touch : Couldn't get GPIO pin \n"));
        goto fail;
    }

    NvOdmGpioConfig(hTouch->hGpio, hTouch->hPin, NvOdmGpioPinMode_InputData);

    /* set default capabilities */
    NvOdmOsMemcpy(&hTouch->Caps, &Synaptics_Capabilities, sizeof(NvOdmTouchCapabilities));

    /* set default I2C speed */
    hTouch->I2cClockSpeedKHz = SYNAPTICS_I2C_SPEED_KHZ;

    /* configure panel */
    if (!Synaptics_Configure(hTouch))
        goto fail;

    /* get the max X and Y co-ordinates */
    if (!SYNAPTICS_READ(hTouch, TP_MAX_X_POSITION_7_0, &Buf[0], 1))
        goto fail;

    if (!SYNAPTICS_READ(hTouch, TP_MAX_X_POSITION_11_8, &Buf[1], 1))
        goto fail;

    MaxX = (NvU32)((Buf[1] << 8) | Buf[0]);

    if (!SYNAPTICS_READ(hTouch, TP_MAX_Y_POSITION_7_0, &Buf[0], 1))
        goto fail;

    if (!SYNAPTICS_READ(hTouch, TP_MAX_Y_POSITION_11_8, &Buf[1], 1))
        goto fail;

    MaxY = (NvU32)((Buf[1] << 8) | Buf[0]);

    /* set max positions */
    hTouch->Caps.XMaxPosition = X_MAX;
    hTouch->Caps.YMaxPosition = Y_MAX;

    *hDevice = (NvOdmTouchDeviceHandle)hTouch;
    return NV_TRUE;

 fail:
    Synaptics_Close((NvOdmTouchDeviceHandle)hTouch);
    hTouch = NULL;
    return NV_FALSE;
}

void Synaptics_Close (NvOdmTouchDeviceHandle hDevice)
{
    Synaptics_TouchDevice *hTouch = (Synaptics_TouchDevice *)hDevice;

    if (hTouch)
    {
        if (hTouch->hGpio)
        {
            if (hTouch->hGpioIntr)
            {
                NvOdmGpioInterruptUnregister(hTouch->hGpio, hTouch->hPin, hTouch->hGpioIntr);
                hTouch->hGpioIntr = NULL;
            }

            if (hTouch->hPin)
            {
                NvOdmGpioReleasePinHandle(hTouch->hGpio, hTouch->hPin);
                hTouch->hPin = NULL;
            }

            NvOdmGpioClose(hTouch->hGpio);
            hTouch->hGpio = NULL;
        }

        if (hTouch->hOdmI2c)
        {
            NvOdmI2cClose(hTouch->hOdmI2c);
            hTouch->hOdmI2c = NULL;
        }

        NvOdmOsFree(hTouch);
        hTouch = NULL;
    }
}

NvBool Synaptics_EnableInterrupt(NvOdmTouchDeviceHandle hDevice, NvOdmOsSemaphoreHandle hIntSema)
{
    Synaptics_TouchDevice *hTouch = (Synaptics_TouchDevice *)hDevice;
    NvOdmTouchCoordinateInfo coord = {0};

    NV_ASSERT(hIntSema);

    /* can only be initialized once */
    if (hTouch->hGpioIntr || hTouch->hIntSema)
        return NV_FALSE;

    /* zero intr status */
    (void)Synaptics_GetSample(hTouch, &coord);

    hTouch->hIntSema = hIntSema;

    if (NvOdmGpioInterruptRegister(
                hTouch->hGpio,
                &hTouch->hGpioIntr,
                hTouch->hPin,
                NvOdmGpioPinMode_InputInterruptLow,
                Synaptics_GpioIsr,
                (void*)hTouch,
                SYNAPTICS_DEBOUNCE_TIME_MS) == NV_FALSE)
    {
        return NV_FALSE;
    }

    if (!hTouch->hGpioIntr)
        return NV_FALSE;

    /* enable interrupts -- only for absolute data */
    if (!SYNAPTICS_WRITE(hTouch, TP_GESTURE_ENABLES_1, TP_GESTURE_INT_ENABLE_SINGLE_TAP_BIT_MASK))
    {
        NvOdmGpioInterruptUnregister(hTouch->hGpio, hTouch->hPin, hTouch->hGpioIntr);
        hTouch->hGpioIntr = NULL;
        return NV_FALSE;
    }

    /* enable interrupts -- to read the left and right clicks */
    if (!SYNAPTICS_WRITE(hTouch, RMI_INTERRUPT_ENABLE_0, INT_ENABLE_0_GPIO_EN_BIT_MASK))
    {
        NvOdmGpioInterruptUnregister(hTouch->hGpio, hTouch->hPin, hTouch->hGpioIntr);
        hTouch->hGpioIntr = NULL;
        return NV_FALSE;
    }

    return NV_TRUE;
}

NvBool Synaptics_HandleInterrupt(NvOdmTouchDeviceHandle hDevice)
{
    Synaptics_TouchDevice *hTouch = (Synaptics_TouchDevice *)hDevice;
    NvU32 pinValue;

    if (hTouch)
    {
        NvOdmGpioGetState(hTouch->hGpio, hTouch->hPin, &pinValue);
        if (!pinValue)
        {
            //interrupt pin is still LOW, read data until interrupt pin is released.
            return NV_FALSE;
        }
        else
            NvOdmGpioInterruptDone(hTouch->hGpioIntr);
    }

    return NV_TRUE;
}

NvBool Synaptics_GetSampleRate(NvOdmTouchDeviceHandle hDevice, NvOdmTouchSampleRate* pTouchSampleRate)
{
    if (pTouchSampleRate)
    {
        pTouchSampleRate->NvOdmTouchSampleRateHigh = SAMPLES_PER_SECOND;
        pTouchSampleRate->NvOdmTouchSampleRateLow = SAMPLES_PER_SECOND;
        pTouchSampleRate->NvOdmTouchCurrentSampleRate = 0; // 0 = low , 1 = high
    }

    return NV_TRUE;
}

NvBool Synaptics_SetSampleRate(NvOdmTouchDeviceHandle hDevice, NvU32 rate)
{
    Synaptics_TouchDevice *hTouch = (Synaptics_TouchDevice *)hDevice;

    if (hTouch)
    {
        if (rate > SAMPLES_PER_SECOND)
            hTouch->SampleRate = SAMPLES_PER_SECOND;
        else
            hTouch->SampleRate = rate;
    }

    return NV_TRUE;
}

NvBool Synaptics_PowerControl(NvOdmTouchDeviceHandle hDevice, NvOdmTouchPowerModeType mode)
{
    Synaptics_TouchDevice *hTouch = (Synaptics_TouchDevice *)hDevice;
    NvU8 SleepMode;

    NV_ASSERT(hTouch->VddId != 0xFF);

    switch(mode)
    {
        case NvOdmTouch_PowerMode_0:
            SleepMode = SLEEP_MODE_NORMAL;
            break;
        case NvOdmTouch_PowerMode_1:
        case NvOdmTouch_PowerMode_2:
        case NvOdmTouch_PowerMode_3:
            SleepMode = SLEEP_MODE_SENSOR_SLEEP;
            break;
        default:
            return NV_FALSE;
    }

    if (hTouch->SleepMode == SleepMode)
        return NV_TRUE;

    if (!SYNAPTICS_WRITE(hTouch, RMI_DEVICE_CONTROL, SleepMode))
        return NV_FALSE;

    hTouch->SleepMode = SleepMode;
    return NV_TRUE;
}

NvBool Synaptics_GetCalibrationData(NvOdmTouchDeviceHandle hDevice, NvU32 NumOfCalibrationData, NvS32* pRawCoordBuffer)
{
#if SYNAPTICS_SCREEN_ANGLE
    //Portrait
    static const NvS32 RawCoordBuffer[] = {2054, 3624, 3937, 809, 3832, 6546, 453, 6528, 231, 890};
#else
    //Landscape
    static NvS32 RawCoordBuffer[] = {2054, 3624, 3832, 6546, 453, 6528, 231, 890, 3937, 809};
#endif

    if (!pRawCoordBuffer)
        return NV_FALSE;

    if (NumOfCalibrationData * 2 != (sizeof(RawCoordBuffer) / sizeof(NvS32)))
    {
        NVODMTOUCH_PRINTF(("WARNING: number of calibration data isn't matched\n"));
        return NV_FALSE;
    }

    NvOdmOsMemcpy(pRawCoordBuffer, RawCoordBuffer, sizeof(RawCoordBuffer));

    return NV_TRUE;
}

