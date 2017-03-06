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

import android.net.Uri;
import android.view.View;
import android.webkit.WebStorage;
import android.webkit.WebView;
import android.webkit.WebViewClassic;
import android.webkit.WebSettingsClassic;
import android.webkit.JavascriptInterface;
import java.io.File;

/** Implementation of JavaScript layout test controller object.
 *
 * A class that is registered as the window.layoutTestController in JavaScript environment. The
 * tests use this to communicate with DumpRenderTree.
 */
public class LayoutTestController {
    private final String WEBKIT_OFFLINE_WEB_APPLICATION_CACHE_ENABLED = "WebKitOfflineWebApplicationCacheEnabled";
    private final String WEBKIT_USES_PAGE_CACHE_PREFERENCE_KEY = "WebKitUsesPageCachePreferenceKey";

    private boolean mDumpAsText;
    private boolean mEnablePixelTest = true;
    private boolean mDumpChildFramesAsText;
    private boolean mDumpDatabaseCallbacks;
    private boolean mCanOpenWindows;
    private boolean mWaitUntilDone;
    private boolean mNotifyDoneFromTest;
    private boolean mIsXSSAuditorEnabled;
    private boolean mDumpApplicationCacheDelegateCallbacks;

    private DumpRenderTreeWebView mWebView;
    private DumpRenderTreeController mDumpRenderTreeController;

    public LayoutTestController(DumpRenderTreeWebView webView, DumpRenderTreeController dumpRenderTreeController) {
        mWebView = webView;
        mDumpRenderTreeController = dumpRenderTreeController;
    }

    public Object getJSObject() {
        return mJSObject;
    }

    synchronized public boolean shouldWaitUntilDone() {
        return mWaitUntilDone;
    }

    synchronized public boolean shouldDumpDatabaseCallbacks() {
        return mDumpDatabaseCallbacks;
    }

    synchronized public boolean shouldDumpApplicationCacheDelegateCallbacks() {
        return mDumpApplicationCacheDelegateCallbacks;
    }

    synchronized public boolean shouldOpenWindows() {
        return mCanOpenWindows;
    }

    synchronized public boolean shouldDumpChildFramesAsText() {
        return mDumpChildFramesAsText;
    }

    synchronized public boolean shouldDumpAsText() {
        return mDumpAsText;
    }

    synchronized public boolean isXSSAuditorEnabled() {
        return mIsXSSAuditorEnabled;
    }

    synchronized public boolean shouldTestPixels() {
        return mEnablePixelTest;
    }

    synchronized public void setShouldDumpAsText()
    {
        mJSObject.dumpAsText(false);
    }

    /** Instance that changes the LayoutTestController state based on JS calls.
     *
     * The methods here mutate the corresponding variables in
     * LayoutTestController.
     */
    class LayoutTestControllerJSObject {
        @JavascriptInterface
        public void clearAllDatabases() {
            WebStorage.getInstance().deleteAllData();
        }

        @JavascriptInterface
        public void dumpAsText() {
            dumpAsText(/*enablePixelTest*/ false);
        }

        @JavascriptInterface
        public void dumpAsText(boolean enablePixelTest) {
            synchronized (LayoutTestController.this) {
                mDumpAsText = true;
                mEnablePixelTest = enablePixelTest;
            }
        }

        @JavascriptInterface
        public String layerTreeAsText() {
            return mWebView.getLayerTreeAsText();
        }

        @JavascriptInterface
        public void dumpChildFramesAsText() {
            synchronized (LayoutTestController.this) {
                mDumpChildFramesAsText = true;
            }
        }

        @JavascriptInterface
        public void dumpDatabaseCallbacks() {
            synchronized (LayoutTestController.this) {
                mDumpDatabaseCallbacks = true;
            }
        }

        @JavascriptInterface
        public void dumpApplicationCacheDelegateCallbacks() {
            synchronized (LayoutTestController.this) {
                mDumpApplicationCacheDelegateCallbacks = true;
            }
        }

        @JavascriptInterface
        public void notifyDone() {
            synchronized (LayoutTestController.this) {
                // Check if test called waitUntilDone(). If not, do nothing.
                mNotifyDoneFromTest = true;
                mDumpRenderTreeController.notifyDoneFromTest();
            }
        }

