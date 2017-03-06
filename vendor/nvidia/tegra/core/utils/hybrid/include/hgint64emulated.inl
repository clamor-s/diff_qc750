/*------------------------------------------------------------------------
 *
 * Hybrid Compatibility Header
 * -----------------------------------------
 *
 * (C) 2002-2004 Hybrid Graphics, Ltd.
 * All Rights Reserved.
 *
 * This file consists of unpublished, proprietary source code of
 * Hybrid Graphics, and is considered Confidential Information for
 * purposes of non-disclosure agreement. Disclosure outside the terms
 * outlined in signed agreement may result in irrepairable harm to
 * Hybrid Graphics and legal action against the party in breach.
 *
 * Description:         Inline code for hgInt64.h
 * Note:                        This file is included _only_ by hgInt64.h
 *
 * $Id: //hybrid/libs/hg/main/hgInt64Emulated.inl#3 $
 * $Date: 2006/07/13 $
 * $Author: janne $ *
 *
 * \note This file contains the "emulated" 64-bit code for platforms
 *               that have no (decent) support for 64-bit ints.
 *
 * \note In practice, this file is used when compiling code in Thumb
 *               mode or with the ADS compiler.
 *
 *----------------------------------------------------------------------*/

#if defined (HG_64_BIT_INTS)
#       error You should include hgInt64Native.inl instead!
#endif

/*@-shiftimplementation -shiftnegative -predboolint -boolops@*/

/*----------------------------------------------------------------------*
 * If we're using ADS, building a release build and not in Thumb mode,
 * let's define HG_ADS_ARM_ASSEMBLER -- this allows the use of certain
 * assembler optimizations.
 *----------------------------------------------------------------------*/

#if (HG_COMPILER == HG_COMPILER_ADS) && (HG_CPU == HG_CPU_ARM) && defined(HG_SUPPORT_ASM) && !defined (__thumb)
#       define HG_ADS_ARM_ASSEMBLER
#endif

/*-------------------------------------------------------------------*//*!
 * \brief       Casts an unsigned 64-bit integer into signed
 * \param       a       Unsigned 64-bit integer
 * \return      64-bit signed integer
 * \note        When properly inlined, this is a no-op
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgToSigned64 (const HGuint64 a)
{
        HGint64 r;
        r.h = (HGint32)(a.h);
        r.l = (HGint32)(a.l);
        return r;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Casts a signed 64-bit integer into unsigned
 * \param       a       Signed 64-bit integer
 * \return      64-bit unsigned integer
 * \note        When properly inlined, this is a no-op
 *//*-------------------------------------------------------------------*/

