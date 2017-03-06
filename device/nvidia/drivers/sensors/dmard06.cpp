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

#include "dmard06.h"
#include "SensorUtil.h"

#define DMARD06_DEV_PATH     "/dev/dmard06"
#define DMARD06_INTERVAL_MS  20

/*****************************************************************************/

Dmard06::Dmard06(int type)
    : SensorBase(DMARD06_DEV_PATH, NULL),
      mEnabled(false),
      mHasPendingEvent(false),
      mLast_value(-1),
      mAlready_warned(true),
      sid(type)
{
	LOGD("Dmard06 open");
	open_device();

	mCurrent_ns = getTimestamp();
	mDelay =20000;
}

Dmard06::~Dmard06() {
	//will be closed by other people
}

int Dmard06::enable(int32_t handle, int en) {
    if (en != 0)
        mEnabled = true;
    else
        mEnabled = false;

   LOGD("Dmard06 enable en=%d",en);
	
    return 0;
}

bool Dmard06::hasPendingEvents() const {
    LOGD("Dmard06 hasPendingEvents mEnabled=%d",mEnabled);
	
    if(mEnabled)
        return true;
    else
        return false;
}

int Dmard06::setDelay(int32_t handle, int64_t ns) {
    mDelay =ns;
    return ns;
}

int Dmard06::readEvents(sensors_event_t* data, int count) {
    static int log_count = 0;    
	static float value = -1.0f;    
	sensors_event_t evt;
	if (count < 1 || data == NULL || !mEnabled ||dev_fd <0)
		return 0;

	input_event const* event;
	short accRaw[3];
	int64_t curTime = getTimestamp();
	if(curTime - mCurrent_ns < DMARD06_INTERVAL_MS * 1000000LL) 
		usleep(DMARD06_INTERVAL_MS * 1000L - (curTime - mCurrent_ns) / 1000L);
	
	int n = read(dev_fd, accRaw, sizeof(accRaw));
	if (n <= 0 && mAlready_warned == false) {
		LOGE("AccelSensor: read from %s failed", DMARD06_DEV_PATH); 
		mAlready_warned = false;        return 0;
		} 
	else if (n > 0 && mAlready_warned) {
		LOGI("AccelSensor: read from %s succeeded", DMARD06_DEV_PATH);
		mAlready_warned = false;
	}
	
	evt.version = sizeof(sensors_event_t);
	evt.sensor = sid;
	evt.type = SENSOR_TYPE_ACCELEROMETER; 
	evt.acceleration.v[0] = -(float)accRaw[0] *(GRAVITY_EARTH / 32.0f ) /2;
	evt.acceleration.v[1] = -(float)accRaw[1] *(GRAVITY_EARTH / 32.0f ) /2;
	evt.acceleration.v[2] = (float)accRaw[2] *(GRAVITY_EARTH / 32.0f ) /2;
	evt.acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
	evt.timestamp = mCurrent_ns = getTimestamp();
	*data = evt;

	LOGV(" readEvents2 x=%d, y=%d,z=%d\n",(int)evt.acceleration.v[0],
		(int)evt.acceleration.v[1],(int)evt.acceleration.v[2]);

	return 1;
}

