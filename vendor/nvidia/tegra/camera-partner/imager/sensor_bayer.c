/**
 * Copyright (c) 2008-2012, NVIDIA Corporation.  All rights reserved.
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
 *
 * Note: For more detailed information, see imager adaptation in your
 *       Tegra BSP Developer's Guide in the \docs directory.
 ****************************************************************************/


/*
 * Sensor default settings. Phase 2. Sensor Dependent.
 * #defines make the code more readable and helps maintainability
 * The template tries to keep the list reduced to just things that
 * are used more than once.  Ideally, all numbers in the code would use
 * a #define.
 * These settings are for full resolution only. If multiple resolutions
 * are supported, use SensorBayerModeDependentSettings.
 *
 * How to check if a setting is mode dependent or not:
 * 1. Find the register address for the setting you want to check
 * 2. Compare the values of the register in i2c sequences for all the
 *    supported resolutions.
 * 3. If the values are not the same in any two of the supported mode,
 *    that setting is mode dependent.
 */
#define SENSOR_BAYER_DEFAULT_EXPOSURE_FINE      (0x0372)
#define SENSOR_BAYER_DEFAULT_LINE_LENGTH        (0x14fc)
#define SENSOR_BAYER_DEFAULT_FRAME_LENGTH       (0x0808)
#define SENSOR_BAYER_DEFAULT_EXPOSURE_COARSE    (0x02fa)
#define SENSOR_BAYER_DEFAULT_PLL_MULT           (0x0032)
#define SENSOR_BAYER_DEFAULT_PLL_PRE_DIV        (0x0005)
#define SENSOR_BAYER_DEFAULT_PLL_VT_PIX_DIV     (0x0005)
#define SENSOR_BAYER_DEFAULT_PLL_VT_SYS_DIV     (0x0001)
#define SENSOR_BAYER_DEFAULT_MAX_GAIN           (0x007f)
#define SENSOR_BAYER_DEFAULT_MIN_GAIN           (0x0008)
#define SENSOR_BAYER_DEFAULT_GAIN_R             (SENSOR_BAYER_DEFAULT_MIN_GAIN)
#define SENSOR_BAYER_DEFAULT_GAIN_GR            (SENSOR_BAYER_DEFAULT_MIN_GAIN)
#define SENSOR_BAYER_DEFAULT_GAIN_GB            (SENSOR_BAYER_DEFAULT_MIN_GAIN)
#define SENSOR_BAYER_DEFAULT_GAIN_B             (SENSOR_BAYER_DEFAULT_MIN_GAIN)
#define SENSOR_BAYER_DEFAULT_MAX_FRAME_LENGTH   (0xffff)
#define SENSOR_BAYER_DEFAULT_MIN_FRAME_LENGTH   (0x0808)

static const char *pOverrideFiles[] =
{
    "/data/camera_overrides.isp",
};

/*
 * ----------------------------------------------------------
 *  Start of Phase 0 code
 * ----------------------------------------------------------
 */

/*
 * Sensor Specific Variables/Defines. Phase 0.
 *
 * If a new sensor code is created from this template, the following
 * variable/defines should be created for the new sensor specifically.
 * 1. A sensor capabilities (NvOdmImagerCapabilities) for this specific sensor
 * 2. A sensor set mode sequence list  (an array of SensorSetModeSequence)
 *    consisting of pairs of a sensor mode and corresponding set mode sequence
 * 3. A set mode sequence for each mode defined in 2.
 */

