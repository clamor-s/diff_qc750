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

#include <fcntl.h>
#include <errno.h>
#include <cutils/log.h>
#include <cutils/atomic.h>
#include <cutils/properties.h>
#include <EGL/egl.h>
#include <linux/tegra_dc_ext.h>
#include <sys/resource.h>

#include "nvhwc.h"
#include "nvassert.h"
#include "nvhwc_debug.h"
#include "nvhwc_external.h"
#include "nvgrsurfutil.h"

/*****************************************************************************/


#ifndef ALLOW_VIDEO_SCALING
#define ALLOW_VIDEO_SCALING 0
#endif

/* Maximum screen dimension at which window B will be allowed to
 * display 32bpp content, due to the risk of underflow.  Ideally we
 * would never use window B for 32bpp but in practice it seems to work
 * for low resolution displays.
 */
#define WINDOW_B_LIMIT_32BPP 1366

#define PATH_FTRACE_MARKER_LOG   "/sys/kernel/debug/tracing/trace_marker"

#if NVDPS_ENABLE
#define PATH_NVDPS  "/sys/class/graphics/fb0/device/nvdps"
#define NVDPS_MIN_REFRESH  "1"
#define NVDPS_MAX_REFRESH  "60"
#endif

#define NV_PROPERTY_COMPOSITE_POLICY            TEGRA_PROP(composite.policy)
#define NV_PROPERTY_COMPOSITE_POLICY_DEFAULT    "auto"

#define NV_PROPERTY_COMPOSITOR                  TEGRA_PROP(compositor)
#define NV_PROPERTY_COMPOSITOR_DEFAULT          "surfaceflinger"

/* The property controls the detection of an idle display based on
 * frame rate.  Values can be a positive integer or 0 to disable.
 */
#define NV_PROPERTY_IDLE_MINIMUM_FPS            TEGRA_PROP(idle.minimum_fps)
#define NV_PROPERTY_IDLE_MINIMUM_FPS_DEFAULT    IDLE_MINIMUM_FPS

/* Select legacy (pre-JB MR1) HDMI behavior instead of multidisplay */
#define NV_PROPERTY_HDMI_LEGACY                 TEGRA_PROP(hdmi.legacy)
#define NV_PROPERTY_HDMI_LEGACY_DEFAULT         0

/* Force a default rotation on the primary display */
#define NV_PROPERTY_PANEL_ROTATION              TEGRA_PROP(panel.rotation)
#define NV_PROPERTY_PANEL_ROTATION_DEFAULT      0

/* convenience macros */
static int hwc_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device);

static void conf_prepare(struct hwc_context_t *ctx,
                         struct hwc_display_t *dpy,
                         struct hwc_prepare_state *prepare);

static void composite_prepare(struct hwc_context_t *ctx,
                              struct hwc_display_t *dpy,
                              struct hwc_prepare_state *prepare);

static int add_to_composite(struct hwc_display_t *dpy,
                            struct hwc_prepare_layer *ll);

static struct hw_module_methods_t hwc_module_methods = {
    open: hwc_device_open
};

hwc_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: HWC_HARDWARE_MODULE_ID,
        name: "NVIDIA Tegra hwcomposer module",
        author: "NVIDIA",
        methods: &hwc_module_methods,
    }
};

static inline int r_empty(const hwc_rect_t* r)
{
    return (r->left >= r->right || r->top >= r->bottom);
}

static inline int r_intersect(const hwc_rect_t* r1,
                              const hwc_rect_t* r2)
{
    return !(r2->left >= r1->right || r2->right <= r1->left ||
             r2->top >= r1->bottom || r2->bottom <= r1->top);
}

static inline int nv_intersect(const NvRect* r1,
                               const NvRect* r2)
{
    return !(r2->left >= r1->right || r2->right <= r1->left ||
             r2->top >= r1->bottom || r2->bottom <= r1->top);
}

static inline void r_grow(hwc_rect_t* dst, const hwc_rect_t* src)
{
    if (r_empty(dst)) {
        *dst = *src;
    } else {
        dst->left = NV_MIN(dst->left, src->left);
        dst->top = NV_MIN(dst->top, src->top);
        dst->right = NV_MAX(dst->right, src->right);
        dst->bottom = NV_MAX(dst->bottom, src->bottom);
    }
}

static inline void nv_translate(NvRect *r, int dx, int dy)
{
    r->left += dx;
    r->right += dx;
    r->top += dy;
    r->bottom += dy;
}

static inline int r_equal(const hwc_rect_t* r1,
                          const hwc_rect_t* r2)
{
    return (r1->left == r2->left && r1->right == r2->right &&
            r1->top == r2->top && r1->bottom == r2->bottom);
}

void hwc_get_scale(int transform, const hwc_rect_t *src, const hwc_rect_t *dst,
                   float *scale_w, float *scale_h)
{
    int src_w = r_width(src);
    int src_h = r_height(src);
    int dst_w, dst_h;

    if (transform & HWC_TRANSFORM_ROT_90) {
        dst_w = r_height(dst);
        dst_h = r_width(dst);
    } else {
        dst_w = r_width(dst);
        dst_h = r_height(dst);
    }

    *scale_w = (float) dst_w / src_w;
    *scale_h = (float) dst_h / src_h;
}

/* translate layer requirements to overlay caps */
static unsigned get_layer_requirements(struct hwc_prepare_layer *ll,
                                       float scale_w, float scale_h)
{
    hwc_layer_t* l = ll->layer;
    NvColorFormat format = get_colorformat(l);
    unsigned req = 0;

    if (ll->transform & HWC_TRANSFORM_ROT_90) {
        req |= NVFB_WINDOW_CAP_SWAPXY;
    }

    if (hwc_need_scale(scale_w, scale_h)) {
        req |= NVFB_WINDOW_CAP_SCALE;
    }

    if (hwc_layer_is_yuv(l))
        req |= NVFB_WINDOW_CAP_YUV_FORMATS;

    /* Not a hard requirement, but a hint to bias overlay selection */
    if (NV_COLOR_GET_BPP(format) <= 16) {
        req |= NVFB_WINDOW_CAP_16BIT;
    }

    return req;
}

/* get number of free windows, adjusted for reserving framebuffer */
static inline int num_free_windows(struct hwc_display_t *dpy,
                                   int num, int more) {
    if (dpy->fb_index == -1 && more)
        num--;
    return num;
}

/* get the least capable window meeting the requirements in a best
 * possible way, remove from mask */
int hwc_get_window(const struct nvfb_display_caps *caps,
                    unsigned *mask, unsigned req, unsigned *unsatisfied)
{
    int ii, best = -1;
    NvBool exact;

    if (unsatisfied) {
        *unsatisfied = req;
        exact = NV_FALSE;
    } else {
        exact = NV_TRUE;
    }

    for (ii = 0; ii < (int)caps->num_windows; ii++) {
        if ((1 << ii) & *mask) {
            unsigned u = (req ^ caps->window_cap[ii]) & req;
            if (exact) {
                /* Consider only overlays meeting all requirements */
                if (!u &&
                    (best == -1 || (caps->window_cap[best] > caps->window_cap[ii])))
                    best = ii;
                continue;
            }
            if (!u) {
                /* Start considering only overlays meeting all requirements */
                exact = NV_TRUE;
                best = ii;
                *unsatisfied = 0;
                continue;
            }
            /* Consider overlays meeting best subset of requirements */
            if (best == -1 ||
                (*unsatisfied > u) ||
                ((*unsatisfied == u) && (caps->window_cap[best] > caps->window_cap[ii]))) {
                best = ii;
                *unsatisfied = u;
            }
        }
    }

    if (best != -1)
        *mask &= ~(1 << best);
    return best;
}

static void r_intersection(hwc_rect_t* r, const hwc_rect_t* r0, const hwc_rect_t* r1)
{
    if (!r_intersect(r0,r1)) {
        /* we define intersection of non-intersecting rectangles to be
         * degenerate rectangle at (-1,-1) */
        r->left = r->top = r->right = r->bottom = -1;
        return;
    }
    r->left = NV_MAX(r0->left, r1->left);
    r->top = NV_MAX(r0->top, r1->top);
    r->right = NV_MIN(r0->right, r1->right);
    r->bottom = NV_MIN(r0->bottom, r1->bottom);
}

static inline NvBool can_use_window(hwc_layer_t* l)
{
    /* TODO erratas? */

    NV_ASSERT(!(l->flags & HWC_SKIP_LAYER));

    if (!l->handle) {
        return NV_FALSE;
    }

    /* format not supported by gralloc */
    if (NvGrTranslateHalFmt(get_halformat(l)) == -1) {
        return NV_FALSE;
    }

    if (r_width(&l->displayFrame) < 4 ||
        r_height(&l->displayFrame) < 4) {
        return NV_FALSE;
    }

    return NV_TRUE;
}

/* assign hw window to layer, if possible */
static int assign_window(struct hwc_context_t *ctx,
                         struct hwc_display_t *dpy,
                         struct hwc_prepare_state *prepare,
                         int idx)
{
    struct hwc_prepare_layer *ll = &prepare->layers[idx];
    struct hwc_window_mapping *map = &dpy->map[prepare->current_map];
    hwc_layer_t *l = ll->layer;
    NvRmSurface *surf = get_nvrmsurface(l);
    NvColorFormat format = surf->ColorFormat;

    unsigned req, unsatisfied_req;
    int window;
    int need_scale = 0;
    int need_swapxy = 0;
    float scale_w = 1.0f;
    float scale_h = 1.0f;

    if (!can_use_window(l)) {
        return -1;
    }

    hwc_get_scale(ll->transform, &ll->src, &ll->dst, &scale_w, &scale_h);
    req = get_layer_requirements(ll, scale_w, scale_h);

    /* scaling limits of dc */
    if (req & NVFB_WINDOW_CAP_SCALE) {
        float min_scale = NVFB_WINDOW_DC_MIN_SCALE;


        /* WAR for bug #968666 - disallow minification of >16bpp.
         * Scaling in window B was designed for video and as such the
         * FIFO is undersized.  The combination of 32bpp +
         * minification + high resolution appears to push beyond
         * reasonable limits and leads to underflow.  Under these
         * extreme conditions, scaling in 2D is much kinder.
         *
         * WAR for bug 980696 - disallow minification on high
         * resolution displays.
         */
        if (ctx->limit_window_b || NV_COLOR_GET_BPP(format) > 16) {
            min_scale = 1.0;
        }

        /* WAR for bug #929654 - disallow scaling of >16bpp surfaces.
         * On high resolution displays, any use of window B for 32bpp
         * surfaces can lead to underflow.
         *
         * WAR for bug #395578 - downscaling beyond the hardware limit
         * can lead to underflow.  Use 2D scaling in this case.
         */
        if ((ctx->limit_window_b && NV_COLOR_GET_BPP(format) > 16) ||
            scale_w < min_scale || scale_h < min_scale) {
            need_scale = 1;
            req &= ~NVFB_WINDOW_CAP_SCALE;
        }
    }

    /* If we need 32bpp horizontal upscaling and window C is
     * available, use it to scale horizontally and use 2D to scale
     * vertically.  This reduces the amount of data to be copied.
     */
    if (need_scale && NV_COLOR_GET_BPP(format) > 16 &&
        (prepare->window_mask & NVFB_WINDOW_C_MASK) &&
        ((ll->transform & HWC_TRANSFORM_ROT_90) ?
         (scale_h >= 1.0) : (scale_w >= 1.0))) {

        /* Assign window C */
        window = NVFB_WINDOW_C_INDEX;
        prepare->window_mask &= ~NVFB_WINDOW_C_MASK;
        unsatisfied_req = req & ~(dpy->caps.window_cap[NVFB_WINDOW_C_INDEX]);

        /* Cancel horizontal scaling */
        if (ll->transform & HWC_TRANSFORM_ROT_90) {
            scale_h = 1.0;
        } else {
            scale_w = 1.0;
        }
    } else {
        window = hwc_get_window(&dpy->caps, &prepare->window_mask, req, &unsatisfied_req);
    }

    /* If no scaling overlays are available, we can use 2D instead. */
    if (unsatisfied_req & NVFB_WINDOW_CAP_SCALE) {
        unsatisfied_req &= ~NVFB_WINDOW_CAP_SCALE;
        need_scale = 1;
    }

    /* Display controller does not support 90 degree rotation, use 2D instead. */
    if (unsatisfied_req & NVFB_WINDOW_CAP_SWAPXY) {
        unsatisfied_req &= ~NVFB_WINDOW_CAP_SWAPXY;
        need_swapxy = 1;
    }

    /* 16bit is a hint, not a hard requirement, designed to bias
     * selection for 16bpp surfaces toward window B. If it fails,
     * window B is already in use. We can use window from get_window().
     */
    unsatisfied_req &= ~NVFB_WINDOW_CAP_16BIT;

    /* Bail out if a window cannot be offered even with 2D support. */
    if (unsatisfied_req) {
        prepare->window_mask |= 1 << window;
        return -1;
    }

    if (need_scale || need_swapxy) {
        int w, h;
        NvRect crop;

        /* If a scaling blit is required, scale only the source crop.
         * This is an optimization for apps which render at smaller
         * than screen resolution and depend on display to scale the
         * result.
         *
         * For rotate-only blits, rotate the entire surface.  This is
         * an optimization for static wallpaper which pans a
         * screen-sized source crop to effect parallax scrolling.  If
         * we rotate only the source crop, we end up blitting every
         * frame even though the base image never changes.
         *
         * There remains a small chance we'll pick the wrong path
         * here: if rotation-only is required for an app which is
         * animating, and the source crop is smaller than the surface,
         * we might have been better off with the crop mode.  This
         * seems unlikely in practice, and we could probably improve
         * the hueristic by applying the uncropped blit only when the
         * source surface exceeds the display size.  Unless and until
         * this is a problem in practice, we'll keep the hueristic
         * simple.  In the worst case, picking the wrong path is a
         * performance bug.
         */
        if (need_scale) {
            w = scale_w * r_width(&ll->src);
            h = scale_h * r_height(&ll->src);
            copy_rect(&ll->src, &crop);
        } else {
            w = surf->Width;
            h = surf->Height;
        }

        map->scratch = scratch_assign(dpy,
            need_swapxy ? ll->transform & HWC_TRANSFORM_ROT_90 : 0,
            w, h, format, need_scale ? &crop : NULL);

        if (map->scratch) {
            map->surf_index = ll->surf_index;

            if (need_scale) {
                ll->src.left = 0;
                ll->src.top = 0;
                ll->src.right = w;
                ll->src.bottom = h;
            }

            /* When a surface is rotated clockwise, the left and right
             * extents of the post-rotation source crop must be offset
             * relative to the pre-rotated surface height.
            */
            if (need_swapxy) {
                int l = h - ll->src.bottom;
                int t = ll->src.left;
                int r = h - ll->src.top;
                int b = ll->src.right;

                ll->src.left = l;
                ll->src.top = t;
                ll->src.right = r;
                ll->src.bottom = b;
            }
        } else {
            prepare->window_mask |= 1 << window;
            return -1;
        }
    } else {
        map->scratch = NULL;

        if (NVGR_USE_TRIPLE_BUFFERING) {
            l->hints |= HWC_HINT_TRIPLE_BUFFER;
        }
    }

    return window;
}

