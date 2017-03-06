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
#include "hgint32.h"

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

HGbool ARMEmulator_executeBranch(ARMEmulator* emu, unsigned int instruction )
{
    const unsigned int BitL = 0x01000000;   /*0=B (branch), 1=BL (branch with link) */
    const unsigned int OffsetMask = 0x00ffffff;
    int offset;
    if( instruction & BitL )
    {   /*branch with link */
        /*store the pointer to the instruction following the branch */
        emu->m_registerFile[14] = emu->m_registerFile[15] + 4;
    }
    offset = signExtend((instruction & OffsetMask)<<2, 23); /*offset is 23 bits + sign bit */
    emu->m_registerFile[15] += offset + emu->m_prefetch;    /*offsets have been adjusted to take the prefetch into account */
    HG_ASSERT( !(emu->m_registerFile[15] & 3) );    /*check for proper alignment */
    return HG_TRUE; /*always writes R15 */
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

HGbool ARMEmulator_executeDataProcessing(ARMEmulator* emu, unsigned int instruction )
{
    ARM_DEBUG_CODE( const unsigned int BitS = 0x00100000; ) /*0=do not write condition codes, 1=write condition codes */
    const unsigned int OpcodeMask = 0x01e00000;
    const unsigned int OpcodeShift = 21;
    const unsigned int FirstOperandMask = 0x000f0000;
    const unsigned int FirstOperandShift = 16;
    const unsigned int DestinationMask = 0x0000f000;
    const unsigned int DestinationShift = 12;
    unsigned int operand1, operand2, barrelShifterCarry;

    HGbool wroteR15 = HG_FALSE;
    HGbool C,V;
    unsigned int result;
    unsigned int opcode = (instruction & OpcodeMask) >> OpcodeShift;

    int Rd = (instruction & DestinationMask) >> DestinationShift;
    int Rn = (instruction & FirstOperandMask) >> FirstOperandShift;
    HG_ASSERT(Rd >= 0 && Rd < 16);  /*just in case */
    HG_ASSERT(Rn >= 0 && Rn < 16);  /*just in case */


    /*TODO ARM8: Data processing instructions with a register-specified shift and at least one of */
    /* Rm and Rn equal to R15 are no longer valid */

    /*If R15 (the PC) is used as an operand in a data processing instruction the register is used
      directly. The PC value will be the address of the instruction, plus 8 or 12 bytes due to
      instruction prefetching. If the shift amount is specified in the instruction, the PC will
      be 8 bytes ahead. If a register is used to specify the shift amount the PC will be 12 bytes
      ahead. */
    operand1 = emu->m_registerFile[Rn];
    if( Rn == 15 )
        operand1 += emu->m_prefetch;    /*take instruction prefetch into account */
    ARMEmulator_fetchOperand2(emu, instruction, &operand2, &barrelShifterCarry);

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
    HG_ASSERT(!(Rd == 15 && (instruction & BitS))); /*we are always in user mode, so we don't need this */

    switch( opcode )
    {
    case AND:   /*logical */
        emu->m_registerFile[Rd] = operand1 & operand2;
        ARMEmulator_setConditionCodesLogical(emu, instruction, emu->m_registerFile[Rd], barrelShifterCarry );
        if( Rd == 15 )
            wroteR15 = HG_TRUE;
        break;
    case EOR:   /*logical */
        emu->m_registerFile[Rd] = operand1 ^ operand2;
        ARMEmulator_setConditionCodesLogical(emu, instruction, emu->m_registerFile[Rd], barrelShifterCarry );
        if( Rd == 15 )
            wroteR15 = HG_TRUE;
        break;
    case SUB:   /*arithmetic */
        /*ARM sub is implemented using the adder by the following way: */
        /*result = ARMEmulator_adder( op1, ~op2, 1 )    where 1 is passed as the carry in */
        /*this comes from the fact that neg(a) = not(a) + 1 */
        emu->m_registerFile[Rd] = ARMEmulator_adder( operand1, ~operand2, 1, &V, &C );
        ARMEmulator_setConditionCodesArithmetic(emu, instruction, emu->m_registerFile[Rd], V, C );
        if( Rd == 15 )
            wroteR15 = HG_TRUE;
        break;
    case RSB:   /*arithmetic */
        emu->m_registerFile[Rd] = ARMEmulator_adder( operand2, ~operand1, 1, &V, &C );
        ARMEmulator_setConditionCodesArithmetic(emu, instruction, emu->m_registerFile[Rd], V, C );
        if( Rd == 15 )
            wroteR15 = HG_TRUE;
        break;
    case ADD:   /*arithmetic */
        emu->m_registerFile[Rd] = ARMEmulator_adder( operand1, operand2, 0, &V, &C );
        ARMEmulator_setConditionCodesArithmetic(emu, instruction, emu->m_registerFile[Rd], V, C );
        if( Rd == 15 )
            wroteR15 = HG_TRUE;
        break;
    case ADC:   /*arithmetic */
        emu->m_registerFile[Rd] = ARMEmulator_adder( operand1, operand2, (emu->m_CPSR & FlagC) ? 1 : 0, &V, &C );
        ARMEmulator_setConditionCodesArithmetic(emu, instruction, emu->m_registerFile[Rd], V, C );
        if( Rd == 15 )
            wroteR15 = HG_TRUE;
        break;
    case SBC:   /*arithmetic */
        emu->m_registerFile[Rd] = ARMEmulator_adder( operand1, ~operand2, (emu->m_CPSR & FlagC) ? 1 : 0, &V, &C );
        ARMEmulator_setConditionCodesArithmetic(emu, instruction, emu->m_registerFile[Rd], V, C );
        if( Rd == 15 )
            wroteR15 = HG_TRUE;
        break;
    case RSC:   /*arithmetic */
        emu->m_registerFile[Rd] = ARMEmulator_adder( operand2, ~operand1, (emu->m_CPSR & FlagC) ? 1 : 0, &V, &C );
        ARMEmulator_setConditionCodesArithmetic(emu, instruction, emu->m_registerFile[Rd], V, C );
        if( Rd == 15 )
            wroteR15 = HG_TRUE;
        break;
    case TST:   /*logical */
        HG_ASSERT(instruction & BitS);  /*S-bit must always be set for this instruction */
        ARMEmulator_setConditionCodesLogical(emu, instruction, operand1 & operand2, barrelShifterCarry );
        break;
    case TEQ:   /*logical */
        HG_ASSERT(instruction & BitS);  /*S-bit must always be set for this instruction */
        ARMEmulator_setConditionCodesLogical(emu, instruction, operand1 ^ operand2, barrelShifterCarry );
        break;
    case CMP:   /*arithmetic */
        HG_ASSERT(instruction & BitS);  /*S-bit must always be set for this instruction */
        result = ARMEmulator_adder( operand1, ~operand2, 1, &V, &C );
        ARMEmulator_setConditionCodesArithmetic(emu, instruction, result, V, C );
        break;
    case CMN:   /*arithmetic */
        HG_ASSERT(instruction & BitS);  /*S-bit must always be set for this instruction */
        result = ARMEmulator_adder( operand1, operand2, 0, &V, &C );
        ARMEmulator_setConditionCodesArithmetic(emu, instruction, result, V, C );
        break;
    case ORR:   /*logical */
        emu->m_registerFile[Rd] = operand1 | operand2;
        ARMEmulator_setConditionCodesLogical(emu, instruction, emu->m_registerFile[Rd], barrelShifterCarry );
        if( Rd == 15 )
            wroteR15 = HG_TRUE;
        break;
    case MOV:   /*logical */
        emu->m_registerFile[Rd] = operand2;
        ARMEmulator_setConditionCodesLogical(emu, instruction, emu->m_registerFile[Rd], barrelShifterCarry );
        if( Rd == 15 )
            wroteR15 = HG_TRUE;
        break;
    case BIC:   /*logical */
        emu->m_registerFile[Rd] = operand1 & ~operand2;
        ARMEmulator_setConditionCodesLogical(emu, instruction, emu->m_registerFile[Rd], barrelShifterCarry );
        if( Rd == 15 )
            wroteR15 = HG_TRUE;
        break;
    case MVN:   /*logical */
        emu->m_registerFile[Rd] = ~operand2;
        ARMEmulator_setConditionCodesLogical(emu, instruction, emu->m_registerFile[Rd], barrelShifterCarry );
        if( Rd == 15 )
            wroteR15 = HG_TRUE;
        break;
    default:
        HG_ASSERT(HG_FALSE);    /*just in case */
        break;
    }

    return wroteR15;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

HGbool ARMEmulator_executePSRTransfer(ARMEmulator* emu, unsigned int instruction )
{
    const unsigned int MRSMask    = 0x0fbf0fff; /*0000 1111 1011 1111 0000 1111 1111 1111 */
    const unsigned int MRSOpcode  = 0x010f0000; /*0000 0001 0000 1111 0000 0000 0000 0000 */
    const unsigned int MSRMask    = 0x0fbffff0; /*0000 1111 1011 1111 1111 1111 1111 0000 */
    const unsigned int MSROpcode  = 0x0129f000; /*0000 0001 0010 1001 1111 0000 0000 0000 */
    const unsigned int MSRFMask   = 0x0dbff000; /*0000 1101 1011 1111 1111 0000 0000 0000 */
    const unsigned int MSRFOpcode = 0x0128f000; /*0000 0001 0010 1000 1111 0000 0000 0000 */

    ARM_DEBUG_CODE( const unsigned int BitP = 0x00400000; ) /*0=CPSR, 1=SPSR */
    HG_ASSERT(!(instruction & BitP));   /*in the user mode the destination must always be CPSR */

    if( (instruction & MRSMask) == MRSOpcode )
    {   /*MRS (transfer PSR contents to a register) */
        const unsigned int DestinationMask = 0xf000;
        const unsigned int DestinationShift = 12;

        int Rd = (instruction & DestinationMask) >> DestinationShift;
        HG_ASSERT(Rd >= 0 && Rd < 16);
        HG_ASSERT(Rd != 15);    /*R15 (PC) cannot be used for this instruction */
        emu->m_registerFile[Rd] = emu->m_CPSR;
    }
    else if( (instruction & MSRMask) == MSROpcode )
    {   /*MSR (transfer register contents to PSR) */
        const unsigned int SourceMask = 0xf;

        int Rm = instruction & SourceMask;
        HG_ASSERT(Rm >= 0 && Rm < 16);
        HG_ASSERT(Rm != 15);    /*R15 (PC) cannot be used for this instruction */
        /*in the user mode only the condition codes can be changed */
        emu->m_CPSR = (emu->m_CPSR & ~(ConditionCodeMask<<ConditionCodeShift)) | (emu->m_registerFile[Rm] & (unsigned int)(ConditionCodeMask<<ConditionCodeShift));
    }
    else if( (instruction & MSRFMask) == MSRFOpcode )
    {   /*MSR (transfer register contents or immediate value to PSR flag bits only) */
        const unsigned int BitI = 0x02000000;   /*0=source is a register, 1=source is an immediate */
        unsigned int dummy, operand;
        /*fetch operand2 */
        if( instruction & BitI )
            /*TODO is this correct? the meaning of the shift bits is not specified in the document */
            ARMEmulator_fetchOperand2Immediate(emu, instruction, &operand, &dummy);
        else
        {
            const unsigned int SourceMask = 0xf;
            int Rm = instruction & SourceMask;
            HG_ASSERT(Rm >= 0 && Rm < 16);
            HG_ASSERT(Rm != 15);    /*R15 (PC) cannot be used for this instruction */
            operand = emu->m_registerFile[Rm];  /*only the four most significant bits are used */
        }
        /*change only the condition codes */
        emu->m_CPSR = (emu->m_CPSR & ~(ConditionCodeMask<<ConditionCodeShift)) | (operand & (unsigned int)(ConditionCodeMask<<ConditionCodeShift));
    }
    else
    {
        HG_ASSERT(HG_FALSE);    /*undefined instruction */
    }
    return HG_FALSE;    /*cannot change R15 */
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

HGbool ARMEmulator_executeMultiply(ARMEmulator* emu, unsigned int instruction )
{
    const unsigned int BitA = 0x00200000;   /*0=MUL, 1=MLA */
    const unsigned int BitS = 0x00100000;   /*0=do not write condition codes, 1=write condition codes */

    const unsigned int DestinationMask = 0xf0000;
    const unsigned int DestinationShift = 16;
    int Rd = (instruction & DestinationMask) >> DestinationShift;

    const unsigned int AddRegisterMask = 0xf000;
    const unsigned int AddRegisterShift = 12;
    int Rn = (instruction & AddRegisterMask) >> AddRegisterShift, valRn;

    const unsigned int Operand2Mask = 0xf;
    int Rm = instruction & Operand2Mask, valRm;

    const unsigned int Operand1Mask = 0xf00;
    const unsigned int Operand1Shift = 8;
    int Rs = (instruction & Operand1Mask) >> Operand1Shift, valRs;

    ARM_DEBUG_CODE(
        const unsigned int MultiplyMask = 0x0fc000f0;   /*0000 1111 1100 0000 0000 0000 1111 0000 */
        const unsigned int MultiplyOpcode = 0x00000090; /*0000 0000 0000 0000 0000 0000 1001 0000 */
    )
    HG_ASSERT( (instruction & MultiplyMask) == MultiplyOpcode );    /*undefined instruction (ARM7) */

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
    /*TODO do we want to assert these? */
    /*reads from program counter are not emulated (would need to take prefetch into account); */
    HG_ASSERT(Rm != Rd);
    HG_ASSERT(Rd != 15);
    HG_ASSERT(Rn != 15);
    HG_ASSERT(Rm != 15);
    HG_ASSERT(Rs != 15);

    valRm = emu->m_registerFile[Rm];
    valRs = emu->m_registerFile[Rs];
    valRn = emu->m_registerFile[Rn];
    emu->m_registerFile[Rd] = valRm * valRs;
    if( instruction & BitA )
    {   /*MLA (multiply accumulate) */
        emu->m_registerFile[Rd] += valRn;
    }
    else
    {
        HG_ASSERT(Rn == 0); /*for plain multiply Rn should be zero for upwards compatibility (v4) */
    }

    /*Setting the CPSR flags is optional, and is controlled by the S bit in the instruction.
      The N (Negative) and Z (Zero) flags are set correctly on the result (N is made equal to
      bit 31 of the result, and Z is set if and only if the result is zero).
      The C (Carry) flag is set to a meaningless value and the V (oVerflow) flag is unaffected.
      NOTE: this is the behaviour of v4 and earlier. v5 is different */
    if( instruction & BitS )
    {   /*set condition codes */
        /*preserve V */
        /*preserve C (TODO is this what is meant by the meaningless value?) */
        if( emu->m_registerFile[Rd] )
            emu->m_CPSR &= ~FlagZ;  /*not zero, clear Z */
        else
            emu->m_CPSR |= FlagZ;   /*zero, set Z */
        if( emu->m_registerFile[Rd] & 0x80000000 )
            emu->m_CPSR |= FlagN;   /*negative, set N */
        else
            emu->m_CPSR &= ~FlagN;  /*not negative, clearN */
    }

    return HG_FALSE;    /*cannot change R15 */
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

/*TODO verify this */
HG_STATICF void muls64(int* rhi, unsigned int* rlo, int a, int b )
{
    unsigned int hh     = (unsigned int)((a>>16)   *(b>>16));
    unsigned int lh     = (unsigned int)((a&0xFFFF)*(b>>16));
    unsigned int hl     = (unsigned int)((a>>16)   *(b&0xFFFF));
    unsigned int ll     = (unsigned int)((a&0xFFFF)*(b&0xFFFF));
    unsigned int oldlo;

    hh += (int)(lh)>>16;
    hh += (int)(hl)>>16;

    oldlo   =  ll;
    ll      += lh<<16;
    if (ll < oldlo)
        hh++;

    oldlo   =  ll;
    ll      += hl<<16;
    if (ll < oldlo)
        hh++;

    *rhi = (int)hh;
    *rlo = ll;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

/*TODO verify this */
HG_STATICF void mulu64( unsigned int* rhi, unsigned int* rlo, unsigned int a, unsigned int b )
{
    unsigned int hh = (a>>16)   * (b>>16);
    unsigned int lh = (a&0xFFFF)* (b>>16);
    unsigned int hl = (a>>16)   * (b&0xFFFFu);
    unsigned int ll = (a&0xFFFF)* (b&0xFFFFu);
    unsigned int oldlo;

    hh += (lh>>16);
    hh += (hl>>16);

    oldlo = ll;
    ll += lh<<16;
    if (ll < oldlo)
        hh++;

    oldlo = ll;
    ll += hl<<16;
    if (ll < oldlo)
        hh++;

    *rhi = hh;
    *rlo = ll;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

/*TODO verify this, or just use HGint64 */
HG_STATICF void adds64( int* rhi, unsigned int* rlo, int ahi, unsigned int alo, int bhi, unsigned int blo )
{
    *rhi = ahi + bhi;
    *rlo = alo + blo;
    *rhi += ((unsigned int)(*rlo) < (unsigned int)(alo)) ? 1 : 0;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

/*TODO verify this, or just use HGint64 */
HG_STATICF void addu64( unsigned int* rhi, unsigned int* rlo, unsigned int ahi, unsigned int alo, unsigned int bhi, unsigned int blo )
{
    *rhi = ahi + bhi;
    *rlo = alo + blo;
    *rhi += ((unsigned int)(*rlo) < (unsigned int)(alo)) ? 1 : 0;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

HGbool ARMEmulator_executeMultiplyLong(ARMEmulator* emu, unsigned int instruction )
{
    const unsigned int BitU = 0x00400000;   /*0=unsigned, 1=signed */
    const unsigned int BitA = 0x00200000;   /*0=MUL, 1=MLA */
    const unsigned int BitS = 0x00100000;   /*0=do not write condition codes, 1=write condition codes */

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

    ARM_DEBUG_CODE(
        const unsigned int MultiplyMask = 0x0f8000f0;   /*0000 1111 1000 0000 0000 0000 1111 0000 */
        const unsigned int MultiplyOpcode = 0x00800090; /*0000 0000 8000 0000 0000 0000 1001 0000 */
    )
    HG_ASSERT( (instruction & MultiplyMask) == MultiplyOpcode );    /*undefined instruction (ARM7) */

    HG_ASSERT(RdHi >= 0 && RdHi < 16);  /*just in case */
    HG_ASSERT(RdLo >= 0 && RdLo < 16);  /*just in case */
    HG_ASSERT(Rm >= 0 && Rm < 16);  /*just in case */
    HG_ASSERT(Rs >= 0 && Rs < 16);  /*just in case */
    HG_ASSERT(Rm != RdHi);
    HG_ASSERT(Rm != RdLo);
    HG_ASSERT(RdLo != RdHi);
    HG_ASSERT(RdHi != 15);
    HG_ASSERT(RdLo != 15);
    HG_ASSERT(Rm != 15);
    HG_ASSERT(Rs != 15);

    if( instruction & BitU )
    {   /*signed */
        int rhi;
        unsigned int rlo;
        muls64( &rhi, &rlo, emu->m_registerFile[Rm], emu->m_registerFile[Rs] );
        if( instruction & BitA )
        {   /*MLA (multiply accumulate) */
            adds64( &rhi, &rlo, rhi, rlo, emu->m_registerFile[RdHi], emu->m_registerFile[RdLo] );
        }
        emu->m_registerFile[RdHi] = (unsigned int)rhi;
        emu->m_registerFile[RdLo] = (unsigned int)rlo;
    }
    else
    {   /*unsigned */
        unsigned int rhi,rlo;
        mulu64( &rhi, &rlo, emu->m_registerFile[Rm], emu->m_registerFile[Rs] );
        if( instruction & BitA )
        {   /*MLA (multiply accumulate) */
            addu64( &rhi, &rlo, rhi, rlo, emu->m_registerFile[RdHi], emu->m_registerFile[RdLo] );
        }
        emu->m_registerFile[RdHi] = (unsigned int)rhi;
        emu->m_registerFile[RdLo] = (unsigned int)rlo;
    }

    /*Setting the CPSR flags is optional, and is controlled by the S bit in the instruction.
      The N (Negative) and Z (Zero) flags are set correctly on the result (N is made equal to
      bit 31 of the result, and Z is set if and only if the result is zero).
      The C (Carry) flag is set to a meaningless value and the V (oVerflow) flag is unaffected.
      NOTE: this is the behaviour of v4 and earlier. v5 is different */
    if( instruction & BitS )
    {   /*set condition codes */
        /*preserve V */
        /*preserve C (TODO is this what is meant by the meaningless value?) */
        if( emu->m_registerFile[RdHi] || emu->m_registerFile[RdLo] )
            emu->m_CPSR &= ~FlagZ;  /*not zero, clear Z */
        else
            emu->m_CPSR |= FlagZ;   /*zero, set Z */
        if( emu->m_registerFile[RdHi] & 0x80000000 )
            emu->m_CPSR |= FlagN;   /*negative, set N */
        else
            emu->m_CPSR &= ~FlagN;  /*not negative, clearN */
    }

    return HG_FALSE;    /*cannot change R15 */
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

HGbool ARMEmulator_executeSingleDataTransfer(ARMEmulator* emu, unsigned int instruction )
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
    int Rn, Rd;
    unsigned int baseBefore, baseAfter, address;
    HGbool wroteR15 = HG_FALSE;

    /*fetch offset */
    unsigned int offset;
    if( instruction & BitI )
    {   /*register offset */
        unsigned int dummy;
        ARM_DEBUG_CODE(
            const unsigned int ShiftIsRegisterMask = 0x10;
            const unsigned int RegisterMask = 0xf;
        )
        HG_ASSERT( !(instruction & ShiftIsRegisterMask) );  /*shift cannot be specified as a register */
        HG_ASSERT( (instruction & RegisterMask) != 15 );    /*R15 cannot be used as the offset register */
        ARMEmulator_fetchOperand2Register(emu, instruction, &offset, &dummy);
    }
    else
    {   /*unsigned 12-bit immediate offset */
        const unsigned int OffsetMask = 0xfff;
        offset = instruction & OffsetMask;
    }

    /*decode the registers */
    Rn = (instruction & BaseMask) >> BaseShift;
    Rd = (instruction & SrcDestMask) >> SrcDestShift;
    HG_ASSERT(Rn >= 0 && Rn < 16);
    HG_ASSERT(Rd >= 0 && Rd < 16);
    /*TODO Do not specify post-indexed loads and stores where Rm and Rn are the same register, */
    /*TODO as they can be impossible to unwind after an abort. */
    /*TODO Do not set register positions Rd and Rn to be the same register when a load instruction */
    /*TODO specifies (or implies) base write-back. */

    /*TODO Do not specify R15 as: */
    /*TODO - the register offset (Rm) */
    /*TODO - the destination register (Rd) of a load instruction (LDRH, LDRSH, LDRSB) */
    /*TODO - the source register (Rd) of a store instruction (STRH, STRSH, STRSB) */

    /*TODO Do not specify either write-back or post-indexing (which forces write-back) if R15 is */
    /*TODO specified as the base register (Rn). When using R15 as the base register you must */
    /*TODO remember that it contains an address 8 bytes on from the address of the current */
    /*TODO instruction. */


    /*form the address */
    baseBefore = emu->m_registerFile[Rn];
    baseAfter = baseBefore;
    /*add/subtract offset */
    if( instruction & BitU )
        baseAfter += offset;
    else
        baseAfter -= offset;

    /*When using R15 as the base register you must remember it contains an address 8 bytes on
      from the address of the current instruction.
      When R15 is the source register (Rd) of a register store (STR) instruction, the stored value
      will be address of the instruction plus 12 (ARM7). */

    /*pre/postindexing */
    if( instruction & BitP )
    {   /*preincrement */
        address = baseAfter;
        /*in a real ARM R15 is 8 bytes ahead and the offset has been adjusted to take that into account.
          we need to emulate it here by adding the prefetch to the address */
        if( Rn == 15 )
            address += emu->m_prefetch;

        if( instruction & BitW )
        {   /*write back the modified base */
            HG_ASSERT(Rn != 15);    /*no write-back to R15 */
            emu->m_registerFile[Rn] = baseAfter;
        }
    }
    else
    {   /*postincrement */
        address = baseBefore;
        HG_ASSERT(!(instruction & BitW));   /*check that write-back is clear with postindexing */
        HG_ASSERT(Rn != 15);    /*no write-back to R15 */
        emu->m_registerFile[Rn] = baseAfter;    /*always write back the modified base */
    }

    if( instruction & BitB )
        if( instruction & BitL )
        {
            emu->m_registerFile[Rd] = ARMEmulator_loadByte(emu, address);
            if( Rd == 15 )
                wroteR15 = HG_TRUE;
        }
        else
        {
            if( Rd == 15 )
                ARMEmulator_storeByte(emu, emu->m_registerFile[Rd]+emu->m_storePrefetch, address);  /*take instruction prefetch into account */
            else
                ARMEmulator_storeByte(emu, emu->m_registerFile[Rd], address);
        }
    else
        if( instruction & BitL )
        {
            emu->m_registerFile[Rd] = ARMEmulator_loadWord(emu, address);
            if( Rd == 15 )
                wroteR15 = HG_TRUE;
        }
        else
        {
            if( Rd == 15 )
                ARMEmulator_storeWord(emu, emu->m_registerFile[Rd] + emu->m_storePrefetch, address);    /*take instruction prefetch into account */
            else
                ARMEmulator_storeWord(emu, emu->m_registerFile[Rd], address);
        }

    return wroteR15;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

HGbool ARMEmulator_executeHalfwordAndSignedDataTransfer(ARMEmulator* emu, unsigned int instruction )
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
    unsigned int offset;
    int Rn, Rd;
    unsigned int baseBefore, baseAfter, address;
    HGbool wroteR15 = HG_FALSE;

    HG_ASSERT((instruction & BitS) || (instruction & BitH));    /*if both S and H are 0, the instruction is SWP */

    if( instruction & BitI )
    {   /*immediate offset */
        const unsigned int OffsetLowMask = 0xf;
        const unsigned int OffsetHighMask = 0xf00;
        const unsigned int OffsetHighShift = 4;
        offset = instruction & OffsetLowMask;
        offset |= (instruction & OffsetHighMask) >> OffsetHighShift;
    }
    else
    {   /*register offset */
        const unsigned int OffsetMask = 0xf;
        int Rm = instruction & OffsetMask;
        HG_ASSERT( Rm >= 0 && Rm < 16 );
        HG_ASSERT( Rm != 15 );      /*R15 is not allowed as the offset register */
        offset = emu->m_registerFile[Rm];
    }

    /*decode the registers */
    Rn = (instruction & BaseMask) >> BaseShift;
    Rd = (instruction & SrcDestMask) >> SrcDestShift;
    HG_ASSERT(Rn >= 0 && Rn < 16);
    HG_ASSERT(Rd >= 0 && Rd < 16);

    /*form the address */
    baseBefore = emu->m_registerFile[Rn];
    baseAfter = baseBefore;
    /*add/subtract offset */
    if( instruction & BitU )
        baseAfter += offset;
    else
        baseAfter -= offset;

    /*When using R15 as the base register you must remember it contains an address 8 bytes on
      from the address of the current instruction.
      When R15 is the source register (Rd) of a register store (STR) instruction, the stored value
      will be address of the instruction plus 12. */

    /*pre/postindexing */
    if( instruction & BitP )
    {   /*preincrement */
        address = baseAfter;
        /*in a real ARM R15 is 8 bytes ahead and the offset has been adjusted to take that into account.
          we need to emulate it here by adding the prefetch to the address */
        if( Rn == 15 )
            address += emu->m_prefetch;

        if( instruction & BitW )
        {   /*write back the modified base */
            HG_ASSERT(Rn != 15);    /*no write-back to R15 */
            emu->m_registerFile[Rn] = baseAfter;
        }
    }
    else
    {   /*postincrement */
        address = baseBefore;
        HG_ASSERT(!(instruction & BitW));   /*check that write-back is clear with postindexing */
        HG_ASSERT(Rn != 15);    /*no write-back to R15 */
        emu->m_registerFile[Rn] = baseAfter;    /*always write back the modified base */
    }

    if( instruction & BitH )
    {   /*halfword transfer */
        if( instruction & BitL )
        {   /*load */
            emu->m_registerFile[Rd] = ARMEmulator_loadHalfword(emu, address);
            if( instruction & BitS )
                emu->m_registerFile[Rd] = (unsigned int)signExtend((int)emu->m_registerFile[Rd], 15);
            if( Rd == 15 )
                wroteR15 = HG_TRUE;
        }
        else
        {   /*store */
            HG_ASSERT( !(instruction & BitS) ); /*no signed store */
            if( Rd == 15 )
                ARMEmulator_storeHalfword(emu, emu->m_registerFile[Rd] + emu->m_storePrefetch, address);    /*take instruction prefetch into account */
            else
                ARMEmulator_storeHalfword(emu, emu->m_registerFile[Rd], address);
        }
    }
    else
    {   /*signed byte load */
        HG_ASSERT( instruction & BitS );    /*always signed (otherwise the instruction is SWP) */
        HG_ASSERT( instruction & BitL );    /*always load (there is no signed store, normal byte store should be used instead) */
        emu->m_registerFile[Rd] = ARMEmulator_loadByte(emu, address);
        emu->m_registerFile[Rd] = (unsigned int)signExtend((int)emu->m_registerFile[Rd], 7);
        if( Rd == 15 )
            wroteR15 = HG_TRUE;
    }
    return wroteR15;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

HGbool ARMEmulator_executeBlockDataTransfer(ARMEmulator* emu, unsigned int instruction )
{
    const unsigned int BitP = 0x01000000;   /*0=postindexing, 1=preindexing */
    const unsigned int BitU = 0x00800000;   /*0=subtract offset, 1=add offset */
    ARM_DEBUG_CODE( const unsigned int BitS = 0x00400000; ) /*0=do not load PSR or force user mode, 1=load PSR or force user mode */
    const unsigned int BitW = 0x00200000;   /*0=no write-back, 1=write-back */
    const unsigned int BitL = 0x00100000;   /*0=store, 1=load */
    const unsigned int BaseMask = 0xf0000;
    const unsigned int BaseShift = 16;
    int Rn, r, radd, i;
    unsigned int address;
    HGbool wroteR15 = HG_FALSE;

    ARM_DEBUG_CODE( const unsigned int BlockDataTransferMask = 0x0e000000; )    /*0000 1110 0000 0000 0000 0000 0000 0000 */
    ARM_DEBUG_CODE( const unsigned int BlockDataTransferOpcode = 0x08000000; )  /*0000 1000 0000 0000 0000 0000 0000 0000 */
    HG_ASSERT((instruction & BlockDataTransferMask) == BlockDataTransferOpcode);
    HG_ASSERT(!(instruction & BitS));   /*S-bit can only be used in a privileged mode */

    Rn = (instruction & BaseMask) >> BaseShift;
    HG_ASSERT(Rn >= 0 && Rn < 16);
    HG_ASSERT(Rn != 15);    /*R15 cannot be used as the base register */

    address = emu->m_registerFile[Rn];
    if( instruction & BitU )
    {   /*add offset */
        r = 0;
        radd = 1;
    }
    else
    {   /*subtract offset */
        r = 15;
        radd = -1;
    }
    /*NOTE: a real ARM transfers the registers in increasing order(R0,R1,R2,...)
       we do not model it here to simplify the loops (as the lowest numbered register will always
       be in the lowest address). */
    for(i=0;i<16;i++,r += radd)
    {
        if( instruction & (1<<r) )
        {   /*transfer register */
            if( instruction & BitP )
            {   /*preincrement */
                address += radd<<2;
            }

            if( instruction & BitL )
            {
                emu->m_registerFile[r] = ARMEmulator_loadWord(emu, address);
                if( r == 15 )
                    wroteR15 = HG_TRUE;
            }
            else
            {
                /*TODO STM instructions with R15 in the list of registers to be stored store the address
                  of the instruction plus 8 (ARM8, ARM7 differs from this) */
                HG_ASSERT( r != 15 );   /*not implemented */
                ARMEmulator_storeWord(emu, emu->m_registerFile[r], address);
            }

            if( !(instruction & BitP) )
            {   /*postincrement */
                address += radd<<2;
            }
        }
    }
    if( instruction & BitW )
    {   /*write back the base register
          When write-back is specified, the base is written back at the end of the second cycle of
          the instruction. During a STM, the first register is written out at the start of the second
          cycle. A STM which includes storing the base, with the base as the first register to be
          stored, will therefore store the unchanged value, whereas with the base second or later in
          the transfer order, will store the modified value. A LDM will always overwrite the updated
          base if the base is in the list. */
        if( instruction & BitL )
        {   /*LDM */
            if( !(instruction & (1<<Rn)) )
                emu->m_registerFile[Rn] = address;  /*the base is not in the list, do write-back */
        }
        else
        {   /*STM */
            HG_ASSERT(!(instruction & (1<<Rn))); /* not implemented properly */
            emu->m_registerFile[Rn] = address;  /*do write-back. TODO do this as in the spec */
        }
    }
    return wroteR15;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

HGbool ARMEmulator_executeSingleDataSwap(ARMEmulator* emu, unsigned int instruction )
{
    const unsigned int BitB = 0x00400000;   /*0=word, 1=byte */
    const unsigned int SourceMask = 0xf;
    const unsigned int DestinationMask = 0xf000;
    const unsigned int DestinationShift = 12;
    const unsigned int BaseMask = 0xf0000;
    const unsigned int BaseShift = 16;
    int Rm, Rd, Rn;
    unsigned int address;

    ARM_DEBUG_CODE( const unsigned int SwapMask = 0x0fb00ff0; )     /*0000 1111 1011 0000 0000 1111 1111 0000 */
    ARM_DEBUG_CODE( const unsigned int SwapOpcode = 0x01000090; )   /*0000 0001 0000 0000 0000 0000 1001 0000 */
    HG_ASSERT( (instruction & SwapMask) == SwapOpcode );

    /*NOTE: Rm and Rd can be the same register */
    /*TODO can Rn be the same as Rm and Rd? */
    Rm = instruction & SourceMask;
    Rd = (instruction & DestinationMask) >> DestinationShift;
    Rn = (instruction & BaseMask) >> BaseShift;
    HG_ASSERT(Rm >= 0 && Rm < 16);
    HG_ASSERT(Rd >= 0 && Rd < 16);
    HG_ASSERT(Rn >= 0 && Rn < 16);
    HG_ASSERT(Rm != 15);    /*R15 cannot be used as an operand for swap */
    HG_ASSERT(Rd != 15);    /*R15 cannot be used as an operand for swap */
    HG_ASSERT(Rn != 15);    /*R15 cannot be used as an operand for swap */

    address = emu->m_registerFile[Rn];
    if( instruction & BitB )
    {
        unsigned int tmp = ARMEmulator_loadByte(emu, address);
        ARMEmulator_storeByte(emu, emu->m_registerFile[Rm], address);
        emu->m_registerFile[Rd] = tmp;
    }
    else
    {
        unsigned int tmp = ARMEmulator_loadWord(emu, address);
        ARMEmulator_storeWord(emu, emu->m_registerFile[Rm], address);
        emu->m_registerFile[Rd] = tmp;
    }
    return HG_FALSE;    /*cannot change R15 */
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

HGbool ARMEmulator_executeClz(ARMEmulator* emu, unsigned int instruction )
{
    const unsigned int SourceMask = 0xf;
    const unsigned int DestinationMask = 0xf000;
    const unsigned int DestinationShift = 12;
    int Rm, Rd;

    ARM_DEBUG_CODE( const unsigned int ClzMask = 0x0ff000f0; )          /*0000 1111 1111 0000 0000 0000 1111 0000 */
    ARM_DEBUG_CODE( const unsigned int ClzOpcode = 0x01600010; )        /*0000 0001 0110 0000 0000 0000 0001 0000 */
    HG_ASSERT( (instruction & ClzMask) == ClzOpcode );

    /*NOTE: Rm and Rd can be the same register */
    /*TODO can Rn be the same as Rm and Rd? */
    Rm = instruction & SourceMask;
    Rd = (instruction & DestinationMask) >> DestinationShift;
    HG_ASSERT(Rm >= 0 && Rm < 16);
    HG_ASSERT(Rd >= 0 && Rd < 16);
    HG_ASSERT(Rm != 15);    /*R15 cannot be used as an operand for swap */
    HG_ASSERT(Rd != 15);    /*R15 cannot be used as an operand for swap */

    emu->m_registerFile[Rd] = hgClz32(emu->m_registerFile[Rm]);
    return HG_FALSE;    /*cannot change R15 */
}
