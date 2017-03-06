/*
 * Copyright (c) 2009-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef INCLUDED_BOOTLOADER_H
#define INCLUDED_BOOTLOADER_H

#define ASSEMBLY_SOURCE_FILE 1

#include "aos_common.h"
#include "nvassert.h"
#include "nvbl_memmap_nvap.h"
#include "nvbl_assembly.h"
#include "nvbl_arm_cpsr.h"
#include "nvbl_arm_cp15.h"
#include "nvrm_arm_cp.h"

//------------------------------------------------------------------------------
// Provide missing enumerators for spec files.
//------------------------------------------------------------------------------

#define NV_BIT_ADDRESS IRAM_PA_BASE
#define NV3P_SIGNATURE 0x5AFEADD8

NvBool NvBlIsMmuEnabled(void);
NvBool NvBlQueryGetNv3pServerFlag(void);

void NvBlAvpStallUs(NvU32 MicroSec);
void NvBlAvpStallMs(NvU32 MilliSec);
void NvBlAvpSetCpuResetVector(NvU32 reset);
void NvBlAvpUnHaltCpu(void);
void NvBlAvpHalt(void);

#endif
