/*
 * Copyright (c) 2011-2013 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */
#include "Barrier.h"

#include "NvCpuService.h"
#include <binder/PermissionCache.h>
#include <fcntl.h>

#define LOG_TAG "NvCpuService"

namespace android {

#define TIMEOUT_CASE(func) \
    case func##TimeoutEnum:
#define HANDLE_CASE(func) \
    case func##HandleEnum:
#define SET_CASE(func) \
    case set##func##Enum:
#define GET_CASE(func) \
    case get##func##Enum:

const String16 mAccessPower("android.permission.DEVICE_POWER");
const String16 mAccessNvidiaCpuPower("android.permission.NVIDIA_CPU_POWER");

NvCpuService::NvCpuService() : BnNvCpuService()
{ }

NvCpuService::~NvCpuService() {
    delete mTimeoutPoker;
}

status_t NvCpuService::onTransact(uint32_t code,
        const Parcel& data, Parcel* reply, uint32_t flags)
{
    IPCThreadState* ipc = IPCThreadState::self();
    const int pid = ipc->getCallingPid();
    const int uid = ipc->getCallingUid();

    switch(code) {
        NVCPU_FUNC_LIST(TIMEOUT_CASE)
        NVCPU_FUNC_LIST(HANDLE_CASE)
        NVCPU_AP_FUNC_LIST(GET_CASE)
        NVCPU_AP_FUNC_LIST(SET_CASE)
            if (!PermissionCache::checkPermission(mAccessPower, pid, uid) &&
                !PermissionCache::checkPermission(mAccessNvidiaCpuPower, pid, uid)) {
                LOGW("Access attempt to change power settings denied from pid=%d uid=%d", pid, uid);
                return PERMISSION_DENIED;
            }
        default:
            break;
    }

    status_t err = BnNvCpuService::onTransact(code, data, reply, flags);
    if (err == UNKNOWN_TRANSACTION || err == PERMISSION_DENIED) {
        return PERMISSION_DENIED;
    }

    // Both NvCpuService::onTransact() and BnNvCpuService::onTransact() will be called on pokeCPU() calls.
    // calling NvCpuService::pokeCPU here will cause two poke from a single request. Also, we really
    // need to check the permission from the daemon itself while data part will be only accessable from
    // the client library. That's the reason why we have two copies of onTransact().
    return NO_ERROR;
}

status_t NvCpuService::requestCpuFreqMin(int strength, nsecs_t timeoutNs)
{
    mTimeoutPoker->requestPmQosTimed(
                "/dev/cpu_freq_min",
                mTimeoutPoker->boostStrengthToFreq((NvCpuBoostStrength)strength),
                timeoutNs, false);
    return NO_ERROR;
}

status_t NvCpuService::requestCpuFreqMax(int strength, nsecs_t timeoutNs)
{
    mTimeoutPoker->requestPmQosTimed(
                "/dev/cpu_freq_max",
                mTimeoutPoker->boostStrengthToFreq((NvCpuBoostStrength)strength),
                timeoutNs, false);
    return NO_ERROR;
}

status_t NvCpuService::requestMinOnlineCpuCount(int numCpus, nsecs_t timeoutNs)
{
    mTimeoutPoker->requestPmQosTimed("/dev/min_online_cpus",
            numCpus, timeoutNs, false);
    return NO_ERROR;
}

status_t NvCpuService::requestMaxOnlineCpuCount(int numCpus, nsecs_t timeoutNs)
{
    mTimeoutPoker->requestPmQosTimed("/dev/max_online_cpus",
            numCpus, timeoutNs, false);
    return NO_ERROR;
}

int NvCpuService::requestMinOnlineCpuCountHandle(int numCpus)
{
    return mTimeoutPoker->createPmQosHandle(
            "/dev/min_online_cpus", numCpus, false);
}

int NvCpuService::requestMaxOnlineCpuCountHandle(int numCpus)
{
    return mTimeoutPoker->createPmQosHandle(
            "/dev/max_online_cpus", numCpus, false);
}

int NvCpuService::requestCpuFreqMinHandle(int strength)
{
    return mTimeoutPoker->createPmQosHandle(
            "/dev/cpu_freq_min",
            mTimeoutPoker->boostStrengthToFreq((NvCpuBoostStrength)strength), false);
}

int NvCpuService::requestCpuFreqMaxHandle(int strength)
{
    return mTimeoutPoker->createPmQosHandle(
            "/dev/cpu_freq_max",
            mTimeoutPoker->boostStrengthToFreq((NvCpuBoostStrength)strength), false);
}

status_t NvCpuService::requestMaxCbusFreq(int frequency, nsecs_t timeoutNs)
{
    mTimeoutPoker->requestPmQosTimed(
                "/dev/cbus_freq_max",
                frequency,
                timeoutNs, false);
    return NO_ERROR;
}

status_t NvCpuService::requestMinCbusFreq(int frequency, nsecs_t timeoutNs)
{
    mTimeoutPoker->requestPmQosTimed("/dev/cbus_freq_min",
            frequency, timeoutNs, false);
    return NO_ERROR;
}

int NvCpuService::requestMaxCbusFreqHandle(int frequency)
{
    return mTimeoutPoker->createPmQosHandle(
            "/dev/cbus_freq_max",
            frequency, false);
}

int NvCpuService::requestMinCbusFreqHandle(int frequency)
{
    return mTimeoutPoker->createPmQosHandle(
            "/dev/cbus_freq_min",
            frequency, false);
}

static void setGenericSysfsKnob(int val, const char *filename)
{
    int fd;
    char valOut[32];
    fd = open(filename, O_RDWR);

    if(fd < 0) {
        LOGE("unable to open %s\n", filename);
        return;
    }

    snprintf(valOut, sizeof(valOut), "%d\n", val);
    write(fd, valOut, sizeof(valOut));
    close(fd);
}

static int getGenericSysfsKnob(const char *filename)
{
    int fd;
    int val;
    char valIn[32];
    fd = open(filename, O_RDONLY);

    if(fd < 0) {
        LOGE("unable to open %s\n", filename);
        return -1;
    }

    read(fd, valIn, sizeof(valIn));
    sscanf(valIn, "%d", &val);
    close(fd);

    return val;
}

void NvCpuService::setCpuCoreBias(int bias)
{
    setGenericSysfsKnob(bias, "/d/tegra_hotplug/core_bias");
}

int NvCpuService::getCpuCoreBias(void)
{
    return getGenericSysfsKnob("/d/tegra_hotplug/core_bias");
}


void NvCpuService::setCpuHighFreqMinDelay(int delay)
{
    setGenericSysfsKnob(delay,
        "/sys/devices/system/cpu/cpufreq/interactive/high_freq_min_delay");
}

int NvCpuService::getCpuHighFreqMinDelay(void)
{
    return getGenericSysfsKnob(
        "/sys/devices/system/cpu/cpufreq/interactive/high_freq_min_delay");
}

void NvCpuService::setCpuMaxNormalFreq(int freq)
{
    setGenericSysfsKnob(freq,
        "/sys/devices/system/cpu/cpufreq/interactive/max_normal_freq");
}

int NvCpuService::getCpuMaxNormalFreq(void)
{
    return getGenericSysfsKnob(
        "/sys/devices/system/cpu/cpufreq/interactive/max_normal_freq");
}

void NvCpuService::setCpuScalingMinFreq(int freq)
{
    setGenericSysfsKnob(freq,
        "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
}

int NvCpuService::getCpuScalingMinFreq(void)
{
    return getGenericSysfsKnob(
        "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
}

void NvCpuService::setGpuCbusCapLevel(int freq)
{
    setGenericSysfsKnob(freq, "/sys/kernel/tegra_cap/cbus_cap_level");
}

int NvCpuService::getGpuCbusCapLevel(void)
{
    return getGenericSysfsKnob("/sys/kernel/tegra_cap/cbus_cap_level");
}

void NvCpuService::setGpuCbusCapState(int state)
{
    setGenericSysfsKnob(state, "/sys/kernel/tegra_cap/cbus_cap_state");
}

int NvCpuService::getGpuCbusCapState(void)
{
    return getGenericSysfsKnob("/sys/kernel/tegra_cap/cbus_cap_state");
}

void NvCpuService::setGpuScalingEnable(int enable)
{
    setGenericSysfsKnob(enable, "/sys/bus/nvhost/devices/gr3d/enable_3d_scaling");
}

int NvCpuService::getGpuScalingEnable(void)
{
    return getGenericSysfsKnob("/sys/bus/nvhost/devices/gr3d/enable_3d_scaling");
}

void NvCpuService::setEnableAppProfiles(int id)
{
  // Nothing to set
  return;
}

int NvCpuService::getEnableAppProfiles(void)
{
  return getGenericSysfsKnob(
    "/sys/module/tegra3_speedo/parameters/tegra_enable_app_profiles");
}

status_t NvCpuService::dump(int fd,
        const Vector<String16>& args)
{
    //TODO add some meaninful state dump
    return NO_ERROR;
}

void NvCpuService::onFirstRef()
{
    LOGI("Initializing NvCpuService");
    Barrier readyToRun;
    mTimeoutPoker = new nvcpu::TimeoutPoker(&readyToRun);
    readyToRun.wait();
    LOGI("Worker now ready");

    // nvcpud is supposed to be loaded on boot.
    // So, it makes sense to boost CPU now to reduce the
    // device boot time. boot timeout is set to 15 experimentally
    mTimeoutPoker->requestPmQosTimed("/dev/cpu_freq_min",
                mTimeoutPoker->boostStrengthToFreq(
                    NVCPU_BOOST_STRENGTH_HIGHEST),
                s2ns(15), true);
}

#undef TIMEOUT_CASE
#undef HANDLE_CASE
#undef SET_CASE
#undef GET_CASE

};
