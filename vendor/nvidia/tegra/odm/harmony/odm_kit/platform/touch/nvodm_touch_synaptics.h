/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 * 
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVODM_TOUCH_SYNAPTICS_H
#define INCLUDED_NVODM_TOUCH_SYNAPTICS_H

#include "nvodm_touch_int.h"
#include "nvodm_services.h"

#if defined(__cplusplus)
extern "C"
{
#endif

// {Xmin, Ymin} = {0, 0}, {Xmax, Ymax} = {2238, 1024}
#define X_MIN                                       0x0
#define Y_MIN                                       0x0
#define X_MAX                                       0x8BE
#define Y_MAX                                       0x400

// 80 samples/sec can be received from the touchpad
#define SAMPLES_PER_SECOND                          80

#define SYNAPTICS_I2C_SPEED_KHZ                     40
#define SYNAPTICS_I2C_TIMEOUT                       NV_WAIT_INFINITE
#define SYNAPTICS_LOW_SAMPLE_RATE                   0    //80 reports per-second
#define SYNAPTICS_MAX_READ_BYTES                    16
#define SYNAPTICS_MAX_PACKET_SIZE                   8
#define SYNAPTICS_MAX_READS ((SYNAPTICS_MAX_READ_BYTES + (SYNAPTICS_MAX_PACKET_SIZE - 1)) / SYNAPTICS_MAX_PACKET_SIZE)
#define SYNAPTICS_BENCHMARK_SAMPLE                  0
#define SYNAPTICS_REPORT_2ND_FINGER_DATA            0
#define SYNAPTICS_SCREEN_ANGLE                      0   // 0=Landscape, 1=Portrait
#define SYNAPTICS_POR_DELAY                         100 // Delay after Power-On Reset

// synaptics Touchpad GUID in the query database
#define SYNAPTICS_TOUCH_DEVICE_GUID NV_ODM_GUID('s','y','n','t','o','u','c','h')

#define SYNAPTICS_WRITE(dev, reg, byte) Synaptics_WriteRegister(dev, reg, byte)
#define SYNAPTICS_READ(dev, reg, buffer, len) Synaptics_ReadRegisterSafe(dev, reg, buffer, len)
#define SYNAPTICS_DEBOUNCE_TIME_MS 0

/* have the maximum dynamic range by setting both the fields - SensorMaxXPos,
 * SensorMaxYPos to their maximum value: 4095
 */
#define SENSOR_MAX_X_Y_POS                          4095

/* Device Reset Command */
#define DEVICE_RESET                                0x1

/* Co-ordinate data length */
#define X_Y_COORDINATE_DATA_LENGTH                  5

typedef struct Synaptics_TouchDevice_Rec
{
    NvOdmTouchDevice OdmTouch;
    NvOdmTouchCapabilities Caps;
    NvOdmServicesI2cHandle hOdmI2c;
    NvOdmServicesGpioHandle hGpio;
    NvOdmServicesPmuHandle hPmu;
    NvOdmGpioPinHandle hPin;
    NvOdmServicesGpioIntrHandle hGpioIntr;
    NvOdmOsSemaphoreHandle hIntSema;
    NvU32 PrevFingers;
    NvU32 DeviceAddr;
    NvU32 SampleRate;
    NvU32 SleepMode;
    NvBool PowerOn;
    NvU32 ChipRevisionId; //Id=0x01:SYNAPTICS chip on Concorde1
                          //id=0x02:SYNAPTICS chip with updated firmware on Concorde2
    NvU32 I2cClockSpeedKHz;
    NvU32 VddId;
} Synaptics_TouchDevice;

/**
 * Gets a handle to the touch pad in the system.
 *
 * @param hDevice A pointer to the handle of the touch pad.
 * @return NV_TRUE if successful, or NV_FALSE otherwise.
 */
NvBool Synaptics_Open(NvOdmTouchDeviceHandle *hDevice);

/**
 *  Releases the touch pad handle.
 *
 * @param hDevice The touch pad handle to be released. If
 *     NULL, this API has no effect.
 */
void Synaptics_Close(NvOdmTouchDeviceHandle hDevice);

/**
 * Gets capabilities for the specified touch device.
 *
 * @param hDevice The handle of the touch pad.
 * @param pCapabilities A pointer to the targeted
 *  capabilities returned by the ODM.
 */
void
Synaptics_GetCapabilities(
        NvOdmTouchDeviceHandle hDevice,
        NvOdmTouchCapabilities* pCapabilities);

/**
 * Gets coordinate info from the touch device.
 *
 * @param hDevice The handle to the touch pad.
 * @param coord  A pointer to the structure holding coordinate info.
 * @return NV_TRUE if successful, or NV_FALSE otherwise.
 */
NvBool
Synaptics_ReadCoordinate(
        NvOdmTouchDeviceHandle hDevice,
        NvOdmTouchCoordinateInfo *pCoord);

/**
 * Hooks up the interrupt handle to the GPIO interrupt and enables the interrupt.
 *
 * @param hDevice The handle to the touch pad.
 * @param hInterruptSemaphore A handle to hook up the interrupt.
 * @return NV_TRUE if successful, or NV_FALSE otherwise.
 */
NvBool
Synaptics_EnableInterrupt(
        NvOdmTouchDeviceHandle hDevice,
        NvOdmOsSemaphoreHandle hInterruptSemaphore);

/**
 * Prepares the next interrupt to get notified from the touch device.
 *
 * @param hDevice A handle to the touch pad.
 * @return NV_TRUE if successful, or NV_FALSE otherwise.
 */
NvBool Synaptics_HandleInterrupt(NvOdmTouchDeviceHandle hDevice);

/**
 * Gets the touch ADC sample rate.
 *
 * @param hDevice A handle to the touch ADC.
 * @param pTouchSampleRate A pointer to the NvOdmTouchSampleRate stucture.
 *
 * @return NV_TRUE if successful, or NV_FALSE otherwise.
*/
NvBool
Synaptics_GetSampleRate(
        NvOdmTouchDeviceHandle hDevice,
        NvOdmTouchSampleRate* pTouchSampleRate);

/**
 * Sets the touch ADC sample rate.
 *
 * @param hDevice A handle to the touch ADC.
 * @param SampleRate 1 indicates high frequency, 0 indicates low frequency.
 *
 * @return NV_TRUE if successful, or NV_FALSE otherwise.
*/
NvBool Synaptics_SetSampleRate(NvOdmTouchDeviceHandle hDevice, NvU32 rate);

/**
 * Sets the touch panel power mode.
 *
 * @param hDevice A handle to the touch ADC.
 * @param mode The mode, ranging from full power to power off.
 *
 * @return NV_TRUE if successful, or NV_FALSE otherwise.
*/
NvBool
Synaptics_PowerControl(
        NvOdmTouchDeviceHandle hDevice,
        NvOdmTouchPowerModeType mode);

/**
 * Gets the touch panel calibration data.
 * This is optional as calibration may perform after the OS is up.
 * This is not required to bring up the touch panel.
 *
 * @param hDevice A handle to the touch panel.
 * @param NumOfCalibrationData Indicates the number of calibration points.
 * @param pRawCoordBuffer The collection of X/Y coordinate data.
 *
 * @return NV_TRUE if preset calibration data is required, or NV_FALSE otherwise.
 */
NvBool
Synaptics_GetCalibrationData(
        NvOdmTouchDeviceHandle hDevice,
        NvU32 NumOfCalibrationData,
        NvS32* pRawCoordBuffer);

/**
 * Powers the touch device on or off.
 *
 * @param hDevice A handle to the touch ADC.
 * @param OnOff  Specify 1 to power ON, 0 to power OFF.
*/
NvBool Synaptics_PowerOnOff(NvOdmTouchDeviceHandle hDevice, NvBool OnOff);

#if defined(__cplusplus)
}
#endif

#endif // INCLUDED_NVODM_TOUCH_SYNAPTICS_H
