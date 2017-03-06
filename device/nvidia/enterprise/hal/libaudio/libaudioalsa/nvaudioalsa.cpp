/*
 * Copyright (c) 2011-2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

/*
 ** Based upon alsa_default.cpp, provided under the following terms:
 **
 ** Copyright 2009 Wind River Systems
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

#define LOG_TAG "NvAudioALSA"
//#define LOG_NDEBUG 0

#include <cutils/properties.h>
#include "nvaudioalsa.h"

// Hack to write AT commands to modem device
const char deviceName[] = "dev/ttyACM2";

#undef DISABLE_HARWARE_RESAMPLING

using namespace android;

#define ALSA_DEFAULT_DEVICE_NAME "default"
#define ALSA_MUSIC_DEVICE_NAME "music"
#define ALSA_VOICE_DEVICE_NAME "voice"
#define ALSA_AUX_DEVICE_NAME "aux"
#define ALSA_AUX_PLUG_DEVICE_NAME "aux_plug"
#define ALSA_VOICE_CALL_DEVICE_NAME "voice_call"
#define ALSA_BT_VOICE_CALL_DEVICE_NAME "bt_voice_call"
#define ALSA_MUSIC_AND_VOICE_DEVICE_NAME "music_and_voice"
#define ALSA_MUSIC_AND_AUX_DEVICE_NAME "music_and_aux"
#define ALSA_VOICE_AND_AUX_DEVICE_NAME "voice_and_aux"
#define ALSA_MUSIC_AND_VOICE_AND_AUX_DEVICE_NAME "music_and_voice_and_aux"

#define ALSA_DEFAULT_DEVICE_NAME_aic326x "default_aic326x"
#define ALSA_MUSIC_DEVICE_NAME_aic326x "music-aic326x"
#define ALSA_VOICE_DEVICE_NAME_aic326x "voice-aic326x"
#define ALSA_MUSIC_AND_VOICE_DEVICE_NAME_aic326x "music_and_voice-aic326x"
#define ALSA_VOICE_CALL_DEVICE_NAME_aic326x "voice_call-aic326x"
#define ALSA_BT_VOICE_CALL_DEVICE_NAME_aic326x "bt_voice_call-aic326x"

#define TEGRA3_DEFAULT_OUT_SAMPLE_RATE   48000
#define TEGRA_MAX_ALSA_CARDS 2

#ifdef TEGRA_VOICE_CALL_CHANNELS
#undef TEGRA_VOICE_CALL_CHANNELS
#endif
#define TEGRA_VOICE_CALL_CHANNELS        2

#define TOTAL_VOICE_VOLUME_STEPS      5

// (MAX SR * PERIOD ALIGN * MAX CH) / (MIN SR * MIN CH)
// (48000 * 8 * 2)/(8000 * 1)
#define ALSA_BUFFER_SIZE_SEARCH_RANGE           96

#define LP_STATE_SYSFS "/sys/power/suspend/mode"
#define LP_STATE_VOICE_CALL "lp1"

/*
 * Helper functions for managing low power modes
 */
static status_t save_lp_mode(tegra_alsa_device_t *dev)
{
    int fd;

    if (strcmp(dev->lp_state_default, "\0"))
        return NO_ERROR;

    fd = open(LP_STATE_SYSFS, O_RDWR);

    if (fd < 0) {
        ALOGE("open failed of sysfs entry %s\n", LP_STATE_SYSFS);
        return NO_INIT;
    }

    read(fd, dev->lp_state_default, 3);
    close(fd);

    return NO_ERROR;
}

static status_t set_lp_mode(char *lp_mode)
{
    int fd;

    fd = open(LP_STATE_SYSFS, O_RDWR);

    if (fd < 0) {
        ALOGE("open failed of sysfs entry %s\n", LP_STATE_SYSFS);
        return NO_INIT;
    }

    write(fd, lp_mode, strlen(lp_mode));
    close(fd);

    return NO_ERROR;
}

static status_t restore_lp_mode(tegra_alsa_device_t *dev)
{
    if (!strcmp(dev->lp_state_default, "\0"))
        return NO_ERROR;

    set_lp_mode(dev->lp_state_default);

    strcpy(dev->lp_state_default, "\0");

    return NO_ERROR;
}

/*
 * Helper functions for writing AT commands to the RIL
 */
static int send_at_cmd(const char *cmd)
{
    char buf[2048];
    int fd=0;
    int ret=0;

    ALOGV("send_at_cmd %s", cmd);

    fd = open(deviceName, O_RDWR);
    if (fd < 0)
    {
        ALOGE("%s open failed err=%d\n",deviceName,fd);
        return fd;
    }

    ret = write(fd, cmd, strlen(cmd));
    ALOGV("at %s ret=%d\n",cmd,ret);
    close(fd);

    return 0;
}

/*******************************************************************/

static void resetAlsaBufferParams(NvAudioALSADev *hDev)
{
    unsigned int size;

    if (hDev->streamType == SND_PCM_STREAM_PLAYBACK) {
        hDev->latency = TEGRA_DEFAULT_OUT_MAX_LATENCY;
        hDev->periods = TEGRA_DEFAULT_OUT_PERIODS;
    }
    else {
        hDev->latency = TEGRA_DEFAULT_IN_MAX_LATENCY;
        hDev->periods = TEGRA_DEFAULT_IN_PERIODS;
    }

    size = (uint32_t)(((uint64_t)hDev->latency * hDev->sampleRate) / 1000000);
    for (int i = 1; (size & ~i) != 0; i<<=1) {
        size &= ~i;
    }
    hDev->bufferSize = size;
    hDev->latency = (uint32_t)(((uint64_t)size * 1000000) / hDev->sampleRate);

    ALOGV("Reset buffer size to %d and latency to %d\n",
        hDev->bufferSize, hDev->latency);
}

const char *streamName(NvAudioALSADev *hDev)
{
    return snd_pcm_stream_name(hDev->streamType);
}

