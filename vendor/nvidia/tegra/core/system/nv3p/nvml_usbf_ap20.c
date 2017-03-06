/*
 * Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvos.h"
#include "nvassert.h"
#include "nvrm_drf.h"
#include "nvml_usbf_common.h"
#include "nvboot_misc_ap20.h"
#include "ap20/include/nvboot_util.h"
#include "ap20/include/nvboot_hardware_access.h"
#include "ap20/arusb.h"
#include "ap20/arapb_misc.h"
#include "ap20/arclk_rst.h"

#define NV_ADDRESS_MAP_CAR_BASE         0x60006000
#define NV_ADDRESS_MAP_TMRUS_BASE       0x60005010
#define NV_ADDRESS_MAP_USB_BASE         0xC5000000
#define NV_ADDRESS_MAP_FUSE_BASE        0x7000F800
#define NV_ADDRESS_MAP_APB_MISC_BASE    0x70000000
#define NV_ADDRESS_MAP_DATAMEM_BASE     0x40000000

//USB transfer buffer offset in IRAM, these macros are used in nvml_usbf.c
#define NV3P_TRANSFER_EP0_BUFFER_OFFSET NV3P_TRANSFER_EP0_BUFFER_OFFSET_AP20
#define NV3P_TRANSFER_EP_BULK_BUFFER_OFFSET NV3P_TRANSFER_EP_BULK_BUFFER_OFFSET_AP20
#define NV3P_USB_BUFFER_OFFSET NV3P_USB_BUFFER_OFFSET_AP20


/* Golbal variables*/
/**
 * USB data store consists of Queue head and device transfer descriptors.
 * This structure must be aligned to 2048 bytes and must be uncached.
 */
static NvBootUsbDescriptorData *s_pUsbDescriptorBuf = NULL;
/**
 * Usbf context record and its pointer used locally by the driver
 */
static NvBootUsbfContext s_UsbfContext;
static NvBootUsbfContext *s_pUsbfCtxt = NULL;


///////////////////////////////////////////////////////////////////////////////////////////////////
//  PLLU configuration information (reference clock is osc/clk_m and PLLU-FOs are fixed at 12MHz/60MHz/480MHz)
//
//  reference frequency         13.0MHz         19.2MHz         12.0MHz         26.0MHz
//  ---------------------------------------------------------------------------------------
//      DIVN                    960 (3c0h)      200 (0c8h)      960 (3c0h)      960 (3c0h)
//      DIVM                    13 ( 0dh)        4 ( 04h)       12 ( 0ch)       26 (1ah)
// Filter frequency (MHz)       1               4.8             6           2
// CPCON                        1100b           0011b           1100b       1100b
// LFCON0                       0               0               0           0
///////////////////////////////////////////////////////////////////////////////
static const UsbPllClockParams s_UsbPllBaseInfo[NvBootClocksOscFreq_Num] =
{
    //DivN, DivM, DivP, CPCON,  LFCON
    {0x3C0, 0x0D, 0x00, 0xC,      0}, // For NvBootClocksOscFreq_13
    {0x0C8, 0x04, 0x00, 0x3,      0}, // For NvBootClocksOscFreq_19_2
    {0x3C0, 0x0C, 0x00, 0xC,      0}, // For NvBootClocksOscFreq_12
    {0x3C0, 0x1A, 0x00, 0xC,      0}  // For NvBootClocksOscFreq_26
};

