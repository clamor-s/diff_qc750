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

#include "armdis.h"

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_executeBranch(ARMDisassembler* dis, unsigned int instruction)
{
    const unsigned int BitL = 0x01000000;   /*0=B (branch), 1=BL (branch with link) */
    ARMDisassembler_append(dis, "b");
    if( instruction & BitL )
        ARMDisassembler_append(dis, "l");   /*branch with link */
    ARMDisassembler_writeConditionCode(dis, instruction);
    ARMDisassembler_appendTab(dis);
/*  ARMDisassembler_append(dis, "0x%04X", ARMDisassembler_getBranchOffset(dis, instruction));*/
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_executeDataProcessing(ARMDisassembler* dis, unsigned int instruction)
{
    const unsigned int BitS = 0x00100000;   /*0=do not write condition codes, 1=write condition codes */
    const unsigned int OpcodeMask = 0x01e00000;
    const unsigned int OpcodeShift = 21;
    const unsigned int FirstOperandMask = 0x000f0000;
    const unsigned int FirstOperandShift = 16;
    const unsigned int DestinationMask = 0x0000f000;
    const unsigned int DestinationShift = 12;

    int Rd = (instruction & DestinationMask) >> DestinationShift;
    int Rn = (instruction & FirstOperandMask) >> FirstOperandShift;
    unsigned int opcode = (instruction & OpcodeMask) >> OpcodeShift;
    HG_ASSERT(Rd >= 0 && Rd < 16);  /*just in case */
    HG_ASSERT(Rn >= 0 && Rn < 16);  /*just in case */

    /*If R15 (the PC) is used as an operand in a data processing instruction the register is used
      directly. The PC value will be the address of the instruction, plus 8 or 12 bytes due to
      instruction prefetching. If the shift amount is specified in the instruction, the PC will
      be 8 bytes ahead. If a register is used to specify the shift amount the PC will be 12 bytes
      ahead. */

    /*logical operations AND, EOR, TST, TEQ, ORR, MOV, BIC, MVN
      perform the logical action on all corresponding bits of the operand
      or operands to produce the result.
      operations (TST, TEQ, CMP, CMN) do not write the result to Rd
      They are used only to perform tests and to set the condition codes on the result
      and always have the S bit set.
      The arithmetic operations (SUB, RSB, ADD, ADC, SBC, RSC, CMP, CMN) treat each operand as
      a 32 bit integer (either unsigned or 2's complement signed, the two are equivalent) */

    /*When Rd is a register other than R15, the condition code flags in the CPSR may be updated
      from the ALU flags as described above.
      When Rd is R15 and the S flag in the instruction is not set the result of the operation is
      placed in R15 and the CPSR is unaffected.
      When Rd is R15 and the S flag is set the result of the operation is placed in R15 and the
      SPSR corresponding to the current mode is moved to the CPSR. This allows state changes which
      atomically restore both PC and CPSR. This form of instruction shall not be used in User mode. */

    /*HG_ASSERT(!(Rd == 15 && (instruction & BitS)));*/ /*we are always in user mode, so we don't need this */

    switch( opcode )
    {
    case AND:   /*logical */
        ARMDisassembler_append(dis, "and");
        ARMDisassembler_writeConditionCode(dis, instruction);
        if( instruction & BitS )
            ARMDisassembler_append(dis, "s");
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, r%d, ",Rd,Rn);
        break;
    case EOR:   /*logical */
        ARMDisassembler_append(dis, "eor");
        ARMDisassembler_writeConditionCode(dis, instruction);
        if( instruction & BitS )
            ARMDisassembler_append(dis, "s");
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, r%d, ",Rd,Rn);
        break;
    case SUB:   /*arithmetic */
        ARMDisassembler_append(dis, "sub");
        ARMDisassembler_writeConditionCode(dis, instruction);
        if( instruction & BitS )
            ARMDisassembler_append(dis, "s");
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, r%d, ",Rd,Rn);
        break;
    case RSB:   /*arithmetic */
        ARMDisassembler_append(dis, "rsb");
        ARMDisassembler_writeConditionCode(dis, instruction);
        if( instruction & BitS )
            ARMDisassembler_append(dis, "s");
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, r%d, ",Rd,Rn);
        break;
    case ADD:   /*arithmetic */
        ARMDisassembler_append(dis, "add");
        ARMDisassembler_writeConditionCode(dis, instruction);
        if( instruction & BitS )
            ARMDisassembler_append(dis, "s");
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, r%d, ",Rd,Rn);
        break;
    case ADC:   /*arithmetic */
        ARMDisassembler_append(dis, "adc");
        ARMDisassembler_writeConditionCode(dis, instruction);
        if( instruction & BitS )
            ARMDisassembler_append(dis, "s");
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, r%d, ",Rd,Rn);
        break;
    case SBC:   /*arithmetic */
        ARMDisassembler_append(dis, "sbc");
        ARMDisassembler_writeConditionCode(dis, instruction);
        if( instruction & BitS )
            ARMDisassembler_append(dis, "s");
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, r%d, ",Rd,Rn);
        break;
    case RSC:   /*arithmetic */
        ARMDisassembler_append(dis, "rsc");
        ARMDisassembler_writeConditionCode(dis, instruction);
        if( instruction & BitS )
            ARMDisassembler_append(dis, "s");
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, r%d, ",Rd,Rn);
        break;
    case TST:   /*logical */
        ARMDisassembler_append(dis, "tst");
        ARMDisassembler_writeConditionCode(dis, instruction);
        if(!(instruction & BitS))   /*S-bit must always be set for this instruction */
            ARMDisassembler_appendNote(dis, "warning: S-bit must always be set for this instruction");
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, ",Rn);
        break;
    case TEQ:   /*logical */
        ARMDisassembler_append(dis, "teq");
        ARMDisassembler_writeConditionCode(dis, instruction);
        if(!(instruction & BitS))   /*S-bit must always be set for this instruction */
            ARMDisassembler_appendNote(dis, "warning: S-bit must always be set for this instruction");
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, ",Rn);
        break;
    case CMP:   /*arithmetic */
        ARMDisassembler_append(dis, "cmp");
        ARMDisassembler_writeConditionCode(dis, instruction );
        if(!(instruction & BitS))   /*S-bit must always be set for this instruction */
            ARMDisassembler_appendNote(dis, "warning: S-bit must always be set for this instruction");
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, ",Rn);
        break;
    case CMN:   /*arithmetic */
        ARMDisassembler_append(dis, "cmn");
        ARMDisassembler_writeConditionCode(dis, instruction );
        if(!(instruction & BitS))   /*S-bit must always be set for this instruction */
            ARMDisassembler_appendNote(dis, "warning: S-bit must always be set for this instruction");
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, ",Rn);
        break;
    case ORR:   /*logical */
        ARMDisassembler_append(dis, "orr");
        ARMDisassembler_writeConditionCode(dis, instruction );
        if( instruction & BitS )
            ARMDisassembler_append(dis, "s");
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, r%d, ",Rd,Rn);
        break;
    case MOV:   /*logical */
        ARMDisassembler_append(dis, "mov");
        ARMDisassembler_writeConditionCode(dis, instruction );
        if( instruction & BitS )
            ARMDisassembler_append(dis, "s");
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, ",Rd);
        break;
    case BIC:   /*logical */
        ARMDisassembler_append(dis, "bic");
        ARMDisassembler_writeConditionCode(dis, instruction );
        if( instruction & BitS )
            ARMDisassembler_append(dis, "s");
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, r%d, ",Rd,Rn);
        break;
    case MVN:   /*logical */
        ARMDisassembler_append(dis, "mvn");
        ARMDisassembler_writeConditionCode(dis, instruction );
        if( instruction & BitS )
            ARMDisassembler_append(dis, "s");
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, ",Rd);
        break;
    default:
        HG_ASSERT(0);   /*just in case */
        break;
    }
    ARMDisassembler_decodeOperand2(dis, instruction);
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_executePSRTransfer(ARMDisassembler* dis, unsigned int instruction)
{
    const unsigned int MRSMask    = 0x0fbf0fff; /*0000 1111 1011 1111 0000 1111 1111 1111 */
    const unsigned int MRSOpcode  = 0x010f0000; /*0000 0001 0000 1111 0000 0000 0000 0000 */
    const unsigned int MSRMask    = 0x0fbffff0; /*0000 1111 1011 1111 1111 1111 1111 0000 */
    const unsigned int MSROpcode  = 0x0129f000; /*0000 0001 0010 1001 1111 0000 0000 0000 */
    const unsigned int MSRFMask   = 0x0dbff000; /*0000 1101 1011 1111 1111 0000 0000 0000 */
    const unsigned int MSRFOpcode = 0x0128f000; /*0000 0001 0010 1000 1111 0000 0000 0000 */

    const unsigned int BitP = 0x00400000;   /*0=CPSR, 1=SPSR */
    if( instruction & BitP )
        ARMDisassembler_appendNote(dis, "warning: in the user mode the destination must always be CPSR");

    if( (instruction & MRSMask) == MRSOpcode )
    {   /*MRS (transfer PSR contents to a register) */
        const unsigned int DestinationMask = 0xf000;
        const unsigned int DestinationShift = 12;

        int Rd = (instruction & DestinationMask) >> DestinationShift;
        HG_ASSERT(Rd >= 0 && Rd < 16);
        if( Rd == 15)
            ARMDisassembler_appendNote(dis, "ERROR: R15 (PC) cannot be used for this instruction");
        ARMDisassembler_append(dis, "mrs");
        ARMDisassembler_writeConditionCode(dis, instruction );
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, CPSR",Rd);
    }
    else if( (instruction & MSRMask) == MSROpcode )
    {   /*MSR (transfer register contents to PSR) */
        const unsigned int SourceMask = 0xf;

        int Rm = instruction & SourceMask;
        HG_ASSERT(Rm >= 0 && Rm < 16);
        if( Rm == 15)
            ARMDisassembler_appendNote(dis, "ERROR: R15 (PC) cannot be used for this instruction");
        /*in the user mode only the condition codes can be changed */
        ARMDisassembler_append(dis, "msr");
        ARMDisassembler_writeConditionCode(dis, instruction );
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "CPSR_flg, r%d",Rm);
    }
    else if( (instruction & MSRFMask) == MSRFOpcode )
    {   /*MSR (transfer register contents or immediate value to PSR flag bits only) */
        const unsigned int BitI = 0x02000000;   /*0=source is a register, 1=source is an immediate */
        ARMDisassembler_append(dis, "msr");
        ARMDisassembler_writeConditionCode(dis, instruction );
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "CPSR_flg, ");

        /*fetch operand2 */
        if( instruction & BitI )
            /*TODO is this correct? the meaning of the shift bits is not specified in the document */
            ARMDisassembler_decodeOperand2Immediate(dis, instruction);
        else
        {
            const unsigned int SourceMask = 0xf;
            int Rm = instruction & SourceMask;
            HG_ASSERT(Rm >= 0 && Rm < 16);
            if( Rm == 15)
                ARMDisassembler_appendNote(dis, "ERROR: R15 (PC) cannot be used for this instruction");
            ARMDisassembler_append(dis, "r%d",Rm);
        }
    }
    else
    {
        ARMDisassembler_appendNote(dis, "ERROR: undefined PSR transfer instruction");
    }

}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_executeMultiply(ARMDisassembler* dis, unsigned int instruction)
{
    const unsigned int BitA = 0x00200000;   /*0=MUL, 1=MLA */
    const unsigned int BitS = 0x00100000;   /*0=do not write condition codes, 1=write condition codes */

    const unsigned int MultiplyMask = 0x0fc000f0;   /*0000 1111 1100 0000 0000 0000 1111 0000 */
    const unsigned int MultiplyOpcode = 0x00000090; /*0000 0000 0000 0000 0000 0000 1001 0000 */

    const unsigned int DestinationMask = 0xf0000;
    const unsigned int DestinationShift = 16;
    int Rd = (instruction & DestinationMask) >> DestinationShift;

    const unsigned int AddRegisterMask = 0xf000;
    const unsigned int AddRegisterShift = 12;
    int Rn = (instruction & AddRegisterMask) >> AddRegisterShift;

    const unsigned int Operand2Mask = 0xf;
    int Rm = instruction & Operand2Mask;

    const unsigned int Operand1Mask = 0xf00;
    const unsigned int Operand1Shift = 8;
    int Rs = (instruction & Operand1Mask) >> Operand1Shift;

    if( (instruction & MultiplyMask) != MultiplyOpcode )
        ARMDisassembler_appendNote(dis, "ERROR: undefined multiply instruction (ARM7)");

    HG_ASSERT(Rd >= 0 && Rd < 16);  /*just in case */
    HG_ASSERT(Rn >= 0 && Rn < 16);  /*just in case */
    HG_ASSERT(Rm >= 0 && Rm < 16);  /*just in case */
    HG_ASSERT(Rs >= 0 && Rs < 16);  /*just in case */

    /*Due to the way multiplication was implemented in other ARM processors, certain combinations
      of operand registers should be avoided. The ARM7’s advanced multiplier can handle all operand
      combinations but by observing these restrictions code written for the ARM7 will be more
      compatible with other ARM processors.
      The destination register Rd shall not be the same as the operand register Rm.
      R15 shall not be used as an operand or as the destination register. */
    if( Rm == Rd )
        ARMDisassembler_appendNote(dis, "ERROR: source register Rm cannot be the same as the destination");
    if( Rd == 15 || Rn == 15 || Rm == 15 || Rs == 15 )
        ARMDisassembler_appendNote(dis, "ERROR: R15 cannot be used");

    if( instruction & BitA )
    {   /*MLA (multiply accumulate) */
        ARMDisassembler_append(dis, "mla");
        ARMDisassembler_writeConditionCode(dis, instruction );
        if( instruction & BitS )
            ARMDisassembler_append(dis, "s");
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, r%d, r%d, r%d",Rd,Rm,Rs,Rn);
    }
    else
    {
        ARMDisassembler_append(dis, "mul");
        ARMDisassembler_writeConditionCode(dis, instruction );
        if( instruction & BitS )
            ARMDisassembler_append(dis, "s");
        ARMDisassembler_appendTab(dis);
        ARMDisassembler_append(dis, "r%d, r%d, r%d",Rd,Rm,Rs);

        if( Rn != 0 )
            ARMDisassembler_appendNote(dis, "ERROR: for MUL Rn should be zero for upwards compatibility (v4)");
    }
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_executeMultiplyLong(ARMDisassembler* dis, unsigned int instruction)
{
    const unsigned int BitU = 0x00400000;   /*0=unsigned, 1=signed */
    const unsigned int BitA = 0x00200000;   /*0=MUL, 1=MLA */
    const unsigned int BitS = 0x00100000;   /*0=do not write condition codes, 1=write condition codes */

    const unsigned int MultiplyMask = 0x0f8000f0;   /*0000 1111 1000 0000 0000 0000 1111 0000 */
    const unsigned int MultiplyOpcode = 0x00800090; /*0000 0000 8000 0000 0000 0000 1001 0000 */

    const unsigned int DestinationHiMask = 0xf0000;
    const unsigned int DestinationHiShift = 16;
    int RdHi = (instruction & DestinationHiMask) >> DestinationHiShift;

    const unsigned int DestinationLoMask = 0xf000;
    const unsigned int DestinationLoShift = 12;
    int RdLo = (instruction & DestinationLoMask) >> DestinationLoShift;

    const unsigned int Operand2Mask = 0xf;
    int Rm = instruction & Operand2Mask;

    const unsigned int Operand1Mask = 0xf00;
    const unsigned int Operand1Shift = 8;
    int Rs = (instruction & Operand1Mask) >> Operand1Shift;

    if( (instruction & MultiplyMask) != MultiplyOpcode )
        ARMDisassembler_appendNote(dis, "ERROR: undefined multiply instruction (ARM8)");

    HG_ASSERT(RdHi >= 0 && RdHi < 16);  /*just in case */
    HG_ASSERT(RdLo >= 0 && RdLo < 16);  /*just in case */
    HG_ASSERT(Rm >= 0 && Rm < 16);  /*just in case */
    HG_ASSERT(Rs >= 0 && Rs < 16);  /*just in case */


    if( Rm == RdLo || Rm == RdHi )
        ARMDisassembler_appendNote(dis, "ERROR: source register Rm cannot be the same as the destination");
    if( RdHi == 15 || RdLo == 15 || Rm == 15 || Rs == 15 )
        ARMDisassembler_appendNote(dis, "ERROR: R15 cannot be used");

    if( instruction & BitU )
        ARMDisassembler_append(dis, "s");
    else
        ARMDisassembler_append(dis, "u");

    if( instruction & BitA )
        ARMDisassembler_append(dis, "mlal");
    else
        ARMDisassembler_append(dis, "mull");

    ARMDisassembler_writeConditionCode(dis, instruction );
    if( instruction & BitS )
        ARMDisassembler_append(dis, "s");
    ARMDisassembler_appendTab(dis);
    ARMDisassembler_append(dis, "r%d, r%d, r%d, r%d",RdLo,RdHi,Rm,Rs);
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_executeSingleDataTransfer(ARMDisassembler* dis, unsigned int instruction)
{
    const unsigned int BitI = 0x02000000;   /*0=immediate offset, 1=register offset */
    const unsigned int BitP = 0x01000000;   /*0=postindexing, 1=preindexing */
    const unsigned int BitU = 0x00800000;   /*0=subtract offset, 1=add offset */
    const unsigned int BitB = 0x00400000;   /*0=word, 1=byte */
    const unsigned int BitW = 0x00200000;   /*0=no write-back, 1=write-back address */
    const unsigned int BitL = 0x00100000;   /*0=store, 1=load */
    const unsigned int BaseMask = 0xf0000;
    const unsigned int BaseShift = 16;
    const unsigned int SrcDestMask = 0xf000;
    const unsigned int SrcDestShift = 12;

    /*decode the registers */
    int Rn = (instruction & BaseMask) >> BaseShift;
    int Rd = (instruction & SrcDestMask) >> SrcDestShift;

    if( instruction & BitL )
        ARMDisassembler_append(dis, "ldr");
    else
        ARMDisassembler_append(dis, "str");
    ARMDisassembler_writeConditionCode(dis, instruction );
    if( instruction & BitB )
        ARMDisassembler_append(dis, "b");   /*byte transfer */

    if( !(instruction & BitP) && (instruction & BitW) )
    {
        ARMDisassembler_append(dis, "t");
        ARMDisassembler_appendNote(dis, "warning: write-back specified with postindexing (forces nonprivileged mode transfer)");
    }

    HG_ASSERT(Rn >= 0 && Rn < 16);
    HG_ASSERT(Rd >= 0 && Rd < 16);

    ARMDisassembler_appendTab(dis);
    ARMDisassembler_append(dis, "r%d, [r%d",Rd,Rn);

    /*When using R15 as the base register you must remember it contains an address 8 bytes on
      from the address of the current instruction.
      When R15 is the source register (Rd) of a register store (STR) instruction, the stored value
      will be address of the instruction plus 12 (ARM7). */

    /*pre/postindexing */
    if( instruction & BitP )
    {   /*preincrement */

        if( instruction & BitI )
        {   /*register offset */
            const unsigned int ShiftIsRegisterMask = 0x10;
            const unsigned int RegisterMask = 0xf;

            if( instruction & ShiftIsRegisterMask )
                ARMDisassembler_appendNote(dis, "ERROR: shift cannot be specified as a register");
            if( (instruction & RegisterMask) == 15 )
                ARMDisassembler_appendNote(dis, "ERROR: R15 cannot be used as the offset register");

            ARMDisassembler_append(dis, ", ");
            if( !(instruction & BitU) )
                ARMDisassembler_append(dis, "-");
            ARMDisassembler_decodeOperand2Register(dis, instruction);
            ARMDisassembler_append(dis, "]");
            if( instruction & BitW )
                ARMDisassembler_append(dis, "!");   /*write-back */
        }
        else
        {   /*unsigned 12-bit immediate offset */
            const unsigned int OffsetMask = 0xfff;
            int offset = instruction & OffsetMask;
            if( !(instruction & BitU) )
                offset = -offset;
            if( Rn == 15 )
                offset += dis->m_prefetch;  /*take into account instruction prefetch */
            if( offset )
            {
                if( dis->m_hex )
                    ARMDisassembler_append(dis, ", #0x%08x]",offset);
                else
                    ARMDisassembler_append(dis, ", #%d]",offset);

                if( instruction & BitW )
                {   /*write back the modified base */
                    ARMDisassembler_append(dis, "!");
                    if( Rn == 15 )
                        ARMDisassembler_appendNote(dis, "ERROR: write-back to R15 is not allowed");
                }
            }
            else
                ARMDisassembler_append(dis, "]");
        }
    }
    else
    {   /*postincrement */
        ARMDisassembler_append(dis, "], ");

        if( instruction & BitI )
        {   /*register offset */
            const unsigned int ShiftIsRegisterMask = 0x10;
            const unsigned int RegisterMask = 0xf;

            if( instruction & ShiftIsRegisterMask )
                ARMDisassembler_appendNote(dis, "ERROR: shift cannot be specified as a register");
            if( (instruction & RegisterMask) == 15 )
                ARMDisassembler_appendNote(dis, "ERROR: R15 cannot be used as the offset register");

            if( !(instruction & BitU) )
                ARMDisassembler_append(dis, "-");
            ARMDisassembler_decodeOperand2Register(dis, instruction);
        }
        else
        {   /*unsigned 12-bit immediate offset */
            const unsigned int OffsetMask = 0xfff;
            int offset = instruction & OffsetMask;
            if( !(instruction & BitU) )
                offset = -offset;
            if( Rn == 15 )
                offset += dis->m_prefetch;  /*take into account instruction prefetch */
            if( offset )
            {
                if( dis->m_hex )
                    ARMDisassembler_append(dis, "#0x%08x",offset);
                else
                    ARMDisassembler_append(dis, "#%d",offset);
            }
        }
    }
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_executeHalfwordAndSignedDataTransfer(ARMDisassembler* dis, unsigned int instruction)
{
    const unsigned int BitP = 0x01000000;   /*0=postindexing, 1=preindexing */
    const unsigned int BitU = 0x00800000;   /*0=subtract offset, 1=add offset */
    const unsigned int BitI = 0x00400000;   /*0=register offset, 1=immediate offset */
    const unsigned int BitW = 0x00200000;   /*0=no write-back, 1=write-back address */
    const unsigned int BitL = 0x00100000;   /*0=store, 1=load */
    const unsigned int BitS = 0x00000040;   /*0=unsigned, 1=signed */
    const unsigned int BitH = 0x00000020;   /*0=byte, 1=halfword */
    const unsigned int BaseMask = 0xf0000;
    const unsigned int BaseShift = 16;
    const unsigned int SrcDestMask = 0xf000;
    const unsigned int SrcDestShift = 12;

    /*decode the registers */
    int Rn = (instruction & BaseMask) >> BaseShift;
    int Rd = (instruction & SrcDestMask) >> SrcDestShift;

    if(!((instruction & BitS) || (instruction & BitH)))
        ARMDisassembler_appendNote(dis, "ERROR: SWP instruction specified instead of halfword and signed data transfer");

    if( instruction & BitL )
        ARMDisassembler_append(dis, "ldr");
    else
        ARMDisassembler_append(dis, "str");
    ARMDisassembler_writeConditionCode(dis, instruction );
    if( instruction & BitS )
        ARMDisassembler_append(dis, "s");   /*signed */
    if( instruction & BitH )
    {
        if( !(instruction & BitL) && (instruction & BitS) )
            ARMDisassembler_appendNote(dis, "ERROR: signed halfword store does not exist");
        ARMDisassembler_append(dis, "h");   /*halfword transfer */
    }
    else
    {
        if( !(instruction & BitS) )
            ARMDisassembler_appendNote(dis, "ERROR: unsigned byte store should not use halfword and signed transfer");
        if( !(instruction & BitL) && (instruction & BitS) )
            ARMDisassembler_appendNote(dis, "ERROR: signed byte store does not exist");
        ARMDisassembler_append(dis, "b");   /*byte transfer */
    }

    if( !(instruction & BitP) && (instruction & BitW) )
        ARMDisassembler_appendNote(dis, "ERROR: write-back specified with postindexing in halfword and signed transfer");

    HG_ASSERT(Rn >= 0 && Rn < 16);
    HG_ASSERT(Rd >= 0 && Rd < 16);

    ARMDisassembler_appendTab(dis);
    ARMDisassembler_append(dis, "r%d, [r%d",Rd,Rn);

    /*When using R15 as the base register you must remember it contains an address 8 bytes on
      from the address of the current instruction.
      When R15 is the source register (Rd) of a register store (STR) instruction, the stored value
      will be address of the instruction plus 12. */

    /*pre/postindexing */
    if( instruction & BitP )
    {   /*preincrement */

        if( !(instruction & BitI) )
        {   /*register offset */
            const unsigned int RegisterMask = 0xf;
            if( (instruction & RegisterMask) == 15 )
                ARMDisassembler_appendNote(dis, "ERROR: R15 cannot be used as the offset register");

            ARMDisassembler_append(dis, ", ");
            if( !(instruction & BitU) )
                ARMDisassembler_append(dis, "-");
            ARMDisassembler_append(dis, "r%d]",(instruction & RegisterMask));
            if( instruction & BitW )
                ARMDisassembler_append(dis, "!");   /*write-back */
        }
        else
        {   /*unsigned 8-bit immediate offset */
            const unsigned int OffsetLowMask = 0xf;
            const unsigned int OffsetHighMask = 0xf00;
            const unsigned int OffsetHighShift = 4;
            int offset = instruction & OffsetLowMask;
            offset |= (instruction & OffsetHighMask) >> OffsetHighShift;
            if( !(instruction & BitU) )
                offset = -offset;
            if( Rn == 15 )
                offset += dis->m_prefetch;  /*take into account instruction prefetch */
            if( offset )
            {
                if( dis->m_hex )
                    ARMDisassembler_append(dis, ", #0x%08x]",offset);
                else
                    ARMDisassembler_append(dis, ", #%d]",offset);

                if( instruction & BitW )
                {   /*write back the modified base */
                    ARMDisassembler_append(dis, "!");
                    if( Rn == 15 )
                        ARMDisassembler_appendNote(dis, "ERROR: write-back to R15 is not allowed");
                }
            }
            else
                ARMDisassembler_append(dis, "]");
        }
    }
    else
    {   /*postincrement */
        ARMDisassembler_append(dis, "], ");

        if( !(instruction & BitI) )
        {   /*register offset */
            const unsigned int RegisterMask = 0xf;
            if( (instruction & RegisterMask) == 15 )
                ARMDisassembler_appendNote(dis, "ERROR: R15 cannot be used as the offset register");

            if( !(instruction & BitU) )
                ARMDisassembler_append(dis, "-");
            ARMDisassembler_append(dis, "r%d",(instruction & RegisterMask));
            if( instruction & BitW )
                ARMDisassembler_append(dis, "!");   /*write-back */
        }
        else
        {   /*unsigned 8-bit immediate offset */
            const unsigned int OffsetLowMask = 0xf;
            const unsigned int OffsetHighMask = 0xf00;
            const unsigned int OffsetHighShift = 4;
            int offset = instruction & OffsetLowMask;
            offset |= (instruction & OffsetHighMask) >> OffsetHighShift;
            if( !(instruction & BitU) )
                offset = -offset;
            if( Rn == 15 )
                offset += dis->m_prefetch;  /*take into account instruction prefetch */
            if( offset )
            {
                if( dis->m_hex )
                    ARMDisassembler_append(dis, "#0x%08x",offset);
                else
                    ARMDisassembler_append(dis, "#%d",offset);
            }
        }
    }
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_executeBlockDataTransfer(ARMDisassembler* dis, unsigned int instruction)
{
    const unsigned int BitP = 0x01000000;   /*0=postindexing, 1=preindexing */
    const unsigned int BitU = 0x00800000;   /*0=subtract offset, 1=add offset */
    const unsigned int BitS = 0x00400000;   /*0=do not load PSR or force user mode, 1=load PSR or force user mode */
    const unsigned int BitW = 0x00200000;   /*0=no write-back, 1=write-back */
    const unsigned int BitL = 0x00100000;   /*0=store, 1=load */
    const unsigned int BaseMask = 0xf0000;
    const unsigned int BaseShift = 16;
    const unsigned int BlockDataTransferMask = 0x0e000000;      /*0000 1110 0000 0000 0000 0000 0000 0000 */
    const unsigned int BlockDataTransferOpcode = 0x08000000;    /*0000 1000 0000 0000 0000 0000 0000 0000 */
    int Rn = (instruction & BaseMask) >> BaseShift;
    int r,i;

    if((instruction & BlockDataTransferMask) != BlockDataTransferOpcode)
        ARMDisassembler_appendNote(dis, "ERROR: undefined block data transfer instruction (ARM7)");
    if(instruction & BitS)
        ARMDisassembler_appendNote(dis, "warning: S-bit can only be used in a privileged mode");

    HG_ASSERT(Rn >= 0 && Rn < 16);
    if( Rn == 15 )
        ARMDisassembler_appendNote(dis, "ERROR: R15 cannot be used as the base register");

    if( instruction & BitL )
        ARMDisassembler_append(dis, "ldm");
    else
        ARMDisassembler_append(dis, "stm");

    ARMDisassembler_writeConditionCode(dis, instruction );
    
    if( instruction & BitU )
        ARMDisassembler_append(dis, "i");   /*add offset */
    else
        ARMDisassembler_append(dis, "d");   /*subtract offset */

    if( instruction & BitP )
        ARMDisassembler_append(dis, "b");   /*preindexing */
    else
        ARMDisassembler_append(dis, "a");   /*postindexing */

    ARMDisassembler_appendTab(dis);
    ARMDisassembler_append(dis, "r%d",Rn);
    if( instruction & BitW )
        ARMDisassembler_append(dis, "!");
    ARMDisassembler_append(dis, ", {");

    r = 0;
    i = 0;
    for(i=0;i<16;i++)
    {
        if( instruction & (1<<i) )
        {   /*transfer register */
            if( r )
                ARMDisassembler_append(dis, ", ");
            ARMDisassembler_append(dis, "r%d",i);
            r++;
        }
    }
    if( r == 0 )
        ARMDisassembler_appendNote(dis, "ERROR: no registers to load/store");
    ARMDisassembler_append(dis, "}");
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_executeSingleDataSwap(ARMDisassembler* dis, unsigned int instruction)
{
    const unsigned int BitB = 0x00400000;   /*0=word, 1=byte */
    const unsigned int SourceMask = 0xf;
    const unsigned int DestinationMask = 0xf000;
    const unsigned int DestinationShift = 12;
    const unsigned int BaseMask = 0xf0000;
    const unsigned int BaseShift = 16;
    const unsigned int SwapMask = 0x0fb00ff0;   /*0000 1111 1011 0000 0000 1111 1111 0000 */
    const unsigned int SwapOpcode = 0x01000090; /*0000 0001 0000 0000 0000 0000 1001 0000 */

    /*NOTE: Rm and Rd can be the same register */
    /*TODO can Rn be the same as Rm and Rd? */
    int Rm = instruction & SourceMask;
    int Rd = (instruction & DestinationMask) >> DestinationShift;
    int Rn = (instruction & BaseMask) >> BaseShift;

    if( (instruction & SwapMask) != SwapOpcode )
        ARMDisassembler_appendNote(dis, "ERROR: undefined single data swap instruction (ARM7)");

    HG_ASSERT(Rm >= 0 && Rm < 16);
    HG_ASSERT(Rd >= 0 && Rd < 16);
    HG_ASSERT(Rn >= 0 && Rn < 16);
    if(Rm == 15 || Rd == 15 || Rn == 15)
        ARMDisassembler_appendNote(dis, "ERROR: R15 cannot be used as an operand for swap");

    ARMDisassembler_append(dis, "swp");
    ARMDisassembler_writeConditionCode(dis, instruction );
    if( instruction & BitB )
        ARMDisassembler_append(dis, "b");
    ARMDisassembler_appendTab(dis);
    ARMDisassembler_append(dis, "r%d, r%d, [r%d]",Rd,Rm,Rn);
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_executeClz(ARMDisassembler* dis, unsigned int instruction)
{
    const unsigned int SourceMask = 0xf;
    const unsigned int DestinationMask = 0xf000;
    const unsigned int DestinationShift = 12;

    /*NOTE: Rm and Rd can be the same register */
    int Rm = instruction & SourceMask;
    int Rd = (instruction & DestinationMask) >> DestinationShift;

    HG_ASSERT(Rm >= 0 && Rm < 16);
    HG_ASSERT(Rd >= 0 && Rd < 16);
    if(Rm == 15 || Rd == 15)
        ARMDisassembler_appendNote(dis, "ERROR: R15 cannot be used as an operand for swap");

    ARMDisassembler_append(dis, "clz");
    ARMDisassembler_writeConditionCode(dis, instruction );
    ARMDisassembler_appendTab(dis);
    ARMDisassembler_append(dis, "r%d, r%d",Rd,Rm);
}
