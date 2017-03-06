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

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <cutils/properties.h>

#include "nvhwc.h"
#include "nvassert.h"
#include "nvhwc_debug.h"


/* The property controls whether only video layers
 * (GRALLOC_USAGE_EXTERNAL_DISP | NVGR_USAGE_CLOSED_CAP) are shown on
 * hdmi during video playback or camera preview.  Value can be one of
 * "On", "Off", "1", "0".
 */
#define NV_PROPERTY_HDMI_VIDEOVIEW              SYS_PROP(hdmi.videoview)
#define NV_PROPERTY_HDMI_VIDEOVIEW_DEFAULT      1

#ifndef HDMI_VIDEOVIEW
#define HDMI_VIDEOVIEW  NV_PROPERTY_HDMI_VIDEOVIEW_DEFAULT
#endif

/* The property controls whether so called cimena mode is enabled.
 * Value can be one of "On", "Off", "1", "0".
 * Default is 0, unless overriden by BoardConfig.mk and Android.mk
 */
#define NV_PROPERTY_CINEMA_MODE                 SYS_PROP(cinemamode)

#ifndef CINEMA_MODE
#define NV_PROPERTY_CINEMA_MODE_DEFAULT         0
#define CINEMA_MODE                             NV_PROPERTY_CINEMA_MODE_DEFAULT
#else
#define NV_PROPERTY_CINEMA_MODE_DEFAULT         CINEMA_MODE
#endif

/* hdmi modes */
enum {
    HDMI_MODE_BLANK,
    HDMI_MODE_MIRROR,
    HDMI_MODE_VIDEO,
    HDMI_MODE_STEREO,
};

#ifndef HDMI_MIRROR_MODE
#define HDMI_MIRROR_MODE Crop
#endif

static int property_bool_changed(char *key, char default_value, NvBool *value)
{
    NvBool prev_value = *value;

    *value = get_property_bool(key, default_value);

    return *value != prev_value;
}

static HWC_MirrorMode get_mirror_mode(void)
{
    char value[PROPERTY_VALUE_MAX];
    HWC_MirrorMode default_mode;

#define TOKEN(x) HWC_MirrorMode_ ## x
#define MIRROR_MODE(x) TOKEN(x)

    default_mode = MIRROR_MODE(HDMI_MIRROR_MODE);

#undef MIRROR_MODE
#undef TOKEN

    property_get("nvidia.hwc.mirror_mode", value, "");
    if (!strcmp(value, "crop"))
        return HWC_MirrorMode_Crop;
    if (!strcmp(value, "scale"))
        return HWC_MirrorMode_Scale;
    if (!strcmp(value, "center"))
        return HWC_MirrorMode_Center;

    return default_mode;
}

/* Given the surfaceflinger rotation, return the transform which is
 * applied to HDMI layers.  Note that rotation direction is reversed
 * because we want to undo the surfaceflinger rotation.
 */
static inline int transform_from_rotation(int rotation)
{
    switch (rotation) {
    case 0:
        return 0;
    case 90:
        return HWC_TRANSFORM_ROT_270;
    case 180:
        return HWC_TRANSFORM_ROT_180;
    case 270:
        return HWC_TRANSFORM_ROT_90;
    }

    NV_ASSERT(!"Unexpected rotation");
    return 0;
}

static inline int hdmi_transform(struct hwc_context_t *ctx,
                                 hwc_layer_t* cur)
{
    return combine_transform(ctx->panel_transform, cur->transform);
}

static inline int supported_stereo_mode(hwc_layer_t* cur)
{
    NvNativeHandle *h = (NvNativeHandle *)cur->handle;

    NV_ASSERT(h && h->Buf);
    if (h->Buf->StereoInfo & NV_STEREO_ENABLE_MASK) {
        int fpa = NV_STEREO_GET_FPA(h->Buf->StereoInfo);
        int ci = NV_STEREO_GET_CI(h->Buf->StereoInfo);

        if (NV_STEREO_IS_SUPPORTED_MODE(fpa, ci)) {
            return 1;
        }
        ALOGE("Unsupported Stereo format fpa:%x ci:%x", fpa, ci);
    }

    return 0;
}

static inline void get_supported_stereo_size(hwc_layer_t* cur,
                                             unsigned int *x,
                                             unsigned int *y)
{
    NvNativeHandle *h = (NvNativeHandle *)cur->handle;

    NV_ASSERT(h && h->Buf);
    NV_ASSERT(h->Buf->StereoInfo & NV_STEREO_ENABLE_MASK);

    *x = *y = 0;
    NvStereoType type = (NvStereoType)(h->Buf->StereoInfo & NV_STEREO_MASK);
    // Currently, we support 1080p stereo only for packed LR/TB stereo
    // whose source is originally 1920x1080
    switch (type) {
        case NV_STEREO_LEFTRIGHT:
        case NV_STEREO_RIGHTLEFT:
            if ((r_width(&cur->sourceCrop) << 1) == 1920) {
                *x = 1920;
                *y = 1080;
            }
            break;
        case NV_STEREO_TOPBOTTOM:
        case NV_STEREO_BOTTOMTOP:
            if ((r_height(&cur->sourceCrop) << 1) == 1080) {
                *x = 1920;
                *y = 1080;
            }
            break;
        case NV_STEREO_SEPARATELR:
        case NV_STEREO_SEPARATERL:
        default:
            break;
    }
    if (!*x) {
        *x = 1280;
        *y = 720;
    }
}

