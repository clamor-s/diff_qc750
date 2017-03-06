/*
 * Copyright (c) 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvodm_scrollwheel.h"
#include "nvodm_services.h"
#include "nvodm_query_discovery.h"
#include "nvodm_query.h"
#include "nvodm_query_gpio.h"

#define DEBOUNCE_TIME_MS 0

typedef struct NvOdmScrollWheelRec
{
    // Gpio Handle
    NvOdmServicesGpioHandle hGpio;
    // Pin handles to all the 4 Gpio pins
    NvOdmGpioPinHandle hInputPin1;
    NvOdmGpioPinHandle hInputPin2;
    NvOdmGpioPinHandle hSelectPin;
    NvOdmGpioPinHandle hOnOffPin;
    // Stores the key events the client had registered for.
    NvOdmScrollWheelEvent RegisterEvents;
    NvOdmOsSemaphoreHandle EventSema;
    NvOdmServicesGpioIntrHandle IntrHandle[2];
    NvOdmScrollWheelEvent Event;
    NvU32 LastPin1Val;
    NvOdmOsMutexHandle hKeyEventMutex;
    NvOdmOsThreadHandle hDebounceRotThread;
    NvOdmOsSemaphoreHandle hDebounceRotSema;
    NvOdmOsSemaphoreHandle hDummySema;
    volatile NvU32 shutdown;
    volatile NvBool Debouncing;
} NvOdmScrollWheel;

static NvU32
GetReliablePinValue(NvOdmServicesGpioHandle hGpio,
                    NvOdmGpioPinHandle hPin)
{
    NvU32 data = (NvU32)-1;
    const int sampleCount = 10;
    int i = 0;

    while (i < sampleCount)
    {
        NvU32 currData;
        NvOdmGpioGetState(hGpio, hPin, &currData);
        if (currData == data)
        {
            i++;
        }
        else
        {
            data = currData;
            i = 0;
        }
    }

    return data;
}

static void
ScrollWheelDebounceRotThread(void *arg)
{
    NvOdmScrollWheelHandle hOdmScrollWheel = (NvOdmScrollWheelHandle)arg;
    const NvU32 debounceTimeMS = 2;

    while (!hOdmScrollWheel->shutdown)
    {
        //  If a scroll wheel event is detected, wait <debounceTime> milliseconds
        //  and then read the Terminal 1 pin to determine the current level
        NvOdmOsSemaphoreWait(hOdmScrollWheel->hDebounceRotSema);
        // The dummy semaphore never gets signalled so it will always timeout
        NvOdmOsSemaphoreWaitTimeout(hOdmScrollWheel->hDummySema, debounceTimeMS);
        hOdmScrollWheel->LastPin1Val = GetReliablePinValue(hOdmScrollWheel->hGpio, hOdmScrollWheel->hInputPin1);
        NvOdmGpioConfig(hOdmScrollWheel->hGpio,
                        hOdmScrollWheel->hInputPin1,
                        hOdmScrollWheel->LastPin1Val ?
                        NvOdmGpioPinMode_InputInterruptFallingEdge :
                        NvOdmGpioPinMode_InputInterruptRisingEdge);
        hOdmScrollWheel->Debouncing = NV_FALSE;
    }
}

static void RotGpioInterruptHandler(void *arg)
{
    NvOdmScrollWheelHandle hOdmScrollWheel = (NvOdmScrollWheelHandle)arg;
    NvU32 InPinValue2;
    NvOdmScrollWheelEvent Event = NvOdmScrollWheelEvent_None;

    /* if still debouncing, ignore interrupt */
    if (hOdmScrollWheel->Debouncing)
    {
        NvOdmGpioInterruptDone(hOdmScrollWheel->IntrHandle[1]);
        return;
    }
    NvOdmGpioGetState(hOdmScrollWheel->hGpio, hOdmScrollWheel->hInputPin2, &InPinValue2);

    if (InPinValue2 == hOdmScrollWheel->LastPin1Val)
    {
        Event = NvOdmScrollWheelEvent_RotateAntiClockWise;
    }
    else
    {
        Event = NvOdmScrollWheelEvent_RotateClockWise;
    }

    Event &= hOdmScrollWheel->RegisterEvents;
    if (Event)
    {
        NvOdmOsMutexLock(hOdmScrollWheel->hKeyEventMutex);
        hOdmScrollWheel->Event &= ~(NvOdmScrollWheelEvent_RotateClockWise |
                                    NvOdmScrollWheelEvent_RotateAntiClockWise);
        hOdmScrollWheel->Event |= Event;
        NvOdmOsMutexUnlock(hOdmScrollWheel->hKeyEventMutex);
        NvOdmOsSemaphoreSignal(hOdmScrollWheel->EventSema);
    }
    /* start debounce */
    hOdmScrollWheel->Debouncing = NV_TRUE;
    NvOdmOsSemaphoreSignal(hOdmScrollWheel->hDebounceRotSema);

    NvOdmGpioInterruptDone(hOdmScrollWheel->IntrHandle[1]);
}

