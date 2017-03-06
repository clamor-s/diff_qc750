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

import android.os.Message;

/** Controls the test execution state
 *
 * A class to control the dump render tree test status. The execution status is mutated by the
 * methods of this class.
 */
class DumpRenderTreeController {
    private StringBuilder mErrors;
    private String mTestUrl;
    private boolean mDoneSent;
    private Message mNotifyDoneMsg;
    private LayoutTestController mLayoutTestController;

    public DumpRenderTreeController(Message notifyDoneMsg) {
        mNotifyDoneMsg = notifyDoneMsg;
    }

    public void setIsControlledBy(LayoutTestController layoutTestController) {
        // FIXME: the mutual ref relationship is a bit bad..
        mLayoutTestController = layoutTestController;
    }

    public synchronized void notifyDoneFromLoad() {
        if (mDoneSent)
            return;

        if (!mLayoutTestController.shouldWaitUntilDone())
            sendNotifyDone();
    }

    public synchronized void notifyDoneFromTest() {
        if (mDoneSent)
            return;

        if (!mLayoutTestController.shouldWaitUntilDone()) {
            reportError("Test tried to notifyDone without waitUntilDone.");
            return;
        }

        sendNotifyDone();
    }

    public synchronized void notifyErrorFromLoad(int errorCode, String description, String url) {
        reportError("Load error: " + description + " (code: " + errorCode + ", error url: " + url + ")");
        if (mDoneSent) {
            reportError("Internal error. Error after done has been sent.");
            return;
        }
        sendNotifyDone();
    }

    public synchronized void reportError(String errorMsg) {
        mErrors.append("Error in ")
            .append(mTestUrl)
            .append(":")
            .append(errorMsg)
            .append("\n");
    }

    public synchronized String getErrors() {
        return mErrors.toString();
    }

    public void startTest(String testUrl) {
        mTestUrl = testUrl;
        mDoneSent = false;
        mErrors = new StringBuilder();
    }

    private void sendNotifyDone() {
        mDoneSent = true;
        mNotifyDoneMsg.sendToTarget();
    }
}