status_t setHardwareParams(NvAudioALSADev *hDev)
{
    snd_pcm_hw_params_t *hwParams;
    status_t err  = 0;
    int i;

    snd_pcm_uframes_t bufferSize = hDev->bufferSize;
    snd_pcm_uframes_t periodSize;
    unsigned int periods = hDev->periods;
    unsigned int latency = hDev->latency;
    unsigned int periodTime;
    unsigned int requestedRate = hDev->sampleRate;
    const char *formatDesc;
    const char *formatName;

    NvAudioALSAControl *alsaHDACtrl = hDev->device->alsaHDAControl;
    // snd_pcm_format_description() and snd_pcm_format_name() do not perform
    // proper bounds checking.
    if ((static_cast<int> (hDev->format) > SND_PCM_FORMAT_UNKNOWN) &&
        (static_cast<int> (hDev->format) <= SND_PCM_FORMAT_LAST)){

        formatDesc = snd_pcm_format_description(hDev->format);
        formatName = snd_pcm_format_name(hDev->format);

    } else {

        formatDesc = "Invalid Format";
        formatName = "UNKNOWN";
    }

    if (snd_pcm_hw_params_malloc(&hwParams) < 0) {
        LOG_ALWAYS_FATAL("Failed to allocate ALSA hardware parameters (%s)!",
                            snd_strerror(err));
        return NO_INIT;
    }

    err = snd_pcm_hw_params_any((snd_pcm_t*)hDev->hpcm, hwParams);
    if (err < 0) {
        ALOGE("Unable to configure hardware: %s", snd_strerror(err));
        goto done;
    }

    // Set the interleaved read and write format.
    err = snd_pcm_hw_params_set_access((snd_pcm_t*)hDev->hpcm, hwParams,
            SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        ALOGE("Unable to configure PCM read/write format: %s",
                snd_strerror(err));
        goto done;
    }

    err = snd_pcm_hw_params_set_format((snd_pcm_t*)hDev->hpcm, hwParams,
            hDev->format);
    if (err < 0) {
        ALOGE("Unable to configure PCM format %s (%s): %s",
                formatName, formatDesc, snd_strerror(err));
        goto done;
    }

    ALOGV("Set %s PCM format to %s (%s)", streamName(hDev), formatName, formatDesc);

    // Since aux_plug is tried to be opened only after failure to open aux
    // device it is not required to overwrite aux max channel count while
    // opening aux_plug.
    if (!strcmp(hDev->alsaDevName, ALSA_AUX_PLUG_DEVICE_NAME))
        hDev->channels = 2;
    else
    hDev->max_out_channels = 2;

    if (!strcmp(hDev->alsaDevName, ALSA_AUX_DEVICE_NAME)) {
        snd_pcm_hw_params_get_channels_max(hwParams, &hDev->max_out_channels);

        if ((hDev->channels < hDev->max_out_channels) &&
            (hDev->max_out_channels <= TEGRA_MAX_CHANNELS))
            hDev->channels = hDev->max_out_channels;

        if (alsaHDACtrl) {
            err = alsaHDACtrl->get("HDA Decode Capability",
                                   hDev->recDecCapable, 0);
            if (err < 0)
                ALOGE("HDA Query has failed");
        }
    }

    if ((hDev->max_out_channels > 0) &&
        (hDev->channels > hDev->max_out_channels))
        hDev->channels = hDev->max_out_channels;

    // as per HDMI spec 1.3a, section 7.6, IEC61937 compressed stream should use
    // 2 channel format
    if (hDev->isCompressed) {
        ALOGV("Force channels to 2 for compressed format");
        hDev->channels = 2;
    }

    err = snd_pcm_hw_params_set_channels((snd_pcm_t*)hDev->hpcm, hwParams,
            hDev->channels);
    if (err < 0) {
        ALOGE("Unable to set channel count to %i: %s",
                hDev->channels, snd_strerror(err));
        goto done;
    }

    ALOGV("Using %i %s for %s. Device can support max %d chnannels",
        hDev->channels, hDev->channels == 1 ? "channel" : "channels",
        streamName(hDev),hDev->max_out_channels);

    err = snd_pcm_hw_params_set_rate_near((snd_pcm_t*)hDev->hpcm, hwParams,
            &requestedRate, 0);

    if (err < 0)
        ALOGE("Unable to set %s sample rate to %u: %s",
                streamName(hDev), hDev->sampleRate, snd_strerror(err));
    else if (requestedRate != hDev->sampleRate) {
        // Some devices have a fixed sample rate, and can not be changed.
        // This may cause resampling problems; i.e. PCM playback will be too
        // slow or fast.
        ALOGW("Requested rate (%u HZ) does not match actual rate (%u HZ)",
                hDev->sampleRate, requestedRate);
        if (!strcmp(hDev->alsaDevName, ALSA_AUX_DEVICE_NAME))
            return -EINVAL;
    }
    else
        ALOGV("Set %s sample rate to %u HZ", streamName(hDev), requestedRate);

#ifdef DISABLE_HARWARE_RESAMPLING
    // Disable hardware re-sampling.
    err = snd_pcm_hw_params_set_rate_resample((snd_pcm_t*)hDev->hpcm,
            hwParams,
            static_cast<int>(resample));
    if (err < 0) {
        ALOGE("Unable to %s hardware resampling: %s",
                resample ? "enable" : "disable",
                snd_strerror(err));
        goto done;
    }
#endif
    // Make sure we succeed in setting hardware params

    i = 0;
    do {
        bufferSize = hDev->bufferSize + ((i <= ALSA_BUFFER_SIZE_SEARCH_RANGE) ?
                       i : (ALSA_BUFFER_SIZE_SEARCH_RANGE - i));
        i++;

        err = snd_pcm_hw_params_set_buffer_size((snd_pcm_t*)hDev->hpcm,
                                                hwParams, bufferSize);
    if (err < 0) {
            ALOGV("Unable to set buffer size to %d: %s\n",
                (int)bufferSize, snd_strerror(err));
            continue;
    }
    } while ((err < 0) && (i < (ALSA_BUFFER_SIZE_SEARCH_RANGE << 1)));

    if (err < 0) {
        periodTime = latency / periods;
        err = snd_pcm_hw_params_set_period_time_near((snd_pcm_t*)hDev->hpcm,
                                                hwParams, &periodTime, NULL);
        if (err < 0) {
            ALOGE("Unable to set period time to %d: %s",
                    (int)periodTime, snd_strerror(err));
            goto done;
    }

        err = snd_pcm_hw_params_get_period_size(hwParams, &periodSize, NULL);
    if (err < 0) {
            ALOGE("Unable to get period size: %s\n", snd_strerror(err));
            goto done;
        }

        bufferSize = periodSize * periods;
        err = snd_pcm_hw_params_set_buffer_size_near((snd_pcm_t*)hDev->hpcm,
                                                 hwParams, &bufferSize);
        if (err < 0) {
            ALOGV("Unable to set buffer size to %d: %s\n",
                    (int)bufferSize, snd_strerror(err));
            goto done;
        }

        err = snd_pcm_hw_params_get_buffer_time(hwParams, &latency, NULL);
        if (err < 0) {
            ALOGE("Unable to get the buffer time: %s", snd_strerror(err));
            goto done;
        }
    }
    else {
        periodSize = bufferSize / periods;
        err = snd_pcm_hw_params_set_period_size_near((snd_pcm_t*)hDev->hpcm,
                                                 hwParams, &periodSize, NULL);
        if (err < 0) {
            ALOGE("Unable to set period size to %d: %s\n",
                    (int)periodSize, snd_strerror(err));

            err = snd_pcm_hw_params_get_period_size_max(hwParams,
                    &periodSize, NULL);
        if (err < 0) {
                ALOGE("Unable to get max period size: %s\n", snd_strerror(err));
            goto done;
        }

            err = snd_pcm_hw_params_set_period_size_near((snd_pcm_t*)hDev->hpcm,
                                                   hwParams, &periodSize, NULL);
        if (err < 0) {
                ALOGE("Unable to set period size to %d : %s\n",
                        (int)periodSize, snd_strerror(err));
            goto done;
        }
        }

        err = snd_pcm_hw_params_get_buffer_time(hwParams, &latency, NULL);
        if (err < 0) {
            ALOGE("Unable to get the buffer time: %s", snd_strerror(err));
            goto done;
        }

        err = snd_pcm_hw_params_get_period_time(hwParams, &periodTime, NULL);
        if (err < 0) {
            ALOGE("Unable to get period time: %s", snd_strerror(err));
            goto done;
        }
    }

    periods = bufferSize/periodSize;
    ALOGV("Buffer size: %d", (int)bufferSize);
    ALOGV("Period size: %d", (int)periodSize);
    ALOGV("Latency: %d", (int)latency);
    ALOGV("Period Time: %d", (int)periodTime);
    ALOGV("Periods: %d", (int)periods);

    hDev->periods = periods;
    hDev->bufferSize = bufferSize;
    hDev->latency = latency;

    // Commit the hardware parameters back to the device.
    err = snd_pcm_hw_params((snd_pcm_t*)hDev->hpcm, hwParams);
    if (err < 0) ALOGE("Unable to set hardware parameters: %s", snd_strerror(err));

    done:
    snd_pcm_hw_params_free(hwParams);

    return err;
}

