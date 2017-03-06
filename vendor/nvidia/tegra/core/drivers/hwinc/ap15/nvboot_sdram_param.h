/**
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * Defines the SDRAM parameter structure.
 *
 * Note that PLLM is used by EMC.
 */

#ifndef INCLUDED_NVBOOT_SDRAM_PARAM_H
#define INCLUDED_NVBOOT_SDRAM_PARAM_H

#include "nvcommon.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * Defines the SDRAM parameter structure
 */
typedef struct NvBootSdramParamsRec
{
    /// Specifies the CPCON value for PllM.
    NvU32 PllMChargePumpSetupControl;

    /// Specifies the LPCON value for Emc PllM.
    NvU32 PllMLoopFilterSetupControl;

    /// Specifies the M value for the Emc PllM.
    NvU32 PllMInputDivider;

    /// Specifies the N value for the Emc PllM.
    NvU32 PllMFeedbackDivider;

    /// Specifies the P value for the Emc PllM.
    NvU32 PllMPostDivider;

    /// Specifies the divider to be used for the Emc Clock Source.
    NvU32 EmcClockDivider;

    /// Specifies whether to use PLLM or PLLP for MC.
    NvBool McPll_PllM;

    /// Specifies the divider for the Mc Clock Source.
    NvU32 McClockDivider;

    /// Specifies the time (in usecs) to wait for the PLLM to lock.
    NvU32 PllMStableTime;

    /// Auto-calibration of EMC pads
    /// Specifies the calibration Interval.
    NvU32 EmcAutoCalIntervalRegValue;

    /// Specifies the calibration configuration.
    NvU32 EmcAutoCalConfigRegValue;    

    /// Specifies the values for Ramtype and Ramwidth.
    NvU32 EmcFbioCfg5RegValue;
    NvU32 EmcPinRegValue;

    /// Specifies the delay (in usec) after programming Pin.
    /// Dram vendors require this to be at least 200us.
    NvU32 DelayAfterEmcPinProgram;

    /// Specifies the NOP register value.
    NvU32 EmcNopRegValue;

    /// Timing parameters required for the SDRAM 
    /// Specifies the value for EMC_TIMING0.
    NvU32 EmcTiming0RegValue;

    /// Specifies the value for EMC_TIMING1.
    NvU32 EmcTiming1RegValue;

    /// Specifies the value for EMC_TIMING2.
    NvU32 EmcTiming2RegValue;

    /// Specifies the value for EMC_TIMING3.
    NvU32 EmcTiming3RegValue;

    /// Specifies the value for EMC_TIMING4.
    NvU32 EmcTiming4RegValue;

    /// Specifies the value for EMC_TIMING5.
    NvU32 EmcTiming5RegValue;
    NvU32 EmcFbioCfg1RegValue;

    /// FBIO configuration values 
    /// Specifies the value for EMC_FBIO_DQSIB_DLY.
    NvU32 EmcFbioDqsibDlyRegValue;

    /// Specifies the value for EMC_FBIO_QUSE_DLY.
    NvU32 EmcFbioQuseDlyRegValue;

    /// Specifies the value for EMC_FBIO_CFG6.
    NvU32 EmcFbioCfg6RegValue;

    /// Specifies the value for EMC_TIMINGCONTROL.
    NvU32 EmcTimingControlRegValue;

    /// Specifies the value for EMC_PRE.
    NvU32 EmcPreRegValue;

    /// Specifies the value for EMC_REF.
    NvU32 EmcRefRegValue;

    ///MRS command values
    /// Specifies the value for EMC_MRS.
    NvU32 EmcMrsRegValue;

    /// Specifies the value for EMC_EMRS.
    NvU32 EmcEmrsRegValue;
 
    /// Specifies the value for EMC_ADR_CFG.
    /// This value holds device, row, bank, col values.
    /// It is also used for Mc_Emc_Adr_Cfg.
    NvU32 EmcAdrCfgRegValue;

    /// Specifies the value for MC_EMEM_CFG.
    /// This register stores the external memory size (in KiBytes).
    /// EMEM_SIZE_KB must be <= (Device size in KB * Number of Devices).
    NvU32 McEmemCfgRegValue;

    /// Specifies the value for MC_LOWLATENCY_CONFIG.
    /// Primary, this valie us used for LL_DRAM_INTERLEAVE.
    /// If the DRAMs do not support interleave mode, turn off this bit
    /// to get correct low-latency path behavior.  It is enabled at reset.
    NvU32 McLowLatencyCfgRegValue;

    /// Specifies the value for EMC_CFG.
    NvU32 EmcCfgRegValue;

    /// Specifies the value for EMC_DBG.
    NvU32 EmcDbgRegValue;    

    /// Specifies the value for EMC_REF_CTRL.
    NvU32 EmcRefCtrlRegValue;

    /// Specifies the value for the Memory Inid done.
    NvU32 AhbArbitrationXbarCtrlRegValue;

} NvBootSdramParams;

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_NVBOOT_SDRAM_PARAM_H */

