/*
 * Copyright (c) 2008 - 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "dc_dsi_hal.h"
#include "dc_hal.h"
#include "nvrm_drf.h"
#include "nvassert.h"
#include "nvddk_disp_test.h"
#include "nvddk_disp_structure.h"

// FIXME: set data format by looking at mode's color depth
#if DSI_ENABLE_DEBUG_PRINTS
#define DSI_PRINT(x) NvOsDebugPrintf x
/* Enable the flag to get the dsi error prints */
#define DSI_ERR_PRINT(x) NvOsDebugPrintf x
#else
#define DSI_PRINT(x)
#define DSI_ERR_PRINT(x)
#endif

/* Configures the display clock */
static void DcDsiDisplayClockConfig( DcController *cont, DcDsi *dsi,
    NvOdmDispDeviceHandle hPanel );
static void DcDsiWaitForVSync( NvRmDeviceHandle hRm, DcController* cont);
static NvError
dsi_transport_init(
    DcDsi *dsi,
    NvU32 DsiInstance,
    NvOdmDispDeviceHandle hPanel );
static NvError
PrivDsiTransInit(
    NvOdmDispDeviceHandle hDevice,
    NvU32 DsiInstance,
    NvU32 flags );
static NvError
PrivDsiCommandModeUpdate(
    NvRmSurface *surf,
    NvPoint *pSrc,
    NvRect *pUpdate,
    NvU32 DsiInstance,
    NvOdmDispDeviceHandle hPanel);
NvU32
PrivDsiGetBurstModeSolDelay(DcController *cont, DcDsi *dsi, NvU32 *MipiClkAdjkHz, NvU32 *PanelFreqKHz);

#define DSI_MAX_CONTROLLERS 2
static DcDsi s_Dsi[DSI_MAX_CONTROLLERS];

static DsiCommandInfo s_PartialDispSeq[] =
{
    // command, Panel Reg Addr, Data Size, Register Data
    { DsiCommand_LongWrite, DSI_SET_PARTIAL_AREA, NV_TRUE},
    { DsiCommand_LongWrite, DSI_SET_PIXEL_FORMAT, NV_TRUE},
    { DsiCommand_LongWrite, DSI_SET_ADDRESS_MODE, NV_TRUE},
    { DsiCommand_LongWrite, DSI_SET_COLUMN_ADDRESS, NV_TRUE},
    { DsiCommand_LongWrite, DSI_SET_PAGE_ADDRESS, NV_TRUE},
    { DsiCommand_LongWrite, DSI_WRITE_MEMORY_START, NV_TRUE},
    { DsiCommand_LongWrite, DSI_WRITE_MEMORY_CONTINUE, NV_TRUE},
};

static NvBool PrivDsiIsCommandModeReady( DcDsi *dsi )
{
    NvU32 val = 0;
    NvU32 timeout = 0;

    while ( timeout <= DSI_MAX_COMMAND_DELAY_USEC)
    {
        val = DSI_READ(dsi, DSI_DSI_TRIGGER_0);
        if (!val)
        {
            break;
        }
        NvOsWaitUS(DSI_COMMAND_DELAY_STEPS_USEC);
        timeout += DSI_COMMAND_DELAY_STEPS_USEC;
    }

    if (timeout > DSI_MAX_COMMAND_DELAY_USEC)
    {
        return NV_FALSE;
    }
    NvOsWaitUS(DSI_COMMAND_COMPLETION_DELAY_USEC);
    return NV_TRUE;
}

NvU32
PrivDsiGetBurstModeSolDelay(DcController *cont, DcDsi *dsi, NvU32 *MipiClkAdjkHz, NvU32 *PanelFreqKHz)
{
    NvU32 hTotal = 0;
    NvU32 vTotal = 0;
    NvU32 panel_freq = 0;
    NvU32 DsiToPixelClkRatio = 0;
    NvU32 SolDelay = DSI_DEFAULT_SOL_DELAY;
    NvOdmDispDeviceTiming *timing = &cont->mode.timing;
    NvU32 temp = 0;
    NvU32 temp1 = 0;

    //Calaculate the SOL delay for burst mode;
    hTotal = timing->Horiz_SyncWidth + timing->Horiz_BackPorch +
            timing->Horiz_DispActive + timing->Horiz_FrontPorch;

    vTotal = timing->Vert_SyncWidth + timing->Vert_BackPorch +
             timing->Vert_DispActive + timing->Vert_FrontPorch;

    panel_freq = (hTotal * vTotal * dsi->RefreshRate);

    // Panel frequency in KHz
    panel_freq = panel_freq / 1000;

    // Get Fdsi/Fpixel ration (note: Fdsi si in bit format)
    DsiToPixelClkRatio = ((dsi->PhyFreq * 2 + panel_freq - 1)/(panel_freq));

    // Convert Fdsi to byte format
    DsiToPixelClkRatio *= (1000/8);

    /* Multiplying by 1000 so that we don't loose the fraction part */
    temp = timing->Horiz_DispActive * 1000;
    temp1 = hTotal - timing->Horiz_FrontPorch;

    switch (dsi->DataFormat)
    {
        case NvOdmDsiDataFormat_16BitP:
            // Here 2 is the number of Bytes per pixel.
            /* Burst mode freq should be greater than or eual to the non burst frequncy */
            NV_ASSERT( (DsiToPixelClkRatio * dsi->DataLanes) >= 2000 );
            SolDelay = (temp1 * DsiToPixelClkRatio) -  ((temp * 2) / dsi->DataLanes);
            break;
        case NvOdmDsiDataFormat_18BitP:
            // Here 2.25 is the number of Bytes per pixel.
            /* Burst mode freq should be greater than or eual to the non burst frequncy */
            NV_ASSERT( (DsiToPixelClkRatio * dsi->DataLanes) >= 2250 );
            SolDelay = (temp1 * DsiToPixelClkRatio) -
                        ((temp * 9) / (4 * dsi->DataLanes));
            break;
        case NvOdmDsiDataFormat_18BitNP:
        case NvOdmDsiDataFormat_24BitP:
            // Here 3 is the number of Bytes per pixel.
            /* Burst mode freq should be greater than or eual to the non burst frequncy */
            NV_ASSERT( (DsiToPixelClkRatio * dsi->DataLanes) >= 3000 );
            SolDelay = (temp1 * DsiToPixelClkRatio) -
                        ((temp * 3) / dsi->DataLanes);
            break;
        default:
            break;
    }
    // Do round up on SolDelay
    SolDelay = (SolDelay + 1000 - 1)/1000;

    if( SolDelay > 480*4)
    {
        SolDelay = 480*4;
        if(MipiClkAdjkHz)
        {
            switch (dsi->DataFormat)
            {
                case NvOdmDsiDataFormat_16BitP:
                    *MipiClkAdjkHz = ((SolDelay + (timing->Horiz_DispActive * 2)/ dsi->DataLanes) * panel_freq)/temp1;
                    break;
                case NvOdmDsiDataFormat_18BitP:
                    *MipiClkAdjkHz = ((SolDelay + (timing->Horiz_DispActive * 9)/ (4 * dsi->DataLanes)) * panel_freq)/temp1;
                    break;
                case NvOdmDsiDataFormat_18BitNP:
                case NvOdmDsiDataFormat_24BitP:
                    *MipiClkAdjkHz = ((SolDelay + (timing->Horiz_DispActive * 3)/ dsi->DataLanes) * panel_freq)/temp1;
                    break;
                default:
                    break;
            }
            *MipiClkAdjkHz <<= 2;
        }
    }
    else
    {
        if(MipiClkAdjkHz)
            *MipiClkAdjkHz = dsi->PhyFreq;
    }

    if(PanelFreqKHz)
        *PanelFreqKHz = panel_freq;

    return SolDelay;
}

void
DsiSetupSolDelay( DcController *cont, DcDsi *dsi )
{
    NvU32 SolDelay = DSI_DEFAULT_SOL_DELAY;

    ///Calaculate the SOL delay for burst mode;
    if (dsi->VideoMode == NvOdmDsiVideoModeType_Burst)
    {
        SolDelay = PrivDsiGetBurstModeSolDelay( cont, dsi, NULL, NULL );
    }
    else
    {
        // Non-Burst and Non-Burst with Sync Ends Mode
        switch (dsi->DataFormat)
        {
            case NvOdmDsiDataFormat_16BitP:
                // Here 2 is the number of Bytes per pixel.
                SolDelay = (8 * 2) / dsi->DataLanes;
                break;
            case NvOdmDsiDataFormat_18BitP:
                // Here 2.25 is the number of Bytes per pixel.
                SolDelay = (8 * 9) / (4 * dsi->DataLanes);
                break;
            case NvOdmDsiDataFormat_18BitNP:
            case NvOdmDsiDataFormat_24BitP:
                // Here 3 is the number of Bytes per pixel.
                SolDelay = (8 * 3) / dsi->DataLanes;
                break;
            default:
                break;
        }
    }

    DSI_WRITE( dsi, DSI_DSI_SOL_DELAY_0, SolDelay);
}

/* setup the packet lengths required for the DSI packet sequence*/
void
DsiSetupPacketLength( DcController *cont, DcDsi *dsi )
{
    NvU32 val = 0;
    NvU32 HsyncPacketLength = 0;
    NvU32 HBackPorcPacketLength = 0;
    NvU32 DispActivePacketLength = 0;
    NvU32 HFrontPorcPacketLength = 0;
    NvOdmDispDeviceTiming *timing = &cont->mode.timing;
    NvU32 bytes = 0;

    if(dsi->VideoMode == NvOdmDsiVideoModeType_DcDrivenCommandMode)
    {
        switch (dsi->DataFormat) {
        case NvOdmDsiDataFormat_16BitP:
            bytes = (timing->Horiz_DispActive * 2) + 1;
            break;
        case NvOdmDsiDataFormat_18BitP:
            bytes = ((timing->Horiz_DispActive * 9) / 4) + 1;
            break;
        case NvOdmDsiDataFormat_18BitNP:
        case NvOdmDsiDataFormat_24BitP:
            bytes = (timing->Horiz_DispActive * 3) + 1;
            break;
        default:
            break;
        }

        val = NV_DRF_NUM(DSI, DSI_PKT_LEN_2_3, LENGTH_2, 0) |
                NV_DRF_NUM(DSI, DSI_PKT_LEN_2_3, LENGTH_3, bytes);
        DSI_WRITE( dsi, DSI_DSI_PKT_LEN_2_3_0, val );

        val = NV_DRF_NUM(DSI, DSI_PKT_LEN_4_5, LENGTH_4, 0) |
                NV_DRF_NUM(DSI, DSI_PKT_LEN_4_5, LENGTH_5,  bytes);
        DSI_WRITE( dsi, DSI_DSI_PKT_LEN_4_5_0, val );

        DSI_WRITE( dsi, DSI_DSI_PKT_LEN_0_1_0, 0);

        val = NV_DRF_NUM(DSI, DSI_PKT_LEN_6_7, LENGTH_6, 0) |
                NV_DRF_NUM(DSI, DSI_PKT_LEN_6_7, LENGTH_7, 0x0f0f);
        DSI_WRITE( dsi, DSI_DSI_PKT_LEN_6_7_0, val);
        return;
    }

    switch (dsi->DataFormat) {
    case NvOdmDsiDataFormat_16BitP:
        // Here 2 is the number of Bytes per pixel.
        DispActivePacketLength = timing->Horiz_DispActive * 2;
        HsyncPacketLength = timing->Horiz_SyncWidth * 2;
        HBackPorcPacketLength = timing->Horiz_BackPorch * 2;
        HFrontPorcPacketLength = timing->Horiz_FrontPorch * 2;
        break;
    case NvOdmDsiDataFormat_18BitP:
        // Here 2.25 is the number of Bytes per pixel.
        DispActivePacketLength = (timing->Horiz_DispActive * 9) / 4;
        HsyncPacketLength = (timing->Horiz_SyncWidth * 9) / 4;
        HBackPorcPacketLength = (timing->Horiz_BackPorch * 9) / 4;
        HFrontPorcPacketLength = (timing->Horiz_FrontPorch * 9) / 4;
        break;
    case NvOdmDsiDataFormat_18BitNP:
    case NvOdmDsiDataFormat_24BitP:
        // Here 3 is the number of Bytes per pixel.
        DispActivePacketLength = timing->Horiz_DispActive * 3;
        HsyncPacketLength = timing->Horiz_SyncWidth * 3;
        HBackPorcPacketLength = timing->Horiz_BackPorch * 3;
        HFrontPorcPacketLength = timing->Horiz_FrontPorch * 3;
        break;
    default:
        break;
    }

    if (dsi->VideoMode != NvOdmDsiVideoModeType_NonBurstSyncEnd)
    {
        HBackPorcPacketLength = HBackPorcPacketLength + HsyncPacketLength;
    }

    HsyncPacketLength = HsyncPacketLength - DSI_HSYNC_BLNK_PKT_OVERHEAD;
    HBackPorcPacketLength = HBackPorcPacketLength -
        DSI_HBACK_PORCH_PKT_OVERHEAD;
    HFrontPorcPacketLength = HFrontPorcPacketLength -
        DSI_HFRONT_PORCH_PKT_OVERHEAD;

    // Setup Length 0 with zero length packet
    val = NV_DRF_NUM(DSI, DSI_PKT_LEN_0_1, LENGTH_0, 0);
    // Setup Length 1 with Horizontal sync Active delay width
    val |= NV_DRF_NUM(DSI, DSI_PKT_LEN_0_1, LENGTH_1, HsyncPacketLength);
    /* update the register */
    DSI_WRITE( dsi, DSI_DSI_PKT_LEN_0_1_0, val );

    // Setup horizontal Back Porch width "h_back_porch"
    val = NV_DRF_NUM(DSI, DSI_PKT_LEN_2_3, LENGTH_2, HBackPorcPacketLength);
    // Setup horizontal active display width "h_disp_active"
    val |= NV_DRF_NUM(DSI, DSI_PKT_LEN_2_3, LENGTH_3, DispActivePacketLength);
    /* update the register */
    DSI_WRITE( dsi, DSI_DSI_PKT_LEN_2_3_0, val );

    // Setup horizontal Front Porch width "h_front_porch"
    val = NV_DRF_NUM(DSI, DSI_PKT_LEN_4_5, LENGTH_4, HFrontPorcPacketLength);
    // Setup Length 5 with zero length packet
    val |= NV_DRF_NUM(DSI, DSI_PKT_LEN_4_5, LENGTH_5, 0);
    /* update the register */
    DSI_WRITE( dsi, DSI_DSI_PKT_LEN_4_5_0, val );

    // Setup Length 6 with zero length packet, Not Used
    val |= NV_DRF_NUM(DSI, DSI_PKT_LEN_6_7, LENGTH_6, 0);
    // Setup Length 7 with zero length packet, Not Used
    val |= NV_DRF_NUM(DSI, DSI_PKT_LEN_6_7, LENGTH_7, 0);
    /* update the register */
    DSI_WRITE( dsi, DSI_DSI_PKT_LEN_6_7_0, val );
}

