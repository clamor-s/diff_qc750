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
 * Copyright (c) 2010-2013, NVIDIA CORPORATION.  All rights reserved.
 */

#ifndef _NVHWC_H
#define _NVHWC_H

#define HWC_REMOVE_DEPRECATED_VERSIONS 1

#include <hardware/gralloc.h>
#include <hardware/hwcomposer.h>
#include "nvgralloc.h"
#include "nvgr_scratch.h"
#include "nvfb.h"
#include "nvhwc_didim.h"

/* Compatibility */
typedef hwc_layer_1_t hwc_layer_t;
typedef hwc_display_contents_1_t hwc_display_contents_t;
typedef hwc_composer_device_1_t hwc_composer_device_t;

#ifndef FB_SCALE_IN_DC
#define FB_SCALE_IN_DC 0
#endif

#define STATUSBAR_BLEND_OPT 1

/* This should be moved to a common header file */
#define TEGRA_PROP_PREFIX "persist.tegra."
#define TEGRA_PROP(s) TEGRA_PROP_PREFIX #s

#define SYS_PROP_PREFIX "persist.sys."
#define SYS_PROP(s) SYS_PROP_PREFIX #s

typedef struct hwc_xform_t {
    float x_scale;
    float y_scale;
    int x_off;
    int y_off;
} hwc_xform;

typedef enum {
    HWC_MirrorMode_Crop,
    HWC_MirrorMode_Center,
    HWC_MirrorMode_Scale,
} HWC_MirrorMode;

typedef enum {
    HWC_Compositor_SurfaceFlinger = 0,
    HWC_Compositor_Gralloc        = 1,
} HWC_Compositor;

typedef enum {
    /* Individual policy bits that control HWC behavior */
    HWC_CompositePolicy_ForceComposite   = 1 << 0,
    HWC_CompositePolicy_FbCache          = 1 << 1,
    HWC_CompositePolicy_CompositeOnIdle  = 1 << 2,

    /* Tested policy combinations that can be set with setprop */
    HWC_CompositePolicy_AssignWindows   = 0,
    HWC_CompositePolicy_CompositeAlways  = HWC_CompositePolicy_ForceComposite,
    HWC_CompositePolicy_Auto             = HWC_CompositePolicy_FbCache
                                         | HWC_CompositePolicy_CompositeOnIdle,
} HWC_CompositePolicy;

#define HWC_MAX_LAYERS 32

struct hwc_prepare_layer {
    /* Original list index */
    int index;
    /* Original layer */
    hwc_layer_t *layer;
    /* Copied from original layer, may be modified locally */
    int transform;
    int blending;
    int surf_index;
    hwc_rect_t src;
    hwc_rect_t dst;
};

struct hwc_prepare_map {
    /* Hardware window index */
    int window;
    /* Index of layer in filtered list */
    int layer_index;
};

struct hwc_prepare_state {
    /* Original layer list */
    hwc_display_contents_t *display;
    /* Map index currently being processed */
    int current_map;
    /* Mask of available windows */
    unsigned window_mask;
    /* Layers to be assigned if possible */
    int numLayers;
    struct hwc_prepare_layer layers[HWC_MAX_LAYERS];
    /* Assigned window map */
    struct hwc_prepare_map layer_map[NVFB_MAX_WINDOWS];
    /* Use windows or composite? */
    NvBool use_windows;
    /* Protected content present? */
    NvBool protect;
    /* Too many layers to process? */
    NvBool overflow;

    struct {
        /* Dedicated layer for the framebuffer */
        struct hwc_prepare_layer layer;
        /* bounding rectangle of all composited layers in layer space */
        hwc_rect_t bounds;
        /* framebuffer blending mode */
        int blending;
    } fb;

#if STATUSBAR_BLEND_OPT
    /* Statusbar layer, if blend optimization enabled */
    struct hwc_prepare_layer *statusbar;
#endif
};

struct hwc_window_mapping {
    int index;
    NvGrScratchSet *scratch; /* scratch buffer set for this window */
    int surf_index; /* source surface index for scratch blit*/
};

