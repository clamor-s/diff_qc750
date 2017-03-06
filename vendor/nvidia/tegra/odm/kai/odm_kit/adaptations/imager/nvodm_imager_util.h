/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef NVODM_IMAGER_UTIL_H
#define NVODM_IMAGER_UTIL_H

#include "nvodm_imager_device.h"
#include "nvodm_imager_int.h"


#if defined(__cplusplus)
extern "C"
{
#endif

#define SEQUENCE_END                    (0xFFFF)
#define SEQUENCE_WAIT_MS                (0xFFFE)
#define SEQUENCE_FAST_SETMODE_START     (0xFFFD)
#define SEQUENCE_FAST_SETMODE_END       (0xFFFC)

#define NVODM_WRITEREG_RETURN_ON_ERROR(_pI2C, _Reg, _X) \
    do { \
        NvOdmI2cStatus RetType; \
        RetType = NvOdmImagerI2cWrite16((_pI2C), (_Reg), (_X)); \
        if (RetType == NvOdmI2cStatus_Timeout) \
        { \
            RetType = NvOdmImagerI2cWrite16((_pI2C), (_Reg), (_X)); \
        } \
        if (RetType != NvOdmI2cStatus_Success) \
        { \
            return NV_FALSE; \
        }}while(0);

#define NVODM_WRITE8_RETURN_ON_ERROR(_pI2C, _Reg, _X) \
    do { \
        NvOdmI2cStatus RetType; \
        RetType = NvOdmImagerI2cWrite16Data8((_pI2C), (_Reg), (NvU8)(_X)); \
        if (RetType == NvOdmI2cStatus_Timeout) \
        { \
            RetType = NvOdmImagerI2cWrite16Data8((_pI2C), (_Reg), (NvU8)(_X)); \
        } \
        if (RetType != NvOdmI2cStatus_Success) \
        { \
            return NV_FALSE; \
        }}while(0);

#define CHECK_PARAM_SIZE_RETURN_MISMATCH(_s, _t) \
    do { \
        if ((_s) != (_t)) \
        { \
            NV_ASSERT(!"Bad size"); \
            return NV_FALSE; \
        } \
    }while(0);

#define CHECK_SENSOR_RETURN_NOT_INITIALIZED(_pContext) \
    do { \
        if (!(_pContext)->SensorInitialized) \
        { \
            return NV_FALSE; \
        } \
    }while(0);

typedef struct SensorSetModeSequenceRec
{
    NvOdmImagerSensorMode Mode;
    DevCtrlReg16 *pSequence;
    void *pModeDependentSettings;
} SensorSetModeSequence;

typedef struct ImagerGpioConfigRec
{
    NvOdmServicesGpioHandle hGpio;
    ImagerGpio Gpios[NvOdmImagerGpio_Num];
    NvOdmGpioPinHandle hPin[MAX_GPIO_PINS];
    NvOdmImagerGpio PinType[MAX_GPIO_PINS];
    NvU32 PinMappings;
    NvU32 PinsUsed;
} ImagerGpioConfig;


NvBool
WriteI2CSequence8(
    NvOdmImagerI2cConnection *pI2c,
    DevCtrlReg16 *pSequence,
    NvBool FastSetMode);

NvBool
WriteI2CSequence(
    NvOdmImagerI2cConnection *pI2c,
    DevCtrlReg16 *pSequence);

NvBool
SetPowerLevelWithPeripheralConnectivity(
    const NvOdmPeripheralConnectivity *pConnections,
    NvOdmImagerI2cConnection *pI2c,
    ImagerGpioConfig *pGpioConfig,
    NvOdmImagerPowerLevel PowerLevel);


#if defined(__cplusplus)
}
#endif

#endif  //NVODM_IMAGER_UTIL_H
