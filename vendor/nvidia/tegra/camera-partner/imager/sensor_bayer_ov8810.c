/*
 * Copyright (c) 2008 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "sensor_bayer.h"
#include "focuser.h"
#include "imager_util.h"
#include "nvodm_query_gpio.h"

#include "sensor_bayer_camera_config.h"

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
#define SENSOR_BAYER_DEFAULT_MIN_EXPOSURE_FINE  (0x00CA)
#define SENSOR_BAYER_DEFAULT_EXPOSURE_COARSE    (0x03de)
#define SENSOR_BAYER_DEFAULT_MAX_GAIN           (0x003f)
#define SENSOR_BAYER_DEFAULT_MIN_GAIN           (0x0000)
#define SENSOR_BAYER_DEFAULT_GAIN_R             (SENSOR_BAYER_DEFAULT_MIN_GAIN)
#define SENSOR_BAYER_DEFAULT_GAIN_GR            (SENSOR_BAYER_DEFAULT_MIN_GAIN)
#define SENSOR_BAYER_DEFAULT_GAIN_GB            (SENSOR_BAYER_DEFAULT_MIN_GAIN)
#define SENSOR_BAYER_DEFAULT_GAIN_B             (SENSOR_BAYER_DEFAULT_MIN_GAIN)
#define SENSOR_BAYER_DEFAULT_MAX_LINE_LENGTH    (0xFFFF)
#define SENSOR_BAYER_DEFAULT_DIFF_E_F           (2)
#define SENSOR_BAYER_DEFAULT_MIN_EXPOSURE_COARSE (0x180)

#define ENABLE_OVISP 0
#define LOCAL_DEBUG 0

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
    "Omnivision 8810",  // string identifier, used for debugging
    NvOdmImagerSensorInterface_Parallel10,
    {
      //NvOdmImagerPixelType_BayerRGGB,
      NvOdmImagerPixelType_BayerBGGR,
      //NvOdmImagerPixelType_BayerGBRG,
      //NvOdmImagerPixelType_BayerGRBG,
    },
    0, // focusing positions, ignored if FOCUSER_GUID is zero
    NvOdmImagerOrientation_180_Degrees, // rotation with respect to main display
    NvOdmImagerDirection_Away,          // toward or away the main display
    12000, // initial sensor clock (in khz)
    { {12000, 64.f/12.f}}, // preferred clock profile
    {
        NvOdmImagerSyncEdge_Rising, // hsync polarity
        NvOdmImagerSyncEdge_Rising, // vsync polarity
        NV_FALSE,                   // uses mclk on vgp0
    }, // parallel settings (VIP)
    {0},
    { 16, 16 }, // min blank time, shouldn't need adjustment
    0, // preferred mode, which resolution to use on startup
    OV8810VCM_GUID, // FOCUSER_GUID, only set if focuser code available
    0, // Flash_GUID, only set if Flash code available
    NVODM_IMAGER_CAPABILITIES_END
};

/**
 * SetMode Sequence for 3264x2448. Phase 0. Sensor Dependent.
 * This sequence should put sensor in streaming mode for 3264x2448
 * This is usually given by the FAE or the sensor vendor.
 */

