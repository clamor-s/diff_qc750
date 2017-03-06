/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVRM_BOOT_H
#define INCLUDED_NVRM_BOOT_H


#if defined(__cplusplus)
extern "C"
{
#endif

#include "nvrm_init.h"

/**
 * Sets the RM chip shmoo data as a boot argument from the system's
 * boot loader.
 * 
 * @param hRmDevice The RM device handle.
 * 
 * @retval NvSuccess If successful, or the appropriate error code.
 */
NvError NvRmBootArgChipShmooSet(NvRmDeviceHandle hRmDevice);

#if defined(__cplusplus)
}
#endif

#endif // INCLUDED_NVRM_BOOT_H
