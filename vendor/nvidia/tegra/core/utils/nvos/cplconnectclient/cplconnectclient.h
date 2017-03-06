/*
 * Copyright (c) 2012, NVIDIA CORPORATION. All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef CPLCONNECTCLIENT_H
#define CPLCONNECTCLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

int   NvCplGetAppProfileSettingInt(const char* exeName, const char* setting);
char* NvCplGetAppProfileSettingString(const char* exeName, const char* setting);
int   NvCplGetAppProfileSetting3DVStruct(const char* exeName, unsigned char* stereoProfs, int len);

#ifdef __cplusplus
}
#endif

#endif
