/*
 * Copyright (C) 2010-2012 The Android Open Source Project
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

/*
 * Copyright (c) 2012-2013, NVIDIA CORPORATION.  All rights reserved.
 */

#ifndef __NVFB_H__
#define __NVFB_H__ 1

#define DEBUG_SYNC 0
#if DEBUG_SYNC
#define SYNCLOG(...) ALOGD("Event Sync: "__VA_ARGS__)
#else
#define SYNCLOG(...)
#endif

/* Stereo modes  TODO FIXME Once these defines are
 * added to the kernel header files (waiting on Erik@Google),
 * then these lines can be removed */
#ifndef FB_VMODE_STEREO_NONE
#define FB_VMODE_STEREO_NONE        0x00000000  /* not stereo */
#define FB_VMODE_STEREO_FRAME_PACK  0x01000000  /* frame packing */
#define FB_VMODE_STEREO_TOP_BOTTOM  0x02000000  /* top-bottom */
#define FB_VMODE_STEREO_LEFT_RIGHT  0x04000000  /* left-right */
#define FB_VMODE_STEREO_MASK        0xFF000000
#endif

#define NVFB_MAX_WINDOWS 6
#define NVFB_WHOLE_TO_FX_20_12(x)    ((x) << 12)

#define NVFB_WINDOW_CAP_YUV_FORMATS    0x1
#define NVFB_WINDOW_CAP_SCALE          0x2
#define NVFB_WINDOW_CAP_16BIT          0x4
#define NVFB_WINDOW_CAP_SWAPXY         0x8
/* Do not allow >2x downscaling in DC as it is
   not reliable and inefficient in memory BW
   perspective, bug 395578 */
#define NVFB_WINDOW_DC_MAX_INC         2
#define NVFB_WINDOW_DC_MIN_SCALE       (1.0 / NVFB_WINDOW_DC_MAX_INC)

// XXX associated functionality will be "dissolved" into caps
#define NVFB_WINDOW_B_INDEX             1
#define NVFB_WINDOW_C_INDEX             2

#define NVFB_WINDOW_B_MASK              (1 << NVFB_WINDOW_B_INDEX)
#define NVFB_WINDOW_C_MASK              (1 << NVFB_WINDOW_C_INDEX)

enum NvFbVideoPolicy {
    NvFbVideoPolicy_Exact,
    NvFbVideoPolicy_Closest,
    NvFbVideoPolicy_RoundUp,
    NvFbVideoPolicy_Stereo_Exact,
    NvFbVideoPolicy_Stereo_Closest,
};

struct nvfb_device;

struct nvfb_config {
    int index;
    int res_x;
    int res_y;
    int dpi_x;
    int dpi_y;
    int period;
};

struct nvfb_display_caps {
    size_t num_windows;
    uint_t window_cap[NVFB_MAX_WINDOWS];
};

struct nvfb_sync {
    NvRmFence fence;
};

struct nvfb_hotplug_status {
    union {
        struct {
            unsigned connected   : 1;  // HDMI device connected bit
            unsigned stereo      : 1;  // HDMI device stereo capable bit
            unsigned demo        : 1;  // HDMI "demo" mode bit
            unsigned rotation    : 9;  // Device rotation in degrees
            unsigned generation  : 16; // hotplug generation counter
        };
        uint32_t value;
    };
};

struct nvfb_window {
    int window_index;
    int blend;
    int transform;
    NvRect src;
    NvRect dst;
    NvPoint offset;
    int surf_index;
};

struct nvfb_window_config {
    int protect;
    struct fb_var_screeninfo *mode;
    struct nvfb_hotplug_status hotplug;
    struct nvfb_window overlay[NVFB_MAX_WINDOWS];
};

struct nvfb_buffer {
    NvNativeHandle *buffer;
    struct nvfb_sync sync;
};

typedef struct nvfb_window NvGrOverlayDesc;
typedef struct nvfb_window_config NvGrOverlayConfig;

struct nvfb_callbacks {
    void *data;
    int (*hotplug)(void *data, int disp, struct nvfb_hotplug_status hotplug);
    int (*acquire)(void *data, int disp);
    int (*release)(void *data, int disp);
};

int nvfb_open(struct nvfb_device **dev, struct nvfb_callbacks *callbacks,
              int *display_mask, struct nvfb_hotplug_status *hotplug);
void nvfb_close(struct nvfb_device *dev);

void nvfb_get_display_caps(struct nvfb_device *dev, int disp,
                           struct nvfb_display_caps *caps);

int nvfb_config_get_count(struct nvfb_device *dev, int disp, size_t *count);
int nvfb_config_get(struct nvfb_device *dev, int disp, uint32_t index,
                    struct nvfb_config *config);
int nvfb_config_get_current(struct nvfb_device *dev, int disp,
                            struct nvfb_config *config);

int nvfb_post(struct nvfb_device *dev, int disp, NvGrOverlayConfig *conf,
              struct nvfb_buffer *bufs, struct nvfb_sync *post_sync);
int nvfb_blank(struct nvfb_device *dev, int disp, int blank);
void nvfb_vblank_wait(struct nvfb_device *dev);
void nvfb_dump(struct nvfb_device *dev, char *buff, int buff_len);

struct fb_var_screeninfo *nvfb_choose_display_mode(struct nvfb_device *dev,
                                                   int disp, int xres, int yres,
                                                   enum NvFbVideoPolicy policy);
int nvfb_set_display_mode(struct nvfb_device *dev, int disp,
                          struct fb_var_screeninfo *mode);

#endif /* __NVFB_H__ */