/* Find a layer with the specified usage.  If unique is TRUE, ensure
 * that only a single layer has the specified usage; finding multiple
 * such layers is the same as finding none.  If unique is FALSE,
 * return the first matching layer found, searching from bottom to
 * top.
 */

int hwc_find_layer(hwc_display_contents_t *list, int usage, NvBool unique)
{
    int ii;
    int found = -1;

    for (ii = 0; ii < (int) list->numHwLayers; ii++) {
        hwc_layer_t* cur = &list->hwLayers[ii];

        if (cur->flags & HWC_SKIP_LAYER)
            continue;

        if (hwc_get_usage(cur) & usage) {
            if (unique) {
                if (found >= 0) {
                    return -1;
                }
                found = ii;
            } else {
                return ii;
            }
        }
    }

    return found;
}

static void sysfs_write_string(const char *pathname, const char *buf)
{
    int fd = open(pathname, O_WRONLY);
    if (fd >= 0) {
        write(fd, buf, strlen(buf));
        close(fd);
    }
}

#if STATUSBAR_BLEND_OPT
/* A layer resembles a statusbar if its up against one edge and is
 * relatively small.
 */
static int is_statusbar(struct hwc_display_t *dpy,
                        struct hwc_prepare_layer *ll)
{
    if (ll->dst.top == 0 &&
        ll->dst.bottom == dpy->device_clip.bottom &&
        (ll->dst.left == 0 ||
         ll->dst.right == dpy->device_clip.right) &&
        r_width(&ll->dst) < dpy->device_clip.right / 8) {
        return NV_TRUE;
    }

    if (ll->dst.left == 0 &&
        ll->dst.right == dpy->device_clip.right &&
        (ll->dst.top == 0 ||
         ll->dst.bottom == dpy->device_clip.bottom) &&
        r_height(&ll->dst) < dpy->device_clip.bottom / 8) {
        return NV_TRUE;
    }

    return NV_FALSE;
}

/* Check if we can handle overlapping blend */
static int statusbar_blend(struct hwc_display_t *dpy,
                           struct hwc_prepare_state *prepare)
{
    unsigned ii;
    struct hwc_prepare_layer *statusbar = &prepare->layers[2];

    NV_ASSERT(prepare->numLayers == 3);

    if (!is_statusbar(dpy, statusbar)) {
        return 0;
    }

    for (ii = 0; ii < 2; ii++) {
        struct hwc_prepare_layer *ll = &prepare->layers[ii];
        hwc_rect_t overlap;

        /* Ensure the layers completely overlap the statusbar region */
        r_intersection(&overlap, &ll->dst, &statusbar->dst);
        if (memcmp(&overlap, &statusbar->dst, sizeof(hwc_rect_t))) {
            return 0;
        }
    }

    return 1;
}
#endif

/* Check if any layers might trigger an overlapping blend with the
 * base layer.
 *
 * Return 1 if detected, 0 otherwise.
 */
static int test_overlapping_blend(struct hwc_display_t *dpy,
                                  struct hwc_prepare_state *prepare,
                                  int base)
{
    int ii;
    hwc_rect_t blend_area;

    /* Optimization: if there is only one layer above the base layer
     * and it does not blend, allow it to overlap even if it might
     * composite.  In this case blending will be disabled for the
     * framebuffer.
     */
    if (base == prepare->numLayers - 2) {
        hwc_layer_t* cur = prepare->layers[base+1].layer;

        NV_ASSERT(!(cur->flags & HWC_SKIP_LAYER));
        if (cur->blending == HWC_BLENDING_NONE) {
            return 0;
        }
    }

    /* Optimization: if there is only one layer behind the base layer
     * and the base layer uses PREMULT, intersect the base with the
     * bottom layer because it may not cover the entire screen.  Any
     * region where the two layers do not intersect can be considered
     * non-blending.
     */
    if (base == 1 &&
        prepare->layers[base].blending == HWC_BLENDING_PREMULT) {
        r_intersection(&blend_area,
                       &prepare->layers[0].dst,
                       &prepare->layers[1].dst);
    } else {
        blend_area = prepare->layers[base].dst;
    }

    /* Check if any layers overlap the base even if they are not
     * blending because any layer might cause compositing.
     */
    for (ii = base + 1; ii < prepare->numLayers; ii++) {
        struct hwc_prepare_layer *ll = &prepare->layers[ii];
        hwc_layer_t* cur = ll->layer;
        hwc_rect_t clipped;

        NV_ASSERT(cur->compositionType != HWC_FRAMEBUFFER_TARGET);
        NV_ASSERT(!(cur->flags & HWC_SKIP_LAYER));

        r_intersection(&clipped, &dpy->device_clip, &ll->dst);
        if (r_intersect(&blend_area, &clipped)) {
#if STATUSBAR_BLEND_OPT
            /* Optimization: if there are exactly three layers, and
             * the top two layers blend and overlap, and the top layer
             * is the status bar, allow the base layer to use a window
             * and resolve the conflict by pre-compositing the status
             * bar overlap rect.
             */
            if (prepare->numLayers == 3 && base == 1) {
                /* cur must be blending or we would have already
                 * optimized this case above.
                 */
                NV_ASSERT(cur->blending != HWC_BLENDING_NONE);
                if (statusbar_blend(dpy, prepare)) {
                    prepare->statusbar = ll;
                    return 0;
                }
            }
#endif
            return 1;
        }
        r_grow(&blend_area, &clipped);
    }

    return 0;
}

/* Record a layer which has requested a framebuffer clear */
static inline void clear_layer_add(struct hwc_display_t *dpy, int ii)
{
    /* Note that failing to record a clear layer is non-fatal since
     * this is merely an optimization.  If there are more than 32
     * layers, performance may be an issue either way.
     */
    if (ii < (int)sizeof(dpy->clear_layers) * 8) {
        dpy->clear_layers |= (1 << ii);
    }
}

/* Request that SurfaceFlinger enable clears for the regions
 * corresponding to layers in windows.
 */
static void clear_layer_enable(struct hwc_display_t *dpy,
                               hwc_display_contents_t *display)
{
    int ii = 0;
    uint32_t mask = dpy->clear_layers;

    while (mask) {
        uint32_t bit = (1 << ii);
        if (mask & bit) {
            hwc_layer_t *cur = &display->hwLayers[ii];
            NV_ASSERT(cur->compositionType == HWC_OVERLAY);
            cur->hints |= HWC_HINT_CLEAR_FB;
            mask &= ~bit;
        }
        ii++;
    }
}

/* Request that SurfaceFlinger disable clears for the regions
 * corresponding to layers in windows.
 */
static void clear_layer_disable(struct hwc_display_t *dpy,
                                hwc_display_contents_t *display)
{
    int ii = 0;
    uint32_t mask = dpy->clear_layers;

    while (mask) {
        uint32_t bit = (1 << ii);
        if (mask & bit) {
            hwc_layer_t *cur = &display->hwLayers[ii];
            NV_ASSERT(cur->compositionType == HWC_OVERLAY);
            cur->hints &= ~HWC_HINT_CLEAR_FB;
            mask &= ~bit;
        }
        ii++;
    }
}

/* Determined if any framebuffer layers have changed since the last
 * frame.
 */
static int fb_cache_clean(struct hwc_fb_cache_t *cache, hwc_display_contents_t* list)
{
    uintptr_t diffs = 0;
    int ii;

    /* Compute a quick comparison of the buffer handles */
    for (ii = 0; ii < cache->numLayers; ii++) {
        int index = cache->layers[ii].index;
        hwc_layer_t* cur = &list->hwLayers[index];

        diffs |= (uintptr_t)cache->layers[ii].handle ^ (uintptr_t)cur->handle;
        cache->layers[ii].handle = cur->handle;
    }

    return !diffs;
}

/* Determine the compositing strategy for this frame and set the layer
 * bits accordingly.  If no composited layers have changed, we can
 * reuse the previous composited result.
 */
void fb_cache_check(struct hwc_display_t *dpy, hwc_display_contents_t* list)
{
    struct hwc_fb_cache_t *cache = &dpy->fb_cache[OLD_CACHE_INDEX(dpy)];
    int recycle = cache->numLayers && fb_cache_clean(cache, list);

    if (dpy->fb_recycle != recycle) {
        if (recycle) {
            FBCLOG("enabled");
        } else {
            FBCLOG("disabled");
        }

        /* Modify layer state only if we're using surfaceflinger */
        if (dpy->composite.engine == HWC_Compositor_SurfaceFlinger) {
            int ii;

            NV_ASSERT(!dpy->composite.list.nLayers);

            if (recycle) {
                for (ii = 0; ii < cache->numLayers; ii++) {
                    int index = cache->layers[ii].index;
                    list->hwLayers[index].compositionType = HWC_OVERLAY;
                }
                clear_layer_disable(dpy, list);
            } else {
                for (ii = 0; ii < cache->numLayers; ii++) {
                    int index = cache->layers[ii].index;
                    list->hwLayers[index].compositionType = HWC_FRAMEBUFFER;
                }
                clear_layer_enable(dpy, list);
            }
        }

        dpy->fb_recycle = recycle;
    }
}

/* Add a layer to the fb_cache */
static void fb_cache_add_layer(struct hwc_display_t *dpy,
                               hwc_layer_t *cur,
                               int li)
{
    struct hwc_fb_cache_t *cache = &dpy->fb_cache[NEW_CACHE_INDEX(dpy)];

    if (0 <= cache->numLayers && cache->numLayers < FB_CACHE_LAYERS) {
        /* Bug 983744 - Disable the fb cache if any layers have the
         * SKIP flag, in which case the rest of the hwc_layer_t is
         * undefined so we cannot reliably determine if it has
         * changed.
         */
        if (cur->flags & HWC_SKIP_LAYER) {
            cache->numLayers = -1;
            return;
        } else {
            int index = cache->numLayers++;

            cache->layers[index].index = li;
            cache->layers[index].handle = cur->handle;
            cache->layers[index].transform = cur->transform;
            cache->layers[index].blending = cur->blending;
            cache->layers[index].sourceCrop = cur->sourceCrop;
            cache->layers[index].displayFrame = cur->displayFrame;
        }
    }
}

