/*
 * Copyright (c) 2007 - 2009 NVIDIA Corporation.  All rights reserved.
 * 
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVBOOT_OSC_H
#define INCLUDED_NVBOOT_OSC_H

/**
 * Defines the oscillator frequencies supported by the hardware.
 */

#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * Set of oscillator frequencies supported by the hardware.
 */
/*
 * Set of oscillator frequencies supproted in the internal API
 * + invalid (measured but not in any valid band)
 * + unknown (not measured at all)
 * The order of the enum MUST match the HW definition in OSC_CTRL for the first
 * four values.  Note that this is a violation of the SW convention on enums.
 */
typedef enum
{  
    /// Specifies an oscillator frequency of 13MHz.
    NvBootClocksOscFreq_13 = 0x0,

    /// Specifies an oscillator frequency of 19.2MHz.
    NvBootClocksOscFreq_19_2,

    /// Specifies an oscillator frequency of 12MHz.
    NvBootClocksOscFreq_12,

    /// Specifies an oscillator frequency of 26MHz.
    NvBootClocksOscFreq_26,

    NvBootClocksOscFreq_Num,       // dummy to get number of frequencies
    NvBootClocksOscFreq_Unknown,
    NvBootClocksOscFreq_Force32 = 0x7fffffff
} NvBootClocksOscFreq;

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_NVBOOT_OSC_H */
