/*
 * Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
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
 * Class structure based upon Camera in Camera.java:
 * Copyright (C) 2009 The Android Open Source Project
 */

/**
 * @file
 * <b>NVIDIA Tegra Android Camera API Extensions</b>
 *
 * @b Description: Exposes additional functionality from the Android Camera
 *    Hardware Abstraction Layer (Camera HAL) to the Java camera API.

 */

/**
 * @defgroup nvcamera_ext_group NVIDIA Android Camera API Extensions
 *
 * To enhance current camera parameters with additional functionality,
 * NVIDIA has exposed API extensions from the HAL to the Java layer
 * You can use the additional extensions that are available inside
 * the Camera HAL to configure NVIDIA's extended functionality.
 *
 * To facilitate setting these parameters, NVIDIA has extended the Android
 * <a href="http://developer.android.com/reference/android/hardware/Camera.html"
 * target="_blank">android.hardware.Camera</a>
 * class in the @c com.nvidia.NvCamera class, and
 * it has extended the <a href=
 * "http://developer.android.com/reference/android/hardware/Camera.Parameters.html"
 * target="_blank">android.hardware.Camera.Parameters</a> class in the
 * @c com.nvidia.NvCamera.NvParameters class. These classes contain the full
 * standard Android API, as they are extensions of the base classes. Because
 * of this, it should be relatively easy to incorporate these classes into
 * existing applications. The extended APIs that these classes implement on
 * top of the stock Android API are described in this section.
 *
 * Unless stated otherwise, invalid inputs are caught at the HAL layer
 * in the same manner as standard camera settings. Upon seeing an invalid
 * input, the Camera HAL will return an error. When the CameraService receives
 * an error, it will throw an exception.
 *
 * All of these settings persist as long as the camera object exists.
 * Therefore, an application must set them again for each new camera object
 * that it creates. It should be noted that Android's camera application
 * creates a new camera object each time it switches between front and back
 * cameras.
 * @{
 */
package com.nvidia;

import android.util.Log;
import java.util.ArrayList;
import java.util.List;
import java.util.StringTokenizer;

import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;

public class NvCamera extends Camera{
    private static final String TAG = "NvCamera";

/** @name Extended Camera Information
 *
 * Use this information to access camera information unavailable in standard
 * Android API. Information comes in from the HAL.
 */
/*@{*/
    /**
     * Gets the extended information about a particular camera.
     * If @c getNumberOfCameras() returns N, the valid ID is 0 to N-1.
     */
    public native static void getNvCameraInfo(int cameraId, NvCameraInfo cameraInfo);

    public static class NvCameraInfo extends CameraInfo {
        /**
         * Holds the attribute indicating the facing of the camera. This
         * is random as the camera can move.
         *
         * Attribute facing in base class defines only front and back.
         * This constant is for cameras that can be moved around.
         * Hence, possible values for attribute facing are
         * \c CAMERA_FACING_BACK, \c CAMERA_FACING_FRONT, or
         * \c CAMERA_FACING_UNKNOWN.
         */
        public static final int CAMERA_FACING_UNKNOWN = 2;

        // Stereo capabilites codes match the enum in include/camera/Camera.h
        /**
         * Indicates undefined stereoscopic capabilities.
         */
        public static final int CAMERA_STEREO_CAPS_UNDEFINED = 0;

        /**
         * Indicates the camera is monoscopic.
         */
        public static final int CAMERA_STEREO_CAPS_MONO = 1;

        /**
         * Indicates the camera is stereoscopic.
         */
        public static final int CAMERA_STEREO_CAPS_STEREO = 2;

        /**
         * Indicates whether the camera can be used in stereo mode. It should be
         * NvCameraInfo.CAMERA_STEREO_CAPS_UNDEFINED,
         * NvCameraInfo.CAMERA_STEREO_CAPS_MONO,
         * or NvCameraInfo.CAMERA_STEREO_CAPS_STEREO.
         *
         * Read-only field.
         */
        public int stereoCaps;

        // Connection type codes match the enum in include/camera/Camera.h
        /**
         * Indicates an undefined connection type.
         */
        public static final int CAMERA_CONNECTION_UNDEFINED = 0;

        /**
         * Indicates the camera is internal (built into the device).
         */
        public static final int CAMERA_CONNECTION_INTERNAL = 1;

        /**
         * Indicates the camera is connected via USB.
         */
        public static final int CAMERA_CONNECTION_USB = 2;

        /**
         * Indicates whether the camera is connected via USB or is built-in.
         *
         * NvCameraInfo.CAMERA_CONNECTION_INTERNAL indicates a built-in camera,
         * while NvCameraInfo.CAMERA_CONNECTION_USB indicates a camera connected
         * via USB. It should be NvCameraInfo.CAMERA_CONNECTION_UNDEFINED,
         * NvCameraInfo.CAMERA_CONNECTION_INTERNAL, or
         * NvCameraInfo.CAMERA_CONNECTION_USB.
         *
         * Read-only field.
         */
        public int connection;
    };
/*@}*/

    public NvWindow newNvWindow() {
        return new NvWindow();
    }

    public static class NvWindow {
        public NvWindow() {
            left = 0;
            top = 0;
            right = 0;
            bottom = 0;
            weight = 0;
        }

        public NvWindow(int l, int t, int r, int b, int w) {
            left = l;
            top = t;
            right = r;
            bottom = b;
            weight = w;
        }

        public int left;
        public int top;
        public int right;
        public int bottom;
        public int weight;
    }

    public static class NvVideoPreviewFps {

        public NvVideoPreviewFps() {
            VideoWidth = 0;
            VideoHeight = 0;
            MaxPreviewWidth = 0;
            MaxPreviewHeight = 0;
            MaxFps = 0;
        }

        @Override
        public boolean equals(Object obj) {
            if (!(obj instanceof NvVideoPreviewFps)) {
                return false;
            }
            NvVideoPreviewFps vpf = (NvVideoPreviewFps) obj;
            return (VideoWidth == vpf.VideoWidth &&
                    VideoHeight == vpf.VideoHeight &&
                    MaxPreviewWidth == vpf.MaxPreviewWidth &&
                    MaxPreviewHeight == vpf.MaxPreviewHeight &&
                    MaxFps == vpf.MaxFps);
        }

