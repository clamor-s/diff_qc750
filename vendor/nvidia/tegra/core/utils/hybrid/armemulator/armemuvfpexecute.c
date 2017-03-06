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
#include "hgint32.h"

#if USE_VFP

#include <math.h>
#ifndef __ARMVFPDEFINES_H
#   include "armvfpdefines.h"
#endif
#ifndef __ARMDEFINES_H
#   include "armdefines.h"
#endif
#ifndef __HGINT64_H
#   include "hgint64.h"
#endif


#define AS_FLOAT(f) (* ((float*) (&(f))))
#define AS_DOUBLE(f) (* ((double*) (&(f))))


/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

HGbool ARMEmulator_executeVFPDataProcessing(ARMEmulator* emu, unsigned int instruction )
{
    unsigned int opcode = ( ((instruction >> 20 ) & 8 ) |  /*p */
                            ((instruction >> 19 ) & 4 ) |  /*q */
                            ((instruction >> 19 ) & 2 ) |  /*r */
                            ((instruction >> 6 ) & 1 ) ); /*s */
    unsigned int fn =( ((instruction >> 15 ) & 0x1e ) |  /*Fn */
                       ((instruction >> 7  ) & 1 ) );    /*N */
    unsigned int fd =( ((instruction >> 11 ) & 0x1e ) |  /*Fd */
                       ((instruction >> 22 ) & 1 ) );    /*D */
    unsigned int fm =( ((instruction << 1  ) & 0x1e ) |  /*Fm */
                       ((instruction >> 5  ) & 1 ) );    /*M */
    unsigned int cp_num =( (instruction >> 8  ) & 0x0f );  /*cp_num */
    unsigned int len = ((emu->m_FPSCR >> 16) & 0x07) + 1;
    unsigned int stride = ((emu->m_FPSCR >> 20) & 0x03) + 1;

    /*cp_num =10, single precission, cp_num=11 double precision, else undefined */
    HG_ASSERT( (cp_num==10) || (cp_num==11) );  
    
    /* opimization in case of scalar */
    if (fd<8)
    {
        len = 1;
    }

    while (len>0)
    {
        if (cp_num==10)
        {
            float* fdp = (float*) &(emu->m_vfpRegisterFile[fd]);
            float* fnp = (float*) &(emu->m_vfpRegisterFile[fn]);
            float* fmp = (float*) &(emu->m_vfpRegisterFile[fm]);

            switch( opcode )
            {
            case FMAC:  (*fdp) = (*fdp) + ( (*fnp) * (*fmp) );
                break;
            case FNMAC: (*fdp) = (*fdp) - ( (*fnp) * (*fmp) );
                break;
            case FMSC:  (*fdp) = - (*fdp) + ( (*fnp) * (*fmp) );
                break;
            case FMNMSC: (*fdp) = - (*fdp) - ( (*fnp) * (*fmp) );
                break;
            case FMUL:  (*fdp) = ( (*fnp) * (*fmp) );
                break;
            case FNMUL: (*fdp) = -( (*fnp) * (*fmp) );
                break;
            case FADD: (*fdp) = ( (*fnp) + (*fmp) );
                break;
            case FSUB: (*fdp) = ( (*fnp) - (*fmp) );
                break;
            case FDIV: /* todo handle this exception case correctly, e.g. according to controlword flags */
                    (*fdp) = ( ((*fmp)==0.0f)||((*fmp)==-0.0f)?(*fmp):(*fnp) / (*fmp) );
                break;
            case EXT:
                switch( fn )
                {
                case FCPY:  (*fdp) = *fmp;
                    break;
                case FABS:  (*fdp) = (float)fabs(*fmp);
                    break;
                case FNEG:  (*fdp) = -(*fmp);
                    break;
                case FSQRT:  (*fdp) = (float)sqrt(*fmp);
                    break;
                case FCMP:  
                case FCMPE:  
                        emu->m_FPSCR = (emu->m_FPSCR & 0x0fffffffu) | 
                            ( ((*fdp) < (*fmp)) ? FlagN : FlagC) |
                            ( ((*fdp) == (*fmp)) ? FlagZ : 0) ;
                    break;
                case FCMPZ:  
                case FCMPEZ:  
                        emu->m_FPSCR = (emu->m_FPSCR & 0x0fffffffu) | 
                            ( ((*fdp) < 0.0f) ? FlagN : FlagC) |
                            ( ((*fdp) == 0.0f) ? FlagZ : 0) ;
                    break;
                case FCVT:  (* ((double*)fdp)) = (*fmp);
                        HG_ASSERT((fd & 1) ==0);
                    break;
                case FUITO:  (*fdp) = (float) *((unsigned int*)fmp);
                    break;
                case FSITO:  (*fdp) = (float) *((int*)fmp);
                    break;
                case FTOUI:  (*((unsigned int*)fdp)) = (unsigned int) ((*fmp) + ((*fmp)>0.0f ? 0.5f : -0.5f));
                    break;
                case FTOUIZ: (*((unsigned int*)fdp)) = (unsigned int) (*fmp);
                    break;
                case FTOSI:  (*((int*)fdp)) = (int) ((*fmp)+ ((*fmp)>0.0f ? 0.5f : -0.5f));
                    break;
                case FTOSIZ:  (*((int*)fdp)) = (int) (*fmp);
                    break;
                default:            
                    HG_ASSERT(HG_FALSE);    /*undefined*/
                    break;
                }
                break;
            default:
                HG_ASSERT(HG_FALSE);    /*just in case */
                break;
            }       
        }
        else if (cp_num==11)
        {
            double* fdp = (double*) &(emu->m_vfpRegisterFile[fd]);
            double* fnp = (double*) &(emu->m_vfpRegisterFile[fn]);
            double* fmp = (double*) &(emu->m_vfpRegisterFile[fm]);
            HG_ASSERT( (fm & 1)==0 );
            HG_ASSERT( (opcode!=FCVT)|| ((fd & 1)==0) );
            HG_ASSERT( (opcode!=EXT) || ((fn & 1)==0) );

            switch( opcode )
            {
            case FMAC:  (*fdp) = (*fdp) + ( (*fnp) * (*fmp) );
                break;
            case FNMAC: (*fdp) = (*fdp) - ( (*fnp) * (*fmp) );
                break;
            case FMSC:  (*fdp) = - (*fdp) + ( (*fnp) * (*fmp) );
                break;
            case FMNMSC: (*fdp) = - (*fdp) - ( (*fnp) * (*fmp) );
                break;
            case FMUL:  (*fdp) = ( (*fnp) * (*fmp) );
                break;
            case FNMUL: (*fdp) = -( (*fnp) * (*fmp) );
                break;
            case FADD: (*fdp) = ( (*fnp) + (*fmp) );
                break;
            case FSUB: (*fdp) = ( (*fnp) - (*fmp) );
                break;
            case FDIV: (*fdp) = ( (*fnp) / (*fmp) );
                break;
            case EXT:
                switch( fn )
                {
                case FCPY:  (*fdp) = *fmp;
                    break;
                case FABS:  (*fdp) = fabs(*fmp);
                    break;
                case FNEG:  (*fdp) = -(*fmp);
                    break;
                case FSQRT:  (*fdp) = sqrt(*fmp);
                    break;
                case FCMP:  
                case FCMPE:  
                        emu->m_FPSCR = (emu->m_FPSCR & 0x0fffffffu) | 
                            ( ((*fdp) < (*fmp)) ? FlagN : FlagC) |
                            ( ((*fdp) == (*fmp)) ? FlagZ : 0) ;
                    break;
                case FCMPZ:  
                case FCMPEZ:  
                        emu->m_FPSCR = (emu->m_FPSCR & 0x0fffffffu) | 
                            ( ((*fdp) < 0.0) ? FlagN : FlagC) |
                            ( ((*fdp) == 0.0) ? FlagZ : 0) ;
                    break;
                case FCVT:  (* ((float*)fdp)) = (float) (*fmp);
                    break;
                case FUITO:  (*fdp) = *((unsigned int*)fmp);
                    break;
                case FSITO:  (*fdp) = *((int*)fmp);
                    break;
                case FTOUI:  (*((unsigned int*)fdp)) = (unsigned int) ((*fmp) + ((*fmp)>0.0 ? 0.5 : -0.5));
                    break;
                case FTOUIZ: (*((unsigned int*)fdp)) = (unsigned int) (*fmp);
                    break;
                case FTOSI:  (*((int*)fdp)) = (int) ((*fmp)+ ((*fmp)>0.0 ? 0.5 : -0.5));
                    break;
                case FTOSIZ:  (*((int*)fdp)) = (int) (*fmp);
                    break;
                default:            
                    HG_ASSERT(HG_FALSE);    /*undefined*/
                    break;
                }
                break;
            default:
                HG_ASSERT(HG_FALSE);    /*just in case */
                break;
            }   
        }

        len --;
        if ((len>0) && (fd>7))
        {
            fd = (fd & 0xf8 ) |  ( (fd + stride ) & 0x7); 
            if (opcode<EXT)
            {
                fn = (fn & 0xf8 ) |  ( (fn + stride ) & 0x7); 
            }
            if (fm>7)
            {
                fm = (fm & 0xf8 ) |  ( (fm + stride ) & 0x7);
            }
        }

    }
    return HG_FALSE;
}

