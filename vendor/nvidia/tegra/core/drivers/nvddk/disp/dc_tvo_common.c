/*
 * Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
 * 
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "dc_tvo_common.h"
#include "nvassert.h"

#define DC_TVO_TEST_MV_DATA (0)

#define READ32( mem, offset, ret ) \
    do { \
        NvU32 x; \
        NvU8 *tmp = (NvU8 *)(mem) + *(offset); \
        x = (NvU32)( tmp[0] | ((NvU32)tmp[1] << 8) | \
            ((NvU32)tmp[2] << 16) | ((NvU32)tmp[3] << 24)); \
        *(ret) = x; \
        *(offset) = *(offset) + 4; \
    } while( 0 );

NvBool DcTvoCommonGetMvData( void *glob, NvMvGlobEntryType entry_type,
    NvMvGlobHardwareType hw_type, NvU32 *num, TvoEncoderData **data )
{
    NvU32 offset;
    NvU32 nEntries;
    NvU32 size;
    NvU32 val;
    NvU32 i;

    /* verify version */
    offset = 0;
    READ32( glob, &offset, &val );
    if( val != 1 )
    {
        return NV_FALSE;
    }

    READ32( glob, &offset, &nEntries );

    for( i = 0; i < nEntries; i++ )
    {
        NvU32 next;

        /* size */
        READ32( glob, &offset, &size );
        next = offset + size - 4;

        /* entry type */
        READ32( glob, &offset, &val );
        if( (NvMvGlobEntryType)val != entry_type )
        {
            offset = next;
            continue;
        }

        /* hardware type */
        READ32( glob, &offset, &val );
        if( (NvMvGlobHardwareType)val != hw_type )
        {
            offset = next;
            continue;
        }

        /* num regs */
        READ32( glob, &offset, &val );
        *num = val;

        /* reg data */
        *data = (TvoEncoderData *)(((NvU8 *)glob) + offset);

#if DC_TVO_TEST_MV_DATA
        {
            NvU32 j;
            char *s;
            TvoEncoderData *d = *data;

            switch( entry_type ) {
            case NvMvGlobEntryType_Ntsc_Disable:
                s = "ntsc disable";
                break;
            case NvMvGlobEntryType_Ntsc_NoSplitBurst:
                s = "ntsc no split burst";
                break;
            case NvMvGlobEntryType_Ntsc_SplitBurst_2_Line:
                s = "ntsc 2 line split burst";
                break;
            case NvMvGlobEntryType_Ntsc_SplitBurst_4_Line:
                s = "ntsc 4 line split burst";
                break;
            case NvMvGlobEntryType_Pal_Disable:
                s = "pal disable";
                break;
            case NvMvGlobEntryType_Pal_Enable:
                s = "pal enable";
                break;
            default:
                NV_ASSERT( !"bad entry type" );
                s = "";
            }

            NvOsDebugPrintf( "mv regs type: %s hw: %s\n", s,
                (hw_type == NvMvGlobHardwareType_Cve3) ?
                    "CVE3" : "CVE5" );

            for( j = 0; j < *num; j++ )
            {
                NvOsDebugPrintf( "0x%.2x 0x%.2x\n", d->addr, d->data );
                d++;
            }
        }
#endif
        return NV_TRUE;
    }

    return NV_FALSE;
}

static const char *NVODM_TVO_GET_GLOB_FUNC = "NvOdmDispTvoGetGlob";
static const char *NVODM_TVO_RELEASE_GLOB_FUNC = "NvOdmDispTvoReleaseGlob";
static const char *NVODM_TVO_LIB = "libnvodm_tvo";

static NvOdmDispTvoGetGlobType DcTvoGetGlob;
static NvOdmDispTvoReleaseGlobType DcTvoReleaseGlob;
static NvOsLibraryHandle hGlobLib;

static void *
NvOdmDispDefaultTvoGetGlob( NvU32 *size )
{
    static NvU8 test_data[] =
    {
        0, 0, 0, 0
    };

    *size = sizeof(test_data);
    return test_data;
}

static void
NvOdmDispDefaultTvoReleaseGlob( void *glob )
{
    // do nothing
}

static void DcTvoGetAdptation( void )
{
    NvError e;

    e = NvOsLibraryLoad( NVODM_TVO_LIB, &hGlobLib );
    if( e != NvSuccess )
    {
        goto clean;
    }

    DcTvoGetGlob = (NvOdmDispTvoGetGlobType)
       NvOsLibraryGetSymbol( hGlobLib, NVODM_TVO_GET_GLOB_FUNC );

    DcTvoReleaseGlob = (NvOdmDispTvoReleaseGlobType)
       NvOsLibraryGetSymbol( hGlobLib, NVODM_TVO_RELEASE_GLOB_FUNC );

clean:
    if( !DcTvoGetGlob )
    {
        DcTvoGetGlob = NvOdmDispDefaultTvoGetGlob;
        DcTvoReleaseGlob = NvOdmDispDefaultTvoReleaseGlob;
    }
}

void *
DcTvoCommonGlobOpen( void )
{
    void *glob;
    NvU32 size;

    DcTvoGetAdptation();

    glob = DcTvoGetGlob( &size );
    NV_ASSERT( glob );

    return glob;
}

void DcTvoCommonGlobClose( void *glob )
{
    if( !glob )
    {
        return;
    }

    DcTvoReleaseGlob( glob );
    NvOsLibraryUnload( hGlobLib );
    hGlobLib = 0;
}
