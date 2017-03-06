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

import java.util.ArrayList;
import java.util.List;

import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.text.Editable;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;
import android.content.Context;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.Binder;
import android.content.ServiceConnection;
import android.content.ComponentName;
import android.widget.LinearLayout.LayoutParams;
import android.graphics.Color;
import android.widget.SeekBar;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.CheckBox;
import android.view.View.OnClickListener;
import android.view.MenuItem;
import android.view.MenuInflater;
import android.view.Menu;

import com.nvidia.nvwfd.*;

final public class NvwfdServiceActivity extends Activity
                       implements OnClickListener {
    private static final String LOGTAG = "NvwfdServiceTest_Activity";
    private static final int SINKSFOUND = 1;
    private static final int CONNECTDONE = 2;
    private static final int DISCONNECT = 3;
    private static final int RESETSINKS = 4;
    private static final int RESTORECONNECTION = 5;
    private static final int HANDLEERROR = 6;
    private ScrollView mScrollView;
    private LinearLayout mLayout;
    private Button mGetFormatsButton;
    private Button mActiveFormatButton;
    private RadioButton m480pButton;
    private RadioButton m720pButton;
    private RadioButton m1080pButton;
    private Button mDisconnectButton;
    private Button sinkClicked;
    private TextView mSinkFormatsText;
    private TextView mActiveFormatText;
    private List<Button> mSinkButtonList;
    private NvwfdServiceMonitor mServiceMonitor =null;
    private Context mWfdContext;
    private RadioGroup mRadioGroup;
    private CheckBox mForceResolution;
    private SeekBar mPolicySeekBar;
    private boolean bForceResolution = false;
    private static final int CONNECTION_480P = 0;
    private static final int CONNECTION_720P = 1;
    private static final int CONNECTION_1080P = 2;
    private int mResolutionCheckId = CONNECTION_720P;
    private boolean isDisconncetPressed = true;
    private static long mConnectionTs;
    private static long mDisconnectionTs;
    private static long mDiscoveryTs;

    //binding with service implementation
    private boolean mIsBound;
    private NvwfdService mNvwfdService = null;
    private ServiceConnection mServiceConnection = new ServiceConnection() {

        // On connection established with service, this gets called.
        public void onServiceConnected(ComponentName className, IBinder binder) {
            Log.d(LOGTAG, "onServiceConnected+");
            // Get service object to interact with the service.
            mNvwfdService = ((NvwfdService.NvwfdServiceBinder)binder).getService();
            Log.d(LOGTAG, "mNvwfdService" + mNvwfdService);
            mServiceMonitor =(NvwfdServiceMonitor)binder;
            mServiceMonitor.registerCallback(listener);
            doStartActivity();
            Log.d(LOGTAG, "onServiceConnected-");
        }
        // When service is disconnected this gets called.
        public void onServiceDisconnected(ComponentName className) {
            mServiceMonitor = null;
            mNvwfdService = null;
            Log.d(LOGTAG, "onServiceDisconnected");
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mWfdContext = getApplicationContext();
        mScrollView = new ScrollView(this);
        mLayout = new LinearLayout(this);
        mSinkButtonList = new ArrayList<Button>();

        mLayout.setOrientation(LinearLayout.VERTICAL);
        mScrollView.addView(mLayout);
        setContentView(mScrollView);
        Log.d(LOGTAG, "Create Nvwfd and bind service");
        doBindService();
    }

    void doBindService() {
        // Attach connection with the service.
        Log.d(LOGTAG, "doBindService+");
        Intent intent = new Intent(this, NvwfdService.class);
        Log.d(LOGTAG, "mServiceConnection " + mServiceConnection);
        mWfdContext.bindService(intent, mServiceConnection, Context.BIND_AUTO_CREATE);
        mIsBound = true;
        Log.d(LOGTAG, "doBindService-");
    }

    void doUnbindService() {
        if (mIsBound) {
            Log.d(LOGTAG, "doUnbindService");
            //Detach existing connection with the service.
            if (mNvwfdService != null && (mNvwfdService.queryConnection() == false)) {
                Log.d(LOGTAG, "Sink is not connected- hence unbind service");
                mServiceMonitor.registerCallback(null);
                mWfdContext.unbindService(mServiceConnection);
                mIsBound = false;
            }
        }
    }

    private NvwfdServiceListener listener=new NvwfdServiceListener() {
        public void sinksFound(String text) {
            Log.d(LOGTAG, "Sinks found message arrived text+" + text);
            Message msg = new Message();
            msg.what = SINKSFOUND;
            msg.obj = (Object)text;
            msgHandler.sendMessage(msg);
        }
        public void connectDone(boolean value) {
            Log.d(LOGTAG, "Connect done message arrived");
            if(value == true) {
                Log.d(LOGTAG, "Connection succeeded");
                msgHandler.sendEmptyMessage(CONNECTDONE);
            } else {
                Log.d(LOGTAG, "Connection failed!!!");
                msgHandler.sendEmptyMessage(RESETSINKS);
            }
        }
        public void disconnect() {
            Log.d(LOGTAG, "Disconnect call back arrived");
            msgHandler.sendEmptyMessage(DISCONNECT);
        }
        public void notifyError(String text) {
            Log.d(LOGTAG, "notifyError arrived text+" + text);
            Message msg = new Message();
            msg.what = HANDLEERROR;
            msg.obj = (Object)text;
            msgHandler.sendMessage(msg);
        }
    };

    private Handler msgHandler = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            switch(msg.what)
            {
                case SINKSFOUND:
                {
                    Log.d(LOGTAG, "handleMessage - Miracast SINKSFOUND+");
                    String text = (String)msg.obj;
                    String connectedSinkId = mNvwfdService.getConnectedSinkId();
                    Log.d(LOGTAG, "QoS: Sink found : " + text +" After :"+
                        (System.currentTimeMillis() -mDiscoveryTs) + " millisecs");
                    if (!(connectedSinkId !=null && connectedSinkId.equalsIgnoreCase(text))) {
                        Button b = new Button(NvwfdServiceActivity.this);
                        if (connectedSinkId !=null) {
                            b.setLayoutParams(new LinearLayout.LayoutParams(300, LayoutParams.WRAP_CONTENT));
                        }
                        b.setText(text);
                        b.setOnClickListener(mOnConnectClicked);
                        mLayout.addView(b);
                        mSinkButtonList.add(b);
                    }
                    break;
                }
                case CONNECTDONE:
                {
                    Log.d(LOGTAG, "QoS:Time for connection is: "+
                        (System.currentTimeMillis() -mConnectionTs) + " millisecs");
                    finish();
                    break;
                }
                case RESTORECONNECTION:
                {
                    viewAfterConnect();
                    break;
                }
                case RESETSINKS:
                {
                    Log.d(LOGTAG, "Connect fail!! Reset SinkButtonList and call doStartActivity");
                    doStartActivity();
                    break;
                }
                case DISCONNECT:
                {
                    Log.d(LOGTAG, "QoS:Time for disconnection is: "+
                        (System.currentTimeMillis()-mDisconnectionTs) + " millisecs");
                    Log.d(LOGTAG, "Disconnect: Reset SinkButtonList and call doStartActivity");
                    // Clear sinks and start over
                    Log.d(LOGTAG, "isDisconncetPressed"+isDisconncetPressed);
                    if (isDisconncetPressed) {
                        doStartActivity();
                    }
                    else
                    {
                        Log.d(LOGTAG, "********Connecting********");
                        // Get Authetication mode and PIN if needed
                        NvwfdServiceActivity.this.autheticationModeDialog();
                    }
                    break;
                }
                case HANDLEERROR:
                {
                    Log.d(LOGTAG, "Finish the activity to handle the error");
                    String text = (String )msg.obj;
                    Toast.makeText(mWfdContext, text, Toast.LENGTH_SHORT).show();
                    finish();
                    break;
                }
            }
        }
    };

    @Override
    public void onDestroy() {
        Log.d(LOGTAG, "Unbind and destroy");
        doUnbindService();
        super.onDestroy();
    }

    private void doStartActivity() {
        mLayout.removeAllViews();
        Log.d(LOGTAG, "doStartActivity Called");
        boolean mWifiInit = false;
        if (mNvwfdService != null) {
            mWifiInit = mNvwfdService.isNvwfdInit();
        }
        Log.d(LOGTAG, "doStartActivity called mWifiInit =" + mWifiInit);

        if(mWifiInit == true) {
            WifiManager wifi = (WifiManager)getSystemService(Context.WIFI_SERVICE);
            if(wifi.isWifiEnabled() == false) {
                fatalErrorDialog("Wifi not enabled, please use Android Settings to enable.");
            } else if (mNvwfdService.queryConnection()) {
                Log.d(LOGTAG, "Sink already connected- hence display connection window");

                List<NvwfdSinkInfo> mskinks;
                mskinks = mNvwfdService.getConnectedSinkList();
                mSinkButtonList.clear();
                for(final NvwfdSinkInfo sink : mskinks) {
                    Button b = new Button(NvwfdServiceActivity.this);
                    b.setText(sink.getSsid());
                    b.setOnClickListener(mOnConnectClicked);
                    mSinkButtonList.add(b);
                }

                String text = mNvwfdService.getConnectedSinkId();
                Log.d(LOGTAG, "SinkConnectedText: " + text);
                for(final Button SinkButton : mSinkButtonList) {
                    if (text == SinkButton.getText()) {
                        Log.d(LOGTAG, "SinkClicked Found");
                        sinkClicked = SinkButton;
                    }
                }
                Log.d(LOGTAG, "SendEmptyMessage CONNECT to display");
                msgHandler.sendEmptyMessage(RESTORECONNECTION);
            } else {
                for(final Button b : mSinkButtonList) {
                    mLayout.removeView(b);
                }
                mSinkButtonList.clear();
                Log.d(LOGTAG, "DiscoverSinks");
                if (mNvwfdService != null) {
                    mDiscoveryTs = System.currentTimeMillis();
                    mNvwfdService.discoverSinks();
                } else {
                    Log.d(LOGTAG, "NULL mNvwfdService");
                }
            }
        } else {
            fatalErrorDialog("WFD Protocol init failed!");
        }
    }

    private View.OnClickListener mOnConnectClicked = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            for (final Button SinkButton : mSinkButtonList) {
                if (v == SinkButton) {
                    sinkClicked = SinkButton;
                    // Select Sink
                    int index = mSinkButtonList.indexOf(SinkButton);
                    mNvwfdService.mapConnectedSinkIndex(index);
                    if (mNvwfdService.getConnectedSinkId()!=null) {
                        isDisconncetPressed = false;
                        mDisconnectionTs = System.currentTimeMillis();
                        mNvwfdService.onDisconnect();
                     } else {
                        Log.d(LOGTAG, "Connecting..........");
                        // Get Authetication mode and PIN if needed
                        NvwfdServiceActivity.this.autheticationModeDialog();
                    }
                }
            }
        }
    };

    private void viewAfterConnect() {
        Log.d(LOGTAG, "+viewAfterConnectfunction");

        if (mNvwfdService.queryConnection()) {

            final NvwfdPolicyManager mNvwfdPolicyManager =
                mNvwfdService.configurePolicyManager();

            mLayout.removeAllViews();
            mScrollView.removeView(mLayout);

            LinearLayout ll1 = new LinearLayout(this);
            ll1.setOrientation(LinearLayout.HORIZONTAL);

            LinearLayout ll2 = new LinearLayout(this);
            ll2.setOrientation(LinearLayout.HORIZONTAL);

            LinearLayout ll3 = new LinearLayout(this);
            ll3.setOrientation(LinearLayout.VERTICAL);

            sinkClicked.setBackgroundColor(Color.CYAN);
            sinkClicked.setEnabled(false);
            mSinkButtonList.remove(mSinkButtonList.indexOf(sinkClicked));
            ll2.addView(sinkClicked);
            ll2.setLayoutParams(new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT,1.0f));
            ll1.addView(ll2);

            // Disconnect
            mDisconnectButton = new Button(this);
            mDisconnectButton.setText("Disconnect");
            mDisconnectButton.setOnClickListener(mOnDisconnectClicked);
            ll3.addView(mDisconnectButton);

            // Get connection formats
            mGetFormatsButton = new Button(this);
            mGetFormatsButton.setText("Get connection formats");
            mGetFormatsButton.setOnClickListener(mOnGetFormatsClicked);
            ll3.addView(mGetFormatsButton);

            // Get Active connection format
            mActiveFormatButton = new Button(this);
            mActiveFormatButton.setText("Get active connection format");
            mActiveFormatButton.setOnClickListener(mOnGetActiveFormatClicked);
            ll3.addView(mActiveFormatButton);

            // 480p connection
            mRadioGroup = new RadioGroup(this);
            mRadioGroup.setOnClickListener(this);
            m480pButton = new RadioButton(this);
            m480pButton.setText("480p");
            m480pButton.setId(CONNECTION_480P);
            m480pButton.setOnClickListener(new RadioButton.OnClickListener() {
                public void onClick(View v) {
                    if (mNvwfdPolicyManager != null &&
                    true == mNvwfdPolicyManager.renegotiateResolution(CONNECTION_480P)) {
                        Log.d(LOGTAG, "480p connection succeeded");
                        mRadioGroup.check(CONNECTION_480P);
                        mResolutionCheckId = CONNECTION_480P;
                        mNvwfdService.mResolutionCheckId = CONNECTION_480P;
                    } else {
                        Log.d(LOGTAG, "480p resolution is not supported");
                        mRadioGroup.check(mResolutionCheckId);
                    }
                }
            });
            mRadioGroup.addView(m480pButton);
            // 720p connection
            m720pButton = new RadioButton(this);
            m720pButton.setText("720p");
            m720pButton.setId(CONNECTION_720P);
            m720pButton.setOnClickListener(new RadioButton.OnClickListener() {
                public void onClick(View v) {
                    if (mNvwfdPolicyManager != null &&
                    true == mNvwfdPolicyManager.renegotiateResolution(CONNECTION_720P)) {
                        Log.d(LOGTAG, "720p connection succeeded");
                        mRadioGroup.check(CONNECTION_720P);
                        mResolutionCheckId = CONNECTION_720P;
                        mNvwfdService.mResolutionCheckId = CONNECTION_720P;
                    } else {
                        Log.d(LOGTAG, "720p resolution is not supported");
                        mRadioGroup.check(mResolutionCheckId);
                    }
                }
            });
            mRadioGroup.addView(m720pButton);
            // 1080p connection
            m1080pButton = new RadioButton(this);
            m1080pButton.setText("1080p");
            m1080pButton.setId(CONNECTION_1080P);
            m1080pButton.setOnClickListener(new RadioButton.OnClickListener() {
                public void onClick(View v) {
                    if (mNvwfdPolicyManager != null &&
                    true == mNvwfdPolicyManager.renegotiateResolution(CONNECTION_1080P)) {
                        Log.d(LOGTAG, "1080p connection succeeded");
                        mRadioGroup.check(CONNECTION_1080P);
                        mResolutionCheckId = CONNECTION_1080P;
                        mNvwfdService.mResolutionCheckId = CONNECTION_1080P;
                    } else {
                        Log.d(LOGTAG, "1080p resolution is not supported");
                        mRadioGroup.check(mResolutionCheckId);
                    }
                }
            });
            mRadioGroup.addView(m1080pButton);
            mRadioGroup.check(mNvwfdService.mResolutionCheckId);
            ll3.addView(mRadioGroup);

            mForceResolution = new CheckBox(this);
            mForceResolution.setText("Force Resolution");
            mForceResolution.setChecked(mNvwfdService.bForceResolution);
            mForceResolution.setOnClickListener(new CheckBox.OnClickListener() {
                public void onClick(View v) {
                    if (bForceResolution) {
                        mForceResolution.setChecked(false);
                        if (mNvwfdPolicyManager != null) {
                            mNvwfdPolicyManager.forceResolution(false);
                        }
                        bForceResolution = false;
                        mNvwfdService.bForceResolution = false;
                    } else {
                        mForceResolution.setChecked(true);
                        if (mNvwfdPolicyManager != null) {
                            mNvwfdPolicyManager.forceResolution(true);
                            mNvwfdPolicyManager.renegotiateResolution(mNvwfdService.mResolutionCheckId);
                        }
                        bForceResolution = true;
                        mNvwfdService.bForceResolution = true;
                    }
                }
            });
            ll3.addView(mForceResolution);
            ll1.addView(ll3);

            LinearLayout ll4 = new LinearLayout(this);
            ll4.setOrientation(LinearLayout.HORIZONTAL);
            TextView tv_quality = new TextView(this);
            tv_quality.setText("Quality");
            tv_quality.setTextColor(Color.CYAN);
            tv_quality.setLayoutParams(new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
            TextView tv_latency = new TextView(this);
            tv_latency.setLayoutParams(new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
            tv_latency.setText("Latency");
            tv_latency.setTextColor(Color.CYAN);
            // Policy seek bar
            mPolicySeekBar = new SeekBar(this);
            mPolicySeekBar.setMax(100);
            mPolicySeekBar.setLayoutParams(new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT,1.0f));
            mPolicySeekBar.setProgress(mNvwfdService.mPolicySeekBarValue);
            mPolicySeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                @Override
                public void onStopTrackingTouch(SeekBar arg0) {
                }

                @Override
                public void onStartTrackingTouch(SeekBar arg0) {
                }

                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                    if (mNvwfdPolicyManager != null) {
                        mNvwfdPolicyManager.updatePolicy(progress);
                        mNvwfdService.mPolicySeekBarValue = progress;
                    }
                }
            });
            ll4.addView(tv_quality);
            ll4.addView(mPolicySeekBar);
            ll4.addView(tv_latency);
            ll1.setLayoutParams(
                new LinearLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT,1.0f));
            mLayout.addView(ll1,0);
            mLayout.addView(ll4);
            for (final Button SinkButton : mSinkButtonList) {
                Log.d(LOGTAG, "SinkButton" + SinkButton.getText());
                if(sinkClicked != SinkButton) {
                    SinkButton.setLayoutParams(new LinearLayout.LayoutParams(300, LayoutParams.WRAP_CONTENT));
                    mLayout.addView(SinkButton);
                }
            }
            mScrollView.addView(mLayout);
        }
        Log.d(LOGTAG, "-viewAfterConnectfunction");
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.refresh, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle item selection
        switch (item.getItemId()) {
            case R.id.refresh:
                for(final Button b : mSinkButtonList) {
                    mLayout.removeView(b);
                }
                mSinkButtonList.clear();
                if (mNvwfdService != null) {
                    // qos discover time start
                    mDiscoveryTs = System.currentTimeMillis();
                    mNvwfdService.discoverSinks();
                } else {
                    Log.d(LOGTAG, "NULL mNvwfdService");
                }
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    private View.OnClickListener mOnGetFormatsClicked = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if (mActiveFormatText != null) {
                mLayout.removeView(mActiveFormatText);
            }
            if (mSinkFormatsText != null) {
                mLayout.removeView(mSinkFormatsText);
            }
            mSinkFormatsText = new TextView(NvwfdServiceActivity.this);
            List<NvwfdAudioFormat> audiofmts = mNvwfdService.getSupportedAudioFormats();
            List<NvwfdVideoFormat> videofmts = mNvwfdService.getSupportedVideoFormats();

            for (final NvwfdVideoFormat videofmt : videofmts) {
                mSinkFormatsText.setText(mSinkFormatsText.getText() + " | " + videofmt.toString());
            }
            mSinkFormatsText.setText(mSinkFormatsText.getText() + "\n VideoFormatCount =" + videofmts.size() + "\n");
            for (final NvwfdAudioFormat audiofmt : audiofmts) {
                mSinkFormatsText.setText(mSinkFormatsText.getText() + " | " + audiofmt.toString());
            }
            mSinkFormatsText.setText(mSinkFormatsText.getText() + "\n AudioFormatCount =" + audiofmts.size() + "\n");
            mLayout.addView(mSinkFormatsText);
        }
    };

    private View.OnClickListener mOnGetActiveFormatClicked = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if (mSinkFormatsText != null) {
                mLayout.removeView(mSinkFormatsText);
            }
            if (mActiveFormatText != null) {
                mLayout.removeView(mActiveFormatText);
            }
            mActiveFormatText = new TextView(NvwfdServiceActivity.this);
            NvwfdAudioFormat audiofmt = mNvwfdService.getActiveAudioFormat();
            NvwfdVideoFormat videofmt = mNvwfdService.getActiveVideoFormat();

            mActiveFormatText.setText(mActiveFormatText.getText() + " | " + videofmt.toString());
            mActiveFormatText.setText(mActiveFormatText.getText() + " | " + audiofmt.toString());
            mLayout.addView(mActiveFormatText);
        }
    };

    private View.OnClickListener mOnDisconnectClicked = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            isDisconncetPressed = true;
            mDisconnectionTs = System.currentTimeMillis();
            mNvwfdService.onDisconnect();
        }
    };

    private void fatalErrorDialog(String message) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setMessage(message);
        builder.setCancelable(false);
        builder.setPositiveButton("Exit", new DialogInterface.OnClickListener() {
           public void onClick(DialogInterface dialog, int id) {
                NvwfdServiceActivity.this.finish();
           }});
        AlertDialog alert = builder.create();
        alert.show();
    }

    private void autheticationModeDialog() {
        final CharSequence[] items = {"OK", "CANCEL", "PIN"};

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Select Authentication Mode");
        builder.setItems(items, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int item) {
                Log.d(LOGTAG, "Mode selected is " + item);
                mConnectionTs = System.currentTimeMillis();
                // Connect in PUSH buttom mode
                if(item == 0) {
                    mNvwfdService.connectOnAuthenticate(null);
                } else if(item == 1) {// Canel entry dialog
                    finish();
                } else if(item == 2) {// Connect in PIN buttom mode
                    mNvwfdService.connectOnAuthenticate("true");
                }
                dialog.cancel();
            }
        });

        AlertDialog alert = builder.create();
        alert.show();
    }

    public void onClick(View arg0) {
        try {
            return;
        } catch (Exception e) {
        }
    }
}