static int get_hdmi_mode(struct hwc_context_t *ctx,
                         hwc_display_contents_t *list,
                         int *ignore_mask, int *use_windows)
{
    struct hwc_display_t *hdmi = &ctx->displays[HWC_DISPLAY_EXTERNAL];
    struct fb_var_screeninfo *disp_mode = NULL;
    int composite = HWC_FORCE_COMPOSITING_ON_HDMI;
    enum NvFbVideoPolicy policy;
    int hdmi_mode;
    unsigned int xres, yres;

    /* Bug 978861 - under rare conditions hwc_prepare will be called
     * with a valid list containing zero layers.  Disable all windows
     * on the HDMI display and clear the display mode to avoid a
     * modeset in gralloc.
     */
    if (!list->numHwLayers) {
        hdmi->conf.mode = NULL;
        return HDMI_MODE_BLANK;
    }

    if (!ctx->hotplug.cached.demo) {
        xres = yres = 0;
        if (ctx->video_out_index >= 0 && ctx->hdmi_video_mode) {
            hwc_layer_t* cur = &list->hwLayers[ctx->video_out_index];

            if (ctx->hotplug.cached.stereo && supported_stereo_mode(cur)) {
                hdmi_mode = HDMI_MODE_STEREO;
                get_supported_stereo_size(cur, &xres, &yres);
                policy = NvFbVideoPolicy_Stereo_Closest;
            } else {
#if ALLOW_VIDEO_SCALING
                policy = NvFbVideoPolicy_RoundUp;
#else
                policy = NvFbVideoPolicy_Exact;
#endif
                hdmi_mode = HDMI_MODE_VIDEO;
            }
            ctx->cc_out_index = hwc_find_layer(list, NVGR_USAGE_CLOSED_CAP,
                                               NV_TRUE);
            ctx->osd_out_index = hwc_find_layer(list, NVGR_USAGE_ONSCREEN_DISP,
                                                NV_TRUE);
        } else {
            /* Look for a secondary display surface */
            ctx->video_out_index = hwc_find_layer(list, NVGR_USAGE_SECONDARY_DISP,
                                                  NV_TRUE);

            // NvFbVideoPolicy_Exact is used for now to catch all
            // potential bugs client app might have. When client app
            // becomes stable, we'll change that to Closest
            policy = NvFbVideoPolicy_Exact;
            hdmi_mode = HDMI_MODE_VIDEO;
        }

        if (ctx->video_out_index >= 0 && ctx->hdmi_video_mode) {
            hwc_layer_t* l = &list->hwLayers[ctx->video_out_index];
            int transform = hdmi_transform(ctx, l);

            if (!xres) {
                if (transform & HWC_TRANSFORM_ROT_90) {
                    xres = r_height(&l->sourceCrop);
                    yres = r_width(&l->sourceCrop);
                } else {
                    xres = r_width(&l->sourceCrop);
                    yres = r_height(&l->sourceCrop);
                }
            }

            disp_mode = nvfb_choose_display_mode(ctx->fb, HWC_DISPLAY_EXTERNAL,
                                                 xres, yres, policy);

            if (ctx->cinema_mode && ctx->hdmi_video_mode &&
                disp_mode && !(hwc_get_usage(l) & NVGR_USAGE_SECONDARY_DISP)) {
                /* If we're in cinema mode, suppress the video, CC and OSD
                 * layers from internal display
                 */
                if (hwc_layer_is_yuv(l)) {
                    *ignore_mask |= GRALLOC_USAGE_EXTERNAL_DISP;
                }
                if (ctx->cc_out_index >= 0) {
                    *ignore_mask |= NVGR_USAGE_CLOSED_CAP;
                }
                if (ctx->osd_out_index >= 0) {
                    *ignore_mask |= NVGR_USAGE_ONSCREEN_DISP;
                }
            }
        }
    }

    if (!disp_mode) {
        struct hwc_display_t *primary = &ctx->displays[HWC_DISPLAY_PRIMARY];
        hdmi_mode = HDMI_MODE_MIRROR;

        ctx->mirror.mode = get_mirror_mode();

        if (ctx->hotplug.cached.demo ||
            ctx->mirror.mode == HWC_MirrorMode_Center) {
            policy = NvFbVideoPolicy_RoundUp;
        } else {
            policy = NvFbVideoPolicy_Closest;
        }

        if (ctx->panel_transform & HWC_TRANSFORM_ROT_90) {
            xres = primary->config.res_y;
            yres = primary->config.res_x;
            /* Force composite so we only rotate once.  Its possible
             * to optimize this case and rotate per-layer, which may
             * be a win depending on how many layers change per frame,
             * and how often.
             */
            composite = 1;
        } else {
            xres = primary->config.res_x;
            yres = primary->config.res_y;
        }

        disp_mode = nvfb_choose_display_mode(ctx->fb, HWC_DISPLAY_EXTERNAL,
                                             xres, yres, policy);
    }

    NV_ASSERT(disp_mode);
    hdmi->conf.mode = disp_mode;

    if (composite) {
        *use_windows = 0;
    }

    return hdmi_mode;
}

static void xform_get_center_scale(hwc_xform *xf,
                                   int src_w, int src_h,
                                   int dst_w, int dst_h,
                                   float par)
{
    float src_aspect = (float)src_w / src_h;
    float dst_aspect = (float)dst_w * par / dst_h;   /* dst is wider by par */

    if (src_aspect > dst_aspect) {
        xf->x_scale = (float) dst_w / src_w;
        xf->y_scale = xf->x_scale * par;
        xf->y_off = (dst_h - src_h * xf->y_scale) / 2;
        xf->x_off = 0;
    } else {
        xf->y_scale = (float) dst_h / src_h;
        xf->x_scale = xf->y_scale / par;
        xf->x_off = (dst_w - src_w * xf->x_scale) / 2;
        xf->y_off = 0;
    }
}

static void xform_apply(hwc_xform *xf, NvRect *rect)
{
    rect->left = rect->left * xf->x_scale + xf->x_off + 0.5;
    rect->right = rect->right * xf->x_scale + xf->x_off + 0.5;
    rect->top = rect->top * xf->y_scale + xf->y_off + 0.5;
    rect->bottom = rect->bottom * xf->y_scale + xf->y_off + 0.5;
}

static inline void scale_rect(NvRect *src, hwc_xform *xf)
{
    #define APPLY_XSCALE(xf, v) {               \
        v = v * xf->x_scale + 0.5;              \
    }
    #define APPLY_YSCALE(xf, v) {               \
        v = v * xf->y_scale + 0.5;              \
    }

    APPLY_YSCALE(xf, src->top);
    APPLY_XSCALE(xf, src->left);
    APPLY_XSCALE(xf, src->right);
    APPLY_YSCALE(xf, src->bottom);

    #undef APPLY_XSCALE
    #undef APPLY_YSCALE

}

static void scale_rect_to_display(int displayWidth, int displayHeight,
                                  NvRect *dst, struct fb_var_screeninfo *mode,
                                  float par)
{
    hwc_xform xf;

    dst->left = 0;
    dst->top = 0;
    dst->right = displayWidth;
    dst->bottom = displayHeight;

    xform_get_center_scale(&xf, dst->right, dst->bottom,
                           mode->xres, mode->yres,
                           par);
    xform_apply(&xf, dst);
}

