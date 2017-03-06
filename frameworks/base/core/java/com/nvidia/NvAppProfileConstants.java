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
 * <b>NVIDIA Tegra Android App Profile management Constants</b>
 *
 * @b Description: Exposes the Constant Values used by App Profile system
 */

package com.nvidia;

/**
 * @hide
 *
 */
public interface NvAppProfileConstants {
    public static final int AP_CPU_CORE_BIAS_DEFAULT = 0;
    public static final int AP_CPU_CORE_BIAS_DISABLED = 3;
    public static final int AP_GPU_CBUS_CAP_LEVEL_DEFAULT = 437000000;
    public static final int AP_CPU_HIGH_FREQ_MIN_DELAY_DEFAULT = 150000;
    public static final int AP_CPU_MAX_NORMAL_FREQ_DEFAULT = 1200000;
    public static final int AP_CPU_SCALING_MIN_FREQ_DEFAULT = 0;
}
