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

#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <linux/fb.h>
#include <linux/tegra_dc_ext.h>
#include <sys/select.h>
#include <sys/stat.h>

#include <cutils/log.h>
#include <cutils/properties.h>

#include "nvhwc.h"
#include "nvhwc_debug.h"
#include "nvassert.h"
#include "nvfb_hdcp.h"
#include "no_capture.h"

/*********************************************************************
 *   Options
 */

/*
 * Video capture
 */
#ifndef NVCAP_VIDEO_ENABLED
#define NVCAP_VIDEO_ENABLED 0
#endif

#if NVCAP_VIDEO_ENABLED
#include <dlfcn.h>
#include "nvcap_video.h"
#endif

/* hwcomposer v1.2 uses only the first config and provides no
 * interface for selecting other configs.  The code as written is
 * forward looking and exposes all configs, which satisfies the needs
 * of the current version if only the first config is used.  Rather
 * than re-ordering configs if we want to use a different mode, this
 * ifdef modifies the behavior to expose only the current config.
 *
 * This ifdef'ed behavior is therefore a workaround for the
 * limitations of hwcomposer 1.2 and will be removed once we drop
 * support for this version.
 */
#define LIMIT_CONFIGS 1

/*********************************************************************
 *   Definitions
 */

#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

enum DisplayType {
    NVFB_DISPLAY_PANEL = 0,
    NVFB_DISPLAY_HDMI,
#if NVCAP_VIDEO_ENABLED
    NVFB_DISPLAY_WFD,
#endif
    NVFB_MAX_DISPLAYS,
};

enum DispMode {
    DispMode_None = -1,
    DispMode_DMT0659 = 0,
    DispMode_480pH,
    DispMode_576pH,
    DispMode_720p,
    DispMode_720p_stereo,
    DispMode_1080p,
    DispMode_1080p_stereo,
    DispMode_Count,
};

/* CEA-861 specifies a maximum of 65 modes in an EDID */
#define CEA_MODEDB_SIZE 65

struct nvfb_display {
    enum DisplayType type;

    /* Current state */
    int connected;
    int blank;

    struct nvfb_display_caps caps;

    /* Device interface */
    union {
        struct {
            struct {
                int fd;
                uint32_t handle;
            } dc;
            struct {
                int fd;
            } fb;
        };
#if NVCAP_VIDEO_ENABLED
        struct {
            void *handle;
            struct nvcap_video *ctx;
            struct nvcap_interface *procs;
        } nvcap;
#endif
    };

    /* Mode table */
    struct {
        size_t num_modes;
        struct fb_var_screeninfo modedb[CEA_MODEDB_SIZE];
        struct fb_var_screeninfo *modes[CEA_MODEDB_SIZE];
        struct fb_var_screeninfo *table[DispMode_Count];
        struct fb_var_screeninfo *current;
        unsigned stereo : 1;
        unsigned dirty  : 1;
    } mode;

    struct {
        NvU32 period;
        NvU32 timeout;
        NvU32 syncpt;
    } vblank;

    struct {
        int (*blank)(struct nvfb_display *dpy, int blank);
        int (*post)(struct nvfb_device *dev, struct nvfb_display *dpy,
                    struct nvfb_window_config *conf, struct nvfb_buffer *bufs,
                    struct nvfb_sync *post_sync);
        int (*set_mode)(struct nvfb_display *dpy,
                        struct fb_var_screeninfo *mode);
        void (*vblank_wait)(struct nvfb_device *dev, struct nvfb_display *dpy);
    } procs;
};

struct nvfb_device {
    int ctrl_fd;

    uint32_t display_mask;
    struct nvfb_display displays[NVFB_MAX_DISPLAYS];
    struct nvfb_display *active_display[HWC_NUM_DISPLAY_TYPES];
    struct nvfb_display *vblank_display;

    struct {
        pthread_mutex_t mutex;
        pthread_t thread;
        int pipe[2];
        struct nvfb_hotplug_status status;
    } hotplug;

    struct nvfb_hdcp *hdcp;
    NvNativeHandle *no_capture;

    struct nvfb_callbacks callbacks;

    NvGrModule *gralloc;

    int null_display;
};

static const int default_xres = 1280;
static const int default_yres = 720;
static const float default_refresh = 60;

static const char TEGRA_CTRL_PATH[] = "/dev/tegra_dc_ctrl";

/*********************************************************************
 *   VSync
 */

#define REFRESH_TO_PERIOD_NS(refresh) (1000000000.0 / refresh)

static inline uint64_t fbmode_calc_period_ps(struct fb_var_screeninfo *info)
{
    return (uint64_t)
        (info->upper_margin + info->lower_margin + info->vsync_len + info->yres)
        * (info->left_margin + info->right_margin + info->hsync_len + info->xres)
        * info->pixclock;
}

static inline int fbmode_calc_period_ns(struct fb_var_screeninfo *info)
{
    uint64_t period = fbmode_calc_period_ps(info);

    return (int) (period ? (period / 1000.0) :
                  REFRESH_TO_PERIOD_NS(default_refresh));
}

static void update_vblank_period(struct nvfb_display *dpy)
{
    dpy->vblank.period = fbmode_calc_period_ns(dpy->mode.current);
    ALOGD("display type %d: vblank period = %u ns", dpy->type,
          dpy->vblank.period);

    if (dpy->vblank.syncpt != NVRM_INVALID_SYNCPOINT_ID) {
        /* Calculate a vblank syncpoint wait timeout in MS. */
        NvU32 timeout = (dpy->vblank.period + 500000) / 1000000;

        /* Add 10ms to the vblank interval to cover jiffies resolution
         * in nvhost.
         */
        timeout += 10;

        dpy->vblank.timeout = timeout;
    }
}

static void update_vblank_display(struct nvfb_device *dev)
{
    struct nvfb_display *primary = dev->active_display[HWC_DISPLAY_PRIMARY];
    struct nvfb_display *external = dev->active_display[HWC_DISPLAY_EXTERNAL];
    struct nvfb_display *vblank = primary;

    /* set vblank trigger to external display when external display's
     * refresh rate is lower than 90% of internal's
     */
    if (external && external->connected && !external->blank) {
        const float ratio = 0.9f;

        if (external->vblank.period * ratio > primary->vblank.period) {
            vblank = external;
        }
    }

    NV_ASSERT(vblank);
    if (vblank != dev->vblank_display) {
        ALOGD("Setting vblank display to %s, new vblank period %u ns",
              vblank == primary ? "Internal" : "External",
              vblank->vblank.period);
        dev->vblank_display = vblank;
    }
}

static void dc_vblank_wait_fb(struct nvfb_device *dev,
                              struct nvfb_display *dpy)
{
    SYNCLOG("FBIO_WAITFORVSYNC");
    if (ioctl(dpy->fb.fd, FBIO_WAITFORVSYNC) < 0) {
        ALOGE("%s: vblank ioctl failed: %s", __FUNCTION__, strerror(errno));
    }
}

static void dc_vblank_wait_syncpt(struct nvfb_device *dev,
                                  struct nvfb_display *dpy)
{
    NvGrModule *m = (NvGrModule*) dev->gralloc;
    NvRmFence fence = {
        .SyncPointID = dpy->vblank.syncpt,
        .Value = 1 + NvRmChannelSyncPointRead(m->Rm, fence.SyncPointID),
    };
    NvError err;

    /* Wait for the next VBLANK */
    SYNCLOG("Waiting for syncpoint %d value %d",
            fence.SyncPointID, fence.Value);

    err = NvRmFenceWait(m->Rm, &fence, dpy->vblank.timeout);
    switch (err) {
    case NvSuccess:
        break;
    case NvError_Timeout:
        SYNCLOG("%s: NvRmFenceWait timed out.", __FUNCTION__);
        break;
    default:
        ALOGE("%s: NvRmFenceWait returned %d", __FUNCTION__, err);
        break;
    }
}

static void vblank_wait_sleep(struct nvfb_device *dev,
                              struct nvfb_display *dpy)
{
    struct timespec interval = {
        .tv_sec = 0,
        .tv_nsec = dpy->vblank.period,
    };

    nanosleep(&interval, NULL);
}

void nvfb_vblank_wait(struct nvfb_device *dev)
{
    struct nvfb_display *dpy = dev->vblank_display;

    NV_ASSERT(dpy);
    if (dpy->blank) {
        return;
    }

    dpy->procs.vblank_wait(dev, dpy);
}


/*********************************************************************
 *   Modes
 */

/* Property value can be one of
   "Auto", "Max", "1080p", "720p", "576p", "480p", "Vga" */
