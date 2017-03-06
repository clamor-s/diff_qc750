/*
 * Copyright (c) 2011, NVIDIA CORPORATION.  All rights reserved.
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
 *
 * Class structure based upon mediaframeworktest CameraTest.java:
 * Copyright (C) 2008 The Android Open Source Project
 */

package com.nvidia.mediaframeworktest.functional;

import com.nvidia.mediaframeworktest.MediaFrameworkTest;

import java.io.*;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.graphics.Rect;
import android.hardware.Camera;
import android.hardware.Camera.Area;
import android.hardware.Camera.CameraInfo;
import android.hardware.Camera.PictureCallback;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.ShutterCallback;
import android.test.ActivityInstrumentationTestCase;
import android.util.Log;
import android.view.SurfaceHolder;

import android.os.Looper;
import android.test.suitebuilder.annotation.LargeTest;

import com.nvidia.NvCamera;
import com.nvidia.NvCamera.NvCameraInfo;

/**
 * Mediaframework test to new NvCamera class
 *
 * Running the test suite:
 *
 * adb shell am instrument \
 *     -e class com.nvidia.mediaframeworktest.functional.NvCameraClassTest \
 *     -w com.nvidia.mediaframeworktest/.MediaFrameworkTestRunner
 *
 */

public class NvCameraClassTest extends ActivityInstrumentationTestCase<MediaFrameworkTest> {
    private String TAG = "NvCameraMfwkTest";

    private boolean rawPictureCallbackResult = false;
    private boolean jpegPictureCallbackResult = false;

    private static int MAX_FRAMES_IN_BURST = 9;
    private static int WAIT_FOR_COMMAND_TO_COMPLETE = 10000;  // Milliseconds.

    private RawPictureCallback mRawPictureCallback      = new RawPictureCallback();
    private JpegPictureCallback mJpegPictureCallback    = new JpegPictureCallback();

    private boolean mInitialized     = false;
    private Looper mLooper           = null;
    private final Object lock        = new Object();
    private final Object previewDone = new Object();

    NvCamera mNvCamera;

    private static final String OUTPUT_FILE_EXT = ".jpg";
    String currentfileName = "";
    String currentfilePath = "/sdcard/NvCameraMfwkTest/";
    // params that all need to get set to something sensible before takePicture.
    // any tests that exercise takePicture should set these appropriately
    boolean burst_performance_test = false;
    int loop_count = 1;
    int nsl_num_buffers = 0;
    int nsl_skip_count = 0;
    int nsl_burst_count = 0;
    int skip_count = 0;
    int burst_count = 3;
    int raw_dump_flag = 0;
    int picture_count = 0;
    int raw_picture_count = 0;
    byte raw_buffer[];
    byte [][] frameArray;
    public int cameraId, backcameraId, frontcameraId, usbcameraId;

    public NvCameraClassTest() {
        super("com.nvidia.mediaframeworktest", MediaFrameworkTest.class);
    }

    protected void setUp() throws Exception {
        super.setUp();
    }

    /*
     * Initializes the message looper so that the Camera object can
     * receive the callback messages.
     */
    private void initializeMessageLooper(final int nvcameraId) {
        Log.v(TAG, "start looper");
        new Thread() {
            @Override
            public void run() {
                // Set up a looper to be used by camera.
                Looper.prepare();
                Log.v(TAG, "start loopRun");
                // Save the looper so that we can terminate this thread
                // after we are done with it.
                mLooper = Looper.myLooper();
                mNvCamera = NvCamera.open(nvcameraId);
                Log.v(TAG, "CameraID: " + Integer.toString(nvcameraId));
                synchronized (lock) {
                    mInitialized = true;
                    lock.notify();
                }
                Looper.loop();  // Blocks forever until Looper.quit() is called.
                Log.v(TAG, "initializeMessageLooper: quit.");
            }
        }.start();
    }

    /*
     * Terminates the message looper thread.
     */
    private void terminateMessageLooper() {
        mLooper.quit();
        //TODO yslau : take out the sleep until bug#1693519 fix
        try {
            Thread.sleep(1000);
        } catch (Exception e){
            Log.v(TAG, e.toString());
        }
        mNvCamera.release();
    }

