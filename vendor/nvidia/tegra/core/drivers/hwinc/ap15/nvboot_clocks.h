/**
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/*
 * nvboot_clocks.h - Useful definitions for clocks.
 */

#ifndef INCLUDED_NVBOOT_CLOCKS_H
#define INCLUDED_NVBOOT_CLOCKS_H

#if defined(__cplusplus)
extern "C"
{
#endif

/* set of valid count ranges per frequency, as define 
 * measuring 13 gives exactly 406 e.g. 
 * The chosen range parameter is:
 * - more than the expected combined frequency deviation
 * - less than half the  relative distance between 12 and 13
 * - expressed as a ratio against a power of two to avoid floating point
 * - so that intermediate overflow is not possible
 *
 * The macros are defined for a frequency of 32768 Hz (not 32000 Hz). 
 * They uses 2^-5 ranges, or about 3.2% and dispenses with rounding
 * Also need to use the full value in Hz in the macro
 */

#define NVBOOT_CLOCKS_MIN_RANGE(F) (( F - (F>>5) - (1<<15) + 1 ) >> 15)
#define NVBOOT_CLOCKS_MAX_RANGE(F) (( F + (F>>5) + (1<<15) - 1 ) >> 15)

// to have an easier ECO (keeping same number of instructions), we need a
// special case for 12 min range
#define NVBOOT_CLOCKS_MIN_CNT_12 (NVBOOT_CLOCKS_MIN_RANGE(12000000) -1)
#define NVBOOT_CLOCKS_MAX_CNT_12 NVBOOT_CLOCKS_MAX_RANGE(12000000)

#define NVBOOT_CLOCKS_MIN_CNT_13 NVBOOT_CLOCKS_MIN_RANGE(13000000)
#define NVBOOT_CLOCKS_MAX_CNT_13 NVBOOT_CLOCKS_MAX_RANGE(13000000)

#define NVBOOT_CLOCKS_MIN_CNT_19_2 NVBOOT_CLOCKS_MIN_RANGE(19200000)
#define NVBOOT_CLOCKS_MAX_CNT_19_2 NVBOOT_CLOCKS_MAX_RANGE(19200000)

#define NVBOOT_CLOCKS_MIN_CNT_26 NVBOOT_CLOCKS_MIN_RANGE(26000000)
#define NVBOOT_CLOCKS_MAX_CNT_26 NVBOOT_CLOCKS_MAX_RANGE(26000000)

// The stabilization delay in usec 
#define NVBOOT_CLOCKS_PLL_STABILIZATION_DELAY (300) 

// other values important when starting PLLP
#define NVBOOT_CLOCKS_PLLP_CPCON_DEFAULT (8)
#define NVBOOT_CLOCKS_PLLP_CPCON_19_2 (1)

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_NVBOOT_CLOCKS_H */