/*
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
    "SensorBayer",  // String identifier, used for debugging.
    NvOdmImagerSensorInterface_Parallel10, // interconnect type
    {
        NvOdmImagerPixelType_BayerBGGR,
    },
    NvOdmImagerOrientation_180_Degrees, // Rotation with respect to main display.
    NvOdmImagerDirection_Away,          // Toward or away the main display.
    6000, // Initial sensor clock (in kHz).
    { {24000, 64.f/24.f}, // Preferred clock profile.
      {64000, 1.0}},      // Backward compatible profile.
    {
        NvOdmImagerSyncEdge_Rising, // Hsync polarity.
        NvOdmImagerSyncEdge_Rising, // Vsync polarity.
        NV_FALSE,                   // Uses MCLK on vgp0.
    }, // Parallel settings (VIP).
    { 0 }, // Serial settings (CSI) (not used).
    { 18, 24 }, // Min blank time; should not need adjustment.
    0, // Preferred mode, which resolution to use on startup.
    FOCUSER_GUID, // FOCUSER_GUID, only set if focuser code available.
    0, // FLASH_GUID, only set if flash device code is available
    NVODM_IMAGER_CAPABILITIES_END
};

/*
 * SetMode Sequence for 2592x1944. Phase 0. Sensor Dependent.
 * This sequence should put sensor in streaming mode for 2592x1944
 * This is usually given by the FAE or the sensor vendor.
 */
static DevCtrlReg16 SetModeSequence_2592x1944[] =
{
    {0x0100, 0x0001},
    {0x0104, 0x0000},
    {0x0202, 0x0610},
    {0x301A, 0x10CC},
    {0x3040, 0x0024},
    {0x0204, 0x0008},
    {0x3088, 0x6FFB},
    {0x308E, 0x2020},
    {0x309E, 0x4400},
    {0x309A, 0xA500},
    {0x30D4, 0x9080},
    {0x30EA, 0x3F06},
    {0x3154, 0x1482},
    {0x3156, 0x1C81},
    {0x3158, 0x97C7},
    {0x315A, 0x97C6},
    {0x3162, 0x074C},
    {0x3164, 0x0756},
    {0x3166, 0x0760},
    {0x316E, 0x8488},
    {0x3172, 0x0003},
    {0x3016, 0x0111},
    {0x0304, 0x0005},
    {0x0306, 0x0032},
    {0x0300, 0x0005},
    {0x0302, 0x0001},
    {0x0308, 0x000a},
    {0x030A, 0x0001},
    {0x3162, 0x074C},

    {SEQUENCE_WAIT_MS, 100},

    {0x0202, 0x02fa},

    {0x0104, 0x0000},
    {0x0104, 0x0001},
    {0x0340, 0x0808},
    {0x0342, 0x14fc},
    {0x0382, 0x0001},
    {0x0386, 0x0001},

    {0x034C, 0x0a20},
    {0x034E, 0x07a8},
    {0x0344, 0x0008},
    {0x0346, 0x0008},
    {0x0348, 0x0a27},
    {0x034A, 0x079f},
    {0x0340, 0x0808},
    {0x0104, 0x0000},

    {SEQUENCE_WAIT_MS, 100},

    {0x0104, 0x0001},
    {0x0208, 0x0008},
    {0x0206, 0x0008},
    {0x020c, 0x0008},
    {0x020a, 0x0008},

    {0x0104, 0x0000},

    {SEQUENCE_WAIT_MS, 100},
    {SEQUENCE_END, 0x0000}
};


/*
 * Phase 3 or when multiple modes are supported.
 */
typedef struct SensorBayerModeDependentSettingsRec
{
    NvU32   Dummy; // We don't need it here because only one mode is supported.
} SensorBayerModeDependentSettings;

SensorBayerModeDependentSettings g_ModeDependentSettings_2592x1977 = {0};

/*
 * A list of sensor mode and its SetModeSequence. Phase 0. Sensor Dependent.
 * When SetMode is called, it searches the list for desired resolution and
 * frame rate and write the SetModeSequence through i2c.
 * For Phase 3, more resolutions will be added to this table, allowing
 * for peak performance during Preview, D1 and 720P encode.
 */
static SensorSetModeSequence g_SensorBayerSetModeSequenceList[] =
{
     // WxH         x,y    fps    set mode sequence
    {{{2592, 1944}, {1, 3}, 10,}, SetModeSequence_2592x1944,
     &g_ModeDependentSettings_2592x1977},
};


