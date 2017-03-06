//
// Copyright (c) 2007-2009 NVIDIA Corporation.  All Rights Reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation.  Any
// use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation
// is strictly prohibited.
//

/**
 * @file
 * <b>NVIDIA Tegra Boot Loader API</b>
 *
 */

#ifndef INCLUDED_NVBL_TIMER_H
#define INCLUDED_NVBL_TIMER_H

/**
 * @defgroup nvbl_timer_group NvBL Timer API
 *
 *
 * @ingroup nvbl_group
 * @{
 */

#include "nvbl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------------------------
// Type Declarations
//----------------------------------------------------------------------------------------------

/**
 * @brief Defines timer identifiers.
 * The order in this enum is important, as timer instance choices
 *  elsewhere follow this order.
 */
typedef enum
{
    /// Specifies OS watch dog timer.
    NvBlTimerId_WatchDogTimer,

    /// Specifies never enable this for CPU.
    NvBlTimerId_AVP,

    /// Specifies OS profiling timer.
    NvBlTimerId_ProfTimer,

    /// Specifies OS tick timer.
    NvBlTimerId_TickTimer,

    NvBlTimerId_Max,
    /// Ignore -- Forces compilers to make 32-bit enums.
    NvBlTimerId_Force32 = 0x7FFFFFFF
} NvBlTimerId;

/**
 * @brief Defines timer types.
 */
typedef enum
{
    /// Specifies a one-shot timer (fires once).
    NvBlTimerType_Oneshot,

    /// Specifies to fires repeatedly at specified period.
    NvBlTimerType_Periodic,

    NvBlTimerType_Max,
    /// Ignore -- Forces compilers to make 32-bit enums.
    NvBlTimerType_Force32 = 0x7FFFFFFF

} NvBlTimerType;


/**
 * @brief Defines timer state.
 */
typedef enum
{
    /// Specifies timer is disabled.
    NvBlTimerState_Disable,

    /// Specifies timer is enabled.
    NvBlTimerState_Enable,

    NvBlTimerState_Max,
    /// Ignore -- Forces compilers to make 32-bit enums.
    NvBlTimerState_Force32 = 0x7FFFFFFF

} NvBlTimerState;


/*
 * @brief Holds timer properties.
 */
typedef struct
{
    /// Holds timer type.
    NvBlTimerType   Type;

    /// Holds timer enable/disable state.
    NvBlTimerState  State;

    /// Holds number of 1 us ticks per interrupt.
    NvU32   TickCount;

} NvBlTimerProperties;

//----------------------------------------------------------------------------------------------
// Function Prototypes
//----------------------------------------------------------------------------------------------

/**
 * @brief Initializes a timer.
 *
 * @param Id The timer identifier.
 * @param pTimerProperties A pointer to timer initialization properties.
 *
 * @retval ::NvError_Success
 * @retval ::NvError_NotInitialized
 * @retval ::NvError_BadParameter
 */
NvError NvBlTimerInit(NvBlTimerId Id, const NvBlTimerProperties* pTimerProperties);

/**
 * @brief Changes the state of a timer.
 *
 * @param Id The timer identifier.
 * @param State The new state for the timer.
 *
 * @retval ::NvError_Success
 * @retval ::NvError_NotInitialized
 * @retval ::NvError_BadParameter
 */
NvError NvBlTimerChangeState(NvBlTimerId Id, NvBlTimerState State);

/**
 * @brief Returns the current state of a timer.
 *
 * @param Id The timer identifier.
 *
 * @retval ::NvBlTimerState_Enable
 * @retval ::NvBlTimerState_Disable
 */
NvBlTimerState    NvBlTimerGetState(NvBlTimerId Id);

/**
 * @brief Timer interrupt handler to acknowledge (clear) a timer interrupt.
 *
 * @param Id The timer identifier.
 *
 * @retval ::NvError_Success
 * @retval ::NvError_NotInitialized
 * @retval ::NvError_BadParameter
 */
NvError  NvBlTimerIntClear(NvBlTimerId Id);

/**
 * @brief Gets the elapsed time from the free-running microsecond counter.
 * @returns Elapsed time in microseconds.
 */
NvU32 NvBlTimerGetMicroseconds(void);


/**
 * @brief Stalls CPU execution for a specified number of microseconds.
 *
 * @pre Must not be called from the AVP.
 *
 * @param microsec The number of microseconds to delay.
 */
void NvBlCpuStallUs(NvU32 microsec);


/**
 * @brief Delays CPU execution for a specified number of milliseconds.
 *
 * @pre Must not be called from the AVP.
 *
 * @param millisec The number of milliseconds to delay.
 */
void NvBlCpuStallMs(NvU32 millisec);


/**
 * @brief Updates the periodic timer.
 *
 * @param Id The timer identifier.
 *
 * @param pTimerProperties A pointer to the properties of the updated timer.
 *
 * @returns An error if timer ID or counts value is incorrect or if timer could not be updated.
 */
NvError NvBlTimerUpdate(NvBlTimerId Id, const NvBlTimerProperties* pTimerProperties);

/**
 * @brief Updates the timer counter.
 *
 * @param Id The timer identifier.
 *
 * @param counts The high resolution timer counts.
 *
 * @returns An error if counts value is incorrect or if timer could not be updated.
 */
NvError NvBlTimerUpdateCount(NvBlTimerId Id, NvU32 counts);

/**
 * @brief Updates the timer counter during OS idle mode entry.
 *
 * @param Id The timer identifier.
 *
 * @param counts The high resolution timer counts.
 *
 * @returns An error if input parameters are incorrect or if timer could not be updated.
 */
NvError NvBlTimerUpdate_IdleEntry(NvBlTimerId Id, NvU32 counts);

/**
 * @brief Updates the timer counter during OS idle mode exit.
 *
 * @param Id The timer identifier.
 *
 * @param counts The high resolution timer counts.
 *
 * @returns An error if input parameters are incorrect or if timer could not be updated.
 */
NvError NvBlTimerUpdate_IdleExit(NvBlTimerId Id, NvU32 counts);

/**
 * @brief Gets the value to be programmed to current microsecond counter.
 *
 * @pre This function should only be called after exiting from power save mode.
 *      This is not a generic function.
 *
 * @param USecVal The suggested microsecond counter value.
 *
 * @param powerState The power state from which system has just exited.
 *
 * @returns The current microsecond count for OS.
 */
NvU32 NvBlTimerGetUSecCntProgVal(NvU32 USecVal, NvU32 powerState);


//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif
/** @} */

#endif // INCLUDED_NVBL_TIMER_H

