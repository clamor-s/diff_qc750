/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
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
 * Copyright (C) 2009 The Android Open Source Project
 */

package com.nvidia;

import android.os.SystemProperties;
import android.util.Log;
import java.lang.StringBuilder;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;

public class NvAppSpecificHeuristics {
    private static final String TAG = "NvAppSpecificHeuristics";
    private static final boolean DEBUG_NV_APP_PROFILE = false;
    private static final int scalingMinFreqDefault = 0;
    private static final boolean disableCpuCoreBiasDefault = false;
    private static final boolean disableGpuScalingDefault = false;

    private static final char[] label_0 = { /* com.aurorasoftworks.quadrant.ui.professional */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        'a' ^ 0x55,
        'u' ^ 0x55,
        'r' ^ 0x55,
        'o' ^ 0x55,
        'r' ^ 0x55,
        'a' ^ 0x55,
        's' ^ 0x55,
        'o' ^ 0x55,
        'f' ^ 0x55,
        't' ^ 0x55,
        'w' ^ 0x55,
        'o' ^ 0x55,
        'r' ^ 0x55,
        'k' ^ 0x55,
        's' ^ 0x55,
        '.' ^ 0x55,
        'q' ^ 0x55,
        'u' ^ 0x55,
        'a' ^ 0x55,
        'd' ^ 0x55,
        'r' ^ 0x55,
        'a' ^ 0x55,
        'n' ^ 0x55,
        't' ^ 0x55,
        '.' ^ 0x55,
        'u' ^ 0x55,
        'i' ^ 0x55,
        '.' ^ 0x55,
        'p' ^ 0x55,
        'r' ^ 0x55,
        'o' ^ 0x55,
        'f' ^ 0x55,
        'e' ^ 0x55,
        's' ^ 0x55,
        's' ^ 0x55,
        'i' ^ 0x55,
        'o' ^ 0x55,
        'n' ^ 0x55,
        'a' ^ 0x55,
        'l' ^ 0x55,
    };
    private static final char[] label_1 = { /* com.aurorasoftworks.quadrant.ui.advanced */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        'a' ^ 0x55,
        'u' ^ 0x55,
        'r' ^ 0x55,
        'o' ^ 0x55,
        'r' ^ 0x55,
        'a' ^ 0x55,
        's' ^ 0x55,
        'o' ^ 0x55,
        'f' ^ 0x55,
        't' ^ 0x55,
        'w' ^ 0x55,
        'o' ^ 0x55,
        'r' ^ 0x55,
        'k' ^ 0x55,
        's' ^ 0x55,
        '.' ^ 0x55,
        'q' ^ 0x55,
        'u' ^ 0x55,
        'a' ^ 0x55,
        'd' ^ 0x55,
        'r' ^ 0x55,
        'a' ^ 0x55,
        'n' ^ 0x55,
        't' ^ 0x55,
        '.' ^ 0x55,
        'u' ^ 0x55,
        'i' ^ 0x55,
        '.' ^ 0x55,
        'a' ^ 0x55,
        'd' ^ 0x55,
        'v' ^ 0x55,
        'a' ^ 0x55,
        'n' ^ 0x55,
        'c' ^ 0x55,
        'e' ^ 0x55,
        'd' ^ 0x55,
    };
    private static final char[] label_2 = { /* com.aurorasoftworks.quadrant.ui.standard */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        'a' ^ 0x55,
        'u' ^ 0x55,
        'r' ^ 0x55,
        'o' ^ 0x55,
        'r' ^ 0x55,
        'a' ^ 0x55,
        's' ^ 0x55,
        'o' ^ 0x55,
        'f' ^ 0x55,
        't' ^ 0x55,
        'w' ^ 0x55,
        'o' ^ 0x55,
        'r' ^ 0x55,
        'k' ^ 0x55,
        's' ^ 0x55,
        '.' ^ 0x55,
        'q' ^ 0x55,
        'u' ^ 0x55,
        'a' ^ 0x55,
        'd' ^ 0x55,
        'r' ^ 0x55,
        'a' ^ 0x55,
        'n' ^ 0x55,
        't' ^ 0x55,
        '.' ^ 0x55,
        'u' ^ 0x55,
        'i' ^ 0x55,
        '.' ^ 0x55,
        's' ^ 0x55,
        't' ^ 0x55,
        'a' ^ 0x55,
        'n' ^ 0x55,
        'd' ^ 0x55,
        'a' ^ 0x55,
        'r' ^ 0x55,
        'd' ^ 0x55,
    };
    private static final char[] label_3 = { /* com.netflix.mediaclient */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        'n' ^ 0x55,
        'e' ^ 0x55,
        't' ^ 0x55,
        'f' ^ 0x55,
        'l' ^ 0x55,
        'i' ^ 0x55,
        'x' ^ 0x55,
        '.' ^ 0x55,
        'm' ^ 0x55,
        'e' ^ 0x55,
        'd' ^ 0x55,
        'i' ^ 0x55,
        'a' ^ 0x55,
        'c' ^ 0x55,
        'l' ^ 0x55,
        'i' ^ 0x55,
        'e' ^ 0x55,
        'n' ^ 0x55,
        't' ^ 0x55,
    };
    private static final char[] label_4 = { /* org.zeroxlab.zeroxbenchmark */
        'o' ^ 0x55,
        'r' ^ 0x55,
        'g' ^ 0x55,
        '.' ^ 0x55,
        'z' ^ 0x55,
        'e' ^ 0x55,
        'r' ^ 0x55,
        'o' ^ 0x55,
        'x' ^ 0x55,
        'l' ^ 0x55,
        'a' ^ 0x55,
        'b' ^ 0x55,
        '.' ^ 0x55,
        'z' ^ 0x55,
        'e' ^ 0x55,
        'r' ^ 0x55,
        'o' ^ 0x55,
        'x' ^ 0x55,
        'b' ^ 0x55,
        'e' ^ 0x55,
        'n' ^ 0x55,
        'c' ^ 0x55,
        'h' ^ 0x55,
        'm' ^ 0x55,
        'a' ^ 0x55,
        'r' ^ 0x55,
        'k' ^ 0x55,
    };
    private static final char[] label_5 = { /* org.zeroxlab.benchmark */
        'o' ^ 0x55,
        'r' ^ 0x55,
        'g' ^ 0x55,
        '.' ^ 0x55,
        'z' ^ 0x55,
        'e' ^ 0x55,
        'r' ^ 0x55,
        'o' ^ 0x55,
        'x' ^ 0x55,
        'l' ^ 0x55,
        'a' ^ 0x55,
        'b' ^ 0x55,
        '.' ^ 0x55,
        'b' ^ 0x55,
        'e' ^ 0x55,
        'n' ^ 0x55,
        'c' ^ 0x55,
        'h' ^ 0x55,
        'm' ^ 0x55,
        'a' ^ 0x55,
        'r' ^ 0x55,
        'k' ^ 0x55,
    };
    private static final char[] label_6 = { /* com.rightware.tdmm2v10jni */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        'r' ^ 0x55,
        'i' ^ 0x55,
        'g' ^ 0x55,
        'h' ^ 0x55,
        't' ^ 0x55,
        'w' ^ 0x55,
        'a' ^ 0x55,
        'r' ^ 0x55,
        'e' ^ 0x55,
        '.' ^ 0x55,
        't' ^ 0x55,
        'd' ^ 0x55,
        'm' ^ 0x55,
        'm' ^ 0x55,
        '2' ^ 0x55,
        'v' ^ 0x55,
        '1' ^ 0x55,
        '0' ^ 0x55,
        'j' ^ 0x55,
        'n' ^ 0x55,
        'i' ^ 0x55,
    };
    private static final char[] label_7 = { /* com.nvidia.linpack */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        'n' ^ 0x55,
        'v' ^ 0x55,
        'i' ^ 0x55,
        'd' ^ 0x55,
        'i' ^ 0x55,
        'a' ^ 0x55,
        '.' ^ 0x55,
        'l' ^ 0x55,
        'i' ^ 0x55,
        'n' ^ 0x55,
        'p' ^ 0x55,
        'a' ^ 0x55,
        'c' ^ 0x55,
        'k' ^ 0x55,
    };
    private static final char[] label_8 = { /* se.nena.nenamark2 */
        's' ^ 0x55,
        'e' ^ 0x55,
        '.' ^ 0x55,
        'n' ^ 0x55,
        'e' ^ 0x55,
        'n' ^ 0x55,
        'a' ^ 0x55,
        '.' ^ 0x55,
        'n' ^ 0x55,
        'e' ^ 0x55,
        'n' ^ 0x55,
        'a' ^ 0x55,
        'm' ^ 0x55,
        'a' ^ 0x55,
        'r' ^ 0x55,
        'k' ^ 0x55,
        '2' ^ 0x55,
    };
    private static final char[] label_9 = { /* com.qualcomm.qx.neocore */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        'q' ^ 0x55,
        'u' ^ 0x55,
        'a' ^ 0x55,
        'l' ^ 0x55,
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        'q' ^ 0x55,
        'x' ^ 0x55,
        '.' ^ 0x55,
        'n' ^ 0x55,
        'e' ^ 0x55,
        'o' ^ 0x55,
        'c' ^ 0x55,
        'o' ^ 0x55,
        'r' ^ 0x55,
        'e' ^ 0x55,
    };
    private static final char[] label_10 = { /* com.nvidia.devtech.fm20 */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        'n' ^ 0x55,
        'v' ^ 0x55,
        'i' ^ 0x55,
        'd' ^ 0x55,
        'i' ^ 0x55,
        'a' ^ 0x55,
        '.' ^ 0x55,
        'd' ^ 0x55,
        'e' ^ 0x55,
        'v' ^ 0x55,
        't' ^ 0x55,
        'e' ^ 0x55,
        'c' ^ 0x55,
        'h' ^ 0x55,
        '.' ^ 0x55,
        'f' ^ 0x55,
        'm' ^ 0x55,
        '2' ^ 0x55,
        '0' ^ 0x55,
    };
    private static final char[] label_11 = { /* com.threed.jpct.bench */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        't' ^ 0x55,
        'h' ^ 0x55,
        'r' ^ 0x55,
        'e' ^ 0x55,
        'e' ^ 0x55,
        'd' ^ 0x55,
        '.' ^ 0x55,
        'j' ^ 0x55,
        'p' ^ 0x55,
        'c' ^ 0x55,
        't' ^ 0x55,
        '.' ^ 0x55,
        'b' ^ 0x55,
        'e' ^ 0x55,
        'n' ^ 0x55,
        'c' ^ 0x55,
        'h' ^ 0x55,
    };
    private static final char[] label_12 = { /* com.tactel.electopia */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        't' ^ 0x55,
        'a' ^ 0x55,
        'c' ^ 0x55,
        't' ^ 0x55,
        'e' ^ 0x55,
        'l' ^ 0x55,
        '.' ^ 0x55,
        'e' ^ 0x55,
        'l' ^ 0x55,
        'e' ^ 0x55,
        'c' ^ 0x55,
        't' ^ 0x55,
        'o' ^ 0x55,
        'p' ^ 0x55,
        'i' ^ 0x55,
        'a' ^ 0x55,
    };
    private static final char[] label_13 = { /* com.glbenchmark.glbenchmark20 */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        'g' ^ 0x55,
        'l' ^ 0x55,
        'b' ^ 0x55,
        'e' ^ 0x55,
        'n' ^ 0x55,
        'c' ^ 0x55,
        'h' ^ 0x55,
        'm' ^ 0x55,
        'a' ^ 0x55,
        'r' ^ 0x55,
        'k' ^ 0x55,
        '.' ^ 0x55,
        'g' ^ 0x55,
        'l' ^ 0x55,
        'b' ^ 0x55,
        'e' ^ 0x55,
        'n' ^ 0x55,
        'c' ^ 0x55,
        'h' ^ 0x55,
        'm' ^ 0x55,
        'a' ^ 0x55,
        'r' ^ 0x55,
        'k' ^ 0x55,
        '2' ^ 0x55,
        '0' ^ 0x55,
    };
    private static final char[] label_14 = { /* com.glbenchmark.glbenchmark21 */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        'g' ^ 0x55,
        'l' ^ 0x55,
        'b' ^ 0x55,
        'e' ^ 0x55,
        'n' ^ 0x55,
        'c' ^ 0x55,
        'h' ^ 0x55,
        'm' ^ 0x55,
        'a' ^ 0x55,
        'r' ^ 0x55,
        'k' ^ 0x55,
        '.' ^ 0x55,
        'g' ^ 0x55,
        'l' ^ 0x55,
        'b' ^ 0x55,
        'e' ^ 0x55,
        'n' ^ 0x55,
        'c' ^ 0x55,
        'h' ^ 0x55,
        'm' ^ 0x55,
        'a' ^ 0x55,
        'r' ^ 0x55,
        'k' ^ 0x55,
        '2' ^ 0x55,
        '1' ^ 0x55,
    };
    private static final char[] label_15 = { /* com.allego.windmill */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        'a' ^ 0x55,
        'l' ^ 0x55,
        'l' ^ 0x55,
        'e' ^ 0x55,
        'g' ^ 0x55,
        'o' ^ 0x55,
        '.' ^ 0x55,
        'w' ^ 0x55,
        'i' ^ 0x55,
        'n' ^ 0x55,
        'd' ^ 0x55,
        'm' ^ 0x55,
        'i' ^ 0x55,
        'l' ^ 0x55,
        'l' ^ 0x55,
    };
    private static final char[] label_16 = { /* com.edburnette.fps2d */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        'e' ^ 0x55,
        'd' ^ 0x55,
        'b' ^ 0x55,
        'u' ^ 0x55,
        'r' ^ 0x55,
        'n' ^ 0x55,
        'e' ^ 0x55,
        't' ^ 0x55,
        't' ^ 0x55,
        'e' ^ 0x55,
        '.' ^ 0x55,
        'f' ^ 0x55,
        'p' ^ 0x55,
        's' ^ 0x55,
        '2' ^ 0x55,
        'd' ^ 0x55,
    };
    private static final char[] label_17 = { /* softweg.hw.performance */
        's' ^ 0x55,
        'o' ^ 0x55,
        'f' ^ 0x55,
        't' ^ 0x55,
        'w' ^ 0x55,
        'e' ^ 0x55,
        'g' ^ 0x55,
        '.' ^ 0x55,
        'h' ^ 0x55,
        'w' ^ 0x55,
        '.' ^ 0x55,
        'p' ^ 0x55,
        'e' ^ 0x55,
        'r' ^ 0x55,
        'f' ^ 0x55,
        'o' ^ 0x55,
        'r' ^ 0x55,
        'm' ^ 0x55,
        'a' ^ 0x55,
        'n' ^ 0x55,
        'c' ^ 0x55,
        'e' ^ 0x55,
    };
    private static final char[] label_18 = { /* com.smartbench.twelve */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        's' ^ 0x55,
        'm' ^ 0x55,
        'a' ^ 0x55,
        'r' ^ 0x55,
        't' ^ 0x55,
        'b' ^ 0x55,
        'e' ^ 0x55,
        'n' ^ 0x55,
        'c' ^ 0x55,
        'h' ^ 0x55,
        '.' ^ 0x55,
        't' ^ 0x55,
        'w' ^ 0x55,
        'e' ^ 0x55,
        'l' ^ 0x55,
        'v' ^ 0x55,
        'e' ^ 0x55,
    };
    private static final char[] label_19 = { /* com.antutu.ABenchMark */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        'a' ^ 0x55,
        'n' ^ 0x55,
        't' ^ 0x55,
        'u' ^ 0x55,
        't' ^ 0x55,
        'u' ^ 0x55,
        '.' ^ 0x55,
        'A' ^ 0x55,
        'B' ^ 0x55,
        'e' ^ 0x55,
        'n' ^ 0x55,
        'c' ^ 0x55,
        'h' ^ 0x55,
        'M' ^ 0x55,
        'a' ^ 0x55,
        'r' ^ 0x55,
        'k' ^ 0x55,
    };
    private static final char[] label_20 = { /* com.glbenchmark.glbenchmark25 */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        'g' ^ 0x55,
        'l' ^ 0x55,
        'b' ^ 0x55,
        'e' ^ 0x55,
        'n' ^ 0x55,
        'c' ^ 0x55,
        'h' ^ 0x55,
        'm' ^ 0x55,
        'a' ^ 0x55,
        'r' ^ 0x55,
        'k' ^ 0x55,
        '.' ^ 0x55,
        'g' ^ 0x55,
        'l' ^ 0x55,
        'b' ^ 0x55,
        'e' ^ 0x55,
        'n' ^ 0x55,
        'c' ^ 0x55,
        'h' ^ 0x55,
        'm' ^ 0x55,
        'a' ^ 0x55,
        'r' ^ 0x55,
        'k' ^ 0x55,
        '2' ^ 0x55,
        '5' ^ 0x55,
    };
    private static final char[] label_21 = { /* com.rightware.tdmm2v10jnifree */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        'r' ^ 0x55,
        'i' ^ 0x55,
        'g' ^ 0x55,
        'h' ^ 0x55,
        't' ^ 0x55,
        'w' ^ 0x55,
        'a' ^ 0x55,
        'r' ^ 0x55,
        'e' ^ 0x55,
        '.' ^ 0x55,
        't' ^ 0x55,
        'd' ^ 0x55,
        'm' ^ 0x55,
        'm' ^ 0x55,
        '2' ^ 0x55,
        'v' ^ 0x55,
        '1' ^ 0x55,
        '0' ^ 0x55,
        'j' ^ 0x55,
        'n' ^ 0x55,
        'i' ^ 0x55,
        'f' ^ 0x55,
        'r' ^ 0x55,
        'e' ^ 0x55,
        'e' ^ 0x55,
    };
    private static final char[] label_22 = { /* com.rightware.uimarkes1 */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        'r' ^ 0x55,
        'i' ^ 0x55,
        'g' ^ 0x55,
        'h' ^ 0x55,
        't' ^ 0x55,
        'w' ^ 0x55,
        'a' ^ 0x55,
        'r' ^ 0x55,
        'e' ^ 0x55,
        '.' ^ 0x55,
        'u' ^ 0x55,
        'i' ^ 0x55,
        'm' ^ 0x55,
        'a' ^ 0x55,
        'r' ^ 0x55,
        'k' ^ 0x55,
        'e' ^ 0x55,
        's' ^ 0x55,
        '1' ^ 0x55,
    };
    private static final char[] label_23 = { /* com.rightware.basemarkgui */
        'c' ^ 0x55,
        'o' ^ 0x55,
        'm' ^ 0x55,
        '.' ^ 0x55,
        'r' ^ 0x55,
        'i' ^ 0x55,
        'g' ^ 0x55,
        'h' ^ 0x55,
        't' ^ 0x55,
        'w' ^ 0x55,
        'a' ^ 0x55,
        'r' ^ 0x55,
        'e' ^ 0x55,
        '.' ^ 0x55,
        'b' ^ 0x55,
        'a' ^ 0x55,
        's' ^ 0x55,
        'e' ^ 0x55,
        'm' ^ 0x55,
        'a' ^ 0x55,
        'r' ^ 0x55,
        'k' ^ 0x55,
        'g' ^ 0x55,
        'u' ^ 0x55,
        'i' ^ 0x55,
    };