static void fb_cache_update_layers(struct hwc_display_t *dpy,
                                   hwc_display_contents_t *display)
{
    struct hwc_fb_cache_t *cache = &dpy->fb_cache[OLD_CACHE_INDEX(dpy)];
    int ii;

    NV_ASSERT(dpy->fb_recycle);

    /* hwc_prepare marked these layers as FRAMEBUFFER,
     * reset to OVERLAY
     */
    for (ii = 0; ii < cache->numLayers; ii++) {
        int index = cache->layers[ii].index;
        NV_ASSERT(display->hwLayers[index].compositionType ==
                  HWC_FRAMEBUFFER);
        display->hwLayers[index].compositionType = HWC_OVERLAY;
    }
}

/* Check if previous config's fb_cache is still valid in this config */
static void fb_cache_validate(struct hwc_display_t *dpy,
                              struct hwc_prepare_state *prepare)
{
    struct hwc_fb_cache_t *old_cache = &dpy->fb_cache[OLD_CACHE_INDEX(dpy)];
    struct hwc_fb_cache_t *new_cache = &dpy->fb_cache[NEW_CACHE_INDEX(dpy)];

    /* Check if we can use the cache at all */
    if (dpy->fb_layers != new_cache->numLayers || prepare->statusbar) {
        if (dpy->fb_layers > FB_CACHE_LAYERS) {
            FBCLOG("de-activated, (%d layers exceeds limit)", dpy->fb_layers);
        } else if (prepare->statusbar) {
            FBCLOG("de-activated, (statusbar detected)");
        }
        old_cache->numLayers = 0;
        dpy->fb_recycle = 0;
        return;
    }

    NV_ASSERT(old_cache->numLayers >= 0 &&
              new_cache->numLayers >= 0);

    /* Check if the old config is still valid */
    if (old_cache->numLayers && old_cache->numLayers == new_cache->numLayers) {
        int match = 1;
        int ii;

        for (ii = 0; ii < old_cache->numLayers; ii++) {
            struct hwc_fb_cache_layer_t *a = &old_cache->layers[ii];
            struct hwc_fb_cache_layer_t *b = &new_cache->layers[ii];

            /* Check all fields except the handle */
            if (a->index != b->index ||
                a->transform != b->transform ||
                a->blending != b->blending ||
                !r_equal(&a->sourceCrop, &b->sourceCrop) ||
                !r_equal(&a->displayFrame, &b->displayFrame)) {
                match = 0;
                break;
            }
        }

        /* If the configs match, check the handles and return */
        if (match) {
            dpy->fb_recycle = fb_cache_clean(old_cache, prepare->display);
            /* Updating the layer state is deferred until composition
             * validation is done.
             */
            if (dpy->fb_recycle) {
                FBCLOG("re-using previous config (enabled)");
            } else {
                FBCLOG("re-using previous config (disabled)");
            }
            return;
        }
    }

    if (new_cache->numLayers) {
        if (!old_cache->numLayers) {
            FBCLOG("activated (%d layers)", new_cache->numLayers);
        }
    } else if (old_cache->numLayers) {
        FBCLOG("de-activated (0 layers)");
    }

    /* Swap to the new cache and disable it */
    dpy->fb_cache_index = NEW_CACHE_INDEX(dpy);
    dpy->fb_recycle = 0;
}

static void add_to_framebuffer(struct hwc_display_t *dpy,
                               struct hwc_prepare_state *prepare,
                               hwc_layer_t *cur,
                               int index)
{
    cur->compositionType = HWC_FRAMEBUFFER;
    if (dpy->fb_index == -1) {
        struct hwc_prepare_map *layer_map;

        dpy->fb_index = prepare->current_map--;
        layer_map = &prepare->layer_map[dpy->fb_index];
        layer_map->window = hwc_get_window(&dpy->caps, &prepare->window_mask,
                                           0, NULL);
        if (prepare->overflow || (cur->flags & HWC_SKIP_LAYER)) {
            prepare->fb.bounds = dpy->layer_clip;
            prepare->fb.blending = HWC_BLENDING_PREMULT;
        } else {
            r_intersection(&prepare->fb.bounds, &dpy->layer_clip,
                           &cur->displayFrame);
            /* Set blending only if the current layer blends.  This
             * allows a single-layer framebuffer to be considered "not
             * blending" during the overlapping blend check.
             */
            prepare->fb.blending = HWC_BLENDING_PREMULT;
        }
        dpy->map[dpy->fb_index].index = -1;
        dpy->map[dpy->fb_index].scratch = NULL;
    } else {
        /* Always enable blending if the fb contains multiple layers
         * even if they are not blending, because the fb region may be
         * discontiguous.
         */
        prepare->fb.blending = HWC_BLENDING_PREMULT;
        if (cur->flags & HWC_SKIP_LAYER) {
            prepare->fb.bounds = dpy->layer_clip;
        } else {
            hwc_rect_t clipped;
            r_intersection(&clipped, &dpy->layer_clip, &cur->displayFrame);
            r_grow(&prepare->fb.bounds, &clipped);
        }
    }
    dpy->fb_layers++;

    fb_cache_add_layer(dpy, cur, index);
}

NvBool get_property_bool(const char *key, NvBool default_value)
{
    char value[PROPERTY_VALUE_MAX];

    if (property_get(key, value, NULL) > 0) {
        if (!strcmp(value, "1") || !strcasecmp(value, "on") ||
            !strcasecmp(value, "true")) {
            return NV_TRUE;
        }
        if (!strcmp(value, "0") || !strcasecmp(value, "off") ||
            !strcasecmp(value, "false")) {
            return NV_FALSE;
        }
    }

    return default_value;
}

static int get_property_int(const char *key, int default_value)
{
    char value[PROPERTY_VALUE_MAX];

    if (property_get(key, value, NULL) > 0) {
        return strtol(value, NULL, 0);
    }

    return default_value;
}

static int get_env_property_int(const char *key, int default_value)
{
    const char *env = getenv(key);
    int value;

    if (env) {
        value = atoi(env);
    } else {
        value = get_property_int(key, default_value);
    }

    return value;
}

/* Utilities from pthread_internal.h that should really be exposed */
static __inline__ void timespec_add( struct timespec*  a, const struct timespec*  b )
{
    a->tv_sec  += b->tv_sec;
    a->tv_nsec += b->tv_nsec;
    if (a->tv_nsec >= 1000000000) {
        a->tv_nsec -= 1000000000;
        a->tv_sec  += 1;
    }
}

static  __inline__ void timespec_sub( struct timespec*  a, const struct timespec*  b )
{
    a->tv_sec  -= b->tv_sec;
    a->tv_nsec -= b->tv_nsec;
    if (a->tv_nsec < 0) {
        a->tv_nsec += 1000000000;
        a->tv_sec  -= 1;
    }
}

static  __inline__ void timespec_zero( struct timespec*  a )
{
    a->tv_sec = a->tv_nsec = 0;
}

static  __inline__ int timespec_is_zero( const struct timespec*  a )
{
    return (a->tv_sec == 0 && a->tv_nsec == 0);
}

static  __inline__ int timespec_cmp( const struct timespec*  a, const struct timespec*  b )
{
    if (a->tv_sec  < b->tv_sec)  return -1;
    if (a->tv_sec  > b->tv_sec)  return +1;
    if (a->tv_nsec < b->tv_nsec) return -1;
    if (a->tv_nsec > b->tv_nsec) return +1;
    return 0;
}

#define timespec_to_ms(t) (t.tv_sec*1000 + t.tv_nsec/1000000)
#define timespec_to_ns(t) ((int64_t)t.tv_sec*1000000000 + t.tv_nsec)

int update_idle_state(struct hwc_context_t* ctx)
{
    struct timespec now;
    int composite;
    int changed;
    int is_idle;
    char buff[20];
    struct hwc_idle_machine_t *idle = &ctx->idle;

    if (!idle->minimum_fps) {
        return 0;
    }

    /* Get current time and test against previous timeout */
    clock_gettime(CLOCK_MONOTONIC, &now);
    is_idle = timespec_cmp(&now, &idle->timeout) > 0;

#if NVDPS_ENABLE
    if (ctx->fd_nvdps >= 0) {
        /* If hwc is idle and disp is not set to idle mode, change
         * refresh to minimum rate. */
        if (is_idle && !ctx->nvdps_idle) {
            ctx->nvdps_idle = 1;
            snprintf(buff, sizeof(buff), NVDPS_MIN_REFRESH);
            write(ctx->fd_nvdps, buff, strlen(buff));
        }
        /* If hwc is not idle and disp is set to idle mode, change
         * refresh to normal rate. */
        if (!is_idle && ctx->nvdps_idle) {
            ctx->nvdps_idle = 0;
            snprintf(buff, sizeof(buff), NVDPS_MAX_REFRESH);
            write(ctx->fd_nvdps, buff, strlen(buff));
        }
    }
#endif

    composite = is_idle && idle->enabled;
    changed = composite != idle->composite;
    idle->composite = composite;

#ifdef DEBUG_IDLE
    if (changed) {
        struct timespec elapsed = now;
        timespec_sub(&elapsed, &idle->frame_times[idle->frame_index]);
        IDLELOG("windows %s, fps = %0.1f",
                composite ? "disabled" : "enabled",
                (1000.0 / timespec_to_ms(elapsed)) * IDLE_FRAME_COUNT);
    }
#endif

    idle->frame_times[idle->frame_index] = now;
    idle->frame_index = (idle->frame_index+1) % IDLE_FRAME_COUNT;

    /* timeout is the absolute time limit for the next frame */
    idle->timeout = idle->frame_times[idle->frame_index];
    timespec_add(&idle->timeout, &idle->time_limit);

    /* Update the timer */
    if (idle->timer && idle->enabled) {
        struct itimerspec its;
        timespec_zero(&its.it_interval);
        if (composite) {
            timespec_zero(&its.it_value);
            timer_settime(idle->timer, 0, &its, NULL);
        } else {
            its.it_value = idle->timeout;
            timer_settime(idle->timer, TIMER_ABSTIME, &its, NULL);
        }
    }

    return changed;
}

void enable_idle_detection(struct hwc_idle_machine_t *idle)
{
    if (!idle->enabled && idle->minimum_fps) {
        struct timespec now;
        int ii;

        IDLELOG("Enabling idle detection");
        android_atomic_release_store(1, &idle->enabled);

        if (idle->timer) {
            struct itimerspec its;
            timespec_zero(&its.it_interval);
            its.it_value = idle->timeout;
            timer_settime(idle->timer, TIMER_ABSTIME, &its, NULL);
        }
    }
}

void disable_idle_detection(struct hwc_idle_machine_t *idle)
{
    if (idle->enabled) {
        IDLELOG("Disabling idle detection");
        idle->composite = 0;
        android_atomic_release_store(0, &idle->enabled);

        if (idle->timer) {
            struct itimerspec its;
            timespec_zero(&its.it_interval);
            timespec_zero(&its.it_value);
            timer_settime(idle->timer, 0, &its, NULL);
        }
    }
}

/* Return NV_TRUE if any configured windows overlap each other */
static NvBool detect_overlap(struct hwc_display_t *dpy, int start)
{
    size_t ii, jj;

    for (ii = start; ii < dpy->caps.num_windows; ii++) {
        NV_ASSERT((int)ii == dpy->fb_index || dpy->map[ii].index != -1);

        for (jj = ii + 1; jj < dpy->caps.num_windows; jj++) {
            NV_ASSERT((int)jj == dpy->fb_index || dpy->map[jj].index != -1);

            if (nv_intersect(&dpy->conf.overlay[ii].dst,
                             &dpy->conf.overlay[jj].dst)) {
                return NV_TRUE;
            }
        }
    }

    return NV_FALSE;
}

static void add_window(struct hwc_display_t *dpy,
                       struct hwc_prepare_state *prepare,
                       int ii, int window)
{
    struct hwc_prepare_layer *ll = &prepare->layers[ii];
    hwc_layer_t* cur = ll->layer;

    cur->compositionType = HWC_OVERLAY;
    cur->hints |= HWC_HINT_CLEAR_FB;
    clear_layer_add(dpy, ll->index);

    dpy->map[prepare->current_map].index = ll->index;
    prepare->layer_map[prepare->current_map].window = window;
    prepare->layer_map[prepare->current_map].layer_index = ii;
    prepare->current_map--;
}

