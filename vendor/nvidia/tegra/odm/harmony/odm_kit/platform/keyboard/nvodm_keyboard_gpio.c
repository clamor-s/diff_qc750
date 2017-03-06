/*
 * Copyright (c) 2007 - 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_keyboard.h"
#include "nvodm_query_discovery.h"
#include "nvodm_services.h"
#include "nvodm_query_gpio.h"
#include "nvodm_keyboard_priv.h"

#define KEY_PRESS_FLAG 2
#define KEY_RELEASE_FLAG 1
#define MAX_GPIO_KEYS 10
typedef struct NvOdmKbdContextRec
{
    const NvOdmGpioPinInfo *GpioPinInfo; // GPIO info struct
    NvOdmGpioPinHandle hOdmPins[MAX_GPIO_KEYS]; // array of gpio pin handles
    NvU32 PinCount;
    NvOdmServicesGpioHandle hOdmGpio;
    NvOdmOsMutexHandle hKeyEventMutex;
    NvOdmOsSemaphoreHandle hEventSema;
    NvOdmServicesGpioIntrHandle IntrHandle[MAX_GPIO_KEYS];
    NvU32 NewState[MAX_GPIO_KEYS];
    NvU32 ReportedState[MAX_GPIO_KEYS];
} NvOdmKbdContext, *NvOdmKbdContextHandle;

static NvOdmKbdContext s_OdmKbdContext;

static void KeyPressGpioInterruptHandler(void *arg)
{
    NvOdmKbdContextHandle hOdmKeys = (NvOdmKbdContextHandle)&s_OdmKbdContext;
    NvU32 CurrSelectPinState;
    int index = (int)arg;

    NvOdmOsMutexLock(hOdmKeys->hKeyEventMutex);
    NvOdmGpioGetState(hOdmKeys->hOdmGpio, hOdmKeys->hOdmPins[index], &CurrSelectPinState);

    if (CurrSelectPinState)
    {
        if (hOdmKeys->GpioPinInfo[index].activeState ==
                                NvOdmGpioPinActiveState_High)
            hOdmKeys->NewState[index] =  KEY_PRESS_FLAG;
        else
            hOdmKeys->NewState[index] =  KEY_RELEASE_FLAG;
    }
    else
    {
        if (hOdmKeys->GpioPinInfo[index].activeState ==
                                NvOdmGpioPinActiveState_Low)
            hOdmKeys->NewState[index] =  KEY_PRESS_FLAG;
        else
            hOdmKeys->NewState[index] =  KEY_RELEASE_FLAG;
    }

    NvOdmOsMutexUnlock(hOdmKeys->hKeyEventMutex);
    NvOdmOsSemaphoreSignal(hOdmKeys->hEventSema);
    NvOdmGpioInterruptDone(hOdmKeys->IntrHandle[index]);
}

static NvBool NvOdmKeyboardInit_Gpio(void)
{
    NvU32 i;
    NvOdmInterruptHandler gpioIntrHandler = (NvOdmInterruptHandler)KeyPressGpioInterruptHandler;
    NvOdmGpioPinKeyInfo *pGpioPinKeyInfo = NULL;

    s_OdmKbdContext.GpioPinInfo = NvOdmQueryGpioPinMap(NvOdmGpioPinGroup_keypadMisc,
                                    0, &s_OdmKbdContext.PinCount);
    if (!s_OdmKbdContext.GpioPinInfo)
        return NV_FALSE;

    if (s_OdmKbdContext.PinCount > MAX_GPIO_KEYS)
        return NV_FALSE;

    s_OdmKbdContext.hKeyEventMutex = NvOdmOsMutexCreate();
    if (!s_OdmKbdContext.hKeyEventMutex)
        return NV_FALSE;

    s_OdmKbdContext.hEventSema = NvOdmOsSemaphoreCreate(0);
    if (!s_OdmKbdContext.hEventSema)
    {
        NvOdmOsMutexDestroy(s_OdmKbdContext.hKeyEventMutex);
        s_OdmKbdContext.hKeyEventMutex = NULL;
        return NV_FALSE;
    }

    // Getting the OdmGpio Handle
    s_OdmKbdContext.hOdmGpio = NvOdmGpioOpen();
    if (!s_OdmKbdContext.hOdmGpio)
        goto FailGpio;

    for (i=0; i < s_OdmKbdContext.PinCount; ++i)
    {
        s_OdmKbdContext.hOdmPins[i] = NULL;
        s_OdmKbdContext.IntrHandle[i] = NULL;
        s_OdmKbdContext.NewState[i] = 0;
        s_OdmKbdContext.ReportedState[i] = 0;
    }

    for (i=0; i < s_OdmKbdContext.PinCount; ++i)
    {
        s_OdmKbdContext.hOdmPins[i] = NvOdmGpioAcquirePinHandle(
                                        s_OdmKbdContext.hOdmGpio,
                                        s_OdmKbdContext.GpioPinInfo[i].Port,
                                        s_OdmKbdContext.GpioPinInfo[i].Pin);
        if (s_OdmKbdContext.hOdmPins[i] == NULL)
            goto FailAcqPin;
    }

    for (i=0; i < s_OdmKbdContext.PinCount; ++i)
        NvOdmGpioConfig(s_OdmKbdContext.hOdmGpio, s_OdmKbdContext.hOdmPins[i],
                            NvOdmGpioPinMode_InputData);

    for (i=0; i < s_OdmKbdContext.PinCount; ++i)
    {
        pGpioPinKeyInfo = s_OdmKbdContext.GpioPinInfo[i].GpioPinSpecificData;
        if (NvOdmGpioInterruptRegister(s_OdmKbdContext.hOdmGpio,
            &s_OdmKbdContext.IntrHandle[i], s_OdmKbdContext.hOdmPins[i],
            NvOdmGpioPinMode_InputInterruptAny, gpioIntrHandler, (void *)(i),
            pGpioPinKeyInfo->DebounceTimeMs) == NV_FALSE)
        {
            goto FailIntReg;
        }
    }

    return NV_TRUE;

FailIntReg:
    for (i=0; i < s_OdmKbdContext.PinCount; ++i)
    {
        if (s_OdmKbdContext.IntrHandle[i])
            NvOdmGpioInterruptUnregister(s_OdmKbdContext.hOdmGpio,
                    s_OdmKbdContext.hOdmPins[i], s_OdmKbdContext.IntrHandle[i]);

        s_OdmKbdContext.IntrHandle[i] = NULL;
    }

FailAcqPin:
    for (i=0; i < s_OdmKbdContext.PinCount; ++i)
    {
        if (s_OdmKbdContext.hOdmPins[i])
            NvOdmGpioReleasePinHandle(s_OdmKbdContext.hOdmGpio,
                                        s_OdmKbdContext.hOdmPins[i]);
        s_OdmKbdContext.hOdmPins[i] = NULL;
    }

    NvOdmGpioClose(s_OdmKbdContext.hOdmGpio);
    s_OdmKbdContext.hOdmGpio = NULL;

FailGpio:
    NvOdmOsMutexDestroy(s_OdmKbdContext.hKeyEventMutex);
    s_OdmKbdContext.hKeyEventMutex = NULL;
    NvOdmOsSemaphoreDestroy(s_OdmKbdContext.hEventSema);
    s_OdmKbdContext.hEventSema = NULL;
    return NV_FALSE;
}

static void NvOdmKeyboardDeInit_Gpio(void)
{
    NvU32 i;
    for (i=0; i < s_OdmKbdContext.PinCount; ++i)
    {
        if (s_OdmKbdContext.IntrHandle[i])
            NvOdmGpioInterruptUnregister(s_OdmKbdContext.hOdmGpio,
                    s_OdmKbdContext.hOdmPins[i], s_OdmKbdContext.IntrHandle[i]);

        s_OdmKbdContext.IntrHandle[i] = NULL;
    }

    for (i=0; i < s_OdmKbdContext.PinCount; ++i)
    {
        if (s_OdmKbdContext.hOdmPins[i])
            NvOdmGpioReleasePinHandle(s_OdmKbdContext.hOdmGpio,
                                        s_OdmKbdContext.hOdmPins[i]);
        s_OdmKbdContext.hOdmPins[i] = NULL;
    }

    NvOdmGpioClose(s_OdmKbdContext.hOdmGpio);
    s_OdmKbdContext.hOdmGpio = NULL;

    NvOdmOsMutexDestroy(s_OdmKbdContext.hKeyEventMutex);
    s_OdmKbdContext.hKeyEventMutex = NULL;
    NvOdmOsSemaphoreDestroy(s_OdmKbdContext.hEventSema);
    s_OdmKbdContext.hEventSema = NULL;

    for (i=0; i < s_OdmKbdContext.PinCount; ++i)
    {
        s_OdmKbdContext.NewState[i] = 0;
        s_OdmKbdContext.ReportedState[i] = 0;
    }
}

/* Gets the actual scan code for a key press */
static NvBool NvOdmKeyboardGetKeyData_Gpio(NvU32 *pKeyScanCode,
                            NvU8 *pScanCodeFlags, NvU32 Timeout)
{
    NvU32 LoopCount;
    NvBool IsReport= NV_FALSE;
    NvOdmGpioPinKeyInfo *pGpioPinKeyInfo = NULL;

    if (!pKeyScanCode || !pScanCodeFlags || !s_OdmKbdContext.hEventSema)
        return NV_FALSE;
    *pScanCodeFlags = 0;
    *pKeyScanCode = 0;

    if (Timeout != 0)
    {
        /* Use the timeout value */
        if (!NvOdmOsSemaphoreWaitTimeout(s_OdmKbdContext.hEventSema, Timeout))
            return NV_FALSE; // timed out
    }
    else
    {
        /* Wait till we get key press*/
        NvOdmOsSemaphoreWait(s_OdmKbdContext.hEventSema);
    }

    NvOdmOsMutexLock(s_OdmKbdContext.hKeyEventMutex);
    for (LoopCount = 0; LoopCount < s_OdmKbdContext.PinCount; ++LoopCount)
    {
        if (s_OdmKbdContext.NewState[LoopCount] !=
                            s_OdmKbdContext.ReportedState[LoopCount])
        {
            pGpioPinKeyInfo = s_OdmKbdContext.GpioPinInfo[LoopCount].GpioPinSpecificData;
            *pKeyScanCode = pGpioPinKeyInfo->Code;
            *pScanCodeFlags = s_OdmKbdContext.NewState[LoopCount];
            s_OdmKbdContext.ReportedState[LoopCount] =
                                        s_OdmKbdContext.NewState[LoopCount];
            IsReport = NV_TRUE;
            break;
        }
    }
    NvOdmOsMutexUnlock(s_OdmKbdContext.hKeyEventMutex);
    return IsReport;
}

void InitGpioKeyboardInterface(OdmKeyBoardIntHandle hOdmKbInt)
{
    hOdmKbInt->KeyboardInit = NvOdmKeyboardInit_Gpio;
    hOdmKbInt->KeyboardDeInit = NvOdmKeyboardDeInit_Gpio;
    hOdmKbInt->KeyboardGetKeyData = NvOdmKeyboardGetKeyData_Gpio;
}

