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
 * Description:         Routines for 8/16/32-bit memory fill operations
 *
 * $Id: //hybrid/libs/hg/main/hgFill.c#1 $
 * $Date: 2005/11/08 $
 * $Author: janne $ *
 *----------------------------------------------------------------------*/

#include "hgfill.h"

/*@-shiftimplementation -predboolint -boolops*/

/*lint -e528 -e702 */   /* don't whine about signed right shifts */

/* 
        ARM assembler variants of the routines. NOTE: WE USE FALLTHRUs
        so DO NOT MOVE THESE FUNCTIONS, OR CHANGE THEIR ORDER OR ADD
        ANY FUNCTIONS IN _BETWEEN_ THEM!!
*/

/* \todo [petri 25/Oct/04] Enable these only when it has been proven that they work on all of our target platforms! */
#if 0 /*(HG_COMPILER == HG_COMPILER_RVCT) && defined(HG_SUPPORT_ASM) && !defined (__thumb)*/

__asm void hgFill8 (
    HGuint8* d, 
    HGuint8  v,  
    int      N)
{
    CMP         r2, #0
    BXLE                lr
    TST         r0, #1
    ADD         r3, r0, r2
    STRB        r1, [r3, #-1]             
    STRNEB      r1, [r0], #1              
    SUBNE       r2, r2, #1
    MOV         r2, r2, LSR #1            
    ADD         r1, r1, r1, LSL #8         
    /* FALLTHRU TO HGFILL16 */
}

__asm void hgFill16 (
    HGuint16* d,
    HGuint16  v,
    int       N)
{
    CMP         r2, #0
    BXLE                lr
    ADD         r3, r0, r2, LSL #1          
    STRH        r1, [r3, #-2]               
    TST         r0, #3
    STRNEH      r1, [r0], #2              
    SUBNE       r2, r2, #1
    MOV         r2, r2, LSR #1            
    ADD         r1, r1,r1, LSL #16        
    /* FALLTHRU TO HGFILL32 */
}

__asm void hgFill32 (
    HGuint32* d,            
    HGuint32  v,            
    int       N)            
{
    STMDB       sp!, {r4,r5,r6}             /* we are going to trash three regs.. */
    MOV         r6, r1                      /* duplicate v to r1,r4,r5,r6 */
    MOV         r5, r1
    MOV         r4, r1
    MOVS        r3, r2, LSR #3              

|L.1|                                      /* fills 32 bytes at a time */
    STMGTIA     r0!, {r1,r4,r5,r6}
    STMGTIA     r0!, {r1,r4,r5,r6}
    SUBNES      r3,  r3, #1
    BNE         |L.1|

    TST         r2,  #4                     
    STMNEIA     r0!, {r1,r4,r5,r6}          /* next four words? */
    MOVS        r2,  r2, LSL #31
    STMHSIA     r0!, {r5,r6}                /* next two?        */
    STRMI       r1,  [r0]                   /* last one?        */
    LDMIA       sp!, {r4,r5,r6}             /* restore stack..  */
    BX                  lr                                                      /* and return       */
}

#else

/*-------------------------------------------------------------------*//*!
 * \brief       Fills memory with specified pattern
 * \param       d       Pointer to destination HGuint32 buffer
 * \param       v       Pattern to use
 * \param       N       Number of entries to fill
 * \note        This is the internal workhorse routine, unrolled by 8 and
 *                      with a variant of Duff's device for handling the trail
 *//*-------------------------------------------------------------------*/

HG_INLINE void hgFill32_inner (
        HGuint32*       d,      
        HGuint32        v, 
        int                     N)
{

        HG_ASSERT (d != HG_NULL && N >= 0 && (N == 0 || !((HGuintptr)(d) & 3)) );

        switch (N & 7)                                  /* FALLTHRU */
        {
                default: HG_NO_DEFAULT_CASE                                             /*@fallthrough@*/
                case 7: d[6] = v;                       /*lint !e616 */         /*@fallthrough@*/
                case 6: d[5] = v;                       /*lint !e616 */         /*@fallthrough@*/
                case 5: d[4] = v;                       /*lint !e616 */         /*@fallthrough@*/
                case 4: d[3] = v;                       /*lint !e616 */         /*@fallthrough@*/
                case 3: d[2] = v;                       /*lint !e616 */         /*@fallthrough@*/
                case 2: d[1] = v;                       /*lint !e616 */         /*@fallthrough@*/
                case 1: d[0] = v;                       /*lint !e616 */         /*@fallthrough@*/
                case 0: break;
        }

        d += (N&7);

        N>>=3;

        while (N--)                                             /* process chunks of 8 dwords at a time */
        {
                d[0] = d[1] = d[2] = d[3] = d[4] = d[5] = d[6] = d[7] = v;
                d+=8;
        }

}

/*-------------------------------------------------------------------*//*!
 * \brief       Fills memory with specified 8-bit pattern
 * \param       d       Pointer to destination HGuint8 buffer
 * \param       v       Pattern to use
 * \param       N       Number of entries to fill (must be >= 0)
 *//*-------------------------------------------------------------------*/

void hgFill8 (
        HGuint8* d,     
        HGuint8  v,  
        int              N)
{
        HG_ASSERT (d != HG_NULL && N >= 0);

        while (N && ((HGuintptr)(d) & 3ul))     /* while misaligned */ /*lint !e620 */
        {
                *d++ = v;
                --N;
        }

        while (N & 3)                                           /* trailing */
        {
                d[--N] = v;
        }

        HG_ASSERT (!(N&3));                     

        {
                HGuint32 v32 = (HGuint32)v;
                v32 += (v32<<8);
                v32 += (v32<<16);

                hgFill32_inner ((HGuint32*)((HGuintptr)d), v32, N>>2);
        }
}

/*-------------------------------------------------------------------*//*!
 * \brief       Fills memory with specified 16-bit pattern
 * \param       d       Pointer to destination HGuint16 buffer
 * \param       v       Pattern to use
 * \param       N       Number of entries to fill (must be >= 0)
 *//*-------------------------------------------------------------------*/

void hgFill16 (
        HGuint16* d,    
        HGuint16  v, 
        int               N)
{
        HG_ASSERT (!((HGuintptr)(d) & 1ul) && d && N >= 0);     /*lint !e620 */

        if (N > 0)
        {
                HGuint32 v32;

                d[N-1] = v;                             /* handle padding (don't bother checking alignment) */

                if ((HGuintptr)(d) & 3ul)       /* misaligned? */ /*lint !e620 */
                {
                        *d++ = v;
                        N--;
                }

                v32 = (HGuint32)v;
                v32 += (v32<<16);

                hgFill32_inner ((HGuint32*)((HGuintptr)d), v32, N>>1);
        }
}

/*-------------------------------------------------------------------*//*!
 * \brief       Fills memory with specified 32-bit pattern
 * \param       d       Pointer to destination HGuint32 buffer
 * \param       v       Pattern to use
 * \param       N       Number of entries to fill (must be >= 0)
 *//*-------------------------------------------------------------------*/

void hgFill32 (
        HGuint32*       d,      
        HGuint32        v, 
        int                     N)
{
        HG_ASSERT (!((HGuintptr)(d) & 3ul) && d && N >= 0); /*lint !e620 */
        hgFill32_inner (d, v, N);
}

#endif
/*----------------------------------------------------------------------*/


