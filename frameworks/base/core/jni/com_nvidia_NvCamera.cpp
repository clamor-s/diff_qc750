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
 * Copyright (C) 2009 The Android Open Source Project
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "NvCamera-JNI"
#include <utils/Log.h>

#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"

#include <utils/Vector.h>

#include <camera/Camera.h>

#include "common_hardware_Camera.h"

static fields_t fields;

static void com_nvidia_NvCamera_setCustomParameters(JNIEnv *env, jobject thiz, jstring params)
{
    LOGV("setCustomParameters");
    sp<Camera> camera = get_native_camera(env, thiz, NULL);
    if (camera == 0) return;

    const jchar* str = env->GetStringCritical(params, 0);
    String8 params8;
    if (params) {
        params8 = String8(str, env->GetStringLength(params));
        env->ReleaseStringCritical(params, str);
    }
    if (camera->setCustomParameters(params8) != NO_ERROR) {
        jniThrowException(env, "java/lang/RuntimeException", "setParameters failed");
        return;
    }
}

static jstring com_nvidia_NvCamera_getCustomParameters(JNIEnv *env, jobject thiz)
{
    LOGV("getCustomParameters");
    sp<Camera> camera = get_native_camera(env, thiz, NULL);
    if (camera == 0) return 0;

    return env->NewStringUTF(camera->getCustomParameters().string());
}

static void com_nvidia_NvCamera_getNvCameraInfo(JNIEnv *env, jobject thiz,
    jint cameraId, jobject info_obj)
{
    LOGV("%s: Enter", __func__);
    CameraInfoExtended cameraInfoExtended;
    status_t rc = Camera::getCameraInfoExtended(cameraId, &cameraInfoExtended);
    if (rc != NO_ERROR) {
        jniThrowException(env, "java/lang/RuntimeException",
                          "Fail to get extended camera info");
        return;
    }
    env->SetIntField(info_obj, fields.facing, cameraInfoExtended.facing);
    env->SetIntField(info_obj, fields.orientation, cameraInfoExtended.orientation);
    env->SetIntField(info_obj, fields.stereoCaps, cameraInfoExtended.stereoCaps);
    env->SetIntField(info_obj, fields.connection, cameraInfoExtended.connection);
}


static JNINativeMethod NvcamMethods[] = {
  { "native_setCustomParameters",
    "(Ljava/lang/String;)V",
    (void *)com_nvidia_NvCamera_setCustomParameters },
  { "native_getCustomParameters",
    "()Ljava/lang/String;",
    (void *)com_nvidia_NvCamera_getCustomParameters },
  { "getNvCameraInfo",
    "(ILcom/nvidia/NvCamera$NvCameraInfo;)V",
    (void*)com_nvidia_NvCamera_getNvCameraInfo },
};

int register_nvidia_hardware_NvCamera(JNIEnv *env)
{
    field fields_to_find[] = {
        { "com/nvidia/NvCamera", "mNativeContext", "I", &fields.context },
        { "com/nvidia/NvCamera$NvCameraInfo", "facing", "I", &fields.facing },
        { "com/nvidia/NvCamera$NvCameraInfo", "orientation", "I", &fields.orientation },
        { "com/nvidia/NvCamera$NvCameraInfo", "stereoCaps", "I", &fields.stereoCaps },
        { "com/nvidia/NvCamera$NvCameraInfo", "connection", "I", &fields.connection }
    };

    if (find_fields(env, fields_to_find, NELEM(fields_to_find)) < 0)
        return -1;

    jclass clazz = env->FindClass("com/nvidia/NvCamera");
    fields.post_event = env->GetStaticMethodID(clazz, "postEventFromNative",
                                               "(Ljava/lang/Object;IIILjava/lang/Object;)V");
    if (fields.post_event == NULL) {
        LOGE("Can't find com/nvidia/NvCamera.postEventFromNative");
        return -1;
    }


    // Register native functions
    return AndroidRuntime::registerNativeMethods(env, "com/nvidia/NvCamera",
                                              NvcamMethods, NELEM(NvcamMethods));
}