/* Move the caption layer to the bottom center of the video rect */
static void scale_caption_to_video(const NvRect *cc_src,
                                   const NvRect *vid_dst,
                                   NvRect *cc_dst,
                                   float par)
{
    hwc_xform xf;

    cc_dst->left = 0;
    cc_dst->top = 0;
    cc_dst->right = r_width(cc_src);
    cc_dst->bottom = r_height(cc_src);

    xform_get_center_scale(&xf,
                           r_width(cc_src), r_height(cc_src),
                           r_width(vid_dst), r_height(vid_dst),
                           par);

    /* Center the caption within the vid rect */
    xf.x_off += vid_dst->left;
    xf.y_off += vid_dst->top;

    xform_apply(&xf, cc_dst);

    /* Translate the caption to the bottom center of vid rect */
    cc_dst->top += vid_dst->bottom - cc_dst->bottom;
    cc_dst->bottom = vid_dst->bottom;
}

/* Move the caption layer to the bottom center of the display rect */
static void scale_caption_to_display(const NvRect *cc_src,
                                     NvRect *cc_dst,
                                     struct fb_var_screeninfo *mode,
                                     float par)
{
    scale_rect_to_display(r_width(cc_src), r_height(cc_src), cc_dst, mode, par);

    /* Translate the caption to the bottom center of vid rect */
    cc_dst->top += mode->yres - cc_dst->bottom;
    cc_dst->bottom = mode->yres;
}

/* Move the OSD layer to the top center of the display */
static void scale_osd_to_display(const NvRect *osd_src,
                                 NvRect *osd_dst,
                                 struct fb_var_screeninfo *mode,
                                 float par)
{
    scale_rect_to_display(r_width(osd_src), r_height(osd_src), osd_dst, mode, par);

    /* Translate the OSD to the top center of the display */
    osd_dst->bottom -= osd_dst->top;
    osd_dst->top = 0;
}

static void disable_overlay(struct hwc_display_t *dpy,
                            unsigned *overlay_mask,
                            int ii)
{
    dpy->conf.overlay[ii].window_index =
        hwc_get_window(&dpy->caps, overlay_mask, 0, NULL);
    dpy->map[ii].index = -1;
    dpy->map[ii].scratch = NULL;
}

static inline NvBool
update_framebuffer_transform(struct hwc_context_t* ctx,
                             int ii, hwc_xform *xf, NvColorFormat format)
{
    struct hwc_display_t *dpy = &ctx->displays[HWC_DISPLAY_EXTERNAL];
    struct nvfb_window *o = dpy->conf.overlay + ii;
    int w = r_width(&o->src);
    int h = r_height(&o->src);
    NvBool need_scratch = NV_FALSE;
    NvRect src_crop = o->src;

    NV_ASSERT(!o->transform);
    NV_ASSERT(!dpy->map[ii].scratch);

    o->transform = ctx->panel_transform;

    if (o->transform & HWC_TRANSFORM_ROT_90) {
        NVGR_SWAP(o->src.left, o->src.top);
        NVGR_SWAP(o->src.right, o->src.bottom);
        NVGR_SWAP(o->dst.left, o->dst.top);
        NVGR_SWAP(o->dst.right, o->dst.bottom);
        need_scratch = NV_TRUE;
    }

#if !FB_SCALE_IN_DC
    if (xf) {
        float xscale = xf->x_scale;
        float yscale = xf->y_scale;

        /* swap the scale factors since this is the pre-rotated size and
         * scaling is post-rotation
         */
        if (o->transform & HWC_TRANSFORM_ROT_90) {
            NVGR_SWAPF(xscale, yscale);
        }

        w = w * xscale + 0.5;
        h = h * yscale + 0.5;

        /* Translate the new sourceCrop to the origin of the scratch buffer */
        o->src.right -= o->src.left; o->src.left = 0;
        o->src.bottom -= o->src.top; o->src.top = 0;
        scale_rect(&o->src, xf);

        need_scratch = NV_TRUE;
    }
#endif

    if (need_scratch) {
        dpy->map[ii].scratch = scratch_assign(dpy, o->transform, w, h, format,
                                              &src_crop);

        if (!dpy->map[ii].scratch) {
            return NV_FALSE;
        }
    }

    return NV_TRUE;
}

