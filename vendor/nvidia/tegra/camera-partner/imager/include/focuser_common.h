/*
 * Copyright (c) 2012, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
*/

#ifndef __FOCUSER_COMMON_H__
#define __FOCUSER_COMMON_H__

#ifndef INT_MAX
#define INT_MAX    0x7fffffff
#endif

#define AF_INVALID_VALUE INT_MAX

#define NV_FOCUSER_SET_MAX              10
#define NV_FOCUSER_SET_DISTANCE_PAIR    16

struct nv_focuser_set_dist_pairs {
 __s32 fdn;
 __s32 distance;
};

struct nv_focuser_set {
 __s32 posture;
 __s32 macro;
 __s32 hyper;
 __s32 inf;
 __s32 hysteresis;
 __u32 settle_time;
 __s32 macro_offset;
 __s32 inf_offset;
 __u32 num_dist_pairs;
 struct nv_focuser_set_dist_pairs dist_pair[NV_FOCUSER_SET_DISTANCE_PAIR];
} __attribute__((packed));

struct nv_focuser_config {
 __u32 focal_length;
 __u32 fnumber;
 __u32 max_aperture;
 __s32 actuator_range;
 __u32 settle_time;
 __u32 range_ends_reversed;
 __s32 pos_working_low;
 __s32 pos_working_high;
 __s32 pos_actual_low;
 __s32 pos_actual_high;
 __u32 slew_rate;
 __u32 circle_of_confusion;
 __u32 num_focuser_sets;
 struct nv_focuser_set focuser_set[NV_FOCUSER_SET_MAX];
} __attribute__((packed));

#endif