// Dynamic idle detection forces compositing when the framerate drops
// below IDLE_MINIMUM_FPS frames per second.  This is determined by
// comparing the current time with a timeout value relative to
// IDLE_FRAME_COUNT frames ago.  The time limit is a function of the
// minimum_fps value.
//
// The original selection of 20 fps as the crossover point is based on
// the assumption of 60fps refresh - this is really intended to be
// refresh_rate/3.  If there are two overlapping fullscreen layers,
// which is typical, over three refreshes there will be six
// isochronous reads in display.  If the two layers are composited,
// there are two reads and one write in 3D, plus three isochronous
// reads - nominally the same bandwidth requirement but easier on the
// memory controller due to less isochronous traffic.  As the frame
// rate drops, the scales tip further toward compositing since the
// three memory touches in 3D are amortized over additional refreshes.
//
// In practice it seems that 20 is too high a threshold, as benchmarks
// which are shader limited and run just below 20fps are negatively
// impacted.  See bug #934644.

#define IDLE_FRAME_COUNT 8
#define IDLE_MINIMUM_FPS 8

struct hwc_idle_machine_t {
    struct timespec frame_times[IDLE_FRAME_COUNT];
    struct timespec time_limit;
    struct timespec timeout;
    uint8_t minimum_fps;
    uint8_t frame_index;
    uint8_t composite;
    timer_t timer;
    int32_t enabled;
};

// Framebuffer cache determines if we can skip re-compositing when fb
// layers are unchanged.

#define FB_CACHE_LAYERS 8

struct hwc_fb_cache_layer_t {
    int index;
    buffer_handle_t handle;
    uint32_t transform;
    int32_t blending;
    hwc_rect_t sourceCrop;
    hwc_rect_t displayFrame;
};

struct hwc_fb_cache_t {
    int numLayers;
    struct hwc_fb_cache_layer_t layers[FB_CACHE_LAYERS];
};

#define OLD_CACHE_INDEX(dpy) ((dpy)->fb_cache_index)
#define NEW_CACHE_INDEX(dpy) ((dpy)->fb_cache_index ^ 1)

struct hwc_display_t {
    int type;
    /* display caps */
    struct nvfb_display_caps caps;
    /* current framebuffer config (mode) */
    struct nvfb_config config;
    /* display bounds */
    hwc_rect_t device_clip;
    /* default display mode */
    struct fb_var_screeninfo *default_mode;
    /* current display mode (content-dependent) */
    struct fb_var_screeninfo *current_mode;
    /* blank state */
    int blank;

    /* transform from layer space to device space */
    int transform;
    /* display bounds in layer space */
    hwc_rect_t layer_clip;

    /* currently locked buffers */
    NvNativeHandle *buffers[NVFB_MAX_WINDOWS];
    /* last flip sync */
    struct nvfb_sync last_flip;

    /* window config */
    NvGrOverlayConfig conf;
    /* windows from top to bottom, including framebuffer if needed */
    struct hwc_window_mapping map[NVFB_MAX_WINDOWS];

    /* index of the framebuffer in map, -1 if none */
    int fb_index;
    /* index of the framebuffer in display list */
    int fb_target;
    /* number of composited layers */
    int fb_layers;
    /* layers marked for clear in SF */
    uint32_t clear_layers;

    /* Framebuffer cache (double buffered) */
    struct hwc_fb_cache_t fb_cache[2];
    /* Active cache index */
    uint8_t fb_cache_index;
    /* Bool, reuse cached framebuffer */
    uint8_t fb_recycle;
    /* windows overlap */
    uint8_t windows_overlap;
    uint8_t unused[1];

    /* Scratch buffers */
    NvGrScratchClient *scratch;

    /* Composite state */
    struct {
        HWC_Compositor engine;
        struct NvGrCompositeListRec list;
        int map[NVGR_COMPOSITE_LIST_MAX];
        NvGrScratchSet *scratch;
    } composite;
};

struct hwc_context_t {
    hwc_composer_device_t device; /* must be first */
    NvGrModule* gralloc;

    struct nvfb_device *fb;
    /* Mask of supported displays */
    int display_mask;
    /* Per-display information and state */
    struct hwc_display_t displays[HWC_NUM_DISPLAY_TYPES];

    /* external device hotplug status */
    struct {
        pthread_mutex_t mutex;
        struct nvfb_hotplug_status cached;
        struct nvfb_hotplug_status latest;
    } hotplug;

