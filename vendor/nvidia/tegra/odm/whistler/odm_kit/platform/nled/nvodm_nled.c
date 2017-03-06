/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 * 
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_nled.h"
#include "nvodm_nled_int.h"

/**
 *  @brief Allocates a handle to the device.
 *  @param hOdmNled  [IN] Opaque handle to the device.
 *  @return  NV_TRUE on success and NV_FALSE on error
 */
NvBool
NvOdmNledOpen(NvOdmNledDeviceHandle *phOdmNled)
{
    NvError e = NvError_Success;

    NV_ODM_TRACE(("NvOdmNledOpen: enter\n"));

    /* allocate the handle */
    *phOdmNled = NvOdmOsAlloc(sizeof(NvOdmNledContext));
    if (*phOdmNled == NULL)
    {
        NV_ODM_TRACE(("NvOdmNledOpen: error allocating NvOdmNledDeviceHandle. \n"));
        goto cleanup;
    }
    NvOsMemset(*phOdmNled, 0, sizeof(NvOdmNledContext));

    e = NvRmOpen(&(*phOdmNled)->hRm, 0);
    if (e != NvError_Success)
        goto cleanup;

    /* Get the PWM handle */
    e = NvRmPwmOpen((*phOdmNled)->hRm ,&(*phOdmNled)->hRmPwm);
    if (e != NvError_Success)
        goto cleanup;

    NV_ODM_TRACE(("NvOdmNledOpen: exit\n"));
    return NV_TRUE;

cleanup:
    NvRmPwmClose((*phOdmNled)->hRmPwm);
    (*phOdmNled)->hRmPwm = NULL;

    NvRmClose((*phOdmNled)->hRm);
    (*phOdmNled)->hRm = NULL;

    NvOdmOsFree(*phOdmNled);
    *phOdmNled = NULL;

    return NV_FALSE;
}

/**
 *  @brief Closes the ODM device and destroys all allocated resources.
 *  @param hOdmNled  [IN] Opaque handle to the device.
 *  @return None.
 */
void NvOdmNledClose(NvOdmNledDeviceHandle hOdmNled)
{
    NV_ODM_TRACE(("NvOdmNledClose enter\n"));

    if (hOdmNled)
    {
        NvRmPwmClose(hOdmNled->hRmPwm);
        hOdmNled->hRmPwm = NULL;

        NvRmClose(hOdmNled->hRm);
        hOdmNled->hRm = NULL;

        NvOdmOsFree(hOdmNled);
        hOdmNled = NULL;
    }

    NV_ODM_TRACE(("NvOdmNledClose exit\n"));
}

/**
 *  @brief .Sets the NLED to solid, blinking or off states. The blink settings should be given when
 *          the state is requested as blinking.
 *  @param hOdmNled  [IN] Opaque handle to the device.
 *  @param State     [IN] State of the LED
 *  @param Settings  [IN] LED settings when the state is blinking
 *  @return NV_TRUE on success
 */
NvBool NvOdmNledSetState(NvOdmNledDeviceHandle hOdmNled, NvOdmNledState State, NvOdmNledSettings *pSettings)
{
    NvError e = NvError_Success;
    NvU32 DutyCycle = 0, CurrPeriod = 0, TotalCycleTime = 0;
    NvBool RetVal = NV_TRUE;

    NV_ODM_TRACE(("NvOdmNledSetState enter\n"));

    if (!hOdmNled)
    {
        NV_ODM_TRACE(("Bad Parameter\n"));
        return NV_FALSE;
    }

    switch (State)
    {
        case NvOdmNledState_On:
            {
                NV_ODM_TRACE("**************** ON ***************");
                DutyCycle = 100; // duty cycle to 100% to keep the LED solid
                e = NvRmPwmConfig(
                        hOdmNled->hRmPwm,
                        NvRmPwmOutputId_Blink,
                        NvRmPwmMode_Blink_32KHzClockOutput,
                        DutyCycle << 16,
                        0x1FFFF,
                        &CurrPeriod);
                if (e != NvError_Success)
                {
                    NV_ODM_TRACE("NvRmPwmConfig failed");
                    RetVal = NV_FALSE;
                }

                break;
            }

        case NvOdmNledState_Off:
            {
                NV_ODM_TRACE("**************** OFF ***************");
                e = NvRmPwmConfig(
                        hOdmNled->hRmPwm,
                        NvRmPwmOutputId_Blink,
                        NvRmPwmMode_Blink_Disable,
                        0,
                        0,
                        &CurrPeriod);
                if (e != NvError_Success)
                {
                    NV_ODM_TRACE("NvRmPwmConfig failed");
                    RetVal = NV_FALSE;
                }

                break;
            }

        case NvOdmNledState_Blinking:
            {
                if (!pSettings)
                {
                    NV_ODM_TRACE("Bad Parameter");
                    RetVal = NV_FALSE;
                    break;
                }

                NV_ODM_TRACE("**************** Blink ***************");

                if (pSettings->TotalCycleTime)
                {
                    DutyCycle = (pSettings->OnTime/pSettings->TotalCycleTime);
                }
                else
                {
                    /* Generally 0 settings do not mean anything and are used only by CETK */
                    RetVal = NV_TRUE;
                    break;
                }

                e = NvRmPwmConfig(
                        hOdmNled->hRmPwm,
                        NvRmPwmOutputId_Blink,
                        NvRmPwmMode_Blink_32KHzClockOutput,
                        DutyCycle << 16,
                        TotalCycleTime,
                        &CurrPeriod);
                if (e != NvError_Success)
                {
                    NV_ODM_TRACE("NvRmPwmConfig failed");
                    RetVal = NV_FALSE;
                }

                break;
            }

        default:
            {
                NV_ODM_TRACE("NvOdmNledSetState: invalid state specified\r\n");
                RetVal = NV_FALSE;
            }
    }

    NV_ODM_TRACE("NvOdmNledSetState exit\r\n");
    return RetVal;
}

/**
 *  @brief .Power handler function
 *  @param pHostContext  [IN] Opaque handle to the device.
 *  @param PowerDown     [IN] Flag to indicate power up/down
 *  @return NV_TRUE on success
 */
NvBool NvOdmNledPowerHandler(NvOdmNledDeviceHandle hOdmNled, NvBool PowerDown)
{
    return NV_TRUE;
}

