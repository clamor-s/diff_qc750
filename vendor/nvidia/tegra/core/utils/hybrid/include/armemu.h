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

#ifndef __ARMEMU_H
#define __ARMEMU_H

/*enable/disable fpu*/
#define USE_VFP 1

#include "armemulator.h"
#include "armdisassembler.h"
#include "armdefines.h"

/*
-V3 introduced 32-bit addressing, and architecture variants:
    -T = Thumb state: 16-bit instruction execution.
    -M = long multiply support (32 x 32 => 64 or 32 x 32 + 64 => 64). This
         feature became standard in architecture V4 onwards.
-V4 added halfword load and store.
-V5 improved ARM and Thumb interworking, count leading-zeroes (CLZ)
    instruction, and architecture variants:
    -E = enhanced DSP instructions including saturated arithmetic operations
         and 16-bit multiply operations
    -J = support for new Java state, offering hardware and optimized software
         acceleration of bytecode execution.

All of the enhancements above become part of the new ARMv6 architecture
specification.
*/

/*TODO from ARM8 on IMB (instruction memory barrier) must be executed before running constructed code */
/*TODO check that writes to PC have bits 0 and 1 clear */


typedef struct
{
    /*a program ends by writing 0 to R15 (PC) */
    /*give info about ARM version required for an instruction */
    /*calculate clock cycles (worst case, average?) */
    /*operates only in the user mode, no exceptions/IRQs/FIQs are emulated */
    /*PC can be a variable number of bytes ahead depending on the situation and processor/architecture */
    /*undefined instructions are trapped and the processor enters in the undefined mode (not emulated) */

    int                 m_prefetch;         /*the program counter is this many bytes ahead of the execution */
    int                 m_storePrefetch;    /*the program counter is this many bytes ahead of the execution in some rare cases on older processors */
    ARMEmulator_processor       m_processor;
    ARMEmulator_endianess       m_endianess;
    unsigned int        m_registerFile[16]; /*R15 is the program counter (PC) */
    unsigned int        m_CPSR;             /*condition codes + processor state */
#if USE_VFP
    unsigned int        m_vfpRegisterFile[32];  /*f0-f31,d0-d15 */
    unsigned int        m_FPSCR;
    unsigned int        m_FPSID;
    unsigned int        m_FPEXC;
#endif
    const char*         m_disassembly;
    const char*         m_disassemblyNotes;
    void*               m_disassembler;
} ARMEmulator;

/*  void            ARMEmulator_addMemoryBlock( unsigned int address, unsigned int length );
void            ARMEmulator_removeMemoryBlock( unsigned int address );
*/
void            ARMEmulator_assertAddressInRange(ARMEmulator* emu, unsigned int address);

HG_INLINE void  ARMEmulator_setStartAddress(ARMEmulator* emu, unsigned int address)     { emu->m_registerFile[15] = address; }

HG_INLINE ARMEmulator_processor     ARMEmulator_getProcessor(const ARMEmulator* emu)                    { return emu->m_processor; }
HG_INLINE ARMEmulator_endianess     ARMEmulator_getEndianess(const ARMEmulator* emu)                    { return emu->m_endianess; }
HG_INLINE int                       ARMEmulator_getInstructionPrefetch(const ARMEmulator* emu)          { return emu->m_prefetch; }
HG_INLINE unsigned int              ARMEmulator_getCPSR(const ARMEmulator* emu)                         { return emu->m_CPSR; }

#if 0
struct MemoryBlock
{
    unsigned int    m_address;
    unsigned int    m_length;   /*in bytes*/

    bool operator==(const MemoryBlock &b) const { return m_address == b.m_address; }
    bool operator<(const MemoryBlock &b) const { return m_address < b.m_address; }
};

typedef std::set<MemoryBlock> mbtype;
std::set<MemoryBlock>   m_memoryBlocks;
#endif

/* Sign extend an N-bit value (N does not include the sign bit!) */
HG_INLINE int signExtend(int value, int nBits)
{
    return (value << (31 - nBits))>>(31-nBits);
}

void    ARMEmulator_fetchOperand2Immediate(ARMEmulator* emu, unsigned int instruction, unsigned int* operand2, unsigned int* carry);
void    ARMEmulator_fetchOperand2Register(ARMEmulator* emu, unsigned int instruction, unsigned int* operand2, unsigned int* carry);
void    ARMEmulator_fetchOperand2(ARMEmulator* emu, unsigned int instruction, unsigned int* operand2, unsigned int* carry);
void    ARMEmulator_setConditionCodesLogical(ARMEmulator* emu, unsigned int instruction, unsigned int result, unsigned int carry);
void    ARMEmulator_setConditionCodesArithmetic(ARMEmulator* emu, unsigned int instruction, unsigned int result, HGbool V, HGbool C);
unsigned int ARMEmulator_adder(unsigned int a, unsigned int b, unsigned int ci, HGbool* V, HGbool* C);

unsigned int    ARMEmulator_loadByte(ARMEmulator* emu, unsigned int address);
unsigned int    ARMEmulator_loadHalfword(ARMEmulator* emu, unsigned int address);
unsigned int    ARMEmulator_loadWord(ARMEmulator* emu, unsigned int address);
void    ARMEmulator_storeByte(ARMEmulator* emu, unsigned int data, unsigned int address);
void    ARMEmulator_storeHalfword(ARMEmulator* emu, unsigned int data, unsigned int address);
void    ARMEmulator_storeWord(ARMEmulator* emu, unsigned int data, unsigned int address);

HGbool  ARMEmulator_executeBranch(ARMEmulator* emu, unsigned int instruction);
HGbool  ARMEmulator_executeDataProcessing(ARMEmulator* emu, unsigned int instruction);
HGbool  ARMEmulator_executePSRTransfer(ARMEmulator* emu, unsigned int instruction);
HGbool  ARMEmulator_executeMultiply(ARMEmulator* emu, unsigned int instruction);
HGbool  ARMEmulator_executeMultiplyLong(ARMEmulator* emu, unsigned int instruction);
HGbool  ARMEmulator_executeSingleDataTransfer(ARMEmulator* emu, unsigned int instruction);
HGbool  ARMEmulator_executeHalfwordAndSignedDataTransfer(ARMEmulator* emu, unsigned int instruction);
HGbool  ARMEmulator_executeBlockDataTransfer(ARMEmulator* emu, unsigned int instruction);
HGbool  ARMEmulator_executeSingleDataSwap(ARMEmulator* emu, unsigned int instruction);
HGbool  ARMEmulator_executeClz(ARMEmulator* emu, unsigned int instruction);

#if USE_VFP
HGbool  ARMEmulator_executeVFPDataProcessing(ARMEmulator* emu, unsigned int instruction);
HGbool  ARMEmulator_executeVFPLoadAndStore(ARMEmulator* emu, unsigned int instruction);
HGbool  ARMEmulator_executeVFPRegisterTransfer(ARMEmulator* emu, unsigned int instruction);
#endif

#endif