void
DsiSetupPhyTiming( DcController *cont, DcDsi *dsi, NvOdmDispDeviceHandle hPanel )
{
    NvU32 HighSpeedTxTimeOut = 0xFFFF; //programming to max value.
    NvU32 NumOfBytesInFrame = 0;
    NvU32 FrameSize = 0;
    NvU32 val = 0;
    const NvOdmDispDsiConfig *pOdmDsiInfo;
    NvU32 PRTimeout = DSI_PR_TO_VALUE;
    NvU32 tlpx = 0;

    pOdmDsiInfo = (NvOdmDispDsiConfig *)NvOdmDispGetConfiguration( hPanel );
    if( pOdmDsiInfo->bIsPhytimingsValid )
    {
        val =
            NV_DRF_NUM(DSI, DSI_PHY_TIMING_0, DSI_THSDEXIT,
                                        (pOdmDsiInfo->PhyTiming.ThsExit)) |
            NV_DRF_NUM(DSI, DSI_PHY_TIMING_0, DSI_THSTRAIL,
                                        (pOdmDsiInfo->PhyTiming.ThsTrail)) |
            NV_DRF_NUM(DSI, DSI_PHY_TIMING_0, DSI_TDATZERO,
                                        (pOdmDsiInfo->PhyTiming.TdatZero)) |
            NV_DRF_NUM(DSI, DSI_PHY_TIMING_0, DSI_THSPREPR,
                                        (pOdmDsiInfo->PhyTiming.ThsPrepr));
        DSI_WRITE(dsi, DSI_DSI_PHY_TIMING_0_0, val);

        val =
            NV_DRF_NUM(DSI, DSI_PHY_TIMING_1, DSI_TCLKTRAIL,
                                        (pOdmDsiInfo->PhyTiming.TclkTrail)) |
            NV_DRF_NUM(DSI, DSI_PHY_TIMING_1, DSI_TCLKPOST,
                                        (pOdmDsiInfo->PhyTiming.TclkPost)) |
            NV_DRF_NUM(DSI, DSI_PHY_TIMING_1, DSI_TCLKZERO,
                                        (pOdmDsiInfo->PhyTiming.TclkZero)) |
            NV_DRF_NUM(DSI, DSI_PHY_TIMING_1, DSI_TTLPX,
                                        (pOdmDsiInfo->PhyTiming.Ttlpx));
        DSI_WRITE(dsi, DSI_DSI_PHY_TIMING_1_0, val);
        tlpx = pOdmDsiInfo->PhyTiming.Ttlpx;
    }
    else
    {
        val = NV_DRF_NUM( DSI, DSI_PHY_TIMING_0, DSI_THSDEXIT,
            DSI_PHY_TIMING_DIV( 100, dsi->freq ) )
            | NV_DRF_NUM( DSI, DSI_PHY_TIMING_0, DSI_THSTRAIL,
            ( 3 + (DSI_PHY_TIMING_DIV( ( DSI_THSTRAIL_VAL( dsi->freq ) ),
            dsi->freq ) ) ) )
            | NV_DRF_NUM( DSI, DSI_PHY_TIMING_0, DSI_TDATZERO,
            DSI_PHY_TIMING_DIV( ( ( 145 + ( 5 * (DSI_TBIT( dsi->freq ) ) ) ) ),
            dsi->freq ) );

        // THSPREP value to minimum 1
        val |= NV_DRF_NUM( DSI, DSI_PHY_TIMING_0, DSI_THSPREPR,
            ( DSI_PHY_TIMING_DIV( ( 65 + 5 * DSI_TBIT( dsi->freq ) ),
            dsi->freq ) == 0 ) ?  1 :
            ( DSI_PHY_TIMING_DIV( ( 65 + 5 * DSI_TBIT( dsi->freq ) ),
            dsi->freq ) ) );

        DSI_WRITE( dsi, DSI_DSI_PHY_TIMING_0_0, val );

        tlpx = ( DSI_PHY_TIMING_DIV( 50, dsi->freq ) == 0 ) ? 1 :
            ( DSI_PHY_TIMING_DIV( 50, dsi->freq ) );

        // Setup DSI D-PHY timing register 1
        val = NV_DRF_NUM( DSI, DSI_PHY_TIMING_1, DSI_TCLKTRAIL,
            DSI_PHY_TIMING_DIV( 60, dsi->freq ) )
            | NV_DRF_NUM( DSI, DSI_PHY_TIMING_1, DSI_TCLKPOST,
            DSI_PHY_TIMING_DIV( ( 60 + (52 * DSI_TBIT( dsi->freq ) ) ),
            dsi->freq ) )
            | NV_DRF_NUM( DSI, DSI_PHY_TIMING_1, DSI_TCLKZERO,
            DSI_PHY_TIMING_DIV( 170, dsi->freq ) )
            | NV_DRF_NUM( DSI, DSI_PHY_TIMING_1, DSI_TTLPX, tlpx );

        DSI_WRITE( dsi, DSI_DSI_PHY_TIMING_1_0, val );
    }

    // Setup DSI D-PHY timing register 2
    DSI_WRITE( dsi, DSI_DSI_PHY_TIMING_2_0,
        DSI_TIMEOUT_VALUE( DSI_ULPM_WAKEUP_TIME_MSEC, dsi->freq ) );

    // Setup DSI D-PHY Bus-Turn-Around timing
    val = NV_DRF_NUM( DSI, DSI_BTA_TIMING, DSI_TTAGET, ( 5 * tlpx ) )
        | NV_DRF_NUM( DSI, DSI_BTA_TIMING, DSI_TTASURE, ( 2 * tlpx ) )
        | NV_DRF_NUM( DSI, DSI_BTA_TIMING, DSI_TTAGO, ( 4 * tlpx ) );

    DSI_WRITE( dsi, DSI_DSI_BTA_TIMING_0, val );

    if( cont )
    {
        NvOdmDispDeviceTiming *timing = &cont->mode.timing;
        // Calaculate the frame size in pixels
        // Frame Size = Horizontal Toatal * Virtical Total
        FrameSize = ((timing->Horiz_SyncWidth + timing->Horiz_BackPorch +
                      timing->Horiz_DispActive + timing->Horiz_FrontPorch) *
                     (timing->Vert_SyncWidth + timing->Vert_BackPorch +
                      timing->Vert_DispActive + timing->Vert_FrontPorch)
                    );

        // Setup Timeout values
        switch(dsi->DataFormat)
        {
            case NvOdmDsiDataFormat_16BitP:
                 NumOfBytesInFrame = FrameSize * 2;
                break;
            case NvOdmDsiDataFormat_18BitP:
                 NumOfBytesInFrame = (FrameSize * 9) / 4;
                break;
            case NvOdmDsiDataFormat_18BitNP:
            case NvOdmDsiDataFormat_24BitP:
                 NumOfBytesInFrame = FrameSize * 3;
                break;
            default:
                break;
        }

        HighSpeedTxTimeOut = NumOfBytesInFrame/DSI_CYCLE_COUNTER_VALUE;
        HighSpeedTxTimeOut =
                ((HighSpeedTxTimeOut / dsi->DataLanes) + DSI_HTX_TO_MARGIN)
                    & 0xffff;
    }

    // Setup DSI Time out terminal count register 0.
    val = NV_DRF_NUM(DSI, DSI_TIMEOUT_0, LRXH_TO, DSI_LRXH_TO_VALUE) |
          NV_DRF_NUM(DSI, DSI_TIMEOUT_0, HTX_TO, HighSpeedTxTimeOut);
    DSI_WRITE(dsi, DSI_DSI_TIMEOUT_0_0, val);

    if( dsi->PanelResetTimeoutMSec )
    {
        PRTimeout = DSI_TIMEOUT_VALUE( dsi->PanelResetTimeoutMSec, dsi->freq );
    }

    // Setup DSI Time out terminal count register 1.
    val =
        NV_DRF_NUM(DSI, DSI_TIMEOUT_1, PR_TO, PRTimeout) |
        NV_DRF_NUM(DSI, DSI_TIMEOUT_1, TA_TO, DSI_TA_TO_VALUE);
   DSI_WRITE(dsi, DSI_DSI_TIMEOUT_1_0, val);

    // Setup DSI Time out tally register.
    // Each time one of the time out counters reaches its terminal count,
    // it will increment the associated tally register.
    val = NV_DRF_NUM(DSI, DSI_TO_TALLY, P_RESET_STATUS, 0x0) |
          NV_DRF_NUM(DSI, DSI_TO_TALLY, TA_TALLY, DSI_TA_TALLY_VALUE) |
          NV_DRF_NUM(DSI, DSI_TO_TALLY, LRXH_TALLY, DSI_LRXH_TALLY_VALUE) |
          NV_DRF_NUM(DSI, DSI_TO_TALLY, HTX_TALLY, DSI_HTX_TALLY_VALUE);
   DSI_WRITE(dsi, DSI_DSI_TO_TALLY_0, val);
}

/* setup the packet sequence required for the DSI */
void
DsiSetupPacketSequence( DcDsi *dsi)
{
    NvU32 val = 0;
    DsiCommand ActiveDisplayCmd = DsiCommand_HActiveLength24Bpp;

    if(dsi->VideoMode ==  NvOdmDsiVideoModeType_DcDrivenCommandMode)
    {
        DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_0_LO_0, val );
        DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_1_LO_0, val );
        DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_2_LO_0, val );
        DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_4_LO_0, val );

        /* Line type 3 */
        /* setting up Horizontal sync start,  Horizontal sync Active,
           Horizontal sync End, Horizontal back Porch, Active display width and
           Horizontal front porch*/
        val = NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_LO, SEQ_3_FORCE_LP, DISABLE) |
              NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_LO, PKT_32_EN, DISABLE) |
              NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_32_ID, 0) |
              NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_32_SIZE, 0) |
              NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_LO, PKT_31_EN, ENABLE) |
              NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_31_ID, 0x08) |
              NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_31_SIZE, 7) |
              NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_LO, PKT_30_EN, ENABLE) |
              NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_30_ID, DsiCommand_LongWrite) |
              NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_30_SIZE, 3);
        /* update the register */
        DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_3_LO_0, val );
        DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_5_LO_0, val );


        val = NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_HI, PKT_35_EN, DISABLE) |
              NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_35_ID, 0) |
              NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_35_SIZE, 0) |
              NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_HI, PKT_34_EN, DISABLE) |
              NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_34_ID, 0) |
              NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_34_SIZE, 0) |
              NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_HI, PKT_33_EN, DISABLE) |
              NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_33_ID,
                                        DsiCommand_Blanking) |
              NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_33_SIZE, 4);
        /* update the register */
        DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_3_HI_0, val );
        DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_5_HI_0, val );
        return;
    }

    switch (dsi->DataFormat)
    {
        case NvOdmDsiDataFormat_16BitP:
            ActiveDisplayCmd = DsiCommand_HActiveLength16Bpp;
            break;
        case NvOdmDsiDataFormat_18BitP:
            ActiveDisplayCmd = DsiCommand_HActiveLength18Bpp;
            break;
        case NvOdmDsiDataFormat_18BitNP:
            ActiveDisplayCmd = DsiCommand_HActiveLength18BppNp;
            break;
        case NvOdmDsiDataFormat_24BitP:
            ActiveDisplayCmd = DsiCommand_HActiveLength24Bpp;
            break;
        default:
            NV_ASSERT( !"unknown data format" );
            break;
    }

    /* Line type 0 */
    /* setting up vertical sync start and blank period in first seq. pkt*/
    val = NV_DRF_NUM(DSI, DSI_PKT_SEQ_0_LO, PKT_00_SIZE, 0) |
        NV_DRF_NUM(DSI, DSI_PKT_SEQ_0_LO, PKT_00_ID, DsiCommand_VSyncStart) |
        NV_DRF_DEF(DSI, DSI_PKT_SEQ_0_LO, SEQ_0_FORCE_LP, ENABLE) |
        NV_DRF_DEF(DSI, DSI_PKT_SEQ_0_LO, PKT_00_EN, ENABLE);
    /* update the register */
    DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_0_LO_0, val );

    /* Line type 2 */
    /* setting up Horizontal sync start in third seq. pkt*/
    val = NV_DRF_NUM(DSI, DSI_PKT_SEQ_2_LO, PKT_20_SIZE, 0) |
          NV_DRF_NUM(DSI, DSI_PKT_SEQ_2_LO, PKT_20_ID, DsiCommand_HSyncStart) |
          NV_DRF_DEF(DSI, DSI_PKT_SEQ_2_LO, SEQ_2_FORCE_LP, ENABLE) |
          NV_DRF_DEF(DSI, DSI_PKT_SEQ_2_LO, PKT_20_EN, ENABLE);
    /* update the register */
    DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_2_LO_0, val );

    /* Line type 4 */
    /* setting up Horizontal sync start in third seq. pkt*/
    val = NV_DRF_NUM(DSI, DSI_PKT_SEQ_4_LO, PKT_40_SIZE, 0) |
          NV_DRF_NUM(DSI, DSI_PKT_SEQ_4_LO, PKT_40_ID, DsiCommand_HSyncStart) |
          NV_DRF_DEF(DSI, DSI_PKT_SEQ_4_LO, SEQ_4_FORCE_LP, ENABLE) |
          NV_DRF_DEF(DSI, DSI_PKT_SEQ_4_LO, PKT_40_EN, ENABLE);
    /* update the register */
    DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_4_LO_0, val );


    if (dsi->VideoMode == NvOdmDsiVideoModeType_NonBurstSyncEnd)
    {
        /* Line type 1 */
        /* setting up Vertical sync end in Second seq. pkt*/
        val = NV_DRF_NUM(DSI, DSI_PKT_SEQ_1_LO, PKT_10_SIZE, 0)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_1_LO, PKT_10_ID,
                DsiCommand_VSyncEnd)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_1_LO, SEQ_1_FORCE_LP, ENABLE)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_1_LO, PKT_10_EN, ENABLE);
        /* update the register */
        DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_1_LO_0, val );

        /* Line type 3 */
        /* setting up Horizontal sync start,  Horizontal sync Active,
         * Horizontal sync End, Horizontal back Porch, Active display width
         * and  Horizontal front porch
         */
        val = NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_LO, SEQ_3_FORCE_LP, DISABLE)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_30_SIZE, 0)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_30_ID,
                DsiCommand_HSyncStart)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_LO, PKT_30_EN, ENABLE)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_31_SIZE, 1)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_31_ID,
                DsiCommand_HSyncActive)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_LO, PKT_31_EN, ENABLE)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_32_SIZE, 0)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_32_ID, DsiCommand_HSyncEnd)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_LO, PKT_32_EN, ENABLE);
        /* update the register */
        DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_3_LO_0, val );

        val = NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_33_SIZE, 2)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_33_ID,
                DsiCommand_HBackPorch)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_HI, PKT_33_EN, ENABLE)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_34_SIZE, 3)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_34_ID, ActiveDisplayCmd)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_HI, PKT_34_EN, ENABLE)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_35_SIZE, 4)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_35_ID,
                DsiCommand_HFrontPorch)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_HI, PKT_35_EN, ENABLE);
        /* update the register */
        DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_3_HI_0, val );

    }
    else if (dsi->VideoMode == NvOdmDsiVideoModeType_NonBurst)
    {
        /* Line type 1 */
        /* setting up Vertical sync end in Second seq. pkt*/
        val = NV_DRF_NUM(DSI, DSI_PKT_SEQ_1_LO, PKT_10_SIZE, 0)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_1_LO, PKT_10_ID,
                DsiCommand_HSyncStart)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_1_LO, SEQ_1_FORCE_LP, ENABLE)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_1_LO, PKT_10_EN, ENABLE);
        /* update the register */
        DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_1_LO_0, val );

        /* Line type 3 */
        /* setting up Horizontal sync start,  Horizontal sync Active,
         * Horizontal sync End, Horizontal back Porch, Active display width
         * and Horizontal front porch
         */
        val = NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_LO, SEQ_3_FORCE_LP, DISABLE)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_30_SIZE, 0)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_30_ID,
                DsiCommand_HSyncStart)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_LO, PKT_30_EN, ENABLE)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_31_SIZE, 2)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_31_ID,
                DsiCommand_HSyncActive)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_LO, PKT_31_EN, ENABLE)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_32_SIZE, 3)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_32_ID, ActiveDisplayCmd)
            |  NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_LO, PKT_32_EN, ENABLE);
        /* update the register */
        DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_3_LO_0, val );

        val = NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_33_SIZE, 4)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_33_ID,
                DsiCommand_Blanking)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_HI, PKT_33_EN, ENABLE)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_34_SIZE, 0)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_34_ID, 0)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_HI, PKT_34_EN, DISABLE)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_35_SIZE, 0)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_35_ID, 0)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_HI, PKT_35_EN, DISABLE);
        /* update the register */
        DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_3_HI_0, val );
    }
    else
    {
        /* Line type 1 */
        /* setting up Vertical sync end in Second seq. pkt*/
        val = NV_DRF_NUM(DSI, DSI_PKT_SEQ_1_LO, PKT_10_SIZE, 0)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_1_LO, PKT_10_ID,
                DsiCommand_HSyncStart)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_1_LO, SEQ_1_FORCE_LP, ENABLE)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_1_LO, PKT_10_EN, ENABLE);
        /* update the register */
        DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_1_LO_0, val );

        /* Line type 3 */
        /* setting up Horizontal sync start,  Horizontal sync Active,
         * Horizontal sync End, Horizontal back Porch, Active display width
         * and Horizontal front porch
         */
        val = NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_LO, SEQ_3_FORCE_LP, ENABLE)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_30_SIZE, 0)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_30_ID,
                DsiCommand_HSyncStart)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_LO, PKT_30_EN, ENABLE)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_31_SIZE, 2)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_31_ID,
                DsiCommand_HSyncActive)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_LO, PKT_31_EN, ENABLE)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_32_SIZE, 3)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_LO, PKT_32_ID, ActiveDisplayCmd)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_LO, PKT_32_EN, ENABLE);
        /* update the register */
        DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_3_LO_0, val );

        val = NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_33_SIZE, 4)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_33_ID,
                DsiCommand_Blanking)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_HI, PKT_33_EN, ENABLE)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_34_SIZE, 0)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_34_ID, 0)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_HI, PKT_34_EN, DISABLE)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_35_SIZE, 0)
            | NV_DRF_NUM(DSI, DSI_PKT_SEQ_3_HI, PKT_35_ID, 0)
            | NV_DRF_DEF(DSI, DSI_PKT_SEQ_3_HI, PKT_35_EN, DISABLE);
        /* update the register */
        DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_3_HI_0, val );
    }

    /* Line type 5 */
    val = DSI_READ(dsi, DSI_DSI_PKT_SEQ_3_LO_0);
    /* update the register */
    DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_5_LO_0, val );

    val = DSI_READ(dsi, DSI_DSI_PKT_SEQ_3_HI_0);
    /* update the register */
    DSI_WRITE( dsi, DSI_DSI_PKT_SEQ_5_HI_0, val );
}

static void
PrivDsiSetHighSpeedMode( DcDsi *dsi, NvOdmDispDeviceHandle hPanel,
    NvBool Enable )
{
    NvU32 RegVal = 0;
    NvU32 cmd_mode_freq_khz = 0;

    // Enable the host clock to access the registers
    HOST_CLOCK_ENABLE(dsi);

    RegVal = DSI_READ(dsi, DSI_HOST_DSI_CONTROL_0);

    if (Enable)
    {
        // Check, to see whether we are in HS mode
        if (NV_DRF_VAL(DSI, HOST_DSI_CONTROL, DSI_HIGH_SPEED_TRANS, RegVal))
        {
            // If Already in High Speed Mode return
            goto clean;
        }
        // Set High Speed mode
        RegVal = NV_FLD_SET_DRF_DEF(DSI,
                    HOST_DSI_CONTROL, DSI_HIGH_SPEED_TRANS, HIGH, RegVal);
        // set the HS command mode Frequency
        cmd_mode_freq_khz = dsi->HsCmdModeFreqKHz;
        dsi->bDsiIsInHsMode = NV_TRUE;
    }
    else
    {
        // Check, to see whether we are in HS mode
        if (NV_DRF_VAL(DSI, HOST_DSI_CONTROL, DSI_HIGH_SPEED_TRANS, RegVal))
        {
            // Set to LS mode
            RegVal = NV_FLD_SET_DRF_DEF(DSI,
                        HOST_DSI_CONTROL, DSI_HIGH_SPEED_TRANS, LOW, RegVal);
            // set the LP command mode Frequency
            cmd_mode_freq_khz = dsi->LpCmdModeFreqKHz;
            dsi->bDsiIsInHsMode = NV_FALSE;
        }
        else
        {
            // If Already in Low Speed Mode return
            goto clean;
        }
    }

    /* Change frequency if current freq is not same as the configured freq */
    if (dsi->freq != cmd_mode_freq_khz)
    {
        // Configure the frequency in comand mode before switching the mode
        NV_ASSERT_SUCCESS(
            NvRmPowerModuleClockConfig( dsi->hRm,
                NVRM_MODULE_ID( NvRmModuleID_Dsi, dsi->DsiInstance ),
                dsi->PowerClientId,
                NvRmFreqUnspecified,
                NvRmFreqUnspecified,
                &cmd_mode_freq_khz, 1, &dsi->freq, NvRmClockConfig_MipiSync)
        );

        /* Setup DSI Phy timming */
        DsiSetupPhyTiming( NULL, dsi, hPanel );
    }

    // Update the dsi with the new mode
    DSI_WRITE(dsi, DSI_HOST_DSI_CONTROL_0, RegVal);

    // wait till the command mode is ready
    if (!PrivDsiIsCommandModeReady(dsi))
    {
        NV_ASSERT(!"H/W seems not working");
    }

clean:
    // Disable the host clock before leaving
    HOST_CLOCK_DISABLE(dsi);
}

