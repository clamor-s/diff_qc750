/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * Defines fields in the PMC scratch registers used by the Boot ROM code.
 */


#ifndef INCLUDED_NVBOOT_PMC_SCRATCH_MAP_H
#define INCLUDED_NVBOOT_PMC_SCRATCH_MAP_H

/**
 * The PMC module in the Always On domain of the chip provides 24
 * scratch registers. These offer SW a non-volatile storage space.
 *
 * These are used by to store state information for on-chip controllers which
 * are to be restored during Warm Boot, whether WB0 or WB1.
 *
 * This header file defines the allocation of scratch register space for
 * storing data needed by Warm Boot.
 *
 * Each of the scratch registers has been sliced into bit fields to store 
 * parameters for various on-chip controllers. Every bit field that needs
 * to be stored has a corresponding bit field in a scratch register.
 * The widths match those of the the original bit fields.
 *
 * Scratch register fields have been defined with self explanatory names
 * and with bit ranges compatible with nvrm_drf macros.
 *
 * Register APBDEV_PMC_SCRATCH0_0 is the *only* scratch register cleared on
 * power-on-reset. This register will also be used by RM and OAL. This will hold
 * the Warm Boot Flag which tells the BR code whether it is WB0. This will be
 * suitably modified by the SW functions that put the chip in LP0.
 * The assignment of bit fields used by the BR is *NOT* to be changed.
 *
 * Registers APBDEV_PMC_SCRATCH1_1 through APBDEV_PMC_SCRATCH15_0 will
 * be used exclusively by the BR. PLL, CLK_RST_CONTROLLER and MC/EMC register
 * data will be written to these registers before entering LP0/LP1 by the SW
 * that puts the chip into LP0/LP1.
 * These functions will also store start address of recovery code in SDRAM
 * and a 32-bit checksum of a part of that code.
 * These registers should *NOT* be modified by rest of the SW.
 *
 * Register APBDEV_PMC_SCRATCH0_23 will be read by the PMC module to store delay
 * interval related to the external PMU. SW initilization routines should
 * write the correct value in this register. This register is *NOT*
 * available for general use.
 *
 * Registers APBDEV_PMC_SCRATCH16_0 through APBDEV_PMC_SCRATCH22_0 *CAN*
 * be used by rest of SW as a non volatile storage space.
 */

/*
 * Scratch registers offsets are part of PMC HW specification - "arapbpm.spec".
 *
 * Ownership Issues: Important!!!!
 *
 * The list of which bit fields to store from MC & EMC was generated after
 * discussion with MC/EMC module owners.
 */

#include "ap15/arapbpm.h"

/**
 * Register APBDEV_PMC_SCRATCH0_0
 *
 * WARM_BOOT0_FLAG directs the BR to perform a warm boot when set to 1.
 * FORCE_RECOVERY directs the BR to enter RCM when set to 1.
 * BL_FAIL_BACK directs the BR to load older generation BLs.
 */
#define APBDEV_PMC_SCRATCH0_0_WARM_BOOT0_FLAG_RANGE         0:0
#define APBDEV_PMC_SCRATCH0_0_FORCE_RECOVERY_FLAG_RANGE     1:1
#define APBDEV_PMC_SCRATCH0_0_BL_FAIL_BACK_FLAG_RANGE       2:2

/**
 * Register APBDEV_PMC_SCRATCH1_0
 *
 * Pointer to the start of recovery code in SDRAM (32 bits)
 */
#define APBDEV_PMC_SCRATCH1_0_PTR_TO_RECOVERY_CODE_RANGE                31:0

/**
 * Register APBDEV_PMC_SCRATCH2_0
 *
 * Checksum of a part of the recover code in SDRAM (32 bits)
 */
#define APBDEV_PMC_SCRATCH2_0_CHECKSUM_OF_RECOVERY_CODE_RANGE           31:0

/**
 * Register APBDEV_PMC_SCRATCH3_0
 *
 * PLLM Configuration Data from CLK_RST_CONTROLLER_PLLM_BASE_0
 * and CLK_RST_CONTROLLER_PLLM_MISC_0 registers
 * CLK_RST_CONTROLLER_OSC_CTRL_0.OSC_FREQ
 *      from OSC_FREQ field of CLK_RST_CONTROLLER_OSC_CTRL_0
 * CLK_RST_CONTROLLER_OSC_CTRL_0.XOBP
 *      from XOBB filed of CLK_RST_CONTROLLER_OSC_CTRL_0
 */
