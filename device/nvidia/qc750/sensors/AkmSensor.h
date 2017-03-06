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

#ifndef ANDROID_AKM_SENSOR_H
#define ANDROID_AKM_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>


#include "sensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

/*****************************************************************************/

struct input_event;

class AkmSensor : public SensorBase {
public:
            AkmSensor();
    virtual ~AkmSensor();

    enum {
        MagneticField = 0,
        Orientation,
        numSensors
    };

    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int enable(int32_t handle, int enabled);
    virtual int forwardEvents(sensors_event_t *data);
    virtual bool isEnabled(int32_t handle);
    virtual int readEvents(sensors_event_t* data, int count);
    void processEvent(int code, int value);

private:
    int update_delay();

	int is_sensor_enabled(uint32_t sensor_type);
	int enable_sensor(uint32_t sensor_type);
	int disable_sensor(uint32_t sensor_type);
	int set_delay(int64_t ns[3]);

    enum {
        AccelAxisX = 0,
        AccelAxisY,
        AccelAxisZ,
        numAccelAxises,
    };

    uint32_t mEnabled;
    uint32_t mPendingMask;
    InputEventCircularReader mInputReader;
    sensors_event_t mPendingEvents[numSensors];
    int64_t mDelays[numSensors];
    int16_t mAccels[numAccelAxises];
};

/*****************************************************************************/

#endif  // ANDROID_AKM_SENSOR_H
