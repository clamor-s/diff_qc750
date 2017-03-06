// Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
package icera.DebugAgent;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Date;
import icera.DebugAgent.BugReportService.BugReportServiceBinder;
import icera.DebugAgent.R;
import android.app.Activity;
import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.os.IBinder;
import android.os.StatFs;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import android.util.Log;

public class SaveLogs extends Activity {
    /** Called when the activity is first created. */

    // Binding to BugReportService
    BugReportService mService;
    boolean mBound = false;
    boolean log_flash = false;

    // Output directory name
    private static String directoryname = "";

    LogCatDetect starter = new LogCatDetect(this);
    Settings logging = new Settings();

    Process marker = null;

    /**
     * Definition of input and output files names and paths
     */
    private static final String COREDUMP = "coredump.txt";

    /**
    * Determining output directory name
    */
    private void setDefaultName() {

        String headMessage = "";
        Date date = new Date();
        String Time = date.toString();
        String[] temp = Time.split(" ");
        String[] temp2 = temp[3].split(":");
        String Thetime = temp[0] + "_" + temp[1] + "_" + temp[2] + "_"
                         + temp2[0] + "_" + temp2[1] + "_" + temp2[2];
        int last_crash_type = BugReportService.NOTIFY_STARTED;

        if (mBound) {
            last_crash_type = mService.getLastNotification();
        }

        //Get the exact issue time for the directory name_time
        SharedPreferences sharedPreferences = getSharedPreferences("AppPrefs", MODE_PRIVATE);
        if (sharedPreferences.getString("issue_time", "") != "")
            Thetime = sharedPreferences.getString("issue_time", "");

        switch (last_crash_type) {
        case BugReportService.NOTIFY_CRASH_KERNEL:
            directoryname = Thetime + "_KERNEL";
            headMessage = "Kernel error detected";
            break;
        case BugReportService.NOTIFY_CRASH_MODEM:
            directoryname = Thetime + "_MODEM";
            headMessage = "Modem error detected";
            break;
        case BugReportService.NOTIFY_CRASH_RIL:
            directoryname = Thetime + "_RIL";
            headMessage = "RIL error detected";
            break;
        default:
            directoryname = Thetime + "_LOGS";
            headMessage = "No error detected";
            break;
        }

        // Display default log name
        EditText text = (EditText) findViewById(R.id.Dirname);
        text.setText(directoryname);

        TextView message = (TextView) findViewById(R.id.ErrorType);
        message.setText(headMessage);

        // Reset comment pane
        EditText text2 = (EditText) findViewById(R.id.Comment);
        String empty = "";
        text2.setText(empty);
    }

