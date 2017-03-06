/*
 * Copyright (c) 2008-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_disp.h"
#include "panel_samsung_test_dsi.h"
#include "nvodm_services.h"
#include "nvodm_query_pinmux.h"
#include "nvodm_query_discovery.h"
#include "nvodm_query_gpio.h"
#include "nvos.h"
#include "nvodm_sdio.h"
#include "nvodm_services.h"
#include "nvodm_pmu.h"

#if defined(DISP_OAL)
#include "nvbl_debug.h"
#else
#define DSI_PANEL_DEBUG(x) NvOsDebugPrintf x
#endif
#define DSI_PANEL_DEBUG(x) NvOsDebugPrintf x

//set to 0 for 1st DSI instance and 1 for 2nd DSI instance
#define DSI_INSTANCE 0

#define TEGRA_DC_OUT_CONTINUOUS_MODE 0
#define TEGRA_DC_OUT_ONESHOT_MODE 1

#define DC_CTRL_MODE    TEGRA_DC_OUT_ONESHOT_MODE

#define PROC_BOARD_E1239    0x0C27

typedef struct DsiPanelDataRec
{
    NvU32 DsiCmd;
    NvU32 PanelReg;
    NvU32 DataSize;
    NvU8 *pData;
    NvBool IsLongPacket;
} DsiPanelData;

static NvBool samsungTestDsi_Setup( NvOdmDispDeviceHandle hDevice );
static void samsungTestDsi_Release( NvOdmDispDeviceHandle hDevice );
static void samsungTestDsi_ListModes( NvOdmDispDeviceHandle hDevice, NvU32 *nModes,
                NvOdmDispDeviceMode *modes );
static NvBool samsungTestDsi_SetMode( NvOdmDispDeviceHandle hDevice,
                NvOdmDispDeviceMode *mode, NvU32 flags );
static void samsungTestDsi_SetBacklight( NvOdmDispDeviceHandle hDevice, NvU8 intensity );
static void samsungTestDsi_SetPowerLevel( NvOdmDispDeviceHandle hDevice,
                NvOdmDispDevicePowerLevel level );
static NvBool samsungTestDsi_GetParameter( NvOdmDispDeviceHandle hDevice,
                NvOdmDispParameter param, NvU32 *val );
static void * samsungTestDsi_GetConfiguration( NvOdmDispDeviceHandle hDevice );
static void samsungTestDsi_GetPinPolarities( NvOdmDispDeviceHandle hDevice, NvU32 *nPins,
                NvOdmDispPin *Pins, NvOdmDispPinPolarity *Values );
static NvBool samsungTestDsi_SetSpecialFunction( NvOdmDispDeviceHandle hDevice,
                NvOdmDispSpecialFunction function, void *config );
static void samsungTestDsi_NullMode( NvOdmDispDeviceHandle hDevice );
static void samsungTestDsi_Resume( NvOdmDispDeviceHandle hDevice );
static void samsungTestDsi_Suspend( NvOdmDispDeviceHandle hDevice );
static NvBool samsungTestDsi_PanelInit( NvOdmDispDeviceHandle hDevice, NvOdmDispDeviceMode *mode);

// Color calibration
static NvU32 s_ColorCalibration_Red     = NVODM_DISP_FP_TO_FX(1.00);
static NvU32 s_ColorCalibration_Green   = NVODM_DISP_FP_TO_FX(1.00);
static NvU32 s_ColorCalibration_Blue    = NVODM_DISP_FP_TO_FX(1.00);

// Other global variables.
static NvOdmDispDsiTransport   s_trans;

/********************************************
 * Please use byte clock for the following table
 * Byte Clock to Pixel Clock conversion:
 * The DSI D-PHY calculation:
 * h_total = (h_sync_width + h_back_porch + h_disp_active + h_front_porch)
 * v_total = (v_sync_width + v_back_porch + v_disp_active + v_front_porch)
 * dphy_clk = h_total * v_total * fps * bpp / data_lanes / 2
 ********************************************/
static NvOdmDispDeviceMode samsungTestDsi_Modes[] =
{
    {
        540,                          // width
        960,                          // height
        24,                           // bpp
        0,                            // refresh
        0,                            // frequency
#if (DC_CTRL_MODE == TEGRA_DC_OUT_ONESHOT_MODE)
        NVODM_DISP_MODE_FLAG_USE_TEARING_EFFECT,    // flags for DC one-shot mode
#else
        NVODM_DISP_MODE_FLAG_NONE,    //flags for DC continuous mode
#endif
        // timing
        {
            4,   // h_ref_to_sync,
            1,   // v_ref_to_sync,
            16,   // h_sync_width,
            1,   // v_sync_width,
            32,  // h_back_porch,
            1,   // v_back_porch
            540,   // h_disp_active
            960,   // v_disp_active
            32,  // h_front_porch,
            2,  // v_front_porch
        },
        NV_FALSE // deprecated.
    }
};

