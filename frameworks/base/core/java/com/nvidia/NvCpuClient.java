/*
 * Copyright (c) 2012-2013 NVIDIA Corporation.  All rights reserved.
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
 *
 * Class structure based upon Camera in Camera.java:
 * Copyright (C) 2009 The Android Open Source Project
 */

/**
 * @file
 * <b>NVIDIA Tegra Android CPU management</b>
 *
 * @b Description: Exposes management of Tegra cores to speed up
 *    computation or save power when necessary
 */

package com.nvidia;
import android.util.Log;
import java.io.FileDescriptor;

/**
 * @hide
 *
 */
public class NvCpuClient extends Object implements NvCpuClientConstants {
    private static final String TAG = "NvCpuClient";

    /** requires permission ACCESS_DEVICE_POWER or NVIDIA_CPU_POWER
     * fails silently if it's missing;
     * the access check is performed asynchronously
     */

    /*
     * alias for requestCpuFreqMin
     */
    public native void pokeCPU(
            int strength,
            long timeoutNs,
            long nowNanoTime);

    /*
     * Timeout Methods
     *
     * These place a ceiling/floor on system power settings
     * for the duration of timeoutNs.  nowNanoTime is the current
     * time as reported by System.nanoTime, and is used for API
     * rate-limiting
     *
     */
    public native void requestMinOnlineCpuCount(
            int numCpus,
            long timeoutNs,
            long nowNanoTime);

    public native void requestMaxOnlineCpuCount(
            int numCpus,
            long timeoutNs,
            long nowNanoTime);

    public native void requestCpuFreqMin(
            int strength,
            long timeoutNs,
            long nowNanoTime);

    public native void requestCpuFreqMax(
            int strength,
            long timeoutNs,
            long nowNanoTime);

    public native void requestMaxCbusFreq(
            int frequency,
            long timeoutNs,
            long nowNanoTime);

    public native void requestMinCbusFreq(
            int frequency,
            long timeoutNs,
            long nowNanoTime);

    /*
     * Handle methods
     *
     * These return a FileDescriptor that acts as a handle for
     * an open system power request.  Closing the file descriptor
     * ends the requests; it is also closed if the client process dies.
     *
     * nowNanoTIme is the current time as reported by System.nanoTime,
     * it is used for API rate limiting
     */

    public native FileDescriptor requestMinOnlineCpuCountHandle(
            int numCpus,
            long nowNanoTime);

    public native FileDescriptor requestMaxOnlineCpuCountHandle(
            int numCpus,
            long nowNanoTime);

    public native FileDescriptor requestCpuFreqMinHandle(
            int strength,
            long nowNanoTime);

    public native FileDescriptor requestCpuFreqMaxHandle(
            int strength,
            long nowNanoTime);

    public native FileDescriptor requestMaxCbusFreqHandle(
            int frequency,
            long nowNanoTime);

    public native FileDescriptor requestMinCbusFreqHandle(
            int frequency,
            long nowNanoTime);

    /*
     * App Profile methods
     * These methods sets/gets information from sysfs knobs
     * and are used by ActivityStack to modify performance
     * depending on App currently running
     */
    public native void setCpuCoreBias(int bias);

    public native int getCpuCoreBias();

    public native void setGpuCbusCapLevel(int freq);

    public native int getGpuCbusCapLevel();

    public native void setGpuCbusCapState(int state);

    public native int getGpuCbusCapState();

    public native void setCpuHighFreqMinDelay(int delay);

    public native int getCpuHighFreqMinDelay();

    public native void setCpuMaxNormalFreq(int freq);

    public native int getCpuMaxNormalFreq();

    public native void setCpuScalingMinFreq(int freq);

    public native int getCpuScalingMinFreq();

    public native void setGpuScalingEnable(int val);

    public native int getGpuScalingEnable();

    public native void setEnableAppProfiles(int val);

    public native int getEnableAppProfiles();

    public NvCpuClient() {
        mNativeNvCpuClient = 0;
        init();
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            super.finalize();
        } finally {
            if (mNativeNvCpuClient != 0) {
                release();
            }
        }
    }

    private native void init();
    private native void release();
    //This is 32-bit dependent code!
    //

    private native static void nativeClassInit();
    static { nativeClassInit(); }

    private int mNativeNvCpuClient;
}