    struct {
        /* mirror state may have changed */
        NvBool dirty;
        /* mirror mode is currently active */
        NvBool enable;
        /* primary to external scale and bias */
        hwc_xform xf;
        /* Mirror behavior (legacy only) */
        HWC_MirrorMode mode;
    } mirror;

    /* show only video layers (GRALLOC_USAGE_EXTERNAL_DISP | NVGR_USAGE_CLOSED_CAP)
     * on hdmi during video playback or camera preview */
    NvBool hdmi_video_mode;
    /* enable cinema mode = video layers shown are only on hdmi;
     * ignored if hdmi_video_mode is false */
    NvBool cinema_mode;
    /* enable logic to limit window B usage to minimize underflow */
    NvBool limit_window_b;

    /* stereo content flag cache */
    NvBool bStereoGLAppRunning;
    /* use legacy HDMI policy */
    NvBool legacy;
    /* alignment */
    NvBool unused[3];

    /* index of the video out layer */
    int video_out_index;
    /* index of the closed captioning layer */
    int cc_out_index;
    /* index of the on-screen display layer */
    int osd_out_index;
    /* cache HDMI behavior */
    int hdmi_mode;
    /* native orientation of panel relative to home */
    int panel_transform;
    /* DIDIM state */
    struct didim_client *didim;
    /* framework hwc callbacks */
    hwc_procs_t *procs;

    /* Idle detection machinery */
    struct hwc_idle_machine_t idle;

    HWC_CompositePolicy composite_policy;
    HWC_Compositor compositor;

    struct {
        int32_t         sync;
        int32_t         shutdown;
        pthread_t       thread;
        pthread_mutex_t mutex;
        pthread_cond_t  condition;
    } event;
    /* nvdps file descriptor */
    int fd_nvdps;
    /* nvdps current status */
    int nvdps_idle;

    /* debug interface */
    void (*post_wait)(struct hwc_context_t *ctx, int disp);
    void (*get_buffers)(struct hwc_context_t *ctx, int disp,
                        NvNativeHandle *buffers[NVFB_MAX_WINDOWS]);
};

#define r_width(r) ((r)->right - (r)->left)
#define r_height(r) ((r)->bottom - (r)->top)

/* r_copy? */
#define copy_rect(src, dst) {                   \
    (dst)->left = (src)->left;                  \
    (dst)->top = (src)->top;                    \
    (dst)->right = (src)->right;                \
    (dst)->bottom = (src)->bottom;              \
}

static inline int hwc_get_usage(hwc_layer_t* l)
{
    if (l->handle)
        return ((NvNativeHandle*)l->handle)->Usage;
    else
        return 0;
}

static inline NvRmSurface* get_nvrmsurface(hwc_layer_t* l)
{
    return (NvRmSurface*)&((NvNativeHandle*)l->handle)->Surf;
}

static inline NvColorFormat get_colorformat(hwc_layer_t* l)
{
    return get_nvrmsurface(l)->ColorFormat;
}

static inline int get_halformat(hwc_layer_t* l)
{
    return ((NvNativeHandle*)l->handle)->Format;
}

int hwc_find_layer(hwc_display_contents_t *list, int usage, NvBool unique);
int hwc_get_window(const struct nvfb_display_caps *caps,
                   unsigned* mask, unsigned req, unsigned* unsatisfied);

void hwc_prepare_begin(struct hwc_context_t *ctx,
                       struct hwc_display_t *dpy,
                       struct hwc_prepare_state *prepare,
                       hwc_display_contents_t *display,
                       int use_windows, int ignore_mask);
void hwc_prepare_end(struct hwc_context_t *ctx,
                     struct hwc_display_t *dpy,
                     struct hwc_prepare_state *prepare,
                     hwc_display_contents_t *display);
void hwc_prepare_add_layer(struct hwc_display_t *dpy,
                           struct hwc_prepare_state *prepare,
                           hwc_layer_t *cur, int index);
int hwc_assign_windows(struct hwc_context_t *ctx,
                       struct hwc_display_t *dpy,
                       struct hwc_prepare_state *prepare);

