/**
 * Copyright (c) 2011 - 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef __NVC_TORCH_H__
#define __NVC_TORCH_H__

struct nvc_torch_level_info {
    __s32 guidenum;
    __u32 sustaintime;
    __s32 rechargefactor;
} __attribute__((packed));

struct nvc_torch_pin_state {
    __u16 mask;
    __u16 values;
} __attribute__((packed));

struct nvc_torch_flash_capabilities {
    __u32 numberoflevels;
    struct nvc_torch_level_info levels[];
} __attribute__((packed));

struct nvc_torch_torch_capabilities {
    __u32 numberoflevels;
    __s32 guidenum[];
} __attribute__((packed));

#endif /* __NVC_TORCH_H__ */