// ============================================================
static DevCtrlReg16 SetModeSequence_3264x2448[] =
// ============================================================
{
    {0x3012, 0x80},
    {0x30fa, 0x00},  // disable streaming mode

    //clock setting
    {0x300f, 0x04},
    {0x300e, 0x05},
    {0x3010, 0x14}, 
    {0x3011, 0x21}, // 14MHz/6fps : 0x22
    
    {0x3000, 0x30}, // AGCL Sensor gain [7:0]
    
#if ENABLE_OVISP
    {0x3302, 0x20}, // ISP Enable Control
#else
    {0x3302, 0x00}, // ISP Enable Control
#endif
    {0x30b2, 0x00}, // Frame exposure I/O Control [bit4] 0:input, 1:output
    {0x30a0, 0x40}, // ???
    {0x3098, 0x24}, // ???
    {0x3099, 0x81}, // ???
    {0x309a, 0x64}, // ???
    {0x309b, 0x00}, // ???
    {0x309d, 0x64}, // ??? AGC_NEG_OOFSET
    {0x309e, 0x12}, // ???
    
#if ENABLE_OVISP
    {0x3320, 0x82}, // AWB
    {0x3321, 0x02}, // AWB
    {0x3322, 0x04}, // AWB
    {0x3328, 0x40}, // AWB
    {0x3329, 0x00}, // AWB
#else    
    {0x3320, 0xC2}, // AWB
    {0x3324, 0x40}, // AWB
    {0x3325, 0x40}, // AWB
    {0x3326, 0x40}, // AWB
#endif    
    {0x3306, 0x00}, // 2LSB digital gain
    {0x3316, 0x03}, //mirror 0x03}, // H pad start for (mirror)
    
    {0x30b3, 0x00}, // DSIO(Pclk divisor) - default

    {0x3068, 0x08}, // PreBLC debug ???
    {0x306b, 0x00}, // PreBLC debug ???
    {0x3065, 0x50}, // PreBLC debug ???
    {0x3067, 0x40}, // PreBLC debug ???
    {0x3069, 0x80}, // PreBLC debug ???
    {0x3071, 0x00}, // BLC 
#if ENABLE_OVISP
    {0x3300, 0xef}, // ISP Enable Control 00 
#else
    {0x3300, 0x81}, // ISP Enable Control 00 0x81???
#endif 

    {0x3091, 0x00}, // Array binning control
    
    {0x3006, 0x00}, // Hign gain
    {0x3082, 0x80}, // ???
    
    {0x331e, 0x94}, // AVG width
    {0x331f, 0x6e}, // AVG height

    {0x3092, 0x00}, // ???
    {0x3094, 0x01}, // ???
    {0x3090, 0x2b}, // ???
    {0x30ab, 0x44}, // ???
    {0x3095, 0x0a}, // ???
    {0x308d, 0x00}, // ???
    {0x3082, 0x00}, // ???
    {0x3080, 0x40}, // ???
    {0x30aa, 0x59}, // ???
    {0x30a9, 0x00}, // Enable internal 1.5v regulator
    {0x30be, 0x08}, // ???
    //{0x308a, 0x21}, // ???
    {0x309f, 0x23}, // ???
    {0x3065, 0x40}, // ???
    {0x3068, 0x00}, // ???
    {0x30bf, 0x80}, // ???
    {0x309c, 0x00}, // ???
    
    {0x3075, 0x29}, // Horizontal window initialized address
    {0x3076, 0x29}, // Horizontal window initialized address
    {0x3077, 0x29}, // Horizontal window initialized address
    {0x3078, 0x29}, // Horizontal window initialized address
    
    {0x306a, 0x05}, // ???
    
    {0x3000, 0x00}, // AGC
    
#if ENABLE_OVISP
    {0x3013, 0xcf}, // AEC/AGC control
#else
    {0x3013, 0x08}, // AEC/AGC control
#endif   
#if ENABLE_OVISP
    //{0x3014, 0x00}, // LAEC Manual control Disable
    {0x3015, 0x33}, // VAEC/AGC Ceiling
#else
    {0x3014, 0x02}, // LAEC Manual control Enable
    {0x3015, 0x02}, // VAEC/AGC Ceiling
#endif    
    {0x308d, 0x01}, // ???
    {0x3509, 0x00},
    {0x307e, 0x00}, // X Array bank for Mirror

    {SEQUENCE_FAST_SETMODE_START, 0},
    {0x30fa, 0x00}, // Disable streaming mode
    {0x3002, 0x05}, // AECL Coarse Exposure Time [high]
    {0x3003, 0xda}, // AECL Coarse Exposure Time [Low]

    {0x30f8, 0x40}, // mirror, 0x40}, // Mirror on, flip off

        //{0x3018, 0x68}, // Luminance Signal/Histogram for AEC/AGC
    //{0x3019, 0x55}, // Luminance Signal/Histogram for AEC/AGC
    {0x3018, 0x3c}, // Luminance Signal/Histogram for AEC/AGC
    {0x3019, 0x38}, // Luminance Signal/Histogram for AEC/AGC


    {0x302c, 0x0c}, // X output size
    {0x302d, 0xc0}, // X output size
    {0x302e, 0x09}, // Y output size
    {0x302f, 0x92}, // Y output size
    {0x3020, 0x09}, // Frame Length
    {0x3021, 0xb8}, // Frame Length
    {0x3022, 0x0f}, // Line Length
    {0x3023, 0x58}, // Line Length
    
    {0x3024, 0x00}, // X start
    {0x3025, 0x0A}, // X Start
    {0x3026, 0x00}, // Y Start
    {0x3027, 0x00}, // Y Start
    {0x3028, 0x0c}, // X End
    {0x3029, 0xd5}, // X End
    {0x302a, 0x09}, // Y End
    {0x302b, 0xa1}, // Y End

    {0x3334, 0x02}, // BLC
    {0x3331, 0x40}, // BLC
    {0x3332, 0x40}, // BLC
    {0x3333, 0x41}, // BLC

    {0x331c, 0x28}, // AVG x_start
    {0x331d, 0x21}, // AVG y_start

    {0x33e5, 0x00}, // VAP(H skip) - default

    {0x3079, 0x04}, // Vertical start line

#if ENABLE_OVISP
    {0x3301, 0x0b}, // ISP Enable Control 01
#else
    {0x3301, 0x00}, // ISP Enable Control 01  0x08 for AVG???
#endif   

#if 0
    // Flex timing BLC bkcenter debug
    {0x3090, 0x36}, // ???
    {0x333e, 0x00}, // BLC offset high byte
    {0x3000, 0x7f}, // AGC
    {0x3087, 0x41}, // ???
    {0x30f2, 0x00}, // ???
    {0x30f3, 0xf0}, // ??? 
    {0x30f4, 0x40}, // ??? 
    {0x308a, 0x02}, // ???
#endif    
    {0x3305, 0xa0}, // Reserved???
#if 1
    {0x308a, 0x02}, // ???
    {0x3319, 0x06}, // ???
#endif
    {0x301a, 0x82}, // Fast Mode Karge Step threshold -- AEC/AGC fast mode only

    {0x350a, 0x96},
    {0x350b, 0xf0},
    {0x3072, 0x01}, // ???
    {0x30fa, 0x01}, // enable streaming Mode
    {SEQUENCE_FAST_SETMODE_END, 0},

    {SEQUENCE_END, 0x0000}
};

