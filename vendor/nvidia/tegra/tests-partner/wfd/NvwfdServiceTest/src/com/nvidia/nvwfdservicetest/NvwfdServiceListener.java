/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

package com.nvidia.nvwfdservicetest;

public interface NvwfdServiceListener {
    void sinksFound(String text);
    void connectDone(boolean status);
    void disconnect();
    void notifyError(String text);
}