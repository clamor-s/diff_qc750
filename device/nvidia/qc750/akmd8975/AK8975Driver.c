/******************************************************************************
 *
 * $Id: AK8975Driver.c 46 2011-03-29 09:46:42Z yamada.rj $
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
#include <fcntl.h>
#include "AKCommon.h"		// DBGPRINT()
#include "AK8975Driver.h"

#define MSENSOR_NAME      "/dev/akm8975_dev"

static int s_fdDev = -1;

/* Include proper acceleration file. */
#ifdef AKMD_ACC_COMBINED
#include "Acc_adxl34x.c"
static ACCFNC_INITDEVICE Acc_InitDevice = ADXL_InitDevice;
static ACCFNC_DEINITDEVICE Acc_DeinitDevice = ADXL_DeinitDevice;
static ACCFNC_GETACCDATA Acc_GetAccelerationData = ADXL_GetAccelerationData;
#else
#include "Acc_aot.c"
static ACCFNC_INITDEVICE Acc_InitDevice = AOT_InitDevice;
static ACCFNC_DEINITDEVICE Acc_DeinitDevice = AOT_DeinitDevice;
static ACCFNC_GETACCDATA Acc_GetAccelerationData = AOT_GetAccelerationData;
#endif

/*!
 Open device driver.
 This function opens both device drivers of magnetic sensor and acceleration
 sensor. Additionally, some initial hardware settings are done, such as
 measurement range, built-in filter function and etc.
 @return If this function succeeds, the return value is #AKD_SUCCESS. 
 Otherwise the return value is #AKD_FAIL.  
 */
int16_t AKD_InitDevice(void)
{
	if (s_fdDev < 0) {
		// Open magnetic sensor's device driver.
		if ((s_fdDev = open(MSENSOR_NAME, O_RDWR)) < 0) {
			AKMERROR_STR("open");
			return AKD_FAIL;
		}
	}
	
	if (Acc_InitDevice() != AKD_SUCCESS) {
		return AKD_FAIL;
	}
	
	return AKD_SUCCESS;
}

/*!
 Close device driver.
 This function closes both device drivers of magnetic sensor and acceleration
 sensor.
 */
void AKD_DeinitDevice(void)
{
	if (s_fdDev >= 0) {
		close(s_fdDev);
		s_fdDev = -1;
	}
	
	Acc_DeinitDevice();
}

/*!
 Writes data to a register of the AK8975.  When more than one byte of data is 
 specified, the data is written in contiguous locations starting at an address 
 specified in \a address.
 @return If this function succeeds, the return value is #AKD_SUCCESS. Otherwise 
 the return value is #AKD_FAIL. 
 @param[in] address Specify the address of a register in which data is to be 
 written.
 @param[in] data Specify data to write or a pointer to a data array containing 
 the data.  When specifying more than one byte of data, specify the starting 
 address of the array.
 @param[in] numberOfBytesToWrite Specify the number of bytes that make up the 
 data to write.  When a pointer to an array is specified in data, this argument 
 equals the number of elements of the array.
 */
int16_t AKD_TxData(const BYTE address,
				 const BYTE * data,
				 const uint16_t numberOfBytesToWrite)
{
	int i;
	char buf[RWBUF_SIZE];
	
	if (s_fdDev < 0) {
		LOGE("%s: Device file is not opened.", __FUNCTION__);
		return AKD_FAIL;
	}
	if (numberOfBytesToWrite > (RWBUF_SIZE-2)) {
		LOGE("%s: Tx size is too large.", __FUNCTION__);
		return AKD_FAIL;
	}

	buf[0] = numberOfBytesToWrite + 1;
	buf[1] = address;
	
	for (i = 0; i < numberOfBytesToWrite; i++) {
		buf[i + 2] = data[i];
	}
	if (ioctl(s_fdDev, ECS_IOCTL_WRITE, buf) < 0) {
		AKMERROR_STR("ioctl");
		return AKD_FAIL;
	} else {

#if ENABLE_AKMDEBUG
		AKMDATA(AKMDATA_MAGDRV, "addr(HEX)=%02x data(HEX)=", address);
		for (i = 0; i < numberOfBytesToWrite; i++) {
			AKMDATA(AKMDATA_MAGDRV, " %02x", data[i]);
		}
		AKMDATA(AKMDATA_MAGDRV, "\n");
#endif
		return AKD_SUCCESS;
	}
}