static void SelectGpioInterruptHandler(void *arg)
{
    NvOdmScrollWheelHandle hOdmScrollWheel = (NvOdmScrollWheelHandle)arg;
    NvU32 CurrSelectPinState;
    NvOdmScrollWheelEvent Event = NvOdmScrollWheelEvent_None;

    NvOdmGpioGetState(hOdmScrollWheel->hGpio, hOdmScrollWheel->hSelectPin, &CurrSelectPinState);
    Event = (CurrSelectPinState) ? NvOdmScrollWheelEvent_Release : NvOdmScrollWheelEvent_Press;
    Event &= hOdmScrollWheel->RegisterEvents;

    if (Event)
    {
        NvOdmOsMutexLock(hOdmScrollWheel->hKeyEventMutex);
        hOdmScrollWheel->Event &= ~(NvOdmScrollWheelEvent_Press | NvOdmScrollWheelEvent_Release);
        hOdmScrollWheel->Event |= Event;
        NvOdmOsMutexUnlock(hOdmScrollWheel->hKeyEventMutex);
        NvOdmOsSemaphoreSignal(hOdmScrollWheel->EventSema);
    }

    NvOdmGpioInterruptDone(hOdmScrollWheel->IntrHandle[0]);
}

NvOdmScrollWheelHandle
NvOdmScrollWheelOpen(
    NvOdmOsSemaphoreHandle hNotifySema,
    NvOdmScrollWheelEvent RegisterEvents)
{
    NvOdmScrollWheelHandle hOdmScroll = NULL;
    NvOdmInterruptHandler RotIntrHandler = (NvOdmInterruptHandler)RotGpioInterruptHandler;
    NvOdmInterruptHandler SelectIntrHandler = (NvOdmInterruptHandler)SelectGpioInterruptHandler;
    NvU32 PinCount;
    const NvOdmGpioPinInfo *GpioPinInfo;

    GpioPinInfo = NvOdmQueryGpioPinMap(NvOdmGpioPinGroup_ScrollWheel,
                                0, &PinCount);
    if ((GpioPinInfo == NULL) || (PinCount != 4))
        return NULL;

    hOdmScroll  = NvOdmOsAlloc(sizeof(NvOdmScrollWheel));
    if(!hOdmScroll)
        return NULL;

    NvOdmOsMemset(hOdmScroll, 0, sizeof(NvOdmScrollWheel));

    hOdmScroll->EventSema = hNotifySema;
    hOdmScroll->RegisterEvents = RegisterEvents;
    hOdmScroll->Event = NvOdmScrollWheelEvent_None;

    hOdmScroll->hKeyEventMutex = NvOdmOsMutexCreate();
    hOdmScroll->hDebounceRotSema = NvOdmOsSemaphoreCreate(0);
    hOdmScroll->hDummySema = NvOdmOsSemaphoreCreate(0);

    if (!hOdmScroll->hKeyEventMutex ||
        !hOdmScroll->hDebounceRotSema ||
        !hOdmScroll->hDummySema)
    {
        goto ErrorExit;
    }

    // Getting the OdmGpio Handle
    hOdmScroll->hGpio = NvOdmGpioOpen();
    if (!hOdmScroll->hGpio)
        goto ErrorExit;

    hOdmScroll->hDebounceRotThread =
        NvOdmOsThreadCreate((NvOdmOsThreadFunction)ScrollWheelDebounceRotThread,
                            (void*)hOdmScroll);
    if (!hOdmScroll->hDebounceRotThread)
        goto ErrorExit;

    // Acquiring Pin Handles for all the four Gpio Pins
    // First entry is ON_OFF
    // Second entry should be Select
    // Third entry should be Input 1
    // 4 th entry should be Input 2
    hOdmScroll->hOnOffPin= NvOdmGpioAcquirePinHandle(hOdmScroll ->hGpio,
                            GpioPinInfo[0].Port, GpioPinInfo[0].Pin);

    hOdmScroll->hSelectPin= NvOdmGpioAcquirePinHandle(hOdmScroll ->hGpio,
                            GpioPinInfo[1].Port, GpioPinInfo[1].Pin);

    hOdmScroll->hInputPin1= NvOdmGpioAcquirePinHandle(hOdmScroll ->hGpio,
                            GpioPinInfo[2].Port, GpioPinInfo[2].Pin);

    hOdmScroll->hInputPin2 = NvOdmGpioAcquirePinHandle(hOdmScroll ->hGpio,
                            GpioPinInfo[3].Port, GpioPinInfo[3].Pin);

    if (!hOdmScroll->hInputPin1 || !hOdmScroll->hInputPin2 ||
        !hOdmScroll->hSelectPin || !hOdmScroll->hOnOffPin)
        goto ErrorExit;

    // Setting the ON/OFF pin to output mode and setting to Low.
    NvOdmGpioConfig(hOdmScroll->hGpio, hOdmScroll->hOnOffPin, NvOdmGpioPinMode_Output);
    NvOdmGpioSetState(hOdmScroll->hGpio, hOdmScroll->hOnOffPin, 0);

    // Configuring the other pins as input
    NvOdmGpioConfig(hOdmScroll->hGpio, hOdmScroll->hSelectPin, NvOdmGpioPinMode_InputData);
    NvOdmGpioConfig(hOdmScroll->hGpio, hOdmScroll->hInputPin1, NvOdmGpioPinMode_InputData);
    NvOdmGpioConfig(hOdmScroll->hGpio, hOdmScroll->hInputPin2, NvOdmGpioPinMode_InputData);

    if (NvOdmGpioInterruptRegister(hOdmScroll->hGpio, &hOdmScroll->IntrHandle[0],
        hOdmScroll->hSelectPin, NvOdmGpioPinMode_InputInterruptAny,
        SelectIntrHandler, (void *)(hOdmScroll), DEBOUNCE_TIME_MS) == NV_FALSE)
        goto ErrorExit;

    hOdmScroll->LastPin1Val = GetReliablePinValue(hOdmScroll->hGpio, hOdmScroll->hInputPin1);

    if (NvOdmGpioInterruptRegister(hOdmScroll->hGpio, &hOdmScroll->IntrHandle[1],
        hOdmScroll->hInputPin1, hOdmScroll->LastPin1Val ?
            NvOdmGpioPinMode_InputInterruptFallingEdge :
            NvOdmGpioPinMode_InputInterruptRisingEdge,
            RotIntrHandler, (void *)(hOdmScroll), DEBOUNCE_TIME_MS) == NV_FALSE)
        goto ErrorExit;

    if (!hOdmScroll->IntrHandle[0] || !hOdmScroll->IntrHandle[1])
        goto ErrorExit;

    hOdmScroll->Debouncing = NV_FALSE;

    return hOdmScroll;

   ErrorExit:
    NvOdmScrollWheelClose(hOdmScroll);
    return NULL;
}

