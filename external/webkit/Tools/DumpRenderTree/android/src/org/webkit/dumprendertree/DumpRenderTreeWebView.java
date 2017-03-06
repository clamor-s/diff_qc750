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

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.net.http.SslError;
import android.opengl.GLES20;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.ActionMode;
import android.view.Window;
import android.webkit.ConsoleMessage;
import android.webkit.GeolocationPermissions;
import android.webkit.HttpAuthHandler;
import android.webkit.JsPromptResult;
import android.webkit.JsResult;
import android.webkit.SslErrorHandler;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebSettingsClassic;
import android.webkit.WebStorage.QuotaUpdater;
import android.webkit.WebStorage;
import android.webkit.WebView;
import android.webkit.WebViewClassic;
import android.webkit.WebViewClient;
import java.io.ByteArrayOutputStream;
import java.util.List;
import java.util.Map;
import java.util.HashMap;
import java.net.MalformedURLException;
import java.net.URL;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;

class DumpRenderTreeWebView extends WebView {
    private static final String LOG_TAG = "DumpRenderTreeWebView";
    private static boolean isFirstInit = true;
    private static final int MSG_DOCUMENT_AS_TEXT = 1;
    private static final int MSG_RENDERTREE_AS_TEXT = 2;
    private static final int MSG_TEST_DONE = 3;
    private static final int MSG_TEST_DONE_WAIT = 4;
    private static final int MSG_DO_DOCUMENT_AS_PNG = 5;
    private static final int MSG_BUILD_LAYER = 6;
    private static final int MSG_REQUEST_FOCUS = 7;
    private static final int MSG_DRAWING_STATE = 8;
    private static final int MSG_FIND_STRING = 9;
    private static final int MSG_FIND_NEXT_STRING = 10;
    private static final int MSG_FIND_STOP = 11;

    private static final int TEST_DONE_WAIT_MS = 300;
    private static final int SCREENSHOT_WAIT_MS = 100;

    private DumpRenderTreeController mDumpRenderTreeController;
    private LayoutTestController mLayoutTestController;
    private EventSender mEventSender;
    private boolean mUseHWPixelTests;
    private boolean mDoGLScreenshotOnDraw;

    private Message mFinishMsg;
    private Bundle mResults;
    private String mAppCachePath;
    private boolean mFindDialogCallIsOngoing;
    private ActionMode mFindActionMode;

    private final static int APP_CACHE_MAX_SIZE = 20 * 1024 * 1024;

    private final Handler mAsyncHandler = new Handler() {
       @Override
       public void handleMessage(Message msg) {
           switch (msg.what) {
           case MSG_TEST_DONE:
               testDoneIfWasWaiting();
               break;

           case MSG_TEST_DONE_WAIT:
               testDone();
               break;

           case MSG_DOCUMENT_AS_TEXT:
               setResultText((String)msg.obj);
               break;

           case MSG_RENDERTREE_AS_TEXT:
               setResultText((String)msg.obj);
               break;

           case MSG_DO_DOCUMENT_AS_PNG:
               if (mUseHWPixelTests) {
                   mDoGLScreenshotOnDraw = true;
                   invalidate();
               } else {
                   byte[] compressedImage = viewToPNG();
                   setResultImage(compressedImage);
               }
               break;

           case MSG_FIND_STRING:
               mFindDialogCallIsOngoing = true;
               showFindDialog((String) msg.obj, false);
               mFindDialogCallIsOngoing = false;
               break;

           case MSG_FIND_NEXT_STRING:
               findNext(msg.arg1 == 1);
               break;

           case MSG_FIND_STOP:
               hideFindDialog();

           case MSG_BUILD_LAYER:
               buildLayer();
               break;

           case MSG_REQUEST_FOCUS:
               requestFocus();
               break;

           case MSG_DRAWING_STATE:
               mStateListener.onDrawingStateChanged(msg.arg1 == 1);
               break;
           }
       }
    };