    //Checking no of cameras connected to device
    public void getCameras() throws Exception {
        int nCameras = Camera.getNumberOfCameras();
        Log.v(TAG, "Total " + nCameras + " cameras");

        CameraInfo backinfo = new CameraInfo();
        for (int i = 0; i < nCameras; i++) {
            Camera.getCameraInfo(i, backinfo);
            if (backinfo.facing == CameraInfo.CAMERA_FACING_BACK) {
                backcameraId = i;
                break;
            }
        }

        CameraInfo frontinfo = new CameraInfo();
        for (int i = 0; i < nCameras; i++) {
            Camera.getCameraInfo(i, frontinfo);
            if (frontinfo.facing == CameraInfo.CAMERA_FACING_FRONT) {
                frontcameraId = i;
                break;
            }
        }

        NvCameraInfo usbinfo = new NvCameraInfo();
        for (int i = 0; i < nCameras; i++) {
            NvCamera.getCameraInfo(i, usbinfo);
            if (usbinfo.facing == NvCameraInfo.CAMERA_FACING_UNKNOWN) {
                usbcameraId = i;
                break;
            }
        }
    }

    //Implement the RawPictureCallback
    private final class RawPictureCallback implements PictureCallback {
        public void onPictureTaken(byte [] rawData, Camera camera) {
            try {
                Log.v(TAG, "Raw Picture callback ++");
                if (rawData != null) {
                    int rawDataLength = rawData.length;
                    File OutFile = new File(currentfilePath);
                    if(!OutFile.exists()) {
                        OutFile.mkdirs();
                    }
                    File rawoutput = new File(currentfilePath + File.separator  + currentfileName + "_" + raw_picture_count + ".nvraw");

                    // since we support burst now, the raw callback could be
                    // called multiple times after a single takePicture request
                    raw_picture_count++;

                    FileOutputStream outstream = new FileOutputStream(rawoutput);
                    outstream.write(rawData);
                    outstream.close();
                    Log.v(TAG, "RawPictureCallback rawDataLength = " + rawDataLength);
                    rawPictureCallbackResult = true;
                } else {
                    rawPictureCallbackResult = false;
                }
                Log.v(TAG, "Raw Picture callback --");
            } catch (Exception e) {
                Log.v(TAG, e.toString());
            }
        }
    };

    //Implement the JpegPictureCallback
    private final class JpegPictureCallback implements PictureCallback {
        public void onPictureTaken(byte [] rawData, Camera camera) {
            try {
                if (rawData != null) {
                    if (burst_performance_test)
                    {
                        frameArray[picture_count] = Arrays.copyOf(rawData, rawData.length);
                    }
                    else
                    {
                        int rawDataLength = rawData.length;
                        File OutFile = new File(currentfilePath);
                        if(!OutFile.exists()) {
                            OutFile.mkdirs();
                        }
                        File rawoutput = new File(currentfilePath + File.separator  + currentfileName + "_" + picture_count + OUTPUT_FILE_EXT);
                        FileOutputStream outstream = new FileOutputStream(rawoutput);
                        outstream.write(rawData);
                        outstream.close();
                        Log.v(TAG, "JpegPictureCallback rawDataLength = " + rawDataLength);
                    }

                    // since we support burst now, the jpeg callback could be
                    // called multiple times after a single takePicture request
                    picture_count++;
                    jpegPictureCallbackResult = true;
                } else {
                    jpegPictureCallbackResult = false;
                }
                Log.v(TAG, "Jpeg Picture callback");
            } catch (Exception e) {
                Log.v(TAG, e.toString());
            }
        }
    };

