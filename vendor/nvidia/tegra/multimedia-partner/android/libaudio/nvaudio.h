/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

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

#ifndef LIBAUDIO_NVAUDIO_H
#define LIBAUDIO_NVAUDIO_H

//#define LOG_NDEBUG 0

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/time.h>

#include <cutils/log.h>
#include <cutils/str_parms.h>

#include <expat.h>

#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>
#include <tinyalsa/asoundlib.h>
#include <audio_utils/resampler.h>
#include <hardware/audio_effect.h>
#include <sound/asound.h>
#include "nvaudio_list.h"
#include "nvaudio_avp.h"
#ifdef NVOICE
#include "nvoice.h"
#endif
/*****************************************************************************
 * MACROS
 *****************************************************************************/
#define XML_CONFIG_FILE_PATH    "/etc/nvaudio_conf.xml"
#define IEC958_AES0_NONAUDIO      (1<<1)   /* 0 = audio, 1 = non-audio */
#define MAX_SUPPORTED_CHANNELS  8
#define ARRAY_SIZE(a)           (sizeof(a) / sizeof(a[0]))

#define ALSA_TO_AUDIO_FORMAT(a) ((a == PCM_FORMAT_S16_LE) ? \
                                AUDIO_FORMAT_PCM_16_BIT : \
                                AUDIO_FORMAT_PCM_32_BIT)
#define AUDIO_TO_ALSA_FORMAT(a) ((a == AUDIO_FORMAT_PCM_16_BIT) ? \
                                PCM_FORMAT_S16_LE : \
                                PCM_FORMAT_S32_LE)

#define SHIFT_Q15               15
#define INT_Q15(f)              (int32_t)((float)f * (1 << SHIFT_Q15))

#define MAX(a, b)               ((a) > (b)) ? (a) : (b)
#define MIN(a, b)               ((a) < (b)) ? (a) : (b)

#define LP_STATE_SYSFS "/sys/power/suspend/mode"
#define LP_STATE_VOICE_CALL "lp1"

#define AVP_LOOPBACK_FILE_PATH                  "/data/audio/audioplay.pcm"

/* Custom NV specific parameters */
#define NVAUDIO_PARAMETER_OUTPUT_AVAILABLE      "nv_param_output_available"
#define NVAUDIO_PARAMETER_MEDIA_ROUTING         "nv_param_media_routing"
#define NVAUDIO_PARAMETER_EFFECTS_COUNT         "nv_param_effects_count"
#define NVAUDIO_PARAMETER_IO_HANDLE             "nv_param_io_handle"
#define NVAUDIO_PARAMETER_ULP_SUPPORTED         "nv_param_ulp_supported"
#define NVAUDIO_PARAMETER_AVP_DECODE_FLUSH      "nv_param_avp_decode_flush"
#define NVAUDIO_PARAMETER_AVP_DECODE_PAUSE      "nv_param_avp_decode_pause"
#define NVAUDIO_PARAMETER_AVP_DECODE_POSITION   "nv_param_avp_decode_position"
#define NVAUDIO_PARAMETER_AVP_DECODE_EOS        "nv_param_avp_decode_eos"
#define NVAUDIO_PARAMETER_DEV_AVAILABLE         "nv_param_dev_available"
#define NVAUDIO_PARAMETER_AVP_LOOPBACK          "nv_param_avp_loopback"
#define NVAUDIO_PARAMETER_ULP_ACTIVE            "nv_param_ulp_active"


#define NVAUDIO_ULP_OUT_DEVICES            (AUDIO_DEVICE_OUT_EARPIECE | \
                                            AUDIO_DEVICE_OUT_SPEAKER | \
                                            AUDIO_DEVICE_OUT_DEFAULT | \
                                            AUDIO_DEVICE_OUT_WIRED_HEADSET | \
                                            AUDIO_DEVICE_OUT_WIRED_HEADPHONE)

#define TEGRA_HW_AC3DEC_PROPERTY "media.tegra.hw.ac3dec"
#define TEGRA_HW_DTSDEC_PROPERTY "media.tegra.hw.dtsdec"
#define REC_DEC_CAP_AC3   (1<<2)
#define REC_DEC_CAP_DTS   (1<<7)

/*****************************************************************************
 * ENUMS
 *****************************************************************************/
/* enums for output and input stream and voice call stream */
enum stream_id {
    STREAM_OUT,
    STREAM_IN,
    STREAM_CALL,
    STREAM_MAX
};

/* enums for alsa devices */
enum alsa_dev_id {
    DEV_ID_MUSIC,
    DEV_ID_BT_SCO,
    DEV_ID_AUX,
    DEV_ID_WFD,
    DEV_ID_NON_CALL_MAX,
    DEV_ID_CALL = DEV_ID_NON_CALL_MAX,
    DEV_ID_BT_CALL,
    DEV_ID_MAX
};

enum path_parse_status {
    PATH_UNINIT,
    PATH_INIT,
    PATH_ON,
    PATH_OFF
};

