// Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
package icera.DebugAgent;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Binder;
import android.os.IBinder;


public class BugReportService extends Service {


    // Binder given to clients
    private final IBinder mBinder = new BugReportServiceBinder();

    private static int last_notification;

    public static List<String> coredumpList;

    /**
     * Class used for the client Binder.  Because we know this service always
     * runs in the same process as its clients, we don't need to deal with IPC.
     */
    public class BugReportServiceBinder extends Binder {
        BugReportService getService() {
            // Return this instance of LocalService so clients can call public methods
            return BugReportService.this;
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }
    
    public int getLastNotification()
    {
        return last_notification;
    }
    
    LogCatDetect starter = new LogCatDetect(this);
    Settings logging = new Settings();
    SaveLogs savelog = new SaveLogs();

    /** 
     * Possible types of notifications
     */ 
    public static final int NOTIFY_STARTED = 0;
    public static final int NOTIFY_CRASH_KERNEL = 1;
    public static final int NOTIFY_CRASH_RIL = 2;
    public static final int NOTIFY_CRASH_MODEM = 3;
    public static final int NOTIFY_SAVING_OK = 4;
    public static final int NOTIFY_SAVING_ISSUE = 5;
    public static final int NOTIFY_DISCARD = 6;
    public int count = 0;

    public static boolean appli_started = false;
     
     /**
     * Notify the user that the application is started by vibrating 
     * and displaying a message in notification area.
     * 
     * Also attach activity to be started when clicking on the message.
     */
    public void notify_message(int notify_type, int previous_type)
    {
        int NOTIFY_ME_ID=1336;
        CharSequence NOTIFICATION_MSG;
        CharSequence NOTIF_TITLE;
        CharSequence NOTIF_MSG;
        int icon;
        long when = System.currentTimeMillis();
        boolean vibrate;

        SharedPreferences sharedPreferences = getSharedPreferences("AppPrefs", 0);
        boolean autoSave = sharedPreferences.getBoolean("autoSave", false );

        last_notification = notify_type;
        count++;

        switch (notify_type) {
        case NOTIFY_CRASH_KERNEL:
            NOTIFICATION_MSG = "Icera Debug Agent - Kernel Crash detected";
            NOTIF_TITLE = "Icera Debug Agent - Kernel crash";
            NOTIF_MSG = "Press to get logs !";
            vibrate = true;
            icon = R.drawable.icon_red;
            break;

        case NOTIFY_CRASH_MODEM:
            NOTIFICATION_MSG = "Icera Debug Agent - Modem Error detected.";
            NOTIF_TITLE = "Icera Debug Agent - Modem error";
            NOTIF_MSG = "Press to get logs !";
            vibrate = true;
            icon = R.drawable.icon_red;
            break;

        case NOTIFY_CRASH_RIL:
            NOTIFICATION_MSG = "Icera Debug Agent - RIL Error detected.";
            NOTIF_TITLE = "Icera Debug Agent - RIL error";
            NOTIF_MSG = "Press to get logs !";
            vibrate = true;
            icon = R.drawable.icon_red;
            break;

        case NOTIFY_STARTED:
            NOTIFICATION_MSG = "Icera Debug Agent - App started";
            NOTIF_TITLE = "Icera Debug Agent Running";
            NOTIF_MSG = "No error detected";
            install_tools();
            vibrate = false;
            if (appli_started == false && logging.CheckLoggingProcess() == false)
            {
                deleteLogFiles();
            }
            icon = R.drawable.icon;
            appli_started = true;
            break;

        case NOTIFY_DISCARD:
            NOTIFICATION_MSG = "Icera Debug Agent - Logs discarded";
            NOTIF_TITLE = "Icera Debug Agent Running";
            NOTIF_MSG = "No error detected";
            vibrate = false;
            icon = R.drawable.icon;
            break;

        case NOTIFY_SAVING_ISSUE:
            NOTIFICATION_MSG = "Icera Debug Agent - Saving logs failed - Please retry";
            if (previous_type == NOTIFY_CRASH_MODEM)
            {
                NOTIF_TITLE = "Icera Debug Agent - Modem error";
                NOTIF_MSG = "Press to get logs !";
                vibrate = true;
                icon = R.drawable.icon_red;
            }
            else if (previous_type == NOTIFY_CRASH_RIL)
            {
                NOTIF_TITLE = "Icera Debug Agent - RIL error";
                NOTIF_MSG = "Press to get logs !";
                vibrate = true;
                icon = R.drawable.icon_red;
            }
            else if (previous_type == NOTIFY_CRASH_KERNEL)
            {
                NOTIF_TITLE = "Icera Debug Agent - Kernel crash";
                NOTIF_MSG = "Press to get logs !";
                vibrate = true;
                icon = R.drawable.icon_red;
            }
            else
            {
                NOTIF_TITLE = "Icera Debug Agent Running";
                NOTIF_MSG = "No error detected";
                vibrate = false;
                icon = R.drawable.icon;
            }
            break;

        case NOTIFY_SAVING_OK:
        default:
            if (count%2 == 0)
                NOTIFICATION_MSG =  "Icera Debug Agent - Logs saved ";
            else
                NOTIFICATION_MSG =  "Icera Debug Agent - Logs saved";
            NOTIF_TITLE = "Icera Debug Agent Running";
            NOTIF_MSG = "No error detected";
            vibrate = false;
            icon = R.drawable.icon;
            break;       
        }

        // Create Start message.
        Notification note=new Notification(icon, NOTIFICATION_MSG, when);        


        Intent pending = new Intent(this, SaveLogs.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, pending, 0);

        // Message to be displayed permanently.
        note.setLatestEventInfo(this, NOTIF_TITLE, NOTIF_MSG, pendingIntent);

        // Vibration.
        if (vibrate) {
            note.vibrate=new long[] {500L, 200L, 200L, 500L};
        }

        // Message cannot be cleared by user.
        note.flags|=Notification.FLAG_NO_CLEAR;

        // run in the foreground to avoid killing this service
        startForeground(NOTIFY_ME_ID, note);

        if(autoSave == true && ((notify_type == NOTIFY_CRASH_MODEM) || (notify_type == NOTIFY_CRASH_RIL)))
        {
            Intent autosave = new Intent(this, AutoSave.class);
            autosave.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(autosave);
        }
    }

