/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVODM_IMAGER_INT_H
#define INCLUDED_NVODM_IMAGER_INT_H

#include "nvodm_services.h"

#include "nvodm_imager.h"
#include "nvodm_imager_i2c.h"
#include "nvodm_imager_device.h"
#include "nvodm_query_discovery.h"


// Module debug: 0=disable, 1=enable
#define NVODMIMAGER_ENABLE_PRINTF (0)

#if (NV_DEBUG && NVODMIMAGER_ENABLE_PRINTF)
#define NVODMSENSOR_PRINTF(x)   NvOdmOsDebugPrintf x
#else
#define NVODMIMAGER_PRINTF(x)
#endif

#if defined(__cplusplus)
extern "C"
{
#endif

#define NVODMIMAGER_MAX_SENSOR_MODES 8

typedef struct {
    NvOdmImagerDeviceMask  devices; /* Set of imager sub-devices */
    NvOdmImagerDeviceContext contexts[NvOdmImagerDevice_Num];
    NvOdmImagerDeviceFptrs   fptrs[NvOdmImagerDevice_Num];
} NvOdmImagerRec;

typedef struct DevCtrlRegRec {
   NvU32 RegAddr;
   NvU32 RegValue;
} DevCtrlReg;

typedef struct {
    const DevCtrlReg* Sequence;
    NvU32 Length;
} SensorTimingInitSequence;



typedef struct NvOdmImagerSensorShadowStruct {
    NvS32 LineLengthPck;
    NvS32 VtPixClkFreqHz;
    NvS32 FrameLengthLines;
    NvF32 LineLengthPck_F;
    NvF32 VtPixClkFreqHz_F;
    NvF32 FrameLengthLines_F;
    NvS32 MaxFrameLengthLines;
    NvS32 MinFrameLengthLines;
    NvU32 MaxSensorInputClock; // in KHz
    NvU32 MinSensorInputClock; // in KHz
    NvF32 MaxSensorGain;
    NvF32 MinSensorGain;
    NvF32 MaxExposure;
    NvF32 MinExposure;
    NvF32 MaxFrameRate;     // sensor's limit
    NvF32 MinFrameRate;
    NvF32 RequestedMaxFrameRate; // requested max frame rate. 0 means no limit
    NvU32 SensorInputClock;   // in KHz
    NvU32 SensorOutputClock;     // in KHz
    NvF32 SensorClockRatio;     // in KHz

    // Shadowed values currently programmed to the sensor:
    NvU32 currFocusPosition;
    NvF32 currExposure;
    NvF32 currGains[4];
    NvF32 currFrameRate;
    NvS32 currCoarse;
    NvU32 currPllPreDiv;
    NvU32 currPllMult;
    NvU32 currPllVtPixDiv;
    NvU32 currPllOpPixDiv;
    NvU32 currPllVtSysDiv;
    NvU32 currPllOpSysDiv;
    NvU32 currPllRowspeed;
} NvOdmImagerSensorShadow;

#define MAX_GPIO_PINS 8
typedef struct SensorContextStruct {
    NvU32 CurrentModeIndex;
    NvOdmServicesGpioHandle hGpio;
    NvOdmGpioPinHandle hPin[MAX_GPIO_PINS];
    NvU32 PinsUsed;
    NvF32 LastFrameRateUsed[NVODMIMAGER_MAX_SENSOR_MODES];

    NvF32 analogGainBalance[4];
    NvS32 groupParameterHold;

    NvBool TestPatternMode;

    NvOdmImagerSensorShadow shadow;
} SensorContext;

typedef struct DevCtrlReg16Rec
{
    NvU16 RegAddr;
    NvU16 RegValue;
} DevCtrlReg16;

typedef struct DevTestReg16Rec
{
    NvU16 RegAddr;
    NvU16 RegMask;
} DevTestReg16;

typedef struct DevCtrlReg8Rec
{
    NvU8 RegAddr;
    NvU8 RegValue;
} DevCtrlReg8;

typedef struct SensorInitSequenceRec
{
    NvU32 Width;
    NvU32 Height;
    DevCtrlReg16 *pInitSequence;
} SensorInitSequence;


#define NVODM_RETURN_ON_ERROR(X) \
    do { \
           if ((X) != NvOdmI2cStatus_Success) { \
               return NV_FALSE; \
           } \
    } while (0);

#define NVODM_GOTO_LABEL_ON_ERROR(LABEL, X) \
    do { \
           if ((X) != NvOdmI2cStatus_Success) { \
               goto LABEL; \
           } \
    } while (0);

#define NVODM_TIMEDIFF(from,to) \
           (((from) <= (to))? ((to)-(from)):((NvU32)~0-((from)-(to))+1))


//////////////////////////////////////////////////////////////////////////////
/*
 * Fixed point conversion for gain
 */
#define NVODM_SENSOR_AEFLOAT_BITS 3
// We assume sensor gain format is:
//           S{31-MI_AEFLOAT_BITS}.{MI_AEFLOAT_BITS}
#define NVODM_SENSOR_GAIN_TO_F32(x) \
        ((NvF32)(x) / (NvF32)(1<<NVODM_SENSOR_AEFLOAT_BITS))

#define NVODM_SENSOR_F32_TO_GAIN(x) \
        (NvS32)((NvF32)(x) * (NvF32)(1<<NVODM_SENSOR_AEFLOAT_BITS))
//////////////////////////////////////////////////////////////////////////////

#define RESOLUTION_MATCHES(_w, _h, _t) \
    (((_w) == (_t)->ActiveDimensions.width) && \
     ((_h) == (_t)->ActiveDimensions.height))
#define TIMINGS_MATCH(_t1, _t2) \
    (((_t1)->ActiveDimensions.width == (_t2)->ActiveDimensions.width) && \
     ((_t1)->ActiveDimensions.height == (_t2)->ActiveDimensions.height))


#if defined(__cplusplus)
}
#endif

/** @} */

#endif // INCLUDED_NVODM_IMAGER_INT_H

