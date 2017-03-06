/*
 * Copyright (c) 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#include <linux/fb.h>
#include <linux/tegra_dc_ext.h>

#include <nvdc.h>

#include "nvdc_priv.h"

static void nvdcFbModeToMode(const struct fb_var_screeninfo *info,
                             struct nvdcMode *mode)
{
    mode->hActive = info->xres;
    mode->vActive = info->yres;

    mode->hFrontPorch = info->right_margin;
    mode->vFrontPorch = info->lower_margin;
    mode->hBackPorch = info->left_margin;
    mode->vBackPorch = info->upper_margin;
    mode->hSyncWidth = info->hsync_len;
    mode->vSyncWidth = info->vsync_len;

    /* XXX what to do with RefToSync? */

    mode->pclkKHz = info->pixclock ? PICOS2KHZ(info->pixclock) : 0;
    mode->bitsPerPixel = info->bits_per_pixel;
}

int nvdcGetMode(struct nvdcState *state, int head, struct nvdcMode *mode)
{
    struct fb_var_screeninfo info = { };

    if (!NVDC_VALID_HEAD(head)) {
        return EINVAL;
    }

    if (ioctl(state->fbFd[head], FBIOGET_VSCREENINFO, &info)) {
        return errno;
    }

    nvdcFbModeToMode(&info, mode);

    return 0;
}

static void nvdcModeToFbMode(const struct nvdcMode *mode,
                             struct fb_var_screeninfo *info)
{
    info->xres = mode->hActive;
    info->yres = mode->vActive;

    info->left_margin = mode->hBackPorch;
    info->upper_margin = mode->vBackPorch;
    info->right_margin = mode->hFrontPorch;
    info->lower_margin = mode->vFrontPorch;
    info->hsync_len = mode->hSyncWidth;
    info->vsync_len = mode->vSyncWidth;

    /* XXX what to do with RefToSync? */

    info->pixclock = mode->pclkKHz ? KHZ2PICOS(mode->pclkKHz) : 0;

    if (mode->bitsPerPixel != 0) {
        info->bits_per_pixel = mode->bitsPerPixel;
    }
}

int nvdcValidateMode(struct nvdcState *state, int head, struct nvdcMode *mode)
{
    struct fb_var_screeninfo info = { };

    if (!NVDC_VALID_HEAD(head)) {
        return EINVAL;
    }

    memset(&info, 0, sizeof(info));

    nvdcModeToFbMode(mode, &info);

    info.activate = FB_ACTIVATE_TEST;

    if (ioctl(state->fbFd[head], FBIOPUT_VSCREENINFO, &info)) {
        return errno;
    }

    nvdcFbModeToMode(&info, mode);

    return 0;
}

int nvdcSetMode(struct nvdcState *state, int head, struct nvdcMode *mode)
{
    struct fb_var_screeninfo info = { };
    struct fb_var_screeninfo current_info = { };

    if (!NVDC_VALID_HEAD(head)) {
        return EINVAL;
    }

#ifndef ANDROID
    /* XXX WAR kernel crashes for now */
    if (!head) return 0;
#endif

    memset(&info, 0, sizeof(info));
    memset(&current_info, 0, sizeof(current_info));

    if (ioctl(state->fbFd[head], FBIOGET_VSCREENINFO, &current_info)) {
        return errno;
    }
    info.bits_per_pixel = current_info.bits_per_pixel;

    nvdcModeToFbMode(mode, &info);

    info.activate = FB_ACTIVATE_NOW;

    if (ioctl(state->fbFd[head], FBIOPUT_VSCREENINFO, &info)) {
        return errno;
    }

    /* blank */
    if (ioctl(state->fbFd[head], FBIOBLANK, FB_BLANK_POWERDOWN)) {
        return errno;
    }

    /* Unblank */
    if (ioctl(state->fbFd[head], FBIOBLANK, FB_BLANK_UNBLANK)) {
        return errno;
    }

    return 0;
}

int nvdcDPMS(struct nvdcState *state, int head, enum nvdcDPMSmode mode)
{
    long fbMode;

    if (!NVDC_VALID_HEAD(head)) {
        return EINVAL;
    }

    switch (mode) {
    case NVDC_DPMS_NORMAL:
        fbMode = FB_BLANK_UNBLANK;
        break;
    case NVDC_DPMS_VSYNC_SUSPEND:
        fbMode = FB_BLANK_VSYNC_SUSPEND;
        break;
    case NVDC_DPMS_HSYNC_SUSPEND:
        fbMode = FB_BLANK_HSYNC_SUSPEND;
        break;
    case NVDC_DPMS_POWERDOWN:
        fbMode = FB_BLANK_POWERDOWN;
        break;
    default:
        return EINVAL;
    }

    if (ioctl(state->fbFd[head], FBIOBLANK, fbMode)) {
        return errno;
    }

    return 0;
}

int nvdcQueryHeadStatus(struct nvdcState *state, int head,
                        struct nvdcHeadStatus *nvdcStatus)
{
    struct tegra_dc_ext_status status = { };

    if (!NVDC_VALID_HEAD(head)) {
        return EINVAL;
    }

    if (ioctl(state->tegraExtFd[head], TEGRA_DC_EXT_GET_STATUS, &status)) {
        return errno;
    }

    nvdcStatus->enabled = status.flags & TEGRA_DC_EXT_FLAGS_ENABLED;

    return 0;
}
