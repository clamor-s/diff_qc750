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
#include <dlfcn.h>
#include <stdint.h>

#include "akm8975.h"

#include <cutils/log.h>

#include "AkmSensor.h"
#include "AkmAotController.h"

/*****************************************************************************/

#define AKM_DATA_NAME      "compass"

#define AKM_CMD_GET 1
#define AKM_CMD_SET 0

#define AKM_ENABLED_BITMASK_M 0x01
#define AKM_ENABLED_BITMASK_O 0x02

/*****************************************************************************/

AkmSensor::AkmSensor()
: SensorBase(NULL, AKM_DATA_NAME),
      mEnabled(0),
      mPendingMask(0),
      mInputReader(32)
{
    memset(mPendingEvents, 0, sizeof(mPendingEvents));

    mPendingEvents[MagneticField].version = sizeof(sensors_event_t);
    mPendingEvents[MagneticField].sensor = ID_M;
    mPendingEvents[MagneticField].type = SENSOR_TYPE_MAGNETIC_FIELD;
    mPendingEvents[MagneticField].magnetic.status = SENSOR_STATUS_ACCURACY_HIGH;

    mPendingEvents[Orientation  ].version = sizeof(sensors_event_t);
    mPendingEvents[Orientation  ].sensor = ID_O;
    mPendingEvents[Orientation  ].type = SENSOR_TYPE_ORIENTATION;
    mPendingEvents[Orientation  ].orientation.status = SENSOR_STATUS_ACCURACY_HIGH;

    for (int i=0 ; i<numSensors ; i++)
        mDelays[i] = -1; // Disable by default

	for (int i=0 ; i<numAccelAxises ; ++i)
		mAccels[i] = 0;

    // read the actual value of all sensors if they're enabled already
    struct input_absinfo absinfo;

    if (is_sensor_enabled(SENSOR_TYPE_MAGNETIC_FIELD))  {
        mEnabled |= AKM_ENABLED_BITMASK_M;
        if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_MAGV_X_K), &absinfo)) {
            mPendingEvents[MagneticField].magnetic.x = absinfo.value * CONVERT_M_K;
        }
        if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_MAGV_Y_K), &absinfo)) {
            mPendingEvents[MagneticField].magnetic.y = absinfo.value * CONVERT_M_K;
        }
        if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_MAGV_Z_K), &absinfo)) {
            mPendingEvents[MagneticField].magnetic.z = absinfo.value * CONVERT_M_K;
        }
        if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_MAGV_STATUS_K), &absinfo)) {
            mPendingEvents[MagneticField].magnetic.status = uint8_t(absinfo.value & SENSOR_STATE_MASK);
        }
    }

    if (is_sensor_enabled(SENSOR_TYPE_ORIENTATION))  {
        mEnabled |= AKM_ENABLED_BITMASK_M;
        if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_YAW_K), &absinfo)) {
            mPendingEvents[Orientation].orientation.azimuth = absinfo.value * CONVERT_O_K;
        }
        if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_PITCH_K), &absinfo)) {
            mPendingEvents[Orientation].orientation.pitch = absinfo.value * CONVERT_O_K;
        }
        if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ROLL_K), &absinfo)) {
            mPendingEvents[Orientation].orientation.roll = -absinfo.value * CONVERT_O_K;
        }
        if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ORIENT_STATUS_K), &absinfo)) {
            mPendingEvents[Orientation].orientation.status = uint8_t(absinfo.value & SENSOR_STATE_MASK);
        }
    }

}

AkmSensor::~AkmSensor()
{
}

int AkmSensor::enable(int32_t handle, int en)
{
    uint32_t mask;
    uint32_t sensor_type;

    switch (handle) {
        case ID_M:
            mask = AKM_ENABLED_BITMASK_M;
            sensor_type = SENSOR_TYPE_MAGNETIC_FIELD;
            break;
        case ID_O:
            mask = AKM_ENABLED_BITMASK_O;
            sensor_type = SENSOR_TYPE_ORIENTATION;
            break;
        default: return -EINVAL;
    }

	int err = 0;
    uint32_t newState  = (mEnabled & ~mask) | (en ? mask : 0);

	if (mEnabled != newState) {
        if (en)
            err = enable_sensor(sensor_type);
        else
            err = disable_sensor(sensor_type);

        LOGE_IF(err, "Could not change sensor state (%s)", strerror(-err));
        if (!err) {
            mEnabled = newState;
            update_delay();
        }
    }

    return err;
}

int AkmSensor::setDelay(int32_t handle, int64_t ns)
{
    if (ns < 0)
        return -EINVAL;

    switch (handle) {
        case ID_M: 
    		mDelays[0] = ns;
			break;
        case ID_O: 
    		mDelays[1] = ns;
			break;
		default:
			return -EINVAL;
    }

	LOGD("setDelay: handle=%d, ns=%lld", handle, ns);
    return update_delay();
}

int AkmSensor::update_delay()
{
    if (mEnabled) {
		int64_t delay[3];
		// Magnetic sensor
		if (mEnabled & AKM_ENABLED_BITMASK_M) {
			delay[0] = mDelays[0];
		} else {
			delay[0] = -1;
		}
		// Acceleration sensor
		delay[1] = -1;
		// Orientation sensor
		if (mEnabled & AKM_ENABLED_BITMASK_O) {
			delay[2] = mDelays[1];
		} else {
			delay[2] = -1;
		}
		LOGD("update_delay:%lld,%lld,%lld",delay[0],delay[1],delay[2]);
		AkmAotController::getInstance().setDelay(delay);
    }
    return 0;
}

