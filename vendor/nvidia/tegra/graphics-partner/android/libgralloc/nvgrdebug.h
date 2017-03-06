/*
 * Copyright (c) 2011-2011 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_NVGRDEBUG_H
#define INCLUDED_NVGRDEBUG_H

#include "nvgralloc.h"

void NvGrDumpBuffer(gralloc_module_t const *module, buffer_handle_t handle, const char *filename);

#endif // INCLUDED_NVGRDEBUG_H