    private static class App {
        public final String packageName;
        public final boolean forceHwUi;
        public final int scalingMinFreq;
        public final boolean disableCpuCoreBias;
        public final boolean disableGpuScaling;

        App(String _packageName, boolean _forceHwUi, int _scalingMinFreq,
            boolean _disableCpuCoreBias, boolean _disableGpuScaling)  {
            packageName = _packageName;
            forceHwUi = _forceHwUi;
            scalingMinFreq = _scalingMinFreq;
            disableCpuCoreBias = _disableCpuCoreBias;
            disableGpuScaling = _disableGpuScaling;
        }

        App(char[] _packageName, boolean _forceHwUi, int _scalingMinFreq,
            boolean _disableCpuCoreBias, boolean _disableGpuScaling)  {
            String p = "";
            int i;

            for (i = 0; i < _packageName.length; i++)
                p += (char) (_packageName[i] ^ 0x55);

            packageName = p;
            forceHwUi = _forceHwUi;
            scalingMinFreq = _scalingMinFreq;
            disableCpuCoreBias = _disableCpuCoreBias;
            disableGpuScaling = _disableGpuScaling;
        }

        App(String _packageName, boolean _forceHwUi, int _scalingMinFreq,
            boolean _disableCpuCoreBias)  {
            this(_packageName, _forceHwUi, scalingMinFreqDefault,
                 _disableCpuCoreBias, disableGpuScalingDefault);
        }

