/*
 * Copyright (c) 2007-2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_disp.h"
#include "panel_sony_vga.h"
#include "nvodm_services.h"

static NvOdmDispDeviceMode sonyvga_modes[] =
{
    // VGA
    { 480, // width
      640, // height
      16, // bpp
      (60 << 16), // refresh
      18000, // frequency
      0, // flags
      // timing
      { 7, // h_ref_to_sync
        1, // v_ref_to_sync
        4, // h_sync_width
        1, // v_sync_width
        20, // h_back_porch
        7, // v_back_porch
        480, // h_disp_active
        640, // v_disp_active
        8, // h_front_porch
        8, // v_front_porch
      },
      NV_FALSE // partial
    },

    // QVGA
    { 240, // width
      320, // height
      16, // bpp
      (60 << 16), // refresh
      5000, // frequency
      0, // flags
      // timing
      { 7, // h_ref_to_sync
        1, // v_ref_to_sync
        4, // h_sync_width
        1, // v_sync_width
        20, // h_back_porch
        7, // v_back_porch
        240, // h_disp_active
        320, // v_disp_active
        8, // h_front_porch
        8, // v_front_porch
      },
      NV_FALSE // partial
    },
};

static NvOdmServicesGpioHandle hGpio;
static NvOdmGpioPinHandle hResetGpioPin;
static NvOdmGpioPinHandle hOnOffGpioPin;
static NvOdmGpioPinHandle hModeSelectPin;
static NvOdmGpioPinHandle hVsyncGpioPin;
static NvOdmGpioPinHandle hHsyncGpioPin;
static NvOdmGpioPinHandle hSlinGpioPin;
static NvOdmGpioPinHandle hPowerEnablePin;

static NvBool sonyvga_Setup( NvOdmDispDeviceHandle hDevice );
static NvBool sonyvga_discover( NvOdmDispDeviceHandle hDevice );
static void sonyvga_Release( NvOdmDispDeviceHandle hDevice );
static void sonyvga_ListModes( NvOdmDispDeviceHandle hDevice, NvU32 *nModes,
            NvOdmDispDeviceMode *modes );
static NvBool sonyvga_SetMode( NvOdmDispDeviceHandle hDevice,
            NvOdmDispDeviceMode *mode, NvU32 flags );
static void sonyvga_SetBacklight( NvOdmDispDeviceHandle hDevice, NvU8 intensity );
static void sonyvga_SetPowerLevel( NvOdmDispDeviceHandle hDevice,
            NvOdmDispDevicePowerLevel level );
static NvBool sonyvga_GetParameter( NvOdmDispDeviceHandle hDevice,
            NvOdmDispParameter param, NvU32 *val );
static void * sonyvga_GetConfiguration( NvOdmDispDeviceHandle hDevice );
static void sonyvga_GetPinPolarities( NvOdmDispDeviceHandle hDevice, NvU32 *nPins,
            NvOdmDispPin *Pins, NvOdmDispPinPolarity *Values );
static NvBool sonyvga_SetSpecialFunction( NvOdmDispDeviceHandle hDevice,
            NvOdmDispSpecialFunction function, void *config );

static void
sonyvga_nullmode( NvOdmDispDeviceHandle hDevice )
{
    hDevice->CurrentMode.width = 0;
    hDevice->CurrentMode.height = 0;
    hDevice->CurrentMode.bpp = 0;
}

static NvBool
sonyvga_Setup( NvOdmDispDeviceHandle hDevice )
{
    NvOdmPeripheralConnectivity const *pCon;
    NvU32 i;

    /* get the peripheral config */
    if( !sonyvga_discover( hDevice ) )
    {
        return NV_FALSE;
    }

    pCon = hDevice->PeripheralConnectivity;

    if( !hDevice->bInitialized )
    {
        hDevice->Type = NvOdmDispDeviceType_TFT;
        hDevice->Usage = NvOdmDispDeviceUsage_Primary;
        hDevice->MaxHorizontalResolution = 480;
        hDevice->MaxVerticalResolution = 640;
        hDevice->BaseColorSize = NvOdmDispBaseColorSize_666;
        hDevice->DataAlignment = NvOdmDispDataAlignment_MSB;
        hDevice->PinMap = NvOdmDispPinMap_Single_Rgb18;
        hDevice->PwmOutputPin = NVODM_DISP_GPIO_MAKE_PIN('w'-'a', 1);
        hDevice->modes = sonyvga_modes;
        hDevice->nModes = NV_ARRAY_SIZE(sonyvga_modes);
        hDevice->power = NvOdmDispDevicePowerLevel_Off;

        sonyvga_nullmode( hDevice );

        hGpio = NvOdmGpioOpen();
        if (!hGpio)
        {
            goto fail;
        }

        for (i = 0; i < pCon->NumAddress; i++)
        {
            if (pCon->AddressList[i].Interface == NvOdmIoModule_Gpio)
            {
                break;
            }
        }

        /* Assumption is 7 GPIO pins are specified in the discovery database
         * next to each other. If that assumption fails, bail out */
        if (i + 6  > pCon->NumAddress)
        {
            goto fail;
        }

        hResetGpioPin = NvOdmGpioAcquirePinHandle(hGpio,
                pCon->AddressList[i+0].Instance,
                pCon->AddressList[i+0].Address);

        hOnOffGpioPin = NvOdmGpioAcquirePinHandle(hGpio,
                pCon->AddressList[i+1].Instance,
                pCon->AddressList[i+1].Address);

        hModeSelectPin = NvOdmGpioAcquirePinHandle(hGpio,
                pCon->AddressList[i+2].Instance,
                pCon->AddressList[i+2].Address);

        hHsyncGpioPin = NvOdmGpioAcquirePinHandle(hGpio,
                pCon->AddressList[i+3].Instance,
                pCon->AddressList[i+3].Address);

        hVsyncGpioPin = NvOdmGpioAcquirePinHandle(hGpio,
                pCon->AddressList[i+4].Instance,
                pCon->AddressList[i+4].Address);

        hPowerEnablePin = NvOdmGpioAcquirePinHandle(hGpio,
                pCon->AddressList[i+5].Instance,
                pCon->AddressList[i+5].Address);


        /* slin may be invalid */
        if (pCon->AddressList[i+6].Instance != 0xFFFFFFFF)
        {
            hSlinGpioPin = NvOdmGpioAcquirePinHandle(hGpio,
                    pCon->AddressList[i+6].Instance,
                    pCon->AddressList[i+6].Address);
        }

        if( !hResetGpioPin || !hOnOffGpioPin || !hModeSelectPin ||
            !hHsyncGpioPin || !hVsyncGpioPin  || !hPowerEnablePin)
        {
            goto fail;
        }

        NvOdmGpioSetState(hGpio, hPowerEnablePin, 0x0);
        NvOdmGpioSetState(hGpio, hResetGpioPin, 0x0);
        NvOdmGpioSetState(hGpio, hOnOffGpioPin, 0x0);

        NvOdmGpioSetState(hGpio, hModeSelectPin, 0x1);
        NvOdmGpioSetState(hGpio, hHsyncGpioPin, 0x1);
        NvOdmGpioSetState(hGpio, hVsyncGpioPin, 0x1);
        if( hSlinGpioPin )
        {
            NvOdmGpioSetState(hGpio, hSlinGpioPin, 0x1);
        }

        NvOdmGpioConfig(hGpio, hResetGpioPin, NvOdmGpioPinMode_Output);
        NvOdmGpioConfig(hGpio, hPowerEnablePin, NvOdmGpioPinMode_Output);
        NvOdmGpioConfig(hGpio, hOnOffGpioPin, NvOdmGpioPinMode_Output);
        NvOdmGpioConfig(hGpio, hModeSelectPin, NvOdmGpioPinMode_Output);
        NvOdmGpioConfig(hGpio, hHsyncGpioPin, NvOdmGpioPinMode_Output);
        NvOdmGpioConfig(hGpio, hVsyncGpioPin, NvOdmGpioPinMode_Output);
        NvOdmGpioConfig(hGpio, hSlinGpioPin, NvOdmGpioPinMode_Output);

        /* Do power cycling, there should be delay of 2 msec between reset and
         * on_off and 5 msec after the power enable pin.
         *
         *  Sony VGA panel has internal regulators which generate LCD voltages
         *  when the GPIO pin hPowerEnablePin is set to high. For some FPGAs
         *  that pin is pulled high automatically and there is no need for
         *  driving that pin.
         *
         * */
        NvOdmGpioSetState(hGpio, hPowerEnablePin, 0x1);
        NvOdmOsWaitUS(5000);
        NvOdmGpioSetState(hGpio, hResetGpioPin, 0x1);
        NvOdmOsWaitUS(2000);
        NvOdmGpioSetState(hGpio, hOnOffGpioPin, 0x1);

        hDevice->bInitialized = NV_TRUE;
    }

    return NV_TRUE;

