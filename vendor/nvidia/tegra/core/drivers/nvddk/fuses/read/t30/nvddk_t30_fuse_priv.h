/*
 * Copyright (c) 2012, NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_NVDDK_T30_FUSE_PRIV_H
#define INCLUDED_NVDDK_T30_FUSE_PRIV_H

#if defined(__cplusplus)
extern "C"
{
#endif

#include "nvddk_fuse_bridge.h"

#define SDMMC_CONTROLLER_INSTANCE_2 2
#define SDMMC_CONTROLLER_INSTANCE_3 3

void MapT30ChipProperties(FuseCore *pFuseCore);

#ifdef __cplusplus
}
#endif

#endif
