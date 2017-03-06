/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * Copyright (c) 2011 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 *
 */

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>
#include <cutils/properties.h>

#include "MagnetoSensor.h"
#include "AccSensor.h"

#define FETCH_FULL_EVENT_BEFORE_RETURN 1
#define ENABLE_CALIBRATION		1
//#define INPUT_SYSFS_PATH_MAGNETO "/sys/class/i2c-adapter/i2c-0/0-001e/"
#define INPUT_SYSFS_PATH_MAGNETO "/sys/class/i2c-adapter/i2c-0/0-001e/"

#define XY_SENSITIVITY_1_3	1055	/* XY sensitivity at 1.3G */
#define XY_SENSITIVITY_1_9	795	/* XY sensitivity at 1.9G */
#define XY_SENSITIVITY_2_5	635	/* XY sensitivity at 2.5G */
#define XY_SENSITIVITY_4_0	430	/* XY sensitivity at 4.0G */
#define XY_SENSITIVITY_4_7	375	/* XY sensitivity at 4.7G */
#define XY_SENSITIVITY_5_6	320	/* XY sensitivity at 5.6G */
#define XY_SENSITIVITY_8_1	230	/* XY sensitivity at 8.1G */

/* Magnetometer Z sensitivity  */
#define Z_SENSITIVITY_1_3	950	/* Z sensitivity at 1.3G */
#define Z_SENSITIVITY_1_9	710	/* Z sensitivity at 1.9G */
#define Z_SENSITIVITY_2_5	570	/* Z sensitivity at 2.5G */
#define Z_SENSITIVITY_4_0	385	/* Z sensitivity at 4.0G */
#define Z_SENSITIVITY_4_7	335	/* Z sensitivity at 4.7G */
#define Z_SENSITIVITY_5_6	285	/* Z sensitivity at 5.6G */
#define Z_SENSITIVITY_8_1	205	/* Z sensitivity at 8.1G */

/*****************************************************************************/

MagnetoSensor::MagnetoSensor(AccSensor* as)
    : SensorBase(NULL, "lsm303dlh_mag"),
      //mEnabled(0),
      mInputReader(4),
      mHasPendingEvent(false),
      mAccSensor(as),
      mEnCount(0)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_M;
    mPendingEvent.type = SENSOR_TYPE_MAGNETIC_FIELD;
    mPendingEvent.magnetic.status = SENSOR_STATUS_ACCURACY_HIGH;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

    int mx, my, mz;

    if (data_fd) {
    char  buffer[PROPERTY_VALUE_MAX];
        property_get("ro.hardware.magsensor", buffer, INPUT_SYSFS_PATH_MAGNETO);
        strcpy(input_sysfs_path, buffer);
        input_sysfs_path_len = strlen(input_sysfs_path);

	MEMSAlgLib_eCompass_Init(1000,1000);

	//if find the last record of calibration data
	if(!read_sensor_calibration_data(&mx, &my, &mz)) //platform related API
	  MEMSAlgLib_eCompass_SetCalibration(mx, my, mz);

        /* enable(0, 1); */
    }
}

MagnetoSensor::~MagnetoSensor() {
    if (mEnCount) {
        enable(0, 0);
    }
}

/*
 * read/write calibration data in /data/system/sensors.dat
 */
int MagnetoSensor::read_sensor_calibration_data(int *mx, int *my, int *mz) {
  //unsigned long fd = fopen("/data/system/sensors.dat1", "r");
  FILE *fd = fopen("/data/system/sensors.dat1", "r");
  if (fd > 0)
  {
    if(fscanf(fd, "%d\n%d\n%d\n", mx, my, mz)==3)
    {
	LOGE("%s: valid calibration data, %d %d %d", __func__, *mx, *my, *mz);
	fclose(fd);
	return 0;
    }
    fclose(fd);
  }
  else
  {
      // sensors.dat1 not exited, create one
      write_sensor_calibration_data(-73,115,-264);
      return 0;
  }
  return -1;
}

int MagnetoSensor::write_sensor_calibration_data(int mx, int my, int mz) {
  FILE *fd = fopen("/data/system/sensors.dat1", "w");
  fprintf(fd, "%d\n%d\n%d\n", mx, my, mz);
  fclose(fd);
  LOGE("%s: valid calibration data, %d %d %d", __func__, mx, my, mz);
  return 0;
}

int MagnetoSensor::setInitialState() {
    struct input_absinfo absinfo_x;
    struct input_absinfo absinfo_y;
    struct input_absinfo absinfo_z;
    float value;
    if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_X), &absinfo_x) &&
        !ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Y), &absinfo_y) &&
        !ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Z), &absinfo_z)) {
        value = absinfo_y.value;
        mPendingEvent.magnetic.x= value * CONVERT_M_X;
        value = absinfo_x.value;
        mPendingEvent.magnetic.y= value * CONVERT_M_Y;
        value = absinfo_z.value;
        mPendingEvent.magnetic.z= value * CONVERT_M_Z;
        mHasPendingEvent = true;
    }
    return 0;
}