int hwc_assign_windows(struct hwc_context_t *ctx,
                       struct hwc_display_t *dpy,
                       struct hwc_prepare_state *prepare)
{
    int blends_overlap = -1;
    int ii;

    /* Attempt to assign layers to windows from back to front.  If a
     * layer must be composited, all remaining layers will be
     * composited for simplicitly.
     */
    for (ii = 0; ii < prepare->numLayers; ii++) {
        struct hwc_prepare_layer *ll = &prepare->layers[ii];
        hwc_layer_t *cur = ll->layer;
        int window = -1;
        int usage = hwc_get_usage(cur);

        /* These layer types should have been removed during
         * pre-processing step.
         */
        NV_ASSERT(cur->compositionType != HWC_FRAMEBUFFER_TARGET);
        NV_ASSERT(!(cur->flags & HWC_SKIP_LAYER));

        if (!prepare->use_windows) {
            add_to_framebuffer(dpy, prepare, cur, ll->index);
            continue;
        }

        /* Disable blending if this is the base layer and PREMULT is
         * requested.
         */
        if (prepare->current_map == (int)dpy->caps.num_windows - 1 &&
            ll->blending == HWC_BLENDING_PREMULT) {
            ll->blending = HWC_BLENDING_NONE;
        }

        /* If blending, test for overlapping blends which are not
         * supported by the display controller.  If detected, all
         * subsequent layers will be composited.
         */
        if (ll->blending != HWC_BLENDING_NONE) {
            if (blends_overlap < 0) {
                /* If there are more remaining layers than windows,
                 * composite the rest.  Otherwise test for overlap.
                 */
                if (prepare->current_map + 1 < prepare->numLayers - ii) {
                    blends_overlap = 1;
                } else {
                    blends_overlap = test_overlapping_blend(
                        dpy, prepare, ii);
                }
                if (blends_overlap) {
                    prepare->use_windows = 0;
                    add_to_framebuffer(dpy, prepare, cur, ll->index);
                    continue;
                }
            }
        }

#if STATUSBAR_BLEND_OPT
        if (ll == prepare->statusbar) {
            /* Disable local composition */
            dpy->composite.engine = HWC_Compositor_SurfaceFlinger;
            prepare->use_windows = 0;
            add_to_framebuffer(dpy, prepare, cur, ll->index);
            cur->compositionType = HWC_OVERLAY;
            continue;
        }
#endif
        if (num_free_windows(dpy, prepare->current_map+1,
                             prepare->numLayers-ii-1) > 0) {
            window = assign_window(ctx, dpy, prepare, ii);
        }

        /* WAR for display underflow on window B when used as a
         * framebuffer.  Detect if only window B remains available and
         * we're going to need a framebuffer, in which case we'll use
         * this window instead and force compositing.
         */
        if (ctx->limit_window_b &&                // need this optimization
                                                  // only window B is left
            prepare->window_mask == NVFB_WINDOW_B_MASK &&
            window != -1 &&                       // got assigned a window
            ii+1 < (int) prepare->numLayers &&    // more layers remaining
            dpy->fb_index == -1) {                // don't have a framebuffer
            hwc_layer_t* next = prepare->layers[ii+1].layer;

            /* Check if the next layer can use window B */
            if (prepare->numLayers > (int) dpy->caps.num_windows ||
                !can_use_window(next) ||
                NV_COLOR_GET_BPP(get_colorformat(next)) > 16) {
                ALOGW("Potential underflow detected, enacting countermeasures");
                // Return the window to the mask
                prepare->window_mask |= (1 << window);
                window = -1;
            }
        }

        if (window == -1) {
            prepare->use_windows = 0;
            add_to_framebuffer(dpy, prepare, cur, ll->index);
#if STATUSBAR_BLEND_OPT
            /* This is unexpected.  Cancel the statusbar optimization */
            NV_ASSERT(ll != prepare->statusbar);
            prepare->statusbar = NULL;
#endif
        } else {
            add_window(dpy, prepare, ii, window);
        }
    }

    if (dpy->fb_index != -1) {
        dpy->map[dpy->fb_index].index = dpy->fb_target;
    }

    /* Disable unused windows */
    for (ii = prepare->current_map; ii >= 0; ii--) {
        prepare->layer_map[ii].window =
            hwc_get_window(&dpy->caps, &prepare->window_mask, 0, NULL);
        dpy->map[ii].index = -1;
    }

    if (ctx->composite_policy & HWC_CompositePolicy_FbCache) {
        /* Setup the fb_cache for this config */
        fb_cache_validate(dpy, prepare);
    }

    if (!dpy->fb_recycle && dpy->composite.scratch) {
        dpy->scratch->unlock(dpy->scratch, dpy->composite.scratch);
        dpy->composite.scratch = NULL;
    }

    if (dpy->fb_index != -1 &&
        dpy->composite.engine != HWC_Compositor_SurfaceFlinger) {
        /* Try to use a local compositor */
        NV_ASSERT(!prepare->statusbar);
        composite_prepare(ctx, dpy, prepare);
    }

    if (ctx->composite_policy & HWC_CompositePolicy_FbCache) {
        if (dpy->fb_recycle) {
            fb_cache_update_layers(dpy, prepare->display);
        }
    }

    /* Clear the CLEAR_FB flag if we're not actually compositing, or
     * if composition is handled internally; otherwise SurfaceFlinger
     * will perform needless clears on the framebuffer and waste
     * power/bandwidth.
     */
    if (dpy->fb_index == -1 || dpy->composite.list.nLayers || dpy->fb_recycle) {
        clear_layer_disable(dpy, prepare->display);
    }

    /* Initialize gralloc config */
    conf_prepare(ctx, dpy, prepare);

    // TODO any meaninful value to return?
    return 0;
}

void hwc_rotate_rect(int transform, const hwc_rect_t *src, hwc_rect_t *dst,
                     int width, int height)
{
    int l, t, r, b;

    switch (transform) {
    default:
        NV_ASSERT(0);
    case 0:
        l = src->left;
        t = src->top;
        r = src->right;
        b = src->bottom;
        break;

    case HWC_TRANSFORM_ROT_90:
        l = height - src->bottom;
        t = src->left;
        r = height - src->top;
        b = src->right;
        break;

    case HWC_TRANSFORM_ROT_180:
        l = width - src->right;
        t = height - src->bottom;
        r = width - src->left;
        b = height - src->top;
        break;

    case HWC_TRANSFORM_ROT_270:
        l = src->top;
        t = width - src->right;
        r = src->bottom;
        b = width - src->left;
        break;
    }

    dst->left = l;
    dst->top = t;
    dst->right = r;
    dst->bottom = b;
}

static inline void prepare_copy_layer(struct hwc_display_t *dpy,
                                      struct hwc_prepare_layer *ll,
                                      hwc_layer_t *cur, int index)
{
    ll->index = index;
    ll->layer = cur;
    ll->blending = cur->blending;
    ll->surf_index = 0;
    ll->src = cur->sourceCrop;
    if (dpy->transform) {
        ll->transform = combine_transform(cur->transform,
                                          dpy->transform);
        hwc_rotate_rect(dpy->transform, &cur->displayFrame, &ll->dst,
                        dpy->config.res_x, dpy->config.res_y);
    } else {
        ll->dst = cur->displayFrame;
        ll->transform = cur->transform;
    }
}

void hwc_prepare_add_layer(struct hwc_display_t *dpy,
                           struct hwc_prepare_state *prepare,
                           hwc_layer_t *cur, int index)
{
    prepare_copy_layer(dpy, &prepare->layers[prepare->numLayers++], cur, index);
}

void hwc_prepare_begin(struct hwc_context_t *ctx,
                       struct hwc_display_t *dpy,
                       struct hwc_prepare_state *prepare,
                       hwc_display_contents_t *display,
                       int use_windows, int ignore_mask)
{
    size_t ii;
    int priority = -1;

    /* Reset window assignment state */
    dpy->fb_index = -1;
    dpy->fb_target = -1;
    dpy->fb_layers = 0;
    dpy->fb_cache[NEW_CACHE_INDEX(dpy)].numLayers = 0;
    dpy->clear_layers = 0;
    dpy->composite.list.nLayers = 0;
    dpy->composite.engine = ctx->compositor;

    memset(prepare, 0, sizeof(struct hwc_prepare_state));

    if (!use_windows ||
        (ctx->composite_policy & HWC_CompositePolicy_ForceComposite)) {
        prepare->use_windows = NV_FALSE;
    } else if (ctx->composite_policy & HWC_CompositePolicy_CompositeOnIdle) {
        prepare->use_windows = !ctx->idle.composite;
    } else {
        prepare->use_windows = NV_TRUE;
    }

    prepare->current_map = (int)dpy->caps.num_windows - 1;
    prepare->window_mask = (1 << dpy->caps.num_windows) - 1;

    for (ii = 0; ii < dpy->caps.num_windows; ii++) {
        prepare->layer_map[ii].layer_index = -1;
    }
    prepare->fb.layer.index = -1;
    prepare->display = display;

    /* If mirroring, look for layers which might require special
     * treatment on the external display.
     */
    if (dpy->type == HWC_DISPLAY_EXTERNAL) {
        if (ctx->mirror.enable) {
            if (hwc_prepare_video(ctx, dpy, prepare, display)) {
                scratch_frame_start(dpy);
                return;
            }
        }
        /* Set default mode */
        hwc_set_display_mode(ctx, HWC_DISPLAY_EXTERNAL, NULL, NULL, NULL);
    }

    /* Bug 1010997 - prioritize the first protected layer
     *
     * Find priority layer only if we are using windows or if we
     * cannot composite protected layers (SurfaceFlinger currently
     * does not support this).
     */
    if (prepare->use_windows ||
        dpy->composite.engine == HWC_Compositor_SurfaceFlinger) {
        priority = hwc_find_layer(display, GRALLOC_USAGE_PROTECTED, NV_FALSE);
        if (priority >= 0) {
            hwc_layer_t *cur = &display->hwLayers[priority];

            prepare->protect = NV_TRUE;

            /* Windows are assigned from back to front.  If the layer
             * is opaque it may be safely assigned the first layer
             * since by definition it occludes anything behind it.
             */
            if (priority == 0 || cur->blending == HWC_BLENDING_NONE) {
                hwc_prepare_add_layer(dpy, prepare, cur, priority);
                /* If there are layers behind this one, we must ensure
                 * that no occluded layers are subsequently assigned
                 * windows in front of this one.  Force compositing
                 * for all other layers which renders only the visible
                 * region of each layer.
                 */
                if (priority > 0) {
                    prepare->use_windows = NV_FALSE;
                }
            } else {
                /* If we cannot safely assign this layer to the first
                 * window, cancel priority handling and let the main
                 * loop deal with it.
                 */
                priority = -1;
            }
        }
    }

    /* Pre-process layer list */
    for (ii = 0; ii < display->numHwLayers; ii++) {
        hwc_layer_t *cur = &display->hwLayers[ii];

        /* The priority layer has already been processed. */
        if ((int)ii == priority) {
            continue;
        }

        /* Save the framebuffer target index but do not process with
         * normal layers.
         */
        if (cur->compositionType == HWC_FRAMEBUFFER_TARGET) {
            NV_ASSERT(dpy->fb_target == -1);
            dpy->fb_target = ii;
            prepare_copy_layer(dpy, &prepare->fb.layer, cur, ii);
            continue;
        }

        /* If any layers are marked SKIP, composite all layers */
        if (cur->flags & HWC_SKIP_LAYER) {
            prepare->use_windows = NV_FALSE;
            dpy->composite.engine = HWC_Compositor_SurfaceFlinger;
            add_to_framebuffer(dpy, prepare, cur, ii);
            continue;
        }

        /* Ignore layers with specific usage bits.  Clear the
         * corresponding portions of the framebuffer.
         */
        if (hwc_get_usage(cur) & ignore_mask) {
            cur->compositionType = HWC_OVERLAY;
            cur->hints |= HWC_HINT_CLEAR_FB;
            clear_layer_add(dpy, ii);
            continue;
        }

        if (prepare->numLayers < HWC_MAX_LAYERS) {
            hwc_prepare_add_layer(dpy, prepare, cur, ii);
        } else {
            prepare->overflow = NV_TRUE;
            /* If there are too many layers, the rest will be
             * composited.  In practice this should never happen.
             */
            dpy->composite.engine = HWC_Compositor_SurfaceFlinger;
            add_to_framebuffer(dpy, prepare, cur, ii);
            prepare->use_windows = NV_FALSE;
        }
    }

    scratch_frame_start(dpy);
}

