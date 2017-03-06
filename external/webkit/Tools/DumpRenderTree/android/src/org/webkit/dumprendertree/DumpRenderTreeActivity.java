/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (c) 2011, NVIDIA CORPORATION. All rights reserved.
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

package org.webkit.dumprendertree;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.PowerManager.WakeLock;
import android.os.PowerManager;
import android.os.RemoteException;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.LinearLayout;

import org.webkit.dumprendertree.R;

import java.lang.IllegalArgumentException;
import java.util.ArrayList;
import java.util.List;

/** Activity to run a single WebKit layout test.
 *
 * This activity executes a single WebKit layout test. It returns the
 * textual result to DumpRenderTreeService.
 *
 * The activity runs in a separate process and sends the results of
 * running the test to DumpRenderTreeService.The process separation is
 * done to be able to handle tests that crash the view.
 */
public class DumpRenderTreeActivity extends Activity {
    private static final String LOG_TAG = "DumpRenderTreeActivity";

    public static String EXTRA_TEST = "test";

    private static final int MSG_TEST_FINISHED = 1;

    private DumpRenderTreeWebView mCurrentView = null;

    private Messenger mServiceMessenger = null;
    private WakeLock mScreenDimLock;
    private String mCurrentTestUrl;
    private RunLayoutTestsCommandReader mCommandReader = null;
    private List<AdbPortForwarder> mPortForwarders = null;
    private boolean mUseHWPixelTests = true;

    private ServiceConnection mServiceConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            mServiceMessenger = new Messenger(service);
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            mServiceMessenger = null;
        }
    };

    private final Handler mResultHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MSG_TEST_FINISHED:
                Bundle bundle = msg.getData();
                testFinishedWithResults(bundle.getString("out"), bundle.getString("err"), bundle.getByteArray("image"));
                break;
            }
        }
    };
    private final DumpRenderTreeWebView.StateListener mStateListener
        = new DumpRenderTreeWebView.StateListener() {
            public void onDrawingStateChanged(boolean isPaused) {
                LinearLayout layout = (LinearLayout) findViewById(R.id.mainLayout);
                if (isPaused) {
                    if (mCurrentView.getParent() != null)
                        layout.removeView(mCurrentView);
                } else {
                    if (mCurrentView.getParent() == null)
                        layout.addView(mCurrentView, new ViewGroup.LayoutParams(800, 600));
                }
            }
    };


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);

        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        mScreenDimLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK | PowerManager.ON_AFTER_RELEASE,
                                        "WakeLock in DumpRenderTreeActivity");
        mScreenDimLock.acquire();

        setContentView(R.layout.main);

        configureForwarding();
    }

    @Override
    protected void onNewIntent(Intent intent) {
        if (intent == null)
            return;

        String action = intent.getAction();
        if (action.equals(Intent.ACTION_REBOOT) || action.equals(Intent.ACTION_SHUTDOWN)) {
            setResult(RESULT_CANCELED);
            finish();
        } else if (action.equals(Intent.ACTION_RUN) || action.equals(Intent.ACTION_VIEW)) {
            setIntent(intent);
        }
    }

    private void setStateFromIntent() {
        if (mCommandReader != null)
            return;

        Intent intent = getIntent();

        String shouldWaitForDebugger = intent.getStringExtra("wait");
        if (shouldWaitForDebugger != null) {
            try {
                Thread.currentThread().sleep(3000);
            } catch (InterruptedException e) { }
        }

        int outPort = intent.getIntExtra("outPort", 0);
        int errPort = intent.getIntExtra("errPort", 0);
        if (outPort == 0  && errPort == 0)
            return;

        if (outPort == 0)
            throw new IllegalArgumentException("Argument outPort missing");
        if (errPort == 0)
            throw new IllegalArgumentException("Argument errPort missing");

        mCommandReader = new RunLayoutTestsCommandReader(this, outPort, errPort);
        mCommandReader.start();

        mUseHWPixelTests = intent.getBooleanExtra("useHWPixelTests", true);
    }

    private void resetWebView() {
        DumpRenderTreeWebView oldView = mCurrentView;

        DumpRenderTreeWebView view = new DumpRenderTreeWebView(this, mResultHandler.obtainMessage(MSG_TEST_FINISHED), mUseHWPixelTests, mStateListener);
        // Make the on-screen DRT view also use SW compositing in the case of SW compositing testing
        if (!mUseHWPixelTests)
            view.setLayerType(View.LAYER_TYPE_SOFTWARE, null);

        view.init(getApplicationContext().getCacheDir().getPath(), getDir("databases", 0).getAbsolutePath());

        mCurrentView = view;

        LinearLayout layout = (LinearLayout) findViewById(R.id.mainLayout);

        if (oldView != null && oldView.getParent() != null)
            layout.removeView(oldView);

        layout.addView(view, new ViewGroup.LayoutParams(800, 600));

        if (oldView != null)
            oldView.destroy();
    }

    @Override
    protected void onResume() {
        super.onResume();
        setStateFromIntent();
        resetWebView();
        startTestFromIntent();
    }

    @Override
    protected void onDestroy() {
        for (AdbPortForwarder fwd : mPortForwarders)
            fwd.requestShutdown();

        mScreenDimLock.release();
        super.onDestroy();
    }

    private void startTestFromIntent() {
        String url = getIntent().getStringExtra(EXTRA_TEST);
        if (url == null)
            return;

        if (!(url.startsWith("http://") || url.startsWith("https://")))
            url = "file://" + url;

        mCurrentTestUrl = url;
        setTitle(mCurrentTestUrl);
        mCurrentView.startTest(mCurrentTestUrl);
    }

    private void testFinishedWithResults(String out, String err, byte[] image) {
        if (mCommandReader != null)
            mCommandReader.testEnd(mCurrentTestUrl, out, err, image);
        else {
            Log.i(LOG_TAG, "DumpRenderTree out for test " + mCurrentTestUrl);
            Log.i(LOG_TAG, out);
            if (err != null && !err.isEmpty()) {
                Log.i(LOG_TAG, "DumpRenderTree err for test " + mCurrentTestUrl);
                Log.i(LOG_TAG, err);
            }
        }
    }

    private void configureForwarding() {
        if (mPortForwarders != null)
            return;

        mPortForwarders = new ArrayList<AdbPortForwarder>();

        int[] ports = { 8000, 8080, 8443 };
        for (int i = 0; i < ports.length; ++i) {
            AdbPortForwarder fwd = new AdbPortForwarder(ports[i]);
            fwd.start();
            mPortForwarders.add(fwd);
        }
    }
}


