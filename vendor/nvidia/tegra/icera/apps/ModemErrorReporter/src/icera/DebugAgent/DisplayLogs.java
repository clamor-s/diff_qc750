// Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
package icera.DebugAgent;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import android.app.AlertDialog;
import android.app.ListActivity;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Toast;

public class DisplayLogs extends ListActivity {
    /** Called when the activity is first created. */

    private File file;
    private List<String> myList;

    public void onCreate(Bundle savedInstanceState) {


        super.onCreate(savedInstanceState);

        myList = new ArrayList<String>();

        String root_sd = Environment.getExternalStorageDirectory().toString();
        file = new File( root_sd + "/BugReport_Logs" ) ;
        file.mkdirs();

        if(!file.exists())
        {
            file = new File( "/data/rfs/data/debug/BugReport_Logs" ) ;
            file.mkdirs();
        }
        File list[] = file.listFiles();

        for( int i=0; i< list.length; i++)
        {
            myList.add( list[i].getName() );
        }

        setListAdapter(new ArrayAdapter<String>(this,
                android.R.layout.simple_list_item_1, myList ));

    }

    protected void onListItemClick(ListView l, View v, int position, long id)
    {
        super.onListItemClick(l, v, position, id);

        final File temp_file = new File( file, myList.get( position ) );

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setMessage("Are you sure you want to delete "+temp_file.getName()+" ?")
               .setCancelable(false)
               .setPositiveButton("Yes", new DialogInterface.OnClickListener() {
                   public void onClick(DialogInterface dialog, int id) {
                       deleteDirectory(temp_file);
                       DisplayLogs.this.finish();
                   }
               })
               .setNegativeButton("No", new DialogInterface.OnClickListener() {
                   public void onClick(DialogInterface dialog, int id) {
                        dialog.cancel();
                   }
               });
        builder.show();

    }

    @Override
    public void onBackPressed() {
        DisplayLogs.this.finish();
    }

    static public boolean deleteDirectory(File path) {
        if( path.exists() ) {
          File[] files = path.listFiles();
          if (files == null) {
              return true;
          }
          for(int i=0; i<files.length; i++) {
             if(files[i].isDirectory()) {
               deleteDirectory(files[i]);
             }
             else {
               files[i].delete();
             }
          }
        }
        return( path.delete() );
      }

}