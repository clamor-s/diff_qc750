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
 * Description:         Source code for hgInt64 library
 *
 * $Id: //hybrid/libs/hg/main/hgInt64.c#1 $
 * $Date: 2005/11/08 $
 * $Author: janne $ *
 *----------------------------------------------------------------------*/

#include "hgint64.h"

#if (HG_COMPILER == HG_COMPILER_MSC)
#       pragma warning(disable:4514)
#endif
/*
HG_INLINE int testos (int x)
{
        int y,z;

        __asm {
                mov   r2, x
                mov   r0, 0       
                adds  r1, r2,7     
                orrne r0, r0, r1    
                adds  r1, r2,7     
                orrne r0, r0, r1    
                adds  r1, r2,7     
                orrne r0, r0, r1    
                adds  r1, r2,7     
                orrne r0, r0, r1    
                adds  r1, r2,7     
                orrne r0, r0, r1    
                adds  r1, r2,7     
                orrne r0, r0, r1    
                adds  r1, r2,7     
                orrne r0, r0, r1    
                adds  r1, r2,7     
                orrne r0, r0, r1    
                adds  r1, r2,7     
                orrne r0, r0, r1    
                mov             z,r0
        }

        return z;
}

int seepra (int z)
{
        int x = 3;
        if (z < 5) x += testos(z); else x += testos(z^0xdeadbabe);
        return x;
}
*/



/*----------------------------------------------------------------------*/

