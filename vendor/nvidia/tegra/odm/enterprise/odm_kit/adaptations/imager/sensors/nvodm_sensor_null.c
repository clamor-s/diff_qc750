/*
 * Copyright (c) 2006-2011 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvodm_imager_int.h"
#include "nvodm_sensor_null.h"

/** Implementation NvOdmSensor driver for a fake vip sensor */
#define SENSOR_NULL_PIXTYPES_MAX 1
#define SENSOR_NULL_TIMINGS_MAX 1

#define EXPOSURE_MIN    1.0f
#define EXPOSURE_MAX    1.0f
#define GAIN_MIN        1.0f
#define GAIN_MAX        16.0f
#define FRAMERATE_MIN   1.0f
#define FRAMERATE_MAX   1.0f
#define CLOCK_MIN       9000.0f
#define CLOCK_MAX       166000.0f
#define FOCUS_MIN       0
#define FOCUS_MAX       8

#include "camera_host_calibration.h"

#define RISING NvOdmImagerSyncEdge_Rising
#define FALLING NvOdmImagerSyncEdge_Falling
NvOdmImagerCapabilities SensorNullBayerCaps =
{
    "NullBayer Sensor",
    NvOdmImagerSensorInterface_Parallel10,
    {
    NvOdmImagerPixelType_BayerRGGB,
    NvOdmImagerPixelType_BayerBGGR,
    NvOdmImagerPixelType_BayerGBRG,
    NvOdmImagerPixelType_BayerGRBG,
    },
    NvOdmImagerOrientation_0_Degrees,
    NvOdmImagerDirection_Away,
    120000, // minimum input clock in khz
    {{120000, 1.0}}, // uses 120MHz always
    {FALLING, RISING, NV_FALSE}, // parallel
    {0}, // serial
    { 32, 24 }, // blank time
    0, // preferred mode
    0ULL, // no focuser
    0, // FLASH_GUID, only set if flash device code is available
    NVODM_IMAGER_CAPABILITIES_END
};
NvOdmImagerCapabilities SensorNullYuvCaps =
{
    "NullYuv Sensor",
    NvOdmImagerSensorInterface_Parallel8,
    {
    NvOdmImagerPixelType_UYVY,
    NvOdmImagerPixelType_YVYU,
    NvOdmImagerPixelType_VYUY,
    NvOdmImagerPixelType_YUYV,
    },
    NvOdmImagerOrientation_0_Degrees,
    NvOdmImagerDirection_Away,
    120000, // minimum input clock in khz
    {{120000, 1.0}}, // uses 120MHz always
    {FALLING, RISING, NV_FALSE}, // parallel
    {0}, // serial
    { 0, 0 }, // blank time
    0, // preferred mode
    0ULL, // no focuser
    0, // FLASH_GUID, only set if flash device code is available
    NVODM_IMAGER_CAPABILITIES_END
};

NvOdmImagerSensorMode SensorNullModes[SENSOR_NULL_TIMINGS_MAX] =
{
    {{320, 240}, {0, 0} },
};

typedef struct {
    NvBool BayerMode;
    // A real sensor would read/write these values to the actual
    // sensor hardware, but for the fake sensors, simply store
    // it in a private structure.
    float exposure;
    float gain;
    float frameRate;
    float clock;
    NvU32 focusPosition;
} NullContext;

NvBool NullCommonOpen(NvOdmImagerDeviceHandle sensor, NvBool BayerMode);
NvBool NullCommonOpen(NvOdmImagerDeviceHandle sensor, NvBool BayerMode)
{
    NvOdmImagerDeviceContext *hSensor;
    NullContext *ctx;

    hSensor = (NvOdmImagerDeviceContext *)sensor;
    hSensor->i2c.DeviceAddr = SENSOR_NULL_DEVICE_ADDR;

    hSensor->GetCapabilities = Sensor_Null_GetCapabilities;
    hSensor->ListModes       = Sensor_Null_ListModes;
    hSensor->SetMode         = Sensor_Null_SetMode;
    hSensor->SetPowerLevel   = Sensor_Null_SetPowerLevel;
    hSensor->SetParameter    = Sensor_Null_SetParameter;
    hSensor->GetParameter    = Sensor_Null_GetParameter;
    hSensor->CurrentPowerLevel = NvOdmImagerPowerLevel_Off;
    ctx = (NullContext *)NvOdmOsAlloc(sizeof(NullContext));
    if (ctx == NULL) {
        return NV_FALSE;
    }
    ctx->BayerMode = BayerMode;
    ctx->exposure = EXPOSURE_MIN;
    ctx->gain = 1.0f;
    ctx->frameRate = FRAMERATE_MAX;
    ctx->clock = CLOCK_MAX;
    ctx->focusPosition = FOCUS_MIN;
    hSensor->prv = (void *)ctx;

    return NV_TRUE;
}