    public interface StateListener {
        public void onDrawingStateChanged(boolean isPaused);
    }
    private StateListener mStateListener;

    public DumpRenderTreeWebView(Context context, Message finishMsg, boolean useHWPixelTests, StateListener stateListener) {
        super(context);
        mFinishMsg = finishMsg;
        mStateListener = stateListener;

        setWebViewClient(mWebViewClient);
        setWebChromeClient(mWebChromeClient);
        mDumpRenderTreeController =
            new DumpRenderTreeController(mAsyncHandler.obtainMessage(MSG_TEST_DONE));

        mLayoutTestController = new LayoutTestController(this, mDumpRenderTreeController);
        mDumpRenderTreeController.setIsControlledBy(mLayoutTestController);

        mEventSender = new EventSender(this);
        mUseHWPixelTests = useHWPixelTests;
        mDoGLScreenshotOnDraw = false;

        addJavascriptInterface(mLayoutTestController.getJSObject(), "layoutTestController");
        addJavascriptInterface(mEventSender.getJSObject(), "eventSender");

        // Disable scrollbars so they don't show up in the HW accelerated screenshot
        setVerticalScrollBarEnabled(false);
        setHorizontalScrollBarEnabled(false);
    }

    static public DumpRenderTreeWebView createWithParent(DumpRenderTreeWebView parent) {
        /* We must use the special constructors, because there's a bug in com.android.webkit.BrowserFrame.
         * Layout tests create empty pages, and for them BrowserFrame.windowObjectCleared is not called. Objects
         * that are added with addJavascriptInterface() will not be added in these cases. */

        HashMap jsObjs = new HashMap<String, Object>();
        jsObjs.put("layoutTestController", parent.mLayoutTestController.getJSObject());
        jsObjs.put("eventSender", parent.mEventSender.getJSObject());
        return new DumpRenderTreeWebView(parent, jsObjs);
    }

    private DumpRenderTreeWebView(DumpRenderTreeWebView parent, Map<String, Object> jsObjs) {
        super(parent.getContext(),
              null, // attribute set
              0, // default style resource ID
              jsObjs,
              false); // is private browsing

        mFinishMsg = null;

        mUseHWPixelTests = parent.mUseHWPixelTests;
        mDoGLScreenshotOnDraw = false;
        if (!mUseHWPixelTests)
            setLayerType(LAYER_TYPE_SOFTWARE, null);

        setWebViewClient(parent.mWebViewClient);
        setWebChromeClient(parent.mWebChromeClient);

        mLayoutTestController = parent.mLayoutTestController;
        mEventSender = parent.mEventSender;
    }