void hwc_prepare_end(struct hwc_context_t *ctx,
                     struct hwc_display_t *dpy,
                     struct hwc_prepare_state *prepare,
                     hwc_display_contents_t *display)
{
    scratch_frame_end(dpy);

    if (display->flags & HWC_GEOMETRY_CHANGED) {
        dpy->windows_overlap = detect_overlap(dpy, prepare->current_map+1);
    }
}


static int hwc_prepare_display(struct hwc_context_t *ctx,
                               struct hwc_display_t *dpy,
                               hwc_display_contents_t *display,
                               int changed)
{
#if DUMP_LAYERLIST
    hwc_dump_display_contents(display);
#endif

    changed |= display->flags & HWC_GEOMETRY_CHANGED;

    dpy->composite.list.geometry_changed = changed;

    if (changed) {
        struct hwc_prepare_state prepare;

        hwc_prepare_begin(ctx, dpy, &prepare, display, 1, 0);
        hwc_assign_windows(ctx, dpy, &prepare);
        hwc_prepare_end(ctx, dpy, &prepare, display);
    } else {
        if (ctx->composite_policy & HWC_CompositePolicy_FbCache) {
            fb_cache_check(dpy, display);
        }
    }

    return 0;
}

static void update_didim_state(struct hwc_context_t *ctx,
                               hwc_display_contents_t *display)
{
    struct hwc_display_t *dpy = &ctx->displays[HWC_DISPLAY_PRIMARY];

    if (dpy->blank || !display) {
        return;
    }

    if (display->flags & HWC_GEOMETRY_CHANGED) {
        hwc_rect_t *didim_window = NULL;
        int video_index = hwc_find_layer(display,
                                         GRALLOC_USAGE_EXTERNAL_DISP,
                                         NV_TRUE);
        if (video_index >= 0) {
            hwc_layer_t *l = &display->hwLayers[video_index];
            int video_size = r_width(&l->displayFrame) *
                             r_height(&l->displayFrame);

            if (video_index >= 0 && hwc_layer_is_yuv(l)) {
                int screen_size = dpy->config.res_x * dpy->config.res_y;

                if (video_size > (screen_size >> 1)) {
                    didim_window = &l->displayFrame;
                }
            }
        }

        /* Update didim for normal mode or video mode */
        ctx->didim->set_window(ctx->didim, didim_window);
    }
}

static int hwc_prepare(hwc_composer_device_t *dev, size_t numDisplays,
                       hwc_display_contents_t **displays)
{
    struct hwc_context_t *ctx = (struct hwc_context_t *)dev;
    size_t ii;
    int err = 0;
    int changed = update_idle_state(ctx);
    NvBool windows_overlap = NV_FALSE;

    if (ctx->hotplug.cached.value != ctx->hotplug.latest.value) {
        ctx->hotplug.cached = ctx->hotplug.latest;
        changed = 1;
    }

    hwc_update_mirror_state(ctx, numDisplays, displays);

    for (ii = 0; ii < numDisplays; ii++) {
        if (displays[ii]) {
            struct hwc_display_t *dpy = &ctx->displays[ii];

            int ret = hwc_prepare_display(ctx, dpy, displays[ii], changed);
            if (ret) err = ret;
            windows_overlap |= dpy->windows_overlap;
        }
    }

    if (windows_overlap) {
        enable_idle_detection(&ctx->idle);
    } else {
        disable_idle_detection(&ctx->idle);
    }

    if (ctx->didim) {
        update_didim_state(ctx, displays[HWC_DISPLAY_PRIMARY]);
    }

#if DUMP_CONFIG
    char temp[2000];
    hwc_dump(dev, temp, NV_ARRAY_SIZE(temp));
    ALOGD("%s\n", temp);
#endif

    return err;
}

/* All composited layers must have the same transform and they
 * must not have tiled layout (3d texture support required
 * for composition).
 */
static NvBool can_composite(struct NvGrCompositeListRec *list)
{
    int ii;
    int transform = list->layers[0].transform;

    for (ii = 0; ii < list->nLayers; ii++) {
        struct NvGrCompositeLayerRec *layer = list->layers + ii;

        if (layer->transform != transform ||
            layer->buffer->Surf[0].Layout == NvRmSurfaceLayout_Tiled) {
            return NV_FALSE;
        }
    }

    return NV_TRUE;
}

static void composite_prepare(struct hwc_context_t *ctx,
                              struct hwc_display_t *dpy,
                              struct hwc_prepare_state *prepare)
{
    int transform = -1;
    int ii;

    NV_ASSERT(dpy->fb_index >= 0);
    NV_ASSERT(dpy->composite.list.nLayers == 0);

    /* Try to composite all layers marked as HWC_FRAMEBUFFER */
    for (ii = 0; ii < prepare->numLayers; ii++) {
        struct hwc_prepare_layer *ll = &prepare->layers[ii];

        if (ll->layer->compositionType == HWC_FRAMEBUFFER) {
            if (add_to_composite(dpy, ll)) {
                dpy->composite.list.nLayers = 0;
                NV_ASSERT(!dpy->fb_recycle);
                NV_ASSERT(!dpy->composite.scratch);
                return;
            }
        }
    }

    NV_ASSERT(dpy->composite.list.nLayers);

    if (!can_composite(&dpy->composite.list)) {
        /* Abort composition */
        dpy->composite.list.nLayers = 0;

        if (dpy->composite.scratch) {
            /* We should only be here if fb_recycle is set. */
            NV_ASSERT(dpy->fb_recycle);

            /* Switch to surfaceflinger composition and re-composite
             * the frame.
             */
            dpy->fb_recycle = 0;
            clear_layer_enable(dpy, prepare->display);

            /* Release the scratch buffer */
            dpy->scratch->unlock(dpy->scratch, dpy->composite.scratch);
            dpy->composite.scratch = NULL;
        }
        return;
    }

    if (dpy->composite.scratch) {
        NV_ASSERT(dpy->fb_recycle);
        return;
    }

    copy_rect(&dpy->device_clip, &dpy->composite.list.clip);
    dpy->composite.scratch = scratch_assign(dpy, 0, /* no transform */
                                            r_width(&dpy->device_clip),
                                            r_height(&dpy->device_clip),
                                            NvColorFormat_A8B8G8R8,
                                            NULL);
    if (dpy->composite.scratch) {
        /* Lock the scratch buffer so it will survive across geometry
         * changes.
         */
        dpy->scratch->lock(dpy->scratch, dpy->composite.scratch);

        /* Tell SurfaceFlinger that we take care of these layers */
        for (ii = 0; ii < dpy->composite.list.nLayers; ii++) {
            int listIndex = dpy->composite.map[ii];
            hwc_layer_t *cur = &prepare->display->hwLayers[listIndex];
            cur->compositionType = HWC_OVERLAY;
        }
    } else {
        /* Abort composition */
        dpy->composite.list.nLayers = 0;
    }
}

void hwc_scale_window(struct hwc_display_t *dpy, int index,
                      int transform, hwc_rect_t *src, hwc_rect_t *dst)
{
    NvGrOverlayDesc *o = &dpy->conf.overlay[index];
    struct hwc_window_mapping *map = &dpy->map[index];
    float dw;
    float dh;

    /* clip destination */
    o->dst.left = NV_MAX(0, dst->left);
    o->dst.top = NV_MAX(0, dst->top);
    o->dst.right = NV_MIN(dpy->device_clip.right, dst->right);
    o->dst.bottom = NV_MIN(dpy->device_clip.bottom, dst->bottom);

    /* clip source */
    dw = (float)r_width(src)  / (float)NV_MAX(1, r_width(dst));
    dh = (float)r_height(src) / (float)NV_MAX(1, r_height(dst));

    if (transform & HWC_TRANSFORM_FLIP_H) {
        o->src.left   = src->left   + 0.5f + dw * (dst->right  - o->dst.right);
        o->src.right  = src->right  + 0.5f + dw * (dst->left   - o->dst.left);
    } else {
        o->src.left   = src->left   + 0.5f + dw * (o->dst.left   - dst->left);
        o->src.right  = src->right  + 0.5f + dw * (o->dst.right  - dst->right);
    }
    /* Guard against zero-sized source */
    if (o->src.left == o->src.right) {
        if (o->src.right < src->right) {
            o->src.right++;
        } else {
            NV_ASSERT(o->src.left > src->left);
            o->src.left--;
        }
    }

    if (transform & HWC_TRANSFORM_FLIP_V) {
        o->src.top    = src->top    + 0.5f + dh * (dst->bottom - o->dst.bottom);
        o->src.bottom = src->bottom + 0.5f + dh * (dst->top    - o->dst.top);
    } else {
        o->src.top    = src->top    + 0.5f + dh * (o->dst.top    - dst->top);
        o->src.bottom = src->bottom + 0.5f + dh * (o->dst.bottom - dst->bottom);
    }
    /* Guard against zero-sized source */
    if (o->src.top == o->src.bottom) {
        if (o->src.bottom < src->bottom) {
            o->src.bottom++;
        } else {
            NV_ASSERT(o->src.top > src->top);
            o->src.top--;
        }
    }
}

static void window_attrs(struct hwc_display_t *dpy, int index,
                         struct hwc_prepare_layer *ll)
{
    hwc_layer_t *l = ll->layer;
    NvGrOverlayDesc *o = &dpy->conf.overlay[index];
    struct hwc_window_mapping *map = &dpy->map[index];

    o->blend = ll->blending;
    o->surf_index = map->scratch ? 0 : ll->surf_index;
    o->transform = fix_transform(ll->transform);

    hwc_scale_window(dpy, index, ll->transform, &ll->src, &ll->dst);
}

static int add_to_composite(struct hwc_display_t *dpy,
                            struct hwc_prepare_layer *ll)
{
    hwc_layer_t *cur = ll->layer;
    struct NvGrCompositeLayerRec *layer;

    if (dpy->composite.list.nLayers >= NVGR_COMPOSITE_LIST_MAX) {
        NV_ASSERT(dpy->composite.list.nLayers == NVGR_COMPOSITE_LIST_MAX);
        ALOGW("%s: exceeded layer count", __FUNCTION__);
        /* This is a performance bug - we will fall back to
         * SurfaceFlinger compositing when this happens.  Should there
         * be an assert to make sure we notice and consider increasing
         * the list size?
         */
        return -ENOMEM;
    }

    dpy->composite.map[dpy->composite.list.nLayers] = ll->index;
    layer = dpy->composite.list.layers + dpy->composite.list.nLayers++;

    // The buffer handle will be re-set on composite_set, but set it
    // here to allow validating composite with can_composite.
    layer->buffer = (NvNativeHandle*) cur->handle;
    switch (ll->blending) {
    case HWC_BLENDING_PREMULT:
        layer->blend = NVGR_COMPOSITE_BLEND_PREMULT;
        break;
    case HWC_BLENDING_COVERAGE:
        layer->blend = NVGR_COMPOSITE_BLEND_COVERAGE;
        break;
    default:
        NV_ASSERT(ll->blending == HWC_BLENDING_NONE);
        layer->blend = NVGR_COMPOSITE_BLEND_NONE;
        break;
    }

    layer->transform = ll->transform;
    copy_rect(&ll->src, &layer->src);
    copy_rect(&ll->dst, &layer->dst);

    return 0;
}