status_t setSoftwareParams(NvAudioALSADev *hDev)
{
    snd_pcm_sw_params_t * swParams;
    int err = 0;

    snd_pcm_uframes_t bufferSize = 0;
    snd_pcm_uframes_t periodSize = 0;
    snd_pcm_uframes_t boundary = 0;
    snd_pcm_uframes_t startThreshold, stopThreshold;
    snd_pcm_uframes_t silenceSize, silenceThreshold;

    if (snd_pcm_sw_params_malloc(&swParams) < 0) {
        LOG_ALWAYS_FATAL("Failed to allocate ALSA software parameters (%s)!",
                            snd_strerror(err));
        return NO_INIT;
    }

    // Get the current software parameters
    err = snd_pcm_sw_params_current((snd_pcm_t*)hDev->hpcm, swParams);
    if (err < 0) {
        ALOGE("Unable to get software parameters: %s", snd_strerror(err));
        goto done;
    }

    // Configure ALSA to start the transfer when the buffer is almost full.
    snd_pcm_get_params((snd_pcm_t*)hDev->hpcm, &bufferSize, &periodSize);
    snd_pcm_sw_params_get_boundary(swParams, &boundary);

    if (hDev->streamType == SND_PCM_STREAM_PLAYBACK) {
        // For playback, configure ALSA to start the transfer when the
        // buffer is full.
        startThreshold = bufferSize - 1;
        if (!strcmp(hDev->alsaDevName, ALSA_AUX_DEVICE_NAME) ||
            !strcmp(hDev->alsaDevName, ALSA_AUX_PLUG_DEVICE_NAME)) {
            stopThreshold = boundary;
            silenceSize = boundary;
            silenceThreshold = 0;
        }
        else {
        stopThreshold = bufferSize;
            silenceSize = 0;
            silenceThreshold = 0;
        }
    } else {
        // For recording, configure ALSA to start the transfer on the
        // first frame.
        startThreshold = 1;
        stopThreshold = bufferSize;
        silenceSize = 0;
        silenceThreshold = 0;
    }

    err = snd_pcm_sw_params_set_silence_size((snd_pcm_t*)hDev->hpcm, swParams,
            silenceSize);
    if (err < 0)
        ALOGE("Unable to set silence size to %lu frames: %s",
                silenceSize, snd_strerror(err));

    err = snd_pcm_sw_params_set_silence_threshold((snd_pcm_t*)hDev->hpcm,
                                               swParams, silenceThreshold);
    if (err < 0)
        ALOGE("Unable to set silence threshold to %lu frames: %s",
                silenceThreshold, snd_strerror(err));

    err = snd_pcm_sw_params_set_start_threshold((snd_pcm_t*)hDev->hpcm,
                                               swParams, startThreshold);
    if (err < 0) {
        ALOGE("Unable to set start threshold to %lu frames: %s",
                startThreshold, snd_strerror(err));
        goto done;
    }

    err = snd_pcm_sw_params_set_stop_threshold((snd_pcm_t*)hDev->hpcm,
                                               swParams, stopThreshold);
    if (err < 0) {
        ALOGE("Unable to set stop threshold to %lu frames: %s",
                stopThreshold, snd_strerror(err));
        goto done;
    }

    // Allow the transfer to start when at least periodSize samples can be
    // processed.
    err = snd_pcm_sw_params_set_avail_min((snd_pcm_t*)hDev->hpcm,
                                                swParams, periodSize);
    if (err < 0) {
        ALOGE("Unable to configure available minimum to %lu: %s",
                periodSize, snd_strerror(err));
        goto done;
    }

    // Set timestamp mode to SND_PCM_TSTAMP_MMAP
    err = snd_pcm_sw_params_set_tstamp_mode((snd_pcm_t*)hDev->hpcm,
                                            swParams, SND_PCM_TSTAMP_MMAP);
    if (err < 0) {
        ALOGE("Unable to configure time stamp mode: %s",
                snd_strerror(err));
    }

    // Commit the software parameters back to the device.
    err = snd_pcm_sw_params((snd_pcm_t*)hDev->hpcm, swParams);
    if (err < 0) ALOGE("Unable to configure software parameters: %s",
            snd_strerror(err));

    done:
    snd_pcm_sw_params_free(swParams);

    return err;
}

static void getAlsaDeviceName(uint32_t devices, bool isVoiceCallDevice, char* devName, char* cardname)
{
    bool isMusicDevice = false, isVoiceDevice = false, isAuxDevice = false;

    if (devices & (AudioSystem::DEVICE_OUT_EARPIECE |
                    AudioSystem::DEVICE_OUT_SPEAKER |
                    AudioSystem::DEVICE_OUT_WIRED_HEADSET |
                    AudioSystem::DEVICE_OUT_WIRED_HEADPHONE |
                    AudioSystem::DEVICE_IN_BUILTIN_MIC |
                    AudioSystem::DEVICE_IN_WIRED_HEADSET |
                    AudioSystem::DEVICE_IN_BACK_MIC)) {
        isMusicDevice = true;
    }

    if (devices & (AudioSystem::DEVICE_OUT_BLUETOOTH_SCO |
                    AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET |
                    AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT |
                    AudioSystem::DEVICE_IN_BLUETOOTH_SCO_HEADSET)) {
        isVoiceDevice = true;
    }

    if (devices & (AudioSystem::DEVICE_OUT_AUX_DIGITAL |
                AudioSystem::DEVICE_IN_AUX_DIGITAL)) {
        isAuxDevice = true;
    }

    if (isVoiceCallDevice && isVoiceDevice) {
        if (!strcmp(cardname,"tegra-aic326x"))
            strcpy(devName, ALSA_BT_VOICE_CALL_DEVICE_NAME_aic326x);
        else
            strcpy(devName, ALSA_BT_VOICE_CALL_DEVICE_NAME);
    }
    else if (isVoiceCallDevice) {
        if (!strcmp(cardname,"tegra-aic326x"))
            strcpy(devName, ALSA_VOICE_CALL_DEVICE_NAME_aic326x);
        else
            strcpy(devName, ALSA_VOICE_CALL_DEVICE_NAME);
    }
    else if (isMusicDevice && isVoiceDevice && isAuxDevice)
        strcpy(devName, ALSA_MUSIC_AND_VOICE_AND_AUX_DEVICE_NAME);
    else if (isMusicDevice && isVoiceDevice) {
        if (!strcmp(cardname,"tegra-aic326x"))
            strcpy(devName, ALSA_MUSIC_AND_VOICE_DEVICE_NAME_aic326x);
        else
            strcpy(devName, ALSA_MUSIC_AND_VOICE_DEVICE_NAME);
    }
    else if (isVoiceDevice && isAuxDevice)
        strcpy(devName, ALSA_VOICE_AND_AUX_DEVICE_NAME);
    else if (isMusicDevice && isAuxDevice)
        strcpy(devName, ALSA_MUSIC_AND_AUX_DEVICE_NAME);
    else if (isMusicDevice) {
        if (!strcmp(cardname,"tegra-aic326x"))
            strcpy(devName, ALSA_MUSIC_DEVICE_NAME_aic326x);
        else
            strcpy(devName, ALSA_MUSIC_DEVICE_NAME);
    }
    else if (isVoiceDevice) {
        if (!strcmp(cardname,"tegra-aic326x"))
            strcpy(devName, ALSA_VOICE_DEVICE_NAME_aic326x);
        else
            strcpy(devName, ALSA_VOICE_DEVICE_NAME);
    }
    else if (isAuxDevice)
        strcpy(devName, ALSA_AUX_DEVICE_NAME);
    else {
        if (!strcmp(cardname,"tegra-aic326x"))
            strcpy(devName, ALSA_DEFAULT_DEVICE_NAME_aic326x);
        else
            strcpy(devName, ALSA_DEFAULT_DEVICE_NAME);
    }

    ALOGV("getAlsaDeviceName::devices 0x%x IsVoiceCallDevice %d devName %s\n",
            devices, isVoiceCallDevice, devName);
}

