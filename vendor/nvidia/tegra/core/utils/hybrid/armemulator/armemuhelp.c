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

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMEmulator_fetchOperand2Immediate(ARMEmulator* emu, unsigned int instruction, unsigned int* operand2, unsigned int* carry )
{
    const unsigned int ImmediateMask = 0xff;
    const unsigned int ShiftMask = 0xf00;
    const unsigned int ShiftShift = 8;

    unsigned int immediate = instruction & ImmediateMask;
    unsigned int shift = ((instruction & ShiftMask) >> ShiftShift);
    shift <<= 1;    /*shift by twice the amount specified */
    HG_ASSERT(shift <= 30);

    /*ROR #shift */
    *operand2 = (immediate >> shift) | (immediate << (32-shift));

    /*leave the carry flag unchanged */
    *carry = emu->m_CPSR & FlagC;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMEmulator_fetchOperand2Register(ARMEmulator* emu, unsigned int instruction, unsigned int* operand2, unsigned int* carry )
{
    const unsigned int OperandMask = 0xf;
    const unsigned int ShiftIsRegisterMask = 0x10;
    const unsigned int ShiftTypeMask = 0x60;
    const unsigned int ShiftTypeShift = 5;
    unsigned int shiftType;

    int Rm = instruction & OperandMask;
    HG_ASSERT(Rm >= 0 && Rm < 16);  /*just in case */
    *operand2 = emu->m_registerFile[Rm];
    if( Rm == 15 )
        *operand2 += emu->m_prefetch;   /*take instruction prefetch into account */

    /*shift operand2 */
    shiftType = (instruction & ShiftTypeMask) >> ShiftTypeShift;
    if( instruction & ShiftIsRegisterMask )
    {   /*shift is specified by a register */
        const unsigned int ShiftRegisterMask = 0xf00;
        const unsigned int ShiftRegisterShift = 8;
        unsigned int shift;
        int Rs = (instruction & ShiftRegisterMask) >> ShiftRegisterShift;
        HG_ASSERT(Rs >= 0 && Rs < 16);  /*just in case */
        HG_ASSERT(Rs != 15);    /*the shift register cannot be the PC */
        HG_ASSERT(!(instruction & 0x80));   /*the bit 7 should be clear */

        shift = emu->m_registerFile[Rs] & 0xff;
        if( !shift )
        {   /*the shift is zero, leave operand2 and carry unchanged */
            *carry = emu->m_CPSR & FlagC;
        }
        else
        {   /*apply shift */
            HG_ASSERT(shift > 0);
            switch(shiftType)
            {
            case LSL:
                if( shift == 32 )
                {
                    *carry = *operand2 & 0x1;
                    *operand2 = 0;
                }
                else if( shift > 32 )
                {
                    *carry = 0;
                    *operand2 = 0;
                }
                else
                {
                    *carry = (*operand2 >> (32-shift)) & 0x1;   /*carry is the least significant dropped bit */
                    *operand2 <<= shift;
                }
                break;
            case LSR:
                if( shift == 32 )
                {
                    *carry = *operand2 & 0x80000000;
                    *operand2 = 0;
                }
                else if( shift > 32 )
                {
                    *carry = 0;
                    *operand2 = 0;
                }
                else
                {
                    *carry = (*operand2 >> (shift-1)) & 0x1;    /*carry is the most significant dropped bit */
                    *operand2 >>= shift;
                }
                break;
            case ASR:
                if( shift > 31 )
                {   /*ASR #32 or higher gives the same result as ASR #31 */
                    shift = 31;
                    *carry = *operand2 & 0x80000000;
                }
                else
                {
                    *carry = (*((int*)operand2) >> (shift-1)) & 0x1;    /*carry is the most significant dropped bit */
                }
                *((int*)operand2) >>= shift;
                break;
            case ROR:
                /*make the shift amount to be in the range [1,32] */
                while( shift > 32 )
                    shift -= 32;
                HG_ASSERT(shift != 0);
                if( shift == 32 )
                {
                    *carry = *operand2 & 0x80000000;
                    /*leave operand2 unchanged */
                }
                else
                {
                    *carry = (*operand2 >> (shift-1)) & 0x1;    /*carry is the most significant dropped bit */
                    *operand2 = (*operand2 >> shift) | (*operand2 << (32-shift));
                }
                break;
            default:
                HG_ASSERT(HG_FALSE);
                break;
            }
        }
    }
    else
    {   /*shift is specified by an immediate */
        const unsigned int ShiftImmediateMask = 0xf80;
        const unsigned int ShiftImmediateShift = 7;
        unsigned int shift = (instruction & ShiftImmediateMask) >> ShiftImmediateShift;
        HG_ASSERT( shift < 32 );

        /*apply shift */
        switch(shiftType)
        {
        case LSL:
            if( !shift )
            {
                *carry = emu->m_CPSR & FlagC;   /*pass through the old value of the carry */
                /*leave operand2 unchanged */
            }
            else
            {
                *carry = (*operand2 >> (32-shift)) & 0x1;   /*carry is the least significant dropped bit */
                *operand2 <<= shift;
            }
            break;
        case LSR:
            if( !shift )
            {   /*LSR #0 is used to denote LSR #32 */
                *carry = *operand2 & 0x80000000;
                *operand2 = 0;
            }
            else
            {
                *carry = (*operand2 >> (shift-1)) & 0x1;    /*carry is the most significant dropped bit */
                *operand2 >>= shift;
            }
            break;
        case ASR:
            if( !shift )
            {   /*ASR #0 is used to denote ASR #32 */
                shift = 31; /*ASR #31 gives the same result as ASR #32 */
                *carry = *operand2 & 0x80000000;
            }
            else
            {
                *carry = (*((signed int*)operand2) >> (shift-1)) & 0x1; /*carry is the most significant dropped bit */
            }
            *((signed int*)operand2) >>= shift;
            break;
        case ROR:
            if( !shift )
            {   /*ROR #0 is used to denote RRX */
                *carry = *operand2 & 0x1;   /*new carry is the dropped bit */
                *operand2 = *operand2 >> 1; /*shift right by 1 */
                /*append old carry flag as the most significant bit */
                if( emu->m_CPSR & FlagC )
                    *operand2 |= 0x80000000;
            }
            else
            {
                *carry = (*operand2 >> (shift-1)) & 0x1;    /*carry is the most significant dropped bit */
                *operand2 = (*operand2 >> shift) | (*operand2 << (32-shift));
            }
            break;
        default:
            HG_ASSERT(HG_FALSE);
            break;
        }
    }
    /*fix carry to the correct bit position */
    if( *carry )
        *carry = FlagC;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMEmulator_fetchOperand2(ARMEmulator* emu, unsigned int instruction, unsigned int* operand2, unsigned int* carry )
{
    const unsigned int BitI = 0x02000000;   /*0=operand2 is a register, 1=operand2 is an immediate */
    /*fetch operand2 */
    if( instruction & BitI )
        ARMEmulator_fetchOperand2Immediate(emu, instruction, operand2, carry);
    else
        ARMEmulator_fetchOperand2Register(emu, instruction, operand2, carry);
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMEmulator_setConditionCodesLogical(ARMEmulator* emu, unsigned int instruction, unsigned int result, unsigned int carry )
{
    /*If the S bit is set (and Rd is not R15, see below) the V flag in the CPSR will
      be unaffected, the C flag will be set to the carry out from the barrel shifter
      (or preserved when the shift operation is LSL #0), the Z flag will be set if
      and only if the result is all zeros, and the N flag will be set to
      the logical value of bit 31 of the result. */

    const unsigned int BitS = 0x00100000;   /*0=do not write condition codes, 1=write condition codes */
    if( instruction & BitS )
    {   /*set condition codes */
        /*preserve V */
        emu->m_CPSR = (emu->m_CPSR & ~FlagC) | carry;   /*C = carry */

        if( !result )
            emu->m_CPSR |= FlagZ;   /*zero, set Z */
        else
            emu->m_CPSR &= ~FlagZ;  /*not zero, clear Z */

        if( result & 0x80000000 )
            emu->m_CPSR |= FlagN;   /*negative, set N */
        else
            emu->m_CPSR &= ~FlagN;  /*not negative, clearN */
    }
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMEmulator_setConditionCodesArithmetic(ARMEmulator* emu, unsigned int instruction, unsigned int result, HGbool V, HGbool C )
{
    /*If the S bit is set (and Rd is not R15) the V flag in the CPSR will be set if an overflow
      occurs into bit 31 of the result; this may be ignored if the operands were considered
      unsigned, but warns of a possible error if the operands were 2's complement signed.
      The C flag will be set to the carry out of bit 31 of the ALU, the Z flag will be set if and
      only if the result was zero, and the N flag will be set to the value of bit 31 of the result
      (indicating a negative result if the operands are considered to be 2's complement signed). */

    const unsigned int BitS = 0x00100000;   /*0=do not write condition codes, 1=write condition codes */
    if( instruction & BitS )
    {   /*set condition codes */
        if( V )
            emu->m_CPSR |= FlagV;
        else
            emu->m_CPSR &= ~FlagV;

        if( C )
            emu->m_CPSR |= FlagC;
        else
            emu->m_CPSR &= ~FlagC;

        if( !result )
            emu->m_CPSR |= FlagZ;   /*zero, set Z */
        else
            emu->m_CPSR &= ~FlagZ;  /*not zero, clear Z */

        if( result & 0x80000000 )
            emu->m_CPSR |= FlagN;   /*negative, set N */
        else
            emu->m_CPSR &= ~FlagN;  /*not negative, clear N */
    }
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

#ifdef _DEBUG
/*adder emulated bit by bit */
HG_STATICF unsigned int debugadder(unsigned int a, unsigned int b, unsigned int c, HGbool* V, HGbool* C)
{
    unsigned int r = 0, ci = 0, co = c;
    int i;
    HG_ASSERT(c == 0 || c == 1);
    for(i=0;i<32;i++,a>>=1,b>>=1)
    {
        unsigned int rr;
        ci = co;                /*(i-1)'th carry out becomes the i'th carry in */
        rr = (a & 1) + (b & 1) + ci;    /*add i'th bits of the input with i'th carry in bit */
        co = (rr >> 1) & 0x1;   /*the second bit of rr is the i'th carry out */
        r |= (rr & 0x1) << i;   /*the first bit of rr is the i'th bit of the result */
    }
    *C = co ? HG_TRUE : HG_FALSE;   /*co = carry out of the msb */
    /*overflow occurs when the sign of the result is wrong */
    /*this is equivalent to testing if the carry in to the msb != carry out of the msb */
    *V = (ci != co);
    return r;
}
#endif

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

HG_INLINE unsigned int add(unsigned int a, unsigned int b, HGbool* V, HGbool* C)
{
    unsigned int r = a + b, c0, ua, ub;
    /*overflow occurs when the sign of the result is wrong */
    *V =(!(a&0x80000000) && !(b&0x80000000) &&  (r&0x80000000)) ||
        ( (a&0x80000000) &&  (b&0x80000000) && !(r&0x80000000));
    c0 = a & b & 1; /*carry of the addition of least significant bits */
    ua = a >> 1;    /*make room for carry detection */
    ub = b >> 1;
    *C = ((ua + ub + c0) & 0x80000000) ? HG_TRUE : HG_FALSE;    /*carry = 33rd bit of the result */
    return r;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

unsigned int ARMEmulator_adder(unsigned int a, unsigned int b, unsigned int c, HGbool* V, HGbool* C)
{
    /*ARM sub is implemented using the adder by the following way:
      result = adder( op1, ~op2, 1 )    where 1 is passed as the carry in
      this comes from the fact that neg(a) = not(a) + 1 */

    unsigned int r = add(a,b,V,C);

    HG_ASSERT(c == 0 || c == 1);

    if( c )
    {   /*add carry in */
        HGbool V2,C2;
        r = add(r, c, &V2, &C2);
        *V = *V ^ V2;
        *C = *C ^ C2;
    }
#ifdef _DEBUG
    {
        HGbool Vc,Cc;
        unsigned int rc = debugadder(a, b, c, &Vc, &Cc);    /*check the result with bit-accurate adder */
        HG_ASSERT(rc == r);
        HG_UNREF(rc);
        HG_ASSERT(Vc == *V);
        HG_ASSERT(Cc == *C);
    }
#endif
    return r;
}
