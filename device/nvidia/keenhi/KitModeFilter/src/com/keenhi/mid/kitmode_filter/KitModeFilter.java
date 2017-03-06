package com.keenhi.mid.kitmode_filter;

import android.app.ActivityManagerNative;
import android.app.AppGlobals;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.IPackageManager;
import android.content.pm.PackageInfo;
import android.database.ContentObserver;
import android.database.Cursor;
import android.net.Uri;
import android.os.Binder;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.os.Process;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.provider.Settings;
import android.util.Slog;

import java.util.ArrayList;
import java.util.Set;

/**
 * created by densy.
 * @hide
 */
public class KitModeFilter {
    
    final static String TAG = KitModeFilter.class.getSimpleName();
    
    private Context mContext;
    ContentObserver mWhiteListObserver;
    ArrayList<String> mWhiteLists;
    Handler mHandler;
    Handler mUiHandler;
    
    public final String KIT_SERVICE_MODE_PROPERTY ="sys.keenhi.kit_mode";
    final int KIT_EXTRA_KIT_MODE_INVALID =0;
    final int KIT_EXTRA_KIT_MODE_KIT =1;
    final int KIT_EXTRA_KIT_MODE_ANDROID =2;
    
    public final String mWarnNoActivityFoundAction ="com.keenhi.android.action.kit_filter_warnning";
    
    final static String[] mActionFilter ={
        Intent.ACTION_VIEW,
    };
    
    final static String[] mMustIntentFilter ={
        Settings.ACTION_USER_DICTIONARY_INSERT
    };
    
    final static String[] mHomeFilter ={
        Intent.CATEGORY_HOME,
    };
    
    final static String[] mSchemeFilter ={
        "market",
        "http",
        "https",
    };
    
    boolean isHomeIntent(Intent intent){
        Set<String> categories =intent.getCategories();
        if(categories==null){
            return false;
        }
        
        for(String one:mHomeFilter){
            if(categories.contains(one)){
                return true;
            }
        }
        
        return false;
    }
    
    boolean isMustActionMatch(Intent intent){
        String action =intent.getAction();
        if(action==null){
            return false;
        }
        
        for(String one:mMustIntentFilter){
            if(one.equals(action)){
                return true;
            }
        }
        
        return false;
    }
    
    boolean isActionMatch(Intent intent){
        String action =intent.getAction();
        if(action==null){
            return false;
        }
        
        for(String one:mActionFilter){
            if(one.equals(action)){
                return true;
            }
        }
        
        return false;
    }
    
    boolean isSchemeMatch(Intent intent){
        String scheme =intent.getScheme();
        if(scheme==null){
            return false;
        }
        
        for(String one:mSchemeFilter){
            if(one.equals(scheme)){
                return true;
            }
        }
        
        return false;
    }
    
    final static String[] mWhiteListHead ={
        "http",
        "https",
    };
    
    final static String[] mWhiteListsSuffix={
        "mp3",
        "mp4"
    };
    
    boolean isInWhiteList(Intent intent){
        String scheme =intent.getScheme();
        if(scheme==null){
            return false;
        }
        scheme =new String(scheme).toLowerCase();
        String callerPackageName =getCallerPackage();
        if(callerPackageName==null){
            return true;
        }
        
        String path =intent.getDataString();
        if(path==null){
            return false;
        }
        
        Slog.d(TAG,"checking white list "+callerPackageName+" path="+path);
        
        for(String one:mWhiteListHead){
            if(one.equals(scheme)){
                for(String suffix:mWhiteListsSuffix){
                    if(path.endsWith(suffix)){
                        synchronized (mWhiteLists) {
                            if(mWhiteLists.contains(callerPackageName)){
                                Slog.d(TAG,"got WL name is " +callerPackageName);
                                return true;
                            }
                        }
                    }
                }
            }
            
        }
        
        return false;
    }
    
    public KitModeFilter(){
        Slog.d(TAG,"yehh...create me");
    }
    
    public void init(Context context){
        
        mContext =context;
        
        mWhiteLists =new ArrayList<String>();
        
        HandlerThread handlerThread =new HandlerThread("kh_filterThread");
        handlerThread.start();
        mHandler =new Handler(handlerThread.getLooper()){
            @Override
            public void handleMessage(Message msg) {
                // TODO Auto-generated method stub
                super.handleMessage(msg);
            }
        };
        mUiHandler =new Handler();
        
        mHandler.postDelayed(mCheckSystemOk,1000);
    }
    
    Runnable mCheckSystemOk =new Runnable() {
        
        @Override
        public void run() {
            // TODO Auto-generated method stub
            
            Slog.d(TAG, "checking system");
            
            if(ActivityManagerNative.getDefault()==null){
                mHandler.postDelayed(mCheckSystemOk, 1000);
                Slog.d(TAG,"system not ok wait "+1000+"ms");
                return;
            }
            Slog.d(TAG,"system ok now...");
            
            mUiHandler.post(new Runnable() {
                
                @Override
                public void run() {
                    // TODO Auto-generated method stub
                    IntentFilter intentFilter =new IntentFilter(Intent.ACTION_BOOT_COMPLETED);
                    mContext.registerReceiver(mBootCompleteReceiver, intentFilter);
                }
            });
        }
    };
    
    Uri mWhiteListUri =Uri.parse("content://com.fuhu.kidzmode.settings/app_table/?notify=true");
    Uri mWhiteListUri_notify =Uri.parse("content://com.fuhu.kidzmode.settings/app_table");
    
    void initWhiteList(){
        
        _updateWhiteList();
        
        mWhiteListObserver =new ContentObserver(mHandler) {
            @Override
            public void onChange(boolean selfChange) {
                // TODO Auto-generated method stub
                super.onChange(selfChange);
                updateWhileList();
            }
        };
        
        ContentResolver cr =mContext.getContentResolver();
        cr.registerContentObserver(mWhiteListUri_notify, true, mWhiteListObserver);
    }
    