/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

HGbool ARMEmulator_executeVFPLoadAndStore(ARMEmulator* emu, unsigned int instruction )
{
    unsigned int i;
    unsigned int adrMode = ( ((instruction >> 22 ) & 4 ) |  /*p */
                            ((instruction >> 22 ) & 2 ) |  /*u */
                            ((instruction >> 21 ) & 1 ) ); /*w */
    unsigned int l = ((instruction >> 20 ) & 1 ); /*l */
    unsigned int offset =( (instruction >> 0 ) & 0xff ); /*offset */
    unsigned int rn =( (instruction >> 16 ) & 0x0f ); /*Rn */
    unsigned int fd =( ((instruction >> 11 ) & 0x1e ) |  /*Fd */
                       ((instruction >> 22 ) & 1 ) );    /*D */
    unsigned int cp_num =( (instruction >> 8  ) & 0x0f );  /*cp_num */
    /*cp_num =10, single precission, cp_num=11 double precision, else undefined */
    HG_ASSERT( (cp_num==10) || (cp_num==11) );

    if (l==0)
    {
        switch( adrMode )
        {
        case Unindexed: 
            for (i=0;i<offset;i++)
            {
                ARMEmulator_storeWord(emu, emu->m_vfpRegisterFile[fd+i], emu->m_registerFile[rn] + i*4 );
            }
            break;
        case Increment: 
            for (i=0;i<offset;i++)
            {
                ARMEmulator_storeWord(emu, emu->m_vfpRegisterFile[fd+i], emu->m_registerFile[rn] + i*4 );
            }
            emu->m_registerFile[rn]+= offset*4;
            break;
        case Decrement: 
            for (i=0;i<offset;i++)
            {
                ARMEmulator_storeWord(emu, emu->m_vfpRegisterFile[fd+i], emu->m_registerFile[rn] -(offset*4) + i*4 );
            }
            emu->m_registerFile[rn]-= offset*4;
            break;
        case PosOffset: 
            ARMEmulator_storeWord(emu, emu->m_vfpRegisterFile[fd], emu->m_registerFile[rn] + offset*4);
            if (cp_num==11)
            {
                ARMEmulator_storeWord(emu, emu->m_vfpRegisterFile[fd+1], emu->m_registerFile[rn] + offset*4 +4);
            }
            break;
        case NegOffset: 
            if (cp_num==10)
            {
                ARMEmulator_storeWord(emu, emu->m_vfpRegisterFile[fd], emu->m_registerFile[rn] - offset*4);
            }
            else
            {
                ARMEmulator_storeWord(emu, emu->m_vfpRegisterFile[fd+1], emu->m_registerFile[rn] - offset*4);
                ARMEmulator_storeWord(emu, emu->m_vfpRegisterFile[fd], emu->m_registerFile[rn] - offset*4-4);
            }
            break;

        default:            
            HG_ASSERT(HG_FALSE);    /*undefined*/
            break;
        }   
    }
    else /* if (l==1) */
    {
        switch( adrMode )
        {
        case Unindexed: 
            for (i=0;i<offset;i++)
            {
                emu->m_vfpRegisterFile[fd+i]=ARMEmulator_loadWord(emu, emu->m_registerFile[rn] + i*4 );
            }
            break;
        case Increment: 
            for (i=0;i<offset;i++)
            {
                emu->m_vfpRegisterFile[fd+i]=ARMEmulator_loadWord(emu, emu->m_registerFile[rn] + i*4 );
            }
            emu->m_registerFile[rn]+= offset*4;
            break;
        case Decrement: 
            for (i=0;i<offset;i++)
            {
                emu->m_vfpRegisterFile[fd+i]=ARMEmulator_loadWord(emu, emu->m_registerFile[rn] -(offset*4) + i*4 );
            }
            emu->m_registerFile[rn]-= offset*4;
            break;
        case PosOffset: 
            emu->m_vfpRegisterFile[fd]=ARMEmulator_loadWord(emu, emu->m_registerFile[rn] + offset*4);
            if (cp_num==11)
            {
                emu->m_vfpRegisterFile[fd+1]=ARMEmulator_loadWord(emu, emu->m_registerFile[rn] + offset*4 + 4);
            }
            break;
        case NegOffset: 
            if (cp_num==10)
            {
                emu->m_vfpRegisterFile[fd]=ARMEmulator_loadWord(emu, emu->m_registerFile[rn] - offset*4);
            }
            else
            {
                emu->m_vfpRegisterFile[fd+1]=ARMEmulator_loadWord(emu, emu->m_registerFile[rn] - offset*4);
                emu->m_vfpRegisterFile[fd]=ARMEmulator_loadWord(emu, emu->m_registerFile[rn] - offset*4 - 4);
            }
            break;

        default:            
            HG_ASSERT(HG_FALSE);    /*undefined*/
            break;
        }   
    }

    return HG_FALSE;
}