static inline NvBool
update_layer_transform(struct hwc_context_t* ctx, hwc_layer_t *l,
                       int ii, hwc_xform *xf)
{
    struct hwc_display_t *dpy = &ctx->displays[HWC_DISPLAY_EXTERNAL];
    struct nvfb_window *o = &dpy->conf.overlay[ii];
    NvBool need_scale = NV_FALSE;
    NvBool need_swapxy = NV_FALSE;
    float scale_w = 1.0f, scale_h = 1.0f;
    hwc_rect_t displayFrame;

    /* Compute scale factors */
#if !FB_SCALE_IN_DC
    hwc_get_scale(l->transform, &l->sourceCrop, &l->displayFrame,
                  &scale_w, &scale_h);
    if (xf) {
        scale_w *= xf->x_scale;
        scale_h *= xf->y_scale;
    }
    if (hwc_need_scale(scale_w, scale_h)) {
        need_scale = NV_TRUE;
    }
#endif

    o->transform = hdmi_transform(ctx, l);
    if (o->transform & HWC_TRANSFORM_ROT_90) {
        need_swapxy = NV_TRUE;
    }

    /* Adjust displayFrame if the panel is rotated */
    if (ctx->panel_transform & HWC_TRANSFORM_ROT_90) {
        int fb_width = ctx->displays[HWC_DISPLAY_PRIMARY].config.res_x;

        displayFrame.left = l->displayFrame.top;
        displayFrame.top = fb_width - l->displayFrame.right;
        displayFrame.bottom = fb_width - l->displayFrame.left;
        displayFrame.right = l->displayFrame.bottom;
    } else {
        displayFrame = l->displayFrame;
    }

    if (need_scale || need_swapxy) {
        NvRmSurface *surf = get_nvrmsurface(l);
        int transform = o->transform & HWC_TRANSFORM_ROT_90;
        int w, h;
        NvRect crop;

        if (need_scale) {
            w = scale_w * r_width(&l->sourceCrop);
            h = scale_h * r_height(&l->sourceCrop);
            copy_rect(&l->sourceCrop, &crop);
        } else {
            w = surf->Width;
            h = surf->Height;
        }

        dpy->map[ii].scratch = scratch_assign(dpy, transform, w, h,
                                              surf->ColorFormat,
                                              need_scale ? &crop : NULL);

        /* Scratch blit handles clockwise rotation */
        o->transform = fix_transform(o->transform) & ~HWC_TRANSFORM_ROT_90;

        if (dpy->map[ii].scratch) {
            hwc_rect_t sourceCrop;
            if (need_scale) {
                sourceCrop.left = 0;
                sourceCrop.top = 0;
                sourceCrop.right = w;
                sourceCrop.bottom = h;
            } else {
                copy_rect(&l->sourceCrop, &sourceCrop);
            }

            /* When a surface is rotated clockwise, the left and right
             * extents of the post-rotation source crop must be offset
             * relative to the pre-rotated surface height.
            */
            if (need_swapxy) {
                int l = h - sourceCrop.bottom;
                int t = sourceCrop.left;
                int r = h - sourceCrop.top;
                int b = sourceCrop.right;

                sourceCrop.left = l;
                sourceCrop.top = t;
                sourceCrop.right = r;
                sourceCrop.bottom = b;
            }
            hwc_scale_window(dpy, ii, l->transform, &sourceCrop, &displayFrame);
        } else {
            return NV_FALSE;
        }

    } else {
        hwc_scale_window(dpy, ii, l->transform, &l->sourceCrop, &displayFrame);
        dpy->map[ii].scratch = NULL;
    }

#if 0
    /* reverse panel transform applied to dst rectangle
     * note: flip_v and flip_h are commutative, rot_90 is not and it must
     * precede flip transforms.*/

    if (ctx->panel_transform & HWC_TRANSFORM_ROT_90) {
        NVGR_SWAP(fb_width, fb_height);
    }
    if (ctx->panel_transform & HWC_TRANSFORM_FLIP_H) {
        NvS32 l = fb_width - o->dst.right;
        NvS32 r = fb_width - o->dst.left;
        o->dst.left = l;
        o->dst.right = r;
    }
    if (ctx->panel_transform & HWC_TRANSFORM_FLIP_V) {
        NvS32 t = fb_height - o->dst.bottom;
        NvS32 b = fb_height - o->dst.top;
        o->dst.top = t;
        o->dst.bottom = b;
    }
#endif

    return NV_TRUE;
}

static NvBool crop_overlay(struct hwc_context_t* ctx, NvGrOverlayDesc *o,
                           int xres, int yres)
{
    if (ctx->panel_transform & HWC_TRANSFORM_ROT_90) {
        NVGR_SWAP(xres, yres);
    }

    if (o->dst.left >= xres || o->dst.top >= yres) {
        return NV_FALSE;
    }

    /* Clip right */
    if (o->dst.right > xres) {
        float scale = (float) (xres - o->dst.left) /
            (float) (o->dst.right - o->dst.left);
        o->src.right = o->src.left + (o->src.right - o->src.left) * scale;
        o->dst.right = xres;
    }

    /* Clip bottom */
    if (o->dst.bottom > yres) {
        float scale = (float) (yres - o->dst.top) /
            (float) (o->dst.bottom - o->dst.top);
        o->src.bottom = o->src.top + (o->src.bottom - o->src.top) * scale;
        o->dst.bottom = yres;
    }

    return NV_TRUE;
}

static void configure_mirror(struct hwc_context_t *ctx,
                             struct hwc_display_t *dpy,
                             hwc_display_contents_t *list)
{
    const struct hwc_display_t *primary = &ctx->displays[HWC_DISPLAY_PRIMARY];
    const struct nvfb_config *fbconfig = &primary->config;
    NvGrOverlayConfig *conf = &dpy->conf;
    unsigned int fb_width = fbconfig->res_x;
    unsigned int fb_height = fbconfig->res_y;
    NvColorFormat fb_format;
    size_t ii;

    memcpy(&dpy->map, &primary->map, sizeof(dpy->map));
    memcpy(&dpy->conf.overlay, &primary->conf.overlay, sizeof(dpy->conf.overlay));
    dpy->conf.protect = primary->conf.protect;

    dpy->fb_index = primary->fb_index;
    dpy->fb_layers = primary->fb_layers;
    dpy->fb_target = primary->fb_target;

    if (dpy->fb_target >= 0 && list->hwLayers[dpy->fb_target].handle) {
        fb_format = get_colorformat(&list->hwLayers[dpy->fb_target]);
    } else {
        fb_format = NvColorFormat_A8B8G8R8;
    }

    /* If the native panel orientation does not match home, adjust the
     * transforms of the overlays for display on an external landscape
     * device.
     */
    if (ctx->panel_transform & HWC_TRANSFORM_ROT_90) {
        NVGR_SWAP(fb_width, fb_height);
    }

    if (ctx->hotplug.cached.demo) {
        /* Rotation? */
        return;
    }

    switch (ctx->mirror.mode) {
    case HWC_MirrorMode_Crop:
        for (ii = 0; ii < dpy->caps.num_windows; ii++) {
            NvGrOverlayDesc *o = &conf->overlay[ii];

            if ((int)ii == dpy->fb_index) {
                if (!crop_overlay(ctx, o, conf->mode->xres, conf->mode->yres) ||
                    !update_framebuffer_transform(ctx, ii, NULL, fb_format)) {
                    dpy->map[ii].scratch = NULL;
                }
            } else if (dpy->map[ii].index != -1) {
                /* No rotation support for individual layers */
                NV_ASSERT(!(ctx->panel_transform & HWC_TRANSFORM_ROT_90));

                if (!crop_overlay(ctx, o, conf->mode->xres, conf->mode->yres)) {
                    dpy->map[ii].index = -1;
                    dpy->map[ii].scratch = NULL;
                }
            }
        }
        break;
    case HWC_MirrorMode_Center:
        if (conf->mode->xres >= fb_width && conf->mode->yres >= fb_height) {
            hwc_xform xf;

            xf.x_scale = xf.y_scale = 1.0;
            xf.x_off = (conf->mode->xres - fb_width) / 2;
            xf.y_off = (conf->mode->yres - fb_height) / 2;

            for (ii = 0; ii < dpy->caps.num_windows; ii++) {
                NvGrOverlayDesc *o = &conf->overlay[ii];

                if ((int)ii == dpy->fb_index) {
                    if (update_framebuffer_transform(ctx, ii, NULL, fb_format)) {
                        xform_apply(&xf, &o->dst);
                    } else {
                        dpy->map[ii].scratch = NULL;
                    }
                } else if (dpy->map[ii].index != -1) {
                    /* No rotation support for individual layers */
                    NV_ASSERT(!(ctx->panel_transform & HWC_TRANSFORM_ROT_90));
                    xform_apply(&xf, &o->dst);
                }
            }
            break;
        }
        /* else fall through to scale */
    case HWC_MirrorMode_Scale:
        if ((ctx->panel_transform & HWC_TRANSFORM_ROT_90) ||
            conf->mode->xres != fb_width || conf->mode->yres != fb_height) {
            hwc_xform xf;
            float mode_par = 1.0;

            if (conf->mode->xres == 720 && conf->mode->yres == 480)
                mode_par = 480.0 / 720 * 16 / 9;
            if (conf->mode->yres == 576)
                mode_par = 576.0 / 720 * 16 / 9;

            xform_get_center_scale(&xf, fb_width, fb_height,
                                   conf->mode->xres, conf->mode->yres, mode_par);

            for (ii = 0; ii < dpy->caps.num_windows; ii++) {
                NvGrOverlayDesc *o = &conf->overlay[ii];

                if ((int)ii == dpy->fb_index) {
                    if (update_framebuffer_transform(ctx, ii, &xf, fb_format)) {
                        xform_apply(&xf, &o->dst);
                    } else {
                        dpy->map[ii].scratch = NULL;
                    }
                } else if (dpy->map[ii].index != -1) {
                    hwc_layer_t *l = &list->hwLayers[dpy->map[ii].index];
                    /* No rotation support for individual layers */
                    NV_ASSERT(!(ctx->panel_transform & HWC_TRANSFORM_ROT_90));
                    if (update_layer_transform(ctx, l, ii, &xf)) {
                        xform_apply(&xf, &o->dst);
                    } else {
                        dpy->map[ii].index = -1;
                        dpy->map[ii].scratch = NULL;
                    }
                }
            }
        }
        break;
    }
}

