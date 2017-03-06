/*------------------------------------------------------------------------
 *
 * Hybrid ARM disassembler
 * --------------------------
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

#ifndef __ARMDISASSEMBLER_H
#define __ARMDISASSEMBLER_H

#include "hgdefs.h"

HG_EXTERN_C_BLOCK_BEGIN

typedef enum
{
    ARMDISASSEMBLER_ARM7,
    ARMDISASSEMBLER_ARM8
} ARMDisassembler_processor;

typedef enum
{
    ARMDISASSEMBLER_LITTLEENDIAN,
    ARMDISASSEMBLER_BIGENDIAN
} ARMDisassembler_endianess;

/* currently supports ARMv4 instruction set */

/* this function disassembles and prints a block of code. Comments can be NULL if no comments are available */
void ARMDisassembler_disassemble(ARMDisassembler_processor p, ARMDisassembler_endianess e, HGbool hexImmediates, const unsigned int* addr, int numInstructions, const char** comments, int* commentOffsets, int numComments);

/* these functions can be used to implement disassembly in your own format */
void*       ARMDisassembler_create(ARMDisassembler_processor p, ARMDisassembler_endianess e, HGbool hexImmediates);
void        ARMDisassembler_destroy(void* disassembler);
const char* ARMDisassembler_decode(void* disassembler, const char** notes, unsigned int ARMBinaryInstruction);
HGbool      ARMDisassembler_isBranch(void* disassembler, unsigned int ARMBinaryInstruction);
int         ARMDisassembler_getBranchOffset(void* disassembler, unsigned int ARMBinaryInstruction);

HG_EXTERN_C_BLOCK_END

#endif /* __ARMDISASSEMBLER_H */