#if STATUSBAR_BLEND_OPT
static void subtract_statusbar(struct hwc_display_t* dpy, NvGrOverlayDesc* o,
                               hwc_rect_t *sb)
{
    enum Edge { left, top, right, bottom } edge;
    int delta;

    #define scale_w(dx, o) \
        ((float) dx * r_width(&o->src) / r_width(&o->dst))
    #define scale_h(dy, o) \
        ((float) dy * r_height(&o->src) / r_height(&o->dst))

    /* adjust dst rect */
    if (sb->bottom != dpy->device_clip.bottom) {
        NV_ASSERT(o->dst.top == sb->top);
        delta = r_height(sb);
        edge = top;
        o->dst.top = sb->bottom;
    } else if (sb->top != 0) {
        NV_ASSERT(o->dst.bottom == sb->bottom);
        delta = r_height(sb);
        edge = bottom;
        o->dst.bottom = sb->top;
    } else if (sb->right != dpy->device_clip.right) {
        NV_ASSERT(o->dst.left == sb->left);
        delta = r_width(sb);
        edge = left;
        o->dst.left = sb->right;
    } else {
        NV_ASSERT(sb->left != 0);
        NV_ASSERT(o->dst.right == sb->right);
        delta = r_width(sb);
        edge = right;
        o->dst.right = sb->left;
    }

    #undef scale_w
    #undef scale_h

    /* adjust src rect */
    if (o->transform & HWC_TRANSFORM_ROT_90) {
        edge = (edge + 3) % 4;
    }
    if (o->transform & HWC_TRANSFORM_FLIP_H) {
        if (edge == left || edge == right) {
            edge = (edge + 2) % 4;
        }
    }
    if (o->transform & HWC_TRANSFORM_FLIP_V) {
        if (edge == top || edge == bottom) {
            edge = (edge + 2) % 4;
        }
    }


    switch (edge) {
    case left:
        o->src.left += delta;
        break;
    case top:
        o->src.top += delta;
        break;
    case right:
        o->src.right -= delta;
        break;
    case bottom:
        o->src.bottom -= delta;
        break;
    default:
        NV_ASSERT(0);
        break;
    }
}

static void statusbar_prepare(struct hwc_context_t *ctx,
                              struct hwc_display_t *dpy,
                              struct hwc_prepare_state *prepare)
{
    struct hwc_prepare_layer *sb = prepare->statusbar;
    int ii, ret;

    if (!sb) {
        return;
    }

    NV_ASSERT(dpy->composite.list.nLayers == 0);
    NV_ASSERT(!dpy->composite.scratch);

    copy_rect(&sb->dst, &dpy->composite.list.clip);

    for (ii = dpy->caps.num_windows-1; ii >= 0; ii--) {
        if (ii != dpy->fb_index) {
            NvGrOverlayDesc *o = &dpy->conf.overlay[ii];
            int li = prepare->layer_map[ii].layer_index;
            struct hwc_prepare_layer *ll = &prepare->layers[li];

            NV_ASSERT(li != -1 && ll != prepare->statusbar);

            ret = add_to_composite(dpy, ll);
            NV_ASSERT(ret == 0);
            subtract_statusbar(dpy, o, &sb->dst);
            /* If the remaining dst rect has no area, disable the
             * window.
             */
            if (r_width(&o->dst) <= 0 || r_height(&o->dst) <= 0) {
                dpy->map[ii].index = -1;
                dpy->map[ii].scratch = NULL;
            }
        }
    }

    NV_ASSERT(dpy->fb_index != -1);
    ret = add_to_composite(dpy, sb);
    NV_ASSERT(ret == 0);

    dpy->composite.scratch = scratch_assign(dpy, 0,
                                            r_width(&sb->dst),
                                            r_height(&sb->dst),
                                            NvColorFormat_R5G6B5, NULL);

    if (dpy->composite.scratch) {
        /* Translate the composite rects to the origin of the scratch
         * buffer, which is smaller than the framebuffer.
         */
        for (ii = 0; ii < dpy->composite.list.nLayers; ii++) {
            nv_translate(&dpy->composite.list.layers[ii].dst,
                         -dpy->composite.list.clip.left,
                         -dpy->composite.list.clip.top);
        }
        nv_translate(&dpy->composite.list.clip,
                     -dpy->composite.list.clip.left,
                     -dpy->composite.list.clip.top);

        /* Set the src to the full size of the scratch buffer */
        dpy->conf.overlay[dpy->fb_index].src.left = 0;
        dpy->conf.overlay[dpy->fb_index].src.top = 0;
        dpy->conf.overlay[dpy->fb_index].src.right = r_width(&sb->dst);
        dpy->conf.overlay[dpy->fb_index].src.bottom = r_height(&sb->dst);

        /* Cancel the transform on this window since the scratch
         * buffer is already in device space.
         */
        dpy->conf.overlay[dpy->fb_index].transform = 0;
    } else {
        /* XXX fallback */
    }
}
#endif

static void conf_prepare(struct hwc_context_t *ctx,
                         struct hwc_display_t *dpy,
                         struct hwc_prepare_state *prepare)
{
    size_t ii;

    dpy->conf.protect = prepare->protect;
    if (dpy->type == HWC_DISPLAY_EXTERNAL) {
        dpy->conf.hotplug = ctx->hotplug.cached;
    }

    for (ii = 0; ii < dpy->caps.num_windows; ii++) {
        struct hwc_prepare_map *layer_map = &prepare->layer_map[ii];

        dpy->conf.overlay[ii].window_index = layer_map->window;

        if ((int)ii == dpy->fb_index) {
            NV_ASSERT(prepare->fb.layer.index != -1);
            NV_ASSERT(!ctx->limit_window_b ||
                      layer_map->window != NVFB_WINDOW_B_INDEX);
            prepare->fb.layer.src = prepare->fb.bounds;
            if (dpy->transform) {
                hwc_rotate_rect(dpy->transform,
                                &prepare->fb.layer.src,
                                &prepare->fb.layer.dst,
                                dpy->config.res_x, dpy->config.res_y);
            } else {
                prepare->fb.layer.dst = prepare->fb.layer.src;
            }
            prepare->fb.layer.blending = prepare->fb.blending;
            window_attrs(dpy, ii, &prepare->fb.layer);
        } else if (prepare->layer_map[ii].layer_index != -1) {
            int li = prepare->layer_map[ii].layer_index;
            struct hwc_prepare_layer *ll = &prepare->layers[li];

            NV_ASSERT(!ctx->limit_window_b ||
                      layer_map->window != NVFB_WINDOW_B_INDEX ||
                      NV_COLOR_GET_BPP(get_colorformat(ll->layer)) <= 16);
            window_attrs(dpy, ii, ll);
        }
    }

#if STATUSBAR_BLEND_OPT
    statusbar_prepare(ctx, dpy, prepare);
#endif
}

static void hwc_composite_set(struct hwc_display_t *dpy,
                              hwc_display_contents_t *display)
{
    int ii;

    NV_ASSERT(!dpy->fb_recycle);

    for (ii = 0; ii < dpy->composite.list.nLayers; ii++) {
        hwc_layer_t *layer = &display->hwLayers[dpy->composite.map[ii]];
        dpy->composite.list.layers[ii].buffer = (NvNativeHandle*) layer->handle;
    }
}

static inline void hwc_set_buffer(struct hwc_display_t *dpy, int ii,
                                  hwc_layer_t *layer, NvNativeHandle **outbuf)
{
    NvNativeHandle *buffer = (NvNativeHandle *) layer->handle;
    NvGrOverlayDesc *o = &dpy->conf.overlay[ii];

    if (dpy->map[ii].scratch) {
        dpy->scratch->blit(dpy->scratch, buffer, dpy->map[ii].surf_index,
                           dpy->map[ii].scratch, o->transform, &o->offset);
        buffer = dpy->scratch->get_buffer(dpy->scratch, dpy->map[ii].scratch);
    } else {
        o->offset.x = o->offset.y = 0;
    }

    *outbuf = buffer;
}

static NvBool hwc_remove_buffer(size_t count, NvNativeHandle **list,
                                NvNativeHandle *buffer)
{
    size_t ii;

    for (ii = 0; ii < count; ii++) {
        if (list[ii] == buffer) {
            list[ii] = NULL;
            return NV_TRUE;
        }
    }

    return NV_FALSE;
}

static inline void hwc_lock_buffer(struct hwc_context_t *ctx,
                                   struct nvfb_buffer *buf)
{
    buffer_handle_t handle = (buffer_handle_t) buf->buffer;
    int numfences = 1;

    ctx->gralloc->Base.lock(
        &ctx->gralloc->Base, handle, GRALLOC_USAGE_HW_FB, 0, 0,
        buf->buffer->Surf[0].Width, buf->buffer->Surf[0].Height, NULL);

    ctx->gralloc->getfences(&ctx->gralloc->Base, handle, &buf->sync.fence,
                            &numfences);
}

static inline void hwc_unlock_buffer(struct hwc_context_t *ctx,
                                     NvNativeHandle *buffer,
                                     struct nvfb_sync *sync)
{
    buffer_handle_t handle = (buffer_handle_t) buffer;

    if (sync->fence.SyncPointID != NVRM_INVALID_SYNCPOINT_ID) {
        ctx->gralloc->addfence(&ctx->gralloc->Base, handle, &sync->fence);
    }
#if NVGR_DEBUG_LOCKS
    ctx->gralloc->clear_lock_usage(handle, GRALLOC_USAGE_HW_FB);
#endif
    ctx->gralloc->Base.unlock(&ctx->gralloc->Base, handle);
}

int hwc_set_display(struct hwc_context_t *ctx, int disp,
                    hwc_display_contents_t* display)
{
    struct hwc_display_t *dpy = &ctx->displays[disp];
    NvNativeHandle *old_buffers[NVFB_MAX_WINDOWS];
    struct nvfb_buffer bufs[NVFB_MAX_WINDOWS];
    int status;
    size_t ii;

    NV_ASSERT(display);

    pthread_mutex_lock(&ctx->hotplug.mutex);

    if (dpy->blank) {
        pthread_mutex_unlock(&ctx->hotplug.mutex);
        return 0;
    }

    for (ii = 0; ii < dpy->caps.num_windows; ii++) {
        if (dpy->map[ii].index != -1) {
            hwc_layer_t *layer = &display->hwLayers[dpy->map[ii].index];
            NvNativeHandle *buffer = NULL;

            hwc_set_buffer(dpy, ii, layer, &buffer);
            bufs[ii].buffer = buffer;
        } else {
            bufs[ii].buffer = NULL;
        }
    }

    if (dpy->composite.scratch) {
        NV_ASSERT(dpy->composite.list.nLayers);
        NV_ASSERT(dpy->fb_index >= 0);

        if (!dpy->fb_recycle) {
            NV_ASSERT(dpy->fb_target >= 0);
            NV_ASSERT(dpy->map[dpy->fb_index].index == dpy->fb_target);

            hwc_composite_set(dpy, display);

            dpy->scratch->composite(dpy->scratch, &dpy->composite.list,
                                    dpy->composite.scratch);
        }

        bufs[dpy->fb_index].buffer =
            dpy->scratch->get_buffer(dpy->scratch, dpy->composite.scratch);
    }

    /* Save old buffer set */
    memcpy(old_buffers, dpy->buffers,
           dpy->caps.num_windows * sizeof(NvNativeHandle *));

    /* Lock new buffers */
    for (ii = 0; ii < dpy->caps.num_windows; ii++) {
        struct nvfb_buffer *buf = bufs + ii;
        if (bufs[ii].buffer) {
            bufs[ii].sync.fence.SyncPointID = NVRM_INVALID_SYNCPOINT_ID;
            if (!hwc_remove_buffer(dpy->caps.num_windows, old_buffers,
                                   bufs[ii].buffer)) {
                hwc_lock_buffer(ctx, buf);
            }
        }
    }

    status = nvfb_post(ctx->fb, disp, &dpy->conf, bufs, &dpy->last_flip);

    /* Unlock old buffers and save new buffer set */
    for (ii = 0; ii < dpy->caps.num_windows; ii++) {
        if (old_buffers[ii]) {
            hwc_unlock_buffer(ctx, old_buffers[ii], &dpy->last_flip);
        }

        dpy->buffers[ii] = bufs[ii].buffer;
    }

    pthread_mutex_unlock(&ctx->hotplug.mutex);

    return status;
}

static int hwc_set(struct hwc_composer_device_1 *dev,
                   size_t numDisplays, hwc_display_contents_t** displays)
{
    struct hwc_context_t *ctx = (struct hwc_context_t *)dev;
    size_t ii;
    int err = 0;

    for (ii = 0; ii < numDisplays; ii++) {
        if (displays[ii]) {
            int ret = hwc_set_display(ctx, ii, displays[ii]);
            if (ret) err = ret;
        }
    }

    return err;
}

static int hwc_set_ftrace(hwc_composer_device_t *dev,
                          size_t numDisplays,
                          hwc_display_contents_t** displays)
{
    int err;

    sysfs_write_string(PATH_FTRACE_MARKER_LOG, "start hwc_set frame\n");
    err = hwc_set(dev, numDisplays, displays);
    sysfs_write_string(PATH_FTRACE_MARKER_LOG, "end hwc_set frame\n");

    return err;
}