#define NV_PROPERTY_HDMI_RESOLUTION         TEGRA_PROP(hdmi.resolution)
#ifndef NV_PROPERTY_HDMI_RESOLUTION_DEF
#define NV_PROPERTY_HDMI_RESOLUTION_DEF     Auto
#endif

/* Minimum resolution which can be set in Auto mode */
#define HDMI_MIN_RES_IN_AUTO DispMode_720p

static float fbmode_calc_refresh(struct fb_var_screeninfo *info)
{
    uint64_t time_ps = fbmode_calc_period_ps(info);

    return time_ps ? (float)1000000000000LLU / time_ps : 60.f;
}

struct nvfb_mode_table {
    enum DispMode index;
    __u32 xres;
    __u32 yres;
    float max_refresh;
    NvBool stereo;
};

static const struct nvfb_mode_table builtin_modes[] = {
    { DispMode_DMT0659,       640,  480, 60, NV_FALSE },
    { DispMode_480pH,         720,  480, 60, NV_FALSE },
    { DispMode_576pH,         720,  576, 60, NV_FALSE },
    { DispMode_720p,         1280,  720, 60, NV_FALSE },
    { DispMode_720p_stereo,  1280,  720, 60, NV_TRUE  },
    { DispMode_1080p,        1920, 1080, 60, NV_FALSE },
    { DispMode_1080p_stereo, 1920, 1080, 60, NV_TRUE  },
};

static const size_t builtin_mode_count =
    sizeof(builtin_modes) / sizeof(builtin_modes[0]);

typedef enum {
    HdmiResolution_Vga = DispMode_DMT0659,
    HdmiResolution_480p = DispMode_480pH,
    HdmiResolution_576p = DispMode_576pH,
    HdmiResolution_720p = DispMode_720p,
    HdmiResolution_1080p = DispMode_1080p,
    HdmiResolution_Max = DispMode_Count,
    HdmiResolution_Auto,
} HdmiResolution;

static inline int is_stereo_mode(struct fb_var_screeninfo *mode)
{
    return mode && (mode->vmode & (FB_VMODE_STEREO_FRAME_PACK |
                                   FB_VMODE_STEREO_LEFT_RIGHT |
                                   FB_VMODE_STEREO_TOP_BOTTOM));
}

static HdmiResolution get_hdmi_resolution(struct nvfb_display *dpy)
{
    char prop_value[PROPERTY_VALUE_MAX];
    char *value;
    HdmiResolution hdmi_res, hdmi_res_default;

#define TOKEN(x) HdmiResolution_ ## x
#define HDMI_RES(x) TOKEN(x)
    hdmi_res = hdmi_res_default = HDMI_RES(NV_PROPERTY_HDMI_RESOLUTION_DEF);
#undef DISP_MODE
#undef TOKEN

    value = getenv(NV_PROPERTY_HDMI_RESOLUTION);
    if (!value && property_get(NV_PROPERTY_HDMI_RESOLUTION, prop_value, "") > 0) {
        value = prop_value;
    }
    if (value) {
        if (!strcmp(value, "Auto"))
            hdmi_res = HdmiResolution_Auto;
        else if (!strcmp(value, "Max"))
            hdmi_res = HdmiResolution_Max;
        else if (!strcmp(value, "1080p"))
            hdmi_res = HdmiResolution_1080p;
        else if (!strcmp(value, "720p"))
            hdmi_res = HdmiResolution_720p;
        else if (!strcmp(value, "576p"))
            hdmi_res = HdmiResolution_576p;
        else if (!strcmp(value, "480p"))
            hdmi_res = HdmiResolution_480p;
        else if (!strcmp(value, "Vga"))
            hdmi_res = HdmiResolution_Vga;
        else
            ALOGW("Invalid HDMI resolution property, use default %d\n",
                hdmi_res_default);

        /* fall back to default resolution policy if the specified mode
           is not supported */
        if ((hdmi_res < HdmiResolution_Max) && !dpy->mode.table[hdmi_res]) {
            ALOGW("Preferred HDMI resolution %d is not supported, use default %d\n",
                hdmi_res, hdmi_res_default);
            hdmi_res = hdmi_res_default;
        }
    }

    /* fall back to Max resolution policy if there is no supported resolution
       equal to or higher than minimum when Auto mode is set, so that next
       highest resolution can be set */
    if (hdmi_res == HdmiResolution_Auto) {
        int ii;
        int high_res_supported = 0;

        for (ii = HDMI_MIN_RES_IN_AUTO; ii < DispMode_Count; ii++) {
            if (dpy->mode.table[ii]) {
                high_res_supported = 1;
                break;
            }
        }

        if (!high_res_supported) {
            ALOGW("No supported resolution higher than %d, use Max resolution\n",
                HDMI_MIN_RES_IN_AUTO);
            hdmi_res = HdmiResolution_Max;
        }
    }

    return hdmi_res;
}

static enum DispMode
GetDisplayMode_Exact(struct nvfb_display *dpy, unsigned xres, unsigned yres,
                     NvBool stereo)
{
    int ii;

    for (ii = HDMI_MIN_RES_IN_AUTO; ii < DispMode_Count; ii++) {
        NvBool stereo_mode;

        if (!dpy->mode.table[ii]) continue;

        stereo_mode = is_stereo_mode(dpy->mode.table[ii]);
        if (stereo != stereo_mode) continue;

        if (dpy->mode.table[ii]->xres == xres &&
            dpy->mode.table[ii]->yres == yres) {
            return (enum DispMode) ii;
        }
    }

    return DispMode_None;
}

static enum DispMode
GetDisplayMode_Closest(struct nvfb_display *dpy, unsigned xres, unsigned yres,
                       NvBool stereo)
{
    int ii;
    enum DispMode best = DispMode_None;
    unsigned best_dx = ~0;
    unsigned best_dy = ~0;

    for (ii = HDMI_MIN_RES_IN_AUTO; ii < DispMode_Count; ii++) {
        unsigned dx, dy;
        NvBool stereo_mode;

        if (!dpy->mode.table[ii]) continue;

        stereo_mode = is_stereo_mode(dpy->mode.table[ii]);
        if (stereo != stereo_mode) continue;

        dx = ABS((int)dpy->mode.table[ii]->xres - (int)xres);
        dy = ABS((int)dpy->mode.table[ii]->yres - (int)yres);

        if (dx < best_dx || dy < best_dy) {
            best = (enum DispMode) ii;
            best_dx = dx;
            best_dy = dy;
        }
    }

    return best;
}

static enum DispMode
GetDisplayMode_RoundUp(struct nvfb_display *dpy, unsigned xres, unsigned yres)
{
    int ii;
    enum DispMode best = DispMode_None;

    for (ii = HDMI_MIN_RES_IN_AUTO; ii < DispMode_Count; ii++) {

        if (!dpy->mode.table[ii]) continue;
        if (is_stereo_mode(dpy->mode.table[ii])) continue;

        if (xres <= dpy->mode.table[ii]->xres &&
            yres <= dpy->mode.table[ii]->yres) {
            return (enum DispMode) ii;
        } else {
            best = (enum DispMode) ii;
        }
    }

    return best;
}

static enum DispMode GetDisplayMode_Highest(struct nvfb_display *dpy)
{
    int ii;

    for (ii = DispMode_Count - 1; ii >= 0; ii--) {

        if (dpy->mode.table[ii] && !is_stereo_mode(dpy->mode.table[ii]))
            return (enum DispMode) ii;
    }

    return DispMode_None;
}

struct fb_var_screeninfo *
nvfb_choose_display_mode(struct nvfb_device *dev, int disp, int xres, int yres,
                         enum NvFbVideoPolicy policy)
{
    struct nvfb_display *dpy = dev->active_display[disp];
    enum DispMode mode = DispMode_None;
    HdmiResolution hdmi_res;

    if (!dpy) {
        return NULL;
    }

    hdmi_res = get_hdmi_resolution(dpy);

    if (hdmi_res == HdmiResolution_Auto ||
        policy == NvFbVideoPolicy_Stereo_Exact ||
        policy == NvFbVideoPolicy_Stereo_Closest) {

        switch (policy) {
        case NvFbVideoPolicy_Exact:
            mode = GetDisplayMode_Exact(dpy, xres, yres, NV_FALSE);
            break;
        case NvFbVideoPolicy_Closest:
            mode = GetDisplayMode_Closest(dpy, xres, yres, NV_FALSE);
            break;
        case NvFbVideoPolicy_RoundUp:
            mode = GetDisplayMode_RoundUp(dpy, xres, yres);
            break;
        case NvFbVideoPolicy_Stereo_Closest:
            mode = GetDisplayMode_Closest(dpy, xres, yres, NV_TRUE);
            break;
        case NvFbVideoPolicy_Stereo_Exact:
            mode = GetDisplayMode_Exact(dpy, xres, yres, NV_TRUE);
            break;
        }
    }
    else if (hdmi_res == HdmiResolution_Max) {
        mode = GetDisplayMode_Highest(dpy);
    }
    else {
        mode = (enum DispMode)hdmi_res;
    }