static void NvAudioALSADevClose(NvAudioALSADev *hDev)
{
    int err = NO_ERROR;
    ALOGV("Closing ALSA device!, curDev 0x%x, curMode %d, streamtype %d",
                        hDev->curDev, hDev->curMode, hDev->streamType);

    AutoMutex lock(hDev->device->m_Lock);

    if (hDev->hpcm) {
        snd_pcm_state_t pcm_state;

        pcm_state = snd_pcm_state((snd_pcm_t*)hDev->hpcm);

        if (pcm_state != SND_PCM_STATE_SUSPENDED) {
            err = snd_pcm_drain((snd_pcm_t*)hDev->hpcm);
            if (err < 0)
                ALOGE("Failed to snd_pcm_drain(), %d", err);

            err = snd_pcm_close((snd_pcm_t*)hDev->hpcm);
            if (err < 0)
                ALOGE("Failed to snd_pcm_close(), %d", err);
            else {
                hDev->hpcm = 0;
                hDev->isSilent = false;
            }
        }
    }
}

static status_t NvAudioALSADevOpen(NvAudioALSADev *hDev, uint32_t devices, int mode)
{
    snd_pcm_stream_t streamtype = hDev->streamType;
    char *alsaDevName = hDev->alsaDevName;
    NvAudioALSAControl *alsaCtrl = hDev->device->alsaControl;
    int err = NO_INIT;

    AutoMutex lock(hDev->device->m_Lock);

    if (!devices) {
        ALOGE("NvAudioALSADevOpen is called with %d device\n",devices);
        return err;
    }

    for ( int i=0; i < TEGRA_MAX_ALSA_CARDS; i++) {
        err = snd_card_get_name( i, &hDev->cardname );
        if(err == 0 && ( !strcmp( hDev->cardname, "tegra-max98088" ) ||
                         !strcmp( hDev->cardname, "tegra-aic326x" )))
            break;
    }

    /* for tegra-aic326x codec*/
    if (!strcmp( hDev->cardname, "tegra-aic326x")) {
        if (alsaCtrl && (hDev->streamType == SND_PCM_STREAM_PLAYBACK)) {
            alsaCtrl->set("PCM Playback Volume", 135);
            alsaCtrl->set("Speaker Amplifier Volume", 1);
            alsaCtrl->set("Receiver Amplifier Volume", 10);
        } else {
            alsaCtrl->set("MICBIAS EXT Power Level", (uint32_t)0);
        }
    }

    getAlsaDeviceName(devices, hDev->isVoiceCallDevice, alsaDevName, hDev->cardname);

    ALOGV("open called for devices %08x in mode %d, streamtype %d...",
            devices, mode, streamtype);

    if (hDev->isVoiceCallDevice) {
        hDev->sampleRate = TEGRA3_DEFAULT_OUT_SAMPLE_RATE;
        hDev->channels = TEGRA_VOICE_CALL_CHANNELS;
    }

    err = snd_pcm_open((snd_pcm_t**)&hDev->hpcm, alsaDevName, streamtype,
                       SND_PCM_ASYNC);
    if (err < 0) {
        // None of the Android defined audio devices exist. Open a generic one.
        ALOGE("Failed to Initialize ALSA %s device: %s",
                alsaDevName, strerror(err));

        /* For voice call device and AUX device don't try to open default
         * ALSA device in failure case.
         */
        if (hDev->isVoiceCallDevice ||
            !strcmp(alsaDevName, ALSA_AUX_DEVICE_NAME))
            goto out;

        strcpy(alsaDevName, ALSA_DEFAULT_DEVICE_NAME);
        err = snd_pcm_open((snd_pcm_t**)&hDev->hpcm, alsaDevName, streamtype, 0
                                   /*Open in Blocking mode*/);
    }

    if (err < 0) {
        ALOGE("Failed to Initialize any ALSA %s device: %s",
                snd_pcm_stream_name(streamtype), strerror(err));
        err = NO_INIT;
        goto out;
    }

    resetAlsaBufferParams(hDev);
    err = setHardwareParams(hDev);
    if (err != NO_ERROR) {
        ALOGE("Failed to set hardware params ALSA %s device: %s",
                snd_pcm_stream_name(streamtype), strerror(err));

        if (!strcmp(alsaDevName,ALSA_AUX_DEVICE_NAME)) {
            snd_pcm_drain((snd_pcm_t*)hDev->hpcm);
            snd_pcm_close((snd_pcm_t*)hDev->hpcm);
            hDev->hpcm = 0;

            strcpy(alsaDevName, ALSA_AUX_PLUG_DEVICE_NAME);
            err = snd_pcm_open((snd_pcm_t**)&hDev->hpcm, alsaDevName, streamtype,
                SND_PCM_ASYNC);
            if (err != NO_ERROR) {
                err = NO_INIT;
                goto out;
            }
            resetAlsaBufferParams(hDev);
            err = setHardwareParams(hDev);
        }

        if (err != NO_ERROR) {
            err = NO_INIT;
            goto out_pcm_close;
        }
    }

    err = setSoftwareParams(hDev);
    if (err != NO_ERROR) {
        ALOGE("Failed to set software params ALSA %s device: %s",
                snd_pcm_stream_name(streamtype), strerror(err));
        err = NO_INIT;
        goto out_pcm_close;
    }

    if (devices == AudioSystem::DEVICE_OUT_AUX_DIGITAL) {
        snd_pcm_start((snd_pcm_t*)hDev->hpcm);
        hDev->isSilent = true;
    }

    ALOGV("Initialized ALSA %s device %s", snd_pcm_stream_name(streamtype), alsaDevName);

    goto out;

out_pcm_close:
    snd_pcm_drain((snd_pcm_t*)hDev->hpcm);
    snd_pcm_close((snd_pcm_t*)hDev->hpcm);
    hDev->hpcm = 0;
out:
    return err;
}

