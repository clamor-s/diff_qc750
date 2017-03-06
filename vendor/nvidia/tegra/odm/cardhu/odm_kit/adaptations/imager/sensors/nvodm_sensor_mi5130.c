/*
 * Copyright (c) 2006-2010 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvodm_imager_int.h"
#include "nvodm_sensor_mi5130.h"
#include "nvodm_sensor_common.h"
#include "nvodm_query_gpio.h"
#include "nvodm_query_discovery.h"
#include "nvos.h"
#include "nvassert.h"
#include "nvodm_imager_guids.h"


// For better video/preview quality, set this to true.
#define USE_JUST_TWO_RESOLUTIONS 0

/// Set to 1 for possible reliability/stability, but no optimized preview
/// Set to 0 for what we have been using since AP15 bringup
#define USE_SLOW_SETMODE (0)

#define NVODM_SENSOR_TYPE          SENSOR_MI5130_DEVID
#define NVODM_SENSOR_MICRON        SENSOR_MI5130_DEVID
#define NVODM_SENSOR_DEVICE_ADDR   SENSOR_MI5130_DEVICE_ADDR

#define NVODM_SENSOR_8_BIT_DEVICE   0

/** Implementation NvOdmImager driver for Micron 5130.
 */

#define NVODM_SENSOR_REG_MODEL_ID               0x0000
#define NVODM_SENSOR_REG_FRAME_COUNT            0x303B
#define NVODM_SENSOR_REG_FRAME_STATUS           0x303C
#define NVODM_SENSOR_REG_DATA_PATH_STATUS       0x306A

/*
 * Sensor Registers Used
 *
 * #defines for making the below code easier to
 * read, each sensor may need different register
 * or numbers of registers to accomplish the same
 * task. For instance, OV9650 stores exposure bits
 * across 3 registers.
 */
#define NVODM_SENSOR_REG_MODE_SELECT            0x0100
#define NVODM_SENSOR_MODE_STREAMING             1
#define NVODM_SENSOR_MODE_STANDBY               0

#define NVODM_SENSOR_REG_STREAMING_CONTROL      0x0100
#define NVODM_SENSOR_REG_SOFTWARE_RESET         0x0103
#define NVODM_SENSOR_REG_GROUP_PARAM_HOLD       0x0104
#define NVODM_SENSOR_REG_EXPOSURE_COARSE        0x0202
#define NVODM_SENSOR_REG_EXPOSURE_FINE          0x0200
#define NVODM_SENSOR_REG_ANALOG_GAIN            0x0204
#define NVODM_SENSOR_REG_ANALOG_GAIN_GR         0x0206
#define NVODM_SENSOR_REG_ANALOG_GAIN_R          0x0208
#define NVODM_SENSOR_REG_ANALOG_GAIN_B          0x020A
#define NVODM_SENSOR_REG_ANALOG_GAIN_GB         0x020C
#define NVODM_SENSOR_REG_DIGITAL_GAIN_0         0x020E
#define NVODM_SENSOR_REG_DIGITAL_GAIN_1         0x0210
#define NVODM_SENSOR_REG_DIGITAL_GAIN_2         0x0212
#define NVODM_SENSOR_REG_DIGITAL_GAIN_3         0x0214

#define NVODM_SENSOR_REG_VT_PIX_CLK_DIV         0x0300
#define NVODM_SENSOR_REG_VT_SYS_CLK_DIV         0x0302
#define NVODM_SENSOR_REG_PLL_PRE_DIV            0x0304
#define NVODM_SENSOR_REG_PLL_MULT               0x0306
#define NVODM_SENSOR_REG_PLL_OP_PIX_DIV         0x0308
#define NVODM_SENSOR_REG_PLL_OP_SYS_DIV         0x030A

#define NVODM_SENSOR_REG_FRAME_LENGTH_LINES     0x0340
#define NVODM_SENSOR_REG_LINE_LENGTH_PCK        0x0342
#define NVODM_SENSOR_REG_SCALING_MODE           0x0400
#define NVODM_SENSOR_REG_SCALE_M                0x0404