// ============================================================
static DevCtrlReg16 SetModeSequence_1632x1224[] =
// ============================================================
{
    {0x3012, 0x80},
    {0x30fa, 0x00},   // disable streaming mode
    
    //clock setting
    {0x300f, 0x04},
    {0x300e, 0x05},
    {0x3010, 0x14}, 
    {0x3011, 0x21}, // 14MHz/6fps : 0x22
    
    {0x3000, 0x30}, // AGCL Sensor gain [7:0]
    
#if ENABLE_OVISP
    {0x3302, 0x20}, // ISP Enable Control
#else
    {0x3302, 0x00}, // ISP Enable Control
#endif
    {0x30b2, 0x00}, // Frame exposure I/O Control [bit4] 0:input, 1:output
    {0x30a0, 0x40}, // ???
    {0x3098, 0x24}, // ???
    {0x3099, 0x81}, // ???
    {0x309a, 0x64}, // ???
    {0x309b, 0x00}, // ???
    {0x309d, 0x64}, // ??? AGC_NEG_OOFSET
    {0x309e, 0x12}, // ???
    
    {0x3320, 0xC2}, // AWB
    {0x3324, 0x40}, // AWB
    {0x3325, 0x40}, // AWB
    {0x3326, 0x40}, // AWB

    {0x3306, 0x00}, // 2LSB digital gain
    {0x3316, 0x03}, //mirror 0x03}, // H pad start for (mirror)
    
    {0x30b3, 0x00}, // DSIO(Pclk divisor) - default
    
    {0x3068, 0x08}, // PreBLC debug ???
    {0x306b, 0x00}, // PreBLC debug ???
    {0x3065, 0x50}, // PreBLC debug ???
    {0x3067, 0x40}, // PreBLC debug ???
    {0x3069, 0x80}, // PreBLC debug ???
    {0x3071, 0x00}, // BLC 
    
#if ENABLE_OVISP
    {0x3300, 0xef}, // ISP Enable Control 00 
#else
    {0x3300, 0x81}, // ISP Enable Control 00 0x81???
#endif    
    
    {0x3091, 0x00},//0xc0}, // Array binning control
    
    {0x3006, 0x00}, // Hign gain
    {0x3082, 0x80}, // ???
    
    {0x331e, 0x94}, // AVG width
    {0x331f, 0x6e}, // AVG height
    
    {0x3092, 0x00}, // ???
    {0x3094, 0x01}, // ???
    {0x3090, 0x2b}, // ???
    {0x30ab, 0x44}, // ???
    {0x3095, 0x0a}, // ???
    {0x308d, 0x00}, // ???
    {0x3082, 0x00}, // ???
    {0x3080, 0x40}, // ???
    {0x30aa, 0x59}, // ???
    {0x30a9, 0x00}, // Enable internal 1.5v regulator
    {0x30be, 0x08}, // ???
    {0x309f, 0x23}, // ???
    {0x3065, 0x40}, // ???
    {0x3068, 0x00}, // ???
    {0x30bf, 0x80}, // ???
    {0x309c, 0x00}, // ???
    
    {0x3075, 0x29}, // Horizontal window initialized address
    {0x3076, 0x29}, // Horizontal window initialized address
    {0x3077, 0x29}, // Horizontal window initialized address
    {0x3078, 0x29}, // Horizontal window initialized address
    
    {0x306a, 0x05}, // ???
    
    {0x3000, 0x00}, // AGC
#if ENABLE_OVISP
    {0x3013, 0xcf}, // AEC/AGC control
#else
    {0x3013, 0x08}, // AEC/AGC control
#endif   
#if ENABLE_OVISP
    //{0x3014, 0x00}, // LAEC Manual control Disable
    {0x3015, 0x33}, // VAEC/AGC Ceiling
#else
    {0x3014, 0x02}, // LAEC Manual control Enable
    {0x3015, 0x02}, // VAEC/AGC Ceiling
#endif    

    {0x308d, 0x01}, // ???
    {0x3509, 0x00},
    {0x307e, 0x00}, // X Array bank for Mirror

    {SEQUENCE_FAST_SETMODE_START, 0},

    {0x30fa, 0x00}, // disable streaming mode

    {0x3002, 0x02}, // AECL Coarse Exposure Time [high]
    {0x3003, 0xed}, // AECL Coarse Exposure Time [Low]

    {0x30f8, 0x45}, // mirror, 0x45}, // Mirror on, flip off

    {0x3018, 0x68}, // Luminance Signal/Histogram for AEC/AGC
    {0x3019, 0x55}, // Luminance Signal/Histogram for AEC/AGC

    {0x302c, 0x06}, // X output size
    {0x302d, 0x60}, // X output size
    {0x302e, 0x04}, // Y output size
    {0x302f, 0xca}, // Y output size
    {0x3020, 0x04}, // Frame Length
    {0x3021, 0xf4}, // Frame Length
    {0x3022, 0x08}, // Line Length
    {0x3023, 0xc0}, // Line Length
    {0x3024, 0x00}, // X start
    {0x3025, 0x08}, // X Start
    {0x3026, 0x00}, // Y Start
    {0x3027, 0x00}, // Y Start
    {0x3028, 0x0c}, // X End
    {0x3029, 0xd7}, // X End
    {0x302a, 0x09}, // Y End
    {0x302b, 0xa1}, // Y End

    {0x3334, 0x02}, // BLC
    {0x3331, 0x60}, // BLC
    {0x3332, 0x60}, // BLC
    {0x3333, 0x41}, // BLC

    // AVG x_start
    // AVG y_start

    {0x33e5, 0x00}, // VAP(H skip) - default

    {0x3079, 0x0a}, // V reference start line (flip)
#if ENABLE_OVISP
    {0x3301, 0x0b}, // ISP Enable Control 01
#else
    {0x3301, 0x00}, // ISP Enable Control 01  0x08 for AVG???
#endif    

     // 0x3305 missing
    {0x308a, 0x21}, // ???

     // 0x3319 missing 
     // missing 0x301a

    {0x350a, 0x96},
    {0x350b, 0xf0},
    {0x3072, 0x0d}, // ???

    {0x30fa, 0x01}, // enable streaming Mode

    {SEQUENCE_FAST_SETMODE_END, 0},

    {SEQUENCE_END, 0x0000}
};

