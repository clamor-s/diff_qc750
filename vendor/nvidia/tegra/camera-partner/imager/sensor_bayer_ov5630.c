/*
 * Copyright (c) 2008-2012, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_imager.h"
#include "sensor_bayer_ov5630.h"
#include "imager_util.h"
#include "nvodm_query_gpio.h"
#include "nvos.h"

#include "camera_omnivision_5630_calibration.h"

#define OVERRIDES_FILENAME_LEN 128
#define OV5630_FULL_RES_NUM_MORE_LINES  (4)
#define OV5630_QUART_RES_NUM_MORE_LINES (4)
#define OV5630_720P_RES_NUM_MORE_LINES  (4)

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
#define SENSOR_OV5630_DEFAULT_PLL_MULT              (12)
#define SENSOR_OV5630_DEFAULT_PLL_PRE_DIV           (1.5)
#define SENSOR_OV5630_DEFAULT_PLL_VT_PIX_DIV        (2)
#define SENSOR_OV5630_DEFAULT_PLL_VT_SYS_DIV        (1)

#define SENSOR_OV5630_DEFAULT_EXPOSURE_FINE         (0x00CA)
#define SENSOR_OV5630_DEFAULT_MAX_GAIN              (0x003F)
#define SENSOR_OV5630_DEFAULT_MIN_GAIN              (0x0000)
#define SENSOR_OV5630_DEFAULT_GAIN_R                (SENSOR_OV5630_DEFAULT_MIN_GAIN)
#define SENSOR_OV5630_DEFAULT_GAIN_GR               (SENSOR_OV5630_DEFAULT_MIN_GAIN)
#define SENSOR_OV5630_DEFAULT_GAIN_GB               (SENSOR_OV5630_DEFAULT_MIN_GAIN)
#define SENSOR_OV5630_DEFAULT_GAIN_B                (SENSOR_OV5630_DEFAULT_MIN_GAIN)
#define SENSOR_OV5630_DEFAULT_MAX_FRAME_LENGTH      (0xFFFF)

#define SENSOR_OV5630_DEFAULT_LINE_LENGTH_FULL      (0x0CA0)

#define SENSOR_OV5630_DEFAULT_FRAME_LENGTH_FULL     (0x7BC + OV5630_FULL_RES_NUM_MORE_LINES) // 2592x1948 sensor output size

#define SENSOR_OV5630_DEFAULT_EXPOSURE_COARSE_FULL  (0x03DE)
#define SENSOR_OV5630_DEFAULT_MIN_FRAME_LENGTH_FULL (SENSOR_OV5630_DEFAULT_FRAME_LENGTH_FULL)

#define SENSOR_OV5630_DEFAULT_LINE_LENGTH_QUAR      (0x0790)

#define SENSOR_OV5630_DEFAULT_FRAME_LENGTH_QUAR     (0x03F0 + OV5630_QUART_RES_NUM_MORE_LINES) // 1296x976 sensor output size

#define SENSOR_OV5630_DEFAULT_EXPOSURE_COARSE_QUAR  (0x03EC)
#define SENSOR_OV5630_DEFAULT_MIN_FRAME_LENGTH_QUAR (SENSOR_OV5630_DEFAULT_FRAME_LENGTH_QUAR)

#define SENSOR_OV5630_DEFAULT_FRAME_LENGTH_720P      (0x02F4 + OV5630_720P_RES_NUM_MORE_LINES) // 1280x720 sensor output size
#define SENSOR_OV5630_DEFAULT_LINE_LENGTH_720P       (0x0780)
#define SENSOR_OV5630_DEFAULT_MIN_FRAME_LENGTH_720P  (SENSOR_OV5630_DEFAULT_FRAME_LENGTH_720P)

#define SENSOR_OV5630_DEFAULT_FRAME_LENGTH_1080P      (0x045C + OV5630_720P_RES_NUM_MORE_LINES) // 1920x1080 sensor output size
#define SENSOR_OV5630_DEFAULT_LINE_LENGTH_1080P       (0x0af0)
#define SENSOR_OV5630_DEFAULT_MIN_FRAME_LENGTH_1080P  (SENSOR_OV5630_DEFAULT_FRAME_LENGTH_1080P)



/**
 * sensor register address
 */
#define SENSOR_OV5630_REGISTER_FRAME_LENGTH_LINES_HIGH  (0x3020)
#define SENSOR_OV5630_REGISTER_FRAME_LENGTH_LINES_LOW   (0x3021)
#define SENSOR_OV5630_REGISTER_LINE_LENGTH_PCK_HIGH     (0x3022)
#define SENSOR_OV5630_REGISTER_LINE_LENGTH_PCK_LOW      (0x3023)
#define SENSOR_OV5630_REGISTER_X_ADDR_START_HIGH        (0x3024)
#define SENSOR_OV5630_REGISTER_X_ADDR_START_LOW         (0x3025)
#define SENSOR_OV5630_REGISTER_Y_ADDR_START_HIGH        (0x3026)
#define SENSOR_OV5630_REGISTER_Y_ADDR_START_LOW         (0x3027)
#define SENSOR_OV5630_REGISTER_X_ADDR_END_HIGH          (0x3028)
#define SENSOR_OV5630_REGISTER_X_ADDR_END_LOW           (0x3029)
#define SENSOR_OV5630_REGISTER_Y_ADDR_END_HIGH          (0x302A)
#define SENSOR_OV5630_REGISTER_Y_ADDR_END_LOW           (0x302B)
#define SENSOR_OV5630_REGISTER_X_OUTPUT_SIZE_HIGH       (0x302C)
#define SENSOR_OV5630_REGISTER_X_OUTPUT_SIZE_LOW        (0x302D)
#define SENSOR_OV5630_REGISTER_Y_OUTPUT_SIZE_HIGH       (0x302E)
#define SENSOR_OV5630_REGISTER_Y_OUTPUT_SIZE_LOW        (0x302F)

#define SENSOR_OV5630_REGISTER_ANALOG_GAIN              (0x3000)
#define SENSOR_OV5630_REGISTER_EXPOSURE_COARSE_HIGH     (0x3002)
#define SENSOR_OV5630_REGISTER_EXPOSURE_COARSE_LOW      (0x3003)
#define SENSOR_OV5630_REGISTER_EXPOSURE_FINE_HIGH       (0x3004)
#define SENSOR_OV5630_REGISTER_EXPOSURE_FINE_LOW        (0x3005)
#define SENSOR_OV5630_REGISTER_R_PLL1                   (0x300E)
#define SENSOR_OV5630_REGISTER_R_PLL2                   (0x300F)
#define SENSOR_OV5630_REGISTER_R_PLL3                   (0x3010)
#define SENSOR_OV5630_REGISTER_R_PLL4                   (0x3011)
#define SENSOR_OV5630_REGISTER_AUTO_1                   (0x3013)
#define SENSOR_OV5630_REGISTER_AUTO_2                   (0x3014)
#define SENSOR_OV5630_REGISTER_AUTO_3                   (0x3015)
#define SENSOR_OV5630_REGISTER_R_CLK                    (0x30BF)
#define SENSOR_OV5630_REGISTER_IMAGE_TRANSFORM          (0x30F8)
#define SENSOR_OV5630_REGISTER_IMAGE_LUM                (0x30F9)
#define SENSOR_OV5630_REGISTER_IMAGE_SYSTEM             (0x30FA)
#define SENSOR_OV5630_REGISTER_ISP_CTRL00               (0x3300)
#define SENSOR_OV5630_REGISTER_DSP_HSIZE_IN_HIGH        (0x3314)
#define SENSOR_OV5630_REGISTER_DSP_HSIZE_IN_LOW         (0x3315)
#define SENSOR_OV5630_REGISTER_DSP_VSIZE_IN_HIGH        (0x3316)
#define SENSOR_OV5630_REGISTER_DSP_VSIZE_IN_LOW         (0x3317)
#define SENSOR_OV5630_REGISTER_DSP_PAD_OUT              (0x3318)


#define SUPPORT_1080P_RES (0)
#define SUPPORT_QUARTER_RES (0)
#define SUPPORT_60FPS (0)

#define VALUE_HIGH(v) (((v) >> 8) & 0xFF)
#define VALUE_LOW(v) ((v) & 0xFF)

#define LENS_FOCAL_LENGTH (4.76f)
#define LENS_HORIZONTAL_VIEW_ANGLE (50.1f)
#define LENS_VERTICAL_VIEW_ANGLE (38.5f)

static NvOdmImagerExpectedValues gs_ExpValues5630 = {
    {
     //   x, y, w,    h,   r,   gr,   gb,    b.
     // white
     {    8, 8, 4, 1928, 1023, 1023, 1023, 1023},
     {  316, 8, 4, 1928, 1023, 1023, 1023, 1023},

     // yellow
     {  332, 8, 4, 1928, 1023, 1023, 1023,    0},
     {  640, 8, 4, 1928, 1023, 1023, 1023,    0},

     // cyan
     {  656, 8, 4, 1928,    0, 1023, 1023, 1023},
     {  964, 8, 4, 1928,    0, 1023, 1023, 1023},

     // green
     {  980, 8, 4, 1928,    0, 1023, 1023,    0},
     { 1288, 8, 4, 1928,    0, 1023, 1023,    0},

     // magenta
     { 1304, 8, 4, 1928, 1023,    0,    0, 1023},
     { 1612, 8, 4, 1928, 1023,    0,    0, 1023},

     // red
     { 1628, 8, 4, 1928, 1023,    0,    0,    0},
     { 1928, 8, 4, 1928, 1023,    0,    0,    0},

     // blue
     { 1952, 8, 4, 1928,    0,    0,    0, 1023},
     { 2260, 8, 4, 1928,    0,    0,    0, 1023},

     // black
     { 2276, 8, 4, 1928,    0,    0,    0,    0},
     { 2584, 8, 4, 1928,    0,    0,    0,    0}}
};

#define SENSOR_EXPECTED_VALUES gs_ExpValues5630

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

#define CSI_A 1 // To switch between CSI-A and CSI-B
#define ONE_LANE 1
#define FIFTY_MHz 1 // else would be for 30 MHz


static NvOdmImagerCapabilities g_SensorBayerOV5630Caps =
{
    "Omnivision 5630",  // string identifier, used for debugging
#if CSI_A
    NvOdmImagerSensorInterface_SerialA,
#else
    NvOdmImagerSensorInterface_SerialB,
#endif
    {
        NvOdmImagerPixelType_BayerBGGR,
    },
#if (BUILD_FOR_HARMONY==1)
    NvOdmImagerOrientation_90_Degrees, // rotation with respect to main display
#else
    NvOdmImagerOrientation_0_Degrees, // rotation with respect to main display
#endif
    NvOdmImagerDirection_Away,          // toward or away the main display
#if (BUILD_FOR_ARUBA==1)
    13000, // initial sensor clock (in khz)
    { {13000, 13.f/13.f}}, // preferred clock profile
#else
    6000, // initial sensor clock (in khz)
    { {22000, 90.f/22.f}}, // preferred clock profile
#endif
      //{6000, 1.0}},      // backward compatible profile
    {
        NvOdmImagerSyncEdge_Rising, // hsync polarity
        NvOdmImagerSyncEdge_Rising,//Rising, // vsync polarity
        NV_FALSE,                   // uses mclk on vgp0
    }, // parallel settings (VIP)
    { 
        NVODMSENSORMIPITIMINGS_PORT_CSI_A,
#if ONE_LANE
        NVODMSENSORMIPITIMINGS_DATALANES_ONE,
#else
        NVODMSENSORMIPITIMINGS_DATALANES_TWO,
#endif
        NVODMSENSORMIPITIMINGS_VIRTUALCHANNELID_ONE,
#if (BUILD_FOR_ARUBA==1)
        0x0, // continuos clk mode
#else
        0x1,
#endif
        0x2
    }, // serial settings (CSI) 
    //{ 18, 24 }, // min blank time, shouldn't need adjustment
    ///{ 40, 16 }, // min blank time, shouldn't need adjustment
    { 16, 16 }, // min blank time, shouldn't need adjustment
    //{ 11, 5 }, //Bad value for OV
    //{ 480, 56 }, // min blank time, shouldn't need adjustment
    0, // preferred mode, which resolution to use on startup
#if (BUILD_FOR_ARUBA==1)
    0,
#else
    FOCUSER_AD5820_GUID, // FOCUSER_GUID, only set if focuser code available
#endif
#if (BUILD_FOR_HARMONY==1) || (BUILD_FOR_ARUBA==1)
    0ULL,  // no flash on harmony
#else
    FLASH_LTC3216_GUID, // FLASH_GUID, only set if flash device code is available
#endif
    NVODM_IMAGER_CAPABILITIES_END
};