#define NVODM_SENSOR_REG_X_ADDR_START           0x0344
#define NVODM_SENSOR_REG_X_ADDR_END             0x0348
#define NVODM_SENSOR_REG_Y_ADDR_START           0x0346
#define NVODM_SENSOR_REG_Y_ADDR_END             0x034A
#define NVODM_SENSOR_REG_X_OUTPUT_SIZE          0x034C
#define NVODM_SENSOR_REG_Y_OUTPUT_SIZE          0x034E
#define NVODM_SENSOR_REG_X_EVEN_INC             0x0380
#define NVODM_SENSOR_REG_X_ODD_INC              0x0382
#define NVODM_SENSOR_REG_Y_EVEN_INC             0x0384
#define NVODM_SENSOR_REG_Y_ODD_INC              0x0386

#define NVODM_SENSOR_REG_RESET_REGISTER         0x301A

#define NVODM_SENSOR_REG_MIN_INPUT_CLOCK        6000
#define NVODM_SENSOR_REG_MAX_INPUT_CLOCK        64000
#define NVODM_SENSOR_REG_MAX_FRAME_LINES        0xFFFF
#define NVODM_SENSOR_REG_FRAME_LINES            0x808
#define NVODM_SENSOR_REG_LINE_LENGTH_PACK       0xA50

#define NVODM_SENSOR_MIN_GAIN                   0x08
#define NVODM_SENSOR_MAX_GAIN                   0x7F

#define NVODM_SENSOR_DEFAULT_EXPOS_COARSE       0x0610
#define NVODM_SENSOR_DEFAULT_EXPOS_FINE         0x0372

#define NVODM_SENSOR_NOREG                      0xFFFF

#define NVODM_SENSOR_REG_TEST_MODE              0x600
#define NVODM_SENSOR_REG_TEST_NORMAL            0
#define NVODM_SENSOR_REG_TEST_SOLID             1
#define NVODM_SENSOR_REG_TEST_COLOR_BARS        2
#define NVODM_SENSOR_REG_TEST_FADE_COLORS       3
#define NVODM_SENSOR_REG_TEST_WALKING_COLORS    256


/////////////////////
// Clock Settings
#define NVODM_SENSOR_PLL_FAST_PRE_DIV                2
#define NVODM_SENSOR_PLL_FAST_MULT                   32
#define NVODM_SENSOR_PLL_FAST_VT_PIX_DIV             4
#define NVODM_SENSOR_PLL_FAST_VT_SYS_DIV             1
#define NVODM_SENSOR_PLL_FAST_OP_PIX_DIV             8
#define NVODM_SENSOR_PLL_FAST_OP_SYS_DIV             1
#define NVODM_SENSOR_PLL_PRE_DIV                5
#define NVODM_SENSOR_PLL_MULT                   50
#define NVODM_SENSOR_PLL_VT_PIX_DIV             5
#define NVODM_SENSOR_PLL_VT_SYS_DIV             1
#define NVODM_SENSOR_PLL_OP_PIX_DIV             10
#define NVODM_SENSOR_PLL_OP_SYS_DIV             1
#define NVODM_SENSOR_ROWSPEED                   0x111
/////////////////////
// Timing limits
//#define MIN_LINE_BLANK  2764  // default 5MP, but now it is calc'd
//#define MIN_FRAME_BLANK 96    // default 5MP, but now it is calc'd
#define NVODM_SENSOR_MIN_LINE_BLANK             1180
#define NVODM_SENSOR_MIN_FRAME_BLANK            85
#define NVODM_SENSOR_MIN_X                      8
#define NVODM_SENSOR_MIN_Y                      8
#define NOT_USED                                0
/////////////////////

#define NVODM_SENSOR_STREAMING_CONTROL_OFF     0

#define SENSOR_COMMON_OPEN    Sensor_MI5130_Open
#define SENSOR_COMMON_CLOSE   Sensor_MI5130_Close

