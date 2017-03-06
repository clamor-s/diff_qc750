/*
 * Copyright (c) 2008 - 2012 NVIDIA Corporation.  All rights reserved.
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
static NvBool samsungTestDsi_Discover( NvOdmDispDeviceHandle hDevice );
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
    // Weird resolution
    {
        864,                          // width
        480,                          // height
        24,                           // bpp
        0,                            // refresh
        0,                            // frequency
        NVODM_DISP_MODE_FLAG_NONE,    // flags
        // timing
        {
            11,   // h_ref_to_sync,
            1,   // v_ref_to_sync,
            16,   // h_sync_width,
            4,   // v_sync_width,
            16,  // h_back_porch,
            4,   // v_back_porch
            864,   // h_disp_active
            480,   // v_disp_active
            16,  // h_front_porch,
            4,  // v_front_porch
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
    if( mode )
    {
        if( hDevice->CurrentMode.width == mode->width &&
            hDevice->CurrentMode.height == mode->height &&
            hDevice->CurrentMode.bpp == mode->bpp &&
            hDevice->CurrentMode.flags == mode->flags)
        {
            return NV_TRUE;
        }

        samsungTestDsi_PanelInit( hDevice, mode );
    }
    else
    {
        samsungTestDsi_NullMode( hDevice );
    }

    return NV_TRUE;
}

static void
samsungTestDsi_SetBacklight( NvOdmDispDeviceHandle hDevice, NvU8 intensity )
{
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
    dsi->RefreshRate = 60;
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

static NvBool
samsungTestDsi_Discover( NvOdmDispDeviceHandle hDevice )
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
    else
        ret = NV_FALSE;

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

    return ret;
}

static void
samsungTestDsi_Resume( NvOdmDispDeviceHandle hDevice )
{
    samsungTestDsi_PanelInit( hDevice, &hDevice->CurrentMode );
}

static void
samsungTestDsi_Suspend( NvOdmDispDeviceHandle hDevice )
{
}

static NvU8 s_ZeroCmd[] = { 0x00 };
static NvU8 s_InitCmd[] = { 0x5A, 0x5A };
static NvOdmServicesGpioHandle s_hGpio;
static NvOdmGpioPinHandle s_hLCDResPin0;
static NvOdmGpioPinHandle s_hLCDResPin1;
static NvOdmGpioPinHandle s_hLCDResPin2;

static NvBool
samsungTestDsi_PanelInit( NvOdmDispDeviceHandle hDevice, NvOdmDispDeviceMode *mode)
{
    NvBool b;
    NvU32 i;

    static DsiPanelData s_PanelInitSeq[] = {
        // Panel Reg Addr, Data Size, Register Data
        { 0x05,      0x01,   1, s_ZeroCmd, NV_FALSE},
        { 0x00, (NvU32)-1,  10,      NULL, NV_FALSE},
        { 0x05,      0x11,   1, s_ZeroCmd, NV_FALSE},
        { 0x00, (NvU32)-1, 130,      NULL, NV_FALSE},
        { 0x39,      0xF0,   2, s_InitCmd,  NV_TRUE},
        { 0x39,      0xF1,   2, s_InitCmd,  NV_TRUE},
        { 0x05,      0x29,   1, s_ZeroCmd, NV_FALSE}
    };

    NV_ASSERT( mode->width && mode->height && mode->bpp );

    if( !hDevice->bInitialized )
    {
        NV_ASSERT( !"device not initialized" );
        return NV_FALSE;
    }

    /* get the peripheral config */
    if( !samsungTestDsi_Discover( hDevice ) )
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
    s_hLCDResPin1 = NvOdmGpioAcquirePinHandle(s_hGpio, (NvU32)('w' - 'a'), 1);
    if( !s_hLCDResPin1 )
    {
        NV_ASSERT( !"Pin handle could not be acquired");
        return NV_FALSE;
    }
    /* On DSIa backlight */
    NvOdmGpioSetState(s_hGpio, s_hLCDResPin1, 0x1);
    NvOdmGpioConfig( s_hGpio, s_hLCDResPin1, NvOdmGpioPinMode_Output);

    s_hLCDResPin0 = NvOdmGpioAcquirePinHandle(s_hGpio, (NvU32)('w' - 'a'), 0);
    if( !s_hLCDResPin0 )
    {
        NV_ASSERT( !"Pin handle could not be acquired");
        return NV_FALSE;
    }

    /* On DSIb backlight */
    NvOdmGpioSetState(s_hGpio, s_hLCDResPin0, 0x1);
    NvOdmGpioConfig( s_hGpio, s_hLCDResPin0, NvOdmGpioPinMode_Output);

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

    s_hLCDResPin2 = NvOdmGpioAcquirePinHandle(s_hGpio, (NvU32)('d' - 'a'), 2);
    if( !s_hLCDResPin2 )
    {
        NV_ASSERT( !"Pin handle could not be acquired");
        return NV_FALSE;
    }

#define DELAY_US 2000
#define DELAY_MS 20
    /* Reset the panel */
    NvOdmGpioSetState(s_hGpio, s_hLCDResPin2, 0x1);
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

