/*
 * Copyright (c) 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

/**
 * @file
 * <b>NVIDIA GoForce ODM Kit:
 *         Sensor Interface</b>
 *
 * @b Description: Defines the sensor interface as ODM Imager sub-device.
 *
 */


#ifndef INCLUDED_NVODM_IMAGER_DEVICE_H
#define INCLUDED_NVODM_IMAGER_DEVICE_H

#if defined(_cplusplus)
extern "C"
{
#endif

#include "nvcommon.h"
#include "nvodm_imager.h"
#include "nvodm_imager_i2c.h"
#include "nvodm_query_discovery.h"

typedef struct NvOdmImagerDeviceContextRec *NvOdmImagerDeviceHandle;

typedef struct ImagerGpioRec
{
    NvU32 port;
    NvU32 pin;
    NvBool activeHigh;
    NvBool enable;
} ImagerGpio;

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


// In order to avoid the pain to listing every single
// register that can be read out for debugging,
// we can group consecutive register regions
// common code can then set thru an array of these regions
// reading them out and giving them to the camera driver to spit out
// into the dng file or debugger window
typedef struct CompactRegisterListRec
{
    NvU16 Start;
    NvU16 Count;
} CompactRegisterList;

typedef struct NvOdmImagerDeviceContextRec {
    void    (*GetCapabilities)(
            NvOdmImagerDeviceHandle sensor,
            NvOdmImagerCapabilities *caps);
    void    (*ListModes)(
            NvOdmImagerDeviceHandle sensor,
            NvOdmImagerSensorMode *modes,
            NvS32 *NumberOfModes);
    NvBool  (*SetMode)(
            NvOdmImagerDeviceHandle sensor,
            const SetModeParameters *pParameters,
            NvOdmImagerSensorMode *pSelectedMode,
            SetModeParameters *pResult);
    NvBool  (*SetPowerLevel)(
            NvOdmImagerDeviceHandle sensor,
            NvOdmImagerPowerLevel PowerLevel);
    NvBool  (*SetParameter)(
            NvOdmImagerDeviceHandle sensor,
            NvOdmImagerParameter param,
            NvS32 SizeOfValue,
            const void *value);
    NvBool  (*GetParameter)(
            NvOdmImagerDeviceHandle sensor,
            NvOdmImagerParameter param,
            NvS32 SizeOfValue,
            void *value);

    NvOdmImagerI2cConnection i2c;

    NvU32 PinMappings;

    ImagerGpio Gpios[NvOdmImagerGpio_Num];

    NvOdmServicesGpioHandle GpioHandle;

    NvOdmImagerPowerLevel CurrentPowerLevel;
    NvOdmImagerSensorMode CurrentMode;
    NvOdmImagerPixelType CurrentPixelType;

    const NvOdmPeripheralConnectivity *connections;
    void *prv; // Used by each sensor to track its own state.
} NvOdmImagerDeviceContext;

typedef struct NvOdmImagerDeviceFptrsRec {
    NvU64 DeviceGUID;
    NvBool (*Open)(NvOdmImagerDeviceHandle);
    void (*Close)(NvOdmImagerDeviceHandle);
} NvOdmImagerDeviceFptrs, *NvOdmImagerDeviceTable;

/* NvOdmImagerCountSensorDevices is used to access the supported
 * sensor list, retrieving the count.
 */
NvS32 NvOdmImagerCountDevices(void);

/* NvOdmImagerGetSensorTable is used to access the supported
 * sensor list, retrieving the table of function ptrs for
 * open and close.
 */
void NvOdmImagerGetDeviceTable(NvOdmImagerDeviceTable *pTable);

void
NvOdmImagerGetDeviceFptrs(
    NvU64 GUID,
    NvOdmImagerDeviceFptrs *pDeviceFptrs,
    NvBool *VirtualDevice);

#if defined(_cplusplus)
}
#endif

/** @} */

#endif // INCLUDED_NVODM_IMAGER_DEVICE_H