    public void init(String appCachePath, String databasePath) {
        /* When we create the first WebView, we need to pause to wait for the WebView thread to spin
         * and up and for it to register its message handlers. */
        if (isFirstInit) {
            try {
                Thread.currentThread().sleep(1000);
            } catch (Exception e) {}
        }
        mAppCachePath = appCachePath;

        /* Setting a touch interval of -1 effectively disables the optimisation in WebView
         * that stops repeated touch events flooding WebCore. The Event Sender only sends a
         * single event rather than a stream of events (like what would generally happen in
         * a real use of touch events in a WebView)  and so if the WebView drops the event,
         * the test will fail as the test expects one callback for every touch it synthesizes.
         */
        setTouchInterval(-1);

        clearCache(true);

        WebSettings webViewSettings = getSettings();
        webViewSettings.setAppCacheEnabled(true);
        webViewSettings.setJavaScriptEnabled(true);
        webViewSettings.setJavaScriptCanOpenWindowsAutomatically(true);
        webViewSettings.setSupportMultipleWindows(true);
        webViewSettings.setLayoutAlgorithm(WebSettings.LayoutAlgorithm.NORMAL);
        webViewSettings.setDatabaseEnabled(true);
        webViewSettings.setDomStorageEnabled(true);
        webViewSettings.setAllowUniversalAccessFromFileURLs(true);
        if (webViewSettings instanceof WebSettingsClassic) {
            ((WebSettingsClassic)webViewSettings).setWorkersEnabled(false);
            ((WebSettingsClassic)webViewSettings).setXSSAuditorEnabled(false);
            ((WebSettingsClassic)webViewSettings).setPageCacheCapacity(0);
            ((WebSettingsClassic)webViewSettings).setPluginState(WebSettings.PluginState.ON);
            ((WebSettingsClassic)webViewSettings).setWebGLEnabled(true);
        }

        // This is asynchronous, but it gets processed by WebCore before it starts loading pages.
        useMockDeviceOrientation();

        if (isFirstInit) {
            // These can be set only once per process.
            webViewSettings.setDatabasePath(databasePath);
            webViewSettings.setAppCachePath(mAppCachePath);
        }
        // Countrary to path setting above, this must be set for all instances,
        // as the default value will overwrite the previous global value.
        // Use of larger values causes unexplained AppCache database corruption.
        // TODO: Investigate what's really going on here.
        webViewSettings.setAppCacheMaxSize(APP_CACHE_MAX_SIZE);


        // Must do this after setting the AppCache path.
        WebStorage.getInstance().deleteAllData();
        WebViewClassic.fromWebView(this).setUseMockDeviceOrientation();
        WebViewClassic.fromWebView(this).setUseMockGeolocation();

        isFirstInit = false;
    }

    public void initFromParent(DumpRenderTreeWebView parent) {
        setTouchInterval(-1);
        try {
            Thread.currentThread().sleep(1000);
        } catch (Exception e) {}

        WebSettings parentSettings = parent.getSettings();
        WebSettings settings = getSettings();

        settings.setAppCacheEnabled(true);
        settings.setJavaScriptEnabled(parentSettings.getJavaScriptEnabled());
        settings.setJavaScriptCanOpenWindowsAutomatically(parentSettings.getJavaScriptCanOpenWindowsAutomatically());
        settings.setSupportMultipleWindows(parentSettings.supportMultipleWindows());
        settings.setLayoutAlgorithm(parentSettings.getLayoutAlgorithm());
        settings.setDatabaseEnabled(parentSettings.getDatabaseEnabled());
        settings.setDomStorageEnabled(parentSettings.getDomStorageEnabled());
        settings.setAllowUniversalAccessFromFileURLs(parentSettings.getAllowUniversalAccessFromFileURLs());
        if (settings instanceof WebSettingsClassic) {
            ((WebSettingsClassic)settings).setWorkersEnabled(false);
            ((WebSettingsClassic)settings).setXSSAuditorEnabled(mLayoutTestController.isXSSAuditorEnabled());
            ((WebSettingsClassic)settings).setPageCacheCapacity(0);
            ((WebSettingsClassic)settings).setWebGLEnabled(true);
        }
        settings.setAppCacheMaxSize(APP_CACHE_MAX_SIZE);

        useMockDeviceOrientation();
    }

    public void startTest(String testUrl) {
        mDumpRenderTreeController.startTest(testUrl);
        if (testUrl.indexOf("dumpAsText") != -1)
            mLayoutTestController.setShouldDumpAsText();

        loadUrl(testUrl);
    }

    public void postShowFindDialog(String stringToFind) {
        mAsyncHandler.obtainMessage(MSG_FIND_STRING, 0, 0, stringToFind).sendToTarget();
    }

    public void postFindNextString(boolean forward) {
        mAsyncHandler.obtainMessage(MSG_FIND_NEXT_STRING, forward ? 1 : 0, 0).sendToTarget();
    }

    public void postHideFindDialog() {
        mAsyncHandler.sendEmptyMessage(MSG_FIND_STOP);
    }

    public void postRequestFocus() {
        mAsyncHandler.sendEmptyMessage(MSG_REQUEST_FOCUS);
    }

    public void postBuildLayer() {
        mAsyncHandler.sendEmptyMessage(MSG_BUILD_LAYER);
    }

