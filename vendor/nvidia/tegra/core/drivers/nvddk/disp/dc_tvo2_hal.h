/*
 * Copyright (c) 2008 - 2010 NVIDIA Corporation.  All rights reserved.
 * 
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_DC_TVO2_HAL_H
#define INCLUDED_DC_TVO2_HAL_H

#include "nvrm_hardware_access.h"
#include "nvddk_disp_edid.h"
#include "nvodm_query_discovery.h"
#include "ap20/artvo.h"
#include "dc_hal.h"
#include "dc_tvo_common.h"

/**
 * This is the TV-OUT driver for AP20 featuring the CVE5 core which handles
 * analog HDTV (up to 1080i).
 */

#if defined(__cplusplus)
extern "C"
{
#endif

#define DC_TVO2_MAX_SATURATION 100
#define DC_TVO2_MAX_CONTRAST 100

#define TVO2_WRITE( t, reg, val ) \
    do { \
        NV_WRITE32( (NvU32 *)((t)->tvo_regs) + (reg), (val) ); \
        if( DC_DUMP_REGS ) \
            NvOsDebugPrintf("[%.8x]=%.8x\n", reg, val); \
    } while( 0 )

#define TVO2_READ( t, reg ) \
    NV_READ32( (NvU32 *)((t)->tvo_regs) + (reg) )

typedef struct DcTvo2Rec
{
    void *tvo_regs;
    NvRmPhysAddr phys;
    NvU32 size;

    /* power client stuff */
    NvU32 PowerClientId;
    NvRmFreqKHz freq;

    NvU64 guid;
    NvOdmPeripheralConnectivity const *conn;

    /* tv attributes */
    NvU32 Overscan;
    NvU32 OverscanY;
    NvU32 OverscanCb;
    NvU32 OverscanCr;
    NvDdkDispTvOutput OutputFormat;
    NvDdkDispTvType Type;
    NvU8 DacAmplitude;
    NvDdkDispTvFilterMode FilterMode;
    NvDdkDispTvScreenFormat ScreenFormat;

    NvBool bEnabled;
    NvBool bMacrovision;
    NvBool bCCinit;
    NvDdkDispMacrovisionContext context;

    NvU32 hpos;
    NvU32 vpos;

    void *glob;
} DcTvo2;

/* power rail stuff */
NvBool DcTvo2Discover( DcTvo2 *tvo );
NvBool DcTvo2Init( void );
void DcTvo2Deinit( void );

void DcTvo2Attrs( NvU32 controller, NvU32 Overscan, NvU32 OverscanY,
    NvU32 OverscanCb, NvU32 OverscanCr, NvDdkDispTvOutput OutputFormat,
    NvU8 DacAmplitude, NvDdkDispTvFilterMode FilterMode,
    NvDdkDispTvScreenFormat ScreenFormat);
void DcTvo2Type( NvDdkDispTvType Type );
void DcTvo2Enable( NvU32 controller, NvBool enable );
void DcTvo2Position( NvU32 controller, NvU32 hpos, NvU32 vpos );
void DcTvo2ListModes( NvU32 *nModes, NvOdmDispDeviceMode *modes );
void DcTvo2SetPowerLevel( NvOdmDispDevicePowerLevel level );
void DcTvo2GetParameter( NvOdmDispParameter param, NvU32 *val );
NvU64 DcTvo2GetGuid( void );
void DcTvo2SetEdid( NvDdkDispEdid *edid, NvU32 flags );
void DcTvo2MacrovisionSetContext( NvU32 controller,
    NvDdkDispMacrovisionContext *context );
NvBool DcTvo2MacrovisionEnable( NvU32 controller, NvBool enable );
void DcTvo2MacrovisionSetCGMSA_20( NvU32 controller, NvU8 cgmsa );
void DcTvo2MacrovisionSetCGMSA_21( NvU32 controller, NvU8 cgmsa );

#if defined(__cplusplus)
}
#endif

#endif