///////////////////////////////////////////////////////////////////////////////
// PLL CONFIGURATION & PARAMETERS: refer to the arapb_misc_utmip.spec file.
///////////////////////////////////////////////////////////////////////////////
// PLL CONFIGURATION & PARAMETERS for different clock generators:
//-----------------------------------------------------------------------------
// Reference frequency     13.0MHz         19.2MHz         12.0MHz     26.0MHz
// ----------------------------------------------------------------------------
// PLLU_ENABLE_DLY_COUNT   02 (02h)        03 (03h)        02 (02h)    04 (04h)
// PLLU_STABLE_COUNT       51 (33h)        75 (4Bh)        47 (2Fh)   102 (66h)
// PLL_ACTIVE_DLY_COUNT    05 (05h)        06 (06h)        04 (04h)    09 (09h)
// XTAL_FREQ_COUNT        127 (7Fh)       187 (BBh)       118 (76h)   254 (FEh)
///////////////////////////////////////////////////////////////////////////////
static const UsbPllDelayParams s_UsbPllDelayParams[NvBootClocksOscFreq_Num] =
{
    //ENABLE_DLY,  STABLE_CNT,  ACTIVE_DLY,  XTAL_FREQ_CNT
    {0x02,         0x33,        0x05,        0x7F}, // For NvBootClocksOscFreq_13
    {0x03,         0x4B,        0x06,        0xBB}, // For NvBootClocksOscFreq_19_2
    {0x02,         0x2F,        0x04,        0x76}, // For NvBootClocksOscFreq_12
    {0x04,         0x66,        0x09,        0xFE}  // For NvBootClocksOscFreq_26
};

///////////////////////////////////////////////////////////////////////////////
// Tracking Length Time: The tracking circuit of the bias cell consumes a
// measurable portion of the USB idle power  To curtail this power consumption
// the bias pad has added a PD_TDK signal to power down the bias cell. It is
// estimated that after 20microsec of bias cell operation the PD_TRK signal can
// be turned high to sve power. This can be automated by programming a timing
// interval as given in the below structure.
static const NvU32 s_UsbBiasTrkLengthTime[NvBootClocksOscFreq_Num] =
{
    /* 20 micro seconds delay after bias cell operation */
    5,  // For NvBootClocksOscFreq_13
    7,  // For NvBootClocksOscFreq_19_2
    5,  // For NvBootClocksOscFreq_12
    9   // For NvBootClocksOscFreq_26
};

///////////////////////////////////////////////////////////////////////////////
// Debounce values IdDig, Avalid, Bvalid, VbusValid, VbusWakeUp, and SessEnd.
// Each of these signals have their own debouncer and for each of those one out
// of 2 debouncing times can be chosen (BIAS_DEBOUNCE_A or BIAS_DEBOUNCE_B)
//
// The values of DEBOUNCE_A and DEBOUNCE_B are calculated as follows:
// 0xffff -> No debouncing at all
// <n> ms = <n> *1000 / (1/19.2MHz) / 4
// So to program a 1 ms debounce for BIAS_DEBOUNCE_A, we have:
// BIAS_DEBOUNCE_A[15:0] = 1000 * 19.2 / 4  = 4800 = 0x12c0
// We need to use only DebounceA for BOOTROM. We don't need the DebounceB
// values, so we can keep those to default.
///////////////////////////////////////////////////////////////////////////////
static const NvU32 s_UsbBiasDebounceATime[NvBootClocksOscFreq_Num] =
{
    /* Ten milli second delay for BIAS_DEBOUNCE_A */
    0x7EF4,  // For NvBootClocksOscFreq_13
    0xBB80,  // For NvBootClocksOscFreq_19_2
    0x7530,  // For NvBootClocksOscFreq_12
    0xFDE8   // For NvBootClocksOscFreq_26
};

////////////////////////////////////////////////////////////////////////////////
// The following arapb_misc_utmip.spec fields need to be programmed to ensure
// correct operation of the UTMIP block:
// Production settings :
//        'HS_SYNC_START_DLY' : 0,
//        'IDLE_WAIT'         : 17,
//        'ELASTIC_LIMIT'     : 16,
// All other fields can use the default reset values.
// Setting the fields above, together with default values of the other fields,
// results in programming the registers below as follows:
//         UTMIP_HSRX_CFG0 = 0x9168c000
//         UTMIP_HSRX_CFG1 = 0x13
////////////////////////////////////////////////////////////////////////////////
//UTMIP Idle Wait Delay
static const NvU8 s_UtmipIdleWaitDelay    = 17;
//UTMIP Elastic limit
static const NvU8 s_UtmipElasticLimit     = 16;
//UTMIP High Speed Sync Start Delay
static const NvU8 s_UtmipHsSyncStartDelay = 0;