static void
PrivDsiEnableCommandMode( DcDsi *dsi, NvOdmDispDeviceHandle hPanel )
{
    NvU32 RegData = 0;
    NvU32 mipi_clock_freq_khz = NVODM_DISP_DSI_COMMAND_MODE_FREQ;
    NvU32 val = 0;

    if (dsi->VideoMode == NvOdmDsiVideoModeType_DcDrivenCommandMode)
    {
        mipi_clock_freq_khz = dsi->LpCmdModeFreqKHz;
        // Configure the frequency in comand mode
        NV_ASSERT_SUCCESS(
        NvRmPowerModuleClockConfig( dsi->hRm,
            NVRM_MODULE_ID( NvRmModuleID_Dsi, dsi->DsiInstance ),
            dsi->PowerClientId,
            NvRmFreqUnspecified,
            NvRmFreqUnspecified,
            &mipi_clock_freq_khz, 1, &dsi->freq, NvRmClockConfig_MipiSync)
        );

        /* Setup DSI Phy timming */
        DsiSetupPhyTiming( NULL, dsi, hPanel );

        /* Setup the DSI host control for sending the Data to the Panel*/
        RegData = NV_DRF_NUM(DSI, HOST_DSI_CONTROL, CRC_RESET, 1) |
              NV_DRF_DEF(DSI, HOST_DSI_CONTROL, DSI_PHY_CLK_DIV, DIV1) |
              NV_DRF_DEF(DSI, HOST_DSI_CONTROL, HOST_TX_TRIG_SRC, IMMEDIATE) |
              NV_DRF_DEF(DSI, HOST_DSI_CONTROL, DSI_ULTRA_LOW_POWER, NORMAL) |
              NV_DRF_DEF(DSI, HOST_DSI_CONTROL, PERIPH_RESET, DISABLE) |

              NV_DRF_DEF(DSI, HOST_DSI_CONTROL, RAW_DATA, DISABLE) |
              NV_DRF_DEF(DSI, HOST_DSI_CONTROL, IMM_BTA, DISABLE) |
              NV_DRF_DEF(DSI, HOST_DSI_CONTROL, PKT_BTA, DISABLE) |
              NV_DRF_DEF(DSI, HOST_DSI_CONTROL, CS_ENABLE, ENABLE) |
              NV_DRF_DEF(DSI, HOST_DSI_CONTROL, ECC_ENABLE, ENABLE);
        RegData |= NV_DRF_DEF(DSI, HOST_DSI_CONTROL, PKT_WR_FIFO_SEL, HOST);

        if (dsi->EnableHsClockInLpMode)
        {
            RegData |= NV_DRF_DEF(DSI, HOST_DSI_CONTROL, DSI_HIGH_SPEED_TRANS, HIGH);
            DSI_WRITE(dsi, DSI_HOST_DSI_CONTROL_0, RegData);

            /* Enable Host Data transfer */
            DSI_WRITE(dsi, DSI_DSI_CONTROL_0,
            NV_DRF_DEF(DSI, DSI_CONTROL, DSI_HOST_ENABLE, ENABLE) |
            NV_DRF_NUM(DSI, DSI_CONTROL, DSI_NUM_DATA_LANES,
                      (dsi->DataLanes - 1)) |
            NV_DRF_DEF(DSI, DSI_CONTROL, DSI_HS_CLK_CTRL, CONTINUOUS));
            dsi->bDsiIsInHsMode = NV_TRUE;
        }
        else
        {
            RegData |= NV_DRF_DEF(DSI, HOST_DSI_CONTROL, DSI_HIGH_SPEED_TRANS, LOW);
            DSI_WRITE(dsi, DSI_HOST_DSI_CONTROL_0, RegData);

            /* Enable Host Data transfer */
            DSI_WRITE(dsi, DSI_DSI_CONTROL_0,
                NV_DRF_DEF(DSI, DSI_CONTROL, DSI_HOST_ENABLE, ENABLE) |
                NV_DRF_NUM(DSI, DSI_CONTROL, DSI_NUM_DATA_LANES,
               (dsi->DataLanes - 1)));
            dsi->bDsiIsInHsMode = NV_FALSE;
        }
    }
    else
    {
         /**
          * Sequence for Switching to host mode from the video mode.
          * 1)Disable DSI interface: This makes the CIL logic to come to idle state
          * 2)Change the configuration to use Host FIFO
          * 3)Update the configuration for Host command mode
          * 4)Enable DSI interface
          */
          /* Disable dsi */
          val = NV_DRF_DEF(DSI, DSI_POWER_CONTROL, LEG_DSI_ENABLE, DISABLE);
          DSI_WRITE( dsi, DSI_DSI_POWER_CONTROL_0, val );
          while(DSI_READ(dsi, DSI_DSI_POWER_CONTROL_0) != val)
              DSI_WRITE(dsi, DSI_DSI_POWER_CONTROL_0, val);
          NvOsWaitUS(DSI_POWER_CONTROL_SETTLE_TIME_US);

          mipi_clock_freq_khz = dsi->LpCmdModeFreqKHz;
          // Configure the frequency in comand mode
          NV_ASSERT_SUCCESS(
               NvRmPowerModuleClockConfig( dsi->hRm,
                     NVRM_MODULE_ID( NvRmModuleID_Dsi, dsi->DsiInstance ),
                     dsi->PowerClientId,
                     NvRmFreqUnspecified,
                     NvRmFreqUnspecified,
                     &mipi_clock_freq_khz,
                     1,
                     &dsi->freq,
                     NvRmClockConfig_MipiSync)
           );

           /* Setup DSI Phy timming */
           DsiSetupPhyTiming( NULL, dsi, hPanel );

           /* Setup the DSI host control for sending the Data to the Panel*/
           RegData = NV_DRF_NUM(DSI, HOST_DSI_CONTROL, CRC_RESET, 1) |
              NV_DRF_DEF(DSI, HOST_DSI_CONTROL, DSI_PHY_CLK_DIV, DIV1) |
              NV_DRF_DEF(DSI, HOST_DSI_CONTROL, HOST_TX_TRIG_SRC, IMMEDIATE) |
              NV_DRF_DEF(DSI, HOST_DSI_CONTROL, DSI_ULTRA_LOW_POWER, NORMAL) |
              NV_DRF_DEF(DSI, HOST_DSI_CONTROL, PERIPH_RESET, DISABLE) |

              NV_DRF_DEF(DSI, HOST_DSI_CONTROL, RAW_DATA, DISABLE) |
              NV_DRF_DEF(DSI, HOST_DSI_CONTROL, IMM_BTA, DISABLE) |
              NV_DRF_DEF(DSI, HOST_DSI_CONTROL, PKT_BTA, DISABLE) |
              NV_DRF_DEF(DSI, HOST_DSI_CONTROL, CS_ENABLE, ENABLE) |
              NV_DRF_DEF(DSI, HOST_DSI_CONTROL, ECC_ENABLE, ENABLE);
           RegData |= NV_DRF_DEF(DSI, HOST_DSI_CONTROL, PKT_WR_FIFO_SEL, HOST);

           if (dsi->EnableHsClockInLpMode)
           {
                  RegData |= NV_DRF_DEF(DSI, HOST_DSI_CONTROL, DSI_HIGH_SPEED_TRANS, HIGH);
                           DSI_WRITE(dsi, DSI_HOST_DSI_CONTROL_0, RegData);

                  /* Enable Host Data transfer */
                  DSI_WRITE(dsi, DSI_DSI_CONTROL_0,
                     NV_DRF_DEF(DSI, DSI_CONTROL, DSI_HOST_ENABLE, ENABLE) |
                     NV_DRF_NUM(DSI, DSI_CONTROL, DSI_NUM_DATA_LANES, (dsi->DataLanes - 1)) |
                  NV_DRF_DEF(DSI, DSI_CONTROL, DSI_HS_CLK_CTRL, CONTINUOUS));
                  dsi->bDsiIsInHsMode = NV_TRUE;
           }
           else
           {
                 RegData |= NV_DRF_DEF(DSI, HOST_DSI_CONTROL, DSI_HIGH_SPEED_TRANS, LOW);
                 DSI_WRITE(dsi, DSI_HOST_DSI_CONTROL_0, RegData);

                 /* Enable Host Data transfer */
                DSI_WRITE(dsi, DSI_DSI_CONTROL_0,
                        NV_DRF_DEF(DSI, DSI_CONTROL, DSI_HOST_ENABLE, ENABLE) |
                        NV_DRF_NUM(DSI, DSI_CONTROL, DSI_NUM_DATA_LANES,
                       (dsi->DataLanes - 1)));
                dsi->bDsiIsInHsMode = NV_FALSE;
           }

           /* enable dsi */
           val = NV_DRF_DEF(DSI, DSI_POWER_CONTROL, LEG_DSI_ENABLE, ENABLE);
           DSI_WRITE( dsi, DSI_DSI_POWER_CONTROL_0, val );

           while(DSI_READ(dsi, DSI_DSI_POWER_CONTROL_0) != val)
           DSI_WRITE(dsi, DSI_DSI_POWER_CONTROL_0, val);
      }

      /* Setup DSI Threshhold for host mode */
      DSI_WRITE(dsi, DSI_DSI_MAX_THRESHOLD_0 , DSI_HOST_FIFO_DEPTH);

      /* reset the trigger and CRC */
      DSI_WRITE(dsi, DSI_DSI_TRIGGER_0 ,0);
      DSI_WRITE(dsi, DSI_DSI_TX_CRC_0 ,0);
      DSI_WRITE(dsi, DSI_DSI_INIT_SEQ_CONTROL_0, 0);

      if (dsi->EnableHsClockInLpMode)
      {
         RegData = DSI_READ(dsi, DSI_HOST_DSI_CONTROL_0);
         RegData = NV_FLD_SET_DRF_DEF(DSI, HOST_DSI_CONTROL,
            DSI_HIGH_SPEED_TRANS, LOW, RegData);
         DSI_WRITE(dsi, DSI_HOST_DSI_CONTROL_0, RegData);
     }
     dsi->HostCtrlEnable = NV_TRUE;
     dsi->bDsiIsInHsMode = NV_FALSE;
}

static void
PrivDsiChangeDataFormat( NvU8* pData, NvU32 DataSize, NvU32 bpp)
{
    NvU32 PixelCount = 0;
    NvU8 temp = 0;
    NvU8* ptr = pData;

    // Format the bytes for 16bpp in the specific format
    // as specified in the spec
    if ( bpp == 16 )
    {
        PixelCount = DataSize >> 1;
        NV_ASSERT((PixelCount << 1) == DataSize);

        while (PixelCount)
        {
            temp = *ptr;
            *ptr =  *(ptr + 1);
            *(ptr + 1) = temp;
            ptr += 2;
            PixelCount--;
        }
    }
}

#if DSI_ENABLE_DEBUG_PRINTS
static void PrivDsiPrintPhyTimings( DcDsi *dsi)
{
    NvU32 val = 0;

    NvOsDebugPrintf("DSI_TBIT[%d] DSI_TBYTE[%d]\n", DSI_TBIT(dsi->freq),
        DSI_TBYTE(dsi->freq));

    val = DSI_READ(dsi, DSI_DSI_PHY_TIMING_0_0);
    NvOsDebugPrintf("NVRM_DSI DSI_DSI_PHY_TIMING_0_0[0x%x]\n", val);

    val = DSI_READ(dsi, DSI_DSI_PHY_TIMING_1_0);
    NvOsDebugPrintf("NVRM_DSI DSI_DSI_PHY_TIMING_1_0[0x%x]\n", val);

    // Setup DSI D-PHY timing register 2
    val = DSI_READ(dsi, DSI_DSI_PHY_TIMING_2_0);
    NvOsDebugPrintf("NVRM_DSI DSI_DSI_PHY_TIMING_2_0[0x%x]\n", val);

    val = DSI_READ(dsi, DSI_DSI_BTA_TIMING_0);
    NvOsDebugPrintf("NVRM_DSI DSI_DSI_BTA_TIMING_0[0x%x]\n", val);

    val = DSI_READ(dsi, DSI_DSI_TIMEOUT_0_0);
    NvOsDebugPrintf("NVRM_DSI DSI_DSI_TIMEOUT_0_0[0x%x]\n", val);

    val = DSI_READ(dsi, DSI_DSI_TIMEOUT_1_0);
    NvOsDebugPrintf("NVRM_DSI DSI_DSI_TIMEOUT_1_0[0x%x]\n", val);

    val = DSI_READ(dsi, DSI_DSI_TO_TALLY_0);
    NvOsDebugPrintf("NVRM_DSI DSI_DSI_TO_TALLY_0[0x%x]\n", val);
}
#endif
#if DSI_USE_SYNC_POINTS
static NvError PrivDsiInit( DcDsi *dsi )
{
    NvError e = NvSuccess;
    NvRmModuleID module[1];

    /* allocate a semaphore */
    NV_CHECK_ERROR_CLEANUP(
        NvOsSemaphoreCreate(&dsi->sem, 0)
    );

    module[0] = NVRM_MODULE_ID( NvRmModuleID_Dsi, dsi->DsiInstance );
    /* get a channel (need one for the sync point api) */
    NV_CHECK_ERROR_CLEANUP(
        NvRmChannelOpen( dsi->hRm, &dsi->ch, 1, module )
    );

    /* get the hardware mutex */
    dsi->ChannelMutex = NvRmChannelGetModuleMutex( NvRmModuleID_Dsi, dsi->DsiInstance );

    /* get a sync point */
    NV_CHECK_ERROR_CLEANUP(
        NvRmChannelGetModuleSyncPoint( dsi->ch,
            NVRM_MODULE_ID( NvRmModuleID_Dsi, dsi->DsiInstance ),
            0,
            &dsi->SyncPointId )
    );

    dsi->SyncPointShadow = NvRmChannelSyncPointRead( dsi->hRm,
        dsi->SyncPointId );

    return e;

fail:
    NvRmChannelClose(dsi->ch);
    dsi->ch = NULL;
    NvOsSemaphoreDestroy(dsi->sem);
    dsi->sem = NULL;

    return e;
}

static NvError
PrivDsiWaitOnTriggerComplete( DcDsi *dsi )
{
    NvU32 val = 0;
    NvError e = NvSuccess;

    // take the lock
    NvRmChannelModuleMutexLock( dsi->hRm, dsi->ChannelMutex );

    dsi->SyncPointShadow++;
    val = NV_DRF_DEF(DSI, INCR_SYNCPT, COND, OP_DONE) |
            NV_DRF_NUM(DSI, INCR_SYNCPT, INDX, dsi->SyncPointId);
    DSI_WRITE(dsi, DSI_INCR_SYNCPT_0, val);

    NvRmChannelModuleMutexUnlock( dsi->hRm, dsi->ChannelMutex );

    /* Trigger the DSI host data to send out the data from the FIFO to Panel */
    DSI_WRITE(dsi, DSI_DSI_TRIGGER_0 ,
          NV_DRF_NUM(DSI, DSI_TRIGGER, DSI_HOST_TRIGGER, 1));

    e = NvRmChannelSyncPointWaitTimeout( dsi->hRm, dsi->SyncPointId,
        dsi->SyncPointShadow, dsi->sem, DSI_MAX_SYNC_POINT_WAIT_MSEC );
    if (e != NvSuccess)
    {
        NV_ASSERT(!"DSI Trigger not done\n");
    }
    return e;
}

static void
PrivDsiWaitOnImmBtaComplete( DcDsi *dsi )
{
    NvU32 val = 0;
    NvError e;

    val = DSI_READ( dsi, DSI_HOST_DSI_CONTROL_0);
    val = NV_FLD_SET_DRF_DEF(DSI, HOST_DSI_CONTROL, IMM_BTA, ENABLE, val);
    DSI_WRITE( dsi, DSI_HOST_DSI_CONTROL_0, val);

    // take the lock
    NvRmChannelModuleMutexLock( dsi->hRm, dsi->ChannelMutex );

    dsi->SyncPointShadow++;
    val = NV_DRF_DEF(DSI, INCR_SYNCPT, COND, OP_DONE) |
            NV_DRF_NUM(DSI, INCR_SYNCPT, INDX, dsi->SyncPointId);
    DSI_WRITE(dsi, DSI_INCR_SYNCPT_0, val);

    NvRmChannelModuleMutexUnlock( dsi->hRm, dsi->ChannelMutex );

    e = NvRmChannelSyncPointWaitTimeout( dsi->hRm, dsi->SyncPointId,
        dsi->SyncPointShadow, dsi->sem, DSI_MAX_SYNC_POINT_WAIT_MSEC );
    if (e != NvSuccess)
    {
        NV_ASSERT(!"DSI IMM_BTA not done\n");
    }
}

#endif

static NvBool PrivDsiIsTeSignled( DcDsi *dsi )
{
    NvError e = NvSuccess;
    if (dsi->GpioIntrHandle)
    {
        // enable the Te signal interrupt
        NvRmGpioInterruptMask(dsi->GpioIntrHandle, NV_FALSE);

        // Do frame update after receiving the signal from the panel
        e = NvOsSemaphoreWaitTimeout(dsi->TeSignalSema,
                                    DSI_TE_WAIT_TIMEOUT_MSEC);
        if (e == NvSuccess)
        {
            return NV_TRUE;
        }
    }
    return NV_FALSE;
}

static void PrivDsiGpioInterruptHandler(void *arg)
{
    DcDsi *dsi = (DcDsi *)arg;

    NvRmGpioInterruptDone(dsi->GpioIntrHandle);

    // disable the Te signal interrupt
    NvRmGpioInterruptMask(dsi->GpioIntrHandle, NV_TRUE);

    // signal the sema
    NvOsSemaphoreSignal(dsi->TeSignalSema);
}

// FIXME: Move this function to RM init
static NvError PrivDsiInitPadCnfg(DcDsi *dsi)
{
    NvError e = NvSuccess;
    NvRmPhysAddr PhysAdr;
    NvU32 *pVirAdr = NULL;
    NvU32 BankSize;
    NvU32 val = 0;

    // Get the secure scratch base address
    NvRmModuleGetBaseAddress( dsi->hRm, NVRM_MODULE_ID(NvRmModuleID_Vi, 0),
        &PhysAdr, &BankSize);

    // Map the physycal memory to virtual memory
    NV_CHECK_ERROR_CLEANUP(
        NvRmPhysicalMemMap( PhysAdr, BankSize, NVOS_MEM_READ_WRITE,
            NvOsMemAttribute_Uncached, (void **)&pVirAdr)
    );

    // Enable the Voltage
    NV_CHECK_ERROR_CLEANUP(
        NvRmPowerVoltageControl( dsi->hRm, NvRmModuleID_Vi, dsi->PowerClientId,
            NvRmVoltsUnspecified, NvRmVoltsUnspecified, NULL, 0, NULL)
    );

    // Emable the clock to vi
    NV_CHECK_ERROR_CLEANUP(
        NvRmPowerModuleClockControl(dsi->hRm, NvRmModuleID_Vi, dsi->PowerClientId,
            NV_TRUE)
    );

    val = NV_READ32((pVirAdr + CSI_CIL_PAD_CONFIG0_0));
    val = NV_FLD_SET_DRF_DEF(CSI, CIL_PAD_CONFIG0, PAD_CIL_PDVREG, SW_DEFAULT,
        val);
    NV_WRITE32((pVirAdr + CSI_CIL_PAD_CONFIG0_0), val);

fail:

    // Disable the clock
    NV_ASSERT_SUCCESS(
        NvRmPowerModuleClockControl(dsi->hRm, NvRmModuleID_Vi, dsi->PowerClientId,
            NV_FALSE)
    );

    // Disable the Voltage
    NV_ASSERT_SUCCESS(
        NvRmPowerVoltageControl(dsi->hRm, NvRmModuleID_Vi, dsi->PowerClientId,
            NvRmVoltsOff, NvRmVoltsOff, NULL, 0, NULL)
    );

    // UnMap the virtual Address
    NvRmPhysicalMemUnmap(pVirAdr, BankSize);

    return e;
}