int AkmSensor::readEvents(sensors_event_t* data, int count)
{
    if (count < 1)
        return -EINVAL;

    ssize_t n = mInputReader.fill(data_fd);

    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS) {
            processEvent(event->code, event->value);
            mInputReader.next();
        } else if (type == EV_SYN) {
            int64_t time = timevalToNano(event->time);
            for (int j=0 ; count && mPendingMask && j<numSensors ; j++) {
                if (mPendingMask & (1<<j)) {
                    mPendingMask &= ~(1<<j);
                    mPendingEvents[j].timestamp = time;
                    if (0 != (mEnabled & (1 << j))) {
                        *data++ = mPendingEvents[j];
                        count--;
                        numEventReceived++;
                    }
                }
            }
            if (!mPendingMask) {
                mInputReader.next();
            }
        } else {
            LOGE("AkmSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
            mInputReader.next();
        }
    }
    return numEventReceived;
}

void AkmSensor::processEvent(int code, int value)
{
    switch (code) {
        case EVENT_TYPE_MAGV_X_K:
            mPendingMask |= 1<<MagneticField;
            mPendingEvents[MagneticField].magnetic.x = value * CONVERT_M_K;
            break;
        case EVENT_TYPE_MAGV_Y_K:
            mPendingMask |= 1<<MagneticField;
            mPendingEvents[MagneticField].magnetic.y = value * CONVERT_M_K;
            break;
        case EVENT_TYPE_MAGV_Z_K:
            mPendingMask |= 1<<MagneticField;
            mPendingEvents[MagneticField].magnetic.z = value * CONVERT_M_K;
            break;
        case EVENT_TYPE_MAGV_STATUS_K:
            mPendingMask |= 1<<MagneticField;
            mPendingEvents[MagneticField].magnetic.status =
                    uint8_t(value & SENSOR_STATE_MASK);
            break;
        case EVENT_TYPE_YAW_K:
            mPendingMask |= 1<<Orientation;
            mPendingEvents[Orientation].orientation.azimuth = value * CONVERT_O_K;
            break;
        case EVENT_TYPE_PITCH_K:
            mPendingMask |= 1<<Orientation;
            mPendingEvents[Orientation].orientation.pitch = value * CONVERT_O_K;
            break;
        case EVENT_TYPE_ROLL_K:
            mPendingMask |= 1<<Orientation;
            mPendingEvents[Orientation].orientation.roll = value * CONVERT_O_K;
            break;
        case EVENT_TYPE_ORIENT_STATUS_K:
            mPendingMask |= 1<<Orientation;
           	mPendingEvents[Orientation].orientation.status =
                    uint8_t(value & SENSOR_STATE_MASK);
            break;
    }
}

static AkmAotController::SensorType convert_sensor_type(uint32_t sensor_type) {
	switch (sensor_type) {
	case SENSOR_TYPE_MAGNETIC_FIELD:
		return AkmAotController::AKM_SENSOR_TYPE_MAGNETOMETER;
	case SENSOR_TYPE_ORIENTATION:
		return AkmAotController::AKM_SENSOR_TYPE_ORIENTATION;
	default:
		return AkmAotController::AKM_SENSOR_TYPE_UNKNOWN;
	}
}

int AkmSensor::is_sensor_enabled(uint32_t sensor_type) {
	return (false == AkmAotController::getInstance().getEnabled(convert_sensor_type(sensor_type))) ? 0 : 1;
}

int AkmSensor::enable_sensor(uint32_t sensor_type) {
	AkmAotController::getInstance().setEnabled(convert_sensor_type(sensor_type), true);
	return 0;
}

int AkmSensor::disable_sensor(uint32_t sensor_type) {
	AkmAotController::getInstance().setEnabled(convert_sensor_type(sensor_type), false);
	return 0;
}

int AkmSensor::set_delay(int64_t ns[3]) {
	AkmAotController::getInstance().setDelay(ns);
	return 0;
}

int AkmSensor::forwardEvents(sensors_event_t *data)
{
    if ((NULL == data) || (ID_A != data->sensor)) {
        return -EINVAL;
    }
    mAccels[AccelAxisX] = (int16_t)(data->acceleration.x / GRAVITY_EARTH * LSG_K);
    mAccels[AccelAxisY] = (int16_t)(data->acceleration.y / GRAVITY_EARTH * LSG_K);
    mAccels[AccelAxisZ] = (int16_t)(data->acceleration.z / GRAVITY_EARTH * LSG_K);
 
#if 0
    mAccels[AccelAxisX] = (int16_t)0;
    mAccels[AccelAxisY] = (int16_t)0;
    mAccels[AccelAxisZ] = (int16_t)720;
#endif 
	
	AkmAotController::getInstance().writeAccels(mAccels);
	return 0;
}

bool AkmSensor::isEnabled(int32_t handle) {
	switch (handle) {
        case ID_M:
            return (0 != (mEnabled & AKM_ENABLED_BITMASK_M)) ? true : false;
        case ID_O:
            return (0 != (mEnabled & AKM_ENABLED_BITMASK_O)) ? true : false;
        default:
            LOGE("Unkown handle(%d).", handle);
            return false;
    }
}