typedef struct ModeDependentSettingsRec
{
    NvU32 LineLength;
    NvU32 FrameLength;
    NvU32 CoarseTime;
    NvU32 MinFrameLength;
    NvBool SupportsFastMode;
} ModeDependentSettings;

#if SUPPORT_60FPS
static ModeDependentSettings ModeDependentSettings_1280x720 =
{
    SENSOR_OV5630_DEFAULT_LINE_LENGTH_720P,
    SENSOR_OV5630_DEFAULT_FRAME_LENGTH_720P,
    SENSOR_OV5630_DEFAULT_EXPOSURE_COARSE_QUAR,
    SENSOR_OV5630_DEFAULT_MIN_FRAME_LENGTH_720P,
    NV_FALSE, // no fast mode
};
#endif

#if SUPPORT_1080P_RES
static ModeDependentSettings ModeDependentSettings_1920x1088 =
{
    SENSOR_OV5630_DEFAULT_LINE_LENGTH_1080P,
    SENSOR_OV5630_DEFAULT_FRAME_LENGTH_1080P,
    SENSOR_OV5630_DEFAULT_EXPOSURE_COARSE_QUAR,
    SENSOR_OV5630_DEFAULT_MIN_FRAME_LENGTH_1080P,
    NV_FALSE, // no fast mode
};
#endif

#if SUPPORT_QUARTER_RES
static ModeDependentSettings ModeDependentSettings_1296x972 = 
{
    SENSOR_OV5630_DEFAULT_LINE_LENGTH_QUAR,
    SENSOR_OV5630_DEFAULT_FRAME_LENGTH_QUAR,
    SENSOR_OV5630_DEFAULT_EXPOSURE_COARSE_QUAR,
    SENSOR_OV5630_DEFAULT_MIN_FRAME_LENGTH_QUAR,
    NV_TRUE, // fast mode
};
#endif
static ModeDependentSettings ModeDependentSettings_2592x1944 = 
{
    SENSOR_OV5630_DEFAULT_LINE_LENGTH_FULL,
    SENSOR_OV5630_DEFAULT_FRAME_LENGTH_FULL,
    SENSOR_OV5630_DEFAULT_EXPOSURE_COARSE_FULL,
    SENSOR_OV5630_DEFAULT_MIN_FRAME_LENGTH_FULL,
    NV_TRUE, // fast mode
};

/**
 * SetMode Sequence for 2592x1944. Phase 0. Sensor Dependent.
 * This sequence should put sensor in streaming mode for 2592x1944
 * This is usually given by the FAE or the sensor vendor.
 */
 #if (BUILD_FOR_ARUBA==1)
 static DevCtrlReg16 SetModeSequence_2592x1944[] =
{
    {0x3012, 0x80},
    {SEQUENCE_WAIT_MS, 1000},
    {SENSOR_OV5630_REGISTER_IMAGE_SYSTEM, 0x00}, //IMAGE_SYSTEM; set to 1 later; kills identify_clk so comment out
    {SENSOR_OV5630_REGISTER_R_PLL2, 0x1},   //R_PLL2; set to 1 later

    {0x30b2, 0x32},
    {0x3084, 0x44},
    {0x3016, 0x01},
    {0x308a, 0x25},

    // OV suggested to use 0x0
    {SENSOR_OV5630_REGISTER_AUTO_1, 0xff},
    {SENSOR_OV5630_REGISTER_AUTO_3, 0x03},
    {SENSOR_OV5630_REGISTER_R_CLK, 0x02},
    {SENSOR_OV5630_REGISTER_ISP_CTRL00, 0xf8},

    {0x3069, 0x00},
    {0x3065, 0x50},
    {0x3068, 0x08},
    {0x30ac, 0x03},
    {0x309d, 0x00},
    {0x309e, 0x24},
    {0x3098, 0x58},
    {0x3091, 0x04},

    {0x3075, 0x22},
    {0x3076, 0x22},
    {0x3077, 0x24},
    {0x3078, 0x24},

    {0x30ae, 0x25},
    {0x30b5, 0x0c},
    {0x3090, 0x37},
    {0x3099, 0x85},

    {SENSOR_OV5630_REGISTER_IMAGE_LUM, 0x11},
    {0x3312, 0x10},

    {0x3103, 0x10},
    {0x305c, 0x01},
    {0x305d, 0x29},
    {0x305e, 0x00},
    {0x305f, 0xf7},
    {0x308d, 0x0b},
    {0x30ad, 0x10},
    {0x3072, 0x0d},

    {0x308b, 0x82},
    // keep vertical relationship to 4 more lines to y output
    {SENSOR_OV5630_REGISTER_DSP_VSIZE_IN_LOW, 0x9C + OV5630_FULL_RES_NUM_MORE_LINES},
    {SENSOR_OV5630_REGISTER_DSP_PAD_OUT, 0x20},
    {SENSOR_OV5630_REGISTER_X_ADDR_START_LOW, 0x20},
    {SENSOR_OV5630_REGISTER_Y_ADDR_START_LOW, 0x08}, // OVT suggested 0x4.
    {SENSOR_OV5630_REGISTER_X_ADDR_END_LOW, 0x3f},
    {SENSOR_OV5630_REGISTER_Y_ADDR_END_LOW, 0xa3},

    {SENSOR_OV5630_REGISTER_R_PLL1, 0xa2},
#if FIFTY_MHz
    {SENSOR_OV5630_REGISTER_R_PLL2, 0x90},
#else
    {SENSOR_OV5630_REGISTER_R_PLL2, 0xf0}, // 30 MHz
#endif
#if ONE_LANE
    {SENSOR_OV5630_REGISTER_R_PLL3, 0x03},  // ONE_LANE
#else
    {SENSOR_OV5630_REGISTER_R_PLL3, 0x07},  // TWO_LANE
#endif
    {SENSOR_OV5630_REGISTER_R_PLL4, 0x40},

    {SENSOR_OV5630_REGISTER_IMAGE_TRANSFORM, 0x00},

    {0x3069, 0x80},
    {0x309d, 0x04},
    {0x309e, 0x24},
    {0x3071, 0x20},

    {SENSOR_OV5630_REGISTER_ISP_CTRL00, 0xf3},

    {0x331f, 0x22},
    {0x3098, 0x50},
    {0x30af, 0x10},
    {0x304a, 0x00},
    {0x304f, 0x00},

    {0x304e, 0x22},
    {0x304f, 0xa0},
    {0x3058, 0x00},
    {0x3059, 0xff},
    {0x305a, 0x00},

    {0x30e9, 0x04},

    {0x3084, 0x44},
    {0x3090, 0x37},
    {0x30e9, 0x04},

    {0x30b5, 0x1c},
    {0x331f, 0x22},
    {0x30b0, 0xfe},

    // MIPI settings
    {0x30b0, 0x00},
    {0x30b1, 0xfc},
    {0x3603, 0x50},
    {0x3601, 0x0f},

#if ONE_LANE
    {SENSOR_OV5630_REGISTER_R_PLL3, 0x03},  // ONE_LANE
#else
    {SENSOR_OV5630_REGISTER_R_PLL3, 0x07},  // TWO_LANE
#endif

    //output test pattern
    //{0x3301, 0x02},      // color bar - bit[1] color bar enable
    //{0x3508, 0x80},      // test pattern ON/OFF select bit[7]

    {SENSOR_OV5630_REGISTER_IMAGE_SYSTEM, 0x01},

    {0x3096, 0x50},
    //{0x3600, 0x24},

    {SEQUENCE_END, 0x0000}
};
#else
static DevCtrlReg16 SetModeSequence_2592x1944[] =
{
    {0x3012, 0x80},
    {SEQUENCE_WAIT_MS, 100},
    {SENSOR_OV5630_REGISTER_IMAGE_SYSTEM, 0x00},
    {SENSOR_OV5630_REGISTER_R_PLL1, 0x74},
    {SENSOR_OV5630_REGISTER_R_PLL2, 0x00},
    {SENSOR_OV5630_REGISTER_R_PLL3, 0x03},
    {SENSOR_OV5630_REGISTER_R_PLL4, 0x40},
    {0x30b2, 0x22},
    {0x3084, 0x44},
    {0x3016, 0x01},
    {0x308a, 0x25},

    // OV suggested to use 0x0
    {SENSOR_OV5630_REGISTER_AUTO_1, 0x0},
    {SENSOR_OV5630_REGISTER_AUTO_2, 0x0},
    {SENSOR_OV5630_REGISTER_AUTO_3, 0x02},

    {SENSOR_OV5630_REGISTER_R_CLK, 0x02},
    //{SENSOR_OV5630_REGISTER_ISP_CTRL00, 0xf8},

    {0x3069, 0x00},
    {0x3065, 0x50},
    {0x3068, 0x08},
    {0x30ac, 0x03},
    {0x3098, 0x58},
   
    {0x309d, 0x00},
    {0x309e, 0x24},
    {0x3098, 0x5c},
    {0x3091, 0x04},

    {0x3075, 0x22},
    {0x3076, 0x22},
    {0x3077, 0x24},
    {0x3078, 0x24},
    {0x30ae, 0x25},
    {0x30b5, 0x0c},
    {0x3090, 0x37},
    {0x3099, 0x85},

    {SENSOR_OV5630_REGISTER_IMAGE_LUM, 0x11},
    {0x3312, 0x10},

    {0x3103, 0x10},

    {0x305c, 0x00},
    {0x305d, 0x7E},
    {0x305e, 0x00},
    {0x305f, 0x69},
  
    {0x308d, 0x0b},
    {0x30ad, 0x10},
    {0x3072, 0x0d},

    {0x308b, 0x82},

    {SENSOR_OV5630_REGISTER_IMAGE_TRANSFORM, 0x00},

    {0x3069, 0x80},
    {0x309d, 0x04},
    {0x309e, 0x24},
    {0x3071, 0x20},

    {SENSOR_OV5630_REGISTER_ISP_CTRL00, 0x01},
    {0x3302, 0xc0},
  
    {0x331f, 0x22},
    {0x3098, 0x50},
    {0x30af, 0x10},
    {0x304a, 0x00},
    {0x304f, 0x00},

    {0x304e, 0x22},
    {0x304f, 0xa0},
    //{0x3058, 0x00},
    {0x3059, 0xff},
    //{0x305a, 0x00},

    {0x30e9, 0x04},

    {0x3084, 0x44},
    {0x3090, 0x37},
   
    {0x30b5, 0x1c},
    {0x331f, 0x22},

    {SENSOR_OV5630_REGISTER_EXPOSURE_FINE_HIGH,
        VALUE_HIGH(SENSOR_OV5630_DEFAULT_EXPOSURE_FINE)},
    {SENSOR_OV5630_REGISTER_EXPOSURE_FINE_LOW,
        VALUE_LOW(SENSOR_OV5630_DEFAULT_EXPOSURE_FINE)},

    //{0x30b0, 0xfe},
    {0x30b0, 0x00},
    {0x30b1, 0xfc},
    {0x3603, 0x50},
    {0x3601, 0x0f},
//    {SENSOR_OV5630_REGISTER_R_PLL3, 0x03},
    {SENSOR_OV5630_REGISTER_IMAGE_SYSTEM, 0x01},
    {0x3096, 0x50},
    {0x3600, 0x24},

    {SEQUENCE_FAST_SETMODE_START, 0},

    {SENSOR_OV5630_REGISTER_FRAME_LENGTH_LINES_HIGH,
        VALUE_HIGH(SENSOR_OV5630_DEFAULT_FRAME_LENGTH_FULL)},
    {SENSOR_OV5630_REGISTER_FRAME_LENGTH_LINES_LOW,
        VALUE_LOW(SENSOR_OV5630_DEFAULT_FRAME_LENGTH_FULL)},
    {SENSOR_OV5630_REGISTER_LINE_LENGTH_PCK_HIGH,
        VALUE_HIGH(SENSOR_OV5630_DEFAULT_LINE_LENGTH_FULL)},
    {SENSOR_OV5630_REGISTER_LINE_LENGTH_PCK_LOW,
        VALUE_LOW(SENSOR_OV5630_DEFAULT_LINE_LENGTH_FULL)},
    {SENSOR_OV5630_REGISTER_X_ADDR_START_HIGH, 0x00},
    {SENSOR_OV5630_REGISTER_X_ADDR_START_LOW, 0x14},
    {SENSOR_OV5630_REGISTER_Y_ADDR_START_HIGH, 0x00},

    // may change to 0x0 if preview->full->preview has minute vertical shift
    {SENSOR_OV5630_REGISTER_Y_ADDR_START_LOW, 0x00}, // OVT suggested 0x4.

    {SENSOR_OV5630_REGISTER_X_ADDR_END_HIGH, 0x0A},
    {SENSOR_OV5630_REGISTER_X_ADDR_END_LOW, 0x3f},
    {SENSOR_OV5630_REGISTER_Y_ADDR_END_HIGH, 0x07},
    {SENSOR_OV5630_REGISTER_Y_ADDR_END_LOW, 0xa3},

    {SENSOR_OV5630_REGISTER_X_OUTPUT_SIZE_HIGH, 0x0A},
    {SENSOR_OV5630_REGISTER_X_OUTPUT_SIZE_LOW, 0x20},
    {SENSOR_OV5630_REGISTER_Y_OUTPUT_SIZE_HIGH, 0x07},

    // y output lower byte: from 1944 lines to 1948 output lines
    {SENSOR_OV5630_REGISTER_Y_OUTPUT_SIZE_LOW, 0x98 + OV5630_FULL_RES_NUM_MORE_LINES},

    {SENSOR_OV5630_REGISTER_IMAGE_TRANSFORM, 0x00},

    {SENSOR_OV5630_REGISTER_IMAGE_LUM, 0x11},

    //{0x30b3, 0x00},
    //{0x3301, 0xc0},
    //{0x3313, 0xf0},

    {SENSOR_OV5630_REGISTER_DSP_HSIZE_IN_HIGH, 0x0A},
    {SENSOR_OV5630_REGISTER_DSP_HSIZE_IN_LOW, 0x22},
    {SENSOR_OV5630_REGISTER_DSP_VSIZE_IN_HIGH, 0x07},

    // keep vertical relationship to 4 more lines to y output
    {SENSOR_OV5630_REGISTER_DSP_VSIZE_IN_LOW, 0x9C + OV5630_FULL_RES_NUM_MORE_LINES},

    {SENSOR_OV5630_REGISTER_DSP_PAD_OUT, 0x22},

    {0x3077, 0x24},
    {0x3078, 0x25},

    {SENSOR_OV5630_REGISTER_R_PLL2, 0x00},

    {SENSOR_OV5630_REGISTER_LINE_LENGTH_PCK_HIGH, 0x0c},
    {SENSOR_OV5630_REGISTER_LINE_LENGTH_PCK_LOW, 0xA0},

    {SENSOR_OV5630_REGISTER_EXPOSURE_COARSE_HIGH,
        VALUE_HIGH(SENSOR_OV5630_DEFAULT_EXPOSURE_COARSE_FULL)},
    {SENSOR_OV5630_REGISTER_EXPOSURE_COARSE_LOW,
        VALUE_LOW(SENSOR_OV5630_DEFAULT_EXPOSURE_COARSE_FULL)},

    {SEQUENCE_FAST_SETMODE_END, 0},

    {SEQUENCE_END, 0x0000}
};
#endif

