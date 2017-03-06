/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
#ifndef INCLUDED_PWM_TEST_H
#define INCLUDED_PWM_TEST_H

#include "nverror.h"
#include "nvcommon.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#define PWM_TEST_INTERVAL_US          200000
#define PWM_TEST_FREQ_MIN             400
#define PWM_TEST_FREQ_MAX             2000
#define PWM_TEST_FREQ_CHANGE          400

/**
 * Intializes the Pwm Controller and Configures the GPIO pin as SFIO to attach
 * to PWM. Configures pwm to lowest in the predefined frequency range.
 * Increases duty cycle by 1, from 0 to 100 in each predefined time interval
 * And then decreases by 1, from 100 to 0 in each predefined time interval.
 * Does the same for different frequencies that predefined frequency range.
 *
 * @retval NvSuccess The Pwm pulse generation is successful.
 * @retval NvError_InsufficientMemory
 * @retval NvError_BadParameter
 */
NvError NvPwmTest(void);

#if defined(__cplusplus)
}
#endif

#endif // INCLUDED_PWM_TEST_H
