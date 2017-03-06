/*
 * Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file          nvodm_usbulpi.c
 * @brief         <b>Adaptation for USB ULPI </b>
 *
 * @Description : Implementation of the USB ULPI adaptation.
 */
#include "nvodm_usbulpi.h"
#include "nvodm_services.h"
#include "nvodm_query_discovery.h"
#include "nvodm_services.h"
#include "nvos.h"

#define SMSC3317GUID NV_ODM_GUID('s','m','s','c','3','3','1','7')

#define MAX_CLOCKS 3

#define NVODM_PORT(x) ((x) - 'a')


typedef struct NvOdmUsbUlpiRec
{
     NvU64    CurrentGUID;
} NvOdmUsbUlpi;

NvOdmUsbUlpiHandle NvOdmUsbUlpiOpen(NvU32 Instance)
{
    NvOdmUsbUlpi *pDevice = NULL;
    NvU32 ClockInstances[MAX_CLOCKS];
    NvU32 ClockFrequencies[MAX_CLOCKS];
    NvU32 NumClocks;
    NvOdmServicesGpioHandle hGpio;
    NvOdmGpioPinHandle hResetPin;
    NvU32 Port = NVODM_PORT('v');
    NvU32 Pin = 1;

    pDevice = NvOdmOsAlloc(sizeof(NvOdmUsbUlpi));
    if (pDevice == NULL)
        return NULL;

    if(!NvOdmExternalClockConfig(SMSC3317GUID, NV_FALSE,
                                 ClockInstances, ClockFrequencies, &NumClocks))
    {
        NvOdmOsDebugPrintf("NvOdmUsbUlpiOpen: NvOdmExternalClockConfig fail\n");
        goto ExitUlpiOdm;
    }
    NvOdmOsSleepMS(10);
    // Pull high on RESETB ( 22nd pin of smsc3315)
    hGpio = NvOdmGpioOpen();
    hResetPin = NvOdmGpioAcquirePinHandle(hGpio, Port, Pin);
    // config as out put pin
    NvOdmGpioConfig(hGpio,hResetPin, NvOdmGpioPinMode_Output);
    // Set low to write high on ULPI_RESETB pin
    NvOdmGpioSetState(hGpio, hResetPin, 0x01);
    NvOdmGpioSetState(hGpio, hResetPin, 0x0);
    NvOdmOsSleepMS(5);
    NvOdmGpioSetState(hGpio, hResetPin, 0x01);

    pDevice->CurrentGUID = SMSC3317GUID;
    return pDevice;

ExitUlpiOdm:
    NvOdmOsFree(pDevice);
    return NULL;
}

void NvOdmUsbUlpiClose(NvOdmUsbUlpiHandle hOdmUlpi)
{
    if (hOdmUlpi)
    {
        NvOdmOsFree(hOdmUlpi);
        hOdmUlpi = NULL;
    }
}