/// CLK_RST_CONTROLLER_PLLM_BASE_0: DIVM, DIVN, DIVP fields
#define APBDEV_PMC_SCRATCH3_0_CLK_RST_PLLM_BASE_PLLM_DIVM_RANGE         4:0
#define APBDEV_PMC_SCRATCH3_0_CLK_RST_PLLM_BASE_PLLM_DIVN_RANGE         14:5
#define APBDEV_PMC_SCRATCH3_0_CLK_RST_PLLM_BASE_PLLM_DIVP_RANGE         17:15

/// CLK_RST_CONTROLLER_PLLM_MISC_0: LFCON, CPCON fields
#define APBDEV_PMC_SCRATCH3_0_CLK_RST_PLLM_MISC_LFCON_RANGE             21:18
#define APBDEV_PMC_SCRATCH3_0_CLK_RST_PLLM_MISC_CPCON_RANGE             25:22
#define APBDEV_PMC_SCRATCH3_0_CLK_RST_OSC_CTRL_OSC_FREQ_RANGE           27:26
#define APBDEV_PMC_SCRATCH3_0_CLK_RST_OSC_CTRL_XOBP_RANGE               28:28

/**
 * Register APBDEV_PMC_SCRATCH4_0
 *
 * PLLM will be used as a clock source for MC & EMC.
 * Stores Clock divisor for MC, EMC and PLLM start time
 */
#define APBDEV_PMC_SCRATCH4_0_ALL_FLDS_RANGE                            31:0
#define APBDEV_PMC_SCRATCH4_0_PLLM_EMC_2X_CLK_DIV_RANGE                 7:0
#define APBDEV_PMC_SCRATCH4_0_PLLM_MEM_CLK_DIV_RANGE                    15:8
#define APBDEV_PMC_SCRATCH4_0_PLLM_STABLIZATION_DLY_RANGE               31:16

/**
 * Register APBDEV_PMC_SCRATCH5_0
 *
 * MC Configuration Data to store the following:
 *      MC_EMEM_CFG_0.EMEM_SIZE_KB (bits 0-21)
 *      MC_LOWLATENCY_CONFIG_0.LL_DRAM_INTERLEAVE (bit 22)
*/

#define APBDEV_PMC_SCRATCH5_0_MC_EMEM_CFG_EMEM_SIZE_KB_RANGE                21:0
#define APBDEV_PMC_SCRATCH5_0_MC_LOWLATENCY_CONFIG_LL_DRAM_INTERLEAVE_RANGE 22:22

/**
 * Register APBDEV_PMC_SCRATCH6_0
 *
 * EMC Configuration Data to store the following:
 *      EMC_ADR_CFG_0.EMEM_COLWIDTH (bits 0-2)
 *      EMC_ADR_CFG_0.EMEM_BANKWIDTH (bits 3-4)
 *      EMC_ADR_CFG_0.EMEM_DEVSIZE (bits 5-7)
 *      EMC_ADR_CFG_0.EMEM_NUMDEV (bits 8-9)
 *      EMC_TIMING1_0.R2W (bits 10-14)
 *      EMC_TIMING1_0.W2R (bits 15-19)
 *      EMC_TIMING1_0.W2P (bits 20-24)
 *      EMC_FBIO_CFG1_0.CFG_DEN_EARLY (bit 25)
 *      EMC_FBIO_CFG5_0.DRAM_WIDTH (bit 26)
 *      EMC_FBIO_CFG5_0.DRAM_TYPE (bit 27)
 *      EMC_FBIO_CFG6_0.CFG_QUSE_LATE (bits 28-30)
 */
#define APBDEV_PMC_SCRATCH6_0_EMC_ADR_CFG_EMEM_COLWIDTH_RANGE       2:0
#define APBDEV_PMC_SCRATCH6_0_EMC_ADR_CFG_EMEM_BANKWIDTH_RANGE      4:3
#define APBDEV_PMC_SCRATCH6_0_EMC_ADR_CFG_EMEM_DEVSIZE_RANGE        7:5
#define APBDEV_PMC_SCRATCH6_0_EMC_ADR_CFG_EMEM_NUMDEV_RANGE         9:8
#define APBDEV_PMC_SCRATCH6_0_EMC_TIMING1_R2W_RANGE                 14:10
#define APBDEV_PMC_SCRATCH6_0_EMC_TIMING1_W2R_RANGE                 19:15
#define APBDEV_PMC_SCRATCH6_0_EMC_TIMING1_W2P_RANGE                 24:20
#define APBDEV_PMC_SCRATCH6_0_EMC_FBIO_CFG1_CFG_DEN_EARLY_RANGE     25:25
#define APBDEV_PMC_SCRATCH6_0_EMC_FBIO_CFG5_DRAM_WIDTH_RANGE        26:26
#define APBDEV_PMC_SCRATCH6_0_EMC_FBIO_CFG5_DRAM_TYPE_RANGE         27:27
#define APBDEV_PMC_SCRATCH6_0_EMC_FBIO_CFG6_CFG_QUSE_LATE_RANGE     30:28