    if (mode != DispMode_None) {
        return dpy->mode.table[mode];
    } else {
        return dpy->mode.current;
    }
}

static void create_fake_mode(struct fb_var_screeninfo *mode,
                             int xres, int yres, float refresh)
{
    memset(mode, 0, sizeof(*mode));
    mode->xres = xres;
    mode->yres = yres;
    mode->pixclock = REFRESH_TO_PERIOD_NS(refresh) * 1000 / (xres * yres);
}

static int dc_set_mode(struct nvfb_display *dpy,
                       struct fb_var_screeninfo *mode)
{
    if (dpy->type != NVFB_DISPLAY_PANEL) {
        /* There is a known display bug where modes get out of sync.
         * The workaround is to blank and unblank around the mode set.
         * This can be removed once it is fixed.
         */
        if (ioctl(dpy->fb.fd, FBIOBLANK, FB_BLANK_POWERDOWN)) {
            ALOGE("FB_BLANK_POWERDOWN failed: %s", strerror(errno));
            return errno;
        }
    }

    if (ioctl(dpy->fb.fd, FBIOPUT_VSCREENINFO, mode)) {
        ALOGE("%s: FBIOPUT_VSCREENINFO: %s", __FUNCTION__, strerror(errno));
        return errno;
    }

    if (dpy->type != NVFB_DISPLAY_PANEL) {
        /* This is currently redundant - setting the mode implicitly
         * unblanks.  That may change however, so explicitly unblank here
         * since we explicitly blanked above.
         */
        if (ioctl(dpy->fb.fd, FBIOBLANK, FB_BLANK_UNBLANK)) {
            ALOGE("FB_BLANK_UNBLANK failed: %s", strerror(errno));
            return errno;
        }

        dpy->blank = 0;
    }

    return 0;
}

static int dc_set_mode_deferred(struct nvfb_display *dpy,
                                struct fb_var_screeninfo *mode)
{
    dpy->mode.current = mode;
    dpy->mode.dirty = 1;
    update_vblank_period(dpy);

    return 0;
}

static int nvfb_update_display_mode(struct nvfb_device *dev,
                                    struct nvfb_display *dpy)
{
    struct fb_var_screeninfo *mode;
    int ret;

    mode = dpy->mode.current;

    /* Only DC displays set the dirty bit, therefore this function
     * cannot be reached from other display types.
     */
    NV_ASSERT(dpy->type == NVFB_DISPLAY_PANEL ||
              dpy->type == NVFB_DISPLAY_HDMI);

    ret = dc_set_mode(dpy, mode);
    if (ret) {
        return ret;
    }

    dpy->mode.dirty = 0;

    if (dpy->type != NVFB_DISPLAY_PANEL) {
        update_vblank_display(dev);
    }

    return 0;
}

int nvfb_set_display_mode(struct nvfb_device *dev, int disp,
                          struct fb_var_screeninfo *mode)
{
    struct nvfb_display *dpy = dev->active_display[disp];

    if (dpy && dpy->connected) {
        if (dpy->mode.current != mode) {
            if (dpy->type == NVFB_DISPLAY_HDMI) {
                update_vblank_display(dev);
            }
            return dpy->procs.set_mode(dpy, mode);
        }
        return 0;
    }

    return -1;
}

static void nvfb_assign_mode(struct nvfb_display *dpy,
                             struct fb_var_screeninfo *mode)
{
    const float refresh_epsilon = 0.01f;
    float refresh = fbmode_calc_refresh(mode);
    NvBool stereo = is_stereo_mode(mode);
    size_t ii;

    for (ii = 0; ii < builtin_mode_count; ii++) {
        enum DispMode index = builtin_modes[ii].index;

        if (builtin_modes[ii].xres == mode->xres &&
            builtin_modes[ii].yres == mode->yres &&
            builtin_modes[ii].stereo == stereo &&
            refresh <= (builtin_modes[ii].max_refresh + refresh_epsilon) &&
            (!dpy->mode.table[index] ||
             fbmode_calc_refresh(dpy->mode.table[index]) < refresh)) {
            dpy->mode.table[index] = mode;
            ALOGD("matched: %d x %d @ %0.2f Hz%s",
                  mode->xres, mode->yres, refresh,
                  stereo ? ", stereo" : "");
            return;
        }
    }
    ALOGD("unmatched: %d x %d @ %0.2f Hz%s",
          mode->xres, mode->yres, refresh,
          stereo ? ", stereo" : "");
}

static NvBool nvfb_modes_match(struct fb_var_screeninfo *a,
                               struct fb_var_screeninfo *b)
{

    #define MATCH(field) if (a->field != b->field) { return NV_FALSE; }

    /* Match resolution */
    MATCH(xres);
    MATCH(yres);

    /* Match timing */
    MATCH(pixclock);
    MATCH(left_margin);
    MATCH(right_margin);
    MATCH(upper_margin);
    MATCH(lower_margin);
    MATCH(hsync_len);
    MATCH(vsync_len);
    MATCH(sync);
    MATCH(vmode);
    MATCH(rotate);

    #undef MATCH

    return NV_TRUE;
}

static int nvfb_get_display_modes(struct nvfb_display *dpy)
{
    struct tegra_fb_modedb fb_modedb;
    size_t ii;

    /* Clear old modes */
    memset(&dpy->mode, 0, sizeof(dpy->mode));

    /* Query all supported modes */
    fb_modedb.modedb = dpy->mode.modedb;
    fb_modedb.modedb_len = sizeof(dpy->mode.modedb);
    if (ioctl(dpy->fb.fd, FBIO_TEGRA_GET_MODEDB, &fb_modedb)) {
        return errno;
    }

    ALOGD("Display %d: found %d modes", dpy->dc.handle,
          fb_modedb.modedb_len);

    /* Filter the modes */
    for (ii = 0; ii < fb_modedb.modedb_len; ii++) {
        struct fb_var_screeninfo *mode = &dpy->mode.modedb[ii];

        /* XXX - Skip modes with invalid timing. This indicates an
         * error in the board file.
         */
        if (!fbmode_calc_period_ps(mode)) {
            ALOGW("Skipping mode with invalid timing (%d x %d)",
                  mode->xres, mode->yres);
            continue;
        }

        /* Match a built-in mode */
        nvfb_assign_mode(dpy, mode);

        dpy->mode.modes[dpy->mode.num_modes++] = mode;
    }

    /* There should always be at least one mode */
    NV_ASSERT(dpy->mode.num_modes);

    /* Check for supported stereo modes */
    dpy->mode.stereo = 0;
    for (ii = 0; ii < builtin_mode_count; ii++) {
        if (builtin_modes[ii].stereo) {
            enum DispMode index = builtin_modes[ii].index;

            if (dpy->mode.table[index]) {
                dpy->mode.stereo = 1;
            }
        }
    }

    return 0;
}

static int nvfb_init_display_mode(struct nvfb_display *dpy)
{
    int ret;

    ret = nvfb_get_display_modes(dpy);
    if (ret) {
        return ret;
    }

    /* Set a default mode. */
    ret = dpy->procs.set_mode(dpy, dpy->mode.modes[0]);
    if (ret) {
        return ret;
    }

    return dpy->procs.blank(dpy, dpy->type != NVFB_DISPLAY_PANEL);
}

static void nvfb_mode_to_config(struct fb_var_screeninfo *mode,
                                struct nvfb_config *config)
{
    config->res_x = mode->xres;
    config->res_y = mode->yres;

    /* From hwcomposer_defs.h:
     *
     * The number of pixels per thousand inches of this configuration.
     */
    if ((int)mode->width <= 0 || (int)mode->height <= 0) {
        /* If the DPI for a configuration is unavailable or the HWC
         * implementation considers it unreliable, it should set these
         * attributes to zero.
         */
        config->dpi_x = 0;
        config->dpi_y = 0;
    } else {
        /* Scaling DPI by 1000 allows it to be stored in an int
         * without losing too much precision.
         */
        config->dpi_x = (int) (mode->xres * 25400.f / mode->width + 0.5f);
        config->dpi_y = (int) (mode->yres * 25400.f / mode->height + 0.5f);
    }

    config->period = fbmode_calc_period_ns(mode);
}

int nvfb_config_get_count(struct nvfb_device *dev, int disp, size_t *count)
{
    struct nvfb_display *dpy = dev->active_display[disp];

    if (dpy && dpy->connected) {
#if LIMIT_CONFIGS
        *count = 1;
#else
        *count = dpy->mode.num_modes;
#endif
        return 0;
    }

    return -1;
}

