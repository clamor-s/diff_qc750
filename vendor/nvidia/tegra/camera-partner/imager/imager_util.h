/*
 * Copyright (c) 2008-2012, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef IMAGER_UTIL_H
#define IMAGER_UTIL_H

#include "imager_hal.h"

#if defined(__cplusplus)
extern "C"
{
#endif


#define MAX_GPIO_PINS (2)
#define NVODM_IMAGER_I2C_PORT (1)
#define NVODM_IMAGER_I2C_MAX_BUFFER 8

#define SEQUENCE_WAIT_MS                (0xFFFE)
#define SEQUENCE_END                    (0xFFFF)
#define SEQUENCE_FAST_SETMODE_START     (0xFFFD)
#define SEQUENCE_FAST_SETMODE_END       (0xFFFC)

#define NVODM_WRITE_RETURN_ON_ERROR(_pI2C, _Reg, _Data) \
    do { \
        NvError RetType; \
        RetType = NvOdmImagerI2cWrite((_pI2C), (_Reg), (_Data)); \
        if (RetType == NvOdmI2cStatus_Timeout) \
        { \
            RetType = NvOdmImagerI2cWrite((_pI2C), (_Reg), (_Data)); \
        } \
        if (RetType != NvOdmI2cStatus_Success) \
        { \
            return NV_FALSE; \
        }}while(0);

#define CHECK_PARAM_SIZE_RETURN_MISMATCH(_s, _t) \
    if ((_s) != (_t)) \
    { \
        NV_ASSERT(!"Bad size"); \
        return NV_FALSE; \
    }

#define CHECK_SENSOR_RETURN_NOT_INITIALIZED(_pContext) \
    do { \
        if (!(_pContext)->SensorInitialized) \
        { \
            return NV_FALSE; \
        } \
    }while(0);


typedef struct NvOdmImagerI2cConnectionRec
{
    NvU32 DeviceAddr;
    NvU32 I2cPort;
    NvOdmServicesI2cHandle hOdmI2c;
    // The write and read routines need to know how to
    // interpret the address/data given.
    NvU8 AddressBitWidth;
    NvU8 DataBitWidth;
} NvOdmImagerI2cConnection;

typedef struct DevCtrlReg8Rec
{
    NvU8 RegAddr;
    NvU8 RegValue;
} DevCtrlReg8;

typedef struct DevCtrlReg16Rec
{
    NvU16 RegAddr;
    NvU16 RegValue;
} DevCtrlReg16;

typedef struct SensorSetModeSequenceRec
{
    NvOdmImagerSensorMode Mode;
    DevCtrlReg16 *pSequence;
    void *pModeDependentSettings;
} SensorSetModeSequence;

typedef enum {
    NvOdmImagerGpio_Reset = 0,
    NvOdmImagerGpio_Powerdown,
    NvOdmImagerGpio_FlashLow,
    NvOdmImagerGpio_FlashHigh,
    NvOdmImagerGpio_PWM,
    NvOdmImagerGpio_MCLK,
    NvOdmImagerGpio_Num,
    NvOdmImagerGpio_Force32 = 0x7FFFFFFF
} NvOdmImagerGpio;

typedef struct ImagerGpioRec
{
    NvU32 Port;
    NvU32 Pin;
    NvBool ActiveHigh;
    NvBool Enable;
    NvU32 HandleIndex;
} ImagerGpio;

typedef struct ImagerGpioConfigRec
{
    NvOdmServicesGpioHandle hGpio;
    ImagerGpio Gpios[NvOdmImagerGpio_Num];
    NvOdmGpioPinHandle hPin[MAX_GPIO_PINS];
    NvU32 PinMappings;
    NvU32 PinsUsed;
} ImagerGpioConfig;


NvBool
WriteI2CSequence(
    NvOdmImagerI2cConnection *pI2c,
    DevCtrlReg16 *pSequence,
    NvBool FastSetMode);

NvBool
WriteI2CSequenceOverride(
    NvOdmImagerI2cConnection *pI2c,
    DevCtrlReg16 *pSequence,
    NvBool FastSetMode,
    DevCtrlReg16 *pOverrides,
    NvU32 OverrideSize);



// Generalize i2c write
// NvU32's are used, but the
// NvOdmImagerI2cConnection structure
// gives the true bit widths in the
// AddressBitWidth and DataBitWidth fields.
NvOdmI2cStatus
NvOdmImagerI2cWrite(
   NvOdmImagerI2cConnection *pI2c,
   NvU32 Address,
   NvU32 Data);
// Generalized i2c read, same as write.
// Data width in pData is specified in the
// DataBitWidth field of the I2cConnection struct.
NvOdmI2cStatus
NvOdmImagerI2cRead(
   NvOdmImagerI2cConnection *pI2c,
   NvU32 Address,
   void *pData);

NvBool
SetPowerLevelWithPeripheralConnectivity(
    const NvOdmPeripheralConnectivity *pConnections,
    NvOdmImagerI2cConnection *pI2c,
    ImagerGpioConfig *pGpioConfig,
    NvOdmImagerPowerLevel PowerLevel);

NvBool
SetPowerLevelWithPeripheralConnectivityHelper(
    const NvOdmPeripheralConnectivity *pConnections,
    NvOdmImagerI2cConnection *pI2c,
    NvOdmImagerI2cConnection *pI2cPmu,
    ImagerGpioConfig *pGpioConfig,
    NvOdmImagerPowerLevel PowerLevel);

char*
LoadOverridesFile(
    const char *pFiles[],
    NvU32 len);

NvBool
LoadBlobFile(
    const char *pFiles[],
    NvU32 len,
    NvU8 *pBlob,
    NvU32 blobSize);

// Find sensor mode that matches requested mode most closely
// @param pRequest Requested mode
// @param pModeList Mode list to search
// @param NumModes Number of modes in list
// @return Index of selected mode
NvU32
NvOdmImagerGetBestSensorMode(
    const NvOdmImagerSensorMode* pRequest,
    const SensorSetModeSequence* pModeList,
    NvU32 NumModes);

#if defined(__cplusplus)
}
#endif

#endif  //IMAGER_UTIL_H
