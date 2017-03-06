/**
 * Copyright (c) 2011 - 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef __NVC_IMAGE_H__
#define __NVC_IMAGE_H__

#include <linux/ioctl.h>

#define NVC_IMAGER_PARAM_BAYER          0xE100
#define NVC_IMAGER_PARAM_FRAME_LEN      0xE101
#define NVC_IMAGER_PARAM_COARSE_TIME    0xE102
#define NVC_IMAGER_PARAM_NVC            0xE103

#define NVC_IMAGER_TEST_NONE            0
#define NVC_IMAGER_TEST_COLORBARS       1
#define NVC_IMAGER_TEST_CHECKERBOARD    2
#define NVC_IMAGER_TEST_WALKING1S       3

#define NVC_IMAGER_TYPE_HUH             0
#define NVC_IMAGER_TYPE_RAW             1
#define NVC_IMAGER_TYPE_SOC             2

#define NVC_IMAGER_INTERFACE_MIPI_A     3
#define NVC_IMAGER_INTERFACE_MIPI_B     4
#define NVC_IMAGER_INTERFACE_MIPI_AB    5

#define NVC_IMAGER_IDENTIFIER_MAX       32
#define NVC_IMAGER_FORMAT_MAX           4
#define NVC_IMAGER_CLOCK_PROFILE_MAX    2
#define NVC_IMAGER_VERSION              (2)
#define NVC_IMAGER_CAPABILITIES_END     ((0x3434 << 16) | NVC_IMAGER_VERSION)

#define NVC_IMAGER_INT2FLOAT_DIVISOR    1000


struct nvc_imager_nvc {
    __u32 coarse_time;
    __u32 max_coarse_diff;
    __u32 min_exposure_course;
    __u32 max_exposure_course;
    __u32 diff_integration_time;
    __u32 line_length;
    __u32 frame_length;
    __u32 min_frame_length;
    __u32 max_frame_length;
    __u32 inherent_gain;
    __u32 min_gain;
    __u32 max_gain;
    __u8 support_fast_mode;
    __u32 pll_mult;
    __u32 pll_prediv;
    __u32 pll_posdiv;
    __u32 pll_vtpixdiv;
    __u32 pll_vtsysdiv;
} __attribute__((packed));

struct nvc_imager_bayer {
    __s32 res_x;
    __s32 res_y;
    __u32 frame_length;
    __u32 coarse_time;
    __u16 gain;
} __attribute__((packed));

struct nvc_imager_mode {
    __s32 res_x;
    __s32 res_y;
    __s32 active_start_x;
    __s32 active_stary_y;
    __u32 peak_frame_rate;
    __u32 pixel_aspect_ratio;
    __u32 pll_multiplier;
} __attribute__((packed));

struct nvc_imager_nvc_rd {
    __s32 res_x;
    __s32 res_y;
    struct nvc_imager_mode *p_mode;
    struct nvc_imager_nvc *p_nvc;
} __attribute__((packed));

struct nvc_imager_mode_list {
    struct nvc_imager_mode *p_modes;
    __s32 *p_num_mode;
} __attribute__((packed));

struct nvc_imager_region {
    __s32 region_start_x;
    __s32 region_start_y;
    __u32 x_scale;
    __u32 y_scale;
} __attribute__((packed));

struct nvc_imager_frame_rates {
    __s32 width;
    __s32 height;
    __u32 frame_length;
    __u32 line_length;
    __u32 max_frame_length;
} __attribute__((packed));

struct nvc_imager_inherent_gain {
    __s32 width;
    __s32 height;
    __u32 inherent_gain;
} __attribute__((packed));

struct nvc_clock_profile {
    __u32 external_clock_khz;
    __u32 clock_multiplier;
} __attribute__((packed));

struct nvc_imager_cap {
    char identifier[NVC_IMAGER_IDENTIFIER_MAX];
    __u32 sensor_nvc_interface;
    __u32 pixel_types[NVC_IMAGER_FORMAT_MAX];
    __u32 orientation;
    __u32 direction;
    __u32 initial_clock_rate_khz;
    struct nvc_clock_profile clock_profiles[NVC_IMAGER_CLOCK_PROFILE_MAX];
    __u32 h_sync_edge;
    __u32 v_sync_edge;
    __u32 mclk_on_vgp0;
    __u8 csi_port;
    __u8 data_lanes;
    __u8 virtual_channel_id;
    __u8 discontinuous_clk_mode;
    __u8 cil_threshold_settle;
    __s32 min_blank_time_width;
    __s32 min_blank_time_height;
    __s32 preferred_mode_index;
    __u64 focuser_guid;
    __u64 torch_guid;
    __u32 cap_end;
} __attribute__((packed));

#define NVC_IOCTL_CAPS          _IOWR('o', 106, struct nvc_imager_cap)
#define NVC_IOCTL_MODE_WR       _IOW('o', 107, struct nvc_imager_bayer)
#define NVC_IOCTL_MODE_RD       _IOWR('o', 108, struct nvc_imager_mode_list)


#endif /* __NVC_IMAGE_H__ */

