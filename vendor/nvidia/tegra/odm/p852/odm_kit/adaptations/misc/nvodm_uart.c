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
 * @file          nvodm_uart.c
 * @brief         <b>Adaptation for uart </b>
 *
 * @Description : Implementation of the uart adaptation.
 */
#include "nvodm_uart.h"
#include "nvodm_services.h"
#include "nvodm_query_discovery.h"
#include "nvodm_services.h"
#include "nvos.h"
#include "nvodm_pmu.h"

#ifdef NV_DRIVER_DEBUG
    #define NV_DRIVER_TRACE NvOsDebugPrintf
#else
    #define NV_DRIVER_TRACE (void)
#endif

typedef struct NvOdmUartRec
{
    // NvODM PMU device handle
    NvOdmServicesPmuHandle hPmu;
    // Gpio Handle
    NvOdmServicesGpioHandle hGpio;
    // Pin handle to Bluetooth Reset Gpio pin
    NvOdmGpioPinHandle hResetPin;
    NvOdmPeripheralConnectivity *pConnectivity;
} NvOdmUart;

NvOdmUartHandle NvOdmUartOpen(NvU32 Instance)
{
    NvOdmUart *pDevice = NULL;
    NvOdmPeripheralConnectivity *pConnectivity;
    NvU32 NumOfGuids = 1;
    NvU64 guid;
    NvU32 searchVals[2];
    const NvOdmPeripheralSearch searchAttrs[] =
    {
        NvOdmPeripheralSearch_IoModule,
        NvOdmPeripheralSearch_Instance,
    };

    searchVals[0] =  NvOdmIoModule_Uart;
    searchVals[1] =  Instance;

    NumOfGuids = NvOdmPeripheralEnumerate(
                                                    searchAttrs,
                                                    searchVals,
                                                    2,
                                                    &guid,
                                                    NumOfGuids);

    pConnectivity = (NvOdmPeripheralConnectivity *)NvOdmPeripheralGetGuid(guid);
    if (pConnectivity == NULL)
        goto ExitUartOdm;

    pDevice = NvOdmOsAlloc(sizeof(NvOdmUart));
    if(pDevice == NULL)
        goto ExitUartOdm;

    pDevice->hPmu = NvOdmServicesPmuOpen();
    if(pDevice->hPmu == NULL)
    {
        goto ExitUartOdm;
    }

    // Switch On UART Interface

    pDevice->pConnectivity = pConnectivity;

    return pDevice;

ExitUartOdm:
    NvOdmOsFree(pDevice);
    pDevice = NULL;

    return NULL;
}

void NvOdmUartClose(NvOdmUartHandle hOdmUart)
{

    if (hOdmUart)
    {
        // Switch OFF UART Interface

        if (hOdmUart->hPmu != NULL)
        {
            NvOdmServicesPmuClose(hOdmUart->hPmu);
        }
        NvOdmOsFree(hOdmUart);
        hOdmUart = NULL;
    }
}

NvBool NvOdmUartSuspend(NvOdmUartHandle hOdmUart)
{
    return NV_FALSE;
}

NvBool NvOdmUartResume(NvOdmUartHandle hOdmUart)
{
    return NV_FALSE;
}

