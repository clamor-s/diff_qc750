/*------------------------------------------------------------------------
 *
 * Hybrid Compatibility Header
 * -----------------------------------------
 *
 * (C) 2002-2003 Hybrid Graphics, Ltd.
 * All Rights Reserved.
 *
 * This file consists of unpublished, proprietary source code of
 * Hybrid Graphics, and is considered Confidential Information for
 * purposes of non-disclosure agreement. Disclosure outside the terms
 * outlined in signed agreement may result in irrepairable harm to
 * Hybrid Graphics and legal action against the party in breach.
 *
 * Description:         Inline code for hgPredicate.h
 * Note:                        This file is included _only_ by hgPredicate.h
 *
 * \note The code has been currently hand-optimized for MSVC and GCC.
 *               If you need to change the implementation in order to improve
 *               the code for some compiler, please use appropriate #ifdefs..
 *
 * $Id: //hybrid/libs/hg/main/hgPredicate.inl#1 $
 * $Date: 2005/11/08 $
 * $Author: janne $ *
 *----------------------------------------------------------------------*/


/*-------------------------------------------------------------------*//*!
 * \brief       Predicated equality comparison against zero
 * \param       a       32-bit signed integer
 * \return      -1 if a == 0, 0 otherwise
 * \note        The same routine can be used for comparing unsigned integers
 *                      against zero.
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredEQZero32 (HGint32 a)
{
#if (HG_COMPILER == HG_COMPILER_MSC)
        return ~((a != 0) ? -1 : 0);    /* generates better code */
#else
        return (a == 0) ? -1 : 0;
#endif
}       

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated inequality comparison against zero
 * \param       a       32-bit signed integer
 * \return      -1 if a != 0, 0 otherwise
 * \note        The same routine can be used for comparing unsigned integers
 *                      against zero.
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredNEZero32 (HGint32 a)
{
        return (a != 0) ? -1 : 0;
}


