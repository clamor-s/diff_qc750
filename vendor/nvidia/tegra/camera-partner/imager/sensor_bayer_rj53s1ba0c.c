/*
 * Copyright (c) 2008-2012, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "sensor_bayer_rj53s1ba0c.h"
#include "focuser.h"
#include "imager_util.h"
#include "nvodm_query_gpio.h"

#include "ap15_sharp_rj53_camera_config.h"

/****************************************************************************
 * Implementing for a New Sensor:
 *
 * When implementing for a new sensor from this template, there are four
 * phases:
 * Phase 0: Gather sensor information. In phase 0, we should gather the sensor
 *          information for bringup such as sensor capabilities, set mode i2c
 *          write sequence, NvOdmIoAddress in nvodm_query_discovery_imager.h,
 *          and so on.
 * Phase 1: Bring up a sensor. After phase 1, we should be able to do a basic
 *          still capture with minimal interactions between camera driver
 *          and the sensor
 * Phase 2: Calibrate a sensor. After phase 2, we should be able to calibrate
 *          a sensor. Calibration will need some interactions between camera
 *          driver and the sensor such as setting exposure, gains, and so on.
 * Phase 3: Fully functioning sensor. After phase 3, the sensor should be fully
 *          functioning as described in nvodm_imager.h
 *
 * To help implementing for a new sensor, the template code will be marked as
 * Phase 0, 1, 2, or 3.
 ****************************************************************************/


/**
 * Sensor default settings. Phase 2. Sensor Dependent.
 * #defines make the code more readable and helps maintainability
 * The template tries to keep the list reduced to just things that
 * are used more than once.  Ideally, all numbers in the code would use
 * a #define.
 */
#define SENSOR_BAYER_DEFAULT_EXPOSURE_FINE      (0x0190)
#define SENSOR_BAYER_DEFAULT_PLL_MULT           (0x0064)
#define SENSOR_BAYER_DEFAULT_PLL_PRE_DIV        (0x0004)
#define SENSOR_BAYER_DEFAULT_PLL_VT_PIX_DIV     (0x000A)
#define SENSOR_BAYER_DEFAULT_PLL_VT_SYS_DIV     (0x0001)
#define SENSOR_BAYER_DEFAULT_MAX_GAIN           (0x0070)
#define SENSOR_BAYER_DEFAULT_MIN_GAIN           (0x0000)
#define SENSOR_BAYER_DEFAULT_GAIN_R             (0x0020)
#define SENSOR_BAYER_DEFAULT_GAIN_GR            (0x0020)
#define SENSOR_BAYER_DEFAULT_GAIN_GB            (0x0020)
#define SENSOR_BAYER_DEFAULT_GAIN_B             (0x0020)
#define SENSOR_BAYER_DEFAULT_MAX_FRAME_LENGTH   (0xFFFF)
#define SENSOR_BAYER_DEFAULT_MIN_FRAME_LENGTH   (1976+21)

#define SENSOR_BAYER_DEFAULT_LINE_LENGTH_FULL       (0x1220)
#define SENSOR_BAYER_DEFAULT_FRAME_LENGTH_FULL      (0x07D0)
#define SENSOR_BAYER_DEFAULT_EXPOSURE_COARSE_FULL   (0x07D0)

#define SENSOR_BAYER_DEFAULT_LINE_LENGTH_QUAR       (0x0910)
#define SENSOR_BAYER_DEFAULT_FRAME_LENGTH_QUAR      (0x03E8)
#define SENSOR_BAYER_DEFAULT_EXPOSURE_COARSE_QUAR   (0x03E8)

#define SENSOR_GAIN_TO_F32(x)     (128.f / (128.f - (NvF32)(x)))
#define SENSOR_F32_TO_GAIN(x)     ((NvU32)(128.f - (128.f / (x))))


static const char *pOverrideFiles[] =
{
    "/data/camera_overrides.isp",
};

/**
 * ----------------------------------------------------------
 *  Start of Phase 0 code
 * ----------------------------------------------------------
 */

/**
 * Sensor Specific Variables/Defines. Phase 0.
 *
 * If a new sensor code is created from this template, the following
 * variable/defines should be created for the new sensor specifically.
 * 1. A sensor capabilities (NvOdmImagerCapabilities) for this specific sensor
 * 2. A sensor set mode sequence list  (an array of SensorSetModeSequence)
 *    consisting of pairs of a sensor mode and corresponding set mode sequence
 * 3. A set mode sequence for each mode defined in 2.
 */

/**
 * Sensor bayer's capabilities. Phase 0. Sensor Dependent.
 * This is the first thing the camera driver requests. The information here
 * is used to setup the proper interface code (CSI, VIP, pixel type), start
 * up the clocks at the proper speeds, and so forth.
 * For Phase 0, one could start with these below as an example for a bayer
 * sensor and just update clocks from what is given in the reference documents
 * for that device. The FAE may have specified the clocks when giving the
 * reccommended i2c settings.
 */
static NvOdmImagerCapabilities g_SensorBayerCaps =
{
    "Sharp RJ 5300",  // string identifier, used for debugging
    NvOdmImagerSensorInterface_Parallel10, // interconnect type
    {
        NvOdmImagerPixelType_BayerRGGB,
    },
    0, // focusing positions, ignored if FOCUSER_GUID is zero
    NvOdmImagerOrientation_0_Degrees, // rotation with respect to main display
    NvOdmImagerDirection_Away,          // toward or away the main display
    6000, // initial sensor clock (in khz)
    { {22000, 75.f/22.f}, // preferred clock profile
    },
    {
        NvOdmImagerSyncEdge_Rising, // hsync polarity
        NvOdmImagerSyncEdge_Rising, // vsync polarity
        NV_TRUE,                   // uses mclk on vgp0
    }, // parallel settings (VIP)
    { 0 }, // serial settings (CSI) (not used)
    { 16, 7 }, // min blank time, shouldn't need adjustment
    0, // preferred mode, which resolution to use on startup
    BH6459_GUID, // FOCUSER_GUID, only set if focuser code available
    0, // FLASH_GUID, only set if flash device code is available
    NVODM_IMAGER_CAPABILITIES_END
};

/**
 * SetMode Sequence for 2592x1944. Phase 0. Sensor Dependent.
 * This sequence should put sensor in streaming mode for 2592x1944
 * This is usually given by the FAE or the sensor vendor.
 */