static void NvMlPrivUsbInitAp20(void)
{
    NvU32 RegVal = 0;
    NvBootClocksOscFreq OscFreq;
    NvU32 PlluStableTime = 0;

    // Get the current Oscillator frequency
    OscFreq = NvBootClocksGetOscFreqAp20();

    // Enable PLL U for USB
    NvBootClocksStartPllAp20(NvBootClocksPllId_PllU,
                         s_UsbPllBaseInfo[OscFreq].M,
                         s_UsbPllBaseInfo[OscFreq].N,
                         s_UsbPllBaseInfo[OscFreq].P,
                         s_UsbPllBaseInfo[OscFreq].CPCON,
                         s_UsbPllBaseInfo[OscFreq].LFCON,
                         &PlluStableTime);

    // Enable clock to the USB controller
    NvBootClocksSetEnableAp20(NvBootClocksClockId_UsbId, NV_TRUE);
    // Reset the USB controller
    NvBootResetSetEnableAp20(NvBootResetDeviceId_UsbId, NV_TRUE);
    NvBootResetSetEnableAp20(NvBootResetDeviceId_UsbId, NV_FALSE);

    // Set USB1_NO_LEGACY_MODE to 1 to use new UTMIP registers in arusb.h.
    USBIF_REG_UPDATE_DEF(NV_ADDRESS_MAP_USB_BASE,
                         USB1_LEGACY_CTRL,
                         USB1_NO_LEGACY_MODE,
                         NEW);

    USBIF_REG_UPDATE_DEF(NV_ADDRESS_MAP_USB_BASE, USB_SUSP_CTRL, UTMIP_RESET, ENABLE);

    // Stop crystal clock by setting UTMIP_PHY_XTAL_CLOCKEN low
    UTMIP_REG_UPDATE_DEF(NV_ADDRESS_MAP_USB_BASE,
                         MISC_CFG1,
                         UTMIP_PHY_XTAL_CLOCKEN,
                         SW_DEFAULT);

    /*
     * To Use the A Session Valid for cable detection logic,
     * VBUS_WAKEUP mux must be switched to actually use a_sess_vld
     * threshold.  This is done by setting VBUS_SENSE_CTL bit in
     * USB_LEGACY_CTRL register.
     */
    USBIF_REG_UPDATE_DEF(NV_ADDRESS_MAP_USB_BASE,
                         USB1_LEGACY_CTRL,
                         USB1_VBUS_SENSE_CTL,
                         A_SESS_VLD);

    // PLL Delay CONFIGURATION settings
    // The following parameters control the bring up of the plls:
    RegVal = UTMIP_REG_RD(NV_ADDRESS_MAP_USB_BASE, MISC_CFG1);
    RegVal = NV_FLD_SET_DRF_NUM(USB1_UTMIP,
                        MISC_CFG1,
                        UTMIP_PLLU_STABLE_COUNT,
                        s_UsbPllDelayParams[OscFreq].StableCount,
                        RegVal);

    RegVal = NV_FLD_SET_DRF_NUM(USB1_UTMIP, MISC_CFG1,
                        UTMIP_PLL_ACTIVE_DLY_COUNT,
                        s_UsbPllDelayParams[OscFreq].ActiveDelayCount,
                        RegVal);

    UTMIP_REG_WR(NV_ADDRESS_MAP_USB_BASE, MISC_CFG1, RegVal);

    // Set PLL enable delay count and Crystal frequency count
    RegVal = UTMIP_REG_RD(NV_ADDRESS_MAP_USB_BASE, PLL_CFG1);
    RegVal = NV_FLD_SET_DRF_NUM(USB1_UTMIP, PLL_CFG1,
                    UTMIP_PLLU_ENABLE_DLY_COUNT,
                    s_UsbPllDelayParams[OscFreq].EnableDelayCount,
                    RegVal);

    RegVal = NV_FLD_SET_DRF_NUM(USB1_UTMIP, PLL_CFG1,
                    UTMIP_XTAL_FREQ_COUNT,
                    s_UsbPllDelayParams[OscFreq].XtalFreqCount,
                    RegVal);

    UTMIP_REG_WR(NV_ADDRESS_MAP_USB_BASE, PLL_CFG1, RegVal);

    // Setting the tracking length time.
    UTMIP_REG_UPDATE_NUM(NV_ADDRESS_MAP_USB_BASE, BIAS_CFG1,
                UTMIP_BIAS_PDTRK_COUNT,
                s_UsbBiasTrkLengthTime[OscFreq]);

    // Program 1ms Debounce time for VBUS to become valid.
    UTMIP_REG_UPDATE_NUM(NV_ADDRESS_MAP_USB_BASE, DEBOUNCE_CFG0,
                UTMIP_BIAS_DEBOUNCE_A,
                s_UsbBiasDebounceATime[OscFreq]);

    // Disable Batery charge enabling bit set to '1' for disable
    UTMIP_REG_UPDATE_NUM(NV_ADDRESS_MAP_USB_BASE, BAT_CHRG_CFG0, UTMIP_PD_CHRG, 1);

    RegVal = UTMIP_REG_RD(NV_ADDRESS_MAP_USB_BASE, XCVR_CFG0);
    // UTMIP_XCVR_SETUP setting to 9
    RegVal = NV_FLD_SET_DRF_NUM(USB1_UTMIP, XCVR_CFG0,
                    UTMIP_XCVR_SETUP,
                    0x9,
                    RegVal);
    // Set UTMIP_XCVR_LSBIAS_SEL to 0
    RegVal = NV_FLD_SET_DRF_NUM(USB1_UTMIP, XCVR_CFG0,
                    UTMIP_XCVR_LSBIAS_SEL,
                    0x0,
                    RegVal);
    // Set UTMIP_XCVR_HSSLEW_MSB to 0
    RegVal = NV_FLD_SET_DRF_NUM(USB1_UTMIP, XCVR_CFG0,
                    UTMIP_XCVR_HSSLEW_MSB,
                    0x0,
                    RegVal);
    UTMIP_REG_WR(NV_ADDRESS_MAP_USB_BASE, XCVR_CFG0, RegVal);

    UTMIP_REG_UPDATE_NUM(NV_ADDRESS_MAP_USB_BASE, XCVR_CFG1, UTMIP_XCVR_TERM_RANGE_ADJ, 0x6);

    /* Configure the UTMIP_IDLE_WAIT and UTMIP_ELASTIC_LIMIT
     * Setting these fields, together with default values of the other
     * fields, results in programming the registers below as follows:
     *         UTMIP_HSRX_CFG0 = 0x9168c000
     *         UTMIP_HSRX_CFG1 = 0x13
     */

    // Set PLL enable delay count and Crystal frequency count
    RegVal = UTMIP_REG_RD(NV_ADDRESS_MAP_USB_BASE, HSRX_CFG0);
    RegVal = NV_FLD_SET_DRF_NUM(USB1_UTMIP,
                    HSRX_CFG0,
                    UTMIP_IDLE_WAIT,
                    s_UtmipIdleWaitDelay,
                    RegVal);

    RegVal = NV_FLD_SET_DRF_NUM(USB1_UTMIP,
                    HSRX_CFG0,
                    UTMIP_ELASTIC_LIMIT,
                    s_UtmipElasticLimit,
                    RegVal);

    UTMIP_REG_WR(NV_ADDRESS_MAP_USB_BASE, HSRX_CFG0, RegVal);

    // Configure the UTMIP_HS_SYNC_START_DLY
    UTMIP_REG_UPDATE_NUM(NV_ADDRESS_MAP_USB_BASE, HSRX_CFG1,
                UTMIP_HS_SYNC_START_DLY,
                s_UtmipHsSyncStartDelay);


    // Resuscitate  crystal clock by setting UTMIP_PHY_XTAL_CLOCKEN
    UTMIP_REG_UPDATE_DEF(NV_ADDRESS_MAP_USB_BASE,
                         MISC_CFG1,
                         UTMIP_PHY_XTAL_CLOCKEN,
                         DEFAULT);

}

