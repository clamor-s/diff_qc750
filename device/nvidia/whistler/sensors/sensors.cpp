/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Copyright (c) 2011-2012, NVIDIA CORPORATION.  All rights reserved.
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

#include "isl29018.h"
#include "adxl34x.h"
#include "hwmon.h"

#include "MPLSensor.h"

#define LUX_SYSFS_PATH       "/sys/bus/iio/devices/device0/illuminance0_input"
#define LUX_SYSFS_PATH_NEW   "/sys/bus/iio/devices/iio:device0/in_illuminance0_input"
#define PROX_SYSFS_PATH      "/sys/bus/iio/devices/device0/proximity_raw"
#define PROX_SYSFS_PATH_NEW  "/sys/bus/iio/devices/iio:device0/in_proximity_raw"
#define TEMP_SYSFS_PATH      "/sys/class/hwmon/hwmon0/device/temp2_input"

/* Proxiimity distance to determine object is near or far from sensor device */
#define PROX_THRESHOLD_ISL29018 25000

static const struct sensor_t sSensorList[] = {
    ADXL34XACCEL_DEF(ID_A),
    ISL29018LIGHT_DEF(ID_L),
    ISL29018PROX_DEF(ID_P),
    ADT7461TEMP_DEF(ID_O),
};

/*****************************************************************************/

static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device);


static int sensors__get_sensors_list(struct sensors_module_t* module,
                                     struct sensor_t const** list)
{
    *list = sSensorList;
    return ARRAY_SIZE(sSensorList);
}

static struct hw_module_methods_t sensors_module_methods = {
        open: open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
        common: {
                tag: HARDWARE_MODULE_TAG,
                version_major: 1,
                version_minor: 0,
                id: SENSORS_HARDWARE_MODULE_ID,
                name: "Whistler sensors module",
                author: "nvidia",
                methods: &sensors_module_methods,
                dso: NULL,
                reserved: {0}
        },
        get_sensors_list: sensors__get_sensors_list,
};

struct sensors_poll_context_t {
    struct sensors_poll_device_t device; // must be first

        sensors_poll_context_t();
        ~sensors_poll_context_t();
    int activate(int handle, int enabled);
    int setDelay(int handle, int64_t ns);
    int pollEvents(sensors_event_t* data, int count);

private:
    enum {
        accelerometer     = 0,
        light             = 1,
        proximity         = 2,
        temperature       = 3,
        numSensorDrivers,       // wake pipe goes here
        mpl_power,              //special handle for MPL pm interaction
        numFds,
    };

    static const size_t wake = numFds - 2;
    static const char WAKE_MESSAGE = 'W';
    struct pollfd mPollFds[numFds];
    int mWritePipeFd;
    SensorBase* mSensors[numSensorDrivers];

    int handleToDriver(int handle) const {
        switch (handle) {
            case ID_A:
                return accelerometer;
            case ID_L:
                return light;
            case ID_P:
                return proximity;
            case ID_O:
                return temperature;
        }
        return -EINVAL;
    }
};

/*****************************************************************************/

sensors_poll_context_t::sensors_poll_context_t()
{
    int fd;

    FUNC_LOG;

    mSensors[accelerometer] = new Adxl34xAccel(ID_A);
    mPollFds[accelerometer].fd = mSensors[accelerometer]->getFd();
    mPollFds[accelerometer].events = POLLIN;
    mPollFds[accelerometer].revents = 0;

    /* try nodes from kernel v3.4 if we fail to open */
    fd = open(LUX_SYSFS_PATH, O_RDONLY);
    if (fd < 0) {
        mSensors[light] = new Isl29018Light(LUX_SYSFS_PATH_NEW, ID_L);
    } else {
        mSensors[light] = new Isl29018Light(LUX_SYSFS_PATH, ID_L);
        close(fd);
    }
    mPollFds[light].fd = -1;

    /* try nodes from kernel v3.4 if we fail to open */
    fd = open(PROX_SYSFS_PATH, O_RDONLY);
    if (fd < 0) {
        mSensors[proximity] = new Isl29018Prox(PROX_SYSFS_PATH_NEW, ID_P,
                                  PROX_THRESHOLD_ISL29018);
    } else {
        mSensors[proximity] = new Isl29018Prox(PROX_SYSFS_PATH, ID_P,
                                  PROX_THRESHOLD_ISL29018);
        close(fd);
    }
    mPollFds[proximity].fd = -1;

    mSensors[temperature] = new HwmonTemp(TEMP_SYSFS_PATH, ID_O);
    mPollFds[temperature].fd = -1;

    int wakeFds[2];
    int result = pipe(wakeFds);
    ALOGE_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
    fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
    fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
    mWritePipeFd = wakeFds[1];

    mPollFds[wake].fd = wakeFds[0];
    mPollFds[wake].events = POLLIN;
    mPollFds[wake].revents = 0;

    mPollFds[mpl_power].fd = -1;
    mPollFds[mpl_power].revents = 0;
}