static DevCtrlReg16 SetModeSequence_2632x1976[] =
{
    {0x0103, 0x01},              //software_reset
    {SEQUENCE_WAIT_MS, 1000},
    {0x0111, 0x01},              //CCP_signalling_mode = strobe
    {0x0301, 0x0A},              //vt_pix_clk_div=10
    {0x0303, 0x01},              //vt_sys_clk_div=1
    {0x0305, 0x04},              //pre_pll_clk_div=4
    {0x0307, SENSOR_BAYER_DEFAULT_PLL_MULT},           //pll_multiplier=91
    // {0x0307, 0x74},           //pll_multiplier=91
    {0x0309, 0x08},              //op_pix_clk_div=8
    {0x030B, 0x02},              //op_sys_clk_div=2

    //Fast SetMode sequence starts here
    {SEQUENCE_FAST_SETMODE_START, 0x00},
    {0x0340, 0x07},              //frame_length_lines
    {0x0341, 0xD0},
    {0x0342, 0x12},              //line_length_pck
    {0x0343, 0x20},
    {0x0344, 0x00},              //x_addr_start
    {0x0345, 0x08},
    {0x0346, 0x00},              //y_addr_start
    {0x0347, 0x08},
    {0x0348, 0x0A},              //x_addr_end
    {0x0349, 0x53},
    {0x034a, 0x07},              //y_addr_end
    {0x034B, 0xBF},
    {0x034C, 0x0A},              //x_output_size
    {0x034D, 0x4C},
    {0x034E, 0x07},              //y_output_size
    {0x034F, 0xB8},
    {0x0381, 0x01},
    {0x0383, 0x01},              //01  =1/2
    {0x0385, 0x01},
    {0x0387, 0x01},              //01  =1/2
    {0x0202, 0x07},              // coarse integration time
    {0x0203, 0xD0},              // ^ 0328=1/15s

    //Fast SetMode sequence ends here
    {SEQUENCE_FAST_SETMODE_END, 0x00},

    {0x0205, 0x20},              //analog gain (70 = x8, 60 = x4, 00 = x1)

    {0x3003, 0x02},           //
    {0x3004, 0x07},           //
    {0x3063, 0xC0},           //FF //
    {0x3079, 0x08},           //
    {0x307A, 0xFC},           //
    {0x307B, 0x14},           //FF //
    {0x307C, 0x60},           //
    {0x307E, 0x01},           //
    {0x307F, 0x08},           // median filter OFF(0x38) ON(0x08)
    {0x3300, 0x02},           //.TRNS_RD_CYC A6
    {0x3301, 0xAB},           //.TRNS_RD_CYC A6
    {0x3308, 0x02},           // READOUT
    {0x3309, 0xAC},           // READOUT
    {0x3312, 0x00},           // RST_RD_S1
    {0x3313, 0x73},           // RST_RD_S1[7:0]
    {0x3318, 0x02},           // RST_RD_R2
    {0x3319, 0xAC},           // RST_RD_R2[7:0]
    {0x3326, 0x01},           // TXCLK_RD_S1
    {0x3327, 0x86},           // TXCLK_RD_S1
    {0x3328, 0x01},           // TXCLK_RD_R1
    {0x3329, 0xEB},           // TXCLK_RD_R1
    {0x3332, 0x00},           // VR28SET_RD_R
    {0x3333, 0x75},           // VR28SET_RD_R
    {0x3336, 0x02},           // ADREN_RD_R
    {0x3337, 0xAC},           // ADREN_RD_R
    {0x3338, 0x00},           // XSDCEN_RD_S
    {0x3339, 0x75},           // XSDCEN_RD_S[7:0]
    {0x333A, 0x02},           // XSDCEN_RD_R
    {0x333B, 0xAC},           // XSDCEN_RD_R[7:0]
    {0x333C, 0x00},           // XBINT_RD_S[7:0]
    {0x333D, 0x78},           // XBINT_RD_S[7:0]
    {0x333E, 0x02},           // XBINT_RD_S
    {0x333F, 0xAC},           // XBINT_RD_R[7:0]
    {0x3346, 0x00},           //
    {0x3347, 0x00},           //
    {0x334A, 0x00},           //
    {0x334B, 0x00},           //

    {0x3364, 0x09},           //
    {0x336E, 0x07},           // Dummy OB Address
    {0x3338, 0x00},           // RTN_RD_S1
    {0x3389, 0x00},           // RTN_RD_S1[7:0]
    {0x338c, 0x00},           // RTN_RD_S2[9:8]
    {0x338D, 0x00},           // RTN_RD_S2[7:0]
    {0x342E, 0x04},
    {0x342F, 0x01},
    {0x343C, 0x01},           //
    {0x3500, 0x80},           // VTX 2.8V
    {0x3501, 0x00},
    {0x3502, 0x52},           // VRD
    {0x3504, 0x03},
    {0x3530, 0xAA},           // XBINT level
    {0x3531, 0x01},
    {0x3532, 0x40},           // RST_Gain_INT
    {0x3533, 0x62},           // PIXEL_LOAD_CONTRO
    {0x3534, 0x84},           //
    {0x3535, 0x1A},           // Pixel load current
    {0x3537, 0x03},           // AD FS voltage (0:650mV,1:735mV ...)
    {0x353B, 0x20},           // XBINT_SELEN
    {0x353D, 0x02},           // VR Floating
    {0x3716, 0x38},           // DC CLAMP CODE
    {0x371A, 0x03},
    {0x371B, 0x30},
    {0x371C, 0x30},
    {0x371D, 0x38},           // DC CLAMP CODE
    {0x371F, 0x50},           //
    {0x3721, 0x0F},           // SUNSPOT_H2[9:8],SUNSPOT_H1[9:8],CAD_RESERVE1,VOB_ST
    {0x3722, 0xFF},           // SUNSPOT_H1[7:0]
    {0x3723, 0xFF},           // SUNSPOT_H2[7:0]
    {0x3724, 0x00},           // SUNSPOT_LEV
    {0x3725, 0x00},           // SUNSPOT_IRIS_LEV[15:8]
    {0x3726, 0x10},           // SUNSPOT_IRIS_LEV[7:0]
    {0x3727, 0x02},           //
    {0x3728, 0x03},           //
    {0x3729, 0xFF},           //
    {0x372A, 0x50},           // Pre_COMP_NUM_H
    {0x372B, 0x04},           // Pre_COMP_NUM_L
    {0x372C, 0x20},           // Corp_OFF_NUM
    {0x372D, 0x04},           // Judge_Interval
    {0x3802, 0x01},           // D_RS_R
    {0x3803, 0x6B},           // AD_RS_R
    {0x3806, 0x01},           // AD_RSD_R
    {0x3807, 0xEB},           // AD_RSD_R
    {0x3808, 0x00},           //
    {0x3809, 0x03},           //
    {0x380A, 0x01},           // AD_SSA_R
    {0x380B, 0x80},           // AD_SSA_R
    {0x380C, 0x01},           // AD_SSB_
    {0x380D, 0xF0},           // AD_SSB_S
    {0x380E, 0x02},           // AD_SSB_R
    {0x380f, 0xAB},           // AD_SSB_R
    {0x3810, 0x02},           // AD_R1_S
    {0x3811, 0xAC},           // AD_R1_S
    {0x3812, 0x01},           //
    {0x3813, 0xEB},           //
    {0x381A, 0x02},           // AD_RAMP_AZ_R
    {0x381B, 0xAC},           // AD_RAMP_AZ_R
    {0x381C, 0x00},           // D_RAMP_PULL_S
    {0x381D, 0x80},           //AD_RAMP_PULL_S
    {0x381E, 0x00},           //.RST_PULL_R
    {0x381F, 0xB0},           // RST_PULL_R
    {0x3003, 0x02},           // Serial:00h Parallel:02h
    {0x3004, 0x3F},           // Serial:00h Parallel:3Fh
    {0x3063, 0xC0},           // Serial:00h Parallel:C0h
    //
    {0x30B0, 0x40},            // SC_TH/SC_R_SEL
    {0x30B2, 0x98},            // SC_OVD_ROT/SC_P_SEL
    {0x30B5, 0x10},           // SC_CV00GR
    {0x30B6, 0x20},           // SC_CV40GR
    {0x30B7, 0x48},           // SC_CV80GR
    {0x30B8, 0x60},           // SC_CVC0GR
    {0x30B9, 0x6B},           // SC_OVDXPGR
    {0x30BA, 0x6E},           // SC_OVDXMGR
    {0x30BB, 0x6F},           // SC_OVDYPGR
    {0x30BC, 0x68},           // SC_OVDYMGR
    {0x30C0, 0x10},           // SC_CV00RR
    {0x30C1, 0x20},           // SC_CV40RR
    {0x30C2, 0x48},           // SC_CV80RR
    {0x30C3, 0x60},           // SC_CVC0RR
    {0x30C4, 0x79},           // SC_OVDXPRR
    {0x30C5, 0x70},           // SC_OVDXMRR
    {0x30C6, 0x67},           // SC_OVDYPRR
    {0x30C7, 0x72},           // SC_OVDYMRR
    {0x30CA, 0x30},           // SC_CV00BB
    {0x30CB, 0x10},           // SC_CV00BB
    {0x30CC, 0x20},           // SC_CV40BB
    {0x30CD, 0x48},           // SC_CV80BB
    {0x30CE, 0x60},           // SC_CVC0BB
    {0x30CF, 0x74},           // SC_OVDXPBB
    {0x30D0, 0x66},           // SC_OVDXMBB
    {0x30D1, 0x76},           // SC_OVDYPBB
    {0x30D2, 0x6A},           // SC_OVDYMBB
    {0x30D7, 0x30},           // SC_CV40GB
    {0x30D6, 0x10},           // SC_CV00GB
    {0x30D7, 0x20},           // SC_CV40GB
    {0x30D8, 0x48},           // SC_CV80GB
    {0x30D9, 0x60},           // SC_CVC0GB
    {0x30DA, 0x6B},           // SC_OVDXPGB
    {0x30DB, 0x6E},           // SC_OVDXMGB
    {0x30DC, 0x6F},           // SC_OVDYPGB
    {0x30DD, 0x68},           // SC_OVDYMGB
    {0x30E0, 0x30},           // SC_OVDYMGB
    {0x30ED, 0x05},           // SC_OVDYMGB
    {0x30EE, 0x2A},           // SC_OVDYMGB
    {0x30EF, 0x03},           // SC_OVDYMGB
    {0x30F0, 0xDC},           // SC_OVDYMGB
    {0x0100, 0x01},           //mode_select
    //{0x3f10, 0xff},
    //{0x3F11, 0x03},
    {0x3F12, 0x50},             //pclk edge
    {SEQUENCE_WAIT_MS, 100},
    {SEQUENCE_END, 0x0000}
};