// This is the publicized list of supported resolutions (modes)
static NvOdmImagerSensorMode SensorMI5130Modes[] =
{
     // WxH         x,y    fps
#if !USE_SLOW_SETMODE && !USE_JUST_TWO_RESOLUTIONS
    {{640, 480},   {1, 3}, 60, },
    {{720, 540},   {1, 3}, 29, },
    {{1280, 720},  {1, 3}, 26, },
#else
    {{1296, 968},  {1, 3}, 30, }, // quarter res
#endif
    //{{1920, 1088}, {1, 3}, 20, },
    {{2592, 1944}, {1, 3}, 10, }, // full res
};

// Internally, this table gives the register settings
// for each of the modes listed.
SMIAScalerSettings ScalerSettings[] =
{
    // Values taken from DevWare when running MI5130
#if !USE_SLOW_SETMODE && !USE_JUST_TWO_RESOLUTIONS
    {1104, 2358, 8, 8, 2593, 1945, 648, 486, 7, 7, 0, 16}, // 640x480
    {1656, 2372, 560, 420, 2037, 1529, 740, 555, 3, 3, 0, 16}, // 720x540
    {826, 2640, 8, 118, 2597, 1597, 1296, 740, 3, 3, 0, 16}, // 1280x720
#else
    {1061, 3770, 8, 4, 2597, 1953, 1296, 976, 3, 3, 0, 16}, // 1.25MP
#endif
    //{1024, 4072, 364, 448, 2299, 1551, 1936, 1104, 1, 1, 0, 16}, // 1920x1088
    {2056, 5372, 8, 8, 2599, 1951, 2592, 1944+16, 1, 1, 0, 16}, // 5MP
    //{2037, 5302, 8, 4, 2599, 1955, 2592, 1952, 1, 1, 0, 16}, // 5MP a little faster
};
//NvU32 g_NumberOfScalerSettings = NV_ARRAY_SIZE(ScalerSettings);

