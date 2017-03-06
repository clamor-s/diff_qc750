/*
 * Copyright (c) 2011 - 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef __NVHOST_DEVCTLS_H
#define __NVHOST_DEVCTLS_H

#include <qnx/rmos_types.h>
#include <devctl.h>

#define NVHOST_DEV_PREFIX           "/dev/nvhost-"
#define NVHOST_NO_TIMEOUT           -1
#define NVHOST_SUBMIT_VERSION_V0    0x0
#define NVHOST_SUBMIT_VERSION_V1    0x1
#define NVHOST_SUBMIT_VERSION_MAX_SUPPORTED NVHOST_SUBMIT_VERSION_V1

/* version 1 header (used with ioctl() submit interface) */
struct nvhost_submit_hdr_ext {
    rmos_u32 syncpt_id;         /* version 0 fields */
    rmos_u32 syncpt_incrs;
    rmos_u32 num_cmdbufs;
    rmos_u32 num_relocs;
    rmos_u32 submit_version;    /* version 1 fields */
    rmos_u32 num_waitchks;
    rmos_u32 waitchk_mask;
    rmos_u32 pad[5];            /* future expansion */
};

struct nvhost_cmdbuf {
    rmos_u32 mem;
    rmos_u32 offset;
    rmos_u32 words;
};

struct nvhost_reloc {
    rmos_u32 cmdbuf_mem;
    rmos_u32 cmdbuf_offset;
    rmos_u32 target;
    rmos_u32 target_offset;
};

struct nvhost_waitchk {
    rmos_u32 mem;
    rmos_u32 offset;
    rmos_u32 syncpt_id;
    rmos_u32 thresh;
};

struct nvhost_get_param_args {
    rmos_u32 value;
};

struct nvhost_set_nvmap_client_args {
    rmos_u32 fd;
};

struct nvhost_read_3d_reg_args {
    rmos_u32 offset;
    rmos_u32 value;
};

#define NVHOST_DEVCTL_CHANNEL_FLUSH             \
    __DIOTF(_DCMD_MISC, 1, struct nvhost_submit_hdr_ext)
#define NVHOST_DEVCTL_CHANNEL_GET_SYNCPOINTS    \
    __DIOF(_DCMD_MISC, 2, struct nvhost_get_param_args)
#define NVHOST_DEVCTL_CHANNEL_GET_WAITBASES     \
    __DIOF(_DCMD_MISC, 3, struct nvhost_get_param_args)
#define NVHOST_DEVCTL_CHANNEL_GET_MODMUTEXES    \
    __DIOF(_DCMD_MISC, 4, struct nvhost_get_param_args)
#define NVHOST_DEVCTL_CHANNEL_SET_NVMAP_CLIENT  \
    __DIOT(_DCMD_MISC, 5, struct nvhost_set_nvmap_client_args)
#define NVHOST_DEVCTL_CHANNEL_READ_3D_REG  \
    __DIOTF(_DCMD_MISC, 6, struct nvhost_read_3d_reg_args)

#define NVHOST_DEVCTL_CHANNEL_MAX_ARG_SIZE sizeof(struct nvhost_submit_hdr_ext)

struct nvhost_ctrl_syncpt_read_args {
    rmos_u32 id;
    rmos_u32 value;
};

struct nvhost_ctrl_syncpt_incr_args {
    rmos_u32 id;
};

struct nvhost_ctrl_syncpt_wait_args {
    rmos_u32 id;
    rmos_u32 thresh;
    rmos_s32 timeout;
};

struct nvhost_ctrl_syncpt_waitex_args {
    rmos_u32 id;
    rmos_u32 thresh;
    rmos_s32 timeout;
    rmos_u32 value;
};

struct nvhost_ctrl_module_mutex_args {
    rmos_u32 id;
    rmos_u32 lock;
};

struct nvhost_ctrl_module_regrdwr_args {
    rmos_u32 id;
    rmos_u32 num_offsets;
    rmos_u32 block_size;
    rmos_u32 *offsets;
    rmos_u32 *values;
    rmos_u32 write;
};

#define NVHOST_DEVCTL_CTRL_SYNCPT_READ      \
    __DIOTF(_DCMD_MISC, 1, struct nvhost_ctrl_syncpt_read_args)
#define NVHOST_DEVCTL_CTRL_SYNCPT_INCR      \
    __DIOT(_DCMD_MISC, 2, struct nvhost_ctrl_syncpt_incr_args)
#define NVHOST_DEVCTL_CTRL_SYNCPT_WAIT      \
    __DIOT(_DCMD_MISC, 3, struct nvhost_ctrl_syncpt_wait_args)
#define NVHOST_DEVCTL_CTRL_MODULE_MUTEX     \
    __DIOTF(_DCMD_MISC, 4, struct nvhost_ctrl_module_mutex_args)
#define NVHOST_DEVCTL_CTRL_MODULE_REGRDWR   \
    __DIOTF(_DCMD_MISC, 5, struct nvhost_ctrl_module_regrdwr_args)
#define NVHOST_DEVCTL_CTRL_SYNCPT_WAITEX    \
    __DIOTF(_DCMD_MISC, 6, struct nvhost_ctrl_syncpt_waitex_args)
#define NVHOST_DEVCTL_CTRL_SYNCPT_READ_MAX  \
    __DIOTF(_DCMD_MISC, 7, struct nvhost_ctrl_syncpt_read_args)

#define NVHOST_DEVCTL_CTRL_MAX_ARG_SIZE sizeof(struct nvhost_ctrl_module_regrdwr_args)

#endif
