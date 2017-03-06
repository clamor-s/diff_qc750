/*
 * Copyright (c) 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef COMMON_MISC_PRIVATE_H
#define COMMON_MISC_PRIVATE_H

/*
 * common_misc_private.h defines the common miscellenious private implementation
 * functions for the resource manager.
 */

#include "nvcommon.h"
#include "nvos.h"

#ifdef __cplusplus
extern "C"
{
#endif  /* __cplusplus */


/**
 * Chip unque id for T30.
 */
NvError
NvRmPrivT30ChipUniqueId(
    NvRmDeviceHandle hDevHandle,
    void* pId);

/**
 * Chip unque id for Ap20.
 */
NvError
NvRmPrivAp20ChipUniqueId(
    NvRmDeviceHandle hDevHandle,
    void* pId);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // COMMON_MISC_PRIVATE_H
