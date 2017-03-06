/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
#include "nvrm_gpio.h"
#include "nvrm_pwm.h"
#include "pwm_test.h"
#include "pwm_test_soc.h"

NvError NvPwmTest(void)
{
    NvU32 RequestedFreq = 0;
    NvU32 ReturnedFreq = 0;
    NvU32 DutyCycle = 0;
    NvError ErrStatus;

    NvRmDeviceHandle hRmDev = NULL;
    NvRmGpioHandle hRmGpio = NULL;
    NvRmGpioPinHandle hPwmTestPin;
    NvRmPwmHandle hRmPwm = NULL;

    // Open RM device handle
    ErrStatus = NvRmOpen(&hRmDev, 0);
    if (ErrStatus != NvSuccess)
    {
        NvOsDebugPrintf("NvRmOpen for Gpio FAILED\n");
        goto RmDev_Fail;
    }

    // Open RM GPIO handle
    ErrStatus = NvRmGpioOpen(hRmDev, &hRmGpio);
    if (ErrStatus != NvSuccess)
    {
        NvOsDebugPrintf("NvRmGpioOpen FAILED\n");
        goto RmGpio_Fail;
    }

    // Get the handle for requested pin
    ErrStatus = NvRmGpioAcquirePinHandle(hRmGpio, PWM_TEST_PIN_PORT,
        PWM_TEST_PIN_NUM, &hPwmTestPin);
    if (ErrStatus != NvSuccess)
    {
        NvOsDebugPrintf("NvRmGpioAcquirePin FAILED\n");
        goto RmGpioPin_Fail;
    }

    // Configure pin as SFIO in Pwm
    ErrStatus = NvRmGpioConfigPins(hRmGpio, &hPwmTestPin, 1,
        NvRmGpioPinMode_Function);
    if (ErrStatus != NvSuccess)
    {
        NvOsDebugPrintf("NvRmGpioConfigPin FAILED\n");
        goto RmGpioPin_Fail;
    }

    // Open RM Pwm handle
    ErrStatus = NvRmPwmOpen(hRmDev, &hRmPwm);
    if (ErrStatus != NvSuccess)
    {
        NvOsDebugPrintf("NvRmPwmOpen FAILED\n");
        goto RmGpioPin_Fail;
    }

    NvOsDebugPrintf("PWM Test Pin Changes Started ... \n");
    // To increase frequency of the pulses in some time interval
    for (RequestedFreq = PWM_TEST_FREQ_MIN; RequestedFreq <= PWM_TEST_FREQ_MAX;
        RequestedFreq += PWM_TEST_FREQ_CHANGE)
    {
        // To increase dutycycle from 0 to 100 with some time interval
        for (DutyCycle = 0; DutyCycle <= 100; DutyCycle += 10)
        {
            ErrStatus = NvRmPwmConfig(hRmPwm, NvRmPwmOutputId_PWM0,
                NvRmPwmMode_Enable, DutyCycle << 16, RequestedFreq,
                    &ReturnedFreq);
            if (ErrStatus != NvSuccess)
            {
                NvOsDebugPrintf("NvRmPwmConfig FAILED\n");
                goto RmPwmConfig_Fail;
            }

            NvOsWaitUS(PWM_TEST_INTERVAL_US);
        }
        // To decrease dutycycle from 100 to 0 with some time interval
        for (DutyCycle = 100; DutyCycle > 0; DutyCycle -= 10)
        {
            ErrStatus = NvRmPwmConfig(hRmPwm, NvRmPwmOutputId_PWM0,
                NvRmPwmMode_Enable, DutyCycle << 16, RequestedFreq,
                    &ReturnedFreq);
            if (ErrStatus != NvSuccess)
            {
                NvOsDebugPrintf("NvRmPwmConfig FAILED\n");
                goto RmPwmConfig_Fail;
            }

            NvOsWaitUS(PWM_TEST_INTERVAL_US);
        }
        NvOsWaitUS(PWM_TEST_INTERVAL_US);
    }

    NvOsDebugPrintf("PWM Test Pin Changes Completed\n");
    ErrStatus = NvSuccess;

RmPwmConfig_Fail:
    // Close the Rm Pwm handle
    NvRmPwmClose(hRmPwm);

RmGpioPin_Fail:
    // Close the Rm Gpio handle
    NvRmGpioClose(hRmGpio);

RmGpio_Fail:
   // Close the Rm device handle
    NvRmClose(hRmDev);

RmDev_Fail:
    return ErrStatus;
}