        @JavascriptInterface
        public void overridePreference(String key, boolean value) {
            if (WEBKIT_OFFLINE_WEB_APPLICATION_CACHE_ENABLED.equals(key)) {
                mWebView.getSettings().setAppCacheEnabled(value);
            }
            else if (WEBKIT_USES_PAGE_CACHE_PREFERENCE_KEY.equals(key)) {
                if (mWebView.getSettings() instanceof WebSettingsClassic)
                    ((WebSettingsClassic)mWebView.getSettings()).setPageCacheCapacity(Integer.MAX_VALUE);
            }
        }

        @JavascriptInterface
        public void setAppCacheMaximumSize(long size) {
            mWebView.getSettings().setAppCacheMaxSize(size);
        }

        @JavascriptInterface
        public void setCanOpenWindows() {
            synchronized (LayoutTestController.this) {
                mCanOpenWindows = true;
            }
        }

        @JavascriptInterface
        public void setDatabaseQuota(long quota) {
            /** TODO: Reset this before every test! */
            WebStorage.getInstance().setQuotaForOrigin(mWebView.getUrl(), quota);
        }

        @JavascriptInterface
        public void setGeolocationPermission(boolean allow) {
            WebViewClassic.fromWebView(mWebView).setMockGeolocationPermission(allow);
        }

        @JavascriptInterface
        public void setMockDeviceOrientation(boolean canProvideAlpha, double alpha,
                                             boolean canProvideBeta, double beta, boolean canProvideGamma, double gamma) {
            // Configuration is in WebKit, so stay on WebCore thread
            mWebView.setMockDeviceOrientation(canProvideAlpha, alpha,
                                              canProvideBeta, beta,
                                              canProvideGamma, gamma);
        }

        @JavascriptInterface
        public void setMockGeolocationError(int code, String message) {
            WebViewClassic.fromWebView(mWebView).setMockGeolocationError(code, message);
        }

        @JavascriptInterface
        public void setMockGeolocationPosition(double latitude, double longitude, double accuracy) {
            WebViewClassic.fromWebView(mWebView).setMockGeolocationPosition(latitude, longitude, accuracy);
        }

        @JavascriptInterface
        public void setXSSAuditorEnabled(boolean flag) {
            synchronized(this) {
                if (mWebView.getSettings() instanceof WebSettingsClassic) {
                    ((WebSettingsClassic)mWebView.getSettings()).setXSSAuditorEnabled(flag);
                    mIsXSSAuditorEnabled = flag;
                }
            }
        }

        @JavascriptInterface
        public void waitUntilDone() {
            synchronized(LayoutTestController.this) {
                if (mWaitUntilDone) {
                    mDumpRenderTreeController.reportError("Test set waitUntilDone multiple times.");
                    return;
                }

                if (mNotifyDoneFromTest) {
                    mDumpRenderTreeController.reportError("Test tried to waitUntilDone after notifyDone.");
                    return;
                }

                mWaitUntilDone = true;
            }
        }

        @JavascriptInterface
        public void showFindDialog(String stringToFind) {
            mWebView.postShowFindDialog(stringToFind);
        }

        @JavascriptInterface
        public void findNext(boolean forward) {
            mWebView.postFindNextString(forward);
        }

        @JavascriptInterface
        public void hideFindDialog() {
            mWebView.postHideFindDialog();
        }

        @JavascriptInterface
        public String pathToLocalResource(String fileName)  {
            String fullPath = "/sdcard/webkit" + fileName;
            File file = (new File(fullPath)).getParentFile();
            file.mkdirs();
            return fullPath;
        }

        @JavascriptInterface
        public void setWindowIsKey(boolean flag) {
            // TODO: this expected to be synchronous.
            // TODO: implement unfocusing of the view
            if (flag) {
                mWebView.postRequestFocus();
            }
        }

        @JavascriptInterface
        public void display() {
            // TODO: this expected to be synchronous.
            mWebView.postBuildLayer();
        }

        @JavascriptInterface
        public void displayInvalidatedRegion() {
            // TODO: this actually might display the whole view, the semantics aren't part of the
            // API.
            display();
        }

        @JavascriptInterface
        public void setAlwaysAcceptCookies(boolean alwaysAccept) {
            if (alwaysAccept) {
                mDumpRenderTreeController.reportError("Always accepting cookies not implemented.");
            }
        }

        @JavascriptInterface
        public void pauseDrawing() {
            mWebView.postDrawingStateChange(true);
        }

        @JavascriptInterface
        public void resumeDrawing() {
            mWebView.postDrawingStateChange(false);
        }
    };

    private LayoutTestControllerJSObject mJSObject = new LayoutTestControllerJSObject();
}