        /**
         * Holds the supported video width.
         */
        public int VideoWidth;
        /**
         * Holds the supported video height.
         */
        public int VideoHeight;
        /**
         * Holds the maximum preview width for supported video resolution.
         */
        public int MaxPreviewWidth;
        /**
         * Holds the maximum preview height for supported video resolution.
         */
        public int MaxPreviewHeight;
        /**
         * Holds the maximum capture frame rate for supported video resolution.
         */
        public int MaxFps;
    }

    private native final void native_setCustomParameters(String params);
    private native final String native_getCustomParameters();

/** @name General Functions
 *
 * Use these functions in the NvCamera class to transfer the
 * @c NvCamera.NvParameters instance between the application and the HAL, which
 * is similar to the set/getParameters interface that exists in the standard
 * Android API.
 *
 * NVIDIA's extended classes follow the general camera parameters API model in
 * the Android API. For more information about this model, see the Android API
 * documentation at:
 *
 * <a href=
 * "http://developer.android.com/reference/android/hardware/Camera.Parameters.html"
 * target="_blank">http://developer.android.com/reference/android/hardware/Camera.Parameters.html</a>
 *
 * Unless otherwise specified, all of the API methods that are described in
 * this section belong to the NvCamera.NvParameters class.
 */
/*@{*/

    public void setParameters(NvParameters params) {
        native_setCustomParameters(params.flatten());
    }

    public NvParameters getParameters() {
        NvParameters p = new NvParameters();
        String s = native_getCustomParameters();
        p.unflatten(s);
        return p;
    }
/*@}*/

    public class NvParameters extends Parameters {
        private static final String NV_CAPTURE_MODE = "nv-capture-mode";
        private static final String NV_CONTINUOUS_SHOT_MODE = "shot2shot";
        private static final String NV_NORMAL_SHOT_MODE = "normal";
        private static final String NV_NSL_NUM_BUFFERS = "nv-nsl-num-buffers";
        private static final String NV_NSL_SKIP_COUNT = "nv-nsl-burst-skip-count";
        private static final String NV_NSL_BURST_PICTURE_COUNT = "nv-nsl-burst-picture-count";
        private static final String NV_SKIP_COUNT = "nv-burst-skip-count";
        private static final String NV_BURST_PICTURE_COUNT = "nv-burst-picture-count";
        private static final String NV_RAW_DUMP_FLAG = "nv-raw-dump-flag";
        private static final String NV_EV_BRACKET_CAPTURE = "nv-ev-bracket-capture";
        private static final String NV_FOCUS_AREAS = "focus-areas";
        private static final String NV_METERING_AREAS = "metering-areas";
        private static final String NV_COLOR_CORRECTION = "nv-color-correction";
        private static final String NV_SATURATION = "nv-saturation";
        private static final String NV_CONTRAST = "nv-contrast";
        private static final String NV_ADVANCED_NOISE_REDUCTION_MODE = "nv-advanced-noise-reduction-mode";
        private static final String NV_EDGE_ENHANCEMENT = "nv-edge-enhancement";
        private static final String NV_EXPOSURE_TIME = "nv-exposure-time";
        private static final String NV_PICTURE_ISO = "nv-picture-iso";
        private static final String NV_FOCUS_POSITION = "nv-focus-position";
        private static final String NV_AUTOWHITEBALANCE_LOCK = "auto-whitebalance-lock";
        private static final String NV_AUTOEXPOSURE_LOCK = "auto-exposure-lock";
        private static final String NV_STEREO_MODE = "nv-stereo-mode";
        private static final String NV_VIDEO_SPEED = "nv-video-speed";
        private static final String NV_SENSOR_CAPTURE_RATE = "nv-sensor-capture-rate";
        private static final String NV_CAPABILITY_FOR_VIDEO_SIZE = "nv-capabilities-for-video-size";
        private static final String NV_AUTO_ROTATION = "nv-auto-rotation";
        private static final String NV_STILL_HDR = "nv-still-hdr";

        // Cloned from Camera.java to avoid changing to protected in there
        private static final String NV_SUPPORTED_VALUES_SUFFIX = "-values";

        protected NvParameters() {
            super();
        }

        // Cloned from Camera.java to avoid changing to protected in there
        // Splits a comma delimited string to an ArrayList of String.
        // Return null if the passing string is null or the size is 0.
    /**
     * @hide
     */
        protected ArrayList<String> splitCloned(String str) {
            if (str == null) return null;

            // Use StringTokenizer because it is faster than split.
            StringTokenizer tokenizer = new StringTokenizer(str, ",");
            ArrayList<String> substrings = new ArrayList<String>();
            while (tokenizer.hasMoreElements()) {
                substrings.add(tokenizer.nextToken());
            }
            return substrings;
        }

/** @name Negative Shutter Lag
 *
 * Negative Shutter Lag (NSL) is a generalization of Zero Shutter Lag.
 * In essence, a circular queue of the most recent images from the sensor
 * is maintained during preview. At capture time, the desired images
 * are returned. This allows not only zero shutter lag, but negative shutter
 * lag, i.e., permitting the capture of images from before the capture was
 * requested.
 *
 * The key components of the NSL API are buffer management and describing
 * the NSL burst.
 *
 * The number of NSL buffers is managed with setNSLNumBuffers() and
 * getNSLNumBuffers(). The implementation will allocate or free memory
 * as required. Note that requesting 0 buffers will free all
 * NSL buffers.
 *
 * The NSL burst is controlled by two parameters, the skip count and the
 * burst count. The burst count is straightforward: it indicates the
 * number of frames to return to the application. The skip count tells
 * the implementation how many frames to drop in between the frames that
 * are returned. To capture the burst with as little change between
 * images as possible, set the skip count to 0. In contrast, a
 * \em best picture feature may want temporal gaps between the frames to
 * allow time for more change between frames.
 *
 * @note NSL burst has the same description as a regular burst.
 * A non-zero NSL burst count indicates the number of frames to capture
 * (return) from the NSL buffers. A non-zero regular burst count
 * indicates the number of frames to capture live (no NSL). If both
 * are non-zero, the NSL frames will be returned first followed by the
 * live burst frames. As an example, to capture 8 frames with the first
 * 5 from the NSL buffers, set the NSL burst length to 5 and the regular
 * burst length to 3.
 *
 */
/*@{*/
    /**
     * Sets the current number of negative shutter lag buffers.
     *
     * Negative shutter lag will actively store frames from the sensor as long
     * as a nonzero number of buffers are allocated. As the number of buffers
     * that the driver can allocate depends on the available memory size, the
     * actual value may be smaller than the set value.
     *
     * Defaults to 0.
     */
        public void setNSLNumBuffers(int num) {
            String v = Integer.toString(num);
            set(NV_NSL_NUM_BUFFERS, v);
        }

