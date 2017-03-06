/*
 * Copyright (c) 2008 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "sensor_bayer_imx046es.h"
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
#define SENSOR_BAYER_DEFAULT_PLL_MULT            (0x001B)
#define SENSOR_BAYER_DEFAULT_PLL_PRE_DIV         (0x0001)
#define SENSOR_BAYER_DEFAULT_PLL_POS_DIV         (0x0001)
#define SENSOR_BAYER_DEFAULT_PLL_VT_PIX_DIV      (0x000A)
#define SENSOR_BAYER_DEFAULT_PLL_VT_SYS_DIV      (0x0001)

#define SENSOR_BAYER_DEFAULT_MAX_GAIN            (0x00e0)
#define SENSOR_BAYER_DEFAULT_MIN_GAIN            (0x0000)
#define SENSOR_BAYER_DEFAULT_MAX_FRAME_LENGTH    (0xffff)
#define SENSOR_BAYER_DEFAULT_MAX_EXPOSURE_COARSE (0xfffc)
#define SENSOR_BAYER_DEFAULT_MIN_EXPOSURE_COARSE (0x0001)

#define SENSOR_BAYER_DEFAULT_MAX_COARSE_DIFF     (3)

#define DIFF_INTEGRATION_TIME_OF_ADDITION_MODE    (0.11f)
#define DIFF_INTEGRATION_TIME_OF_ELIMINATION_MODE (0.23f)
#define DIFF_INTEGRATION_TIME_OF_MODE            DIFF_INTEGRATION_TIME_OF_ELIMINATION_MODE

#define SENSOR_GAIN_TO_F32(x)     (256.f / (256.f - (NvF32)(x)))
#define SENSOR_F32_TO_GAIN(x)     ((NvU32)(256.f - (256.f / (x))))

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
    "Sharp RJ63VC",  // string identifier, used for debugging
    NvOdmImagerSensorInterface_SerialA,
    {
    NvOdmImagerPixelType_BayerGRBG,
    },
    0, // focusing positions, ignored if FOCUSER_GUID is zero
    NvOdmImagerOrientation_180_Degrees, // rotation with respect to main display
    NvOdmImagerDirection_Away,          // toward or away the main display
    6000, // initial sensor clock (in khz)
    { {24000, 130.f/24.f}}, // preferred clock profile
    {
        NvOdmImagerSyncEdge_Rising, // hsync polarity
        NvOdmImagerSyncEdge_Rising, // vsync polarity
        NV_FALSE,                   // uses mclk on vgp0
    }, // parallel settings (VIP)
    { 
        NVODMSENSORMIPITIMINGS_PORT_CSI_A,
        NVODMSENSORMIPITIMINGS_DATALANES_TWO,
        NVODMSENSORMIPITIMINGS_VIRTUALCHANNELID_ONE,
        0x1, 
        0x2
    }, // serial settings (CSI) 
    { 16, 16 }, // min blank time, shouldn't need adjustment
    0, // preferred mode, which resolution to use on startup
    0, // FOCUSER_GUID, only set if focuser code available
    0, // FLASH_GUID, only set if flash device code is available
    NVODM_IMAGER_CAPABILITIES_END
};

/**
 * SetMode Sequence for 3264x2448. Phase 0. Sensor Dependent.
 * This sequence should put sensor in streaming mode for 2592x1944
 * This is usually given by the FAE or the sensor vendor.
 */
static DevCtrlReg16 SetModeSequence_3280X2464[] =
{
    {0x0307 , 0x1b},
    {0x30e5 , 0x04},
    {0x3300 , 0x00},
    {0x0101 , 0x03},
    {0x300a , 0x80},
    {0x3014 , 0x08},
    {0x3015 , 0x37},
    {0x3017 , 0x60},
    {0x301c , 0x01},
    {0x3031 , 0x28},
    {0x3040 , 0x00},
    {0x3041 , 0x60},
    {0x3051 , 0x24},
    {0x3053 , 0x34},
    {0x3055 , 0x3b},
    {0x3060 , 0x30},
    {0x3065 , 0x00},
    {0x30aa , 0x88},
    {0x30ab , 0x1c},
    {0x30b0 , 0x32},
    {0x30b2 , 0x83},
    {0x31a4 , 0xd8},
    {0x31a6 , 0x17},
    {0x31ac , 0xcf},
    {0x31ae , 0xf1},
    {0x31b4 , 0xd8},
    {0x31b6 , 0x17},
    {0x3302 , 0x0a},
    {0x3303 , 0x09},
    {0x3304 , 0x05},
    {0x3305 , 0x04},
    {0x3306 , 0x15},
    {0x3307 , 0x03},
    {0x3308 , 0x13},
    {0x3309 , 0x05},
    {0x330a , 0x0b},
    {0x330b , 0x04},
    {0x330c , 0x0b},
    {0x330d , 0x06},
    {0x0340 , 0x09},
    {0x0341 , 0xCE},
    {0x034E , 0x09},
    {0x034F , 0xA0},
    {0x0385 , 0x01},
    {0x0387 , 0x01},
    {0x3016 , 0x06},
    {0x0100 , 0x01},
    
    {SEQUENCE_END, 0x0000}
};