int nvfb_config_get(struct nvfb_device *dev, int disp, uint32_t index,
                    struct nvfb_config *config)
{
    struct nvfb_display *dpy = dev->active_display[disp];

#if LIMIT_CONFIGS
    if (dpy && dpy->connected && index == 0) {
        nvfb_mode_to_config(dpy->mode.current, config);
        return 0;
    }
#else
    if (dpy && dpy->connected && index < dpy->mode.num_modes) {
        nvfb_mode_to_config(dpy->mode.modes[index], config);
        return 0;
    }
#endif

    return -1;
}

int nvfb_config_get_current(struct nvfb_device *dev, int disp,
                            struct nvfb_config *config)
{
    struct nvfb_display *dpy = dev->active_display[disp];

    if (dpy && dpy->connected) {
        nvfb_mode_to_config(dpy->mode.current, config);
        return 0;
    }

    return -1;
}


/*********************************************************************
 *   Post
 */

static inline int convert_blend(int blend)
{
    switch (blend) {
    case HWC_BLENDING_PREMULT:
        return TEGRA_FB_WIN_BLEND_PREMULT;
    case HWC_BLENDING_COVERAGE:
        return TEGRA_FB_WIN_BLEND_COVERAGE;
    default:
        NV_ASSERT(blend == HWC_BLENDING_NONE);
        return TEGRA_FB_WIN_BLEND_NONE;
    }
}

static int SetWinAttr(struct tegra_dc_ext_flip_windowattr* w, int z,
                      struct nvfb_window *o, struct nvfb_buffer *buf)
{
    NvNativeHandle *h = buf ? buf->buffer : NULL;
    NvRmSurface* surf;

    w->index = o ? o->window_index : z;
    w->z = z;

    NV_ASSERT(w->index >= 0 && w->index < NVFB_MAX_WINDOWS);

    if (!h) {
        w->buff_id = 0;
        w->buff_id_u = 0;
        w->buff_id_v = 0;
        w->blend = TEGRA_FB_WIN_BLEND_NONE;
        w->flags = 0;
        return -1;
    }

    NV_ASSERT((int)h->SurfCount >= o->surf_index*2);
    surf = &h->Surf[o->surf_index];

    w->buff_id = (unsigned)surf->hMem;
    w->buff_id_u = 0;
    w->buff_id_v = 0;
    w->pre_syncpt_id = (unsigned)-1;

    w->x = NVFB_WHOLE_TO_FX_20_12(o->src.left + o->offset.x);
    w->y = NVFB_WHOLE_TO_FX_20_12(o->src.top + o->offset.y);
    w->w = NVFB_WHOLE_TO_FX_20_12(o->src.right - o->src.left);
    w->h = NVFB_WHOLE_TO_FX_20_12(o->src.bottom - o->src.top);
    w->out_x = o->dst.left;
    w->out_y = o->dst.top;
    w->out_w = o->dst.right - o->dst.left;
    w->out_h = o->dst.bottom - o->dst.top;

    /* Sanity check */
    NV_ASSERT(o->dst.right > o->dst.left);
    NV_ASSERT(o->dst.bottom > o->dst.top);

    w->stride = surf->Pitch;
    w->stride_uv = h->Surf[o->surf_index+1].Pitch;
    w->offset = surf->Offset;
    w->offset_u = h->Surf[o->surf_index+1].Offset;
    w->offset_v = h->Surf[o->surf_index+2].Offset;
    w->blend = convert_blend(o->blend);
    w->flags = 0;
    if (o->transform & HWC_TRANSFORM_FLIP_H) {
        w->flags |= TEGRA_FB_WIN_FLAG_INVERT_H;
    }
    if (o->transform & HWC_TRANSFORM_FLIP_V) {
        w->flags |= TEGRA_FB_WIN_FLAG_INVERT_V;
    }
    if (surf->Layout == NvRmSurfaceLayout_Tiled) {
        w->flags |= TEGRA_FB_WIN_FLAG_TILED;
    }

    w->pixformat = NvGrTranslateHalFmt(h->Format);

    NV_ASSERT((int)w->pixformat != -1);

    w->pre_syncpt_id  = buf->sync.fence.SyncPointID;
    w->pre_syncpt_val = buf->sync.fence.Value;

    return 0;
}

static int init_no_capture(struct nvfb_device *dev)
{
    NvGrModule *m = (NvGrModule*) dev->gralloc;
    uint8_t *src, *dst;
    int ii, err, format, elementSize;

    switch (image_nocapture_rgb.bpp) {
    case 16:
        format = HAL_PIXEL_FORMAT_RGB_565;
        elementSize = 2;
        break;
    case 32:
        format = HAL_PIXEL_FORMAT_RGBX_8888;
        elementSize = 4;
        break;
    default:
        return -1;
    }

    err = m->alloc_scratch(m,
                           image_nocapture_rgb.xsize,
                           image_nocapture_rgb.ysize,
                           format,
                           (GRALLOC_USAGE_SW_READ_NEVER |
                            GRALLOC_USAGE_SW_WRITE_RARELY |
                            GRALLOC_USAGE_HW_FB),
                           NvRmSurfaceLayout_Pitch,
                           &dev->no_capture);
    if (err) {
        return -1;
    }

    m->Base.lock(&m->Base,
                 (buffer_handle_t)dev->no_capture,
                 GRALLOC_USAGE_SW_WRITE_RARELY, 0, 0,
                 image_nocapture_rgb.xsize,
                 image_nocapture_rgb.ysize,
                 (void**)&dst);

    src = image_nocapture_rgb.buf;
    for (ii = 0; ii < image_nocapture_rgb.ysize; ii++) {
        memcpy(dst, src, image_nocapture_rgb.xsize * elementSize);
        src += elementSize * image_nocapture_rgb.xsize;
        dst += dev->no_capture->Surf[0].Pitch;
    }

    m->Base.unlock(&m->Base, (buffer_handle_t)dev->no_capture);

    return 0;
}

static void set_no_capture(struct nvfb_device *dev,
                           struct nvfb_display *dpy,
                           struct tegra_dc_ext_flip_windowattr *w)
{
    struct nvfb_window o;
    NvRmSurface *surf;
    struct nvfb_buffer buf;

    if (!dev->no_capture) {
        if (init_no_capture(dev)) {
            return;
        }
    }

    surf = &dev->no_capture->Surf[0];

    o.blend      = HWC_BLENDING_NONE;
    o.src.left   = 0;
    o.src.top    = 0;
    o.src.right  = surf->Width;
    o.src.bottom = surf->Height;
    o.surf_index = 0;
    o.window_index = 0;
    o.offset.x = 0;
    o.offset.y = 0;

    /* If the image fits within the display, center it; otherwise
     * scale to fit.
     */
    if (surf->Width  <= dpy->mode.current->xres &&
        surf->Height <= dpy->mode.current->yres) {
        o.dst.left   = (dpy->mode.current->xres - surf->Width)/2;
        o.dst.top    = (dpy->mode.current->yres - surf->Height)/2;
        o.dst.right  = o.dst.left + surf->Width;
        o.dst.bottom = o.dst.top  + surf->Height;
    } else {
        o.dst.left   = 0;
        o.dst.top    = 0;
        o.dst.right  = dpy->mode.current->xres;
        o.dst.bottom = dpy->mode.current->yres;
    }

    buf.buffer = dev->no_capture;
    buf.sync.fence.SyncPointID = NVRM_INVALID_SYNCPOINT_ID;
    SetWinAttr(w, 0, &o, &buf);
}

static int dc_post(struct nvfb_device *dev, struct nvfb_display *dpy,
                   struct nvfb_window_config *conf, struct nvfb_buffer *bufs,
                   struct nvfb_sync *post_sync)
{
    struct tegra_dc_ext_flip args;
    int show_warning = 0;
    size_t ii;

    NV_ASSERT(dpy->dc.fd >= 0);

    if (dpy->type == NVFB_DISPLAY_HDMI) {
        if (conf) {
            if (conf->hotplug.value != dev->hotplug.status.value) {
                conf = NULL;
            } else if (conf->protect) {
                enum nvfb_hdcp_status hdcp = nvfb_hdcp_get_status(dev->hdcp);

                if (hdcp != HDCP_STATUS_COMPLIANT) {
                    conf = NULL;
                }
                if (hdcp == HDCP_STATUS_NONCOMPLIANT) {
                    show_warning = 1;
                }
            }
        }
    } else {
        NV_ASSERT(dpy->type == NVFB_DISPLAY_PANEL);
    }

    if (dpy->mode.dirty) {
        nvfb_update_display_mode(dev, dpy);
    }