/**
 * Register APBDEV_PMC_SCRATCH7_0
 *
 * EMC Configuration Data to store the following:
 *      EMC_TIMING0_0.RC (bits 0-5)
 *      EMC_TIMING0_0.RFC (bits 6-11)
 *      EMC_TIMING0_0.RAS (bits 12-17)
 *      EMC_TIMING0_0.RP (bits 18-23)
 *      EMC_FBIO_DQSIB_DLY_0.CFG_DQSIB_DLY_BYTE_0 (bits 24-31)
 */
#define APBDEV_PMC_SCRATCH7_0_EMC_TIMING0_RC_RANGE                              5:0
#define APBDEV_PMC_SCRATCH7_0_EMC_TIMING0_RFC_RANGE                             11:6
#define APBDEV_PMC_SCRATCH7_0_EMC_TIMING0_RAS_RANGE                             17:12
#define APBDEV_PMC_SCRATCH7_0_EMC_TIMING0_RP_RANGE                              23:18
#define APBDEV_PMC_SCRATCH7_0_EMC_FBIO_DQSIB_DLY_CFG_DQSIB_DLY_BYTE_0_RANGE     31:24

/**
 * Register APBDEV_PMC_SCRATCH8_0
 *
 * EMC Configuration Data stores the following:
 *      EMC_TIMING2_0.RD_RCD (bits 0-4)
 *      EMC_TIMING2_0.RRD (bits 5-8)
 *      EMC_TIMING2_0.REXT (bits 9-12)
 *      EMC_TIMING4_0.REFRESH (bits 13-23)
 *      EMC_FBIO_QUSE_DLY_0.CFG_QUSE_DLY_BYTE_0 (bits 24-31)
 */
#define APBDEV_PMC_SCRATCH8_0_EMC_TIMING2_RD_RCD_RANGE                         4:0
#define APBDEV_PMC_SCRATCH8_0_EMC_TIMING2_RRD_RANGE                            8:5
#define APBDEV_PMC_SCRATCH8_0_EMC_TIMING2_REXT_RANGE                           12:9
#define APBDEV_PMC_SCRATCH8_0_EMC_TIMING4_REFRESH_RANGE                        23:13
#define APBDEV_PMC_SCRATCH8_0_EMC_FBIO_QUSE_DLY_CFG_QUSE_DLY_BYTE_0_RANGE      31:24

/**
 * Register APBDEV_PMC_SCRATCH9_0
 *
 * EMC Configuration Data stores the following:
 *      EMC_TIMING3_0.QUSE (bits 0-3)
 *      EMC_TIMING3_0.QRST (bits 4-7)
 *      EMC_TIMING3_0.RDV (bits 8-12)
 *      EMC_TIMING5_0.PDEX2RD (bits 13-16)
 *      EMC_TIMING5_0.RW2PDEN (bits 17-20)
 */
#define APBDEV_PMC_SCRATCH9_0_EMC_TIMING3_QUSE_RANGE                   3:0
#define APBDEV_PMC_SCRATCH9_0_EMC_TIMING3_QRST_RANGE                   7:4
#define APBDEV_PMC_SCRATCH9_0_EMC_TIMING3_RDV_RANGE                    12:8
#define APBDEV_PMC_SCRATCH9_0_EMC_TIMING5_PDEX2RD_RANGE                16:13
#define APBDEV_PMC_SCRATCH9_0_EMC_TIMING5_RW2PDEN_RANGE                20:17

/**
 * Register APBDEV_PMC_SCRATCH10_0
 *
 * NOT USED!!!!!!!!
 *
 */

/**
 * Register APBDEV_PMC_SCRATCH11_0
 *
 * PLLC Configuration Data from CLK_RST_CONTROLLER_PLLM_BASE_0
 * and CLK_RST_CONTROLLER_PLLM_MISC_0 registers
 */
/// CLK_RST_CONTROLLER_PLLC_BASE_0: DIVM, DIVN, DIVP fields
#define APBDEV_PMC_SCRATCH11_0_CLK_RST_PLLC_BASE_PLLC_DIVM_RANGE         4:0
#define APBDEV_PMC_SCRATCH11_0_CLK_RST_PLLC_BASE_PLLC_DIVN_RANGE         14:5
#define APBDEV_PMC_SCRATCH11_0_CLK_RST_PLLC_BASE_PLLC_DIVP_RANGE         17:15

