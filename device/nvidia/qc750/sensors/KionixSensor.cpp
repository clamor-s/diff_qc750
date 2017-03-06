/*
 * Copyright (C) 2011 The Android Open Source Project
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

#include "KionixSensor.h"

#ifndef ID_M
#define ID_M 100
#endif
#ifndef ID_O
#define ID_O 101
#endif

/*****************************************************************************/
#define KIONIX_ENABLE_BITMASK_A 0x01 /* logical driver for accelerometer */
#define KIONIX_ENABLE_BITMASK_M 0x02 /* logical driver for magnetic field sensor */
#define KIONIX_ENABLE_BITMASK_O 0x04 /* logical driver for orientation sensor */
/*****************************************************************************/
#define KIONIX_UNIT_CONVERSION(value) ((value) * GRAVITY_EARTH / (1024.0f))
/*****************************************************************************/

KionixSensor::KionixSensor()
: SensorBase(NULL, "kionix_accel"),
      mEnabled(0),
      mInputReader(8),
      mHasPendingEvent(false)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_A;
    mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvent.acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

    if (data_fd) {
        strcpy(input_sysfs_path, "/sys/class/input/");
        strcat(input_sysfs_path, input_name);
        strcat(input_sysfs_path, "/device/");
        input_sysfs_path_len = strlen(input_sysfs_path);
    }

    for (int i=0 ; i<3 ; ++i)
        mDelay[i] = 200000000; // 200 ms by default
}

KionixSensor::~KionixSensor() {
    if (mEnabled) {
        enable(ID_A, 0);
        enable(ID_M, 0);
        enable(ID_O, 0);
    }
}

int KionixSensor::enable(int32_t handle, int en)
{
    uint32_t mask;

    switch (handle) {
    case ID_A:
        mask = KIONIX_ENABLE_BITMASK_A;
        break;
    case ID_M:
        mask = KIONIX_ENABLE_BITMASK_M;
        break;
    case ID_O:
        mask = KIONIX_ENABLE_BITMASK_O;
        break;
    default:
        LOGE("Unknown handle is passed to KionixSensor.");
        return -EINVAL;
    }

	int err = 0;
    uint32_t newState  = (mEnabled & ~mask) | (en ? mask : 0);

    if (mEnabled != newState) {
        if (newState && !mEnabled)
            err = enable_sensor();
        else if (!newState)
            err = disable_sensor();
        LOGE_IF(err, "Could not change sensor state \"%d -> %d\" (%s).", mEnabled, newState, strerror(-err));
        if (!err) {
            mEnabled = newState;
            update_delay();
        }
    }

    return err;
}

int KionixSensor::setDelay(int32_t handle, int64_t ns)
{
    if (ns < 0)
        return -EINVAL;

    int index = -1;
    switch (handle) {
    case ID_A:
        index = Accel;
        break;
    case ID_M:
        index = Mag;
        break;
    case ID_O:
        index = Ori;
        break;
    default:
        LOGE("Unknown handle(%d).", handle);
        return -EINVAL;
    }

    mDelay[index] = ns;
    return update_delay();
}

int KionixSensor::update_delay()
{
	int fd;

    if (mEnabled) {
        int64_t wanted = 0x7FFFFFFF;
        for (int i=0 ; i<numChannels ; ++i) {
            if (0 != (mEnabled & (1<<i))) {
                int64_t const ns = mDelay[i];
                wanted = (wanted) < ns ? wanted : ns;
            }
        }
        strcpy(&input_sysfs_path[input_sysfs_path_len], "device/delay");
        fd = open(input_sysfs_path, O_WRONLY);
        if (fd >= 0) {
            char buf[10];
            sprintf(buf, "%d", (int)(wanted/1000000));
            write(fd, buf, strlen(buf)+1);
            close(fd);
            return 0;
        }

        return -1;
    }
    return 0;
}

bool KionixSensor::hasPendingEvents() const {
    return mHasPendingEvent;
}

int KionixSensor::readEvents(sensors_event_t* data, int count)
{
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

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS) {
            switch (event->code) {
                case EVENT_TYPE_ACCEL_X_K:
                    mPendingEvent.acceleration.x = KIONIX_UNIT_CONVERSION(event->value);
                    break;
                case EVENT_TYPE_ACCEL_Y_K:
                    mPendingEvent.acceleration.y = KIONIX_UNIT_CONVERSION(event->value);
                    break;
                case EVENT_TYPE_ACCEL_Z_K:
                    mPendingEvent.acceleration.z = KIONIX_UNIT_CONVERSION(event->value);
                    break;
            }
        } else if (type == EV_SYN) {
            mPendingEvent.timestamp = timevalToNano(event->time);
            if (mEnabled) {
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }
        } else {
            LOGE("KionixSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}

int KionixSensor::enable_sensor() {
    int fd;
    int newState = 1;

	strcpy(&input_sysfs_path[input_sysfs_path_len], "device/enable");
    fd = open(input_sysfs_path, O_WRONLY);
    if (fd >= 0) {
        char buf[2];
        sprintf(buf, "%d", newState);
        write(fd, buf, strlen(buf)+1);
        close(fd);
        return 0;
    }
    return -1;
}

int KionixSensor::disable_sensor() {
    int fd;
    int newState = 0;

	strcpy(&input_sysfs_path[input_sysfs_path_len], "device/enable");
    fd = open(input_sysfs_path, O_WRONLY);
    if (fd >= 0) {
        char buf[2];
        sprintf(buf, "%d", newState);
        write(fd, buf, strlen(buf)+1);
        close(fd);
        return 0;
    }
    return -1;
}

bool KionixSensor::isEnabled(int32_t handle) {
	switch (handle) {
	case ID_A:
		return (0 != (mEnabled & KIONIX_ENABLE_BITMASK_A)) ? true : false;
	case ID_M:
		return (0 != (mEnabled & KIONIX_ENABLE_BITMASK_M)) ? true : false;
	case ID_O:
		return (0 != (mEnabled & KIONIX_ENABLE_BITMASK_O)) ? true : false;
	default:
		LOGE("Unknown handle(%d).", handle);
		return false;
	}
}