static inline int r_include(const hwc_rect_t* r1,
                            const hwc_rect_t* r2)
{
    return (r2->left >= r1->left && r2->right <= r1->right &&
            r2->top >= r1->top && r2->bottom <= r1->bottom);
}

static void configure_video(struct hwc_context_t *ctx,
                            struct hwc_display_t *dpy,
                            hwc_display_contents_t *list)
{
    NvGrOverlayConfig *conf = &dpy->conf;
    hwc_layer_t* l;
    unsigned overlay_mask = (1 << dpy->caps.num_windows) - 1;
    int req;
    float video_par = 1.0;   /* pixel aspect ratio of the video (pixel_w/pixel_h)*/
    float mode_par = 1.0;    /* pixel aspect ratio of given mode */
    int displayWidth;
    int displayHeight;

    NV_ASSERT(ctx->video_out_index >= 0);

    conf->protect = 0;

#define OVL_VID 2
#define OVL_CC  1
#define OVL_OSD 0

    /* Video layer */
    l = &list->hwLayers[ctx->video_out_index];
    conf->protect |= hwc_layer_is_protected(l);

    req = NVFB_WINDOW_CAP_YUV_FORMATS | NVFB_WINDOW_CAP_SCALE;
    conf->overlay[OVL_VID].window_index =
        hwc_get_window(&dpy->caps, &overlay_mask, req, NULL);
    conf->overlay[OVL_VID].blend = HWC_BLENDING_NONE;
    conf->overlay[OVL_VID].surf_index = 0;
    conf->overlay[OVL_VID].transform = hdmi_transform(ctx, l);
    if (conf->overlay[OVL_VID].transform & HWC_TRANSFORM_ROT_90) {
        NvRmSurface *surf = get_nvrmsurface(l);

        conf->overlay[OVL_VID].src.left = surf->Height - l->sourceCrop.bottom;
        conf->overlay[OVL_VID].src.right = surf->Height - l->sourceCrop.top;
        conf->overlay[OVL_VID].src.top = l->sourceCrop.left;
        conf->overlay[OVL_VID].src.bottom = l->sourceCrop.right;
        conf->overlay[OVL_VID].transform &= ~HWC_TRANSFORM_ROT_90;
        dpy->map[OVL_VID].scratch = scratch_assign(dpy, HWC_TRANSFORM_ROT_90,
                                                   surf->Width, surf->Height,
                                                   surf->ColorFormat, NULL);
        /* If scratch assignment fails, fall back to mirror mode */
        if (!dpy->map[OVL_VID].scratch) {
            configure_mirror(ctx, dpy, list);
            return;
        }
    } else {
        copy_rect(&l->sourceCrop, &conf->overlay[OVL_VID].src);
        dpy->map[OVL_VID].scratch = NULL;
    }

    dpy->map[OVL_VID].index = ctx->video_out_index;


    /* Anamorphic 16:9 format for 720x480 (480pH mode in CAE standard) and
     * 720x576 (576pH) resolutions. Available only for video playback.
     * Two assumptions are made:
     *   1) If TV/monitor provides 720p mode, its native size is 16:9, thus 4:3
     *      DAR will not be selected by dc driver for 720x576 (576p) resulution;
     *   2) TV/monitor will stretch width so that resulting DAR is 16:9; this
     *      behavior may need to be forced from within TV/monitor's own menu.
     *      The DAR has to be inferred from mode dimensions because original
     *      informatiopn from EDID is not preserved through fb_var_screeninfo.
     */
    if (conf->mode->xres == 720 && conf->mode->yres == 480)
        mode_par = 480.0 / 720 * 16 / 9;
    if (conf->mode->yres == 576)
        mode_par = 576.0 / 720 * 16 / 9;

    /* Original pixel aspect ratio of the video has to be used for scaling, it
     * can be back computed from sourceCrop and displayFrame rectangles.
     * displayFrame is rotated according device orientation.
     */
    if (ctx->panel_transform & HWC_TRANSFORM_ROT_90) {
        displayWidth = r_height(&l->displayFrame);
        displayHeight = r_width(&l->displayFrame);
    } else {
        displayWidth = r_width(&l->displayFrame);
        displayHeight = r_height(&l->displayFrame);
    }
    video_par = ((float)displayWidth  * r_height(&l->sourceCrop)) /
                ((float)displayHeight * r_width(&l->sourceCrop));

    /* If the video pixel aspect ratio is almost 1:1, it probably
     * really is 1:1 but we lost some precision along the way.
     */
    #define PAR_EPSILON 0.004f
    if ((1.0f - PAR_EPSILON) <= video_par &&
        (1.0f + PAR_EPSILON) >= video_par) {
        video_par = 1.0f;
    }

    scale_rect_to_display(r_width(&l->sourceCrop),
                          r_height(&l->sourceCrop),
                          &conf->overlay[OVL_VID].dst,
                          conf->mode, mode_par / video_par);

    /* Closed Caption layer */
    if (ctx->cc_out_index >= 0) {
        l = &list->hwLayers[ctx->cc_out_index];
        conf->protect |= hwc_layer_is_protected(l);
        req = 0;
        conf->overlay[OVL_CC].window_index =
            hwc_get_window(&dpy->caps, &overlay_mask, req, NULL);
        conf->overlay[OVL_CC].blend = HWC_BLENDING_COVERAGE;
        conf->overlay[OVL_CC].surf_index = 0;
        conf->overlay[OVL_CC].transform = 0;
        copy_rect(&l->sourceCrop, &conf->overlay[OVL_CC].src);

        if (r_include(&list->hwLayers[ctx->video_out_index].displayFrame,
            &l->displayFrame)) {
            scale_caption_to_video(&conf->overlay[OVL_CC].src,
                                   &conf->overlay[OVL_VID].dst,
                                   &conf->overlay[OVL_CC].dst,
                                   mode_par);
        } else {
            scale_caption_to_display(&conf->overlay[OVL_CC].src,
                                     &conf->overlay[OVL_CC].dst,
                                     conf->mode,
                                     mode_par);
        }

        dpy->map[OVL_CC].index = ctx->cc_out_index;
        dpy->map[OVL_CC].scratch = NULL;
    } else {
        disable_overlay(dpy, &overlay_mask, OVL_CC);
    }

    /* On-screen Display layer */
    if (ctx->osd_out_index >= 0) {
        l = &list->hwLayers[ctx->osd_out_index];
        conf->protect |= hwc_layer_is_protected(l);
        req = 0;
        conf->overlay[OVL_OSD].window_index =
            hwc_get_window(&dpy->caps, &overlay_mask, req, NULL);
        conf->overlay[OVL_OSD].blend = HWC_BLENDING_COVERAGE;
        conf->overlay[OVL_OSD].surf_index = 0;
        conf->overlay[OVL_OSD].transform = 0;
        copy_rect(&l->sourceCrop, &conf->overlay[OVL_OSD].src);

        scale_osd_to_display(&conf->overlay[OVL_OSD].src,
                             &conf->overlay[OVL_OSD].dst,
                             conf->mode,
                             mode_par);

        dpy->map[OVL_OSD].index = ctx->osd_out_index;
        dpy->map[OVL_OSD].scratch = NULL;
    } else {
        disable_overlay(dpy, &overlay_mask, OVL_OSD);
    }

#undef OVL_VID
#undef OVL_CC
#undef OVL_OSD
}