//*************************************************************************************
static DevCtrlReg16 SetModeSequence_816x612[] =
//*************************************************************************************
{
    {0x3012, 0x80},
    {0x30fa, 0x00},   // disable streaming mode
    
    //clock setting
    {0x300f, 0x04},
    {0x300e, 0x05},
    {0x3010, 0x14}, 
    {0x3011, 0x21}, // 14MHz/6fps : 0x22
    
    {0x3000, 0x30}, // AGCL Sensor gain [7:0]
    
#if ENABLE_OVISP
    {0x3302, 0x20}, // ISP Enable Control
#else
    {0x3302, 0x00}, // ISP Enable Control
#endif
    {0x30b2, 0x00}, // Frame exposure I/O Control [bit4] 0:input, 1:output
    {0x30a0, 0x40}, // ???
    {0x3098, 0x24}, // ???
    {0x3099, 0x81}, // ???
    {0x309a, 0x64}, // ???
    {0x309b, 0x00}, // ???
    {0x309d, 0x64}, // ??? AGC_NEG_OOFSET
    {0x309e, 0x12}, // ???
    
    {0x3320, 0xC2}, // AWB
    {0x3324, 0x40}, // AWB
    {0x3325, 0x40}, // AWB
    {0x3326, 0x40}, // AWB

    {0x3306, 0x00}, // 2LSB digital gain
    {0x3316, 0x03}, //mirror 0x03}, // H pad start for (mirror)
    
    {0x30b3, 0x00}, // DSIO(Pclk divisor) - default

    {0x3068, 0x08}, // PreBLC debug ???
    {0x306b, 0x00}, // PreBLC debug ???
    {0x3065, 0x50}, // PreBLC debug ???
    {0x3067, 0x40}, // PreBLC debug ???
    {0x3069, 0x80}, // PreBLC debug ???
    {0x3071, 0x00}, // BLC 

#if ENABLE_OVISP
    {0x3300, 0xef}, // ISP Enable Control 00 
#else
    {0x3300, 0x81}, // ISP Enable Control 00 0x81???
#endif    
    {0x3091, 0x00},//0xc0}, // Array binning control
    
    {0x3006, 0x00}, // Hign gain
    {0x3082, 0x80}, // ???
    
    {0x331e, 0x94}, // AVG width
    {0x331f, 0x6e}, // AVG height
    
    {0x3092, 0x00}, // ???
    {0x3094, 0x01}, // ???
    {0x3090, 0x2b}, // ???  @@@36
    {0x30ab, 0x44}, // ???
    {0x3095, 0x0a}, // ???
    {0x308d, 0x00}, // ???
    {0x3082, 0x00}, // ???
    {0x3080, 0x40}, // ???
    {0x30aa, 0x59}, // ???
    {0x30a9, 0x00}, // Enable internal 1.5v regulator
    {0x30be, 0x08}, // ???
    {0x309f, 0x23}, // ???
    {0x3065, 0x40}, // ???
    {0x3068, 0x00}, // ???
    {0x30bf, 0x80}, // ???
    {0x309c, 0x00}, // ???
    
    {0x3075, 0x29}, // Horizontal window initialized address
    {0x3076, 0x29}, // Horizontal window initialized address
    {0x3077, 0x29}, // Horizontal window initialized address
    {0x3078, 0x29}, // Horizontal window initialized address
    
    {0x306a, 0x05}, // ???
    
    {0x3000, 0x00}, // AGC
     
#if ENABLE_OVISP
    {0x3013, 0xcf}, // AEC/AGC control
#else
    {0x3013, 0x08}, // AEC/AGC control
#endif    

#if ENABLE_OVISP
    //{0x3014, 0x00}, // LAEC Manual control Disable
    {0x3015, 0x32}, // VAEC/AGC Ceiling
#else
    {0x3014, 0x02}, // LAEC Manual control Enable
    {0x3015, 0x02}, // VAEC/AGC Ceiling
#endif  
    {0x308d, 0x01}, // ???
    {0x3509, 0x00},
    {0x307e, 0x00}, // X Array bank for Mirror

    {SEQUENCE_FAST_SETMODE_START, 0},
    {0x30fa, 0x00}, // disable streaming mode 

    {0x3002, 0x01}, // AECL Coarse Exposure Time [high]
    {0x3003, 0x76}, // AECL Coarse Exposure Time [Low]

    {0x30f8, 0x4a}, // mirror, 0x4a}, // Mirror on, flip off

    {0x3018, 0x78}, // Luminance Signal/Histogram for AEC/AGC (programmed to default)
    {0x3019, 0x68}, // Luminance Signal/Histogram for AEC/AGC (programmed to default)

    {0x302c, 0x03}, // X output size
    {0x302d, 0x30}, // X output size
    {0x302e, 0x02}, // Y output size
    {0x302f, 0x66}, // Y output size
    {0x3020, 0x02}, // Frame Length
    {0x3021, 0x92}, // Frame Length
    {0x3022, 0x09}, // Line Length
    {0x3023, 0x08}, // Line Length
    {0x3024, 0x00}, // X start
    {0x3025, 0x00}, // X Start
    {0x3026, 0x00}, // Y Start
    {0x3027, 0x00}, // Y Start
    {0x3028, 0x0c}, // X End
    {0x3029, 0xef}, // X End
    {0x302a, 0x09}, // Y End
    {0x302b, 0x9f}, // Y End

    {0x3334, 0x02}, // BLC
    {0x3331, 0x60}, // BLC
    {0x3332, 0x60}, // BLC
    {0x3333, 0x41}, // BLC

    // AVG x_start
    // AVG y_start

    {0x33e5, 0x01}, // VAP(H skip) - default

    {0x3079, 0x0a}, // V reference start line (flip)

#if ENABLE_OVISP
    {0x3301, 0x0b}, // ISP Enable Control 01
#else
    {0x3301, 0x04}, // ISP Enable Control 01  0x08 for AVG???
#endif
    // missing 0x3305
    {0x308a, 0x21}, // ???
    // missing 0x3319
    {0x3319, 0x00}, // this affect vertical Y start.
    // missing 0x301a
 
    {0x350a, 0x25},
    {0x350b, 0xbc},
    {0x3072, 0x0d}, // ???

    {0x30fa, 0x01}, // enable streaming Mode

    {SEQUENCE_FAST_SETMODE_END, 0},

    {SEQUENCE_WAIT_MS, 500},
    {SEQUENCE_END, 0x0000}
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
     // WxH         x,y    fps    set mode sequence            mult   PreDiv PixDiv SysDiv  FL     LL
    {{{3264, 2448}, {0, 1}, 5, }, SetModeSequence_3264x2448, {0x0014,0x0001,0x0005,0x0001, 0x09b8,0x0f58}},
    {{{1632, 1224}, {0, 1}, 15,}, SetModeSequence_1632x1224, {0x0014,0x0001,0x0005,0x0001, 0x04f4,0x08c0}},
    {{{ 816,  612}, {0, 1}, 30,}, SetModeSequence_816x612,   {0x0014,0x0001,0x0005,0x0001, 0x0292,0x0908}},
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
    NvF32 MaxFrameRate;
    NvF32 MinFrameRate;

    // Phase 2 variables. Sensor Dependent.
    // These are used to set or get exposure, frame rate, and so on.
    NvU32 PllMult;
    NvU32 PllPreDiv;
    NvU32 PllVtPixDiv;
    NvU32 PllVtSysDiv;

    NvU32 CoarseTime;
    NvU32 VtPixClkFreqHz;

    NvU32 DefaultFrameLength;
    NvU32 FrameLength;
    NvU32 MaxFrameLength;
    NvU32 MinFrameLength;

    NvU32 DefaultLineLength;
    NvU32 LineLength;
    NvU32 MaxLineLength;
    NvU32 MinLineLength;
    NvBool FastSetMode;
}SensorBayerContext;

