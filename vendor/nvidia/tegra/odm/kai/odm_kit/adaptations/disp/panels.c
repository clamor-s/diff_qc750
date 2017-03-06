/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "panels.h"
#include "nvodm_services.h"
#include "nvodm_backlight.h"
#include "panel_lvds_wsga.h"

#define PRIMARY_DISP_GUID LVDS_WSVGA_GUID

NvOdmDispDevice g_PrimaryDisp;

static NvBool s_init = NV_FALSE;

static void
SelectDisplayDriver(void)
{
    lvds_wsga_GetHal( &g_PrimaryDisp );
}

void
NvOdmDispListDevices( NvU32 *nDevices, NvOdmDispDeviceHandle *phDevices )
{
    NvBool b;
    NvU32 count;

    if( !s_init )
    {
        SelectDisplayDriver();
        b = g_PrimaryDisp.Setup( &g_PrimaryDisp );
        NV_ASSERT( b );
        b = b; // Otherwise release builds get 'unused variable'.

        s_init = NV_TRUE;
    }

    if( !(*nDevices) )
    {
        *nDevices = 1;
    }
    else
    {
        NV_ASSERT( phDevices );
        count = *nDevices;

        *phDevices = &g_PrimaryDisp;
        phDevices++;
        count--;
    }
}

void
NvOdmDispReleaseDevices( NvU32 nDevices, NvOdmDispDeviceHandle *hDevices )
{
    while( nDevices-- )
    {
        (*hDevices)->Release( *hDevices );
        hDevices++;
    }

    s_init = NV_FALSE;
}

void
NvOdmDispGetDefaultGuid( NvU64 *guid )
{
    *guid = PRIMARY_DISP_GUID;
}

NvBool
NvOdmDispGetDeviceByGuid( NvU64 guid, NvOdmDispDeviceHandle *phDevice )
{
    switch( guid ) {
    case PRIMARY_DISP_GUID:
        *phDevice = &g_PrimaryDisp;
        break;
    default:
        *phDevice = 0;
        return NV_FALSE;
    }

    return NV_TRUE;
}

void
NvOdmDispListModes( NvOdmDispDeviceHandle hDevice, NvU32 *nModes,
    NvOdmDispDeviceMode *modes )
{
    hDevice->ListModes( hDevice, nModes, modes );
}

NvBool
NvOdmDispSetMode( NvOdmDispDeviceHandle hDevice, NvOdmDispDeviceMode *mode,
    NvU32 flags )
{
    return hDevice->SetMode( hDevice, mode, flags );
}

void
NvOdmDispSetPowerLevel( NvOdmDispDeviceHandle hDevice,
    NvOdmDispDevicePowerLevel level )
{
    hDevice->SetPowerLevel( hDevice, level );
}

NvBool
NvOdmDispGetParameter( NvOdmDispDeviceHandle hDevice,
    NvOdmDispParameter param, NvU32 *val )
{
    return hDevice->GetParameter( hDevice, param, val );
}

void *
NvOdmDispGetConfiguration( NvOdmDispDeviceHandle hDevice )
{
    return hDevice->GetConfiguration( hDevice );
}

void
NvOdmDispGetPinPolarities( NvOdmDispDeviceHandle hDevice, NvU32 *nPins,
    NvOdmDispPin *Pins, NvOdmDispPinPolarity *Values )
{
    hDevice->GetPinPolarities( hDevice, nPins, Pins, Values );
}

NvBool
NvOdmDispSetSpecialFunction( NvOdmDispDeviceHandle hDevice,
    NvOdmDispSpecialFunction function, void *config )
{
    return hDevice->SetSpecialFunction( hDevice, function, config );
}

NvBool
NvOdmBacklightInit(NvOdmDispDeviceHandle hDevice, NvBool bReopen)
{
    if (hDevice->BacklightSetup != NULL)
        return hDevice->BacklightSetup(hDevice, bReopen);
    else
        return NV_FALSE;
}

void
NvOdmBacklightDeinit(NvOdmDispDeviceHandle hDevice)
{
    if (hDevice->BacklightRelease != NULL)
        hDevice->BacklightRelease(hDevice);
}

void
NvOdmBacklightIntensity(NvOdmDispDeviceHandle hDevice,
                      NvU8 intensity)
{
    if (hDevice->BacklightIntensity != NULL)
        hDevice->BacklightIntensity(hDevice, intensity);
}


void
NvOdmDispSetBacklight( NvOdmDispDeviceHandle hDevice, NvU8 intensity )
{
    static NvBool IsInit = NV_FALSE;

    hDevice->SetBacklight( hDevice, intensity );

    if (!IsInit)
    {
        NvOdmBacklightInit(hDevice, NV_FALSE);
        IsInit = NV_TRUE;
    }
    NvOdmBacklightIntensity(hDevice, 100);
}