static DevCtrlReg16 SetModeSequence_1316x988[] =
{
    {0x0103, 0x01}, //software_reset
    {SEQUENCE_WAIT_MS, 1000},
    {0x0111, 0x01}, //CCP_signalling_mode = strobe
    {0x0301, 0x0A}, //vt_pix_clk_div=10
    {0x0303, 0x01}, //vt_sys_clk_div=1
    {0x0305, 0x04}, //pre_pll_clk_div=4
    {0x0307, SENSOR_BAYER_DEFAULT_PLL_MULT}, //6B //50 //pll_multiplier=116
    {0x0309, 0x08}, //op_pix_clk_div=8
    {0x030B, 0x02}, //op_sys_clk_div=2

    //Fast SetMode sequence starts here
    {SEQUENCE_FAST_SETMODE_START, 0x00},
    {0x0340, 0x03}, //frame_length_lines =1009
    {0x0341, 0xE8}, //
    {0x0342, 0x09}, //line_length_pck =//2160
    {0x0343, 0x10},
    {0x0344, 0x00}, //x_addr_start
    {0x0345, 0x08},
    {0x0346, 0x00}, //y_addr_start
    {0x0347, 0x08},
    {0x0348, 0x0A}, //x_addr_end
    {0x0349, 0x53},
    {0x034A, 0x07}, //y_addr_end
    {0x034B, 0xC4},
    {0x034C, 0x05}, //0A //x_output_size =1316
    {0x034D, 0x26},
    {0x034E, 0x03}, //07 //y_output_size  =988
    {0x034F, 0xDC},
    {0x0381, 0x01}, //01
    {0x0383, 0x03}, //01  =1/2
    {0x0385, 0x01}, //01
    {0x0387, 0x03}, //01  =1/2
    {0x0202, 0x03}, // coarse integration time
    {0x0203, 0xE8}, // 0328=1/15s

    //Fast SetMode sequence starts here
    {SEQUENCE_FAST_SETMODE_END, 0x00},

    {0x0205, 0x20}, //analog gain (70 = x8, 60 = x4, 00 = x1)
    {0x3003, 0x02},
    {0x3004, 0x07},
    {0x3063, 0xC0},
    {0x3079, 0x08}, //
    {0x307A, 0xFC}, //
    {0x307B, 0x14}, //FF //
    {0x307C, 0x60},
    {0x307E, 0x01}, //
    {0x307F, 0x08}, // median filter OFF(0x38) ON(0x08)
    {0x3300, 0x02}, // TRNS_RD_CYC A6
    {0x3301, 0xAB}, // TRNS_RD_CYC A6
    {0x3308, 0x02}, // READOUT  A6
    {0x3309, 0xAC}, // READOUT  A6
    {0x3312, 0x00}, // RST_RD_S1
    {0x3313, 0x73}, // RST_RD_S1[7:0]
    {0x3318, 0x02}, // RST_RD_R2
    {0x3319, 0xAC}, // RST_RD_R2[7:0]
    {0x3326, 0x01}, // TXCLK_RD_S1
    {0x3327, 0x86}, // TXCLK_RD_S1
    {0x3328, 0x01}, // TXCLK_RD_R1
    {0x3329, 0xEB}, // TXCLK_RD_R1
    {0x3332, 0x00}, // VR28SET_RD_R
    {0x3333, 0x75}, // VR28SET_RD_R
    {0x3336, 0x02}, // ADREN_RD_R
    {0x3337, 0xAC}, // ADREN_RD_R
    {0x3338, 0x00}, // SDCEN_RD_S
    {0x3339, 0x75}, // XSDCEN_RD_S[7:0]
    {0x333A, 0x02}, // XSDCEN_RD_R
    {0x333B, 0xAC}, // XSDCEN_RD_R[7:0]
    {0x333C, 0x00}, // XBINT_RD_S[7:0]
    {0x333D, 0x78}, // XBINT_RD_S[7:0]
    {0x333E, 0x02}, // XBINT_RD_S
    {0x333F, 0xAC}, // XBINT_RD_R[7:0]
    {0x3346, 0x00}, //
    {0x3347, 0x00}, //
    {0x334A, 0x00}, //
    {0x334B, 0x00}, //
    {0x3364, 0x09}, //
    {0x336E, 0x07}, // Dummy OB Address
    {0x3338, 0x00}, // RTN_RD_S1
    {0x3389, 0x00}, // RTN_RD_S1[7:0]
    {0x338C, 0x00}, // RTN_RD_S2[9:8]
    {0x338D, 0x00}, // RTN_RD_S2[7:0]
    {0x342E, 0x04},
    {0x342F, 0x01}, //
    {0x343C, 0x01}, //
    {0x3500, 0x80}, // VTX 2.8V
    {0x3501, 0x00}, //
    {0x3502, 0x52}, // VRD
    {0x3504, 0x03}, //
    {0x3530, 0xAA}, // XBINT level
    {0x3531, 0x01}, // RST_PULL_CNT
    {0x3532, 0x40}, // RST_Gain_INT
    {0x3533, 0x62}, // PIXEL_LOAD_CONTROL
    {0x3534, 0x84}, //
    {0x3535, 0x1A}, // Pixel load current
    {0x3537, 0x03}, // AD FS voltage (0:650mV,1:735mV ...)
    {0x353A, 0x47}, //
    {0x353B, 0x20}, // XBINT_SELEN
    {0x353D, 0x02}, // VR Floating
    {0x3565, 0x01}, //
    {0x3716, 0x38}, // DC CLAMP CODE
    {0x371A, 0x03}, //
    {0x371B, 0x30}, //
    {0x371C, 0x30}, //
    {0x371D, 0x38}, // DC CLAMP CODE
    {0x371F, 0x50}, //
    {0x3721, 0x0F}, //// SUNSPOT_H2[9:8],SUNSPOT_H1[9:8],CAD_RESERVE1,VOB_ST
    {0x3722, 0xFF}, //// SUNSPOT_H1[7:0]
    {0x3723, 0xFF}, //// SUNSPOT_H2[7:0]
    {0x3724, 0x00}, //// SUNSPOT_LEV
    {0x3725, 0x00}, //// SUNSPOT_IRIS_LEV[15:8]
    {0x3726, 0x10}, //// SUNSPOT_IRIS_LEV[7:0]
    {0x3727, 0x02}, //
    {0x3728, 0x03}, //
    {0x3729, 0xFF}, //
    {0x372A, 0x50}, //// Pre_COMP_NUM_H
    {0x372B, 0x04}, //// Pre_COMP_NUM_L
    {0x372C, 0x20}, //// Corp_OFF_NUM
    {0x372D, 0x04}, //// Judge_Interval
    {0x3802, 0x01}, // AD_RS_R
    {0x3803, 0x6B}, // AD_RS_R   10Sep08 .KIM
    {0x3806, 0x01}, // AD_RSD_R
    {0x3807, 0xEB}, // AD_RSD_R  10Sep08 .KIM
    {0x3808, 0x00}, //
    {0x3809, 0x03}, //
    {0x380A, 0x01}, // AD_SSA_R
    {0x380B, 0x80}, // AD_SSA_R
    {0x380C, 0x01}, // AD_SSB_S
    {0x380D, 0xF0}, // AD_SSB_S
    {0x380E, 0x02}, // AD_SSB_R
    {0x380F, 0xAB}, // AD_SSB_R
    {0x3810, 0x02}, // AD_R1_S
    {0x3811, 0xAC}, // AD_R1_S
    {0x3812, 0x01}, //
    {0x3813, 0xEB}, //
    {0x381A, 0x02}, // AD_RAMP_AZ_R
    {0x381B, 0xAC}, // AD_RAMP_AZ_R
    {0x381C, 0x00}, // AD_RAMP_PULL_S
    {0x381D, 0x80}, // AD_RAMP_PULL_S
    {0x381E, 0x00}, // RST_PULL_R
    {0x381F, 0xB0}, // RST_PULL_R
    {0x3003, 0x02}, // Serial:00h Parallel:02h
    {0x3004, 0x3F}, // Serial:00h Parallel:3Fh
    {0x3063, 0xC0}, // Serial:00h Parallel:C0h
    {0x30B0, 0x40}, // SC_TH/SC_R_SEL
    {0x30B2, 0x98}, // SC_OVD_ROT/SC_P_SEL
    {0x30B5, 0x10}, // SC_CV00GR
    {0x30B6, 0x20}, // SC_CV40GR
    {0x30B7, 0x48}, // SC_CV80GR
    {0x30B8, 0x60}, // SC_CVC0GR
    {0x30B9, 0x6B}, //7B // SC_OVDXPGR Right
    {0x30BA, 0x6E}, //7E // SC_OVDXMGR Left
    {0x30BB, 0x6F}, //7F // SC_OVDYPGR
    {0x30BC, 0x68}, //78 // SC_OVDYMGR
    {0x30BF, 0x30}, //
    {0x30C0, 0x10}, // SC_CV00RR
    {0x30C1, 0x20}, // SC_CV40RR
    {0x30C2, 0x48}, // SC_CV80RR
    {0x30C3, 0x60}, // SC_CVC0RR
    {0x30C4, 0x79}, // SC_OVDXPRR Right
    {0x30C5, 0x70}, // SC_OVDXMRR Left
    {0x30C6, 0x67}, //68 //77 // SC_OVDYPRR Bottom
    {0x30C7, 0x72}, //70 // SC_OVDYMRR Top
    {0x30CA, 0x30}, //
    {0x30CB, 0x10}, // SC_CV00BB
    {0x30CC, 0x20}, // SC_CV40BB
    {0x30CD, 0x48}, // SC_CV80BB
    {0x30CE, 0x60}, //00 // SC_CVC0BB
    {0x30CF, 0x74}, //6B // SC_OVDXPBB Right
    {0x30D0, 0x66}, // SC_OVDXMBB Left
    {0x30D1, 0x76}, // SC_OVDYPBB Bottom
    {0x30D2, 0x6A}, //63 //73 // SC_OVDYMBB Top
    {0x30D7, 0x30}, //
    {0x30D6, 0x10}, // SC_CV00GB
    {0x30D7, 0x20}, // SC_CV40GB
    {0x30D8, 0x48}, // SC_CV80GB
    {0x30D9, 0x60}, // SC_CVC0GB
    {0x30DA, 0x6B}, //7B // SC_OVDXPGB Right
    {0x30DB, 0x6E}, //7E // SC_OVDXMGB Left
    {0x30DC, 0x6F}, //7F // SC_OVDYPGB
    {0x30DD, 0x68}, //78 // SC_OVDYMGB
    {0x30E0, 0x30}, //
    {0x30ED, 0x05}, //
    {0x30EE, 0x2A}, //
    {0x30EF, 0x03}, //
    {0x30F0, 0xDC}, //
    {0x0100, 0x01}, //mode_select
    {0x3f12, 0x50}, //0x47 //0x47 //pclk edge
    {SEQUENCE_WAIT_MS, 100},
    {SEQUENCE_END, 0x0000}
};


