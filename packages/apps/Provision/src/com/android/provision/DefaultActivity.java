/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.provision;

import android.app.Activity;
import android.content.ComponentName;
import android.content.pm.PackageManager;
import android.graphics.drawable.AnimationDrawable;
import android.os.Bundle;
import android.os.SystemProperties;
import android.provider.Settings;
import android.util.Log;
import android.widget.ImageView;
import android.app.Activity;
import android.view.Menu;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.net.Uri;
import android.provider.Browser;
import android.util.Log;
import android.webkit.WebIconDatabase;
import android.widget.Toast;
import android.util.Log;
import java.io.ByteArrayOutputStream;
import android.media.AudioManager;
import android.media.AudioSystem;
import java.util.Date;
/**
 * Application that sets the provisioned bit, like SetupWizard does.
 */
public class DefaultActivity extends Activity {

    ImageView imageView;
    AnimationDrawable mDrawable;
    AudioManager mAudioManager;
    private final String TAG="Bookmark";
    private int mSafeMediaVolumeIndex;	
     private int mMediaVolumeIndex;
    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setContentView(R.layout.animation);
        imageView = (ImageView) findViewById(R.id.imageView);
        mDrawable = new AnimationDrawable();
        for (int i = 0; i <= 23; i++) {
            int id = getResources().getIdentifier("p"+(3000+i), "drawable", "com.android.provision");
            mDrawable.addFrame(getResources().getDrawable(id), 50);
        }
        mDrawable.setOneShot(false);
        imageView.setImageDrawable(mDrawable);
        mDrawable.start();
        boolean productionMode =SystemProperties.getBoolean("sys.kh.production_mode", false);
        if(productionMode){
            Settings.Secure.putInt(getContentResolver(),
                           Settings.Global.ADB_ENABLED, 1);
        }
	
        // Add a persistent setting to allow other apps to know the device has been provisioned.
        Settings.Global.putInt(getContentResolver(), Settings.Global.DEVICE_PROVISIONED, 1);
	mAudioManager = (AudioManager)getSystemService(AUDIO_SERVICE);
	mSafeMediaVolumeIndex =8;
	mMediaVolumeIndex = 7;
	 int index =  mAudioManager.getStreamVolume(AudioSystem.STREAM_MUSIC);
	 Log.i(TAG, "mAudioManager.getStreamVolume:"+index);
	 if(index > mSafeMediaVolumeIndex){
	 	mAudioManager.setStreamVolume(AudioSystem.STREAM_MUSIC,mSafeMediaVolumeIndex, 0);
	 }
	 else{
	 	mAudioManager.setStreamVolume(AudioSystem.STREAM_MUSIC,mMediaVolumeIndex, 0);
	 }
        mJobThread.start();
    }
    void done(){
        // terminate the activity.
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                // TODO Auto-generated method stub
             // remove this activity from the package manager.
                PackageManager pm = getPackageManager();
                ComponentName name = new ComponentName(DefaultActivity.this, DefaultActivity.class);
                pm.setComponentEnabledSetting(name, PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                        PackageManager.DONT_KILL_APP);
                mDrawable.stop();
                finish();
            }
        });
    }
    Thread mJobThread = new Thread("doing factory..."){
        public void run() {
            new InstallFactoryStuff().install(DefaultActivity.this);
            done();
        };
    };
}

