#ifndef __HGPREDICATE_H
#define __HGPREDICATE_H
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
 * Description:         Predicate support
 *
 * $Id: //hybrid/libs/hg/main/hgPredicate.h#2 $
 * $Date: 2005/11/08 $
 * $Author: janne $ *
 *----------------------------------------------------------------------*/

#if !defined (__HGINT64_H)
#       include "hgint64.h"
#endif

/* TODO should we actually use int32 here? */
typedef HGint32 HGpredicate;    /*!< -1 or 0 (used by predicates)*/

/*----------------------------------------------------------------------*
 * 32-bit predicate routines. These functions perform a comparison
 * (without using branching) and return -1 if the result is true,
 * or 0 otherwise. The return value can then be used for constructing
 * branchless conditional instructions on platforms that lack predicated
 * instructions. 
 *
 * For example, a conditional assignment can be performed using the
 * resulting mask as follows:
 *
 * a = a + ((b-a) & mask);      
 *
 * If a boolean result is needed (i.e., true = 1, false = 0), it can
 * be simply obtained by negating the result of a predicate function.
 *----------------------------------------------------------------------*/

HG_INLINE       HGpredicate             hgPredEQZero32          (HGint32);                              /* (a == 0) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredNEZero32          (HGint32);                              /* (a != 0) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredLTZero32          (HGint32);                              /* (a <  0) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredGTZero32          (HGint32);                              /* (a >  0) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredLEZero32          (HGint32);                              /* (a <= 0) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredGEZero32          (HGint32);                              /* (a >= 0) ? -1 : 0 */

HG_INLINE       HGpredicate             hgPredEQ32                      (HGint32,  HGint32);    /* (a == b) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredNE32                      (HGint32,  HGint32);    /* (a != b) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredLT32                      (HGint32,  HGint32);    /* (a <  b) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredGT32                      (HGint32,  HGint32);    /* (a >  b) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredGE32                      (HGint32,  HGint32);    /* (a >= b) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredLE32                      (HGint32,  HGint32);    /* (a <= b) ? -1 : 0 */

HG_INLINE       HGpredicate             hgPredLTU32                     (HGuint32, HGuint32);   /* (a <  b) ? -1 : 0 (unsigned) */
HG_INLINE       HGpredicate             hgPredGTU32                     (HGuint32, HGuint32);   /* (a >  b) ? -1 : 0 (unsigned) */
HG_INLINE       HGpredicate             hgPredGEU32                     (HGuint32, HGuint32);   /* (a >= b) ? -1 : 0 (unsigned) */
HG_INLINE       HGpredicate             hgPredLEU32                     (HGuint32, HGuint32);   /* (a <= b) ? -1 : 0 (unsigned) */

/*----------------------------------------------------------------------*
 * 64-bit predicate routines. Note that output predicate is still
 * 32-bit, use hgSet64(mask,mask) to generate a 64-bit mask.
 *----------------------------------------------------------------------*/

HG_INLINE       HGpredicate             hgPredEQZero64          (HGint64);                              /* (a == 0) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredNEZero64          (HGint64);                              /* (a != 0) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredLTZero64          (HGint64);                              /* (a <  0) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredGTZero64          (HGint64);                              /* (a >  0) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredLEZero64          (HGint64);                              /* (a <= 0) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredGEZero64          (HGint64);                              /* (a >= 0) ? -1 : 0 */

HG_INLINE       HGpredicate             hgPredEQ64                      (HGint64,  HGint64);    /* (a == b) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredNE64                      (HGint64,  HGint64);    /* (a != b) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredLT64                      (HGint64,  HGint64);    /* (a <  b) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredGT64                      (HGint64,  HGint64);    /* (a >  b) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredGE64                      (HGint64,  HGint64);    /* (a >= b) ? -1 : 0 */
HG_INLINE       HGpredicate             hgPredLE64                      (HGint64,  HGint64);    /* (a <= b) ? -1 : 0 */

HG_INLINE       HGpredicate             hgPredLTU64                     (HGuint64, HGuint64);   /* (a <  b) ? -1 : 0 (unsigned) */
HG_INLINE       HGpredicate             hgPredGTU64                     (HGuint64, HGuint64);   /* (a >  b) ? -1 : 0 (unsigned) */
HG_INLINE       HGpredicate             hgPredGEU64                     (HGuint64, HGuint64);   /* (a >= b) ? -1 : 0 (unsigned) */
HG_INLINE       HGpredicate             hgPredLEU64                     (HGuint64, HGuint64);   /* (a <= b) ? -1 : 0 (unsigned) */

#include "hgPredicate.inl"

/*----------------------------------------------------------------------*/
#endif /* __HGPREDICATE_H */
