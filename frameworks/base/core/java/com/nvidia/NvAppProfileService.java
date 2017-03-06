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
 * @b Description: Exposes app profiles and app heuristics system
 *    to the frameworks
 */

package com.nvidia;
import android.util.Log;
import android.content.Context;

import com.nvidia.NvCPLSvc.NvAppProfiles;
import com.nvidia.NvCpuClient;
import com.nvidia.NvAppSpecificHeuristics;

import libcore.io.IoUtils;
import java.io.FileDescriptor;
import java.io.IOException;

/**
 * @hide
 *
 */
public class NvAppProfileService implements NvAppProfileConstants{
    private static final String TAG = "NvAppProfileService";

    private NvCpuClient mCpuClient;
    private NvAppProfiles mAppProfile;
    private NvAppSpecificHeuristics mAppSpecificHeuristics;
    private Context mContext;
    private boolean enableAppProfiles;
    private boolean initAppProfiles;

    private FileDescriptor mNvCpuMaxFreqCbusHdl;

    public NvAppProfileService (Context context) {
      mCpuClient = new NvCpuClient();
      mAppProfile = new NvAppProfiles(context);
      mAppSpecificHeuristics = new NvAppSpecificHeuristics();
      mContext = context;
      enableAppProfiles = false;
      //NvCpuD not ready at this time, defer initialization of profiles
      initAppProfiles = false;

      mNvCpuMaxFreqCbusHdl = null;
    }

    private synchronized void NvClearCbusMaxFreq() {
        try {
            if (mNvCpuMaxFreqCbusHdl != null) {
                IoUtils.close(mNvCpuMaxFreqCbusHdl);
                mNvCpuMaxFreqCbusHdl = null;
            }
        } catch (IOException ioe) {
            Log.w(TAG, "Trouble closing mNvCpuMaxFreqCbusHdl");
        }
    }

    /*
     * These are functions that solely depend on NvAppSpecificHeuristics
     * and are used to whitelist certain apps, independent of
     * platform
     */
    private void setAppHeuristicsCpuScalingMinFreq(String packageName) {
      int freq =
        mAppSpecificHeuristics.getAppSpecificScalingMinFreq(packageName);
      mCpuClient.setCpuScalingMinFreq(freq);
    }

    /*
     * These are functions that depend on NvAppProfiles and may or may not
     * be supported for certain platforms. In the latter case, these methods
     * should have no effect when called.
     */
    private void setAppProfileCpuCoreBias(String packageName) {
      /*
       * A kill switch is added which overwrites App Profile decision if
       * needed
       */
      if (mAppSpecificHeuristics.getAppSpecificDisableCpuCoreBias(packageName))
        mCpuClient.setCpuCoreBias(AP_CPU_CORE_BIAS_DISABLED);
      else {
        if (enableAppProfiles) {
          int bias = mAppProfile.getApplicationProfile(packageName, "core_bias");
          if (bias < 0) {
            mCpuClient.setCpuCoreBias(AP_CPU_CORE_BIAS_DEFAULT);
          }
          else {
            mCpuClient.setCpuCoreBias(bias);
          }
        }
        else
          mCpuClient.setCpuCoreBias(AP_CPU_CORE_BIAS_DEFAULT);
      }
    }

    private void setAppProfileGpuScaling(String packageName) {
      /*
       * A kill switch is added which overwrites App Profile decision if
       * needed
       */
      if (mAppSpecificHeuristics.getAppSpecificDisableGpuScaling(packageName))
        mCpuClient.setGpuScalingEnable(0);
      else
        mCpuClient.setGpuScalingEnable(1);
      /* todo: uncomment once gpu scaling is added in app profiles */
      /*
      else {
        if (enableAppProfiles) {
          int state = mAppProfile.getApplicationProfile(packageName, "gpu_scaling");
          if (state == 0)
            mCpuClient.setGpuScalingEnable(state);
        }
      }
      */
    }

    private void setAppProfileCpuMaxNormalFreq(String packageName) {
      if (!enableAppProfiles)
        return;
      int freq = mAppProfile.getApplicationProfile(packageName, "cpu_freq_bias");
      if (freq < 0) {
        mCpuClient.setCpuMaxNormalFreq(AP_CPU_MAX_NORMAL_FREQ_DEFAULT);
      }
      else {
        mCpuClient.setCpuMaxNormalFreq(freq);
      }
    }

    private void setAppProfileGpuCbusCapLevel(String packageName) {
      if (!enableAppProfiles)
        return;
      int bias = mAppProfile.getApplicationProfile(packageName, "gpu_core_cap");
      NvClearCbusMaxFreq();
      if (bias >= 0) {
        final long now = System.nanoTime();
        if(mCpuClient.getGpuCbusCapState() == 0)
          mCpuClient.setGpuCbusCapState(1);
        mNvCpuMaxFreqCbusHdl = mCpuClient.requestMaxCbusFreqHandle(bias, now);
      }
    }

    /*
     * Interface for the caller
     */
    public void setAppProfile(String packageName) {
      // Greedy initialization of App Profiles
      if (!initAppProfiles) {
        if (mCpuClient.getEnableAppProfiles() == 1) {
          enableAppProfiles = true;
          mCpuClient.setGpuCbusCapState(1);  //Turns cbus cap on
          mCpuClient.setGpuCbusCapLevel(AP_GPU_CBUS_CAP_LEVEL_DEFAULT);
          Log.w(TAG, "App Profiles: Enabled");
        }
        else {
          Log.w(TAG, "App Profiles: Not supported");
        }
        initAppProfiles = true;
      }

      setAppHeuristicsCpuScalingMinFreq(packageName);
      setAppProfileCpuCoreBias(packageName);
      setAppProfileCpuMaxNormalFreq(packageName);
      setAppProfileGpuCbusCapLevel(packageName);
      setAppProfileGpuScaling(packageName);
    }

    /*
     * Exposes old NvCpuClient functionality to the outside
     */
    public void requestMinOnlineCpuCount(int numCpus, long timeoutNs,
      long nowNanoTime) {
      mCpuClient.requestMinOnlineCpuCount(numCpus, timeoutNs, nowNanoTime);
    }

    public void requestMaxOnlineCpuCount(int numCpus, long timeoutNs,
      long nowNanoTime) {
      mCpuClient.requestMaxOnlineCpuCount(numCpus, timeoutNs, nowNanoTime);
    }

    public void requestCpuFreqMin(int strength, long timeoutNs,
      long nowNanoTime) {
      mCpuClient.requestCpuFreqMin(strength, timeoutNs, nowNanoTime);
    }

    public void requestCpuFreqMax(int strength, long timeoutNs,
      long nowNanoTime) {
      mCpuClient.requestCpuFreqMax(strength, timeoutNs, nowNanoTime);
    }
}

