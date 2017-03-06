/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_disp.h"
#include "panel_lvds_wsga.h"
#include "nvodm_services.h"
#include "nvodm_query_pinmux.h"
#include "nvodm_query_discovery.h"
#include "nvodm_query_gpio.h"
#include "nvodm_backlight.h"

static NvBool s_is_lvds_wsga_mode = NV_TRUE;

static NvOdmDispDeviceMode lvds_wsga_modes[] =
{
    // WSGA, 18 bit
    { 800, // width
      1280, // height
      24, // bpp
      0, // refresh
      66770,//51206, // frequency
      0, // flags
      // timing
      { 11, // h_ref_to_sync
        1, // v_ref_to_sync
        10, // h_sync_width
        5, // v_sync_width
        10, // h_back_porch
        15, // v_back_porch
        800, // h_disp_active
        1280, // v_disp_active
        300, // h_front_porch
        15, // v_front_porch
      },
      NV_FALSE // partial
    }
};

#define GpioIndex_LVDSEnableGpioPin               0
#define GpioIndex_BacklightPowerEnableGpioPin     1
#define GpioIndex_BacklightEnableGpioPin          2
#define GpioIndex_PanelEnableGpioPin              3
#define GpioIndex_PanelPwmGpio                    4
#define GpioIndex_Mode0                           5
#define GpioIndex_Mode1                           6
#define GpioIndex_Bpp                             7
#define GpioIndex_LcdUd                           8
#define GpioIndex_LcdLr                           9
#define GpioIndex_Stby                           10
#define GpioIndex_Reset                          11
#define GpioIndex_Rs                             12
#define GpioIndex_LcdAvddEn                      13
#define GpioIndex_LcdVglEn                       14
#define GpioIndex_LcdVghEn                       15
#define GpioIndex_Charge_Z4                       16

#define EDID_FRAME_SIZE 128
#define EDID_SEGMENT_ADD 0x60
#define I2C_SPEED_KHZ_EDID_READ 40
#define I2C_TRANSACTION_TIMEOUT_MS 5000
#define EDID_HORIZ_SIZE_INDEX 0x15
#define EDID_VERT_SIZE_INDEX 0x16

static NvOdmServicesGpioHandle s_hGpio;
static NvOdmGpioPinHandle s_hLVDSEnableGpioPin;
static NvOdmGpioPinHandle s_hBacklightPowerEnableGpioPin;
static NvOdmGpioPinHandle s_hBacklightEnableGpioPin;
static NvOdmGpioPinHandle s_hPanelEnableGpioPin;
static NvOdmGpioPinHandle s_hBacklightIntensityPin;

static NvOdmServicesPwmHandle s_hOdmPwm = NULL;

static const NvOdmGpioPinInfo *s_gpioPinTable;

static NvBool s_bReopen = NV_FALSE;
static NvBool s_bBacklight = NV_FALSE;
static NvU8 s_backlight_intensity;
static NvU8 s_backlight_resume_intensity;

NvBool lvds_wsga_panel_init( NvOdmDispDeviceHandle hDevice, NvU32 bpp );
NvBool lvds_wsga_discover( NvOdmDispDeviceHandle hDevice );
void lvds_wsga_suspend( NvOdmDispDeviceHandle hDevice );
void lvds_wsga_resume( NvOdmDispDeviceHandle hDevice );
void Keenhi_Charge_Pin( NvBool  state );


void Keenhi_Charge_Pin( NvBool  state )
{
	NvOdmGpioPinHandle tmpGpioPin;
        NvOdmGpioPinHandle s_hResetGpioPin; 

        //Begin GPIO init sequence
        if(s_hGpio && s_gpioPinTable) {
	        tmpGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
	                s_gpioPinTable[GpioIndex_Charge_Z4].Port,
	                s_gpioPinTable[GpioIndex_Charge_Z4].Pin);
	        NvOdmGpioSetState(s_hGpio, tmpGpioPin, state);
	        NvOdmGpioConfig(s_hGpio, tmpGpioPin, NvOdmGpioPinMode_Output);
        }
}


static void
lvds_wsga_nullmode( NvOdmDispDeviceHandle hDevice )
{
    hDevice->CurrentMode.width = 0;
    hDevice->CurrentMode.height = 0;
    hDevice->CurrentMode.bpp = 0;
}