static void SetVoiceVolumeForDevice(NvAudioALSADev *hDev,
                    uint32_t devices, float volume)
{
    NvAudioALSAControl *alsaCtrl = hDev->device->alsaControl;
    char value[PROPERTY_VALUE_MAX] = {0};
    char prop[PROPERTY_KEY_MAX] = {0};
    char ctrlName[128] = {0};
    uint32_t index;
    int vol;

    if (!strcmp( hDev->cardname, "tegra-aic326x")) {
        return;
    }

    index = volume * TOTAL_VOICE_VOLUME_STEPS;

    if (devices & (AudioSystem::DEVICE_OUT_SPEAKER |
                   AudioSystem::DEVICE_OUT_DEFAULT)) {
        snprintf(prop, sizeof(prop), "audio.speaker.voice.%d", index);
        strcpy(ctrlName, "Speaker Volume");
        hDev->speakerVol = volume;
    } else if (devices & AudioSystem::DEVICE_OUT_EARPIECE) {
        snprintf(prop, sizeof(prop), "audio.earpiece.voice.%d", index);
        strcpy(ctrlName, "Receiver Volume");
        hDev->earpieceVol = volume;
    } else if (devices & (AudioSystem::DEVICE_OUT_WIRED_HEADSET |
                          AudioSystem::DEVICE_OUT_WIRED_HEADPHONE)) {
        snprintf(prop, sizeof(prop), "audio.headset.voice.%d", index);
        strcpy(ctrlName, "Headphone Volume");
        hDev->headsetVol = volume;
    }

    if (property_get(prop, value, NULL)) {
        int vol_min, vol_max, vol_cur;
        vol = atoi(value);
        alsaCtrl->get(ctrlName, vol_cur, vol_min, vol_max, 0);
        ALOGV("%s: vol %d, vol_cur %d, vol_min %d, vol_max %d", __FUNCTION__,
                    vol, vol_cur, vol_min, vol_max);
        if (vol < vol_min) vol = vol_min;
        if (vol > vol_max) vol = vol_max;
        alsaCtrl->set(ctrlName, vol, 0, 2);
    }
}

static status_t NvAudioALSADevSetMute(NvAudioALSADev *hDev,uint32_t devices,
                                                                bool is_mute)
{
    NvAudioALSAControl *alsaCtrl = hDev->device->alsaControl;
    status_t err = 0;

    if (!strcmp( hDev->cardname, "tegra-aic326x")) {
        return NO_INIT;
    }

    if (!alsaCtrl)
        return NO_INIT;

    if (devices & AudioSystem::DEVICE_IN_WIRED_HEADSET) {
        if (is_mute) {
            err = alsaCtrl->set("Left ADC Mixer MIC2 Switch", (uint32_t)0);
            if (NO_ERROR == err)
                err = alsaCtrl->set("Right ADC Mixer MIC2 Switch", (uint32_t)0);
        }
        else {
            err = alsaCtrl->set("Left ADC Mixer MIC2 Switch", 1);
            if (NO_ERROR == err)
                err = alsaCtrl->set("Right ADC Mixer MIC2 Switch", 1);
        }
    }
    else if (devices & (AudioSystem::DEVICE_IN_BUILTIN_MIC|
                                AudioSystem::DEVICE_IN_DEFAULT |
                            AudioSystem::DEVICE_IN_BACK_MIC|
                            AudioSystem::DEVICE_IN_AMBIENT)) {
        if (is_mute)
            err = alsaCtrl->set("Digital Mic Enable", (uint32_t)0);
        else
            err = alsaCtrl->set("Digital Mic Enable", 3);
    }
    else
        return NO_INIT;

    if (err < 0)
        return err;

    return NO_ERROR;
}

