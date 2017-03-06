/*
 * Copyright (c) 2009-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#define ASSEMBLY_SOURCE_FILE 1

#include "bootloader.h"
#include "ap20/arpg.h"
#include "ap20/nvboot_bit.h"
#include "ap20/nvboot_version.h"
#include "ap20/nvboot_pmc_scratch_map.h"
#include "ap20/nvboot_osc.h"
#include "ap20/nvboot_clocks.h"
#include "ap20/arclk_rst.h"
#include "ap20/artimerus.h"
#include "ap20/arflow_ctlr.h"
#include "ap20/arapb_misc.h"
#include "ap20/arapbpm.h"
#include "ap20/armc.h"
#include "ap20/aremc.h"
#include "ap20/arahb_arbc.h"
#include "ap20/arevp.h"
#include "ap20/arcsite.h"

#define AHB_PA_BASE         0x6000C004  // Base address for arahb_arbc.h registers

#define USE_PLLC_OUT1               0       // 0 ==> PLLP_OUT4, 1 ==> PLLC_OUT1
#define NVBL_PLL_BYPASS 0
#define NVBL_PLLP_SUBCLOCKS_FIXED   (1)

#define NVBL_PLLM_KHZ               167000
#define NVBL_PLLP_KHZ               (432000/2)
#define DISABLE_LOW_LATENCY_PATH 0

static NvBool NvBlIsValidBootRomSignature(const NvBootInfoTable*  pBootInfo);
static NvBool NvBlIsChipInitialized( void );
static NvBool NvBlIsChipInRecovery( void );
static NvU32 NvBlGetOscillatorDriveStrength( void );
static NvU32  NvBlAvpQueryOscillatorFrequency( void );
static void NvBlAvpEnableCpuClock(NvBool enable);
static void NvBlClockInit_AP20(NvBool IsChipInitialized);
static void InitPllM(NvBootClocksOscFreq OscFreq);
static void InitPllP(NvBootClocksOscFreq OscFreq);
static void InitPllU(NvBootClocksOscFreq OscFreq);
static void NvBlMemoryControllerInit(NvBool IsChipInitialized);
static NvBool NvBlAvpIsCpuPowered(void);
static void NvBlAvpRemoveCpuIoClamps(void);
static void  NvBlAvpPowerUpCpu(void);
static void NvBlAvpResetCpu(NvBool reset);
NvBool NvBlAvpInit_AP20(NvBool IsRunningFromSdram);
void NvBlStartCpu_AP20(NvU32 ResetVector);
NV_NAKED void NvBlStartUpAvp_AP20( void );
NV_NAKED void ColdBoot_AP20( void );