    /**
     * Gets the current number of negative shutter lag buffers.
     *
     * Use this to find out how many NSL buffers were successfully allocated on
     * a request to the driver. It is the application's responsibility to query
     * this after setting it, to confirm the number of successfully allocated
     * buffers. If buffers are allocated and an application requests a
     * resolution change, the driver will dynamically re-allocate the buffers
     * to accommodate the new resolution. If it is unable to allocate the
     * number of previously requested buffers due to memory restrictions, it
     * will allocate as many as it is able to. Because of this, applications
     * must also query this parameter after a resolution change to find out if
     * the number of allocated buffers has changed.
     */
        public int getNSLNumBuffers() {
            return getInt(NV_NSL_NUM_BUFFERS);
        }

    /**
     * Sets the current burst skip count for negative shutter lag.
     *
     * This determines the number of frames that will be
     * skipped between each frame that is saved for application use; this is
     * useful for requesting fewer NSL images that span a larger time period.
     * If skip count is set to N, one out of every N+1 frames will be saved
     * to the NSL buffer list for access by the application; the intermediate
     * frames will be lost forever. With large skip counts, there could be a
     * potentially large time difference between each frame that is saved.
     * For this reason and because most applications will likely want to ensure
     * that there is at least one frame that is as close in time as possible to
     * the shutter press, the driver will make an exception to the general
     * algorithm of "save one frame, skip N frames, save one frame..." by
     * always saving the most recent frame that is captured from the sensor.
     *
     * For example, in an NSL burst capture of 1, the frame that is returned
     * will always be as close in time as possible to the shutter press. In
     * an NSL burst capture of 5, the frame that is closest in time to the
     * shutter press will be returned, and the remaining 4 frames will be
     * spaced out in time according to the current value of the skip count.
     *
     * Defaults to 0.
     */
        public void setNSLSkipCount(int count) {
            String v = Integer.toString(count);
            set(NV_NSL_SKIP_COUNT, v);
        }

    /**
     * Gets the current burst skip count for negative shutter lag.
     */
        public int getNSLSkipCount() {
            return getInt(NV_NSL_SKIP_COUNT);
        }

    /**
     * Sets the current burst count for negative shutter lag captures.
     *
     * The value must be not more than the number of allocated negative shutter
     * lag buffers. If this parameter is set, the requested frames will be
     * delivered one-at-a-time to the JPEG callback that is triggered by the
     * standard @c takePicture() request.  Apps should not call @c startPreview()
     * until all of the requested burst frames have been received by the JPEG
     * callback.
     *
     * For example, a negative shutter lag burst of 5 will call the JPEG
     * callback 5 times. The negative shutter lag frames will be delivered
     * chronologically; the frame with the oldest timestamp will be the first
     * frame that is sent to the JPEG callback.
     *
     * Defaults to 0.
     */
        public void setNSLBurstCount(int count) {
            String v = Integer.toString(count);
            set(NV_NSL_BURST_PICTURE_COUNT, v);
        }

    /**
     * Gets the current burst count for negative shutter lag captures.
     */
        public int getNSLBurstCount() {
            return getInt(NV_NSL_BURST_PICTURE_COUNT);
        }
/*@}*/

/** @name Burst Capture
 *
 * The Burst Capture API provides a facility for capturing a burst of
 * images live from the sensor--a 'normal' burst capture sequence,
 * contrasted with the NSL captures described in "Negative Shutter Lag".
 *
 * The Burst Capture API does not involve allocation of additional
 * buffers. In some applications, this may be preferable to NSL's
 * memory footprint increase, not to mention the savings in memory
 * bandwidth and power from keeping the sensor in a more efficient
 * preview resolution during preview. Further, the normal burst captures
 * do not suffer from preview frame rate reductions due to limitations in
 * the full-resolution frame rate available from some sensors.
 *
 * The burst is controlled by two parameters: the skip count and the
 * burst count. The burst count is straightforward--it indicates the
 * number of frames to return to the application. The skip count tells
 * the implementation how many frames to drop in between the frames that
 * are returned. To capture the burst with as little change between
 * images as possible, set the skip count to 0. In contrast, a
 * <em>best picture</em> feature may want temporal gaps between the frames to
 * allow time for more change between frames.
 *
 * @note NSL burst has the same description as a regular burst.
 * A non-zero NSL burst count indicates the number of frames to capture
 * (return) from the NSL buffers. A non-zero regular burst count
 * indicates the number of frames to capture live (no NSL). If both
 * are non-zero, the NSL frames will be returned first followed by the
 * live burst frames. As an example, to capture 8 frames with the first
 * 5 from the NSL buffers, set the NSL burst length to 5 and the regular
 * burst length to 3.
 */
/*@{*/

    /**
     * Sets the number of frames from the sensor to be skipped between frames
     * that are sent to the encoder. For example, with a burst count of 2 and a
     * skip count of 1, the first frame from the sensor will be captured and
     * processed, the next frame will be dropped, and then the frame after that
     * will be captured and processed.
     *
     * Defaults to 0.
     */
        public void setSkipCount(int count) {
            String v = Integer.toString(count);
            set(NV_SKIP_COUNT, v);
        }

    /**
     * Gets the current burst skip count.
     */
        public int getSkipCount() {
            return getInt(NV_SKIP_COUNT);
        }

    /**
     * Sets the current burst picture count. Apps should not call
     * @c startPreview() until all of the requested burst frames
     * have been received by the JPEG callback.
     *
     * Defaults to 1.
     */
        public void setBurstCount(int count) {
            String v = Integer.toString(count);
            set(NV_BURST_PICTURE_COUNT, v);
        }

