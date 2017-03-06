/*------------------------------------------------------------------------
 *
 * Hybrid ARM emulator
 * -------------------
 *
 * (C) 2004-2005 Hybrid Graphics, Ltd.
 * All Rights Reserved.
 *
 * This file consists of unpublished, proprietary source code of
 * Hybrid Graphics, and is considered Confidential Information for
 * purposes of non-disclosure agreement. Disclosure outside the terms
 * outlined in signed agreement may result in irreparable harm to
 * Hybrid Graphics and legal action against the party in breach.
 *
 *//*-------------------------------------------------------------------*/

#ifndef __ARMEMULATOR_H
#define __ARMEMULATOR_H

#include "hgdefs.h"

HG_EXTERN_C_BLOCK_BEGIN

typedef enum
{
    ARMEMULATOR_ARM7,
    ARMEMULATOR_ARM8
} ARMEmulator_processor;

typedef enum
{
    ARMEMULATOR_LITTLEENDIAN,
    ARMEMULATOR_BIGENDIAN
} ARMEmulator_endianess;

/* currently supports ARMv4 instruction set */

void*           ARMEmulator_create (ARMEmulator_processor p, ARMEmulator_endianess e, HGbool enableDisassembly);
void            ARMEmulator_destroy (void* emulator);
void            ARMEmulator_run (void* emulator, const void* startAddress);
void            ARMEmulator_setRegister (void* emulator, int r, unsigned int a);
unsigned int    ARMEmulator_getRegister (const void* emulator, int r);

HG_EXTERN_C_BLOCK_END

#endif /* __ARMEMULATOR_H */