NvBool
ResetFocuserPosition(NvOdmImagerHandle hImager);

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

static NV_INLINE NvBool
SensorBayer_ChooseModeIndex(
    SensorBayerContext *pContext,
    NvSize Resolution,
    NvU32 *pIndex);

static NvF32 Gain_H2F(NvU32 gain)
{
    return (NvF32)((1.0+(NvF32)((gain>>4)&0x7))*(1.0+(NvF32)(gain&0xf)/16.0));
}
static NvU32 Gain_F2H(NvF32 gain)
{
    NvU32  hGain;
    
    if( gain >= 4.f )
    {
        hGain = 0x30;
        gain /= 4.f;
    }else if( gain >= 2.f )
    {
        hGain = 0x10;
        gain /= 2.f;
    }else
        hGain = 0;
        
    hGain |= (NvU32)((gain - 1.0)*16.f);
    return hGain;
}
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
    NvF32 *pNewExposureTime)
{
    NvF32 Freq = (NvF32)pContext->VtPixClkFreqHz;
    NvF32 LineLength = (NvF32)(pContext->DefaultLineLength);
    NvF32 ExpTime;
    NvU32 NewCoarseTime = 0;
    NvU32 NewFrameLength = 0;
    NvU32 NewLineLength = 0;
    NvU16 RegLAECCtrl, ValLAEC;
    NvU16 VsyncWidth;
    NvOdmImagerI2cConnection *pI2c = &pContext->I2c;

#if LOCAL_DEBUG
    NvOdmOsDebugPrintf("--WriteExposure input = %f    (%f,  %f), mode %d\n", *pNewExposureTime,pContext->MinExposure,
                        pContext->MaxExposure, pContext->ModeIndex);
#endif

    if(pContext->ModeIndex == 0)
    {
        *pNewExposureTime *= 2.0f;
    }
    ExpTime = *pNewExposureTime;
    //if (*pNewExposureTime > pContext->MaxExposure ||
    //    *pNewExposureTime < pContext->MinExposure)
    //    return NV_FALSE;
    if (*pNewExposureTime > pContext->MaxExposure )
        ExpTime = pContext->MaxExposure;
    else if(*pNewExposureTime < pContext->MinExposure)
        ExpTime = pContext->MinExposure;

    // Here, we have to update the registers in order to set the desired exposure time.
    // For this sensor, Coarse time <= FrameLength - 2 so we may have to calculate a new FrameLength/LineLength
    
    //Step 1 : Assume that LINE LENGTH doesn't need to change
    NewCoarseTime = (NvU32)((Freq * ExpTime) / (NvF32)pContext->DefaultLineLength);
    if( NewCoarseTime > (pContext->DefaultFrameLength - SENSOR_BAYER_DEFAULT_DIFF_E_F) )
    {
        // Change Line Length first
        NewCoarseTime = pContext->DefaultFrameLength - SENSOR_BAYER_DEFAULT_DIFF_E_F;
        NewLineLength = (NvU32)((Freq * ExpTime) / (NvF32)NewCoarseTime);
        if( NewLineLength > pContext->MaxLineLength )
        {
            // When reaching Max. line length, starting insert V-blank
            NewLineLength = pContext->MaxLineLength;
            NewCoarseTime = (NvU32)((Freq * ExpTime) / NewLineLength);
            NewFrameLength = NewCoarseTime + SENSOR_BAYER_DEFAULT_DIFF_E_F;
            if(NewFrameLength > pContext->MaxFrameLength)    // Pretty long exposure
            {
                NewFrameLength = pContext->MaxFrameLength;
                NewCoarseTime = NewFrameLength - SENSOR_BAYER_DEFAULT_DIFF_E_F;
            }
        }else
        {
            NewFrameLength = pContext->DefaultFrameLength;
        }
    }else
    {
        NewFrameLength = pContext->DefaultFrameLength;
        NewLineLength  = pContext->DefaultLineLength;
    }
#if LOCAL_DEBUG
    NvOdmOsDebugPrintf("newFrameLength = %d(%d)        NewLineLength = %d(%d) \r\n",  NewFrameLength, pContext->DefaultFrameLength, NewLineLength,pContext->DefaultLineLength);
#endif

    pContext->FrameLength = NewFrameLength;
    pContext->LineLength  = NewLineLength;
    pContext->FrameRate = (NvF32)pContext->VtPixClkFreqHz / (NvF32)(pContext->FrameLength * pContext->LineLength);
    pContext->CoarseTime = NewCoarseTime;

    if( NewCoarseTime == 0 ) //LAEC
    {
        ValLAEC = (NvU32)(Freq * ExpTime);
        if( ValLAEC < SENSOR_BAYER_DEFAULT_MIN_EXPOSURE_COARSE ) ValLAEC = SENSOR_BAYER_DEFAULT_MIN_EXPOSURE_COARSE;
        if( ValLAEC > (LineLength-0x3B2) ) ValLAEC = (pContext->LineLength-0x3B2);
        ExpTime = (NvF32)ValLAEC / Freq;
        
        NvOdmImagerI2cRead(pI2c, 0x3013, &RegLAECCtrl);
        RegLAECCtrl |= 0x08;
        
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x30B7,0x88); //Group Write enable
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x301e,0);    //VsyncWidth should be 0
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x301f,0);    //VsyncWidth should be 0
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x3022,(NewLineLength>>8)&0xff);    //LineLength should be default
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x3023, NewLineLength&0xff);        //LineLength should be default
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x3002,0);
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x3003,0);
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x3004,(ValLAEC >> 8) & 0xFF);
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x3005,ValLAEC & 0xFF);
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x3013,RegLAECCtrl);
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x30B7,0x80); //Group Write disable
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x30FF,0xFF); //Group Write flag
        pContext->Exposure = ExpTime;
    }
    else
    {
        NvOdmImagerI2cRead(pI2c, 0x3013, &RegLAECCtrl);
        RegLAECCtrl &= ~0x08;

        if( NewFrameLength > pContext->DefaultFrameLength )
        {
            VsyncWidth = NewFrameLength - pContext->DefaultFrameLength;
        }else
        {
            VsyncWidth = 0;
        }

        // Current CoarseTime needs to be updated?
        //if (pContext->CoarseTime != NewCoarseTime)
        {
            // Calculate the new exposure based on the sensor and sensor settings.
            pContext->Exposure = ((NvF32)NewCoarseTime * NewLineLength) / Freq;
            NewCoarseTime -= VsyncWidth;

            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x30B7,0x88); //Group Write enable
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x301e,(VsyncWidth >> 8) & 0xFF);
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x301f,VsyncWidth & 0xFF);
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x3022,(NewLineLength>>8)&0xff);    //LineLength should be default
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x3023, NewLineLength&0xff);        //LineLength should be default
            //DON'T TOUCH FRAME_LENGTH REGISTER, OTHERWISE, AT LEAST, ONE FRAME SHOULD BE DROPPED
            //NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x3020,(NewFrameLength >> 8) & 0xFF);
            //NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x3021,NewFrameLength & 0xFF);
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x3002,(NewCoarseTime >> 8) & 0xFF);
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x3003,NewCoarseTime & 0xFF);
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x3013,RegLAECCtrl);
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x30B7,0x80); //Group Write disable
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c,0x30FF,0xFF); //Group Write flag
        }
    }
    if(pContext->ModeIndex == 0)
    {
        *pNewExposureTime = pContext->Exposure / 2.0f;
    }
    else
    {
        *pNewExposureTime = pContext->Exposure;
    }
