/*
 * Copyright (C) 2010 The Android Open Source Project
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

/* this implements a GPS hardware library for the Android emulator.
 * the following code should be built as a shared library that will be
 * placed into /system/lib/hw/gps.goldfish.so
 *
 * it will be loaded by the code in hardware/libhardware/hardware.c
 * which is itself called from android_location_GpsLocationProvider.cpp
 */
 
#define LOG_TAG "PMS"

#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <math.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cutils/properties.h>
#include <cutils/sockets.h>

#include <utils/Log.h>
#include <utils/threads.h>

#define SERIALNO_PATH "/forever/system/serialno.bin"

void repaceString(char* buffer, int size){
	int i;
	for(i=0;i<size;i++){
		char c =buffer[i];
		LOGD("check %d",c);
		if((!(c>='0' && c<='9'))
                && (!(c>='a' && c<='z'))
                    && (!(c>='A' && c<='Z'))){
           LOGD("found one");
			if(c==0){
				return;
			}
			else if(c=='\n')
				c=0;
			else {
				c='Z';
			}
			buffer[i]=c;
		}
	}
}

int main(int argc, char *argv[]) {
	char buffer[33];

	 int sn_fd = open(SERIALNO_PATH, O_RDONLY);

	if(sn_fd>0){
		int length = read(sn_fd, buffer, sizeof(buffer)-1);
		if(length<0){
			close(sn_fd);
			goto FAIL;
		}
		buffer[length]=0;
		repaceString(buffer,32);
		LOGD("update serial to %s\n",buffer);
		property_set("ro.serialno", buffer);
		close(sn_fd);
		goto OUT;
	}

FAIL:
 	LOGD("update serial from cpuid");
	char value[PROPERTY_VALUE_MAX];
 	property_get("ro.cpuid",value,"unknow");
    property_set("ro.serialno", value);

OUT:
	close(open("/dev/.serial_done", O_WRONLY | O_CREAT, 0000));
	return 0;

}