static NvBool NvMlPrivUsbfIsCableConnectedAp20(NvBootUsbfContext *pUsbContext)
{
    // Return the A Session valid status for cable detection
    // A session valid is "1" = NV_TRUE means cable is present
    // A session valid is "0" = NV_FALSE means cable is not present
    return (USBIF_DRF_VAL(USB_PHY_VBUS_SENSORS,
                          A_SESS_VLD_STS,
                          USBIF_REG_RD(NV_ADDRESS_MAP_USB_BASE, USB_PHY_VBUS_SENSORS)));

}

static NvBootError NvMlPrivUsbfHwEnableControllerAp20(void)
{
    NvBootError ErrorValue = NvBootError_Success;
    NvU32 PhyClkValid = 0;
    NvU32 TimeOut = 0;

    //Bring respective USB and PHY out of reset by writing 0 to UTMIP_RESET
    USBIF_REG_UPDATE_DEF(NV_ADDRESS_MAP_USB_BASE,
                         USB_SUSP_CTRL,
                         UTMIP_RESET,
                         DISABLE);

    TimeOut = CONTROLLER_HW_TIMEOUT_US;
    do {
        //Wait for the phy clock to become valid
        PhyClkValid = USBIF_REG_READ_VAL(NV_ADDRESS_MAP_USB_BASE,
                                         USB_SUSP_CTRL,
                                         USB_PHY_CLK_VALID);
        if (!TimeOut)
        {
            break;
        }
        NvOsWaitUS(1);
        TimeOut--;
    } while (!PhyClkValid);
    return ErrorValue;
}

