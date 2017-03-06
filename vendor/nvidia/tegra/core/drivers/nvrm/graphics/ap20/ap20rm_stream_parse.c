/*
 * Copyright (c) 2009-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvassert.h"
#include "nvos.h"
#include "nvutil.h"
#include "nvrm_stream.h"
#include "nvrm_rmctrace.h"
#include "ap20rm_channel.h"
#include "ap20rm_hwcontext.h"
#include "ap20/arhost1x.h"
#include "ap20/arhost1x_channel.h"
#include "ap20/arhost1x_sync.h"
#include "ap20/host1x_module_ids.h"
#include "ap20/project_arhost1x_sw_defs.h"

static NvRmRmcFile *s_rmc;

static void
NvRmChPrivParseDataWord( NvRmRmcParser *parser, NvU32 data )
{
    int firstBit = 0;

    if( parser->opcode == HCFMASK_OPCODE_MASK ||
        parser->opcode == HCFSETCL_OPCODE_SETCL )
    {
        /* handle mask funnyness */
        firstBit = NvULowestBitSet( parser->mask, 32 );
        parser->offset += firstBit;
    }

    switch( parser->opcode ) {
    case HCFINCR_OPCODE_INCR:
        NVRM_RMC_TRACE((s_rmc, "CommandWrite32 0x%x\n", data ));
        parser->offset++;
        break;

    case HCFNONINCR_OPCODE_NONINCR:
        NVRM_RMC_TRACE((s_rmc, "CommandWrite32 0x%x\n", data));
        break;

    case HCFSETCL_OPCODE_SETCL:
    case HCFMASK_OPCODE_MASK:
        NVRM_RMC_TRACE((s_rmc, "CommandWrite32 0x%x\n", data));
        parser->offset -= firstBit;
        parser->mask -= (1 << firstBit);
        break;

    case HCFGATHER_OPCODE_GATHER:
        NVRM_RMC_TRACE((s_rmc,"ADDRESS 0x%x # gpu ptr to data 0x%08x\n",
            data >> HCFGATHER_ADDRESS_SHIFT, data ));
        break;

    default:
        NV_ASSERT( !"NvRmChPrivParseDataWord: bad opcode" );
        return;
    }

    parser->data_count--;
}