    memset(&args, 0, sizeof(args));
    for (ii = 0; ii < dpy->caps.num_windows; ii++) {
        struct tegra_dc_ext_flip_windowattr *w = &args.win[ii];

        if (conf) {
            SetWinAttr(w, ii, &conf->overlay[ii], &bufs[ii]);
        } else {
            SetWinAttr(w, ii, NULL, NULL);
        }
    }

    if (show_warning) {
        set_no_capture(dev, dpy, &args.win[0]);
    }

#if 0
    if (!conf->protect) {
        DebugBorders(m, &args);
        NvGrDumpOverlays(&dev->gralloc->Base, &dpy->Config, &args);
    }

    if (conf && dpy->overscan.use_overscan) {
        apply_underscan(dpy, w);
    }
#endif

    if (ioctl(dpy->dc.fd, TEGRA_DC_EXT_FLIP, &args) < 0) {
        if (errno != EPIPE) {
            // When display is suspended, this will fail with EPIPE.
            // Any other error is unexpected.
            ALOGE("%s: Display %d: TEGRA_DC_EXT_FLIP: %s\n", __FUNCTION__,
                  dpy->dc.handle, strerror(errno));
        }
        // bug 857053 - keep going but invalidate the syncpoint so we
        // don't wait for it.
        args.post_syncpt_id = NVRM_INVALID_SYNCPOINT_ID;
    }

    post_sync->fence.SyncPointID = args.post_syncpt_id;
    post_sync->fence.Value       = args.post_syncpt_val;

    return 0;
}

int nvfb_post(struct nvfb_device *dev, int disp, NvGrOverlayConfig *conf,
              struct nvfb_buffer *bufs, struct nvfb_sync *post_sync)
{
    struct nvfb_display *dpy = dev->active_display[disp];

    if (dpy && dpy->connected && (!dpy->blank || dpy->mode.dirty)) {
        dpy->procs.post(dev, dpy, conf, bufs, post_sync);
    }

    return 0;
}

static int dc_blank(struct nvfb_display *dpy, int blank)
{
    ALOGD("%s: display %d, [%d -> %d]", __FUNCTION__,
          dpy->type, dpy->blank, blank);

    if (dpy->blank != blank) {
        if (ioctl(dpy->fb.fd, FBIOBLANK,
                  blank ? FB_BLANK_POWERDOWN : FB_BLANK_UNBLANK)) {
            ALOGE("FB_BLANK_%s failed: %s",
                  blank ? "POWERDOWN" : "UNBLANK", strerror(errno));
            return -1;
        }

        dpy->blank = blank;
    }

    return 0;
}

int nvfb_blank(struct nvfb_device *dev, int disp, int blank)
{
    struct nvfb_display *dpy = dev->active_display[disp];

    if (dpy) {
        return dpy->procs.blank(dpy, blank);
    }

    return -1;
}

/*********************************************************************
 *   Hot Plug
 */

static void nvfb_hotplug(struct nvfb_device *dev, int connected)
{
    struct nvfb_display *dpy = dev->active_display[HWC_DISPLAY_EXTERNAL];

    dev->hotplug.status.connected = connected;
    dev->hotplug.status.stereo = connected ? dpy->mode.stereo : 0;
    dev->hotplug.status.generation++;
    dev->callbacks.hotplug(dev->callbacks.data, HWC_DISPLAY_EXTERNAL,
                           dev->hotplug.status);
    update_vblank_display(dev);
}

#if NVCAP_VIDEO_ENABLED
static void nvfb_set_hotplug_display(struct nvfb_device *dev, int disp)
{
    struct nvfb_display *old_dpy = dev->active_display[HWC_DISPLAY_EXTERNAL];
    struct nvfb_display *new_dpy = &dev->displays[disp];

    if (old_dpy != new_dpy) {
        int old_connected = old_dpy && old_dpy->connected;
        int new_connected = new_dpy && new_dpy->connected;

        /* If connected state hasn't changed, skip the hotplug */
        if (old_connected && new_connected &&
            old_dpy->mode.current->xres == new_dpy->mode.current->xres &&
            old_dpy->mode.current->yres == new_dpy->mode.current->yres) {

            /* Release the old display */
            dev->callbacks.release(dev->callbacks.data, HWC_DISPLAY_EXTERNAL);
            old_dpy->procs.blank(old_dpy, 1);

            /* Hot swap the displays */
            dev->active_display[HWC_DISPLAY_EXTERNAL] = new_dpy;

            /* Acquire the new display */
            new_dpy->procs.blank(new_dpy, 0);
            dev->callbacks.acquire(dev->callbacks.data, HWC_DISPLAY_EXTERNAL);
            update_vblank_display(dev);
        } else {
            /* Disconnect old display */
            if (old_connected) {
                nvfb_hotplug(dev, 0);
                old_dpy->procs.blank(old_dpy, 1);
            }

            /* Hot swap the displays */
            dev->active_display[HWC_DISPLAY_EXTERNAL] = new_dpy;

            /* Connect new display */
            if (new_connected) {
                new_dpy->procs.blank(new_dpy, 0);
                nvfb_hotplug(dev, 1);
            }
        }

        if (old_connected && old_dpy->type == NVFB_DISPLAY_HDMI) {
            nvfb_hdcp_disable(dev->hdcp);
        } else if (new_connected && new_dpy->type == NVFB_DISPLAY_HDMI) {
            nvfb_hdcp_enable(dev->hdcp);
        }
    }
}
#endif

static void handle_hotplug(struct nvfb_device *dev, void *data)
{
    const int disp = NVFB_DISPLAY_HDMI;
    struct nvfb_display *dpy = &dev->displays[disp];
    __u32 handle = ((struct tegra_dc_ext_control_event_hotplug *) data)->handle;
    struct tegra_dc_ext_control_output_properties props;
    int was_connected = dpy->connected;

    if (handle != dpy->dc.handle) {
        ALOGD("Hotplug: not HDMI");
        return;
    }

    props.handle = handle;
    if (ioctl(dev->ctrl_fd, TEGRA_DC_EXT_CONTROL_GET_OUTPUT_PROPERTIES,
              &props)) {
        ALOGE("Could not query out properties: %s", strerror(errno));
        return;
    }

    pthread_mutex_lock(&dev->hotplug.mutex);

    NV_ASSERT(props.handle == dpy->dc.handle);
    dpy->connected = props.connected;

    ALOGI("hotplug: connected = %d", dpy->connected);

    /* Disable if not connected or init_display_mode fails */
    if (!dpy->connected || nvfb_init_display_mode(dpy)) {
        if (dpy->connected) {
            ALOGE("Failed to get display modes: %s", strerror(errno));
            dpy->connected = 0;
        }
        dpy->mode.num_modes = 0;
        dpy->mode.current = NULL;
        dpy->mode.dirty = 0;
    }

    if (dpy->connected != was_connected &&
        dpy == dev->active_display[HWC_DISPLAY_EXTERNAL]) {
        nvfb_hotplug(dev, dpy->connected);
        if (dpy->connected) {
            nvfb_hdcp_enable(dev->hdcp);
        } else {
            nvfb_hdcp_disable(dev->hdcp);
        }
    }

    pthread_mutex_unlock(&dev->hotplug.mutex);
}

static void *hotplug_thread(void *arg)
{
    struct nvfb_device *dev = (struct nvfb_device *) arg;
    int nfds;

    nfds = MAX(dev->ctrl_fd, dev->hotplug.pipe[0]) + 1;

    while (1) {
        fd_set rfds;

        FD_ZERO(&rfds);
        FD_SET(dev->ctrl_fd, &rfds);
        FD_SET(dev->hotplug.pipe[0], &rfds);

        int status = select(nfds, &rfds, NULL, NULL, NULL);

        if (status < 0) {
            if (errno != EINTR) {
                ALOGE("%s: select failed: %s", __FUNCTION__, strerror(errno));
                return (void *) -1;
            }
        } else if (status) {
            ssize_t bytes;

            if (FD_ISSET(dev->hotplug.pipe[0], &rfds)) {
                uint8_t value = 0;

                ALOGD("%s: processing pipe fd", __FUNCTION__);
                do {
                    bytes = read(dev->hotplug.pipe[0], &value, sizeof(value));
                } while (bytes < 0 && errno == EINTR);

                if (value) {
                    ALOGD("%s: exiting", __FUNCTION__);
                    pthread_exit(0);
                }
            }

            if (FD_ISSET(dev->ctrl_fd, &rfds)) {
                char buf[1024];
                struct tegra_dc_ext_event *event = (void *)buf;

                ALOGD("%s: processing control fd", __FUNCTION__);
                do {
                    bytes = read(dev->ctrl_fd, &buf, sizeof(buf));
                } while (bytes < 0 && errno == EINTR);

                if (bytes < (ssize_t) sizeof(event)) {
                    return (void *) -1;
                }

                switch (event->type) {
                case TEGRA_DC_EXT_EVENT_HOTPLUG:
                    handle_hotplug(dev, event->data);
                    break;
                default:
                    ALOGE("%s: unexpected event type %d", __FUNCTION__,
                          event->type);
                    break;
                }
            }
        }
    }

    return (void *) 0;
}

