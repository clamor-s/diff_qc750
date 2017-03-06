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
 */

/**
 * @file
 * <b>NVIDIA Tegra Android CPU management Constants</b>
 *
 * @b Description: Exposes the Constant Values used in
 *    management of Tegra cores to speed up
 *    computation or save power when necessary
 */

package com.nvidia;

/**
 * @hide
 *
 */
public interface NvCpuClientConstants {

    //IMPORTANT! Keep in sync with frameworks/base/include/nvcpud/INvCpuService::NvCpuBoostStrength
    public static final int NVCPU_BOOST_STRENGTH_LOWEST = 0;
    public static final int NVCPU_BOOST_STRENGTH_LOW = 1;
    public static final int NVCPU_BOOST_STRENGTH_MEDIUM_LOW = 2;
    public static final int NVCPU_BOOST_STRENGTH_MEDIUM = 3;
    public static final int NVCPU_BOOST_STRENGTH_MEDIUM_HIGH = 4;
    public static final int NVCPU_BOOST_STRENGTH_HIGH = 5;
    public static final int NVCPU_BOOST_STRENGTH_HIGHEST = 6;
    public static final int NVCPU_BOOST_STRENGTH_COUNT = 7;

    public static final int NVCPU_CORE_BIAS_DEFAULT = 0;
    public static final int NVCPU_GPU_CBUS_CAP_LEVEL_DEFAULT = 437000000;
    public static final int NVCPU_CPU_OVERCLOCK_MIN_DELAY_DEFAULT = 150000;
    public static final int NVCPU_CPU_SCALING_MIN_FREQ_DEFAULT = 0;
}
