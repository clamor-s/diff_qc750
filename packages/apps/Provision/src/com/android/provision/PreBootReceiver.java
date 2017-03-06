package com.android.provision;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.SystemProperties;
import android.util.Log;

public class PreBootReceiver extends BroadcastReceiver {

    private String TAG =PreBootReceiver.class.getSimpleName();
    @Override
    public void onReceive(Context context, Intent intent) {
        // TODO Auto-generated method stub
        Log.d(TAG,"doing update");
        SystemProperties.set("ctl.start", "mufin");
    }
}
