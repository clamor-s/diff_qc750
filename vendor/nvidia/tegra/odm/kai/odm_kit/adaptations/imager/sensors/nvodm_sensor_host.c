/*
 * Copyright (c) 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvodm_imager_int.h"
#include "nvodm_sensor_host.h"

/** Implementation NvOdmSensor driver for HOST
 * Host Mode is when the user chooses to inject the
 * camera driver with buffers.  This is most useful
 * when sending Bayer data through ISP processing.
 */
#define SENSOR_HOST_PIXTYPES_MAX 2
#define SENSOR_HOST_TIMINGS_MAX 1

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


NvOdmImagerCapabilities SensorHostCaps =
{
    "Host",
    NvOdmImagerSensorInterface_Host,
    {
    NvOdmImagerPixelType_UYVY,
    NvOdmImagerPixelType_BayerRGGB,
    },
    NvOdmImagerOrientation_0_Degrees,
    NvOdmImagerDirection_Away,
    120000, // minimum input clock in khz
    {{120000, 1.0}}, // clocks are both 120MHz
    { 0 }, // parallel
    { 0 }, // serial
    { 48, 16 }, // padding for blank
    0, // preferred mode 0
    0ULL,
    0ULL,
    NVODM_IMAGER_CAPABILITIES_END
};

NvOdmImagerSensorMode SensorHostModes[SENSOR_HOST_TIMINGS_MAX] =
{
    {{320, 240}, {0, 0}}, //{48, 16}},
};

typedef struct {
    // A real sensor would read/write these values to the actual
    // sensor hardware, but for the fake sensors, simply store
    // it in a private structure.
    float exposure;
    float gain;
    float frameRate;
    float clock;
    NvU32 focusPosition;
} HostContext;

NvBool Sensor_Host_Open(NvOdmImagerDeviceHandle sensor)
{
    NvOdmImagerDeviceContext *hSensor;
    HostContext *ctx;

    hSensor = (NvOdmImagerDeviceContext *)sensor;
    hSensor->i2c.DeviceAddr = SENSOR_HOST_DEVICE_ADDR;
    hSensor->GetCapabilities = Sensor_Host_GetCapabilities;
    hSensor->ListModes       = Sensor_Host_ListModes;
    hSensor->SetMode         = Sensor_Host_SetMode;
    hSensor->SetPowerLevel   = Sensor_Host_SetPowerLevel;
    hSensor->SetParameter    = Sensor_Host_SetParameter;
    hSensor->GetParameter    = Sensor_Host_GetParameter;

    ctx = (HostContext*)NvOdmOsAlloc(sizeof(HostContext));
    if (ctx == NULL) {
        return NV_FALSE;
    }
    ctx->exposure = EXPOSURE_MIN;
    ctx->gain = 1.0f;
    ctx->frameRate = FRAMERATE_MAX;
    ctx->clock = CLOCK_MAX;
    ctx->focusPosition = FOCUS_MIN;
    hSensor->prv = (void *)ctx;

    return NV_TRUE;
}

void Sensor_Host_Close(NvOdmImagerDeviceHandle sensor)
{
    NvOdmImagerDeviceContext *hSensor =
                         (NvOdmImagerDeviceContext *)sensor;
    NvOdmOsFree(hSensor->prv);
    NvOdmI2cClose(hSensor->i2c.hOdmI2c);
}

void
Sensor_Host_GetCapabilities(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerCapabilities *capabilities)
{
    NvOdmOsMemcpy(capabilities, &SensorHostCaps, sizeof(SensorHostCaps));
}

void
Sensor_Host_ListModes(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerSensorMode *Modes,
        NvS32 *NumberOfModes)
{
    NvU32 count = SENSOR_HOST_TIMINGS_MAX;
    NvU32 size = count * sizeof (NvOdmImagerSensorMode);

    if (NumberOfModes)
    {
        *NumberOfModes = count;
        if (Modes)
            NvOdmOsMemcpy(Modes, &SensorHostModes, size);
    }
}