static int hwc_query(hwc_composer_device_t *dev, int what, int* value)
{
    struct hwc_context_t *ctx = (struct hwc_context_t *)dev;

    switch (what) {
    case HWC_BACKGROUND_LAYER_SUPPORTED:
        // XXX - Implement me eventually, Google not using it yet
        *value = 0;
        return 0;
    case HWC_DISPLAY_TYPES_SUPPORTED:
        return ctx->legacy ? HWC_DISPLAY_PRIMARY_BIT : ctx->display_mask;
    default:
        return -EINVAL;
    }
}

static int hwc_eventControl(hwc_composer_device_t *dev, int dpy,
                            int event, int enabled)
{
    struct hwc_context_t *ctx = (struct hwc_context_t *)dev;
    int status = -EINVAL;

    /* Only legal values for "enabled" are 0 and 1 */
    if (!(enabled & ~1)) {
        switch (event) {
        case HWC_EVENT_VSYNC:
            pthread_mutex_lock(&ctx->event.mutex);
            SYNCLOG("%s eventSync", enabled ? "Enabling" : "Disabling");
            if (enabled && !ctx->event.sync) {
                pthread_cond_signal(&ctx->event.condition);
            }
            ctx->event.sync = enabled;
            pthread_mutex_unlock(&ctx->event.mutex);
            status = 0;
            break;
        default:
            break;
        }
    }

    return status;
}

static void hotplug_display(struct hwc_context_t *ctx, int disp)
{
    struct hwc_display_t *dpy = &ctx->displays[disp];

    if (nvfb_config_get_current(ctx->fb, disp, &dpy->config)) {
        return;
    }

    nvfb_get_display_caps(ctx->fb, disp, &dpy->caps);

    /* device_clip is the real clip applied to window coords */
    dpy->device_clip.left = 0;
    dpy->device_clip.top = 0;
    dpy->device_clip.right = dpy->config.res_x;
    dpy->device_clip.bottom = dpy->config.res_y;

    /* If there is a display rotation, apply to the config */
    if (dpy->transform & HWC_TRANSFORM_ROT_90) {
        NVGR_SWAP(dpy->config.res_x, dpy->config.res_y);
    }

    /* layer_clip is the virtual clip applied to layer coords */
    dpy->layer_clip.left = 0;
    dpy->layer_clip.top = 0;
    dpy->layer_clip.right = dpy->config.res_x;
    dpy->layer_clip.bottom = dpy->config.res_y;

    ALOGD("Display %d is %d x %d", disp,
          dpy->config.res_x, dpy->config.res_y);
}

static int set_external_display_mode(struct hwc_context_t *ctx)
{
    struct hwc_display_t *primary = &ctx->displays[HWC_DISPLAY_PRIMARY];
    struct fb_var_screeninfo *mode;
    int xres, yres;

    if (ctx->panel_transform & HWC_TRANSFORM_ROT_90) {
        xres = primary->config.res_y;
        yres = primary->config.res_x;
    } else {
        xres = primary->config.res_x;
        yres = primary->config.res_y;
    }

    mode = nvfb_choose_display_mode(ctx->fb, HWC_DISPLAY_EXTERNAL,
                                    xres, yres, NvFbVideoPolicy_Closest);

    if (mode) {
        ctx->displays[HWC_DISPLAY_EXTERNAL].default_mode = mode;
        ctx->displays[HWC_DISPLAY_EXTERNAL].current_mode = mode;
        return nvfb_set_display_mode(ctx->fb, HWC_DISPLAY_EXTERNAL, mode);
    }

    return -1;
}

static void hwc_blank_display(struct hwc_context_t *ctx, int disp, int blank)
{
    struct hwc_display_t *dpy = &ctx->displays[disp];

    pthread_mutex_lock(&ctx->hotplug.mutex);

    ALOGD("%s: display %d: [%d -> %d]", __FUNCTION__, disp, dpy->blank, blank);
    dpy->blank = blank;

    if (blank) {
        size_t ii;

        /* Disable DIDIM to avoid artifacts on unblank */
        if (ctx->didim && disp == HWC_DISPLAY_PRIMARY) {
            ctx->didim->set_window(ctx->didim, NULL);
        }

        dpy->fb_recycle = 0;
        nvfb_post(ctx->fb, disp, NULL, NULL, &dpy->last_flip);

        for (ii = 0; ii < dpy->caps.num_windows; ii++) {
            if (dpy->buffers[ii]) {
                ALOGD("%s: releasing buffer 0x%x", __FUNCTION__,
                      dpy->buffers[ii]->MemId);
                hwc_unlock_buffer(ctx, dpy->buffers[ii], &dpy->last_flip);
                dpy->buffers[ii] = NULL;
            }
        }
    } else {
        if (disp == HWC_DISPLAY_EXTERNAL) {
            set_external_display_mode(ctx);
        }
        hotplug_display(ctx, disp);
    }

    pthread_mutex_unlock(&ctx->hotplug.mutex);
}

static int hwc_blank(struct hwc_composer_device_1* dev, int disp, int blank)
{
    struct hwc_context_t *ctx = (struct hwc_context_t *)dev;

    ALOGD("%s: display %d: %sblank", __FUNCTION__, disp, blank ? "" : "un");

    hwc_blank_display(ctx, disp, blank);

    return nvfb_blank(ctx->fb, disp, blank);
}

/*****************************************************************************/

static int hwc_invalidate(void *data)
{
    struct hwc_context_t *ctx = (struct hwc_context_t *) data;

    if (ctx->procs && ctx->procs->invalidate) {
        ctx->procs->invalidate(ctx->procs);
        return 0;
    } else {
        /* fail if the hwc invalidate callback is not registered */
        ALOGE("invalidate callback not registered");
        return -1;
    }
}

static int hwc_hotplug(void *data, int disp, struct nvfb_hotplug_status hotplug)
{
    struct hwc_context_t *ctx = (struct hwc_context_t *) data;
    struct hwc_display_t *dpy = &ctx->displays[disp];

    ctx->hotplug.latest = hotplug;

    ALOGD("%s: display %d %sconnected", __FUNCTION__, disp,
          ctx->hotplug.latest.connected ? "" : "dis");
    hwc_blank_display(ctx, disp, !ctx->hotplug.latest.connected);

    if (ctx->procs && ctx->procs->hotplug) {
        if (!ctx->legacy) {
            ctx->procs->hotplug(ctx->procs, disp,
                                ctx->hotplug.latest.connected);
        }
        return 0;
    } else {
        /* fail if the hwc hotplug callback is not registered */
        ALOGE("hotplug callback not registered");
        return -1;
    }
}

static int hwc_acquire(void *data, int disp)
{
    struct hwc_context_t *ctx = (struct hwc_context_t *) data;
    struct hwc_display_t *dpy = &ctx->displays[disp];

    ctx->hotplug.latest.connected = 1;
    ctx->hotplug.cached.value = -1;
    hwc_blank_display(ctx, disp, 0);

    /* Force a frame for the new display */
    hwc_invalidate(ctx);

    return 0;
}

static int hwc_release(void *data, int disp)
{
    struct hwc_context_t *ctx = (struct hwc_context_t *) data;
    struct hwc_display_t *dpy = &ctx->displays[disp];

    ctx->hotplug.latest.connected = 0;
    ctx->hotplug.cached.value = -1;
    hwc_blank_display(ctx, disp, 1);

    return 0;
}

static void hwc_registerProcs(hwc_composer_device_t *dev, hwc_procs_t const* p)
{
    struct hwc_context_t* ctx = (struct hwc_context_t *)dev;
    ctx->procs = (hwc_procs_t *)p;
}

static void idle_handler(union sigval si)
{
    struct hwc_context_t *ctx = (struct hwc_context_t *) si.sival_ptr;

    if (android_atomic_acquire_load(&ctx->idle.enabled)) {
        IDLELOG("timeout reached, generating idle frame");
        hwc_invalidate(ctx);
    } else {
        IDLELOG("disabled, ignoring timeout");
    }
}

static void create_idle_timer(struct hwc_context_t *ctx)
{
    struct sigevent ev;

    ev.sigev_notify = SIGEV_THREAD;
    ev.sigev_value.sival_ptr = ctx;
    ev.sigev_notify_function = idle_handler;
    ev.sigev_notify_attributes = NULL;
    if (timer_create(CLOCK_MONOTONIC, &ev, &ctx->idle.timer) < 0) {
        ctx->idle.timer = 0;
        ALOGE("Can't create idle timer: %s", strerror(errno));
    }
}

static void init_idle_machine(struct hwc_context_t *ctx)
{
    const char *env = getenv("NVHWC_IDLE_MINIMUM_FPS");

    ctx->idle.minimum_fps = get_property_int(NV_PROPERTY_IDLE_MINIMUM_FPS,
                                             NV_PROPERTY_IDLE_MINIMUM_FPS_DEFAULT);
    if (env) {
        int val = atoi(env);
        if (val >= 0) {
            ctx->idle.minimum_fps = val;
            IDLELOG("overriding minimum_fps, new value = %d", ctx->idle.minimum_fps);
        }
    }

    if (ctx->idle.minimum_fps) {
        struct timespec *time_limit = (struct timespec *) &ctx->idle.time_limit;
        time_t time_limit_ms = IDLE_FRAME_COUNT * 1000 / ctx->idle.minimum_fps;
        time_limit->tv_sec = time_limit_ms / 1000;
        time_limit->tv_nsec = (time_limit_ms * 1000000) % 1000000000;

        create_idle_timer(ctx);
    }

    android_atomic_release_store(0, &ctx->idle.enabled);
}

static void init_composite(struct hwc_context_t *dev)
{
    char value[PROPERTY_VALUE_MAX];

    property_get(NV_PROPERTY_COMPOSITOR, value, NV_PROPERTY_COMPOSITOR_DEFAULT);
    if (!strcmp(value, "surfaceflinger")) {
        dev->compositor = HWC_Compositor_SurfaceFlinger;
    } else if (!strcmp(value, "gralloc")) {
        dev->compositor = HWC_Compositor_Gralloc;
    } else {
        ALOGE("Unsupported %s: %s", NV_PROPERTY_COMPOSITOR, value);
    }

    property_get(NV_PROPERTY_COMPOSITE_POLICY, value, NV_PROPERTY_COMPOSITE_POLICY_DEFAULT);
    if (!strcmp(value, "auto")) {
        dev->composite_policy = HWC_CompositePolicy_Auto;
    } else if (!strcmp(value, "composite-always")) {
        dev->composite_policy = HWC_CompositePolicy_CompositeAlways;
    } else if (!strcmp(value, "assign-overlays")) {
        dev->composite_policy = HWC_CompositePolicy_AssignWindows;
    } else {
        ALOGE("Unsupported %s: %s", NV_PROPERTY_COMPOSITE_POLICY, value);
    }
}

static void *event_monitor(void *arg)
{
    const char *thread_name = "hwc_eventmon";
    struct hwc_context_t *ctx = (struct hwc_context_t *) arg;

    setpriority(PRIO_PROCESS, 0, -20);

    pthread_setname_np(ctx->event.thread, thread_name);

    while (1) {
        /* Sleep until needed */
        pthread_mutex_lock(&ctx->event.mutex);
        while (!ctx->event.sync && !ctx->event.shutdown) {
            pthread_cond_wait(&ctx->event.condition, &ctx->event.mutex);
        }
        pthread_mutex_unlock(&ctx->event.mutex);

        if (ctx->event.shutdown) {
            break;
        }

        /* Notify SurfaceFlinger */
        if (ctx->procs && ctx->procs->vsync) {
            struct timespec timestamp;

            nvfb_vblank_wait(ctx->fb);

            /* Per Mathias (4/19/2012), timestamp must use CLOCK_MONOTONIC */
            clock_gettime(CLOCK_MONOTONIC, &timestamp);

            /* Cannot hold a mutex during the callback, else a
             * deadlock will occur in SurfaceFlinger.  Instead use an
             * atomic load, which means there is a very small chance
             * SurfaceFlinger may disable the callback before it is
             * received.  Fortunately it can handle this.
             */
            if (android_atomic_acquire_load(&ctx->event.sync)) {
                int64_t timestamp_ns = timespec_to_ns(timestamp);
                ctx->procs->vsync(ctx->procs, 0, timestamp_ns);
                SYNCLOG("Signaled, timestamp is %lld", timestamp_ns);
            } else {
                SYNCLOG("eventSync disabled, not signaling");
            }
        }
    }

    return NULL;
}

