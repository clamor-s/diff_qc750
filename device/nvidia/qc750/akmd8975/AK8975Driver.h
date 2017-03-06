/******************************************************************************
 *
 * $Id: AK8975Driver.h 27 2011-03-23 07:31:31Z yamada.rj $
 *
 * -- Copyright Notice --
 *
 * Copyright (c) 2009 Asahi Kasei Microdevices Corporation, Japan
 * All Rights Reserved.
 *
 * This software program is proprietary program of Asahi Kasei Microdevices 
 * Corporation("AKM") licensed to authorized Licensee under Software License
 * Agreement (SLA) executed between the Licensee and AKM.
 *
 * Use of the software by unauthorized third party, or use of the software
 * beyond the scope of the SLA is strictly prohibited.
 *
 * -- End Asahi Kasei Microdevices Copyright Notice --
 *
 ******************************************************************************/
#ifndef AKMD_INC_AK8975DRIVER_H
#define AKMD_INC_AK8975DRIVER_H

#include "akm8975.h"	// Device driver
#include <stdint.h>		// int8_t, int16_t etc.

/*** Constant definition ******************************************************/
//#define BYTE		unsigned char	/*!< Unsigned 8-bit char */
#define AKD_TRUE	1		/*!< Represents true */
#define AKD_FALSE	0		/*!< Represents false */
#define AKD_SUCCESS	1		/*!< Represents success.*/
#define AKD_FAIL	0		/*!< Represents fail. */
#define AKD_ERROR	-1		/*!< Represents error. */

#define AKD_DBG_DATA	0	/*!< 0:Don't Output data, 1:Output data */
#define AK8975_MEASUREMENT_TIME	10000000	/*! Typical interval in ns */
#define AK8975_MEASURE_TIMEOUT	100

// 720 LSG = 1G = 9.8 m/s2
#define LSG			720


/*** Type declaration *********************************************************/
typedef unsigned char BYTE;

/*!
 Open device driver.
 This function opens device driver of acceleration sensor.
 @return If this function succeeds, the return value is #AKD_SUCCESS. Otherwise 
 the return value is #AKD_FAIL.
 */
typedef int16_t(*ACCFNC_INITDEVICE)(void);

/*!
 Close device driver.
 This function closes device drivers of acceleration sensor.
 */
typedef void(*ACCFNC_DEINITDEVICE)(void);

/*!
 Acquire acceleration data from acceleration sensor and convert it to Android 
 coordinate system.
 @return If this function succeeds, the return value is #AKD_SUCCESS. Otherwise 
 the return value is #AKD_FAIL.
 @param[out] data A acceleration data array. The coordinate system of the 
 acquired data follows the definition of Android. Unit is SmartCompass.
 */
typedef int16_t(*ACCFNC_GETACCDATA)(short data[3]);


/*** Global variables *********************************************************/

/*** Prototype of Function  ***************************************************/

int16_t AKD_InitDevice(void);

void AKD_DeinitDevice(void);

int16_t AKD_TxData(
	const BYTE address, 
	const BYTE* data, 
	const uint16_t numberOfBytesToWrite);

int16_t AKD_RxData(
	const BYTE address, 
	BYTE* data,
	const uint16_t numberOfBytesToRead);

int16_t AKD_GetMagneticData(BYTE data[SENSOR_DATA_SIZE]);

void AKD_SetYPR(const short buf[12]);

int AKD_GetOpenStatus(int* status);

int AKD_GetCloseStatus(int* status);

int16_t AKD_SetMode(const BYTE mode);

int16_t AKD_GetDelay(int64_t delay[3]);

int16_t AKD_GetLayout(int16_t* layout);

int16_t AKD_GetAccelerationData(int16_t data[3]);

#endif //AKMD_INC_AK8975DRIVER_H
