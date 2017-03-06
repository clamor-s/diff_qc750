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

#ifndef __ARMDEFINES_H
#define __ARMDEFINES_H

#include "hgdefs.h"

#ifdef HG_DEBUG
#   define ARM_DEBUG_CODE(x) x
#else
#   define ARM_DEBUG_CODE(x)
#endif

#define FlagN   0x80000000u /*negative/less than*/
#define FlagZ   0x40000000u /*zero*/
#define FlagC   0x20000000u /*carry/borrow/extend*/
#define FlagV   0x10000000u /*overflow*/
/*#define FlagQ ??? */      /*sticky carry*/

typedef enum
{
    EQ = 0x0,   /*Z set (equal)*/
    NE = 0x1,   /*Z clear (not equal)*/
    CS = 0x2,   /*C set (unsigned higher or same)*/
    CC = 0x3,   /*C clear (unsigned lower)*/
    MI = 0x4,   /*N set (negative)*/
    PL = 0x5,   /*N clear (positive or zero)*/
    VS = 0x6,   /*V set (overflow)*/
    VC = 0x7,   /*V clear (no overflow)*/
    HI = 0x8,   /*C set and Z clear (unsigned higher)*/
    LS = 0x9,   /*C clear or Z set (unsigned lower or same)*/
    GE = 0xa,   /*N set and V set, or N clear and V clear (greater or equal)*/
    LT = 0xb,   /*N set and V clear, or N clear and V set (less than)*/
    GT = 0xc,   /*Z clear, and either N set and V set, or N clear and V clear (greater than)*/
    LE = 0xd,   /*Z set, or N set and V clear, or N clear and V set (less than or equal)*/
    AL = 0xe,   /*always*/
    NV = 0xf,   /*never*/

    ConditionCodeMask = 0xf,
    ConditionCodeShift = 28
} ConditionCode;

typedef enum
{
    AND = 0x0,  /*operand1 AND operand2*/
    EOR = 0x1,  /*operand1 EOR operand2*/
    SUB = 0x2,  /*operand1 - operand2*/
    RSB = 0x3,  /*operand2 - operand1*/
    ADD = 0x4,  /*operand1 + operand2*/
    ADC = 0x5,  /*operand1 + operand2 + carry*/
    SBC = 0x6,  /*operand1 - operand2 + carry - 1*/
    RSC = 0x7,  /*operand2 - operand1 + carry - 1*/
    TST = 0x8,  /*as AND, but result is not written*/
    TEQ = 0x9,  /*as EOR, but result is not written*/
    CMP = 0xa,  /*as SUB, but result is not written*/
    CMN = 0xb,  /*as ADD, but result is not written*/
    ORR = 0xc,  /*operand1 OR operand2*/
    MOV = 0xd,  /*operand2 (operand1 is ignored)*/
    BIC = 0xe,  /*operand1 AND NOT operand2 (Bit clear)*/
    MVN = 0xf   /*NOT operand2 (operand1 is ignored)*/
} DataProcessingOpcode;

typedef enum
{
    LSL = 0x0,
    LSR = 0x1,
    ASR = 0x2,
    ROR = 0x3
} ShiftType;

#endif