typedef struct ModeDependentSettingsRec
{
    NvU32 LineLength;
    NvU32 FrameLength;
    NvU32 MinFrameLength;
    NvU32 CoarseTime;
} ModeDependentSettings;

static ModeDependentSettings ModeDependentSettings_1316x988 =
{
    SENSOR_BAYER_DEFAULT_LINE_LENGTH_QUAR,
    SENSOR_BAYER_DEFAULT_FRAME_LENGTH_QUAR,
    SENSOR_BAYER_DEFAULT_FRAME_LENGTH_QUAR,
    SENSOR_BAYER_DEFAULT_EXPOSURE_COARSE_QUAR,
};

static ModeDependentSettings ModeDependentSettings_2632x1976 =
{
    SENSOR_BAYER_DEFAULT_LINE_LENGTH_FULL,
    SENSOR_BAYER_DEFAULT_FRAME_LENGTH_FULL,
    SENSOR_BAYER_DEFAULT_FRAME_LENGTH_FULL,
    SENSOR_BAYER_DEFAULT_EXPOSURE_COARSE_FULL,
};

/**
 * A list of sensor mode and its SetModeSequence. Phase 0. Sensor Dependent.
 * When SetMode is called, it searches the list for desired resolution and
 * frame rate and write the SetModeSequence through i2c.
 * For Phase 3, more resolutions will be added to this table, allowing
 * for peak performance during Preview, D1 and 720P encode.
 */
static SensorSetModeSequence g_SensorBayerSetModeSequenceList[] =
{
     // WxH         x,y    fps    set mode sequence
    {{{1316, 988}, {2, 2}, 1.5,}, SetModeSequence_1316x988,
     &ModeDependentSettings_1316x988},
    {{{2632, 1976}, {2, 2}, 1.5,}, SetModeSequence_2632x1976,
     &ModeDependentSettings_2632x1976},
};


