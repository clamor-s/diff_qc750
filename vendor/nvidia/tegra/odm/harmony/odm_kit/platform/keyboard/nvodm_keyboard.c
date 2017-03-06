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
#include "nvodm_keyboard_priv.h"

static OdmKeyBoardInt s_OdmKbInterface = { NULL, NULL, NULL};
static void InitInterface(void)
{
    if (s_OdmKbInterface.KeyboardInit)
        return;

    InitGpioKeyboardInterface(&s_OdmKbInterface);
}

NvBool NvOdmKeyboardInit(void)
{
    if (!s_OdmKbInterface.KeyboardInit)
        InitInterface();
    return s_OdmKbInterface.KeyboardInit();
}

void NvOdmKeyboardDeInit(void)
{
    if (!s_OdmKbInterface.KeyboardInit)
        InitInterface();
    s_OdmKbInterface.KeyboardDeInit();
}

/* Gets the actual scan code for a key press */
NvBool NvOdmKeyboardGetKeyData(NvU32 *pKeyScanCode, NvU8 *pScanCodeFlags,
                                NvU32 Timeout)
{
    if (!s_OdmKbInterface.KeyboardInit)
        InitInterface();
    return s_OdmKbInterface.KeyboardGetKeyData(pKeyScanCode, pScanCodeFlags,
                                    Timeout);
}