/*
 * ----------------------------------------------------------
 *  Start of Phase 1, Phase 2, and Phase 3 code
 * ----------------------------------------------------------
 */

/*
 * Sensor bayer's private context. Phase 1.
 */
typedef struct SensorBayerContextRec
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

}SensorBayerContext;


/*
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

/*
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
    NvF32 LineLength = (NvF32)pContext->LineLength;
    NvF32 ExpTime = *pNewExposureTime;
    NvU32 NewCoarseTime = 0;
    NvU32 NewFrameLength = 0;

    if (*pNewExposureTime > pContext->MaxExposure ||
        *pNewExposureTime < pContext->MinExposure)
        return NV_FALSE;

    // Here, we have to update the registers in order to set the desired
    // exposure time.
    // For this sensor, Coarse time <= FrameLength - 1 so we may have to
    // calculate a new FrameLength.
    NewCoarseTime = (NvU32)((Freq * ExpTime) / LineLength);
    NewFrameLength = NewCoarseTime + 1;

    if (NewFrameLength > pContext->MaxFrameLength)
        NewFrameLength = pContext->MaxFrameLength;
    else if (NewFrameLength < pContext->MinFrameLength)
        NewFrameLength = pContext->MinFrameLength;

    // Need to update FrameLength?
    if (NewFrameLength != pContext->FrameLength)
    {
        NVODM_WRITEREG_RETURN_ON_ERROR(&pContext->I2c,
                0x0340, NewFrameLength);
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
        NVODM_WRITEREG_RETURN_ON_ERROR(&pContext->I2c,
                0x0202,
                NewCoarseTime);

        // Calculate the new exposure based on the sensor and sensor
        // settings.
        pContext->Exposure = ((NvF32)NewCoarseTime * LineLength +
                              SENSOR_BAYER_DEFAULT_EXPOSURE_FINE) /
                             Freq;
        pContext->CoarseTime = NewCoarseTime;
    }

    *pNewExposureTime = pContext->Exposure;

    NvOdmOsDebugPrintf("new exposure = %f\n", pContext->Exposure);

    return NV_TRUE;
}

/*
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


/*
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
        NewGains[i] = (NvU16)(pGains[i] * 8.0f);
    }

    NVODM_WRITEREG_RETURN_ON_ERROR(&pContext->I2c, 0x0104, 1);
    NVODM_WRITEREG_RETURN_ON_ERROR(&pContext->I2c, 0x0208, NewGains[0]);
    NVODM_WRITEREG_RETURN_ON_ERROR(&pContext->I2c, 0x0206, NewGains[1]);
    NVODM_WRITEREG_RETURN_ON_ERROR(&pContext->I2c, 0x020C, NewGains[2]);
    NVODM_WRITEREG_RETURN_ON_ERROR(&pContext->I2c, 0x020A, NewGains[3]);
    NVODM_WRITEREG_RETURN_ON_ERROR(&pContext->I2c, 0x0104,0);

    NvOdmOsMemcpy(pContext->Gains, pGains, sizeof(NvF32)*4);

    NvOdmOsDebugPrintf("new gains = %f, %f, %f, %f\n", pGains[0],
        pGains[1], pGains[2], pGains[3]);

    return NV_TRUE;
}

/*
 * SensorBayer_Open. Phase 1.
 * Initialize sensor bayer and its private context
 */
