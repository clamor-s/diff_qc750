// Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
package icera.DebugAgent;

import java.io.IOException;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.telephony.ServiceState;
import android.content.SharedPreferences;


public class BugReportReceiver extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent)
    {
        if (intent.getAction().equals("android.intent.action.BOOT_COMPLETED"))
        {
            SharedPreferences prefs = context.getSharedPreferences("AppPrefs", Context.MODE_PRIVATE);
            if (prefs.getBoolean("launch",false) && BugReportService.appli_started == false)
            {
                Intent startupIntent = new Intent(context, BugReportService.class);
                startupIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startService(startupIntent);
             }
        }
        else if (intent.getAction().equals("android.intent.action.SERVICE_STATE") && BugReportService.appli_started == true)
        {
            int service_state = intent.getIntExtra("state", 69);
            handleStateChanged(context, service_state);
        }
    }

    /**
     * Handle check of logcat depending on the phone state.
     *
     */
    public static String crashlog = "No Crashlog / check the coredump";

    private void handleStateChanged(Context mContext, int state) {
        LogCatDetect logcat = new LogCatDetect(mContext);
        Boolean possible_crash = false;
        switch (state) {

        // Phone has been disconected - check logcat when service comes back.
        case ServiceState.STATE_POWER_OFF:
            //Moniter the logcat log to detect an error
            Lock.acquire(mContext);
            logcat.detect();
            Lock.release();
            break;

        case ServiceState.STATE_OUT_OF_SERVICE:
            //Monitor the logcat log to retrieve the crash report
            if(possible_crash == true)
            {
                // delete the logcat history to avoid twice the same detection
                try {
                    Runtime.getRuntime().exec(new String[] { "logcat", "-b", "radio", "-b", "main", "-c"});
                } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
                }
                possible_crash = false;
            }
            else
            {
                Lock.acquire(mContext);
                crashlog = logcat.crashlog();
                Lock.release();
            }
            break;

        case ServiceState.STATE_IN_SERVICE:
            possible_crash = true;
            break;

        case ServiceState.STATE_EMERGENCY_ONLY:
        default:
            // Nothing special to handle.
            break;

        }
    }
}