static void
PrivDsiSaveAndEnableHostEnable( DcDsi *dsi, NvU32* orgHostDsiCtrl,
    NvU32* orgDsiCtrl )
{
    NvU32 val = 0;

    /* save DSI_HOST_DSI_CONTROL_0 */
    *orgHostDsiCtrl = DSI_READ( dsi, DSI_HOST_DSI_CONTROL_0 );
    /* save DSI_DSI_CONTROL_0 */
    *orgDsiCtrl = DSI_READ( dsi, DSI_DSI_CONTROL_0 );

    /* enable immediate trigger mode */
    val = NV_FLD_SET_DRF_DEF( DSI, HOST_DSI_CONTROL, HOST_TX_TRIG_SRC,
        IMMEDIATE, *orgHostDsiCtrl );
    DSI_WRITE( dsi, DSI_HOST_DSI_CONTROL_0, val );

    /* enable host_enable */
    val = NV_FLD_SET_DRF_DEF( DSI, DSI_CONTROL, DSI_HOST_ENABLE, ENABLE,
        *orgDsiCtrl );
    /* disable host_video */
    val = NV_FLD_SET_DRF_DEF( DSI, DSI_CONTROL, DSI_VID_ENABLE, DISABLE, val );
    /* disable DCS command insertion */
    val = NV_FLD_SET_DRF_DEF( DSI, DSI_CONTROL, VID_DCS_ENABLE, DISABLE, val );
    DSI_WRITE( dsi, DSI_DSI_CONTROL_0, val );
}

static void
PrivDsiRestoreHostEnable( DcDsi *dsi, NvU32 orgHostDsiCtrl, NvU32 orgDsiCtrl )
{
    /* restore DSI_HOST_DSI_CONTROL_0 and DSI_DSI_CONTROL_0 */
    DSI_WRITE( dsi, DSI_HOST_DSI_CONTROL_0, orgHostDsiCtrl );
    DSI_WRITE( dsi, DSI_DSI_CONTROL_0, orgDsiCtrl );
}

static void
dsi_set_clock(
    DcDsi *dsi,
    NvU32 DsiClockFreqKHz,
    NvU32 PixelClockFreqKHz,
    NvU32* DsiConfiguredFreqKHz )
{
    NvU32 RequestedFreqKHz = DsiClockFreqKHz;
    NvU32 freq = 0;

    if( RequestedFreqKHz )
    {
        NV_ASSERT(DsiConfiguredFreqKHz);

        NvRmPowerModuleClockControl( dsi->hRm,
            NVRM_MODULE_ID( NvRmModuleID_Dsi, dsi->DsiInstance ),
            dsi->PowerClientId,
            NV_TRUE );

        NV_ASSERT_SUCCESS(
            NvRmPowerModuleClockConfig( dsi->hRm,
                NVRM_MODULE_ID( NvRmModuleID_Dsi, dsi->DsiInstance ),
                dsi->PowerClientId,
                (RequestedFreqKHz * 99) / 100,
                (RequestedFreqKHz * 109) / 100 ,
                &RequestedFreqKHz,
                1,
                &dsi->freq,
                NvRmClockConfig_MipiSync)
        );

        NV_ASSERT_SUCCESS(
            NvRmPowerModuleClockConfig( dsi->hRm,
                NVRM_MODULE_ID( NvRmModuleID_Display, dsi->ControllerIndex ),
                dsi->PowerClientId,
                (PixelClockFreqKHz * 99) / 100,
                (PixelClockFreqKHz * 109) / 100 ,
                &PixelClockFreqKHz,
                1,
                &freq,
                (NvRmClockConfig_MipiSync | NvRmClockConfig_InternalClockForPads))
        );
        *DsiConfiguredFreqKHz = dsi->freq;
    }
    else
    {
        // disable the clock to the controller
        NvRmPowerModuleClockControl( dsi->hRm,
            NVRM_MODULE_ID( NvRmModuleID_Dsi, dsi->DsiInstance ),
            dsi->PowerClientId,
            NV_FALSE );
    }
}

NvError
dsi_write_data( DcDsi *dsi, NvU32 CommandType, NvU8 RegAddr, NvU8 *pData,
    NvU32 DataSize, NvBool IsLongPacket )
{
    NvU32 Ecc =0;
    NvU32 RegData = 0;
    NvU32 DataIdx = 0;
    NvU32 ByteShift = 0;
    NvU32 OrgHostDsiCtrl = 0;
    NvU32 OrgDsiCtrl = 0;
    NvError e = NvSuccess;

    NV_ASSERT(dsi);
    NV_ASSERT(pData);

    if( !dsi->bInit )
    {
        return NvError_NotInitialized;
    }

    NvOsMutexLock(dsi->DsiTransportMutex);

    HOST_CLOCK_ENABLE(dsi);

    if ( !dsi->HostCtrlEnable )
    {
        PrivDsiSaveAndEnableHostEnable( dsi, &OrgHostDsiCtrl, &OrgDsiCtrl );
    }

    if (!PrivDsiIsCommandModeReady(dsi))
    {
        NV_ASSERT(!"H/W seems not working");
    }

    if (!IsLongPacket)
    {
        /* DSI Short Packet */
        /* Short Packet Format: Byte0 : [B7-6] Virtual Channel ID
                                        [B5-0] DSI command
           Byte1: DSI Packet Data Byte0
           Byte2: DSI Packet Data Byte1
           Byte3: ECC Data. Note!! We are Using H/W ECC in the DSI controller.
         */
        RegData = ((Ecc << 24) |
                   (pData[0] << 16) |
                   (RegAddr << 8) |
                   ((dsi->VirChannel << DSI_VIR_CHANNEL_BIT_POSITION) |
                    CommandType));
        /* Write Data to the DSI FIFO */
        DSI_WRITE(dsi, DSI_DSI_WR_DATA_0 , RegData);
    }
    else
    {
        /* DSI Long Packet:  */
        /* First Four Bytes forms the Packet header and
           subsequent bytes contain the packet data.
           Long Packet Format:
           Byte0 : [B7-6] Virtual Channel ID
                                        [B5-0] DSI command
           Byte1: DSI Packet Data Size(WordCount) Byte0
           Byte2: DSI Packet Data Size(WordCount) Byte1
           Byte3: ECC Data. Note!! We are Using H/W ECC in the DSI controller.
           Byte4: DSI Packet Data 0
           Byte5: DSI Packet Data 1
           ByteN: DSI Packet Data N, where N is WordCount.
         */
        RegData = ((Ecc << 24) |
                   (((DataSize + 1) & 0xFFFF) << 8) |
                   ((dsi->VirChannel << DSI_VIR_CHANNEL_BIT_POSITION) |
                    CommandType));
        /* Write Packet header to the DSI FIFO */
        DSI_WRITE(dsi, DSI_DSI_WR_DATA_0 , RegData);
        /* Write the DSI register address at the lower byte */
        RegData = RegAddr;
        ByteShift = 1;
        while (DataSize)
        {
            /* fast write */
            if (DataSize >= 4 && ByteShift == 0)
            {
                RegData = (pData[DataIdx]) | (pData[DataIdx+1]<<8) |
                    (pData[DataIdx+2]<<16) | (pData[DataIdx+3]<<24);
                DSI_WRITE(dsi, DSI_DSI_WR_DATA_0 , RegData);
                RegData = 0;
                DataSize -= 4;
                DataIdx += 4;
                ByteShift = 0;
            }
            else
            {
                RegData |= (pData[DataIdx++] << (ByteShift++ << 3));
                DataSize--;
                /* Write data to the DSI FIFO, when one word is framed out of
                 * data or the end of the data size
                 */
                if ((ByteShift == 4) || (!DataSize))
                {
                    DSI_WRITE(dsi, DSI_DSI_WR_DATA_0 , RegData);
                    RegData = 0;
                    ByteShift = 0;
                }
            }
        }
    }

    if ( NV_TRUE == dsi->bDsiIsInHsMode )
    {
        RegData = DSI_CMD_HS_EOT_PACKAGE;
        DSI_WRITE( dsi, DSI_DSI_WR_DATA_0 , RegData );
    }

#if DSI_USE_SYNC_POINTS
    e = PrivDsiWaitOnTriggerComplete( dsi );
    if (e != NvSuccess)
    {
     DSI_ERR_PRINT(("Write trigger failed ..\n"));
    }
#else
    /* Trigger the DSI host data to send out the data from the FIFO to Panel */
    DSI_WRITE(dsi, DSI_DSI_TRIGGER_0 ,
              NV_DRF_NUM(DSI, DSI_TRIGGER, DSI_HOST_TRIGGER, 1));

    if (!PrivDsiIsCommandModeReady(dsi))
    {
        NV_ASSERT(!"H/W seems not working");
    }
#endif

    if ( !dsi->HostCtrlEnable )
    {
        PrivDsiRestoreHostEnable( dsi, OrgHostDsiCtrl, OrgDsiCtrl );
    }

    HOST_CLOCK_DISABLE(dsi);
    NvOsMutexUnlock(dsi->DsiTransportMutex);

    return e;
}

#define DSI_READ_FIFO_DEPTH (32 << 2)
#define DSI_DELAY_FOR_READ_FIFO 5
NvError
dsi_bta_check_error( DcDsi *dsi )
{
    NvError status = NvError_Timeout;
    NvU32 val = 0;
    NvU32 PollTime = 0;
    NvU32 i = 0;
    NvU32 WordCount = 0;
    NvU32 OrgHostDsiCtrl = 0;
    NvU32 OrgDsiCtrl = 0;

    NV_ASSERT(dsi);

    if( !dsi->bInit )
    {
        return NvError_NotInitialized;
    }

    NvOsMutexLock(dsi->DsiTransportMutex);

    HOST_CLOCK_ENABLE(dsi);

    if (!dsi->HostCtrlEnable)
    {
        PrivDsiSaveAndEnableHostEnable( dsi, &OrgHostDsiCtrl, &OrgDsiCtrl );
    }

    val = DSI_READ(dsi, DSI_DSI_STATUS_0);
    val = NV_DRF_VAL(DSI, DSI_STATUS, RD_FIFO_COUNT, val);
    if(val)
    {
        DSI_ERR_PRINT(("Read Fifo is not empty in the start of read[%d]..\n", val));
        status = NvError_Timeout;
        goto fail;
    }

#if DSI_USE_SYNC_POINTS
    PrivDsiWaitOnImmBtaComplete( dsi );
    val = DSI_READ(dsi, DSI_HOST_DSI_CONTROL_0);
    val = NV_DRF_VAL(DSI, HOST_DSI_CONTROL, IMM_BTA, val);
    if (val)
    {
        NV_ASSERT(!"IMM_BTA not cleared");
    }
#else
    val = DSI_READ( dsi, DSI_HOST_DSI_CONTROL_0);
    val = NV_FLD_SET_DRF_DEF(DSI, HOST_DSI_CONTROL, IMM_BTA, ENABLE, val);
    DSI_WRITE( dsi, DSI_HOST_DSI_CONTROL_0, val);

    while (PollTime <  DSI_STATUS_POLLING_DURATION_USEC)
    {
        val = DSI_READ(dsi, DSI_HOST_DSI_CONTROL_0);
        val = NV_DRF_VAL(DSI, HOST_DSI_CONTROL, IMM_BTA, val);
        if (!val)
        {
            break;
        }
        NvOsWaitUS(DSI_STATUS_POLLING_DELAY_USEC);
        PollTime += DSI_STATUS_POLLING_DELAY_USEC;
    }
#endif

    if (!val)
    {
        PollTime = 0;
        while (PollTime <  DSI_DELAY_FOR_READ_FIFO)
        {
            NvOsSleepMS(1);
            val = DSI_READ(dsi, DSI_DSI_STATUS_0);
            WordCount =
              NV_DRF_VAL(DSI, DSI_STATUS, RD_FIFO_COUNT, val);
            if ((WordCount << 2) > DSI_READ_FIFO_DEPTH)
            {
                DSI_ERR_PRINT(("Read fifo full..\n"));
                break;
            }
            PollTime++;
        }
    }
    else
    {
        DSI_ERR_PRINT(("IMM_BTA timeout..\n"));
        status = NvError_Timeout;
        goto fail;
    }

    if( ( val >> 8 ) & 3 )
    {
        DSI_ERR_PRINT(("OVERFLOW/UNDERFLOW error [%d]..\n", val));
        status = NvError_Timeout;
        goto fail;
    }

    if ( ( WordCount != 0 ) )
    {
        DSI_ERR_PRINT(("Error on BTA read [0x%x]..\n", WordCount));
        status = NvError_Timeout;

        for (i = 0; i < WordCount; i++)
        {
            val = DSI_READ(dsi, DSI_DSI_RD_DATA_0);

            if ( ( 0 == i ) && ( 1 == WordCount ) )
            {
                /* mask out bit[31:24] and check if the error can be ignore */
                if ( DSI_BTA_ACK_SINGLE_ECC_ERR == ( val & 0x00FFFFFF ) )
                {
                    status = NvSuccess;
                    break;
                }
                // Test code for checking moto panel. Remove it before
                // checking in the code
                if ( DSI_BTA_ACK_FALSE_CTRL_ERR == ( val & 0x00FFFFFF ) )
                {
                    status = NvSuccess;
                    break;
                }
            }
            DSI_ERR_PRINT(("Error msg[%d] = 0x%08x\n", i, val));
        }
        if ( NvSuccess != status )
        {
            goto fail;
        }
    }
    status = NvSuccess;

fail:
    if (!dsi->HostCtrlEnable)
    {
        PrivDsiRestoreHostEnable( dsi, OrgHostDsiCtrl, OrgDsiCtrl );
    }
    HOST_CLOCK_DISABLE(dsi);
    NvOsMutexUnlock(dsi->DsiTransportMutex);
    return status;
}

NvError
dsi_read_data( DcDsi *dsi, NvU8 RegAddr, NvU8 *pData, NvU32 DataSize,
    NvBool IsLongPacket )
{
    NvError status = NvError_Timeout;
    NvU32 Ecc =0;
    NvU32 val = 0;
    NvU32 PollTime = 0;
    NvU32 i = 0;
    NvU32 ReadBytes = 0;
    NvU32 WordCount = 0;
    NvU32 OrgHostDsiCtrl = 0;
    NvU32 OrgDsiCtrl = 0;
    NvU8  ReadFifo[DSI_READ_FIFO_DEPTH];
    NvU8* ptr = pData;
    NvU8 *pReadFifo = ReadFifo;

    NV_ASSERT(dsi);
    NV_ASSERT(pData);
    NV_ASSERT(DataSize);

    if( !dsi->bInit )
    {
        return NvError_NotInitialized;
    }

    NvOsMutexLock(dsi->DsiTransportMutex);

    HOST_CLOCK_ENABLE(dsi);

    if (!dsi->HostCtrlEnable)
    {
        PrivDsiSaveAndEnableHostEnable( dsi, &OrgHostDsiCtrl, &OrgDsiCtrl );
    }

    val = DSI_READ(dsi, DSI_DSI_STATUS_0);
    val = NV_DRF_VAL(DSI, DSI_STATUS, RD_FIFO_COUNT, val);
    if(val)
    {
        DSI_ERR_PRINT(("Read Fifo is not empty in the start of read[%d]..\n", val));
        status = NvError_Timeout;
        goto fail;
    }

    /* First Four Bytes forms the Packet header.
       Long Packet Format:
       Byte0 : [B7-6] Virtual Channel ID
               [B5-0] DSI command
       Byte1: DSI Packet Data Size(Max return size) Byte0
       Byte2: DSI Packet Data Size(Max return size) Byte1
       Byte3: ECC Data. Note!! We are Using H/W ECC in the DSI controller.
       Byte4: DSI Packet Data 0
       Byte5: DSI Packet Data 1
       ByteN: DSI Packet Data N, where N is WordCount.
    */
     // configure the max return packet size
     val = ((Ecc << 24) |
               ((DataSize & 0xFFFF) << 8) |
               ((dsi->VirChannel << DSI_VIR_CHANNEL_BIT_POSITION) |
                DsiCommand_MaxReturnPacketSize));

    DSI_WRITE(dsi, DSI_DSI_WR_DATA_0 , val);
#if DSI_USE_SYNC_POINTS
    status = PrivDsiWaitOnTriggerComplete( dsi );
    if (status != NvSuccess)
       goto fail;
#else
    /* Trigger the DSI host data to send out the data from the FIFO to Panel */
    DSI_WRITE(dsi, DSI_DSI_TRIGGER_0 ,
              NV_DRF_NUM(DSI, DSI_TRIGGER, DSI_HOST_TRIGGER, 1));

    if (!PrivDsiIsCommandModeReady(dsi))
    {
        NV_ASSERT(!"H/W seems not working");
    }
#endif

    // read the packet
    val = ((Ecc << 24) |
               ((RegAddr & 0xFFFF) << 8) |
               ((dsi->VirChannel << DSI_VIR_CHANNEL_BIT_POSITION) |
                DsiCommand_DcsReadWithNoParams));

    DSI_WRITE(dsi, DSI_DSI_WR_DATA_0 , val);
#if DSI_USE_SYNC_POINTS
    status = PrivDsiWaitOnTriggerComplete( dsi );
    if (status != NvSuccess)
       goto fail;
#else
    /* Trigger the DSI host data to send out the data from the FIFO to Panel */
    DSI_WRITE(dsi, DSI_DSI_TRIGGER_0 ,
              NV_DRF_NUM(DSI, DSI_TRIGGER, DSI_HOST_TRIGGER, 1));

    if (!PrivDsiIsCommandModeReady(dsi))
    {
        NV_ASSERT(!"H/W seems not working");
    }
#endif

#if DSI_USE_SYNC_POINTS
    PrivDsiWaitOnImmBtaComplete( dsi );
    val = DSI_READ(dsi, DSI_HOST_DSI_CONTROL_0);
    val = NV_DRF_VAL(DSI, HOST_DSI_CONTROL, IMM_BTA, val);
    if (val)
    {
        NV_ASSERT(!"IMM_BTA not cleared");
    }
#else
    val = DSI_READ( dsi, DSI_HOST_DSI_CONTROL_0);
    val = NV_FLD_SET_DRF_DEF(DSI, HOST_DSI_CONTROL, IMM_BTA, ENABLE, val);
    DSI_WRITE( dsi, DSI_HOST_DSI_CONTROL_0, val);

    while (PollTime <  DSI_STATUS_POLLING_DURATION_USEC)
    {
        val = DSI_READ(dsi, DSI_HOST_DSI_CONTROL_0);
        val = NV_DRF_VAL(DSI, HOST_DSI_CONTROL, IMM_BTA, val);
        if (!val)
        {
            break;
        }
        NvOsWaitUS(DSI_STATUS_POLLING_DELAY_USEC);
        PollTime += DSI_STATUS_POLLING_DELAY_USEC;
    }
#endif

    if (!val)
    {
        PollTime = 0;
        while (PollTime <  DSI_DELAY_FOR_READ_FIFO)
        {
            NvOsSleepMS(1);
            val = DSI_READ(dsi, DSI_DSI_STATUS_0);
            WordCount =
              NV_DRF_VAL(DSI, DSI_STATUS, RD_FIFO_COUNT, val);
            if ((WordCount << 2) > DSI_READ_FIFO_DEPTH)
            {
                DSI_ERR_PRINT(("Read fifo full..\n"));
                break;
            }
            PollTime++;
        }
    }
    else
    {
        DSI_ERR_PRINT(("IMM_BTA timeout..\n"));
        status = NvError_Timeout;
        goto fail;
    }

    if((val >> 8) & 3)
    {
        DSI_ERR_PRINT(("OVERFLOW/UNDERFLOW error [%d]..\n", val));
        status = NvError_Timeout;
        goto fail;
    }

    if ((!WordCount) || (WordCount > (DSI_READ_FIFO_DEPTH >> 2)))
    {
        DSI_ERR_PRINT(("Invalid word count[0x%x]..\n", WordCount));
        status = NvError_Timeout;
        goto fail;
    }

    DSI_PRINT(("Wordcount in FIFO[0x%x]..\n", WordCount));
    DSI_PRINT(("Start of Read FIFO..\n"));
    /* Read the data from the fifo */
    for (i = 0; i < WordCount; i++)
    {
        val = DSI_READ(dsi, DSI_DSI_RD_DATA_0);
        NvOsMemcpy(pReadFifo, &val, 4);
        DSI_PRINT(("count[%d] val[%x]\n", i, val));
        DSI_PRINT(("ReadFifo[%x] [%x] [%x] [%x]\n",
          *pReadFifo, *(pReadFifo + 1), *(pReadFifo + 2), *(pReadFifo + 3)));
        pReadFifo += 4;
    }
    DSI_PRINT(("End of Read FIFO..\n"));

    /* Make sure all the data is read from the FIFO */
    val = DSI_READ(dsi, DSI_DSI_STATUS_0);
    val = NV_DRF_VAL(DSI, DSI_STATUS, RD_FIFO_COUNT, val);
    if(val)
    {
        DSI_ERR_PRINT(("Read Fifo is not empty[%d]..\n", val));
        status = NvError_Timeout;
        goto fail;
    }

    /* Parse the read response */
    pReadFifo = ReadFifo;
    DSI_PRINT(("Read Data[0x%x]..\n", *pReadFifo));
    switch (ReadFifo[0] & 0xFF) {
       case 0x1A:
       case 0x1C:
           ReadBytes = (ReadFifo[1] | (ReadFifo[2] << 8)) & 0xFFFF;
           pReadFifo += 4;
           WordCount--;
           status = NvSuccess;
           DSI_PRINT(("Long read response Packet ReadBytes[0x%x]\n", ReadBytes));
           break;
       case 0x11:
       case 0x21:
           ReadBytes =  1;
           pReadFifo++;
           status = NvSuccess;
           DSI_PRINT(("Short read response Packet ReadBytes[0x%x]\n", ReadBytes));
           break;
       case 0x12:
       case 0x22:
           ReadBytes =  2;
           pReadFifo++;
           status = NvSuccess;
           DSI_PRINT(("Short read response Packet ReadBytes[0x%x]\n", ReadBytes));
           break;
       case 0x02:
           ReadBytes = 2;
           pReadFifo++;
           status = NvError_DispDsiReadAckError;
           DSI_PRINT(("Acknowledge error report response Packet ReadBytes[0x%x]\n", ReadBytes));
           break;
       default:
           // In case of invalid read response, read the requested bytes from the FIFO.
           ReadBytes = (WordCount << 2);
           status = NvError_DispDsiReadInvalidResp;
           DSI_PRINT(("Invalid Read response ReadBytes[0x%x]\n", ReadBytes));
           break;
       }

       ReadBytes = (ReadBytes > DataSize) ? DataSize : ReadBytes;
       ReadBytes = (ReadBytes > (WordCount << 2)) ? (WordCount << 2): ReadBytes;

       DSI_PRINT(("WordCount[%d] Readytes[%d] DataSize[%d]..\n", WordCount, ReadBytes, DataSize));
       NvOsMemcpy(ptr, pReadFifo, ReadBytes);

#if DSI_ENABLE_DEBUG_PRINTS
      NvOsDebugPrintf("Start of DSI read data..\n");
      for(i = 0; i < DataSize; i++)
      {
        NvOsDebugPrintf("ReadByte[%d]= 0x%x\n", i, *ptr);
        ptr++;
      }
      NvOsDebugPrintf("End of DSI read data..\n");
#endif

fail:
    if (!dsi->HostCtrlEnable)
    {
        PrivDsiRestoreHostEnable( dsi, OrgHostDsiCtrl, OrgDsiCtrl );
    }
    HOST_CLOCK_DISABLE(dsi);
    NvOsMutexUnlock(dsi->DsiTransportMutex);
    return status;
}

