/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
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
#define LOG_TAG "NvCpuClient-JNI"
#include <utils/Log.h>
#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"
#include <nvcpud/NvCpuClient.h>
using namespace android;

static jfieldID mNativeNvCpuClient;

static NvCpuClient* getClientPtr(JNIEnv* env, jobject thiz)
{
    return (NvCpuClient*)env->GetIntField(thiz,
            mNativeNvCpuClient);
}

static void setClientPtr(JNIEnv* env,
        jobject thiz, NvCpuClient* ptr)
{
    env->SetIntField(thiz, mNativeNvCpuClient, (int)ptr);
}

#define JNI_TIMEOUT_DECL(func) \
static void com_nvidia_NvCpuClient_##func(JNIEnv *env, \
        jobject thiz, \
        jint arg, \
        jlong timeoutNs, \
        jlong nowNanoTime) \
{ \
    NvCpuClient* ptr = getClientPtr(env, thiz); \
    if (ptr) { \
        ptr->func((int)arg, timeoutNs, nowNanoTime); \
    } \
}

#define JNI_HANDLE_DECL(func) \
static jobject com_nvidia_NvCpuClient_##func##Handle(JNIEnv *env, \
        jobject thiz, \
        jint numCpus, \
        jlong nowNanoTime) \
{ \
    NvCpuClient* ptr = getClientPtr(env, thiz); \
    if (ptr) { \
        int fd = ptr->func##Handle((int)numCpus, nowNanoTime); \
        if (fd >= 0) { \
            return jniCreateFileDescriptor(env, fd); \
        } \
    } \
    return NULL; \
}

#define JNI_SET_DECL(func) \
static void com_nvidia_NvCpuClient_set##func(JNIEnv *env, \
        jobject thiz, \
        jint arg) \
{ \
    NvCpuClient *ptr = getClientPtr(env, thiz); \
    if (ptr) { \
        ptr->set##func((int)arg); \
    } \
}

#define JNI_GET_DECL(func) \
static jint com_nvidia_NvCpuClient_get##func(JNIEnv *env, \
        jobject thiz) \
{ \
    NvCpuClient *ptr = getClientPtr(env, thiz); \
    if (ptr) { \
      return (jint) ptr->get##func(); \
    } \
    return -1; \
}

NVCPU_FUNC_LIST(JNI_TIMEOUT_DECL)
NVCPU_FUNC_LIST(JNI_HANDLE_DECL)
NVCPU_AP_FUNC_LIST(JNI_GET_DECL)
NVCPU_AP_FUNC_LIST(JNI_SET_DECL)

#undef JNI_TIMEOUT_DECL
#undef JNI_HANDLE_DECL
#undef JNI_SET_DECL
#undef JNI_GET_DECL

static void com_nvidia_NvCpuClient_init(JNIEnv* env, jobject thiz)
{
    setClientPtr(env, thiz, new NvCpuClient());
}

static void com_nvidia_NvCpuClient_release(JNIEnv* env, jobject thiz)
{
    NvCpuClient* ptr = getClientPtr(env, thiz);
    setClientPtr(env, thiz, NULL);
    if (ptr) {
        delete ptr;
    }
}

static void nativeClassInit(JNIEnv* env, jclass clazz);

#define JNI_TIMEOUT_REG(func) \
    { #func, "(IJJ)V", (void*)com_nvidia_NvCpuClient_##func },
#define JNI_HANDLE_REG(func) \
    { #func "Handle", "(IJ)Ljava/io/FileDescriptor;", (void*)com_nvidia_NvCpuClient_##func##Handle },

#define JNI_SET_REG(func) \
    { "set" #func, "(I)V", (void *)com_nvidia_NvCpuClient_set##func },
#define JNI_GET_REG(func) \
    { "get" #func, "()I", (void *)com_nvidia_NvCpuClient_get##func },

static JNINativeMethod NvCpuClientMethods[] = {
  NVCPU_FUNC_LIST(JNI_TIMEOUT_REG)
  NVCPU_FUNC_LIST(JNI_HANDLE_REG)
  NVCPU_AP_FUNC_LIST(JNI_SET_REG)
  NVCPU_AP_FUNC_LIST(JNI_GET_REG)
  { "nativeClassInit", "()V", (void*)nativeClassInit},
  { "init", "()V", (void*)com_nvidia_NvCpuClient_init},
  { "release", "()V", (void*)com_nvidia_NvCpuClient_release},
};

#undef JNI_HANDLE_REG
#undef JNI_TIMEOUT_REG
#undef JNI_GET_REG
#undef JNI_SET_REG

static void nativeClassInit(JNIEnv* env, jclass clazz)
{
    mNativeNvCpuClient = env->GetFieldID(
            clazz, "mNativeNvCpuClient", "I");
}

int register_com_nvidia_NvCpuClient(JNIEnv* env)
{
    return AndroidRuntime::registerNativeMethods(env,
            "com/nvidia/NvCpuClient", NvCpuClientMethods, NELEM(NvCpuClientMethods));
}
