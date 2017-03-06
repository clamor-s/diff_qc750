/*
 * Copyright (c) 2009 - 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvodm_query_nv3p.h"
#include "fastboot.h"

/* The USB driver implementation in NV3P defines several weakly-linked
 * ODM functions to return parameters for the interface descriptor and
 * product ID string.  The implementations below are for supporting both
 * the recovery mode protocol (3P Server) and Android's Fastboot.
 */

static NvBool s_Is3pServerMode = NV_FALSE;

void FastbootSetUsbMode(NvBool Is3p)
{
    s_Is3pServerMode = Is3p;
}

NvBool NvOdmQueryUsbConfigDescriptorInterface(
    NvU32 *pIfcClass,
    NvU32 *pIfcSubclass,
    NvU32 *pIfcProtocol)
{
    if (s_Is3pServerMode)
        return NV_FALSE;

    *pIfcClass = 0xff;
    *pIfcSubclass = 0x42;
    *pIfcProtocol = 0x03;
    return NV_TRUE;
}

NvBool NvOdmQueryUsbDeviceDescriptorIds(
    NvU32 *pVendorId,
    NvU32 *pProductId,
    NvU32 *pBcdDeviceId)
{
    if (s_Is3pServerMode)
        return NV_FALSE;

    *pVendorId = 0x0955;
    *pProductId = 0x7000;
    *pBcdDeviceId = 0;
    return NV_TRUE;
}

NvU8 *NvOdmQueryUsbProductIdString(void)
{
    NV_ALIGN(4) static NvU8 RecoveryString[] = {
        0x00, 0x03,
        'F', 0x00,
        'a', 0x00,
        's', 0x00,
        't', 0x00,
        'b', 0x00,
        'o', 0x00,
        'o', 0x00,
        't', 0x00};

    RecoveryString[0] = NV_ARRAY_SIZE(RecoveryString);

    if (s_Is3pServerMode)
        return NULL;
    else
    {
        return RecoveryString;
    }
}

/* This matches the serial string implementation from the Linux Kernel.
   Any changes need to the output need to be synchronized so that
   'adb devices' and 'fastboot devices' use the same values */
NvU8 *NvOdmQueryUsbSerialNumString(void)
{
    NV_ALIGN(4) static NvU8 SerialString[] =
    {
        0x00, 0x03,
        '0', 0x00,
        '0', 0x00,
        '0', 0x00,
        '0', 0x00,
        '0', 0x00,
        '0', 0x00,
        '0', 0x00,
        '0', 0x00,
        '0', 0x00,
        '0', 0x00,
        '0', 0x00,
        '0', 0x00,
        '0', 0x00,
        '0', 0x00,
        '0', 0x00,
        '0', 0x00
    };
    NvU64 uid;
    NvU32 i = 0, k = 0;
    NvRmDeviceHandle hRm = NULL;

    if (s_Is3pServerMode)
        return NULL;

    SerialString[0] = NV_ARRAY_SIZE(SerialString);

    NV_ASSERT_SUCCESS(NvRmOpen(&hRm, 0));
    NvRmQueryChipUniqueId(hRm, sizeof(uid), &uid);
    NvRmClose(hRm);

    for (i = 16; i > 0; i--)
    {
        if ((uid & 0xF) < 0xA)
            SerialString[i * 2] = (uid & 0xF) + '0';
        else
            SerialString[i * 2] = (uid & 0xF) - 0xA + 'a';
        uid = uid >> 4;
    }

    return SerialString;
}
