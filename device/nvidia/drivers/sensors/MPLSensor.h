/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) 2011 Invensense, Inc.
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef ANDROID_MPL_SENSOR_H
#define ANDROID_MPL_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <poll.h>
#include <utils/Vector.h>
#include <utils/KeyedVector.h>
#include <linux/input.h>
#include <hardware/sensors.h>

#include "sensors.h"
#include "SensorBase.h"

#include "ml.h"

#define EXTRA_VERBOSE (0)
#define FUNC_LOG ALOGV("%s", __PRETTY_FUNCTION__)
#define VFUNC_LOG ALOGV_IF(EXTRA_VERBOSE, "%s", __PRETTY_FUNCTION__)
#define CALL_MEMBER_FN(pobject, ptrToMember) ((pobject)->*(ptrToMember))

#define ACCEL_THRESHOLD 0.1
#define ACCEL_WINDOW_SIZE 10
#define IDLE_POLL_PERIOD 400 /* millisecond */
#define DEFAULT_POLL_PERIOD 40 /* millisecond */

/*****************************************************************************/
/** MPLSensor implementation which fits into the HAL example for crespo provided
 * * by Google.
 * * WARNING: there may only be one instance of MPLSensor, ever.
 */

class NineAxisSensorFusion
{
    public:
        NineAxisSensorFusion();
        ~NineAxisSensorFusion();
    public:
        void registerEnabler(inv_error_t (*fp_enable_9x_fusion) (void));
        void registerDisabler(inv_error_t (*fp_disable_9x_fusion) (void));
        inv_error_t enable();
        inv_error_t disable();
        int status() { return enabled; }

    private:
        inv_error_t (*fp_enable_9x_fusion) (void);
        inv_error_t (*fp_disable_9x_fusion) (void);
        int enabled;
};
class MPLSensor : public SensorBase {

    typedef void (MPLSensor::*hfunc_t)(sensors_event_t*, uint32_t*, int);

public:
    bool initFailed;
            MPLSensor();
    virtual ~MPLSensor();

    enum {
#ifdef ENABLE_GESTURE_MANAGER
          Tap             = 0, //0 Tap
          Shake           = 1, //1 Shake
          YawIR           = 2, //2 Yaw Image Rotate
          Orient6         = 3, //3 orientation
          GridNum         = 4, //4 grid num
          GridDelta       = 5, //5 grid change
          CtrlSig         = 6, //6 ctrl sig
          Motion          = 7, //7 motion
          Step            = 8, //8 step
          Snap            = 9, //9 snap
          RotationVector  = 10,
#else
          RotationVector  = 0,
#endif
          LinearAccel,
          Gravity,
          Gyro,
          Accelerometer,
          MagneticField,
          Orientation,
          numSensors
    };
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int enable(int32_t handle, int enabled);
    virtual int readEvents(sensors_event_t *data, int count);
    virtual void computeLocalSensorMask(int enabled_sensors);
    virtual bool needDMPStop();
    virtual bool needStateChange(bool changing_sensors, bool restart) { return (changing_sensors || restart); }
    virtual void enableFeatures() { return; }
    virtual void shutdownFeatures() { return; }
    virtual void adjustFifoRate(int& rate) { return ; }
    virtual int getFd() const;
    virtual int getAccelFd() const;
    virtual int getTimerFd() const;
    virtual int getPowerFd() const;
    virtual int getPollTime();
    virtual bool hasPendingEvents() const;
    virtual void handlePowerEvent();
    virtual void sleepEvent();
    virtual void wakeEvent();
    void cbOnMotion(uint16_t);
    void cbProcData();

    //static pointer to the object that will handle callbacks
    static MPLSensor* gMPLSensor;

protected:

    void clearIrqData(bool* irq_set);
    virtual void setPowerStates(int enabledsensor);
    void initMPL();
    void enableFIFO();
    void setupCallbacks();
    void gyroHandler(sensors_event_t *data, uint32_t *pendmask, int index);
    void accelHandler(sensors_event_t *data, uint32_t *pendmask, int index);
    void compassHandler(sensors_event_t *data, uint32_t *pendmask, int index);
    void rvHandler(sensors_event_t *data, uint32_t *pendmask, int index);
    void laHandler(sensors_event_t *data, uint32_t *pendmask, int index);
    void gravHandler(sensors_event_t *data, uint32_t *pendmask, int index);
    void orienHandler(sensors_event_t *data, uint32_t *pendmask, int index);
    void calcOrientationSensor(float *Rx, float *Val);
    int estimateCompassAccuracy();
    void loadCompassCalibrationEnabler(void* h_dmp_lib, const char* suffix);
    void enable9Axis(int enable);
    virtual int64_t now_ns();
    virtual int update_delay();

    void resetAccelWindow();
    bool isDeviceOrientationStable();
    void updateAccelWindow(float accel0, float accel1, float accel2);

    int mMpuAccuracy; // global storage for the current accuracy status
    int mNewData; // flag indicating that the MPL calculated new output values
    int mDmpStarted;
    long mMasterSensorMask;
    long mLocalSensorMask;
    int mPollTime;
    int mCurFifoRate; // current fifo rate
    bool mHaveGoodMpuCal; // flag indicating that the cal file can be written
    bool mUseTimerIrqAccel;
    bool mUsetimerIrqCompass;
    bool mUseTimerirq;
    struct pollfd mPollFds[4];
    int mSampleCount;
    pthread_mutex_t mMplMutex;
    NineAxisSensorFusion nineAxisSF;

    enum FILEHANDLES
    {
        MPUIRQ_FD, ACCELIRQ_FD, COMPASSIRQ_FD, TIMERIRQ_FD,
    };

    int accel_fd;
    int timer_fd;

    uint32_t mEnabled;
    uint32_t mPendingMask;
    sensors_event_t mPendingEvents[numSensors];
    uint64_t mDelays[numSensors];
    hfunc_t mHandlers[numSensors];
    bool mForceSleep;
    long int mOldEnabledMask;
    android::KeyedVector<int, int> mIrqFds;

    int mPollPeriod;
    int mPollPeriodSave;
    bool mWindowFull;
    int mAccelWindowIndex;
    float mAccelWindowSum[3];
    float mAccelWindow[ACCEL_WINDOW_SIZE][3];
};

extern "C" {
void setCallbackObject(MPLSensor*);
MPLSensor* getCallbackObject();
}

/*****************************************************************************/

#endif  // ANDROID_MPL_SENSOR_H
