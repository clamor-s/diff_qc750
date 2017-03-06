/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

package com.nvidia.nvwfdservicetest;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;
import android.os.Bundle;
import android.widget.Toast;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.Process;
import java.util.Random;
import android.os.Binder;
import android.content.Context;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.widget.Button;
import android.view.View;
import android.widget.RemoteViews;
import java.util.ArrayList;
import java.util.List;
import com.nvidia.nvwfd.*;

public class NvwfdService extends Service {

    private Looper mServiceLooper;
    private ServiceHandler mServiceHandler;
    private Context mContext;
    private Intent mIntent;
    private NotificationManager mNotificationManager;
    private static final String TAG = "NvwfdServiceTest";
    private NvwfdSinkInfo mNvwfdSink;
    private Nvwfd mNvwfd;
    private List<NvwfdSinkInfo> mSinks;
    private NvwfdConnection mConnection;
    private NvwfdSinkInfo mConnectedSink;
    private NvwfdPolicyManager mNvwfdPolicyManager;
    private NvwfdServiceListener mNvwfdServiceListenerCallback;
    private boolean mHandleMessage = false;
    private boolean mInit = false;
    private NvwfdListener mNvwfdListener;
    //binder object receives interactions from clients.
    private final IBinder mBinder = new NvwfdServiceBinder();
    private static final int SINKSFOUND = 1;
    private static final int CONNECTDONE = 2;
    private static final int DISCONNECT = 3;
    private static final int DISCOVERYFAILED = 4;
    private static final int DISCONNECTCOMPLETED = 5;
    private static long mDiscoveryTs;
    private static long mStartConnectionTs;
    private static long mDisconnectionTs;
    private String qosMetric;
    private String connectedSinkId = null;
    public int mResolutionCheckId;
    public boolean bForceResolution;
    public int mPolicySeekBarValue;
   /**
     * Class for clients to access.
     */
    public class NvwfdServiceBinder extends Binder implements NvwfdServiceMonitor {
        NvwfdService getService() {
            return NvwfdService.this;
        }
        public void registerCallback(NvwfdServiceListener callback) {
            mNvwfdServiceListenerCallback = callback;
        }
    }

    private class NvwfdListener implements Nvwfd.NvwfdListener {
        @Override
        public void onDiscoverSinks(NvwfdSinkInfo sink) {
            String text = sink.getSsid();
            Log.d(TAG, "QoS:Sink " + text +" found After :"+ (System.currentTimeMillis() -mDiscoveryTs) + " millisecs");
            mSinks.add(sink);
            Message msg = new Message();
            msg.what = SINKSFOUND;
            msg.obj = (Object)sink;
            mServiceHandler.sendMessage(msg);
        }
        @Override
        public void onConnect(NvwfdConnection connection) {
            if (connection == null) {
                Log.d(TAG, "onConnect:: Connection is NULL failed");
            } else {
                Log.d(TAG, "onConnect:: Connection succeeded");
            }
            mConnection = connection;
            mServiceHandler.sendEmptyMessage(CONNECTDONE);
        }
        @Override
        public void discoverSinksResult(boolean result) {
            Log.d(TAG, "discoverSinksResult " + result);
            if (result == false) {
                Message msg = new Message();
                msg.what = DISCOVERYFAILED;
                mServiceHandler.sendMessage(msg);
            }
        }
    }

    private class NvwfdAppListener implements NvwfdPolicyManager.NvwfdAppListener {
        @Override
        public void onDisconnect() {
            Log.d(TAG, "onDisconnect - detection from policy manager  ");
            mDisconnectionTs = System.currentTimeMillis();
            Message msg = new Message();
            msg.what = DISCONNECT;
            msg.obj = null;
            mServiceHandler.sendMessage(msg);
        }
    }

    private class NvwfdDisconnectListener implements NvwfdConnection.NvwfdDisconnectListener {

        @Override
        public void onDisconnectResult(boolean result) {
            Log.d(TAG, "onDisconnectResult " + result);
            Message msg = new Message();
            msg.what = DISCONNECTCOMPLETED;
            mServiceHandler.sendMessage(msg);
        }
    };

    // Handler that receives messages from the thread
    private final class ServiceHandler extends Handler {
        public ServiceHandler(Looper looper) {
          super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            Log.d(TAG, "Message: " + msg);
            handleNotification(msg);
        }
    }

