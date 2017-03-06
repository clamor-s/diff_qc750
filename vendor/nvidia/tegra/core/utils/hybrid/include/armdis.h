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

#ifndef __ARMDIS_H
#define __ARMDIS_H

#include "armdefines.h"
#include "armdisassembler.h"

typedef struct
{
    /*give info about ARM version required for an instruction*/
    /*calculate clock cycles (worst case, average?)*/
    /*operates only in the user mode, no exceptions/IRQs/FIQs are emulated*/
    /*PC can be a variable number of bytes ahead depending on the situation and processor/architecture*/
    /*undefined instructions are trapped and the processor enters in the undefined mode (not emulated)*/
    /*a program ends by writing 0 to R15 (PC)*/

    int             m_prefetch;         /*the program counter is this many bytes ahead of the execution*/
    ARMDisassembler_processor       m_processor;
    ARMDisassembler_endianess       m_endianess;
    char            m_string[256];
    int             m_index;
    char            m_notes[2048];
    int             m_noteIndex;
    HGbool          m_hex;
} ARMDisassembler;

/* Sign extend an N-bit value (N does not include the sign bit!)*/
HG_INLINE int signExtend(int value, int nBits)
{
    return (value << (31 - nBits))>>(31-nBits);
}

void    ARMDisassembler_decodeOperand2Immediate(ARMDisassembler* dis, unsigned int instruction);
void    ARMDisassembler_decodeOperand2Register(ARMDisassembler* dis, unsigned int instruction);
void    ARMDisassembler_decodeOperand2(ARMDisassembler* dis, unsigned int instruction);
void    ARMDisassembler_writeConditionCode(ARMDisassembler* dis, unsigned int instruction);
HGbool  ARMDisassembler_testConditionCode(ARMDisassembler* dis, unsigned int code);

unsigned int ARMDisassembler_instructionFetch(ARMDisassembler* dis, unsigned int address);
unsigned int ARMDisassembler_loadWord(ARMDisassembler* dis, unsigned int address);

void    ARMDisassembler_executeBranch(ARMDisassembler* dis, unsigned int instruction);
void    ARMDisassembler_executeDataProcessing(ARMDisassembler* dis, unsigned int instruction);
void    ARMDisassembler_executePSRTransfer(ARMDisassembler* dis, unsigned int instruction);
void    ARMDisassembler_executeMultiply(ARMDisassembler* dis, unsigned int instruction);
void    ARMDisassembler_executeMultiplyLong(ARMDisassembler* dis, unsigned int instruction);
void    ARMDisassembler_executeSingleDataTransfer(ARMDisassembler* dis, unsigned int instruction);
void    ARMDisassembler_executeHalfwordAndSignedDataTransfer(ARMDisassembler* dis, unsigned int instruction);
void    ARMDisassembler_executeBlockDataTransfer(ARMDisassembler* dis, unsigned int instruction);
void    ARMDisassembler_executeSingleDataSwap(ARMDisassembler* dis, unsigned int instruction);
void    ARMDisassembler_executeClz(ARMDisassembler* dis, unsigned int instruction);

void    ARMDisassembler_append(ARMDisassembler* dis, const char *format, ...);
void    ARMDisassembler_appendTab(ARMDisassembler* dis);
void    ARMDisassembler_appendNote(ARMDisassembler* dis, const char *format, ...);

#endif