/*!
 Acquires data from a register or the EEPROM of the AK8975.
 @return If this function succeeds, the return value is #AKD_SUCCESS. Otherwise 
 the return value is #AKD_FAIL.
 @param[in] address Specify the address of a register from which data is to be 
 read.
 @param[out] data Specify a pointer to a data array which the read data are 
 stored.
 @param[in] numberOfBytesToRead Specify the number of bytes that make up the 
 data to read.  When a pointer to an array is specified in data, this argument 
 equals the number of elements of the array.
 */
int16_t AKD_RxData(const BYTE address, 
				 BYTE * data,
				 const uint16_t numberOfBytesToRead)
{
	int i;
	char buf[RWBUF_SIZE];
	
	memset(data, 0, numberOfBytesToRead);
	
	if (s_fdDev < 0) {
		LOGE("%s: Device file is not opened.", __FUNCTION__);
		return AKD_FAIL;
	}
	if (numberOfBytesToRead > (RWBUF_SIZE-1)) {
		LOGE("%s: Rx size is too large.", __FUNCTION__);
		return AKD_FAIL;
	}

	buf[0] = numberOfBytesToRead;
	buf[1] = address;
	
	if (ioctl(s_fdDev, ECS_IOCTL_READ, buf) < 0) {
		AKMERROR_STR("ioctl");
		return AKD_FAIL;
	} else {
		for (i = 0; i < numberOfBytesToRead; i++) {
			data[i] = buf[i + 1];
		}
#if ENABLE_AKMDEBUG
		AKMDATA(AKMDATA_MAGDRV, "addr(HEX)=%02x len=%d data(HEX)=",
				address, numberOfBytesToRead);
		for (i = 0; i < numberOfBytesToRead; i++) {
			AKMDATA(AKMDATA_MAGDRV, " %02x", data[i]);
		}
		AKMDATA(AKMDATA_MAGDRV, "\n");
#endif
		return AKD_SUCCESS;
	}
}

/*!
 Acquire magnetic data from AK8975. If measurement is not done, this function 
 waits until measurement completion.
 @return If this function succeeds, the return value is #AKD_SUCCESS. Otherwise 
 the return value is #AKD_FAIL.
 @param[out] data A magnetic data array. The size should be larger than #SENSOR_DATA_SIZE.
 */
