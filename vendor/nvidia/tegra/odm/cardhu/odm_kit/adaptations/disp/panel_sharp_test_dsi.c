/*
 * Copyright (c) 2012-2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_disp.h"
#include "panel_sharp_test_dsi.h"
#include "nvodm_services.h"
#include "nvodm_query_pinmux.h"
#include "nvodm_query_discovery.h"
#include "nvodm_query_gpio.h"
#include "nvos.h"
#include "nvodm_sdio.h"
#include "nvodm_services.h"
#include "nvodm_pmu.h"
#include "nvodm_backlight.h"

#define DSI_PANEL_DEBUG(x) NvOsDebugPrintf x

//set to 0 for 1st DSI instance and 1 for 2nd DSI instance
#define DSI_INSTANCE 0

#define TEGRA_DC_OUT_CONTINUOUS_MODE 0
#define TEGRA_DC_OUT_ONESHOT_MODE 1

#define DC_CTRL_MODE    TEGRA_DC_OUT_ONESHOT_MODE
#define DELAY_MS 20

typedef struct DsiPanelDataRec
{
    NvU32 DsiCmd;
    NvU32 PanelReg;
    NvU32 DataSize;
    NvU8 *pData;
    NvBool IsLongPacket;
} DsiPanelData;

static NvBool sharpTestDsi_Setup( NvOdmDispDeviceHandle hDevice );
static void sharpTestDsi_Release( NvOdmDispDeviceHandle hDevice );
static void sharpTestDsi_ListModes( NvOdmDispDeviceHandle hDevice, NvU32 *nModes,
                NvOdmDispDeviceMode *modes );
static NvBool sharpTestDsi_SetMode( NvOdmDispDeviceHandle hDevice,
                NvOdmDispDeviceMode *mode, NvU32 flags );
static void sharpTestDsi_SetBacklight( NvOdmDispDeviceHandle hDevice, NvU8 intensity );
static void sharpTestDsi_SetBacklightIntensity( NvOdmDispDeviceHandle hDevice, NvU8 intensity );
static void sharpTestDsi_SetPowerLevel( NvOdmDispDeviceHandle hDevice,
                NvOdmDispDevicePowerLevel level );
static NvBool sharpTestDsi_GetParameter( NvOdmDispDeviceHandle hDevice,
                NvOdmDispParameter param, NvU32 *val );
static void * sharpTestDsi_GetConfiguration( NvOdmDispDeviceHandle hDevice );
static void sharpTestDsi_GetPinPolarities( NvOdmDispDeviceHandle hDevice, NvU32 *nPins,
                NvOdmDispPin *Pins, NvOdmDispPinPolarity *Values );
static NvBool sharpTestDsi_SetSpecialFunction( NvOdmDispDeviceHandle hDevice,
                NvOdmDispSpecialFunction function, void *config );
static void sharpTestDsi_NullMode( NvOdmDispDeviceHandle hDevice );
static NvBool sharpTestDsi_Discover( NvOdmDispDeviceHandle hDevice );
static void sharpTestDsi_Resume( NvOdmDispDeviceHandle hDevice );
static void sharpTestDsi_Suspend( NvOdmDispDeviceHandle hDevice );
static NvBool sharpTestDsi_PanelInit( NvOdmDispDeviceHandle hDevice, NvOdmDispDeviceMode *mode);

static NvOdmGpioPinHandle s_hBacklightEnablePin;
static NvOdmGpioPinHandle s_hBacklightIntensityPin;

static NvOdmServicesPwmHandle s_hOdmPwm = NULL;

static NvBool s_bBacklight = NV_FALSE;

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
static NvOdmDispDeviceMode sharpTestDsi_Modes[] =
{
    // 720p, 24 bit
    { 720, // width
      1280, // height
      24, // bpp
      0, // refresh
      0, //frequency
#if (DC_CTRL_MODE == TEGRA_DC_OUT_ONESHOT_MODE)
      NVODM_DISP_MODE_FLAG_USE_TEARING_EFFECT,    // flags for DC one-shot mode
#else
      NVODM_DISP_MODE_FLAG_NONE,    // flags
#endif
      // timing
      { 2,      // h_ref_to_sync
        2,      // v_ref_to_sync
        4,     // h_sync_width
        4,      // v_sync_width
        58,     // h_back_porch
        14,      // v_back_porch
        720,   // h_disp_active
        1280,    // v_disp_active
        4,     // h_front_porch
        4,      // v_front_porch
      },
      NV_FALSE // partial
    }
};

static NvBool
sharpTestDsi_Setup( NvOdmDispDeviceHandle hDevice )
{
    if( !hDevice->bInitialized )
    {
        // Get PWM handle for backlight intensity
        s_hOdmPwm = NvOdmPwmOpen();
        NV_ASSERT(s_hOdmPwm);

        hDevice->modes = sharpTestDsi_Modes;
        hDevice->nModes = NV_ARRAY_SIZE(sharpTestDsi_Modes);

        hDevice->Type = NvOdmDispDeviceType_DSI;
        hDevice->Usage = NvOdmDispDeviceUsage_Primary;
        hDevice->MaxHorizontalResolution = (NvU32)sharpTestDsi_Modes[0].width;
        hDevice->MaxVerticalResolution = (NvU32)sharpTestDsi_Modes[0].height;
        hDevice->BaseColorSize = NvOdmDispBaseColorSize_888;
        hDevice->DataAlignment = NvOdmDispDataAlignment_MSB;
        hDevice->PinMap = NvOdmDispPinMap_Single_Rgb24_Spi5;
        hDevice->PwmOutputPin = NvOdmDispPwmOutputPin_Unspecified;

        hDevice->power = NvOdmDispDevicePowerLevel_Off;
        sharpTestDsi_NullMode( hDevice );

        hDevice->bInitialized = NV_TRUE;
    }

    return NV_TRUE;
}

static void
sharpTestDsi_NullMode( NvOdmDispDeviceHandle hDevice )
{
    hDevice->CurrentMode.width = 0;
    hDevice->CurrentMode.height = 0;
    hDevice->CurrentMode.bpp = 0;
    hDevice->CurrentMode.flags = (NvU32)-1;
}

static void
sharpTestDsi_Release( NvOdmDispDeviceHandle hDevice )
{
    hDevice->bInitialized = NV_FALSE;
    sharpTestDsi_NullMode( hDevice );
}

static void
sharpTestDsi_ListModes( NvOdmDispDeviceHandle hDevice, NvU32 *nModes,
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
sharpTestDsi_SetMode( NvOdmDispDeviceHandle hDevice,
    NvOdmDispDeviceMode *mode, NvU32 flags )
{
    NV_ASSERT( hDevice );

    if( mode )
    {
        hDevice->CurrentMode = *mode;
    }
    else
    {
        sharpTestDsi_NullMode( hDevice );
    }

    if( hDevice->power != NvOdmDispDevicePowerLevel_On )
    {
        return NV_TRUE;
    }

    /* FIXME: Check more item if necessary */
    if( mode )
    {
        while(1);
        if( hDevice->CurrentMode.width == mode->width &&
            hDevice->CurrentMode.height == mode->height &&
            hDevice->CurrentMode.bpp == mode->bpp &&
            hDevice->CurrentMode.flags == mode->flags)
        {
            return NV_TRUE;
        }

        sharpTestDsi_PanelInit( hDevice, mode );
    }
    else
    {
        sharpTestDsi_NullMode( hDevice );
    }

    return NV_TRUE;
}