/*-------------------------------------------------------------------*//*!
 * \brief       Predicated less-than comparison against zero
 * \param       a       32-bit signed integer
 * \return      -1 if a < 0, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredLTZero32 (HGint32 a)
{
        return (a >> 31);       /* propagate sign */
}

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated greater-than comparison against zero
 * \param       a       32-bit signed integer
 * \return      -1 if a > 0, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredGTZero32 (HGint32 a)
{
        return (a > 0) ? -1 : 0;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated greater-than-or-equal comparison against zero
 * \param       a       32-bit signed integer
 * \return      -1 if a >= 0, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredGEZero32 (HGint32 a)
{
/* Need to use inline assembler here to get the single-instruction 
   variant, as GCC's optimizer otherwise generates three instructions! 
*/
#if (HG_COMPILER == HG_COMPILER_GCC) && (HG_CPU == HG_CPU_ARM) && defined (HG_SUPPORT_ASM)
        asm ("mvn %[a], %[a], asr #31\n"
                   : [a] "+r" (a) );                                    
        return a;
#else
        return ~(a>>31);
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated less-than-or-equal comparison against zero
 * \param       a       32-bit signed integer
 * \return      -1 if a <= 0, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredLEZero32 (HGint32 a)
{
        return (a <= 0) ? -1 : 0;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated equality comparison between two signed integers
 * \param       a       First 32-bit signed integer
 * \param       b       Second 32-bit signed integer
 * \return      -1 if a == b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredEQ32 (
        HGint32 a,  
        HGint32 b)

{
        return (a == b) ? -1 : 0;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated inequality comparison between two signed integers
 * \param       a       First 32-bit signed integer
 * \param       b       Second 32-bit signed integer
 * \return      -1 if a != b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredNE32 (
        HGint32 a,  
        HGint32 b)
{
        return (a-b) ? -1 : 0;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated less-than comparison between two signed integers
 * \param       a       First 32-bit signed integer
 * \param       b       Second 32-bit signed integer
 * \return      -1 if a < b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredLT32 (
        HGint32 a,  
        HGint32 b)

{
        return (a < b) ? -1 : 0;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated greater-than comparison between two signed integers
 * \param       a       First 32-bit signed integer
 * \param       b       Second 32-bit signed integer
 * \return      -1 if a > b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredGT32 (
        HGint32 a,  
        HGint32 b)

{
        return (a > b) ? -1 : 0;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated greater-than-or-equal comparison between two 
 *                      signed integers
 * \param       a       First 32-bit signed integer
 * \param       b       Second 32-bit signed integer
 * \return      -1 if a >= b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredGE32 (
        HGint32 a,  
        HGint32 b)

{
        return (a >= b) ? -1 : 0;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated less-than-or-equal comparison between two 
 *                      signed integers
 * \param       a       First 32-bit signed integer
 * \param       b       Second 32-bit signed integer
 * \return      -1 if a <= b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredLE32 (
        HGint32 a,  
        HGint32 b)

{
        return (a <= b) ? -1 : 0;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated less-than comparison between two 
 *                      unsigned integers
 * \param       a       First 32-bit unsigned integer
 * \param       b       Second 32-bit unsigned integer
 * \return      -1 if a < b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredLTU32 (
        HGuint32 a,  
        HGuint32 b)

{
        return (a < b) ? -1 : 0;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated greater-than comparison between two 
 *                      unsigned integers
 * \param       a       First 32-bit unsigned integer
 * \param       b       Second 32-bit unsigned integer
 * \return      -1 if a > b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredGTU32 (
        HGuint32 a,  
        HGuint32 b)

{
        return (a > b) ? -1 : 0;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated greater-than-or-equal comparison between two 
 *                      unsigned integers
 * \param       a       First 32-bit unsigned integer
 * \param       b       Second 32-bit unsigned integer
 * \return      -1 if a >= b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredGEU32 (
        HGuint32 a,  
        HGuint32 b)

{
#if (HG_COMPILER == HG_COMPILER_MSC)
        return ~((a < b) ? -1 : 0);     /* generates better code */
#else
        return (a >= b) ? -1 : 0;
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated less-than-or-equal comparison between two 
 *                      unsigned integers
 * \param       a       First 32-bit unsigned integer
 * \param       b       Second 32-bit unsigned integer
 * \return      -1 if a <= b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredLEU32 (
        HGuint32 a,  
        HGuint32 b)

{
#if (HG_COMPILER == HG_COMPILER_MSC)
        return ~((a > b) ? -1 : 0); /* generates better code */
#else
        return (a <= b) ? -1 : 0;
#endif
}

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated equality comparison against zero
 * \param       a       64-bit signed integer
 * \return      -1 if a == 0, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredEQZero64 (HGint64 a)
{
#if (HG_COMPILER == HG_COMPILER_MSC)
        return ~(hgIsZero64(a) ? 0 : -1);       /* generates better code */
#else
        return -hgIsZero64(a);
#endif
}       

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated inequality comparison against zero
 * \param       a       64-bit signed integer
 * \return      -1 if a != 0, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredNEZero64 (HGint64 a)
{
        return hgIsZero64(a) ? 0 : -1;
}       

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated less-than comparison against zero
 * \param       a       64-bit signed integer
 * \return      -1 if a < 0, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredLTZero64 (HGint64 a)
{
        return hgPredLTZero32(hgGet64h(a));
}       

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated greater-than-or-equal comparison against zero
 * \param       a       64-bit signed integer
 * \return      -1 if a >= 0, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredGEZero64 (HGint64 a)
{
        return hgPredLTZero32(~hgGet64h(a));
}       

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated less-than-or-equal comparison against zero
 * \param       a       64-bit signed integer
 * \return      -1 if a <= 0, 0 otherwise
 * \note        This function is pretty expensive
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredLEZero64 (HGint64 a)
{
        return (hgPredEQZero64(a) | (hgGet64h(a)>>31));
}       

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated greater-than comparison against zero
 * \param       a       64-bit signed integer
 * \return      -1 if a > 0, 0 otherwise
 * \note        This function is pretty expensive
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredGTZero64 (HGint64 a)
{
        return ~hgPredLEZero64(a);
}

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated equality comparison between two 64-bit ints
 * \param       a       First  64-bit signed integer
 * \param       b       Second 64-bit signed integer
 * \return      -1 if a == b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredEQ64 (
        HGint64 a,
        HGint64 b)
{
#if (HG_COMPILER == HG_COMPILER_MSC)
        return hgPredEQZero64(hgSub64(a,b));
#else
        return -hgEq64(a,b);
#endif
}       

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated inequality comparison between two 64-bit ints
 * \param       a       First  64-bit signed integer
 * \param       b       Second 64-bit signed integer
 * \return      -1 if a != b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredNE64 (
        HGint64 a,
        HGint64 b)
{
#if (HG_COMPILER == HG_COMPILER_MSC)
        return hgPredNEZero64(hgSub64(a,b));
#else
        return ~hgPredEQ64(a,b);
#endif
}       

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated less-than comparison between two 64-bit ints
 * \param       a       First  64-bit signed integer
 * \param       b       Second 64-bit signed integer
 * \return      -1 if a < b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredLT64 (
        HGint64 a,
        HGint64 b)
{
        return -hgLessThan64(a,b);
}       

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated greater-than-or-equal comparison between 
 *                      two 64-bit ints
 * \param       a       First  64-bit signed integer
 * \param       b       Second 64-bit signed integer
 * \return      -1 if a >= b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredGE64 (
        HGint64 a,
        HGint64 b)
{
        return ~hgPredLT64(a,b);
}       

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated greater-than comparison between 
 *                      two 64-bit ints
 * \param       a       First  64-bit signed integer
 * \param       b       Second 64-bit signed integer
 * \return      -1 if a > b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredGT64 (
        HGint64 a,
        HGint64 b)
{
        return -hgLessThan64(b,a);
}       

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated greater-than comparison between 
 *                      two 64-bit ints
 * \param       a       First  64-bit signed integer
 * \param       b       Second 64-bit signed integer
 * \return      -1 if a <= b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredLE64 (
        HGint64 a,
        HGint64 b)
{
        return ~hgPredGT64(a,b);
}


/*-------------------------------------------------------------------*//*!
 * \brief       Predicated less-than comparison between two 64-bit ints
 * \param       a       First  64-bit unsigned integer
 * \param       b       Second 64-bit unsigned integer
 * \return      -1 if a < b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredLTU64 (
        HGuint64 a,
        HGuint64 b)
{
        return -hgLessThan64u(a,b);
}       

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated greater-than-or-equal comparison between 
 *                      two 64-bit ints
 * \param       a       First  64-bit unsigned integer
 * \param       b       Second 64-bit unsigned integer
 * \return      -1 if a >= b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredGEU64 (
        HGuint64 a,
        HGuint64 b)
{
        return ~hgPredLTU64(a,b);
}       

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated greater-than comparison between 
 *                      two 64-bit ints
 * \param       a       First  64-bit unsigned integer
 * \param       b       Second 64-bit unsigned integer
 * \return      -1 if a > b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredGTU64 (
        HGuint64 a,
        HGuint64 b)
{
        return -hgLessThan64u(b,a);
}       

/*-------------------------------------------------------------------*//*!
 * \brief       Predicated greater-than comparison between 
 *                      two 64-bit ints
 * \param       a       First  64-bit unsigned integer
 * \param       b       Second 64-bit unsigned integer
 * \return      -1 if a <= b, 0 otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE HGpredicate hgPredLEU64 (
        HGuint64 a,
        HGuint64 b)
{
        return ~hgPredGTU64(a,b);
}       
        
        
/*----------------------------------------------------------------------*/