// Values from HDMI1.4a spec
#define STEREO_720p_GAP 30
#define STEREO_1080p_GAP 45

static void configure_stereo(struct hwc_context_t *ctx,
                             struct hwc_display_t *dpy,
                             hwc_display_contents_t *display)
{
    NvGrOverlayConfig *conf = &dpy->conf;
    hwc_layer_t* l = &display->hwLayers[ctx->video_out_index];
    unsigned overlay_mask = (1 << dpy->caps.num_windows) - 1;
    NvNativeHandle *h = (NvNativeHandle *)l->handle;
    NvStereoType type = (NvStereoType)(h->Buf->StereoInfo & NV_STEREO_MASK);
    int leftfirst = NV_STEREO_GET_CI(h->Buf->StereoInfo) == NV_STEREO_CI_LEFTFIRST;
    int nxtidx = (h->Type == NV_NATIVE_BUFFER_STEREO) ? 1 : 0;
    int req = NVFB_WINDOW_CAP_YUV_FORMATS;
    NvRect src;
    int displayWidth;
    int displayHeight;

    NV_ASSERT(ctx->video_out_index >= 0);

    conf->protect = hwc_layer_is_protected(l);

    /* use one overlay for left and one for right image of frame
     * packing format */

#define OVL_L      2
#define OVL_R      1
#define OVL_UNUSED 0

    /* Left */
    conf->overlay[OVL_L].window_index =
        hwc_get_window(&dpy->caps, &overlay_mask, req, NULL);
    conf->overlay[OVL_L].blend = HWC_BLENDING_NONE;
    conf->overlay[OVL_L].surf_index = leftfirst ? 0 : nxtidx;
    conf->overlay[OVL_L].transform = 0;

    /* Left eye is the sourceCrop so handle this like a normal video
     * layer
     */
    copy_rect(&l->sourceCrop, &conf->overlay[OVL_L].src);
    if (ctx->panel_transform & HWC_TRANSFORM_ROT_90) {
        displayWidth = r_height(&l->displayFrame);
        displayHeight = r_width(&l->displayFrame);
    } else {
        displayWidth = r_width(&l->displayFrame);
        displayHeight = r_height(&l->displayFrame);
    }
    scale_rect_to_display(displayWidth,
                          displayHeight,
                          &conf->overlay[OVL_L].dst,
                          conf->mode, 1.0);

    dpy->map[OVL_L].index = ctx->video_out_index;
    dpy->map[OVL_L].scratch = NULL;

    /* Right */
    conf->overlay[OVL_R].window_index =
        hwc_get_window(&dpy->caps, &overlay_mask, req, NULL);
    conf->overlay[OVL_R].blend = HWC_BLENDING_NONE;
    conf->overlay[OVL_R].surf_index = leftfirst ? nxtidx : 0;
    conf->overlay[OVL_R].transform = 0;

    /* Right eye is a simple transform from the left. */
    copy_rect(&conf->overlay[OVL_L].src, &conf->overlay[OVL_R].src);
    switch (type) {
    case NV_STEREO_LEFTRIGHT:
        conf->overlay[OVL_R].src.left += r_width(&l->sourceCrop);
        conf->overlay[OVL_R].src.right += r_width(&l->sourceCrop);
        break;
    case NV_STEREO_RIGHTLEFT:
        conf->overlay[OVL_R].src.left -= r_width(&l->sourceCrop);
        conf->overlay[OVL_R].src.right -= r_width(&l->sourceCrop);
        break;
    case NV_STEREO_TOPBOTTOM:
        conf->overlay[OVL_R].src.top += r_height(&l->sourceCrop);
        conf->overlay[OVL_R].src.bottom += r_height(&l->sourceCrop);
        break;
    case NV_STEREO_BOTTOMTOP:
        conf->overlay[OVL_R].src.top -= r_height(&l->sourceCrop);
        conf->overlay[OVL_R].src.bottom -= r_height(&l->sourceCrop);
        break;
    case NV_STEREO_SEPARATELR:
    case NV_STEREO_SEPARATERL:
        break;
    default:
        ALOGE("Unsupported stereo type 0x%x", (int)type);
        break;
    }

    copy_rect(&conf->overlay[OVL_L].dst, &conf->overlay[OVL_R].dst);
    if (conf->mode->vmode & FB_VMODE_STEREO_FRAME_PACK) {
        if (conf->mode->xres == 1920) {
            conf->overlay[OVL_R].dst.top    += conf->mode->yres + STEREO_1080p_GAP;
            conf->overlay[OVL_R].dst.bottom += conf->mode->yres + STEREO_1080p_GAP;
        } else {
            conf->overlay[OVL_R].dst.top    += conf->mode->yres + STEREO_720p_GAP;
            conf->overlay[OVL_R].dst.bottom += conf->mode->yres + STEREO_720p_GAP;
        }
    } else if (conf->mode->vmode & FB_VMODE_STEREO_LEFT_RIGHT) {
        /* Change only the horizontal cooridinates */
        int dst_left = conf->overlay[OVL_L].dst.left / 2;
        int dst_width = (conf->overlay[OVL_L].dst.right - conf->overlay[OVL_L].dst.left) / 2;
        int xres_center = conf->mode->xres / 2;
        int dst_l = OVL_L;
        int dst_r = OVL_R;
        switch (type) {
        /* No switch thing to do for joint modes or separate LR */
        case NV_STEREO_TOPBOTTOM:
        case NV_STEREO_LEFTRIGHT:
        case NV_STEREO_BOTTOMTOP:
        case NV_STEREO_RIGHTLEFT:
        case NV_STEREO_SEPARATELR:
            break;
        /* Only this mode requires switch between views */
        case NV_STEREO_SEPARATERL:
            dst_l = OVL_R; dst_r = OVL_L;
            break;
        default:
            ALOGE("Unsupported stereo type 0x%08x", (int)type);
            dst_l = dst_r = OVL_UNUSED;
            break;
        }
        if (OVL_UNUSED != dst_l && OVL_UNUSED != dst_r) {
            conf->overlay[dst_l].dst.left  = dst_left;
            conf->overlay[dst_l].dst.right = dst_left + dst_width;
            conf->overlay[dst_r].dst.left  = xres_center + dst_left;
            conf->overlay[dst_r].dst.right = xres_center + dst_left + dst_width;
            /* Top and bottom need no update as transform is only horizontal */
        }
    } else if (conf->mode->vmode & FB_VMODE_STEREO_TOP_BOTTOM) {
        /* Change only the vertical cooridinates */
        int dst_top = conf->overlay[OVL_L].dst.top / 2;
        int dst_height = (conf->overlay[OVL_L].dst.bottom - conf->overlay[OVL_L].dst.top) / 2;
        int yres_center = conf->mode->yres / 2;
        int dst_l = OVL_L;
        int dst_r = OVL_R;
        switch (type) {
        /* No switch thing to do for joint modes or separate LR */
        case NV_STEREO_TOPBOTTOM:
        case NV_STEREO_LEFTRIGHT:
        case NV_STEREO_BOTTOMTOP:
        case NV_STEREO_RIGHTLEFT:
        case NV_STEREO_SEPARATELR:
            break;
        /* Only this mode requires switch between views */
        case NV_STEREO_SEPARATERL:
            dst_l = OVL_R; dst_r = OVL_L;
            break;
        default:
            ALOGE("Unsupported stereo type 0x%08x", (int)type);
            dst_l = dst_r = OVL_UNUSED;
            break;
        }
        if (OVL_UNUSED != dst_l && OVL_UNUSED != dst_r) {
            conf->overlay[dst_l].dst.top  = dst_top;
            conf->overlay[dst_l].dst.bottom = dst_top + dst_height;
            conf->overlay[dst_r].dst.top  = yres_center + dst_top;
            conf->overlay[dst_r].dst.bottom = yres_center + dst_top + dst_height;
            /* Left and right need no update as transform is only vertical */
        }
    } else {
        ALOGE("Unsupported HDMI output stereo type...");
    }

    dpy->map[OVL_R].index = ctx->video_out_index;
    dpy->map[OVL_R].scratch = NULL;

    /* Unused */
    disable_overlay(dpy, &overlay_mask, OVL_UNUSED);

#undef OVL_L
#undef OVL_R
#undef OVL_UNUSED
}