    public void postDrawingStateChange(boolean pauseDrawing) {
        mAsyncHandler.sendMessage(mAsyncHandler.obtainMessage(MSG_DRAWING_STATE, pauseDrawing ? 1 : 0, 0));
    }

    private String getAppCachePath() {
        return mAppCachePath;
    }

    @Override
    public ActionMode startActionMode(ActionMode.Callback callback) {
        // This seems to be the only way to be able to dismiss the find dialog.
        ActionMode mode = super.startActionMode(callback);
        if (mode != null && mFindDialogCallIsOngoing) {
            mFindActionMode = mode;
        }
        return mode;
    }

    private void hideFindDialog() {
        if (mFindActionMode == null)
            return;

        mFindActionMode.finish();
        mFindActionMode = null;
    }

    private void testDoneIfWasWaiting() {
        /* Sometimes test gets marked as done by load before JS onload handler fires or gets run
         * to completion. The reason for this is unknown.  This causes some tests fail, because
         * they mark the dumpAsText in onload handler. Presumably this can affect other results
         * aswell. Wait for some time to let JS onload handler run. */

        if (!mLayoutTestController.shouldWaitUntilDone()) {
            mAsyncHandler.sendEmptyMessageDelayed(MSG_TEST_DONE_WAIT, TEST_DONE_WAIT_MS);
            return;
        }

        testDone();
    }

    private byte[] viewToPNG() {
        int w = getWidth();
        int h = getHeight();
        Bitmap bmp = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bmp);
        canvas.translate(-getScrollX(), -getScrollY());

