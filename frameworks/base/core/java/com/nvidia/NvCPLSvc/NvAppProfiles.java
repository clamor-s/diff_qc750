/*
 * Copyright (c) 2012, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

package com.nvidia.NvCPLSvc;

import android.util.Log;
import android.content.Context;
import android.content.ComponentName;
import android.content.Intent;
import android.os.IBinder;
import android.content.ServiceConnection;
import com.nvidia.NvCPLSvc.INvCPLRemoteService;

public class NvAppProfiles {
    private static final String TAG = "NvAppProfiles";

    /**
     * Unique name used for NvCPLSvc to whitelist this class
     */
    static final String NV_APP_PROFILES_NAME = "Frameworks_NvAppProfiles";

    /**
     * Connection established with the App Profile service
     */
    private NvCPLSvcConnection connCPLSvc;

    /**
     * Callback class given by the NvCPLService
     */
    private INvCPLRemoteService mNvCPLSvc;

    private Context mContext;

    public NvAppProfiles(Context context) {
      mContext = context;
    }

    /**
     * Sets up callback function into NvCPLService
     */
    class NvCPLSvcConnection implements ServiceConnection {
      public void onServiceConnected(ComponentName name, IBinder service) {
        if(service == null) {
          Log.w(TAG, "App Profile: Invalid Binder given");
          return;
        }
        mNvCPLSvc = INvCPLRemoteService.Stub.asInterface(service);
      }
      public void onServiceDisconnected(ComponentName name) {
        mNvCPLSvc = null;
      }
    }

    /**
     * Verifies binding to NvCPLSvc is valid. If invalid then it attempts to
     * establish it. Returns true if success, false if mContext is not valid
     * or exception caught.
     */
    private boolean checkBinding() {
      try {
        if (mNvCPLSvc == null) {
          if (mContext == null) {
            return false;
          }
          connCPLSvc = new NvCPLSvcConnection();
          Intent intentCPLSvc = new Intent();
          intentCPLSvc.setClassName("com.nvidia.NvCPLSvc", "com.nvidia.NvCPLSvc.NvCPLService");
          intentCPLSvc.putExtra("nvcplsvc_client_name", NV_APP_PROFILES_NAME);
          mContext.bindService(intentCPLSvc, connCPLSvc, Context.BIND_AUTO_CREATE);
        }
        return true;
      } catch (Exception e) {
        Log.w(TAG, "App Profile: Failed to establish binding. Error="+e.getMessage());
        return false;
      }
    }


    public int getApplicationProfile(String packageName, String tag) {
      if (!checkBinding()) {
        return -1;
      }
      /*
       * Even if binding was created, it is still possible that call
       * back has not been called yet.
       */
      if (mNvCPLSvc == null) {
        return -1;
      }

      try {
        return mNvCPLSvc.getAppProfileSettingInt(packageName, tag);
      } catch (Exception e) {
        Log.w(TAG, "App Profile: Failed to retrieve profile. Error="+e.getMessage());
        return -1;
      }
    }
}