    @Override
    public void onCreate() {
        // Start the thread running the service and in background priority.
        Log.v(TAG, "+StartAtBootService Created");
        mNotificationManager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
        HandlerThread thread = new HandlerThread("ServiceStartArguments",
            Process.THREAD_PRIORITY_BACKGROUND);
        Log.v(TAG, "thread start");
        thread.start();
        mContext = getApplicationContext();
        // Get the HandlerThread's Looper and use it for our Handler
        mServiceLooper = thread.getLooper();
        mServiceHandler = new ServiceHandler(mServiceLooper);
        Log.v(TAG, "start new Intent and Nvwfd stuff");
        mIntent = new Intent(mContext, NvwfdServiceActivity.class);
        mNvwfd = new Nvwfd();
        mNvwfdListener = new NvwfdListener();
        mSinks = new ArrayList<NvwfdSinkInfo>();
        mResolutionCheckId = 1;
        bForceResolution = false;
        mPolicySeekBarValue = 50;
        Log.v(TAG, "-StartAtBootService Created");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // start request be handled by message. start ID to track request on finish the work etc.,.,

        Message msg = mServiceHandler.obtainMessage();
        msg.arg1 = startId;
        msg.arg2 = flags;
        if (intent != null) {
            Log.d(TAG, "onStartCommand:: Starting #" + startId + ": " + intent.getExtras());
            msg.obj = intent.getExtras();
        }
        mServiceHandler.sendMessage(msg);
        Log.d(TAG, "Sending: " + msg);
        // If we get killed, after returning from here, restart
        return START_STICKY;
    }

