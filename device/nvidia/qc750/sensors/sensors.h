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

#ifndef ANDROID_SENSORS_H
#define ANDROID_SENSORS_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <linux/input.h>

#include <hardware/hardware.h>
#include <hardware/sensors.h>

__BEGIN_DECLS

/*****************************************************************************/

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define ID_A  (0)
#define ID_M  (1)
#define ID_O    (2)
//#define ID_GY (2)
#define ID_L  (3)
#define ID_P  (4)

/*****************************************************************************/

/*
 * The SENSORS Module
 */

/* the GP2A is a binary proximity sensor that triggers around 5 cm on
 * this hardware */
#define PROXIMITY_THRESHOLD_GP2A  5.0f

/*****************************************************************************/

#define EVENT_TYPE_ACCEL_X          REL_Y
#define EVENT_TYPE_ACCEL_Y          REL_X
#define EVENT_TYPE_ACCEL_Z          REL_Z

#define EVENT_TYPE_MAGV_X           REL_DIAL
#define EVENT_TYPE_MAGV_Y           REL_HWHEEL
#define EVENT_TYPE_MAGV_Z           REL_MISC

#define EVENT_TYPE_GYRO_X           REL_RY
#define EVENT_TYPE_GYRO_Y           REL_RX
#define EVENT_TYPE_GYRO_Z           REL_RZ

#define EVENT_TYPE_PROXIMITY        ABS_DISTANCE
#define EVENT_TYPE_LIGHT            ABS_MISC

// conversion of acceleration data to SI units (m/s^2)
#define RANGE_A                     (2*GRAVITY_EARTH)
#define CONVERT_A                   (GRAVITY_EARTH / 1024.0)
#define CONVERT_A_X                 (CONVERT_A)
#define CONVERT_A_Y                 (CONVERT_A)
#define CONVERT_A_Z                 (CONVERT_A)

// conversion of magnetic data to uT units
#define CONVERT_M                   (1.0f/16.0f)
#define CONVERT_M_X                 (CONVERT_M)
#define CONVERT_M_Y                 (CONVERT_M)
#define CONVERT_M_Z                 (CONVERT_M)

// conversion of gyro data to SI units (radian/sec)
#define RANGE_GYRO                  (2000.0f*(float)M_PI/180.0f)
#define CONVERT_GYRO                ((70.0f / 1000.0f) * ((float)M_PI / 180.0f))
#define CONVERT_GYRO_X              (CONVERT_GYRO)
#define CONVERT_GYRO_Y              (CONVERT_GYRO)
#define CONVERT_GYRO_Z              (CONVERT_GYRO)

/*****************************************************************************/

/*
 * The SENSORS Module
 */

/* the CM3602 is a binary proximity sensor that triggers around 9 cm on
 * this hardware */
#define PROXIMITY_THRESHOLD_CM  9.0f

/*****************************************************************************/

#define AKM_DEVICE_NAME     "/dev/akm8973_aot"
#define CM_DEVICE_NAME      "/dev/cm3602"
#define LS_DEVICE_NAME      "/dev/lightsensor"

#define EVENT_TYPE_ACCEL_STATUS_K     ABS_WHEEL

#define EVENT_TYPE_YAW_K              ABS_RX
#define EVENT_TYPE_PITCH_K            ABS_RY
#define EVENT_TYPE_ROLL_K             ABS_RZ
#define EVENT_TYPE_ORIENT_STATUS_K    ABS_RUDDER

/* For AK8975 */
#define EVENT_TYPE_MAGV_X_K           ABS_HAT0X
#define EVENT_TYPE_MAGV_Y_K           ABS_HAT0Y
#define EVENT_TYPE_MAGV_Z_K          ABS_BRAKE
#define EVENT_TYPE_MAGV_STATUS_K      ABS_GAS

#define EVENT_TYPE_PROXIMITY_K        ABS_DISTANCE
#define EVENT_TYPE_LIGHT_K            ABS_MISC


#define EVENT_TYPE_ACCEL_X_K      ABS_X
#define EVENT_TYPE_ACCEL_Y_K      ABS_Y
#define EVENT_TYPE_ACCEL_Z_K      ABS_Z

// 720 LSG = 1G
#define LSG_K                         (720.0f)
#define NUMOFACCDATA_K                8

// conversion of acceleration data to SI units (m/s^2)
#define RANGE_A_K                     (2*GRAVITY_EARTH)
#define RESOLUTION_A_K                (RANGE_A/(256*NUMOFACCDATA_K))
#define CONVERT_A_K                   (GRAVITY_EARTH / LSG / NUMOFACCDATA_K)

/* conversion of magnetic data to uT units (1/5 * 0.3) */
#define CONVERT_M_K                   (0.06f)

/* conversion of orientation data to degree units (1/64) */
#define CONVERT_O_K                   (0.015625f)

#define SENSOR_STATE_MASK           (0x7FFF)

/*****************************************************************************/

__END_DECLS

#endif  // ANDROID_SENSORS_H
