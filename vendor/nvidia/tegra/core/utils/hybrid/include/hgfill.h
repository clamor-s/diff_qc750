#ifndef __HGFILL_H
#define __HGFILL_H
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
 * $Id: //hybrid/libs/hg/main/hgFill.h#1 $
 * $Date: 2005/11/08 $
 * $Author: janne $ *
 *----------------------------------------------------------------------*/

#if !defined (__HGDEFS_H)
#       include "hgdefs.h"
#endif

/*----------------------------------------------------------------------*
 * Fill routines
 *----------------------------------------------------------------------*/

HG_EXTERN_C void hgFill8  (HGuint8*,    HGuint8,  int);
HG_EXTERN_C void hgFill16 (HGuint16*,   HGuint16, int);
HG_EXTERN_C void hgFill32 (HGuint32*,   HGuint32, int);

/*----------------------------------------------------------------------*/
#endif /* __HGFILL_H */