#if SUPPORT_QUARTER_RES 
/**
 * SetMode Sequence for 1296x972. Phase 3. Sensor Dependent.
 * This sequence should put sensor in streaming mode for 1296x972.
 * This is derived from the sequence for 2592x1944.
 */
static DevCtrlReg16 SetModeSequence_1296x972[] =
{
    {0x3012, 0x80}, // system control
    {SEQUENCE_WAIT_MS, 100},

    {SENSOR_OV5630_REGISTER_IMAGE_SYSTEM, 0x00}, // IMAGE_SYSTEM
    {SENSOR_OV5630_REGISTER_R_PLL2, 0x00}, // R_PLL2
    {0x30b2, 0x22}, // IO_CTRL
    {0x3084, 0x44},
    {0x3016, 0x01}, // AUTO_4
    {0x308a, 0x25},
        
    {SENSOR_OV5630_REGISTER_AUTO_1, 0x0},
    {SENSOR_OV5630_REGISTER_AUTO_2, 0x0},
    {SENSOR_OV5630_REGISTER_AUTO_3, 0x02}, //max gain is 8x
    {SENSOR_OV5630_REGISTER_R_CLK, 0x02},
                   
    //{SENSOR_OV5630_REGISTER_ISP_CTRL00, 0xf8},
           
    {0x3069, 0x00},
    {0x3065, 0x50},
    {0x3068, 0x08},
    {0x30ac, 0x03},
    {0x309d, 0x00},
    {0x309e, 0x24},
    {0x3098, 0x58},
    {0x3091, 0x04},
        
    {0x3075, 0x22},
    {0x3076, 0x22},
    {0x3077, 0x24},
    {0x3078, 0x24},
        
    {0x30ae, 0x25},
    {0x30b5, 0x0c}, // DSIO?
    {0x3090, 0x37},
    {0x3099, 0x85},
        
    {SENSOR_OV5630_REGISTER_IMAGE_LUM, 0x11},
    {0x3312, 0x10},
        
    {0x3103, 0x10},
    {0x305c, 0x00},
    {0x305d, 0x7E},
    {0x305e, 0x00},
    {0x305f, 0x69},
    {0x308d, 0x0b},
    {0x30ad, 0x10},
    {0x3072, 0x0d},
        
    {0x308b, 0x82},
                
    {SENSOR_OV5630_REGISTER_R_PLL1, 0x74},
    {SENSOR_OV5630_REGISTER_R_PLL2, 0x00},
    {SENSOR_OV5630_REGISTER_R_PLL3, 0x03},
    {SENSOR_OV5630_REGISTER_R_PLL4, 0x40},
        
    //{SENSOR_OV5630_REGISTER_IMAGE_TRANSFORM, 0x00},

    {0x3069, 0x80},
    {0x309d, 0x04},
    {0x309e, 0x24},
    {0x3071, 0x20},
    {SENSOR_OV5630_REGISTER_ISP_CTRL00, 0x01},
           
    {0x331f, 0x22},
    {0x3098, 0x50},
                
    {0x30af, 0x10},
    {0x304a, 0x00},
    {0x304f, 0x00},
        
    {0x304e, 0x22},
    {0x304f, 0xa0},
    //{0x3058, 0x00},
    {0x3059, 0xff},
    //{0x305a, 0x00},
        
    //{0x30e9, 0x04},
        
    {0x3084, 0x44},
    {0x3090, 0x37},
    {0x30e9, 0x04},
        
    {0x30b5, 0x1c},
    {0x331f, 0x22},

    {SENSOR_OV5630_REGISTER_IMAGE_TRANSFORM, 0x05},
    {0x3319, 0x03},

    {SENSOR_OV5630_REGISTER_EXPOSURE_FINE_HIGH,
        VALUE_HIGH(SENSOR_OV5630_DEFAULT_EXPOSURE_FINE)},
    {SENSOR_OV5630_REGISTER_EXPOSURE_FINE_LOW,
        VALUE_LOW(SENSOR_OV5630_DEFAULT_EXPOSURE_FINE)},

    //{0x30b0, 0xfe},

    {0x30b0, 0x00},
    {0x30b1, 0xfc},
        
    {0x3603, 0x50},
    {0x3601, 0x0F},
        
    //{SENSOR_OV5630_REGISTER_R_PLL3, 0x03},
        
    {SENSOR_OV5630_REGISTER_IMAGE_SYSTEM, 0x01},
        
    {0x3096, 0x50},

    {0x3600, 0x24},

    {SEQUENCE_FAST_SETMODE_START, 0},

    {SENSOR_OV5630_REGISTER_FRAME_LENGTH_LINES_HIGH,
        VALUE_HIGH(SENSOR_OV5630_DEFAULT_FRAME_LENGTH_QUAR)},
    {SENSOR_OV5630_REGISTER_FRAME_LENGTH_LINES_LOW,
        VALUE_LOW(SENSOR_OV5630_DEFAULT_FRAME_LENGTH_QUAR)},
    {SENSOR_OV5630_REGISTER_LINE_LENGTH_PCK_HIGH,
        VALUE_HIGH(SENSOR_OV5630_DEFAULT_LINE_LENGTH_QUAR)},
    {SENSOR_OV5630_REGISTER_LINE_LENGTH_PCK_LOW,
        VALUE_LOW(SENSOR_OV5630_DEFAULT_LINE_LENGTH_QUAR)},
    {SENSOR_OV5630_REGISTER_X_ADDR_START_HIGH, 0x00},
    {SENSOR_OV5630_REGISTER_X_ADDR_START_LOW, 0x14},
    {SENSOR_OV5630_REGISTER_Y_ADDR_START_HIGH, 0x00},
    {SENSOR_OV5630_REGISTER_Y_ADDR_START_LOW, 0x00},

    {SENSOR_OV5630_REGISTER_X_ADDR_END_HIGH, 0x0A},
    {SENSOR_OV5630_REGISTER_X_ADDR_END_LOW, 0x3f},
    {SENSOR_OV5630_REGISTER_Y_ADDR_END_HIGH, 0x07},
    {SENSOR_OV5630_REGISTER_Y_ADDR_END_LOW, 0xa3},

    {SENSOR_OV5630_REGISTER_X_OUTPUT_SIZE_HIGH, 0x05},
    {SENSOR_OV5630_REGISTER_X_OUTPUT_SIZE_LOW, 0x10},
    {SENSOR_OV5630_REGISTER_Y_OUTPUT_SIZE_HIGH, 0x03},

    // y output lower byte: from 972 lines to 976 output lines
    {SENSOR_OV5630_REGISTER_Y_OUTPUT_SIZE_LOW, 0xcc + OV5630_QUART_RES_NUM_MORE_LINES}, // 0xD0

    {SENSOR_OV5630_REGISTER_IMAGE_TRANSFORM, 0x05},

    {SENSOR_OV5630_REGISTER_IMAGE_LUM, 0x13},

    {SENSOR_OV5630_REGISTER_DSP_HSIZE_IN_HIGH, 0x05},
    {SENSOR_OV5630_REGISTER_DSP_HSIZE_IN_LOW, 0x14},
    {SENSOR_OV5630_REGISTER_DSP_VSIZE_IN_HIGH, 0x03},

    {SENSOR_OV5630_REGISTER_DSP_VSIZE_IN_LOW, 0xd0 + OV5630_QUART_RES_NUM_MORE_LINES}, // 0xD4

    {SENSOR_OV5630_REGISTER_DSP_PAD_OUT, 0x22},

    {0x3077, 0x24},
    {0x3078, 0x24},

    {SENSOR_OV5630_REGISTER_R_PLL2, 0x00},

    {SENSOR_OV5630_REGISTER_EXPOSURE_COARSE_HIGH,
        VALUE_HIGH(SENSOR_OV5630_DEFAULT_EXPOSURE_COARSE_QUAR)},
    {SENSOR_OV5630_REGISTER_EXPOSURE_COARSE_LOW,
        VALUE_LOW(SENSOR_OV5630_DEFAULT_EXPOSURE_COARSE_QUAR)},

    {SEQUENCE_FAST_SETMODE_END, 0},

    {SEQUENCE_END, 0x0000}

};
#endif

#if SUPPORT_60FPS
/**
 * SetMode Sequence for 1280x720. Phase 3. Sensor Dependent.
 * This sequence should put sensor in streaming mode for 1280x720.
 * This is derived from the sequence for 1296x972.
 */