        // onDraw() is used instead of draw() because draw()
        // does checks and can draw decorations we do not want.
        // capturePicture() can't be used as it only captures the base layer.
        onDraw(canvas);
        ByteArrayOutputStream compressedBytesStream = new ByteArrayOutputStream();
        bmp.compress(Bitmap.CompressFormat.PNG, 100, compressedBytesStream);
        return compressedBytesStream.toByteArray();
    }

    private void testDone() {
        if (mLayoutTestController.shouldDumpAsText()) {
            Message msg = mAsyncHandler.obtainMessage(MSG_DOCUMENT_AS_TEXT);
            /* arg1 - should dump top frame as text
             * arg2 - should dump child frames as text */
            msg.arg1 = 1;
            msg.arg2 = mLayoutTestController.shouldDumpChildFramesAsText() ? 1 : 0;
            documentAsText(msg);
        } else {
            Message msg = mAsyncHandler.obtainMessage(MSG_RENDERTREE_AS_TEXT);
            externalRepresentation(msg);
        }
    }

    private void setResultText(String testResultText) {
        mResults = new Bundle();
        mResults.putString("out", mWebChromeClient.getExtraAPIOutput() + testResultText);
        // Text dump is ready. Now handle the screenshot if necessary.
        if (mLayoutTestController.shouldTestPixels()) {
            // Let JS and painting finish by introducing a slight delay before taking the screenshot
            mAsyncHandler.sendEmptyMessageDelayed(MSG_DO_DOCUMENT_AS_PNG, SCREENSHOT_WAIT_MS);
        } else {
            sendFinishMessage();
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (mDoGLScreenshotOnDraw && canvas.isHardwareAccelerated() && wasLastDrawSuccessful()) {
            mDoGLScreenshotOnDraw = false;

            GLES20 gl = new GLES20();
            IntBuffer glviewport = IntBuffer.allocate(4);
            gl.glGetIntegerv(GLES20.GL_VIEWPORT, glviewport);

            // The DRT window is in the middle of the bottom of the viewport
            int winw = getWidth();
            int winh = getHeight();
            int winx = (glviewport.get(2) - winw) / 2;

            ByteBuffer buf = ByteBuffer.allocateDirect(winw * winh * 4);
            buf.order(ByteOrder.nativeOrder());
            gl.glReadPixels(winx, 0, winw, winh, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, buf);
            int intbufFlipped[] = new int[winw * winh];
            buf.asIntBuffer().get(intbufFlipped);
            int intbuf[] = new int[winw * winh];
            // Flip y
            for (int y = 0; y < winh; ++y) {
                for (int x = 0; x < winw; ++x) {
                    intbuf[y * winw + x] = intbufFlipped[(winh - y - 1) * winw + x];
                }
            }
            // Convert colors
            for (int i = 0; i < winw * winh; ++i) {
                int pixel = intbuf[i];
                int red = pixel & 0x000000ff;
                int green = (pixel & 0x0000ff00) >>> 8;
                int blue = (pixel & 0x00ff0000) >>> 16;
                int alpha = (pixel & 0xff000000) >>> 24;
                intbuf[i] = (alpha << 24) | (red << 16) | (green << 8) | blue;
            }

            Bitmap bmp = Bitmap.createBitmap(intbuf, winw, winh, Bitmap.Config.ARGB_8888);
            ByteArrayOutputStream compressedBytesStream = new ByteArrayOutputStream();
            bmp.compress(Bitmap.CompressFormat.PNG, 100, compressedBytesStream);
            setResultImage(compressedBytesStream.toByteArray());
        }
        else if (mDoGLScreenshotOnDraw)
            invalidate();
    }

    private void setResultImage(byte[] image) {
        mResults.putByteArray("image", image);
        sendFinishMessage();
    }

    private void sendFinishMessage() {
        mResults.putString("err", mDumpRenderTreeController.getErrors());
        mFinishMsg.setData(mResults);
        mFinishMsg.sendToTarget();
        mFinishMsg = null;
        mResults = null;
    }

    private WebViewClient mWebViewClient = new WebViewClient() {
        @Override
        public void onPageFinished(WebView view, String url) {
            if (view != DumpRenderTreeWebView.this)
                return;

            mDumpRenderTreeController.notifyDoneFromLoad();
        }

        @Override
        public void onReceivedHttpAuthRequest(WebView view, HttpAuthHandler handler,
                                              String host, String realm) {
            if (handler.useHttpAuthUsernamePassword() && view != null) {
                String[] credentials = view.getHttpAuthUsernamePassword(host, realm);
                if (credentials != null && credentials.length == 2) {
                    handler.proceed(credentials[0], credentials[1]);
                    return;
                }
            }
            handler.cancel();
        }

        @Override
        public void onReceivedSslError(WebView view, SslErrorHandler handler, SslError error) {
            // We ignore SSL errors. In particular, the certificate used by the LayoutTests server
            // produces an error as it lacks a CN field.
            handler.proceed();
        }

        @Override
        public void onReceivedError(WebView view, int errorCode, String description, String failingUrl) {
            if (view != DumpRenderTreeWebView.this)
                return;

            mDumpRenderTreeController.notifyErrorFromLoad(errorCode, description, failingUrl);
        }
     };

    private DumpRenderTreeWebChromeClient mWebChromeClient = new DumpRenderTreeWebChromeClient();

    class DumpRenderTreeWebChromeClient extends WebChromeClient {
        private StringBuilder quotaOutput;
        private StringBuilder dialogOutput;
        private StringBuilder consoleOutput;

        public DumpRenderTreeWebChromeClient() {
            quotaOutput = new StringBuilder();
            dialogOutput = new StringBuilder();
            consoleOutput = new StringBuilder();
        }

        public String getExtraAPIOutput() {
            String s = dialogOutput.toString() + quotaOutput.toString() + consoleOutput.toString();
            return s;
        }

        @Override
        public void onExceededDatabaseQuota(String urlString, String databaseIdentifier,
                long currentQuota, long estimatedSize, long totalUsedQuota,
                QuotaUpdater quotaUpdater) {
            /** TODO: This should be recorded as part of the text result */
            /** TODO: The quota should also probably be reset somehow for every test? */
            if (mLayoutTestController.shouldDumpDatabaseCallbacks()) {
                String protocol = "";
                String host = "";
                int port = 0;

                try {
                    URL url = new URL(urlString);
                    protocol = url.getProtocol();
                    host = url.getHost();
                    if (url.getPort() > -1) {
                        port = url.getPort();
                    }
                } catch (MalformedURLException e) { }

                quotaOutput.append("UI DELEGATE DATABASE CALLBACK: ");
                quotaOutput.append("exceededDatabaseQuotaForSecurityOrigin:{");
                quotaOutput.append(protocol + ", " + host + ", " + port + "} ");
                quotaOutput.append("database:" + databaseIdentifier + "\n");
            }
            quotaUpdater.updateQuota(5 * 1024 * 1024);
        }

        @Override
        public void onReachedMaxAppCacheSize(long spaceNeeded, long totalUsedQuota, WebStorage.QuotaUpdater quotaUpdater) {
            if (mLayoutTestController.shouldDumpApplicationCacheDelegateCallbacks()) {
                String protocol = "";
                String host = "";
                int port = 0;

                try {
                    URL url = new URL(getUrl());
                    protocol = url.getProtocol();
                    host = url.getHost();
                    if (url.getPort() > -1) {
                        port = url.getPort();
                    }
                } catch (MalformedURLException e) { }

                quotaOutput.append("UI DELEGATE APPLICATION CACHE CALLBACK:");
                quotaOutput.append("exceededApplicationCacheOriginQuotaForSecurityOrigin:{");
                quotaOutput.append(protocol + ", " + host + ", " + port + "} ");
            }
            quotaUpdater.updateQuota(totalUsedQuota);
        }

        @Override
        public boolean onJsAlert(WebView view, String url, String message, JsResult result) {
            dialogOutput.append("ALERT: ");
            dialogOutput.append(message);
            dialogOutput.append('\n');
            result.confirm();
            return true;
        }

        @Override
        public boolean onJsConfirm(WebView view, String url, String message, JsResult result) {
            dialogOutput.append("CONFIRM: ");
            dialogOutput.append(message);
            dialogOutput.append('\n');
            result.confirm();
            return true;
        }

        @Override
        public boolean onJsPrompt(WebView view, String url, String message, String defaultValue,
                JsPromptResult result) {

            dialogOutput.append("PROMPT: ");
            dialogOutput.append(message);
            dialogOutput.append(", default text: ");
            dialogOutput.append(defaultValue);
            result.confirm();
            return true;
        }

        @Override
        public boolean onConsoleMessage(ConsoleMessage consoleMessage) {
            Log.i(LOG_TAG, "CONSOLE MSG: " + consoleMessage.message());
            
            consoleOutput.append("CONSOLE MESSAGE: line " + consoleMessage.lineNumber());
            consoleOutput.append(": " + consoleMessage.message() + "\n");
            return true;
        }

        @Override
        public boolean onCreateWindow(WebView view, boolean dialog, boolean userGesture,
                Message resultMsg) {
            WebView.WebViewTransport transport = (WebView.WebViewTransport)resultMsg.obj;

            /** By default windows cannot be opened, so just send null back. */
            DumpRenderTreeWebView newWindowWebView = null;

            if (mLayoutTestController.shouldOpenWindows()) {
                /**
                 * We never display the new window, just create the view and allow it's content to
                 * execute and be recorded by the executor.
                 */
                newWindowWebView = DumpRenderTreeWebView.createWithParent(DumpRenderTreeWebView.this);
                newWindowWebView.initFromParent(DumpRenderTreeWebView.this);
            }

            transport.setWebView(newWindowWebView);
            resultMsg.sendToTarget();
            return true;
        }

        @Override
        public void onGeolocationPermissionsShowPrompt(String origin,
                GeolocationPermissions.Callback callback) {
            throw new RuntimeException("The WebCore mock used by DRT should bypass the usual permissions flow.");
        }
    };
}