// Write a flag that indicates whether we have stereo content
static void set_stereo_gl_app_running(NvBool bStereoGLAppRunning)
{
    // No action to be taken on the return code, so don't check if this succeeded. But for
    // reference, property_set is supposed to return 0 on success, < 0 on failure.
    property_set("persist.sys.NV_STEREOAPP", (bStereoGLAppRunning == NV_TRUE) ? "1" : "0");
}

static void configure_blank(struct hwc_context_t* ctx,
                            struct hwc_display_t *dpy)
{
    size_t ii;
    unsigned overlay_mask = (1 << dpy->caps.num_windows) - 1;

    dpy->conf.mode = NULL;

    for (ii = 0; ii < dpy->caps.num_windows; ii++) {
        disable_overlay(dpy, &overlay_mask, ii);
    }
}

static void hdmi_prepare(struct hwc_context_t *ctx,
                         hwc_display_contents_t *display)
{
    struct hwc_display_t *dpy = &ctx->displays[HWC_DISPLAY_EXTERNAL];
    NvBool bStereoGLAppRunning = NV_FALSE;

    dpy->fb_index = -1;
    dpy->conf.hotplug = ctx->hotplug.cached;

    /* Update the current config to match the selected mode */
    if (dpy->conf.mode) {
        dpy->config.res_x = dpy->conf.mode->xres;
        dpy->config.res_y = dpy->conf.mode->yres;
    }

    scratch_frame_start(dpy);

    switch (ctx->hdmi_mode) {
    case HDMI_MODE_BLANK:
    default:
        configure_blank(ctx, dpy);
        break;
    case HDMI_MODE_MIRROR:
        configure_mirror(ctx, dpy, display);
        break;
    case HDMI_MODE_VIDEO:
        configure_video(ctx, dpy, display);
        break;
    case HDMI_MODE_STEREO:
        configure_stereo(ctx, dpy, display);
        // Stereo video playback does not require the flag to be set. Check to
        // make sure a game is running
        hwc_layer_t* cur = &display->hwLayers[ctx->video_out_index];
        if (hwc_get_usage(cur) & NVGR_USAGE_STEREO) {
            bStereoGLAppRunning = NV_TRUE;
        }
        break;
    }

    scratch_frame_end(dpy);

    set_stereo_gl_app_running(bStereoGLAppRunning);
}

