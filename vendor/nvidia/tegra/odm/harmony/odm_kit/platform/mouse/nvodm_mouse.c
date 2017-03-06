/*
 * Copyright (c) 2006 - 2009 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvodm_mouse.h"
#include "nvodm_mouse_int.h"
#include "nvrm_drf.h"

// Module debug: 0=disable, 1=enable
#define NVODM_ENABLE_PRINTF      0

#if NVODM_ENABLE_PRINTF
#define NVODM_PRINTF(x) NvOdmOsDebugPrintf x
#else
#define NVODM_PRINTF(x)
#endif

// wake from mouse disabled for now
#define WAKE_FROM_MOUSE 0

/**
 * streaming data from mouse can be compressed if (uncompressed) packet size is
 * 3 bytes (as it is for all legacy ps2 mice).  Compression works by not sending
 * the first byte of the packet when it hasn't change.
 */

#define ENABLE_COMPRESSION 1

#define ECI_MOUSE_DISABLE_SUPPORTED 1

/**
 * specify the ps2 port where the mouse is connected
 */

#define MOUSE_PS2_PORT_ID NVEC_SUBTYPE_0_AUX_PORT_ID_0

// context structure
// FIXME -- should be allocated dynamically

NvOdmMouseDevice hMouseDev = {0};

/** Implementation for the NvOdm Mouse */

NvBool
NvOdmMouseDeviceOpen(
    NvOdmMouseDeviceHandle *hDevice)
{
    NvBool ret = NV_FALSE;
    return ret;
}

void
NvOdmMouseDeviceClose(
    NvOdmMouseDeviceHandle hDevice)
{
}

NvBool NvOdmMouseEnableInterrupt(
    NvOdmMouseDeviceHandle hDevice, 
    NvOdmOsSemaphoreHandle hInterruptSemaphore)
{
    return NV_FALSE;
}

NvBool
NvOdmMouseDisableInterrupt(
    NvOdmMouseDeviceHandle hDevice)
{
    return NV_FALSE;
}

NvBool NvOdmMouseGetEventInfo(
    NvOdmMouseDeviceHandle hDevice, 
    NvU32 *NumPayLoad, 
    NvU8 *PayLoadBuf)
{
    return NV_FALSE;
}

NvBool
NvOdmMouseSendRequest(
    NvOdmMouseDeviceHandle hDevice, 
    NvU32 cmd, 
    NvU32 ExpectedResponseSize,
    NvU32 *NumPayLoad, 
    NvU8 *PayLoadBuf)
{
    return NV_FALSE;
}

NvBool
NvOdmMouseStartStreaming(
    NvOdmMouseDeviceHandle hDevice,
    NvU32 NumBytesPerSample)
{
    return NV_FALSE;
}


/**
 *  Power suspend for mouse.
 *
 */
NvBool NvOdmMousePowerSuspend(NvOdmMouseDeviceHandle hDevice)
{
    return NV_FALSE;
}

/**
 *  Power resume for mouse.
 *
 */
NvBool NvOdmMousePowerResume(NvOdmMouseDeviceHandle hDevice)
{
    return NV_FALSE;
}
