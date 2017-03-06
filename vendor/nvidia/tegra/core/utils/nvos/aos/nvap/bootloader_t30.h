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

#ifndef INCLUDED_BOOTLOADER_T30_H
#define INCLUDED_BOOTLOADER_T30_H

#include "bootloader.h"
#include "t30/arpg.h"
#include "t30/nvboot_bit.h"
#include "t30/nvboot_version.h"
#include "t30/nvboot_pmc_scratch_map.h"
#include "t30/nvboot_osc.h"
#include "t30/nvboot_clocks.h"
#include "t30/arclk_rst.h"
#include "t30/artimerus.h"
#include "t30/arflow_ctlr.h"
#include "t30/arapb_misc.h"
#include "t30/arapbpm.h"
#include "t30/armc.h"
#include "t30/aremc.h"
#include "t30/arahb_arbc.h"
#include "t30/arevp.h"
#include "t30/arcsite.h"
#include "t30/arapb_misc.h"
#include "t30/ari2c.h"
#include "t30/arfuse.h"
#include "t30/argpio.h"
#include "nvassert.h"

#define AHB_PA_BASE         0x6000C000  // Base address for arahb_arbc.h registers
#define PWRI2C_PA_BASE      0x7000D000  // Base address for ari2c.h registers

#define FUSE_SKU_DIRECT_CONFIG_0_NUM_DISABLED_CPUS_RANGE  4:3
#define FUSE_SKU_DIRECT_CONFIG_0_DISABLE_ALL_RANGE        5:5

#define USE_PLLC_OUT1               0       // 0 ==> PLLP_OUT4, 1 ==> PLLC_OUT1
#define NVBL_PLL_BYPASS 0
#define NVBL_PLLP_SUBCLOCKS_FIXED   (1)

#define NV_PWRI2C_REGR(pPwrI2c, reg)        NV_READ32( (((NvUPtr)(pPwrI2c)) + I2C_##reg##_0))
#define NV_PWRI2C_REGW(pPwrI2c, reg, val)   NV_WRITE32((((NvUPtr)(pPwrI2c)) + I2C_##reg##_0), (val))

#define NVBL_PLLC_KHZ               0
#define NVBL_PLLM_KHZ               167000

// Ensure that CONFIG_PLLP_BASE_AS_408MHZ is defined.
extern NvU32 ct_assert_config_pllp[CONFIG_PLLP_BASE_AS_408MHZ?1:1];
#if CONFIG_PLLP_BASE_AS_408MHZ
#define NVBL_PLLP_KHZ               408000
#else
#define NVBL_PLLP_KHZ               216000
#endif

#define NVBL_USE_PLL_LOCK_BITS      1

extern NvBool s_FpgaEmulation;
extern NvU32 s_Netlist;
extern NvU32 s_NetlistPatch;

static NvBool NvBlIsValidBootRomSignature(const NvBootInfoTable*  pBootInfo);
static NvBool NvBlIsChipInitialized( void );
static NvBool NvBlIsChipInRecovery( void );
static NvU32 NvBlGetOscillatorDriveStrength( void );
static NvU32  NvBlAvpQueryOscillatorFrequency( void );
static void NvBlAvpEnableCpuClock(NvBool enable);
static void NvBlClockInit_T30(NvBool IsChipInitialized);
static void InitPllM(NvBootClocksOscFreq OscFreq);
static void InitPllP(NvBootClocksOscFreq OscFreq);
static void InitPllU(NvBootClocksOscFreq OscFreq);
static void NvBlMemoryControllerInit(NvBool IsChipInitialized);
static NvBool NvBlAvpIsCpuPowered(NvBlCpuClusterId ClusterId);
static void NvBlAvpRemoveCpuIoClamps(void);
static void  NvBlAvpPowerUpCpu(void);
static void NvBlAvpResetCpu(NvBool reset);
static NvBlCpuClusterId NvBlAvpQueryFlowControllerClusterId(void);
static NvU32 NvBlAvpI2cReadBoardId(NvU32 Addr);
static NvU32 NvBlAvpI2cReadBoardFab(NvU32 Addr);
NvBool NvBlAvpInit_T30(NvBool IsRunningFromSdram);
void NvBlStartCpu_T30(NvU32 ResetVector);
NV_NAKED void NvBlStartUpAvp_T30( void );
NV_NAKED void ColdBoot_T30( void );

#endif // INCLUDED_BOOTLOADER_T30_H