static NvBool
samsungTestDsi_Setup( NvOdmDispDeviceHandle hDevice )
{
    if( !hDevice->bInitialized )
    {
        hDevice->modes = samsungTestDsi_Modes;
        hDevice->nModes = NV_ARRAY_SIZE(samsungTestDsi_Modes);

        hDevice->Type = NvOdmDispDeviceType_DSI;
        hDevice->Usage = NvOdmDispDeviceUsage_Primary;
        hDevice->MaxHorizontalResolution = (NvU32)samsungTestDsi_Modes[0].width;
        hDevice->MaxVerticalResolution = (NvU32)samsungTestDsi_Modes[0].height;
        hDevice->BaseColorSize = NvOdmDispBaseColorSize_888;
        hDevice->DataAlignment = NvOdmDispDataAlignment_MSB;
        hDevice->PinMap = NvOdmDispPinMap_Single_Rgb24_Spi5;

        hDevice->power = NvOdmDispDevicePowerLevel_Off;
        samsungTestDsi_NullMode( hDevice );

        hDevice->bInitialized = NV_TRUE;
    }

    return NV_TRUE;
}

static void
samsungTestDsi_NullMode( NvOdmDispDeviceHandle hDevice )
{
    hDevice->CurrentMode.width = 0;
    hDevice->CurrentMode.height = 0;
    hDevice->CurrentMode.bpp = 0;
    hDevice->CurrentMode.flags = (NvU32)-1;
}

static void
samsungTestDsi_Release( NvOdmDispDeviceHandle hDevice )
{
    hDevice->bInitialized = NV_FALSE;
    samsungTestDsi_NullMode( hDevice );
}

static void
samsungTestDsi_ListModes( NvOdmDispDeviceHandle hDevice, NvU32 *nModes,
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
samsungTestDsi_SetMode( NvOdmDispDeviceHandle hDevice,
    NvOdmDispDeviceMode *mode, NvU32 flags )
{
    NV_ASSERT( hDevice );

    if( mode )
    {
        hDevice->CurrentMode = *mode;
    }
    else
    {
        samsungTestDsi_NullMode( hDevice );
    }

    if( hDevice->power != NvOdmDispDevicePowerLevel_On )
    {
        return NV_TRUE;
    }

    /* FIXME: Check more item if necessary */
    if( !mode )
    {
        samsungTestDsi_NullMode( hDevice );
    }

    return NV_TRUE;
}

static NvU8 s_ZeroCmd[] = { 0x00 };
static NvOdmServicesGpioHandle s_hGpio;
static NvOdmGpioPinHandle s_hLCDResPin0;
static NvOdmGpioPinHandle s_hLCDResPin1;
static NvOdmGpioPinHandle s_hLCDResPin2;
static NvOdmGpioPinHandle s_hLCDStereoMode;

static void
samsungTestDsi_SetBacklight( NvOdmDispDeviceHandle hDevice, NvU8 intensity )
{
    NvOdmBoardInfo BoardInfo;

    NvOdmPeripheralGetBoardInfo(0, &BoardInfo);
    /*
     * In either one-shot mode or continuous mode,
     * trigger frame, delay, then,
     * need to enable backlight.
     * without this, garbage screen remained in panel built-in memory
     * is observed in the first bootloader display.
     */
    NvOdmOsWaitUS( 60000 );
    if( !s_hGpio )
        s_hGpio = NvOdmGpioOpen();
    if( !s_hGpio )
    {
        NV_ASSERT( !"Gpio open failed");
        return NV_FALSE;
    }
    if (BoardInfo.BoardID == PROC_BOARD_E1239)
    {
        s_hLCDResPin1 = NvOdmGpioAcquirePinHandle(s_hGpio, (NvU32)('h' - 'a'), 3);
    }
    else
    {
        s_hLCDResPin1 = NvOdmGpioAcquirePinHandle(s_hGpio, (NvU32)('w' - 'a'), 1);
    }
    if( !s_hLCDResPin1 )
    {
        NV_ASSERT( !"Pin handle could not be acquired");
        return NV_FALSE;
    }
    /* On DSIa backlight */
    NvOdmGpioSetState(s_hGpio, s_hLCDResPin1, (intensity == 0)? 0x0 : 0x1);
    NvOdmGpioConfig( s_hGpio, s_hLCDResPin1, NvOdmGpioPinMode_Output);

    if (BoardInfo.BoardID != PROC_BOARD_E1239)
    {
        s_hLCDStereoMode = NvOdmGpioAcquirePinHandle(s_hGpio, (NvU32)('h' - 'a'), 1);
        if( !s_hLCDStereoMode )
        {
            NV_ASSERT( !"Pin handle could not be acquired");
            return NV_FALSE;
        }
        NvOdmGpioSetState(s_hGpio, s_hLCDStereoMode, 0x1); //2d:1 and 3d:0
        NvOdmGpioConfig( s_hGpio, s_hLCDStereoMode, NvOdmGpioPinMode_Output);
    }
}

static void
samsungTestDsi_SetPowerLevel( NvOdmDispDeviceHandle hDevice,
    NvOdmDispDevicePowerLevel level )
{
    NV_ASSERT( hDevice );

    hDevice->power = level;

    switch( level ) {
    case NvOdmDispDevicePowerLevel_Off:
    case NvOdmDispDevicePowerLevel_Suspend:
        samsungTestDsi_Suspend( hDevice );
        break;
    case NvOdmDispDevicePowerLevel_On:
        if( hDevice->CurrentMode.width && hDevice->CurrentMode.height &&
            hDevice->CurrentMode.bpp )
        {
            samsungTestDsi_Resume( hDevice );
        }
        break;
    default:
        break;
    }
}

static NvBool
samsungTestDsi_GetParameter( NvOdmDispDeviceHandle hDevice,
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
    case NvOdmDispParameter_ColorCalibrationRed:
        *val = s_ColorCalibration_Red;
        break;
    case NvOdmDispParameter_ColorCalibrationGreen:
        *val = s_ColorCalibration_Green;
        break;
    case NvOdmDispParameter_ColorCalibrationBlue:
        *val = s_ColorCalibration_Blue;
        break;
    case NvOdmDispParameter_PwmOutputPin:
        *val = NvOdmDispPwmOutputPin_Unspecified;
        break;
    case NvOdmDispParameter_DsiInstance:
        *val = DSI_INSTANCE;
        break;
    default:
        return NV_FALSE;
    }

    return NV_TRUE;
}