#if LOCAL_DEBUG
    NvOdmOsDebugPrintf("new exposure = %f     RegLAECCtrl=0x%x\n", pContext->Exposure,RegLAECCtrl);
    NvOdmOsDebugPrintf("Freq = %f    FrameLen=0x%x(%d), NewCoarseTime=0x%x(%d), LineLength=0x%x(%d) VsyncWidth=0x%x(%d)\n", 
                       Freq,pContext->FrameLength,pContext->FrameLength,NewCoarseTime,NewCoarseTime,pContext->LineLength,
                       pContext->LineLength, VsyncWidth, VsyncWidth);
#endif
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
#if LOCAL_DEBUG    
NvOdmOsDebugPrintf("SensorBayer_SetInputClock : Sensor Initialized\r\n");
#endif
        pContext->VtPixClkFreqHz =
            (NvU32)((pContext->SensorInputClockkHz * 1000 * pContext->PllMult) /
                    (pContext->PllPreDiv * pContext->PllVtPixDiv *
                     pContext->PllVtSysDiv));

        pContext->Exposure = (NvF32)(pContext->CoarseTime * pContext->LineLength) /(NvF32)pContext->VtPixClkFreqHz;
        pContext->MaxExposure = (NvF32)((pContext->DefaultFrameLength+0xffff-SENSOR_BAYER_DEFAULT_DIFF_E_F) * pContext->MaxLineLength ) / (NvF32)pContext->VtPixClkFreqHz;
        pContext->MinExposure = (NvF32)(SENSOR_BAYER_DEFAULT_MIN_EXPOSURE_FINE) / (NvF32)pContext->VtPixClkFreqHz;
    }
#if LOCAL_DEBUG    
NvOdmOsDebugPrintf("SensorBayer_SetInputClock : %f (%f, %f), Freq=%f\r\n", 
             pContext->Exposure,
             pContext->MinExposure,
             pContext->MaxExposure,
             pContext->VtPixClkFreqHz);
