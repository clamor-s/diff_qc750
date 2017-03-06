/*
 * Copyright (c) 2007 - 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */


#ifndef INCLUDED_T30RM_CLOCKS_H
#define INCLUDED_T30RM_CLOCKS_H

#include "nvrm_clocks.h"
#include "nvodm_query_memc.h"

#ifdef __cplusplus
extern "C"
{
#endif  /* __cplusplus */

extern const NvRmModuleClockInfo g_T30ModuleClockTable[];
extern const NvU32 g_T30ModuleClockTableSize;

/**
 * Enables/disables module clock.
 *
 * @param hDevice The RM device handle.
 * @param ModuleId Combined module ID and instance of the target module.
 * @param ClockState Target clock state.
 */
void
T30EnableModuleClock(
    NvRmDeviceHandle hDevice,
    NvRmModuleID ModuleId,
    ModuleClockState ClockState);

/**
 * Resets module (assert/delay/deassert reset signal) if the hold paramter is
 * NV_FLASE. If the hols paramter is NV_TRUE, just assert the reset and return.
 *
 * @param hDevice The RM device handle.
 * @param Module Combined module ID and instance of the target module.
 * @param hold      To hold or relese the reset.
 */
void
T30ModuleReset(NvRmDeviceHandle hDevice, NvRmModuleID ModuleId, NvBool hold);

/**
 * Resets 2D module.
 *
 * @param hRmDevice The RM device handle.
 */
void
NvRmPrivT30Reset2D(NvRmDeviceHandle hRmDevice);

/**
 * Initializes clock source table.
 *
 * @return Pointer to the clock sources descriptor table.
 */
NvRmClockSourceInfo* NvRmPrivT30ClockSourceTableInit(void);

/**
 * Initializes PLL references table.
 *
 * @param pPllReferencesTable A pointer to a pointer which this function sets
 *  to the PLL reference table base.
 * @param pPllReferencesTableSize A pointer to a variable which this function
 *  sets to the PLL reference table size.
 */
void
NvRmPrivT30PllReferenceTableInit(
    NvRmPllReference** pPllReferencesTable,
    NvU32* pPllReferencesTableSize);

/**
 * Configures oscillator (main) clock doubler.
 *
 * @param hRmDevice The RM device handle.
 * @param OscKHz Oscillator (main) clock frequency in kHz.
 *
 * @return NvSuccess if the specified oscillator frequency is supported, and
 * NvError_NotSupported, otherwise.
 */
NvError
NvRmPrivT30OscDoublerConfigure(
    NvRmDeviceHandle hRmDevice,
    NvRmFreqKHz OscKHz);

/**
 * Initializes oscillator and pll reference frequencies.
 *
 * @param hRmDevice The RM device handle.
 * @param pOscKHz A pointer to the variable, which returns osc frequency.
 * @param pPllRefKHz A pointer to the variable, which returns pll
 *  reference frequency.
 */
void
NvRmPrivT30OscPllRefFreqInit(
    NvRmDeviceHandle hRmDevice,
    NvRmFreqKHz* pOscKHz,
    NvRmFreqKHz* pPllRefKHz);

/**
 * Controls PLLE.
 *
 * @param hRmDevice The RM device handle.
 * @param Enable Specifies if PLLE should be enabled or disabled (PLLE power
 *  management is not supported, and it is never disabled as of now).
 */
void NvRmPrivT30PllEControl(NvRmDeviceHandle hRmDevice, NvBool Enable);

void NvRmPrivT30FastClockConfig(NvRmDeviceHandle hRmDevice);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  // INCLUDED_T30RM_CLOCKS_H