static DevCtrlReg16 SetModeSequence_1280x720[] =
{
    {0x3012, 0x80}, // system control
    {SEQUENCE_WAIT_MS, 100},

    {SENSOR_OV5630_REGISTER_IMAGE_SYSTEM, 0x00}, // IMAGE_SYSTEM
    {SENSOR_OV5630_REGISTER_R_PLL2, 0x00}, // R_PLL2
    {0x30b2, 0x22}, // IO_CTRL
    {0x3084, 0x44},
    {0x3016, 0x01}, // AUTO_4
    {0x308a, 0x25},

    {SENSOR_OV5630_REGISTER_AUTO_1, 0x0},
    {SENSOR_OV5630_REGISTER_AUTO_2, 0x0},
    {SENSOR_OV5630_REGISTER_AUTO_3, 0x02}, //max gain is 8x
    {SENSOR_OV5630_REGISTER_R_CLK, 0x02},

    {0x3069, 0x00},
    {0x3065, 0x50},
    {0x3068, 0x08},
    {0x30ac, 0x03},
    {0x309d, 0x00},
    {0x309e, 0x24},
    {0x3098, 0x58},
    {0x3091, 0x04},

    {0x3075, 0x22},
    {0x3076, 0x22},
    {0x3077, 0x24},
    {0x3078, 0x24},

    {0x30ae, 0x25},
    {0x30b5, 0x0c}, // DSIO?
    {0x3090, 0x37},
    {0x3099, 0x85},

    {SENSOR_OV5630_REGISTER_IMAGE_LUM, 0x13},
    {0x3312, 0x10},

    {0x3103, 0x10},
    {0x305c, 0x00},
    {0x305d, 0x7E},
    {0x305e, 0x00},
    {0x305f, 0x69},
    {0x308d, 0x0b},
    {0x30ad, 0x10},
    {0x3072, 0x0d},

    {0x308b, 0x82},

    {SENSOR_OV5630_REGISTER_R_PLL1, 0x74},
    {SENSOR_OV5630_REGISTER_R_PLL2, 0x00},
    {SENSOR_OV5630_REGISTER_R_PLL3, 0x03},
    {SENSOR_OV5630_REGISTER_R_PLL4, 0x40},

    //{SENSOR_OV5630_REGISTER_IMAGE_TRANSFORM, 0x00},

    {0x3069, 0x80},
    {0x309d, 0x04},
    {0x309e, 0x24},
    {0x3071, 0x20},
    {SENSOR_OV5630_REGISTER_ISP_CTRL00, 0x01},

    {0x331f, 0x22},
    {0x3098, 0x50},

    {0x30af, 0x10},
    {0x304a, 0x00},
    {0x304f, 0x00},

    {0x304e, 0x22},
    {0x304f, 0xa0},
    {0x3059, 0xff},

    {0x3084, 0x44},
    {0x3090, 0x37},
    {0x30e9, 0x04},

    {0x30b5, 0x1c},
    {0x331f, 0x22},

    {SENSOR_OV5630_REGISTER_IMAGE_TRANSFORM, 0x05},
    {0x3319, 0x03},

    {SENSOR_OV5630_REGISTER_EXPOSURE_FINE_HIGH,
        VALUE_HIGH(SENSOR_OV5630_DEFAULT_EXPOSURE_FINE)},
    {SENSOR_OV5630_REGISTER_EXPOSURE_FINE_LOW,
        VALUE_LOW(SENSOR_OV5630_DEFAULT_EXPOSURE_FINE)},

    {0x30b0, 0x00},
    {0x30b1, 0xfc},

    {0x3603, 0x50},
    {0x3601, 0x0F},

    {SENSOR_OV5630_REGISTER_IMAGE_SYSTEM, 0x01},

    {0x3096, 0x50},

    {0x3600, 0x24},

    {SEQUENCE_FAST_SETMODE_START, 0},

    {SENSOR_OV5630_REGISTER_FRAME_LENGTH_LINES_HIGH,
        VALUE_HIGH(SENSOR_OV5630_DEFAULT_FRAME_LENGTH_720P)},
    {SENSOR_OV5630_REGISTER_FRAME_LENGTH_LINES_LOW,
        VALUE_LOW(SENSOR_OV5630_DEFAULT_FRAME_LENGTH_720P)},
    {SENSOR_OV5630_REGISTER_LINE_LENGTH_PCK_HIGH,
        VALUE_HIGH(SENSOR_OV5630_DEFAULT_LINE_LENGTH_720P)},
    {SENSOR_OV5630_REGISTER_LINE_LENGTH_PCK_LOW,
        VALUE_LOW(SENSOR_OV5630_DEFAULT_LINE_LENGTH_720P)},
    {SENSOR_OV5630_REGISTER_X_ADDR_START_HIGH, 0x00},
    {SENSOR_OV5630_REGISTER_X_ADDR_START_LOW, 0x14},
    {SENSOR_OV5630_REGISTER_Y_ADDR_START_HIGH, 0x00},
    {SENSOR_OV5630_REGISTER_Y_ADDR_START_LOW, 0x00},

    {SENSOR_OV5630_REGISTER_X_ADDR_END_HIGH, 0x0A},
    {SENSOR_OV5630_REGISTER_X_ADDR_END_LOW, 0x2f},
    {SENSOR_OV5630_REGISTER_Y_ADDR_END_HIGH, 0x07},
    {SENSOR_OV5630_REGISTER_Y_ADDR_END_LOW, 0x97},

    {SENSOR_OV5630_REGISTER_X_OUTPUT_SIZE_HIGH, 0x05},
    {SENSOR_OV5630_REGISTER_X_OUTPUT_SIZE_LOW, 0x00},
    {SENSOR_OV5630_REGISTER_Y_OUTPUT_SIZE_HIGH, 0x02},

    // y output lower byte: from 972 lines to 976 output lines
    {SENSOR_OV5630_REGISTER_Y_OUTPUT_SIZE_LOW, 0xd0 + OV5630_QUART_RES_NUM_MORE_LINES}, // 0xD0

    {SENSOR_OV5630_REGISTER_IMAGE_TRANSFORM, 0x05},

    {SENSOR_OV5630_REGISTER_IMAGE_LUM, 0x13},

    {SENSOR_OV5630_REGISTER_DSP_HSIZE_IN_HIGH, 0x05},
    {SENSOR_OV5630_REGISTER_DSP_HSIZE_IN_LOW, 0x04},
    {SENSOR_OV5630_REGISTER_DSP_VSIZE_IN_HIGH, 0x02},

    {SENSOR_OV5630_REGISTER_DSP_VSIZE_IN_LOW, 0xd4 + OV5630_QUART_RES_NUM_MORE_LINES}, // 0xD4

    {SENSOR_OV5630_REGISTER_DSP_PAD_OUT, 0x22},

    {0x3077, 0x24},
    {0x3078, 0x24},

    {SENSOR_OV5630_REGISTER_R_PLL2, 0x00},

    {SENSOR_OV5630_REGISTER_EXPOSURE_COARSE_HIGH,
        VALUE_HIGH(SENSOR_OV5630_DEFAULT_EXPOSURE_COARSE_QUAR)},
    {SENSOR_OV5630_REGISTER_EXPOSURE_COARSE_LOW,
        VALUE_LOW(SENSOR_OV5630_DEFAULT_EXPOSURE_COARSE_QUAR)},

    {SEQUENCE_FAST_SETMODE_END, 0},

    {SEQUENCE_END, 0x0000}
};
#endif

/**
 * SetMode Sequence for 1920x1080. Phase 3. Sensor Dependent.
 * This sequence should put sensor in streaming mode for 1920x1080.
 * This is derived from the sequence for 1296x972.
 */
#if SUPPORT_1080P_RES
static DevCtrlReg16 SetModeSequence_1920x1088[] =
{
    {0x3012, 0x80}, // system control
    {SEQUENCE_WAIT_MS, 100},

    {SENSOR_OV5630_REGISTER_IMAGE_SYSTEM, 0x00}, // IMAGE_SYSTEM
    {SENSOR_OV5630_REGISTER_R_PLL2, 0x00}, // R_PLL2
    {0x30b2, 0x22}, // IO_CTRL
    {0x3084, 0x44},
    {0x3016, 0x01}, // AUTO_4
    {0x308a, 0x25},

    {SENSOR_OV5630_REGISTER_AUTO_1, 0x0},
    {SENSOR_OV5630_REGISTER_AUTO_2, 0x0},
    {SENSOR_OV5630_REGISTER_AUTO_3, 0x02}, //max gain is 8x
    {SENSOR_OV5630_REGISTER_R_CLK, 0x02},

    {0x3069, 0x00},
    {0x3065, 0x50},
    {0x3068, 0x08},
    {0x30ac, 0x03},
    {0x309d, 0x00},
    {0x309e, 0x24},
    {0x3098, 0x58},
    {0x3091, 0x04},

    {0x3075, 0x22},
    {0x3076, 0x22},
    {0x3077, 0x24},
    {0x3078, 0x24},

    {0x30ae, 0x25},
    {0x30b5, 0x0c}, // DSIO?
    {0x3090, 0x37},
    {0x3099, 0x85},

    {SENSOR_OV5630_REGISTER_IMAGE_LUM, 0x11},
    {0x3312, 0x10},

    {0x3103, 0x10},
    {0x305c, 0x00},
    {0x305d, 0x7E},
    {0x305e, 0x00},
    {0x305f, 0x69},
    {0x308d, 0x0b},
    {0x30ad, 0x10},
    {0x3072, 0x0d},

    {0x308b, 0x82},

    {SENSOR_OV5630_REGISTER_R_PLL1, 0x74},
    {SENSOR_OV5630_REGISTER_R_PLL2, 0x00},
    {SENSOR_OV5630_REGISTER_R_PLL3, 0x03},
    {SENSOR_OV5630_REGISTER_R_PLL4, 0x40},

    {0x3069, 0x80},
    {0x309d, 0x04},
    {0x309e, 0x24},
    {0x3071, 0x20},
    {SENSOR_OV5630_REGISTER_ISP_CTRL00, 0x01},

    {0x331f, 0x22},
    {0x3098, 0x50},

    {0x30af, 0x10},
    {0x304a, 0x00},
    {0x304f, 0x00},

    {0x304e, 0x22},
    {0x304f, 0xa0},
    {0x3059, 0xff},

    {0x3084, 0x44},
    {0x3090, 0x37},
    {0x30e9, 0x04},

    {0x30b5, 0x1c},
    {0x331f, 0x22},

    {0x3319, 0x03},

    {SENSOR_OV5630_REGISTER_EXPOSURE_FINE_HIGH,
        VALUE_HIGH(SENSOR_OV5630_DEFAULT_EXPOSURE_FINE)},
    {SENSOR_OV5630_REGISTER_EXPOSURE_FINE_LOW,
        VALUE_LOW(SENSOR_OV5630_DEFAULT_EXPOSURE_FINE)},

    {0x30b0, 0x00},
    {0x30b1, 0xfc},

    {0x3603, 0x50},
    {0x3601, 0x0F},

    {SENSOR_OV5630_REGISTER_IMAGE_SYSTEM, 0x01},

    {0x3096, 0x50},

    {0x3600, 0x24},

    {SEQUENCE_FAST_SETMODE_START, 0},

    {SENSOR_OV5630_REGISTER_FRAME_LENGTH_LINES_HIGH,
        VALUE_HIGH(SENSOR_OV5630_DEFAULT_FRAME_LENGTH_1080P)},
    {SENSOR_OV5630_REGISTER_FRAME_LENGTH_LINES_LOW,
        VALUE_LOW(SENSOR_OV5630_DEFAULT_FRAME_LENGTH_1080P)},
    {SENSOR_OV5630_REGISTER_LINE_LENGTH_PCK_HIGH,
        VALUE_HIGH(SENSOR_OV5630_DEFAULT_LINE_LENGTH_1080P)},
    {SENSOR_OV5630_REGISTER_LINE_LENGTH_PCK_LOW,
        VALUE_LOW(SENSOR_OV5630_DEFAULT_LINE_LENGTH_1080P)},
    {SENSOR_OV5630_REGISTER_X_ADDR_START_HIGH, 0x00},
    {SENSOR_OV5630_REGISTER_X_ADDR_START_LOW, 0x14},
    {SENSOR_OV5630_REGISTER_Y_ADDR_START_HIGH, 0x00},
    {SENSOR_OV5630_REGISTER_Y_ADDR_START_LOW, 0x00},

    {SENSOR_OV5630_REGISTER_X_ADDR_END_HIGH, 0x08},
    {SENSOR_OV5630_REGISTER_X_ADDR_END_LOW, 0xef}, // 2287
    {SENSOR_OV5630_REGISTER_Y_ADDR_END_HIGH, 0x05},
    {SENSOR_OV5630_REGISTER_Y_ADDR_END_LOW, 0xfb}, // 1523

    {SENSOR_OV5630_REGISTER_X_OUTPUT_SIZE_HIGH, 0x07},
    {SENSOR_OV5630_REGISTER_X_OUTPUT_SIZE_LOW, 0x80}, // 1920
    {SENSOR_OV5630_REGISTER_Y_OUTPUT_SIZE_HIGH, 0x04},

    // y output lower byte: from 972 lines to 976 output lines
    {SENSOR_OV5630_REGISTER_Y_OUTPUT_SIZE_LOW, 0x40 + OV5630_QUART_RES_NUM_MORE_LINES}, // 1084

    {SENSOR_OV5630_REGISTER_IMAGE_TRANSFORM, 0x00},

    {SENSOR_OV5630_REGISTER_IMAGE_LUM, 0x11},

    {SENSOR_OV5630_REGISTER_DSP_HSIZE_IN_HIGH, 0x07},
    {SENSOR_OV5630_REGISTER_DSP_HSIZE_IN_LOW, 0x82},
    {SENSOR_OV5630_REGISTER_DSP_VSIZE_IN_HIGH, 0x04},

    {SENSOR_OV5630_REGISTER_DSP_VSIZE_IN_LOW, 0x44 + OV5630_QUART_RES_NUM_MORE_LINES},

    {SENSOR_OV5630_REGISTER_DSP_PAD_OUT, 0x22},

    {0x3077, 0x24},
    {0x3078, 0x24},

    {SENSOR_OV5630_REGISTER_R_PLL2, 0x00},

    {SENSOR_OV5630_REGISTER_EXPOSURE_COARSE_HIGH,
        VALUE_HIGH(SENSOR_OV5630_DEFAULT_EXPOSURE_COARSE_QUAR)},
    {SENSOR_OV5630_REGISTER_EXPOSURE_COARSE_LOW,
        VALUE_LOW(SENSOR_OV5630_DEFAULT_EXPOSURE_COARSE_QUAR)},

    {SEQUENCE_FAST_SETMODE_END, 0},

    {SEQUENCE_END, 0x0000}
};
#endif