/*-------------------------------------------------------------------*//*!
 * \brief   
 * \param   
 * \return  
 * \note        
 *//*-------------------------------------------------------------------*/

HGbool ARMEmulator_executeVFPRegisterTransfer(ARMEmulator* emu, unsigned int instruction )
{
    unsigned int opcode = ((instruction >> 20 ) & 0x0f );
    unsigned int fn =( ((instruction >> 15 ) & 0x1e ) |  /*Fn */
                       ((instruction >> 7  ) & 1 ) );    /*N */

    unsigned int rd =( (instruction >> 12 ) & 0x0f ); /*Rd */
    unsigned int cp_num =( (instruction >> 8  ) & 0x0f );  /*cp_num */
    /*cp_num =10, single precission, cp_num=11 double precision, else undefined */
    HG_ASSERT( (cp_num==10) || (cp_num==11) );

    if (cp_num==10)
    {
        switch( opcode )
        {
        case FMSR: emu->m_vfpRegisterFile[fn] = emu->m_registerFile[rd];
            break;
        case FMRS: emu->m_registerFile[rd] = emu->m_vfpRegisterFile[fn];
            break;
        case FMXR: 
                switch( fn )
                {
                case FPSCR: emu->m_FPSCR = emu->m_registerFile[rd];
                    break;
                case FPSID: emu->m_FPSID = emu->m_registerFile[rd];
                    break;
                case FPEXC: emu->m_FPEXC = emu->m_registerFile[rd];
                    break;
                default: 
                    HG_ASSERT(HG_FALSE);    /*undefined*/
                    break;
                }
            break;
        case FMRX: 

            if ((rd==15) && (fn == FPSCR)) /* special case FSTAT */
            {
                emu->m_CPSR = ( emu->m_CPSR & 0x0fffffff) | ( emu->m_FPSCR & 0xf0000000);
            }
            else
            {
                switch( fn )
                {
                case FPSCR: emu->m_registerFile[rd] = emu->m_FPSCR;
                    break;
                case FPSID: emu->m_registerFile[rd] = emu->m_FPSID;
                    break;
                case FPEXC: emu->m_registerFile[rd] = emu->m_FPEXC;
                    break;
                default: 
                    HG_ASSERT(HG_FALSE);    /*undefined*/
                    break;
                }
            }
            break;
            
        default:            
            HG_ASSERT(HG_FALSE);    /*undefined*/
            break;
        }
    }
    else if (cp_num==11)
    {
        switch( opcode )
        {
        case FMDLR: *((HGuint64*) &emu->m_vfpRegisterFile[fn]) = hgSet64u(
                            hgGet64uh(* ((HGuint64*) &emu->m_vfpRegisterFile[fn])), 
                            emu->m_registerFile[rd] );          
            break;
        case FMRDL: emu->m_registerFile[rd] = hgGet64ul(* ((HGuint64*) &emu->m_vfpRegisterFile[fn]));
            break;
        case FMDHR: *((HGuint64*) &emu->m_vfpRegisterFile[fn]) = hgSet64u(
                            emu->m_registerFile[rd], 
                            hgGet64ul(* ((HGuint64*) &emu->m_vfpRegisterFile[fn])) );           
            break;
        case FMRDH: emu->m_registerFile[rd] = hgGet64uh(* ((HGuint64*) &emu->m_vfpRegisterFile[fn]));
            break;
        default:            
            HG_ASSERT(HG_FALSE);    /*undefined*/
            break;
        }       
    }

    return HG_FALSE;
}


#endif /* USE_VFP */