enum audio_sample_rates {
    RATE_8000 = (1 << 0),
    RATE_11025 = (1 << 1),
    RATE_12000 = (1 << 2),
    RATE_16000 = (1 << 3),
    RATE_22050 = (1 << 4),
    RATE_24000 = (1 << 5),
    RATE_32000 = (1 << 6),
    RATE_44100 = (1 << 7),
    RATE_48000 = (1 << 8),
    RATE_64000 = (1 << 9),
    RATE_88200 = (1 << 10),
    RATE_96000 = (1 << 11)
};

 enum tegra_audio_channel_id {
    LF = 1,                           // Left Front
    RF,                               // Right Front
    CF,                               // Center Front
    LS,                               // Left Surround
    RS,                               // Right Surround
    LFE,                              // Low Frequency Effect
    CS,                               // Center Surround
    LR,                               // Rear Left
    RR,                               // Rear Right
    TEGRA_AUDIO_CHANNEL_ID_LAST
};

/*****************************************************************************
 * Structures
 *****************************************************************************/
static const uint32_t tegra_default_ch_map[8] = {
    (LF << 0),
    (LF << 0) | (RF << 4),
    (LF << 0) | (RF << 4) | (CF << 8),
    (LF << 0) | (RF << 4) | (LS << 8) | (RS << 12),
    (LF << 0) | (RF << 4) | (CF << 8) | (LS << 12) | (RS << 16),
    (LF << 0) | (RF << 4) | (CF << 8) | (LFE << 12) | (LS << 16) | (RS << 20),
    (LF << 0) | (RF << 4) | (CF << 8) | (LS << 12) | (RS << 16) | (LR << 20) |\
    (RR << 24),
    (LF << 0) | (RF << 4) | (CF << 8) | (LFE << 12) | (LS << 16) | (RS << 20) |\
    (LR << 24) | (RR << 28)
};

// HDA channel order
static const uint32_t tegra_dest_ch_map[8] = {
    (LF << 0),
    (LF << 0) | (RF << 4),
    (LF << 0) | (RF << 4) | (CF << 8),
    (LF << 0) | (RF << 4) | (LS << 8) | (RS << 12),
    (LF << 0) | (RF << 4) | (LS << 8) | (RS << 12) | (CF << 16),
    (LF << 0) | (RF << 4) | (LS << 8) | (RS << 12) | (CF << 16) | (LFE << 20),
    (LF << 0) | (RF << 4) | (LR << 8) | (RR << 12) | (CF << 16) | (LS << 20) |\
    (RS << 24),
    (LF << 0) | (RF << 4) | (LR << 8) | (RR << 12) | (CF << 16) | (LFE << 20) |\
    (LS << 24) | (RS << 28)
};

static const uint32_t tegra_stereo_dwn_mix_ch_map[8] = {
    (LF << 0),
    (LF << 0) | (RF << 4),
    (CF << 0) | (LF << 4) | (RF << 8),
    (LS << 0) | (LF << 4) | (RF << 8) | (RS << 12),
    (CF << 0) | (LF << 4) | (RF << 8) | (LS << 12) | (RS << 16),
    (CF << 0) | (LF << 4) | (RF << 8) | (LS << 12) | (RS << 16) | (LFE << 20),
    (CF << 0) | (LF << 4) | (RF << 8) | (LR << 12) | (RR << 16) | (LS << 20) |\
    (RS << 24),
    (CF << 0) | (LF << 4) | (RF << 8) | (LR << 12) | (RR << 16) | (LS << 20) |\
    (RS << 24) |(LFE << 28)
};

static const char* tegra_audio_chid_str[TEGRA_AUDIO_CHANNEL_ID_LAST] =
{"", "LF", "RF", "CF", "LS", "RS", "LFE", "CS", "LR", "RR"};


static const uint32_t ch_mask[8] =
{ 0xF, 0xFF, 0xFFF, 0xFFFF, 0xFFFFF, 0xFFFFFF, 0xFFFFFFF, 0xFFFFFFFF};

/* mapping between alsa device names and id */
static const struct {
    enum alsa_dev_id id;
    const char *name;
} alsa_dev_names[] = {
    {DEV_ID_MUSIC, "music" },
    {DEV_ID_BT_SCO, "voice" },
    {DEV_ID_AUX, "aux" },
    {DEV_ID_WFD, "wfd" },
    {DEV_ID_CALL, "voice-call" },
    {DEV_ID_BT_CALL, "bt-voice-call" },
};