void
NvRmChParseCmd( NvRmDeviceHandle hRm, NvRmRmcParser *parser, NvU32 data )
{
    NvU32 classid;
    NvU32 immdata;
    NvU32 tmp;

    (void)classid;
    (void)immdata;
    (void)tmp;

    if( !s_rmc )
    {
        NvRmGetRmcFile( hRm, &s_rmc );
        if( !s_rmc )
        {
            return;
        }
    }

    // FIXME: handle flush check/opcode boundary

    if( parser->data_count )
    {
        /* write data to rmc with name lookup */
        NvRmChPrivParseDataWord( parser, data );
        return;
    }

    /* parse the opcode and its arguments/data */
    parser->opcode = OP_DRF_VAL(HCFSETCL, OPCODE, data);
    switch( parser->opcode ) {
    case HCFSETCL_OPCODE_SETCL:

        parser->mask = OP_DRF_VAL(HCFSETCL, MASK, data);
        parser->offset = OP_DRF_VAL(HCFSETCL, OFFSET, data);
        classid = OP_DRF_VAL(HCFSETCL, CLASSID, data);

        NVRM_RMC_TRACE((s_rmc, "\n# SetClass to 0x%x\n", classid));
        NVRM_RMC_TRACE((s_rmc, "-HCFSETCL\n"));
        NVRM_RMC_TRACE((s_rmc, "CLASSID 0x%x\n", classid));
        NVRM_RMC_TRACE((s_rmc, "OFFSET 0x%x\n", parser->offset));
        NVRM_RMC_TRACE((s_rmc, "MASK 0x%x\n", parser->mask));

        tmp = parser->mask;
        while( tmp )
        {
            if( tmp & 1 )
            {
                parser->data_count++;
            }
            tmp >>= 1;
        }
        break;

    case HCFMASK_OPCODE_MASK:
        parser->mask = OP_DRF_VAL(HCFMASK, MASK, data);
        parser->offset = OP_DRF_VAL(HCFMASK, OFFSET, data);

        NVRM_RMC_TRACE((s_rmc, "\n-HCFMASK\n"));
        NVRM_RMC_TRACE((s_rmc, "OFFSET 0x%x\n", parser->offset));
        NVRM_RMC_TRACE((s_rmc, "MASK 0x%x\n", parser->mask));
        NVRM_RMC_TRACE((s_rmc, "CHANPROTECT 0x0\n"));

        tmp = parser->mask;
        while( tmp )
        {
            if( tmp & 1 )
            {
                parser->data_count++;
            }
            tmp >>= 1;
        }
        break;

    case HCFNONINCR_OPCODE_NONINCR:
        parser->offset = OP_DRF_VAL(HCFNONINCR, OFFSET, data);
        parser->data_count = OP_DRF_VAL(HCFNONINCR, COUNT, data);

        NVRM_RMC_TRACE((s_rmc, "\n-HCFNONINCR\n"));
        NVRM_RMC_TRACE((s_rmc, "OFFSET 0x%x\n", parser->offset ));
        NVRM_RMC_TRACE((s_rmc, "COUNT 0x%x\n", parser->data_count));
        NVRM_RMC_TRACE((s_rmc, "CHANPROTECT 0x0\n"));
        break;

    case HCFINCR_OPCODE_INCR:
        parser->data_count = OP_DRF_VAL(HCFINCR, COUNT, data);
        parser->offset = OP_DRF_VAL(HCFINCR, OFFSET, data);

        NVRM_RMC_TRACE((s_rmc, "\n-HCFINCR\n"));
        NVRM_RMC_TRACE((s_rmc, "OFFSET 0x%x\n", parser->offset));
        NVRM_RMC_TRACE((s_rmc, "COUNT 0x%x\n", parser->data_count));
        NVRM_RMC_TRACE((s_rmc, "CHANPROTECT 0x0\n"));
        break;

    case HCFIMM_OPCODE_IMM:
        parser->offset = OP_DRF_VAL(HCFIMM, OFFSET, data);
        immdata = OP_DRF_VAL(HCFIMM, IMMDATA, data);
        parser->data_count = 0;

        NVRM_RMC_TRACE((s_rmc, "\n-HCFIMM\n"));
        NVRM_RMC_TRACE((s_rmc, "OFFSET 0x%x\n", parser->offset));
        NVRM_RMC_TRACE((s_rmc, "IMMDATA 0x%04x\n", immdata));
        break;

    case HCFRESTART_OPCODE_RESTART:
        NVRM_RMC_TRACE((s_rmc,"\n-HCFRESTART\n\n"));
        parser->data_count = 0;
        break;

    case HCFGATHER_OPCODE_GATHER:
        parser->offset = OP_DRF_VAL(HCFGATHER, OFFSET, data);
        NVRM_RMC_TRACE((s_rmc, "\n-HCFGATHER\n"));
        NVRM_RMC_TRACE((s_rmc, "OFFSET 0x%x\n", parser->offset));
        NVRM_RMC_TRACE((s_rmc, "INSERT %d # %s\n",
            OP_DRF_VAL(HCFGATHER, INSERT, data),
            OP_DRF_VAL(HCFGATHER, INSERT, data) ?
                "ENABLE (insert opcode)" :
                "DISABLE (do not insert opcode)" ));
        NVRM_RMC_TRACE((s_rmc, "TYPE %d # %s\n",
            OP_DRF_VAL(HCFGATHER, TYPE, data),
            OP_DRF_VAL(HCFGATHER, TYPE, data) ?  "INCR " : "NONINCR" ));
        NVRM_RMC_TRACE((s_rmc, "COUNT 0x%x\n",
            OP_DRF_VAL(HCFGATHER, COUNT, data)));
        parser->data_count = 1;
        break;

    case HCFEXTEND_OPCODE_EXTEND:
        parser->opcode = OP_DRF_VAL(HCFEXTEND, SUBOP, data);
        switch( parser->opcode ) {
        case HCFACQUIRE_MLOCK_SUBOP_ACQUIRE_MLOCK:
            parser->data_count = 0;
            NVRM_RMC_TRACE(( s_rmc, "\n-HCFACQUIRE_MLOCK\n" ));
            NVRM_RMC_TRACE(( s_rmc, "INDX %d\n",
                OP_DRF_VAL(HCFACQUIRE, MLOCK_INDX, data) ));
            break;

        case HCFRELEASE_MLOCK_SUBOP_RELEASE_MLOCK:
            parser->data_count = 0;
            NVRM_RMC_TRACE(( s_rmc, "\n-HCFRELEASE_MLOCK\n"));
            NVRM_RMC_TRACE(( s_rmc, "INDX %d\n",
                OP_DRF_VAL(HCFRELEASE, MLOCK_INDX, data) ));
            break;

        default:
            parser->data_count = 0;
            NVRM_RMC_TRACE((s_rmc, "\n# Error Invalid opcode; Last opcode: "
                "0x%x\n", parser->opcode));
            break;
        }

        break;
    default:
        parser->data_count = 0;
        NVRM_RMC_TRACE((s_rmc,
            "\n# Error Invalid opcode; Last opcode: 0x%x\n",
            parser->opcode));
        break;
    }
}