static DevCtrlReg16 SetModeSequence_3280X1232[] =
{
    {0x0307 , 0x1b},
    {0x302b , 0x4b},
    {0x30e5 , 0x04},
    {0x3300 , 0x00},
    {0x0101 , 0x03},
    {0x300a , 0x80},
    {0x3014 , 0x08},
    {0x3015 , 0x37},
    {0x3017 , 0x60},
    {0x301c , 0x01},
    {0x3031 , 0x28},
    {0x3040 , 0x00},
    {0x3041 , 0x60},
    {0x3051 , 0x24},
    {0x3053 , 0x34},
    {0x3055 , 0x3b},
    {0x3060 , 0x30},
    {0x3065 , 0x00},
    {0x30aa , 0x88},
    {0x30ab , 0x1c},
    {0x30b0 , 0x32},
    {0x30b2 , 0x83},
    {0x31a4 , 0xd8},
    {0x31a6 , 0x17},
    {0x31ac , 0xcf},
    {0x31ae , 0xf1},
    {0x31b4 , 0xd8},
    {0x31b6 , 0x17},
    {0x3302 , 0x0a},
    {0x3303 , 0x09},
    {0x3304 , 0x05},
    {0x3305 , 0x04},
    {0x3306 , 0x15},
    {0x3307 , 0x03},
    {0x3308 , 0x13},
    {0x3309 , 0x05},
    {0x330a , 0x0b},
    {0x330b , 0x04},
    {0x330c , 0x0b},
    {0x330d , 0x06},
    {0x0340 , 0x04},
    {0x0341 , 0xE6},
    {0x034E , 0x04},
    {0x034F , 0xD0},
    {0x0385 , 0x01},
    {0x0387 , 0x03},
    {0x3016 , 0x06},
    {0x0100 , 0x01},
    
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
     // WxH         x,y    fps    set mode sequence
     // Full-Size Mode
    {{{3280, 2464}, {0, 0}, 15,}, SetModeSequence_3280X2464, {0xd70, 0x9ce, 0x3e8, 0x9ce}},
    //1/2V mode. Need H-scaling down by VI or 2D
//    {{{3280, 1232}, {0, 0}, 30,}, SetModeSequence_3280X1232, {0xd70, 0x4e6, 0x3e8, 0x4e6}},
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
    NvF32 SensorClockRatio;    // pclk/mclk

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
    NvU32 PllPosDiv;
    NvU32 PllVtPixDiv;
    NvU32 PllVtSysDiv;

    NvU32 CoarseTime;
    NvU32 VtPixClkFreqHz;

    NvU32 LineLength;
    NvU32 FrameLength;

    NvU32 MaxFrameLength;
    NvU32 MinFrameLength;

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

    // Here, we have to decide if the new exposure time is valid
    // based on the sensor and current sensor settings.
    // Within smaller size mode, 0.23 should be changed to 0.11 if using V-addition calculation 
    NewCoarseTime = (NvU32)((Freq * ExpTime) / LineLength+DIFF_INTEGRATION_TIME_OF_MODE);
    if( NewCoarseTime == 0 ) NewCoarseTime = 1;
    NewFrameLength = NewCoarseTime + SENSOR_BAYER_DEFAULT_MAX_COARSE_DIFF;

    // Clamp to sensor's limit
    if (NewFrameLength > pContext->MaxFrameLength)
        NewFrameLength = pContext->MaxFrameLength;
    else if (NewFrameLength < pContext->MinFrameLength)
        NewFrameLength = pContext->MinFrameLength;
    
    // Need to update FrameLength?
    if (NewFrameLength != pContext->FrameLength)
    {
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x0340, (NewFrameLength >> 8) & 0xFF);
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x0341, NewFrameLength & 0xFF);
        pContext->FrameLength = NewFrameLength;

        // Update variables depending on FrameLength.
        pContext->FrameRate = (NvF32)pContext->VtPixClkFreqHz /
                              (NvF32)(pContext->FrameLength * pContext->LineLength);
    }
    
    // FrameLength should have been updated but we have to check again
    // in case FrameLength gets clamped.
    if (NewCoarseTime >= pContext->FrameLength)
    {
        NewCoarseTime = pContext->FrameLength - SENSOR_BAYER_DEFAULT_MAX_COARSE_DIFF;
    }
    
    // Current CoarseTime needs to be updated?
    if (pContext->CoarseTime != NewCoarseTime)
    {
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x0104, 1);
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x0202, (NewCoarseTime >> 8) & 0xFF);
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x0203, NewCoarseTime & 0xFF);
        NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x0104, 0);

        // Calculate the new exposure based on the sensor and sensor settings.
        pContext->Exposure = ((NewCoarseTime-(NvF32)DIFF_INTEGRATION_TIME_OF_MODE) *
                              (NvF32)LineLength) / Freq;
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
            (NvU32)((pContext->SensorInputClockkHz * 1000 * pContext->PllMult*2) /  //2=channel
                    (pContext->PllPreDiv * pContext->PllPosDiv)/10);

        pContext->Exposure =
                  (((NvF32)pContext->CoarseTime-DIFF_INTEGRATION_TIME_OF_MODE) * 
                    (NvF32)pContext->LineLength) /(NvF32)pContext->VtPixClkFreqHz;
        pContext->MaxExposure =
                  (((NvF32)SENSOR_BAYER_DEFAULT_MAX_EXPOSURE_COARSE-DIFF_INTEGRATION_TIME_OF_MODE) *
                    (NvF32)pContext->LineLength ) / (NvF32)pContext->VtPixClkFreqHz;
        pContext->MinExposure =
                  (((NvF32)SENSOR_BAYER_DEFAULT_MIN_EXPOSURE_COARSE-DIFF_INTEGRATION_TIME_OF_MODE) *
                    (NvF32)pContext->LineLength ) / (NvF32)pContext->VtPixClkFreqHz;
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

    /**
     * the following is Phase 2.
     */
    pContext->SensorInitialized = NV_TRUE;

    // Update sensor context after set mode
    //NV_ASSERT(pContext->SensorInputClockkHz > 0);


    // These hardcoded numbers are from the set mode sequence and this formula
    // is based on this sensor. Sensor Dependent.
    pContext->PllMult = SENSOR_BAYER_DEFAULT_PLL_MULT;
    pContext->PllPreDiv = SENSOR_BAYER_DEFAULT_PLL_PRE_DIV;
    pContext->PllPosDiv = SENSOR_BAYER_DEFAULT_PLL_POS_DIV;
    pContext->PllVtPixDiv = SENSOR_BAYER_DEFAULT_PLL_VT_PIX_DIV;
    pContext->PllVtSysDiv = SENSOR_BAYER_DEFAULT_PLL_VT_SYS_DIV;
    pContext->VtPixClkFreqHz =
        (NvU32)((pContext->SensorInputClockkHz * 1000 * pContext->PllMult*2) /
                (pContext->PllPreDiv * pContext->PllPosDiv)/10);

    pModeSettings =
        (ModeDependentSettings*)&g_SensorBayerSetModeSequenceList[Index].ModeDependentSettings;

    pContext->LineLength = pModeSettings->LineLength;
    pContext->FrameLength = pModeSettings->FrameLength;
    pContext->CoarseTime = pModeSettings->CoarseTime;
    pContext->MaxFrameLength = SENSOR_BAYER_DEFAULT_MAX_FRAME_LENGTH;
    pContext->MinFrameLength = pModeSettings->MinFrameLength;

    pContext->Exposure    = 
              (((NvF32)pContext->CoarseTime-DIFF_INTEGRATION_TIME_OF_MODE) *
               (NvF32)pContext->LineLength) /(NvF32)pContext->VtPixClkFreqHz;
    pContext->MaxExposure =
              (((NvF32)SENSOR_BAYER_DEFAULT_MAX_EXPOSURE_COARSE-DIFF_INTEGRATION_TIME_OF_MODE) *
               (NvF32)pContext->LineLength ) / (NvF32)pContext->VtPixClkFreqHz;
    pContext->MinExposure =
              (((NvF32)SENSOR_BAYER_DEFAULT_MIN_EXPOSURE_COARSE-DIFF_INTEGRATION_TIME_OF_MODE) *
               (NvF32)pContext->LineLength ) / (NvF32)pContext->VtPixClkFreqHz;

    pContext->FrameRate =
              (NvF32)pContext->VtPixClkFreqHz /
              (NvF32)(pContext->FrameLength * pContext->LineLength);
    pContext->MaxFrameRate =
              (NvF32)pContext->VtPixClkFreqHz /
              (NvF32)(pContext->MinFrameLength * pContext->LineLength);
    pContext->MinFrameRate =
              (NvF32)pContext->VtPixClkFreqHz /
              (NvF32)(SENSOR_BAYER_DEFAULT_MAX_FRAME_LENGTH * pContext->LineLength);

    pContext->Gains[0] = 1.0;
    pContext->Gains[1] = 1.0;
    pContext->Gains[2] = 1.0;
    pContext->Gains[3] = 1.0;

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

        // This is optional but nice to have.
        case NvOdmImagerParameter_SelfTest:
            //TODO: Not Implemented.
            break;

        // Phase 3.
        case NvOdmImagerParameter_MaxSensorFrameRate:
            //TODO: Not Implemented.
            break;
        default:
            NV_ASSERT(!"Not Implemented");
            break;
    }

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
                    NvOdmImagerI2cRead(pI2c, 0x0002, &pStatus->Values[0]);
                    pStatus->Count = 1;
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

        // Get the override config data.
        case NvOdmImagerParameter_CalibrationOverrides:
            CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
                sizeof(NvOdmImagerCalibrationData));
            {
                // Try to fstat the overrides file, allocate a buffer
                // of the appropriate size, and read it in.
                // The name "Camera_overrides.isp" are reserved by nVidia, please don't remove them
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




