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

#ifndef __ARMVFPDEFINES_H
#define __ARMVFPDEFINES_H

#include "hgdefs.h"


typedef enum
{
    FMAC = 0x0, 
    FNMAC = 0x1,
    FMSC = 0x2, 
    FMNMSC = 0x3,
    FMUL = 0x4,
    FNMUL = 0x5,
    FADD = 0x6, 
    FSUB = 0x7, 
    FDIV = 0x8, 
    EXT = 0xf   /*exension*/
} VFPDataProcessingOpcode;

typedef enum
{
    FCPY   = 0x00,  
    FABS   = 0x01,
    FNEG   = 0x02,  
    FSQRT  = 0x03,
    FCMP   = 0x08,
    FCMPE  = 0x09,
    FCMPZ  = 0x0A,
    FCMPEZ = 0x0B,  
    FCVT   = 0x0F,  
    FUITO  = 0x10,  
    FSITO  = 0x11,  
    FTOUI  = 0x18,
    FTOUIZ = 0x19,  
    FTOSI  = 0x1A,  
    FTOSIZ = 0x1B
} VFPDataProcessingExtOpcode;


typedef enum
{
    Unindexed = 0x02,   
    Increment = 0x03,
    NegOffset = 0x04,   
    Decrement = 0x05,
    PosOffset = 0x06
} VFPLoadAndStoreAdressingMode;


typedef enum
{
    FMSR   = 0x00,  
    FMRS   = 0x01,
    FMXR   = 0x0E,  
    FMRX   = 0x0F
} VFPSingleRegisterTransferOpcode;

typedef enum
{
    FMDLR   = 0x00, 
    FMRDL   = 0x01,
    FMDHR   = 0x02, 
    FMRDH   = 0x03
} VFPDoubleRegisterTransferOpcode;

typedef enum
{
    FPSID   = 0x0,  
    FPSCR   = 0x2,
    FPEXC   = 0x8
} VFPSystemRegisterEncoding;


#endif /*__ARMVFPDEFINES_H*/
