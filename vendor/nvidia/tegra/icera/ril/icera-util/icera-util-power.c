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
/*
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/*************************************************************************************************
 * Project header files. The first in this list should always be the public interface for this
 * file. This ensures that the public interface header file compiles standalone.
 ************************************************************************************************/
#include "icera-util.h"
#include "icera-util-local.h"

/*************************************************************************************************
 * Standard header files (e.g. <string.h>, <stdlib.h> etc.
 ************************************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <cutils/properties.h>
#include <utils/Log.h>

/**************************************************************************************************************/
/* Private Macros  */
/**************************************************************************************************************/

/**************************************************************************************************************/
/* Private Functions Declarations (if needed) */
/**************************************************************************************************************/

/**************************************************************************************************************/
/* Private Type Declarations */
/**************************************************************************************************************/

typedef struct
{
    char power_device_config[PROPERTY_VALUE_MAX];
    char *power_device_path;
    char *power_on_string;
    char *power_off_string;
}PowerDevice;

/**************************************************************************************************************/
/* Private Variable Definitions  */
/**************************************************************************************************************/

static PowerDevice power_control_device =
{
    .power_device_path = NULL,
    .power_on_string   = NULL,
    .power_off_string  = NULL
};

static PowerDevice usb_modem_power_device =
{
    .power_device_path = NULL,
    .power_on_string   = NULL,
    .power_off_string  = NULL
};

/*************************************************************************************************
 * Private function definitions
 ************************************************************************************************/

/**
 * Effective write to a sysfs device.
 *
 * @param device_path path to the device.
 * @param data string to be written to the device.
 *
 * @return static void
 */
static void SysFsDeviceWrite(char *device_path,
                             char *data)
{
	int fd;

    fd = open(device_path, O_WRONLY);
    if(fd < 0)
    {
        ALOGE("%s: Fail to open %s. %s", __FUNCTION__, device_path, strerror(errno));
        return;
    }

    ALOGD("%s: writing '%s' to '%s'", __FUNCTION__, data, device_path);
    if(write(fd, data, strlen(data)) != (int)strlen(data))
    {
        ALOGE("%s: Fail to write to %s. %s\n",__FUNCTION__, device_path, strerror(errno));
    }

    close(fd);

    return;
}

/*************************************************************************************************
 * Public function definitions (Not doxygen commented, as they should be exported in the header
 * file)
 ************************************************************************************************/

void IceraModemPowerControlConfig(void)
{
    int ok;

    memset(power_control_device.power_device_config, 0, PROPERTY_VALUE_MAX);
    ok = property_get("modem.power.device",
                      power_control_device.power_device_config,
                      "");
    ALOGD("%s: found property %d %s\n",
         __FUNCTION__,
         ok,
         power_control_device.power_device_config);
    if(ok)
    {
        /* Configure power control with property input */
        power_control_device.power_device_path = strtok (power_control_device.power_device_config,",");
        power_control_device.power_off_string  = strtok (NULL,",");
        power_control_device.power_on_string   = strtok (NULL,",");
    }
    else
    {
        ALOGI("Modem Power Control not configured with modem.power.device property.\n");
    }

    return;
}

power_control_status_t IceraModemPowerStatus(void)
{
    int ok;
    char power_control[PROPERTY_VALUE_MAX];

    if(power_control_device.power_device_path == NULL)
    {
        ALOGD("Power control device not configured - exiting power control.\n");
        return UNCONFIGURED;
    }

    /* Modem power control can be disabled at runtime (during a reboot for example...)
        Check this property prior to any modem power action */
    ok = property_get("modem.powercontrol", power_control ,"");
    if(ok)
    {
        if((strcmp(power_control, "fil_disabled") == 0) || (strcmp(power_control, "disabled") == 0))
        {
            return OVERRIDDEN;
        }
    }

    return NORMAL;
}

power_control_status_t IceraModemPowerControl(power_control_command_t cmd, int force)
{
    power_control_status_t powerStatus = IceraModemPowerStatus();

    if((powerStatus==UNCONFIGURED)||
      ((powerStatus!=NORMAL)&&(force==0)))
    {
        return powerStatus;
    }

    ALOGD("%s: %d %s %s\n",
         __FUNCTION__,
         cmd,
         power_control_device.power_device_path,
         power_control_device.power_on_string);

    switch(cmd)
    {
    case MODEM_POWER_ON:
        ALOGD("Modem Power ON \n");
        SysFsDeviceWrite(power_control_device.power_device_path,
                         power_control_device.power_on_string);
        break;
    case MODEM_POWER_OFF:
        ALOGD("Modem Power OFF \n");
        SysFsDeviceWrite(power_control_device.power_device_path,
                         power_control_device.power_off_string);
        break;
    case MODEM_POWER_CYCLE:
        ALOGD("Modem Power Cycle \n");
        SysFsDeviceWrite(power_control_device.power_device_path,
                         power_control_device.power_off_string);
        usleep(100);
        SysFsDeviceWrite(power_control_device.power_device_path,
                         power_control_device.power_on_string);
        break;
    default:
        ALOGD("Unknown Power Control command \n");
        break;
    }

    return NORMAL;
}

int IceraUsbStackConfig(void)
{
    int ok;

    memset(usb_modem_power_device.power_device_config, 0, PROPERTY_VALUE_MAX);
    ok = property_get("modem.power.usbdevice",
                      usb_modem_power_device.power_device_config,
                      "");
    ALOGD("%s: found property %d %s\n",
         __FUNCTION__,
         ok,
         usb_modem_power_device.power_device_config);
    if(ok)
    {
        /* Configure power control with property input */
        usb_modem_power_device.power_device_path = strtok (usb_modem_power_device.power_device_config,",");
        usb_modem_power_device.power_off_string  = strtok (NULL,",");
        usb_modem_power_device.power_on_string   = strtok (NULL,",");
    }
    else
    {
        ALOGI("Modem USB Power Control not configured with modem.power.usbdevice property.\n");
    }

    return ok;
}

void IceraUnloadUsbStack(void)
{
    if(usb_modem_power_device.power_device_path == NULL)
    {
        ALOGD("Modem USB Power control device not configured - exiting usb power control.\n");
        return;
    }

    ALOGI("%s: Unload modem USB stack", __FUNCTION__);
    SysFsDeviceWrite(usb_modem_power_device.power_device_path,
                     usb_modem_power_device.power_off_string);
}

void IceraReloadUsbStack(void)
{
    if(usb_modem_power_device.power_device_path == NULL)
    {
        ALOGD("Modem USB Power control device not configured - exiting usb power control.\n");
        return;
    }

    ALOGI("%s: Reload modem USB stack", __FUNCTION__);
    SysFsDeviceWrite(usb_modem_power_device.power_device_path,
                     usb_modem_power_device.power_on_string);
}
