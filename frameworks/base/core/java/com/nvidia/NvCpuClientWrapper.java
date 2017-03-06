/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
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

/**
 * @file
 * <b>NVIDIA Tegra Android CPU management</b>
 *
 * @b Description: Exposes management of Tegra cores to speed up
 *    computation or save power when necessary. This is a wrapper
 *    for NvCpuClient; it helps clients call CPU management
 *    functions without worrying about file descriptors.
 */

package com.nvidia;

import android.util.Log;

import libcore.io.IoUtils;

import java.io.FileDescriptor;
import java.io.IOException;

/**
 * @hide
 *
 */
public class NvCpuClientWrapper implements NvCpuClientConstants {

    private static final String TAG = "NvCpuClientWrapper";

    private NvCpuClient mNvCpuClient;
    private FileDescriptor mNvCpuMaxFreqHdl;
    private FileDescriptor mNvCpuMinFreqHdl;
    private FileDescriptor mNvCpuMinOnlineHdl;
    private FileDescriptor mNvCpuMaxOnlineHdl;

    public NvCpuClientWrapper() {
        mNvCpuClient = new NvCpuClient();
        mNvCpuMaxFreqHdl = null;
        mNvCpuMinFreqHdl = null;
        mNvCpuMinOnlineHdl = null;
        mNvCpuMaxOnlineHdl = null;
    }

    /**
     * Limits the Cpu Frequency to Max value for a timeout preiod
     */
    public void NvSetCpuMaxFreq(int cpuMaxFreq, long timeoutNs) {
        final long now = System.nanoTime();
        mNvCpuClient.requestCpuFreqMax(cpuMaxFreq,timeoutNs,now);
    }

    /**
     * Boosts the Cpu Frequency to Min Value for a timeout period
     */
    public void NvSetCpuMinFreq(int cpuMinFreq, long timeoutNs) {
        final long now = System.nanoTime();
        mNvCpuClient.requestCpuFreqMin(cpuMinFreq,timeoutNs,now);
    }

     /** Sets the Minimum number of CPUs that should stay Online for a
      * timeout period
      */
    public void NvSetCpuMinOnline(int cpuMinOnline, long timeoutNs) {
        final long now = System.nanoTime();
        mNvCpuClient.requestMinOnlineCpuCount(cpuMinOnline,timeoutNs,now);
    }

    /** Limits the Cpu Frequency to a Max value until it is cleared by
     * NvClearCpuMaxFreq()
     */
    public synchronized void NvSetCpuMaxFreq(int cpuMaxFreq) {
        final long now = System.nanoTime();
        if (mNvCpuMaxFreqHdl != null) {
            NvClearCpuMaxFreq();
        }
        mNvCpuMaxFreqHdl = mNvCpuClient.requestCpuFreqMaxHandle(cpuMaxFreq,now);
    }

    /** Sets the Minimum number of CPUs that should stay Online until it is cleared
     * by NvClearCpuMinOnline()
     */
    public synchronized void NvSetCpuMinOnline(int cpuMinOnline) {
        final long now = System.nanoTime();
        if (mNvCpuMinOnlineHdl != null) {
            NvClearCpuMinOnline();
        }
        mNvCpuMinOnlineHdl = mNvCpuClient.requestMinOnlineCpuCountHandle(cpuMinOnline,now);
    }

    /** Sets the Maximum number of CPUs that should stay Online until it is cleared
     * by NvClearCpuMaxOnline()
     */
    public synchronized void NvSetCpuMaxOnline(int cpuMaxOnline) {
        final long now = System.nanoTime();
        if (mNvCpuMaxOnlineHdl != null) {
            NvClearCpuMaxOnline();
        }
        mNvCpuMaxOnlineHdl = mNvCpuClient.requestMaxOnlineCpuCountHandle(cpuMaxOnline,now);
    }

    /**
     * Clears the Max CPU Frequency set by NvSetCpuMaxFreq()
     */
    public synchronized void NvClearCpuMaxFreq() {
        try {
            if (mNvCpuMaxFreqHdl != null) {
                IoUtils.close(mNvCpuMaxFreqHdl);
                mNvCpuMaxFreqHdl = null;
            }
        } catch (IOException ioe) {
            Log.w(TAG, "Trouble closing mNvCpuMaxFreqHdl");
        }
    }

    /**
     * Clears the Minimum number of online CPUs set by NvSetCpuMinOnline()
     */
    public synchronized void NvClearCpuMinOnline() {
        try {
            if (mNvCpuMinOnlineHdl != null) {
                IoUtils.close(mNvCpuMinOnlineHdl);
                mNvCpuMinOnlineHdl = null;
            }
        } catch (IOException ioe) {
            Log.w(TAG, "Trouble closing mNvCpuMinOnlineHdl");
        }
    }

    /**
     * Clears the Maximum number of online CPUs set by NvSetCpuMaxOnline()
     */
    public synchronized void NvClearCpuMaxOnline() {
        try {
            if (mNvCpuMaxOnlineHdl != null) {
                IoUtils.close(mNvCpuMaxOnlineHdl);
                mNvCpuMaxOnlineHdl = null;
            }
        } catch (IOException ioe) {
            Log.w(TAG, "Trouble closing mNvCpuMaxOnlineHdl");
        }
    }
}