/**
 * A list of sensor mode and its SetModeSequence. Phase 0. Sensor Dependent.
 * When SetMode is called, it searches the list for desired resolution and
 * frame rate and write the SetModeSequence through i2c.
 * For Phase 3, more resolutions will be added to this table, allowing
 * for peak performance during Preview, D1 and 720P encode.
 */

static SensorSetModeSequence *g_SensorBayerOV5630SetModeSequenceList;

static SensorSetModeSequence g_SensorBayerSetModeSequenceListCSIDual[] =
{
    // vertical shift 2 more lines. TO CLEAN. 
    // WxH         x,y    fps  ar    set mode sequence
#if (BUILD_FOR_ARUBA==1)
    {{{2592, 1942}, {0, 2}, 14, 1.0, 20.0}, SetModeSequence_2592x1944,
     &ModeDependentSettings_2592x1944},
#else
    {{{2592, 1944}, {0, 2}, 14, 1.0, 20.0}, SetModeSequence_2592x1944,
     &ModeDependentSettings_2592x1944},// full res
#endif

#if SUPPORT_1080P_RES
     {{{1920, 1088},  {0, 2}, 30, 1.0, 20.0}, SetModeSequence_1920x1088,
     &ModeDependentSettings_1920x1088},
#endif

#if SUPPORT_QUARTER_RES 
    {{{1296, 972},  {0, 2}, 27, 1.0, 20.0}, SetModeSequence_1296x972,
     &ModeDependentSettings_1296x972}, // quarter res
#endif

#if SUPPORT_60FPS
    {{{1280, 720},  {0, 2}, 60, 1.0, 20.0}, SetModeSequence_1280x720,
     &ModeDependentSettings_1280x720},
#endif
};

static SensorSetModeSequence g_SensorBayerSetModeSequenceListCSISingle[] =
{
    {{{1280, 720},  {0, 2}, 60, 1.0}, SetModeSequence_2592x1944,
     &ModeDependentSettings_2592x1944},
    {{{1280, 720},  {0, 2}, 60, 1.0}, SetModeSequence_2592x1944,
     &ModeDependentSettings_2592x1944},
};

/**
 * ----------------------------------------------------------
 *  Start of Phase 1, Phase 2, and Phase 3 code
 * ----------------------------------------------------------
 */

/**
 * Sensor bayer's private context. Phase 1.
 */
typedef struct SensorBayerOV5630ContextRec
{
    // Phase 1 variables.
    NvOdmImagerI2cConnection I2c;
    NvOdmImagerI2cConnection I2cR;
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
    NvF32 MaxFrameRate;
    NvF32 MinFrameRate;
    NvF32 RequestedMaxFrameRate;

    // Phase 2 variables. Sensor Dependent.
    // These are used to set or get exposure, frame rate, and so on.
    NvU32 PllMult;
    NvF32 PllPreDiv;
    NvU32 PllVtPixDiv;
    NvU32 PllVtSysDiv;

    NvU32 CoarseTime;
    NvU32 VtPixClkFreqHz;

    NvU32 LineLength;
    NvU32 FrameLength;

    NvU32 MaxFrameLength;
    NvU32 MinFrameLength;
    NvBool TestPatternMode;
    NvBool FastSetMode;
    
    NvOdmImagerStereoCameraMode StereoCameraMode;
}SensorBayerOV5630Context;


/*
 * Static functions
 * Some compilers require these to be declared upfront.
 */
static NvBool SensorBayerOV5630_Open(NvOdmImagerHandle hImager);

static void SensorBayerOV5630_Close(NvOdmImagerHandle hImager);

static void
SensorBayerOV5630_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities);

static void
SensorBayerOV5630_ListModes(
    NvOdmImagerHandle hImager,
    NvOdmImagerSensorMode *pModes,
    NvS32 *pNumberOfModes);

static NvBool
SensorBayerOV5630_SetMode(
    NvOdmImagerHandle hImager,
    const SetModeParameters *pParameters,
    NvOdmImagerSensorMode *pSelectedMode,
    SetModeParameters *pResult);

static NvBool
SensorBayerOV5630_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel);

static NvBool
SensorBayerOV5630_SetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    const void *pValue);

static NvBool
SensorBayerOV5630_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue);

static void
SensorBayerOV5630_GetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel *pPowerLevel);

static NV_INLINE
NvBool SensorBayerOV5630_ChooseModeIndex(
    SensorBayerOV5630Context *pContext,
    NvSize Resolution,
    NvU32 *pIndex);

static NvBool SensorBayerOV5630_HardReset(NvOdmImagerHandle hImager);

static NvF32 Gain_H2F(NvU32 Gain)
{
    return (NvF32)((1.0 + (NvF32)((Gain >> 4) & 0x7)) *
                   (1.0 + (NvF32)(Gain & 0xF) / 16.0));
}
static NvU32 Gain_F2H(NvF32 Gain)
{
    NvU32  GainH;
    
    if(Gain >= 4.f )
    {
        GainH = 0x30;
        Gain /= 4.f;
    }else if( Gain >= 2.f )
    {
        GainH = 0x10;
        Gain /= 2.f;
    }else
        GainH = 0;
        
    GainH |= (NvU32)((Gain - 1.0) * 16.f);
    return GainH;
}
/**
 * SensorBayerOV5630_WriteExposure. Phase 2. Sensor Dependent.
 * Calculate and write the values of the sensor registers for the new
 * exposure time.
 * Without this, calibration will not be able to capture images
 * at various brightness levels, and the final product won't be able
 * to compensate for bright or dark scenes. 
 */
static NvBool
SensorBayerOV5630_WriteExposure(
    SensorBayerOV5630Context *pContext,
    const NvF32 *pNewExposureTime)
{
    NvF32 Freq = (NvF32)pContext->VtPixClkFreqHz;
    NvF32 LineLength = (NvF32)pContext->LineLength;
    NvF32 ExpTime = *pNewExposureTime;
    NvU32 NewCoarseTime = 0;
    NvU32 NewFrameLength = 0;
    NvU16 RegVAEC, ValLAEC;
    NvOdmImagerI2cConnection *pI2c = &pContext->I2c;
 
    NV_DEBUG_PRINTF(("input exposure = %f    (%f,  %f)\n",
        *pNewExposureTime, pContext->MinExposure, pContext->MaxExposure));

    if (*pNewExposureTime > pContext->MaxExposure)
        ExpTime = pContext->MaxExposure;
    else if(*pNewExposureTime < pContext->MinExposure)
        ExpTime = pContext->MinExposure;

    NvOdmImagerI2cRead(pI2c, 0x3015, &RegVAEC);
    RegVAEC &= ~0x70;
    // Here, we have to update the registers in order to set the desired
    // exposure time.
    // For this sensor, Coarse time <= FrameLength - 1 so we may have to
    // calculate a new FrameLength.
    NewCoarseTime = (NvU32)((Freq * ExpTime) / LineLength);
    NewFrameLength = NewCoarseTime + 4;

    // Consider requested max frame rate,
    if (pContext->RequestedMaxFrameRate > 0.0f)
    {
        NvU32 RequestedMinFrameLengthLines = 0;
        RequestedMinFrameLengthLines =
           (NvU32)(Freq / (LineLength * pContext->RequestedMaxFrameRate));

        if (NewFrameLength < RequestedMinFrameLengthLines)
            NewFrameLength = RequestedMinFrameLengthLines;
    }

    if (NewFrameLength > pContext->MaxFrameLength)
        NewFrameLength = pContext->MaxFrameLength;
    else if (NewFrameLength < pContext->MinFrameLength)
        NewFrameLength = pContext->MinFrameLength;
    
    if(NewCoarseTime == 0) //LAEC
    {
        ValLAEC = (NvU32)(Freq * ExpTime);
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x3002, 0);
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x3003, 0);
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x3004,
                                     (ValLAEC >> 8) & 0xFF);
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x3005, ValLAEC & 0xFF);
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x3015, RegVAEC);
        pContext->Exposure = ExpTime;
        pContext->CoarseTime = NewCoarseTime;
    }
    else
    {
        if (NewFrameLength != pContext->FrameLength)
        {
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,
                                         SENSOR_OV5630_REGISTER_FRAME_LENGTH_LINES_HIGH,
                                         VALUE_HIGH(NewFrameLength));
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,
                                         SENSOR_OV5630_REGISTER_FRAME_LENGTH_LINES_LOW,
                                         VALUE_LOW(NewFrameLength));
            pContext->FrameLength = NewFrameLength;
            pContext->FrameRate = (NvF32)pContext->VtPixClkFreqHz /
                                  (NvF32)(pContext->FrameLength *
                                          pContext->LineLength);
        }
        if (NewCoarseTime >= (pContext->FrameLength - 4))
        {
            NewCoarseTime = pContext->FrameLength - 4;
        }
    
        // Current CoarseTime needs to be updated?
        if (pContext->CoarseTime != NewCoarseTime)
        {
            // Calculate the new exposure based on the sensor and sensor
            // settings.
            pContext->Exposure = ((NvF32)NewCoarseTime * LineLength) / Freq;
            pContext->CoarseTime = NewCoarseTime;

            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x3002,
                                         VALUE_HIGH(NewCoarseTime));
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x3003,
                                         VALUE_LOW(NewCoarseTime));
        }
    }

    NV_DEBUG_PRINTF((
        "Freq = %f FrameLen = %d, NewCoarseTime = %d, LineLength = %d\n",
        Freq, pContext->FrameLength, NewCoarseTime, pContext->LineLength));
    NV_DEBUG_PRINTF(("New exposure = %f new frame rate = %f\n",
        pContext->Exposure, pContext->FrameRate));

    return NV_TRUE;
}

