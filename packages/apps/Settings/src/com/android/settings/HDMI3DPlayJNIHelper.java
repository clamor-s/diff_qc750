/*
 * Copyright (c) 2011 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

package com.android.settings;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

/*
 * Displays preferences for HDMI 3DPlay
 */
public class HDMI3DPlayJNIHelper {

    private static final String STATE_ID_KEY = "StateID";
    private static final int STATE_ID_3DV_CONTROLS  = 1;

    private static final String TAG = "HDMI3DPlayJNIHelper";

    private Context mContext = null;

    /* JNI Function Prototypes */
    private native String getPropertyUsingJNI(String propname);
    public native boolean is3DVSupported();
    public native boolean isUsbHostModeMenuSupported();

    /* Load JNI library */
    static {
        System.loadLibrary("nvhdmi3dplay_jni");
    }

    public HDMI3DPlayJNIHelper(Context context) {
        mContext = context;
    }

    public void setProperty(String propName, String propVal) {

        if (mContext == null) {
            Log.v(TAG, "setProperty(): NULL Context");
            return;
        }

        Intent intent = new Intent();
        intent.setClassName("com.nvidia.NvCPLSvc", "com.nvidia.NvCPLSvc.NvCPLService");
        intent.putExtra("com.nvidia.NvCPLSvc."+ propName, propVal);
        intent.putExtra("com.nvidia.NvCPLSvc."+ STATE_ID_KEY, STATE_ID_3DV_CONTROLS);
        try {
            mContext.startService(intent);
        } catch (Exception e) {
            Log.e(TAG, "Exception launching NV Service", e);
        }
    }

    public String getProperty(String propName) {
        return getPropertyUsingJNI("persist.sys." + propName);
    }
}
