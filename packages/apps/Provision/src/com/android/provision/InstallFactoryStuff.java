package com.android.provision;

import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.IPackageDataObserver;
import android.content.pm.IPackageInstallObserver;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.ConditionVariable;
import android.os.Environment;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.util.Log;
import android.provider.Settings;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;

public class InstallFactoryStuff {
    void install(Context context){
        Log.d(TAG,"install stuff...");
        boolean factoryMode =isInFactoryMode();
        Log.d(TAG,"mode is "+(factoryMode?"fatory, will install sometings":"normal, no install"));
        mContext =context;
        if(factoryMode){
	     Settings.System.putInt(mContext.getContentResolver(), Settings.System.ACCELEROMETER_ROTATION, 0);
            getAllMediaFile();
            installMediaFile("media");
            getAllApk("app-test");
            getAllTestFile();
            installMediaFile("test");
            if(mInstallApks.size()>0){
                precessNextApk();
                mLock.block();
            }
	   Settings.Secure.putInt(mContext.getContentResolver(),Settings.Global.PACKAGE_VERIFIER_ENABLE,0);
	   Settings.Secure.putInt(mContext.getContentResolver(),Settings.Global.PACKAGE_VERIFIER_INCLUDE_ADB,0);
        }else{
            getAllMediaFile();
            installMediaFile("media");
            getAllApk("app");
            if(mInstallApks.size()>0){
                precessNextApk();
                mLock.block();
            }
	     
        }
         Settings.System.putInt(mContext.getContentResolver(), Settings.System.ACCELEROMETER_ROTATION, 1);
			initClearLauncherUserData();
        mContext =null;
    }
    static boolean isInFactoryMode(){
        return SystemProperties.getBoolean(PRODUCT_MODE_PROPERTY, false);
    }
    private final static String PRODUCT_MODE_PROPERTY = "sys.kh.production_mode";
    private String TAG =PreBootReceiver.class.getSimpleName();
    private Context mContext;
    private final String mMountPath ="/vendor/test/";
    final InstallObserver mInstallObserver =new InstallObserver();
    ArrayList<String> mInstallApks =new ArrayList<String>();
    ArrayList<String> mMediaFiles =new ArrayList<String>();
    ConditionVariable mLock =new ConditionVariable(false);

    class InstallObserver extends IPackageInstallObserver.Stub{
	@Override
        public void packageInstalled(String packageName, int returnCode) throws RemoteException {
            // TODO Auto-generated method stub
            Log.d(TAG,"installing "+packageName+" returnCode="+returnCode);
            precessNextApk();
        }
    }

    void setDone(){
        mLock.open();
    }

    void precessNextApk(){
        String next =null;
        ArrayList<String> apkStrings;
        apkStrings =  mInstallApks;
        Log.d(TAG,"precessNext count="+apkStrings.size());
        synchronized (apkStrings) {
            if(apkStrings.size()<=0){
                setDone();
                return;
            }
            next =apkStrings.remove(0);
        }
        if(next !=null){
            PackageManager pm =mContext.getPackageManager();
            pm.installPackage(Uri.parse(next), mInstallObserver, PackageManager.INSTALL_REPLACE_EXISTING, mContext.getPackageName());
        }
        else{
            precessNextApk();
        }
    }

    void getAllApk(String path){
        File srcApp =new File(mMountPath+path);
        mInstallApks.clear();
        searchApkDir(srcApp);
        Log.d(TAG,"we got pre_install apk, size="+mInstallApks.size());
    }

    void searchApkDir(File dir){
        File allAppName[] =dir.listFiles();
        if(allAppName==null){
            return;
        }
        for(File one:allAppName){
            if(!one.getName().endsWith(".apk")){
                if(one.isDirectory()){
                    searchApkDir(one);
                }else{
                    Log.d(TAG,"find an invalid file "+one);
                }
            }
            else{
                mInstallApks.add(one.getAbsolutePath());
            }
        }
    }

    void getAllTestFile(){
        File srcMedia =new File(mMountPath+"test");
        mMediaFiles.clear();
        searchMediaDir(srcMedia);
    }

    void getAllMediaFile(){
        File srcMedia =new File(mMountPath+"media");
        mMediaFiles.clear();
        searchMediaDir(srcMedia);
    }

    void searchMediaDir(File dir){
        File allAppName[] =dir.listFiles();
        if(allAppName ==null){
            return;
        }
        for(File one:allAppName){
             if(one.isDirectory()){
                 searchMediaDir(one);
            }
            else{
                mMediaFiles.add(one.getAbsolutePath());
            }
        }
    }

    void installMediaFile(String type){
        int size =mMediaFiles.size();
        if(size==0){
            return;
        }
        String sdcard =Environment.getExternalStorageDirectory().getAbsolutePath();
        for(int one =0; one<size; one++){
            String src =mMediaFiles.get(one);
            String des=src.replaceFirst(mMountPath+type, sdcard);
            Log.d(TAG,"copy to "+src);
            copyFile(new File(src), new File(des));
        }
        restartMediaScan();
    }

    void restartMediaScan(){
        Intent intent =new Intent(Intent.ACTION_MEDIA_MOUNTED);
        Uri uri =Uri.fromFile(new File("/sdcard/"));
        intent.setData(uri);
        mContext.sendBroadcast(intent);
    }

    void copyFile(File from, File to){
        to.getParentFile().mkdirs();
        to.delete();
        byte[] buffer =new byte[1024];
        try {
            FileInputStream src =new FileInputStream(from);
            FileOutputStream des = new FileOutputStream(to);
            int count;
            while((count=src.read(buffer)) !=-1){
                des.write(buffer, 0, count);
            };
            des.flush();
            src.close();
            des.close();
        } catch (FileNotFoundException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            return;
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

	
	private void initClearLauncherUserData() {
			String packageName = "com.android.launcher";
			Log.i(TAG, "initClearLauncherUserData");
			ActivityManager am = (ActivityManager)mContext.getSystemService(Context.ACTIVITY_SERVICE);
			am.clearApplicationUserData(packageName, (IPackageDataObserver) new ClearUserDataObserver() );
		}
		
		class ClearUserDataObserver extends IPackageDataObserver.Stub {
			public void onRemoveCompleted(final String packageName, final boolean succeeded) {
				if (!succeeded) {
					// Clearing data failed for some obscure reason. Just log error for now
					Log.i(TAG, "Couldnt clear application user data for package:"+packageName);
				} else {
					Log.i(TAG, "Clear application user data for package:"+packageName);
				}
			 }
		 }
	
}