NvBool lvds_wsga_Setup( NvOdmDispDeviceHandle hDevice )
{
    volatile NvU32* ptr;

    if( !hDevice->bInitialized )
    {
        NvU32 gpioPinCount;

        hDevice->Type = NvOdmDispDeviceType_TFT;
        hDevice->Usage = NvOdmDispDeviceUsage_Primary;
        hDevice->MaxHorizontalResolution = 800;//1024;
        hDevice->MaxVerticalResolution = 1280;//600;
        hDevice->BaseColorSize = NvOdmDispBaseColorSize_666;
        hDevice->DataAlignment = NvOdmDispDataAlignment_MSB;
        hDevice->PinMap = NvOdmDispPinMap_Single_Rgb18;
        hDevice->PwmOutputPin = NvOdmDispPwmOutputPin_Unspecified;
        hDevice->modes = lvds_wsga_modes;
        hDevice->nModes = NV_ARRAY_SIZE(lvds_wsga_modes);
        hDevice->power = NvOdmDispDevicePowerLevel_Off;

        lvds_wsga_nullmode( hDevice );

        s_hOdmPwm = NvOdmPwmOpen();
        NV_ASSERT(s_hOdmPwm);

        lvds_wsga_SetBacklightIntensity(hDevice, 255);

        if( !s_hGpio )
        {
            s_hGpio = NvOdmGpioOpen();
            if( !s_hGpio )
            {
                return NV_FALSE;
            }
        }

        s_gpioPinTable = NvOdmQueryGpioPinMap(NvOdmGpioPinGroup_LvdsDisplay, 0,
                                       &gpioPinCount);
        ptr = (volatile NvU32*)0x7000313c;
        *ptr = 0x2a;
        ptr = (volatile NvU32*)0x70003134;
        *ptr = 0x2a;
        ptr = (volatile NvU32*)0x700031dc;
        *ptr = 0x2a;
        ptr = (volatile NvU32*)0x70003218;
        *ptr = 0x2a;

        // Setup PWM pin in "function mode" (used for backlight intensity)
        s_hBacklightIntensityPin = NvOdmGpioAcquirePinHandle(s_hGpio,
                                     s_gpioPinTable[GpioIndex_PanelPwmGpio].Port,
                                     s_gpioPinTable[GpioIndex_PanelPwmGpio].Pin);
        NvOdmGpioConfig(s_hGpio, s_hBacklightIntensityPin,
            NvOdmGpioPinMode_Function);
        NvOdmOsWaitUS(300);

        s_hLVDSEnableGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
                s_gpioPinTable[GpioIndex_LVDSEnableGpioPin].Port,
                s_gpioPinTable[GpioIndex_LVDSEnableGpioPin].Pin);

        s_hBacklightEnableGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
                s_gpioPinTable[GpioIndex_BacklightEnableGpioPin].Port,
                s_gpioPinTable[GpioIndex_BacklightEnableGpioPin].Pin);

        s_hPanelEnableGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
                s_gpioPinTable[GpioIndex_PanelEnableGpioPin].Port,
                s_gpioPinTable[GpioIndex_PanelEnableGpioPin].Pin);

        NvOdmGpioPinHandle tmpGpioPin;
        NvOdmGpioPinHandle s_hResetGpioPin;

        //Begin GPIO init sequence
        
        tmpGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
                s_gpioPinTable[GpioIndex_Charge_Z4].Port,
                s_gpioPinTable[GpioIndex_Charge_Z4].Pin);
        NvOdmGpioSetState(s_hGpio, tmpGpioPin, 0x1);
        NvOdmGpioConfig(s_hGpio, tmpGpioPin, NvOdmGpioPinMode_Output);
		
        tmpGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
                s_gpioPinTable[GpioIndex_Mode0].Port,
                s_gpioPinTable[GpioIndex_Mode0].Pin);
        NvOdmGpioSetState(s_hGpio, tmpGpioPin, 0x0);
        NvOdmGpioConfig(s_hGpio, tmpGpioPin, NvOdmGpioPinMode_Output);

        tmpGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
                s_gpioPinTable[GpioIndex_Mode1].Port,
                s_gpioPinTable[GpioIndex_Mode1].Pin);
        NvOdmGpioSetState(s_hGpio, tmpGpioPin, 0x0);
        NvOdmGpioConfig(s_hGpio, tmpGpioPin, NvOdmGpioPinMode_Output);

        NvOdmGpioSetState(s_hGpio, s_hPanelEnableGpioPin, 0x1);
        NvOdmGpioConfig(s_hGpio, s_hPanelEnableGpioPin, NvOdmGpioPinMode_Output);

        tmpGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
                s_gpioPinTable[GpioIndex_LcdUd].Port,
                s_gpioPinTable[GpioIndex_LcdUd].Pin);
        NvOdmGpioSetState(s_hGpio, tmpGpioPin, 0x0);
        NvOdmGpioConfig(s_hGpio, tmpGpioPin, NvOdmGpioPinMode_Output);

        tmpGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
                s_gpioPinTable[GpioIndex_LcdLr].Port,
                s_gpioPinTable[GpioIndex_LcdLr].Pin);
        NvOdmGpioSetState(s_hGpio, tmpGpioPin, 0x1);
        NvOdmGpioConfig(s_hGpio, tmpGpioPin, NvOdmGpioPinMode_Output);

        tmpGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
                s_gpioPinTable[GpioIndex_Bpp].Port,
                s_gpioPinTable[GpioIndex_Bpp].Pin);
        NvOdmGpioSetState(s_hGpio, tmpGpioPin, 0x0);
        NvOdmGpioConfig(s_hGpio, tmpGpioPin, NvOdmGpioPinMode_Output);

        tmpGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
                s_gpioPinTable[GpioIndex_Stby].Port,
                s_gpioPinTable[GpioIndex_Stby].Pin);
        NvOdmGpioSetState(s_hGpio, tmpGpioPin, 0x1);
        NvOdmGpioConfig(s_hGpio, tmpGpioPin, NvOdmGpioPinMode_Output);

        NvOdmOsWaitUS(1000);

        s_hResetGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
                s_gpioPinTable[GpioIndex_Reset].Port,
                s_gpioPinTable[GpioIndex_Reset].Pin);
        NvOdmGpioSetState(s_hGpio, s_hResetGpioPin, 0x1);
        NvOdmGpioConfig(s_hGpio, s_hResetGpioPin, NvOdmGpioPinMode_Output);

        tmpGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
                s_gpioPinTable[GpioIndex_LcdAvddEn].Port,
                s_gpioPinTable[GpioIndex_LcdAvddEn].Pin);
        NvOdmGpioSetState(s_hGpio, tmpGpioPin, 0x1);
        NvOdmGpioConfig(s_hGpio, tmpGpioPin, NvOdmGpioPinMode_Output);

        NvOdmOsWaitUS(1000);

        tmpGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
                s_gpioPinTable[GpioIndex_LcdVglEn].Port,
                s_gpioPinTable[GpioIndex_LcdVglEn].Pin);
        NvOdmGpioSetState(s_hGpio, tmpGpioPin, 0x1);
        NvOdmGpioConfig(s_hGpio, tmpGpioPin, NvOdmGpioPinMode_Output);

        NvOdmOsWaitUS(1000);

        tmpGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
                s_gpioPinTable[GpioIndex_LcdVghEn].Port,
                s_gpioPinTable[GpioIndex_LcdVghEn].Pin);
        NvOdmGpioSetState(s_hGpio, tmpGpioPin, 0x1);
        NvOdmGpioConfig(s_hGpio, tmpGpioPin, NvOdmGpioPinMode_Output);

        NvOdmOsWaitUS(50000);

        //Pulse LCD reset
        NvOdmGpioSetState(s_hGpio, s_hResetGpioPin, 0x0);
        NvOdmGpioConfig(s_hGpio, s_hResetGpioPin, NvOdmGpioPinMode_Output);

        NvOdmOsWaitUS(1000); //TODO: spec says 50uS is ok. 1ms is following the GFShell script

        NvOdmGpioSetState(s_hGpio, s_hResetGpioPin, 0x1);
        NvOdmGpioConfig(s_hGpio, s_hResetGpioPin, NvOdmGpioPinMode_Output);

        // Enable Backlight Power
        s_hBacklightPowerEnableGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
                s_gpioPinTable[GpioIndex_BacklightPowerEnableGpioPin].Port,
                s_gpioPinTable[GpioIndex_BacklightPowerEnableGpioPin].Pin);
        NvOdmGpioSetState(s_hGpio, s_hBacklightPowerEnableGpioPin, 0x1);
        NvOdmGpioConfig(s_hGpio, s_hBacklightPowerEnableGpioPin, NvOdmGpioPinMode_Output);

        NvOdmOsWaitUS(10000);

        tmpGpioPin = NvOdmGpioAcquirePinHandle(s_hGpio,
                s_gpioPinTable[GpioIndex_Rs].Port,
                s_gpioPinTable[GpioIndex_Rs].Pin);
        NvOdmGpioSetState(s_hGpio, tmpGpioPin, 0x1);
        NvOdmGpioConfig(s_hGpio, tmpGpioPin, NvOdmGpioPinMode_Output);

        NvOdmOsWaitUS(10000);

        NvOdmGpioSetState(s_hGpio, s_hLVDSEnableGpioPin , 0x1);
        NvOdmGpioConfig(s_hGpio, s_hLVDSEnableGpioPin, NvOdmGpioPinMode_Output);

        NvOdmOsWaitUS(100000);

        hDevice->bInitialized = NV_TRUE;
    }

    return NV_TRUE;
}