#if !USE_SLOW_SETMODE
// Initialization Sequences
static DevCtrlReg16 SuggestedInitSequence[] = {
    {0x0100, 0x0001}, // mode_select =
    {0x0104, 0x0000}, // grouped parameter hold off
    {0x0202, NVODM_SENSOR_DEFAULT_EXPOS_COARSE}, // coarse_int time default
    {0x301A, 0x10CC}, // reset register
    {0x3040, 0x0024}, // read mode 0x6C
    {NVODM_SENSOR_REG_ANALOG_GAIN, NVODM_SENSOR_MIN_GAIN},
    {0x3088, 0x6FFB}, // undocumented register (addendum)
    {0x308E, 0x2020}, // undocumented - fixes column fixed pattern noise
    {0x309E, 0x4400}, // undocumented - column fixed pattern noise: Errata1
    {0x309A, 0xA500}, // undocumented register (micron tools?)
    {0x30D4, 0x9080}, // undocumented register (addendum)
    {0x30EA, 0x3F06}, // undocumented register (addendum)
    {0x3154, 0x1482}, // undocumented register (addendum)
    {0x3156, 0x1C81}, // undocumented register (micron tools?)
    {0x3158, 0x97C7}, // undocumented register (addendum)
    {0x315A, 0x97C6}, // undocumented register (addendum)
    {0x3162, 0x074C}, // undocumented register (addendum)
    {0x3164, 0x0756}, // undocumented register (addendum)
    {0x3166, 0x0760}, // undocumented register (addendum)
    {0x316E, 0x8488}, // undocumented register (addendum)
    {0x3172, 0x0003}, // undocumented register (addendum)
    {0x3016, NVODM_SENSOR_ROWSPEED}, // row speed
    {0x0304, NVODM_SENSOR_PLL_PRE_DIV}, // pre_pll_clk_div (default 2)
    {0x0306, NVODM_SENSOR_PLL_MULT}, // pll_multiplier (default 50)
    {0x0300, NVODM_SENSOR_PLL_VT_PIX_DIV}, // vt_pix_clk_div (default 5)
    {0x0302, NVODM_SENSOR_PLL_VT_SYS_DIV}, // vt_sys_clk_div (default 1)
    {0x0308, NVODM_SENSOR_PLL_OP_PIX_DIV}, // op_pix_clk_div (default 10)
    {0x030A, NVODM_SENSOR_PLL_OP_SYS_DIV}, // op_sys_clk_div (default 1)
    {0x3162, 0x074C}, // global reset end
    /* End-of-Sequence marker */
    {0xFFFF, 0x0}
};
static DevCtrlReg16 SuggestedInitSequence_fast[] = {
    {0x0100, 0x0001}, // mode_select =
    {0x0104, 0x0000}, // grouped parameter hold off
    {0x0202, NVODM_SENSOR_DEFAULT_EXPOS_COARSE}, // coarse_int time default
    {0x301A, 0x10CC}, // reset register
    {0x3040, 0x0024}, // read mode 0x6C
    {NVODM_SENSOR_REG_ANALOG_GAIN, NVODM_SENSOR_MIN_GAIN},
    {0x3088, 0x6FFB}, // undocumented register (addendum)
    {0x308E, 0x2020}, // undocumented - fixes column fixed pattern noise
    {0x309E, 0x4400}, // undocumented - column fixed pattern noise: Errata1
    {0x309A, 0xA500}, // undocumented register (micron tools?)
    {0x30D4, 0x9080}, // undocumented register (addendum)
    {0x30EA, 0x3F06}, // undocumented register (addendum)
    {0x3154, 0x1482}, // undocumented register (addendum)
    {0x3156, 0x1C81}, // undocumented register (micron tools?)
    {0x3158, 0x97C7}, // undocumented register (addendum)
    {0x315A, 0x97C6}, // undocumented register (addendum)
    {0x3162, 0x074C}, // undocumented register (addendum)
    {0x3164, 0x0756}, // undocumented register (addendum)
    {0x3166, 0x0760}, // undocumented register (addendum)
    {0x316E, 0x8488}, // undocumented register (addendum)
    {0x3172, 0x0003}, // undocumented register (addendum)
    {0x3016, NVODM_SENSOR_ROWSPEED}, // row speed
    {0x0304, NVODM_SENSOR_PLL_FAST_PRE_DIV}, // pre_pll_clk_div (default 2)
    {0x0306, NVODM_SENSOR_PLL_FAST_MULT}, // pll_multiplier (default 50)
    {0x0300, NVODM_SENSOR_PLL_FAST_VT_PIX_DIV}, // vt_pix_clk_div (default 5)
    {0x0302, NVODM_SENSOR_PLL_FAST_VT_SYS_DIV}, // vt_sys_clk_div (default 1)
    {0x0308, NVODM_SENSOR_PLL_FAST_OP_PIX_DIV}, // op_pix_clk_div (default 10)
    {0x030A, NVODM_SENSOR_PLL_FAST_OP_SYS_DIV}, // op_sys_clk_div (default 1)
    {0x3162, 0x074C}, // global reset end
    /* End-of-Sequence marker */
    {0xFFFF, 0x0}
};

#else

/// Moved all the tables to another file
#include "nvodm_sensor_mi5130_i2c.c"

// Since our Concordes have both A01 and A02 chips
// we need to handle both cases for now.
static SensorInitSequence SensorMI5130InitSequenceList[] =
{
    // Pre-A02
    {1296, 968, InitSequence_1296x968},
    {2592, 1944, InitSequence_2592x1944},
};
static SensorInitSequence SensorMI5130InitSequenceList_fast[] =
{
    // A02 or later
    {1296, 968, InitSequence_1296x968_fast},
    {2592, 1944, InitSequence_2592x1944_fast},
};

#endif