    public boolean isNvwfdInit() {
        return mInit;
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.d(TAG, "+onBind: mInit :" + mInit);
        if (mInit == false) {
            mInit = mNvwfd.init(mContext, mNvwfdListener);
        }
        Log.d(TAG, "-onBind: mInit :" + mInit);
        return mBinder;
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "onDestroy");
        if(mInit && (mConnection == null)) {
            Log.d(TAG, "onDestroy - mOnDisconnectClicked");
            mNvwfd.deinit();
            mInit = false;
        }
    }

    public void discoverSinks() {
        mSinks.clear();
        mConnectedSink = null;
        // qos discover time start
        mDiscoveryTs = System.currentTimeMillis();
        mNvwfd.discoverSinks();
    }

    public void onDisconnect() {
        Log.d(TAG, "onDisconnect -button preses ");
        mDisconnectionTs = System.currentTimeMillis();
        Message msg = new Message();
        msg.what = DISCONNECT;
        msg.obj = null;
        mServiceHandler.sendMessage(msg);
    }

    public void mapConnectedSinkIndex(int index) {
        mConnectedSink = mSinks.get(index);
    }

    public List<NvwfdSinkInfo> getConnectedSinkList() {
        return mSinks;
    }

    public String getConnectedSinkId() {
        return connectedSinkId;
    }

    public boolean queryConnection() {
        return ((mConnection != null) ? true : false);
    }

    public List<NvwfdAudioFormat> getSupportedAudioFormats() {
        if (mConnection == null) {
            return null;
        }
        List<NvwfdAudioFormat> audiofmts = mConnection.getSupportedAudioFormats();
        return audiofmts;
    }

    public List<NvwfdVideoFormat> getSupportedVideoFormats() {
        if (mConnection == null) {
            return null;
        }
        List<NvwfdVideoFormat> videofmts = mConnection.getSupportedVideoFormats();
        return videofmts;
    }

    public NvwfdAudioFormat getActiveAudioFormat() {
        if (mConnection == null) {
            return null;
        }
        NvwfdAudioFormat audioFormat = mConnection.getActiveAudioFormat();
        return audioFormat;
    }

    public NvwfdVideoFormat getActiveVideoFormat() {
        if (mConnection == null) {
            return null;
        }
        NvwfdVideoFormat videoFormat = mConnection.getActiveVideoFormat();
        return videoFormat;
    }

    public void connectOnAuthenticate(String mAuthentication) {
        if (mConnectedSink != null && mSinks.contains(mConnectedSink)) {
            // Attempt connection
            Log.d(TAG, "Attempting connect to " + mConnectedSink.getSsid());
            mStartConnectionTs = System.currentTimeMillis();
            mNvwfd.connect(mConnectedSink, mAuthentication);
            Toast.makeText(this, "Connecting, please wait...", Toast.LENGTH_LONG).show();
        } else {
            Log.e(TAG, "Sink not available. Discover sinks again.");
        }
    }

    void connectionTime() {
        Log.d(TAG, "QoS:Time for connection is: "+
            (System.currentTimeMillis() -mStartConnectionTs) + " millisecs");
        qosMetric = String.format(
            "Time for connection: %d millisecs",
            (System.currentTimeMillis() -mStartConnectionTs));
        Toast.makeText(this, qosMetric, Toast.LENGTH_SHORT).show();
    }

    void disConnectionTime() {
        Log.d(TAG, "QoS:Time for disconnection is: "+ (System.currentTimeMillis()-mDisconnectionTs) + " millisecs");
        qosMetric = String.format("Time for dis-connection : %d millisecs", (System.currentTimeMillis() -mDisconnectionTs));
        Toast.makeText(this, qosMetric, Toast.LENGTH_SHORT).show();
    }

    public NvwfdPolicyManager configurePolicyManager() {
        if (mNvwfdPolicyManager == null) {
            mNvwfdPolicyManager = new NvwfdPolicyManager();
            mNvwfdPolicyManager.configure(this, mNvwfd, mConnection);
            // Register connection changed listener
            mNvwfdPolicyManager.start(new NvwfdAppListener());
        }
        return mNvwfdPolicyManager;
    }

    private void handleNotification(Message msg) {
        Log.d(TAG, "+Handle NvwfdService Miracat Notification icon");
        CharSequence NotificationTitle = "Miracast";
        CharSequence NotificationContent = "OFF";
        switch (msg.what) {
            case SINKSFOUND:
            {
                NvwfdSinkInfo sink = (NvwfdSinkInfo)msg.obj;
                String text = sink.getSsid();
                if (mNvwfdServiceListenerCallback != null) {
                    mNvwfdServiceListenerCallback.sinksFound(text);
                }
                Log.d(TAG, "handleNotification::Miracast SINKSFOUND");
                NotificationContent = "ReadyToConnectSinks";
                mHandleMessage = false;
                break;
            }
            case CONNECTDONE:
            {
                connectionTime();
                Log.d(TAG, "handleNotification::Miracast CONNECTDONE");
                if(mConnection != null) {
                    configurePolicyManager();
                    if (mNvwfdServiceListenerCallback != null) {
                        mNvwfdServiceListenerCallback.connectDone(true);
                    }
                    NotificationContent = "CONNECTED";
                    connectedSinkId = mConnectedSink.getSsid();
                } else {
                     Log.d(TAG, "RESET SinkButtonList");
                    if (mNvwfdServiceListenerCallback != null) {
                         mNvwfdServiceListenerCallback.connectDone(false);
                    }
                     NotificationContent = "CONNECTION_FAIL!!";
                }
                mHandleMessage = false;
                break;
            }
            case DISCONNECT:
            {
                Log.d(TAG, "handleNotification::Miracast DISCONNECT");
                NotificationContent = "OFF -DiscoverySinksInProgress";
                if (mNvwfdPolicyManager != null) {
                    mNvwfdPolicyManager.stop();

                    Log.d(TAG, "QoS:No of Frames Dropped : "+ (NvwfdPolicyManager.mFramedropCount));
                    String qosMetric = String.format("QoS:nFrames Dropped : %d ",(NvwfdPolicyManager.mFramedropCount));
                    Toast.makeText(this, qosMetric, Toast.LENGTH_SHORT).show();
                    mNvwfdPolicyManager = null;
                }
                if (mConnection != null) {
                    int result = mConnection.disconnect(new NvwfdDisconnectListener());
                    mConnection = null;
                    if (result == -1) {
                        Log.d(TAG, "disconnect fail!!");
                        if (mNvwfdServiceListenerCallback != null) {
                            mNvwfdServiceListenerCallback.disconnect();
                        }
                        disConnectionTime();
                    }
                }
                connectedSinkId = null;
                mHandleMessage = false;
                break;
            }
            case DISCONNECTCOMPLETED:
            {
                disConnectionTime();
                Log.d(TAG, "handleNotification::Miracast DISCONNECTCOMPLETED");
                NotificationContent = "OFF";
                if (mNvwfdServiceListenerCallback != null) {
                    mNvwfdServiceListenerCallback.disconnect();
                }
                mHandleMessage = false;
                break;
            }
            case DISCOVERYFAILED:
            {
                Log.d(TAG, "handleNotification::Miracast DISCOVERYFAILED");
                NotificationContent = "OFF";
                if (mNvwfdPolicyManager != null) {
                    mNvwfdPolicyManager.stop();
                    mNvwfdPolicyManager = null;
                }
                if (mConnection != null) {
                    mConnection.disconnect(new NvwfdDisconnectListener());
                    mConnection = null;
                }
                if (mNvwfdServiceListenerCallback != null) {
                    mNvwfdServiceListenerCallback.notifyError("Sinks Discovery Failed");
                }
                mHandleMessage = false;
                break;
            }
            default:
            {
                Log.d(TAG, "handleNotification::handle default message");
                NotificationContent = "OFF";
                break;
            }
        }
        if (mHandleMessage == false) 
        {
            Notification.Builder builder = new Notification.Builder(mContext);
            if (mIntent != null) {
                PendingIntent pendingIntent = PendingIntent.getActivity(mContext, 0, mIntent, 0);
                builder
                    .setSmallIcon(R.drawable.ic_launcher)
                    .setTicker("MiracastServiceEnabler")
                    .setLights(0xFFFF0000, 10000, 0)
                    .setContentIntent(pendingIntent)
                    .setAutoCancel(false);
                Notification notification = builder.getNotification();
                notification.setLatestEventInfo(mContext, NotificationTitle, NotificationContent, pendingIntent);
                mNotificationManager.notify(R.drawable.ic_launcher, notification);
                mIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP| Intent.FLAG_ACTIVITY_SINGLE_TOP);
                mHandleMessage = true;
            }
        }
        Log.d(TAG, "-Handle NvwfdService - Miracat Touch Notification");
    }
}
