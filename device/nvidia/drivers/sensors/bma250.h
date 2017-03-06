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

#ifndef ANDROID_BMA250_H
#define ANDROID_BMA250_H

#include "SensorBase.h"

#define BMA250_DEF(handle) {           \
    "BMA250 Accelerometer", \
    "BOSCH",                               \
    1, handle,                                \
    SENSOR_TYPE_ACCELEROMETER, 1024.0f, 0.1f,         \
    0.5f, 20000, { } }

class Bma250 : public SensorBase {
    bool mEnabled;
    bool mHasPendingEvent;
    float mLast_value;
    bool mAlready_warned;	
    int64_t mCurrent_ns;
    int sid;	

public:
            Bma250(int type);
    virtual ~Bma250();
    virtual int readEvents(sensors_event_t* data, int count);
    virtual bool hasPendingEvents() const;
    virtual int enable(int32_t handle, int enabled);
};

class Bma250Prox : public SensorBase {
    bool mEnabled;
    bool mHasPendingEvent;
    bool mAlready_warned;
    float mLast_value;
    const char *mSysPath;
    int sid;

public:
            Bma250Prox(const char *sysPath, int sensor_id);
    virtual ~Bma250Prox();
    virtual int readEvents(sensors_event_t* data, int count);
    virtual bool hasPendingEvents() const;
    virtual int enable(int32_t handle, int enabled);
};

#endif  // ANDROID_BMA250_H