fail:
    sonyvga_Release( hDevice );
    return NV_FALSE;
}

static void
sonyvga_Release( NvOdmDispDeviceHandle hDevice )
{
    hDevice->bInitialized = NV_FALSE;
    NvOdmGpioReleasePinHandle(hGpio, hResetGpioPin);
    NvOdmGpioReleasePinHandle(hGpio, hOnOffGpioPin);
    NvOdmGpioReleasePinHandle(hGpio, hModeSelectPin);
    NvOdmGpioReleasePinHandle(hGpio, hHsyncGpioPin);
    NvOdmGpioReleasePinHandle(hGpio, hVsyncGpioPin);
    NvOdmGpioReleasePinHandle(hGpio, hPowerEnablePin);
    NvOdmGpioReleasePinHandle(hGpio, hSlinGpioPin);
    NvOdmGpioClose(hGpio);

    hResetGpioPin = 0;
    hOnOffGpioPin = 0;
    hModeSelectPin = 0;
    hHsyncGpioPin = 0;
    hVsyncGpioPin = 0;
    hPowerEnablePin = 0;
    hSlinGpioPin = 0;

    sonyvga_nullmode( hDevice );
}

static void
sonyvga_ListModes( NvOdmDispDeviceHandle hDevice, NvU32 *nModes,
        NvOdmDispDeviceMode *modes )
{
    NV_ASSERT( hDevice );
    NV_ASSERT( nModes );

    if( !(*nModes ) )
    {
        *nModes = hDevice->nModes;
    }
    else
    {
        NvU32 i;
        NvU32 len;

        len = NV_MIN( *nModes, hDevice->nModes );

        for( i = 0; i < len; i++ )
        {
            modes[i] = hDevice->modes[i];
        }
    }
}

