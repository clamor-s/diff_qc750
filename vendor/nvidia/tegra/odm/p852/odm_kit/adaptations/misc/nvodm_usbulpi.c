/*
 * Copyright (c) 2007-2011 NVIDIA Corporation.  All rights reserved.
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

#define SMSC83340GUID NV_ODM_GUID('s','m','s','8','3','3','4','0')

#ifdef NV_DRIVER_DEBUG
    #define NV_DRIVER_TRACE NvOsDebugPrintf
#else
    #define NV_DRIVER_TRACE (void)
#endif

typedef struct NvOdmUsbUlpiRec
{
    NvU64    CurrentGUID;

    // Gpio Handle
    NvOdmServicesGpioHandle hGpio;
    // Pin handle to ULPI Reset Gpio pin
    NvOdmGpioPinHandle hResetPin;
} NvOdmUsbUlpi;

static NvBool UlpiSetPowerOn(NvOdmUsbUlpiHandle hOdmUlpi, NvBool IsEnable)
{
    if (IsEnable)
    {
        // Power On Reset Sequence
        NvOdmGpioSetState(hOdmUlpi->hGpio, hOdmUlpi->hResetPin, 0x0); //RST -> Low
        NvOdmOsWaitUS(1);
        NvOdmGpioSetState(hOdmUlpi->hGpio, hOdmUlpi->hResetPin, 0x1); //RST -> High
     }
     else
     {
         // Power Off sequence
         NvOdmGpioSetState(hOdmUlpi->hGpio, hOdmUlpi->hResetPin, 0x0); //RST -> Low
     }

    return NV_TRUE;
}

NvOdmUsbUlpiHandle NvOdmUsbUlpiOpen(NvU32 Instance)
{
    static NvOdmUsbUlpi *pDevice = NULL;
    NvOdmPeripheralConnectivity *pConnectivity;
    NvBool Status = NV_TRUE;

    // Get the peripheral connectivity information
    pConnectivity =
        (NvOdmPeripheralConnectivity *)NvOdmPeripheralGetGuid(SMSC83340GUID);
    if (pConnectivity == NULL ||
        pConnectivity->NumAddress == 0 ||
        pConnectivity->AddressList[0].Interface != NvOdmIoModule_Gpio)
    {
        return NULL;
    }

    pDevice = NvOdmOsAlloc(sizeof(NvOdmUsbUlpi));
    if(pDevice == NULL)
        goto ExitUlpiOdm;
    NvOdmOsMemset(pDevice, 0, sizeof(NvOdmUsbUlpi));

    pDevice->hGpio = NvOdmGpioOpen();
    if (pDevice->hGpio == NULL)
        goto ExitUlpiOdm;

    // Acquiring Pin Handles for Reset Pin
    pDevice->hResetPin= NvOdmGpioAcquirePinHandle(pDevice->hGpio,
        pConnectivity->AddressList[0].Instance,
        pConnectivity->AddressList[0].Address);

    // Setting Reset pin to output
    NvOdmGpioConfig(pDevice->hGpio, pDevice->hResetPin, NvOdmGpioPinMode_Output);

    // Setting Reset pin to low (active)
    NvOdmGpioSetState(pDevice->hGpio, pDevice->hResetPin, 0x0);

    // Apply Reset and keep it high
    Status = UlpiSetPowerOn(pDevice, NV_TRUE);
    if (Status != NV_TRUE)
        goto ExitUlpiOdm;

    pDevice->CurrentGUID = SMSC83340GUID;
    return pDevice;

ExitUlpiOdm:
    if (pDevice->hResetPin)
        NvOdmGpioReleasePinHandle(pDevice->hGpio, pDevice->hResetPin);
    if (pDevice->hGpio)
        NvOdmGpioClose(pDevice->hGpio);
    NvOdmOsFree(pDevice);
    pDevice = NULL;

    return NULL;
}

void NvOdmUsbUlpiClose(NvOdmUsbUlpiHandle hOdmUlpi)
{
    if (hOdmUlpi)
    {
        NvOdmGpioReleasePinHandle(hOdmUlpi->hGpio, hOdmUlpi->hResetPin);
        NvOdmGpioClose(hOdmUlpi->hGpio);
        NvOdmOsFree(hOdmUlpi);
        hOdmUlpi = NULL;
    }
}

