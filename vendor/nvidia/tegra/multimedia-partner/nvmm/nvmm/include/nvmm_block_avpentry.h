/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVMM_BLOCK_AVPENTRY_H
#define INCLUDED_NVMM_BLOCK_AVPENTRY_H

#include "nvmm_transport.h"
#include "nvmm_transport_private.h"
#include "nvrm_transport.h"
#include "nvrm_memmgr.h"
#include "nvmm_queue.h"
#include "nvassert.h"
#include "nvmm_debug.h"
#include "nvrm_memmgr.h"
#include "nvrm_moduleloader.h"
#include "nvrm_avp_swi_registry.h"

NvError AxfModuleMain(NvRmModuleLoaderReason Reason, void *Args, NvU32 ArgsSize);

#endif // INCLUDED_NVMM_BLOCK_AVPENTRY_H
