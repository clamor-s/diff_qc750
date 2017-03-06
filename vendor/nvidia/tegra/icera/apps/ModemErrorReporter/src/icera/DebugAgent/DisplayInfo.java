// Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
package icera.DebugAgent;

import java.io.BufferedReader;
import java.io.File;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;

import icera.DebugAgent.R;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.Dialog;
import android.app.ListActivity;
import android.content.ActivityNotFoundException;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.util.Log;

public class DisplayInfo extends Activity {
    /** Called when the activity is first created. */

    LogCatDetect logcat = new LogCatDetect(this);
    String display_info = "";
    BufferedReader br = null;
    String line = "";
    Process p = null;

    public void onCreate(Bundle savedInstanceState) {

        Log.d("Icera","Creating view");

        super.onCreate(savedInstanceState);

        // Display main screen
        setContentView(R.layout.displayinfo);

        SharedPreferences prefs = getSharedPreferences("AppPrefs", 0);

        if (prefs.getString("error","NO_CRASH").contains("NOTIFY_CRASH_MODEM"))
        {
            TextView message = (TextView) findViewById(R.id.CrashInfo);
            message.setTextSize(13);
            message.setText(BugReportReceiver.crashlog);
        }
        else
        {
            TextView message = (TextView) findViewById(R.id.CrashInfo);
            message.setTextSize(13);
            display_info = "platformConfig : \n";
            try {
                p = Runtime.getRuntime().exec(new String[] { "cat", "/data/rfs/data/factory/platformConfig.xml"});
            } catch (IOException e) {
            // TODO Auto-generated catch block
                e.printStackTrace();
            }
            br = new BufferedReader(new InputStreamReader(p.getInputStream()));

            try {
                while ((line = br.readLine()) != null) {
                    display_info = display_info + "   " + line + "\n";
                }
            } catch (IOException e) {
            // TODO Auto-generated catch block
                e.printStackTrace();
            }
            message.setText(display_info);
        }

        Log.d("Icera","View created");
    }

    public void DisplayLogs(View view) {
        // Action to be executed on click.
        Intent DisplayLogs = new Intent(this, DisplayLogs.class);
        startActivity(DisplayLogs);
    }
}