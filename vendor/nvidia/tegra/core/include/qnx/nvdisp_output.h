/*
 * Copyright (c) 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef __QNX_NVDISP_OUTPUT_H
#define __QNX_NVDISP_OUTPUT_H

#include <devctl.h>
#define NVDISP_MAX_OUTPUTS    (2)

enum nvdisp_display_type {
    NVDISP_LCD,
    NVDISP_HDMI,
};

struct nvdisp_output {
    enum nvdisp_display_type disp;
    unsigned int hActive;
    unsigned int vActive;
    unsigned int hSyncWidth;
    unsigned int vSyncWidth;
    unsigned int hFrontPorch;
    unsigned int vFrontPorch;
    unsigned int hBackPorch;
    unsigned int vBackPorch;
    unsigned int pclkKHz;
    unsigned int bpp;
};

#define NVDISP_CTRL_REGISTER_OUTPUTS    \
    __DIOT(_DCMD_MISC, 3, unsigned int)

#endif /* __QNX_NVDISP_OUTPUT_H */
