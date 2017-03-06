/*
 * Copyright (c) 2011 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#define LOG_TAG "NvHDMI3D_JNI"

#include <jni.h>
#include <string.h>
#include <stdio.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <dlfcn.h>

#ifndef HAS_3DV_SUPPORT
#define HAS_3DV_SUPPORT 0
#endif

#ifndef ENABLE_HOST_MODE_MENU
#define ENABLE_HOST_MODE_MENU 0
#endif

jstring getProperty(JNIEnv * env, jobject jobj, jstring propname);
jboolean is3DVSupported();
jboolean isUsbHostModeMenuSupported();
int register_natives(JNIEnv* env);

jstring getProperty(JNIEnv * env, jobject jobj, jstring propname)
{
    jboolean isCopy;
    const char * szPropName = env->GetStringUTFChars(propname, &isCopy);
    char prop_value[PROPERTY_VALUE_MAX];
    jclass cls;
    jmethodID mid;
    bool bSuccess = false;

    LOGD("getProperty(): START, get property %s", szPropName);

    if (property_get(szPropName, prop_value, NULL) > 0)
    {
        LOGD("getProperty(): Could GET the property, %s = %s", szPropName, prop_value);
        bSuccess = true;
    }
    else
    {
        LOGD("getProperty(): Could not GET the property");
        bSuccess = false;
    }

    env->ReleaseStringUTFChars(propname, szPropName);

    LOGD("getProperty(): FINISHED");

    return (bSuccess) ? env->NewStringUTF(prop_value) : env->NewStringUTF("");
}

jboolean is3DVSupported()
{
#if HAS_3DV_SUPPORT
    LOGD("is3DVSupported(): Return JNI_TRUE");
    return JNI_TRUE;
#else
    LOGD("is3DVSupported(): Return JNI_FALSE");
    return JNI_FALSE;
#endif
}

jboolean isUsbHostModeMenuSupported()
{
    // typically enabled for phones and not for tablets
#if ENABLE_HOST_MODE_MENU
    return JNI_TRUE;
#else
    return JNI_FALSE;
#endif
}

static JNINativeMethod sMethods[] = {
     /* name, signature, funcPtr */
    { "getPropertyUsingJNI", "(Ljava/lang/String;)Ljava/lang/String;",  (void*)getProperty
    },
    { "is3DVSupported", "()Z",  (void*)is3DVSupported
    },
    { "isUsbHostModeMenuSupported", "()Z",  (void*)isUsbHostModeMenuSupported
    },
};

int register_natives(JNIEnv* env)
{
    static const char* const kClassName = "com/android/settings/HDMI3DPlayJNIHelper";
    jclass myclass = env->FindClass(kClassName);

    LOGD("Register JNI Methods.\n");

    if (myclass == NULL) {
        LOGD("Cannot find classname %s!", kClassName);
        return -1;
    }

    if (env->RegisterNatives(myclass, sMethods, sizeof(sMethods)/sizeof(sMethods[0])) != JNI_OK) {
        LOGD("Failed registering methods for %s \n", kClassName);
        return -1;
    }

    return 0;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        LOGD("GetEnv Failed!");
        return -1;
    }
    if (env == 0) {
        LOGD("Could not retrieve the env!");
    }

    register_natives(env);

    return JNI_VERSION_1_4;
}
