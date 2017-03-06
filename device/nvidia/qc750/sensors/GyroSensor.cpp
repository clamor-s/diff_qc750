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
#include <utils/BitSet.h>
#include <cutils/properties.h>

#include "GyroSensor.h"
#include "MEMSAlgLib_Fusion.h"

#define FETCH_FULL_EVENT_BEFORE_RETURN 1
#define IGNORE_EVENT_TIME 350000000

#define INPUT_SYSFS_PATH_GYRO "/sys/class/i2c-adapter/i2c-0/0-0068/"

/*****************************************************************************/

GyroSensor::GyroSensor()
    : SensorBase(NULL, "l3g4200d_gyr"),
      mEnabled(0),
      mInputReader(4),
      mHasPendingEvent(false)
{
    memset(&mPendingEvent, 0, sizeof(mPendingEvent));

    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_GY;
    mPendingEvent.type = SENSOR_TYPE_GYROSCOPE;
    mPendingEvent.acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;

    if (data_fd >= 0) {
        char  buffer[PROPERTY_VALUE_MAX];
        property_get("ro.hardware.gyrosensor", buffer, INPUT_SYSFS_PATH_GYRO);
        strcpy(input_sysfs_path, buffer);
        input_sysfs_path_len = strlen(input_sysfs_path);
    } else {
    	LOGE("%s: data_fd=[%d]\n", __func__, data_fd);
    }
}

GyroSensor::~GyroSensor() {
    if (mEnabled) {
        enable(ID_GY, 0);
    }

}

int GyroSensor::enable(int32_t handle, int en) {
    if (en != mEnabled) {
        int fd;
        strcpy(&input_sysfs_path[input_sysfs_path_len], "enable");
        fd = open(input_sysfs_path, O_RDWR);
        //LOGD("GyroSensor::enable(en=%d) : fd=%d, path=%s", en, fd, input_sysfs_path);
        if (fd >= 0) {
            char buf[2];
            int err;
            buf[1] = 0;
            if (en) {
                buf[0] = '1';
                //mEnabledTime = getTimestamp() + IGNORE_EVENT_TIME;
            } else {
                android::BitSet32 bs(mEnabled);
            	if (bs.count() == 1)
            		buf[0] = '0';
            	else
            		buf[0] = '1';
            }
            err = write(fd, buf, sizeof(buf));
            close(fd);
            mEnabled = en;
            return 0;
        }
        return -1;
    }
    return 0;
}
bool GyroSensor::hasPendingEvents() const {
    return mHasPendingEvent;
}

int GyroSensor::setDelay(int32_t handle, int64_t delay_ns)
{
    LOGD("GyroSensor::setDelay(handle=%d, ns=%d)");
    int fd;
	int mDelay = delay_ns/1000;
    strcpy(&input_sysfs_path[input_sysfs_path_len], "pollrate_ms");
    fd = open(input_sysfs_path, O_RDWR);
    if (fd >= 0) {
        char buf[80];
        sprintf(buf, "%lld", delay_ns/1000000);
        write(fd, buf, strlen(buf)+1);
        close(fd);
        if(handle==ID_GY)
            return 0;
    }
    return -1;
}

int GyroSensor::readEvents(sensors_event_t* data, int count)
{
    //LOGD("*******************Gyro readEvents");
    //LOGD("count: %d, mHasPendingEvent: %d", count, mHasPendingEvent);
    static int64_t prev_time;
    int64_t time;

    if (count < 1)
        return -EINVAL;

    if (mHasPendingEvent) {
        mHasPendingEvent = false;
        mPendingEvent.timestamp = getTimestamp();
        *data = mPendingEvent;
        return mEnabled ? 1 : 0;
    }

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;
    float gyrox, gyroy, gyroz;

#if FETCH_FULL_EVENT_BEFORE_RETURN
again:
#endif
    while (count && mInputReader.readEvent(&event)) {

        //LOGD("GyroSensor::readEvents() coutn = %d, event->value = %f", count, event->value);
        int type = event->type;
        if (type == EV_ABS) {
            float value = event->value;
            if (event->code == EVENT_TYPE_GYRO_X) {
                gyrox = value;
            } else if (event->code == EVENT_TYPE_GYRO_Y) {
                gyroy = value;
            } else if (event->code == EVENT_TYPE_GYRO_Z) {
                gyroz = value;
            }
        }else if (type == EV_SYN) {
            NineAxisTypeDef nineInput;
            nineInput.ax =  1;
            nineInput.ay =  1;
            nineInput.az =  1000;
            nineInput.mx =  300;
            nineInput.my =  300;
            nineInput.mz =  300;
            nineInput.gx =  gyrox;
            nineInput.gy =  gyroy;
            nineInput.gz =  gyroz;

            nineInput.time = getTimestamp()/1000000;

            FusionTypeDef fusionData = MEMSAlgLib_Fusion_Update(nineInput);
            float offx, offy, offz;
            MEMSAlgLib_Fusion_Get_GyroOffset(&offx,&offy,&offz);
            LOGD("gyro offset: %f, %f, %f", offx, offy, offz);
            mPendingEvent.data[0] = (gyrox-offx) * CONVERT_GYRO_X;
            mPendingEvent.data[1] = (gyroy-offy) * CONVERT_GYRO_Y;
            mPendingEvent.data[2] = (gyroz-offz) * CONVERT_GYRO_Z;

            time = timevalToNano(event->time);

            if(mEnabled) {
                mPendingEvent.timestamp = time;

                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }

        }else {
            LOGE("GyroSensor: unknown event (type=%d, code=%d)", type, event->code);
        }
        mInputReader.next();
    }

#if FETCH_FULL_EVENT_BEFORE_RETURN
    /* if we didn't read a complete event, see if we can fill and
       try again instead of returning with nothing and redoing poll. */
    if (numEventReceived == 0 && mEnabled == 1) {
        n = mInputReader.fill(data_fd);
        if (n)
            goto again;
    }
#endif

    return numEventReceived;
}

