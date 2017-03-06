/*
 * Copyright (c) 2007 - 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_AP15RM_CLOCKS_H
#define INCLUDED_AP15RM_CLOCKS_H

#include "nvrm_clocks.h"

#ifdef __cplusplus
extern "C"
{
#endif  /* __cplusplus */

/*****************************************************************************/


// Separate API to control TVDAC clock independently of TVO
// (when TVDAC is used for CRT)  
void
Ap15EnableTvDacClock(
    NvRmDeviceHandle hDevice,
    ModuleClockState ClockState);

/**
 * Sets "as is" specified PLL configuration: switches PLL in bypass mode,
 * changes PLL settings, waits for PLL stabilization, and switches to PLL
 * output.
 * 
 * @param hRmDevice The RM device handle.
 * @param pCinfo Pointer to the PLL description structure.
 * @param M PLL input divider setting.
 * @param N PLL feedback divider setting.
 * @param P PLL output divider setting.
 *  PLL is left disabled (not bypassed) if either M or N setting is zero:
 *  M = 0 or N = 0; otherwise, M, N, P validation is caller responsibility.
 * @param StableDelayUs PLL stabilization delay in microseconds. If specified
 *  value is above guaranteed stabilization time, the latter one is used.
 * @param cpcon PLL charge pump control setting; ignored if TypicalControls
 *  is true.
 * @param lfcon PLL loop filter control setting; ignored if TypicalControls
 *  is true.
 * @param TypicalControls If true, both charge pump and loop filter parameters
 *  are ignored and typical controls that corresponds to specified M, N, P
 *  values will be set. If false, the cpcon and lfcon parameters are set; in
 *  this case parameter validation is caller responsibility.
 * @param flags PLL specific flags. Thse flags are valid only for some PLLs,
 *  see @NvRmPllConfigFlags.
 */
void
NvRmPrivAp15PllSet(
    NvRmDeviceHandle hRmDevice,
    const NvRmPllClockInfo* pCinfo,
    NvU32 M,
    NvU32 N,
    NvU32 P,
    NvU32 StableDelayUs,
    NvU32 cpcon,
    NvU32 lfcon,
    NvBool TypicalControls,
    NvU32 flags);

/**
 * Configures output frequency for specified PLL.
 * 
 * @param hRmDevice The RM device handle.
 * @param PllId Targeted PLL ID.
 * @param MaxOutKHz Upper limit for PLL output frequency.
 * @param pPllOutKHz A pointer to the requested PLL frequency on entry,
 *  and to the actually configured frequency on exit.
 */
void
NvRmPrivAp15PllConfigureSimple(
    NvRmDeviceHandle hRmDevice,
    NvRmClockSource PllId,
    NvRmFreqKHz MaxOutKHz,
    NvRmFreqKHz* pPllOutKHz);

/**
 * Configures specified PLL output to the CM of fixed HDMI frequencies.
 * 
 * @param hRmDevice The RM device handle.
 * @param PllId Targeted PLL ID.
 * @param pPllOutKHz A pointer to the actually configured frequency on exit.
 */
void
NvRmPrivAp15PllConfigureHdmi(
    NvRmDeviceHandle hRmDevice,
    NvRmClockSource PllId,
    NvRmFreqKHz* pPllOutKHz);

/**
 * Gets PLL output frequency.
 * 
 * @param hRmDevice The RM device handle.
 * @param pCinfo Pointer to the PLL description structure.
 * 
 * @return PLL output frequency in kHz (reference frequency if PLL
 *  is by-passed; zero if PLL is disabled and not by-passed).
 */
NvRmFreqKHz
NvRmPrivAp15PllFreqGet(
    NvRmDeviceHandle hRmDevice,
    const NvRmPllClockInfo* pCinfo);

/**
 * Determines if module clock configuration requires AP15-specific handling,
 * and configures the clock if yes.
 * 
 * @param hRmDevice The RM device handle.
 * @param pCinfo Pointer to the module clock descriptor. 
 * @param ClockSourceCount Number of module clock sources.
 * @param MinFreq Requested minimum module clock frequency.
 * @param MaxFreq Requested maximum module clock frequency.
 * @param PrefFreqList Pointer to a list of preferred frequencies sorted
 *  in the decreasing order of priority.
 * @param PrefCount Number of entries in the PrefFreqList array.
 * @param pCstate Pointer to module state structure filled in if special
 *  handling is completed.
 * @param flags Module specific flags
 *
 * @return True indicates that module clock is configured, and regular
 *  configuration should be aborted; False indicates that regular clock
 *  configuration should proceed.
 */
NvBool
NvRmPrivAp15IsModuleClockException(
    NvRmDeviceHandle hRmDevice,
    NvRmModuleClockInfo *pCinfo,
    NvU32 ClockSourceCount,
    NvRmFreqKHz MinFreq,
    NvRmFreqKHz MaxFreq,
    const NvRmFreqKHz* PrefFreqList,
    NvU32 PrefCount,
    NvRmModuleClockState* pCstate,
    NvU32 flags);

/**
 * Disables PLLs
 * 
 * @param hRmDevice The RM device handle.
 * @param pCinfo Pointer to the last configured module clock descriptor. 
 * @param pCstate Pointer to the last configured module state structure.
 */
void
NvRmPrivAp15DisablePLLs(
    NvRmDeviceHandle hRmDevice,
    const NvRmModuleClockInfo* pCinfo,
    const NvRmModuleClockState* pCstate);

/**
 * Turns PLLD (MIPI PLL) power On/Off
 * 
 * @param hRmDevice The RM device handle.
 * @param ConfigEntry NV_TRUE if this function is called before display
 *  clock configuration; NV_FALSE otherwise.
 * @param Pointer to the current state of MIPI PLL power rail, updated
 *  by this function.
 */
void
NvRmPrivAp15PllDPowerControl(
    NvRmDeviceHandle hRmDevice,
    NvBool ConfigEntry,
    NvBool* pMipiPllVddOn);

/**
 * Configures some special bits in the clock source register for given module.
 * 
 * @param hRmDevice The RM device handle.
 * @param Module Target module ID.
 * @param ClkSourceOffset Clock source register offset.
 * @param flags Module specific clock configuration flags.
 */
void
NvRmPrivAp15ClockConfigEx(
    NvRmDeviceHandle hRmDevice,
    NvRmModuleID Module,
    NvU32 ClkSourceOffset,
    NvU32 flags);

/**
 * Enables PLL in simulation.
 * 
 * @param hRmDevice The RM device handle.
 */
void NvRmPrivAp15SimPllInit(NvRmDeviceHandle hRmDevice);

/**
 * Initializes oscillator and pll reference frequencies.
 *
 * @param hRmDevice The RM device handle.
 * @param pOscKHz A pointer to the variable, which returns osc frequency.
 * @param pPllRefKHz A pointer to the variable, which returns pll
 *  reference frequency.
 */
void
NvRmPrivAp15OscPllRefFreqInit(
    NvRmDeviceHandle hRmDevice,
    NvRmFreqKHz* pOscKHz,
    NvRmFreqKHz* pPllRefKHz);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  // INCLUDED_AP15RM_CLOCKS_H 