static void *
samsungTestDsi_GetConfiguration( NvOdmDispDeviceHandle hDevice )
{
    NvOdmDispDsiConfig *dsi = &hDevice->Config.dsi;
    dsi->DataFormat = NvOdmDsiDataFormat_24BitP;
    dsi->VideoMode = NvOdmDsiVideoModeType_DcDrivenCommandMode;
    dsi->VirChannel = NvOdmDsiVirtualChannel_0;
    dsi->NumOfDataLines = 2;
#if (DC_CTRL_MODE == TEGRA_DC_OUT_ONESHOT_MODE)
    dsi->RefreshRate = 62;
#else
    dsi->RefreshRate = 60;
#endif
    dsi->Freq = 162000;
    dsi->PanelResetTimeoutMSec = 202;
    dsi->LpCommandModeFreqKHz = 4165;
    dsi->HsCommandModeFreqKHz = 20250;
    dsi->HsSupportForFrameBuffer = NV_FALSE;
    dsi->flags = 0;
    dsi->HsClockControl = NvOdmDsiHsClock_TxOnly;
    dsi->EnableHsClockInLpMode = NV_FALSE;
    dsi->bIsPhytimingsValid = NV_FALSE;
    dsi->DsiDisplayMode = NvOdmDispDsiMode_Normal;
    return (void *)dsi;
}

static void
samsungTestDsi_GetPinPolarities( NvOdmDispDeviceHandle hDevice, NvU32 *nPins,
    NvOdmDispPin *Pins, NvOdmDispPinPolarity *Values )
{
    *nPins = 0;
}

static NvBool
samsungTestDsi_SetSpecialFunction( NvOdmDispDeviceHandle hDevice,
    NvOdmDispSpecialFunction function, void *config )
{
    switch( function )
    {
        case NvOdmDispSpecialFunction_DSI_Transport:
            s_trans = *(NvOdmDispDsiTransport *)config;
            break;
        default:
            // Don't bother asserting.
            return NV_FALSE;
    }

    return NV_TRUE;
}

typedef struct NvOdmSdioRec
{
    // NvODM PMU device handle
    NvOdmServicesPmuHandle hPmu;
    // Gpio Handle
    NvOdmServicesGpioHandle hGpio;
    // Pin handle to Wlan Reset Gpio pin
    NvOdmGpioPinHandle hResetPin;
    // Pin handle to Wlan PWR GPIO Pin
    NvOdmGpioPinHandle hPwrPin;
    NvOdmPeripheralConnectivity *pConnectivity;
    // Power state
    NvBool PoweredOn;
    // Instance
    NvU32 Instance;
} NvOdmSdio;

