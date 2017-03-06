/*
 * Copyright (c) 2007-2008 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVODM_DTVTUNER_INT_H
#define INCLUDED_NVODM_DTVTUNER_INT_H

// Module debug: 0=disable, 1=enable
#define NVODMTVTUNER_ENABLE_PRINTF (1)

#if (NV_DEBUG && NVODMTVTUNER_ENABLE_PRINTF)
#define NVODMTVTUNER_PRINTF(x)   NvOdmOsDebugPrintf x
#else
#define NVODMTVTUNER_PRINTF(x) 
#endif

#if defined(__cplusplus)
extern "C"
{
#endif

#define I2CSPEED 100
#define I2C_WAIT_LIMIT 100 // milliseconds, or NV_WAIT_INFINITE

/**
 * Enum the supported tv tuner device.
 */
typedef enum
{
    /** ODM tuner will do query and select the current tuner if there is one */ 
    NvOdmDtvtuner_Default = 0,

    /** Murata SUMUDDJ-LS057 ISDB-T tuner. */
    NvOdmDtvtuner_Murata,
} NvOdmDtvtunerDevice;

typedef struct I2CRegDataRec {
   NvU8 RegAddr;
   NvU8 RegValue;
} I2CRegData;

typedef struct NvOdmDeviceHWContextREC
{
    const NvOdmPeripheralConnectivity *pPeripheralConnectivity;
    NvOdmServicesPmuHandle hPmu;
    NvOdmServicesI2cHandle hI2C;
    NvOdmServicesI2cHandle hI2CPMU;
    NvOdmISDBTSegment      CurrentSeg;
    NvU32                  CurrentChannel;
    NvOdmServicesGpioHandle hGpio;
    NvOdmGpioPinHandle *pGpioPins;
} NvOdmDeviceHWContext;

typedef
void (*NvOdmHWSetPowerLevelFunc)(
        NvOdmDeviceHWContext *pHW,
        NvOdmDtvtunerPowerLevel PowerLevel);
typedef NvOdmHWSetPowerLevelFunc NvOdmHWSetPowerLevel;

typedef
NvBool (*NvOdmHWGetCapFunc)(
          NvOdmDeviceHWContext *pHW,
          NvOdmDtvtunerCap *pCap);
typedef NvOdmHWGetCapFunc NvOdmHWGetCap;

typedef
NvBool (*NvOdmHWInitFunc)(
          NvOdmDeviceHWContext *pHW,
          NvOdmISDBTSegment seg,
          NvU32 channel);
typedef NvOdmHWInitFunc NvOdmHWInit;

typedef
NvBool (*NvOdmHWSetSegmentChannelFunc)(
         NvOdmDeviceHWContext *pHW,
         NvOdmISDBTSegment seg,
         NvU32 channel,
         NvU32 PID);
typedef NvOdmHWSetSegmentChannelFunc NvOdmHWSetSegmentChannel;

typedef
NvBool (*NvOdmHWSetChannelFunc)(
         NvOdmDeviceHWContext *pHW,
         NvU32 channel,
         NvU32 PID);
typedef NvOdmHWSetChannelFunc NvOdmHWSetChannel;

typedef 
NvBool (*NvOdmHWGetSignalStatFunc)(
         NvOdmDeviceHWContext *pHW,
         NvOdmDtvtunerStat *SignalStat);
typedef NvOdmHWGetSignalStatFunc NvOdmHWGetSignalStat;

typedef struct NvOdmDtvtunerContextREC
{
    NvU64                       CurrnetGUID;
    NvOdmDtvtunerPowerLevel     CurrentPowerLevel;
    NvOdmISDBTSegment           CurrentSeg;
    NvU32                       CurrentChannel;
    
    // Point to tuner specific functions:
    NvOdmHWSetPowerLevel     SetPowerLevel;
    NvOdmHWGetCap            GetCap;
    NvOdmHWInit              Init;
    NvOdmHWSetSegmentChannel SetSegmentChannel;
    NvOdmHWSetChannel        SetChannel;
    NvOdmHWGetSignalStat     GetSignalStat;

    // Tuner HW specific context
    NvOdmDeviceHWContext HWctx;
}NvOdmDtvtunerContext;

//helper functions
NvOdmI2cStatus NvOdmTVI2cWrite8(
    NvOdmServicesI2cHandle hI2C,
    NvU8 devAddr,
    NvU8 regAddr,
    NvU8 Data);

NvOdmI2cStatus NvOdmTVI2cRead8(
    NvOdmServicesI2cHandle hI2C,
    NvU8 devAddr,
    NvU8 regAddr,
    NvU8 *Data);

#if defined(__cplusplus)
}
#endif

/** @} */

#endif // INCLUDED_NVODM_DTVTUNER_INT_H