/**
 * ----------------------------------------------------------
 *  Start of Phase 1, Phase 2, and Phase 3 code
 * ----------------------------------------------------------
 */

/**
 * Sensor bayer's private context. Phase 1.
 */
typedef struct SensorBayerContextRec
{
    // Phase 1 variables.
    NvOdmImagerI2cConnection I2c;
    NvU32 ModeIndex;
    NvU32 NumModes;
    NvOdmImagerPowerLevel PowerLevel;
    ImagerGpioConfig GpioConfig;


    // Phase 2 variables.
    NvBool SensorInitialized;

    NvU32 SensorInputClockkHz; // mclk (extclk)
    NvF32 SensorClockRatio; // pclk/mclk

    NvF32 Exposure;
    NvF32 MaxExposure;
    NvF32 MinExposure;

    NvF32 Gains[4];
    NvF32 MaxGain;
    NvF32 MinGain;

    NvF32 FrameRate;
    NvF32 RequestedMaxFrameRate; // requested limit
    NvF32 MinFrameRate; // sensor's limit
    NvF32 MaxFrameRate; // sensor's limit

    // Phase 2 variables. Sensor Dependent.
    // These are used to set or get exposure, frame rate, and so on.
    NvU32 PllMult;
    NvU32 PllPreDiv;
    NvU32 PllVtPixDiv;
    NvU32 PllVtSysDiv;

    NvU32 CoarseTime;
    NvU32 VtPixClkFreqHz;

    NvU32 LineLength;
    NvU32 FrameLength;

    NvU32 MaxFrameLength;
    NvU32 MinFrameLength;

    // Phase 3.
    NvBool FastSetMode;


}SensorBayerContext;


/**
 * Static functions
 * Some compilers require these to be declared upfront.
 */
static NvBool SensorBayer_Open(NvOdmImagerHandle hImager);

static void SensorBayer_Close(NvOdmImagerHandle hImager);

static void
SensorBayer_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities);

static void
SensorBayer_ListModes(
    NvOdmImagerHandle hImager,
    NvOdmImagerSensorMode *pModes,
    NvS32 *pNumberOfModes);

static NvBool
SensorBayer_SetMode(
    NvOdmImagerHandle hImager,
    const SetModeParameters *pParameters,
    NvOdmImagerSensorMode *pSelectedMode,
    SetModeParameters *pResult);

static NvBool
SensorBayer_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel);

static NvBool
SensorBayer_SetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    const void *pValue);

static NvBool
SensorBayer_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue);

static void
SensorBayer_GetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel *pPowerLevel);

static NvBool
SensorBayer_WriteFramerate(
        SensorBayerContext *pContext,
        NvF32 *pFrameRate,
        NvBool WriteNewFrameRate);

/**
 * SensorBayer_WriteExposure. Phase 2. Sensor Dependent.
 * Calculate and write the values of the sensor registers for the new
 * exposure time.
 * Without this, calibration will not be able to capture images
 * at various brightness levels, and the final product won't be able
 * to compensate for bright or dark scenes.
 */
static NvBool
SensorBayer_WriteExposure(
    SensorBayerContext *pContext,
    const NvF32 *pNewExposureTime)
{
    NvF32 Freq = (NvF32)pContext->VtPixClkFreqHz;
    NvF32 LineLength = (NvF32)pContext->LineLength;
    NvF32 ExpTime = *pNewExposureTime;
    NvU32 NewCoarseTime = 0;
    NvU32 NewFrameLength = 0;


    if (*pNewExposureTime > pContext->MaxExposure ||
        *pNewExposureTime < pContext->MinExposure)
        return NV_FALSE;

    // Here, we have to decide if the new exposure time is valid
    // based on the sensor and current sensor settings.
    NewCoarseTime = (NvU32)((Freq * ExpTime) / LineLength);
    NewFrameLength = NewCoarseTime + 1;

    // Consider requested max frame rate,
    if (pContext->RequestedMaxFrameRate > 0.0f)
    {
        NvU32 RequestedMinFrameLengthLines = 0;
        RequestedMinFrameLengthLines =
           (NvU32)(Freq / (LineLength * pContext->RequestedMaxFrameRate));

        if (NewFrameLength < RequestedMinFrameLengthLines)
            NewFrameLength = RequestedMinFrameLengthLines;
    }

    // Clamp to sensor's limit
    if (NewFrameLength > pContext->MaxFrameLength)
        NewFrameLength = pContext->MaxFrameLength;
    else if (NewFrameLength < pContext->MinFrameLength)
        NewFrameLength = pContext->MinFrameLength;

    // Need to update FrameLength?
    if (NewFrameLength != pContext->FrameLength)
    {
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,
                0x0340, (NewFrameLength >> 8) & 0xFF);
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,
                0x0341, NewFrameLength & 0xFF);
        pContext->FrameLength = NewFrameLength;

        // Update variables depending on FrameLength.
        pContext->FrameRate = (NvF32)pContext->VtPixClkFreqHz /
                              (NvF32)(pContext->FrameLength *
                                      pContext->LineLength);
    }

    // FrameLength should have been updated but we have to check again
    // in case FrameLength gets clamped.
    if (NewCoarseTime >= pContext->FrameLength)
    {
        NewCoarseTime = pContext->FrameLength - 1;
    }

    // Current CoarseTime needs to be updated?
    if (pContext->CoarseTime != NewCoarseTime)
    {
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,
                0x0202, (NewCoarseTime >> 8) & 0xFF);
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,
                0x0203, NewCoarseTime & 0xFF);

        // Calculate the new exposure based on the sensor and sensor
        // settings.
        pContext->Exposure = ((NvF32)NewCoarseTime * LineLength +
                              SENSOR_BAYER_DEFAULT_EXPOSURE_FINE) /
                             Freq;
        pContext->CoarseTime = NewCoarseTime;
    }

    return NV_TRUE;
}

/**
 * SensorBayer_SetInputClock. Phase 2. Sensor Dependent.
 * Setting the input clock and updating the variables based on the input clock
 * The frame rate and exposure calculations are directly related to the
 * clock speed, so this is how the camera driver (the controller of the clocks)
 * can inform the nvodm driver of the current clock speed.
 */
static NvBool
SensorBayer_SetInputClock(
    SensorBayerContext *pContext,
    const NvOdmImagerClockProfile *pClockProfile)
{

    if (pClockProfile->ExternalClockKHz <
        g_SensorBayerCaps.InitialSensorClockRateKHz)
        return NV_FALSE;

    pContext->SensorInputClockkHz = pClockProfile->ExternalClockKHz;
    pContext->SensorClockRatio = pClockProfile->ClockMultiplier;

    // if sensor is initialized (SetMode has been called), then we need to
    // update necessary sensor variables (based on this particular sensor)
    if (pContext->SensorInitialized)
    {
        pContext->VtPixClkFreqHz =
            (NvU32)((pContext->SensorInputClockkHz * 1000 * pContext->PllMult) /
                    (pContext->PllPreDiv * pContext->PllVtPixDiv *
                     pContext->PllVtSysDiv));

        pContext->Exposure = (NvF32)(pContext->CoarseTime *
                                     pContext->LineLength +
                                     SENSOR_BAYER_DEFAULT_EXPOSURE_FINE) /
                             (NvF32)pContext->VtPixClkFreqHz;
        pContext->MaxExposure = (NvF32)((SENSOR_BAYER_DEFAULT_MAX_FRAME_LENGTH - 1) *
                                        pContext->LineLength +
                                        SENSOR_BAYER_DEFAULT_EXPOSURE_FINE) /
                                (NvF32)pContext->VtPixClkFreqHz;
        pContext->MinExposure = (NvF32)SENSOR_BAYER_DEFAULT_EXPOSURE_FINE /
                                (NvF32)pContext->VtPixClkFreqHz;

    }

    return NV_TRUE;
}