/*********************************************************************
 *   Initialization
 */

#define FB_PREFERRED_FORMAT HAL_PIXEL_FORMAT_RGBA_8888

// bpp, red, green, blue, alpha
static const int fb_format_map[][9] = {
    { 0,  0,  0,  0,  0,  0,  0,  0,  0}, // INVALID
    {32,  0,  8,  8,  8, 16,  8, 24,  8}, // HAL_PIXEL_FORMAT_RGBA_8888
    {32,  0,  8,  8,  8, 16,  8,  0,  0}, // HAL_PIXEL_FORMAT_RGBX_8888
    {24, 16,  8,  8,  8,  0,  8,  0,  0}, // HAL_PIXEL_FORMAT_RGB_888
    {16, 11,  5,  5,  6,  0,  5,  0,  0}, // HAL_PIXEL_FORMAT_RGB_565
    {32, 16,  8,  8,  8,  0,  8, 24,  8}, // HAL_PIXEL_FORMAT_BGRA_8888
    {16, 11,  5,  6,  5,  1,  5,  0,  1}, // HAL_PIXEL_FORMAT_RGBA_5551
    {16, 12,  4,  8,  4,  4,  4,  0,  4}  // HAL_PIXEL_FORMAT_RGBA_4444
};

static int FillFormat (struct fb_var_screeninfo* info, int format)
{
    const int *p;

    if (format == 0 || format > HAL_PIXEL_FORMAT_RGBA_4444)
        return -ENOTSUP;

    p = fb_format_map[format];
    info->bits_per_pixel = *(p++);
    info->red.offset = *(p++);
    info->red.length = *(p++);
    info->green.offset = *(p++);
    info->green.length = *(p++);
    info->blue.offset = *(p++);
    info->blue.length = *(p++);
    info->transp.offset = *(p++);
    info->transp.length = *(p++);
    return 0;
}

void nvfb_get_display_caps(struct nvfb_device *dev, int disp,
                           struct nvfb_display_caps *caps)
{
    struct nvfb_display *dpy = dev->active_display[disp];

    memset(caps, 0, sizeof(struct nvfb_display_caps));

    if (dpy) {
        *caps = dpy->caps;
    }
}

// XXX - query from DC
static struct nvfb_display_caps T30_display_caps =
{
    .num_windows = 3,
    .window_cap = {
        0,
        NVFB_WINDOW_CAP_YUV_FORMATS | NVFB_WINDOW_CAP_SCALE | NVFB_WINDOW_CAP_16BIT,
        NVFB_WINDOW_CAP_YUV_FORMATS
    }
};

static int nvfb_calc_display_caps(struct nvfb_display *dpy)
{
    dpy->caps = T30_display_caps;
    return 0;
}

static int nvfb_open_display(struct nvfb_display *dpy, int disp)
{
    int fb, dc;
    size_t ii;
    char path[64];
    char const * const fb_template[] = {
        "/dev/graphics/fb%u",
        "/dev/fb%u",
        0
    };
    char const * const dc_template[] = {
        "/dev/tegra_dc_%u",
        0};
    const char *smart_panel_template =
        "/sys/class/graphics/fb%u/device/smart_panel";
    struct stat buf;

    dpy->fb.fd = dpy->dc.fd = -1;

    for (ii = 0, fb = -1; fb == -1 && fb_template[ii]; ii++) {
        snprintf(path, sizeof(path), fb_template[ii], disp);
        fb = open(path, O_RDWR, 0);
    }
    if (fb == -1) {
        ALOGE("Can't open framebuffer device %d: %s\n", disp, strerror(errno));
        return errno;
    }

    for (ii = 0, dc = -1; dc == -1 && dc_template[ii]; ii++) {
        snprintf(path, sizeof(path), dc_template[ii], disp);
        dc = open(path, O_RDWR, 0);
    }
    if (dc == -1) {
        ALOGE("Can't open DC device %d: %s\n", disp, strerror(errno));
        close(fb);
        return errno;
    }

    dpy->fb.fd = fb;
    dpy->dc.fd = dc;

    if (ioctl(dpy->dc.fd, TEGRA_DC_EXT_SET_NVMAP_FD,
              NvRm_MemmgrGetIoctlFile()) < 0) {
        ALOGE("Can't set nvmap fd: %s\n", strerror(errno));
        return errno;
    }

    /* Set up vblank wait */
    snprintf(path, sizeof(path), smart_panel_template, disp);
    if (!stat(path, &buf)) {
        dpy->vblank.syncpt = NVRM_INVALID_SYNCPOINT_ID;
    } else if (ioctl(dpy->dc.fd, TEGRA_DC_EXT_GET_VBLANK_SYNCPT,
                     &dpy->vblank.syncpt)) {
        ALOGE("Can't query vblank syncpoint for device %d: %s\n",
              disp, strerror(errno));
        return errno;
    }

    if (nvfb_calc_display_caps(dpy) < 0) {
        ALOGE("Can't get display caps for display %d\n", disp);
        return errno;
    }

    for (ii = 0; ii < dpy->caps.num_windows; ii++) {
        if (ioctl(dpy->dc.fd, TEGRA_DC_EXT_GET_WINDOW, ii) < 0) {
            ALOGE("Failed to allocate window %d for display %d\n", ii, disp);
            return errno;
        }
    }

    return 0;
}

static void nvfb_close_display(struct nvfb_display *dpy)
{
    close(dpy->fb.fd);
    close(dpy->dc.fd);
}

static int nvfb_get_displays(struct nvfb_device *dev)
{
    uint_t num, ii;

    dev->ctrl_fd = open(TEGRA_CTRL_PATH, O_RDWR);
    if (dev->ctrl_fd < 0) {
        return errno;
    }

    if (ioctl(dev->ctrl_fd, TEGRA_DC_EXT_CONTROL_GET_NUM_OUTPUTS, &num)) {
        return errno;
    }

    if (num > HWC_NUM_DISPLAY_TYPES) {
        num = HWC_NUM_DISPLAY_TYPES;
    }

    for (ii = 0; ii < num; ii++) {
        struct nvfb_display *dpy = dev->displays + ii;
        struct tegra_dc_ext_control_output_properties props;

        props.handle = ii;
        if (ioctl(dev->ctrl_fd, TEGRA_DC_EXT_CONTROL_GET_OUTPUT_PROPERTIES,
                  &props)) {
            return errno;
        }

        dpy->type = (enum DisplayType) ii;
        dpy->dc.handle = props.handle;
        dpy->connected = props.connected;

        if (nvfb_open_display(dpy, ii)) {
            return errno;
        }

        dpy->procs.blank = dc_blank;
        dpy->procs.post = dc_post;
        dpy->procs.set_mode = dc_set_mode_deferred;
        if (dpy->vblank.syncpt != NVRM_INVALID_SYNCPOINT_ID) {
            dpy->procs.vblank_wait = dc_vblank_wait_syncpt;
        } else {
            dpy->procs.vblank_wait = dc_vblank_wait_fb;
        }

        if (dpy->connected) {
            if (nvfb_init_display_mode(dpy)) {
                return errno;
            }
        } else {
            dpy->blank = 1;
        }

        dev->display_mask |= (1 << ii);
    }

    return 0;
}

static int null_blank(struct nvfb_display *dpy, int blank)
{
    dpy->blank = blank;

    return 0;
}

static int
null_post(struct nvfb_device *dev, struct nvfb_display *dpy,
          struct nvfb_window_config *conf, struct nvfb_buffer *bufs,
          struct nvfb_sync *post_sync)
{
    size_t ii;

    for (ii = 0; ii < dpy->caps.num_windows; ii++) {
        NvRmFence *fence = &bufs[ii].sync.fence;
        if (fence->SyncPointID != NVRM_INVALID_SYNCPOINT_ID) {
            NvRmFenceWait(dev->gralloc->Rm, fence, NV_WAIT_INFINITE);
        }
    }

    post_sync->fence.SyncPointID = NVRM_INVALID_SYNCPOINT_ID;

    return 0;
}

static int null_set_mode(struct nvfb_display *dpy,
                          struct fb_var_screeninfo *mode)
{
    return 0;
}