    // this routine sets the appropriate burst / NSL params, and then exercises
    // the takePicture API call to trigger the capture request that has been set
    // up with those params
    private void checkTakePicture() {
        SurfaceHolder mSurfaceHolder;
        try {
            mSurfaceHolder = MediaFrameworkTest.mSurfaceView.getHolder();
            mNvCamera.setPreviewDisplay(mSurfaceHolder);
            Log.v(TAG, "Start preview");
            mNvCamera.startPreview();

            // handle all of the param settings
            NvCamera.NvParameters p = mNvCamera.getParameters();
            // for NSL, we need to set up the number of buffers, and then query it
            // immediately afterwards.  if the driver cannot allocate all of the
            // buffers that are requested due to memory restrictions, it will return
            // the number of successfully allocated NSL buffers on the next
            // getParameters call
            Log.v(TAG, "Trying to allocate " + nsl_num_buffers + " NSL buffers");
            p.setNSLNumBuffers(nsl_num_buffers);
            mNvCamera.setParameters(p);
            p = mNvCamera.getParameters();
            nsl_num_buffers = p.getNSLNumBuffers();
            Log.v(TAG, nsl_num_buffers + " NSL buffers successfully allocated");

            // set up the rest of the parameters normally.  these will all get
            // processed during takePicture
            p.setNSLSkipCount(nsl_skip_count);
            p.setNSLBurstCount(nsl_burst_count);
            p.setSkipCount(skip_count);
            p.setBurstCount(burst_count);
            p.setRawDumpFlag(raw_dump_flag);

            if (burst_performance_test)
            {
                // set encode quality for optimized burst performance
                p.setJpegQuality(50);
            }

            mNvCamera.setParameters(p);

            // give NSL circular buffer some time to fill, etc.
            Thread.sleep(2000);

            if (raw_dump_flag == 1 || raw_dump_flag == 5)
            {
                // if rawdump is enabled, we need to send the HAL a buffer and
                // register the callback
                raw_buffer = new byte[1024*1024*20];
                mNvCamera.addRawImageCallbackBuffer(raw_buffer);
                mNvCamera.takePicture(null, mRawPictureCallback, mJpegPictureCallback);
            }
            else
            {
                // TODO: call autoFocus for normal captures,  don't call AF for NSL
                // if it's a standard jpeg request, we only need the jpeg callback
                mNvCamera.takePicture(null, null, mJpegPictureCallback);
            }
            Thread.sleep(5000);
        } catch (Exception e) {
            Log.v(TAG, e.toString());
        }
    }

    // Test case for normal burst captures
    public void testNvCameraBurst() throws Exception {
        synchronized (lock) {
            getCameras();
            // only test the back camera for now
            initializeMessageLooper(backcameraId);
            try {
                lock.wait(WAIT_FOR_COMMAND_TO_COMPLETE);
            } catch(Exception e) {
                Log.v(TAG, "runTestOnMethod: wait was interrupted.");
            }
        }

        Thread.sleep(1000);

        // zero out all the NSL stuff, we only want to test standard burst
        nsl_num_buffers = 0;
        nsl_skip_count = 0;
        nsl_burst_count = 0;

        // set up a few standard burst captures in a loop,
        // burst 1, burst 2, burst 3, ...
        // these will all get applied in checkTakePicture
        loop_count = 5;
        for (int i = 1; i < loop_count; i++){
            picture_count = 1; // reinit this, used by jpeg callback
            raw_picture_count = 1; // reinit this, used by raw callback

            burst_count = i;  // main burst param

            // we could also set this, if we wanted to do a burst capture with
            // skipped frames between each capture frame
            skip_count = 0;

            currentfileName = "IMG_cameraId" + cameraId + "_NvCameraBurst" + i;
            checkTakePicture();
            Thread.sleep(500);
        }
        terminateMessageLooper();
    }

    // Test case for normal burst captures
    public void testNvCameraBurstPerformance() throws Exception {
        synchronized (lock) {
            getCameras();
            // only test the back camera for now
            initializeMessageLooper(backcameraId);
            try {
                lock.wait(WAIT_FOR_COMMAND_TO_COMPLETE);
            } catch(Exception e) {
                Log.v(TAG, "runTestOnMethod: wait was interrupted.");
            }
        }

        Thread.sleep(1000);

        frameArray = new byte[MAX_FRAMES_IN_BURST][];
        burst_performance_test = true;
        skip_count = 0;
        burst_count = MAX_FRAMES_IN_BURST;

        // zero out all the NSL stuff, we only want to test standard burst
        nsl_num_buffers = 0;
        nsl_skip_count = 0;
        nsl_burst_count = 0;

        picture_count = 0;
        checkTakePicture();
        Thread.sleep(500);

        try {
            for(int i = 0; i < MAX_FRAMES_IN_BURST; i++) {
                if (frameArray[i] != null) {
                    int rawDataLength = frameArray[i].length;
                    currentfileName = "IMG_cameraId" + cameraId + "_NvCameraBurst_" + i;
                    File OutFile = new File(currentfilePath);
                    if(!OutFile.exists()) {
                        OutFile.mkdirs();
                    }
                    File rawoutput = new File(currentfilePath + File.separator  + currentfileName + OUTPUT_FILE_EXT);
                    FileOutputStream outstream = new FileOutputStream(rawoutput);
                    outstream.write(frameArray[i]);
                    outstream.close();
                    Log.v(TAG, "JpegPictureCallback rawDataLength = " + rawDataLength);
                }
            }
        } catch (Exception e) {
            Log.v(TAG, e.toString());
        }

        terminateMessageLooper();
    }