    /**
     * Gets the current burst picture count.
     */
        public int getBurstCount() {
            return getInt(NV_BURST_PICTURE_COUNT);
        }
/*@}*/

/** @name Continuous Shot Mode
 *
 * Continuous Shot Mode is a feature in which camera continually takes snapshots
 * while the user is holding the shutter button.
 *
 * Camera enters in the continuous shot mode once the application detects that
 * the user has pressed and held the shutter button for a period of time. Then the
 * application repeatedly issues capture requests after receiving shutter callbacks
 * for previous captures until it detects the user has released shutter button.
 * Camera then goes back to normal capture mode.
 *
 * The difference between the continuous shot and burst capture is that in continuous shot
 * the application needs to send new captures repeatedly and user will hear the shutter sound.
 * NSL and burst capture needs to be disabled before entering to continuous shot mode.
 * Otherwise, camera will enter to continuous shot mode, do a fix number burst captures
 * first then do the continuous shot.
 *
 * To maximize image capture frame rate in continuous shot mode,
 * ANR, face detection and postview are automatically disabled.
 * Continuous auto focus runs half the normal speed.
 * Flash will only strobe the last frame of the sequence.
 * After camera exits continuous shot mode, previously enabled features and behaviors resume.
 */
/*@{*/
    /**
     * Sets nv capture mode.
     * When mode is NV_CONTINUOUS_SHOT_MODE, continuous shot mode is enabled
     * When mode is NV_NORMAL_SHOT_MODE, camera go back to normal mode
     *
     * Defaults to NV_NORMAL_SHOT_MODE.
     */
        public boolean setNVShotMode(String mode) {
            boolean retVal = false;
            if(mode != null) {
                if(mode.equals(NV_CONTINUOUS_SHOT_MODE)) {
                    set(NV_CAPTURE_MODE, NV_CONTINUOUS_SHOT_MODE);
                    retVal = true;
                }
                else if(mode.equals(NV_NORMAL_SHOT_MODE)) {
                    set(NV_CAPTURE_MODE, NV_NORMAL_SHOT_MODE);
                    retVal = true;
                }
            }
            return retVal;
        }

    /**
     * Returns true if the camera device is in Continuous Shot Mode.
     */
        public boolean inContinuousShotMode() {
            String nvCaptureMode = get(NV_CAPTURE_MODE);
            if(nvCaptureMode != null)
                return nvCaptureMode.equals(NV_CONTINUOUS_SHOT_MODE);
            else return false;
        }
/*@}*/

/** @name Raw Dump Flag
 *
 * Manages the raw dump flags.
 *
 * The Raw Dump API controls the functionality for capturing raw image
 * data from sensors.
 *
 * @note This refers to the raw Bayer data from
 * the sensors, not YUV data right before image or video compression.
 *
 * The raw dump value contains three controls:
 * - Bit 0 turns the facility on and off.
 * - Bit 1 distinguishes between capturing raw data to mass storage
 *     and providing raw data to the application.
 * - Bit 2 controls the inclusion of NVIDIA's raw file header.
 */
/*@{*/

    /**
     * Sets the current raw dump flag.
     * This can be used to trigger a Bayer raw dump.  This feature is
     * currently under development.
     *
     * This is a 3-bit indicator, where:
     * - bit 0 is raw dump on/off
     * - bit 1 is dump-to-file in camera driver on/off
     * - bit 2 is include NV raw header on/off
     *
     * @note Raw dump feature must be off in burst capture mode.
     *
     * Defaults to 0.
     */
        public void setRawDumpFlag(int flag) {
            String v = Integer.toString(flag);
            set(NV_RAW_DUMP_FLAG, v);
        }

    /**
     * Gets the current raw dump flag.
     */
        public int getRawDumpFlag() {
            return getInt(NV_RAW_DUMP_FLAG);
        }

/*@}*/
/** @name Bracket Capture
 *
 * Sets the EV bracket capture setting.
 *
 * After this property is set and a burst capture command is issued, each
 * image in the burst capture sequence will be adjusted by the specified
 * amount by the EV setting (-3 to 3 inclusive).
 *
 * The first image received will have the left most compensation value;
 * likewise, the last image received will contain the last specified
 * compensation value.
 *
 * Setting the value to an empty string disables bracket capture.
 *
 * Interaction with NSL captures during bracket capture NSL is not
 * available. Bracket capture is cleared by setting an empty string, and
 * once cleared NSL will operate as normal. Non-burst captures will operate
 * as normal.
 *
 * - Item value range: [-3.0, +3.0] float
 * - Number of items:  [0, 2-7] , 0 or at least two and up to seven values.
 */
/*@{*/
        public void setEvBracketCapture(float[] evValues) {
            StringBuilder evString = new StringBuilder(35);

            if (evValues == null)
            {
                evString.append(" ");
            }
            else
            {
                for (int i = 0; i < evValues.length - 1; i++)
                {
                    evString.append(evValues[i]);
                    evString.append(",");
                }
                evString.append(evValues[evValues.length -1]);
            }
            set(NV_EV_BRACKET_CAPTURE, evString.toString());
        }
/*@}*/

/** @name Sensor Capture Rate
 *
 * Sets the sensor's capture rate during slow motion video capture.
 *
 * Sets the sensor's capture rate (in fps).  This is used with
 * setVideoSpeed(). To give the effects of slow motion video
 * capture, the sensor is configured at a high framerate, and the timestamps
 * are modified based on the value of the video speed parameter.
 *
 * For example, if video speed is set for a slow motion video capture (0.5
 * or half speed), and the sensor's capture rate is set at 60 fps, then to
 * get a slow motion video capture effect, the outgoing camera buffer's
 * timestamp will be modified to 33 ms (1 / FrameRate * recording speed)
 * instead of 17 ms (1 / FrameRate).
 *
 * This parameter must be set after <a href=
"http://developer.android.com/reference/android/hardware/Camera.Parameters.html#setPreviewFrameRate(int)"
 * target="_blank">PreviewFrameRate</a>. \c PreviewFrameRate
 * is used across encoder and writer, and as this is a special case we
 * want the playback rate different from the capture rate.
 *
 * This parameter is only valid for a slow-motion use case, and will only be
 * processed by the driver if video speed is less than 1x.
 */
/*@{*/
        public void setSensorCaptureRate(int value) {
            String v = Integer.toString(value);
            set(NV_SENSOR_CAPTURE_RATE, v);
        }