sensors_poll_context_t::~sensors_poll_context_t()
{
    FUNC_LOG;
    for (int i=0 ; i<numSensorDrivers ; i++) {
        delete mSensors[i];
    }
    close(mPollFds[wake].fd);
    close(mWritePipeFd);
}

int sensors_poll_context_t::activate(int handle, int enabled)
{
    FUNC_LOG;
    int index = handleToDriver(handle);
    if (index < 0) return index;
    int err =  mSensors[index]->enable(handle, enabled);
    if (!err) {
        const char wakeMessage(WAKE_MESSAGE);
        int result = write(mWritePipeFd, &wakeMessage, 1);
        ALOGE_IF(result<0, "error sending wake message (%s)", strerror(errno));
    }
    return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns)
{
    FUNC_LOG;
    int index = handleToDriver(handle);
    if (index < 0) return index;
    return mSensors[index]->setDelay(handle, ns);
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
{
    //FUNC_LOG;
    int nbEvents = 0;
    int n = 0;
    int polltime = -1;
    bool canPoll = false;

    do {
        // see if we have some leftover from the last poll()
        for (int i=0 ; count && i<numSensorDrivers ; i++) {
            SensorBase* const sensor(mSensors[i]);
            if ((mPollFds[i].revents & POLLIN) || (sensor->hasPendingEvents())) {
                int nb = sensor->readEvents(data, count);
                if (nb < count) {
                    // no more data for this sensor
                    mPollFds[i].revents = 0;
                }
                count -= nb;
                nbEvents += nb;
                data += nb;
            }
        }

        if (count) {
            // we still have some room, so try to see if we can get
            // some events immediately or just wait if we don't have
            // anything to return
            int i;

            n = poll(mPollFds, numFds, nbEvents ? 0 : polltime);
            if (n < 0) {
                ALOGE("poll() failed (%s)", strerror(errno));
                if (errno == EINTR)
                    return nbEvents;
                else
                    return -errno;
            }

            if (mPollFds[wake].revents & POLLIN) {
                char msg;
                int result = read(mPollFds[wake].fd, &msg, 1);
                ALOGE_IF(result<0, "error reading from wake pipe (%s)", strerror(errno));
                ALOGE_IF(msg != WAKE_MESSAGE, "unknown message on wake queue (0x%02x)", int(msg));
                mPollFds[wake].revents = 0;
            }
        }
        // if we have events and space, go read them
    } while (n && count);

    return nbEvents;
}

/*****************************************************************************/

static int poll__close(struct hw_device_t *dev)
{
    FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    if (ctx) {
        delete ctx;
    }
    return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
                          int handle, int enabled)
{
    FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
                          int handle, int64_t ns)
{
    FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->setDelay(handle, ns);
}

static int poll__poll(struct sensors_poll_device_t *dev,
                      sensors_event_t* data, int count)
{
    //FUNC_LOG;
    sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
    return ctx->pollEvents(data, count);
}

/*****************************************************************************/

/** Open a new instance of a sensor device using name */
static int open_sensors(const struct hw_module_t* module, const char* id,
                        struct hw_device_t** device)
{
    FUNC_LOG;
    int status = -EINVAL;
    sensors_poll_context_t *dev = new sensors_poll_context_t();

    memset(&dev->device, 0, sizeof(sensors_poll_device_t));

    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version  = 0;
    dev->device.common.module   = const_cast<hw_module_t*>(module);
    dev->device.common.close    = poll__close;
    dev->device.activate        = poll__activate;
    dev->device.setDelay        = poll__setDelay;
    dev->device.poll            = poll__poll;

    *device = &dev->device.common;
    status = 0;

    return status;
}
