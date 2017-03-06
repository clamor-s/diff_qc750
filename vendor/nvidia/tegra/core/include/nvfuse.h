/**
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVFUSE_H
#define INCLUDED_NVFUSE_H

#include "nvcommon.h"
#include "nvbl_operatingmodes.h"
#include "nvbit.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * NvFuseGetOperatingMode
 *  Returns the current operating mode of the chip based on the burned fuses.
 */
NvBlOperatingMode NvFuseGetOperatingMode(void);

/**
 * NvFuseGetSecondaryBootDevice
 *  Returns the secondary boot device that is programmed into the device fuses and its instance
 */
void NvFuseGetSecondaryBootDevice(NvU32* BootDevice, NvU32* Instance);

#ifdef __cplusplus
}
#endif

#endif
