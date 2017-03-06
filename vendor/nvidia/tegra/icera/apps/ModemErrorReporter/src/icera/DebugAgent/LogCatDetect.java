// Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
package icera.DebugAgent;

import java.io.IOException;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Date;

public class LogCatDetect {

    private Context mContext;
    static Process full_logcat = null;
    static Process kmsg = null;
    Settings logging = new Settings();

    public LogCatDetect(Context context) {
        mContext = context;
    }

    public void start() {

        SharedPreferences prefs = mContext.getSharedPreferences("AppPrefs", Context.MODE_PRIVATE);

        if (full_logcat != null)
            full_logcat.destroy();

        // runtime logcat function
        if (prefs.getBoolean("logcat", false))
        {
            try {
                full_logcat = Runtime.getRuntime().exec(new String[] { "logcat", "-v", "threadtime", "-b", "main", "-b", "radio", "-f", "/data/data/icera.DebugAgent/files/logcat_ril_main.txt"});
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
    }

    public void start_kmsg() {

        if (kmsg != null)
            kmsg.destroy();

        // runtime logcat function
        try {
            kmsg = Runtime.getRuntime().exec(new String[] { "cat", "/proc/kmsg", ">", "/data/data/icera.DebugAgent/files/log_kernel.txt"});
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    private static boolean modem_crash = false;

    public void detect() {

        StringBuilder sb = new StringBuilder();
        BufferedReader br = null;
        InputStream coredump = null;
        String logcontent = "";
        String line = "";
        Process p = null;
        Date date = new Date();
        String Time = date.toString();
        String[] temp = Time.split(" ");
        String[] temp2 = temp[3].split(":");
        String Thetime = temp[0] + "_" + temp[1] + "_" + temp[2] + "_"
                         + temp2[0] + "_" + temp2[1] + "_" + temp2[2];
        Intent startupIntent = new Intent(mContext, BugReportService.class);
        SharedPreferences prefs = mContext.getSharedPreferences("AppPrefs", Context.MODE_PRIVATE);
        SharedPreferences.Editor prefEditor = prefs.edit();

        try {

            p = Runtime.getRuntime().exec(new String[] { "logcat", "-d", "-v", "threadtime", "-b", "radio"});
            br = new BufferedReader(new InputStreamReader(p.getInputStream()));

            while ((line = br.readLine()) != null)
            {
                sb.append(line);
                sb.append('\n');
            }
            logcontent = sb.toString();

            if (logcontent.contains("RIL_Init") || logcontent.contains("Error Recovery"))
            {
                prefEditor.putString("issue_time",Thetime);
                File single = new File("/data/rfs/data/debug");
                if (single.exists())
                {
                    try {
                        Thread.sleep(4000);
                    } catch (InterruptedException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                    String[]  listFiles;
                    listFiles = single.list();
                    int i;
                    for(i=0;i<listFiles.length;i++)
                    {
                        if (listFiles[i].startsWith("coredump_") && !BugReportService.coredumpList.contains(listFiles[i]))
                        {
                            modem_crash = true;
                            prefEditor.putBoolean("full_coredump",true);
                            prefEditor.putString("coredump",listFiles[i]);
                            break;
                        }
                        else if (listFiles[i].startsWith("crash_dump") && !BugReportService.coredumpList.contains(listFiles[i]))
                        {
                            modem_crash = true;
                            prefEditor.putBoolean("full_coredump",false);
                            prefEditor.putString("coredump",listFiles[i]);
                        }
                    }
                }
                else
                {
                    try {
                        coredump = mContext.openFileInput("coredump.txt");
                    } catch (FileNotFoundException e) {
                       // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                    try {
                       // if the coredump.txt content is different than "No Crash Happened", it means a modem crash occured
                        if (coredump.available() > 17)
                        {
                            modem_crash = true;
                        }
                    } catch (IOException e) {
                         // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                }

                if(modem_crash == true)
                {
                    prefEditor.putString("error","NOTIFY_CRASH_MODEM");
                    modem_crash = false;
                }
                else {
                    prefEditor.putString("error","NOTIFY_CRASH_RIL");
                }
                prefEditor.commit();
                mContext.startService(startupIntent);
            }

        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        return;
    }

    public String crashlog() {

        SharedPreferences prefs = mContext.getSharedPreferences("AppPrefs", Context.MODE_PRIVATE);
        StringBuilder sb = new StringBuilder();
        BufferedReader br = null;
        String crashlog = "";
        String line = "";
        Boolean get_crashlog = false;
        Process p = null;

        try {

            p = Runtime.getRuntime().exec(new String[] { "logcat", "-d","-b", "radio"});
            br = new BufferedReader(new InputStreamReader(p.getInputStream()));

            while ((line = br.readLine()) != null) {

                if (line.contains("DXP0 Crash Report"))
                    get_crashlog = true;
                else if (line.contains("Register"))
                    get_crashlog = false;
                else if (line.contains("Fullcoredump"))
                    get_crashlog = false;
                else if (line.contains("DXP1 Crash Report"))
                {
                    sb.append('\n');
                    sb.append('\n');
                    get_crashlog = true;
                }
                if (get_crashlog)
                {
                    String[] temp = line.split("<");
                    sb.append(temp[1]);
                    sb.append('\n');
                    // Restart the logging after a crash
                    if (prefs.getBoolean("logging", false))
                        logging.StartLogging();
                }
            }
            crashlog = sb.toString();

        } catch (IOException e) {
            // TODO Auto-generated catch block
              e.printStackTrace();
        }
        return crashlog;
    }


    public void stop() {

        // runtime logcat function
        if (full_logcat != null) {
            full_logcat.destroy();
            full_logcat = null;
        }
        File folder = new File("/data/data/icera.DebugAgent/files");
        File files[] = folder.listFiles();
        for (int i = 0; i < files.length; i++) {
            if (files[i].getAbsolutePath().contains("logcat"))
                files[i].delete();
        }
    }
}