    // Test case for NSL
    public void testNvCameraNSL() throws Exception {
        synchronized (lock) {
            getCameras();
            // only test the back camera for now
            initializeMessageLooper(backcameraId);
            try {
                lock.wait(WAIT_FOR_COMMAND_TO_COMPLETE);
            } catch(Exception e) {
                Log.v(TAG, "runTestOnMethod: wait was interrupted.");
            }
        }

        Thread.sleep(1000);
        // test NSL API
        // zero out normal burst stuff, we only want to test NSL now
        burst_count = 0;
        skip_count = 0;
        loop_count = 3;
        for (int i = 1; i < loop_count; i++){
            picture_count = 1; // reinit this, used by jpeg callback
            raw_picture_count = 1; // reinit this, used by raw callback

            nsl_burst_count = i; // main NSL burst param
            // NSL needs at least as many buffers as the capture will ask for,
            // or else the capture will fail
            nsl_num_buffers = 4;
            // we could also set this, if we wanted to do a burst capture
            // with skipped frames between each capture frame
            nsl_skip_count = 0;

            currentfileName = "IMG_cameraId" + cameraId + "_NvCameraNSL" + i;
            checkTakePicture();

            // checkTakePicture might find out that we couldn't allocate all of our
            // buffers, and it will overwrite the nsl_num_buffers var with it.
            // sanity check to make sure we don't loop too far
            if (nsl_num_buffers + 1 < loop_count)
                loop_count = nsl_num_buffers + 1;

            Thread.sleep(500);
        }
        terminateMessageLooper();
    }

    public void testNvCameraNSLAndBurst() throws Exception {
        synchronized (lock) {
            getCameras();
            // only test the back camera for now
            initializeMessageLooper(backcameraId);
            try {
                lock.wait(WAIT_FOR_COMMAND_TO_COMPLETE);
            } catch(Exception e) {
                Log.v(TAG, "runTestOnMethod: wait was interrupted.");
            }
        }

        Thread.sleep(1000);
        // test NSL and burst APIs together, see testNvCameraNSL and testNvCameraBurst
        // for more detailed explanations of how to use them for each case
        loop_count = 3;
        nsl_num_buffers = 4;
        nsl_skip_count = 0;
        burst_count = 3; // just hardcode this, already tested in testNvCameraBurst
        skip_count = 0;
        // this test assumes that at least one buffer will be successfully
        // allocated.
        for (int i = 1; i < loop_count; i++){
            picture_count = 1;
            raw_picture_count = 1;

            nsl_burst_count = i;

            currentfileName = "IMG_cameraId" + cameraId + "_NvCameraNSLAndBurst" + i;
            checkTakePicture();
            // sanity check to make sure we don't loop farther than the number
            // of successfully allocated bufers
            if (nsl_num_buffers + 1 < loop_count)
                loop_count = nsl_num_buffers + 1;
            Thread.sleep(500);
        }
        terminateMessageLooper();
    }

    public void testNvCameraBracketedBurst() throws Exception {
        synchronized (lock) {
            getCameras();
            // only test the back camera for now
            initializeMessageLooper(backcameraId);
            try {
                lock.wait(WAIT_FOR_COMMAND_TO_COMPLETE);
            } catch(Exception e) {
                Log.v(TAG, "runTestOnMethod: wait was interrupted.");
            }
        }

        Thread.sleep(1000);
        // set up for bracketed burst capture
        NvCamera.NvParameters p = mNvCamera.getParameters();
        float [] exposureValues;

        exposureValues = new float[3];

        exposureValues[0] = -2.0f;
        exposureValues[1] =  0.0f;
        exposureValues[2] =  2.0f;

        p.setEvBracketCapture(exposureValues);
        mNvCamera.setParameters(p);

        // now set all the other params that are shared with the other tests
        loop_count = 0;
        nsl_num_buffers = 0;
        nsl_skip_count = 0;
        burst_count = 3;
        skip_count = 0;
        picture_count = 1;
        raw_picture_count = 1;
        nsl_burst_count = 0;

        currentfileName = "IMG_cameraId" + cameraId + "_NvCameraBracketedBurst" + 0;

        checkTakePicture();

        // un-set bracketed stuff so it doesn't interfere with other tests
        p.setEvBracketCapture(null);
        mNvCamera.setParameters(p);

        terminateMessageLooper();
    }

