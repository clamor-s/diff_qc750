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

#ifndef ANDROID_Dmt_SENSOR_H
#define ANDROID_Dmt_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include "sensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

/*****************************************************************************/

struct input_event;

class DmtSensor : public SensorBase {
public:
            DmtSensor();
    virtual ~DmtSensor();
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int enable(int32_t handle, int enabled);
    virtual int readEvents(sensors_event_t* data, int count);
    virtual bool hasPendingEvents() const;
//    virtual bool isEnabled(int32_t handle);

private:
    enum {
        Accel = 0,
        Mag,
        Ori,
        numChannels
    };
    uint32_t mEnabled;
    InputEventCircularReader mInputReader;
    sensors_event_t mPendingEvent;
    bool mHasPendingEvent;
    int64_t mDelay[numChannels];
    char input_sysfs_path[PATH_MAX];
    int input_sysfs_path_len;

	int enable_sensor();
	int disable_sensor();

	int update_delay();
};

/*****************************************************************************/

#endif  // ANDROID_Dmt_SENSOR_H
