/*
 * Copyright (c) 2012, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

package com.nvidia.NvCPLSvc;

/**
 * {@hide}
 */
interface INvCPLRemoteService {

    // access for different profile settings - string, int, and bool
    String getAppProfileSettingString(String pkgName, String tag);

    int getAppProfileSettingInt(String pkgName, String tag);

    boolean getAppProfileSettingBoolean(String pkgName, String tag);

    // retrieve older-style 3DV stereo profile settings
    byte[] getAppProfileSetting3DVStruct(String pkgName);
}