    public void testNvCameraAEWindows() throws Exception {
        List<Area> meteringAreas;
        SurfaceHolder mSurfaceHolder;
        synchronized (lock) {
            getCameras();
            // only test the back camera for now
            initializeMessageLooper(backcameraId);
            try {
                lock.wait(WAIT_FOR_COMMAND_TO_COMPLETE);
            } catch(Exception e) {
                Log.v(TAG, "runTestOnMethod: wait was interrupted.");
            }
        }

        mSurfaceHolder = MediaFrameworkTest.mSurfaceView.getHolder();
        mNvCamera.setPreviewDisplay(mSurfaceHolder);
        Log.v(TAG, "Start preview");
        mNvCamera.startPreview();
        Thread.sleep(1000);

        // test AE windows API, spec'd in android API documentation online
        NvCamera.NvParameters p = mNvCamera.getParameters();

        Rect orig = new Rect (0, 0, 0, 0);
        Rect upperLeft = new Rect(-1000,-1000, -800, -800);
        Rect upperRight = new Rect (800, -1000, 1000, -800);
        Rect lowerLeft = new Rect (-1000, 800, -800, 1000);
        Rect lowerRight = new Rect (800, 800, 1000, 1000);

        meteringAreas = new ArrayList<Area>();

        // test case 1: single window in upper left corner of screen
        if (p.getMaxNumMeteringAreas() > 0)
        {
           meteringAreas.add(new Area(upperLeft, 1));
           p.setMeteringAreas(meteringAreas);
           mNvCamera.setParameters(p);
           // sleep for 10 seconds to give test runner enough time to look at
           // all regions
           Thread.sleep(10000);
        }

        // test case 2:
        // one window in upper left corner of screen
        // one window in upper right corner
        // equal weights
        if (p.getMaxNumMeteringAreas() > 1)
        {
           meteringAreas.add(new Area(upperRight, 1));
           p.setMeteringAreas(meteringAreas);
           mNvCamera.setParameters(p);
           // sleep for 10 seconds to give test runner enough time to look at
           // all regions
           Thread.sleep(10000);
        }

        // test case 3:
        // one window in upper left corner of screen
        // one window in upper right corner
        // one window in lower left corner
        // equal weights
        if (p.getMaxNumMeteringAreas() > 2)
        {
           meteringAreas.add(new Area(lowerLeft, 1));
           p.setMeteringAreas(meteringAreas);
           mNvCamera.setParameters(p);
           // sleep for 10 seconds to give test runner enough time to look at
           // all regions
           Thread.sleep(10000);
        }

        // test case 4:
        // one window in upper left corner of screen
        // one window in upper right corner
        // one window in lower left corner
        // one window in lower right corner
        // equal weights
        if (p.getMaxNumMeteringAreas() > 3)
        {
           meteringAreas.add(new Area(lowerRight, 1));
           p.setMeteringAreas(meteringAreas);
           mNvCamera.setParameters(p);
           // sleep for 10 seconds to give test runner enough time to look at
           // all regions
           Thread.sleep(10000);
        }

        // test case 5:
        // if there is more than one window set by above test cases,
        // make weight of top right huge, so that the other windows
        // will be negligible
        if (p.getMaxNumMeteringAreas() > 1)
        {
           meteringAreas.set(1, new Area(upperRight, 1000));
           p.setMeteringAreas(meteringAreas);
           mNvCamera.setParameters(p);
           // sleep for 10 seconds to give test runner enough time to look at
           // all regions
           Thread.sleep(10000);
        }

        // test case 6:
        // restore default window, all others should be ignored when the 0th
        // window is set to this
        if (p.getMaxNumMeteringAreas() > 0)
        {
           meteringAreas.clear();
           meteringAreas.add(new Area(orig, 0));
           p.setMeteringAreas(meteringAreas);
           mNvCamera.setParameters(p);
           // sleep for 10 seconds to give test runner enough time to look at
           // all regions
           Thread.sleep(10000);
        }

        Thread.sleep(2000);

        synchronized (previewDone) {
            try {
                previewDone.wait(WAIT_FOR_COMMAND_TO_COMPLETE);
                Log.v(TAG, "Preview Done");
            } catch (Exception e) {
                Log.v(TAG, "wait was interrupted.");
            }
        }
        mNvCamera.stopPreview();


        terminateMessageLooper();
    }