#endif             
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
        NewGains[i] = (NvU16)(Gain_F2H(pGains[i]));
    }

#if LOCAL_DEBUG
    NvOdmOsDebugPrintf("- gains 0x3000 = 0x%x,\n", NewGains[1]);
#endif

    NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x3000, NewGains[1]);

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
    pSensorBayerContext->MaxGain = Gain_H2F(SENSOR_BAYER_DEFAULT_MAX_GAIN);
    pSensorBayerContext->MinGain = Gain_H2F(SENSOR_BAYER_DEFAULT_MIN_GAIN);
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

    NvOdmOsDebugPrintf("----Setmode begin: resolution %dx%d\n",
        pParameters->Resolution.width, pParameters->Resolution.height);

    // Find the right entrys in g_SensorBayerSetModeSequenceList that matches
    // the desired resolution and framerate
    for (Index = 0; Index < pContext->NumModes; Index++)
    {
        if ((pParameters->Resolution.width ==
             g_SensorBayerSetModeSequenceList[Index].Mode.
             ActiveDimensions.width) &&
            (pParameters->Resolution.height ==
             g_SensorBayerSetModeSequenceList[Index].Mode.
             ActiveDimensions.height))
             break;
    }

    // No match found
    if (Index == pContext->NumModes)
        return NV_FALSE;

    if (pSelectedMode)
    {
        *pSelectedMode = g_SensorBayerSetModeSequenceList[Index].Mode;
    }

    if (pContext->ModeIndex == Index)
        return NV_TRUE;
 
    // i2c writes for the set mode sequence
    Status = WriteI2CSequence8(&pContext->I2c,
                 g_SensorBayerSetModeSequenceList[Index].pSequence,
                 pContext->FastSetMode && pContext->SensorInitialized);
    
    if(!pContext->SensorInitialized)
        ResetFocuserPosition(hImager);

    if (!Status)
        return NV_FALSE;

    pModeSettings =
        (ModeDependentSettings*)&g_SensorBayerSetModeSequenceList[Index].ModeDependentSettings;

    /**
     * the following is Phase 2.
     */
    pContext->SensorInitialized = NV_TRUE;

    // Update sensor context after set mode
    NV_ASSERT(pContext->SensorInputClockkHz > 0);


    // These hardcoded numbers are from the set mode sequence and this formula
    // is based on this sensor. Sensor Dependent.
    pContext->PllMult = pModeSettings->PllMult;
    pContext->PllPreDiv = pModeSettings->PllPreDiv;
    pContext->PllVtPixDiv = pModeSettings->PllVtPixDiv;
    pContext->PllVtSysDiv = pModeSettings->PllVtSysDiv;
    pContext->VtPixClkFreqHz =
        (NvU32)((pContext->SensorInputClockkHz * 1000 * pContext->PllMult) /
                (pContext->PllPreDiv * pContext->PllVtPixDiv *
                 pContext->PllVtSysDiv));

    pContext->LineLength = 
    pContext->MinLineLength = 
    pContext->DefaultLineLength = pModeSettings->LineLength;
    pContext->MaxLineLength = SENSOR_BAYER_DEFAULT_MAX_LINE_LENGTH;

    pContext->FrameLength = 
    pContext->MinFrameLength = 
    pContext->DefaultFrameLength = pModeSettings->FrameLength;
    pContext->MaxFrameLength = (0xFFFF+pContext->DefaultFrameLength);
    
    pContext->CoarseTime = SENSOR_BAYER_DEFAULT_EXPOSURE_COARSE;
    pContext->Exposure = (NvF32)(pContext->CoarseTime * pContext->LineLength) /
                         (NvF32)pContext->VtPixClkFreqHz;
    pContext->MaxExposure = (NvF32)((pContext->DefaultFrameLength+0xffff-SENSOR_BAYER_DEFAULT_DIFF_E_F) *
                                    pContext->MaxLineLength ) /
                            (NvF32)pContext->VtPixClkFreqHz;
    pContext->MinExposure = (NvF32)(SENSOR_BAYER_DEFAULT_MIN_EXPOSURE_FINE) /
                            (NvF32)pContext->VtPixClkFreqHz;

    pContext->FrameRate = (NvF32)pContext->VtPixClkFreqHz /
                          (NvF32)(pContext->FrameLength *
                                  pContext->LineLength);
    pContext->MaxFrameRate = pContext->FrameRate;
    pContext->MinFrameRate = (NvF32)pContext->VtPixClkFreqHz /
                             (NvF32)((pContext->DefaultFrameLength+0xffff) *
                                     pContext->MaxLineLength);
    pContext->Gains[0] = Gain_H2F(SENSOR_BAYER_DEFAULT_GAIN_R);
    pContext->Gains[1] = Gain_H2F(SENSOR_BAYER_DEFAULT_GAIN_GR);
    pContext->Gains[2] = Gain_H2F(SENSOR_BAYER_DEFAULT_GAIN_GB);
    pContext->Gains[3] = Gain_H2F(SENSOR_BAYER_DEFAULT_GAIN_B);
 
    pContext->ModeIndex = Index;
   if (pParameters->Exposure != 0.0)
    {
        Status = SensorBayer_WriteExposure(pContext, &((NvF32)pParameters->Exposure));
        if (!Status)
        {
            NV_DEBUG_PRINTF(("SensorBayer_WriteExposure failed\n"));    
        }
    }

    if (pParameters->Gains[0] != 0.0 && pParameters->Gains[1] != 0.0 &&
        pParameters->Gains[2] != 0.0 && pParameters->Gains[3] != 0.0)
    {
        Status = SensorBayer_WriteGains(pContext, pParameters->Gains);
        if (!Status)
        {
            NV_DEBUG_PRINTF(("SensorBayer_WriteGains failed\n"));
        }
    }

    //Wait 5 frames for gains/exposure to take effect. 
    //This is same as SensorExposureLatchTime.
    NvOsSleepMS((NvU32)(5000.0 / (NvF32)(pContext->FrameRate)));

