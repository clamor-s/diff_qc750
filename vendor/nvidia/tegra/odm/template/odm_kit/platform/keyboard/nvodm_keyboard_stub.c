/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_keyboard.h"


NvBool NvOdmKeyboardInit(void)
{
    return NV_FALSE;
}

void NvOdmKeyboardDeInit(void)
{
}

NvBool NvOdmKeyboardGetKeyData(NvU32 *KeyScanCode, NvBool *IsKeyUp, NvU32 Timeout)
{
    return NV_FALSE;
}

NvBool NvOdmKeyboardToggleLights(NvU32 LedId)
{
    return NV_FALSE;
}

NvBool NvOdmKeyboardPowerHandler(NvBool PowerDown)
{
    return NV_FALSE;
}

/* -----------Implemetation for Hold Switch Adaptation starts here-------*/

NvBool NvOdmHoldSwitchInit(void)
{
    return NV_FALSE;
}

void NvOdmHoldSwitchDeInit(void)
{
}

