/*
 * Copyright (c) 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef __QNX_NVDISP_DEVCTL_H
#define __QNX_NVDISP_DEVCTL_H

#include <nvdc.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <devctl.h>
#include <qnx/nvdisp_output.h>

#define NVDISP_GET_WINDOW            \
    __DIOT(_DCMD_MISC, 1, unsigned int)

#define NVDISP_PUT_WINDOW            \
    __DIOT(_DCMD_MISC, 2, unsigned int)

#define NVDISP_FLIP                \
    __DIOTF(_DCMD_MISC, 3, struct nvdcFlipArgs)

#define NVDISP_GET_MODE                \
    __DIOF(_DCMD_MISC, 4, struct nvdcMode)

#define NVDISP_SET_MODE                \
    __DIOT(_DCMD_MISC, 5, struct nvdcMode)

#define NVDISP_SET_LUT                \
    __DIOT(_DCMD_MISC, 6, struct nvdisp_lut)

struct nvdisp_lut {
    int window;
    unsigned int start;
    unsigned int len;
    /*
     * Following arrays have type char since current hardware only supports
     * 8 bit color values (note "struct nvdc" uses bloated type NvU16).
     */
    char r[256];
    char g[256];
    char b[256];
};

struct nvdisp_csc {
    int window;
    uint16_t yof;
    uint16_t kyrgb;
    uint16_t kur;
    uint16_t kvr;
    uint16_t kug;
    uint16_t kvg;
    uint16_t kub;
    uint16_t kvb;
};

#define NVDISP_SET_CSC                \
    __DIOT(_DCMD_MISC, 7, struct nvdisp_csc)

struct nvdisp_output_prop {
    enum nvdcDisplayType type;
    unsigned int handle;
    bool connected;
    int assosciated_head;
    unsigned int head_mask;
};

#define NVDISP_CTRL_GET_NUM_OUTPUTS        \
    __DIOF(_DCMD_MISC, 1, unsigned int)

#define NVDISP_CTRL_GET_OUTPUT_PROPERTIES    \
    __DIOTF(_DCMD_MISC, 2, struct nvdisp_output_prop)

#endif /* __QNX_NVDISP_DEVCTL_H */
