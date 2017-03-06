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

#include "armemu.h"
#include <stdlib.h>
#include <string.h>

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void* ARMEmulator_create(ARMEmulator_processor p, ARMEmulator_endianess e, HGbool enableDisassembly)
{
    ARMEmulator* emu = (ARMEmulator*)malloc(sizeof(ARMEmulator));
    memset(emu, 0, sizeof(ARMEmulator));

    emu->m_prefetch = 8;
    emu->m_storePrefetch = 12;
    emu->m_processor = p;
    emu->m_endianess = e;
    emu->m_CPSR = 0;

    switch( p )
    {
    case ARMEMULATOR_ARM7:
        emu->m_storePrefetch = 12;  /*ARM<=7 = 12 */
        break;
    case ARMEMULATOR_ARM8:
        emu->m_storePrefetch = 8;   /*ARM8 = 8 */
        break;
    default:
        HG_ASSERT(0);   /*undefined processor */
        break;
    }

    emu->m_disassembler = HG_NULL;
    if(enableDisassembly)
        emu->m_disassembler = ARMDisassembler_create(
            (ARMDisassembler_processor)p,
            (ARMDisassembler_endianess)e,
            HG_FALSE);

#if USE_VFP
    emu->m_FPSID = 0x41000000;
    emu->m_FPSID = 0;
    emu->m_FPEXC = 0;
#endif

/*  m_memoryBlocks.clear(); */
    return emu;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMEmulator_destroy(void* emu)
{
    HG_ASSERT(emu);
    if(((ARMEmulator*)emu)->m_disassembler)
        ARMDisassembler_destroy(((ARMEmulator*)emu)->m_disassembler);
    free(((ARMEmulator*)emu));
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

unsigned int ARMEmulator_getRegister(const void* emu, int r)
{
    HG_ASSERT(emu);
    HG_ASSERT( r >= 0 && r < 16 );
    return ((const ARMEmulator*)emu)->m_registerFile[r];
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMEmulator_setRegister(void* emu, int r, unsigned int a)
{
    HG_ASSERT(emu);
    HG_ASSERT( r >= 0 && r < 16 );
    ((ARMEmulator*)emu)->m_registerFile[r] = a;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

HG_INLINE HGbool ARMEmulator_testConditionCode(ARMEmulator* emu, unsigned int instruction )
{
    unsigned int code = (instruction >> ConditionCodeShift) & ConditionCodeMask;
    switch( code )
    {
    case EQ:    return  (emu->m_CPSR & FlagZ);
    case NE:    return !(emu->m_CPSR & FlagZ);
    case CS:    return  (emu->m_CPSR & FlagC);
    case CC:    return !(emu->m_CPSR & FlagC);
    case MI:    return  (emu->m_CPSR & FlagN);
    case PL:    return !(emu->m_CPSR & FlagN);
    case VS:    return  (emu->m_CPSR & FlagV);
    case VC:    return !(emu->m_CPSR & FlagV);
    case HI:    return ((emu->m_CPSR & FlagC) && !(emu->m_CPSR & FlagZ));
    case LS:    return (!(emu->m_CPSR & FlagC) || (emu->m_CPSR & FlagZ));
    case GE:    return (((emu->m_CPSR & FlagN) && (emu->m_CPSR & FlagV)) || (!(emu->m_CPSR & FlagN) && !(emu->m_CPSR & FlagV)));
    case LT:    return (((emu->m_CPSR & FlagN) && !(emu->m_CPSR & FlagV)) || (!(emu->m_CPSR & FlagN) && (emu->m_CPSR & FlagV)));
    case GT:    return (!(emu->m_CPSR & FlagZ) && (((emu->m_CPSR & FlagN) && (emu->m_CPSR & FlagV)) || (!(emu->m_CPSR & FlagN) && !(emu->m_CPSR & FlagV))));
    case LE:    return ((emu->m_CPSR & FlagZ) || ((emu->m_CPSR & FlagN) && !(emu->m_CPSR & FlagV)) || (!(emu->m_CPSR & FlagN) && (emu->m_CPSR & FlagV)));
    case AL:    return HG_TRUE;
    case NV:
        HG_ASSERT(HG_FALSE);    /*should not be used (ARM7) */
        return HG_FALSE;
    default:
        HG_ASSERT(HG_FALSE);    /*should never get here. if it does, there's a bug in the emulator */
        break;
    }
    HG_ASSERT(HG_FALSE);    /*should never get here. if it does, there's a bug in the emulator */
    return HG_FALSE;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

HG_INLINE void ARMEmulator_decodeAndExecute(ARMEmulator* emu, unsigned int instruction)
{
    const unsigned int InstructionTypeMask = 0x0c000000;
    const unsigned int InstructionTypeShift = 26;

    const unsigned int MultiplyMask = 0x0fc000f0;       /*0000 1111 1100 0000 0000 0000 1111 0000 */
    const unsigned int MultiplyOpcode = 0x00000090;     /*0000 0000 0000 0000 0000 0000 1001 0000 */
    const unsigned int MultiplyLongMask = 0x0f8000f0;   /*0000 1111 1000 0000 0000 0000 1111 0000 */
    const unsigned int MultiplyLongOpcode = 0x00800090; /*0000 0000 1000 0000 0000 0000 1001 0000 */
    const unsigned int ClzMask = 0x0ff000f0;            /*0000 1111 1111 0000 0000 0000 1111 0000 */
    const unsigned int ClzOpcode = 0x01600010;          /*0000 0001 0110 0000 0000 0000 0001 0000 */
    const unsigned int SwapMask = 0x0fb00ff0;           /*0000 1111 1011 0000 0000 1111 1111 0000 */
    const unsigned int SwapOpcode = 0x01000090;         /*0000 0001 0000 0000 0000 0000 1001 0000 */
    /*NOTE: these match also the swap instruction. therefore these must be checked AFTER checking swap! */
    const unsigned int HalfwordAndSignedDataMask = 0x0e000090;      /*0000 1110 0000 0000 0000 1111 1001 0000 */
    const unsigned int HalfwordAndSignedDataOpcode = 0x00000090;    /*0000 0000 0000 0000 0000 0000 1001 0000 */

    HGbool wroteR15 = HG_FALSE;
    unsigned int instructionType = (instruction & InstructionTypeMask) >> InstructionTypeShift;

    if(emu->m_disassembler)
        emu->m_disassembly = ARMDisassembler_decode(emu->m_disassembler, &emu->m_disassemblyNotes, instruction);

    /*check the condition code to see if the instruction should be executed */
    if( !ARMEmulator_testConditionCode(emu, instruction) )
    {
        emu->m_registerFile[15] += 4;   /*advance to the next instruction */
        return;
    }

    switch( instructionType )
    {
    case 0x0:   /*data processing, clz, psr transfer, multiply, halfword or signed transfer, or single data swap */
        if( (instruction & MultiplyMask) == MultiplyOpcode )
            wroteR15 = ARMEmulator_executeMultiply(emu, instruction );
        else if( (instruction & MultiplyLongMask) == MultiplyLongOpcode )
            wroteR15 = ARMEmulator_executeMultiplyLong(emu, instruction );
        else if( (instruction & ClzMask) == ClzOpcode )
            wroteR15 = ARMEmulator_executeClz(emu, instruction);
        else if( (instruction & SwapMask) == SwapOpcode )
            wroteR15 = ARMEmulator_executeSingleDataSwap(emu, instruction );
        else if( (instruction & HalfwordAndSignedDataMask) == HalfwordAndSignedDataOpcode )
            wroteR15 = ARMEmulator_executeHalfwordAndSignedDataTransfer(emu, instruction );
        else
        {   /*data processing or psr transfer */
            unsigned int opcode = instruction & 0x01900000; /*extract bits 20(S),23 and 24 */
            /*psr transfer instructions are encoded as TST/TEQ/CMP/CMN with S-bit clear */
            if( opcode == 0x01000000 )
                wroteR15 = ARMEmulator_executePSRTransfer(emu, instruction );
            else
                wroteR15 = ARMEmulator_executeDataProcessing(emu, instruction );
        }
        break;
    case 0x1:   /*single data transfer or undefined */
        /*UndefinedMask   = 0000 1110 0000 0000 0000 0000 0001 0000 */
        /*UndefinedOpcode = 0000 0110 0000 0000 0000 0000 0001 0000 */
        if( (instruction & 0x0e000010) == 0x06000010 )
        {
            HG_ASSERT(0);   /*undefined instructions are not emulated */
        }
        else
            wroteR15 = ARMEmulator_executeSingleDataTransfer(emu, instruction );
        break;
    case 0x2:   /*block data transfer or branch */
        if( (instruction & 0x02000000) )
            wroteR15 = ARMEmulator_executeBranch(emu, instruction );    /*no advance to the next instruction */
        else
        {
            wroteR15 = ARMEmulator_executeBlockDataTransfer(emu, instruction );
        }
        break;
    case 0x3:   /*coprocessor or software interrupt */
#if USE_VFP
        if( (instruction & 0x0f000010 ) == 0x0e000000)
        {
            wroteR15 = ARMEmulator_executeVFPDataProcessing(emu, instruction );
        }
        else if( (instruction & 0x0e000000 ) == 0x0c000000)
        {
            wroteR15 = ARMEmulator_executeVFPLoadAndStore(emu, instruction );
        }
        if( (instruction & 0x0f000070 ) == 0x0e000010)
        {
            wroteR15 = ARMEmulator_executeVFPRegisterTransfer(emu, instruction );
        }
        else
        {
            HG_ASSERT(0);   /*not emulated */
        }
#else
        HG_ASSERT(0);   /*not emulated */
#endif
        break;
    default:
        HG_ASSERT(0);   /*should never get here. if it does, there's a bug in the emulation code */
        break;
    };

    /*if R15 was not written during execution, increment it */
    if( !wroteR15 )
        emu->m_registerFile[15] += 4;   /*advance to the next instruction */
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

HG_INLINE HGbool ARMEmulator_instructionFetch(ARMEmulator* emu, unsigned int* instruction)
{
    HG_ASSERT(!(emu->m_registerFile[15] & 3));  /*check that the instruction address is properly aligned */
    if( emu->m_registerFile[15] == 0 )
        return HG_FALSE;    /*end of program. */
    *instruction = ARMEmulator_loadWord(emu, emu->m_registerFile[15]);
    return HG_TRUE;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMEmulator_run(void* emu, const void* startAddress)
{
    HG_ASSERT(emu);
    ARMEmulator_setStartAddress(((ARMEmulator*)emu), (unsigned int)startAddress);
    for(;;)
    {
        unsigned int instruction = 0;
        if( !ARMEmulator_instructionFetch(((ARMEmulator*)emu), &instruction) )
            return;
        ARMEmulator_decodeAndExecute(((ARMEmulator*)emu), instruction);
    }
}