/**
 * SensorBayerOV5630_SetInputClock. Phase 2. Sensor Dependent.
 * Setting the input clock and updating the variables based on the input clock
 * The frame rate and exposure calculations are directly related to the
 * clock speed, so this is how the camera driver (the controller of the clocks)
 * can inform the nvodm driver of the current clock speed.
 */
static NvBool
SensorBayerOV5630_SetInputClock(
    SensorBayerOV5630Context *pContext,
    const NvOdmImagerClockProfile *pClockProfile)
{

    if (pClockProfile->ExternalClockKHz <
        g_SensorBayerOV5630Caps.InitialSensorClockRateKHz)
        return NV_FALSE;

    pContext->SensorInputClockkHz = pClockProfile->ExternalClockKHz;
    pContext->SensorClockRatio = pClockProfile->ClockMultiplier;

    // if sensor is initialized (SetMode has been called), then we need to
    // update necessary sensor variables (based on this particular sensor)
    if (pContext->SensorInitialized)
    {
        NV_DEBUG_PRINTF(("SetInputClock : Sensor Initialized\r\n"));
        
        pContext->VtPixClkFreqHz =
           (NvU32)((pContext->SensorInputClockkHz * 1000 * pContext->PllMult) /
                   (pContext->PllPreDiv * pContext->PllVtPixDiv *
                    pContext->PllVtSysDiv));

        pContext->Exposure = (NvF32)(pContext->CoarseTime *
                                     pContext->LineLength) /
                             (NvF32)pContext->VtPixClkFreqHz;
        pContext->MaxExposure =
            (NvF32)(SENSOR_OV5630_DEFAULT_MAX_FRAME_LENGTH *
                    pContext->LineLength) / (NvF32)pContext->VtPixClkFreqHz;
        pContext->MinExposure =
            (NvF32)(SENSOR_OV5630_DEFAULT_EXPOSURE_FINE) /
            (NvF32)pContext->VtPixClkFreqHz;
    }

    NV_DEBUG_PRINTF(("SensorBayerOV5630_SetInputClock : Freq=%d\r\n",
                     pContext->SensorInputClockkHz));
    return NV_TRUE;
}


/** 
 * SensorBayerOV5630_WriteGains. Phase 2. Sensor Dependent.
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
SensorBayerOV5630_WriteGains(
    SensorBayerOV5630Context *pContext,
    const NvF32 *pGains)
{
    NvU32 i = 0;
    NvU16 NewGains[4] = {0};

    for(i = 0; i < 4; i++)
    {
        if (pGains[i] > pContext->MaxGain)
            return NV_FALSE;

        // convert float to gain's unit
        NewGains[i] = (NvU16)(Gain_F2H(pGains[i]));
    }

    NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,
        SENSOR_OV5630_REGISTER_ANALOG_GAIN, NewGains[1]);

    NvOdmOsMemcpy(pContext->Gains, pGains, sizeof(NvF32) * 4);

    NV_DEBUG_PRINTF(("new gains = %f, %f, %f, %f\n", pGains[0],
        pGains[1], pGains[2], pGains[3]));

    return NV_TRUE;
}

/**
 * SensorBayerOV5630_Open. Phase 1.
 * Initialize sensor bayer and its private context
 */
NvBool SensorBayerOV5630_Open(NvOdmImagerHandle hImager)
{
    SensorBayerOV5630Context *pSensorBayerOV5630Context = NULL;
    NvOdmImagerI2cConnection *pI2c = NULL;

    if (!hImager || !hImager->pSensor)
        return NV_FALSE;

    pSensorBayerOV5630Context =
        (SensorBayerOV5630Context*)NvOdmOsAlloc(sizeof(SensorBayerOV5630Context));
    if (!pSensorBayerOV5630Context)
        goto fail;

    NvOdmOsMemset(pSensorBayerOV5630Context, 0, sizeof(SensorBayerOV5630Context));

    // Setup the i2c bit widths so we can call the
    // generalized write/read utility functions.
    // This is done here, since it will vary from sensor to sensor.
    pI2c = &pSensorBayerOV5630Context->I2c;
    pI2c->AddressBitWidth = 16;
    pI2c->DataBitWidth = 8;

    if (hImager->pSensor->GUID == SENSOR_BYRST_OV5630_GUID) // Stereo board
    {
        g_SensorBayerOV5630SetModeSequenceList = g_SensorBayerSetModeSequenceListCSISingle;
    pSensorBayerOV5630Context->NumModes =
            NV_ARRAY_SIZE(g_SensorBayerSetModeSequenceListCSISingle);
    }
    else // E911 Board
    {
        g_SensorBayerOV5630SetModeSequenceList = g_SensorBayerSetModeSequenceListCSIDual;
        pSensorBayerOV5630Context->NumModes =
            NV_ARRAY_SIZE(g_SensorBayerSetModeSequenceListCSIDual);
    }

    pSensorBayerOV5630Context->ModeIndex =
        pSensorBayerOV5630Context->NumModes; // invalid mode
    
    /** 
     * Phase 2. Initialize exposure and gains.
     */
    pSensorBayerOV5630Context->Exposure = -1.0; // invalid exposure

     // convert gain's unit to float
    pSensorBayerOV5630Context->MaxGain = Gain_H2F(SENSOR_OV5630_DEFAULT_MAX_GAIN);
    pSensorBayerOV5630Context->MinGain = Gain_H2F(SENSOR_OV5630_DEFAULT_MIN_GAIN);
    /**
     * Phase 2 ends here.
     */

    hImager->pSensor->pPrivateContext = pSensorBayerOV5630Context;

    return NV_TRUE;    

fail:
    NvOdmOsFree(pSensorBayerOV5630Context);
    return NV_FALSE;
}


/**
 * SensorBayerOV5630_Close. Phase 1.
 * Free the sensor's private context
 */
void SensorBayerOV5630_Close(NvOdmImagerHandle hImager)
{
    SensorBayerOV5630Context *pContext = NULL;
        
    if (!hImager || !hImager->pSensor || !hImager->pSensor->pPrivateContext)
        return;

    pContext = (SensorBayerOV5630Context*)hImager->pSensor->pPrivateContext;

    // cleanup
    NvOdmOsFree(pContext);
    hImager->pSensor->pPrivateContext = NULL;
}

/**
 * SensorBayerOV5630_GetCapabilities. Phase 1.
 * Returnt sensor bayer's capabilities
 */
static void
SensorBayerOV5630_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities)
{
    // Copy the sensor caps from g_SensorBayerOV5630Caps
    NvOdmOsMemcpy(pCapabilities, &g_SensorBayerOV5630Caps,
        sizeof(NvOdmImagerCapabilities));
}

/**
 * SensorBayerOV5630_ListModes. Phase 1.
 * Return a list of modes that sensor bayer supports.
 * If called with a NULL ptr to pModes, then it just returns the count
 */
static void
SensorBayerOV5630_ListModes(
    NvOdmImagerHandle hImager,
    NvOdmImagerSensorMode *pModes,
    NvS32 *pNumberOfModes)
{
    SensorBayerOV5630Context *pContext =
        (SensorBayerOV5630Context*)hImager->pSensor->pPrivateContext;
    NvS32 i;

    if (pNumberOfModes)
    {
        *pNumberOfModes = (NvS32)pContext->NumModes;
        if (pModes)
        {
            // Copy the modes from g_SensorBayerOV5630SetModeSequenceList
            for (i = 0; i < *pNumberOfModes; i++)
                pModes[i] = g_SensorBayerOV5630SetModeSequenceList[i].Mode;

            if (pContext->StereoCameraMode == StereoCameraMode_Stereo)
                pModes[i].PixelAspectRatio /= 2.0;
        }
    }
}

/**
 * SensorBayerOV5630_SetMode. Phase 1.
 * Set sensor bayer to the mode of the desired resolution and frame rate.
 */
