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

unsigned int ARMEmulator_loadByte(ARMEmulator* emu, unsigned int address)
{
    ARMEmulator_assertAddressInRange(emu, address);
    return (unsigned int)*((unsigned char *)address);
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMEmulator_storeByte(ARMEmulator* emu, unsigned int data, unsigned int address )
{
    ARMEmulator_assertAddressInRange(emu, address);
    *((unsigned char *)address) = (unsigned char)(data & 0xff);
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

unsigned int ARMEmulator_loadHalfword(ARMEmulator* emu, unsigned int address)
{
    ARMEmulator_assertAddressInRange(emu, address);
    HG_ASSERT(!(address & 1));  /*address must be halfword aligned */
    if(emu->m_endianess == ARMEMULATOR_LITTLEENDIAN )
    {   /*little endian */
        unsigned int d = (unsigned int)*((unsigned char *)(address+1));
        d <<= 8;
        d |= (unsigned int)*((unsigned char *)(address+0));
        return d;
    }
    else
    {   /*big endian */
        unsigned int d = (unsigned int)*((unsigned char *)(address+0));
        d <<= 8;
        d |= (unsigned int)*((unsigned char *)(address+1));
        return d;
    }
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMEmulator_storeHalfword(ARMEmulator* emu, unsigned int data, unsigned int address )
{
    ARMEmulator_assertAddressInRange(emu, address);
    HG_ASSERT(!(address & 1));  /*address must be halfword aligned */
    if(emu->m_endianess == ARMEMULATOR_LITTLEENDIAN )
    {   /*little endian */
        *((unsigned char *)(address+0)) = (unsigned char)(data & 0xff);
        *((unsigned char *)(address+1)) = (unsigned char)((data & 0xff00) >> 8);
    }
    else
    {   /*big endian */
        *((unsigned char *)(address+1)) = (unsigned char)(data & 0xff);
        *((unsigned char *)(address+0)) = (unsigned char)((data & 0xff00) >> 8);
    }
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

unsigned int ARMEmulator_loadWord(ARMEmulator* emu, unsigned int address)
{
    unsigned int data = 0;
    ARMEmulator_assertAddressInRange(emu, address);
    if(emu->m_endianess == ARMEMULATOR_LITTLEENDIAN )
    {   /*little endian */
        /*TODO take into account nonaligned addresses (page 37 of ARM7vC.pdf) */
        HG_ASSERT(!(address & 3));  /*address must be word aligned */
        data |= (unsigned int)*((unsigned char *)(address+3));
        data <<= 8;
        data |= (unsigned int)*((unsigned char *)(address+2));
        data <<= 8;
        data |= (unsigned int)*((unsigned char *)(address+1));
        data <<= 8;
        data |= (unsigned int)*((unsigned char *)(address+0));
    }
    else
    {   /*big endian */
        /*TODO take into account nonaligned addresses (page 38 of ARM7vC.pdf) */
        HG_ASSERT(!(address & 3));  /*address must be word aligned */
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

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMEmulator_storeWord(ARMEmulator* emu, unsigned int data, unsigned int address)
{
    ARMEmulator_assertAddressInRange(emu, address);
    HG_ASSERT(!(address & 3));  /*address must be word aligned */
    if(emu->m_endianess == ARMEMULATOR_LITTLEENDIAN)
    {   /*little endian */
        *((unsigned char *)(address+0)) = (unsigned char)(data & 0xff);
        *((unsigned char *)(address+1)) = (unsigned char)((data & 0xff00) >> 8);
        *((unsigned char *)(address+2)) = (unsigned char)((data & 0xff0000) >> 16);
        *((unsigned char *)(address+3)) = (unsigned char)((data & 0xff000000) >> 24);
    }
    else
    {   /*big endian */
        *((unsigned char *)(address+3)) = (unsigned char)(data & 0xff);
        *((unsigned char *)(address+2)) = (unsigned char)((data & 0xff00) >> 8);
        *((unsigned char *)(address+1)) = (unsigned char)((data & 0xff0000) >> 16);
        *((unsigned char *)(address+0)) = (unsigned char)((data & 0xff000000) >> 24);
    }
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

#if 0
void ARMEmulator_addMemoryBlock( unsigned int address, unsigned int length )
{
    MemoryBlock m;
    m.m_address = address;
    m.m_length = length;
    std::pair<mbtype::iterator,HGbool> a = m_memoryBlocks.insert( m );
    HG_ASSERT(a.second);    /*check that insertion succeeds (ie. no duplicates) */
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMEmulator_removeMemoryBlock( unsigned int address )
{
    MemoryBlock m;
    m.m_address = address;
    m.m_length = 0; /*not needed */
    int a = m_memoryBlocks.erase( m );
    HG_ASSERT(a == 1);  /*check that exactly 1 element was removed */
}
#endif
/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMEmulator_assertAddressInRange(ARMEmulator* emu, unsigned int address)
{
    (void)emu;
    (void)address;
#if 0
    /*TODO it may be possible to speed this up by using find/lower_bound with some tricks */
    /*we need to find a memory block with address <= address to be checked */
    mbtype::iterator it = m_memoryBlocks.begin();
    while(it != m_memoryBlocks.end())
    {
        unsigned int a = (*it).m_address;
        unsigned int l = (*it).m_length;
        if( (address >= a) && (address < a + l) )
            return;
        it++;
    }
    HG_ASSERT(false);   /*INVALID ADDRESS */
#endif
}