static int hwc_prepare_display_legacy(struct hwc_context_t *ctx,
                                      struct hwc_display_t *dpy,
                                      hwc_display_contents_t *list)
{
    struct hwc_prepare_state prepare;
    int ignore_mask = NVGR_USAGE_SECONDARY_DISP;
    int use_windows = 1;
    int changed;
    size_t ii;
    hwc_rect_t *didim_window = NULL;

#if DUMP_LAYERLIST
    hwc_dump_display_contents(list);
#endif

    if (list->flags & HWC_GEOMETRY_CHANGED) {
        changed = 1;
    } else {
        changed = 0;
    }

    changed |= update_idle_state(ctx);

    if (ctx->hotplug.cached.value != ctx->hotplug.latest.value) {
        ctx->hotplug.cached = ctx->hotplug.latest;
        changed = 1;
    }

    if (ctx->hotplug.cached.connected && (changed || ctx->video_out_index >= 0)) {
        changed |= property_bool_changed(NV_PROPERTY_HDMI_VIDEOVIEW,
                                         NV_PROPERTY_HDMI_VIDEOVIEW_DEFAULT,
                                         &ctx->hdmi_video_mode);
        changed |= property_bool_changed(NV_PROPERTY_CINEMA_MODE,
                                         NV_PROPERTY_CINEMA_MODE_DEFAULT,
                                         &ctx->cinema_mode);
    }

    dpy->composite.list.geometry_changed = changed;

    if (!changed) {
        if (ctx->composite_policy & HWC_CompositePolicy_FbCache) {
            fb_cache_check(dpy, list);
        }
        return 0;
    }

    ctx->video_out_index = hwc_find_layer(list,
                                          GRALLOC_USAGE_EXTERNAL_DISP |
                                          NVGR_USAGE_STEREO,
                                          NV_TRUE);

    if (ctx->video_out_index >= 0) {
        hwc_layer_t *l = &list->hwLayers[ctx->video_out_index];
        int video_size = r_width(&l->displayFrame) * r_height(&l->displayFrame);
        const hwc_region_t *r = &l->visibleRegionScreen;

        /* WAR bug #1023763 - if a video layer is found but mostly
         * occluded, ignore it.
         */
        if (r->numRects > 1 ||
            memcmp(&r->rects[0], &l->displayFrame, sizeof(hwc_rect_t))) {
            /* Video is partially occluded; calculate visible area. */
            int visible = 0;

            for (ii = 0; ii < r->numRects; ii++) {
                visible += r_width(&r->rects[ii]) * r_height(&r->rects[ii]);
            }

            if (visible < (video_size >> 1)) {
                ctx->video_out_index = -1;
            }
        }

        if (ctx->didim && ctx->video_out_index >= 0 && hwc_layer_is_yuv(l)) {
            int screen_size = dpy->config.res_x * dpy->config.res_y;

            if (video_size > (screen_size >> 1)) {
                didim_window = &l->displayFrame;
            }
        }
    }

    /* Update didim for normal mode or video mode */
    if (ctx->didim) {
        ctx->didim->set_window(ctx->didim, didim_window);
    }

    //ctx->panel_transform = transform_from_rotation(ctx->hotplug.cached.rotation);
    if (ctx->hotplug.cached.connected) {
        ctx->hdmi_mode = get_hdmi_mode(ctx, list, &ignore_mask, &use_windows);
    }

    hwc_prepare_begin(ctx, dpy, &prepare, list, use_windows, ignore_mask);

    hwc_assign_windows(ctx, dpy, &prepare);

    /* Prepare HDMI state */
    if (ctx->hotplug.cached.connected) {
        hdmi_prepare(ctx, list);
    }

    hwc_prepare_end(ctx, dpy, &prepare, list);

    if (dpy->windows_overlap) {
        enable_idle_detection(&ctx->idle);
    } else {
        disable_idle_detection(&ctx->idle);
    }

    return 0;
}

int hwc_legacy_prepare(hwc_composer_device_t *dev, size_t numDisplays,
                       hwc_display_contents_t **displays)
{
    struct hwc_context_t *ctx = (struct hwc_context_t *)dev;
    int err = 0;

    if (displays[HWC_DISPLAY_PRIMARY]) {
        err = hwc_prepare_display_legacy(ctx,
                                         &ctx->displays[HWC_DISPLAY_PRIMARY],
                                         displays[HWC_DISPLAY_PRIMARY]);
    }

#if DUMP_CONFIG
    char temp[2000];
    hwc_dump(dev, temp, NV_ARRAY_SIZE(temp));
    ALOGD("%s\n", temp);
#endif

    return err;
}

int hwc_legacy_set(struct hwc_composer_device_1 *dev,
                   size_t numDisplays, hwc_display_contents_t** displays)
{
    struct hwc_context_t *ctx = (struct hwc_context_t *)dev;
    int err = 0;

    if (displays[HWC_DISPLAY_PRIMARY]) {
        err = hwc_set_display(ctx, HWC_DISPLAY_PRIMARY,
                              displays[HWC_DISPLAY_PRIMARY]);
        if (!err && ctx->hotplug.cached.connected) {
            err = hwc_set_display(ctx, HWC_DISPLAY_EXTERNAL,
                                  displays[HWC_DISPLAY_PRIMARY]);
        }
    }

    return err;
}