static NvBool
SensorBayerOV5630_SetMode(
    NvOdmImagerHandle hImager,
    const SetModeParameters *pParameters,
    NvOdmImagerSensorMode *pSelectedMode,
    SetModeParameters *pResult)
{
    NvBool Status = NV_FALSE;
    SensorBayerOV5630Context *pContext =
        (SensorBayerOV5630Context*)hImager->pSensor->pPrivateContext;
    NvU32 Index;
    ModeDependentSettings *pModeSettings;
    NvBool UseFastMode = NV_FALSE;


    NV_DEBUG_PRINTF(("\nSetting resolution to %dx%d\n",
        pParameters->Resolution.width, pParameters->Resolution.height));


    Status = SensorBayerOV5630_ChooseModeIndex(pContext,
                pParameters->Resolution, &Index);

    // No match found
    if (!Status)
        return NV_FALSE;

    if (pSelectedMode)
    {
        *pSelectedMode = g_SensorBayerOV5630SetModeSequenceList[Index].Mode;
    }

    if (pContext->ModeIndex == Index)
        return NV_TRUE;

    pModeSettings =
        (ModeDependentSettings*)g_SensorBayerOV5630SetModeSequenceList[Index].
                                pModeDependentSettings;

    UseFastMode = pContext->FastSetMode && pContext->SensorInitialized &&
                  pModeSettings->SupportsFastMode;

    // i2c writes for the set mode sequence
    Status = WriteI2CSequence(&pContext->I2c,
                 g_SensorBayerOV5630SetModeSequenceList[Index].pSequence, 
                 UseFastMode);
    if (!Status)
        return NV_FALSE;
    
    if (pContext->I2cR.hOdmI2c)
    {
        Status = WriteI2CSequence(&pContext->I2cR,
                 g_SensorBayerOV5630SetModeSequenceList[Index].pSequence,
                 UseFastMode);

        if (!Status)
            return NV_FALSE;
    }
    /**
     * the following is Phase 2.
     */
    pContext->SensorInitialized = NV_TRUE;

    // Update sensor context after set mode
    NV_ASSERT(pContext->SensorInputClockkHz > 0);

    if (hImager->pSensor->GUID == SENSOR_BYRST_OV5630_GUID) // Stereo board
    {
        pContext->PllMult = 12;
        pContext->PllPreDiv = 1.5;
        pContext->PllVtPixDiv =  1;
    }
    else
    {
        // These hardcoded numbers are from the set mode sequence and this formula
        // is based on this sensor. Sensor Dependent.
        pContext->PllMult = SENSOR_OV5630_DEFAULT_PLL_MULT;
        pContext->PllPreDiv = SENSOR_OV5630_DEFAULT_PLL_PRE_DIV;
        pContext->PllVtPixDiv = SENSOR_OV5630_DEFAULT_PLL_VT_PIX_DIV;
    }

    pContext->PllVtSysDiv = SENSOR_OV5630_DEFAULT_PLL_VT_SYS_DIV;
    pContext->VtPixClkFreqHz =
        (NvU32)((pContext->SensorInputClockkHz * 1000 * pContext->PllMult) /
                (pContext->PllPreDiv * pContext->PllVtPixDiv *
                 pContext->PllVtSysDiv));

    pContext->LineLength = pModeSettings->LineLength;
    pContext->FrameLength = pModeSettings->FrameLength;
    pContext->CoarseTime = pModeSettings->CoarseTime;
    pContext->MaxFrameLength = SENSOR_OV5630_DEFAULT_MAX_FRAME_LENGTH;
    pContext->MinFrameLength = pModeSettings->MinFrameLength;

    pContext->Exposure = (NvF32)(pContext->CoarseTime * pContext->LineLength) /
                         (NvF32)pContext->VtPixClkFreqHz;

    pContext->MaxExposure = (NvF32)((SENSOR_OV5630_DEFAULT_MAX_FRAME_LENGTH - 4) *
                                    pContext->LineLength ) /
                            (NvF32)pContext->VtPixClkFreqHz;
    pContext->MinExposure = (NvF32)(SENSOR_OV5630_DEFAULT_EXPOSURE_FINE) /
                            (NvF32)pContext->VtPixClkFreqHz;

    pContext->FrameRate = (NvF32)pContext->VtPixClkFreqHz /
                          (NvF32)(pContext->FrameLength *
                                  pContext->LineLength);
    pContext->MaxFrameRate = pContext->FrameRate;
    pContext->MinFrameRate = (NvF32)pContext->VtPixClkFreqHz /
                             (NvF32)(SENSOR_OV5630_DEFAULT_MAX_FRAME_LENGTH *
                                     pContext->LineLength);

    pContext->Gains[0] = Gain_H2F(SENSOR_OV5630_DEFAULT_GAIN_R);
    pContext->Gains[1] = Gain_H2F(SENSOR_OV5630_DEFAULT_GAIN_GR);
    pContext->Gains[2] = Gain_H2F(SENSOR_OV5630_DEFAULT_GAIN_GB);
    pContext->Gains[3] = Gain_H2F(SENSOR_OV5630_DEFAULT_GAIN_B);

    if (pParameters->Exposure != 0.0)
    {
        Status = SensorBayerOV5630_WriteExposure(pContext, &pParameters->Exposure);
        if (!Status)
        {
            NV_DEBUG_PRINTF(("SensorBayerOV5630_WriteExposure failed\n"));    
        }
    }

    if (pParameters->Gains[0] != 0.0 && pParameters->Gains[1] != 0.0 &&
        pParameters->Gains[2] != 0.0 && pParameters->Gains[3] != 0.0)
    {
        Status = SensorBayerOV5630_WriteGains(pContext, pParameters->Gains);
        if (!Status)
        {
            NV_DEBUG_PRINTF(("SensorBayerOV5630_WriteGains failed\n"));
        }
    }


    //Wait 2 frames for gains/exposure to take effect.
    NvOsSleepMS((NvU32)(2000.0 / (NvF32)(pContext->FrameRate)));

    //NvOsDebugPrintf("Wait %d milliseconds\n", (NvU32)(2000.0 / (NvF32)(pContext->FrameRate)));


    NV_DEBUG_PRINTF(("-------------SetMode---------------\r\n"));
    NV_DEBUG_PRINTF(("Exposure : %f (%f, %f)\r\n", 
                 pContext->Exposure,
                 pContext->MinExposure,
                 pContext->MaxExposure));
    NV_DEBUG_PRINTF(("Gain : %f (%f, %f)\r\n", 
                 pContext->Gains[1],
                 pContext->MinGain,
                 pContext->MaxGain));
    NV_DEBUG_PRINTF(("FrameRate : %f (%f, %f)\r\n", 
                pContext->FrameRate,
                pContext->MinFrameRate,
                pContext->MaxFrameRate));


    if (pResult)
    {
        pResult->Resolution = g_SensorBayerOV5630SetModeSequenceList[Index].
                              Mode.ActiveDimensions;
        pResult->Exposure = pContext->Exposure;
        NvOdmOsMemcpy(pResult->Gains, &pContext->Gains, sizeof(NvF32) * 4);
    }
    /**
     * Phase 2 ends here.
     */
    
    pContext->ModeIndex = Index;

    // program the test mode registers if test mode is enabled
    if (pContext->TestPatternMode)
    {
        NvF32 Gains[4];
        NvU32 i;

        // reset gains
        for (i = 0; i < 4; i++)
            Gains[i] = pContext->MinGain;

        Status = SensorBayerOV5630_WriteGains(pContext, Gains);
        if (!Status)
            return NV_FALSE;

         // Enable the test mode register
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x3508, 0x80);
        // Enable color bar and select the standard color bar
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x3301, 0xF2);
        NvOdmOsWaitUS(350 * 1000);
    }
           
    if (hImager->pSensor->GUID == SENSOR_BYRST_OV5630_GUID) // Sync two sensors
    {
        NvOdmGpioSetState(pContext->GpioConfig.hGpio,
                          pContext->GpioConfig.hPin[pContext->GpioConfig.Gpios[NvOdmImagerGpio_Powerdown].HandleIndex],
                          NvOdmGpioPinActiveState_High);
        NvOdmOsWaitUS(1000);
        NvOdmGpioSetState(pContext->GpioConfig.hGpio,
                          pContext->GpioConfig.hPin[pContext->GpioConfig.Gpios[NvOdmImagerGpio_Powerdown].HandleIndex],
                          NvOdmGpioPinActiveState_Low);
        NvOdmOsWaitUS(1000);
    }

    return Status;
}


/**
 * SensorBayerOV5630_SetPowerLevel. Phase 1
 * Set the sensor's power level
 */
static NvBool
SensorBayerOV5630_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel)
{
    NvBool Status = NV_TRUE;
    SensorBayerOV5630Context *pContext =
        (SensorBayerOV5630Context*)hImager->pSensor->pPrivateContext;
    const NvOdmPeripheralConnectivity *pConnections;

    if (pContext->PowerLevel == PowerLevel)
        return NV_TRUE;

    NV_ASSERT(hImager->pSensor->pConnections);
    pConnections = hImager->pSensor->pConnections;

    switch(PowerLevel)
    {
        case NvOdmImagerPowerLevel_On:
            Status = SetPowerLevelWithPeripheralConnectivityHelper(pConnections,
                         &pContext->I2c, &pContext->I2cR, &pContext->GpioConfig,
                         NvOdmImagerPowerLevel_On);
            if (!Status)
                return NV_FALSE;

            break;

        case NvOdmImagerPowerLevel_Standby:

            Status = SetPowerLevelWithPeripheralConnectivityHelper(pConnections,
                         &pContext->I2c, &pContext->I2cR, &pContext->GpioConfig,
                         NvOdmImagerPowerLevel_Standby);

            break;
        case NvOdmImagerPowerLevel_Off:

             // Set the sensor to standby mode
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x0100, 0);

            Status = SetPowerLevelWithPeripheralConnectivityHelper(pConnections,
                         &pContext->I2c, &pContext->I2cR, &pContext->GpioConfig,
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
 * SensorBayerOV5630_SetParameter. Phase 2.
 * Set sensor specific parameters. 
 * For Phase 1. This can return NV_TRUE.
 */
static NvBool
SensorBayerOV5630_SetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    const void *pValue)
{
    NvBool Status = NV_TRUE;
    SensorBayerOV5630Context *pContext =
        (SensorBayerOV5630Context*)hImager->pSensor->pPrivateContext;

    switch(Param)
    {
        // Phase 2.
        case NvOdmImagerParameter_StereoCameraMode:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerStereoCameraMode));
            if (hImager->pSensor->GUID == SENSOR_BAYER_OV5630_GUID)
            {
                pContext->StereoCameraMode = *((NvOdmImagerStereoCameraMode*)pValue);
            }
            else
            {
                Status = NV_FALSE;
            }
            break;
        // Phase 2.
        case NvOdmImagerParameter_SensorExposure:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvF32));
            
            Status = SensorBayerOV5630_WriteExposure(pContext, (NvF32*)pValue);
            break;

        // Phase 2.
        case NvOdmImagerParameter_SensorGain:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, 4 * sizeof(NvF32));

            Status = SensorBayerOV5630_WriteGains(pContext, pValue);
            break;

        // Phase 2.
        case NvOdmImagerParameter_SensorInputClock:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                                             sizeof(NvOdmImagerClockProfile));
            
            Status = SensorBayerOV5630_SetInputClock(
                      pContext,
                      ((NvOdmImagerClockProfile*)pValue));
            break;

        // Phase 3.
        case NvOdmImagerParameter_OptimizeResolutionChange:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvBool));

            // Sensor bayer doesn't support optimized resolution change.
            pContext->FastSetMode = *((NvBool*)pValue);
            return NV_TRUE;

        // This is optional but nice to have.
        case NvOdmImagerParameter_SelfTest:
            //Not Implemented.
            break;
        // Phase 3.

        case NvOdmImagerParameter_MaxSensorFrameRate:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvF32));

            pContext->RequestedMaxFrameRate = *((NvF32*)pValue);
            break;
            
        case NvOdmImagerParameter_TestMode:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvBool));
            
            pContext->TestPatternMode = *((NvBool *)pValue);
            
            // If Sensor is initialized then only program the test mode registers
            // else just save the test mode in pContext->TestPatternMode and once
            // the sensor gets initialized in SensorBayerOV5630_SetMode() there itself 
            // program the test mode registers.
            if(pContext->SensorInitialized)
            {
                if (pContext->TestPatternMode)
                {
                    NvF32 Gains[4];
                    NvU32 i;

                    // reset gains
                    for (i = 0; i < 4; i++)
                        Gains[i] = pContext->MinGain;

                    Status = SensorBayerOV5630_WriteGains(pContext, Gains);
                    if (!Status)
                        return NV_FALSE;

                     // Enable the test mode register
                    NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x3508, 0x80);
                    // Enable color bar and select the standard color bar
                    NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x3301, 0xF2);
                                    
                }
                else
                {
                    // Disable the test mode 
                    NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x3508, 0x00);
                    // Disable the color bar
                    NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x3301, 0xC0);
                }
                NvOdmOsWaitUS(350 * 1000);
            }
            break;

        case NvOdmImagerParameter_Reset:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerReset));
            switch(*(NvOdmImagerReset*)pValue)
            {
                case NvOdmImagerReset_Hard:
                    Status = SensorBayerOV5630_HardReset(hImager);
                    break;

                case NvOdmImagerReset_Soft:
                default:
                    NV_ASSERT(!"Not Implemented!");
                    Status = NV_FALSE;
            }
            break;
         case NvOdmImagerParameter_CustomizedBlockInfo:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerCustomInfo));

            if (pValue)
            {
                NvOdmImagerCustomInfo *pCustom =
                    (NvOdmImagerCustomInfo *)pValue;
                if (pCustom->pCustomData && pCustom->SizeOfData)
                {
                    NvOsDebugPrintf("nvodm_imager: (%d) %s\n",
                                       pCustom->SizeOfData,
                                       pCustom->pCustomData);
                    // one could parse and interpret the data packet here
                }
                else
                {
                    NvOsDebugPrintf("nvodm_imager: null packet\n");
                }
            }
            else
            {
                NvOsDebugPrintf("nvodm_imager: null pointer\n");
            }
            break;

        default:
            NV_DEBUG_PRINTF(("Setparameter : %x\r\n", Param));
            NV_ASSERT(!"Not Implemented");
            break;
    }

    return Status;
}

/**
 * SensorBayerOV5630_GetParameter. Phase 1.
 * Get sensor specific parameters
 */
