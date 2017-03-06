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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>
#include <cutils/properties.h>

#include "AccSensor.h"

#define FETCH_FULL_EVENT_BEFORE_RETURN 1
#define INPUT_SYSFS_PATH_ACC "/sys/bus/i2c/devices/0-0018/"

/*****************************************************************************/

AccSensor::AccSensor()
      : SensorBase(NULL, "lis3dh_acc"),
      //mEnabled(0),
      mInputReader(4),
      mEnCount(0)
{
  memset(&mAccData, 0, sizeof(mAccData));

    char  buffer[PROPERTY_VALUE_MAX];

    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_A;
    mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvent.acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
    if (data_fd >= 0) {
        property_get("ro.hardware.accsensor", buffer, INPUT_SYSFS_PATH_ACC);
        strcpy(input_sysfs_path, buffer);
        input_sysfs_path_len = strlen(input_sysfs_path);
        LOGE("AccSensor: sysfs[%s]\n",input_sysfs_path);
        //enable(0, 1);
    } else {
    	LOGE("%s: data_fd=[%d]\n", __func__, data_fd);
    }
}

AccSensor::~AccSensor() {

}

int AccSensor::setInitialState() {
    struct input_absinfo absinfo_x;
    struct input_absinfo absinfo_y;
    struct input_absinfo absinfo_z;
    float value;
    if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_X), &absinfo_x) &&
        !ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Y), &absinfo_y) &&
        !ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Z), &absinfo_z)) {
        value = absinfo_x.value;
        mPendingEvent.acceleration.x = value * CONVERT_A_X;
        value = absinfo_y.value;
        mPendingEvent.acceleration.y = value * CONVERT_A_Y;
        value = absinfo_z.value;
        mPendingEvent.acceleration.z = value * CONVERT_A_Z;
        mHasPendingEvent = true;
    }
    return 0;
}

int AccSensor::enable(int32_t handle, int en) {
    int oldCount = mEnCount;

LOGE("AccSensor: enable!");	
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
        strcpy(&input_sysfs_path[input_sysfs_path_len], "enable_device");

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
            //setInitialState();
            return 0;
        }
		LOGE("AccSensor:  open fd file\n!");	
      return -1;
    }
    return 0;
}

bool AccSensor::hasPendingEvents() const {
    return mHasPendingEvent;
}

int AccSensor::setDelay(int32_t handle, int64_t delay_ns)
{
    int fd;
    strcpy(&input_sysfs_path[input_sysfs_path_len], "pollrate_ms");
    fd = open(input_sysfs_path, O_RDWR);
    if (fd >= 0) {
        char buf[80];
	 LOGE("AccSensor: success!\n");	
        sprintf(buf, "%lld", delay_ns/1000000);
        write(fd, buf, strlen(buf)+1);
        close(fd);
        return 0;
    }
	LOGE("AccSensor: fail!\n");	
    return -1;
}

int AccSensor::readEvents(sensors_event_t* data, int count)
{

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
            if (event->code == EVENT_TYPE_ACCEL_X) {
                mPendingEvent.acceleration.x = value * CONVERT_A_X;
            } else if (event->code == EVENT_TYPE_ACCEL_Y) {
                mPendingEvent.acceleration.y = value * CONVERT_A_Y;
            } else if (event->code == EVENT_TYPE_ACCEL_Z) {
                mPendingEvent.acceleration.z = value * CONVERT_A_Z;  
            }
        } else if (type == EV_SYN) {
            mPendingEvent.timestamp = timevalToNano(event->time);
            if (mEnCount) {
                *data++ = mPendingEvent;
                mAccData = mPendingEvent;
                count--;
                numEventReceived++;
            }
        } else {
            LOGE("AccSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

#if FETCH_FULL_EVENT_BEFORE_RETURN
    /* if we didn't read a complete event, see if we can fill and
       try again instead of returning with nothing and redoing poll. */
    if (numEventReceived == 0 && mEnCount >0) {
        n = mInputReader.fill(data_fd);
        if (n)
            goto again;
    }
#endif

    return numEventReceived;
}

