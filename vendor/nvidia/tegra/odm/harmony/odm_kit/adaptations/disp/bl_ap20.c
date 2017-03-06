/*
 * Copyright (c) 2007 - 2009 NVIDIA Corporation.  All rights reserved.
 * 
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_disp.h"
#include "nvodm_services.h"
#include "nvodm_query_pinmux.h"
#include "nvodm_query_discovery.h"
#include "nvodm_query_gpio.h"
#include "nvodm_backlight.h"
#include "nvassert.h"
#include "panel_sharp_wvga.h"
#include "bl_ap20.h"

#if NV_DEBUG
#define ASSERT_SUCCESS( expr ) \
    do { \
        NvBool b = (expr); \
        NV_ASSERT( b == NV_TRUE ); \
    } while( 0 )
#else
#define ASSERT_SUCCESS( expr ) \
    do { \
        (void)(expr); \
    } while( 0 )
#endif

typedef struct DeviceGpioRec
{
    NvOdmPeripheralConnectivity const * pPeripheralConnectivity;
    NvOdmServicesGpioHandle hGpio;
    NvOdmGpioPinHandle hPin;
} DeviceGpio;

static DeviceGpio s_DeviceGpio = {
    NULL,
    NULL,
    NULL,
};

static NvBool ap20_BacklightSetup( NvOdmDispDeviceHandle hDevice, NvBool bReopen );
static void ap20_BacklightRelease( NvOdmDispDeviceHandle hDevice );
static void ap20_BacklightIntensity( NvOdmDispDeviceHandle hDevice, NvU8 intensity );
static NvBool ap20_PrivBacklightEnable( NvBool Enable );

// These backlight functions are controlled by an on-board GPIO,
// whereas some configurations use a PMU GPIO.
static NvBool ap20_BacklightSetup( NvOdmDispDeviceHandle hDevice, NvBool bReopen )
{
    if( bReopen )
    {
        return NV_TRUE;
    }

    if (!ap20_PrivBacklightEnable( NV_TRUE ))
        return NV_FALSE; 

    return NV_TRUE;
}

static void ap20_BacklightRelease( NvOdmDispDeviceHandle hDevice )
{
    ASSERT_SUCCESS(
        ap20_PrivBacklightEnable( NV_FALSE )
    );

    return;
}

static void ap20_BacklightIntensity( NvOdmDispDeviceHandle hDevice, NvU8 intensity )
{
    return;
}

static NvBool
ap20_PrivBacklightEnable( NvBool Enable )
{
    NvBool RetVal = NV_TRUE;
    NvU32 GpioPort = 0;
    NvU32 GpioPin  = 0;

    NvU32 Index;

    // 0. Get the peripheral connectivity information
    NvOdmPeripheralConnectivity const *pConnectivity;

    pConnectivity = NvOdmPeripheralGetGuid( SHARP_WVGA_AP20_GUID );

    if ( !pConnectivity )
        return NV_FALSE;

    s_DeviceGpio.pPeripheralConnectivity = pConnectivity;

    // 1. Retrieve the GPIO Port & Pin from peripheral connectivity entry --

    // Use Peripheral Connectivity DB to locate the Port & Pin configuration
    //    NOTE: Revise & refine this search if there is more than one GPIO defined.
    for ( Index = 0; Index < s_DeviceGpio.pPeripheralConnectivity->NumAddress; ++Index )
    {
        if ( s_DeviceGpio.pPeripheralConnectivity->AddressList[Index].Interface == NvOdmIoModule_Gpio )
        {
            RetVal = NV_TRUE;
            GpioPort = s_DeviceGpio.pPeripheralConnectivity->AddressList[Index].Instance;
            GpioPin  = s_DeviceGpio.pPeripheralConnectivity->AddressList[Index].Address;
        }
    }

    if ( !RetVal )
        return NV_FALSE;

    // 2. Open the GPIO Port (acquire a handle)
    s_DeviceGpio.hGpio = NvOdmGpioOpen();

    if ( !s_DeviceGpio.hGpio )
    {
        RetVal = NV_FALSE;
        goto ap20_PrivBacklightEnable_Fail;
    }
    else
    {
        RetVal = NV_TRUE;
    }

    // 3. Acquire a pin handle from the opened GPIO port
    s_DeviceGpio.hPin = NvOdmGpioAcquirePinHandle( s_DeviceGpio.hGpio, GpioPort, GpioPin );
    if ( !s_DeviceGpio.hPin )
    {
        RetVal = NV_FALSE;
        goto ap20_PrivBacklightEnable_Fail;
    }
    else
    {
        RetVal = NV_TRUE;
    }

    // 4. Set the GPIO as requested
    NvOdmGpioConfig( s_DeviceGpio.hGpio, s_DeviceGpio.hPin,
        NvOdmGpioPinMode_Output );
    NvOdmGpioSetState( s_DeviceGpio.hGpio, s_DeviceGpio.hPin, (NvU32)Enable );
    return NV_TRUE;

ap20_PrivBacklightEnable_Fail:
    NvOdmGpioClose( s_DeviceGpio.hGpio );
    s_DeviceGpio.hGpio = NULL;

    return RetVal;
}

void BL_AP20_GetHal( NvOdmDispDeviceHandle hDevice )
{
    NV_ASSERT(hDevice);

    // Backlight functions
    hDevice->pfnBacklightSetup = ap20_BacklightSetup;
    hDevice->pfnBacklightRelease = ap20_BacklightRelease;
    hDevice->pfnBacklightIntensity = ap20_BacklightIntensity;
}