        App(String _packageName, boolean _forceHwUi, int _scalingMinFreq) {
            this(_packageName, _forceHwUi, scalingMinFreqDefault,
                 disableCpuCoreBiasDefault, disableGpuScalingDefault);
        }

        App(String _packageName, boolean _forceHwUi) {
            this(_packageName, _forceHwUi, scalingMinFreqDefault,
                 disableCpuCoreBiasDefault, disableGpuScalingDefault);
        }
    }


    private static final App[] mAppList = {
        new App(label_0, /* com.aurorasoftworks.quadrant.ui.professional */
                true  /* force HW UI */, 1200000,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_1, /* com.aurorasoftworks.quadrant.ui.advanced */
                true  /* force HW UI */, 1200000,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_2, /* com.aurorasoftworks.quadrant.ui.standard */
                true  /* force HW UI */, 1200000,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_3, /* com.netflix.mediaclient */
                true  /* force HW UI */, scalingMinFreqDefault,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_4, /* org.zeroxlab.zeroxbenchmark */
                true  /* force HW UI */, 1200000,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_5, /* org.zeroxlab.benchmark */
                true  /* force HW UI */, 1200000,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_6, /* com.rightware.tdmm2v10jni */ /*Basemark 2.0*/
                false  /* force HW UI */,
                1200000 /* scalingMinFreq */,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_7, /* com.nvidia.linpack */
                false  /* force HW UI */,
                1200000 /* scalingMinFreq */,
                true   /* disableCpuCoreBias */,
                true   /* disable gpu scaling */),
        new App(label_8, /* se.nena.nenamark2 */
                false  /* don't force HW UI */, scalingMinFreqDefault,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_9, /* com.qualcomm.qx.neocore */
                false  /* don't force HW UI */, scalingMinFreqDefault,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_10, /* com.nvidia.devtech.fm20 */
                false  /* don't force HW UI */, scalingMinFreqDefault,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_11, /* com.threed.jpct.bench */
                false  /* don't force HW UI */, scalingMinFreqDefault,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_12, /* com.tactel.electopia */
                false  /* don't force HW UI */, scalingMinFreqDefault,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_13, /* com.glbenchmark.glbenchmark20 */
                false  /* don't force HW UI */, 1200000,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_14, /* com.glbenchmark.glbenchmark21 */
                false  /* don't force HW UI */, 1200000,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_15, /* com.allego.windmill */
                false  /* don't force HW UI */, 1200000,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_16, /* com.edburnette.fps2d */
                false  /* don't force HW UI */, scalingMinFreqDefault,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_17, /* softweg.hw.performance */
                false  /* don't force HW UI */, 1200000,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_18, /* com.smartbench.twelve */
                true  /* force HW UI */, 1200000,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_19, /* com.antutu.ABenchMark */
                false  /* don't force HW UI */, 1200000,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_20, /* com.glbenchmark.glbenchmark25 */
                false  /* don't force HW UI */, 1200000,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_21, /* com.rightware.tdmm2v10jnifree */ /* Taiji Free */
                false  /* force HW UI */, 1200000 /* scalingMinFreq */,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_22, /* com.rightware.uimarkes1 */ /* Basemark GUI */
                false  /* force HW UI */, 1200000 /* scalingMinFreq */,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
        new App(label_23, /* com.rightware.basemarkgui */ /* Basemark GUI Free */
                false  /* force HW UI */, 1200000 /* scalingMinFreq */,
                disableCpuCoreBiasDefault, true /* disable gpu scaling */),
    };

    private static String getPackageName(String appName) {
        int index = appName.indexOf('/');
        if (index < 0) {
            Log.e(TAG, "appName does not contain '/'. the packageName cannot be extracted from appName!");
            return null;
        }
        return appName.substring(0, index);
    }

    public static boolean canForceHwUi(final String appName) {
        if (appName == null) {
            return false;
        }

        final String packageName = getPackageName(appName);

        if (packageName == null) {
            return false;
        }

        if (DEBUG_NV_APP_PROFILE) {
            Log.d(TAG, "appName = " + appName);
            Log.d(TAG, "packageName = " + packageName);
        }

        for (int i = 0; i < mAppList.length; i++) {
            if (!mAppList[i].forceHwUi)
                continue;

            if (mAppList[i].packageName.equals(packageName)) {
                if (DEBUG_NV_APP_PROFILE) {
                    Log.d(TAG, "found matched app. forcing HW accelerated UI");
                }
                return true;
            }
        }

        return false;
    }

    public static int getAppSpecificScalingMinFreq(final String packageName) {
        int freq = 0;

        if (packageName == null) {
            return freq;
        }

        if (DEBUG_NV_APP_PROFILE) {
            Log.d(TAG, "packageName = " + packageName);
        }

        for (int i = 0; i < mAppList.length; i++) {
            if (mAppList[i].scalingMinFreq <= 0)
                continue;

            if (mAppList[i].packageName.equals(packageName)) {
                freq = mAppList[i].scalingMinFreq;
                if (DEBUG_NV_APP_PROFILE) {
                    Log.d(TAG, "found matched app." +
                          "returning scalingMinFreq as " + freq);
                }
                return freq;
            }
        }

        return freq;
    }

    public static boolean getAppSpecificDisableCpuCoreBias(
        final String packageName) {
        boolean disable = false;

        if (packageName == null) {
            return disable;
        }

        if (DEBUG_NV_APP_PROFILE) {
            Log.d(TAG, "packageName = " + packageName);
        }

        for (int i = 0; i < mAppList.length; i++) {
            if (mAppList[i].packageName.equals(packageName)) {
                disable = mAppList[i].disableCpuCoreBias;
                if (DEBUG_NV_APP_PROFILE) {
                    Log.d(TAG, "found matched app." +
                          "returning disableCoreBias as " + disable);
                }
                return disable;
            }
        }

        return disable;
    }

    public static boolean getAppSpecificDisableGpuScaling(
        final String packageName) {
        boolean disable = false;

        if (packageName == null) {
            return disable;
        }

        if (DEBUG_NV_APP_PROFILE) {
            Log.d(TAG, "packageName = " + packageName);
        }

        for (int i = 0; i < mAppList.length; i++) {
            if (mAppList[i].packageName.equals(packageName)) {
                disable = mAppList[i].disableGpuScaling;
                if (DEBUG_NV_APP_PROFILE) {
                    Log.d(TAG, "found matched app." +
                          "returning disableGpuScaling as " + disable);
                }
                return disable;
            }
        }

        return disable;
    }
}