static void
samsungTestDsi_Resume( NvOdmDispDeviceHandle hDevice )
{
    samsungTestDsi_PanelInit( hDevice, &hDevice->CurrentMode );
}

static void
samsungTestDsi_Suspend( NvOdmDispDeviceHandle hDevice )
{
}

static NvBool
samsungTestDsi_PanelInit( NvOdmDispDeviceHandle hDevice, NvOdmDispDeviceMode *mode)
{
    NvBool b;
    NvU32 i;

    static DsiPanelData s_PanelInitSeq[] = {
        // Panel Reg Addr, Data Size, Register Data
        { 0x05,      0x11,   1, s_ZeroCmd, NV_FALSE},
        { 0x00, (NvU32)-1, 150,      NULL, NV_FALSE},
#if (DC_CTRL_MODE == TEGRA_DC_OUT_ONESHOT_MODE)
        { 0x15,      0x35,   1, s_ZeroCmd, NV_FALSE}, //te signal for one-shot mode
#endif
        { 0x05,      0x29,   1, s_ZeroCmd, NV_FALSE},
        { 0x00, (NvU32)-1, 20,      NULL, NV_FALSE},
    };

    NV_ASSERT( mode->width && mode->height && mode->bpp );

    if( !hDevice->bInitialized )
    {
        NV_ASSERT( !"device not initialized" );
        return NV_FALSE;
    }

    if( !s_hGpio )
        s_hGpio = NvOdmGpioOpen();
    if( !s_hGpio )
    {
        NV_ASSERT( !"Gpio open failed");
        return NV_FALSE;
    }

    b = s_trans.NvOdmDispDsiInit( hDevice, DSI_INSTANCE, 0 );
    if( !b )
    {
        NV_ASSERT( !"dsi init failed" );
        return NV_FALSE;
    }

    b = s_trans.NvOdmDispDsiEnableCommandMode( hDevice, DSI_INSTANCE );
    if( !b )
    {
        NV_ASSERT( !"dsi command mode failed" );
        return NV_FALSE;
    }

    s_hLCDResPin2 = NvOdmGpioAcquirePinHandle(s_hGpio, (NvU32)('w' - 'a'), 0);
    if( !s_hLCDResPin2 )
    {
        NV_ASSERT( !"Pin handle could not be acquired");
        return NV_FALSE;
    }

#define DELAY_US 2000
#define DELAY_MS 20
    /* Reset the panel */
    NvOdmGpioConfig( s_hGpio, s_hLCDResPin2, NvOdmGpioPinMode_Output);
    NvOdmGpioSetState(s_hGpio, s_hLCDResPin2, 0x0);
    NvOdmOsWaitUS(DELAY_US);
    NvOdmGpioSetState(s_hGpio, s_hLCDResPin2, 0x1);
    NvOdmOsWaitUS(DELAY_US);

    /* Send panel Init sequence */
    for (i = 0; i < NV_ARRAY_SIZE(s_PanelInitSeq); i++)
    {
        if (s_PanelInitSeq[i].pData == NULL)
        {
            NvOdmOsSleepMS(s_PanelInitSeq[i].DataSize);
            continue;
        }

        NvOdmOsSleepMS(DELAY_MS);

        b = s_trans.NvOdmDispDsiWrite( hDevice,
                s_PanelInitSeq[i].DsiCmd,
                s_PanelInitSeq[i].PanelReg,
                s_PanelInitSeq[i].DataSize,
                s_PanelInitSeq[i].pData,
                DSI_INSTANCE,
                s_PanelInitSeq[i].IsLongPacket );
        if( !b )
        {
            DSI_PANEL_DEBUG(("Error> DSI Panel Initialization Failed\r\n"));
            return NV_FALSE;
        }
    }
    return NV_TRUE;
}

void
samsungTestDsi_GetHal(NvOdmDispDeviceHandle hDevice)
{
    NV_ASSERT(hDevice);

    hDevice->Setup = samsungTestDsi_Setup;
    hDevice->Release = samsungTestDsi_Release;
    hDevice->ListModes = samsungTestDsi_ListModes;
    hDevice->SetMode = samsungTestDsi_SetMode;
    hDevice->SetPowerLevel = samsungTestDsi_SetPowerLevel;
    hDevice->GetParameter = samsungTestDsi_GetParameter;
    hDevice->GetPinPolarities = samsungTestDsi_GetPinPolarities;
    hDevice->GetConfiguration = samsungTestDsi_GetConfiguration;
    hDevice->SetSpecialFunction = samsungTestDsi_SetSpecialFunction;
    hDevice->SetBacklight = samsungTestDsi_SetBacklight;
}