static void init_null_display(struct nvfb_device *dev)
{
    struct nvfb_display *dpy = &dev->displays[NVFB_DISPLAY_PANEL];
    struct fb_var_screeninfo *mode;

    dpy->type = NVFB_DISPLAY_PANEL;
    dpy->blank = 0;
    nvfb_calc_display_caps(dpy);
    dpy->vblank.syncpt = NVRM_INVALID_SYNCPOINT_ID;

    dpy->procs.blank = null_blank;
    dpy->procs.post = null_post;
    dpy->procs.set_mode = null_set_mode;
    dpy->procs.vblank_wait = vblank_wait_sleep;

    mode = dpy->mode.modedb;
    dpy->mode.num_modes = 1;
    dpy->mode.modes[0] = mode;
    dpy->mode.current = mode;

    /* Set the default mode */
    create_fake_mode(mode, default_xres, default_yres, default_refresh);
    update_vblank_period(dpy);

    dev->display_mask = HWC_DISPLAY_PRIMARY_BIT;
}

#if NVCAP_VIDEO_ENABLED
static void nvfb_nvcap_set_mode(void *data, struct nvcap_mode *mode)
{
    struct nvfb_device *dev = (struct nvfb_device *) data;
    struct nvfb_display *dpy = &dev->displays[NVFB_DISPLAY_WFD];

    pthread_mutex_lock(&dev->hotplug.mutex);

    if (dpy->connected) {
        NV_ASSERT(dpy == dev->active_display[HWC_DISPLAY_EXTERNAL]);
        /* disconnect */
        nvfb_hotplug(dev, 0);
    }

    ALOGD("%s: %d x %d @ %1.f Hz", __FUNCTION__, mode->xres, mode->yres,
          mode->refresh);

    create_fake_mode(dpy->mode.current, mode->xres, mode->yres, mode->refresh);
    update_vblank_period(dpy);

    if (dpy->connected) {
        /* reconnect */
        nvfb_hotplug(dev, 1);
    }

    pthread_mutex_unlock(&dev->hotplug.mutex);
}

static void nvfb_nvcap_enable(void *data, int enable)
{
    struct nvfb_device *dev = (struct nvfb_device *) data;
    struct nvfb_display *dpy = &dev->displays[NVFB_DISPLAY_WFD];

    ALOGD("%s: nvcap %sabled", __FUNCTION__, enable ? "en" : "dis");

    pthread_mutex_lock(&dev->hotplug.mutex);

    if (enable) {
        dpy->connected = 1;
        nvfb_set_hotplug_display(dev, NVFB_DISPLAY_WFD);
    } else {
        nvfb_set_hotplug_display(dev, NVFB_DISPLAY_HDMI);
        dpy->connected = 0;
    }

    pthread_mutex_unlock(&dev->hotplug.mutex);
}

static void nvfb_nvcap_close(struct nvfb_device *dev)
{
    struct nvfb_display *dpy = &dev->displays[NVFB_DISPLAY_WFD];

    if (dpy->nvcap.ctx) {
        dpy->nvcap.procs->close(dpy->nvcap.ctx);
        dpy->nvcap.ctx = NULL;
    }

    if (dpy->nvcap.handle) {
        dlclose(dpy->nvcap.handle);
        dpy->nvcap.handle = NULL;
    }
}

static int nvfb_nvcap_open(struct nvfb_device *dev)
{
    struct nvfb_display *dpy = &dev->displays[NVFB_DISPLAY_WFD];
    const char *library_name = NVCAP_VIDEO_LIBRARY_STRING;
    const char *interface_string = NVCAP_VIDEO_INTERFACE_STRING;
    struct nvcap_callbacks callbacks = {
        .data = dev,
        .enable = nvfb_nvcap_enable,
        .set_mode = nvfb_nvcap_set_mode,
    };

    dpy->nvcap.handle = dlopen(library_name, RTLD_LAZY);
    if (!dpy->nvcap.handle) {
        ALOGE("failed to load %s: %s", library_name, dlerror());
        goto fail;
    }

    dpy->nvcap.procs = (struct nvcap_interface *)
        dlsym(dpy->nvcap.handle, interface_string);
    if (!dpy->nvcap.procs) {
        ALOGE("failed to load nvcap interface: %s%s",
              dlerror(), interface_string);
        goto fail;
    }

    ALOGD("Creating nvcap video capture service");
    dpy->nvcap.ctx = dpy->nvcap.procs->open(&callbacks);
    if (!dpy->nvcap.ctx) {
        ALOGE("failed to create nvcap video capture service\n");
        goto fail;
    }

    return 0;

fail:
    nvfb_nvcap_close(dev);
    return -1;
}

static struct nvfb_display_caps nvcap_display_caps =
{
    .num_windows = 1,
    .window_cap = {
        NVFB_WINDOW_CAP_YUV_FORMATS | NVFB_WINDOW_CAP_SCALE,
    }
};

static int nvcap_blank(struct nvfb_display *dpy, int blank)
{
    ALOGD("%s: display %d, [%d -> %d]", __FUNCTION__,
          dpy->type, dpy->blank, blank);

    if (dpy->blank != blank) {
        if (blank) {
            dpy->nvcap.procs->post(dpy->nvcap.ctx, NULL);
        }
        dpy->blank = blank;
    }

    return 0;
}

static int
nvcap_post(struct nvfb_device *dev, struct nvfb_display *dpy,
           struct nvfb_window_config *conf, struct nvfb_buffer *bufs,
           struct nvfb_sync *post_sync)
{
    struct nvcap_layer layer;

    if (!conf) {
        return nvcap_blank(dpy, 1);
    }

    if (conf->protect) {
        layer.buffer = NULL;
    } else {
        layer.buffer = bufs[0].buffer;
    }
    layer.fence = bufs[0].sync.fence;
    layer.protect = conf->protect;
    layer.transform = conf->overlay[0].transform;
    layer.src = conf->overlay[0].src;

    post_sync->fence.SyncPointID = NVRM_INVALID_SYNCPOINT_ID;

    return dpy->nvcap.procs->post(dpy->nvcap.ctx, &layer);
}

static void nvcap_init_display(struct nvfb_device *dev)
{
    struct nvfb_display *dpy = &dev->displays[NVFB_DISPLAY_WFD];
    struct fb_var_screeninfo *mode;

    dpy->type = NVFB_DISPLAY_WFD;
    dpy->blank = 1;
    dpy->caps = nvcap_display_caps;
    dpy->vblank.syncpt = NVRM_INVALID_SYNCPOINT_ID;

    dpy->procs.blank = nvcap_blank;
    dpy->procs.post = nvcap_post;
    dpy->procs.set_mode = null_set_mode;
    dpy->procs.vblank_wait = vblank_wait_sleep;

    /* WFD supports only a single mode at a time */
    mode = dpy->mode.modedb;
    dpy->mode.num_modes = 1;
    dpy->mode.modes[0] = mode;
    dpy->mode.current = mode;

    /* Set the default mode */
    create_fake_mode(mode, default_xres, default_yres, default_refresh);
    update_vblank_period(dpy);
}
#endif /* NVCAP_VIDEO_ENABLED */

#include <android/configuration.h>
static int get_bucketed_dpi(int dpi)
{
    int dpi_bucket[] = {
        ACONFIGURATION_DENSITY_DEFAULT, ACONFIGURATION_DENSITY_LOW,
        ACONFIGURATION_DENSITY_MEDIUM, ACONFIGURATION_DENSITY_HIGH,
        ACONFIGURATION_DENSITY_XHIGH, ACONFIGURATION_DENSITY_XXHIGH,
        ACONFIGURATION_DENSITY_NONE
    };
    int ii;

    for (ii = 1; ii < (int)NV_ARRAY_SIZE(dpi_bucket); ii++) {
        /* Find the range of bucketed values that contain the value
         * specified in dpi and choose the bucket that is closest.
         */
        if (dpi <= dpi_bucket[ii]) {
            dpi = dpi_bucket[ii] - dpi > dpi - dpi_bucket[ii-1] ?
                dpi_bucket[ii-1] : dpi_bucket[ii];
            break;
        }
    }

    return dpi;
}