static status_t NvAudioALSADevRoute(NvAudioALSADev *hDev, uint32_t devices, int mode)
{
    status_t err = 0;
    NvAudioALSAControl *alsaCtrl = hDev->device->alsaControl;
    char newAlsaDevName[TEGRA_ALSA_DEVICE_NAME_MAX_LEN] = "";
    ALOGV("route called for devices %08x in mode %d...", devices, mode);

    if (hDev->streamType == SND_PCM_STREAM_PLAYBACK)
        devices &= AudioSystem::DEVICE_OUT_ALL;
    else
        devices &= AudioSystem::DEVICE_IN_ALL;

    if ((hDev && hDev->curDev == devices && hDev->curMode == mode) || !devices)
    {
        ALOGV("route unchanged...");
        return NO_ERROR;
    }

    if (hDev->isVoiceCallDevice &&
            (hDev->curMode != AudioSystem::MODE_IN_CALL) &&
            (mode != AudioSystem::MODE_IN_CALL)) {
        hDev->curDev = devices;
        ALOGV("mode unchanged...");
        return NO_ERROR;
    }

    for ( int i=0; i < TEGRA_MAX_ALSA_CARDS; i++) {
        err = snd_card_get_name( i, &hDev->cardname );
        if(err == 0 && ( !strcmp( hDev->cardname, "tegra-max98088" ) ||
                         !strcmp( hDev->cardname, "tegra-aic326x" )))
            break;
    }

    /* for tegra-aic326x codec*/
    if (!strcmp( hDev->cardname, "tegra-aic326x")) {
        if (alsaCtrl && (hDev->curMode != mode)  &&
             (mode == AudioSystem::MODE_IN_CALL && hDev->isVoiceCallDevice)) {
            if (!hDev->hpcm) {
                err = NvAudioALSADevOpen(hDev, devices, mode);
                if (err < 0) {
                    ALOGE("route:Failed to open alsa voice call device\n");
                    return err;
                }
            }
            alsaCtrl->set("MicPGA Volume Control", 50);
        }
    }

    if (alsaCtrl) {
        uint32_t alsaDevices = 0;

    if ((hDev->curDev != devices) ||
       (hDev->curMode != mode)) {
        if (hDev->streamType == SND_PCM_STREAM_PLAYBACK) {

            /* for tegra-max98088 codec*/
            if (!strcmp( hDev->cardname, "tegra-max98088")) {

                if (devices & (AudioSystem::DEVICE_OUT_SPEAKER |
                    AudioSystem::DEVICE_OUT_DEFAULT)) {
                    alsaCtrl->set("Left SPK Mixer Left DAC1 Switch", 1);
                    alsaCtrl->set("Right SPK Mixer Right DAC1 Switch", 1);

                    if (mode == AudioSystem::MODE_IN_CALL)
                        SetVoiceVolumeForDevice(hDev, devices, hDev->speakerVol);
                    else /* set codec speaker volume to +5 dB*/
                        alsaCtrl->set("Speaker Volume", 26, 0, 2);

                    alsaCtrl->set("Int Spk Switch", (uint32_t)1);
                    send_at_cmd("at%iaudcnf=2\r\n");
                } else if (hDev->curDev & (AudioSystem::DEVICE_OUT_SPEAKER |
                    AudioSystem::DEVICE_OUT_DEFAULT)) {
                    alsaCtrl->set("Left SPK Mixer Left DAC1 Switch", (uint32_t)0);
                    alsaCtrl->set("Right SPK Mixer Right DAC1 Switch", (uint32_t)0);
                    alsaCtrl->set("Int Spk Switch", (uint32_t)0);
                }

                if (devices & AudioSystem::DEVICE_OUT_EARPIECE) {
                    alsaCtrl->set("Left REC Mixer Left DAC1 Switch", 1);
                    alsaCtrl->set("Right REC Mixer Right DAC1 Switch", 1);

                    if (mode == AudioSystem::MODE_IN_CALL)
                        SetVoiceVolumeForDevice(hDev, devices, hDev->earpieceVol);
                    else /* set codec Receiver volume to 0 dB*/
                        alsaCtrl->set("Receiver Volume", 21, 0, 2);

                    alsaCtrl->set("Earpiece Switch", 1);
                    send_at_cmd("at%iaudcnf=2\r\n");
                } else if (hDev->curDev & AudioSystem::DEVICE_OUT_EARPIECE) {
                    alsaCtrl->set("Left REC Mixer Left DAC1 Switch", (uint32_t)0);
                    alsaCtrl->set("Right REC Mixer Right DAC1 Switch", (uint32_t)0);
                    alsaCtrl->set("Earpiece Switch", (uint32_t)0);
                }

                if (devices & (AudioSystem::DEVICE_OUT_WIRED_HEADSET |
                    AudioSystem::DEVICE_OUT_WIRED_HEADPHONE)) {
                    alsaCtrl->set("Left HP Mixer Left DAC1 Switch", 1);
                    alsaCtrl->set("Right HP Mixer Right DAC1 Switch", 1);

                    if (mode == AudioSystem::MODE_IN_CALL)
                        SetVoiceVolumeForDevice(hDev, devices, hDev->headsetVol);
                    else /* set codec Headphone volume to 0 dB */
                        alsaCtrl->set("Headphone Volume", 26, 0, 2);

                    alsaCtrl->set("Headphone Jack Switch", 1);
                    send_at_cmd("at%iaudcnf=1\r\n");
                } else if (hDev->curDev & (AudioSystem::DEVICE_OUT_WIRED_HEADSET |
                    AudioSystem::DEVICE_OUT_WIRED_HEADPHONE)) {
                    alsaCtrl->set("Left HP Mixer Left DAC1 Switch", (uint32_t)0);
                    alsaCtrl->set("Right HP Mixer Right DAC1 Switch", (uint32_t)0);
                    alsaCtrl->set("Headphone Jack Switch", (uint32_t)0);
                }

                if (devices & (AudioSystem::DEVICE_OUT_BLUETOOTH_SCO |
                    AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET |
                    AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT)) {
                    send_at_cmd("at%iaudcnf=4\r\n");
                    //TODO: bt-sco playback
                }

                if (devices & AudioSystem::DEVICE_OUT_AUX_DIGITAL) {
                    //Nothing to do here
                }
            }

            /* for tegra-aic326x codec*/
            if (!strcmp( hDev->cardname, "tegra-aic326x")) {

                if (hDev->curDev & (AudioSystem::DEVICE_OUT_EARPIECE |
                     AudioSystem::DEVICE_OUT_SPEAKER |
                     AudioSystem::DEVICE_OUT_DEFAULT)) {
                    /*Disconnect LDAC and RDAC signals from LOL and LOR*/
                    alsaCtrl->set("LOL Output Mixer LDAC Switch", (uint32_t)0);
                    alsaCtrl->set("LOR Output Mixer RDAC Switch",(uint32_t)0);
                    if (hDev->curDev & (AudioSystem::DEVICE_OUT_SPEAKER |
                         AudioSystem::DEVICE_OUT_DEFAULT)) {
                        alsaCtrl->set("SPKL Output Mixer LOL Volume", 127);
                        alsaCtrl->set("SPKR Output Mixer LOR Volume", 127);
                        alsaCtrl->set("Int Spk Switch", (uint32_t)0);
                    }
                    if (hDev->curDev & AudioSystem::DEVICE_OUT_EARPIECE) {
                        alsaCtrl->set("REC Output Mixer LOL-B2 Volume", 127);
                        alsaCtrl->set("REC Output Mixer LOR-B2 Volume", 127);
                        alsaCtrl->set("Earpiece Switch", (uint32_t)0);
                    }
                }

                if (devices & (AudioSystem::DEVICE_OUT_SPEAKER |
                    AudioSystem::DEVICE_OUT_DEFAULT)) {
                    /*Route LOL and LOR to SPKL and SPR*/
                    /*Connect LDAC and RDAC signals to LOL and LOR*/
                    alsaCtrl->set("Int Spk Switch", 1);
                    alsaCtrl->set("LOL Output Mixer LDAC Switch", 1);
                    alsaCtrl->set("LOR Output Mixer RDAC Switch", 1);
                    alsaCtrl->set("SPKL Output Mixer LOL Volume", 10);
                    alsaCtrl->set("SPKR Output Mixer LOR Volume", 10);
                }

                if (devices & AudioSystem::DEVICE_OUT_EARPIECE) {
                    /*Route LOL and LOR to RECP and RECM*/
                    alsaCtrl->set("Earpiece Switch", 1);
                    alsaCtrl->set("LOL Output Mixer LDAC Switch", 1);
                    alsaCtrl->set("LOR Output Mixer RDAC Switch", 1);
                    alsaCtrl->set("REC Output Mixer LOL-B2 Volume", 10);
                    alsaCtrl->set("REC Output Mixer LOR-B2 Volume", 10);
                    alsaCtrl->set("PCM Playback Volume", 150);
                }

                if (devices & (AudioSystem::DEVICE_OUT_WIRED_HEADSET |
                               AudioSystem::DEVICE_OUT_WIRED_HEADPHONE)) {
                    alsaCtrl->set("HPL Output Mixer LDAC Switch", 1);
                    alsaCtrl->set("HPR Output Mixer RDAC Switch", 1);
                    alsaCtrl->set("Headphone Jack Switch", 1);
                } else if (hDev->curDev & (AudioSystem::DEVICE_OUT_WIRED_HEADSET |
                               AudioSystem::DEVICE_OUT_WIRED_HEADPHONE)){
                    alsaCtrl->set("Headphone Jack Switch", (uint32_t)0);
                    alsaCtrl->set("HPL Output Mixer LDAC Switch", (uint32_t)0);
                    alsaCtrl->set("HPR Output Mixer RDAC Switch", (uint32_t)0);
                }

                if (devices & (AudioSystem::DEVICE_OUT_BLUETOOTH_SCO |
                            AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET |
                            AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT)) {
                    // TODO: BT SCO playback
                }

                if (devices & AudioSystem::DEVICE_OUT_AUX_DIGITAL) {
                    // Nothing to do here
                }
            }

        } else {

            /* for tegra-max98088 codec*/
            if (!strcmp( hDev->cardname, "tegra-max98088")) {

                if (devices & (AudioSystem::DEVICE_IN_COMMUNICATION |
                                AudioSystem::DEVICE_IN_VOICE_CALL)) {
                    //TODO: add voice call record
                }

                if (devices & AudioSystem::DEVICE_IN_BLUETOOTH_SCO_HEADSET) {
                    //TODO: add bt-sco record
                }

                if (devices & AudioSystem::DEVICE_IN_AUX_DIGITAL) {
                    ALOGW("Capture through AUX digital is not supported \n");
                }

                if (devices & (AudioSystem::DEVICE_IN_BUILTIN_MIC |
                    AudioSystem::DEVICE_IN_DEFAULT |
                    AudioSystem::DEVICE_IN_BACK_MIC |
                    AudioSystem::DEVICE_IN_AMBIENT)) {
                    alsaCtrl->set("Digital Mic Enable", 3);
                    alsaCtrl->set("Left ADC Mixer MIC1 Switch", 1);
                    alsaCtrl->set("Right ADC Mixer MIC1 Switch", 1);
                    alsaCtrl->set("Int Mic Switch", 1);
                } else if (hDev->curDev & (AudioSystem::DEVICE_IN_BUILTIN_MIC |
                    AudioSystem::DEVICE_IN_DEFAULT |
                    AudioSystem::DEVICE_IN_BACK_MIC |
                    AudioSystem::DEVICE_IN_AMBIENT)) {
                    alsaCtrl->set("Digital Mic Enable", (uint32_t)0);
                    alsaCtrl->set("Left ADC Mixer MIC1 Switch", (uint32_t)0);
                    alsaCtrl->set("Right ADC Mixer MIC1 Switch", (uint32_t)0);
                    alsaCtrl->set("Int Mic Switch", (uint32_t)0);
                }

                if (devices & AudioSystem::DEVICE_IN_WIRED_HEADSET) {
                    alsaCtrl->set("Left ADC Mixer MIC2 Switch", 1);
                    alsaCtrl->set("Right ADC Mixer MIC2 Switch", 1);
                    alsaCtrl->set("Mic Jack Switch", 1);
                }  else if (hDev->curDev & AudioSystem::DEVICE_IN_WIRED_HEADSET) {
                    alsaCtrl->set("Left ADC Mixer MIC2 Switch", (uint32_t)0);
                    alsaCtrl->set("Right ADC Mixer MIC2 Switch", (uint32_t)0);
                    alsaCtrl->set("Mic Jack Switch", (uint32_t)0);
                }

                if (hDev->device->isMicMute)
                    NvAudioALSADevSetMute(hDev, devices, hDev->device->isMicMute);
            }

            /* for tegra-aic326x codec*/
            if (!strcmp( hDev->cardname, "tegra-aic326x")) {

                if (devices & (AudioSystem::DEVICE_IN_COMMUNICATION |
                     AudioSystem::DEVICE_IN_VOICE_CALL)) {
                     //TODO: Implement voice call record
                }

                if (devices & (AudioSystem::DEVICE_IN_BUILTIN_MIC |
                     AudioSystem::DEVICE_IN_DEFAULT |
                     AudioSystem::DEVICE_IN_AMBIENT |
                     AudioSystem::DEVICE_IN_BACK_MIC)) {
                    alsaCtrl->set("Int Mic Switch", 1);
                    alsaCtrl->set("Left Input Mixer IN2L Switch", 1);
                    alsaCtrl->set("Right Input Mixer IN2R Switch", 1);
                    alsaCtrl->set("Left Input Mixer CM2L Switch", 1);
                } else if (hDev->curDev & (AudioSystem::DEVICE_IN_BUILTIN_MIC |
                               AudioSystem::DEVICE_IN_DEFAULT |
                               AudioSystem::DEVICE_IN_AMBIENT |
                               AudioSystem::DEVICE_IN_BACK_MIC)) {
                    alsaCtrl->set("Left Input Mixer IN2L Switch", (uint32_t)0);
                    alsaCtrl->set("Right Input Mixer IN2R Switch", (uint32_t)0);
                    alsaCtrl->set("Left Input Mixer CM2L Switch", (uint32_t)0);
                    alsaCtrl->set("Int Mic Switch", (uint32_t)0);
                }

                if (devices & AudioSystem::DEVICE_IN_WIRED_HEADSET) {
                    alsaCtrl->set("Mic Jack Switch", 1);
                    alsaCtrl->set("Left Input Mixer IN1L Switch", 1);
                    alsaCtrl->set("Left Input Mixer CM1L Switch", 1);
                    alsaCtrl->set("MICBIAS_EXT ON", 1);
                    alsaCtrl->set("Mic Bias ext independent enable", 1);
                    alsaCtrl->set("Input CM mode", 1);
                } else if (hDev->curDev & AudioSystem::DEVICE_IN_WIRED_HEADSET) {
                    alsaCtrl->set("Left Input Mixer IN1L Switch", (uint32_t)0);
                    alsaCtrl->set("Left Input Mixer CM1L Switch", (uint32_t)0);
                    alsaCtrl->set("MICBIAS_EXT ON", (uint32_t)0);
                    alsaCtrl->set("Mic Bias ext independent enable", (uint32_t)0);
                    alsaCtrl->set("Input CM mode", (uint32_t)0);
                    alsaCtrl->set("Mic Jack Switch", (uint32_t)0);
                }

                if (devices & AudioSystem::DEVICE_IN_BLUETOOTH_SCO_HEADSET) {
                    //TODO: BT SCO record
                }

                if (devices & AudioSystem::DEVICE_IN_AUX_DIGITAL)
                    ALOGW("Capture through AUX digital is not supported \n");
            }
        }
    }

        if (hDev->curMode != mode) {
            if (mode == AudioSystem::MODE_IN_CALL && hDev->isVoiceCallDevice) {
                if (!hDev->hpcm) {
                    err = NvAudioALSADevOpen(hDev, devices, mode);
                    if (err < 0) {
                        ALOGE("route:Failed to open alsa voice call device\n");
                        return err;
                    }
                }
                err = save_lp_mode(hDev->device);
                if (err != NO_ERROR)
                    ALOGE("route: Failed to save lp mode.\n");

                err = set_lp_mode(LP_STATE_VOICE_CALL);
                if (err != NO_ERROR)
                    ALOGE("route: Failed to set lp1 mode.\n");

                alsaCtrl->set("Call Mode Switch", (uint32_t)true);
            }
            else if (hDev->curMode == AudioSystem::MODE_IN_CALL) {
                alsaCtrl->set("Call Mode Switch", (uint32_t)false);
                err = restore_lp_mode(hDev->device);
                if (err != NO_ERROR)
                    ALOGE("route: Failed to restore lp mode.\n");

                /* Don't close playback voice device if voice call is
                   received or made during music playback */
                if (!((hDev->streamType == SND_PCM_STREAM_PLAYBACK) &&
                            hDev->device->isPlaybackActive)) {
                    NvAudioALSADevClose(hDev);
                }
            }
        }
    }

    if (hDev->hpcm) {
        getAlsaDeviceName(devices, hDev->isVoiceCallDevice, newAlsaDevName, hDev->cardname);
        if (strcmp(newAlsaDevName, hDev->alsaDevName)) {
            if (mode == AudioSystem::MODE_IN_CALL && hDev->isVoiceCallDevice) {
                    alsaCtrl->set("Call Mode Switch", (uint32_t)false);
                    NvAudioALSADevClose(hDev);
                    err = NvAudioALSADevOpen(hDev, devices, mode);
                    if (err < 0) {
                        ALOGE("route:Failed to open alsa voice call device\n");
                        return err;
                    }

                     /* for tegra-aic326x codec*/
                    if (!strcmp( hDev->cardname, "tegra-aic326x")) {
                        alsaCtrl->set("MicPGA Volume Control", 50);
                    }

                    alsaCtrl->set("Call Mode Switch", (uint32_t)true);
            } else {
                    NvAudioALSADevClose(hDev);
            }
        }
    }

    if (alsaCtrl) {
        // Dummy code for platform specific alsa control settings
        if ((devices & AudioSystem::DEVICE_OUT_WIRED_HEADSET) ||
                (devices & AudioSystem::DEVICE_IN_WIRED_HEADSET)) {
            /* Add Headset devices */
            ALOGV("Add DEVICE_OUT_WIRED_HEADPHONE and DEVICE_IN_WIRED_HEADSET to devices for Headset");
        }

        if (devices & AudioSystem::DEVICE_OUT_ALL) {
            /* Mute/Un-Mute */
            if (devices & (AudioSystem::DEVICE_OUT_SPEAKER | AudioSystem::DEVICE_OUT_DEFAULT)) {
                ALOGV("Set device route to DEVICE_OUT_SPEAKER and DEVICE_OUT_DEFAULT");
                //equalizer setting

            } else if (devices & AudioSystem::DEVICE_OUT_WIRED_HEADPHONE) {
                ALOGV("Set device route to DEVICE_OUT_WIRED_HEADPHONE");
                //equalizer setting

            }else {
                ALOGV("Unknown output devices 0x%x, mode %d", devices, mode);
            }
        }

        if (devices & AudioSystem::DEVICE_IN_ALL) {
            if (devices & (AudioSystem::DEVICE_IN_BUILTIN_MIC | AudioSystem::DEVICE_IN_DEFAULT)) {
                ALOGV("Set device route to DEVICE_IN_BUILTIN_MIC and DEVICE_IN_DEFAULT");

                /* Audio Source Select */
            } else if (devices & AudioSystem::DEVICE_IN_WIRED_HEADSET) {
                ALOGV("Set device route to DEVICE_IN_WIRED_HEADSET");

            } else {
                ALOGV("Unknown input devices 0x%x, mode %d", devices, mode);
            }
        }
    }

    hDev->curDev = devices;
    hDev->curMode = mode;

    return err;
}

