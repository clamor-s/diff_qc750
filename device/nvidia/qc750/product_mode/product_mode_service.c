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


#define DEVICE "/dev/block/platform/sdhci-tegra.3/by-name/UDC"

#define PARTITION_MAGIC "part"
#define PRODUCT_MODE_MAGIC "production_mode"

#define GAP ":"

void setProductionMode(int yes){
	if(yes){
		property_set("sys.kh.production_mode", "true");
	}
	else{
		property_set("sys.kh.production_mode", "false");
	}
}

int testIsGap(char* buffer){
	if(!strncmp(buffer, GAP ,strlen(GAP))){
		return 1;
	}
	return 0;
}

int clearProductionMode(){

	LOGV("in clearProductionMode");

	if(checkProductionMode()!=0){
		return -1;
	}

	LOGV("ready to clear the pm");

	char buffer[4096];

	memset(buffer,0,4096);

	int dev_fd = open(DEVICE, O_WRONLY);

	write(dev_fd,buffer,4096);

	close(dev_fd);
	
	return 0;
}

int checkProductionMode(int updateProp){
	char buffer[4096];

	/* start the dumpstate service */
	//property_set("ctl.start", "dumpstate");

	int dev_fd = open(DEVICE, O_RDONLY);

	LOGV("dev_fd=%d",dev_fd);	

	if(dev_fd<0){
		goto FAIL;
	}

	int length = read(dev_fd, buffer, sizeof(buffer));
	LOGV("length=%d",length);

	if (length <= 0 || length !=4096){
		goto FAIL;
	}

	LOGV("buffer=%d,%d,%d,%d",buffer[0],buffer[1],buffer[2],buffer[3]);
	//LOGV("buffer=%s",buffer);

	int tmp =strlen(PARTITION_MAGIC);
	int start =0;
	char* data =&buffer[start];

	LOGV("tmp=%d",tmp);

	if(!strncmp(data, PARTITION_MAGIC ,tmp)){
		start +=tmp +strlen(GAP);

		LOGV("ok start=%d",start);

		data =&buffer[start];

		tmp =strlen(PRODUCT_MODE_MAGIC);
		if(!strncmp(data, PRODUCT_MODE_MAGIC ,tmp)){
			
		}
		else{
			goto FAIL;
		}
	}
	else{
		goto FAIL;
	}

	close(dev_fd);
	if(updateProp)
		setProductionMode(1);
	return 0;
FAIL:
	LOGV("fail");
	if(dev_fd>=0){
		close(dev_fd);
	}
	if(updateProp)
		setProductionMode(0);
	return -1;
}

int main(int argc, char *argv[]) {
	if(argc <=1){
		LOGD("invalid");
		return 0;
	}

	char* what =argv[1];
	LOGV("what = %s",what);

	int len =strlen(what);

	if(!strncmp(what, "clear", len)){
		return clearProductionMode();
	}
	else if(!strncmp(what, "check", len)){
		return checkProductionMode(1);
	}

	return 0;
}