     /**
      * Gets the supported sensor capture rate.
      */
        public List<Integer> getSupportedSensorCaptureRate() {

            String str = get(NV_SENSOR_CAPTURE_RATE + NV_SUPPORTED_VALUES_SUFFIX);
            if (str == null) return null;

            ArrayList<Integer> SensorCaptureRate = new ArrayList<Integer>();
            StringTokenizer tokenizer = new StringTokenizer(str, ",");

            while (tokenizer.hasMoreElements()) {
                String token = tokenizer.nextToken();
                int r = Integer.parseInt(token);
                SensorCaptureRate.add(r);
            }
            return SensorCaptureRate;
        }
/*@}*/

/** @name Video Speed
 *
 * Sets the video speed for camera recording.
 *
 * The video speed parameter adjusts the timestamps of captured frames to
 * give the effect of slow motion and fast motion video recording.
 * Currently supported values are
 * - 0.25  - 1/4x  - One-fourth speed (Slow motion)
 * - 0.5   - 1/2x  - Half speed (Slow motion)
 * - 1.0   - 1x    - Normal speed
 * - 2.0   - 2x    - Twice the normal speed (Fast motion)
 * - 3.0   - 3x    - Three times the normal speed (Fast motion)
 * - 4.0   - 4x    - Four times the normal speed (Fast motion)
 *
 * For example, if video speed is set for a slow motion video capture (0.5
 * or half speed), and the sensor's capture rate is set at 60 fps, then to
 * get a slow motion video capture effect, the outgoing camera buffer's
 * timestamp will be modified to 33 ms (1 / FrameRate * recording speed)
 * instead of 17 ms (1 / FrameRate).
 *
 * When slow-motion recording is complete, the application must first reset
 * the video speed to its default value (1x), and then it must call
 * setPreviewFrameRate() to restore preview to the desired frame-rate.
 */
/*@{*/
        /**
         * Sets the video speed for camera recording.
         */
        public void setVideoSpeed(float value) {
            String v = Float.toString(value);
            set(NV_VIDEO_SPEED, v);
        }

     /**
      * Gets the current supported video speed.
      */
        public List<Float> getSupportedVideoSpeeds() {

            String str = get(NV_VIDEO_SPEED + NV_SUPPORTED_VALUES_SUFFIX);
            if (str == null) return null;

            ArrayList<Float> videospeeds = new ArrayList<Float>();
            StringTokenizer tokenizer = new StringTokenizer(str, ",");

            while (tokenizer.hasMoreElements()) {
                String token = tokenizer.nextToken();
                float s = Float.parseFloat(token);
                videospeeds.add(s);
            }
            return videospeeds;
        }
/*@}*/


/** @name Maximum Video Preview Resolution/Frame Rate
 *
 * Gets the list of maximum preview resolution supported with the maximum
 * frame rate for each video resolution. Applications can get this list and
 * then, based upon the suppported preview resolutions and the supported
 * frame rates, set the preview resolution and the frame rate for a
 * specific video resolution, ensuring the preview resolution set by it
 * does not exceed the maximum preview reolution mentioned in this list.
 * Applications must also ensure the sensor-capture-rate does not
 * exceed the maximum frame rate mentioned in this list for the requested
 * video size.
 */
/*@{*/
        public List<NvVideoPreviewFps> getCapabilitiesForVideoSizes() {
            ArrayList<NvVideoPreviewFps> VideoPreviewSizeFPS =
                new ArrayList<NvVideoPreviewFps>();

            /** Gets the supported sensor mode list. */
            String str = get(NV_CAPABILITY_FOR_VIDEO_SIZE + NV_SUPPORTED_VALUES_SUFFIX);
            if (str == null) return null;
            StringTokenizer tokenizer1 = new StringTokenizer(str, ",");

            while (tokenizer1.hasMoreElements()) {
                NvVideoPreviewFps TempVideoPreviewFps = new NvVideoPreviewFps();

                String token1 = tokenizer1.nextToken();

                /** Breaks down the string into individual sensor modes. */
                StringTokenizer tokenizer2 = new StringTokenizer(token1, ":");

                String token2 = tokenizer2.nextToken();
                StringTokenizer tokenizer3 = new StringTokenizer(token2, "x");
                String token3 = tokenizer3.nextToken();

                /** Parses video height, width. */
                TempVideoPreviewFps.VideoWidth = Integer.parseInt(token3);
                token3 = tokenizer3.nextToken();
                TempVideoPreviewFps.VideoHeight = Integer.parseInt(token3);

                token2 = tokenizer2.nextToken();
                tokenizer3 = new StringTokenizer(token2, "x");
                token3 = tokenizer3.nextToken();

                /** Parses maximum preview height, width. */
                TempVideoPreviewFps.MaxPreviewWidth = Integer.parseInt(token3);
                token3 = tokenizer3.nextToken();
                TempVideoPreviewFps.MaxPreviewHeight = Integer.parseInt(token3);

                token2 = tokenizer2.nextToken();
                /* Parses maximum frame rate. */
                TempVideoPreviewFps.MaxFps = Integer.parseInt(token2);

                VideoPreviewSizeFPS.add(TempVideoPreviewFps);
            }
            return VideoPreviewSizeFPS;
        }

/*@}*/

/** @name Focus Areas
 *
 * The Focus Areas API manages the focus regions of interest.
 *
 * Focus Areas allow an application to guide where the ISP computes the
 * focus statistics in an image. Two examples of how an application might
 * use this are touch to focus and focusing on faces. In touch to focus,
 * the application maps a touch event on a preview image to a change in the
 * focus windows. To focus on faces, the application converts the face regions
 * reported by a face detection algorithm into focus areas.
 *
 * To simplify the application logic, the sensor's field of view (FOV) is
 * mapped to a canonical rectangle with coordinates (-1000, -1000) in the upper
 * left corner to (1000, 1000) in the lower right corner. When digital
 * zoom settings change, the mapping is updated to ensure that the FOV (and
 * thus the images available to the application) always map to the canonical
 * rectangle.
 *
 * A list of windows can be expressed as either a string or as an
 * @c ArrayList of @c NvWindows and can be set and queried in either form.
 * The getMaxNumFocusAreas() call is provided to query the number of
 * focus windows supported by the device upon which the code is running.
 */
/*@{*/

