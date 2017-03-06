/*
 * Copyright (c) 2007 - 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef NVODM_KEYBOARD_PRIV_H
#define NVODM_KEYBOARD_PRIV_H
#include "nvcommon.h"
#if defined(__cplusplus)
extern "C"
{
#endif

// The structure of the function pointers to provide the interface to
// access the spi/slink hw registers and their property.
typedef struct
{
    NvBool (*KeyboardInit)(void);
    void   (*KeyboardDeInit)(void);
    NvBool (*KeyboardGetKeyData)(NvU32 *pKeyScanCode,
                                NvU8 *pScanCodeFlags, NvU32 Timeout);
} OdmKeyBoardInt, *OdmKeyBoardIntHandle;

void InitGpioKeyboardInterface(OdmKeyBoardIntHandle hOdmKbInt);

#if defined(__cplusplus)
}
#endif

#endif /* NVODM_KEYBOARD_PRIV_H */