    public void testAELock() throws Exception {
        SurfaceHolder mSurfaceHolder;
        synchronized (lock) {
            getCameras();
            // only test the back camera for now
            initializeMessageLooper(backcameraId);
            try {
                lock.wait(WAIT_FOR_COMMAND_TO_COMPLETE);
            } catch(Exception e) {
                Log.v(TAG, "runTestOnMethod: wait was interrupted.");
            }
        }

        mSurfaceHolder = MediaFrameworkTest.mSurfaceView.getHolder();
        mNvCamera.setPreviewDisplay(mSurfaceHolder);
        Log.v(TAG, "Start preview");
        mNvCamera.startPreview();
        Thread.sleep(1000);

        NvCamera.NvParameters p = mNvCamera.getParameters();

        if (p.isAutoExposureLockSupported()) {
            Log.v(TAG, "testing AE Lock...");
            Log.v(TAG, "point the camera at a bright light");
            Thread.sleep(7000);

            // test basic lock functionality
            Log.v(TAG, "locking AE");
            p.setAutoExposureLock(true);
            mNvCamera.setParameters(p);
            p = mNvCamera.getParameters();
            assertTrue(p.getAutoExposureLock() == true);

            Log.v(TAG, "AE locked!  move the camera around, exposure shouldn't change");
            Thread.sleep(7000);

            // make sure that exposure compensation still does things
            Log.v(TAG, "sweeping exposure compensation from -2, 0, +2, this should work");
            p.setExposureCompensation(-20);
            mNvCamera.setParameters(p);
            p = mNvCamera.getParameters();
            assertTrue(p.getAutoExposureLock() == true);
            Log.v(TAG, "exposure compensation is -2");
            Thread.sleep(4000);

            p.setExposureCompensation(0);
            mNvCamera.setParameters(p);
            p = mNvCamera.getParameters();
            assertTrue(p.getAutoExposureLock() == true);
            Log.v(TAG, "exposure compensation is 0");
            Thread.sleep(4000);

            p.setExposureCompensation(20);
            mNvCamera.setParameters(p);
            p = mNvCamera.getParameters();
            assertTrue(p.getAutoExposureLock() == true);
            Log.v(TAG, "exposure compensation is 2");
            Thread.sleep(4000);

            Log.v(TAG, "resetting exp. comp. to 0, AE should still be locked");
            p.setExposureCompensation(0);
            mNvCamera.setParameters(p);
            p = mNvCamera.getParameters();
            assertTrue(p.getAutoExposureLock() == true);
            Thread.sleep(4000);

            // make sure that it is still locked after takePicture
            Log.v(TAG, "calling takePicture, AE should still be locked after this...");
            // reset all the other params that are shared with the other tests
            loop_count = 0;
            nsl_num_buffers = 0;
            nsl_skip_count = 0;
            burst_count = 1;
            skip_count = 0;
            picture_count = 1;
            raw_picture_count = 1;
            nsl_burst_count = 0;
            currentfileName = "IMG_cameraId" + cameraId + "_NvCameraAELock" + 0;

            checkTakePicture();
            mNvCamera.startPreview();
            Thread.sleep(1000);
            Log.v(TAG, "done with takePicture, make sure AE is still locked");
            p = mNvCamera.getParameters();
            assertTrue(p.getAutoExposureLock() == true);
            Thread.sleep(7000);

            // finally, test out the unlock
            Log.v(TAG, "unlocking AE...");
            p.setAutoExposureLock(false);
            mNvCamera.setParameters(p);
            p = mNvCamera.getParameters();
            assertTrue(p.getAutoExposureLock() == false);
            Log.v(TAG, "AE unlocked, behavior should be back to auto");
            Thread.sleep(7000);
        }
        else {
            Log.v(TAG, "AE lock not supported!");
        }

        Thread.sleep(2000);

        synchronized (previewDone) {
            try {
                previewDone.wait(WAIT_FOR_COMMAND_TO_COMPLETE);
                Log.v(TAG, "Preview Done");
            } catch (Exception e) {
                Log.v(TAG, "wait was interrupted.");
            }
        }
        mNvCamera.stopPreview();


        terminateMessageLooper();
    }