static NvOdmImagerCapabilities SensorMI5130Caps =
{
    "Micron 5130",
    NvOdmImagerSensorInterface_Parallel10,
    {
        NvOdmImagerPixelType_BayerGRBG,
    },
    NvOdmImagerOrientation_180_Degrees,
    NvOdmImagerDirection_Away,
    12000, // minimum input clock   (in khz)
    { //{24000, 64.f/24.f}, // preferred clock profile
      {64000, 1.0}},      // backward compatible profile
    { NvOdmImagerSyncEdge_Rising,
        NvOdmImagerSyncEdge_Rising, NV_FALSE }, // parallel
    { NOT_USED }, // serial
    { 18, 24 }, // min blank time,
    0, // preferred mode
    0ULL, // no focuser
    0, // FLASH_GUID, only set if flash device code is available
    NVODM_IMAGER_CAPABILITIES_END
};

static NvU32 g_NumberOfResolutions = NV_ARRAY_SIZE(SensorMI5130Modes);
static const NvS32 gs_PreferredModeIndex = 0;  // 640x480 default right now

#define INVALID_MODE_INDEX (0x7FFFFFFF)

#include "camera_micron_5130_calibration.h"

#if USE_SLOW_SETMODE
#define SENSOR_INIT_SEQUENCE_LIST   SensorMI5130InitSequenceList
#else
#define SENSOR_INIT_SEQUENCE        SuggestedInitSequence
#define SENSOR_INIT_SEQUENCE_FAST   SuggestedInitSequence_fast
#endif

#define SENSOR_CAPABILITIES         SensorMI5130Caps
#define SENSOR_MODES                SensorMI5130Modes

#define NVODM_READREG(retVal)                       \
    {                                               \
        NVODM_READREG_RETURN_ON_ERROR(0x00, value);             \
        NvOdmOsDebugPrintf("Read device id of 0x%x\n", value);  \
        retVal = (value == 0x2800);                             \
    }

// To create this list, inspect the register spec for the sensor
// and identify the groupings of registers. (consecutive addresses)
static CompactRegisterList gs_RegList5130[] = {
    // Succinct
    { NVODM_SENSOR_REG_MODEL_ID, 1}, // pg35
    { NVODM_SENSOR_REG_FRAME_COUNT, 1}, // pg 49
    { NVODM_SENSOR_REG_FRAME_STATUS, 1}, // pg 49
    { NVODM_SENSOR_REG_DATA_PATH_STATUS, 1}, // pg 52
 #if 0
    // Verbose
    { 0x0000, 5 },
    { 0x0040, 16 },
    { 0x0080, 10 },
    { 0x00C0, 8 },
    { 0x0100, 3 },
    { 0x0120, 1 },
    { 0x0200, 11 },
    { 0x0300, 6 },
    { 0x0340, 8 },
    { 0x0380, 4 },
    { 0x0400, 4 },
    { 0x0500, 1 },
    { 0x0600, 9 },
#endif
};
static NvU32 gs_RegList5130Count = NV_ARRAY_SIZE(gs_RegList5130);

#define SENSOR_REGISTER_LIST  gs_RegList5130
#define SENSOR_REGISTER_LIST_COUNT  gs_RegList5130Count

static NvOdmImagerExpectedValues gs_ExpValues5130 = {
    {
     //   x, y, w,    h,   tl,   tr,   bl,   br.
     {    8, 8, 4, 1928, 1008, 1008, 1008, 1008},
     {  316, 8, 4, 1928, 1008, 1008, 1008, 1008},
     {  332, 8, 4, 1928,    0, 1008, 1008, 1008},
     {  640, 8, 4, 1928,    0, 1008, 1008, 1008},
     {  656, 8, 4, 1928, 1008, 1008, 1008,    0},
     {  964, 8, 4, 1928, 1008, 1008, 1008,    0},
     {  980, 8, 4, 1928,    0, 1008, 1008,    0},
     { 1288, 8, 4, 1928,    0, 1008, 1008,    0},
     { 1304, 8, 4, 1928, 1008,    0,    0, 1008},
     { 1612, 8, 4, 1928, 1008,    0,    0, 1008},
     { 1628, 8, 4, 1928,    0,    0,    0, 1008},
     { 1928, 8, 4, 1928,    0,    0,    0, 1008},
     { 1952, 8, 4, 1928, 1008,    0,    0,    0},
     { 2260, 8, 4, 1928, 1008,    0,    0,    0},
     { 2276, 8, 4, 1928,    0,    0,    0,    0},
     { 2584, 8, 4, 1928,    0,    0,    0,    0}}
};

