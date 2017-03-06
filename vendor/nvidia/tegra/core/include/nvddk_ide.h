/*
 * Copyright (c) 2007-2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * <b>NVIDIA Driver Development Kit:
 *           IDE Interface</b>
 *
 * @b Description: Declares Interface for NvDDK IDE module.
 *
 */

#ifndef INCLUDED_NVDDK_IDE_H
#define INCLUDED_NVDDK_IDE_H

#include "nvos.h"
#include "nvddk_blockdev.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @defgroup nvbdk_ddk_ide IDE Interface
 *
 * This defines the interface for IDE.
 * @ingroup nvddk_modules
 * @{
 */

/**
 * Opens the IDE controller (enables and initializes) and returns a handle to
 * the caller. Only one instance of the IDE exists. 
 *
 * @pre IDE client(s) must first call this API before doing any further
 *      IDE operations.
 *
 *
 * @param Instance Instance to the IDE controller.
 * @param MinorInstance Minor instance of IDE controller.
 * @param phBlockDev A pointer to the block device handle.
 *
 * @retval NvSuccess IDE successfully opened and enabled.
  */
NvError 
NvDdkIdeBlockDevOpen(
    NvU32 Instance, 
    NvU32 MinorInstance,
    NvDdkBlockDevHandle *phBlockDev);

NvError 
NvDdkIdeGetIdentifyData(
    NvU32 Instance, 
    NvU8 *pIdentifyData);

NvError
NvDdkIdeBlockDevInit(NvRmDeviceHandle hDevice);

void
NvDdkIdeBlockDevDeinit(void);

#ifdef __cplusplus
}
#endif

/** @} */
#endif // INCLUDED_NVDDK_IDE_H

