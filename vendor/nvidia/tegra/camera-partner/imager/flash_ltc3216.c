/*
 * Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "flash_ltc3216.h"
#include "imager_util.h"
#include "nvodm_services.h"
#include "nvodm_query_discovery_imager.h"

#define NUM_CTRL_PINS_LTC3216 2
#define NUM_FLASH_LEVELS    (1 << NUM_CTRL_PINS_LTC3216)

typedef struct NvOdmFlashLTC3216ContextRec
{
    NvOdmImagerI2cConnection I2c;
    ImagerGpioConfig GpioConfig;
    NvOdmImagerPowerLevel PowerLevel;

    NvOdmImagerFlashPinState DesiredPinstate;

} NvOdmFlashLTC3216Context;

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
// For example, if for levels[2] pin[0] in NvOdmFlashLTC3216State
// arrays should be turned off and pin[1] should be turned on,
// value at index 2 would be 0010b (0x2).
static const NvU32 LevelIndexToPinstate[NUM_FLASH_LEVELS] =
{
    0, 1, 2, 3
};


static NvBool FlashLTC3216_Open(NvOdmImagerHandle hImager);

static void FlashLTC3216_Close(NvOdmImagerHandle hImager);

static void
FlashLTC3216_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities);

static NvBool
FlashLTC3216_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel);

static NvBool
FlashLTC3216_SetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    const void *pValue);

static NvBool FlashLTC3216_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue);

static NvBool FlashLTC3216InitPins(NvOdmImagerHandle hImager)
{
    NvOdmFlashLTC3216Context *pContext =
        (NvOdmFlashLTC3216Context *)hImager->pFlash->pPrivateContext;

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

static NvBool FlashLTC3216_Open(NvOdmImagerHandle hImager)
{
    NvOdmFlashLTC3216Context *pContext = NULL;

    if (!hImager || !hImager->pFlash)
        return NV_FALSE;

    pContext = (NvOdmFlashLTC3216Context *)
        NvOdmOsAlloc(sizeof(NvOdmFlashLTC3216Context));
    if (!pContext)
    {
        return NV_FALSE;
    }

    NvOdmOsMemset(pContext, 0, sizeof(NvOdmFlashLTC3216Context));

    pContext->PowerLevel = NvOdmImagerPowerLevel_Off;

    hImager->pFlash->pPrivateContext = pContext;

    return NV_TRUE;
}

static void FlashLTC3216_Close(NvOdmImagerHandle hImager)
{
    NvOdmFlashLTC3216Context *pContext = NULL;

    if (!hImager || ! hImager->pFlash || !hImager->pFlash->pPrivateContext)
        return;

    pContext = (NvOdmFlashLTC3216Context *)hImager->pFlash->pPrivateContext;

    if (pContext->PowerLevel != NvOdmImagerPowerLevel_Off)
    {
        (void)FlashLTC3216_SetPowerLevel(hImager, NvOdmImagerPowerLevel_Off);
    }
    NvOdmOsFree(pContext);
    hImager->pFlash->pPrivateContext = NULL;
}


static void
FlashLTC3216_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities)
{

}

static NvBool
FlashLTC3216_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel)
{
    NvBool Result = NV_TRUE;
    NvOdmFlashLTC3216Context *pContext =
        (NvOdmFlashLTC3216Context *)hImager->pFlash->pPrivateContext;

    if (pContext->PowerLevel == PowerLevel)
        return NV_TRUE;

    switch (PowerLevel)
    {
        case NvOdmImagerPowerLevel_On:
            Result = FlashLTC3216InitPins(hImager);
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
FlashLTC3216_SetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    const void *pValue)
{
    NvBool Result = NV_TRUE;
    NvOdmFlashLTC3216Context *pContext =
        (NvOdmFlashLTC3216Context *)hImager->pFlash->pPrivateContext;
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
FlashLTC3216_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue)
{
    NvBool Result = NV_TRUE;
    NvOdmFlashLTC3216Context *pContext =
        (NvOdmFlashLTC3216Context *)hImager->pFlash->pPrivateContext;

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


NvBool FlashLTC3216_GetHal(NvOdmImagerHandle hImager)
{
    if (!hImager || !hImager->pFlash)
        return NV_FALSE;

    hImager->pFlash->pfnOpen = FlashLTC3216_Open;
    hImager->pFlash->pfnClose = FlashLTC3216_Close;
    hImager->pFlash->pfnGetCapabilities = FlashLTC3216_GetCapabilities;
    hImager->pFlash->pfnSetPowerLevel = FlashLTC3216_SetPowerLevel;
    hImager->pFlash->pfnSetParameter = FlashLTC3216_SetParameter;
    hImager->pFlash->pfnGetParameter = FlashLTC3216_GetParameter;

    return NV_TRUE;
}