/**
 * SensorBayer_WriteGains. Phase 2. Sensor Dependent.
 * Writing the sensor registers for the new gains.
 * Just like exposure, this allows the sensor to brighten an image. If the
 * exposure time is insufficient to make the picture viewable, gains are
 * applied.  During calibration, the gains will be measured for maximum
 * effectiveness before the noise level becomes too high.
 * If per-channel gains are available, they are used to normalize the color.
 * Most sensors output overly greenish images, so the red and blue channels
 * are increased.
 */
static NvBool
SensorBayer_WriteGains(
    SensorBayerContext *pContext,
    const NvF32 *pGains)
{
    NvU32 i = 0;
    NvU16 NewGains[4] = {0};

    for(i = 0; i < 4; i++)
    {
        if (pGains[i] > pContext->MaxGain)
            return NV_FALSE;

        // convert float to gain's unit
        NewGains[i] = (NvU16)SENSOR_F32_TO_GAIN(pGains[i]);
    }

    NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x0104, 1);
    NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x0205, NewGains[1]);
    NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x3000,2);
    NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x0104,0);

    NvOdmOsMemcpy(pContext->Gains, pGains, sizeof(NvF32)*4);

    return NV_TRUE;
}


/**
 * SensorBayer_Open. Phase 1.
 * Initialize sensor bayer and its private context
 */
static NvBool SensorBayer_Open(NvOdmImagerHandle hImager)
{
    SensorBayerContext *pSensorBayerContext = NULL;
    NvOdmImagerI2cConnection *pI2c = NULL;

    if (!hImager || !hImager->pSensor)
        return NV_FALSE;

    pSensorBayerContext =
        (SensorBayerContext*)NvOdmOsAlloc(sizeof(SensorBayerContext));
    if (!pSensorBayerContext)
        goto fail;

    NvOdmOsMemset(pSensorBayerContext, 0, sizeof(SensorBayerContext));

    // Setup the i2c bit widths so we can call the
    // generalized write/read utility functions.
    // This is done here, since it will vary from sensor to sensor.
    pI2c = &pSensorBayerContext->I2c;
    pI2c->AddressBitWidth = 16;
    pI2c->DataBitWidth = 16;

    pSensorBayerContext->NumModes =
        NV_ARRAY_SIZE(g_SensorBayerSetModeSequenceList);
    pSensorBayerContext->ModeIndex =
        pSensorBayerContext->NumModes + 1; // invalid mode

    /**
     * Phase 2. Initialize exposure and gains.
     */
    pSensorBayerContext->Exposure = -1.0; // invalid exposure

     // convert gain's unit to float
    pSensorBayerContext->MaxGain = SENSOR_GAIN_TO_F32(SENSOR_BAYER_DEFAULT_MAX_GAIN);
    pSensorBayerContext->MinGain = SENSOR_GAIN_TO_F32(SENSOR_BAYER_DEFAULT_MIN_GAIN);

    /**
     * Phase 2 ends here.
     */

    hImager->pSensor->pPrivateContext = pSensorBayerContext;

    return NV_TRUE;

fail:
    NvOdmOsFree(pSensorBayerContext);
    return NV_FALSE;
}


/**
 * SensorBayer_Close. Phase 1.
 * Free the sensor's private context
 */
static void SensorBayer_Close(NvOdmImagerHandle hImager)
{
    SensorBayerContext *pContext = NULL;

    if (!hImager || !hImager->pSensor || !hImager->pSensor->pPrivateContext)
        return;

    pContext = (SensorBayerContext*)hImager->pSensor->pPrivateContext;

    // cleanup
    NvOdmOsFree(pContext);
    hImager->pSensor->pPrivateContext = NULL;
}

/**
 * SensorBayer_GetCapabilities. Phase 1.
 * Returnt sensor bayer's capabilities
 */
static void
SensorBayer_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities)
{
    // Copy the sensor caps from g_SensorBayerCaps
    NvOdmOsMemcpy(pCapabilities, &g_SensorBayerCaps,
        sizeof(NvOdmImagerCapabilities));
}

/**
 * SensorBayer_ListModes. Phase 1.
 * Return a list of modes that sensor bayer supports.
 * If called with a NULL ptr to pModes, then it just returns the count
 */
static void
SensorBayer_ListModes(
    NvOdmImagerHandle hImager,
    NvOdmImagerSensorMode *pModes,
    NvS32 *pNumberOfModes)
{
    SensorBayerContext *pContext =
        (SensorBayerContext*)hImager->pSensor->pPrivateContext;
    NvS32 i;

    if (pNumberOfModes)
    {
        *pNumberOfModes = (NvS32)pContext->NumModes;
        if (pModes)
        {
            // Copy the modes from g_SensorBayerSetModeSequenceList
            for (i = 0; i < *pNumberOfModes; i++)
                pModes[i] = g_SensorBayerSetModeSequenceList[i].Mode;
        }
    }
}

static NV_INLINE NvBool SensorBayerChooseModeIndex(
    SensorBayerContext *pContext,
    NvSize Resolution,
    NvU32 *pIndex)
{
    NvU32 Index;

    for (Index = 0; Index < pContext->NumModes; Index++)
    {
        if ((Resolution.width ==
             g_SensorBayerSetModeSequenceList[Index].Mode.
             ActiveDimensions.width) &&
            (Resolution.height ==
             g_SensorBayerSetModeSequenceList[Index].Mode.
             ActiveDimensions.height))
        {
            *pIndex = Index;
            return NV_TRUE;
        }
    }

    return NV_FALSE;
}

/**
 * SensorBayer_SetMode. Phase 1.
 * Set sensor bayer to the mode of the desired resolution and frame rate.
 */