int16_t AKD_GetMagneticData(BYTE data[SENSOR_DATA_SIZE])
{
	memset(data, 0, SENSOR_DATA_SIZE);
	
	if (s_fdDev < 0) {
		LOGE("%s: Device file is not opened.", __FUNCTION__);
		return AKD_FAIL;
	}
	
#if 0
	int i;
	// Wait until measurement done.
	msleep(AK8975_MEASUREMENT_TIME);
	
	for (i = 0; i < AK8975_MEASURE_TIMEOUT; i++) {
		if (AKD_RxData(AK8975_REG_ST1, data, 1) != AKD_SUCCESS) {
			return AKD_FAIL;
		}
		if (data[0] & 0x01) {
			if (AKD_RxData(AK8975_REG_ST1, data, SENSOR_DATA_SIZE) != AKD_SUCCESS) {
				return AKD_FAIL;
			}
			break;
		}
		usleep(1000);
	}
	if (i >= AK8975_MEASURE_TIMEOUT) {
		LOGE("%s: DRDY timeout.", __FUNCTION__);
		return AKD_FAIL;
	}
#else
	if (ioctl(s_fdDev, ECS_IOCTL_GETDATA, data) < 0) {
		AKMERROR_STR("ioctl");
		return AKD_FAIL;
	}
#endif
	
	AKMDATA(AKMDATA_MAGDRV, 
		"bdata(HEX)= %02x %02x %02x %02x %02x %02x %02x %02x\n",
		data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

	return AKD_SUCCESS;
}

/*!
 Set calculated data to device driver.
 @param[in] buf 
 */
void AKD_SetYPR(const short buf[12])
{
	if (ioctl(s_fdDev, ECS_IOCTL_SET_YPR, buf) < 0) {
		AKMERROR_STR("ioctl");
	}
#if 0
	/* reformat to old order */
	short tmp[12];
	tmp[0] = buf[9];
	tmp[1] = buf[10];
	tmp[2] = buf[11];
	tmp[3] = 25;
	tmp[4] = buf[4];
	tmp[5] = buf[8];
	tmp[6] = buf[5];
	tmp[7] = buf[6];
	tmp[8] = buf[7];
	tmp[9] = buf[1];
	tmp[10] = buf[2];
	tmp[11] = buf[3];
	if (ioctl(s_fdDev, ECS_IOCTL_SET_YPR, &tmp) < 0) {
		AKMERROR_STR("ioctl");
	}
#endif
}

/*!
 */
int AKD_GetOpenStatus(int* status)
{
	if (ioctl(s_fdDev, ECS_IOCTL_GET_OPEN_STATUS, status) < 0) {
		AKMERROR_STR("ioctl");
		return AKD_FAIL;
	}
	return AKD_SUCCESS;
}

/*!
 */
int AKD_GetCloseStatus(int* status)
{
	if (ioctl(s_fdDev, ECS_IOCTL_GET_CLOSE_STATUS, status) < 0) {
		AKMERROR_STR("ioctl");
		return AKD_FAIL;
	}
	return AKD_SUCCESS;
}

/*!
 Set AK8975 to the specific mode. 
 @return If this function succeeds, the return value is #AKD_SUCCESS. Otherwise 
 the return value is #AKD_FAIL.
 @param[in] mode This value should be one of the AK8975_Mode which is defined in
 akm8975.h file.
 */
int16_t AKD_SetMode(const BYTE mode)
{
	if (s_fdDev < 0) {
		LOGE("%s: Device file is not opened.", __FUNCTION__);
		return AKD_FAIL;
	}
	
	if (ioctl(s_fdDev, ECS_IOCTL_SET_MODE, &mode) < 0) {
		AKMERROR_STR("ioctl");
		return AKD_FAIL;
	}
	
	return AKD_SUCCESS;
}

/*!
 Acquire delay
 @return If this function succeeds, the return value is #AKD_SUCCESS. Otherwise 
 the return value is #AKD_FAIL.
 @param[out] delay A delay in microsecond.
 */
int16_t AKD_GetDelay(int64_t delay[3])
{
	if (s_fdDev < 0) {
		LOGE("%s: Device file is not opened.\n", __FUNCTION__);
		return AKD_FAIL;
	}
	if (ioctl(s_fdDev, ECS_IOCTL_GET_DELAY, delay) < 0) {
		AKMERROR_STR("ioctl");
		return AKD_FAIL;
	}
	return AKD_SUCCESS;
}

/*!
 Get layout information from device driver, i.e. platform data.
 */
int16_t AKD_GetLayout(int16_t* layout)
{
	//TODO: Implement in device driver
#if 0
	short tmp;

	if (s_fdDev < 0) {
		LOGE("%s: Device file is not opened.", __FUNCTION__);
		return AKD_FAIL;
	}
	
	if (ioctl(s_fdDev, ECS_IOCTL_GET_LAYOUT, &tmp) < 0) {
		AKMERROR_STR("ioctl");
		return AKD_FAIL;
	}
	
	*layout = tmp;
#else
	*layout = 1;
#endif
	return AKD_SUCCESS;
}

/* Get measurement data. See "AK8975Driver.h" */
int16_t AKD_GetAccelerationData(int16_t data[3])
{
	return Acc_GetAccelerationData(data);
}
