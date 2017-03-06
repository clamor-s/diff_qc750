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
import android.content.Intent;
import android.util.Log;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.UnsupportedEncodingException;
import java.lang.Thread;
import java.net.ServerSocket;
import java.net.Socket;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;


public class RunLayoutTestsCommandReader extends Thread {
    private static final String LOG_TAG = "RunLayoutTestsCommandReader";
    private int mOutPort;
    private int mErrPort;
    private boolean mShouldStop;
    private String mCurrentTestName;
    private String mCurrentTestResultOut;
    private String mCurrentTestResultErr;
    private byte[] mCurrentTestImage;
    private DumpRenderTreeActivity mActivity;

    public RunLayoutTestsCommandReader(DumpRenderTreeActivity activity, int outPort, int errPort) {
        mActivity = activity;
        mOutPort = outPort;
        mErrPort = errPort;
    }

    @Override
    public void run() {
        ServerSocket outServer = null;
        Socket outSocket = null;
        OutputStream out = null;

        ServerSocket errServer = null;
        Socket errSocket = null;
        OutputStreamWriter err = null;

        try {
            outServer = new ServerSocket(mOutPort);
            errServer = new ServerSocket(mErrPort);
            outSocket = outServer.accept();
            errSocket = errServer.accept();
            BufferedReader in = new BufferedReader(new InputStreamReader(outSocket.getInputStream(), "UTF-8"));

            // A raw output stream is needed to output the PNG image.
            // All text is encoded in UTF-8.
            out = outSocket.getOutputStream();
            err = new OutputStreamWriter(errSocket.getOutputStream(), "UTF-8");

            while (true) {
                String command = in.readLine();
                if (command == null) {
                    break;
                }

                String testcase;
                String expectedHash;
                int delimiterIndex = command.indexOf('\'');
                if (delimiterIndex != -1) {
                    expectedHash = command.substring(delimiterIndex + 1);
                    testcase = command.substring(0, delimiterIndex);
                } else {
                    expectedHash = "";
                    testcase = command;
                }

                startOrReuseActivity(testcase);

                waitForTestResults();

                if (shouldStop())
                    break;

                String resultStr = mCurrentTestResultOut + "#EOF\n";
                out.write(resultStr.getBytes("UTF-8"));
                // Output an image if we have one.
                // if we don't have an image, or MD5 algorithm isn't found,
                // there's just a line break outputted between #EOFs.
                if (mCurrentTestImage != null) {
                    try {
                        // Compute the MD5 hash of the image data
                        MessageDigest digester = MessageDigest.getInstance("MD5");
                        byte[] hash = digester.digest(mCurrentTestImage);
                        StringBuffer hexHash = new StringBuffer();
                        for (byte hashByte : hash) {
                            String hex = Integer.toHexString(0xFF & hashByte);
                            if (hex.length() == 1)
                                hexHash.append('0');
                            hexHash.append(hex);
                        }
                        if (expectedHash.equals("") || !hexHash.equals(expectedHash)) {
                            // output the image metadata and the image data
                            String imageHeader =
                                "ActualHash: " + hexHash + "\n" +
                                "ExpectedHash: " + expectedHash + "\n" +
                                "Content-Type: image/png\n" +
                                "Content-Length: " + mCurrentTestImage.length + "\n";
                            out.write(imageHeader.getBytes("UTF-8"));
                            out.write(mCurrentTestImage);
                        }
                    } catch (NoSuchAlgorithmException e) {}
                }
                String eofStr = "#EOF\n";
                out.write(eofStr.getBytes("UTF-8"));

                out.flush();
                err.write(mCurrentTestResultErr);
                err.flush();
            }
        } catch (UnsupportedEncodingException e) {

        } catch (IOException e) {

        } finally {
            try {
                if (out != null)
                    out.flush();
            } catch (IOException e) { }

            try {
                if (outSocket != null)
                    outSocket.close();
            } catch (IOException e) { }

            try {
                if (outServer != null)
                    outServer.close();
            } catch (IOException e) { }

            try {
                if (err != null)
                    err.flush();
            } catch (IOException e) { }

            try {
                if (errSocket != null)
                    errSocket.close();
            } catch (IOException e) { }

            try {
                if (errServer != null)
                    errServer.close();
            } catch (IOException e) { }

        }
        Intent intent = new Intent();
        intent.setClass(mActivity, DumpRenderTreeActivity.class);
        intent.setAction(Intent.ACTION_SHUTDOWN);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mActivity.startActivity(intent);
    }

    private synchronized void startOrReuseActivity(String testcase) {
        mCurrentTestName = testcase;
        mCurrentTestResultOut = null;
        mCurrentTestResultErr = null;
        mCurrentTestImage = null;

        Intent intent = new Intent();
        intent.setClass(mActivity, DumpRenderTreeActivity.class);
        intent.setAction(Intent.ACTION_RUN);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.putExtra(DumpRenderTreeActivity.EXTRA_TEST,
                        testcase);
        mActivity.startActivity(intent);
    }

    public synchronized void testEnd(String testcase, String resultOut, String resultErr, byte[] resultImage) {
        assert testcase == mCurrentTestName;
        mCurrentTestResultOut = resultOut;
        mCurrentTestResultErr = resultErr;
        mCurrentTestImage = resultImage;
        this.notifyAll();
    }

    private boolean shouldStop() {
        return mShouldStop;
    }

    private boolean haveResults() {
        return mCurrentTestResultOut != null;
    }

    private synchronized void waitForTestResults() {
        try {
            if (!shouldStop() && !haveResults())
                this.wait();
        } catch (InterruptedException e) {
            Log.e(LOG_TAG, "WAIT EXCEPTION", e);
        }
    }

    public synchronized void requestShutdown() {
        mShouldStop = true;
        this.notifyAll();
    }
}
