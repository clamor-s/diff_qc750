/*
 * Copyright (C) 2008 The Android Open Source Project
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

//#define LOG_TAG "nvtflash native.cpp"
//#include <utils/Log.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/stat.h>
#include <linux/delay.h>
#include "jni.h"
#include <utils/Log.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <dlfcn.h>
//#define JNI_DEBUG  
  
#ifdef JNI_DEBUG    
#ifndef LOG_TAG  
#define LOG_TAG "JNI_DEBUG"  
#endif  
  
#include <android/log.h>  

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)  
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)  
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)  
#endif


int register_natives(JNIEnv* env);
int fdev;
unsigned char dev_name[20];
#define DEV_NODE "/sys/devices/platform/gpio_switch.0/switches"

static jint set_status(JNIEnv *env, jobject thiz, jint status)
{
	char buf[2];
	fdev = open(DEV_NODE, O_RDWR);
	if(fdev<0)
	{	
		LOGE("Unable to open otg_switch device");
		return -1;
	}
	LOGE("write status to switch status = %d\n",status);
	sprintf(buf, "%d", status);
	if(write(fdev,buf,sizeof(buf)+1)<0)
		return -4;
	close(fdev);
	
	return status;
	
}
/*
static jint Open(JNIEnv *env, jobject thiz)
{
	int ret;

	fdev = open(DEV_NODE, O_RDWR);

	if(fdev < 0){
		LOG_FATAL_IF(fdev <0, "Unable to open otg_switch device");
		return NULL;
	}

	return 1;
}

static jint Close(JNIEnv *env, jobject thiz)
{

	if(fdev >= 0){
		close(fdev);
		fdev = -1;
	}

	return 1;
}

*/

static JNINativeMethod sMethods[] = {
//	{ "Open", "()I", (void *)Open },
//	{ "Close", "()I", (void *)Close },
	{"set_status", "(I)I", (void *)set_status },
	
};

int register_natives(JNIEnv* env)
{
	static const char *kClassName = "com/android/settings/DevelopmentSettings";
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