    public void testAWBLock() throws Exception {
        SurfaceHolder mSurfaceHolder;
        synchronized (lock) {
            getCameras();
            // only test the back camera for now
            initializeMessageLooper(backcameraId);
            try {
                lock.wait(WAIT_FOR_COMMAND_TO_COMPLETE);
            } catch(Exception e) {
                Log.v(TAG, "runTestOnMethod: wait was interrupted.");
            }
        }

        mSurfaceHolder = MediaFrameworkTest.mSurfaceView.getHolder();
        mNvCamera.setPreviewDisplay(mSurfaceHolder);
        Log.v(TAG, "Start preview");
        mNvCamera.startPreview();
        Thread.sleep(1000);

        NvCamera.NvParameters p = mNvCamera.getParameters();

        if (p.isAutoWhiteBalanceLockSupported()) {
            Log.v(TAG, "testing AWB Lock...");
            Log.v(TAG, "point the camera at a solid color");
            Thread.sleep(7000);

            // test basic lock functionality
            Log.v(TAG, "locking AWB");
            p.setAutoWhiteBalanceLock(true);
            mNvCamera.setParameters(p);
            p = mNvCamera.getParameters();
            assertTrue(p.getAutoWhiteBalanceLock() == true);
            Log.v(TAG, "AWB locked!  point the camera at something white, it should be tinted");
            Thread.sleep(7000);

            // make sure that it is still locked after takePicture
            Log.v(TAG, "calling takePicture, AWB should still be locked after this...");
            // reset all the other params that are shared with the other tests
            loop_count = 0;
            nsl_num_buffers = 0;
            nsl_skip_count = 0;
            burst_count = 1;
            skip_count = 0;
            picture_count = 1;
            raw_picture_count = 1;
            nsl_burst_count = 0;
            currentfileName = "IMG_cameraId" + cameraId + "_NvCameraAWBLock" + 0;

            checkTakePicture();
            mNvCamera.startPreview();
            Thread.sleep(1000);
            Log.v(TAG, "done with takePicture, make sure AWB is still locked");
            p = mNvCamera.getParameters();
            assertTrue(p.getAutoWhiteBalanceLock() == true);
            Thread.sleep(7000);

            // test out the unlock
            Log.v(TAG, "unlocking AWB...");
            p.setAutoWhiteBalanceLock(false);
            mNvCamera.setParameters(p);
            Log.v(TAG, "AWB unlocked, behavior should be back to auto");
            p = mNvCamera.getParameters();
            assertTrue(p.getAutoWhiteBalanceLock() == false);
            Thread.sleep(7000);

            // relock and make sure changing WB setting some other way releases it
            Log.v(TAG, "locking AWB again...");
            Log.v(TAG, "point the camera at a solid color");
            Thread.sleep(7000);
            p.setAutoWhiteBalanceLock(true);
            mNvCamera.setParameters(p);
            Log.v(TAG, "AWB locked!  point the camera at something white, it should be tinted");
            p = mNvCamera.getParameters();
            assertTrue(p.getAutoWhiteBalanceLock() == true);
            Thread.sleep(7000);

            Log.v(TAG, "going to change a WB setting now, should unlock AWB lock");
            p.setWhiteBalance("fluorescent");
            mNvCamera.setParameters(p);
            p = mNvCamera.getParameters();
            assertTrue(p.getAutoWhiteBalanceLock() == false);
            Thread.sleep(7000);

            Log.v(TAG, "resetting WB to auto");
            p.setWhiteBalance("auto");
            mNvCamera.setParameters(p);
        }
        else {
            Log.v(TAG, "AWB lock not supported!");
        }

        Thread.sleep(2000);

        synchronized (previewDone) {
            try {
                previewDone.wait(WAIT_FOR_COMMAND_TO_COMPLETE);
                Log.v(TAG, "Preview Done");
            } catch (Exception e) {
                Log.v(TAG, "wait was interrupted.");
            }
        }
        mNvCamera.stopPreview();

        terminateMessageLooper();
    }
}
