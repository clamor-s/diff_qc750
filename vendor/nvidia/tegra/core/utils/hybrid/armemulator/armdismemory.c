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

unsigned int ARMDisassembler_loadWord(ARMDisassembler* dis, unsigned int address)
{
    unsigned int data = 0;
    if(dis->m_endianess == ARMDISASSEMBLER_LITTLEENDIAN)
    {
        /*TODO take into account nonaligned addresses (page 37 of ARM7vC.pdf)*/
        HG_ASSERT(!(address & 3));  /*address must be word aligned*/
        data |= (unsigned int)*((unsigned char *)(address+3));
        data <<= 8;
        data |= (unsigned int)*((unsigned char *)(address+2));
        data <<= 8;
        data |= (unsigned int)*((unsigned char *)(address+1));
        data <<= 8;
        data |= (unsigned int)*((unsigned char *)(address+0));
    }
    else
    {   /*big endian*/
        /*TODO take into account nonaligned addresses (page 38 of ARM7vC.pdf)*/
        HG_ASSERT(!(address & 3));  /*address must be word aligned*/
        data |= (unsigned int)*((unsigned char *)(address+0));
        data <<= 8;
        data |= (unsigned int)*((unsigned char *)(address+1));
        data <<= 8;
        data |= (unsigned int)*((unsigned char *)(address+2));
        data <<= 8;
        data |= (unsigned int)*((unsigned char *)(address+3));
    }
    return data;
}