static void
DcDsiDisplayClockConfig( DcController *cont, DcDsi *dsi,
    NvOdmDispDeviceHandle hPanel )
{
    NvU32 div = 0;
    NvU32 panel_freq;
    NvU32 mipi_clock_freq_khz = 0;
    NvU32 ByteClockFreq = 0;
    NvU32 HorizontalWidth, VerticalWidth;
    NvU32 val;

    // if Burst mode is selected then take the odm frequency
    if (dsi->VideoMode == NvOdmDsiVideoModeType_Burst)
    {
        PrivDsiGetBurstModeSolDelay(cont, dsi, &mipi_clock_freq_khz,
                                        &panel_freq);
        div = (mipi_clock_freq_khz * 2 + panel_freq - 1)/(panel_freq) - 2;
    }
    else
    {
        HorizontalWidth = cont->mode.timing.Horiz_SyncWidth +
                            cont->mode.timing.Horiz_BackPorch +
                            cont->mode.timing.Horiz_DispActive +
                            cont->mode.timing.Horiz_FrontPorch;

        VerticalWidth = cont->mode.timing.Vert_SyncWidth +
                            cont->mode.timing.Vert_BackPorch +
                            cont->mode.timing.Vert_DispActive +
                            cont->mode.timing.Vert_FrontPorch;

        /* Calculate the panel frequency */
        panel_freq = HorizontalWidth * VerticalWidth * dsi->RefreshRate;

        /*
        // Calculate the MIPI clock frequency and DC pixel clock divider
        // Byte Clock Freq = Pixel freq * bytes per pixel / no of data lanes
        // DC Pixel Clock Divider = [[(4 * Bytes per pixel)/dsi lanes] - 1] * 2
        // To get the proper intiger value for DC divisor above equation is
        // derived to Div = [(8 * Bytes per pixel)/ dsi lanes] - 2
        */
        switch ( dsi->DataFormat)
        {
            case NvOdmDsiDataFormat_16BitP:
                /* 2 bytes per pixel */
                ByteClockFreq = (panel_freq * 2)/ dsi->DataLanes;
                div = (16 / dsi->DataLanes) - 2;
                break;
            case NvOdmDsiDataFormat_18BitP:
                /* 2.25 bytes per pixel */
                ByteClockFreq = (panel_freq * 9) /
                                    (4 * dsi->DataLanes);
                div = (18 / dsi->DataLanes) - 2;
                break;
            case NvOdmDsiDataFormat_18BitNP:
            case NvOdmDsiDataFormat_24BitP:
            default:
                 /* 3 bytes per pixel */
                ByteClockFreq = (panel_freq * 3) / (dsi->DataLanes);
                div = (24 / dsi->DataLanes) - 2;
                break;
        }

        // Mipi Clock Freq  = Byte Clock Freq * 8
        mipi_clock_freq_khz = ByteClockFreq * 8;

        // Mipi clock frequency in KHz
        mipi_clock_freq_khz = mipi_clock_freq_khz / 1000;

        // Panel frequency in KHz
        panel_freq = panel_freq / 1000;

        /*
        // MIPI CLOCK Frequency = 1/2 of the calaculated value
        // Clock driver is doubling the requested frequency, so make it half before
        // configuring the Mipi PLL.
        */
        mipi_clock_freq_khz = (mipi_clock_freq_khz / 2);
    }

    // For FPGA, force Pclk Div to be 1.
    if( NVDDK_DISP_CAP_IS_SET( cont->caps, NvDdkDispCapBit_Dsi_ForcePclkDiv1 ) )
    {
        div = 1;
    }

    dsi_set_clock( dsi, mipi_clock_freq_khz, panel_freq, &cont->freq);

    if( cont->pinmap == NvOdmDispPinMap_Reserved1 )
    {
        val = NV_DRF_DEF( DC_DISP, DISP_CLOCK_CONTROL, PIXEL_CLK_DIVIDER,
            PCD3 );
    }
    else
    {
        val = NV_DRF_DEF( DC_DISP, DISP_CLOCK_CONTROL, PIXEL_CLK_DIVIDER,
            PCD1 );
    }
    val = NV_FLD_SET_DRF_NUM( DC_DISP, DISP_CLOCK_CONTROL, SHIFT_CLK_DIVIDER,
        div, val );
    DC_WRITE( cont, DC_DISP_DISP_CLOCK_CONTROL_0, val );
}

static NvError
dsi_transport_init(
    DcDsi *dsi,
    NvU32 DsiInstance,
    NvOdmDispDeviceHandle hPanel )
{
    NvError e;
    NvU32 val = 0;
    NvRmPhysAddr phys = 0;
    NvU32 size = 0;
    NvRmPhysAddr PhysAdr;
    NvU32 BankSize;
    NvU32 *pVirAddr = NULL;

    // Minimum Frequency required for the DSI
    NvU32 mipi_clock_freq_khz = NVODM_DISP_DSI_COMMAND_MODE_FREQ;
    NvRmClockConfigFlags clock_flags = (NvRmClockConfigFlags)0;
    const NvOdmDispDsiConfig *pOdmDsiInfo;
    NvOsInterruptHandler IntrHandler =
        (NvOsInterruptHandler)PrivDsiGpioInterruptHandler;
    const NvOdmGpioPinInfo *pDsiGpioPinInfo = NULL;
    NvU32 GpioPinCount = 0;
    NvU32 MaxInstances = 0;

    NV_ASSERT( dsi );
    NV_ASSERT( hPanel );

    if( DsiInstance == 1 )
    {
        NvRmModuleGetBaseAddress( dsi->hRm,
                NVRM_MODULE_ID(NvRmModuleID_Misc, 0),
                &PhysAdr, &BankSize);

        NV_CHECK_ERROR_CLEANUP(
            NvRmPhysicalMemMap( PhysAdr, BankSize, NVOS_MEM_READ_WRITE,
                NvOsMemAttribute_Uncached, (void **)&pVirAddr)
        );
#ifdef NV_MIPI_PAD_CTRL_EXISTS
        val = NV_READ32( (NvU32 *)(pVirAddr) + (APB_MISC_GP_MIPI_PAD_CTRL_0) );
        val = NV_FLD_SET_DRF_DEF(APB_MISC, GP_MIPI_PAD_CTRL,
                                    DSIB_MODE, DSI, val);
        NV_WRITE32( ((NvU32 *)(pVirAddr) +
                    (APB_MISC_GP_MIPI_PAD_CTRL_0)), val );
#endif
        NvRmPhysicalMemUnmap(pVirAddr, BankSize);
    }

    NV_CHECK_ERROR_CLEANUP(
        NvOsMutexCreate( &dsi->DsiTransportMutex )
    );

    /* map the registers */
    NvRmModuleGetBaseAddress( dsi->hRm,
            NVRM_MODULE_ID( NvRmModuleID_Dsi, DsiInstance ), &phys, &size );

    NV_CHECK_ERROR_CLEANUP(
        NvRmPhysicalMemMap(phys, size, NVOS_MEM_READ_WRITE,
            NvOsMemAttribute_Uncached, &dsi->VirAddr )
    );
    dsi->PhyAddr = phys;
    dsi->BankSize = size;

    dsi->PowerClientId = NVRM_POWER_CLIENT_TAG('D','S','I','1');
    NV_CHECK_ERROR_CLEANUP(
        NvRmPowerRegister( dsi->hRm, 0, &dsi->PowerClientId)
    );

    pOdmDsiInfo = (NvOdmDispDsiConfig *)NvOdmDispGetConfiguration( hPanel );
    dsi->DataLanes = pOdmDsiInfo->NumOfDataLines;
    dsi->DataFormat = pOdmDsiInfo->DataFormat;
    dsi->VideoMode = pOdmDsiInfo->VideoMode;
    dsi->VirChannel = pOdmDsiInfo->VirChannel;
    dsi->LpCmdModeFreqKHz = ( pOdmDsiInfo->LpCommandModeFreqKHz ) ?
        pOdmDsiInfo->LpCommandModeFreqKHz : NVODM_DISP_DSI_COMMAND_MODE_FREQ;
    mipi_clock_freq_khz = dsi->LpCmdModeFreqKHz;
    dsi->HsCmdModeFreqKHz = ( pOdmDsiInfo->HsCommandModeFreqKHz ) ?
        pOdmDsiInfo->HsCommandModeFreqKHz : NVODM_DISP_DSI_COMMAND_MODE_FREQ;
    dsi->HsSupportForFrameBuffer = pOdmDsiInfo->HsSupportForFrameBuffer;
    dsi->PanelResetTimeoutMSec = pOdmDsiInfo->PanelResetTimeoutMSec;
    dsi->RefreshRate = ( pOdmDsiInfo->RefreshRate ) ?
        pOdmDsiInfo->RefreshRate : 60;
    dsi->PhyFreq = pOdmDsiInfo->Freq;
    dsi->HsClockControl = pOdmDsiInfo->HsClockControl;
    dsi->WarMinThsprepr = pOdmDsiInfo->WarMinThsprepr;
    dsi->bDsiIsInHsMode = NV_FALSE;
    /**
     * Set DSI for PLL to be min(WarMaxInitDsiClkKHz,
     * DSI_MAX_DSICLK_FOR_THS_WAR)
     */
    if ( ( DSI_MAX_DSICLK_FOR_THS_WAR < pOdmDsiInfo->WarMaxInitDsiClkKHz ) ||
         ( 0 == pOdmDsiInfo->WarMaxInitDsiClkKHz) )
    {
        dsi->WarMaxInitDsiClkKHz = DSI_MAX_DSICLK_FOR_THS_WAR;
    }
    else
    {
        dsi->WarMaxInitDsiClkKHz = pOdmDsiInfo->WarMaxInitDsiClkKHz;
    }
    dsi->EnableHsClockInLpMode = pOdmDsiInfo->EnableHsClockInLpMode;

#if NVRM_AUTO_HOST_ENABLE
        NV_ASSERT_SUCCESS(
            NvRmPowerVoltageControl( dsi->hRm,
                NvRmModuleID_GraphicsHost,
                dsi->PowerClientId,
                NvRmVoltsUnspecified, NvRmVoltsUnspecified,
                NULL, 0, NULL )
        );
#endif
    HOST_CLOCK_ENABLE( dsi );

    // Initialize the pad configuration for dsi
    NV_CHECK_ERROR_CLEANUP(
        PrivDsiInitPadCnfg( dsi )
    );

    clock_flags = NvRmClockConfig_MipiSync;

    NV_CHECK_ERROR_CLEANUP(
        NvRmPowerModuleClockConfig( dsi->hRm,
            NVRM_MODULE_ID( NvRmModuleID_Dsi, dsi->DsiInstance ),
            dsi->PowerClientId,
            NvRmFreqUnspecified,
            NvRmFreqUnspecified,
            &mipi_clock_freq_khz, 1, &dsi->freq, clock_flags)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvRmPowerModuleClockControl( dsi->hRm,
            NVRM_MODULE_ID( NvRmModuleID_Dsi, dsi->DsiInstance ),
            dsi->PowerClientId,
            NV_TRUE )
    );

    // Reset controller
    NvRmModuleReset( dsi->hRm,
        NVRM_MODULE_ID( NvRmModuleID_Dsi, dsi->DsiInstance ));

    //Initializing DSI registers
    DSI_WRITE(dsi, DSI_DSI_WR_DATA_0, 0);
    DSI_WRITE(dsi, DSI_INT_ENABLE_0, 0);
    DSI_WRITE(dsi, DSI_INT_STATUS_0, 0);
    DSI_WRITE(dsi, DSI_INT_MASK_0, 0);
    DSI_WRITE(dsi, DSI_DSI_INIT_SEQ_DATA_0_0, 0);
    DSI_WRITE(dsi, DSI_DSI_INIT_SEQ_DATA_1_0, 0);
    DSI_WRITE(dsi, DSI_DSI_INIT_SEQ_DATA_2_0, 0);
    DSI_WRITE(dsi, DSI_DSI_INIT_SEQ_DATA_3_0, 0);
    DSI_WRITE(dsi, DSI_DSI_DCS_CMDS_0, 0);
    DSI_WRITE(dsi, DSI_DSI_PKT_SEQ_0_LO_0, 0);
    DSI_WRITE(dsi, DSI_DSI_PKT_SEQ_1_LO_0, 0);
    DSI_WRITE(dsi, DSI_DSI_PKT_SEQ_2_LO_0, 0);
    DSI_WRITE(dsi, DSI_DSI_PKT_SEQ_3_LO_0, 0);
    DSI_WRITE(dsi, DSI_DSI_PKT_SEQ_4_LO_0, 0);
    DSI_WRITE(dsi, DSI_DSI_PKT_SEQ_5_LO_0, 0);
    DSI_WRITE(dsi, DSI_DSI_PKT_SEQ_0_HI_0, 0);
    DSI_WRITE(dsi, DSI_DSI_PKT_SEQ_1_HI_0, 0);
    DSI_WRITE(dsi, DSI_DSI_PKT_SEQ_2_HI_0, 0);
    DSI_WRITE(dsi, DSI_DSI_PKT_SEQ_3_HI_0, 0);
    DSI_WRITE(dsi, DSI_DSI_PKT_SEQ_4_HI_0, 0);
    DSI_WRITE(dsi, DSI_DSI_PKT_SEQ_5_HI_0, 0);
    DSI_WRITE( dsi, DSI_DSI_CONTROL_0, 0);
    DSI_WRITE( dsi, DSI_PAD_CONTROL_0, 0);
    DSI_WRITE( dsi, DSI_PAD_CONTROL_CD_0, 0);
    DSI_WRITE( dsi, DSI_DSI_SOL_DELAY_0, DSI_DEFAULT_SOL_DELAY);
    /* Setup Max thresh hold for DSI packets */
    DSI_WRITE(dsi, DSI_DSI_MAX_THRESHOLD_0 ,DSI_VIDEO_FIFO_DEPTH);
    DSI_WRITE(dsi, DSI_DSI_TRIGGER_0 ,0);
    DSI_WRITE(dsi, DSI_DSI_INIT_SEQ_CONTROL_0, 0);
    DSI_WRITE(dsi, DSI_DSI_PKT_LEN_0_1_0, 0);
    DSI_WRITE(dsi, DSI_DSI_PKT_LEN_2_3_0, 0);
    DSI_WRITE(dsi, DSI_DSI_PKT_LEN_4_5_0, 0);
    DSI_WRITE(dsi, DSI_DSI_PKT_LEN_6_7_0, 0);
    /* Setup DSI Phy timming */
    DsiSetupPhyTiming( NULL, dsi, hPanel );

    MaxInstances = NvRmModuleGetNumInstances( dsi->hRm,
        NvRmModuleID_Dsi );

    /* enable dsi */
    val = DSI_READ(dsi, DSI_PAD_CONTROL_0);
    val = NV_FLD_SET_DRF_NUM(DSI, PAD_CONTROL,
            DSI_PAD_PDIO, 0, val);
    val = NV_FLD_SET_DRF_NUM(DSI, PAD_CONTROL,
            DSI_PAD_PDIO_CLK, 0, val);
    if( MaxInstances == 2 )
    {
        val = NV_FLD_SET_DRF_NUM(DSI, PAD_CONTROL,
                DSI_PAD_PULLDN_ENAB, 1, val);
    }
    DSI_WRITE(dsi, DSI_PAD_CONTROL_0, val);

    val = NV_DRF_DEF(DSI, DSI_POWER_CONTROL, LEG_DSI_ENABLE, ENABLE);
    DSI_WRITE( dsi, DSI_DSI_POWER_CONTROL_0, val );
    while(DSI_READ(dsi, DSI_DSI_POWER_CONTROL_0) != val)
    DSI_WRITE(dsi, DSI_DSI_POWER_CONTROL_0, val);

    HOST_CLOCK_DISABLE( dsi );

#if DSI_USE_SYNC_POINTS
    e = PrivDsiInit( dsi );
    if (e != NvSuccess)
    {
        NV_ASSERT(!"NVRM_DSI Failed to initialize");
        goto fail;
    }
#endif

    dsi->HostCtrlEnable = NV_FALSE;

    pDsiGpioPinInfo = NvOdmQueryGpioPinMap(NvOdmGpioPinGroup_Dsi, 0,
        &GpioPinCount);
    if (GpioPinCount)
    {
        NV_CHECK_ERROR_CLEANUP(
            NvRmGpioOpen( dsi->hRm, &dsi->DsiTeGpioHandle )
        );

        NvRmGpioAcquirePinHandle(dsi->DsiTeGpioHandle,
            pDsiGpioPinInfo[NvOdmGpioPin_DsiLcdTeId].Port,
            pDsiGpioPinInfo[NvOdmGpioPin_DsiLcdTeId].Pin,
            &dsi->DsiTePinHandle);

        NV_CHECK_ERROR_CLEANUP(
            NvRmGpioConfigPins(dsi->DsiTeGpioHandle,
                &dsi->DsiTePinHandle, 1, NvRmGpioPinMode_InputData)
        );

        NV_CHECK_ERROR_CLEANUP(
            NvRmGpioInterruptRegister(dsi->DsiTeGpioHandle, dsi->hRm,
                dsi->DsiTePinHandle, IntrHandler,
                NvRmGpioPinMode_InputInterruptRisingEdge, dsi,
                &dsi->GpioIntrHandle, 0)
        );

        NV_CHECK_ERROR_CLEANUP(
            NvRmGpioInterruptEnable(dsi->GpioIntrHandle)
        );
        // disable the Te signal interrupt
        NvRmGpioInterruptMask(dsi->GpioIntrHandle, NV_TRUE);
        NV_CHECK_ERROR_CLEANUP(
            NvOsSemaphoreCreate(&dsi->TeSignalSema, 0)
        );
        dsi->TeFrameUpdate = NV_TRUE;
    }
    else
    {
        dsi->TeFrameUpdate = NV_FALSE;
    }

    dsi->bInit = NV_TRUE;
    return e;

fail:

    NvOsSemaphoreDestroy(dsi->TeSignalSema);
    dsi->TeSignalSema = NULL;

    NvOsMutexDestroy(dsi->DsiTransportMutex);
    dsi->DsiTransportMutex = NULL;

    // unregister the gpio interrupt handler
    NvRmGpioInterruptUnregister(dsi->DsiTeGpioHandle,
            dsi->hRm, dsi->GpioIntrHandle);
    // close the gpio handle
    NvRmGpioClose(dsi->DsiTeGpioHandle);
    dsi->DsiTeGpioHandle = NULL;

    // Unregister driver from Power Manager
    NvRmPowerUnRegister( dsi->hRm, dsi->PowerClientId );
    NvRmPhysicalMemUnmap( dsi->VirAddr, dsi->BankSize );

    /* Clear the dsi structure initally */
    NvOsMemset( dsi, 0, sizeof(DcDsi) );

    return e;
}