HG_INLINE HGuint64 hgToUnsigned64 (const HGint64 a)
{
        HGuint64 r;
        r.h = (HGuint32)(a.h);
        r.l = (HGuint32)(a.l);
        return r;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Constructs a 64-bit unsigned integer from two 32-bit values
 * \param       h       High 32 bits
 * \param       l       Low  32 bits
 * \return      Resulting 64-bit unsigned integer
 *//*-------------------------------------------------------------------*/

HG_INLINE HGuint64 hgSet64u (
        HGuint32 h,
        HGuint32 l)
{
        HGuint64 r;
        r.h = h;
        r.l = l;
        return r;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Constructs a 64-bit integer from two 32-bit value
 * \param       h       High 32 bits
 * \param       l       Low 32 bits
 * \return      Resulting 64-bit signed integer
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgSet64 (
        HGint32 h,
        HGint32 l)
{
        HGint64 r;
        r.h = h;
        r.l = l;
        return r;
}


/*-------------------------------------------------------------------*//*!
 * \brief       Sign-extends a 32-bit signed integer into a HGint64
 * \param       l       Signed 32-bit integer
 * \return      64-bit integer
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgSet64s (HGint32 l)
{
        HGint64 r;
        r.l = l;
        r.h = (l>>31);          /* sign-extend */
        return r;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns a 64-bit integer with the value 0
 * \return      64-bit integer with the value 0
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgZero64 (void)
{
        HGint64 r;
        r.h = 0;
        r.l = 0;
        return r;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Assigns 32-bit integer into high 32 bits of a 64-bit
 *                      integer and sets low part to zero
 * \param       a       32-bit input value
 * \return      64-bit integer
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgSet64x (HGint32 hi)
{
        HGint64 r;
        r.h = hi;
        r.l = 0;
        return r;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns high 32 bits of a 64-bit integer
 * \param       a       64-bit integer
 * \return      High 32 bits
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint32 hgGet64h (const HGint64 a)
{
        return a.h;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns high 32 bits of a 64-bit unsigned integer
 * \param       a       64-bit unsigned integer
 * \return      High 32 bits
 *//*-------------------------------------------------------------------*/

HG_INLINE HGuint32 hgGet64uh (const HGuint64 a)
{
        return a.h;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns low 32 bits of a 64-bit unsigned integer
 * \param       a       64-bit unsigned integer
 * \return      Low 32 bits
 *//*-------------------------------------------------------------------*/

HG_INLINE HGuint32 hgGet64ul (const HGuint64 a)
{
        return (HGuint32)a.l;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns low 32 bits of a 64-bit integer
 * \param       a       64-bit integer
 * \return      Low 32 bits (note that return value is signed!)
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint32 hgGet64l (const HGint64 a)
{
        return a.l;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns Nth bit of a 64-bit integer
 * \param       x       64-bit integer
 * \param       offset  Bit offset [0,63]
 * \return      offset'th bit [0,1]
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint32 hgGetBit64 (
        HGint64         x,
        int                     offset)
{
        HG_ASSERT (offset >= 0 && offset <= 63);
        return (offset >= 32) ? ((x.h >> (offset&31)) & 1) : ((x.l >> offset) & 1);
}

/*-------------------------------------------------------------------*//*!
 * \brief       Sets specified bit of a 64-bit integer
 * \param       x               64-bit integer
 * \param       offset  Bit offset
 * \return      64-bit integer
 * \note        The offset does not need to be in  [0,63] range (always
 *                      modulated internally)
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgSetBit64 (
        HGint64         x,
        int                     offset)
{
        HG_ASSERT (offset >= 0 && offset <= 63);
        {
                HGint32 ofsMask = 1<<(offset & 31);
                if (offset & 0x20)
                        x.h |= ofsMask;
                else
                        x.l |= ofsMask;
                return x;
        }
}

/*-------------------------------------------------------------------*//*!
 * \brief       Clears specified bit of a 64-bit integer
 * \param       x               64-bit integer
 * \param       offset  Bit offset [0,63]
 * \return      64-bit integer
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgClearBit64 (
        HGint64 x,
        int             offset)
{
        HGint32 ofsMask = 1<<(offset & 31);
        if (offset & 0x20)
                x.h &=~ofsMask;
        else
                x.l &=~ofsMask;
        return x;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Performs logical or between two 64-bit integer
 * \param       a       First 64-bit integer
 * \param       b       Second 64-bit integer
 * \return      64-bit integer
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgOr64 (
        const HGint64 a,
        const HGint64 b)
{
        return hgSet64(a.h | b.h, a.l | b.l);
}

/*-------------------------------------------------------------------*//*!
 * \brief       Performs logical eor between two 64-bit integer
 * \param       a       First 64-bit integer
 * \param       b       Second 64-bit integer
 * \return      64-bit integer
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgXor64 (
        const HGint64 a,
        const HGint64 b)
{
        return hgSet64(a.h ^ b.h, a.l ^ b.l);
}

/*-------------------------------------------------------------------*//*!
 * \brief       Performs logical and between two 64-bit integer
 * \param       a       First 64-bit integer
 * \param       b       Second 64-bit integer
 * \return      64-bit integer
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgAnd64 (
        const HGint64 a,
        const HGint64 b)
{
        return hgSet64(a.h & b.h, a.l & b.l);
}

/*-------------------------------------------------------------------*//*!
 * \brief       Performs 64-bit addition
 * \param       a       First 64-bit value
 * \param       b       Second 64-bit value
 * \return      64-bit integer
 * \todo        Optimize for VC02
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgAdd64 (
        const HGint64 a,
        const HGint64 b)
{
#if defined (HG_ADS_ARM_ASSEMBLER)
        HGint64 r;
        __asm {
                ADDS r.l, a.l, b.l
                ADC  r.h, a.h, b.h
        }
        return r;
#else
        HGint32 hi              = a.h+b.h;
        HGint32 lo              = a.l+b.l;

        hi += ((HGuint32)(lo) < (HGuint32)(a.l)) ? 1 : 0;
        return hgSet64(hi, lo);
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Increments 64-bit value by one
 * \param       a       64-bit value
 * \return      64-bit value
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgInc64 (
        HGint64 a)
{
#if defined (HG_ADS_ARM_ASSEMBLER)
        __asm {
                ADDS a.l, a.l, 1
                ADC  a.h, a.h, 0
        }
        return a;
#else
        a.l++;
        a.h += !a.l;
        return a;       
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Decrements 64-bit value by one
 * \param       a       64-bit value
 * \return      64-bit value
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgDec64 (
        HGint64 a)
{
#if defined (HG_ADS_ARM_ASSEMBLER)
        __asm {
                SUBS a.l, a.l, 1
                SBC  a.h, a.h, 0
        }
        return a;
#else
        a.h -= (a.l == 0);
        a.l--;
        return a;       
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       64-bit integer subtraction
 * \param       a       First 64-bit integer
 * \param       b       Second 64-bit integer
 * \return      64-bit integer
 * \todo        Optimize for VC02
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgSub64 (
        HGint64 a,
        HGint64 b)
{
#if defined (HG_ADS_ARM_ASSEMBLER)
        HGint64 r;
        __asm {
                SUBS r.l, a.l, b.l
                SBC  r.h, a.h, b.h
        }
        return r;
#else
        HGint32 sub = (HGuint32)(a.l) < (HGuint32)(b.l);
        a.h -= b.h;
        a.h -= sub;
        a.l -= b.l;
        return a;
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns boolean indicating whether a < b
 * \param       a       First 64-bit integer
 * \param       b       Second 64-bit integer
 * \return      a < b -> 1, else 0
 * \todo        [wili 29/Jan/03] Optimize for SH3!!
 * \todo        Optimize for VC02
 *//*-------------------------------------------------------------------*/

HG_INLINE HGbool hgLessThan64 (
        const HGint64 a,
        const HGint64 b)
{

#if defined (HG_ADS_ARM_ASSEMBLER)
        HGbool tmp;
        __asm {
                SUBS    tmp, a.l, b.l
                SBCS    tmp, a.h, b.h
                MOVLT   tmp,#1
                MOVGE   tmp,#0
        }
        return tmp;
#else
        HGint32 r = (HGint32)((a.h == b.h) & ((HGuint32)a.l < (HGuint32)b.l));
        r |= (a.h < b.h);
        return r;
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns boolean indicating whether a < b (unsigned)
 * \param       a       First 64-bit unsigned integer
 * \param       b       Second 64-bit unsigned integer
 * \return      a < b -> 1, else 0
 * \note        THIS IS A CRITICAL ROUTINE (as all other unsigned comparison
 *                      routines use this)
 * \todo        Optimize for VC02
 *//*-------------------------------------------------------------------*/

HG_INLINE HGbool hgLessThan64u (
        HGuint64 a,
        HGuint64 b)
{
        HGint32 r = (HGint32)((a.h == b.h) & (a.l < b.l));
        r |= (a.h < b.h);
        return r;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns boolean indicating whether a < b (unsigned)
 * \param       a       64-bit unsigned integer
 * \param       b       32-bit unsigned integer
 * \return      a < b -> 1, else 0
 * \todo        Optimize for VC02
 *//*-------------------------------------------------------------------*/

HG_INLINE HGbool hgLessThan64_32u (
        HGuint64 a,
        HGuint32 b)
{
        return (HGbool)((hgGet64h(hgToSigned64(a)) == 0) && (((HGuint32)hgGet64l(hgToSigned64(a))) < b));
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns boolean indicating whether a > b
 * \param       a       First 64-bit integer
 * \param       b       Second 64-bit integer
 * \return      a > b -> 1, else 0
 *//*-------------------------------------------------------------------*/

HG_INLINE HGbool hgGreaterThan64 (
        const HGint64 a,
        const HGint64 b)
{
        HGint32 r = (HGint32)((a.h == b.h) & ((HGuint32)a.l > (HGuint32)b.l));
        r |= (a.h > b.h);
        return r;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns boolean indicating whether a > b (unsigned)
 * \param       a       First 64-bit unsigned integer
 * \param       b       Second 64-bit unsigned integer
 * \return      a > b -> 1, else 0
 * \note        [wili 02/Oct/03] Fixed bug here!
 *//*-------------------------------------------------------------------*/

HG_INLINE HGbool hgGreaterThan64u (
        const HGuint64 a,
        const HGuint64 b)
{
        HGint32 r = (HGint32)((a.h == b.h) & (a.l > b.l));
        r |= (a.h > b.h);
        return r;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns boolean value indicating whether 64-bit value is
 *                      zero
 * \param       a       64-bit integer
 * \return      a = 0 -> 1 else 0
 *//*-------------------------------------------------------------------*/

HG_INLINE HGbool hgIsZero64 (const HGint64 a)
{
        return (HGbool)((hgGet64h(a) | hgGet64l(a)) == 0);
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns boolean value indicating whether 64-bit value is
 *                      less than zero (i.e. if the value is negative)
 * \param       a       64-bit integer
 * \return      a < 0 ? 1 : 0
 *//*-------------------------------------------------------------------*/

HG_INLINE HGbool hgLessThanZero64 (const HGint64 a)
{
        return (HGbool)((HGuint32)(hgGet64h(a)) >> 31);         /* sign bit set? */
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns boolean value indicating whether two 64-bit values
 *                      are equal
 * \param       a       First 64-bit integer
 * \param       b       Second 64-bit integer
 * \return      a = b -> 1 else 0
 *//*-------------------------------------------------------------------*/

HG_INLINE HGbool hgEq64 (
        const HGint64 a,
        const HGint64 b)
{
#if (HG_COMPILER == HG_COMPILER_ADS)
        return (a.h == b.h && a.l == b.l);
#else
        return (HGbool)(((hgGet64h(a)^hgGet64h(b))|(hgGet64l(a)^hgGet64l(b)))==0);
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns smaller of two 64-bit value (signed comparison)
 * \param       a       First 64-bit value
 * \param       b       Second 64-bit value
 * \return      Smaller of the two values
 * \todo        Must be optimized for VC02
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgMin64 (
        HGint64 a,
        HGint64 b)
{
#if defined (HG_ADS_ARM_ASSEMBLER)
        HGint32 tmp;
        __asm {
                SUBS    tmp, a.l, b.l
                SBCS    tmp, a.h, b.h
                MOVGE   a.h, b.h
                MOVGE   a.l, b.l
        }
        return a;
#elif (HG_COMPILER == HG_COMPILER_HCVC) && (HG_CPU == HG_CPU_VC02)
        if ((b.h < a.h) || ((b.h == a.h) && ((HGuint32)(b.l) < (HGuint32)(a.l))))
                a = b;
        return a;
#else
        return (hgLessThan64(a,b)) ? a : b;
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns larger of two 64-bit value (signed comparison)
 * \param       a       First 64-bit value
 * \param       b       Second 64-bit value
 * \return      Larger of the two values
 * \todo        Must be optimized for VC02
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgMax64 (
        HGint64 a,
        HGint64 b)
{
#if defined (HG_ADS_ARM_ASSEMBLER)
        HGint32   tmp;
        __asm {
                SUBS    tmp, a.l, b.l
                SBCS    tmp, a.h, b.h
                MOVLT   a.h, b.h
                MOVLT   a.l, b.l
        }
        return a;
#elif (HG_COMPILER == HG_COMPILER_HCVC) && (HG_CPU == HG_CPU_VC02)
        if ((b.h > a.h) || ((b.h == a.h) && ((HGuint32)(b.l) > (HGuint32)(a.l))))
                a = b;
        return a;
#else
        return (hgLessThan64(a,b)) ? b : a;
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns max(a,0)
 * \param       a       64-bit integer
 * \return      max(a,0)
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgMaxZero64 (HGint64 a)
{
#if (HG_COMPILER == HG_COMPILER_ADS)
        a.l = a.l &~(a.h>>31);                          /* single instruction in ARM */
        a.h = a.h &~(a.h>>31);
        return a;
#else
        HGint32 mask = ((HGint32)a.h >> 31);
        return hgSet64 (a.h &~mask, a.l &~mask);
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Rotate 64-bit value to the left
 * \param       a       64-bit integer
 * \param       sh      Rotation value (internally modulated by 64)
 * \return      64-bit integer
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgRol64 (
        const HGint64   a,
        HGint32                 sh)
{
        HGint32 hi,lo;

        hi = hgGet64h(a);
        lo = hgGet64l(a);

        if (sh & 0x20)                          /* sh = [32,63] */
        {
                hi = hgGet64l(a);
                lo = hgGet64h(a);
        }

        sh &= 31;


#if (HG_SHIFTER_MODULO <= 32)
        if (sh)
#endif
        {
                HGint32 tlo = (HGint32)((HGuint32)(hi) >> (32-sh));
                HGint32 thi = (HGint32)((HGuint32)(lo) >> (32-sh));
                hi = thi | (hi << sh);
                lo = tlo | (lo << sh);
        }

        return hgSet64(hi, lo);

}

/*-------------------------------------------------------------------*//*!
 * \brief       Rotate 64-bit value to the right
 * \param       a       64-bit integer
 * \param       sh      Rotation value (internally modulated by 64)
 * \return      64-bit integer
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgRor64 (
        const HGint64   a,
        HGint32                 sh)
{
        HGint32 hi,lo;

        if (sh & 0x20)                          /* sh = [32,63] */
        {
                hi = hgGet64l(a);
                lo = hgGet64h(a);
        } else
        {
                hi = hgGet64h(a);
                lo = hgGet64l(a);
        }

        sh &= 31;

#if (HG_SHIFTER_MODULO <= 32)
        if (sh)
#endif
        {
                HGint32 tlo = hi << (32-sh);
                HGint32 thi = lo << (32-sh);
                hi = thi | (HGint32)((HGuint32)(hi) >> sh);
                lo = tlo | (HGint32)((HGuint32)(lo) >> sh);
        }

        return hgSet64(hi, lo);

}

/*-------------------------------------------------------------------*//*!
 * \brief       Logical shift left for 64-bit integers
 * \param       a       64-bit integer
 * \param       sh      Shift value [0,63]
 * \return      64-bit integer
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgLsl64 (
        HGint64         a,
        HGint32         sh)
{
        HG_ASSERT(sh >= 0 && sh <= 63);

#if defined (HG_ADS_ARM_ASSEMBLER)
        {

/*      HGint64 r;
        r.h = (a.h << sh) | ((HGuint32)a.l >> (32-sh)) | (a.l << (sh-32));
        r.l = a.l << sh;
*/
                HGint64 r;
                HGint32 rsh;
                __asm
                {
                        RSB      rsh,sh,#0x20
                        MOV      rsh,a.l,LSR rsh
                        ORR      r.h,rsh,a.h,LSL sh
                        SUB      rsh,sh,#0x20
                        ORR      r.h,r.h,a.l,LSL rsh
                        MOV      r.l,a.l,LSL sh

                }
                return r;
        }
#else
  {
                HGuint32 t = (HGuint32)(a.l) >> (31 - sh);
                
                t   >>= 1;
                a.h <<= sh;
                a.h  |= t;
                a.l <<= (sh & 31);
                
                if (sh >= 32)
                {
                        a.h = a.l;
                        a.l = 0;
                }
                                
                return a;
        }
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Logical shift left for 64-bit integers, returns high 32 bits
 * \param       a       64-bit integer
 * \param       sh      Shift value [0,63]
 * \return      High 32 bits of a 64-bit integer
 **//*-------------------------------------------------------------------*/

HG_INLINE HGint32 hgLsl64h (
        const HGint64   a,
        HGint32                 sh)
{
        HG_ASSERT((int)sh >= 0 && sh <= 63);

#if defined (HG_ADS_ARM_ASSEMBLER)
        {
                HGint32 r;
                HGint32 rsh;
                __asm
                {
                        RSB      rsh,sh,#0x20
                        MOV      rsh,a.l,LSR rsh
                        ORR      r,rsh,a.h,LSL sh
                        SUB      rsh,sh,#0x20
                        ORR      r,r,a.l,LSL rsh
                }
                return r;
        }
#else
        if ((HGuint32)(sh-1) >= 31u)
                return sh ? (a.l<<(sh-32)) : a.h;
        else
                return ((a.h << sh) | ((HGuint32)(a.l) >> (32-sh)));
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Performs logical left shift of a 64-bit value, returns
 *                      high 32 bits
 * \param       a       64-bit integer value
 * \param       sh      Shift value [0,31]
 * \return      High 32 bits of resulting 64-bit value
 * \note        This function has a limited shift range !!!! [0,31]
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint32 hgLsl64h_0_31 (
        const HGint64   a,
        HGint32                 sh)
{
        HG_ASSERT (sh >= 0 && sh <= 31);

#if (HG_SHIFTER_MODULO <= 32)
        if (!sh)
                return a.h;
#endif

        return ((a.h << sh) | ((HGuint32)(a.l) >> (32-sh)));
}

/*-------------------------------------------------------------------*//*!
 * \brief       Logical shift right for 64-bit integers
 * \param       a       64-bit integer
 * \param       sh      Shift value [0,63]
 * \return      Resulting 64-bit integer
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgLsr64 (
        const HGint64   a,
        HGint32                 sh)
{
        HGuint32 hi,lo;

        HG_ASSERT(sh >= 0 && sh <= 63);

        hi = (HGuint32)hgGet64h(a);
        lo = (HGuint32)hgGet64l(a);

        if (sh >= 32)
        {
                lo = hi >> (sh-32);
                hi = 0;
        } else
        {
                if (sh)
                {
                        lo = ((HGuint32)(lo) >> sh) | (hi << (32-sh));
                        hi >>= sh;
                }
        }

        return hgSet64((HGint32)hi,(HGint32)lo);
}

/*-------------------------------------------------------------------*//*!
 * \brief       Logical shift right for 64-bit integers (with limited
 *                      shifter range)
 * \param       a       64-bit integer
 * \param       sh      Shift value [0,31]
 * \return      Resulting 64-bit integer
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgLsr64_0_31 (
        const HGint64   a,
        HGint32                 sh)
{

        HGuint32 hi     = (HGuint32)hgGet64h(a);
        HGuint32 lo     = (HGuint32)hgGet64l(a);

        HG_ASSERT (sh >= 0 && sh <= 31);

        {
                HGint32  mask = (HGint32)((sh == 0) ? 0 : hi);
                lo = ((HGuint32)(lo) >> sh) | (mask << (32-sh));
                hi >>= sh;
        }

        return hgSet64((HGint32)hi,(HGint32)lo);
}


/*-------------------------------------------------------------------*//*!
 * \brief       Performs arithmetic shift right for a 64-bit value
 * \param       a       64-bit integer
 * \param       sh      Shift value [0,63]
 * \return      Resulting 64-bit integer
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgAsr64 (
        const HGint64   a,
        HGint32                 sh)
{
        HGint32 hi,lo;

        HG_ASSERT(sh >= 0 && sh <= 63);

        lo = (HGint32)(((HGuint32)(a.l) >> sh) | (a.h << (32-sh)));
        hi = a.h >> sh;

        if(sh == 0)
        {
                hi = a.h;
                lo = a.l;
        }

        if (sh >= 32)
        {
                lo = a.h >> (sh-32);
                hi = a.h >> 31;
        }

        return hgSet64(hi,lo);
}

/*-------------------------------------------------------------------*//*!
 * \brief       Performs arithmetic shift right for a 64-bit value
 * \param       a       64-bit integer
 * \param       sh      Shift value [0,31]
 * \return      64-bit integer
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgAsr64_0_31 (
        const HGint64   a,
        HGint32                 sh)
{
        HGint32 hi,lo;

        HG_ASSERT(sh >= 0 && sh <= 31);

        lo = (HGint32)(((HGuint32)(a.l) >> sh) | (a.h << (32-sh)));
        hi = a.h >> sh;

        if(sh == 0)
        {
                hi = a.h;
                lo = a.l;
        }

        return hgSet64(hi,lo);
}


/*-------------------------------------------------------------------*//*!
 * \brief       Performs arithmetic shift right and returns bottom 32
 *                      bits of the result
 * \param       a       64-bit integer
 * \param       sh      Shift value [0,63]
 * \return      Low 32 bits of a 64-bit integer
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint32 hgAsr64l (
        const HGint64   a,
        HGint32                 sh)
{
        HGint32 lo, sh2 = sh-32;

        HG_ASSERT(sh >= 0 && sh <= 63);

#       if (HG_SHIFTER_MODULO <= 32)
                lo = sh ? (HGint32)(((HGuint32)(a.l) >> sh) | (a.h << (32-sh))) : a.l;
#       else
                lo = ((HGuint32)(a.l) >> sh) | (a.h << (32-sh));
#       endif

        if (sh2 >= 0)
                lo = a.h >> sh2;

        return lo;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Performs 64-bit shift operation, returns low 32 bits
 * \param       a       64-bit integer
 * \param       sh      Shift value [0,63]
 * \return      Low 32 bits of a 64-bit integer
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint32 hgLsr64l (
        const HGint64   a,
        HGint32                 sh)
{
        HG_ASSERT(sh >= 0 && sh <= 63);
        return hgLsr64(a,sh).l;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Negates a 64-bit value
 * \param       a       Input 64-bit integer
 * \return      64-bit integer after negation
 * \todo        Optimize for VC02 (using hgSub64()?)
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgNeg64 (const HGint64 a)
{
#if defined (HG_ADS_ARM_ASSEMBLER)
        HGint64 r;
        __asm {
                RSBS r.l, a.l, #0
                RSC  r.h, a.h, #0
        }
        return r;
#else
        HGint32 lo              = -a.l;
        HGint32 mask    = (lo | a.l)>>31;
        HGint32 hi              = ~a.h - ~mask;
        return hgSet64(hi,lo);
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Performs logical not for a 64-bit integer
 * \param       a       Input 64-bit integer
 * \return      64-bit integer
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgNot64 (const HGint64 a)
{
        return hgSet64(~a.h,~a.l);
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns absolute value of 64-bit integer
 * \param       a       Input 64-bit integer
 * \return      Absolute value of 64-bit integer
 * \note        If high 32 bits of a are 0x80000000, the result is still
 *                      negative!
 * \todo        Improve for RVCT
 * \todo        Optimize for VC02
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgAbs64 (HGint64 a)
{
        HGint64 r = hgNeg64(a);
        if (hgGet64h(a) < 0)
                a = r;
        return a;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns negative absolute value of 64-bit integer
 * \param       a       Input 64-bit integer
 * \return      Negative absolute value of 64-bit integer
 * \todo        Improve for RVCT
 * \todo        Optimize for VC02
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgNabs64 (HGint64 a)
{
        HGint64 r = hgNeg64(a);
        if (!(hgGet64h(a) < 0))
                a = r;

        return a;
}

/*-------------------------------------------------------------------*//*!
 * \brief       32x32->64-bit signed multiplication
 * \param       a       32-bit signed integer
 * \param       b       32-bit signed integer
 * \return      64-bit signed result of the multiplication
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgMul64 (
        HGint32 a,
        HGint32 b)
{
#if defined (HG_ADS_ARM_ASSEMBLER) && defined(__TARGET_FEATURE_MULTIPLY)
        __asm { SMULL a, b, a, b}
        return hgSet64(b,a);
#elif (HG_COMPILER == HG_COMPILER_HCVC) && (HG_CPU == HG_CPU_VC02)
        return hgSet64(hgMul64h(a,b), a * b);
#else
        HGuint32 hh             = (HGuint32)((a>>16)   *(b>>16));
        HGuint32 lh             = (HGuint32)((a&0xFFFF)*(b>>16));
        HGuint32 hl             = (HGuint32)((a>>16)   *(b&0xFFFF));
        HGuint32 ll             = (HGuint32)((a&0xFFFF)*(b&0xFFFF));
        HGuint32 oldlo;

        hh += (HGint32)(lh)>>16;
        hh += (HGint32)(hl)>>16;

        oldlo   =  ll;
        ll              += lh<<16;
        if (ll < oldlo)
                hh++;

        oldlo   =  ll;
        ll              += hl<<16;
        if (ll < oldlo)
                hh++;

        return hgSet64((HGint32)hh,(HGint32)ll);
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       32x32->64-bit unsigned multiplication
 * \param       a       32-bit unsigned integer
 * \param       b       32-bit unsigned integer
 * \return      64-bit unsigned result of the multiplication
 *//*-------------------------------------------------------------------*/

HG_INLINE HGuint64 hgMulu64 (
        HGuint32 a,
        HGuint32 b)
{
#if defined (HG_ADS_ARM_ASSEMBLER) && defined(__TARGET_FEATURE_MULTIPLY)
        HGuint64 r;
        __asm { UMULL r.l, r.h, a, b }
        return r;
#elif (HG_COMPILER == HG_COMPILER_HCVC) && (HG_CPU == HG_CPU_VC02)
        return hgSet64u(hgMulu64h(a,b), a * b);
#else
        HGuint32 hh = (a>>16)   * (b>>16);
        HGuint32 lh = (a&0xFFFF)* (b>>16);
        HGuint32 hl = (a>>16)   * (b&0xFFFFu);
        HGuint32 ll = (a&0xFFFF)* (b&0xFFFFu);
        HGuint32 oldlo;

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

        return hgSet64u(hh,ll);
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       64x64->64-bit signed multiplication (returns lowest 64-bits)
 * \param       a       64-bit signed integer
 * \param       b       64-bit signed integer
 * \return      64-bit signed result of the multiplication (lowest bits)
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgMul64x64 (
        HGint64 a,
        HGint64 b)
{
        HGint64 res = hgToSigned64(hgMulu64((HGuint32)hgGet64l(a), (HGuint32)hgGet64l(b)));
        res = hgSet64(hgGet64h(res) + hgGet64h(a) * hgGet64l(b), hgGet64l(res));
        res = hgSet64(hgGet64h(res) + hgGet64h(b) * hgGet64l(a), hgGet64l(res));
        return res;
}

/*-------------------------------------------------------------------*//*!
 * \brief       64x64->64-bit unsigned multiplication (returns lowest 64-bits)
 * \param       a       64-bit unsigned integer
 * \param       b       64-bit unsigned integer
 * \return      64-bit unsigned result of the multiplication (lowest bits)
 *//*-------------------------------------------------------------------*/

HG_INLINE HGuint64 hgMulu64x64 (
        HGuint64 a,
        HGuint64 b)
{
        HGint64 res = hgToSigned64(hgMulu64(hgGet64ul(a), hgGet64ul(b) ));
        res = hgSet64(hgGet64h(res) + hgGet64uh(a) * hgGet64ul(b), hgGet64l(res));
        res = hgSet64(hgGet64h(res) + hgGet64uh(b) * hgGet64ul(a), hgGet64l(res));
        return hgToUnsigned64(res);
}

/*-------------------------------------------------------------------*//*!
 * \brief       64x32->64-bit signed multiplication (returns lowest 64-bits)
 * \param       a       64-bit signed integer
 * \param       b       32-bit signed integer
 * \return      64-bit signed result of the multiplication (lowest bits)
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgMul64x32 (
        HGint64 a,
        HGint32 b)
{
        HGint64 res = hgToSigned64(hgMulu64((HGuint32)hgGet64l(a), (HGuint32)b));
        res = hgSet64(hgGet64h(res) + (hgGet64h(a) * b) - (hgGet64l(a)&(b>>31)),        hgGet64l(res));
        return res;
}

/*-------------------------------------------------------------------*//*!
 * \brief       64x32->64-bit unsigned multiplication (returns lowest 64-bits)
 * \param       a       64-bit unsigned integer
 * \param       b       32-bit unsigned integer
 * \return      64-bit unsigned result of the multiplication (lowest 64 bits)
 *//*-------------------------------------------------------------------*/

HG_INLINE HGuint64 hgMulu64x32 (
        HGuint64 a,
        HGuint32 b)
{
        HGint64 res = hgToSigned64(hgMulu64(hgGet64ul(a), b ));
        res = hgSet64(hgGet64h(res) + hgGet64uh(a) * b, hgGet64l(res));
        return hgToUnsigned64(res);
}


/*-------------------------------------------------------------------*//*!
 * \brief       Performs a 32*32 -> 64 signed multiplication followed
 *                      by a 64-bit addition
 * \param       a               64-bit signed integer
 * \param       b               32-bit signed integer
 * \param       c               32-bit signed integer
 * \return      a+(b*c)
 * \note        This function produces the same results as
 *                      hgAdd64(a,hgMul64(b,c)) but is provided as a separate
 *                      function because some platforms have custom instructions
 *                      for implementing this.
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgMadd64 (
        HGint64 a,
        HGint32 b,
        HGint32 c)
{
#if defined (HG_ADS_ARM_ASSEMBLER) && defined(__TARGET_FEATURE_MULTIPLY)
        __asm { SMLAL a.l, a.h, b, c}
        return a;
#else
        return hgAdd64(a,hgMul64(b,c));
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Performs a 32*32 -> 64 signed multiplication followed
 *                      by a 64-bit addition, return high 32-bits
 * \param       a               64-bit signed integer
 * \param       b               32-bit signed integer
 * \param       c               32-bit signed integer
 * \return      (a+(b*c))>>32
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint32 hgMadd64h (
        HGint64 a,
        HGint32 b,
        HGint32 c)
{
#if defined (HG_ADS_ARM_ASSEMBLER) && defined(__TARGET_FEATURE_MULTIPLY)
        __asm { SMLAL a.l, a.h, b, c}
        return a.h;
#else
        return hgGet64h(hgAdd64(a,hgMul64(b,c)));
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Performs a 32*32 -> 64 unsigned multiplication followed
 *                      by a 64-bit addition
 * \param       a               64-bit unsigned integer
 * \param       b               32-bit unsigned integer
 * \param       c               32-bit unsigned integer
 * \return      a+(b*c)
 * \note        This function produces the same results as
 *                      hgAdd64(hgMulu64(a,b),c) but is provided as a separate
 *                      function because some platforms have custom instructions
 *                      for implementing this.
 *//*-------------------------------------------------------------------*/

HG_INLINE HGuint64 hgMaddu64 (
        HGuint64 a,
        HGuint32 b,
        HGuint32 c)
{
#if defined (HG_ADS_ARM_ASSEMBLER) && defined(__TARGET_FEATURE_MULTIPLY) && (HG_COMPILER != HG_COMPILER_ADS)
        __asm { UMLAL a.l, a.h, b, c}
        return a;
#else
        return hgToUnsigned64(hgAdd64(hgToSigned64(a),hgToSigned64(hgMulu64(b,c))));
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Performs a 32x32->64 signed multiplication followed by
 *                      a shift to the right
 * \param       a               32-bit signed integer
 * \param       b               32-bit signed integer
 * \param       shift   Shift value [0,63]
 * \return      64-bit integer
 * \note        This function is somewhat obsolete as exactly the same
 *                      code is generated by mul+asr combination. It is provided
 *                      mainly so that assembly optimizations could be performed
 *                      for some platforms.
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgMulAsr64 (
        HGint32         a,
        HGint32         b,
        HGint32         shift)
{
        HG_ASSERT (shift <= 63);
        return hgAsr64(hgMul64(a,b),shift);
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns number of leading zeroes for a 64-bit integer
 * \param       a       Input 64-bit integer
 * \return      Number of leading zeroes [0,64]
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint32 hgClz64 (const HGint64 a)
{
        HGuint32        x               = (HGuint32)hgGet64h(a);
        int         bits        = 0;

        if (!x)
        {
                x               = (HGuint32)hgGet64l(a);
                bits    = 32;
        }

        return bits + hgClz32(x);
}


/*-------------------------------------------------------------------*//*!
 * \brief       Returns number of trailing zeroes for a 64-bit integer
 * \param       a       Input 64-bit integer
 * \return      Number of trailing zeroes [0,64]
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint32 hgCtz64 (const HGint64 a)
{
        HGuint32        x               = (HGuint32)hgGet64l(a);
        int         bits        = 0;

        if (!x)
        {
                x               = (HGuint32)hgGet64h(a);
                bits    = 32;
        }

        return bits + hgCtz32(x);
}

/*-------------------------------------------------------------------*//*!
 * \brief       Population count of a 64-bit integer
 * \param       a       64-bit input integer
 * \return      Population count [0,64] (i.e., number of set bits)
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint32 hgPop64 (const HGint64 a)
{
        return hgPop32((HGuint32)hgGet64h(a)) +
                   hgPop32((HGuint32)hgGet64l(a));
}


/*-------------------------------------------------------------------*//*!
 * \brief       Performs addition, saturates signed sum to
 *          0x80000000 and 0x7fffffff
 * \param       a       First 32-bit signed integer
 * \param       b       Second 32-bit signed integer
 * \return      saturated sum
 * \todo        move to hgInt32.h
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint32 hgAdds32 (
        HGint32 a,
        HGint32 b)
{
#if (defined HG_ADS_ARM_ASSEMBLER) && (HG_COMPILER != HG_COMPILER_ADS)
        const HGint32 t = 0x80000000;

        __asm
        {
                adds    a,a,b
                eorvs   a,t,a,ASR #31
        }
        return a;
#elif (HG_COMPILER == HG_COMPILER_HCVC) && (HG_CPU == HG_CPU_VC02)
        return _adds (a,b);
#else
        HGint32 v = hgGet64h(hgAdd64(hgSet64s(a),hgSet64s(b)));
        a += b;

        if((v ^ a) < 0) /* different sign? */
        {
                a = v^0x7fffffff;
        }

        return a;
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Internal routine for performing division
 *//*-------------------------------------------------------------------*/

HG_INLINE HGuint32 hgDiv32Internal (
        HGuint32        cio[1], 
        HGuint64        aio[1], 
        HGuint32        b, 
        HGint32         n)
{
        HG_ASSERT(n >= 0 && n <= 31);
        HG_ASSERT(n == 31 || cio[0] == 0);

        {
                HGuint32        q;
                HGuint32        ah = hgGet64uh(aio[0]);
                HGuint32        al = hgGet64ul(aio[0]);
                HGuint32        carry = cio[0];
                HGuint64        a;

                q = 0;

                do
                {
                        if(!carry)      carry = (ah >= b);
                        if(carry)       ah -= b;
                        q = q+q+carry;
                        carry = (ah >= 0x80000000u ? 1 : 0);
                        a = hgToUnsigned64(hgAdd64(hgSet64(ah,al),hgSet64(ah,al)));
                        al = hgGet64ul(a);
                        ah = hgGet64uh(a);
                } while(n--);

                aio[0] = hgSet64u(ah,al);
                cio[0] = carry;
                return q;
        }
}

/*-------------------------------------------------------------------*//*!
 * \brief       Performs 64:32 unsigned division
 * \param       a       64-bit unsigned integer to be divided
 * \param       b       32-bit unsigned integer divisor
 * \return      (a/b) (64-bit unsigned)
 * \note        Division by zero is asserted in debug build
 *//*-------------------------------------------------------------------*/

HG_INLINE HGuint64 hgDivu64_32 (
        HGuint64 a,
        HGuint32 b)
{
#if (HG_COMPILER == HG_COMPILER_HCVC) && (HG_CPU == HG_CPU_VC02)
        unsigned long long x = ((unsigned long long)(a.h) << 32) | a.l;
        x /= b;
        return hgSet64u((HGuint32)(x >> 32), (HGuint32)(x));
#else
        HGuint32        qh, ql, carry;
        HGint32         sha, shb;
        
        HG_ASSERT (b != 0);

        if(hgLessThan64_32u(a,b))
                return hgSet64u(0,0);

        shb             = hgClz32(b);
        b         <<= shb;
        sha             = hgClz64(hgToSigned64(a));
        a               = hgToUnsigned64(hgLsl64(hgToSigned64(a),sha));
        carry   = 0;
        qh              = 0;

        if((shb-sha) >= 0)
                qh = hgDiv32Internal(&carry, &a, b, shb-sha);

        ql = hgDiv32Internal(&carry, &a, b, hgMin32(32+shb-sha,31));

        return hgSet64u(qh,ql);
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Performs 64:32 signed division
 * \param       a       64-bit signed integer to be divided
 * \param       b       32-bit signed integer divisor
 * \return      (a/b) (64-bit)
 * \note        Division by zero is asserted in debug build
 *//*-------------------------------------------------------------------*/

HG_INLINE HGint64 hgDiv64_32 (
        HGint64 a,
        HGint32 b)
{
        HGbool          sign = (HGbool)(((HGuint32)(hgGet64h(a)^b))>>31);
        HGint64         res;

        HG_ASSERT (b != 0);

        res = hgToSigned64(hgDivu64_32(hgToUnsigned64(hgAbs64(a)),(HGuint32)hgAbs32(b)));

        if (sign)                                       /* todo: avoid comparison?      */
                res = hgNeg64(res);

        return res;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns 0xffffffff:ffffffff / b
 * \param       b       32-bit unsigned integer divisor
 * \return      (0xffffffff:ffffffff/b) (64-bit unsigned)
 * \note        Division by zero is asserted in debug build
 *//*-------------------------------------------------------------------*/

HG_INLINE HGuint64 hgRcpu64 (
        HGuint32 b)
{
        return hgDivu64_32(hgSet64u(0xffffffffu,0xffffffffu),b);
}


#undef HG_ADS_ARM_ASSEMBLER

/*@=shiftimplementation =shiftnegative =predboolint =boolops@*/

/*----------------------------------------------------------------------*/
