/*
 * Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "flash_ssl3250a.h"
#include "imager_util.h"
#include "nvodm_services.h"
#include "nvodm_query_discovery_imager.h"

#define NUM_CTRL_PINS_SSL3250A 2
#define NUM_FLASH_LEVELS    (1 << NUM_CTRL_PINS_SSL3250A)

typedef struct NvOdmFlashSSL3250AContextRec
{
    NvOdmImagerI2cConnection I2c;
    ImagerGpioConfig GpioConfig;
    NvOdmImagerPowerLevel PowerLevel;

    NvOdmImagerFlashPinState DesiredPinstate;

} NvOdmFlashSSL3250AContext;

static const NvOdmImagerFlashCapabilities FlashCapabilities =
{
    4,
    {
        { 0.f, 0xFFFFFFFF,   0 },
        { 2.f,       1000,   1 },
        { 5.f,       1000,   1 },
        { 7.f,        500,   2 }
    }
};

// Which pins should be active for a given flash level
// For example, if for levels[2] pin[0] in NvOdmFlashSSL3250AState
// arrays should be turned off and pin[1] should be turned on,
// value at index 2 would be 0010b (0x2).
static const NvU32 LevelIndexToPinstate[NUM_FLASH_LEVELS] =
{
    0, 1, 2, 3
};


static NvBool FlashSSL3250A_Open(NvOdmImagerHandle hImager);

static void FlashSSL3250A_Close(NvOdmImagerHandle hImager);

static void
FlashSSL3250A_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities);

static NvBool
FlashSSL3250A_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel);

static NvBool
FlashSSL3250A_SetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    const void *pValue);

static NvBool FlashSSL3250A_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue);

static NvBool FlashSSL3250AInitPins(NvOdmImagerHandle hImager)
{
    NvOdmFlashSSL3250AContext *pContext =
        (NvOdmFlashSSL3250AContext *)hImager->pFlash->pPrivateContext;

    const NvOdmIoAddress *pIOAddr = NULL;
    const NvOdmPeripheralConnectivity *pConnections =
        hImager->pFlash->pConnections;
    NvOdmImagerGpio ImagerGpio = 0;
    NvU32 i = 0;

    if (!pConnections)
    {
        return NV_FALSE;
    }

    for (i = 0; i < pConnections->NumAddress; i++)
    {
        pIOAddr = &pConnections->AddressList[i];

        if (pIOAddr->Interface != NvOdmIoModule_Gpio)
        {
            goto fail;
        }

        switch(NVODM_IMAGER_FIELD(pIOAddr->Address))
        {
            case NVODM_IMAGER_FIELD(NVODM_IMAGER_FLASH0):
                ImagerGpio = NvOdmImagerGpio_FlashLow;
                break;

            case NVODM_IMAGER_FIELD(NVODM_IMAGER_FLASH1):
                ImagerGpio = NvOdmImagerGpio_FlashHigh;
                break;

            default:
                goto fail;
        }

        pContext->GpioConfig.Gpios[ImagerGpio].Port = pIOAddr->Instance;
        pContext->GpioConfig.Gpios[ImagerGpio].Pin =
            NVODM_IMAGER_CLEAR(pIOAddr->Address);
        pContext->GpioConfig.Gpios[ImagerGpio].Enable = NV_TRUE;

        // All pins are active high for current devices.
        // If we ever have a device that has active low pins, the query data
        // will need to be expanded to include this information.
        pContext->GpioConfig.Gpios[ImagerGpio].ActiveHigh = NV_TRUE;
        pContext->GpioConfig.Gpios[ImagerGpio].HandleIndex =
            pContext->GpioConfig.PinsUsed;
        pContext->GpioConfig.PinsUsed++;
    }

    return NV_TRUE;

fail:

    return NV_FALSE;

}

static NvBool FlashSSL3250A_Open(NvOdmImagerHandle hImager)
{
    NvOdmFlashSSL3250AContext *pContext = NULL;

    if (!hImager || !hImager->pFlash)
        return NV_FALSE;

    pContext = (NvOdmFlashSSL3250AContext *)
        NvOdmOsAlloc(sizeof(NvOdmFlashSSL3250AContext));
    if (!pContext)
    {
        return NV_FALSE;
    }

    NvOdmOsMemset(pContext, 0, sizeof(NvOdmFlashSSL3250AContext));

    pContext->PowerLevel = NvOdmImagerPowerLevel_Off;

    hImager->pFlash->pPrivateContext = pContext;

    return NV_TRUE;
}

static void FlashSSL3250A_Close(NvOdmImagerHandle hImager)
{
    NvOdmFlashSSL3250AContext *pContext = NULL;

    if (!hImager || ! hImager->pFlash || !hImager->pFlash->pPrivateContext)
        return;

    pContext = (NvOdmFlashSSL3250AContext *)hImager->pFlash->pPrivateContext;

    if (pContext->PowerLevel != NvOdmImagerPowerLevel_Off)
    {
        (void)FlashSSL3250A_SetPowerLevel(hImager, NvOdmImagerPowerLevel_Off);
    }
    NvOdmOsFree(pContext);
    hImager->pFlash->pPrivateContext = NULL;
}


static void
FlashSSL3250A_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities)
{

}

static NvBool
FlashSSL3250A_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel)
{
    NvBool Result = NV_TRUE;
    NvOdmFlashSSL3250AContext *pContext =
        (NvOdmFlashSSL3250AContext *)hImager->pFlash->pPrivateContext;

    if (pContext->PowerLevel == PowerLevel)
        return NV_TRUE;

    switch (PowerLevel)
    {
        case NvOdmImagerPowerLevel_On:
            Result = FlashSSL3250AInitPins(hImager);
            break;

        case NvOdmImagerPowerLevel_Off:
        case NvOdmImagerPowerLevel_Standby:
        default:
            break;
    }

    if (Result)
    {
        pContext->PowerLevel = PowerLevel;
    }

    return Result;
}

static NvBool
FlashSSL3250A_SetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    const void *pValue)
{
    NvBool Result = NV_TRUE;
    NvOdmFlashSSL3250AContext *pContext =
        (NvOdmFlashSSL3250AContext *)hImager->pFlash->pPrivateContext;
    NvU32 i = 0;
    NvF32 FloatVal = *(NvF32*)pValue;
    NvU32 DesiredLevel = 0;
    NvOdmImagerGpio FlashGpio[2] = {NvOdmImagerGpio_FlashLow,
                                    NvOdmImagerGpio_FlashHigh};

    if (pContext->PowerLevel != NvOdmImagerPowerLevel_On)
        return NV_FALSE;

    switch (Param)
    {
        case NvOdmImagerParameter_FlashLevel:
            // Find requested level, or return false if invalid request
            for (DesiredLevel = 0; DesiredLevel < NUM_FLASH_LEVELS;
                 DesiredLevel++)
            {
                if (FloatVal ==
                    FlashCapabilities.levels[DesiredLevel].guideNum)
                {
                    break;
                }
            }

            if (DesiredLevel >= NUM_FLASH_LEVELS)
                return NV_FALSE;

            // Clear pin state
            NvOdmOsMemset(&pContext->DesiredPinstate, 0,
                sizeof(NvOdmImagerFlashPinState));

            for (i = 0; i < NV_ARRAY_SIZE(FlashGpio); i++)
            {
                NvBool EnableThisPin = (LevelIndexToPinstate[DesiredLevel] &
                                        (1 << i)) ? 1 : 0;

                // High if activeHigh && enable or !activeHigh && !enable
                NvBool DriveHigh =
                    pContext->GpioConfig.Gpios[FlashGpio[i]].ActiveHigh ^
                    !EnableThisPin;

                pContext->DesiredPinstate.mask |=
                    1 << pContext->GpioConfig.Gpios[FlashGpio[i]].Pin;

                if (DriveHigh)
                {
                    pContext->DesiredPinstate.values |=
                        1 << pContext->GpioConfig.Gpios[FlashGpio[i]].Pin;
                }
            }
            break;
        default:
            NV_ASSERT(!"Unexpected SetParameter!");
            Result = NV_FALSE;
            break;
    }

    return Result;
}

static NvBool
FlashSSL3250A_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue)
{
    NvBool Result = NV_TRUE;
    NvOdmFlashSSL3250AContext *pContext =
        (NvOdmFlashSSL3250AContext *)hImager->pFlash->pPrivateContext;

    if (pContext->PowerLevel != NvOdmImagerPowerLevel_On)
        return NV_FALSE;

    switch(Param)
    {
        case NvOdmImagerParameter_FlashCapabilities:
            if (SizeOfValue != sizeof(NvOdmImagerFlashCapabilities))
                return NV_FALSE;

            NvOdmOsMemcpy(pValue, &FlashCapabilities,
                sizeof(NvOdmImagerFlashCapabilities));

            break;

        case NvOdmImagerParameter_FlashPinState:
            if (SizeOfValue != sizeof(NvOdmImagerFlashPinState))
                return NV_FALSE;

            NvOdmOsMemcpy(pValue, &pContext->DesiredPinstate,
                sizeof(NvOdmImagerFlashPinState));

            break;

        default:
            return NV_FALSE;
    }
    return Result;
}


NvBool FlashSSL3250A_GetHal(NvOdmImagerHandle hImager)
{
    if (!hImager || !hImager->pFlash)
        return NV_FALSE;

    hImager->pFlash->pfnOpen = FlashSSL3250A_Open;
    hImager->pFlash->pfnClose = FlashSSL3250A_Close;
    hImager->pFlash->pfnGetCapabilities = FlashSSL3250A_GetCapabilities;
    hImager->pFlash->pfnSetPowerLevel = FlashSSL3250A_SetPowerLevel;
    hImager->pFlash->pfnSetParameter = FlashSSL3250A_SetParameter;
    hImager->pFlash->pfnGetParameter = FlashSSL3250A_GetParameter;

    return NV_TRUE;
}