    /**
    * Called at creation of the activity - display layout - bind to service
    */
    @Override
    public void onCreate(Bundle savedInstanceState) {

        Log.d("Icera","Creating view");

        super.onCreate(savedInstanceState);

        // Display main screen
        setContentView(R.layout.savelogs);

        // Reset comment pane
        EditText text = (EditText) findViewById(R.id.MarkerComment);
        String empty = "";
        text.setText(empty);

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

    /**
    * Copy of the full log directory to external storage
    */
    public void copyDir(String dirname) {

        Log.e("ANDRO_ASYNC",dirname);
        new CopyFilesAsync().execute(dirname);

    }

    public void StoreLogs(View view) {

        int last_crash_type = mService.getLastNotification();

        // Save comment if something is entered in textbox.
        Save_comment();

        Lock.acquire(this);
        // Save crashlog if a modem error occured.
        if (last_crash_type == 3)   // public static final int NOTIFY_CRASH_MODEM = 3;
           Save_crashlog();
        Lock.release();

        // Copy the logs.
        try {
            copyLogs(last_crash_type);
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    public void Settings(View view) {
        // Action to be executed on click.
        Intent settings = new Intent(this, Settings.class);
        startActivity(settings);
        }

    public void DisplayInfo(View view) {
        // Action to be executed on click.
        Intent DisplayInfo = new Intent(this, DisplayInfo.class);
        startActivity(DisplayInfo);
        }

    public void GetKmsg() {

        StringBuilder sb = new StringBuilder();
        BufferedReader br = null;
        Process kmsg = null;
        String kmesg = "";

        // runtime logcat function

        try {

            kmsg = Runtime.getRuntime().exec(new String[] {"dmesg"});
            br = new BufferedReader(new InputStreamReader(kmsg.getInputStream()));

            String line;

            while ((line = br.readLine()) != null) {

                sb.append(line);
                sb.append('\n');

            }

            kmesg = sb.toString();

        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }

        // save the logcat stream
        try {
            FileOutputStream file_kmsg = openFileOutput( "kmsg", Context.MODE_WORLD_READABLE | Context.MODE_WORLD_WRITEABLE );
            file_kmsg.write(kmesg.getBytes());
            file_kmsg.close();
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    /**
     * Get the value for the given key.
     * @return an empty string if the key isn't found
     * @throws IllegalArgumentException if the key exceeds 32 characters
     */
    @SuppressWarnings("unchecked")
    public static String getProp(Context context, String key) throws IllegalArgumentException {

        String ret= "";

        try{

          ClassLoader cl = context.getClassLoader();
          @SuppressWarnings("rawtypes")
          Class SystemProperties = cl.loadClass("android.os.SystemProperties");

          //Parameters Types
          @SuppressWarnings("rawtypes")
              Class[] paramTypes= new Class[1];
          paramTypes[0]= String.class;

          Method get = SystemProperties.getMethod("get", paramTypes);

          //Parameters
          Object[] params= new Object[1];
          params[0]= new String(key);

          ret= (String) get.invoke(SystemProperties, params);

        }catch( IllegalArgumentException iAE ){
            throw iAE;
        }catch( Exception e ){
            ret= "";
            //TODO
        }

        return ret;

    }

    public String save_path = "";

    private void copyLogs(int crash_type) throws IOException {

        String output_dir_name = "";
        String sd_output_dir_name = "";
        String fs_output_dir_name = "";

        StatFs stat = new StatFs(Environment.getExternalStorageDirectory().getPath());

        SharedPreferences sharedPreferences = getSharedPreferences("AppPrefs", MODE_PRIVATE);

        // Get the directory name in case the user modified it
        EditText text = (EditText) findViewById(R.id.Dirname);
        directoryname = text.getText().toString();

        sd_output_dir_name = Environment.getExternalStorageDirectory().getPath() + "/BugReport_Logs/" + directoryname;

        fs_output_dir_name = "/data/rfs/data/debug/BugReport_Logs/" + directoryname;

        // Check available SD card free space
        double sdAvailSize = (double)stat.getAvailableBlocks()
                           * (double)stat.getBlockSize();

        String[] files = null;
        double lenghtOfFiles = 0;

        try{
            files = SaveLogs.this.fileList();
        } catch (Exception e) {
            e.printStackTrace();
        }
        for (int i = 0; i < files.length; i++) {
            File input = new File(getFilesDir()+ "/" +files[i]);
            lenghtOfFiles += input.length();
        }

        // 33554432 is the size of a fullcoredump
        if ( (lenghtOfFiles + 33554432) > sdAvailSize)
        {
            Toast.makeText(this, "SD card full or not present",Toast.LENGTH_LONG).show();
            Toast.makeText(this, "Save logs on the flash",Toast.LENGTH_LONG).show();
            log_flash = true;
            output_dir_name = fs_output_dir_name;
            File fs_output_dir = new File(output_dir_name);
            fs_output_dir.mkdirs();
            save_path = "/data/rfs/data/debug/BugReport_Logs/";
        }
        else
        {
            output_dir_name = sd_output_dir_name;
            File sd_output_dir = new File(output_dir_name);
            sd_output_dir.mkdirs();
            save_path = "inserted SD card";
        }
        // Handle saving process based on crash reported
        if (crash_type == BugReportService.NOTIFY_CRASH_KERNEL) {
            // Copy last kmsg
            Lock.acquire(this);
            copy(getText(R.string.last_kmsg_filename).toString(),
                         output_dir_name + "/last_kmsg");
            Lock.release();
        }

        if (crash_type == BugReportService.NOTIFY_CRASH_MODEM) {
            File single = new File("/data/rfs/data/debug");
            if (single.exists())
            {
                // copy mini coredump in sdcard directory
                if (!sharedPreferences.getBoolean("full_coredump", false ))
                {
                    Lock.acquire(this);
                    copy("/data/rfs/data/debug/"+sharedPreferences.getString("coredump", "" ), output_dir_name + "/coredump.zip");
                    Toast.makeText(this, "INFO : Full coredump is disabled!",
                            Toast.LENGTH_LONG).show();
                    Lock.release();
                    //Runtime.getRuntime().exec(new String[] { "rm /data/rfs/data/debug/crash_dump00001.zip"});
                }
                // copy full coredump in sdcard directory
                else
                {
                    Lock.acquire(this);
                    copy("/data/rfs/data/debug/"+sharedPreferences.getString("coredump", "" ), output_dir_name + "/full_coredump.bin");
                    Toast.makeText(this, "INFO : Full coredump is enabled!",
                            Toast.LENGTH_LONG).show();
                    Lock.release();
                }
            }
            else {
                Toast.makeText(this, "/data/rfs/data/debug/ does not exist",
                        Toast.LENGTH_LONG).show();
            }
        }
        // copy the kmsg log
        GetKmsg();

        // copy other logs in application directory
        File output_dir = new File(output_dir_name);
        if (output_dir.exists()) {
            Lock.acquire(this);
            copyDir(output_dir_name);
            Lock.release();
        }
        else
            Toast.makeText(this, output_dir_name + " does not exist",
                    Toast.LENGTH_LONG).show();

        // reset the previous coredump
        sharedPreferences.edit().putString("coredump", "" );

        if (log_flash == true)
        {
            String fs_output_bugreport = "/data/rfs/data/debug/BugReport_Logs";
            File report = new File(fs_output_bugreport);
            if (report.exists())
            {
                File[] listDir = dirListByDescendingDate(report);

                if (listDir.length > 6)
                {
                    try {
                        Runtime
                                  .getRuntime()
                                  .exec("rm -r "+ listDir[6].getAbsolutePath() );
                    } catch (IOException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                    }
                }
            }
        }
    }

    public void InsertMarker(View view) {

        EditText text = (EditText) findViewById(R.id.MarkerComment);
        String comment = text.getText().toString();

        Date date = new Date();
        String Time = date.toString();
        String[] temp = Time.split(" ");
        String[] temp2 = temp[3].split(":");
        String Thetime = "_" + temp[1] + "-" + temp[2] + "_"
                         + temp2[0] + ":" + temp2[1] + ":" + temp2[2] + "_";
        String command = "AT%IMARKER=MARKER"+Thetime+comment;

        // display marker in logcat main buffer
        Log.d("Icera MARKER",command);

        // display marker in the genie log
        if (marker != null)
            marker.destroy();
        try {
            marker = Runtime
                      .getRuntime()
                      .exec(new String[] {"/system/bin/sh", "-c", "echo \"AT%IMARKER=MARKER\""+Thetime+comment+ " > /dev/ttyACM2"});
            Toast.makeText(this, "MARKER ADDED : "+ command,Toast.LENGTH_LONG).show();
        } catch (IOException e) {
        // TODO Auto-generated catch block
        e.printStackTrace();
        }
    }

    /**
     * Delete previously created logs to avoid recopying them.
     */
    void deleteLogFiles()
    {
        Boolean save_new_log = false;
        int log = 0;
        SharedPreferences prefs = getSharedPreferences("AppPrefs", MODE_PRIVATE);
        String new_log = "";
        //delete the previous crashlog
        BugReportReceiver.crashlog = "No Crashlog / check the coredump";

        File folder = new File("/data/data/icera.DebugAgent/files");
        File[] files_sort = dirListByDescendingDate(folder);


        if (prefs.getString("error","NO_CRASH").contains("NOTIFY_CRASH_MODEM"))
        {
            for (int i = 0; i < files_sort.length; i++) {
                if (files_sort[i].getAbsolutePath().contains("_000.iom") && (save_new_log == false))
                {
                    save_new_log = true;
                    new_log = files_sort[i].getAbsolutePath().split("_000.iom")[0];
                }
            }
            for (int i = 0; i < files_sort.length; i++) {
                if (!files_sort[i].getAbsolutePath().contains("tracegen.ix") && !files_sort[i].getAbsolutePath().contains("daemon.log") && !files_sort[i].getAbsolutePath().contains(new_log))
                    files_sort[i].delete();
                if (files_sort[i].getAbsolutePath().contains("_000.iom"))
                    log++;
            }
            // In case of wrong behavior of the logging tool, restart a proper log
            if (log > 2)
                mService.deleteLogFiles();
            // Start the logcat
            else
                starter.start();
        }
        else if(prefs.getString("error","NO_CRASH").contains("NOTIFY_CRASH_RIL"))
        {
            for (int i = 0; i < files_sort.length; i++)
            {
                if (files_sort[i].getAbsolutePath().contains("logcat_ril_main.txt") || files_sort[i].getAbsolutePath().contains("comment.txt")
                     || files_sort[i].getAbsolutePath().contains("kmsg"))
                    files_sort[i].delete();
                if (files_sort[i].getAbsolutePath().contains("_000.iom"))
                    log++;
            }
            // In case of wrong behavior of the logging tool, restart a proper log
            if (log > 1)
                mService.deleteLogFiles();
            // Start the logcat
            else
                starter.start();
        }
        else {
            for (int i = 0; i < files_sort.length; i++)
            {
                if (files_sort[i].getAbsolutePath().contains("comment.txt") || files_sort[i].getAbsolutePath().contains("kmsg"))
                    files_sort[i].delete();
                if (files_sort[i].getAbsolutePath().contains("_000.iom"))
                    log++;
            }
            // In case of wrong behavior of the logging tool, restart a proper log
            if (log > 1)
                mService.deleteLogFiles();
        }

        File single = new File("/data/rfs/data/debug");
        if (!(single.exists()))
        {// Create Coredump.txt file because RIL cannot do it by itself ...
            try {
                String string = "No Crash Happened";
                FileOutputStream file_coredump = openFileOutput( COREDUMP, Context.MODE_WORLD_READABLE | Context.MODE_WORLD_WRITEABLE );
                file_coredump.write(string.getBytes());
                file_coredump.close();
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }

        // Set the error property to NO_CRASH
        SharedPreferences.Editor prefEditor = prefs.edit();
        prefEditor.putString("error","NO_CRASH");
        prefEditor.commit();
    }

    public void DiscardLogs(View view) {
        SharedPreferences prefs = getSharedPreferences("AppPrefs", MODE_PRIVATE);
        // TODO: remove useless logs
        mService.notify_message(BugReportService.NOTIFY_DISCARD, mService.getLastNotification());
        mService.deleteLogFiles();
        // Set the error property to NO_CRASH
        SharedPreferences.Editor prefEditor = prefs.edit();
        prefEditor.putString("error","NO_CRASH");
        prefEditor.commit();
        this.finish();
    }

    public void Save_comment() {

        EditText text = (EditText) findViewById(R.id.Comment);
        String comment = text.getText().toString();

        try {
            FileOutputStream file_comment = openFileOutput("comment.txt",
                                                           Context.MODE_WORLD_READABLE | Context.MODE_WORLD_WRITEABLE);
            file_comment.write(comment.getBytes());
            file_comment.close();
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    public void Save_crashlog() {

        if (BugReportReceiver.crashlog != "No Crashlog / check the coredump")
        {
        try {
            FileOutputStream file_crashlog = openFileOutput("crashlog.txt",
                                                           Context.MODE_WORLD_READABLE | Context.MODE_WORLD_WRITEABLE);
            file_crashlog.write(BugReportReceiver.crashlog.getBytes());
            file_crashlog.close();
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        }
    }

    public static void copy(String fromFileName, String toFileName)
         throws IOException {
        File fromFile = new File(fromFileName);
        File toFile = new File(toFileName);

        if (!fromFile.exists())
            throw new IOException("FileCopy: " + "no such source file: "
                      + fromFileName);
        if (!fromFile.isFile())
            throw new IOException("FileCopy: " + "can't copy directory: "
                      + fromFileName);
        if (!fromFile.canRead())
            throw new IOException("FileCopy: " + "source file is unreadable: "
                      + fromFileName);

        if (toFile.isDirectory())
            toFile = new File(toFile, fromFile.getName());

        if (toFile.exists()) {
            if (!toFile.canWrite())
                throw new IOException("FileCopy: "
                          + "destination file is unwriteable: " + toFileName);
                System.out.print("Overwrite existing file " + toFile.getName()
                          + "? (Y/N): ");
                System.out.flush();
                BufferedReader in = new BufferedReader(new InputStreamReader(
                          System.in));
                String response = in.readLine();
                if (!response.equals("Y") && !response.equals("y"))
                    throw new IOException("FileCopy: "
                          + "existing file was not overwritten.");
        } else {
            String parent = toFile.getParent();
            if (parent == null)
                parent = System.getProperty("user.dir");
                File dir = new File(parent);
                if (!dir.exists())
                    throw new IOException("FileCopy: "
                              + "destination directory doesn't exist: " + parent);
                if (dir.isFile())
                    throw new IOException("FileCopy: "
                              + "destination is not a directory: " + parent);
                if (!dir.canWrite())
                    throw new IOException("FileCopy: "
                              + "destination directory is unwriteable: " + parent);
        }

        FileInputStream from = null;
        FileOutputStream to = null;
        try {
            from = new FileInputStream(fromFile);
            to = new FileOutputStream(toFile);
            byte[] buffer = new byte[4096];
            int bytesRead;

            while ((bytesRead = from.read(buffer)) != -1)
                to.write(buffer, 0, bytesRead); // write
        } finally {
            if (from != null)
                try {
                    from.close();
                } catch (IOException e) {
                    ;
                }
            if (to != null)
                try {
                    to.close();
                } catch (IOException e) {
                    ;
                }
        }
    }

    @SuppressWarnings({ "unchecked", "rawtypes" })
    public static File[] dirListByDescendingDate(File folder) {
        if (!folder.isDirectory()) {
            return null;
        }
        File files[] = folder.listFiles();
        Arrays.sort( files, new Comparator()
        {
            public int compare(final Object o1, final Object o2) {
                return new Long(((File)o2).lastModified()).compareTo
                      (new Long(((File) o1).lastModified()));
            }
        });
        return files;
    }

    /** Defines callbacks for service binding, passed to bindService() */
    private ServiceConnection mConnection = new ServiceConnection() {

        public void onServiceConnected(ComponentName className, IBinder service) {
            // We've bound to LocalService, cast the IBinder and get
            // LocalService instance
            BugReportServiceBinder binder = (BugReportServiceBinder) service;
            mService = binder.getService();
            mBound = true;
            setDefaultName();
        }

        public void onServiceDisconnected(ComponentName arg0) {
            mBound = false;
        }
    };

class CopyFilesAsync extends AsyncTask<String, String, Boolean> {

    private ProgressDialog dialog;

    @Override
    // can use UI thread here
    protected void onPreExecute() {
        int current_orientation = getResources().getConfiguration().orientation;
        if (current_orientation == Configuration.ORIENTATION_LANDSCAPE) {
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        } else {
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        }
        dialog = new ProgressDialog(SaveLogs.this);
        dialog.setMessage("Saving logs under "+ save_path);
        dialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        dialog.setCancelable(false);
        dialog.show();
    }
    @Override
    protected Boolean doInBackground(String... dirname) {
        String[] files = null;
        byte[] buffer = new byte[1024];
        int read;
        long total = 0;
        long lenghtOfFiles = 0;

        try{
            files = SaveLogs.this.fileList();
        } catch (Exception e) {
            e.printStackTrace();
        }
        for (int i = 0; i < files.length; i++) {
            File input = new File(getFilesDir()+ "/" +files[i]);
            lenghtOfFiles += input.length();
        }
        for (int i = 0; i < files.length; i++) {
            InputStream in = null;
            OutputStream out = null;
            try {
                in = SaveLogs.this.openFileInput(files[i]);
                out = new FileOutputStream(dirname[0] + "/" + files[i]);
                while ((read = in.read(buffer)) != -1) {
                    total += read;
                    publishProgress(""+(int)((total*100)/lenghtOfFiles));
                    out.write(buffer, 0, read);
                }
                in.close();
                in = null;
                out.flush();
                out.close();
                out = null;
                } catch (Exception e) {
                    e.printStackTrace();
                    return false;
               }
            }
        return true;
    }
    @Override
    protected void onProgressUpdate(String... progress) {
        dialog.setProgress(Integer.parseInt(progress[0]));
    }
    @Override
    protected void onPostExecute(Boolean result) {
        try {
            dialog.dismiss();
            dialog = null;
        } catch (Exception e) {
            // nothing
        }
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR);
        if (result = true)
        {
            mService.notify_message(BugReportService.NOTIFY_SAVING_OK,mService.getLastNotification());
            deleteLogFiles();
        }
        else
        {
            mService.notify_message(BugReportService.NOTIFY_SAVING_ISSUE,mService.getLastNotification());
        }
        finish();
    }
}
}