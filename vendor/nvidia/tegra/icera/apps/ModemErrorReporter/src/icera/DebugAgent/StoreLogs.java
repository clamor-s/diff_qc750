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

public class StoreLogs extends Activity {
    /** Called when the activity is first created. */

    boolean log_flash = false;

    // Output directory name
    private static String directoryname = "";

    /**
    * Called at creation of the activity - display layout - bind to service
    */
    @Override
    public void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

        StoreLogsOnly(null);

    }

    /**
    * Called when stopping the activity - unbind service
    */
    @Override
    protected void onStop() {
        super.onStop();
    }

    /**
    * Copy of the full log directory to external storage
    */
    public void copyDir(String dirname) {

        Log.e("ANDRO_ASYNC",dirname);
        new CopyFilesAsync().execute(dirname);

    }

    public void StoreLogsOnly(View view) {

        // Copy the logs.
        try {
            copyLogs();
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
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


    public String save_path = "";

    private void copyLogs() throws IOException {

        String output_dir_name = "";
        String sd_output_dir_name = "";
        String fs_output_dir_name = "";

        Date date = new Date();
        String Time = date.toString();
        String[] temp = Time.split(" ");
        String[] temp2 = temp[3].split(":");
        String Thetime = temp[0] + "_" + temp[1] + "_" + temp[2] + "_"
                         + temp2[0] + "_" + temp2[1] + "_" + temp2[2];
        directoryname = Thetime + "_LOGS";
        StatFs stat = new StatFs(Environment.getExternalStorageDirectory().getPath());

        sd_output_dir_name = Environment.getExternalStorageDirectory().getPath() + "/BugReport_Logs/" + directoryname;

        fs_output_dir_name = "/data/rfs/data/debug/BugReport_Logs/" + directoryname;

        // Check available SD card free space
        double sdAvailSize = (double)stat.getAvailableBlocks()
                           * (double)stat.getBlockSize();

        String[] files = null;
        double lenghtOfFiles = 0;

        try{
            files = StoreLogs.this.fileList();
        } catch (Exception e) {
            e.printStackTrace();
        }
        for (int i = 0; i < files.length; i++) {
            File input = new File(getFilesDir()+ "/" +files[i]);
            lenghtOfFiles += input.length();
        }

        if ( lenghtOfFiles > sdAvailSize)
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
        dialog = new ProgressDialog(StoreLogs.this);
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
            files = StoreLogs.this.fileList();
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
                in = StoreLogs.this.openFileInput(files[i]);
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
        finish();
    }
}
}