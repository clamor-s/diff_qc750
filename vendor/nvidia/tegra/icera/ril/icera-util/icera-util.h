/* Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved. */
/*
** Copyright 2010, Icera Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** You may not use this file except in compliance with the License.
** You may obtain a copy of the License at:
**
** http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** imitations under the License.
*/

#ifndef ICERA_UTIL_H
#define ICERA_UTIL_H

/*************************************************************************************************
 * Project header files
 ************************************************************************************************/

/*************************************************************************************************
 * Standard header files (e.g. <string.h>, <stdlib.h> etc.
 ************************************************************************************************/
#include <stdbool.h>

/*************************************************************************************************
 * Macros
 ************************************************************************************************/

/*************************************************************************************************
 * Public Types
 ************************************************************************************************/

typedef enum
{
    MODEM_POWER_ON,
    MODEM_POWER_OFF,
    MODEM_POWER_CYCLE
}power_control_command_t;

typedef enum
{
    UNCONFIGURED =  -2,
    OVERRIDDEN =    -1,
    NORMAL =         0,
}power_control_status_t;

/*************************************************************************************************
 * Public variable declarations
 ************************************************************************************************/

/*************************************************************************************************
 * Public function declarations
 ************************************************************************************************/

/**
 *  Configure sysfs device that will be used for power control
 *  utilities.
 *
 *  Requires "modem.power.device" set as system property with
 *  following format:
 *  <power_sysfs_device_path>,<power_off_string>,<power_on_string>
 *
 *  where <power_off_string>,<power_on_string> are values to
 *  write in <power_sysfs_device_path> to handle poff or pon.
 *
 */
void IceraModemPowerControlConfig(void);

/**
 * Power control utilities in power_control_command_t.
 *
 * @param cmd    power cmd in power_control_command_t
 * @param force  bypass "modem.powercontrol" setting
 *
 * @return power_control_status_t NORMAL if command is successful, OVERRIDDEN if overridden UNCONFIGURED if unconfigured
 */
power_control_status_t IceraModemPowerControl(power_control_command_t cmd, int force);

/**
 * Gt power control status.
 *
 * @param None
 *
 * @return power_control_status_t NORMAL if command is successful, OVERRIDDEN if overridden UNCONFIGURED if unconfigured
 */
power_control_status_t IceraModemPowerStatus(void);

/**
 *  Register for wake lock framework
 *
 *  To enable/disable platform LP0
 *
 *
 * @param name      name of lock
 * @param use_count if true, all call to IceraWakeLockAcquire
 *                  increments a counter and each call to
 *                  IceraWakeLockRelease decrements it. Unlock
 *                  done only if counter is zero.
 *                  if false, unlock each time
 *                  IceraWakeLockRelease is called regardless
 *                  num of previous calls to
 *                  IceraWakeLockAcquire
 */
void IceraWakeLockRegister(char *name, bool use_count);

/**
 * Prevent LP0
 */
void IceraWakeLockAcquire(void);

/**
 * Allow LP0 (always if registered with use_count=false, only
 * when all unlock done if registered with use_count==true)
 */
void IceraWakeLockRelease(void);

/**
 *  Configure sysfs device that will be used for modem usb stack
 *  utilities.
 *
 *  Requires "modem.power.usbdevice" set as system property with
 *  following format:
 *  <mdm_usb_sysfs_device_path>,<unload_string>,<reload_string>
 *
 *  where <unload_string>,<reload_string> are values to
 *  write in <mdm_usb_sysfs_device_path> to handle unload/reload
 *  of modem USB stack.
 *
 *  @return 1 if config was OK, 0 if not.
 *
 */
int IceraUsbStackConfig(void);

/**
 * Unload modem USB stack
 */
void IceraUnloadUsbStack(void);

/**
 * Reload modem USB stack
 */
void IceraReloadUsbStack(void);


/**
 * Dumps a kernel timereference into the current log
 */
#define LogKernelTimeStamp(A) {\
                struct timespec time_stamp;\
                clock_gettime(CLOCK_MONOTONIC, &time_stamp);\
                ALOGD("Kernel time: [%d.%6d]",(int)time_stamp.tv_sec,(int)time_stamp.tv_nsec/1000);\
                }
#endif /* ICERA_UTIL_H */