/// CLK_RST_CONTROLLER_PLLC_MISC_0: LFCON, CPCON fields
#define APBDEV_PMC_SCRATCH11_0_CLK_RST_PLLC_MISC_LFCON_RANGE             21:18
#define APBDEV_PMC_SCRATCH11_0_CLK_RST_PLLC_MISC_CPCON_RANGE             25:22

/**
 * Register APBDEV_PMC_SCRATCH12_0
 *
 * PLLC Start (stablization) time
 */
#define APBDEV_PMC_SCRATCH12_0_PLLC_STABLIZATION_DLY_RANGE               15:0

/**
 * Register APBDEV_PMC_SCRATCH13_0
 *
 * EMC Registers EMC_CFG_0 and EMC_DBG_0
 *
 * EMC_CFG_0.DRAM_CLKSTOP (1 bit)
 * EMC_CFG_0.DRAM_ACPD (1 bit)
 * EMC_CFG_0.AUTO_PRE_WR (1 bit)
 * EMC_CFG_0.AUTO_PRE_RD (1 bit)
 * EMC_CFG_0.CLEAR_AP_PREV_SPREQ (1 bit)
 *
 * EMC_DBG_0.CFG_PRIORITY (1 bit)
 * EMC_DBG_0.AP_REQ_BUSY_CTRL (1 bit)
 * EMC_DBG_0.READ_DQM_CTRL (1 bit)
 * EMC_DBG_0.PERIODIC_QRST (1 bit)
 * EMC_DBG_0.MRS_WAIT (1 bit)
 */
#define APBDEV_PMC_SCRATCH13_0_EMC_CFG_DRAM_CLKSTOP_RANGE               0:0
#define APBDEV_PMC_SCRATCH13_0_EMC_CFG_DRAM_ACPD_RANGE                  1:1
#define APBDEV_PMC_SCRATCH13_0_EMC_CFG_AUTO_PRE_WR_RANGE                2:2
#define APBDEV_PMC_SCRATCH13_0_EMC_CFG_AUTO_PRE_RD_RANGE                3:3
#define APBDEV_PMC_SCRATCH13_0_EMC_CFG_CLEAR_AP_PREV_SPREQ_RANGE        4:4

#define APBDEV_PMC_SCRATCH13_0_EMC_DBG_CFG_PRIORITY_RANGE               5:5
#define APBDEV_PMC_SCRATCH13_0_EMC_DBG_AP_REQ_BUSY_CTRL_RANGE           6:6
#define APBDEV_PMC_SCRATCH13_0_EMC_DBG_READ_DQM_CTRL_RANGE              7:7
#define APBDEV_PMC_SCRATCH13_0_EMC_DBG_PERIODIC_QRST_RANGE              8:8
#define APBDEV_PMC_SCRATCH13_0_EMC_DBG_MRS_WAIT_RANGE                   9:9

/**
 * Register APBDEV_PMC_SCRATCH14_0
 *
 * EMC Registers EMC_AUTO_CAL_CONFIG_0
 */
#define APBDEV_PMC_SCRATCH14_0_EMC_AUTO_CAL_CONFIG_ALL_BITS_RANGE       31:0

/**
 * Register APBDEV_PMC_SCRATCH15_0
 *
 * EMC Registers EMC_AUTO_CAL_INTERVAL_0
 */
#define APBDEV_PMC_SCRATCH14_0_EMC_AUTO_CAL_INTERVAL_ALL_BITS_RANGE     31:0

/**
 * The following are NOT used by the Boot ROM code
 *
 *      Register APBDEV_PMC_SCRATCH10_0
 *      Register APBDEV_PMC_SCRATCH16_0
 *      Register APBDEV_PMC_SCRATCH17_0
 *      Register APBDEV_PMC_SCRATCH18_0
 *      Register APBDEV_PMC_SCRATCH19_0
 *      Register APBDEV_PMC_SCRATCH20_0
 *      Register APBDEV_PMC_SCRATCH21_0
 *      Register APBDEV_PMC_SCRATCH22_0
 *
 * The following should *NOT* used by SW at all
 * It is used to store PMU/PMC response delay configuration and needs to
 * maintain its contents!!
 *
 *      Register APBDEV_PMC_SCRATCH23_0
 */

#endif // INCLUDED_NVBOOT_PMC_SCRATCH_MAP_H

