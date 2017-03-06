// Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
package icera.DebugAgent;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import icera.DebugAgent.BugReportService.BugReportServiceBinder;
import icera.DebugAgent.R;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.IBinder;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.Toast;
import android.widget.ToggleButton;
import android.util.Log;

public class Settings extends Activity {
    /** Called when the activity is first created. */

    // Binding to BugReportService
    BugReportService mService;
    boolean mBound = false;

    // Output directory name
    private static String LogSize = "200Mo";
    private static int keep_pos = 3;

    private static boolean modemLoggingActive = true;
    private static boolean logcatLoggingActive = true;
    private static boolean autoLaunchActive = false;
    private static boolean autoSavingActive = false;

    /**
    * Called at creation of the activity - display layout - bind to service
    */
    @Override
    public void onCreate(Bundle savedInstanceState) {

        Log.d("Icera","Creating view");

        super.onCreate(savedInstanceState);

        autoLaunchActive = LoadPreferences("launch");
        modemLoggingActive = LoadPreferences("logging");
        autoSavingActive = LoadPreferences("autoSave");
        logcatLoggingActive = LoadPreferences("logcat");

        if(savedInstanceState != null) {
            modemLoggingActive = savedInstanceState.getBoolean("logging", modemLoggingActive);
            logcatLoggingActive = savedInstanceState.getBoolean("logcat", logcatLoggingActive);
        }

        // Display main screen
        setContentView(R.layout.settings);

        ToggleButton togglebutton = (ToggleButton) findViewById(R.id.ButtonLog);

        togglebutton.setChecked(modemLoggingActive);

        ToggleButton togglebutton2 = (ToggleButton) findViewById(R.id.ButtonLogcat);

        togglebutton2.setChecked(logcatLoggingActive);

        ToggleButton togglebutton3 = (ToggleButton) findViewById(R.id.ButtonBoot);

        togglebutton3.setChecked(autoLaunchActive);

        ToggleButton togglebutton4 = (ToggleButton) findViewById(R.id.ButtonAutoSave);

        togglebutton4.setChecked(autoSavingActive);

        // Spinner display
        Spinner spinner = (Spinner) findViewById(R.id.SpinnerLogSize);
        ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(
               this, R.array.PossibleSize, android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(adapter);
        spinner.setSelection(keep_pos);
        spinner.setOnItemSelectedListener(new SetLogSize());

        // Bind to service
        Intent intent = new Intent(this, BugReportService.class);
        bindService(intent, mConnection, Context.BIND_AUTO_CREATE);

        Log.d("Icera","View created");
    }

    /**
    * Called when stopping the activity - unbind service
    */
    @Override
    protected void onStop() {
        super.onStop();
        // Unbind from the service
        if (mBound) {
            unbindService(mConnection);
            mBound = false;
        }
    }

    /** Defines callbacks for service binding, passed to bindService() */
    private ServiceConnection mConnection = new ServiceConnection() {

        public void onServiceConnected(ComponentName className, IBinder service) {
            // We've bound to LocalService, cast the IBinder and get
            // LocalService instance
            BugReportServiceBinder binder = (BugReportServiceBinder) service;
            mService = binder.getService();
            mBound = true;
        }

        public void onServiceDisconnected(ComponentName arg0) {
            mBound = false;
        }
    };


    public class SetLogSize implements OnItemSelectedListener {

        public void onItemSelected(AdapterView<?> parent,
            View view, int pos, long id) {

            if ( parent.getItemAtPosition(pos).toString() != LogSize)
            {
                LogSize = parent.getItemAtPosition(pos).toString();
                keep_pos = pos;
                mService.deleteLogFiles();
            }
        }

        public void onNothingSelected(AdapterView<?> parent) {
          // Do nothing.
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putBoolean("logging", modemLoggingActive);
        outState.putBoolean("logcat", logcatLoggingActive);
    }

    /**
    * Handling GEnie logging on/off switch
    *
    * @param view
    */

    private static Process log_proc = null;
    private static Process genie_ios = null;
    static Process tracegen = null;

    public void GetTracegen() {

        File port = new File("/dev/log_modem");
        File output_tracegen = new File("/data/data/icera.DebugAgent/files/tracegen.ix");
        if ( port.exists() && ( !output_tracegen.exists() || (output_tracegen.exists() && (output_tracegen.length() < 500000)) ) ) {
            StopLogging();
            try {
                tracegen = Runtime.getRuntime().exec("icera_log_serial_arm -e/data/data/icera.DebugAgent/files/tracegen.ix -d/dev/log_modem");
                Thread.sleep(2000);
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            } catch (InterruptedException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
    }

    public boolean CheckLoggingProcess(){
        BufferedReader br = null;
        boolean proc_present = false;
        String line = "";

        try {

            Process p = Runtime.getRuntime().exec(new String[] { "ps"});
            br = new BufferedReader(new InputStreamReader(p.getInputStream()));

            while ((line = br.readLine()) != null) {

                if (line.contains("icera_log_serial_arm"))
                    proc_present = true;
            }
        } catch (IOException e) {
            // TODO Auto-generated catch block
              e.printStackTrace();
        }
        return proc_present;
    }

    public boolean StartLogging() {

        File output_dir = new File("/dev/log_modem");
        File new_filter = new File("/data/rfs/data/debug/icera_filter.tsf");
        String[] FileSize = LogSize.split("Mo");
        int file_size = Integer.parseInt(FileSize[0])/5;

        if (log_proc != null)
            log_proc.destroy();
        if (genie_ios != null)
            genie_ios.destroy();
        if (output_dir.exists() && modemLoggingActive == true && CheckLoggingProcess() == false) {
            if (new_filter.exists())
            {
                try {
                    genie_ios = Runtime
                              .getRuntime()
                              .exec("/data/data/icera.DebugAgent/app_tools/genie-ios-arm" + " /data/rfs/data/debug/dyn_filter.ios" + " /data/data/icera.DebugAgent/files/tracegen.ix" + " /data/rfs/data/debug/icera_filter.tsf");
                    log_proc = Runtime
                              .getRuntime()
                              .exec("icera_log_serial_arm -b -d/dev/log_modem -p32768 -f -s /data/rfs/data/debug/dyn_filter.ios -a/data/data/icera.DebugAgent/files,"+FileSize[0]+","+file_size);
                } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
                try {
                    Thread.sleep(2000);
                } catch (InterruptedException e1) {
                // TODO Auto-generated catch block
                    e1.printStackTrace();
                }
                log_proc.destroy();
                return false;
                }
            }
            else
            {
                try {
                    log_proc = Runtime
                              .getRuntime()
                              .exec("icera_log_serial_arm -b -d/dev/log_modem -p32768 -f -a/data/data/icera.DebugAgent/files,"+FileSize[0]+","+file_size);
                } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
                try {
                    Thread.sleep(2000);
                } catch (InterruptedException e1) {
                    // TODO Auto-generated catch block
                    e1.printStackTrace();
                }
                log_proc.destroy();
                return false;
                }
            }
        } else {
        return false;
        }
        return true;
    }

    public void StopLogging() {

        if (log_proc != null)
            log_proc.destroy();
        if (genie_ios != null)
            genie_ios.destroy();
     }

    public void LoggingSwitch(View view) {

        Log.d("Icera","Toggle button");

        boolean result = false;

        ToggleButton togglebutton = (ToggleButton) findViewById(R.id.ButtonLog);

        if (togglebutton.isChecked() && modemLoggingActive == false) {

            modemLoggingActive = true;

            GetTracegen();

            result = StartLogging();

            if (result == false) {
                togglebutton.setChecked(false);
                Toast.makeText(this, "Error while starting logging",Toast.LENGTH_SHORT).show();
                Toast.makeText(this, "check icera_log_serial_arm tool under /system/bin/",Toast.LENGTH_SHORT).show();
                Toast.makeText(this, "and restart the phone",Toast.LENGTH_SHORT).show();
                modemLoggingActive = false;
            }
            else {
                try {
                    Thread.sleep(2000);
                } catch (InterruptedException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                File beanielog_folder = new File("/data/data/icera.DebugAgent/files");
                File[] files = SaveLogs.dirListByDescendingDate(beanielog_folder);
                boolean error = true;
                for (int i = 0; i < files.length; i++) {
                    if (files[i].getAbsolutePath().contains("_000.iom"))
                    {
                        error = false;
                    }
                }
                if (error == false)
                    Toast.makeText(this, "Logging started", Toast.LENGTH_SHORT).show();
                else {
                    togglebutton.setChecked(false);
                    Toast.makeText(this, "Error while starting logging",Toast.LENGTH_SHORT).show();
                    Toast.makeText(this, "update the icera_log_serial_arm tool under /system/bin/",Toast.LENGTH_SHORT).show();
                    modemLoggingActive = false;
                }
            }

        } else if (modemLoggingActive == true){
            modemLoggingActive = false;
            Toast.makeText(this, "Logging stopped", Toast.LENGTH_SHORT).show();
            mService.deleteLogFiles();
        }
        SavePreferences("logging", modemLoggingActive);

    }

    public void LaunchSwitch(View view) {
        ToggleButton togglebutton = (ToggleButton) findViewById(R.id.ButtonBoot);

        if (togglebutton.isChecked()) {
            autoLaunchActive = true;
            Toast.makeText(this, "Auto Launch after power on",Toast.LENGTH_SHORT).show();
        } else {
            autoLaunchActive = false;
            Toast.makeText(this, "Don't Launch after power on", Toast.LENGTH_SHORT).show();
        }
        SavePreferences("launch", autoLaunchActive);
    }

    public void LogcatSwitch(View view) {
        ToggleButton togglebutton = (ToggleButton) findViewById(R.id.ButtonLogcat);
        LogCatDetect logcat = new LogCatDetect(this);

        if (togglebutton.isChecked()) {
            logcatLoggingActive = true;
            SavePreferences("logcat", logcatLoggingActive);
            logcat.start();
            Toast.makeText(this, "Logcat Logging started",Toast.LENGTH_SHORT).show();
        } else {
            logcatLoggingActive = false;
            SavePreferences("logcat", logcatLoggingActive);
            logcat.stop();
            Toast.makeText(this, "Logcat Logging stopped", Toast.LENGTH_SHORT).show();
        }
    }

    public void AutomaticLogSaving(View view) {
        Boolean autoLogSaving = false;
        ToggleButton togglebutton = (ToggleButton) findViewById(R.id.ButtonAutoSave);

        if (togglebutton.isChecked()) {
            autoLogSaving = true;
            Toast.makeText(this, "Enable Automatic Log Saving",Toast.LENGTH_SHORT).show();
        } else {
            autoLogSaving = false;
            Toast.makeText(this, "Disable Automatic Log Saving", Toast.LENGTH_SHORT).show();
        }
        SavePreferences("autoSave", autoLogSaving);
    }

    private void SavePreferences(String key, Boolean value){
        SharedPreferences settings = getSharedPreferences("AppPrefs", MODE_PRIVATE);
        SharedPreferences.Editor prefEditor = settings.edit();
        prefEditor.putBoolean(key, value);
        prefEditor.commit();
    }

    private boolean LoadPreferences(String key){
        SharedPreferences sharedPreferences = getSharedPreferences("AppPrefs", MODE_PRIVATE);
        boolean value = sharedPreferences.getBoolean(key, false );
        return value;
    }
}