/* Return a transform which is the combination of two transforms */
static inline int combine_transform(int t0, int t1)
{
    int mask = t0 & HWC_TRANSFORM_ROT_90;

    if (t1 & mask) {
        mask |= ~t0 & HWC_TRANSFORM_ROT_180;
    } else {
        mask |= t0 & HWC_TRANSFORM_ROT_180;
    }

    return t1 ^ mask;
}

/* We use 2D to perform ROT_90 *before* display performs FLIP_{H|V}.
 * hardware.h states that ROT_90 is applied *after* FLIP_{H|V}.
 *
 * If both FLIP flags are applied the result is the same. However if
 * only one is applied (i.e. the surface is mirrored) the order does
 * matter. To compensate, swap these flags.
 */
static inline int fix_transform(int transform)
{
    switch (transform) {
    case HWC_TRANSFORM_ROT_90 | HWC_TRANSFORM_FLIP_H:
        return HWC_TRANSFORM_ROT_90 | HWC_TRANSFORM_FLIP_V;
    case HWC_TRANSFORM_ROT_90 | HWC_TRANSFORM_FLIP_V:
        return HWC_TRANSFORM_ROT_90 | HWC_TRANSFORM_FLIP_H;
    default:
        return transform;
    }
}

/* If the scaling is almost 1:1, it probably really is 1:1 but
 * we lost some precision along the way.
 */
static inline int hwc_need_scale(float scale_w, float scale_h)
{
    const float epsilon = 0.004f;

    return ((1.0f - epsilon) > scale_w ||
            (1.0f + epsilon) < scale_w ||
            (1.0f - epsilon) > scale_h ||
            (1.0f + epsilon) < scale_h);
}

void hwc_get_scale(int transform, const hwc_rect_t *src, const hwc_rect_t *dst,
                   float *scale_w, float *scale_h);

void hwc_scale_window(struct hwc_display_t *dpy, int index,
                      int transform, hwc_rect_t *src, hwc_rect_t *dst);

void hwc_rotate_rect(int transform, const hwc_rect_t *src, hwc_rect_t *dst,
                     int width, int height);

void enable_idle_detection(struct hwc_idle_machine_t *idle);
void disable_idle_detection(struct hwc_idle_machine_t *idle);
int update_idle_state(struct hwc_context_t* ctx);
void fb_cache_check(struct hwc_display_t *dpy,
                    hwc_display_contents_t* list);
NvBool get_property_bool(const char *key, NvBool default_value);

int hwc_set_display(struct hwc_context_t *ctx, int disp,
                    hwc_display_contents_t* display);

static inline NvBool hwc_layer_is_yuv(hwc_layer_t* l)
{
    NvColorFormat format = get_colorformat(l);

    switch (NV_COLOR_GET_COLOR_SPACE(format)) {
    case NvColorSpace_YCbCr601:
    case NvColorSpace_YCbCr601_RR:
    case NvColorSpace_YCbCr601_ER:
    case NvColorSpace_YCbCr709:
        return NV_TRUE;
    }

    return NV_FALSE;
}

static inline NvBool hwc_layer_is_protected(hwc_layer_t* l)
{
    return !!(hwc_get_usage(l) & GRALLOC_USAGE_PROTECTED);
}

static inline NvGrScratchSet *
scratch_assign(struct hwc_display_t *dpy, int transform, size_t width,
               size_t height, NvColorFormat format, NvRect *src_crop)
{
    return dpy->scratch->assign(dpy->scratch, transform, width, height,
                                format, src_crop);
}

static inline void scratch_frame_start(struct hwc_display_t *dpy)
{
    dpy->scratch->frame_start(dpy->scratch);
}

static inline void scratch_frame_end(struct hwc_display_t *dpy)
{
    dpy->scratch->frame_end(dpy->scratch);
}

int hwc_set_display_mode(struct hwc_context_t *ctx, int disp,
                         struct fb_var_screeninfo *mode,
                         NvPoint *image_size, NvPoint *device_size);

/* Legacy HDMI support */
int hwc_legacy_prepare(hwc_composer_device_t *dev, size_t numDisplays,
                       hwc_display_contents_t **displays);
int hwc_legacy_set(hwc_composer_device_t *dev, size_t numDisplays,
                   hwc_display_contents_t **displays);
#endif /* ifndef _NVHWC_H */
