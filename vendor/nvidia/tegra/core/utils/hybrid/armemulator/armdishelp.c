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
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


#ifdef _WIN32
//#pragma warning( disable : 4996 )
#endif


/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_append(ARMDisassembler* dis, const char *format, ...)
{
    static char string[1024];
    int l;

    va_list arg;
    va_start( arg, format );
    vsprintf( string, format, arg );
    va_end( arg );

    l = strlen(string);
    HG_ASSERT(dis->m_index + l < 255);
    strcpy(&dis->m_string[dis->m_index],string);
    dis->m_index += l;
    dis->m_string[dis->m_index] = '\0';
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_appendTab(ARMDisassembler* dis)
{
    int tab = 8 - dis->m_index;
    int i;
    for(i=0;i<tab;i++)
    {
        dis->m_string[dis->m_index++] = ' ';
    }
    dis->m_string[dis->m_index] = '\0';
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_appendNote(ARMDisassembler* dis, const char *format, ...)
{
    static char string[1024];
    int l;

    va_list arg;
    va_start( arg, format );
    vsprintf( string, format, arg );
    va_end( arg );

    l = strlen(string);
    HG_ASSERT(dis->m_noteIndex + l < 2047);
    strcpy(&dis->m_notes[dis->m_noteIndex],string);
    dis->m_noteIndex += l;
    dis->m_string[dis->m_noteIndex] = '\0';
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_writeConditionCode(ARMDisassembler* dis, unsigned int instruction)
{
    unsigned int code = (instruction >> ConditionCodeShift) & ConditionCodeMask;
    switch( code )
    {
    case EQ: ARMDisassembler_append(dis, "eq"); return;
    case NE: ARMDisassembler_append(dis, "ne"); return;
    case CS: ARMDisassembler_append(dis, "cs"); return;
    case CC: ARMDisassembler_append(dis, "cc"); return;
    case MI: ARMDisassembler_append(dis, "mi"); return;
    case PL: ARMDisassembler_append(dis, "pl"); return;
    case VS: ARMDisassembler_append(dis, "vs"); return;
    case VC: ARMDisassembler_append(dis, "vc"); return;
    case HI: ARMDisassembler_append(dis, "hi"); return;
    case LS: ARMDisassembler_append(dis, "ls"); return;
    case GE: ARMDisassembler_append(dis, "ge"); return;
    case LT: ARMDisassembler_append(dis, "lt"); return;
    case GT: ARMDisassembler_append(dis, "gt"); return;
    case LE: ARMDisassembler_append(dis, "le"); return;
    case AL: return;
    case NV: ARMDisassembler_append(dis, "nv"); ARMDisassembler_appendNote(dis, "condition code NV should never be used (ARM7)."); return;
    default:
        HG_ASSERT(0);   /*should never get here. if it does, there's a bug in the emulator*/
        break;
    }
    HG_ASSERT(0);   /*should never get here. if it does, there's a bug in the emulator*/
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_decodeOperand2Immediate(ARMDisassembler* dis, unsigned int instruction)
{
    const unsigned int ImmediateMask = 0xff;
    const unsigned int ShiftMask = 0xf00;
    const unsigned int ShiftShift = 8;

    unsigned int immediate = instruction & ImmediateMask;
    unsigned int shift = ((instruction & ShiftMask) >> ShiftShift);
    unsigned int operand2;
    shift <<= 1;    /*shift by twice the amount specified*/
    HG_ASSERT(shift <= 30);

    /*ROR #shift*/
    operand2 = (immediate >> shift) | (immediate << (32-shift));

    if( dis->m_hex )
        ARMDisassembler_append(dis, "#0x%08x",operand2);
    else
        ARMDisassembler_append(dis, "#%d",operand2);
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_decodeOperand2Register(ARMDisassembler* dis, unsigned int instruction)
{
    const unsigned int OperandMask = 0xf;
    const unsigned int ShiftIsRegisterMask = 0x10;
    const unsigned int ShiftTypeMask = 0x60;
    const unsigned int ShiftTypeShift = 5;
    unsigned int shiftType;

    int Rm = instruction & OperandMask;
    HG_ASSERT(Rm >= 0 && Rm < 16);  /*just in case*/
    ARMDisassembler_append(dis, "r%d",Rm);

    /*shift operand2*/
    shiftType = (instruction & ShiftTypeMask) >> ShiftTypeShift;
    if( instruction & ShiftIsRegisterMask )
    {   /*shift is specified by a register*/
        const unsigned int ShiftRegisterMask = 0xf00;
        const unsigned int ShiftRegisterShift = 8;
        int Rs = (instruction & ShiftRegisterMask) >> ShiftRegisterShift;
        HG_ASSERT(Rs >= 0 && Rs < 16);  /*just in case*/
        if( Rs == 15 )
            ARMDisassembler_appendNote(dis, "ERROR: shift register cannot be R15");
        if( instruction & 0x80 )
            ARMDisassembler_appendNote(dis, "ERROR: bit 7 of the instruction should be clear");

        switch(shiftType)
        {
        case LSL:
            ARMDisassembler_append(dis, ", lsl r%d",Rs);
            break;
        case LSR:
            ARMDisassembler_append(dis, ", lsr r%d",Rs);
            break;
        case ASR:
            ARMDisassembler_append(dis, ", asr r%d",Rs);
            break;
        case ROR:
            ARMDisassembler_append(dis, ", ror r%d",Rs);
            break;
        default:
            HG_ASSERT(0);
            break;
        }
    }
    else
    {   /*shift is specified by an immediate*/
        const unsigned int ShiftImmediateMask = 0xf80;
        const unsigned int ShiftImmediateShift = 7;
        unsigned int shift = (instruction & ShiftImmediateMask) >> ShiftImmediateShift;
        HG_ASSERT( shift < 32 );

        /*apply shift*/
        switch(shiftType)
        {
        case LSL:
            if( shift )
            {
                if( dis->m_hex )
                    ARMDisassembler_append(dis, ", lsl #0x%08x",shift);
                else
                    ARMDisassembler_append(dis, ", lsl #%d",shift);
            }
            break;
        case LSR:
            if( !shift )
                if( dis->m_hex )
                    ARMDisassembler_append(dis, ", lsr #0x00000020");   /*LSR #0 is used to denote LSR #32*/
                else
                    ARMDisassembler_append(dis, ", lsr #32");   /*LSR #0 is used to denote LSR #32*/
            else
                if( dis->m_hex )
                    ARMDisassembler_append(dis, ", lsr #0x%08x",shift);
                else
                    ARMDisassembler_append(dis, ", lsr #%d",shift);
            break;
        case ASR:
            if( !shift )
                if( dis->m_hex )
                    ARMDisassembler_append(dis, ", asr #0x00000020");   /*ASR #0 is used to denote ASR #32*/
                else
                    ARMDisassembler_append(dis, ", asr #32");   /*ASR #0 is used to denote ASR #32*/
            else
                if( dis->m_hex )
                    ARMDisassembler_append(dis, ", asr #0x%08x",shift);
                else
                    ARMDisassembler_append(dis, ", asr #%d",shift);
            break;
        case ROR:
            if( !shift )
                ARMDisassembler_append(dis, " RRX");    /*ROR #0 is used to denote RRX*/
            else
                if( dis->m_hex )
                    ARMDisassembler_append(dis, ", ror #0x%08x",shift);
                else
                    ARMDisassembler_append(dis, ", ror #%d",shift);
            break;
        default:
            HG_ASSERT(0);
            break;
        }
    }
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_decodeOperand2(ARMDisassembler* dis, unsigned int instruction)
{
    const unsigned int BitI = 0x02000000;   /*0=operand2 is a register, 1=operand2 is an immediate*/
    /*fetch operand2*/
    if( instruction & BitI )
        ARMDisassembler_decodeOperand2Immediate(dis, instruction);
    else
        ARMDisassembler_decodeOperand2Register(dis, instruction);
}