static NvBool
SensorBayer_SetMode(
    NvOdmImagerHandle hImager,
    const SetModeParameters *pParameters,
    NvOdmImagerSensorMode *pSelectedMode,
    SetModeParameters *pResult)
{
    NvBool Status = NV_FALSE;
    SensorBayerContext *pContext =
        (SensorBayerContext*)hImager->pSensor->pPrivateContext;
    NvU32 Index;
    ModeDependentSettings *pModeSettings;

    Status = SensorBayerChooseModeIndex(pContext, pParameters->Resolution,
                                        &Index);

    // No match found
    if (!Status)
        return NV_FALSE;

    if (pSelectedMode)
    {
        *pSelectedMode = g_SensorBayerSetModeSequenceList[Index].Mode;
    }

    if (pContext->ModeIndex == Index)
        return NV_TRUE;

    // TODO: Do we need to set a lower coarse time for new framelenghtline?
    // I don't think so because the set mode sequence reset the sensor.

    // i2c writes for the set mode sequence
    Status = WriteI2CSequence(&pContext->I2c,
                 g_SensorBayerSetModeSequenceList[Index].pSequence,
                 pContext->SensorInitialized && pContext->FastSetMode);
    if (!Status)
        return NV_FALSE;
    /**
     * the following is Phase 2.
     */
    pContext->SensorInitialized = NV_TRUE;

    // Update sensor context after set mode
    NV_ASSERT(pContext->SensorInputClockkHz > 0);

    // These hardcoded numbers are from the set mode sequence and this formula
    // is based on this sensor. Sensor Dependent.
    pContext->PllMult = SENSOR_BAYER_DEFAULT_PLL_MULT;
    pContext->PllPreDiv = SENSOR_BAYER_DEFAULT_PLL_PRE_DIV;
    pContext->PllVtPixDiv = SENSOR_BAYER_DEFAULT_PLL_VT_PIX_DIV;
    pContext->PllVtSysDiv = SENSOR_BAYER_DEFAULT_PLL_VT_SYS_DIV;
    pContext->VtPixClkFreqHz =
        (NvU32)((pContext->SensorInputClockkHz * 1000 * pContext->PllMult) /
                (pContext->PllPreDiv * pContext->PllVtPixDiv *
                 pContext->PllVtSysDiv));

    pModeSettings =
        (ModeDependentSettings*)g_SensorBayerSetModeSequenceList[Index].
                                pModeDependentSettings;

    pContext->LineLength = pModeSettings->LineLength;
    pContext->FrameLength = pModeSettings->FrameLength;
    pContext->CoarseTime = pModeSettings->CoarseTime;

    pContext->Exposure = (NvF32)(pContext->CoarseTime * pContext->LineLength +
                                 SENSOR_BAYER_DEFAULT_EXPOSURE_FINE) /
                         (NvF32)pContext->VtPixClkFreqHz;
    pContext->MaxExposure = (NvF32)((SENSOR_BAYER_DEFAULT_MAX_FRAME_LENGTH - 1) *
                                    pContext->LineLength +
                                    SENSOR_BAYER_DEFAULT_EXPOSURE_FINE) /
                            (NvF32)pContext->VtPixClkFreqHz;
    pContext->MinExposure = (NvF32)SENSOR_BAYER_DEFAULT_EXPOSURE_FINE /
                            (NvF32)pContext->VtPixClkFreqHz;

    pContext->MaxFrameLength = SENSOR_BAYER_DEFAULT_MAX_FRAME_LENGTH;
    pContext->MinFrameLength = pModeSettings->MinFrameLength;

    pContext->FrameRate = (NvF32)pContext->VtPixClkFreqHz /
                          (NvF32)(pContext->FrameLength *
                                  pContext->LineLength);
    pContext->MaxFrameRate = pContext->FrameRate;
    pContext->MinFrameRate = (NvF32)pContext->VtPixClkFreqHz /
                             (NvF32)(SENSOR_BAYER_DEFAULT_MAX_FRAME_LENGTH *
                                     pContext->LineLength);

    pContext->Gains[0] = SENSOR_BAYER_DEFAULT_GAIN_R;
    pContext->Gains[1] = SENSOR_BAYER_DEFAULT_GAIN_GR;
    pContext->Gains[2] = SENSOR_BAYER_DEFAULT_GAIN_GB;
    pContext->Gains[3] = SENSOR_BAYER_DEFAULT_GAIN_B;



    SensorBayer_WriteExposure(pContext, &pParameters->Exposure);
    SensorBayer_WriteGains(pContext, pParameters->Gains);

    // Do we need to slow down the frame rate?
    if (pContext->RequestedMaxFrameRate > 0.0f)
    {
        NvU32 RequestedMinFrameLengthLines =
            (NvU32)(pContext->VtPixClkFreqHz / (pContext->LineLength *
                    pContext->RequestedMaxFrameRate));

        if (RequestedMinFrameLengthLines > pContext->MaxFrameLength)
            RequestedMinFrameLengthLines = pContext->MaxFrameLength;

        if (pContext->FrameLength < RequestedMinFrameLengthLines)
        {
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,
                    0x0340, (RequestedMinFrameLengthLines >> 8) & 0xFF);
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,
                    0x0341, RequestedMinFrameLengthLines & 0xFF);

            pContext->FrameLength = RequestedMinFrameLengthLines;
        }
    }


    if (pResult)
    {
        pResult->Resolution = g_SensorBayerSetModeSequenceList[Index].
                              Mode.ActiveDimensions;
        pResult->Exposure = pContext->Exposure;
        NvOdmOsMemcpy(pResult->Gains, &pContext->Gains, sizeof(NvF32) * 4);
    }

    /**
     * Phase 2 ends here.
     */

    pContext->ModeIndex = Index;

    //Wait 2 frames for gains/exposure to take effect.
    NvOsSleepMS((NvU32)(2000.0 / (NvF32)(pContext->FrameRate)));

    return Status;
}


/**
 * SensorBayer_SetPowerLevel. Phase 1
 * Set the sensor's power level
 */
static NvBool
SensorBayer_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel)
{
    NvBool Status = NV_TRUE;
    SensorBayerContext *pContext =
        (SensorBayerContext*)hImager->pSensor->pPrivateContext;
    const NvOdmPeripheralConnectivity *pConnections;

    if (pContext->PowerLevel == PowerLevel)
        return NV_TRUE;

    NV_ASSERT(hImager->pSensor->pConnections);
    pConnections = hImager->pSensor->pConnections;

    switch(PowerLevel)
    {
        case NvOdmImagerPowerLevel_On:
            Status = SetPowerLevelWithPeripheralConnectivity(pConnections,
                         &pContext->I2c, &pContext->GpioConfig,
                         NvOdmImagerPowerLevel_On);
            if (!Status) {
                return NV_FALSE;
            }

            break;

        case NvOdmImagerPowerLevel_Standby:

            Status = SetPowerLevelWithPeripheralConnectivity(pConnections,
                         &pContext->I2c, &pContext->GpioConfig,
                         NvOdmImagerPowerLevel_Standby);

            break;
        case NvOdmImagerPowerLevel_Off:

             // Set the sensor to standby mode
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x0100, 0);

            Status = SetPowerLevelWithPeripheralConnectivity(pConnections,
                         &pContext->I2c, &pContext->GpioConfig,
                         NvOdmImagerPowerLevel_Off);

            // wait for the standby 0.1 sec should be enough
            NvOdmOsWaitUS(100000);

            break;
        default:
            NV_ASSERT("!Unknown power level\n");
            Status = NV_FALSE;
            break;
    }

    if (Status)
        pContext->PowerLevel = PowerLevel;

    return Status;
}

/**
 * SensorBayer_SetParameter. Phase 2.
 * Set sensor specific parameters.
 * For Phase 1. This can return NV_TRUE.
 */
static NvBool
SensorBayer_SetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    const void *pValue)
{
    NvBool Status = NV_TRUE;
    SensorBayerContext *pContext =
        (SensorBayerContext*)hImager->pSensor->pPrivateContext;

    switch(Param)
    {
        // Phase 2.
        case NvOdmImagerParameter_SensorExposure:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvF32));

            Status = SensorBayer_WriteExposure(pContext, (NvF32*)pValue);
            break;

        // Phase 2.
        case NvOdmImagerParameter_SensorGain:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, 4 * sizeof(NvF32));

            Status = SensorBayer_WriteGains(pContext, pValue);
            break;

        // Phase 2.
        case NvOdmImagerParameter_SensorInputClock:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                                             sizeof(NvOdmImagerClockProfile));

            Status = SensorBayer_SetInputClock(
                      pContext,
                      ((NvOdmImagerClockProfile*)pValue));
            break;

        // Phase 3.
        case NvOdmImagerParameter_OptimizeResolutionChange:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvBool));

            pContext->FastSetMode = *((NvBool*)pValue);
            break;

        // This is optional but nice to have.
        case NvOdmImagerParameter_SelfTest:
            //TODO: Not Implemented.
            break;
        // Phase 3.
        case NvOdmImagerParameter_Reset:
            return NV_FALSE;
            break;

        case NvOdmImagerParameter_MaxSensorFrameRate:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvF32));

            pContext->RequestedMaxFrameRate = *((NvF32*)pValue);
            break;

        default:
            NV_ASSERT(!"Not Implemented");
            break;
    }

    return NV_TRUE;
}