NvOdmOsDebugPrintf("-------------SetMode Ends---------------\r\n");
#if LOCAL_DEBUG
NvOdmOsDebugPrintf("Exposure : %f (%f, %f)\r\n", 
             pContext->Exposure,
             pContext->MinExposure,
             pContext->MaxExposure);
NvOdmOsDebugPrintf("Gain : %f (%f, %f)\r\n", 
             pContext->Gains[1],
             pContext->MinGain,
             pContext->MaxGain);
NvOdmOsDebugPrintf("FrameRate : %f (%f, %f)\r\n", 
            pContext->FrameRate,
            pContext->MinFrameRate,
            pContext->MaxFrameRate);
#endif            
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
            if (!Status)
                return NV_FALSE;

            break;

        case NvOdmImagerPowerLevel_Standby:

            Status = SetPowerLevelWithPeripheralConnectivity(pConnections,
                         &pContext->I2c, &pContext->GpioConfig,
                         NvOdmImagerPowerLevel_Standby);

            break;
        case NvOdmImagerPowerLevel_Off:

             // Set the sensor to standby mode
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x30fa, 0);

            Status = SetPowerLevelWithPeripheralConnectivity(pConnections,
                         &pContext->I2c, &pContext->GpioConfig,
                         NvOdmImagerPowerLevel_Off);
            
            // wait for the standby 0.1 sec should be enough
            NvOdmOsWaitUS(100000); 

            break;
        default:
            //NV_ASSERT("!Unknown power level\n");
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
#if !ENABLE_OVISP
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
            return NV_TRUE;

#if 0
        // This is optional but nice to have.
        case NvOdmImagerParameter_SelfTest:
            //TODO: Not Implemented.
            break;
        // Phase 3.

        case NvOdmImagerParameter_MaxSensorFrameRate:
            //TODO: Not Implemented.
            break;
#endif            
        default:
            //NvOdmOsDebugPrintf("Setparameter : %x\r\n", Param);
            //NV_ASSERT(!"Not Implemented");
            break;
    }
#endif    
    return Status;
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

    switch(Param)
    {
        // Phase 1.
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
                    NvOdmImagerI2cRead(pI2c, 0x300a, &pStatus->Values[0]);
                    NvOdmImagerI2cRead(pI2c, 0x300B, &pStatus->Values[1]);
                    pStatus->Count = 2;
                }
            }
            break;

        // Phase 1, it can return min = max = 10.0f
        //          (the fps in g_SensorBayerSetModeSequenceList)
        // Phase 2, return the real numbers
        case NvOdmImagerParameter_SensorFrameRateLimits:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                2 * sizeof(NvF32));
            if(pContext->ModeIndex == 0)
            {
                // OV8810 in binning and non-full-res mode need exposure 
                // adjustment.
                pFloatValue[0] = pContext->MinFrameRate * 2.0f;
                pFloatValue[1] = pContext->MaxFrameRate * 2.0f;
            }
            else
            {
                pFloatValue[0] = pContext->MinFrameRate;
                pFloatValue[1] = pContext->MaxFrameRate;
            }

            break;

        // Phase 1, it can return 1.0f
        // Phase 2, return the real numbers.
        case NvOdmImagerParameter_SensorFrameRate:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue, sizeof(NvF32));

            if(pContext->ModeIndex == 0)
            {
                // OV8810 in binning and non-full-res mode need exposure 
                // adjustment.
                pFloatValue[0] = pContext->FrameRate * 2.0f;
            }
            else
            {
                pFloatValue[0] = pContext->FrameRate;
            }
            break;

        case NvOdmImagerParameter_CalibrationOverrides:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerCalibrationData));
            {
                // Try to fstat the overrides file, allocate a buffer
                // of the appropriate size, and read it in.
                static const char *pFiles[] =
                {
                    "\\release\\camera_overrides.isp",
                    "\\NandFlash\\camera_overrides.isp",
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
                    SensorBayer_ChooseModeIndex(pContext, pData->Resolution,
                    &Index);

                if (!MatchFound)
                {
                    pData->MinFrameRate = 0.0f;
                    pData->MaxFrameRate = 0.0f;
                    break;
                }

                pModeSettings = 
                    &g_SensorBayerSetModeSequenceList[Index].
                    ModeDependentSettings;

                pData->MaxFrameRate = (NvF32)pContext->VtPixClkFreqHz /
                                      (NvF32)(pModeSettings->FrameLength *
                                              pModeSettings->LineLength);
                pData->MinFrameRate =
                    (NvF32)pContext->VtPixClkFreqHz /
                    (NvF32)((pModeSettings->FrameLength + 0xFFFF) *
                             SENSOR_BAYER_DEFAULT_MAX_LINE_LENGTH);
                if(Index == 0)
                {
                    // OV8810 in binning and non-full-res mode need exposure 
                    // adjustment.
                    pData->MinFrameRate *= 2.0f;
                    pData->MaxFrameRate *= 2.0f;
                }
            }
            break;

        case NvOdmImagerParameter_SensorExposureLatchTime:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                            sizeof(NvU32));
            {
                NvU32 *pLatchTime = (NvU32*)pValue;
                *pLatchTime = 1;
            }
            break;
        // Phase 3.
        default:
            //NV_ASSERT("!Not Implemented");
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

static NV_INLINE NvBool
SensorBayer_ChooseModeIndex(
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