static int open_event_monitor(struct hwc_context_t *ctx)
{
    int status;
    pthread_attr_t attr;
    struct sched_param param;

    /* Create event mutex and condition */
    pthread_mutex_init(&ctx->event.mutex, NULL);
    pthread_cond_init(&ctx->event.condition, NULL);

    /* Set thread priority */
    memset(&param, 0, sizeof(param));
    param.sched_priority = HAL_PRIORITY_URGENT_DISPLAY;
    pthread_attr_init(&attr);
    pthread_attr_setschedparam(&attr, &param);

    /* Initialize event state */
    android_atomic_release_store(0, &ctx->event.sync);

    /* Create event monitor thread */
    status = pthread_create(&ctx->event.thread, &attr, event_monitor, ctx);

    /* cleanup */
    pthread_attr_destroy(&attr);

    return -status;
}

static void close_event_monitor(struct hwc_context_t *ctx)
{
    if (ctx->event.thread) {
        /* Tell event thread to exit */
        pthread_mutex_lock(&ctx->event.mutex);
        ctx->event.sync = 0;
        ctx->event.shutdown = 1;
        pthread_cond_signal(&ctx->event.condition);
        pthread_mutex_unlock(&ctx->event.mutex);

        /* Wait for the thread to exit and cleanup */
        pthread_join(ctx->event.thread, NULL);
        pthread_mutex_destroy(&ctx->event.mutex);
        pthread_cond_destroy(&ctx->event.condition);
    }
}

static int
hwc_getDisplayConfigs(struct hwc_composer_device_1* dev, int disp,
                      uint32_t* configs, size_t* numConfigs)
{
    struct hwc_context_t *ctx = (struct hwc_context_t *) dev;
    size_t count = 0;

    if (ctx->legacy && disp != HWC_DISPLAY_PRIMARY) {
        return -1;
    }

    if (nvfb_config_get_count(ctx->fb, disp, &count)) {
        return -1;
    }

    if (count > *numConfigs) {
        count = *numConfigs;
    }

    *numConfigs = count;

    while (count--) {
        configs[count] = count;
    }

    return 0;
}

static int
hwc_getDisplayAttributes(struct hwc_composer_device_1* dev, int disp,
                         uint32_t index, const uint32_t* attributes,
                         int32_t* values)
{
    struct hwc_context_t *ctx = (struct hwc_context_t *) dev;
    struct nvfb_config config;

    if (ctx->legacy && disp != HWC_DISPLAY_PRIMARY) {
        return -1;
    }

    if (nvfb_config_get(ctx->fb, disp, index, &config)) {
        return -1;
    }

    if (ctx->displays[disp].transform & HWC_TRANSFORM_ROT_90) {
        NVGR_SWAP(config.res_x, config.res_y);
    }

    while (*attributes != HWC_DISPLAY_NO_ATTRIBUTE) {
        switch (*attributes) {
        case HWC_DISPLAY_VSYNC_PERIOD:
            SYNCLOG("Returning vsync period %d", config.period);
            *values = config.period;
            break;
        case HWC_DISPLAY_WIDTH:
            *values = config.res_x;
            ALOGD("config[%d].x = %d", disp, config.res_x);
            break;
        case HWC_DISPLAY_HEIGHT:
            *values = config.res_y;
            ALOGD("config[%d].y = %d", disp, config.res_y);
            break;
        case HWC_DISPLAY_DPI_X:
            *values = config.dpi_x;
            break;
        case HWC_DISPLAY_DPI_Y:
            *values = config.dpi_y;
            break;
        default:
            return -1;
        }
        attributes++;
        values++;
    }

    return 0;
}

/*********************************************************************
 *   Test interface
 */

static void hwc_post_wait(struct hwc_context_t *ctx, int disp)
{
    struct hwc_display_t *dpy = &ctx->displays[disp];

    if (dpy->last_flip.fence.SyncPointID != NVRM_INVALID_SYNCPOINT_ID) {
        NvRmFenceWait(ctx->gralloc->Rm, &dpy->last_flip.fence, NV_WAIT_INFINITE);
    }
}

static void hwc_get_buffers(struct hwc_context_t *ctx, int disp,
                            NvNativeHandle *buffers[NVFB_MAX_WINDOWS])
{
    struct hwc_display_t *dpy = &ctx->displays[disp];

    memcpy(buffers, dpy->buffers, sizeof(dpy->buffers));
}

/*********************************************************************
 *   Device initialization
 */

static int get_panel_rotation(void)
{
    int rotation = get_env_property_int(NV_PROPERTY_PANEL_ROTATION,
                                        NV_PROPERTY_PANEL_ROTATION_DEFAULT);

    switch (rotation) {
    default:
        ALOGW("Ignoring invalid panel rotation %d", rotation);
    case 0:
        return 0;
    case 90:
        return HWC_TRANSFORM_ROT_90;
    case 180:
        return HWC_TRANSFORM_ROT_180;
    case 270:
        return HWC_TRANSFORM_ROT_270;
    }
}

int hwc_set_display_mode(struct hwc_context_t *ctx, int disp,
                         struct fb_var_screeninfo *mode,
                         NvPoint *image_size, NvPoint *device_size)
{
    struct hwc_display_t *dpy = &ctx->displays[disp];

    if (!mode) {
        mode = dpy->default_mode;
    }

    if (mode != dpy->current_mode) {
        int err = nvfb_set_display_mode(ctx->fb, HWC_DISPLAY_EXTERNAL, mode);
        if (err) {
            return err;
        }

        /* Set new device clip */
        dpy->device_clip.left = 0;
        dpy->device_clip.top = 0;
        if (device_size) {
            dpy->device_clip.right = device_size->x;
            dpy->device_clip.bottom = device_size->y;
        } else {
            dpy->device_clip.right = mode->xres;
            dpy->device_clip.bottom = mode->yres;
        }

        /* Set new config and layer clip */
        if (image_size) {
            dpy->config.res_x = image_size->x;
            dpy->config.res_y = image_size->y;
        } else {
            dpy->config.res_x = mode->xres;
            dpy->config.res_y = mode->yres;
        }

        /* If there is a display rotation, apply to the config */
        if (dpy->transform & HWC_TRANSFORM_ROT_90) {
            NVGR_SWAP(dpy->config.res_x, dpy->config.res_y);
        }

        /* layer_clip is the virtual clip applied to layer coords */
        dpy->layer_clip.left = 0;
        dpy->layer_clip.top = 0;
        dpy->layer_clip.right = dpy->config.res_x;
        dpy->layer_clip.bottom = dpy->config.res_y;

        dpy->current_mode = mode;
    }

    return 0;
}

static int hwc_framebuffer_open(struct hwc_context_t *ctx)
{
    int ii, status;
    struct nvfb_callbacks callbacks = {
        .data = (void *) ctx,
        .hotplug = hwc_hotplug,
        .acquire = hwc_acquire,
        .release = hwc_release,
    };
    const struct nvfb_config *fbconfig =
        &ctx->displays[HWC_DISPLAY_PRIMARY].config;

    status = nvfb_open(&ctx->fb, &callbacks, &ctx->display_mask,
                       &ctx->hotplug.latest);
    if (status < 0) {
        return status;
    }

    ctx->displays[HWC_DISPLAY_PRIMARY].transform = get_panel_rotation();

    for (ii = 0; ii < HWC_NUM_DISPLAY_TYPES; ii++) {
        if (ctx->display_mask & (1 << ii)) {
            struct hwc_display_t *dpy = &ctx->displays[ii];

            dpy->type = ii;

            status = ctx->gralloc->scratch_open(
                ctx->gralloc, NVFB_MAX_WINDOWS, &dpy->scratch);
            if (status < 0) {
                return status;
            }
        }
    }

    /* Set initial mode and blank state.  SurfaceFlinger currently
     * assumes external connected displays are unblanked, whereas the
     * main display starts blanked.
     */
    hotplug_display(ctx, HWC_DISPLAY_PRIMARY);
    if (fbconfig->res_y > fbconfig->res_x) {
        ctx->panel_transform = HWC_TRANSFORM_ROT_270;
    }
    ctx->displays[HWC_DISPLAY_PRIMARY].blank = 1;

    if (ctx->hotplug.latest.connected) {
        set_external_display_mode(ctx);
        hotplug_display(ctx, HWC_DISPLAY_EXTERNAL);
    }

    /* A gentle reminder to hotplug all displays, in case more display
     * types are added later.
     */
    NV_ASSERT(2 == HWC_NUM_DISPLAY_TYPES);

    if (fbconfig->res_x > WINDOW_B_LIMIT_32BPP ||
        fbconfig->res_y > WINDOW_B_LIMIT_32BPP) {
        ctx->limit_window_b = NV_TRUE;
    }

    if (fbconfig->res_y > fbconfig->res_x) {
        ctx->panel_transform = HWC_TRANSFORM_ROT_270;
    }

    return 0;
}

static void hwc_framebuffer_close(struct hwc_context_t *ctx)
{
    size_t ii;

    for (ii = 0; ii < HWC_NUM_DISPLAY_TYPES; ii++) {
        struct hwc_display_t *dpy = &ctx->displays[ii];

        if (dpy->scratch) {
            ctx->gralloc->scratch_close(ctx->gralloc, dpy->scratch);
        }
    }

    nvfb_close(ctx->fb);
}

static int hwc_device_close(struct hw_device_t *dev)
{
    struct hwc_context_t* ctx = (struct hwc_context_t*)dev;

    if (ctx) {
        didim_close(ctx->didim);

        close_event_monitor(ctx);

        if (ctx->idle.timer) {
            timer_delete(ctx->idle.timer);
        }

#if NVDPS_ENABLE
        if (ctx->fd_nvdps >= 0)
            close(ctx->fd_nvdps);
#endif

        hwc_framebuffer_close(ctx);

        pthread_mutex_destroy(&ctx->hotplug.mutex);

        free(ctx);
    }

    return 0;
}

static int hwc_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device)
{
    struct hwc_context_t *dev = NULL;
    int status = -EINVAL;

    if (!strcmp(name, HWC_HARDWARE_COMPOSER)) {
        const struct hw_module_t *gralloc;

        status = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &gralloc);
        if (status < 0)
            return status;
        if (strcmp(gralloc->author, "NVIDIA") != 0)
            return -ENOTSUP;

        dev = (struct hwc_context_t*)malloc(sizeof(*dev));
        if (!dev)
            return -ENOMEM;

        /* initialize our state here */
        memset(dev, 0, sizeof(*dev));

        /* Choose HDMI behavior */
        dev->legacy = get_property_bool(NV_PROPERTY_HDMI_LEGACY,
                                        NV_PROPERTY_HDMI_LEGACY_DEFAULT);

        /* initialize the procs */
        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version = HWC_DEVICE_API_VERSION_1_1;
        dev->device.common.module = (struct hw_module_t*)module;
        dev->device.common.close = hwc_device_close;
        if (dev->legacy) {
            dev->device.prepare = hwc_legacy_prepare;
            dev->device.set = hwc_legacy_set;
        } else {
            dev->device.prepare = hwc_prepare;
            dev->device.set = hwc_set;
        }
        dev->device.eventControl = hwc_eventControl;
        dev->device.blank = hwc_blank;
        dev->device.dump = hwc_dump;
        dev->device.registerProcs = hwc_registerProcs;
        dev->device.query = hwc_query;
        dev->device.getDisplayConfigs = hwc_getDisplayConfigs;
        dev->device.getDisplayAttributes = hwc_getDisplayAttributes;
        dev->gralloc = (NvGrModule*)gralloc;

        /* Initialize didim */
        dev->didim = didim_open();

#if NVDPS_ENABLE
        dev->fd_nvdps = open(PATH_NVDPS, O_WRONLY);
        if (dev->fd_nvdps < 0)
            ALOGE("Failed to open NvDPS.\n");
#endif

        /* Open framebuffer device */
        status = hwc_framebuffer_open(dev);
        if (status < 0) {
            goto fail;
        }

        /* Mutex to protect display state during hotplug events */
        pthread_mutex_init(&dev->hotplug.mutex, NULL);

        init_idle_machine(dev);
        init_composite(dev);

        status = open_event_monitor(dev);
        if (status < 0) {
            goto fail;
        }

        /* Check whether we want to enable ftrace */
        if (get_property_bool("nvidia.hwc.ftrace_enable", 0)) {
            dev->device.set = hwc_set_ftrace;
        }

        /* Debug interface */
        dev->post_wait = hwc_post_wait;
        dev->get_buffers = hwc_get_buffers;

        *device = &dev->device.common;
        status = 0;
    }

    return status;

fail:
    if (dev) {
        hwc_device_close((struct hw_device_t *)dev);
    }
    return status;
}