int MagnetoSensor::enable(int32_t, int en) {
    int oldCount = mEnCount;
    if(en)
        mEnCount++;
    else
        mEnCount--;
    if(mEnCount<0)
        mEnCount=0;

    int flags = -1;
    if(oldCount==0 && mEnCount>0)
        flags = 1;
    if(oldCount>0 && mEnCount==0)
        flags = 0;

    if (flags != -1) {
        int fd;
        strcpy(&input_sysfs_path[input_sysfs_path_len], "enable");
        fd = open(input_sysfs_path, O_RDWR);

        if (fd >= 0) {
            char buf[2];
            int err;
            buf[1] = 0;
            if (flags) {
                buf[0] = '1';
            } else {
                buf[0] = '0';
            }
            err = write(fd, buf, sizeof(buf));
            close(fd);
            //mEnabled = flags;
            setInitialState();
            return 0;
        }

        mAccSensor->enable(ID_A, flags);

        return -1;
    }
    return 0;
}

bool MagnetoSensor::hasPendingEvents() const {
    return mHasPendingEvent;
}

int MagnetoSensor::setDelay(int32_t handle, int64_t delay_ns)
{
    int fd;
    strcpy(&input_sysfs_path[input_sysfs_path_len], "pollrate_ms");
    fd = open(input_sysfs_path, O_RDWR);
    if (fd >= 0) {
        char buf[80];
        sprintf(buf, "%d", (int)(delay_ns / 1000000));
        write(fd, buf, strlen(buf)+1);
        close(fd);
        return 0;
    }
    return 0;
}

int MagnetoSensor::readEvents(sensors_event_t* data, int count)
{

    static ECOM_ValueTypeDef event_val;

    short MAGOFFX1,MAGOFFY1,MAGOFFZ1;
    short MAGOFFX2,MAGOFFY2,MAGOFFZ2;

    if (count < 1)
        return -EINVAL;

    if (mHasPendingEvent) {
        mHasPendingEvent = false;
        mPendingEvent.timestamp = getTimestamp();
        *data = mPendingEvent;
        return mEnCount ? 1 : 0;
    }

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

#if FETCH_FULL_EVENT_BEFORE_RETURN
again:
#endif
    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS) {
            float value = event->value;

            //calibrate here
            if (event->code == EVENT_TYPE_MAGV_X) {
                mPendingEvent.magnetic.x = value * CONVERT_M_X;
            } else if (event->code == EVENT_TYPE_MAGV_Y) {
                mPendingEvent.magnetic.y = value * CONVERT_M_Y;
            } else if (event->code == EVENT_TYPE_MAGV_Z) {
                mPendingEvent.magnetic.z = value * CONVERT_M_Z;
            }

#if ENABLE_CALIBRATION
            if (event->code == EVENT_TYPE_MAGV_X) {
                event_val.mx=value;
            } else if (event->code == EVENT_TYPE_MAGV_Y) {
                event_val.my=value;
            } else if (event->code == EVENT_TYPE_MAGV_Z) {
                event_val.mz=value;
            }

            sensors_event_t accData;
            mAccSensor->getAccData(&accData);
            event_val.ax = accData.acceleration.x*1024/GRAVITY_EARTH;
            event_val.ay = accData.acceleration.y*1024/GRAVITY_EARTH;
            event_val.az = accData.acceleration.z*1024/GRAVITY_EARTH;
            event_val.time = (signed long)(getTimestamp() / 1000000);

            if(MEMSAlgLib_eCompass_IsCalibrated()) {
                MEMSAlgLib_eCompass_GetCalibration(&MAGOFFX1,&MAGOFFY1,&MAGOFFZ1);
                //LOGE("%s cal: %d %d %d\n", __FUNCTION__, MAGOFFX1, MAGOFFY1, MAGOFFZ1);

                mPendingEvent.magnetic.x  = (event_val.mx-MAGOFFX1)* CONVERT_M_X;
                mPendingEvent.magnetic.y  = (event_val.my-MAGOFFY1)* CONVERT_M_Y;
                mPendingEvent.magnetic.z  = (event_val.mz-MAGOFFZ1)* CONVERT_M_Z;
            }

            MEMSAlgLib_eCompass_Update(&event_val);

            if(MEMSAlgLib_eCompass_IsCalibrated()) {
                MEMSAlgLib_eCompass_GetCalibration(&MAGOFFX2,&MAGOFFY2,&MAGOFFZ2);
                if( (MAGOFFX1!=MAGOFFX2) || (MAGOFFY1!=MAGOFFY2) || (MAGOFFZ1!=MAGOFFZ2) ) {
                    LOGE("%s write: %d %d %d\n", __FUNCTION__, MAGOFFX1, MAGOFFY1, MAGOFFZ1);
                    write_sensor_calibration_data((int)MAGOFFX2, (int)MAGOFFY2, (int)MAGOFFZ1);
                }
            }
#endif

        } else if (type == EV_SYN) {
            mPendingEvent.timestamp = timevalToNano(event->time);
            if (mEnCount) {
                *data++ = mPendingEvent;

                count--;
                numEventReceived++;
            }
        } else {
            LOGE("MagnetoSensor: unknown event (type=%d, code=%d)", type, event->code);
        }
        mInputReader.next();
    }

#if FETCH_FULL_EVENT_BEFORE_RETURN
    /* if we didn't read a complete event, see if we can fill and
       try again instead of returning with nothing and redoing poll. */
    if (numEventReceived == 0 && mEnCount>0) {
        n = mInputReader.fill(data_fd);
        if (n)
            goto again;
    }
#endif

    return numEventReceived;
}


