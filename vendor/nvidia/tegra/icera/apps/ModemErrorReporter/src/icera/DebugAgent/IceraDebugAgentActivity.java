// Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
package icera.DebugAgent;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import icera.DebugAgent.BugReportService;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.widget.Toast;

public class IceraDebugAgentActivity extends Activity {

    private String LoadPreferences(String key){
        SharedPreferences sharedPreferences = getSharedPreferences("AppPrefs", MODE_PRIVATE);
        String value = sharedPreferences.getString(key, "NO_CRASH");
        return value;
    }

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

        if (BugReportService.appli_started == false)
        {
            // Start the background service.
            Intent startupIntent = new Intent(this, BugReportService.class);
            if (LoadPreferences("error") == "NO_CRASH")
                startupIntent.putExtra("notification", BugReportService.NOTIFY_STARTED);
            else if (LoadPreferences("error") == "NOTIFY_CRASH_MODEM")
                startupIntent.putExtra("notification", BugReportService.NOTIFY_CRASH_MODEM);
            else if (LoadPreferences("error") == "NOTIFY_CRASH_RIL")
                startupIntent.putExtra("notification", BugReportService.NOTIFY_CRASH_RIL);
            else if (LoadPreferences("error") == "NOTIFY_CRASH_KERNEL")
                startupIntent.putExtra("notification", BugReportService.NOTIFY_CRASH_KERNEL);
            startService(startupIntent);

            BugReportService.appli_started = true;
            File single = new File("/data/rfs/data/debug");
            if (!(single.exists()))
            {// re-create the coredump.txt which is used by the RIL
                try {
                    String string = "No Crash Happened";
                    FileOutputStream file_coredump = openFileOutput("coredump.txt",
                                                                    Context.MODE_WORLD_READABLE | Context.MODE_WORLD_WRITEABLE);
                    file_coredump.write(string.getBytes());
                    file_coredump.close();
                    Toast.makeText(this, "coredump created", Toast.LENGTH_LONG).show();
                } catch (IOException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }

            //delete the entire logs
            String[] files = null;

            try {
                files = fileList();
            } catch (Exception e) {
                e.printStackTrace();
            }
            for (int i = 0; i < files.length; i++) {
                deleteFile(files[i]);
            }
        }
        else
            Toast.makeText(this, "Application already started", Toast.LENGTH_LONG).show();

        // Nothing to display so close the activity.
        this.finish();
    }
    @Override
    public void onStop() {
        super.onStop();
        Settings logging = new Settings();
        logging.StopLogging();
        LogCatDetect logcat = new LogCatDetect(this);
        logcat.stop();
        // The activity is about to become visible.
    }
    @Override
    public void onDestroy(){
        super.onDestroy();
    }
}