NvBool Sensor_Host_SetMode(
    NvOdmImagerDeviceHandle sensor,
    const SetModeParameters *pParameters,
    NvOdmImagerSensorMode *pSelectedMode,
    SetModeParameters *pResult)
{
    NvOdmImagerDeviceContext *hSensor;
    HostContext *ctx;

    hSensor = (NvOdmImagerDeviceContext *)sensor;
    ctx = hSensor->prv;

    // In host mode, we allow changing the resolution to anything
    SensorHostModes[0].ActiveDimensions = pParameters->Resolution;
    if (pSelectedMode)
    {
        *pSelectedMode = SensorHostModes[0];
    }

    if (pResult)
    {
        NvU32 i;
        pResult->Resolution = SensorHostModes[0].ActiveDimensions;
        pResult->Exposure = ctx->exposure;

        for (i = 0; i < 4; i++)
            pResult->Gains[i] = ctx->gain;
    }

    return NV_TRUE;
}

NvBool Sensor_Host_SetPowerLevel(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerPowerLevel powerLevel)
{
    return NV_TRUE;
}

NvBool Sensor_Host_SetParameter(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerParameter param,
        NvS32 sizeOfValue,
        const void *value)
{
    NvOdmImagerDeviceContext *hSensor;
    HostContext *ctx;
    const float *floatValue = value;
    const NvU32 *intValue = value;
    NvBool bStatus = NV_TRUE;

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
            if (*intValue > FOCUS_MAX) {
                // It can't be less than FOCUS_MIN since that's 0.
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
                        bStatus = NV_TRUE;
                    default:
                        bStatus = NV_FALSE;
                }
            }
            break;
        default:
            bStatus = NV_FALSE;
    }
    return bStatus;
}

NvBool Sensor_Host_GetParameter(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerParameter param,
        NvS32 SizeOfValue,
        void *value)
{
    NvOdmImagerDeviceContext *hSensor;
    HostContext *ctx;
    float *floatValue = value;
    NvU32 *intValue = value;
    NvBool Status = NV_TRUE;

    hSensor = (NvOdmImagerDeviceContext *)sensor;
    ctx = hSensor->prv;

    switch(param)
    {
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
            break;

        case NvOdmImagerParameter_RegionUsedByCurrentResolution:
            if (SizeOfValue == sizeof(NvOdmImagerRegion))
            {
                NvOdmImagerRegion *pRegion = (NvOdmImagerRegion*)value;
                pRegion->RegionStart.x = 0;
                pRegion->RegionStart.y = 0;
                pRegion->xScale = 1;
                pRegion->yScale = 1;
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;

        case NvOdmImagerParameter_CalibrationData:
        if (SizeOfValue == sizeof(NvOdmImagerCalibrationData))
        {
            NvOdmImagerCalibrationData *pCalibration =
                (NvOdmImagerCalibrationData*)value;
            pCalibration->CalibrationData = sensorCalibrationData;
            pCalibration->NeedsFreeing = NV_FALSE;
        }
        else
        {
            return NV_FALSE;
        }
        break;

        case NvOdmImagerParameter_CalibrationOverrides:
        //CHECK_PARAM_SIZE_RETURN_MISMATCH(SizeOfValue,
        //                                 sizeof(NvOdmImagerCalibrationData));
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
                (NvOdmImagerCalibrationData*)value;
            NvOdmOsStatType Stat;
            NvOdmOsFileHandle hFile = NULL;
            // use a tempString so we can properly respect const-ness
            char *pTempString;
            NvU32 i;

            NvOdmOsDebugPrintf("NvOdmSensor Host : Loading Overrides\n");

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
                    NV_ASSERT(
                              !"Failed to open a file that fstatted just fine");
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
        default:
            return NV_FALSE;
    }
    return NV_TRUE;
}