static NvBootError NvMlPrivUsbWaitClockAp20(void)
{
    NvU32 TimeOut = CONTROLLER_HW_TIMEOUT_US;
    NvU32 PhyClkValid = 0;

    do {
        //wait for the phy clock to become valid
        PhyClkValid = USBIF_REG_READ_VAL(NV_ADDRESS_MAP_USB_BASE,
                                         USB_SUSP_CTRL,
                                         USB_PHY_CLK_VALID);
        if (!TimeOut)
        {
            return NvBootError_HwTimeOut;
        }
        NvOsWaitUS(1);
        TimeOut--;
    } while (!PhyClkValid);

    return NvBootError_Success;
}

static void NvMlPrivInitBaseAddressAp20(NvBootUsbfContext * pUsbContext)
{
    // Return the base address to USB_BASE, as only USB1 can be used for
    // recovery mode.
    pUsbContext->UsbBaseAddr = NV_ADDRESS_MAP_USB_BASE;
}

static void NvMlPrivGetPortSpeedAp20(NvBootUsbfContext * pUsbContext)
{
    pUsbContext->UsbPortSpeed = (NvBootUsbfPortSpeed)
                                                                USB_REG_READ_VAL(PORTSC1, PSPD);
}


#define NVML_FUN_SOC(FN,ARG) FN##Ap20 ARG
#define NVBOOT_FUN_SOC(FN,ARG) FN##Ap20 ARG
#include "nvml_usbf.c"
#undef NVML_FUN_SOC
#undef NVBOOT_FUN_SOC