static void set_panel_properties(struct nvfb_device *dev)
{
    struct nvfb_display *dpy = dev->active_display[HWC_DISPLAY_PRIMARY];
    char value[PROPERTY_VALUE_MAX];

    /* Normally the lcd_density property is set in system.prop but
     * some reference boards support multiple panels and therefore
     * cannot reliably set this value.  If the override property is
     * set and the lcd_density is not, compute the density from the
     * panel mode and set it here.
     */
    property_get("ro.sf.override_null_lcd_density", value, "0");
    if (strcmp(value, "0")) {
        property_get("ro.sf.lcd_density", value, "0");
        if (!strcmp(value, "0")) {
            struct fb_var_screeninfo *mode = dpy->mode.current;
            int dpi;

            if ((int)mode->width > 0) {
                dpi = (int) (mode->xres * 25.4f / mode->width + 0.5f);
            } else {
                dpi = 160;
            }

            // lcd density needs to be set to a bucketed dpi value.
            sprintf(value, "%d", get_bucketed_dpi(dpi));
            property_set("ro.sf.lcd_density", value);
        }
    }

    /* XXX get rid of this please */
    sprintf(value, "%d", dpy->mode.current->xres);
    property_set("persist.sys.NV_DISPXRES", value);
    sprintf(value, "%d", dpy->mode.current->yres);
    property_set("persist.sys.NV_DISPYRES", value);
}

int nvfb_open(struct nvfb_device **fbdev, struct nvfb_callbacks *callbacks,
              int *display_mask, struct nvfb_hotplug_status *hotplug)
{
    struct nvfb_device *dev;
    hw_module_t const *module;
    char value[PROPERTY_VALUE_MAX];
    int ii;

    dev = malloc(sizeof(struct nvfb_device));
    if (!dev) {
        return -1;
    }

    memset(dev, 0, sizeof(struct nvfb_device));
    dev->ctrl_fd = -1;
    for (ii = 0; ii < NVFB_MAX_DISPLAYS; ii++) {
        dev->displays[ii].dc.fd = -1;
        dev->displays[ii].fb.fd = -1;
        /* blank state unknown */
        dev->displays[ii].blank = -1;
    }

    dev->callbacks = *callbacks;

    property_get("nvidia.hwc.null_display", value, "0");
    dev->null_display = !strcmp(value, "1");

    if (dev->null_display) {
        init_null_display(dev);
    } else {
        if (nvfb_get_displays(dev)) {
            ALOGE("Can't get display information: %s", strerror(errno));
            goto fail;
        }
    }

    if (!dev->display_mask) {
        goto fail;
    }

    /* Set up active displays */
    for (ii = 0; ii < HWC_NUM_DISPLAY_TYPES; ii++) {
        if (dev->display_mask & (1 << ii)) {
            dev->active_display[ii] = &dev->displays[ii];
        }
    }

    /* Use panel by default for vblank waits */
    update_vblank_display(dev);
    NV_ASSERT(dev->vblank_display);

    if (dev->display_mask & HWC_DISPLAY_EXTERNAL_BIT) {
        struct nvfb_display *dpy = &dev->displays[NVFB_DISPLAY_HDMI];

        pthread_mutex_init(&dev->hotplug.mutex, NULL);
        pthread_mutex_lock(&dev->hotplug.mutex);
        dev->hotplug.status.connected = dpy->connected;
        dev->hotplug.status.stereo = dpy->connected ? dpy->mode.stereo : 0;
        dev->hotplug.status.generation++;
        pthread_mutex_unlock(&dev->hotplug.mutex);

        if (ioctl(dev->ctrl_fd, TEGRA_DC_EXT_CONTROL_SET_EVENT_MASK,
                  TEGRA_DC_EXT_EVENT_HOTPLUG)) {
            ALOGE("Failed to register hotplug event: %s\n", strerror(errno));
            goto fail;
        }

        if (pipe(dev->hotplug.pipe) < 0) {
            ALOGE("pipe failed: %s", strerror(errno));
            goto fail;
        }

        if (pthread_create(&dev->hotplug.thread, NULL, hotplug_thread, dev)) {
            ALOGE("Can't create hotplug thread: %s\n", strerror(errno));
            goto fail;
        }

        nvfb_hdcp_open(&dev->hdcp);

        if (dpy->connected) {
            nvfb_hdcp_enable(dev->hdcp);
        }
    }

#if NVCAP_VIDEO_ENABLED
    /* Start nvcap service */
    if (!nvfb_nvcap_open(dev)) {
        nvcap_init_display(dev);
    }
#endif

    set_panel_properties(dev);

    hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    dev->gralloc = (NvGrModule *) module;

    *fbdev = dev;
    *display_mask = dev->display_mask;
    *hotplug = dev->hotplug.status;

    return 0;

fail:
    nvfb_close(dev);
    return -errno;
}

void nvfb_close(struct nvfb_device *dev)
{
    if (!dev) {
        return;
    }

#if NVCAP_VIDEO_ENABLED
    nvfb_nvcap_close(dev);
#endif

    if (dev->display_mask & HWC_DISPLAY_EXTERNAL_BIT) {
        if (dev->no_capture) {
            NvGrModule *m = (NvGrModule*) dev->gralloc;
            m->free_scratch(m, dev->no_capture);
        }

        nvfb_hdcp_close(dev->hdcp);

        if (dev->hotplug.thread) {
            int status;
            uint8_t value = 1;

            /* Notify the hotplug thread to terminate */
            if (write(dev->hotplug.pipe[1], &value, sizeof(value)) < 0) {
                ALOGE("hotplug thread: write failed: %s", strerror(errno));
            }

            /* Wait for the thread to exit */
            status = pthread_join(dev->hotplug.thread, NULL);
            if (status) {
                ALOGE("hotplug thread: thread_join: %d", status);
            }

            close(dev->hotplug.pipe[0]);
            close(dev->hotplug.pipe[1]);
        }

        pthread_mutex_destroy(&dev->hotplug.mutex);
    }

    if (dev->display_mask & (1 << HWC_DISPLAY_PRIMARY)) {
        nvfb_close_display(&dev->displays[NVFB_DISPLAY_PANEL]);
    }
    if (dev->display_mask & (1 << HWC_DISPLAY_EXTERNAL)) {
        nvfb_close_display(&dev->displays[NVFB_DISPLAY_HDMI]);
    }

    close(dev->ctrl_fd);

    free(dev);
}

#if 0
static int MapFormat (struct fb_var_screeninfo* info)
{
    int i;
    struct fb_var_screeninfo cmp;

    for (i = 1; i <= HAL_PIXEL_FORMAT_RGBA_4444; i++)
    {
        FillFormat(&cmp, i);
        if ((info->bits_per_pixel == cmp.bits_per_pixel) &&
            (info->red.offset == cmp.red.offset) &&
            (info->red.length == cmp.red.length) &&
            (info->green.offset == cmp.green.offset) &&
            (info->green.length == cmp.green.length) &&
            (info->blue.offset == cmp.blue.offset) &&
            (info->blue.length == cmp.blue.length) &&
            (info->transp.offset == cmp.transp.offset) &&
            (info->transp.length == cmp.transp.length))
            return i;
    }
    return 0;
}

void nvfb_dump(struct nvfb_device *dev, char *buff, int buff_len)
{
    struct fb_fix_screeninfo finfo;

    if (dev->null_display) {
        strcpy(finfo.id, "null display");
    } else if (dev->display_mask & PRIMARY) {
        int pixformat;

        if (ioctl(dev->displays[NVFB_DISPLAY_PANEL].fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
            ALOGE("Can't get FSCREENINFO: %s\n", strerror(errno));
            goto fail;
        }

        pixformat = MapFormat(mode);
        if (pixformat == 0) {
            ALOGE("Unrecognized framebuffer pixel format\n");
            errno = EINVAL;
            goto fail;
        } else if (pixformat != FB_PREFERRED_FORMAT) {
            ALOGW("Using format %d instead of preferred %d\n",
                  pixformat, FB_PREFERRED_FORMAT);
        }
    }

    ALOGI("using (fd=%d)\n"
         "id           = %s\n"
         "xres         = %d px\n"
         "yres         = %d px\n"
         "xres_virtual = %d px\n"
         "yres_virtual = %d px\n"
         "bpp          = %d\n"
         "r            = %2u:%u\n"
         "g            = %2u:%u\n"
         "b            = %2u:%u\n"
         "smem_start   = 0x%08lx\n"
         "smem_len     = 0x%08x\n",
         dev->displays[NVFB_DISPLAY_PANEL].fd,
         finfo.id,
         info.xres,
         info.yres,
         info.xres_virtual,
         info.yres_virtual,
         info.bits_per_pixel,
         info.red.offset, info.red.length,
         info.green.offset, info.green.length,
         info.blue.offset, info.blue.length,
         finfo.smem_start,
         finfo.smem_len
    );
    ALOGI("format       = %d\n"
         "stride       = %d\n"
         "width        = %d mm (%f dpi)\n"
         "height       = %d mm (%f dpi)\n"
         "refresh rate = %.2f Hz\n",
         pixformat,
         dev->Base.stride,
         info.width,  dev->Base.xdpi,
         info.height, dev->Base.ydpi,
         dev->Base.fps
    );
}
#endif