static NvError
PrivDsiSetDsiHsClkCtrl( DcDsi *dsi, NvU32 clockCtrl )
{
    NvU32 val;

    val = DSI_READ( dsi, DSI_DSI_CONTROL_0 );
    val = NV_FLD_SET_DRF_NUM( DSI, DSI_CONTROL, DSI_HS_CLK_CTRL,
        clockCtrl, val );
    DSI_WRITE( dsi, DSI_DSI_CONTROL_0, val );

    return NvSuccess;
}

static NvError
PrivDsiWriteNopCommand( DcDsi *dsi )
{
    NvU8 dispData;
    NvError retVal;

    dispData = DSI_CMD_NOP_COMMAND_DATA;
    retVal = dsi_write_data( dsi, DSI_CMD_NOP_COMMAND_TYPE,
        DSI_CMD_NOP_COMMAND_REG, &dispData, DSI_CMD_NOP_COMMAND_SIZE,
        DSI_CMD_NOP_COMMAND_ISLONG );

    return retVal;
}

static NvError
PrivDsiWriteDisplayOnOffCommand( DcDsi *dsi, NvBool displayOn )
{
    NvU8 dispData;
    NvU8 regData;
    NvError retVal;

    dispData = DSI_CMD_DISPLAY_ONOFF_DATA;
    if ( NV_TRUE == displayOn )
    {
        regData = DSI_CMD_DISPLAY_ON_REG;
    }
    else
    {
        regData = DSI_CMD_DISPLAY_OFF_REG;
    }

    retVal = dsi_write_data( dsi, DSI_CMD_DISPLAY_ONOFF_TYPE, regData,
        &dispData, DSI_CMD_DISPLAY_ONOFF_SIZE, DSI_CMD_DISPLAY_ONOFF_ISLONG );
    return retVal;
}

static NvError
PrivDsiRunSanityCheck( DcDsi *dsi )
{
    NvError retVal;


    /* issue a dummy write (nop command) */
    retVal = PrivDsiWriteNopCommand( dsi );

    /* issue a read to check if there is any error on write and BTA */
    if ( NvSuccess == retVal )
    {
        retVal = dsi_bta_check_error( dsi );
    }
    else
    {
        NvOsDebugPrintf("Error when writing data\n");
    }

    return retVal;
}

static NvError
PrivDsiTurnOnDsiClock( DcController *cont, DcDsi *dsi,
    NvOdmDispDeviceHandle hPanel )
{
    NvError retVal;
    NvU32 hostCtrl;
    NvU32 saveFreq;
    NvU32 actualDsiFreq;

    retVal = NvSuccess;
    /**
     * Select the High Speed clock control line
     * Note: If NvOdmDsiHsClock_Continuous_Thsprepr_WAR or
     *       NvOdmDsiHsClock_Continuous_Thsprepr_PLL_WAR is specified,
     *       set the HS clock mode to be in TX_ONLY mode so that
     *       no clock will be output to the target device at this stage.
     */
    if( dsi->HsClockControl == NvOdmDsiHsClock_Continuous )
    {
        PrivDsiSetDsiHsClkCtrl(dsi,
            DSI_DSI_CONTROL_0_DSI_HS_CLK_CTRL_CONTINUOUS);
    }
    else
    {
        PrivDsiSetDsiHsClkCtrl(dsi,
            DSI_DSI_CONTROL_0_DSI_HS_CLK_CTRL_TX_ONLY);
    }

    hostCtrl = NV_DRF_DEF( DSI, HOST_DSI_CONTROL, PKT_WR_FIFO_SEL, HOST ) |
        NV_DRF_DEF( DSI, HOST_DSI_CONTROL, CS_ENABLE, ENABLE ) |
        NV_DRF_DEF( DSI, HOST_DSI_CONTROL, ECC_ENABLE, ENABLE );
    DSI_WRITE( dsi, DSI_HOST_DSI_CONTROL_0, hostCtrl );

    /**
     * SW WARs for tHSPREPR (applicable only for continuous mode):
     * Before outputing DSI clock to target device
     * A. For NvOdmDsiHsClock_Continuous_Thsprepr_WAR
     *    1. Set DSI_PHY_TIMING_0_DSI_THSPREPR to be max(user_value, 300ns)
     *    2. Set the HS clock mode to be in continuous mode
     *    3. Program DSI_HOST_DSI_CONTROL_0
     *    4. Wait for 1mS
     *    5. Set DSI_PHY_TIMING_0_DSI_THSPREPR to be back to original value
     * B. For NvOdmDsiHsClock_Continuous_Thsprepr_PLL_WAR
     *    1. Set DSI_PHY_TIMING_0_DSI_THSPREPR to 0
     *    2. Set PLL for DSI to be min(WarMaxInitDsiClkKHz, 1.2MHz)
     *    3. Set the HS clock mode to be in continuous mode
     *    4. Program DSI_HOST_DSI_CONTROL_0
     *    5. Set PLL for DSI to be back to the original value
     *    6. Set DSI_PHY_TIMING_0_DSI_THSPREPR to be back to original value
     */
    if ( ( dsi->HsClockControl == NvOdmDsiHsClock_Continuous_Thsprepr_WAR ) ||
         ( dsi->HsClockControl == NvOdmDsiHsClock_Continuous_Thsprepr_PLL_WAR
         ) )
    {
        NvU32 timing0Val;
        NvU32 tHSPREPR;

        /* turning off display in LP state */
        PrivDsiWriteDisplayOnOffCommand( dsi, NV_FALSE );

        if ( dsi->HsClockControl == NvOdmDsiHsClock_Continuous_Thsprepr_WAR )
        {
            /* max( user_value, DSI_MIN_THSPREPR_FOR_THS_WAR ) */
            if ( DSI_MIN_THSPREPR_FOR_THS_WAR > dsi->WarMinThsprepr )
            {
                tHSPREPR = DSI_MIN_THSPREPR_FOR_THS_WAR;
            }
            else
            {
                tHSPREPR = dsi->WarMinThsprepr;
            }

            /* convert tHSPREPR from nS to num of clock */
            tHSPREPR = DSI_PHY_TIMING_DIV( tHSPREPR, dsi->freq ) + 1;

            /* clamp to max value in case the user value is too large */
            if ( tHSPREPR > NV_FIELD_MASK(
                    DSI_DSI_PHY_TIMING_0_0_DSI_THSPREPR_RANGE ) )
            {
                tHSPREPR = NV_FIELD_MASK(
                        DSI_DSI_PHY_TIMING_0_0_DSI_THSPREPR_RANGE );
            }
        }
        else
        {
            NvU32 initDsiFreq;

            /* Thsprepr_PLL_WAR, set tHSPREPR to 0 and */
            tHSPREPR = 0;

            /* actual freq set to pll is required to be div by 2 */
            initDsiFreq = dsi->WarMaxInitDsiClkKHz / 2;

            /* save the original frequency */
            NV_ASSERT_SUCCESS(
                NvRmPowerModuleClockConfig( dsi->hRm,
                    NVRM_MODULE_ID( NvRmModuleID_Dsi, dsi->DsiInstance ),
                    dsi->PowerClientId,
                    0, 0, NULL, 0, &saveFreq, 0 )
            );

            /* set PLL for DSI to be at a very low speed */
            NV_ASSERT_SUCCESS(
                NvRmPowerModuleClockConfig( dsi->hRm,
                    NVRM_MODULE_ID( NvRmModuleID_Dsi, dsi->DsiInstance ),
                    dsi->PowerClientId,
                    (initDsiFreq * 99) / 100,
                    (initDsiFreq * 109) / 100 ,
                    &initDsiFreq,
                    1,
                    &actualDsiFreq,
                    NvRmClockConfig_MipiSync )
            );
        }

        /* update tHSPREPR */
        timing0Val = DSI_READ( dsi, DSI_DSI_PHY_TIMING_0_0 );
        timing0Val = NV_FLD_SET_DRF_NUM( DSI, DSI_PHY_TIMING_0, DSI_THSPREPR,
            tHSPREPR, timing0Val );
        DSI_WRITE( dsi, DSI_DSI_PHY_TIMING_0_0, timing0Val );

        /* set HS clock mode back to continuous mode */
        PrivDsiSetDsiHsClkCtrl(dsi,
            DSI_DSI_CONTROL_0_DSI_HS_CLK_CTRL_CONTINUOUS);
    }

    /*
     * (Setup DSI host control):
     * Enable Hardware ECC, Enable Hardware Checksum, enable high speed
     * packet transmition, enable write data to the host and video line FIFO
     */
    hostCtrl |= NV_DRF_DEF( DSI, HOST_DSI_CONTROL, DSI_HIGH_SPEED_TRANS,
        HIGH );
    DSI_WRITE( dsi, DSI_HOST_DSI_CONTROL_0, hostCtrl );
    dsi->bDsiIsInHsMode = NV_TRUE;

    if ( ( dsi->HsClockControl == NvOdmDsiHsClock_Continuous_Thsprepr_WAR ) ||
         ( dsi->HsClockControl == NvOdmDsiHsClock_Continuous_Thsprepr_PLL_WAR
         ) )
    {
        /* Delay for 1mS and set tHSPREPR back to original value */
        NvOsSleepMS( 1 );
        if ( dsi->HsClockControl == NvOdmDsiHsClock_Continuous_Thsprepr_PLL_WAR )
        {
            /* Restore the PLL setting for DSI */
            NV_ASSERT_SUCCESS(
                NvRmPowerModuleClockConfig( dsi->hRm,
                    NVRM_MODULE_ID( NvRmModuleID_Dsi, dsi->DsiInstance),
                    dsi->PowerClientId,
                    (saveFreq * 99) / 100,
                    (saveFreq * 109) / 100 ,
                    &saveFreq,
                    1,
                    &actualDsiFreq,
                    NvRmClockConfig_MipiSync )
            );
        }
        DsiSetupPhyTiming( cont, dsi, hPanel );

        /* Run sanity check to make sure that DSI is working fine */
        retVal = PrivDsiRunSanityCheck( dsi );
        if ( NvSuccess != retVal )
        {
            NvOsDebugPrintf("Error on clock en!!! Set to Tx_only mode!!!\n");
            /**
             * if error, set the clock mode to be Tx_Only and issue another
             * dummy write. The clock lane will be back to LP-11 in this case.
             */
            PrivDsiSetDsiHsClkCtrl(dsi,
                DSI_DSI_CONTROL_0_DSI_HS_CLK_CTRL_TX_ONLY);
        }

        /* turning display back on */
        PrivDsiWriteDisplayOnOffCommand( dsi, NV_TRUE );
    }

    return retVal;
}