static NvBool SensorBayer_Open(NvOdmImagerHandle hImager)
{
    SensorBayerContext *pSensorBayerContext = NULL;

    if (!hImager || !hImager->pSensor)
        return NV_FALSE;

    pSensorBayerContext =
        (SensorBayerContext*)NvOdmOsAlloc(sizeof(SensorBayerContext));
    if (!pSensorBayerContext)
        goto fail;

    NvOdmOsMemset(pSensorBayerContext, 0, sizeof(SensorBayerContext));

    pSensorBayerContext->NumModes =
        NV_ARRAY_SIZE(g_SensorBayerSetModeSequenceList);
    pSensorBayerContext->ModeIndex =
        pSensorBayerContext->NumModes + 1; // invalid mode

    /**
     * Phase 2. Initialize exposure and gains.
     */
    pSensorBayerContext->Exposure = -1.0; // invalid exposure

     // convert gain's unit to float
    pSensorBayerContext->MaxGain = SENSOR_BAYER_DEFAULT_MAX_GAIN / 8.0;
    pSensorBayerContext->MinGain = SENSOR_BAYER_DEFAULT_MIN_GAIN / 8.0;
    /**
     * Phase 2 ends here.
     */

    hImager->pSensor->pPrivateContext = pSensorBayerContext;

    return NV_TRUE;

fail:
    NvOdmOsFree(pSensorBayerContext);
    return NV_FALSE;
}


/*
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

/*
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

/*
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

/*
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

    NvOdmOsDebugPrintf("Setting resolution to %dx%d\n",
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
    Status = WriteI2CSequence(&pContext->I2c,
                 g_SensorBayerSetModeSequenceList[Index].pSequence, NV_FALSE);
    if (!Status)
        return NV_FALSE;

    /* On Sony Sharp 8MP sensor, it was found out that we need to wait
       for 2 frames based on the frame rate which is the minimum of the
       previous frame rate and current frame rate after writing the i2c
       init sequence to get stable output.
       Please check the spec for the needed delay.
    */

    /*
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

    pContext->LineLength = SENSOR_BAYER_DEFAULT_LINE_LENGTH;
    pContext->FrameLength = SENSOR_BAYER_DEFAULT_FRAME_LENGTH;
    pContext->CoarseTime = SENSOR_BAYER_DEFAULT_EXPOSURE_COARSE;
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
    pContext->MinFrameLength = SENSOR_BAYER_DEFAULT_MIN_FRAME_LENGTH;

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

    if (pResult)
    {
        pResult->Resolution = g_SensorBayerSetModeSequenceList[Index].
                              Mode.ActiveDimensions;
        pResult->Exposure = pContext->Exposure;
        NvOdmOsMemcpy(pResult->Gains, &pContext->Gains, sizeof(NvF32) * 4);
    }

    /*
     * Phase 2 ends here.
     */

    pContext->ModeIndex = Index;

    return Status;
}


/*
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
            NVODM_WRITEREG_RETURN_ON_ERROR(&pContext->I2c, 0x0100, 0);

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

/*
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

            // Sensor bayer doesn't support optimized resolution change.
            if (*((NvBool*)pValue) == NV_FALSE)
                return NV_TRUE;
            else
                return NV_FALSE;

        // This is optional but nice to have.
        case NvOdmImagerParameter_SelfTest:
            // Not Implemented.
            break;
        // Phase 3.
        default:
            NV_ASSERT(!"Not Implemented");
            break;
    }

    return NV_TRUE;
}

/*
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
                    NvOdmImagerI2cRead16(pI2c, 0x0000, &pStatus->Values[0]);
                    NvOdmImagerI2cRead16(pI2c, 0x303B, &pStatus->Values[1]);
                    NvOdmImagerI2cRead16(pI2c, 0x303C, &pStatus->Values[2]);
                    NvOdmImagerI2cRead16(pI2c, 0x306A, &pStatus->Values[3]);
                    pStatus->Count = 4;
                }
            }
            break;

        // Phase 1, it can return min = max = 10.0f
        //          (the fps in g_SensorBayerSetModeSequenceList)
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

            pFloatValue[0] = pContext->FrameRate;
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
        default:
            NV_ASSERT("!Not Implemented");
            Status = NV_FALSE;
    }

    return Status;
}

/*
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

/*
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