NvBool Sensor_NullYuv_Open(NvOdmImagerDeviceHandle sensor)
{
    return NullCommonOpen(sensor, NV_FALSE);
}
NvBool Sensor_NullBayer_Open(NvOdmImagerDeviceHandle sensor)
{
    return NullCommonOpen(sensor, NV_TRUE);
}

void Sensor_Null_Close(NvOdmImagerDeviceHandle sensor)
{
    NvOdmImagerDeviceContext *hSensor;
    NullContext *ctx;
    hSensor = (NvOdmImagerDeviceContext *)sensor;
    ctx = hSensor->prv;
    NvOdmOsFree(ctx);
}

void
Sensor_Null_GetCapabilities(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerCapabilities *capabilities)
{
    NvOdmImagerDeviceContext *hSensor;
    NullContext *ctx;
    hSensor = (NvOdmImagerDeviceContext *)sensor;
    ctx = hSensor->prv;
    if (ctx->BayerMode)
        NvOdmOsMemcpy(capabilities, &SensorNullBayerCaps, sizeof(SensorNullBayerCaps));
    else
        NvOdmOsMemcpy(capabilities, &SensorNullYuvCaps, sizeof(SensorNullYuvCaps));
}

void
Sensor_Null_ListModes(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerSensorMode *mode,
        NvS32 *NumberOfModes)
{
    NvU32 count = SENSOR_NULL_TIMINGS_MAX;
    NvU32 size = count * sizeof (NvOdmImagerSensorMode);
    NvOdmImagerDeviceContext *hSensor;
    NullContext *ctx;

    hSensor = (NvOdmImagerDeviceContext *)sensor;
    ctx = hSensor->prv;
    if (ctx->BayerMode)
    {
        // Pad for the demosaic
        SensorNullModes[0].ActiveStart.x = 10;
        SensorNullModes[0].ActiveStart.y = 10;
    }
    else
    {
        // No pad for null yuv
        SensorNullModes[0].ActiveStart.x = 0;
        SensorNullModes[0].ActiveStart.y = 0;
    }
    if (NumberOfModes)
    {
        *NumberOfModes = count;
        if (mode)
            NvOdmOsMemcpy(mode, &SensorNullModes, size);
    }
}

NvBool Sensor_Null_SetMode(
    NvOdmImagerDeviceHandle sensor,
    const SetModeParameters *pParameters,
    NvOdmImagerSensorMode *pSelectedMode,
    SetModeParameters *pResult)
{
    NvOdmImagerDeviceContext *hSensor;
    NullContext *ctx;
    hSensor = (NvOdmImagerDeviceContext *)sensor;
    ctx = hSensor->prv;
    // In Null mode, we allow changing the resolution to anything
    SensorNullModes[0].ActiveDimensions = pParameters->Resolution;

#if 0
    if (ctx->BayerMode)
    {
        NvU32 Delta, Margin;
        // If insufficient Bayer blank time, adjust. 10% ?
        Delta = (SensorNullModes[0].ScanDimensions.width -
                 SensorNullModes[0].ActiveDimensions.width);
        Margin = (SensorNullModes[0].ActiveDimensions.width / 10);
        if (Delta < Margin)
        {
            SensorNullModes[0].ScanDimensions.width =
                SensorNullModes[0].ActiveDimensions.width + Margin;
        }
        Delta = (SensorNullModes[0].ScanDimensions.height -
                 SensorNullModes[0].ActiveDimensions.height);
        Margin = (SensorNullModes[0].ActiveDimensions.height / 10);
        if (Delta < Margin)
        {
            SensorNullModes[0].ScanDimensions.height =
                SensorNullModes[0].ActiveDimensions.height + Margin;
        }
    }
#endif

    if (pSelectedMode)
    {
        *pSelectedMode = SensorNullModes[0];
    }

    if (pResult)
    {
        NvU32 i;
        pResult->Resolution = SensorNullModes[0].ActiveDimensions;
        pResult->Exposure = ctx->exposure;

        for (i = 0; i < 4; i++)
            pResult->Gains[i] = ctx->gain;
    }

    return NV_TRUE;
}

NvBool Sensor_Null_SetPowerLevel(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerPowerLevel powerLevel)
{
    if (sensor == NULL)
    {
        return NV_FALSE;
    }

    return NV_TRUE;
}