void lvds_wsga_Release( NvOdmDispDeviceHandle hDevice )
{
    hDevice->bInitialized = NV_FALSE;

    NvOdmBacklightIntensity( hDevice, 0 );
    NvOdmBacklightDeinit(hDevice);
    s_bBacklight = NV_FALSE;

    NvOdmGpioReleasePinHandle(s_hGpio, s_hLVDSEnableGpioPin);
    NvOdmGpioReleasePinHandle(s_hGpio, s_hBacklightEnableGpioPin);
    NvOdmGpioReleasePinHandle(s_hGpio, s_hPanelEnableGpioPin);
    NvOdmGpioClose(s_hGpio);

    s_hLVDSEnableGpioPin = 0;
    s_hBacklightEnableGpioPin = 0;
    s_hPanelEnableGpioPin = 0;
    s_hGpio = 0;

    lvds_wsga_nullmode( hDevice );
}

void lvds_wsga_ListModes( NvOdmDispDeviceHandle hDevice, NvU32 *nModes,
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

NvBool lvds_wsga_SetMode( NvOdmDispDeviceHandle hDevice,
    NvOdmDispDeviceMode *mode, NvU32 flags )
{
    NV_ASSERT( hDevice );

    if( mode )
    {
        if( flags & NVODM_DISP_MODE_FLAGS_REOPEN )
        {
            s_bReopen = NV_TRUE;
        }

        if( !hDevice->CurrentMode.width && !hDevice->CurrentMode.height &&
            !hDevice->CurrentMode.bpp )
        {
            lvds_wsga_panel_init( hDevice, mode->bpp );
        }

        if( !s_bBacklight && !NvOdmBacklightInit( hDevice, s_bReopen ) )
        {
            return NV_FALSE;
        }

        s_bBacklight = NV_TRUE;

        hDevice->CurrentMode = *mode;
    }
    else
    {
        if( !s_bBacklight )
        {
            if( !NvOdmBacklightInit( hDevice, NV_FALSE ) )
            {
                return NV_FALSE;
            }
            s_bBacklight = NV_TRUE;
        }
        NvOdmBacklightIntensity( hDevice, 0 );
        lvds_wsga_nullmode( hDevice );
    }

    return NV_TRUE;
}

void lvds_wsga_SetBacklight( NvOdmDispDeviceHandle hDevice, NvU8 intensity )
{
    if( !s_bBacklight )
    {
        NvOdmBacklightInit( hDevice, NV_FALSE );
        s_bBacklight = NV_TRUE;
    }

    NvOdmBacklightIntensity( hDevice, intensity );
    s_backlight_intensity = intensity;
}

NvBool lvds_wsga_BacklightSetup( NvOdmDispDeviceHandle hDevice, NvBool bReopen )
{
    NV_ASSERT( hDevice );

    lvds_wsga_Setup(hDevice);

    return NV_TRUE;
}

void lvds_wsga_BacklightRelease( NvOdmDispDeviceHandle hDevice )
{
    NV_ASSERT( hDevice );

    NvOdmGpioReleasePinHandle(s_hGpio, s_hBacklightEnableGpioPin);
    s_hBacklightEnableGpioPin = 0;
}

void lvds_wsga_SetBacklightIntensity( NvOdmDispDeviceHandle hDevice, NvU8 intensity )
{
    NvU32 RequestedFreq, ReturnedFreq;
    NvU32 DutyCycle, PwmPin = 0;
    unsigned int i;
    const NvOdmPeripheralConnectivity *conn;

    NV_ASSERT( hDevice );

    conn = hDevice->PeripheralConnectivity;
    if (!conn)
        return;

    for (i=0; i<conn->NumAddress; i++)
    {
        if (conn->AddressList[i].Interface == NvOdmIoModule_Pwm)
        {
            PwmPin = conn->AddressList[i].Address;
            break;
        }
    }

    RequestedFreq = 200; // Hz
	intensity = 30;
    // Upper 16 bits of DutyCycle is intensity percentage (whole number portion)
    // Intensity must be scaled from 0 - 255 to a percentage.
    DutyCycle = ((intensity * 100)/255) << 16;

    if (intensity != 0)
    {
        NvOdmGpioSetState(s_hGpio, s_hBacklightEnableGpioPin, 0x1);

        NvOdmPwmConfig(s_hOdmPwm, NvOdmPwmOutputId_PWM0 + PwmPin,
                       NvOdmPwmMode_Enable, DutyCycle, &RequestedFreq,
                       &ReturnedFreq);
    }
    else
    {
        NvOdmGpioSetState(s_hGpio, s_hBacklightEnableGpioPin, 0x0);

        // Harmony-specific backlight adjustment (using PWM0)
        NvOdmPwmConfig(s_hOdmPwm, NvOdmPwmOutputId_PWM0 + PwmPin,
                       NvOdmPwmMode_Disable, DutyCycle, &RequestedFreq,
                       &ReturnedFreq);
    }
}

void lvds_wsga_SetPowerLevel( NvOdmDispDeviceHandle hDevice,
    NvOdmDispDevicePowerLevel level )
{
    NV_ASSERT( hDevice );
    hDevice->power = level;

    switch( level ) {
    case NvOdmDispDevicePowerLevel_Off:
    case NvOdmDispDevicePowerLevel_Suspend:
        s_backlight_resume_intensity = s_backlight_intensity;
        lvds_wsga_SetBacklight( hDevice, 0x0);
        lvds_wsga_suspend( hDevice );
        break;
    case NvOdmDispDevicePowerLevel_On:
        lvds_wsga_resume( hDevice );
        lvds_wsga_SetBacklight( hDevice, s_backlight_resume_intensity);
        break;
    default:
        break;
    }
}

NvBool lvds_wsga_GetParameter( NvOdmDispDeviceHandle hDevice,
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

void * lvds_wsga_GetConfiguration( NvOdmDispDeviceHandle hDevice )
{
    NvOdmDispTftConfig *tft = &hDevice->Config.tft;
    return (void *)tft;
}

void lvds_wsga_GetPinPolarities( NvOdmDispDeviceHandle hDevice, NvU32 *nPins,
    NvOdmDispPin *Pins, NvOdmDispPinPolarity *Values )
{
    *nPins = 4;

    Pins[0] = NvOdmDispPin_DataEnable;

    Values[0] = NvOdmDispPinPolarity_High;

    Pins[1] = NvOdmDispPin_HorizontalSync;

    Values[1] = NvOdmDispPinPolarity_Low;

    Pins[2] = NvOdmDispPin_VerticalSync;

    Values[2] = NvOdmDispPinPolarity_Low;

    Pins[3] = NvOdmDispPin_PixelClock;

    Values[3] = NvOdmDispPinPolarity_Low;

}

NvBool lvds_wsga_SetSpecialFunction( NvOdmDispDeviceHandle hDevice,
    NvOdmDispSpecialFunction function, void *config )
{
    return NV_FALSE;
}

NvBool
lvds_wsga_discover( NvOdmDispDeviceHandle hDevice )
{
    NvU64 guid;
    NvOdmPeripheralConnectivity const *conn;

    /* get the main panel */
    guid = LVDS_WSVGA_GUID;

    /* get the connectivity info */
    conn = NvOdmPeripheralGetGuid( guid );
    if( !conn )
    {
        NV_ASSERT(!"ERROR: Panel not found.");
        return NV_FALSE;
    }

    hDevice->PeripheralConnectivity = conn;

    return NV_TRUE;
}

void
lvds_wsga_resume( NvOdmDispDeviceHandle hDevice )
{
    NvOdmPeripheralConnectivity const *conn;
    NvOdmServicesPmuHandle hPmu;
    NvU32 settle_us;
    NvU32 i;

    /** WARNING **/
    // This must call the power rail config regardless of the reopen
    // flag. Reference counts are not preserved across the bootloader
    // to operating system transition. All code must be self-balancing.

    /* enable the power rail */
    hPmu = NvOdmServicesPmuOpen();
    conn = hDevice->PeripheralConnectivity;

    NV_ASSERT( conn );

    NvOdmGpioSetState(s_hGpio, s_hBacklightPowerEnableGpioPin, 0x1);
    // Enable LVDS Panel Power
    NvOdmGpioConfig(s_hGpio, s_hPanelEnableGpioPin, NvOdmGpioPinMode_Output);
    NvOdmOsWaitUS(300);
    // Enable LVDS Transmitter
    NvOdmGpioSetState(s_hGpio, s_hLVDSEnableGpioPin, 0x1);
    // Enable Backlight Intensity.
    NvOdmGpioConfig(s_hGpio, s_hBacklightEnableGpioPin, NvOdmGpioPinMode_Output);

    for( i = 0; i < conn->NumAddress; i++ )
    {
        if( conn->AddressList[i].Interface == NvOdmIoModule_Vdd )
        {
            NvOdmServicesPmuVddRailCapabilities cap;

            /* address is the vdd rail id */
            NvOdmServicesPmuGetCapabilities( hPmu,
                conn->AddressList[i].Address, &cap );

            /* set the rail volatage to the recommended */
            NvOdmServicesPmuSetVoltage( hPmu,
                conn->AddressList[i].Address, cap.requestMilliVolts,
                &settle_us );

            /* wait for rail to settle */
            NvOdmOsWaitUS( settle_us );
        }
    }

    NvOdmServicesPmuClose( hPmu );

    if( s_bReopen )
    {
        // the bootloader has already initialized the panel, skip this resume.
        s_bReopen = NV_FALSE;
        return;
    }
}

void
lvds_wsga_suspend( NvOdmDispDeviceHandle hDevice )
{
    NvOdmPeripheralConnectivity const *conn;
    NvOdmServicesPmuHandle hPmu;
    NvU32 RequestedFreq, ReturnedFreq;
    NvU32 DutyCycle;
    NvU32 i;

    /* disable power */
    hPmu = NvOdmServicesPmuOpen();
    conn = hDevice->PeripheralConnectivity;

    NV_ASSERT( conn );

    DutyCycle = RequestedFreq = 0;

    for (i = 0; i<conn->NumAddress; i++)
    {
        NvU32 Pwm;
        if (conn->AddressList[i].Interface != NvOdmIoModule_Pwm)
            continue;
        Pwm = conn->AddressList[i].Address;
        NvOdmPwmConfig(s_hOdmPwm, NvOdmPwmOutputId_PWM0 + Pwm,
                       NvOdmPwmMode_Disable, DutyCycle, &RequestedFreq,
                       &ReturnedFreq);
        break;
    }

    for( i = 0; i < conn->NumAddress; i++ )
    {
        if( conn->AddressList[i].Interface == NvOdmIoModule_Vdd )
        {
            /* set the rail volatage to the recommended */
            NvOdmServicesPmuSetVoltage( hPmu,
                conn->AddressList[i].Address, NVODM_VOLTAGE_OFF, 0 );
        }
    }

    // Disable Backlight Intensity.
    NvOdmGpioConfig(s_hGpio, s_hBacklightEnableGpioPin, NvOdmGpioPinMode_Tristate);
    // Disable LVDS Transmitter
    NvOdmGpioSetState(s_hGpio, s_hLVDSEnableGpioPin, 0x0);
    NvOdmOsWaitUS(300);
    // Disable LVDS Panel Power
    NvOdmGpioConfig(s_hGpio, s_hPanelEnableGpioPin, NvOdmGpioPinMode_Tristate);
    NvOdmGpioSetState(s_hGpio, s_hBacklightPowerEnableGpioPin, 0x0);
    NvOdmOsWaitUS (300);
    NvOdmServicesPmuClose( hPmu );
}

NvBool
lvds_wsga_panel_init( NvOdmDispDeviceHandle hDevice, NvU32 bpp )
{
    if( !hDevice->bInitialized )
    {
        return NV_FALSE;
    }

    /* get the peripheral config */
    if( !lvds_wsga_discover( hDevice ) )
    {
        return NV_FALSE;
    }

    return NV_TRUE;
}

static void ConfigDisplayPower(
    const NvOdmPeripheralConnectivity *conn,
    NvOdmServicesPmuHandle hPmu,
    NvU32 *rail_mask,
    NvBool IsPowerEnable)
{
    NvU32 i;
    if (IsPowerEnable)
    {
        for (i = 0; i < conn->NumAddress; i++)
        {
            if (conn->AddressList[i].Interface == NvOdmIoModule_Vdd)
            {
                NvOdmServicesPmuVddRailCapabilities cap;
                NvU32 settle_us;
                NvU32 mv;

                /* address is the vdd rail id */
                NvOdmServicesPmuGetCapabilities(hPmu,
                    conn->AddressList[i].Address, &cap);

                /* Only enable a rail that's off -- save the rail state and
                 * restore it at the end of this function. this will prevent
                 * issues of accidentally disabling rails that were enabled
                 * by the bootloader.
                 */
                mv = 0;
                NvOdmServicesPmuGetVoltage(hPmu, conn->AddressList[i].Address,
                                        &mv);
                if (mv)
                    continue;

                *rail_mask |= (1UL << i);

                /* set the rail volatage to the recommended */
                NvOdmServicesPmuSetVoltage(hPmu,
                    conn->AddressList[i].Address, cap.requestMilliVolts,
                    &settle_us);

                /* wait for the rail to settle */
                if (settle_us)
                    NvOdmOsWaitUS(settle_us);
            }
        }
    }
    else
    {
        for (i = 0; i < conn->NumAddress; i++)
        {
            if ((*rail_mask & (1UL << i)) &&
                (conn->AddressList[i].Interface == NvOdmIoModule_Vdd))
            {
                /* set the rail volatage to the recommended */
                NvOdmServicesPmuSetVoltage(hPmu,
                    conn->AddressList[i].Address, NVODM_VOLTAGE_OFF, 0);
            }
        }
    }
}

static NvBool
ReadEdid_I2c(NvOdmServicesI2cHandle hOdmI2c,
    NvU32 DispI2cAdd, NvU8 segment, NvU32 addr, NvU8 *out)
{
    NvU8 buffer[EDID_FRAME_SIZE + 2]; // 2 extra bytes for segment, etc.
    NvU8 *b;
    NvOdmI2cTransactionInfo trans[3];
    NvU32 TransCount = 0;
    NvOdmI2cStatus  status = NvOdmI2cStatus_Success;
    NvU8 *rb;

    b = &buffer[0];

    if (segment)
    {
        *b = segment;
        trans[TransCount].Address = EDID_SEGMENT_ADD;
        trans[TransCount].Buf = b;
        trans[TransCount].Flags = NVODM_I2C_IS_WRITE | NVODM_I2C_USE_REPEATED_START;
        trans[TransCount].NumBytes = 1;
        b++;
        TransCount++;
    }

    *b = addr;
    trans[TransCount].Address = DispI2cAdd;
    trans[TransCount].Buf = b;
    trans[TransCount].Flags = NVODM_I2C_IS_WRITE | NVODM_I2C_USE_REPEATED_START;
    trans[TransCount].NumBytes = 1;
    b++;
    TransCount++;

    rb = b;
    trans[TransCount].Address = DispI2cAdd;
    trans[TransCount].Buf = rb;
    trans[TransCount].Flags = 0;
    trans[TransCount].NumBytes = EDID_FRAME_SIZE;
    TransCount++;

    status = NvOdmI2cTransaction(hOdmI2c, trans, TransCount,
                          I2C_SPEED_KHZ_EDID_READ, I2C_TRANSACTION_TIMEOUT_MS);
    if (status == NvOdmI2cStatus_Success)
    {
        NvOdmOsMemcpy(out, rb, EDID_FRAME_SIZE);
        return NV_TRUE;
    }
    return NV_FALSE;
}

static NvBool SetDisplaySizeTypeBasedOnEdid(void)
{
    const NvOdmPeripheralConnectivity *conn = NULL;
    const NvOdmIoAddress *addrlist = NULL;
    NvOdmServicesI2cHandle hOdmI2c = NULL;
    NvOdmServicesPmuHandle hPmu = NULL;
    NvU8 raw[EDID_FRAME_SIZE];
    NvU32 rail_mask = 0;
    NvBool ret;
    NvU32 addr, inst;
    NvU32 i;

    conn = NvOdmPeripheralGetGuid(LVDS_WSVGA_GUID);
    if (!conn)
        return NV_FALSE;

    for(i = 0; i < conn->NumAddress; i++)
    {
        if (conn->AddressList[i].Interface == NvOdmIoModule_I2c)
        {
            addrlist = &conn->AddressList[i];
            break;
        }
    }

    if (!addrlist)
        return NV_FALSE;

    addr = addrlist->Address;
    inst = addrlist->Instance;

    /* enable the power rails (may already be enabled, which is fine) */
    hPmu = NvOdmServicesPmuOpen();
    if (!hPmu)
    {
        ret = NV_FALSE;
        goto PmuFail;
    }

    hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, inst);
    if (!hOdmI2c)
    {
        ret = NV_FALSE;
        goto OdmI2cFail;
    }

    ConfigDisplayPower(conn, hPmu, &rail_mask, NV_TRUE);

    /* read the raw edid bytes */
    if (!ReadEdid_I2c(hOdmI2c, addr, 0, 0, &raw[0]))
    {
        ret = NV_FALSE;
        goto ReadFail;
    }
    if ((raw[EDID_HORIZ_SIZE_INDEX] == 0x16) &&
                     (raw[EDID_VERT_SIZE_INDEX] == 0xd))
        s_is_lvds_wsga_mode = NV_TRUE;
    else
        s_is_lvds_wsga_mode = NV_FALSE;
    ret = NV_TRUE;

ReadFail:
    ConfigDisplayPower(conn, hPmu, &rail_mask, NV_FALSE);
    NvOdmI2cClose(hOdmI2c);
OdmI2cFail:
    NvOdmServicesPmuClose(hPmu);
PmuFail:
    return ret;
}

void lvds_wsga_GetHal( NvOdmDispDeviceHandle hDevice )
{
    static NvBool IsFirstTime = NV_TRUE;
    NV_ASSERT(hDevice);

    if (IsFirstTime)
    {
        //SetDisplaySizeTypeBasedOnEdid();
        IsFirstTime = NV_FALSE;
    }


    hDevice->Setup = lvds_wsga_Setup;
    hDevice->Release = lvds_wsga_Release;
    hDevice->ListModes = lvds_wsga_ListModes;
    hDevice->SetMode = lvds_wsga_SetMode;
    hDevice->SetPowerLevel = lvds_wsga_SetPowerLevel;
    hDevice->GetParameter = lvds_wsga_GetParameter;
    hDevice->GetPinPolarities = lvds_wsga_GetPinPolarities;
    hDevice->GetConfiguration = lvds_wsga_GetConfiguration;
    hDevice->SetSpecialFunction = lvds_wsga_SetSpecialFunction;

    hDevice->BacklightSetup = lvds_wsga_BacklightSetup;
    hDevice->BacklightRelease = lvds_wsga_BacklightRelease;
    hDevice->BacklightIntensity = lvds_wsga_SetBacklightIntensity;

    // Backlight functions
    hDevice->SetBacklight = lvds_wsga_SetBacklight;
}