#define SENSOR_EXPECTED_VALUES gs_ExpValues5130

// set this to 1 so nothing will be changed for now.
#define SENSOR_CAP_OPTIMIZE_RESOLUTION_CHANGE 1
#if USE_SLOW_SETMODE
static NvBool SensorMI5130SetMode(
        NvOdmImagerDeviceHandle sensor,
        const NvSize *Resolution,
        NvF32 FrameRate,
        NvOdmImagerSensorMode *SelectedMode,
        NvOdmImagerPixelType PixelType);

static NvBool SensorMI5130_InitProgramming(
        NvOdmImagerI2cConnection *i2c,
        DevCtrlReg16 *pInitSequence);

#define SENSOR_SET_MODE SensorMI5130SetMode
#define SENSOR_INIT_PROGRAMMING SensorMI5130_InitProgramming
#endif

#include "nvodm_sensor_common.c"

#if USE_SLOW_SETMODE
static NvBool SensorMI5130_InitProgramming(
        NvOdmImagerI2cConnection *i2c,
        DevCtrlReg16 *pInitSequence)
{
    NvU32 I2CBufferAddrData[I2C_BUFFER_SIZE];
    NvU32 I2CBufferSize = 0;
    NvOdmI2cStatus Status;

    if (i2c->hOdmI2c == NULL) {
        i2c->hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, i2c->I2cPort);
        if (!i2c->hOdmI2c) {
            return NV_FALSE;
        }
    }

    while (pInitSequence->RegAddr != 0xFFFF)
    {
        if (pInitSequence->RegAddr == 0xFFFE) // Delay
        {
            // Flush the buffer of i2c writes
            if (I2CBufferSize)
            {
                Status = NvOdmImagerI2cWriteBuffer16(i2c,
                                                     I2CBufferAddrData,
                                                     I2CBufferSize);
                if (Status != NvOdmI2cStatus_Success)
                {
                    NvOdmOsDebugPrintf("I2C WriteBuffer failed\n");
                    return NV_FALSE;
                }
                I2CBufferSize = 0;
            }
            // convert sleep value from milli to micro and wait
            NvOdmOsWaitUS(pInitSequence->RegValue * 1000);
        }
        else
        {
            // I2C writes
            NVODM_WRITEREG_BUFFER(pInitSequence->RegAddr, pInitSequence->RegValue);
        }

        pInitSequence += 1;
    }
    if (I2CBufferSize)
    {
        Status = NvOdmImagerI2cWriteBuffer16(i2c,
                                             I2CBufferAddrData,
                                             I2CBufferSize);
        if (Status != NvOdmI2cStatus_Success)
        {
            NvOdmOsDebugPrintf("I2C WriteBuffer failed\n");
            return NV_FALSE;
        }
    }

    return NV_TRUE;
}