    /**
     * Check if a kernel crash log is present in the file system
     */
    private void checkKernelState()
    {
        String kernel_file = this.getString(R.string.last_kmsg_filename);   

        try {
            BufferedReader kernel_content = new BufferedReader(new FileReader(kernel_file));            
            if (kernel_content!=null) {
                String str;

                while ((str = kernel_content.readLine()) != null) {
                    if (str.contains("Internal error")) {
                        notify_message(NOTIFY_CRASH_KERNEL, getLastNotification());
                        break;
                    }
                }
            }
            kernel_content.close();
        } catch (java.io.FileNotFoundException e) {
            // that's OK, we probably haven't created it yet
        } catch (Throwable t) {
        }
    }

    @Override
    public void onDestroy() {
      stop();
    }

    private void stop() {
          stopForeground(true);
    }

    @Override
    public void onCreate() {

        super.onCreate();

    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // The service is starting, due to a call to startService()
        SharedPreferences prefs = getSharedPreferences("AppPrefs", MODE_PRIVATE);
        // initialize the coredump list
        coredumpList = getCoredumpList("/data/rfs/data/debug");

        int notif = NOTIFY_STARTED;
        if (prefs.getString("error","NO_CRASH").contains("NOTIFY_CRASH_MODEM"))
            notif = NOTIFY_CRASH_MODEM;
        else if (prefs.getString("error","NO_CRASH").contains("NOTIFY_CRASH_RIL"))
            notif = NOTIFY_CRASH_RIL;
        else if (prefs.getString("error","NO_CRASH").contains("NOTIFY_CRASH_KERNEL"))
            notif = NOTIFY_CRASH_KERNEL;
        // Display notification area messages 
        // and provide access to main application page (click on the notification area).
        notify_message(notif, getLastNotification());

        //Check kernel crash only at start-up
        if (notif == NOTIFY_STARTED) {
            // Check for a previous kernel crash.
            checkKernelState();
        }
        return START_STICKY;
    }

    /**
     * Init list with existing coredump in directory
     *
      * @param directory
     *            the coredump directory.
     */
     public List<String> getCoredumpList(String directory){
           List<String> list = new ArrayList<String>();

           File single = new File("/data/rfs/data/debug");
           if (single.exists())
           {
               File dir = new File(directory);
               String[] listFiles;

               listFiles = dir.list();
               int i;
               for (i = 0; i < listFiles.length; i++) {
                     if (listFiles[i].startsWith("coredump_") || listFiles[i].startsWith("crash_dump")) {
                           list.add(listFiles[i]);
                     }
               }
           }
           return list;
     }

    public void deleteLogFiles()
    {
        SharedPreferences prefs = getSharedPreferences("AppPrefs", 0);
        String[] files = null;
        Settings logging = new Settings();
        // Stop logging before deleting the logs;
        logging.StopLogging();
        // start the logging if not the case
        starter.start();
        try {
            files = fileList();
        } catch (Exception e) {
            e.printStackTrace();
        }

        for (int i = 0; i < files.length; i++) {

            if (!files[i].contentEquals("tracegen.ix"))
                deleteFile(files[i]);
        }

        // Start the genie log
        if (prefs.getBoolean("logging", false))
        {
            logging.GetTracegen();
            logging.StartLogging();
        }
    }

    /**
     * Copy logging tool into /data/data/icera.DebugAgent/app_tools directory
     */
    static Process p = null;

    private void install_tools()
    {
        InputStream in = null;
        OutputStream out = null;
        int size = 0;


        // Create storage directory for tools
        File tools_dir = getDir("tools", Context.MODE_WORLD_READABLE | Context.MODE_WORLD_WRITEABLE);

        try {
            in = getResources().getAssets().open("genie-ios-arm");
            size = in.available();
            // Read the entire resource into a local byte buffer
            byte[] buffer = new byte[size];
            in.read(buffer);
            in.close();

            out = new FileOutputStream(tools_dir.getAbsolutePath() + "/" + "genie-ios-arm");

            out.write(buffer);
            out.close();
            p = Runtime.getRuntime().exec("chmod 777 data/data/icera.DebugAgent/app_tools/genie-ios-arm");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}