static NvError
dsi_enable( DcController *cont, DcDsi *dsi, NvOdmDispDeviceHandle hPanel )
{
    NvU32 val;
    NvU32 clkEnableTimeout;
    NvError retVal;

    HOST_CLOCK_ENABLE(dsi);
    /* Setup DSI Phy timming */
    DsiSetupPhyTiming( cont, dsi, hPanel );

    /* Setup DSI packet sequence */
    DsiSetupPacketSequence(dsi);

    /* setup dsi packet lengths */
    DsiSetupPacketLength(cont, dsi);

    if(dsi->VideoMode == NvOdmDsiVideoModeType_DcDrivenCommandMode)
    {
        val = NV_DRF_NUM(DSI, DSI_DCS_CMDS, LT5_DCS_CMD,
                DSI_WRITE_MEMORY_START)
            | NV_DRF_NUM(DSI, DSI_DCS_CMDS, LT3_DCS_CMD,
                DSI_WRITE_MEMORY_CONTINUE);
        DSI_WRITE(dsi, DSI_DSI_DCS_CMDS_0, val);
    }

    /* Clear the DSI control and host control to setup again */
    DSI_WRITE( dsi, DSI_HOST_DSI_CONTROL_0, 0);
    DSI_WRITE( dsi, DSI_DSI_TRIGGER_0, 0);
    DSI_WRITE( dsi, DSI_DSI_CONTROL_0, 0);

    dsi->HostCtrlEnable = NV_FALSE;

    // Calaculate the SOL delay = 8*FreqDsi/FreqPixel;
    /*
      !!NOTE!! AlternatAlternatively, a fixed value of SOL_DELAY that will
      work for all pixel formats and numbers of lanes can be programming by
      taking the worst-case ratio of 1:3 (pixel:byte) clock and multiplying
      by 8 to give SOL_DELAY = 24.
    */
    //DSI_WRITE( dsi, DSI_DSI_SOL_DELAY_0, DSI_DEFAULT_SOL_DELAY);
    DsiSetupSolDelay( cont, dsi );

    /* Setup Max thresh hold for DSI packets */
    DSI_WRITE(dsi, DSI_DSI_MAX_THRESHOLD_0 ,DSI_VIDEO_FIFO_DEPTH);

    /* enable dsi */
    val = NV_DRF_DEF(DSI, DSI_POWER_CONTROL, LEG_DSI_ENABLE, ENABLE);
    DSI_WRITE( dsi, DSI_DSI_POWER_CONTROL_0, val );

    /* Setup DSI control configuration:
     * select source of display or displayb
     */
    if( dsi->ControllerIndex == 0 )
    {
        val = NV_DRF_DEF( DSI, DSI_CONTROL, DSI_VID_SOURCE, DISPLAY_0 );
    }
    else
    {
        val = NV_DRF_DEF( DSI, DSI_CONTROL, DSI_VID_SOURCE, DISPLAY_1 );
    }

    /* enable video DSI interface */
    val |= NV_DRF_DEF(DSI, DSI_CONTROL, DSI_VID_ENABLE, ENABLE);

    /* select the source of trigger to start sending the packets */
    val |= NV_DRF_NUM(DSI, DSI_CONTROL, DSI_NUM_DATA_LANES,
        (dsi->DataLanes - 1));

    /* select the source of trigger to start sending the packets */
    val |= NV_DRF_DEF(DSI, DSI_CONTROL, VID_TX_TRIG_SRC, SOL);

    /* select Pixel data format */
    switch( dsi->DataFormat ) {
    case NvOdmDsiDataFormat_16BitP:
        val |= NV_DRF_DEF(DSI, DSI_CONTROL, DSI_DATA_FORMAT, BIT16P );
        break;
    case NvOdmDsiDataFormat_18BitNP:
        val |= NV_DRF_DEF(DSI, DSI_CONTROL, DSI_DATA_FORMAT, BIT18NP );
        break;
    case NvOdmDsiDataFormat_18BitP:
        val |= NV_DRF_DEF(DSI, DSI_CONTROL, DSI_DATA_FORMAT, BIT18P );
        break;
    case NvOdmDsiDataFormat_24BitP:
        val |= NV_DRF_DEF(DSI, DSI_CONTROL, DSI_DATA_FORMAT, BIT24P );
        break;
    default:
        NV_ASSERT( !"unknown format" );
        return NvError_BadValue;
    }

    /**
     * Select the High Speed clock control line
     * Note: If NvOdmDsiHsClock_Continuous_Thsprepr_WAR is specified,
     *       set the HS clock mode to be in TX_ONLY mode so that
     *       no clock will be output to the target device at this stage.
     */
    if( dsi->HsClockControl == NvOdmDsiHsClock_Continuous )
        val |= NV_DRF_DEF( DSI, DSI_CONTROL, DSI_HS_CLK_CTRL, CONTINUOUS );
        val |= NV_DRF_DEF( DSI, DSI_CONTROL, DSI_HS_CLK_CTRL, TX_ONLY );

    /* Select if the panel is DcDriven command mode */
    if (dsi->VideoMode == NvOdmDsiVideoModeType_DcDrivenCommandMode)
    {
        val |= NV_DRF_DEF( DSI, DSI_CONTROL, VID_DCS_ENABLE, ENABLE );
    }

    /* select the Virtual Channel ID */
    val |= NV_DRF_NUM( DSI, DSI_CONTROL, DSI_VIRTUAL_CHANNEL,
        dsi->VirChannel );

    /* update the register */
    DSI_WRITE( dsi, DSI_DSI_CONTROL_0, val );

    /**
     * SW WAR for tHSPREPR (applicable only for continuous mode):
     * Before outputing DSI clock to target device
     * 1. Set DSI_PHY_TIMING_0_DSI_THSPREPR to be max(user_value, 300ns)
     * 2. Set the HS clock mode to be in continuous mode
     * 3. Program DSI_HOST_DSI_CONTROL_0
     * 4. Wait for 1mS
     * 5. Set DSI_PHY_TIMING_0_DSI_THSPREPR to be back to original value
     */
    if ( dsi->HsClockControl == NvOdmDsiHsClock_Continuous_Thsprepr_WAR )
    {
        NvU32 timing0Val;
        NvU32 tHSPREPR;

        /* max( user_value, DSI_MIN_THSPREPR_FOR_THS_WAR ) */
        if ( DSI_MIN_THSPREPR_FOR_THS_WAR > dsi->WarMinThsprepr )
        {
            tHSPREPR = DSI_MIN_THSPREPR_FOR_THS_WAR;
        }
        else
        {
            tHSPREPR = dsi->WarMinThsprepr;
        }

        /* convert tHSPREPR from nS to num of clock*/
        tHSPREPR = DSI_PHY_TIMING_DIV( tHSPREPR, dsi->freq ) + 1;

        /* clamp to max value in case the user value is too large */
        if ( tHSPREPR > NV_FIELD_MASK(
                DSI_DSI_PHY_TIMING_0_0_DSI_THSPREPR_RANGE ) )
        {
            tHSPREPR = NV_FIELD_MASK(
                    DSI_DSI_PHY_TIMING_0_0_DSI_THSPREPR_RANGE );
        }

        /* update tHSPREPR */
        timing0Val = DSI_READ( dsi, DSI_DSI_PHY_TIMING_0_0 );
        timing0Val |= NV_FLD_SET_DRF_NUM( DSI, DSI_PHY_TIMING_0, DSI_THSPREPR,
            tHSPREPR, timing0Val );
        DSI_WRITE( dsi, DSI_DSI_PHY_TIMING_0_0, timing0Val );

        /* set HS clock mode back to continuous mode */
        val = NV_FLD_SET_DRF_DEF( DSI, DSI_CONTROL, DSI_HS_CLK_CTRL,
            CONTINUOUS, val );
        DSI_WRITE( dsi, DSI_DSI_CONTROL_0, val );
    }

    /*
     * (Setup DSI host control):
     * Enable Hardware ECC, Enable Hardware Checksum, enable high speed
     * packet transmition, enable write data to the host and video line FIFO
     */
    val = NV_DRF_DEF( DSI, HOST_DSI_CONTROL, DSI_HIGH_SPEED_TRANS, HIGH ) |
        NV_DRF_DEF( DSI, HOST_DSI_CONTROL, PKT_WR_FIFO_SEL, VIDEO ) |
        NV_DRF_DEF( DSI, HOST_DSI_CONTROL, CS_ENABLE, ENABLE ) |
        NV_DRF_DEF( DSI, HOST_DSI_CONTROL, ECC_ENABLE, ENABLE );
    DSI_WRITE( dsi, DSI_HOST_DSI_CONTROL_0, val );

    if ( dsi->HsClockControl == NvOdmDsiHsClock_Continuous_Thsprepr_WAR )
    {
        /* Delay for 1mS and set tHSPREPR back to original value */
        NvOsSleepMS( 1 );
        DsiSetupPhyTiming( cont, dsi, hPanel );
    }

    /* Enable clock out on dsi clock lane */
    retVal = NvSuccess;
    clkEnableTimeout = DSI_MAX_CLOCK_ENABLE_TIMEOUT;
    do {
        retVal = PrivDsiTurnOnDsiClock( cont, dsi, hPanel );
        clkEnableTimeout--;
    } while( ( NvSuccess != retVal ) && ( 0 != clkEnableTimeout ) );

    NV_ASSERT( clkEnableTimeout );

    HOST_CLOCK_DISABLE( dsi );
    return retVal;
}

static NvError
PrivDsiTransInit(
    NvOdmDispDeviceHandle hDevice,
    NvU32 DsiInstance,
    NvU32 flags )
{
    NvRmDeviceHandle hRm;
    DcDsi *dsi;
    NvError e = NvSuccess;

    dsi = &s_Dsi[DsiInstance];

    if ( dsi->bInit )
    {
        return e;
    }

    e = NvRmOpenNew( &hRm );
    if( e != NvSuccess )
    {
        goto fail;
    }

    dsi->hRm = hRm;

    e = dsi_transport_init( dsi, DsiInstance, hDevice );
    if( e != NvSuccess )
    {
        goto fail;
    }
    return e;

fail:
    NvRmClose( hRm );
    return e;
}

NvError
DcDsiEnable(
    DcController *cont,
    NvU32 DsiInstance,
    NvOdmDispDeviceHandle hPanel )
{
    NvError e;
    DcDsi *dsi;

    /* the controller mode must be set */
    NV_ASSERT( cont->mode.width && cont->mode.height && cont->mode.bpp );

    dsi = &s_Dsi[DsiInstance];
    dsi->hRm = cont->hRm;

    if( !dsi->bInit )
    {
        e = dsi_transport_init( dsi, DsiInstance, hPanel );
        if( e != NvSuccess )
        {
            NV_ASSERT( !"dsi init failed" );
            cont->bFailed = NV_TRUE;
            return NvError_InvalidState;
        }
    }

    /* configure the clock */
    DcDsiDisplayClockConfig( cont, dsi, hPanel );

    /* set the color base */
    DcSetColorBase( cont, NvOdmDispBaseColorSize_888 );

    e = dsi_enable( cont, dsi, hPanel );
    if( e != NvSuccess )
    {
        NV_ASSERT( !"dsi enable failed" );
        cont->bFailed = NV_TRUE;
        return NvError_InvalidState;
    }

    /* set the dsi enable bit */
    DcEnable( cont, DC_HAL_DSI, NV_TRUE );
    dsi->bEnabled = NV_TRUE;

    return NvSuccess;
}

void
DcDsiDisable( DcDsi *dsi )
{
    if( dsi->bEnabled || dsi->bInit )
    {
        NvU32 val;

        /* shutdown dsi */
        val = NV_DRF_DEF(DSI, DSI_POWER_CONTROL, LEG_DSI_ENABLE, DISABLE);
        DSI_WRITE( dsi, DSI_DSI_POWER_CONTROL_0, val );

        // disable the clock to the dsi controller
        dsi_set_clock( dsi, 0, 0, NULL );

#if NVRM_AUTO_HOST_ENABLE
        NV_ASSERT_SUCCESS(
            NvRmPowerVoltageControl( dsi->hRm,
                NvRmModuleID_GraphicsHost,
                dsi->PowerClientId,
                NvRmVoltsOff, NvRmVoltsOff,
                NULL, 0, NULL )
        );
#endif

        // unregister the gpio interrupt handler
        NvRmGpioInterruptUnregister(dsi->DsiTeGpioHandle,
            dsi->hRm, dsi->GpioIntrHandle);
        // close the gpio handle
        NvRmGpioClose(dsi->DsiTeGpioHandle);
        NvOsSemaphoreDestroy(dsi->TeSignalSema);

        // Unregister driver from Power Manager
        NvRmPowerUnRegister( dsi->hRm, dsi->PowerClientId);

        /* unmap memory*/
        NvRmPhysicalMemUnmap( dsi->VirAddr, dsi->BankSize );

#if DSI_USE_SYNC_POINTS
        // destroy the channel
        NvRmChannelClose(dsi->ch);
        // destroy the sema for waiting on sync point
        NvOsSemaphoreDestroy(dsi->sem);
#endif

        NvOsMutexDestroy(dsi->DsiTransportMutex);
        dsi->DsiTransportMutex = NULL;

        /* Clear the dsi structure initally */
        NvOsMemset( dsi, 0, sizeof(DcDsi) );
    }
}

static NvU8
PrivDsiGetPixelFormat(NvU32 Bpp, NvU32* BytesPerPixel)
{
    NvU32 format = 0;

    switch(Bpp) {
    case 3:
        format = DsiDbiPixelFormats_3;
        *BytesPerPixel = 1;
        break;
    case 8:
        format = DsiDbiPixelFormats_8;
        *BytesPerPixel = 1;
         break;
    case 12:
        format = DsiDbiPixelFormats_12;
        *BytesPerPixel = 2;
        break;
    case 16:
        format = DsiDbiPixelFormats_16;
        *BytesPerPixel = 2;
        break;
    case 18:
        format = DsiDbiPixelFormats_18;
        *BytesPerPixel = 3;
        break;
    case 24:
        format = DsiDbiPixelFormats_24;
        *BytesPerPixel = 3;
        break;
    default:
        *BytesPerPixel = 0;
        NV_ASSERT(!"UnSupported BPP\n");
        break;
    }
    return format;
}

static void
dsi_command_mode_update( DcDsi *dsi, NvOdmDispDeviceHandle hPanel,
    DsiPartialRegionInfo *pUpdate, NvU32 NoOfRegions, NvU8 *pSurfaceData,
    NvU32 SurfaceDataSize )
{
    NvU32 size = 0;
    NvU8 Buf[4];
    NvU8* pData = Buf;
    NvU32 DataSize = 0;
    NvU32 i = 0;
    NvU32 dataSent = 0;
    NvU32 BytesPerPixel = 0;
    NvBool IsBGRFormat = NV_FALSE;
    NvU32 FifoDepth = 0;

    NV_ASSERT(pUpdate);
    NV_ASSERT(pSurfaceData);

    PrivDsiChangeDataFormat(pSurfaceData, SurfaceDataSize,
        pUpdate->BitsPerPixel);

    size = NV_ARRAY_SIZE(s_PartialDispSeq);
    FifoDepth =
        (dsi->VideoMode == NvOdmDsiVideoModeType_DcDrivenCommandMode)?
        DSI_VIDEO_FIFO_DEPTH_BYTES : DSI_HOST_FIFO_DEPTH_BYTES;

    for (i = 0; i < size; i++)
    {
        switch(s_PartialDispSeq[i].DsiCmd)
        {
            case DSI_SET_PIXEL_FORMAT:
                /* set pixel format */
                pData = Buf;
                *pData = PrivDsiGetPixelFormat(pUpdate->BitsPerPixel,
                    &BytesPerPixel);
                DataSize = 1;
                break;

            case DSI_AREA_COLOR_MODE:
                /* Area color code */
                 pData = Buf;
                /** TODO: Check if the data is in packed format or not and
                 * appropriately pass the data for color mode.
                 */
                *pData = 0x0;
                DataSize = 1;
                break;

            case DSI_SET_PARTIAL_AREA:
                /* Partial display start line and end line */
                NV_ASSERT(pUpdate->top_bar < pUpdate->bot_bar ||
                          pUpdate->top_bar >= 0 ||
                          pUpdate->bot_bar > 0);
                pData = Buf;
                pData[0] = pUpdate->top_bar >> DSI_MSB_ADDRESS_MASK ;
                pData[1] = pUpdate->top_bar & 0xFF;
                pData[2] = pUpdate->bot_bar >> DSI_MSB_ADDRESS_MASK;
                pData[3] = pUpdate->bot_bar & 0xFF;
                DataSize = 4;
                break;

            case DSI_SET_ADDRESS_MODE:
                /* Partial display start line and end line */
               /** TODO: Get the incoming data format i.e RGB or BGR and
                * appropriately configure the order.
                */
                pData = Buf;
                pData[0] = 0 |( IsBGRFormat << 3);
                DataSize = 1;
                break;

            case DSI_SET_PAGE_ADDRESS:
                /* Partial display start line and end line */
                pData = Buf;

                pData[0] = pUpdate->top >> DSI_MSB_ADDRESS_MASK ;
                pData[1] = pUpdate->top & 0xFF;
                pData[2] = pUpdate->bottom >> DSI_MSB_ADDRESS_MASK;
                pData[3] = pUpdate->bottom & 0xFF;
                DataSize = 4;
                break;

            case DSI_SET_COLUMN_ADDRESS:
                pData = Buf;
                /* partial display start colum and end column */
                pData[0] = pUpdate->left >> DSI_MSB_ADDRESS_MASK;
                pData[1] = pUpdate->left & 0xFF;
                pData[2] = pUpdate->right >> DSI_MSB_ADDRESS_MASK;
                pData[3] = pUpdate->right & 0xFF;
                DataSize = 4;
                break;

            case DSI_WRITE_MEMORY_START:
                if (dsi->HsSupportForFrameBuffer)
                {
                    /* By now we are already in the command mode,
                       set the HS mode if panel supports HS for frame buffer*/
                    PrivDsiSetHighSpeedMode( dsi, hPanel, NV_TRUE);
                }
                pData = (NvU8*)pSurfaceData;
                DataSize = NV_MIN((SurfaceDataSize),
                    (((FifoDepth/BytesPerPixel) *
                        (BytesPerPixel))));
                dataSent = DataSize;        /* Init dataSent */

                // wait for TE signal before doing the frame update
                if (dsi->TeFrameUpdate)
                {
                    PrivDsiIsTeSignled( dsi );
                }
                break;

            case DSI_WRITE_MEMORY_CONTINUE:
                while (dataSent < SurfaceDataSize)
                {
                    pData = ((NvU8*)pSurfaceData) + dataSent;
                    DataSize = NV_MIN((SurfaceDataSize - dataSent),
                        (((FifoDepth/BytesPerPixel) *
                            (BytesPerPixel))));
                    dataSent += DataSize;       /* Increment dataSent */
                    dsi_write_data( dsi, s_PartialDispSeq[i].DsiCmdType,
                        s_PartialDispSeq[i].DsiCmd, pData, DataSize,
                        s_PartialDispSeq[i].IsLongPacket );
                }
                if (dataSent == SurfaceDataSize)
                {
                    continue;
                }
                else
                {
                    NV_ASSERT(!"NVRM_DSI Partial Data Write Overflow\n");
                }

                break;

            default:
                NV_ASSERT(!"Invalid DSI command for partial mode");
                break;
        }

        if (DataSize > 0)
        {
            dsi_write_data( dsi, s_PartialDispSeq[i].DsiCmdType,
                s_PartialDispSeq[i].DsiCmd,  pData, DataSize,
                s_PartialDispSeq[i].IsLongPacket);
        }
    }
}

NvError
PrivDsiCommandModeUpdate(
    NvRmSurface *surf,
    NvPoint *pSrc,
    NvRect *pUpdate,
    NvU32 DsiInstance,
    NvOdmDispDeviceHandle hPanel)
{
    NvU32 BitsPerPixel;
    NvU32 size = 0;
    DsiPartialRegionInfo Region;
    DcDsi *dsi;
    NvU8 *pSurfaceData = NULL;
    NvError e = NvSuccess;

    dsi = &s_Dsi[DsiInstance];

    if( !dsi->bInit )
    {
        return NvError_NotInitialized;
    }

    // [TODO 2009-08-11 mholtmanns] Null surface call is reserved for future
    // use for now just return
    if (!surf)
    {
        return NvError_InvalidSurface;
    }

    NvOsMutexLock(dsi->DsiTransportMutex);

    BitsPerPixel = NV_COLOR_GET_BPP(surf->ColorFormat);

    Region.top_bar = pUpdate->top;
    Region.bot_bar = pUpdate->bottom;
    Region.left = pSrc->x;
    Region.right = pSrc->x + surf->Width - 1;
    Region.top = pSrc->y;
    Region.bottom = pSrc->y + surf->Height;
    Region.BitsPerPixel = BitsPerPixel;
    size = NvRmSurfaceComputeSize(surf);

    pSurfaceData = NvOsAlloc(size);
    if (!pSurfaceData)
    {
        NV_ASSERT(!"Insufficient Memory\n");
        e = NvError_InsufficientMemory;
        NvOsMutexUnlock(dsi->DsiTransportMutex);
        return e;
    }

    NvRmSurfaceRead(surf, 0, 0, surf->Width, surf->Height,
        (void *)pSurfaceData);

    // Update the DSI panel with the surface data in command mode.
    dsi_command_mode_update( dsi, hPanel, &Region, 1, pSurfaceData,
        size );

    NvOsFree(pSurfaceData);
    pSurfaceData = NULL;
    NvOsMutexUnlock(dsi->DsiTransportMutex);

    return e;
}

NvBool
DcHal_HwDsiCommandModeUpdate(
    NvU32 controller,
    NvRmSurface *surf,
    NvU32 count,
    NvPoint *pSrc,
    NvRect *pUpdate,
    NvU32 DsiInstance,
    NvOdmDispDeviceHandle hPanel )
{
    NvError e;
    DcController *cont;

    DC_CONTROLLER( controller, cont, return NV_FALSE );

    e = PrivDsiCommandModeUpdate( surf, pSrc, pUpdate, DsiInstance, hPanel );
    if ( e != NvSuccess )
    {
        cont->bFailed = NV_TRUE;
        return NV_FALSE;
    }

    return NV_TRUE;
}

