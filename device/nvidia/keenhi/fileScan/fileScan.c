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
#include <dirent.h> 
#include <sys/types.h>
#define DIRECTORY_PATH "/forever/info/"
#define PROP_PREFIX "ro.keenhi."
void fileScan(char* path, char *prop_string){

	DIR *directory_pointer;
	struct dirent *entry;
	char buffer[4096];
	char path_buff[100];
	char prop_buff[100];
	char prop[100];
	int dev_fd;
	int length;
	if((directory_pointer=opendir(path)) == NULL)
		LOGE("can not open dir %s,please check the directory exist",path);
	 else
    	{
		while((entry=readdir(directory_pointer))!=NULL)
		{
		  // LOGE("the directory name is %s\n",entry-> d_name);
		   if((strcmp(".",entry->d_name) == 0) || (strcmp("..",entry->d_name) == 0))
			continue;
		   strcpy(path_buff,path);
		   strcat(path_buff,entry-> d_name);
		   dev_fd = open(path_buff,O_RDONLY);
		   LOGV("dev_fd=%d",dev_fd);
	
		   if(dev_fd<0)
			continue;
		   length = read(dev_fd,buffer,sizeof(buffer));
		   if(length < 0){
			close(dev_fd);
			continue;
		  }	
		   strcpy(prop_buff,prop_string);
		   strcat(prop_buff,entry-> d_name);
		  // LOGE("region length=%d,%s\n",length,buffer);
		   strncpy(prop,buffer,length);
		   property_set(prop_buff,prop);
		   memset(prop,0,sizeof(prop));
		  // LOGE("region length2=%d,%s\n",length,prop_buff);	
	 	   close(dev_fd);
		}
		closedir(directory_pointer);
    	}
	return 0;
}
static int usage()
{
    fprintf(stderr, "Usage: fileScan [OPTION] <PATH> <PROP>\n");
    fprintf(stderr, "  --help                  display this help and exit\n");

    return 10;
}

int main(int argc, char *argv[]) {

	 int i;

    if (argc < 3 || strcmp(argv[1], "--help") == 0) {
        return usage();
    }
   
    fileScan(argv[1], argv[2]);

    return 0;
}

