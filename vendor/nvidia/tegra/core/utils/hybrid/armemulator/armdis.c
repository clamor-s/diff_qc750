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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void* ARMDisassembler_create(ARMDisassembler_processor p, ARMDisassembler_endianess e, HGbool hex)
{
    ARMDisassembler* dis = (ARMDisassembler*)malloc(sizeof(ARMDisassembler));
    memset(dis, 0, sizeof(ARMDisassembler));

    dis->m_prefetch = 8;
    dis->m_processor = p;
    dis->m_endianess = e;
    dis->m_index = 0;
    dis->m_noteIndex = 0;
    dis->m_hex = hex;
    return dis;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_destroy(void* dis)
{
    HG_ASSERT(dis);
    free(((ARMDisassembler*)dis));
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

HGbool ARMDisassembler_isBranch(void* disassembler, unsigned int instruction)
{
    const unsigned int InstructionTypeMask = 0x0c000000;
    const unsigned int InstructionTypeShift = 26;
    unsigned int instructionType = (instruction & InstructionTypeMask) >> InstructionTypeShift;
    HG_UNREF(disassembler);
    if(instructionType == 0x2 && instruction & 0x02000000)
        return HG_TRUE;
    return HG_FALSE;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

int ARMDisassembler_getBranchOffset(void* disassembler, unsigned int instruction)
{
    const unsigned int OffsetMask = 0x00ffffff;
    int offset;
    HG_ASSERT(ARMDisassembler_isBranch(disassembler, instruction));
    offset = signExtend((instruction & OffsetMask)<<2, 23); /*offset is 23 bits + sign bit */
    return offset + ((ARMDisassembler*)disassembler)->m_prefetch;   /*addjust the offset to take the instruction prefetch into account */
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

const char* ARMDisassembler_decode(void* disassembler, const char** notes, unsigned int instruction)
{
    const unsigned int InstructionTypeMask = 0x0c000000;
    const unsigned int InstructionTypeShift = 26;

    const unsigned int MultiplyMask = 0x0fc000f0;       /*0000 1111 1100 0000 0000 0000 1111 0000*/
    const unsigned int MultiplyOpcode = 0x00000090;     /*0000 0000 0000 0000 0000 0000 1001 0000*/
    const unsigned int MultiplyLongMask = 0x0f8000f0;   /*0000 1111 1000 0000 0000 0000 1111 0000*/
    const unsigned int MultiplyLongOpcode = 0x00800090; /*0000 0000 1000 0000 0000 0000 1001 0000*/
    const unsigned int ClzMask = 0x0ff000f0;            /*0000 1111 1111 0000 0000 0000 1111 0000 */
    const unsigned int ClzOpcode = 0x01600010;          /*0000 0001 0110 0000 0000 0000 0001 0000 */
    const unsigned int SwapMask = 0x0fb00ff0;           /*0000 1111 1011 0000 0000 1111 1111 0000*/
    const unsigned int SwapOpcode = 0x01000090;         /*0000 0001 0000 0000 0000 0000 1001 0000*/
    /*NOTE: these match also the swap instruction. therefore these must be checked AFTER checking swap!*/
    const unsigned int HalfwordAndSignedDataMask = 0x0e000090;      /*0000 1110 0000 0000 0000 1111 1001 0000*/
    const unsigned int HalfwordAndSignedDataOpcode = 0x00000090;    /*0000 0000 0000 0000 0000 0000 1001 0000*/

    unsigned int instructionType = (instruction & InstructionTypeMask) >> InstructionTypeShift;
    ARMDisassembler* dis = (ARMDisassembler*)disassembler;

    HG_ASSERT(dis);
    dis->m_index = 0;
    dis->m_noteIndex = 0;
    dis->m_string[0] = '\0';
    dis->m_notes[0] = '\0';

    switch( instructionType )
    {
    case 0x0:   /*data processing, psr transfer, multiply or single data swap*/
        if( (instruction & MultiplyMask) == MultiplyOpcode )
            ARMDisassembler_executeMultiply(dis, instruction);
        else if( (instruction & MultiplyLongMask) == MultiplyLongOpcode )
            ARMDisassembler_executeMultiplyLong(dis, instruction);
        else if( (instruction & ClzMask) == ClzOpcode )
            ARMDisassembler_executeClz(dis, instruction);
        else if( (instruction & SwapMask) == SwapOpcode )
            ARMDisassembler_executeSingleDataSwap(dis, instruction);
        else if( (instruction & HalfwordAndSignedDataMask) == HalfwordAndSignedDataOpcode )
            ARMDisassembler_executeHalfwordAndSignedDataTransfer(dis, instruction);
        else
        {   /*data processing or psr transfer*/
            unsigned int opcode = instruction & 0x01900000; /*extract bits 20(S),23 and 24*/
            /*psr transfer instructions are encoded as TST/TEQ/CMP/CMN with S-bit clear*/
            if( opcode == 0x01000000 )
                ARMDisassembler_executePSRTransfer(dis, instruction);
            else
                ARMDisassembler_executeDataProcessing(dis, instruction);
        }
        break;
    case 0x1:   /*single data transfer or undefined*/
        /*UndefinedMask   = 0000 1110 0000 0000 0000 0000 0001 0000*/
        /*UndefinedOpcode = 0000 0110 0000 0000 0000 0000 0001 0000*/
        if( (instruction & 0x0e000010) == 0x06000010 )
            ARMDisassembler_append(dis, "undefined instruction (not emulated)");
        else
            ARMDisassembler_executeSingleDataTransfer(dis, instruction);
        break;
    case 0x2:   /*block data transfer or branch*/
        if( (instruction & 0x02000000) )
            ARMDisassembler_executeBranch(dis, instruction);    /*no advance to the next instruction*/
        else
            ARMDisassembler_executeBlockDataTransfer(dis, instruction);
        break;
    case 0x3:   /*coprocessor or software interrupt*/
        ARMDisassembler_append(dis, "coprocessor or SWI (not emulated)");
        break;
    default:
        HG_ASSERT(0);
        break;
    };
    if(notes)
        *notes = dis->m_notes;
    return dis->m_string;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

unsigned int ARMDisassembler_instructionFetch(ARMDisassembler* dis, unsigned int address)
{
    HG_ASSERT(!(address & 3));  /*check that the instruction address is properly aligned*/
    return ARMDisassembler_loadWord(dis, address);
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

void ARMDisassembler_disassemble(ARMDisassembler_processor p, ARMDisassembler_endianess e, HGbool hexImmediates, const unsigned int* addr, int numInstructions, const char** comments, int* commentOffsets, int numComments)
{
#define MAXLABELS 100
    void* dis = ARMDisassembler_create(p, e, hexImmediates);

    int labels[MAXLABELS];
    int numLabels = 0, i;
    int commentNdx = 0;
    /* add label addresses into label table */
    for(i=0;i<numInstructions;i++)
    {
        unsigned int instruction = addr[i];
        if(ARMDisassembler_isBranch(dis, instruction))
            labels[numLabels++] = ARMDisassembler_getBranchOffset(dis, instruction) + i*4;
        HG_ASSERT(numLabels <= MAXLABELS);
    }

    for(i=0;i<numInstructions;i++)
    {
        int j;

        /* print label */
        for(j=0;j<numLabels;j++)
        {
            if(i*4 == labels[j])
            {
                printf(".label%d:\n",j);
                break;
            }
        }

        /* print comments */
        while(comments && commentOffsets && commentNdx < numComments && commentOffsets[commentNdx] == i*4)
        {
            printf("  ;%s\n", comments[commentNdx]);
            commentNdx++;
        }

        /* print instruction */
        {
            unsigned int instruction = addr[i];
            const char* notes;
            printf("\t%s", ARMDisassembler_decode(dis, &notes, instruction));

            /* print branch instruction label */
            if(ARMDisassembler_isBranch(dis, instruction))
            {
                int offset = ARMDisassembler_getBranchOffset(dis, instruction) + i*4;
                int j;
                for(j=0;j<numLabels;j++)
                {
                    if(offset == labels[j])
                    {
                        printf(".label%d",j);
                        break;
                    }
                }
            }

            /* print instruction notes */
            printf("\t\t%s\n",notes);
        }
    }

    fflush(stdout);
    ARMDisassembler_destroy(dis);
#undef MAXLABELS
}