NvBool Sensor_Null_SetParameter(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerParameter param,
        NvS32 sizeOfValue,
        const void *value)
{
    NvOdmImagerDeviceContext *hSensor;
    NullContext *ctx;
    const float *floatValue = value;
    const NvU32 *intValue = value;

    hSensor = (NvOdmImagerDeviceContext *)sensor;
    ctx = hSensor->prv;

    switch(param)
    {
        case NvOdmImagerParameter_SensorExposure:
            if (*floatValue > EXPOSURE_MAX ||
                *floatValue < EXPOSURE_MIN)
            {
                return NV_FALSE;
            }
            ctx->exposure = *floatValue;
            break;

        case NvOdmImagerParameter_SensorGain:
            if (*floatValue > GAIN_MAX ||
                *floatValue < GAIN_MIN)
            {
                return NV_FALSE;
            }
            ctx->gain = *floatValue;
            break;

        case NvOdmImagerParameter_SensorFrameRate:
            if (*floatValue > FRAMERATE_MAX ||
                *floatValue < FRAMERATE_MIN)
            {
                return NV_FALSE;
            }
            ctx->frameRate = *floatValue;
            break;

        case NvOdmImagerParameter_SensorInputClock:
            ctx->clock = (NvF32)((NvOdmImagerClockProfile *)value)->ExternalClockKHz;
            break;

        case NvOdmImagerParameter_FocuserLocus:
            if (*intValue >= FOCUS_MAX) {
                return NV_FALSE;
            }
            ctx->focusPosition = *intValue;
            break;

        case NvOdmImagerParameter_SelfTest:
            {
                NvOdmImagerSelfTest *pTest = (NvOdmImagerSelfTest *)value;
                switch (*pTest)
                {
                    case NvOdmImagerSelfTest_Existence:
                    case NvOdmImagerSelfTest_DeviceId:
                    case NvOdmImagerSelfTest_InitValidate:
                    case NvOdmImagerSelfTest_FullTest:
                        return NV_TRUE;
                    default:
                        return NV_FALSE;
                }
            }
            break;

        default:
            return NV_FALSE;
    }
    return NV_TRUE;
}

NvBool Sensor_Null_GetParameter(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerParameter param,
        NvS32 SizeOfValue,
        void *value)
{
    NvOdmImagerDeviceContext *hSensor;
    NullContext *ctx;
    float *floatValue = value;
    NvU32 *intValue = value;

    hSensor = (NvOdmImagerDeviceContext *)sensor;
    ctx = hSensor->prv;

    switch (param) {
        // Read/write params
        case NvOdmImagerParameter_SensorExposure:
            *floatValue = ctx->exposure;
            break;

        case NvOdmImagerParameter_SensorGain:
            *floatValue = ctx->gain;
            break;

        case NvOdmImagerParameter_SensorFrameRate:
            *floatValue = ctx->frameRate;
            break;

        case NvOdmImagerParameter_FocuserLocus:
            *intValue = ctx->focusPosition;
            break;

        // Read-only params
        case NvOdmImagerParameter_SensorExposureLimits:
            floatValue[0] = EXPOSURE_MIN;
            floatValue[1] = EXPOSURE_MAX;
            break;

        case NvOdmImagerParameter_SensorGainLimits:
            floatValue[0] = GAIN_MIN;
            floatValue[1] = GAIN_MAX;
            break;

        case NvOdmImagerParameter_SensorFrameRateLimits:
            floatValue[0] = FRAMERATE_MIN;
            floatValue[1] = FRAMERATE_MAX;
            break;

        case NvOdmImagerParameter_SensorClockLimits:
            floatValue[0] = CLOCK_MIN;
            floatValue[1] = CLOCK_MAX;
            break;

        case NvOdmImagerParameter_SensorExposureLatchTime:
            *intValue = 1;

        case NvOdmImagerParameter_CalibrationData:
        if (SizeOfValue == sizeof(NvOdmImagerCalibrationData)) {
            NvOdmImagerCalibrationData *pCalibration =
                (NvOdmImagerCalibrationData*) value;
            pCalibration->CalibrationData = sensorCalibrationData;
            pCalibration->NeedsFreeing = NV_FALSE;
        }
        else
        {
            return NV_FALSE;
        }
        break;

        case NvOdmImagerParameter_CalibrationOverrides:
        if (SizeOfValue == sizeof(NvOdmImagerCalibrationData))
        {
            // BKC TODO: Try to fstat the overrides file, allocate a buffer
            // of the appropriate size,
            NvOdmImagerCalibrationData *pCalibration =
                (NvOdmImagerCalibrationData*)value;
#if 1 // TODO
            pCalibration->CalibrationData = NULL;
            pCalibration->NeedsFreeing = NV_FALSE;
#else
            pCalibration->CalibrationData = the result of some fread;
            pCalibration->NeedsFreeing = NV_TRUE;
#endif
        }
        else
        {
            return NV_FALSE;
        }
        break;

        default:
            return NV_FALSE;
    }
    return NV_TRUE;
}

