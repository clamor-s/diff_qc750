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
#include <cutils/log.h>
#include <stdlib.h>
#include <linux/input.h>
#include <hardware/sensors.h>

#include "bma250.h"
#include "SensorUtil.h"

#define BMA250_DEV_PATH     "/dev/bma250"
#define BMA250_INTERVAL_MS  20

/*****************************************************************************/

Bma250::Bma250(int type)
    : SensorBase(BMA250_DEV_PATH, NULL),
      mEnabled(false),
      mHasPendingEvent(false),
      mLast_value(-1),
      mAlready_warned(true),
      sid(type)
{
	LOGD("Bma250 open");
	open_device();
}

Bma250::~Bma250() {
	//will be closed by other people
}

int Bma250::enable(int32_t handle, int en) {
    if (en != 0)
        mEnabled = true;
    else
        mEnabled = false;

   LOGD("Bma250 enable en=%d",en);
	
    return 0;
}

bool Bma250::hasPendingEvents() const {
    if(mEnabled)
        return true;
    else
        return false;
}

int Bma250::readEvents(sensors_event_t* data, int count) {
    static int log_count = 0;    
	static float value = -1.0f;    
	sensors_event_t evt;    
	if (count < 1 || data == NULL || !mEnabled)
		return 0;

	
	
	input_event const* event;
	short accRaw[3];
	int64_t curTime = getTimestamp();
	if(curTime - mCurrent_ns < BMA250_INTERVAL_MS * 1000000LL) 
		usleep(BMA250_INTERVAL_MS * 1000L - (curTime - mCurrent_ns) / 1000L);
	int n = read(dev_fd, accRaw, sizeof(accRaw));
	if (n <= 0 && mAlready_warned == false) {
		LOGE("AccelSensor: read from %s failed", BMA250_DEV_PATH); 
		mAlready_warned = false;        return 0;
		} 
	else if (n > 0 && mAlready_warned) {
		LOGI("AccelSensor: read from %s succeeded", BMA250_DEV_PATH);
		mAlready_warned = false;
	}
	
	evt.version = sizeof(sensors_event_t);
	evt.sensor = sid;
	evt.type = SENSOR_TYPE_ACCELEROMETER; 
	evt.acceleration.v[0] = accRaw[1] / 256.0f * GRAVITY_EARTH;
	evt.acceleration.v[1] = accRaw[0] / 256.0f * GRAVITY_EARTH;
	evt.acceleration.v[2] = -accRaw[2] / 256.0f * GRAVITY_EARTH;
	evt.acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
	evt.timestamp = mCurrent_ns = getTimestamp();
	*data = evt;

	LOGV(" readEvents x=%d, y=%d,z=%d\n",evt.acceleration.v[0],
		evt.acceleration.v[1],evt.acceleration.v[2]);

	return 1;
}