/**
 * SensorBayer_GetParameter. Phase 1.
 * Get sensor specific parameters
 */
static NvBool
SensorBayer_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue)
{
    NvBool Status = NV_TRUE;
    NvF32 *pFloatValue = pValue;
    SensorBayerContext *pContext =
        (SensorBayerContext*)hImager->pSensor->pPrivateContext;
    NvU32 *pUIntValue = pValue;

    switch(Param)
    {
        // Phase 1.
        case NvOdmImagerParameter_CalibrationData:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerCalibrationData));
            {
                NvOdmImagerCalibrationData *pCalibration =
                        (NvOdmImagerCalibrationData*)pValue;
                pCalibration->CalibrationData = sensorCalibrationData;
                pCalibration->NeedsFreeing = NV_FALSE;
            }
            break;

        // Phase 1, it can return min = max = 1.0f
        // Phase 2, return the real numbers.
        case NvOdmImagerParameter_SensorExposureLimits:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, 2 * sizeof(NvF32));

            pFloatValue[0] = pContext->MinExposure;
            pFloatValue[1] = pContext->MaxExposure;
            break;

        // Phase 1, it can return min = max = 1.0f
        // Phase 2, return the real numbers.
        case NvOdmImagerParameter_SensorGainLimits:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, 2 * sizeof(NvF32));
            pFloatValue[0] = pContext->MinGain;
            pFloatValue[1] = pContext->MaxGain;

            break;

        // Get the sensor status. This is optional but nice to have.
        case NvOdmImagerParameter_DeviceStatus:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerDeviceStatus));
            {
                NvOdmImagerDeviceStatus *pStatus;
                pStatus = (NvOdmImagerDeviceStatus *)pValue;
                {
                    NvOdmImagerI2cConnection *pI2c = &pContext->I2c;
                    // Pick your own useful registers to use for debug
                    // If the camera hangs, these register values are printed
                    // Sensor Dependent.
                    NvOdmImagerI2cRead(pI2c, 0x0000, &pStatus->Values[0]);
                    NvOdmImagerI2cRead(pI2c, 0x303B, &pStatus->Values[1]);
                    NvOdmImagerI2cRead(pI2c, 0x303C, &pStatus->Values[2]);
                    NvOdmImagerI2cRead(pI2c, 0x306A, &pStatus->Values[3]);
                    pStatus->Count = 4;
                }
            }
            break;
        case NvOdmImagerParameter_SensorFrameRate:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvF32));
            {
                pFloatValue[0] = 1.1; // TODO update with a calculated rate
            }
            Status = NV_TRUE;
            break;

        case NvOdmImagerParameter_SensorFrameRateLimits:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                2 * sizeof(NvF32));
            pFloatValue[0] = pContext->MinFrameRate;
            pFloatValue[1] = pContext->MaxFrameRate;
            break;

        case NvOdmImagerParameter_RegionUsedByCurrentResolution:
            if (SizeOfValue != sizeof(NvOdmImagerRegion)) {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            if (pContext->ModeIndex == (pContext->NumModes+1)) { // Invalid mode
                // No info available until we've actually set a mode.
                return NV_FALSE;
            }
            {
                NvOdmImagerRegion *pRegion = (NvOdmImagerRegion *)pValue;
                pRegion->RegionStart.x = 0;
                pRegion->RegionStart.y = 0;
                if (pContext->ModeIndex == 0) { // Quarter res
                    pRegion->xScale = 2;
                    pRegion->yScale = 2;
                } else { // Should be Full res.
                    pRegion->xScale = 1;
                    pRegion->yScale = 1;
                }
                return NV_TRUE;
            }
            break;

        case NvOdmImagerParameter_SensorFrameRateLimitsAtResolution:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerFrameRateLimitAtResolution));
            {
                NvOdmImagerFrameRateLimitAtResolution *pData;
                NvU32 Index;
                NvBool MatchFound = NV_FALSE;
                ModeDependentSettings *pModeSettings = NULL;

                pData = (NvOdmImagerFrameRateLimitAtResolution *)pValue;
                MatchFound =
                    SensorBayerChooseModeIndex(pContext, pData->Resolution,
                    &Index);

                if (!MatchFound)
                {
                    pData->MinFrameRate = 0.0f;
                    pData->MaxFrameRate = 0.0f;
                    break;
                }

                pModeSettings = (ModeDependentSettings*)
                    g_SensorBayerSetModeSequenceList[Index].
                    pModeDependentSettings;

                pData->MaxFrameRate = (NvF32)pContext->VtPixClkFreqHz /
                                      (NvF32)(pModeSettings->FrameLength *
                                              pModeSettings->LineLength);
                pData->MinFrameRate =
                    (NvF32)pContext->VtPixClkFreqHz /
                    (NvF32)(SENSOR_BAYER_DEFAULT_MAX_FRAME_LENGTH *
                            pModeSettings->LineLength);

            }
            break;

        // Phase 3.
        // Get the override config data.
        case NvOdmImagerParameter_CalibrationOverrides:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerCalibrationData));
            {
                NvOdmImagerCalibrationData *pCalibration =
                    (NvOdmImagerCalibrationData*)pValue;

                pCalibration->CalibrationData =
                    LoadOverridesFile(pOverrideFiles,
                        (sizeof(pOverrideFiles)/sizeof(pOverrideFiles[0])));
                pCalibration->NeedsFreeing = (pCalibration->CalibrationData != NULL);
                Status = pCalibration->NeedsFreeing;
            }
            break;

        case NvOdmImagerParameter_SensorExposureLatchTime:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvU32));
                *pUIntValue = 1;
            break;

        // Phase 3.
        default:
            NV_ASSERT("!Not Implemented");
            Status = NV_FALSE;
    }

    return Status;
}

/**
 * SensorBayer_GetPowerLevel. Phase 3.
 * Get the sensor's current power level
 */
static void
SensorBayer_GetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel *pPowerLevel)
{
    SensorBayerContext *pContext =
        (SensorBayerContext*)hImager->pSensor->pPrivateContext;

    *pPowerLevel = pContext->PowerLevel;
}

/**
 * SensorBayer_GetHal. Phase 1.
 * return the hal functions associated with sensor bayer
 */
NvBool SensorBayer_GetHal(NvOdmImagerHandle hImager)
{
    if (!hImager || !hImager->pSensor)
        return NV_FALSE;

    hImager->pSensor->pfnOpen = SensorBayer_Open;
    hImager->pSensor->pfnClose = SensorBayer_Close;
    hImager->pSensor->pfnGetCapabilities = SensorBayer_GetCapabilities;
    hImager->pSensor->pfnListModes = SensorBayer_ListModes;
    hImager->pSensor->pfnSetMode = SensorBayer_SetMode;
    hImager->pSensor->pfnSetPowerLevel = SensorBayer_SetPowerLevel;
    hImager->pSensor->pfnGetPowerLevel = SensorBayer_GetPowerLevel;
    hImager->pSensor->pfnSetParameter = SensorBayer_SetParameter;
    hImager->pSensor->pfnGetParameter = SensorBayer_GetParameter;

    return NV_TRUE;
}