void NvOdmScrollWheelClose(NvOdmScrollWheelHandle hOdmScrollWheel)
{
    if (hOdmScrollWheel)
    {
        hOdmScrollWheel->shutdown = 1;

        if (hOdmScrollWheel->hDebounceRotThread)
        {
            if (hOdmScrollWheel->hDebounceRotSema)
                NvOdmOsSemaphoreSignal(hOdmScrollWheel->hDebounceRotSema);
            NvOdmOsThreadJoin(hOdmScrollWheel->hDebounceRotThread);
        }

        if (hOdmScrollWheel->hGpio)
        {
            if (hOdmScrollWheel->IntrHandle[0])
                NvOdmGpioInterruptUnregister(hOdmScrollWheel->hGpio,
                                            hOdmScrollWheel->hSelectPin,
                                            hOdmScrollWheel->IntrHandle[0]);

            if (hOdmScrollWheel->IntrHandle[1])
                NvOdmGpioInterruptUnregister(hOdmScrollWheel->hGpio,
                                            hOdmScrollWheel->hInputPin1,
                                            hOdmScrollWheel->IntrHandle[1]);

            if (hOdmScrollWheel->hOnOffPin)
                NvOdmGpioReleasePinHandle(hOdmScrollWheel->hGpio, hOdmScrollWheel->hOnOffPin);

            if (hOdmScrollWheel->hInputPin1)
                NvOdmGpioReleasePinHandle(hOdmScrollWheel->hGpio, hOdmScrollWheel->hInputPin1);

            if (hOdmScrollWheel->hInputPin2)
                NvOdmGpioReleasePinHandle(hOdmScrollWheel->hGpio, hOdmScrollWheel->hInputPin2);

            if (hOdmScrollWheel->hSelectPin)
                NvOdmGpioReleasePinHandle(hOdmScrollWheel->hGpio, hOdmScrollWheel->hSelectPin);

            NvOdmGpioClose(hOdmScrollWheel->hGpio);
        }

        if (hOdmScrollWheel->hDummySema)
            NvOdmOsSemaphoreDestroy(hOdmScrollWheel->hDummySema);

        if (hOdmScrollWheel->hDebounceRotSema)
            NvOdmOsSemaphoreDestroy(hOdmScrollWheel->hDebounceRotSema);

        if (hOdmScrollWheel->hKeyEventMutex)
            NvOdmOsMutexDestroy(hOdmScrollWheel->hKeyEventMutex);

        NvOdmOsFree(hOdmScrollWheel);
    }
}

NvOdmScrollWheelEvent NvOdmScrollWheelGetEvent(NvOdmScrollWheelHandle hOdmScrollWheel)
{
    NvOdmScrollWheelEvent Event;
    NvOdmOsMutexLock(hOdmScrollWheel->hKeyEventMutex);
    Event = hOdmScrollWheel->Event;
    hOdmScrollWheel->Event = NvOdmScrollWheelEvent_None;
    NvOdmOsMutexUnlock(hOdmScrollWheel->hKeyEventMutex);
    return Event;
}