static NvBool SensorMI5130SetMode(
        NvOdmImagerDeviceHandle sensor,
        const NvSize *pResolution,
        NvF32 FrameRate,
        NvOdmImagerSensorMode *pSelectedMode,
        NvOdmImagerPixelType PixelType)
{
    NvBool Status = NV_FALSE;
    NvOdmImagerDeviceContext *hSensor = (NvOdmImagerDeviceContext *)sensor;
    SensorContext *pCtx = (SensorContext *)hSensor->prv;
    NvOdmImagerI2cConnection *i2c = &hSensor->i2c;
    NvU32 Index;

    NvOdmOsDebugPrintf("Setting resolution to %dx%d\n",
                       pResolution->width, pResolution->height);
    for (Index = 0; Index < g_NumberOfResolutions; Index++) {
        if (RESOLUTION_MATCHES(pResolution->width, pResolution->height,
            &SensorMI5130Modes[Index]) &&
            FrameRate <= SensorMI5130Modes[Index].PeakFrameRate)
            break;
    }

    if (Index == g_NumberOfResolutions)
        return NV_FALSE;

    if (pSelectedMode)
    {
        *pSelectedMode = SensorMI5130Modes[Index];
    }

    if (pCtx->CurrentModeIndex == Index)
        return NV_TRUE;

    NV_ASSERT(
        (NvU32)pResolution->width ==
        SensorMI5130InitSequenceList[Index].Width &&
        (NvU32)pResolution->height ==
        SensorMI5130InitSequenceList[Index].Height);

    // Multiply the clock by 2, because we currently know that
    // the sensor doesn't like to be initialized at full speed
    // so it gets slowed to 1/2 the requested rate.
    // We request 24MHz input and 64MHz output, but if the nvmm driver
    // gives it 32MHz (64/2) instead of 12MHz, then we know we
    // are on an older APX2500 chip (pre A02).
    if (pCtx->shadow.SensorClockRatio == 1.0)
    {
        SensorMI5130_InitProgramming(i2c,
                SensorMI5130InitSequenceList[Index].pInitSequence);
        pCtx->shadow.currPllPreDiv = NVODM_SENSOR_PLL_PRE_DIV;
        pCtx->shadow.currPllMult = NVODM_SENSOR_PLL_MULT;
        pCtx->shadow.currPllVtPixDiv = NVODM_SENSOR_PLL_VT_PIX_DIV;
        pCtx->shadow.currPllOpPixDiv = NVODM_SENSOR_PLL_OP_PIX_DIV;
        pCtx->shadow.currPllVtSysDiv = NVODM_SENSOR_PLL_VT_SYS_DIV;
        pCtx->shadow.currPllOpSysDiv = NVODM_SENSOR_PLL_OP_SYS_DIV;
        NvOsDebugPrintf("Using slow PLL settings\n");

        // replace with something clever
        if (Index == 0)
        {
            pCtx->shadow.LineLengthPck = 5372;
            pCtx->shadow.FrameLengthLines = 2056;
        }
        else
        {
            pCtx->shadow.LineLengthPck = 3770;
            pCtx->shadow.FrameLengthLines = 1067;
        }
    }
    else
    {
        SensorMI5130_InitProgramming(i2c,
                SensorMI5130InitSequenceList_fast[Index].pInitSequence);
        pCtx->shadow.currPllPreDiv = NVODM_SENSOR_PLL_FAST_PRE_DIV;
        pCtx->shadow.currPllMult = NVODM_SENSOR_PLL_FAST_MULT;
        pCtx->shadow.currPllVtPixDiv = NVODM_SENSOR_PLL_FAST_VT_PIX_DIV;
        pCtx->shadow.currPllOpPixDiv = NVODM_SENSOR_PLL_FAST_OP_PIX_DIV;
        pCtx->shadow.currPllVtSysDiv = NVODM_SENSOR_PLL_FAST_VT_SYS_DIV;
        pCtx->shadow.currPllOpSysDiv = NVODM_SENSOR_PLL_FAST_OP_SYS_DIV;
        NvOsDebugPrintf("Using fast PLL settings\n");
        // replace with something clever
        if (Index == 0)
        {
            pCtx->shadow.LineLengthPck = 5372;
            pCtx->shadow.FrameLengthLines = 2056;
        }
        else
        {
            pCtx->shadow.LineLengthPck = 3770;
            pCtx->shadow.FrameLengthLines = 1067;
        }
    }

#if 0
    NVODM_SYNC_FRAME("init programming", Status);
#else
    Status = NV_TRUE;
#endif
    NV_ASSERT(Status);

    // TODO: the following shadow is hardcoded for 5MP for debugging.
    pCtx->shadow.LineLengthPck_F = (NvF32) pCtx->shadow.LineLengthPck;
    pCtx->shadow.currCoarse = pCtx->shadow.FrameLengthLines-1;

    pCtx->CurrentModeIndex = Index;


    return Status;
}

#endif