static NvBool
sonyvga_SetMode( NvOdmDispDeviceHandle hDevice,
    NvOdmDispDeviceMode *mode, NvU32 flags )
{
    NV_ASSERT( hDevice );

    if( mode )
    {
        if (mode->width == 480)
        {
            NvOdmGpioSetState( hGpio, hModeSelectPin, 0x1 );
        }
        else
        {
            NvOdmGpioSetState( hGpio, hModeSelectPin, 0x0 );
        }

        hDevice->CurrentMode = *mode;
    }
    else
    {
       sonyvga_nullmode( hDevice );
    }

    return NV_TRUE;
}

static void
sonyvga_SetBacklight( NvOdmDispDeviceHandle hDevice, NvU8 intensity )
{
}

static void
sonyvga_SetPowerLevel( NvOdmDispDeviceHandle hDevice,
    NvOdmDispDevicePowerLevel level )
{
    NV_ASSERT( hDevice );
    hDevice->power = level;
}

static NvBool
sonyvga_GetParameter( NvOdmDispDeviceHandle hDevice,
    NvOdmDispParameter param, NvU32 *val )
{
    NV_ASSERT( hDevice );
    NV_ASSERT( val );

    switch( param ) {
    case NvOdmDispParameter_Type:
        *val = hDevice->Type;
        break;
    case NvOdmDispParameter_Usage:
        *val = hDevice->Usage;
        break;
    case NvOdmDispParameter_MaxHorizontalResolution:
        *val = hDevice->MaxHorizontalResolution;
        break;
    case NvOdmDispParameter_MaxVerticalResolution:
        *val = hDevice->MaxVerticalResolution;
        break;
    case NvOdmDispParameter_BaseColorSize:
        *val = hDevice->BaseColorSize;
        break;
    case NvOdmDispParameter_DataAlignment:
        *val = hDevice->DataAlignment;
        break;
    case NvOdmDispParameter_PinMap:
        *val = hDevice->PinMap;
        break;
    case NvOdmDispParameter_PwmOutputPin:
        *val = hDevice->PwmOutputPin;
        break;
    default:
        return NV_FALSE;
    }

    return NV_TRUE;
}

static void *
sonyvga_GetConfiguration( NvOdmDispDeviceHandle hDevice )
{
    NvOdmDispTftConfig *tft = &hDevice->Config.tft;
    tft->flags = NvOdmDispTftFlag_DataEnableMode;
    return (void *)tft;
}

static void
sonyvga_GetPinPolarities( NvOdmDispDeviceHandle hDevice, NvU32 *nPins,
    NvOdmDispPin *Pins, NvOdmDispPinPolarity *Values )
{
    *nPins = 0;
}

static NvBool
sonyvga_SetSpecialFunction( NvOdmDispDeviceHandle hDevice,
    NvOdmDispSpecialFunction function, void *config )
{
    return NV_FALSE;
}

static NvBool
sonyvga_discover( NvOdmDispDeviceHandle hDevice )
{
    NvU64 guid;
    NvOdmPeripheralConnectivity const *conn;

    /* get the main panel */
    guid = SONY_VGA_GUID;

    /* get the connectivity info */
    conn = NvOdmPeripheralGetGuid( guid );
    if( !conn )
    {
        return NV_FALSE;
    }

    hDevice->PeripheralConnectivity = conn;

    return NV_TRUE;
}

void
sonyvga_GetHal( NvOdmDispDeviceHandle hDevice )
{
    NV_ASSERT(hDevice);

    hDevice->Setup = sonyvga_Setup;
    hDevice->Release = sonyvga_Release;
    hDevice->ListModes = sonyvga_ListModes;
    hDevice->SetMode = sonyvga_SetMode;
    hDevice->SetBacklight = sonyvga_SetBacklight;
    hDevice->SetPowerLevel = sonyvga_SetPowerLevel;
    hDevice->GetParameter = sonyvga_GetParameter;
    hDevice->GetConfiguration = sonyvga_GetConfiguration;
    hDevice->GetPinPolarities = sonyvga_GetPinPolarities;
    hDevice->SetSpecialFunction = sonyvga_SetSpecialFunction;
}
