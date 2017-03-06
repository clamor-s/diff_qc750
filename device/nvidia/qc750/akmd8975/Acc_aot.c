/******************************************************************************
 *
 * $Id: Acc_aot.c 43 2011-03-28 09:05:30Z yamada.rj $
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
#include "Acc_aot.h"
#include "AKCommon.h"       // DBGPRINT()

/* Initialize communication device. See "AK8975Driver.h" */
int16_t AOT_InitDevice(void)
{
	/* When this function is called, the device file is already opened. */
	return AKD_SUCCESS;
}

/* Release communication device and resources. See "AK8975Driver.h" */
void AOT_DeinitDevice(void)
{
	/* When this function is called, the device file is already closed. */
}

/* Get measurement data. See "AK8975Driver.h" */
int16_t AOT_GetAccelerationData(int16_t data[3])
{
	/* Get acceleration data from "/dev/akm8975_aot" */
	if (ioctl(s_fdDev, ECS_IOCTL_GET_ACCEL, data) < 0) {
		AKMERROR_STR("ioctl");
		return AKD_FAIL;
	}

	AKMDATA(AKMDATA_ACCDRV, "%s: acc=%d, %d, %d\n",
			__FUNCTION__, data[0], data[1], data[2]);

	return AKD_SUCCESS;
}