/* mapping between audio device mask and name */
static const struct {
    int devices;
    const char *name;
} audio_device_names[] = {
    /* playback devices*/
    { AUDIO_DEVICE_OUT_SPEAKER |
      AUDIO_DEVICE_OUT_DEFAULT, "speaker" },
    {AUDIO_DEVICE_OUT_EARPIECE, "earpiece"},
    { AUDIO_DEVICE_OUT_WIRED_HEADSET |
      AUDIO_DEVICE_OUT_WIRED_HEADPHONE, "headphone" },
    { AUDIO_DEVICE_OUT_ALL_SCO , "bt-sco" },
    { AUDIO_DEVICE_OUT_AUX_DIGITAL, "aux" },
#ifdef FRAMEWORK_HAS_WIFI_DISPLAY_SUPPORT
    { AUDIO_DEVICE_OUT_REMOTE_SUBMIX, "wfd" },
#endif

    /* capture devices*/
    { AUDIO_DEVICE_IN_BUILTIN_MIC |
      AUDIO_DEVICE_IN_AMBIENT |
      AUDIO_DEVICE_IN_BACK_MIC |
      AUDIO_DEVICE_IN_DEFAULT, "builtin-mic" },
    { AUDIO_DEVICE_IN_WIRED_HEADSET, "headset-mic" },
    { AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET, "bt-sco-mic" },
};

struct alsa_dev_cfg {
    struct alsa_dev             *dev;
    uint32_t                    devices;
    bool                        on;

    list_handle                 on_route_list;
    list_handle                 off_route_list;
    struct at_command           *at_cmd;
};

struct config_parse_state {
    struct nvaudio_hw_device    *hw_dev;
    struct alsa_dev             *dev;
    struct alsa_dev_cfg         *dev_cfg;

    list_handle                 init_route_list;
    char                        *dev_type;
    enum path_parse_status      path_status;
};

struct route_ctls {
    char                        *ctl_name;
    int                         int_val;
    char                        *str_val;
};

struct at_command {
    char                        *node;
    char                        *command;
};

struct alsa_dev {
    struct pcm_config           default_config;
    struct pcm_config           config;
    struct pcm                  *pcm;
    struct mixer                *mixer;
    pthread_mutex_t             lock;
    pthread_cond_t               cond;

    enum alsa_dev_id            id;
    enum audio_sample_rates     hw_rates;

    list_handle                 dev_cfg_list;
    list_handle                 on_route_list;
    list_handle                 off_route_list;
    list_handle                 active_stream_list;

    uint32_t                    devices;
    uint32_t                    card;
    uint32_t                    device_id;
    char                        *card_name;
    uint32_t                    pcm_flags;

    avp_stream_handle_t         avp_stream;
    bool                        is_i2s_master;
    bool                        use_avp_src;
    bool                        ulp_supported;
    int                         ref_count;
    int                         mode;
    int16_t    *mix_buffer;
    uint32_t    mix_buff_size;
    volatile uint32_t    buff_cnt;

    bool                    is_pass_thru;
    int                     rec_dec_capable;
};

/*****************************************************************************
 * nvaudio_hw_device
 *****************************************************************************/
struct nvaudio_hw_device {
    audio_hw_device_t           hw_device;
    pthread_mutex_t             lock;

    int                         mode;
    uint32_t                    call_devices;
    uint32_t                    call_devices_capture;
    char                        lp_state_default[4];
    bool                        mic_mute;
    uint32_t                    media_devices;
    uint32_t                    effects_count;
    uint32_t                    dev_available;

    list_handle                 dev_list[STREAM_MAX];
    list_handle                 stream_out_list;
    list_handle                 stream_in_list;

    avp_audio_handle_t          avp_audio;
    avp_stream_handle_t         avp_active_stream;
};

/*****************************************************************************
 * nvaudio_stream_out
 *****************************************************************************/
struct nvaudio_stream_out {
    struct audio_stream_out     stream;
    struct nvaudio_hw_device    *hw_dev;
    pthread_mutex_t             lock;

    struct resampler_itfe       *resampler[DEV_ID_NON_CALL_MAX];
    int16_t                     *buffer[DEV_ID_NON_CALL_MAX];
    int16_t                     *resampling_buffer[DEV_ID_NON_CALL_MAX];
    uint32_t                    buff_pos[DEV_ID_NON_CALL_MAX];

    list_handle                 dev_list;

    audio_io_handle_t           io_handle;
    uint32_t                    rate;
    uint32_t                    channels;
    uint32_t                    channel_count;
    audio_format_t              format;
    uint32_t                    devices;
    uint32_t                    frame_count;
    uint32_t                    volume[MAX_SUPPORTED_CHANNELS];

    avp_stream_handle_t         avp_stream;
    bool                        standby;
};

/*****************************************************************************
 * nvaudio_stream_in
 *****************************************************************************/
struct nvaudio_stream_in {
    struct audio_stream_in      stream;
    struct nvaudio_hw_device    *hw_dev;
    struct alsa_dev             *dev;
    pthread_mutex_t             lock;

    struct resampler_itfe       *resampler;
    int16_t                     *buffer;
    uint32_t                    buffer_offset;

    audio_io_handle_t           io_handle;
    uint32_t                    rate;
    uint32_t                    channels;
    uint32_t                    channel_count;
    audio_format_t              format;
    uint32_t                    source;
    uint32_t                    devices;

    bool                        standby;
};

#endif // LIBAUDIO_NVAUDIO_H
