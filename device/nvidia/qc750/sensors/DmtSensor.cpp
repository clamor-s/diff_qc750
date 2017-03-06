/*
 * Copyright (C) 2011 The Android Open Source Project
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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>

#include <cutils/log.h>

#include "DmtSensor.h"

#ifndef ID_M
#define ID_M 100
#endif
#ifndef ID_O
#define ID_O 101
#endif

/*****************************************************************************/
#define DMT_ENABLE_BITMASK_A 0x01 /* logical driver for accelerometer */
#define DMT_ENABLE_BITMASK_M 0x02 /* logical driver for magnetic field sensor */
#define DMT_ENABLE_BITMASK_O 0x04 /* logical driver for orientation sensor */
/*****************************************************************************/
#define CONVERT_A_D	(GRAVITY_EARTH)/32.0f
/*****************************************************************************/
sensors_event_t mPendingEvents[3];//3 for acc,mag,ori sensors
static unsigned int  gsensor_delay_time = 100000; // 100 ms
DmtSensor::DmtSensor()
: SensorBase(NULL, "dmard06"),
      mEnabled(0),
      mInputReader(8),
      mHasPendingEvent(false)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = 0;
    mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
    mPendingEvent.acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));

    if (data_fd) {
        strcpy(input_sysfs_path, "/sys/class/input/");
        strcat(input_sysfs_path, input_name);
        strcat(input_sysfs_path, "/device/");
        input_sysfs_path_len = strlen(input_sysfs_path);
    }

    for (int i=0 ; i<3 ; ++i)
        mDelay[i] = 200000000; // 200 ms by default
}

DmtSensor::~DmtSensor() {
   
}

static int ProcessEvent(int code,int value)
{
	switch (code)
	{
		case ABS_X:
			mPendingEvents[0].acceleration.x=value*(CONVERT_A_D);
			break;
		case ABS_Y:
			mPendingEvents[0].acceleration.y=-value*(CONVERT_A_D);
			break;
		case ABS_Z:
			mPendingEvents[0].acceleration.z=-value*(CONVERT_A_D);
			break;
	}
	return 0;
}

int write_sys_attribute(const char *path, const char *value, int bytes)
{
    int ifd, amt;

	ifd = open(path, O_WRONLY);
    if (ifd < 0) {
        LOGE("Write_attr failed to open %s (%s)",path, strerror(errno));
        return -1;
	}

    amt = write(ifd, value, bytes);
	amt = ((amt == -1) ? -errno : 0);
	LOGE_IF(amt < 0, "Write_int failed to write %s (%s)",
		path, strerror(errno));
    close(ifd);
	return amt;
}

int DmtSensor::enable(int32_t handle, int en)
{

	char path[]="/sys/class/accelemeter/dmard06/enable_acc";
	char value[2]={0,0};
	value[0]= en ? '1':'0';
	LOGE("%s:en=%s\n",__func__,value);
	write_sys_attribute(path,value, 1);
	return 0;	
}

int DmtSensor::setDelay(int32_t handle, int64_t ns)
{
  	char value[11]={0};
	int bytes;
	char path[]="/sys/class/accelemeter/dmard06/delay_acc";	
	gsensor_delay_time = ns / 1000;
	
	bytes=sprintf(value,"%u",gsensor_delay_time);
	LOGE("value=%s ,gsensor=%u, bytes=%d\n",value,gsensor_delay_time,bytes);
		
	write_sys_attribute(path,value, bytes);
	return 0;
}

bool DmtSensor::hasPendingEvents() const {
    return mHasPendingEvent;
}
int DmtSensor::readEvents(sensors_event_t* data, int count)
{
	struct input_event event[5];
	uint32_t new_sensors = 0;
	int64_t time=0 ;
	int nread=0;
	int i,ReturntoPoll;
	if(data_fd<0)
	{
	 LOGE("%s: input device is not ready !\n",__func__);
	 return 0;
	}
	ReturntoPoll=0;
   	nread=read(data_fd, &event[0], 5*sizeof(struct input_event));
	nread/=sizeof(struct input_event);
  	if(nread>0)
	{
	 for(i=0;i<nread;i++)
	 {
		switch(event[i].type)
		{
		case EV_ABS:	
			if(event[i].value==65536)
			{
				close(data_fd);
				event[i].value=0;
			}	
			ProcessEvent(event[i].code,event[i].value);
			break;
		case EV_SYN:
      			time= event[0].time.tv_sec * 1000000000LL + event[0].time.tv_usec * 1000;
               		mPendingEvents[0].timestamp = time;	
			*data=mPendingEvents[0];
			ReturntoPoll=1;
			break;		
		}
	 }
	}
	return ReturntoPoll;// then Android will not analysis the data , even data is legal.

}