static void DcDsiWaitForVSync( NvRmDeviceHandle hRm, DcController *cont)
{
    NvU32 val;
    NvRmFence fence;

    cont->stream.SyncPointID = cont->VsyncSyncPointId;
    cont->stream.SyncPointsUsed = 0;
    cont->stream.LastEngineUsed = NvRmModuleID_Display;

    NVRM_STREAM_BEGIN( &cont->stream, cont->pb, 3 );

    NVRM_STREAM_PUSH_U( cont->pb,
        NVRM_CH_OPCODE_SET_CLASS( NV_HOST1X_CLASS_ID, 0, 0 ) );
    val = NV_DRF_NUM( NV_CLASS_HOST, WAIT_SYNCPT_INCR, INDX,
            cont->VsyncSyncPointId );

    NVRM_STREAM_PUSH_U( cont->pb, NVRM_CH_OPCODE_NONINCR(
        NV_CLASS_HOST_WAIT_SYNCPT_INCR_0, 1 ) );
    NVRM_STREAM_PUSH_U( cont->pb, val );

    NVRM_STREAM_END( &cont->stream, cont->pb );

    NvRmStreamFlush( &cont->stream, &fence );

    NvRmChannelSyncPointWait( hRm, cont->VsyncSyncPointId, fence.Value,
        cont->sem );
}

void
DcHal_HwDsiEnable(
    NvU32 controller,
    NvOdmDispDeviceHandle hPanel,
    NvU32 DsiInstance,
    NvBool enable)
{
    DcController *cont = 0;

    DC_CONTROLLER( controller, cont,
        do { cont->bFailed = NV_TRUE; return; } while( 0 ); );

    if( enable )
    {
        NvError e;
        e = DcDsiEnable(cont, DsiInstance, hPanel);
        if( e != NvSuccess )
        {
            NV_ASSERT( !"dsi enable failed" );
            cont->bFailed = NV_TRUE;
        }
    }
    else
    {
        /* unset the dsi enable bit */
        DcEnable( cont, DC_HAL_DSI, NV_FALSE );
        DcDsiDisable( &s_Dsi[DsiInstance] );
    }
}

void
DcHal_HwDsiSetIndex(
    NvU32 controller,
    NvU32 DsiInstance,
    NvOdmDispDeviceHandle hPanel )
{
    DcDsi *dsi = &s_Dsi[DsiInstance];
    dsi->ControllerIndex = controller;
}

void
DcHal_HwDsiSwitchMode(
    NvRmDeviceHandle hRm,
    NvU32 controller,
    NvU32 DsiInstance,
    NvOdmDispDeviceHandle hPanel )
{
    NvError e = NvSuccess;
    DcController *cont;

    DC_CONTROLLER( controller, cont, return );

    if( cont->mode.flags & NVDDK_DISP_MODE_FLAG_PARTIAL )
    {
        // Abrupt switching of the dsi mode from video to command causes image
        // fade out. So, wait for vblank and then switch the mode from video to
        // partial mode.
        DcDsiWaitForVSync(hRm, cont);
        // stop data from the DC
        DcEnable( cont, DC_HAL_DSI, NV_FALSE );
    }

    NvOdmDispSetMode( hPanel, &cont->mode, NV_FALSE );

    e = DcDsiEnable(cont, DsiInstance, hPanel);
    if( e != NvSuccess )
    {
        cont->bFailed = NV_TRUE;
    }
}

NvError
dsi_enable_command_mode( DcDsi *dsi, NvOdmDispDeviceHandle hPanel )
{
    NV_ASSERT(dsi);

    if( !dsi->bInit )
    {
        return NvError_NotInitialized;
    }

    NvOsMutexLock(dsi->DsiTransportMutex);
    HOST_CLOCK_ENABLE(dsi);

    // Enable the command mode in LP for init command sequence
    PrivDsiEnableCommandMode( dsi, hPanel );

    HOST_CLOCK_DISABLE(dsi);
    NvOsMutexUnlock(dsi->DsiTransportMutex);

    return NvSuccess;
}

/** public exports */

NvBool
DcDsiTransInit(
    NvOdmDispDeviceHandle hDevice,
    NvU32 DsiInstance,
    NvU32 flags )
{
    NvError e;
    NvBool b = NV_FALSE;

    e = PrivDsiTransInit( hDevice, DsiInstance, flags );
    if( e == NvSuccess )
    {
        b = NV_TRUE;
    }
    return b;
}

void DcDsiTransDeinit( NvOdmDispDeviceHandle hDevice, NvU32 DsiInstance )
{
    DcDsiDisable( &s_Dsi[DsiInstance] );
}

NvBool
DcDsiTransWrite(
    NvOdmDispDeviceHandle hDevice,
    NvU32 DataType,
    NvU32 RegAddr,
    NvU32 DataSize,
    NvU8 *pData,
    NvU32 DsiInstance,
    NvBool bLongPacket )
{
    NvError e;
    DcDsi *dsi;

    dsi = &s_Dsi[DsiInstance];

    // FIXME: this won't work unless command mode is enabled

    e = dsi_write_data(
                    dsi,
                    DataType,
                    RegAddr,
                    pData,
                    DataSize,
                    bLongPacket );
    if( e == NvSuccess )
    {
        return NV_TRUE;
    }

    return NV_FALSE;
}

NvBool
DcDsiTransRead(
    NvOdmDispDeviceHandle hDevice,
    NvU8 RegAddr,
    NvU32 DataSize,
    NvU8 *pData,
    NvU32 DsiInstance,
    NvBool bLongPacket )
{
    NvError e;
    DcDsi *dsi;

    dsi = &s_Dsi[DsiInstance];
    e = dsi_read_data(
                    dsi,
                    RegAddr,
                    pData,
                    DataSize,
                    bLongPacket );
    if( e == NvSuccess )
    {
        return NV_TRUE;
    }

    return NV_FALSE;
}

NvBool DcDsiTransEnableCommandMode( NvOdmDispDeviceHandle hDevice, NvU32 DsiInstance )
{
    NvError e;
    DcDsi *dsi;

    dsi = &s_Dsi[DsiInstance];
    e = dsi_enable_command_mode( dsi, hDevice );
    if( e == NvSuccess )
    {
        return NV_TRUE;
    }

    return NV_FALSE;
}

NvBool
DcDsiTransUpdate(
    NvOdmDispDeviceHandle hDevice,
    NvU8 *pSurface,
    NvU32 width,
    NvU32 height,
    NvU32 bpp,
    NvPoint *pSrc,
    NvU32 DsiInstance,
    NvRect *pUpdate )
{
    DsiPartialRegionInfo Region;
    NvU32 size;
    DcDsi *dsi;
    NvU32 BytesPerPixel =0;

    dsi = &s_Dsi[DsiInstance];

    NvOsMutexLock(dsi->DsiTransportMutex);

    Region.top_bar = pUpdate->top;
    Region.bot_bar = pUpdate->bottom;
    Region.left = pSrc->x;
    Region.right = pSrc->x + width - 1;
    Region.top = pSrc->y;
    Region.bottom = pSrc->y + height;
    Region.BitsPerPixel = bpp;
    PrivDsiGetPixelFormat(bpp, &BytesPerPixel);
    size = width * height * BytesPerPixel;

    dsi_command_mode_update( dsi, hDevice, &Region, 1, pSurface, size );

    NvOsMutexUnlock(dsi->DsiTransportMutex);

    return NV_TRUE;
}

NvBool
DcDsiTransGetPhyFreq(
    NvOdmDispDeviceHandle hDevice,
    NvU32 DsiInstance,
    NvU32 *pDsiPhyFreqKHz )
{
    DcDsi *dsi;

    NV_ASSERT(pDsiPhyFreqKHz);
    dsi = &s_Dsi[DsiInstance];

    if( !dsi->bInit )
    {
        *pDsiPhyFreqKHz = 0;
        return NV_TRUE;
    }

    NvOsMutexLock(dsi->DsiTransportMutex);
    *pDsiPhyFreqKHz = dsi->freq;
    NvOsMutexUnlock(dsi->DsiTransportMutex);

    return NV_TRUE;
}

NvBool
DcDsiTransPing(
    NvOdmDispDeviceHandle hDevice,
    NvU8 RegAddr,
    NvU32 DataSize,
    NvU8 *pData,
    NvU32 *pReadResp,
    NvU32 DsiInstance,
    NvU32 flags )
{
    NvError e;
    DcController *cont;
    DcDsi *dsi;
    NvU32 reg, freq;
    NvU32 dsi_control, host_dsi_control, save_freq, host_enable;
    NvBool b = NV_FALSE;
    NvU32 delay_msec = 0;

    dsi = &s_Dsi[DsiInstance];
    DC_CONTROLLER( dsi->ControllerIndex, cont, return NV_FALSE );

    NvOsMutexLock( cont->frame_mutex );

    NvOsMutexLock( dsi->DsiTransportMutex );
    DC_LOCK(cont);
    HOST_CLOCK_ENABLE( dsi );

    /**
     * FIXME: Need to replace delay with a proper fix.
     * Delay is set as time required to send a full frame to
     * the panel multiplied by 3.
     * This delay is added to make sure the fullframe pixel data
     * from the dsi is sent to the panel, so that dsi is idle
     * to do other operations like read.
     */
    delay_msec = (1000/(dsi->RefreshRate));
    NvOsSleepMS(delay_msec * 3);

    /* save original state */
    NV_CHECK_ERROR_CLEANUP(
        NvRmPowerModuleClockConfig( dsi->hRm,
            NVRM_MODULE_ID( NvRmModuleID_Dsi, dsi->DsiInstance ),
            dsi->PowerClientId,
            0, 0, NULL, 0, &save_freq, 0 )
    );

    host_dsi_control = DSI_READ( dsi, DSI_HOST_DSI_CONTROL_0 );
    dsi_control = DSI_READ( dsi, DSI_DSI_CONTROL_0 );

    freq = dsi->LpCmdModeFreqKHz;

    NV_CHECK_ERROR_CLEANUP(
        NvRmPowerModuleClockConfig( dsi->hRm,
            NVRM_MODULE_ID( NvRmModuleID_Dsi, dsi->DsiInstance ),
            dsi->PowerClientId,
            NvRmFreqUnspecified,
            NvRmFreqUnspecified,
            &freq, 1, &dsi->freq, NvRmClockConfig_MipiSync)
    );

    /* Setup DSI Phy timming */
    DsiSetupPhyTiming( NULL, dsi, hDevice );

    /* Setup DSI Threshhold for host mode */
    DSI_WRITE(dsi, DSI_DSI_MAX_THRESHOLD_0 , DSI_HOST_FIFO_DEPTH);

    reg = NV_DRF_NUM( DSI, HOST_DSI_CONTROL, CRC_RESET, 1 )
        | NV_DRF_DEF( DSI, HOST_DSI_CONTROL, DSI_PHY_CLK_DIV, DIV1 )
        | NV_DRF_DEF( DSI, HOST_DSI_CONTROL, ECC_ENABLE, ENABLE )
        | NV_DRF_DEF( DSI, HOST_DSI_CONTROL, CS_ENABLE, ENABLE )
        | NV_DRF_DEF( DSI, HOST_DSI_CONTROL, PKT_BTA, DISABLE )
        | NV_DRF_DEF( DSI, HOST_DSI_CONTROL, IMM_BTA, DISABLE )
        | NV_DRF_DEF( DSI, HOST_DSI_CONTROL, PKT_WR_FIFO_SEL, HOST )
        | NV_DRF_DEF( DSI, HOST_DSI_CONTROL, DSI_HIGH_SPEED_TRANS, LOW )
        | NV_DRF_DEF( DSI, HOST_DSI_CONTROL, RAW_DATA, DISABLE )
        | NV_DRF_DEF( DSI, HOST_DSI_CONTROL, PERIPH_RESET, DISABLE )
        | NV_DRF_DEF( DSI, HOST_DSI_CONTROL, DSI_ULTRA_LOW_POWER, NORMAL )
        | NV_DRF_DEF( DSI, HOST_DSI_CONTROL, HOST_TX_TRIG_SRC,
            IMMEDIATE );
    DSI_WRITE( dsi, DSI_HOST_DSI_CONTROL_0, reg );

    reg = NV_DRF_DEF( DSI, DSI_CONTROL, DSI_HOST_ENABLE, ENABLE )
        | NV_DRF_NUM(DSI, DSI_CONTROL, DSI_NUM_DATA_LANES,
            (dsi->DataLanes - 1) );
    DSI_WRITE( dsi, DSI_DSI_CONTROL_0, reg );

    host_enable = dsi->HostCtrlEnable;
    dsi->HostCtrlEnable = NV_TRUE;
    dsi->bDsiIsInHsMode = NV_FALSE;

    e = dsi_read_data( dsi, RegAddr, pData, DataSize, NV_FALSE );
    if(e == NvSuccess)
    {
        DSI_ERR_PRINT(( "data: %x %x %x %x %x %x\n", pData[0], pData[1],
            pData[2], pData[3], pData[4], e ));
        b = NV_TRUE;
        *pReadResp = NvOdmDsiReadResponse_Success;
    }
    else
    {
       b = NV_FALSE;

       if(e == NvError_DispDsiReadAckError)
           *pReadResp = NvOdmDsiReadResponse_ReadAckError;
       else if(e == NvError_DispDsiReadInvalidResp)
           *pReadResp = NvOdmDsiReadResponse_InvalidResponse;
       else
       {
           *pReadResp = NvOdmDsiReadResponse_NoReadResponse;
           DSI_ERR_PRINT(("Read failed..\n"));
      }
    }

    /* restore original state */

    NV_CHECK_ERROR_CLEANUP(
        NvRmPowerModuleClockConfig( dsi->hRm,
            NVRM_MODULE_ID( NvRmModuleID_Dsi, dsi->DsiInstance ),
            dsi->PowerClientId,
            NvRmFreqUnspecified,
            NvRmFreqUnspecified,
            &save_freq, 1, &dsi->freq, NvRmClockConfig_MipiSync)
    );

    DsiSetupPhyTiming( cont, dsi, hDevice );

    DSI_WRITE( dsi, DSI_HOST_DSI_CONTROL_0, host_dsi_control );
    DSI_WRITE( dsi, DSI_DSI_CONTROL_0, dsi_control );
    dsi->HostCtrlEnable = host_enable;

    goto clean;

fail:
    b = NV_FALSE;

clean:
    HOST_CLOCK_DISABLE( dsi );
    DC_UNLOCK(cont);
    NvOsMutexUnlock( dsi->DsiTransportMutex );
    NvOsMutexUnlock( cont->frame_mutex );
    return b;
}

NvError
NvDdkDispTestDsiInit(
    NvDdkDispDisplayHandle hDisplay,
    NvU32 DsiInstance,
    NvU32 flags )
{
    NvError e = NvError_DispInitFailed;
    DcController *cont;
    NvU32 controller = s_Dsi[DsiInstance].ControllerIndex;

    NV_ASSERT( hDisplay );
    NV_ASSERT( hDisplay->panel );

    DC_CONTROLLER( controller, cont, return NvError_NotInitialized);
    DC_LOCK(cont);
    DcWaitFrame(cont);


    e = PrivDsiTransInit( hDisplay->panel->hPanel, DsiInstance, flags );
    DC_UNLOCK(cont);
    return e;
}

NvError
NvDdkDispTestDsiSetMode(
    NvDdkDispDisplayHandle hDisplay,
    NvU32 DsiInstance,
    NvDdkDispDsiMode Mode )
{
    DcDsi *dsi;
    NvError e = NvError_NotSupported;
    DcController *cont;
    NvU32 controller = s_Dsi[DsiInstance].ControllerIndex;

    NV_ASSERT( hDisplay );
    NV_ASSERT( hDisplay->panel );

    DC_CONTROLLER( controller, cont, return NvError_NotInitialized);
    DC_LOCK(cont);
    DcWaitFrame(cont);

    dsi = &s_Dsi[DsiInstance];
    if( Mode == NvDdkDispDsiMode_DcDrivenCommandMode )
    {
        DcHal_HwDsiEnable(dsi->ControllerIndex, hDisplay->panel->hPanel,
            DsiInstance, NV_FALSE);
        DcHal_HwDsiEnable(dsi->ControllerIndex, hDisplay->panel->hPanel,
            DsiInstance, NV_TRUE);
        e = NvSuccess;
    }
    else if( Mode == NvDdkDispDsiMode_CommandMode )
    {
        e = dsi_enable_command_mode( dsi, hDisplay->panel->hPanel );
    }

    DC_UNLOCK(cont);
    return e;
}

NvError
NvDdkDispTestDsiSendCommand(
    NvDdkDispDisplayHandle hDisplay,
    NvU32 DsiInstance,
    NvDdkDispDsiCommand *pCommand,
    NvU32 DataSize,
    NvU8 *pData )
{
    DcDsi *dsi;
    NvError e = NvError_DispInitFailed;
    DcController *cont;
    NvU32 controller = s_Dsi[DsiInstance].ControllerIndex;

    NV_ASSERT( pCommand );
    NV_ASSERT( pData );

    DC_CONTROLLER( controller, cont, return NvError_NotInitialized);
    DC_LOCK(cont);
    DcWaitFrame(cont);

    dsi = &s_Dsi[DsiInstance];
    e = dsi_write_data(
        dsi,
        pCommand->CommandType,
        pCommand->Command,
        pData,
        DataSize,
        pCommand->bIsLongPacket);

    DC_UNLOCK(cont);
    return e;
}

NvError
NvDdkDispTestDsiRead(
    NvDdkDispDisplayHandle hDisplay,
    NvU32 DsiInstance,
    NvU32 Addr,
    NvU32 DataSize,
    NvU8 *pData )
{
    NvError e = NvSuccess;
    NvU32 resp = 0;
    NvBool b = NV_FALSE;
    DcController *cont;
    NvU32 controller = s_Dsi[DsiInstance].ControllerIndex;

    NV_ASSERT( DataSize );
    NV_ASSERT( pData );

    DC_CONTROLLER( controller, cont, return NvError_NotInitialized);
    DC_LOCK(cont);
    DcWaitFrame(cont);

    b = DcDsiTransPing(
                    hDisplay->panel->hPanel,
                    Addr,
                    DataSize,
                    pData,
                    &resp,
                    DsiInstance,
                    0);
    if(!b)
    {
        if(resp == NvOdmDsiReadResponse_ReadAckError)
            e = NvError_DispDsiReadAckError;
        else if(resp == NvOdmDsiReadResponse_InvalidResponse)
            e = NvError_DispDsiReadInvalidResp;
        else
            e = NvError_Timeout;
    }

    DC_UNLOCK(cont);
    return e;
}

NvError
NvDdkDispTestDsiUpdate(
    NvDdkDispDisplayHandle hDisplay,
    NvU32 DsiInstance,
    NvRmSurface *surf,
    NvPoint *pSrc,
    NvRect *pUpdate)
{
    NvError e = NvError_DispInitFailed;
    DcController *cont;
    NvU32 controller = s_Dsi[DsiInstance].ControllerIndex;

    NV_ASSERT( hDisplay );
    NV_ASSERT( hDisplay->panel );
    NV_ASSERT( pSrc );
    NV_ASSERT( pUpdate );

    DC_CONTROLLER( controller, cont, return NvError_NotInitialized);
    DC_LOCK(cont);
    DcWaitFrame(cont);

    e = PrivDsiCommandModeUpdate(
                            surf,
                            pSrc,
                            pUpdate,
                            DsiInstance,
                            hDisplay->panel->hPanel);

    DC_UNLOCK(cont);
    return e;
}