static NvOdmServicesGpioHandle s_hGpio;
static void
sharpTestDsi_SetBacklight( NvOdmDispDeviceHandle hDevice, NvU8 intensity )
{
    if( !s_bBacklight )
    {
        NvOdmBacklightInit( hDevice, NV_FALSE );
        s_bBacklight = NV_TRUE;
    }
    /* Add delay before initializing backlight to avoid any
       corruption and  give time for panel init to stabilize */
    NvOdmOsSleepMS(DELAY_MS);
    NvOdmBacklightIntensity( hDevice, intensity );
}

static void
sharpTestDsi_SetBacklightIntensity( NvOdmDispDeviceHandle hDevice, NvU8 intensity )
{
    NvU32 RequestedFreq, ReturnedFreq;
    NvU32 DutyCycle;

    RequestedFreq = 1000; // Hz

    // Upper 16 bits of DutyCycle is intensity percentage (whole number portion)
    // Intensity must be scaled from 0 - 255 to a percentage.
    DutyCycle = ((intensity * 100)/255) << 16;

    if (intensity != 0)
    {
        NvOdmPwmConfig(s_hOdmPwm, NvOdmPwmOutputId_PWM0,
                       NvOdmPwmMode_Enable, DutyCycle, &RequestedFreq,
                       &ReturnedFreq);
    }
    else
    {
        NvOdmGpioSetState(s_hGpio, s_hBacklightIntensityPin, 0x0);

        NvOdmPwmConfig(s_hOdmPwm, NvOdmPwmOutputId_PWM0,
                       NvOdmPwmMode_Disable, DutyCycle, &RequestedFreq,
                       &ReturnedFreq);
    }
}