static NvBool
SensorBayerOV5630_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue)
{
    NvBool Status = NV_TRUE;
    NvF32 *pFloatValue = pValue;
    SensorBayerOV5630Context *pContext =
        (SensorBayerOV5630Context*)hImager->pSensor->pPrivateContext;

    switch(Param)
    {
        // Phase 1.
        case NvOdmImagerParameter_StereoCapable:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvBool));
            {
                NvBool *pBL = (NvBool*)pValue;
                *pBL = ((hImager->pSensor->GUID == SENSOR_BAYER_OV5630_GUID) ? NV_TRUE : NV_FALSE);
            }
            break;

        case NvOdmImagerParameter_StereoCameraMode:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerStereoCameraMode));
            if (hImager->pSensor->GUID == SENSOR_BAYER_OV5630_GUID)
            {
                NvOdmImagerStereoCameraMode *pMode = (NvOdmImagerStereoCameraMode*)pValue;
                *pMode = pContext->StereoCameraMode;
            }
            else
            {
                Status = NV_FALSE;
            }
            break;

        case NvOdmImagerParameter_CalibrationData:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerCalibrationData));
            {
                NvOdmImagerCalibrationData *pCalibration =
                        (NvOdmImagerCalibrationData*)pValue;
                pCalibration->CalibrationData = pSensorCalibrationData;
                pCalibration->NeedsFreeing = NV_FALSE;
            }
            break;

        // Phase 1, it can return min = max = 1.0f
        // Phase 2, return the real numbers.
        case NvOdmImagerParameter_SensorExposureLimits:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, 2 * sizeof(NvF32));
            CHECK_SENSOR_RETURN_NOT_INITIALIZED(pContext);

            pFloatValue[0] = pContext->MinExposure;
            pFloatValue[1] = pContext->MaxExposure;
            break;

        // Phase 1, it can return min = max = 1.0f
        // Phase 2, return the real numbers.
        case NvOdmImagerParameter_SensorGainLimits:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, 2 * sizeof(NvF32));
            CHECK_SENSOR_RETURN_NOT_INITIALIZED(pContext);

            pFloatValue[0] = pContext->MinGain;
            pFloatValue[1] = pContext->MaxGain;

            break;

        // Get the sensor status. This is optional but nice to have.
        case NvOdmImagerParameter_DeviceStatus:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerDeviceStatus));
            CHECK_SENSOR_RETURN_NOT_INITIALIZED(pContext);

            {
                NvOdmImagerDeviceStatus *pStatus;
                pStatus = (NvOdmImagerDeviceStatus *)pValue;
                {
                    NvOdmImagerI2cConnection *pI2c = &pContext->I2c;
                    // Pick your own useful registers to use for debug
                    // If the camera hangs, these register values are printed
                    // Sensor Dependent.
                    NvOdmImagerI2cRead(pI2c, 0x300a, &pStatus->Values[0]);
                    NvOdmImagerI2cRead(pI2c, 0x300B, &pStatus->Values[1]);
                    pStatus->Count = 2;
                }
            }
            break;

        // Phase 1, it can return min = max = 10.0f
        //          (the fps in g_SensorBayerOV5630SetModeSequenceList)
        // Phase 2, return the real numbers
        case NvOdmImagerParameter_SensorFrameRateLimits:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                2 * sizeof(NvF32));

            pFloatValue[0] = pContext->MinFrameRate;
            pFloatValue[1] = pContext->MaxFrameRate;

            break;

        // Phase 1, it can return 1.0f
        // Phase 2, return the real numbers.
        case NvOdmImagerParameter_SensorFrameRate:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvF32));
            CHECK_SENSOR_RETURN_NOT_INITIALIZED(pContext);

            pFloatValue[0] = pContext->FrameRate;
            break;

        case NvOdmImagerParameter_CalibrationOverrides:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerCalibrationData));
            {
                static const char *pFiles[] =
                {
                    "camera_overrides.isp",
                };
                NvOdmImagerCalibrationData *pCalibration =
                    (NvOdmImagerCalibrationData*)pValue;
                NvOdmOsStatType Stat;
                NvOdmOsFileHandle hFile = NULL;
                // use a tempString so we can properly respect const-ness
                char *pTempString;
                NvU32 i;

                for(i = 0; i < NV_ARRAY_SIZE(pFiles); i++)
                {
                    if(NvOdmOsStat(pFiles[i], &Stat) && Stat.size > 0)
                        break;
                }

                if(i < NV_ARRAY_SIZE(pFiles))
                {
                    pTempString = NvOdmOsAlloc((size_t)Stat.size + 1);
                    if(pTempString == NULL)
                    {
                        NV_ASSERT(pTempString != NULL &&
                            "Couldn't alloc memory to read config file");
                        Status = NV_FALSE;
                        break;
                    }
                    if(!NvOdmOsFopen(pFiles[i], NVODMOS_OPEN_READ, &hFile))
                    {
                        NV_ASSERT(!"Failed to open a file that fstatted just fine");
                        Status = NV_FALSE;
                        NvOdmOsFree(pTempString);
                        break;
                    }
                    NvOdmOsFread(hFile, pTempString, (size_t)Stat.size, NULL);
                    pTempString[Stat.size] = '\0';
                    NvOdmOsFclose(hFile);
                    pCalibration->CalibrationData = pTempString;
                    pCalibration->NeedsFreeing = NV_TRUE;
                }
                else
                {
                    pCalibration->CalibrationData = NULL;
                    pCalibration->NeedsFreeing = NV_FALSE;
                }
            }
            break;

        case NvOdmImagerParameter_MaxSensorFrameRate:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvF32));
            CHECK_SENSOR_RETURN_NOT_INITIALIZED(pContext);

            pFloatValue[0] = pContext->RequestedMaxFrameRate;
            break;
            
        case NvOdmImagerParameter_ExpectedValues:
            {
                NvOdmImagerExpectedValuesPtr *pValuesPtr;
                pValuesPtr = (NvOdmImagerExpectedValuesPtr *)pValue;
                *pValuesPtr = & (SENSOR_EXPECTED_VALUES);
            }
            break;

        case NvOdmImagerParameter_SensorGain:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, 4 * sizeof(NvF32));
            CHECK_SENSOR_RETURN_NOT_INITIALIZED(pContext);

            NvOdmOsMemcpy(pValue, pContext->Gains, sizeof(NvF32) * 4);
            break;

        case NvOdmImagerParameter_SensorExposure:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvF32));
            CHECK_SENSOR_RETURN_NOT_INITIALIZED(pContext);

            NvOdmOsMemcpy(pValue, &pContext->Exposure, sizeof(NvF32));
            break;

        case NvOdmImagerParameter_SensorFrameRateLimitsAtResolution:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerFrameRateLimitAtResolution));
            CHECK_SENSOR_RETURN_NOT_INITIALIZED(pContext);

            {
                NvOdmImagerFrameRateLimitAtResolution *pData;
                NvU32 Index;
                NvBool MatchFound = NV_FALSE;
                ModeDependentSettings *pModeSettings = NULL;

                pData = (NvOdmImagerFrameRateLimitAtResolution *)pValue;
                pData->MinFrameRate = 0.0f;
                pData->MaxFrameRate = 0.0f;

                MatchFound =
                    SensorBayerOV5630_ChooseModeIndex(pContext, pData->Resolution,
                    &Index);

                if (!MatchFound)
                {
                    Status = NV_FALSE;
                    break;
                }

                pModeSettings = (ModeDependentSettings*)
                    g_SensorBayerOV5630SetModeSequenceList[Index].
                    pModeDependentSettings;

                pData->MaxFrameRate = (NvF32)pContext->VtPixClkFreqHz /
                                      (NvF32)(pModeSettings->FrameLength *
                                              pModeSettings->LineLength);
                pData->MinFrameRate =
                    (NvF32)pContext->VtPixClkFreqHz /
                    (NvF32)(SENSOR_OV5630_DEFAULT_MAX_FRAME_LENGTH *
                            pModeSettings->LineLength);

            }
            break;

        case NvOdmImagerParameter_FocalLength:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvF32));
            pFloatValue[0] = LENS_FOCAL_LENGTH;
            break;

        case NvOdmImagerParameter_HorizontalViewAngle:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvF32));
            pFloatValue[0] = LENS_HORIZONTAL_VIEW_ANGLE;
            break;

        case NvOdmImagerParameter_VerticalViewAngle:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvF32));
            pFloatValue[0] = LENS_VERTICAL_VIEW_ANGLE;
            break;

        // Phase 3.
        default:
            NV_ASSERT("!Not Implemented");
            Status = NV_FALSE;
    }
 
    return Status;
}

static NV_INLINE NvBool
SensorBayerOV5630_ChooseModeIndex(
    SensorBayerOV5630Context *pContext,
    NvSize Resolution,
    NvU32 *pIndex)
{
    NvU32 Index;

    for (Index = 0; Index < pContext->NumModes; Index++)
    {
        if ((Resolution.width ==
             g_SensorBayerOV5630SetModeSequenceList[Index].Mode.
             ActiveDimensions.width) &&
            (Resolution.height ==
             g_SensorBayerOV5630SetModeSequenceList[Index].Mode.
             ActiveDimensions.height))
        {
            *pIndex = Index;
            return NV_TRUE;
        }
    }

    return NV_FALSE;
}

static NvBool SensorBayerOV5630_HardReset(NvOdmImagerHandle hImager)
{
    SensorBayerOV5630Context *pContext =
        (SensorBayerOV5630Context*)hImager->pSensor->pPrivateContext;
    SetModeParameters ModeParameters;
    NvBool Status = NV_TRUE;
    NvU32 ResetPinIndex = 0;
    ImagerGpioConfig *pGpioConfig = &pContext->GpioConfig;

    // If the sensor didn't even survive initialization, reset won't help
    if (!pContext->SensorInitialized)
        return NV_FALSE;

    // 1. Disable streaming
    NV_DEBUG_PRINTF(("Disable streaming\n"));
    NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,
        SENSOR_OV5630_REGISTER_IMAGE_SYSTEM, 0x00);

    // 2. Wait for the soft standby state is reached.
    NV_DEBUG_PRINTF(("Wait for the soft standby state is reached\n"));
    NvOdmOsWaitUS(1000);

    // 3. Assert RESET_B (active LOW) to reset the sensor
    if (pGpioConfig->Gpios[NvOdmImagerGpio_Reset].Enable)
    {
        NvOdmGpioPinActiveState LevelA = NvOdmGpioPinActiveState_Low,
                                LevelB = NvOdmGpioPinActiveState_High;

        ResetPinIndex = pGpioConfig->Gpios[NvOdmImagerGpio_Reset].HandleIndex;

        if (!pGpioConfig->Gpios[NvOdmImagerGpio_Reset].ActiveHigh)
        {
            LevelA = NvOdmGpioPinActiveState_High;
            LevelB = NvOdmGpioPinActiveState_Low;
        }

        NV_DEBUG_PRINTF(("Assert RESET_B\n"));
        NvOdmGpioSetState(pGpioConfig->hGpio,
            pGpioConfig->hPin[ResetPinIndex], LevelA);
        NvOdmOsWaitUS(1000); // wait 1ms
        NvOdmGpioSetState(pGpioConfig->hGpio,
            pGpioConfig->hPin[ResetPinIndex], LevelB);
        NvOdmOsWaitUS(1000); // wait 1ms
        NvOdmGpioSetState(pGpioConfig->hGpio,
            pGpioConfig->hPin[ResetPinIndex], LevelA);
        NvOdmOsWaitUS(1000); // allow pin changes to settle
    }

    // 4. Only initialize and set sensor mode if it's already initialized
    if (pContext->SensorInitialized)
    {
        NV_DEBUG_PRINTF(("Initialize and SetMode\n"));
        NvOdmOsMemset(&ModeParameters, 0, sizeof(SetModeParameters));
        ModeParameters.Resolution =
            g_SensorBayerOV5630SetModeSequenceList[pContext->ModeIndex].
            Mode.ActiveDimensions;
        ModeParameters.Exposure = pContext->Exposure;
        NvOdmOsMemcpy(&ModeParameters.Gains, &pContext->Gains,
            sizeof(NvF32) * 4);

        pContext->SensorInitialized = NV_FALSE;
        pContext->ModeIndex = pContext->NumModes;

        Status = SensorBayerOV5630_SetMode(hImager, &ModeParameters, NULL, NULL);
    }

    NV_DEBUG_PRINTF(("Hard resetting...Done\n"));

    return Status;
}


/*
 * SensorBayerOV5630_GetPowerLevel. Phase 3.
 * Get the sensor's current power level
 */
static void
SensorBayerOV5630_GetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel *pPowerLevel)
{
    SensorBayerOV5630Context *pContext =
        (SensorBayerOV5630Context*)hImager->pSensor->pPrivateContext;

    *pPowerLevel = pContext->PowerLevel;
}

NvBool SensorBayerOV5630_GetHal(NvOdmImagerHandle hImager)
{
    if (!hImager || !hImager->pSensor)
        return NV_FALSE;

    hImager->pSensor->pfnOpen = SensorBayerOV5630_Open;
    hImager->pSensor->pfnClose = SensorBayerOV5630_Close;
    hImager->pSensor->pfnGetCapabilities = SensorBayerOV5630_GetCapabilities;
    hImager->pSensor->pfnListModes = SensorBayerOV5630_ListModes;
    hImager->pSensor->pfnSetMode = SensorBayerOV5630_SetMode;
    hImager->pSensor->pfnSetPowerLevel = SensorBayerOV5630_SetPowerLevel;
    hImager->pSensor->pfnGetPowerLevel = SensorBayerOV5630_GetPowerLevel;
    hImager->pSensor->pfnSetParameter = SensorBayerOV5630_SetParameter;
    hImager->pSensor->pfnGetParameter = SensorBayerOV5630_GetParameter;

    return NV_TRUE;
}