    void updateWhileList(){
        Slog.d(TAG,"updateWhileList...");
        mHandler.removeCallbacks(mUpdateWhiteListRunnable);
        mHandler.post(mUpdateWhiteListRunnable);
    }
    
    private Runnable mUpdateWhiteListRunnable =new Runnable() {
        
        @Override
        public void run() {
            // TODO Auto-generated method stub
            _updateWhiteList();
        }
    };
    
    void _updateWhiteList(){
        
        Slog.d(TAG, "doing update");
        
        String[] columns =new String[]{
                "_id",
                "package_name",
        };
        
        ContentResolver cr =mContext.getContentResolver();
        Cursor cursor=null;
        try{
            cursor =cr.query(mWhiteListUri, columns, null, null, null);
        }
        catch (NullPointerException e) {
            // TODO: handle exception
        }
        
        if(cursor==null){
            return;
        }
        
        ArrayList<String> newList =new ArrayList<String>();
        
        int count =cursor.getCount();
        try{
            Slog.d(TAG, "count is "+count);
            
            if(count<=0){
                return;
            }
            int packageIndex =cursor.getColumnIndex("package_name");
            
            cursor.moveToFirst();
            while(!cursor.isAfterLast()){
                String packageName =cursor.getString(packageIndex);
                
                if(packageName!=null && !newList.contains(packageName)){
                    newList.add(packageName);
                    Slog.d(TAG, "find one packageName="+packageName);
                }
                cursor.moveToNext();
            }
            cursor.close();
            
            synchronized (mWhiteLists) {
                mWhiteLists =newList;
            }
            
        }
        finally{
            Slog.d(TAG,"finish update");
        }
    }
    
    BroadcastReceiver mBootCompleteReceiver =new BroadcastReceiver() {
        
        @Override
        public void onReceive(Context context, Intent intent) {
            // TODO Auto-generated method stub
            Slog.d(TAG,"boot complete");
            initWhiteList();
        }
    };
    /*
    boolean isWhiteList(Intent intent){
        
        if(intent==null){
            return false;
        }
        
        synchronized (mWhiteLists) {
            
            String packageName =intent.getPackage();
            
            if(packageName==null){
                
            }
        }
    }
    */
    
    void toWarn(Intent intent){
        Slog.d(TAG, "start warnning app");
        intent.setAction(null);
        intent.setData(null);
        intent.setType(null);
        intent.setComponent(new ComponentName("com.keenhi.mid.kitservice", "com.keenhi.mid.kitservice.WarnningNoFilterActivity"));
    }
    
    //for action filter                                                                                                           
    public boolean kitModeFilter(Intent intent){
        if(intent==null)
            return false;
        
        int mode = SystemProperties.getInt(KIT_SERVICE_MODE_PROPERTY, KIT_EXTRA_KIT_MODE_INVALID);
        
        if(mode ==KIT_EXTRA_KIT_MODE_INVALID){
            return false;
        }
        
        ComponentName cp =intent.getComponent();
        if(cp!=null){
            //TBD
        }
        
        if(mode == KIT_EXTRA_KIT_MODE_KIT){
            
            if(isHomeIntent(intent) && !callerIsSystemApk(intent)){
                intent.setAction(null);
                intent.setData(null);
                intent.setType(null);
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                intent.setComponent(new ComponentName("com.fuhu.kidzmode", "com.fuhu.kidzmode.Launcher"));
                return true;
            }
            
            boolean kill =false;
            try{
                if(isMustActionMatch(intent)){
                    kill=true;
                    return true;
                }
                
                if(isInWhiteList(intent)){
                    return false;
                }
                
                if(!isActionMatch(intent)){
                    return false;
                }
                
                if(!isSchemeMatch(intent)){
                    return false;
                }
                kill=true;
            }
            finally{
                if(kill){
                    toWarn(intent);
                }
            }
            
            
            return true;
        }
        Slog.e(TAG, "action = "+intent.getAction()+" data="+((intent.getData()!=null)?intent.getData().toString():null)+
                " comp="+((intent.getComponent()!=null)?intent.getComponent().toString():null)
                +" type="+intent.getType());
        return false;                                                                                                               
    }
    
    String getCallerPackage(){
        int callingUid = Binder.getCallingUid();
        
        IPackageManager pm =AppGlobals.getPackageManager();
        try {
            String[] packagenames =pm.getPackagesForUid(callingUid);
            
            if(packagenames!=null && packagenames.length>=1){
                String packagename =packagenames[0];
                
                Slog.d(TAG, "caller2 packagename="+packagename);
                
                return packagename;
            }
            
        } catch (RemoteException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        
        return null;
    }
    
    boolean callerIsSystemApk(Intent intent){
        int callingUid = Binder.getCallingUid();
        
        if(callingUid ==Process.SYSTEM_UID ||callingUid ==0){
            return false;
        }
        
        IPackageManager pm =AppGlobals.getPackageManager();
        try {
            String[] packagenames =pm.getPackagesForUid(callingUid);
            
            if(packagenames!=null && packagenames.length>=1){
                String packagename =packagenames[0];
                
                Slog.d(TAG, "caller packagename="+packagename);
                
                PackageInfo pi =pm.getPackageInfo(packagename, 0,0);
                if(pi!=null){
                    ApplicationInfo ai =pi.applicationInfo;
                    if(ai!=null){
                        if((ai.flags & ApplicationInfo.FLAG_SYSTEM)!=0){
                            Slog.d(TAG,"let it to, system app");
                            return true;
                        }
                    }
                }
            }
            
        } catch (RemoteException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        
        return false;
    }
}