    /**
     * Sets the current focus region of interest.
     *
     * The left, top, right, and bottom window coordinates are specified in
     * screen coordinates, from -1000 to 1000. Weight should always be set to 1
     * for focus areas. When the image is zoomed, the region will take effect
     * relative to the zoomed field of view. The default focus area is subject
     * to change, and we recommend that applications query it with a call to
     * getFocusAreas() before setting it. See the Example 3.
     * At the time this documentation was written, the default focus window was
     * (-240,-240,240,240,1).
     *
     * @note The NvCamera.NvWindow class has integer members left, right, top,
     * bottom, and weight, to facilitate the convenient setting of focus and
     * exposure areas.
     */
        public void setFocusAreas(ArrayList<NvWindow> windowList) {
            String str = WindowsToString(windowList);
            if (str != null)
                set(NV_FOCUS_AREAS, str);
        }

    /**
     * Queries the current focus region of interest.
     *
     * @return The argument that is passed in has the current focus region
     * written to it when the function returns.
     */
        public void getFocusAreas(ArrayList<NvWindow> windowList) {
            String str = get(NV_FOCUS_AREAS);
            StringToWindows(windowList, str);
        }

/*@}*/

/** @name Exposure Areas
 *
 * Exposure Modes can be viewed as special use-cases of the exposure
 * areas. Detailed examples are provided in the example code section.
 *
 * - Center: We are currently working out the HAL APIs for programmable
 *   center-weighted windows of varying sizes. Until that is complete, we can
 *   leverage the fact that the driver's default exposure algorithm is
 *   center-weighted across the entire image. We can program this behavior by
 *   setting the special (0,0,0,0,0) exposure area.
 * - Average: this is equivalent to setting an exposure area that covers the
 *   entire field of view, i.e. (-1000,-1000,1000,1000,1);
 * - Spot: this is equivalent to setting a small exposure area around the region
 *   of interest.
 */
/*@{*/

    /**
     * Sets the current exposure region of interest.
     *
     * The left, top, right, and bottom coordinates are specified in screen
     * coordinates, from -1000 to 1000. The weight parameter can range anywhere
     * from 1 to 1000, and it is used by the driver to weight the @b importance
     * of the regions relative to each other in the exposure calculations.
     *
     * The regions can overlap, which may be useful if a smoother AE change is
     * desired. The weights are added in the overlap region.
     * For example, setting areas of
     *   (-900,-900,900,900,1),(-100,-100,100,100,1)
     * will configure a large region of weight 1, with a small centered region
     * of weight 2.
     *
     * When the image is zoomed, the regions will take effect relative to the
     * zoomed field of view. The application can set up to the number of areas
     * that is returned by getMaxNumMeteringAreas(). Multiple regions can be
     * configured in a single call by appending them to the list that is passed
     * as an argument. The special case of all 0's will restore the driver's
     * default exposure algorithm, which uses a Gaussian, center-weighted
     * calculation over the whole image.
     */
        public void setMeteringAreas(ArrayList<NvWindow> windowList) {
            String str = WindowsToString(windowList);
            if (str != null)
                set(NV_METERING_AREAS, str);
        }

    /**
     * Queries the current exposure region of interest.
     *
     * @return The argument that is passed in will have the current
     * exposure region written to it when the function returns.
     */
        public void getMeteringAreas(ArrayList<NvWindow> windowList) {
            String str = get(NV_METERING_AREAS);
            StringToWindows(windowList, str);
        }
/*@}*/

/** @name Color Correction
 *
 * The Color Correction API provides for application control of a 4x4
 * color correction matrix (CCM) in the imaging pipeline. An example use of
 * the color correction API is to provide a color effect like sepia.
 *
 * @note The CCM controlled by this API is also the RGB -> YUV
 * conversion matrix. It should therefore fold in this calculation as well.
 *
 */
/*@{*/

    /**
     * Sets the manual 4x4 color correction matrix.
     *
     * Passing in a matrix of all 0's restores the default driver behavior.
     * Values in the array are specified across the rows, i.e. row 0 will be
     * entries 0-3 of the array, row 2 will be entries 4-7, etc.
     *
     * Defaults to all 0's.
     */
        public void setColorCorrection(String str) {
            set(NV_COLOR_CORRECTION, str);
        }

    /**
     * Gets the current color correction matrix, filling the argument with the
     * current values.
     */
        public String getColorCorrection() {
            return get(NV_COLOR_CORRECTION);
        }

        public void setColorCorrection(float[] matrix) {
            if (matrix != null && matrix.length == 16)
            {
                StringBuilder matrixString = new StringBuilder(256);
                for (int i = 0; i < 15; i++)
                {
                    matrixString.append(matrix[i]);
                    matrixString.append(",");
                }
                matrixString.append(matrix[15]);
                set(NV_COLOR_CORRECTION, matrixString.toString());
            }
        }

        public void getColorCorrection(float[] matrix) {
            if (matrix != null && matrix.length == 16)
            {
                String str = get(NV_COLOR_CORRECTION);
                if (str == null)
                {
                    for (int i = 0; i < 16; i++)
                        matrix[i] = 0;
                }
                else
                {
                    StringTokenizer tokenizer = new StringTokenizer(str, ",");
                    int index = 0;
                    while (tokenizer.hasMoreElements())
                    {
                        String token = tokenizer.nextToken();
                        matrix[index++] = Float.parseFloat(token);
                    }
                }
            }
        }
/*@}*/

/** @name Saturation
 *
 * The Saturation API overrides the saturation value described in
 * the configuration file, if present. The saturation value is an integer
 * in the range [-100, 100].
 */
/*@{*/

    /**
     * Sets the current saturation value.
     *
     * Defaults to 0.
     */
        public void setSaturation(int saturation) {
            String v = Integer.toString(saturation);
            set(NV_SATURATION, v);
        }