static ssize_t NvAudioALSADevWrite(NvAudioALSADev *hDev,
                                   void *buffer, size_t bytes)
{
    int isAux = (hDev->curDev == AudioSystem::DEVICE_OUT_AUX_DIGITAL);
    status_t err;

    if (hDev->isSilent && isAux) {
        snd_pcm_reset((snd_pcm_t*)hDev->hpcm);
        hDev->isSilent = false;
    }

    snd_pcm_sframes_t n = 0;
    size_t sent = 0;

    while (hDev->hpcm && (sent < bytes)) {
        n = snd_pcm_writei((snd_pcm_t*)hDev->hpcm, (char *)buffer + sent,
               snd_pcm_bytes_to_frames((snd_pcm_t*)hDev->hpcm, bytes - sent));
        if (n == -EBADFD) {
            // Somehow the stream is in a bad state. The driver probably
            // has a bug and snd_pcm_recover() doesn't seem to handle this.
            ALOGE("write: failed to snd_pcm_writei, %d, %s",
                (int)n, snd_strerror((int)n));
            NvAudioALSADevClose(hDev);
            err = NvAudioALSADevOpen(hDev, hDev->curDev, hDev->curMode);
            if (err < 0) {
                ALOGE("Failed to open alsa device for output stream, %d", err);
                break;
            }
        } else if (n == -EBADF) {
            ALOGE("write: failed to snd_pcm_writei, %d, %s",
                (int)n, snd_strerror((int)n));
            // For Bad file number case don't try to close alsa device
            hDev->hpcm = 0;
            err = NvAudioALSADevOpen(hDev, hDev->curDev, hDev->curMode);
            if (err < 0) {
                ALOGE("Failed to open alsa device for output stream, %d", err);
                break;
            }
        } else if (n == -ESTRPIPE) {
            // Somehow the stream remains in suspend state even though device
            // has been already resumed. The driver probably has a bug and
            // snd_pcm_recover() doesn't seem to handle this.
            ALOGE("write: failed to snd_pcm_writei, %d, %s",
                (int)n, snd_strerror((int)n));
            snd_pcm_close((snd_pcm_t*)hDev->hpcm);
            err = NvAudioALSADevOpen(hDev, hDev->curDev, hDev->curMode);
            if (err < 0) {
                ALOGE("Failed to open alsa device for output stream, %d", err);
                break;
            }
        } else if (n < 0) {
            ALOGV("write: failed to snd_pcm_writei, %d, %s",
                (int)n, snd_strerror((int)n));
            // snd_pcm_recover() will return 0 if successful in recovering from
            // an error, or -errno if the error was unrecoverable.
            if (hDev->hpcm)
                n = snd_pcm_recover((snd_pcm_t*)hDev->hpcm, n, 1);
        }

        if (n > 0)
            sent += static_cast<ssize_t>(snd_pcm_frames_to_bytes
                                         ((snd_pcm_t*)hDev->hpcm, n));

        //ALOGV("alsa: device %s write_bytes %d",
        //     isAux ? "AUX":"MUSIC",
        //    (int)snd_pcm_frames_to_bytes((snd_pcm_t*)hDev->hpcm, n));
    }

    return sent;
}