static void
sharpTestDsi_SetPowerLevel( NvOdmDispDeviceHandle hDevice,
    NvOdmDispDevicePowerLevel level )
{
    NV_ASSERT( hDevice );

    hDevice->power = level;

    switch( level ) {
    case NvOdmDispDevicePowerLevel_Off:
    case NvOdmDispDevicePowerLevel_Suspend:
        sharpTestDsi_Suspend( hDevice );
        break;
    case NvOdmDispDevicePowerLevel_On:
        if( hDevice->CurrentMode.width && hDevice->CurrentMode.height &&
            hDevice->CurrentMode.bpp )
        {
            sharpTestDsi_Resume( hDevice );
        }
        break;
    default:
        break;
    }
}

static NvBool
sharpTestDsi_GetParameter( NvOdmDispDeviceHandle hDevice,
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
        *val = hDevice->PwmOutputPin;
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
sharpTestDsi_GetConfiguration( NvOdmDispDeviceHandle hDevice )
{
    NvOdmDispDsiConfig *dsi = &hDevice->Config.dsi;
    dsi->DataFormat = NvOdmDsiDataFormat_24BitP;
    dsi->VideoMode = NvOdmDsiVideoModeType_DcDrivenCommandMode;
    dsi->VirChannel = NvOdmDsiVirtualChannel_0;
    dsi->NumOfDataLines = 2;
#if (DC_CTRL_MODE == TEGRA_DC_OUT_ONESHOT_MODE)
    dsi->RefreshRate = 66;
#else
    dsi->RefreshRate = 60;
#endif
    dsi->Freq = 162000;
    dsi->PanelResetTimeoutMSec = 202;
    dsi->LpCommandModeFreqKHz = 4165;
    dsi->HsCommandModeFreqKHz = 20250;
    dsi->HsSupportForFrameBuffer = NV_FALSE;
    dsi->flags = 0;
    dsi->HsClockControl = NvOdmDsiHsClock_Continuous;
    dsi->EnableHsClockInLpMode = NV_FALSE;
    dsi->bIsPhytimingsValid = NV_FALSE;
    dsi->DsiDisplayMode = NvOdmDispDsiMode_Normal;
    return (void *)dsi;
}

static void
sharpTestDsi_GetPinPolarities( NvOdmDispDeviceHandle hDevice, NvU32 *nPins,
    NvOdmDispPin *Pins, NvOdmDispPinPolarity *Values )
{
    *nPins = 0;
}

static NvBool
sharpTestDsi_SetSpecialFunction( NvOdmDispDeviceHandle hDevice,
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

static NvBool
sharpTestDsi_Discover( NvOdmDispDeviceHandle hDevice )
{
    NvOdmServicesI2cHandle hOdmI2c;
    NvOdmI2cTransactionInfo trans[3];
    NvU8 buf1[8];
    NvBool ret=NV_TRUE, status;
    NvOdmPeripheralConnectivity const *conn;
    static NvOdmSdio *pDevice = NULL;
    NvU32 SettlingTime = 0;
    NvOdmServicesPmuVddRailCapabilities RailCaps;

    pDevice = NvOdmOsAlloc(sizeof(NvOdmSdio));
    if(pDevice == NULL)
        return NV_FALSE;
    pDevice->hPmu = NvOdmServicesPmuOpen();
    if(pDevice->hPmu == NULL)
    {
        NvOdmOsFree(pDevice);
        pDevice = NULL;
        return NV_FALSE;
    }

    /* get the connectivity info */
#define MIPI_GUID NV_ODM_GUID('N', 'V', 'D', 'D', 'M', 'I', 'P', 'I')
    conn = (NvOdmPeripheralConnectivity *)NvOdmPeripheralGetGuid( MIPI_GUID );
    if( !conn )
    {
        return NV_FALSE;
    }

    hDevice->PeripheralConnectivity = conn;
    NvOdmServicesPmuGetCapabilities(pDevice->hPmu, conn->AddressList[0].Address, &RailCaps);
    NvOdmServicesPmuSetVoltage(pDevice->hPmu, conn->AddressList[0].Address,
                                RailCaps.requestMilliVolts, &SettlingTime);
    if (SettlingTime)
    {
        NvOdmOsWaitUS(SettlingTime);
    }

    hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, 4);
    if (!hOdmI2c)
    {
        ret = NV_FALSE;
        goto fail_i2c;
    }

    buf1[0] = 7; // command byte for O/p port 1
    buf1[1] = 0x7f; //making all ones in o/p port 1 register
    trans[0].Address = 0x20 << 1;
    trans[0].Buf = buf1;
    trans[0].Flags = NVODM_I2C_IS_WRITE;
    trans[0].NumBytes = 2;

    status = NvOdmI2cTransaction(hOdmI2c, trans, 1, 40, 5000);
    if (status == NvOdmI2cStatus_Success)
        ret = NV_TRUE;
    else {
        ret = NV_FALSE;
        goto fail_i2c;
    }

    buf1[0] = 3; // command byte for O/p port 1
    buf1[1] = 0x80; //making all ones in o/p port 1 register
    trans[0].Address = 0x20 << 1;
    trans[0].Buf = buf1;
    trans[0].Flags = NVODM_I2C_IS_WRITE;
    trans[0].NumBytes = 2;

    status = NvOdmI2cTransaction(hOdmI2c, trans, 1, 40, 5000);
    if (status == NvOdmI2cStatus_Success)
        ret = NV_TRUE;
    else
        ret = NV_FALSE;

fail_i2c:
    if (hOdmI2c)
        NvOdmI2cClose(hOdmI2c);

    return ret;
}

static void
sharpTestDsi_Resume( NvOdmDispDeviceHandle hDevice )
{
    sharpTestDsi_PanelInit( hDevice, &hDevice->CurrentMode );
}

static void
sharpTestDsi_Suspend( NvOdmDispDeviceHandle hDevice )
{
}

static NvBool
sharpTestDsi_PanelInit( NvOdmDispDeviceHandle hDevice, NvOdmDispDeviceMode *mode)
{
    NvBool b;
    NvU32 i;
    static NvU8 s_ZeroCmd[] = { 0x00 };
    static NvU8 s_InitCmd1[] = { 0x0c};
    static NvU8 s_InitCmd2[] = { 0x11 };
    static NvU8 s_PwdCmd[] = { 0xB9, 0xff, 0x83, 0x92 };
    static DsiPanelData s_PanelInitSeq[] = {
        // Panel Reg Addr, Data Size, Register Data
        { 0x05,      0x11,   1, s_ZeroCmd, NV_FALSE},
        { 0x00, (NvU32)-1,  160,      NULL, NV_FALSE},
#if (DC_CTRL_MODE == TEGRA_DC_OUT_ONESHOT_MODE)
        { 0x15,      0x35,   1, s_ZeroCmd, NV_FALSE}, //te signal for one-shot mode
#endif
        { 0x39,      0x0,   4, s_PwdCmd,  NV_TRUE},
        { 0x00, (NvU32)-1,  10,      NULL, NV_FALSE},
        { 0x15,      0xd4,   1, s_InitCmd1, NV_FALSE},
        { 0x00, (NvU32)-1,  10,      NULL, NV_FALSE},
        { 0x15,      0xba,   1, s_InitCmd2, NV_FALSE},
        { 0x00, (NvU32)-1,  10,      NULL, NV_FALSE},
        { 0x05,      0x29,   1, s_ZeroCmd, NV_FALSE},
        { 0x00, (NvU32)-1,  150,      NULL, NV_FALSE},
    };
    static NvOdmGpioPinHandle s_hResPin;

    NV_ASSERT( mode->width && mode->height && mode->bpp );
    if( !hDevice->bInitialized )
    {
        NV_ASSERT( !"device not initialized" );
        return NV_FALSE;
    }
    /* get the peripheral config */
    if( !sharpTestDsi_Discover( hDevice ) )
    {
        NV_ASSERT( !"failed to find guid in discovery database" );
        return NV_FALSE;
    }

    s_hGpio = NvOdmGpioOpen();
    if( !s_hGpio )
    {
        NV_ASSERT( !"Gpio open failed");
        return NV_FALSE;
    }

    /* Turn on MIPI_BR_VDDIO_1V8 */
    s_hResPin = NvOdmGpioAcquirePinHandle(s_hGpio, (NvU32)('w' - 'a'), 1);
    if( !s_hResPin )
    {
        NV_ASSERT( !"Pin handle could not be acquired");
        return NV_FALSE;
    }
    NvOdmGpioSetState(s_hGpio, s_hResPin, 0x1);
    NvOdmGpioConfig( s_hGpio, s_hResPin, NvOdmGpioPinMode_Output);

    /* Turn on DISP_AVDD_3V2 which is the panel's 3.2V rail */
    s_hResPin = NvOdmGpioAcquirePinHandle(s_hGpio, (NvU32)('h' - 'a'), 1);
    if( !s_hResPin )
    {
        NvOdmOsPrintf( "Pin handle could not be acquired");
        return NV_FALSE;
    }
    NvOdmGpioConfig( s_hGpio, s_hResPin, NvOdmGpioPinMode_Output);
    NvOdmGpioSetState(s_hGpio, s_hResPin, 0x1);

    NvOdmOsWaitUS(10000);

    b = s_trans.NvOdmDispDsiInit( hDevice, DSI_INSTANCE, 0 );
    if( !b )
    {
        NvOdmOsPrintf( "dsi init failed" );
        NV_ASSERT( !"dsi init failed" );
        return NV_FALSE;
    }
    b = s_trans.NvOdmDispDsiEnableCommandMode( hDevice, DSI_INSTANCE );
    if( !b )
    {
        NvOdmOsPrintf( "dsi command mode failed" );
        NV_ASSERT( !"dsi command mode failed" );
        return NV_FALSE;
    }

    s_hResPin = NvOdmGpioAcquirePinHandle(s_hGpio, (NvU32)('d' - 'a'), 2);
    if( !s_hResPin )
    {
        NV_ASSERT( !"Pin handle could not be acquired");
        return NV_FALSE;
    }

    /* Reset the panel */
    NvOdmGpioSetState(s_hGpio, s_hResPin, 0x1);
    NvOdmGpioConfig( s_hGpio, s_hResPin, NvOdmGpioPinMode_Output);
    NvOdmOsWaitUS(1000);
    NvOdmGpioSetState(s_hGpio, s_hResPin, 0x0);
    NvOdmOsWaitUS(1000);
    NvOdmGpioSetState(s_hGpio, s_hResPin, 0x1);
    NvOdmOsWaitUS(20000);

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
            NvOdmOsPrintf( "Error> DSI Panel Initialization Failed\n" );
            DSI_PANEL_DEBUG(("Error> DSI Panel Initialization Failed\r\n"));
            return NV_FALSE;
        }
    }

    /* Turn on DSI backlight power */
    s_hBacklightEnablePin = NvOdmGpioAcquirePinHandle(s_hGpio, (NvU32)('h' - 'a'), 2);
    if( !s_hBacklightEnablePin )
    {
        NvOdmOsPrintf( "Pin handle could not be acquired");
        return NV_FALSE;
    }
    NvOdmGpioConfig( s_hGpio, s_hBacklightEnablePin, NvOdmGpioPinMode_Output);
    NvOdmGpioSetState(s_hGpio, s_hBacklightEnablePin, 0x1);
    NvOdmOsSleepMS(DELAY_MS);

    return NV_TRUE;
}

void
sharpTestDsi_GetHal(NvOdmDispDeviceHandle hDevice)
{
    NV_ASSERT(hDevice);

    hDevice->Setup = sharpTestDsi_Setup;
    hDevice->Release = sharpTestDsi_Release;
    hDevice->ListModes = sharpTestDsi_ListModes;
    hDevice->SetMode = sharpTestDsi_SetMode;
    hDevice->SetPowerLevel = sharpTestDsi_SetPowerLevel;
    hDevice->GetParameter = sharpTestDsi_GetParameter;
    hDevice->GetPinPolarities = sharpTestDsi_GetPinPolarities;
    hDevice->GetConfiguration = sharpTestDsi_GetConfiguration;
    hDevice->SetSpecialFunction = sharpTestDsi_SetSpecialFunction;
    hDevice->SetBacklight = sharpTestDsi_SetBacklight;
    hDevice->BacklightIntensity = sharpTestDsi_SetBacklightIntensity;
}