    /**
     * Gets the current saturation value.
     */
        public int getSaturation() {
            return getInt(NV_SATURATION);
        }
/*@}*/

/** @name Contrast
 *
 * The Contrast control provides an application with the ability to
 * bias the calcuation performed by the automatic tone curve calculation.
 *
 * The implementation maps strings onto constrast bias values.
 * The current mapping is from  "lowest", "low", "normal", "high", "highest"
 * onto biases of -100, -50, 0, 50, 100, though this is subject to change.
 *
 * A bias value of:
 * - 0 indicates no bias--the automatic tone cuve will be used without
 *    modification.
 * - -100 indicates maximum contrast reduction on top of the automatic tone
 *    curve.
 * - 100 indicates maximum contrast boost on top of the automatic tone curve.
 *
 */
/*@{*/

    /**
     * Sets the current contrast value. String argument can be "lowest",
     * "low", "normal", "high", or "highest".
     *
     * Defaults to "normal".
     */
        public void setContrast(String str) {
            set(NV_CONTRAST, str);
        }

    /**
     * Gets the current contrast value.
     */
        public String getContrast() {

            return get(NV_CONTRAST);
        }
/*@}*/

/** @name Edge Enhancement
 *
 * The Edge Enhancement API enables an application to bias the edge
 * enhancement value in the range [-100, 100].
 */
/*@{*/

    /**
     * Sets the edge enhancement value. The valid range is from -100 to 100.
     * Set -101 to turn edge enhancement off.
     *
     * Defaults to 0.
     */

        public void setEdgeEnhancement(int value) {
            String v = Integer.toString(value);
            set(NV_EDGE_ENHANCEMENT, v);
        }


    /**
     * Gets the current edge enhancement value.
     */
        public int getEdgeEnhancement() {
            return getInt(NV_EDGE_ENHANCEMENT);
        }
/*@}*/

/** @name ADVANCED_NOISE_REDUCTION
 *
 * The advanced noise reduction control provides an application with the
 * ability to change the mode of operation of the hardware accelerated
 * advanced noise reduction algorithms.
 *
 * The implementation maps strings onto different modes of operation.
 *
 * A mode value of:
 * - off indicates that the algorithm should be turned completely off
 * - forceon indicates that the algorithm always applies onto the scene
 *   no matter the conditions.
 * - auto indicates that the algorithm applies onto the scene when
 *   conditions warrant.
 *
 */
/*@{*/

    /**
     * Sets the hardware accelerated advanced noise reduction mode value.
     * String argument can be "off", "forceon", "auto"
     *
     * Defaults to "auto".
     */
        public void setAdvancedNoiseReductionMode(String str) {
            set(NV_ADVANCED_NOISE_REDUCTION_MODE, str);
        }

    /**
     * Gets the current advaned noise reduction mode value.
     */
        public String getAdvancedNoiseReductionMode() {

            return get(NV_ADVANCED_NOISE_REDUCTION_MODE);
        }
/*@}*/

/** @name Exposure Time
 *
 * The Exposure Time API constrains the sensor exposure time to
 * the requested number of microseconds. Setting an exposure time of zero
 * remove the exposure time constraint.
 *
 * Note that the exposure time constraint will be respected, even if AE
 * is unable to reach a desired exposure. Said another way, if AE reaches
 * gain control limits, the image will be too bright (at minimum gain)
 * or too dark (at maximum gain).
 */
/*@{*/

    /**
     * Sets the current exposure time. Set to 0 to enable auto-exposure.
     * Nonzero values are interpreted in units of microseconds.
     *
     *  Defaults to auto.
     */
        public void setExposureTime(int value) {
            String v = Integer.toString(value);
            set(NV_EXPOSURE_TIME, v);
        }

    /**
     * Gets the current exposure time.
     */
        public int getExposureTime() {
            return getInt(NV_EXPOSURE_TIME);
        }
/*@}*/

/** @name ISO
 *
 * The Picture ISO API constrains the ISO to the selected value.
 * Setting ISO to "auto" returns to automatic ISO selection.
 *
 * @note The exposure time constraint will be respected, even if AE
 * is unable to reach a desired exposure. Said another way, if AE reaches
 * exposure control limits, the image will be too bright (at minimum exposure)
 * or too dark (at maximum exposure).
 */
/*@{*/

    /**
     * Sets the current ISO value. The string can take on values of
     * "auto","100","200","400", or "800".
     *
     * Defaults to "auto".
     */
        public void setPictureISO(String str) {
            set(NV_PICTURE_ISO, str);
        }

    /**
     * Gets the current ISO value.
     */
        public String getPictureISO() {
            return get(NV_PICTURE_ISO);
        }
/*@}*/

/** @name Focus Position
 *
 * The focus position can be queried and set.
 *
 * The Focus Position API provides access to low-level focuser movement.
 *
 * The position units are focus steps, which are normally fall in the
 * range [0, 1000]. In addition, three special focus positions can be used:
 * INF1 (far limit), MAC1 (macro limit), and HYPERFOCAL.
 *
 * Setting focus position is only effective in certain circumstances.
 * It is always effective in manual focus mode. Auto Focus (AF) mode is
 * more complicated: the focus position only takes effect when
 * continuous auto-focus (CAF) is not enabled and AF has converged.
 */
/*@{*/

    /**
     * Sets the current focus position, if it is supported by the focuser.
     *
     * Defaults to 0.
     */
        public void setFocusPosition(int position) {
            String v = Integer.toString(position);
            set(NV_FOCUS_POSITION, v);
        }

    /**
     * Gets the current focus position.
     */
        public int getFocusPosition() {
            return getInt(NV_FOCUS_POSITION);
        }
/*@}*/

/** @name Auto White Balance and Auto Exposure Lock
 *
 * @c AutoWhiteBalance can be locked, so the colors no longer are adjusted
 * automatically based off the scene. It can be unlocked with the corresponding
 * function.
 *
 * @c AutoExposure can be locked, so the exposure is no longer automatically
 * adjusted based off the scene.
 */
/*@{*/

    /**
     * Sets the auto white balance lock.
     */
        public void setAutoWhiteBalanceLock(boolean lock) {
            String v = Boolean.toString(lock);
            set(NV_AUTOWHITEBALANCE_LOCK, v);
        }

    /**
     * Gets the auto white balance lock.
     */
        public boolean getAutoWhiteBalanceLock() {
            String v = get(NV_AUTOWHITEBALANCE_LOCK);
            return Boolean.valueOf(v);
        }