status_t NvAudioALSADevSetVoiceVolume(NvAudioALSADev *hDev, float volume)
{
    status_t err = 0;
    ALOGV("%s: volume %f, curDev %X, mode %d ",
        __FUNCTION__, volume, hDev->curDev, hDev->curMode);

    if (hDev->isVoiceCallDevice && hDev->curMode == AudioSystem::MODE_IN_CALL) {
        if (hDev->streamType == SND_PCM_STREAM_PLAYBACK) {
            SetVoiceVolumeForDevice(hDev, hDev->curDev, volume);
        }
    }
    return err;
}

status_t NvAudioALSADevSetGain(NvAudioALSADev *hDev, float gain)
{
    status_t err = 0;

    ALOGV("%s: gain %f", __FUNCTION__, gain);

    return err;
}

status_t NvAudioALSADevSetVolume(NvAudioALSADev *hDev, float left, float right)
{
    status_t err = 0;

    ALOGV("%s: left %f, right %f", __FUNCTION__, left, right);

    return err;
}

static int tegra_alsa_close(hw_device_t* device)
{
    free(device);
    return 0;
}

static int tegra_alsa_open(const hw_module_t* module, const char* name, hw_device_t** device)
{
    ALOGV("tegra_alsa_open: IN");

    struct tegra_alsa_device_t *dev;

    dev = (struct tegra_alsa_device_t*)malloc(sizeof(*dev));
    if (!dev)
            return -ENOMEM;

    memset(dev, 0, sizeof(*dev));

    /* initialize the procs */
    dev->common.tag     = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module  = (hw_module_t *) module;
    strcpy(dev->lp_state_default,"\0");
    dev->common.close   = tegra_alsa_close;
    dev->default_sampleRate = TEGRA3_DEFAULT_OUT_SAMPLE_RATE;
    dev->open           = NvAudioALSADevOpen;
    dev->close          = NvAudioALSADevClose;
    dev->route          = NvAudioALSADevRoute;
    dev->write          = NvAudioALSADevWrite;
    dev->set_volume     = NvAudioALSADevSetVolume;
    dev->set_voice_volume = NvAudioALSADevSetVoiceVolume;
    dev->set_gain       = NvAudioALSADevSetGain;
    dev->set_mute       = NvAudioALSADevSetMute;

    *device = &dev->common;

    return 0;

}

struct hw_module_methods_t tegra_alsa_module_methods = {
    /** Open a specific device */
    open : tegra_alsa_open,
};

extern "C" const struct hw_module_t TEGRA_ALSA_MODULE = {
            tag              : (uint32_t )HARDWARE_MODULE_TAG,
            version_major    : 1,
            version_minor    : 0,
            id               : TEGRA_ALSA_HARDWARE_MODULE_ID,
            name             : "NVIDIA Tegra alsa module",
            author           : "NVIDIA",
            methods          : &tegra_alsa_module_methods,
            dso              : 0,
            reserved         : { 0, },
};