    /**
     * Sets the auto exposure lock.
     */
        public void setAutoExposureLock(boolean lock) {
            String v = Boolean.toString(lock);
            set(NV_AUTOEXPOSURE_LOCK, v);
        }

    /**
     * Gets the auto exposure lock.
     */
        public boolean getAutoExposureLock() {
            String v = get(NV_AUTOEXPOSURE_LOCK);
            return Boolean.valueOf(v);
        }
/*@}*/

/** @name Auto Rotation
 *
 * Auto rotation can be turned on to allow rotation of images before
 * writing to disk, while turning it off writes the orientation value in
 * the JPEG EXIF info.
 */
/*@{*/
    /**
     * Sets the auto rotation mode for images.
     */
        public void setAutoRotation(boolean value) {
            String v = Boolean.toString(value);
            set(NV_AUTO_ROTATION, v);
        }

    /**
     * Gets the auto rotation mode for images.
     */
        public boolean getAutoRotation() {
            String v = get(NV_AUTO_ROTATION);
            return Boolean.valueOf(v);
        }
/*@}*/

/** @name High Dynamic Range (HDR) Processing
 *
 * You can enable High Dynamic Range (HDR) processing. When enabled, HDR does an
 * hdr capture. HDR  combines multiple images captured at differing exposures
 * into one combined image.
 */
/*@{*/

    /**
     * Sets the HDR processing mode.
     */
        public void setStillHDR(boolean enable) {
            String v = Boolean.toString(enable);
            set(NV_STILL_HDR, v);
        }

    /**
     * Gets the HDR processing mode.
     */
        public boolean getStillHDR() {
            String v = get(NV_STILL_HDR);
            return Boolean.valueOf(v);
        }
/*@}*/

/** @name Stereo Mode
 *
 * Stereo mode must be set before the camera sensor powers on.
 */
/*@{*/
    /**
     * Sets the current camera stereo mode.
     *
     * This method sets the camera stereo mode as specified in the argument.
     * Like with standard parameter calls, the subsequent call to
     * setParameters() fails if the given stereo mode is not supported.
     *
     * @param stereoMode This string can take any of the values returned by
     * com.nvidia.NvCamera.NvParameters::getSupportedStereoModes().
     *
     */
        public void setStereoMode(String stereoMode) {
            set(NV_STEREO_MODE, stereoMode);
        }

    /**
     * Gets the current stereo camera mode.
     *
     * This method gets the currently-selected stereo mode of the camera.
     *
     * @return One of the values returned by
     * com.nvidia.NvCamera.NvParameters::getSupportedStereoModes().
     */
        public String getStereoMode() {
            return get(NV_STEREO_MODE);
        }

    /**
     * Gets the current stereo camera mode.
     *
     * This method returns all the stereo modes that will be accepted by a
     * setStereoMode() method call.
     *
     * @return List of supported stereo camera modes for the
     * currenly selected camera. Will be a subset of "(left, right, stereo)".
     * Mono cameras will have list with only one element "(left)".
     * @see com.nvidia.NvCamera.NvParameters#setStereoMode(),
     * com.nvidia.NvCamera.NvParameters#getStereoMode()
     */
        public List<String> getSupportedStereoModes() {
            String str = get(NV_STEREO_MODE + NV_SUPPORTED_VALUES_SUFFIX);
            return splitCloned(str);
        }
/*@}*/

        private String WindowsToString(ArrayList<NvWindow> windowList) {
            if (windowList == null || windowList.size() == 0)
            {
                return null;
            }
            else
            {
                int size = windowList.size();
                StringBuilder windowsString = new StringBuilder(256);
                for (int i = 0; i < size; i++)
                {
                    NvWindow window = windowList.get(i);
                    windowsString.append("(");
                    windowsString.append(window.left);
                    windowsString.append(",");
                    windowsString.append(window.top);
                    windowsString.append(",");
                    windowsString.append(window.right);
                    windowsString.append(",");
                    windowsString.append(window.bottom);
                    windowsString.append(",");
                    windowsString.append(window.weight);
                    windowsString.append(")");
                    if (i != size - 1)
                        windowsString.append(",");
                }
                return windowsString.toString();
            }
        }

        private void StringToWindows(ArrayList<NvWindow> windowList, String str) {
            if (windowList != null && str != null)
            {
                StringTokenizer tokenizer = new StringTokenizer(str, "(");
                while (tokenizer.hasMoreElements())
                {
                    NvWindow window = new NvWindow();
                    String token = tokenizer.nextToken();
                    StringTokenizer subTokenizer = new StringTokenizer(token, ",");
                    String subToken = subTokenizer.nextToken();
                    window.left = Integer.parseInt(subToken);
                    subToken = subTokenizer.nextToken();
                    window.top = Integer.parseInt(subToken);
                    subToken = subTokenizer.nextToken();
                    window.right = Integer.parseInt(subToken);
                    subToken = subTokenizer.nextToken();
                    window.bottom = Integer.parseInt(subToken);
                    subToken = subTokenizer.nextToken();
                    StringTokenizer endTokenizer = new StringTokenizer(subToken, ")");
                    String endToken = endTokenizer.nextToken();
                    window.weight = Integer.parseInt(endToken);
                    windowList.add(window);
                }
            }
        }
    };

/** @name Camera Object
 *
 */
/*@{*/

    /**
     * Creates a new Camera object to access a particular hardware camera.
     */
    public static NvCamera open(int cameraId) {
        return new NvCamera(cameraId);
    }

    /**
     * Creates a new Camera object to access the first back-facing camera on the
     * device. If the device does not have a back-facing camera, this returns
     * null.
     */
    public static NvCamera open() {
        int numberOfCameras = getNumberOfCameras();
        NvCameraInfo cameraInfo = new NvCameraInfo();
        for (int i = 0; i < numberOfCameras; i++) {
            getCameraInfo(i, cameraInfo);
            if (cameraInfo.facing == NvCameraInfo.CAMERA_FACING_BACK) {
                return new NvCamera(i);
            }
        }
        return null;
    }

    NvCamera(int cameraId) {
        super(cameraId);
    }


}

/** @} */

/** @} */
