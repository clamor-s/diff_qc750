/*
 * Copyright (c) 2009 - 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "dc_tvo2_hal.h"
#include "nvrm_drf.h"
#include "nvrm_module.h"
#include "nvrm_analog.h"
#include "nvrm_pinmux.h"
#include "nvodm_disp.h"
#include "nvodm_services.h"

//[7-5]=RGB sync
//[4]=0 RGB outputs
//[3]=COMP_YUV
//   = 0: Component RGB on the RGB video output (default)
//   = 1: Component YUV on the RGB video output
//[0]=CVBS_ENABLE
//   = 0: when bit [3]=0
//   = 1: when bit [3]=1
#define MISC1_QT_VALUE 0xE0
#define MISC1_SI_VALUE 0xE9
#define MISC1_VALUE MISC1_SI_VALUE

static DcTvo2 s_Tvo;

static NvOdmDispDeviceMode tv_modes[] =
{
    // NTSC, 480p
    { 720, // width
      480, // height
      16, // bpp
      0, // refresh
      54000, // frequency in khz
      0, // flags
      // timing
      { 0, // h_ref_to_sync
        2, // v_ref_to_sync
        2, // h_sync_width
        2, // v_sync_width
        10, // h_back_porch
        -1, // v_back_porch
        720, // h_disp_active
        480, // v_disp_active
        3, // h_front_porch
        3, // v_front_porch
      },
      NV_FALSE // partial
    },

    // PAL, 576p
    { 720, // width
      576, // height
      16, // bpp
      0, // refresh
      54000, // frequency
      0, // flags
      // timing
      { 0, // h_ref_to_sync
        2, // v_ref_to_sync
        2, // h_sync_width
        2, // v_sync_width
        10, // h_back_porch
        -1, // v_back_porch
        720, // h_disp_active
        576, // v_disp_active
        3, // h_front_porch
        3, // v_front_porch
      },
      NV_FALSE // partial
    },

    // 720p
    { 1280, // width
      720, // height
      16, // bpp
      0, // refresh
      74250, // frequency
      0, // flags
      // timing
      { 0, // h_ref_to_sync
        2, // v_ref_to_sync
        2, // h_sync_width
        2, // v_sync_width
        10, // h_back_porch
        -1, // v_back_porch
        1280, // h_disp_active
        720, // v_disp_active
        3, // h_front_porch
        3, // v_front_porch
      },
      NV_FALSE // partial
    },

    // 1080i
    { 1920, // width
      1080, // height
      16, // bpp
      0, // refresh
      148500, // frequency
      0, // flags
      // timing
      { 0, // h_ref_to_sync
        2, // v_ref_to_sync
        2, // h_sync_width
        2, // v_sync_width
        10, // h_back_porch
        -1, // v_back_porch
        1920, // h_disp_active
        1080, // v_disp_active
        3, // h_front_porch
        3, // v_front_porch
      },
      NV_FALSE // partial
    },
};

/* the tvo encoder core needs some programming per mode.
 * see artvo.spec for encoder programming tables, with minor modifications.
 *
 * macrovision and wss registers are secret and magical.
 */

static TvoEncoderData s_NtscEncData[] =
{
    { NV_PTVO_INDIR_CHROMA_BYTE3, 0x10 },
    { NV_PTVO_INDIR_CHROMA_BYTE2, 0xf8 },
    { NV_PTVO_INDIR_CHROMA_BYTE1, 0x3e },
    { NV_PTVO_INDIR_CHROMA_BYTE0, 0x0f },
    { NV_PTVO_INDIR_CHROMA_PHASE, 0xf9 },
    /* cphase_rst 2, chroma_bw_yuv 1*/
    { NV_PTVO_INDIR_MISC3, (NvU8)(2 << 4) | (1 << 2)},
    { NV_PTVO_INDIR_CR_GAIN, 139 },
    { NV_PTVO_INDIR_CB_GAIN, 139 },
    { NV_PTVO_INDIR_CR_BURST_AMP, 0 },
    { NV_PTVO_INDIR_CB_BURST_AMP, 60 },
    { NV_PTVO_INDIR_HSYNC_WIDTH, 126 },
    { NV_PTVO_INDIR_BURST_WIDTH, 64 },
    { NV_PTVO_INDIR_BACK_PORCH, 126 },
    { NV_PTVO_INDIR_FRONT_PORCH_MSB, (NvU8) (40 >> 8) },
    { NV_PTVO_INDIR_FRONT_PORCH_LSB, (NvU8) (40 & 0xFF) },
    { NV_PTVO_INDIR_BREEZE_WAY, 16 },
    { NV_PTVO_INDIR_ACTIVELINE_MSB, (NvU8)(1424 >> 8) },
    { NV_PTVO_INDIR_ACTIVELINE_LSB, (NvU8)(1424 & 0xFF ) },
    { NV_PTVO_INDIR_BLANK_LEVEL_MSB, (NvU8)(240 >> 8) },
    { NV_PTVO_INDIR_BLANK_LEVEL_LSB, (NvU8)(240 & 0xFF) },
    { NV_PTVO_INDIR_VBI_BLANK_LEVEL_MSB, (NvU8)(240 >> 8) },
    { NV_PTVO_INDIR_VBI_BLANK_LEVEL_LSB, (NvU8)(240 & 0xFF) },
    { NV_PTVO_INDIR_BLACK_LEVEL_MSB, (NvU8)(282 >> 8) },
    { NV_PTVO_INDIR_BLACK_LEVEL_LSB, (NvU8)(282 & 0xFF) },
    { NV_PTVO_INDIR_WHITE_LEVEL_MSB, (NvU8)(800 >> 8) },
    { NV_PTVO_INDIR_WHITE_LEVEL_LSB, (NvU8)(800 & 0xFF) },
    { NV_PTVO_INDIR_SYNC_LEVEL, 16 },
    { NV_PTVO_INDIR_NUM_LINES_MSB, (NvU8)(525 >> 8) },
    { NV_PTVO_INDIR_NUM_LINES_LSB, (NvU8)(525 & 0xFF) },
    { NV_PTVO_INDIR_FIRSTVIDEOLINE, 21 },
    /* yc_delay 4 */
    { NV_PTVO_INDIR_MISC0, (4 << 2) },
    /* cvbs_enable 1, g_sync 1 */
    { NV_PTVO_INDIR_MISC1, MISC1_VALUE },
    /* progress 0, trisync 0, pal_mode 0, secam_mode 0, vsync5 0 */
    { NV_PTVO_INDIR_MISC2, 0 },
    /* sys625_50 0 */
    { NV_PTVO_INDIR_N22, 0 },

    /* macrovision is disabled by default */
    { 0x40, 0x0000 },

    { NV_PTVO_INDIR_SOFT_RESET, 0 },
};

static TvoEncoderData s_480pEncData[] =
{
    { NV_PTVO_INDIR_CHROMA_BYTE3, 0x10 },
    { NV_PTVO_INDIR_CHROMA_BYTE2, 0xf8 },
    { NV_PTVO_INDIR_CHROMA_BYTE1, 0x3e },
    { NV_PTVO_INDIR_CHROMA_BYTE0, 0x0f },
    { NV_PTVO_INDIR_CHROMA_PHASE, 0 },
    /* cphase_rst 2, chroma_bw_yuv 1*/
    { NV_PTVO_INDIR_MISC3, (NvU8)(2 << 4) | (1 << 2)},
    { NV_PTVO_INDIR_CR_GAIN, 139 },
    { NV_PTVO_INDIR_CB_GAIN, 139 },
    { NV_PTVO_INDIR_CR_BURST_AMP, 0 },
    { NV_PTVO_INDIR_CB_BURST_AMP, 67 },
    { NV_PTVO_INDIR_HSYNC_WIDTH, 126 },
    { NV_PTVO_INDIR_BURST_WIDTH, 68 },
    { NV_PTVO_INDIR_BACK_PORCH, 118 },
    { NV_PTVO_INDIR_FRONT_PORCH_MSB, (NvU8) (32 >> 8) },
    { NV_PTVO_INDIR_FRONT_PORCH_LSB, (NvU8) (32 & 0xFF) },
    { NV_PTVO_INDIR_BREEZE_WAY, 22 },
    { NV_PTVO_INDIR_ACTIVELINE_MSB, (NvU8)(1440 >> 8) },
    { NV_PTVO_INDIR_ACTIVELINE_LSB, (NvU8)(1440 & 0xFF ) },
    { NV_PTVO_INDIR_BLANK_LEVEL_MSB, (NvU8)(251 >> 8) },
    { NV_PTVO_INDIR_BLANK_LEVEL_LSB, (NvU8)(251 & 0xFF) },
    { NV_PTVO_INDIR_VBI_BLANK_LEVEL_MSB, (NvU8)(251 >> 8) },
    { NV_PTVO_INDIR_VBI_BLANK_LEVEL_LSB, (NvU8)(251 & 0xFF) },
    { NV_PTVO_INDIR_BLACK_LEVEL_MSB, (NvU8)(251 >> 8) },
    { NV_PTVO_INDIR_BLACK_LEVEL_LSB, (NvU8)(251 & 0xFF) },
    { NV_PTVO_INDIR_WHITE_LEVEL_MSB, (NvU8)(800 >> 8) },
    { NV_PTVO_INDIR_WHITE_LEVEL_LSB, (NvU8)(800 & 0xFF) },
    { NV_PTVO_INDIR_SYNC_LEVEL, 16 },
    { NV_PTVO_INDIR_NUM_LINES_MSB, (NvU8)(525 >> 8) },
    { NV_PTVO_INDIR_NUM_LINES_LSB, (NvU8)(525 & 0xFF) },
    { NV_PTVO_INDIR_FIRSTVIDEOLINE, 42 },
    /* yc_delay 4 */
    { NV_PTVO_INDIR_MISC0, (4 << 2) },
    /* cvbs_enable 1, g_sync 1 */
    { NV_PTVO_INDIR_MISC1, (NvU8)MISC1_VALUE},
    /* progress 1, trisync 0, pal_mode 0, secam_mode 0, vsync5 0 */
    { NV_PTVO_INDIR_MISC2, 1 },
    /* sys625_50 0 */
    { NV_PTVO_INDIR_N22, 0 },

    /* macrovision is disabled by default */
    { 0x40, 0x0000 },

    { NV_PTVO_INDIR_SOFT_RESET, 0 },
};

static TvoEncoderData s_576pEncData[] =
{
    { NV_PTVO_INDIR_CHROMA_BYTE3, 0x10 },
    { NV_PTVO_INDIR_CHROMA_BYTE2, 0xf8 },
    { NV_PTVO_INDIR_CHROMA_BYTE1, 0x3e },
    { NV_PTVO_INDIR_CHROMA_BYTE0, 0x0f },
    { NV_PTVO_INDIR_CHROMA_PHASE, 0 },
    /* cphase_rst 2, chroma_bw_yuv 1*/
    { NV_PTVO_INDIR_MISC3, (NvU8)(2 << 4) | (1 << 2)},
    { NV_PTVO_INDIR_CR_GAIN, 139 },
    { NV_PTVO_INDIR_CB_GAIN, 139 },
    { NV_PTVO_INDIR_CR_BURST_AMP, 0 },
    { NV_PTVO_INDIR_CB_BURST_AMP, 67 },
    { NV_PTVO_INDIR_HSYNC_WIDTH, 126 },
    { NV_PTVO_INDIR_BURST_WIDTH, 64 },
    { NV_PTVO_INDIR_BACK_PORCH, 138 },
    { NV_PTVO_INDIR_FRONT_PORCH_MSB, (NvU8) (24 >> 8) },
    { NV_PTVO_INDIR_FRONT_PORCH_LSB, (NvU8) (24 & 0xFF) },
    { NV_PTVO_INDIR_BREEZE_WAY, 26 },
    { NV_PTVO_INDIR_ACTIVELINE_MSB, (NvU8)(1440 >> 8) },
    { NV_PTVO_INDIR_ACTIVELINE_LSB, (NvU8)(1440 & 0xFF ) },
    { NV_PTVO_INDIR_BLANK_LEVEL_MSB, (NvU8)(251 >> 8) },
    { NV_PTVO_INDIR_BLANK_LEVEL_LSB, (NvU8)(251 & 0xFF) },
    { NV_PTVO_INDIR_VBI_BLANK_LEVEL_MSB, (NvU8)(251 >> 8) },
    { NV_PTVO_INDIR_VBI_BLANK_LEVEL_LSB, (NvU8)(251 & 0xFF) },
    { NV_PTVO_INDIR_BLACK_LEVEL_MSB, (NvU8)(251 >> 8) },
    { NV_PTVO_INDIR_BLACK_LEVEL_LSB, (NvU8)(251 & 0xFF) },
    { NV_PTVO_INDIR_WHITE_LEVEL_MSB, (NvU8)(800 >> 8) },
    { NV_PTVO_INDIR_WHITE_LEVEL_LSB, (NvU8)(800 & 0xFF) },
    { NV_PTVO_INDIR_SYNC_LEVEL, 16 },
    { NV_PTVO_INDIR_NUM_LINES_MSB, (NvU8)(629 >> 8) },
    { NV_PTVO_INDIR_NUM_LINES_LSB, (NvU8)(629 & 0xFF) },
    { NV_PTVO_INDIR_FIRSTVIDEOLINE, 42 },
    /* yc_delay 4 */
    { NV_PTVO_INDIR_MISC0, (4 << 2) },
    /* cvbs_enable 1, g_sync 1 */
    { NV_PTVO_INDIR_MISC1, (NvU8)MISC1_VALUE},
    /* progress 1, trisync 0, pal_mode 0, secam_mode 0, vsync5 0 */
    { NV_PTVO_INDIR_MISC2, 1 },
    /* sys625_50 0 */
    { NV_PTVO_INDIR_N22, 0 },

    /* macrovision is disabled by default */
    { 0x40, 0x0000 },

    { NV_PTVO_INDIR_SOFT_RESET, 0 },
};

/* Taken from enc.s file generated from hw run on QT under tvo_init.js
 * This enc.s is the same as file:
 * hw/Handheld/aurora/traces/tvo/nv5x_stuff/P720Progress_p720.s
 */
static TvoEncoderData s_720pEncData[] =
{
    { NV_PTVO_INDIR_CHROMA_BYTE3, 0x10 },
    { NV_PTVO_INDIR_CHROMA_BYTE2, 0xf8 },
    { NV_PTVO_INDIR_CHROMA_BYTE1, 0x3e },
    { NV_PTVO_INDIR_CHROMA_BYTE0, 0x0f },
    { NV_PTVO_INDIR_CHROMA_PHASE, 0 },
    /* cphase_rst 2, chroma_bw_yuv 1*/
    { NV_PTVO_INDIR_MISC3, (NvU8)(2 << 4) | (1 << 2)},
    { NV_PTVO_INDIR_CR_GAIN, 139 },
    { NV_PTVO_INDIR_CB_GAIN, 139 },
    { NV_PTVO_INDIR_CR_BURST_AMP, 0 },
    { NV_PTVO_INDIR_CB_BURST_AMP, 60 },
    { NV_PTVO_INDIR_HSYNC_WIDTH, 80 },
    { NV_PTVO_INDIR_BURST_WIDTH, 8 },
    { NV_PTVO_INDIR_BACK_PORCH, 220},
    { NV_PTVO_INDIR_FRONT_PORCH_MSB, (NvU8) (70 >> 8) },
    { NV_PTVO_INDIR_FRONT_PORCH_LSB, (NvU8) (70 & 0xFF) },
    { NV_PTVO_INDIR_BREEZE_WAY, 8 },
    { NV_PTVO_INDIR_ACTIVELINE_MSB, (NvU8)(1280 >> 8) },
    { NV_PTVO_INDIR_ACTIVELINE_LSB, (NvU8)(1280 & 0xFF ) },
    { NV_PTVO_INDIR_BLANK_LEVEL_MSB, (NvU8)(251 >> 8) },
    { NV_PTVO_INDIR_BLANK_LEVEL_LSB, (NvU8)(251 & 0xFF) },
    { NV_PTVO_INDIR_VBI_BLANK_LEVEL_MSB, (NvU8)(251>> 8) },
    { NV_PTVO_INDIR_VBI_BLANK_LEVEL_LSB, (NvU8)(251 & 0xFF) },
    { NV_PTVO_INDIR_BLACK_LEVEL_MSB, (NvU8)(251>> 8) },
    { NV_PTVO_INDIR_BLACK_LEVEL_LSB, (NvU8)(251 & 0xFF) },
    { NV_PTVO_INDIR_WHITE_LEVEL_MSB, (NvU8)(800 >> 8) },
    { NV_PTVO_INDIR_WHITE_LEVEL_LSB, (NvU8)(800 & 0xFF) },
    { NV_PTVO_INDIR_SYNC_LEVEL, 16 },
    { NV_PTVO_INDIR_NUM_LINES_MSB, (NvU8)(750 >> 8) },
    { NV_PTVO_INDIR_NUM_LINES_LSB, (NvU8)(750 & 0xFF) },
    { NV_PTVO_INDIR_FIRSTVIDEOLINE, 25},
    { NV_PTVO_INDIR_ACTIVEVIDEOLINE_MSB, (NvU8)(720 >> 8) },
    { NV_PTVO_INDIR_ACTIVEVIDEOLINE_LSB, (NvU8)(720 & 0xFF) },
    /* yc_delay 4 */
    { NV_PTVO_INDIR_MISC0, (4 << 2) },
    /* cvbs_enable 1, g_sync 1
     * NOTE: 0xE1 get correct first frame but not correct from 2nd frame
     */
    { NV_PTVO_INDIR_MISC1, (NvU8)MISC1_VALUE},
    /* progress 1, trisync 1, pal_mode 0, secam_mode 0, vsync5 0 */
    { NV_PTVO_INDIR_MISC2, 1 | (1 << 1) },
    /* sys625_50 0 */
    { NV_PTVO_INDIR_N22, 0 },

    /* macrovision is disabled by default */
    { 0x40, 0x0000 },

    { NV_PTVO_INDIR_SOFT_RESET, 0 },
};

static TvoEncoderData s_1080iEncData[] =
{
    { NV_PTVO_INDIR_CHROMA_BYTE3, 0x10 },
    { NV_PTVO_INDIR_CHROMA_BYTE2, 0xf8 },
    { NV_PTVO_INDIR_CHROMA_BYTE1, 0x3e },
    { NV_PTVO_INDIR_CHROMA_BYTE0, 0x0f },
    { NV_PTVO_INDIR_CHROMA_PHASE, 0 },
    /* cphase_rst 2, chroma_bw_yuv 1*/
    { NV_PTVO_INDIR_MISC3, (NvU8)(2 << 4) | (1 << 2)},
    { NV_PTVO_INDIR_CR_GAIN, 139 },
    { NV_PTVO_INDIR_CB_GAIN, 139 },
    { NV_PTVO_INDIR_CR_BURST_AMP, 0 },
    { NV_PTVO_INDIR_CB_BURST_AMP, 60 },
    { NV_PTVO_INDIR_HSYNC_WIDTH, 88 },
    { NV_PTVO_INDIR_BURST_WIDTH, 8 },
    { NV_PTVO_INDIR_BACK_PORCH, 148 },
    { NV_PTVO_INDIR_FRONT_PORCH_MSB, (NvU8) (44 >> 8) },
    { NV_PTVO_INDIR_FRONT_PORCH_LSB, (NvU8) (44 & 0xFF) },
    { NV_PTVO_INDIR_BREEZE_WAY, 8 },
    { NV_PTVO_INDIR_ACTIVELINE_MSB, (NvU8)(1920 >> 8) },
    { NV_PTVO_INDIR_ACTIVELINE_LSB, (NvU8)(1920 & 0xFF ) },
    { NV_PTVO_INDIR_BLANK_LEVEL_MSB, (NvU8)(251 >> 8) },
    { NV_PTVO_INDIR_BLANK_LEVEL_LSB, (NvU8)(251 & 0xFF) },
    { NV_PTVO_INDIR_VBI_BLANK_LEVEL_MSB, (NvU8)(251 >> 8) },
    { NV_PTVO_INDIR_VBI_BLANK_LEVEL_LSB, (NvU8)(251 & 0xFF) },
    { NV_PTVO_INDIR_BLACK_LEVEL_MSB, (NvU8)(251 >> 8) },
    { NV_PTVO_INDIR_BLACK_LEVEL_LSB, (NvU8)(251 & 0xFF) },
    { NV_PTVO_INDIR_WHITE_LEVEL_MSB, (NvU8)(800 >> 8) },
    { NV_PTVO_INDIR_WHITE_LEVEL_LSB, (NvU8)(800 & 0xFF) },
    { NV_PTVO_INDIR_SYNC_LEVEL, 16 },
    { NV_PTVO_INDIR_NUM_LINES_MSB, (NvU8)(1125 >> 8) },
    { NV_PTVO_INDIR_NUM_LINES_LSB, (NvU8)(1125 & 0xFF) },
    { NV_PTVO_INDIR_FIRSTVIDEOLINE, 21 },
    { NV_PTVO_INDIR_ACTIVEVIDEOLINE_MSB, (NvU8)(540 >> 8) },
    { NV_PTVO_INDIR_ACTIVEVIDEOLINE_LSB, (NvU8)(540 & 0xFF) },
    /* yc_delay 4 */
    { NV_PTVO_INDIR_MISC0, (4 << 2) },
    /* cvbs_enable 1, g_sync 1 */
    { NV_PTVO_INDIR_MISC1, (NvU8)MISC1_VALUE},
    /* progress 0, trisync 1, pal_mode 0, secam_mode 0, vsync5 0 */
    { NV_PTVO_INDIR_MISC2, (1 << 1)},
    /* sys625_50 0 */
    { NV_PTVO_INDIR_N22, 0 },

    /* macrovision is disabled by default */
    { 0x40, 0x0000 },

    { NV_PTVO_INDIR_SOFT_RESET, 0 },
};

static TvoEncoderData s_PalEncData[] =
{
    { NV_PTVO_INDIR_CHROMA_BYTE3, 0x15 },
    { NV_PTVO_INDIR_CHROMA_BYTE2, 0x04 },
    { NV_PTVO_INDIR_CHROMA_BYTE1, 0xC5 },
    { NV_PTVO_INDIR_CHROMA_BYTE0, 0x66 },
    { NV_PTVO_INDIR_CHROMA_PHASE, 0x17 },
    /* cphase_rst 0, chroma_bw_yuv 1 */
    { NV_PTVO_INDIR_MISC3, (1 << 2) },
    { NV_PTVO_INDIR_CR_GAIN, 148 },
    { NV_PTVO_INDIR_CB_GAIN, 148 },
    { NV_PTVO_INDIR_CR_BURST_AMP, 31 },
    { NV_PTVO_INDIR_CB_BURST_AMP, 44 },
    { NV_PTVO_INDIR_HSYNC_WIDTH, 126 },
    { NV_PTVO_INDIR_BURST_WIDTH, 64 },
    { NV_PTVO_INDIR_BACK_PORCH, 138 },
    { NV_PTVO_INDIR_FRONT_PORCH_MSB, (NvU8) (40 >> 8) },
    { NV_PTVO_INDIR_FRONT_PORCH_LSB, (NvU8) (40 & 0xFF) },
    { NV_PTVO_INDIR_BREEZE_WAY, 26 },
    { NV_PTVO_INDIR_ACTIVELINE_MSB, (NvU8)(1424 >> 8) },
    { NV_PTVO_INDIR_ACTIVELINE_LSB, (NvU8)(1424 & 0xFF ) },
    { NV_PTVO_INDIR_BLANK_LEVEL_MSB, (NvU8)(251 >> 8) },
    { NV_PTVO_INDIR_BLANK_LEVEL_LSB, (NvU8)(251 & 0xFF) },
    { NV_PTVO_INDIR_VBI_BLANK_LEVEL_MSB, (NvU8)(251 >> 8) },
    { NV_PTVO_INDIR_VBI_BLANK_LEVEL_LSB, (NvU8)(251 & 0xFF) },
    { NV_PTVO_INDIR_BLACK_LEVEL_MSB, (NvU8)(251 >> 8) },
    { NV_PTVO_INDIR_BLACK_LEVEL_LSB, (NvU8)(251 & 0xFF) },
    { NV_PTVO_INDIR_WHITE_LEVEL_MSB, (NvU8)(800 >> 8) },
    { NV_PTVO_INDIR_WHITE_LEVEL_LSB, (NvU8)(800 & 0xFF) },
    { NV_PTVO_INDIR_SYNC_LEVEL, 16 },
    { NV_PTVO_INDIR_NUM_LINES_MSB, (NvU8)(625 >> 8) },
    { NV_PTVO_INDIR_NUM_LINES_LSB, (NvU8)(625 & 0xFF) },
    { NV_PTVO_INDIR_FIRSTVIDEOLINE, 23 },
    /* yc_delay 4 */
    { NV_PTVO_INDIR_MISC0, 0x10},
    /* cvbs_enable 1, g_sync 1 */
    { NV_PTVO_INDIR_MISC1, MISC1_VALUE },
    /* progress 0, trisync 0, pal_mode 1, secam_mode 0, vsync5 1 */
    { NV_PTVO_INDIR_MISC2, (1 << 4) | (1 << 2) },
     /* sys625_50 1 */
    { NV_PTVO_INDIR_N22, 1 },

    /* macrovision disabled by default) */
    { 0x40, 0x0000 },

    { NV_PTVO_INDIR_SOFT_RESET, 0 },
};

static TvoEncoderData s_PalWssEnable[] =
{
    /* registers 0x67-6C contain the field-0 and field-1 wss 14-bit data */

    // bit 0 field-0 enable, bit 1 field-1 enable, bit[3:2]=01=PAL
    { 0x60, 0x67 },
    { 0x61, 0x1 },
    { 0x62, 0x7b },
    { 0x65, 0x16 },
    { 0x66, 0x16 },
    { 0x63, 0x2 },
    { 0x64, 0x82 },
};

static TvoEncoderData s_WssEnable[] =
{
    /* registers 0x67-6C contain the field-0 and field-1 wss 20-bit data */

    // bit 0 field-0 enable, bit 1 field-1 enable, bit[3:2]=10=NTSC
    { 0x60, 0x5b },
    { 0x61, 0x2 },
    { 0x62, 0xf7 },
    { 0x65, 0x13 },
    { 0x66, 0x13 },
    { 0x63, 0x2 },
    { 0x64, 0x78 },
};

static TvoEncoderData s_WssDisable[] =
{
    { 0x60, 0x58 }, // bit 0 field-0 enable, bit 1 field-1 enable
};

static void
DcTvo2ProgramEncoder( DcTvo2 *tvo, TvoEncoderData *data, NvU32 len )
{
    NvU32 val;

    while( len )
    {
        val = NV_DRF_NUM( TVO, ENCODER_REGISTER, REGINDEX, data->addr )
            | NV_DRF_NUM( TVO, ENCODER_REGISTER, REGDATA, data->data )
            | NV_DRF_NUM( TVO, ENCODER_REGISTER, REG_RWN, 0 );
        NV_DEBUG_PRINTF(( "tvo encoder register: %.8x\n", val ));
        TVO2_WRITE( tvo, TVO_ENCODER_REGISTER_0, val );

        data++;
        len--;
    }
}

static void
DcTvoProgramCustomFilter( DcTvo2 *tvo )
{
    NvU32 val;

    val = 0x00008000;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_W00_0, val );     // 0x542c0114
    TVO2_WRITE( tvo, TVO_HFILTER_C_W00_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_W00_0, val );
    val = 0x00006600;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_W01_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_W01_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_W01_0, val );
    val = 0x00004600;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_W02_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_W02_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_W02_0, val );
    val = 0x00004000;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_W03_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_W03_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_W03_0, val );
    val = 0x00004600;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_W04_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_W04_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_W04_0, val );
    val = 0x00006600;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_W05_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_W05_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_W05_0, val );
    val = 0x00008000;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_W06_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_W06_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_W06_0, val );

    val = 0x8000ee00;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_PW00_0, val );    // 0x542c0130
    TVO2_WRITE( tvo, TVO_HFILTER_C_PW00_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_PW00_0, val );
    val = 0x8000f200;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_PW01_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_PW01_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_PW01_0, val );
    val = 0x00000c00;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_PW02_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_PW02_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_PW02_0, val );
    val = 0x00002000;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_PW03_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_PW03_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_PW03_0, val );
    val = 0x00003a00;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_PW04_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_PW04_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_PW04_0, val );
    val = 0x00007400;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_PW05_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_PW05_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_PW05_0, val );
    val = 0x00009200;
    TVO2_WRITE( tvo,TVO_HFILTER_Y_PW06_0 , val );
    TVO2_WRITE( tvo,TVO_HFILTER_C_PW06_0 , val );
    TVO2_WRITE( tvo,TVO_VFILTER_YC_PW06_0 , val );

    val = 0x8000dc00;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_W10_0, val );     // 0x542c014c
    TVO2_WRITE( tvo, TVO_HFILTER_C_W10_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_W10_0, val );
    val = 0x8000e200;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_W11_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_W11_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_W11_0, val );
    val = 0x8000e800;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_W12_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_W12_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_W12_0, val );
    val = 0x8000ea00;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_W13_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_W13_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_W13_0, val );
    val = 0x8000e800;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_W14_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_W14_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_W14_0, val );
    val = 0x8000e200;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_W15_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_W15_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_W15_0, val );
    val = 0x8000dc00;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_W16_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_W16_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_W16_0, val );

    val = 0x00000000;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_PW10_0, val );    // 0x542c0168
    TVO2_WRITE( tvo, TVO_HFILTER_C_PW10_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_PW10_0, val );
    val = 0x00000000;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_PW11_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_PW11_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_PW11_0, val );
    val = 0x8000fa00;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_PW12_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_PW12_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_PW12_0, val );
    val = 0x8000f600;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_PW13_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_PW13_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_PW13_0, val );
    val = 0x8000ec00;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_PW14_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_PW14_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_PW14_0, val );
    val = 0x8000e200;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_PW15_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_PW15_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_PW15_0, val );
    val = 0x8000dc00;
    TVO2_WRITE( tvo, TVO_HFILTER_Y_PW16_0, val );
    TVO2_WRITE( tvo, TVO_HFILTER_C_PW16_0, val );
    TVO2_WRITE( tvo, TVO_VFILTER_YC_PW16_0, val );

    val = 0x13;
    TVO2_WRITE( tvo, TVO_VFILTCTRL_0, val );
}

static void
DcTvo2DisableCve( DcTvo2 *tvo )
{
    TvoEncoderData data;

    /* disable the cve */
    data.addr = NV_PTVO_INDIR_SOFT_RESET;
    data.data = 1;

    DcTvo2ProgramEncoder( tvo, &data, 1 );
}

static void
DcTvo2SetPosition( DcTvo2 *tvo )
{
    NvU32 val;

    /* note that the fraction bits aren't actually used in the hardware.
     */
    val = NV_DRF_NUM( TVO, HPOS, HPOS_PIXELS, (tvo->hpos >> 16) & 0x7ff)
        | NV_DRF_NUM( TVO, HPOS, HPOS_SIGN, (tvo->hpos >> 31));
    TVO2_WRITE( tvo, TVO_HPOS_0, val );

    val = NV_DRF_NUM( TVO, VPOS, VPOS_LINES, (tvo->vpos >> 16) & 0x7ff)
        | NV_DRF_NUM( TVO, VPOS, VPOS_SIGN, (tvo->vpos >> 31));
    TVO2_WRITE( tvo, TVO_VPOS_0, val );
}

static void
DcTvo2SetCsc( DcTvo2 *tvo )
{
    NvU32 val;

    val = NV_DRF_NUM( TVO, CSC_RGB2Y_COEFF1, CSC_R2Y_COEFF, 66 )
        | NV_DRF_NUM( TVO, CSC_RGB2Y_COEFF1, CSC_G2Y_COEFF, 129 );
    TVO2_WRITE( tvo, TVO_CSC_RGB2Y_COEFF1_0, val );

    val = NV_DRF_NUM( TVO, CSC_RGB2Y_COEFF2, CSC_B2Y_COEFF, 25 )
        | NV_DRF_NUM( TVO, CSC_RGB2Y_COEFF2, CSC_YOFF_COEFF, 16 )
        | NV_DRF_DEF( TVO, CSC_RGB2Y_COEFF2, YUVHB_ENABLE, ENABLE )
        | NV_DRF_DEF( TVO, CSC_RGB2Y_COEFF2, CSC_ENABLE, ENABLE );
    TVO2_WRITE( tvo, TVO_CSC_RGB2Y_COEFF2_0, val );

    val = NV_DRF_NUM( TVO, CSC_RGB2U_COEFF1, CSC_R2U_COEFF,
            (1 << 8) | 38 ) // -38
        | NV_DRF_NUM( TVO, CSC_RGB2U_COEFF1, CSC_G2U_COEFF,
            (1 << 8) | 74 ); // -74
    TVO2_WRITE( tvo, TVO_CSC_RGB2U_COEFF1_0, val );

    val = NV_DRF_NUM( TVO, CSC_RGB2U_COEFF2, CSC_B2U_COEFF, 112 );
    TVO2_WRITE( tvo, TVO_CSC_RGB2U_COEFF2_0, val );

    val = NV_DRF_NUM( TVO, CSC_RGB2V_COEFF1, CSC_R2V_COEFF, 112 )
        | NV_DRF_NUM( TVO, CSC_RGB2V_COEFF1, CSC_G2V_COEFF,
            (1 << 8) | 94 ); // -94
    TVO2_WRITE( tvo, TVO_CSC_RGB2V_COEFF1_0, val );

    val = NV_DRF_NUM( TVO, CSC_RGB2V_COEFF2, CSC_B2V_COEFF,
            (1 << 8) | 18 ); // -18
    TVO2_WRITE( tvo, TVO_CSC_RGB2V_COEFF2_0, val );
}

static void
DcTvoGetWssAspectRatio( DcTvo2 *tvo, TvoEncoderData *data, NvU32 *len )
{
    NvU32 i;

    TvoEncoderData wss_data[] =
    {
        { 0x67, 0 },
        { 0x68, 0 },
        { 0x69, 0 },
        { 0x6A, 0 },
        { 0x6B, 0 },
        { 0x6C, 0 },
    };

    switch(tvo->ScreenFormat) {
    case NvDdkDispTvScreenFormat_Letterbox_16_9:
        if (tvo->Type == NvDdkDispTvType_Ntsc)
        {
            // 6-bit crc = 0x3a, aspect ratio b1b0 = 0x2
            wss_data[0].data = 0xe;
            wss_data[1].data = 0x80;
            wss_data[2].data = 0x2;
            wss_data[3].data = 0xe;
            wss_data[4].data = 0x80;
            wss_data[5].data = 0x2;
        }
        else
        {
            wss_data[2].data = 0xb;
            wss_data[5].data = 0xb;
        }
        break;

    case NvDdkDispTvScreenFormat_Squeeze:
        if (tvo->Type == NvDdkDispTvType_Ntsc)
        {
            // 6-bit crc = 0x18, aspect ratio b1b0 = 0x1
            wss_data[0].data = 0x6;
            wss_data[2].data = 0x1;
            wss_data[3].data = 0x6;
            wss_data[5].data = 0x1;
        }
        else
        {
            wss_data[2].data = 0x7;
            wss_data[5].data = 0x7;
        }
        break;
    case NvDdkDispTvScreenFormat_Standard_4_3:
    default:
        if (tvo->Type == NvDdkDispTvType_Ntsc)
        {
            // 6-bit crc = 0x6, aspect ratio b1b0 = 0x0
            wss_data[0].data = 0x1;
            wss_data[1].data = 0x80;
            wss_data[3].data = 0x1;
            wss_data[4].data = 0x80;
        }
        else
        {
            wss_data[2].data = 0x8;
            wss_data[5].data = 0x8;
        }
        break;
    }

    *len = NV_ARRAY_SIZE( wss_data );
    for (i = 0; i < (*len); i++)
    {
        data[i].data = wss_data[i].data;
    }
}

static void
DcTvoProgramScreenFormat( DcTvo2 *tvo )
{
    TvoEncoderData *enc = 0;
    NvU32 len = 0;

    TvoEncoderData wss_data[] =
    {
        { 0x67, 0 },
        { 0x68, 0 },
        { 0x69, 0 },
        { 0x6A, 0 },
        { 0x6B, 0 },
        { 0x6C, 0 },
    };

    NV_ASSERT( tvo );
    NV_ASSERT( (tvo->Type == NvDdkDispTvType_Ntsc) ||
        (tvo->Type == NvDdkDispTvType_Pal));

    if ( tvo->Type == NvDdkDispTvType_Ntsc)
    {
        enc = s_WssEnable;
        len = NV_ARRAY_SIZE( s_WssEnable );
    }
    else
    {
        enc = s_PalWssEnable;
        len = NV_ARRAY_SIZE( s_PalWssEnable );
    }
    DcTvo2ProgramEncoder( tvo, enc, len );

    DcTvoGetWssAspectRatio( tvo, wss_data, &len );
    DcTvo2ProgramEncoder( tvo, wss_data, len );
}

// FIXME: allow arbitrary controller modes and set the scaler registers
// to convert to ntsc/pal (v/hphase, etc).

static NvBool
DcTvo2Commit( DcController *cont, DcTvo2 *tvo, NvBool update )
{
    NvError e;

    TvoEncoderData *enc = 0;
    NvRmAnalogTvDacConfig cfg;
    NvRmAnalogInterface interface;
    NvU32 len = 0;
    NvU32 val, vlines, hpixels;
    NvDdkDispCapabilities *caps = cont->caps;

    NV_ASSERT( cont );
    NV_ASSERT( tvo );

    /* the controller mode must be set */
    NV_ASSERT( cont->mode.width && cont->mode.height && cont->mode.bpp );

    /* map tvo registers */
    if( !tvo->tvo_regs )
    {
        NvRmPhysAddr phys;
        NvU32 size;
        NvU32 TvoFreq, CveFreq;

        NV_ASSERT( !update );

        NvRmModuleGetBaseAddress( cont->hRm,
           NVRM_MODULE_ID( NvRmModuleID_Tvo, 0 ),
           &phys, &size);

        NV_CHECK_ERROR_CLEANUP(
            NvRmPhysicalMemMap( phys, size, NVOS_MEM_READ_WRITE,
                NvOsMemAttribute_Uncached, &tvo->tvo_regs )
        );

        tvo->phys = phys;
        tvo->size = size;

        tvo->PowerClientId = NVRM_POWER_CLIENT_TAG('T','V','O','2');
        NV_CHECK_ERROR_CLEANUP(
            NvRmPowerRegister( cont->hRm, 0, &tvo->PowerClientId )
        );

        if (( cont->mode.width == 1920) && (cont->mode.height == 1080))
        {
            CveFreq = 148500;
            if( NVDDK_DISP_CAP_IS_SET( caps, NvDdkDispCapBit_RatioCveTvoFreq_1_2))
            {
                TvoFreq = 2 * CveFreq;
            }
            else
            {
                TvoFreq = 240000;
            }
        }
        else if (( cont->mode.width == 1280) && (cont->mode.height == 720))
        {
            TvoFreq = 120000;
            CveFreq = 148500;
        }
        else
        {
            TvoFreq = 54000;
            CveFreq = 54000;
        }

        // BUG 518736 -- clocks must be enabled before configuring them
        NV_CHECK_ERROR_CLEANUP(
            NvRmPowerModuleClockControl( cont->hRm,
                NVRM_MODULE_ID( NvRmModuleID_Tvo, 0 ),
                tvo->PowerClientId,
                NV_TRUE )
        );

        NV_CHECK_ERROR_CLEANUP(
            NvRmPowerModuleClockConfig( cont->hRm,
                NVRM_MODULE_ID( NvRmModuleID_Tvo, 0 ),
                tvo->PowerClientId,
                NvRmFreqUnspecified, NvRmFreqUnspecified,
                &TvoFreq, 1, &TvoFreq, 0 )
        );

        NV_CHECK_ERROR_CLEANUP(
            NvRmPowerModuleClockConfig( cont->hRm,
                NVRM_MODULE_ID( NvRmModuleID_Tvo, 0 ),
                tvo->PowerClientId,
                NvRmFreqUnspecified, NvRmFreqUnspecified,
                &CveFreq, 1, &CveFreq, NvRmClockConfig_SubConfig )
        );

        tvo->glob = DcTvoCommonGlobOpen();

        /* default macrovision config is PSP on, no split burst */
        tvo->context.burst = NvDdkDispMacrovisionBurst_None;
        tvo->bMacrovision = NV_FALSE;

        /* make sure encoder is off */
        DcTvo2DisableCve( tvo );

        /* color space conversion */
        DcTvo2SetCsc( tvo );
    }

    /* analog configuration -- enable the tvdac */
    interface = NVRM_ANALOG_INTERFACE( NvRmAnalogInterface_Tv, 0 );
    cfg.Source = NVRM_MODULE_ID( NvRmModuleID_Tvo, 0 );
    cfg.DacAmplitude = tvo->DacAmplitude;

    NV_CHECK_ERROR_CLEANUP(
        NvRmAnalogInterfaceControl( cont->hRm, interface, NV_TRUE,
             &cfg, sizeof(cfg) )
    );

    /* overscan stuff */
    val = NV_DRF_NUM( TVO, OVERSCAN_COMP, OVERSCAN_COMP_VALUE, tvo->Overscan );
    TVO2_WRITE( tvo, TVO_OVERSCAN_COMP_0, val );

    val = NV_DRF_NUM( TVO, OVERSCAN_COLOR, OVERSCAN_Y, tvo->OverscanY )
        | NV_DRF_NUM( TVO, OVERSCAN_COLOR, OVERSCAN_CB, tvo->OverscanCb )
        | NV_DRF_NUM( TVO, OVERSCAN_COLOR, OVERSCAN_CR, tvo->OverscanCr );
    TVO2_WRITE( tvo, TVO_OVERSCAN_COLOR_0, val );

    /* h and v position */
    DcTvo2SetPosition( tvo );

    /* output format */
    switch( tvo->OutputFormat ) {
    case NvDdkDispTvOutput_Composite:
        val = NV_DRF_DEF( TVO, CONTROL_OUT, RED, COMPOSITE )
            | NV_DRF_DEF( TVO, CONTROL_OUT, GREEN, COMPOSITE )
            | NV_DRF_DEF( TVO, CONTROL_OUT, BLUE, COMPOSITE )
            | NV_DRF_DEF( TVO, CONTROL_OUT, RED_CONTROL, ENABLE );
        break;
    case NvDdkDispTvOutput_Component:
        val = NV_DRF_DEF( TVO, CONTROL_OUT, RED, COMPONENT )
            | NV_DRF_DEF( TVO, CONTROL_OUT, GREEN, COMPONENT )
            | NV_DRF_DEF( TVO, CONTROL_OUT, BLUE, COMPONENT )
            | NV_DRF_DEF( TVO, CONTROL_OUT, RED_CONTROL, ENABLE );
        break;
    case NvDdkDispTvOutput_SVideo:
        val = NV_DRF_DEF( TVO, CONTROL_OUT, GREEN, CHROMA )
            | NV_DRF_DEF( TVO, CONTROL_OUT, BLUE, LUMA );
        break;
    default:
        NV_ASSERT(!" DcTvo2Enable Error: Unsupported Output Format. ");
    }

    val = NV_FLD_SET_DRF_DEF( TVO, CONTROL_OUT, BLUE_CONTROL, ENABLE, val );
    val = NV_FLD_SET_DRF_DEF( TVO, CONTROL_OUT, GREEN_CONTROL, ENABLE, val );

    TVO2_WRITE( tvo, TVO_CONTROL_OUT_0, val );

    /* enable tvo mode in the display controller */
    DcEnable( cont, DC_HAL_TVO, NV_TRUE );

    /* tvo top-level control register */
    val = NV_DRF_DEF( TVO, CONTROL, MASTER_CONTROL, ENABLE )
        | NV_DRF_DEF( TVO, CONTROL, OVERSCAN, ENABLE )
        | NV_DRF_DEF( TVO, CONTROL, OS_BORDER_SLOPE, ENABLE )
        | NV_DRF_DEF( TVO, CONTROL, POSITION, ENABLE )
        | NV_DRF_DEF( TVO, CONTROL, HSCALE, ENABLE )
        | NV_DRF_DEF( TVO, CONTROL, VSCALE, ENABLE )
        | NV_DRF_DEF( TVO, CONTROL, HAUTOFILT, DISABLE )
        | NV_DRF_DEF( TVO, CONTROL, VAUTOFILT, DISABLE )
        | NV_DRF_DEF( TVO, CONTROL, CLAMP, ENABLE );

    /* Filter mode */
    switch( tvo->FilterMode ) {
    case NvDdkDispTvFilterMode_PointSample:
        val = NV_FLD_SET_DRF_DEF( TVO, CONTROL, FILTER_MODE, PTSAMPLE, val );
        break;
    case NvDdkDispTvFilterMode_LowPass1:
        val = NV_FLD_SET_DRF_DEF( TVO, CONTROL, FILTER_MODE, LOWPASS1, val );
        break;
    case NvDdkDispTvFilterMode_LowPass2:
        val = NV_FLD_SET_DRF_DEF( TVO, CONTROL, FILTER_MODE, LOWPASS2, val );
        break;
    case NvDdkDispTvFilterMode_Custom:
        val = NV_FLD_SET_DRF_DEF( TVO, CONTROL, FILTER_MODE, CUSTOM, val );
        break;
    default:
        NV_ASSERT(!" DcTvo2Commit Error: Unsupported Filter Mode. ");
    }

    if( cont->index == 0 )
    {
        val = NV_FLD_SET_DRF_DEF( TVO, CONTROL, HEAD, CRTC1, val );
    }
    else
    {
        val = NV_FLD_SET_DRF_DEF( TVO, CONTROL, HEAD, CRTC2, val );
    }

    TVO2_WRITE( tvo, TVO_CONTROL_0, val );

    if( tvo->Type == NvDdkDispTvType_Ntsc )
    {
        // FIXME: remove this assert once the scaler registers are supported
        NV_ASSERT( cont->mode.height == 480 );

        val = NV_DRF_DEF( TVO, VFILTCTRL, AUTOCALC_VPHASE, ENABLE);
        TVO2_WRITE( tvo, TVO_VFILTCTRL_0, val );

        val = NV_DRF_NUM( TVO, VRES, VRES_LINES, 240 );
        TVO2_WRITE( tvo, TVO_VRES_0, val );

        val = NV_DRF_NUM( TVO, HRES, HRES_PIXELS, 720 );
        TVO2_WRITE( tvo, TVO_HRES_0, val );

        val = 0;
        TVO2_WRITE( tvo, TVO_VPHASE_FIELD1_0, val );

        val = 4096;
        TVO2_WRITE( tvo, TVO_VPHASE_FIELD2_0, val );

        enc = s_NtscEncData;
        len = NV_ARRAY_SIZE(s_NtscEncData);
    }
    else if( tvo->Type == NvDdkDispTvType_Pal )
    {
        // FIXME: remove this assert once the scaler registers are supported
        NV_ASSERT( cont->mode.height == 576 );

        val = NV_DRF_NUM( TVO, VRES, VRES_LINES, 288 );
        TVO2_WRITE( tvo, TVO_VRES_0, val );

        val = NV_DRF_NUM( TVO, HRES, HRES_PIXELS, 720 );
        TVO2_WRITE( tvo, TVO_HRES_0, val );

        /* fields fractions are opposite to ntsc */
        val = 4096;
        TVO2_WRITE( tvo, TVO_VPHASE_FIELD1_0, val );

        val = 0;
        TVO2_WRITE( tvo, TVO_VPHASE_FIELD2_0, val );

        enc = s_PalEncData;
        len = NV_ARRAY_SIZE(s_PalEncData);
    }
    else if( tvo->Type == NvDdkDispTvType_Hdtv )
    {
        hpixels = cont->mode.width;
        if( cont->mode.height == 1080 )
        {
            vlines = cont->mode.height/2;
        }
        else
        {
            vlines = cont->mode.height;
        }

        val = NV_DRF_NUM( TVO, VRES, VRES_LINES, vlines );
        TVO2_WRITE( tvo, TVO_VRES_0, val );

        val = NV_DRF_NUM( TVO, HRES, HRES_PIXELS, hpixels );
        TVO2_WRITE( tvo, TVO_HRES_0, val );

        val = 0;
        TVO2_WRITE( tvo, TVO_VPHASE_FIELD1_0, val );

        val = 0;
        TVO2_WRITE( tvo, TVO_VPHASE_FIELD2_0, val );

        switch( vlines ) {
        case 480:
            enc = s_480pEncData;
            len = NV_ARRAY_SIZE(s_480pEncData);
            break;
        case 576:
            enc = s_576pEncData;
            len = NV_ARRAY_SIZE(s_576pEncData);
            break;
        case 720:
            enc = s_720pEncData;
            len = NV_ARRAY_SIZE(s_720pEncData);
            break;
        case 540:
            enc = s_1080iEncData;
            len = NV_ARRAY_SIZE(s_1080iEncData);
            break;
        default:
            NV_ASSERT( !"bad mode value" );
            return NV_FALSE;
        }
    }

    // Screen Format
    if ((tvo->Type == NvDdkDispTvType_Ntsc) ||
        (tvo->Type == NvDdkDispTvType_Pal))
    {
        DcTvoProgramScreenFormat(tvo);
    }

    // Custom filter
    if (tvo->FilterMode == NvDdkDispTvFilterMode_Custom)
        DcTvoProgramCustomFilter(tvo);

    NV_ASSERT( enc );

#if NV_DEBUG
    val = NV_DRF_DEF( TVO, CRC_CONTROL, VSYNC_CAPTURE, ENABLE )
        | NV_DRF_DEF( TVO, CRC_CONTROL, CRC_FIELD1, ENABLE )
        | NV_DRF_DEF( TVO, CRC_CONTROL, CRC_FIELD2, ENABLE )
        | NV_DRF_DEF( TVO, CRC_CONTROL, OUT_VALID, ALL );
    TVO2_WRITE( tvo, TVO_CRC_CONTROL_0, val );
#endif

    if( !update )
    {
        /* finish encoder core programming, the core must be brought out of
         * reset last (soft_reset register)
         */
        DcTvo2ProgramEncoder( tvo, enc, len );
    }

    return NV_TRUE;

fail:
    DcEnable( cont, DC_HAL_TVO, NV_FALSE );
    NvRmPowerUnRegister( cont->hRm, tvo->PowerClientId );
    NvRmPhysicalMemUnmap( tvo->tvo_regs, tvo->size );
    tvo->PowerClientId = 0;
    tvo->tvo_regs = 0;
    tvo->bEnabled = NV_FALSE;
    return NV_FALSE;
}

void DcTvo2Type( NvDdkDispTvType Type )
{
    DcTvo2 *tvo;

    tvo = &s_Tvo;
    tvo->Type = Type;
}

void DcTvo2Attrs( NvU32 controller, NvU32 Overscan, NvU32 OverscanY,
    NvU32 OverscanCb, NvU32 OverscanCr, NvDdkDispTvOutput OutputFormat,
    NvU8 DacAmplitude, NvDdkDispTvFilterMode FilterMode,
    NvDdkDispTvScreenFormat ScreenFormat)
{
    DcController *cont;
    DcTvo2 *tvo;

    DC_CONTROLLER( controller, cont, return );
    DC_SAFE( cont, return );

    tvo = &s_Tvo;
    tvo->Overscan = Overscan;
    tvo->OverscanY = OverscanY;
    tvo->OverscanCb = OverscanCb;
    tvo->OverscanCr = OverscanCr;
    tvo->OutputFormat = OutputFormat;
    tvo->DacAmplitude = DacAmplitude;
    tvo->FilterMode = FilterMode;
    tvo->ScreenFormat = ScreenFormat;

    if( !tvo->bEnabled )
    {
        return;
    }

    /* update hardware */
    if( !DcTvo2Commit( cont, tvo, NV_TRUE ) )
    {
        cont->bFailed = NV_TRUE;
    }
}

void
DcTvo2Enable( NvU32 controller, NvBool enable )
{
    NvError e;
    DcController *cont;
    NvRmAnalogInterface interface;
    NvU32 val;
    DcTvo2 *tvo;

    DC_CONTROLLER( controller, cont, return );

    tvo = &s_Tvo;

    tvo->bEnabled = enable;
    if( enable )
    {
        DC_SAFE( cont, return );
        HOST_CLOCK_ENABLE( cont );

        DcTvo2Commit( cont, tvo, NV_FALSE );

        /* make sure memory is running fast enough */
        NV_CHECK_ERROR_CLEANUP(
            NvRmPowerBusyHint( cont->hRm, NvRmDfsClockId_Emc,
                tvo->PowerClientId, NV_WAIT_INFINITE, NvRmFreqMaximum )
        );
    }
    else
    {
        if( !tvo->tvo_regs )
        {
            return;
        }

        /* display must be off to shutdown display */
        DcStopDisplay( cont, NV_TRUE );

        if( tvo->bMacrovision )
        {
            DcTvo2MacrovisionEnable( controller, NV_FALSE );
            tvo->bMacrovision = NV_FALSE;
            tvo->bCCinit = NV_FALSE;
        }

        DcTvo2DisableCve( tvo );

        val = NV_DRF_DEF( TVO, CONTROL, MASTER_CONTROL, DISABLE );
        TVO2_WRITE( tvo, TVO_CONTROL_0, val );

        DcEnable( cont, DC_HAL_TVO, NV_FALSE );

        interface = NVRM_ANALOG_INTERFACE( NvRmAnalogInterface_Tv, 0 );
        NV_ASSERT_SUCCESS(
            NvRmAnalogInterfaceControl( cont->hRm, interface, NV_FALSE,
                NULL, 0 )
        );

        /* cancel the busy hint */
        NV_ASSERT_SUCCESS(
            NvRmPowerBusyHint( cont->hRm, NvRmDfsClockId_Emc,
                tvo->PowerClientId, 0, 0 )
        );

        NvRmPowerModuleClockControl( cont->hRm,
            NVRM_MODULE_ID( NvRmModuleID_Tvo, 0 ),
            tvo->PowerClientId,
            NV_FALSE );

        DcTvoCommonGlobClose( tvo->glob );
        tvo->glob = 0;

        NvRmPowerUnRegister( cont->hRm, tvo->PowerClientId );
        NvRmPhysicalMemUnmap( tvo->tvo_regs, tvo->size );
        NvOsMemset( tvo, 0, sizeof(DcTvo2) );

        HOST_CLOCK_DISABLE( cont );
    }

    return;

fail:
    interface = NVRM_ANALOG_INTERFACE( NvRmAnalogInterface_Tv, 0 );
    NV_ASSERT_SUCCESS(
        NvRmAnalogInterfaceControl( cont->hRm, interface, NV_FALSE, 0, 0 )
    );

    cont->bFailed = NV_TRUE;
}

void DcTvo2Position( NvU32 controller, NvU32 hpos, NvU32 vpos )
{
    DcController *cont;
    DcTvo2 *tvo;

    DC_CONTROLLER( controller, cont, return );
    DC_SAFE( cont, return );

    tvo = &s_Tvo;
    tvo->hpos = hpos;
    tvo->vpos = vpos;

    if( tvo->tvo_regs )
    {
        DcTvo2SetPosition( tvo );
    }
}

void DcTvo2ListModes( NvU32 *nModes, NvOdmDispDeviceMode *modes )
{
    DcTvo2 *tvo;
    tvo = &s_Tvo;

    NV_ASSERT( nModes );

    /* NOTE: special case the mode array based on the Type attribute for
     * NTSC or PAL.
     */

    if( !(*nModes ) )
    {
        if( tvo->Type == NvDdkDispTvType_Hdtv )
        {
            *nModes = 4;
        }
        else
        {
            *nModes = 1;
        }
    }
    else
    {
        if( !tvo->tvo_regs && (tvo->Type != NvDdkDispTvType_Pal) &&
            (tvo->Type != NvDdkDispTvType_Hdtv) )
        {
            tvo->Type = NvDdkDispTvType_Ntsc;
        }

        if( tvo->Type == NvDdkDispTvType_Ntsc )
        {
            NV_ASSERT( *nModes == 1 );
            modes[0] = tv_modes[0];
        }
        else if( tvo->Type == NvDdkDispTvType_Pal )
        {
            NV_ASSERT( *nModes == 1 );
            modes[0] = tv_modes[1];
        }
        else if( tvo->Type == NvDdkDispTvType_Hdtv )
        {
            NvU32 i = 0;
            NV_ASSERT( *nModes <= 4 );
            for( i = 0; i < *nModes; i++ )
            {
                modes[i] = tv_modes[i];
            }
        }
        else
        {
            NV_ASSERT( !"bad tv type" );
        }
    }
}

NvBool DcTvo2Discover( DcTvo2 *tvo )
{
    NvU64 guid;
    const NvU32 vals[] =
        {
            NvOdmPeripheralClass_Display,
            NvOdmIoModule_Tvo,
        };
    const NvOdmPeripheralSearch search[] =
        {
            NvOdmPeripheralSearch_PeripheralClass,
            NvOdmPeripheralSearch_IoModule,
        };
    NvOdmPeripheralConnectivity const *conn;

    /* find a display guid */
    if( !NvOdmPeripheralEnumerate( search, vals, 2, &guid, 1 ) )
    {
        return NV_FALSE;
    }

    /* get the connectivity info */
    conn = NvOdmPeripheralGetGuid( guid );
    if( !conn )
    {
        return NV_FALSE;
    }

    tvo->guid = guid;
    tvo->conn = conn;

    return NV_TRUE;
}

NvBool DcTvo2Init( void )
{
    NvU32 i;
    NvOdmPeripheralConnectivity const *conn;
    NvOdmServicesPmuHandle hPmu;
    NvU32 settle_us;
    DcTvo2 *tvo;

    tvo = &s_Tvo;

    /* get the peripheral config */
    if( !tvo->conn )
    {
        if( !DcTvo2Discover( tvo ) )
        {
            return NV_FALSE;
        }
    }

    /* enable the power rail */
    hPmu = NvOdmServicesPmuOpen();
    conn = tvo->conn;

    for( i = 0; i < conn->NumAddress; i++ )
    {
        if( conn->AddressList[i].Interface == NvOdmIoModule_Vdd )
        {
            NvOdmServicesPmuVddRailCapabilities cap;

            /* address is the vdd rail id */
            NvOdmServicesPmuGetCapabilities( hPmu,
                conn->AddressList[i].Address, &cap );

            /* set the rail volatage to the recommended */
            NvOdmServicesPmuSetVoltage( hPmu,
                conn->AddressList[i].Address, cap.requestMilliVolts,
                &settle_us );

            /* wait for the rail to settle */
            NvOdmOsWaitUS( settle_us );
        }
    }

    NvOdmServicesPmuClose( hPmu );

    return NV_TRUE;
}

void
DcTvo2Deinit( void )
{
    NvU32 i;
    NvOdmPeripheralConnectivity const *conn;
    NvOdmServicesPmuHandle hPmu;
    DcTvo2 *tvo;

    tvo = &s_Tvo;

    /* disable the power rail */
    hPmu = NvOdmServicesPmuOpen();
    if( !tvo->conn )
    {
        if( !DcTvo2Discover( tvo ) )
        {
            return;
        }
    }

    conn = tvo->conn;

    for( i = 0; i < conn->NumAddress; i++ )
    {
        if( conn->AddressList[i].Interface == NvOdmIoModule_Vdd )
        {
            /* set the rail volatage to the recommended */
            NvOdmServicesPmuSetVoltage( hPmu,
                conn->AddressList[i].Address, NVODM_VOLTAGE_OFF, 0 );
        }
    }

    NvOdmServicesPmuClose( hPmu );
}

void DcTvo2SetPowerLevel( NvOdmDispDevicePowerLevel level )
{
    switch( level ) {
    case NvOdmDispDevicePowerLevel_Off:
    case NvOdmDispDevicePowerLevel_Suspend:
        DcTvo2Deinit();
        break;
    case NvOdmDispDevicePowerLevel_On:
        DcTvo2Init();
        break;
    default:
        break;
    }
}

void DcTvo2GetParameter( NvOdmDispParameter param, NvU32 *val )
{
    switch( param ) {
    case NvOdmDispParameter_Type:
        *val = NvOdmDispDeviceType_TV;
        break;
    case NvOdmDispParameter_Usage:
        *val = NvOdmDispDeviceUsage_Removable;
        break;
    case NvOdmDispParameter_MaxHorizontalResolution:
        *val = 1920;
        break;
    case NvOdmDispParameter_MaxVerticalResolution:
        *val = 1080;
        break;
    case NvOdmDispParameter_BaseColorSize:
        *val = 0;
        break;
    case NvOdmDispParameter_DataAlignment:
        *val = 0;
        break;
    case NvOdmDispParameter_PinMap:
        *val = 0;
        break;
    case NvOdmDispParameter_PwmOutputPin:
        *val = 0;
        break;
    default:
        break;
    }
}

NvU64 DcTvo2GetGuid( void )
{
    DcTvo2 *tvo;

    tvo = &s_Tvo;
    if( !tvo->conn )
    {
        if( !DcTvo2Discover( tvo ) )
        {
            return (NvU64)-1;
        }
    }

    return tvo->guid;
}

void DcTvo2SetEdid( NvDdkDispEdid *edid, NvU32 flags )
{
    // do nothing
}

static NvBool
DcTvo2MacrovisionNtscEnable( DcController *cont, DcTvo2 *tvo )
{
    TvoEncoderData *enc;
    NvU32 len;
    NvBool b;

    if( tvo->Type != NvDdkDispTvType_Ntsc )
    {
        return NV_FALSE;
    }

    switch( tvo->context.burst ) {
    case NvDdkDispMacrovisionBurst_2line:
        b = DcTvoCommonGetMvData( tvo->glob,
                NvMvGlobEntryType_Ntsc_SplitBurst_2_Line,
                NvMvGlobHardwareType_Cve5,
                &len, &enc );
        break;
    case NvDdkDispMacrovisionBurst_4line:
        b = DcTvoCommonGetMvData( tvo->glob,
                NvMvGlobEntryType_Ntsc_SplitBurst_4_Line,
                NvMvGlobHardwareType_Cve5,
                &len, &enc );
        break;
    case NvDdkDispMacrovisionBurst_None:
    default:
        b = DcTvoCommonGetMvData( tvo->glob,
                NvMvGlobEntryType_Ntsc_NoSplitBurst,
                NvMvGlobHardwareType_Cve5,
                &len, &enc );
        break;
    }

    if( b )
    {
        DcTvo2ProgramEncoder( tvo, enc, len );
        return NV_TRUE;
    }

    return NV_FALSE;
}

void
DcTvo2MacrovisionSetContext( NvU32 controller,
    NvDdkDispMacrovisionContext *context )
{
    DcTvo2 *tvo;

    tvo = &s_Tvo;

    if( context )
    {
        tvo->context = *context;
    }
    else
    {
        NvOsMemset( &tvo->context, 0, sizeof(tvo->context) );
    }
}

NvBool
DcTvo2MacrovisionEnable( NvU32 controller, NvBool enable )
{
    DcController *cont;
    DcTvo2 *tvo;
    TvoEncoderData *enc;
    NvU32 len;
    NvBool b;

    DC_CONTROLLER( controller, cont, return NV_FALSE );

    tvo = &s_Tvo;

    if( tvo->Type == NvDdkDispTvType_Ntsc )
    {
        if( enable )
        {
            b = DcTvo2MacrovisionNtscEnable( cont, tvo );
            if( !b )
            {
                goto fail;
            }
        }
        else
        {
            TvoEncoderData data;

            enc = s_WssDisable;
            len = NV_ARRAY_SIZE( s_WssDisable );
            DcTvo2ProgramEncoder( tvo, enc, len );

            /* disable CC fields */
            data.addr = 0x6D;
            data.data = 0;
            DcTvo2ProgramEncoder( tvo, &data, 1 );

            b = DcTvoCommonGetMvData( tvo->glob,
                    NvMvGlobEntryType_Ntsc_Disable,
                    NvMvGlobHardwareType_Cve5,
                    &len, &enc );
            if( b )
            {
                DcTvo2ProgramEncoder( tvo, enc, len );
            }
            else
            {
                goto fail;
            }
        }

        tvo->bCCinit = NV_FALSE;
    }
    else if( tvo->Type == NvDdkDispTvType_Pal ||
             tvo->Type == NvDdkDispTvType_Hdtv )
    {
        NvMvGlobHardwareType hw_type = NvMvGlobHardwareType_Cve5;
        NvMvGlobEntryType entry_type;

        if( enable )
        {
            entry_type = ( tvo->Type == NvDdkDispTvType_Pal ) ?
                NvMvGlobEntryType_Pal_Enable :
                NvMvGlobEntryType_HighDef_Enable;
        }
        else
        {
            if( tvo->Type == NvDdkDispTvType_Pal)
            {
                enc = s_WssDisable;
                len = NV_ARRAY_SIZE( s_WssDisable );
                DcTvo2ProgramEncoder( tvo, enc, len );
            }

            entry_type = ( tvo->Type == NvDdkDispTvType_Pal ) ?
                NvMvGlobEntryType_Pal_Disable :
                NvMvGlobEntryType_HighDef_Disable;
        }

        b = DcTvoCommonGetMvData( tvo->glob, entry_type, hw_type,
               &len, &enc );
        if( b )
        {
            DcTvo2ProgramEncoder( tvo, enc, len );
        }
        else
        {
            goto fail;
        }
    }
    else
    {
        NV_ASSERT( !"bad tv type" );
        goto fail;
    }

    tvo->bMacrovision = enable;
    return NV_TRUE;

fail:
    tvo->bMacrovision = NV_FALSE;
    return NV_FALSE;
}

static void
DcTvo2MacrovisionPalSetCGMSA_20( NvU32 controller, NvU8 cgmsa )
{
    DcController *cont;
    DcTvo2 *tvo;
    TvoEncoderData *enc;
    NvU32 len;

    TvoEncoderData wss_data[] =
        {
            { 0x67, 0 },
            { 0x68, 0 },
            { 0x69, 0 },
            { 0x6A, 0 },
            { 0x6B, 0 },
            { 0x6C, 0 },
        };

    tvo = &s_Tvo;

    DC_CONTROLLER( controller, cont, return );

    enc = s_PalWssEnable;
    len = NV_ARRAY_SIZE( s_PalWssEnable );
    DcTvo2ProgramEncoder( tvo, enc, len );

    /* PAL wss layout:
     *
     * 0-3: aspect ratio
     * 7-4: 0's
     * 10-8: subtitiles
     * 11: surround sound bit
     * 13-12: copy protection
     *    00: Content may be copied without restriction
     *    01: Unused
     *    10: One generation may be copied
     *    11: Content may not be copied
     */

    DcTvoGetWssAspectRatio( tvo, wss_data, &len );

    wss_data[1].data = (cgmsa & 0x3) << 4;
    wss_data[4].data = (cgmsa & 0x3) << 4;

    enc = wss_data;
    len = NV_ARRAY_SIZE( wss_data );
    DcTvo2ProgramEncoder( tvo, enc, len );

    cont->bCgmsa = NV_TRUE;
}

static void
DcTvo2MacrovisionNtscSetCGMSA_20( NvU32 controller, NvU8 cgmsa )
{
    /* indexed by cgms-a value */
    static NvU8 s_crc[3][32] =
    {
        {
            0x6, 0x12, 0x2e, 0x3a, 0x37, 0x23, 0x1f, 0xb, 0x5, 0x11,
            0x2d, 0x39, 0x34, 0x20, 0x1c, 0x8, 0x0, 0x14, 0x28, 0x3c,
            0x31, 0x25, 0x19, 0xd, 0x3, 0x17, 0x2b, 0x3f, 0x32, 0x26,
            0x1a, 0xe
        },

        {
            0x18, 0xc, 0x30, 0x24, 0x29, 0x3d, 0x1, 0x15, 0x1b, 0xf,
            0x33, 0x27, 0x2a, 0x3e, 0x2, 0x16, 0x1e, 0xa, 0x36, 0x22,
            0x2f, 0x3b, 0x7, 0x13, 0x1d, 0x9, 0x35, 0x21, 0x2c, 0x38,
            0x4, 0x10
        },

        {
            0x3a, 0x2e, 0x12, 0x6, 0xb, 0x1f, 0x23, 0x37, 0x39, 0x2d,
            0x11, 0x5, 0x8, 0x1c, 0x20, 0x34, 0x3c, 0x28, 0x14, 0x0,
            0xd, 0x19, 0x25, 0x31, 0x3f, 0x2b, 0x17, 0x3, 0xe, 0x1a,
            0x26, 0x32
        }
    };

    DcController *cont;
    DcTvo2 *tvo;
    TvoEncoderData *enc;
    NvU32 len, index, wss;
    NvBool bUseMacrovision = NV_TRUE;

    TvoEncoderData wss_data[] =
        {
            { 0x67, 0 },
            { 0x68, 0 },
            { 0x69, 0 },
            { 0x6A, 0 },
            { 0x6B, 0 },
            { 0x6C, 0 },
        };

    tvo = &s_Tvo;

    DC_CONTROLLER( controller, cont, return );

    if (( tvo->Type == NvDdkDispTvType_Pal ) ||
        (( tvo->Type == NvDdkDispTvType_Hdtv) &&
         ( cont->mode.width == 720) &&
         ( cont->mode.height == 576)))
    {
        return;
    }

    /* get the burst mode */
    switch( (cgmsa >> 2) & 0x3 ) {
    case 1:
        tvo->context.burst = NvDdkDispMacrovisionBurst_None;
        break;
    case 2:
        tvo->context.burst = NvDdkDispMacrovisionBurst_2line;
        break;
    case 3:
        tvo->context.burst = NvDdkDispMacrovisionBurst_4line;
        break;
    case 0:
    default:
        tvo->context.burst = NvDdkDispMacrovisionBurst_None;
        bUseMacrovision = NV_FALSE;
        break;
    }

    if( bUseMacrovision )
    {
        DcTvo2MacrovisionNtscEnable( cont, tvo );
        cont->bMacrovision = NV_TRUE;
    }

    enc = s_WssEnable;
    len = NV_ARRAY_SIZE( s_WssEnable );
    DcTvo2ProgramEncoder( tvo, enc, len );

    /* CGMS-A is 5 bits and should be in bits 6-10, wss layout:
     *
     * 0-5: 0's
     * 6-10: cgms-a
     * 11-13: 0's
     * 14-19: crc
     */
    DcTvoGetWssAspectRatio( tvo, wss_data, &len );
    index = wss_data[2].data & 0x3;
    wss = ((NvU32)cgmsa) << 6 | ((NvU32)s_crc[index][cgmsa] << 14) |
        (wss_data[2].data & 0x3);

    wss_data[0].data = (NvU8)((wss >> 16) & 0xf);
    wss_data[1].data = (NvU8)((wss >> 8) & 0xff);
    wss_data[2].data = (NvU8)(wss & 0xff);
    wss_data[3].data = (NvU8)((wss >> 16) & 0xf);
    wss_data[4].data = (NvU8)((wss >> 8) & 0xff);
    wss_data[5].data = (NvU8)(wss & 0xff);

    enc = wss_data;
    len = NV_ARRAY_SIZE( wss_data );
    DcTvo2ProgramEncoder( tvo, enc, len );

    cont->bCgmsa = NV_TRUE;
}

void
DcTvo2MacrovisionSetCGMSA_20( NvU32 controller, NvU8 cgmsa )
{
    DcTvo2 *tvo;

    tvo = &s_Tvo;

    if ( tvo->Type == NvDdkDispTvType_Ntsc )
        DcTvo2MacrovisionNtscSetCGMSA_20(controller, cgmsa);
    else if ( tvo->Type == NvDdkDispTvType_Pal )
        DcTvo2MacrovisionPalSetCGMSA_20(controller, cgmsa);
}

/* Waits for the Closed Captioning data to be transmitted.
 */
static void
DcTvo2WaitForCC( DcTvo2 *tvo )
{
    NvU32 val;
    NvU32 status;

    for( ;; )
    {
        val = NV_DRF_NUM( TVO, ENCODER_REGISTER, REGINDEX,
                NV_PTVO_INDIR_CC_MISC1 )
            | NV_DRF_NUM( TVO, ENCODER_REGISTER, REG_RWN, 1 );
        TVO2_WRITE( tvo, TVO_ENCODER_REGISTER_0, val );

        val = TVO2_READ( tvo, TVO_ENCODER_REGISTER_0 );
        status = NV_DRF_VAL( TVO, ENCODER_REGISTER, REGDATA, val );
        if( (status & 0x2) == 0 ) // field 1 status is bit 1
        {
            break;
        }
    }
}

void
DcTvo2MacrovisionSetCGMSA_21( NvU32 controller, NvU8 cgmsa )
{
    /* magic values */
    TvoEncoderData cc_data[] =
        {
            { 0x6E, 0x0},
            { 0x6F, 0xe3 },
            { 0x70, 0x2 },
            { 0x71, 0x08 },
            { 0x72, 0x14 },
            { 0x73, 0x14 },
            { 0x75, 0x0 },
            { 0x76, 0x0 },
            { 0x77, 0x0 },
            { 0x78, 0x0 },
        };

    /* Line 21 bit order is backwards:
     *
     * cgms-a b1, b0:
     *   0,0 - copy is permitted
     *   0,1 - reserved
     *   1,0 - one generation
     *   1,1 - no copy
     *
     * aps b1, b0:
     *   0,0 - none
     *   0,1 - psp on, split burst off
     *   1,0 - psp on, 2 line burst
     *   1,1 - psp on, 4 line burst
     *
     * Character A is defined as:
     * { P1, 1, 0, cgmsa-a b1, cgmsa-a b0, aps b1, aps b0, asb }
     *
     * P1 - odd parity bit
     */
    NvU8 char_a[] =
        {
            64, 193, 194, 67, 196, 69, 70, 199, 200, 73, 74, 203, 74, 205,
            206, 79, 208, 81, 82, 211, 84, 213, 214, 87, 88, 217, 218, 91,
            220, 93, 94, 223
        };
    /* check sum over all 3 packets (see below) */
    NvU8 char_c[] =
        {
            104, 103, 230, 229, 100, 227, 98, 97, 224, 223, 94, 93, 220, 91,
            218, 217, 88, 87, 214, 213, 84, 211, 82, 81, 208, 79, 206, 205,
            76, 203, 74, 73
        };

    DcController *cont;
    DcTvo2 *tvo;
    TvoEncoderData *enc;
    TvoEncoderData data[2] = {{0,0},{0,0}};
    NvU32 len;
    NvU32 idx;

    tvo = &s_Tvo;

    DC_CONTROLLER( controller, cont, return );

    if( tvo->Type != NvDdkDispTvType_Ntsc )
    {
        return;
    }

    if( !tvo->bCCinit )
    {
        enc = cc_data;
        len = NV_ARRAY_SIZE( cc_data );
        DcTvo2ProgramEncoder( tvo, enc, len );

        tvo->bCCinit = NV_TRUE;
        cont->bCgmsa = NV_TRUE;
    }

    /* NOTE: the bit order of cgms-a is backward in line 21 vs line 20:
     * reverse the bits -- use as an index to look up the CC packets.
     */
    idx = ((cgmsa >> 4) & 1)
        | (((cgmsa >> 3) & 1) << 1)
        | (((cgmsa >> 2) & 1) << 2)
        | (((cgmsa >> 1) & 1) << 3)
        | ((cgmsa & 1) << 4);

    /* send 3 packets, each packet is 2 characters:
     * packet 1: { 0x1, 0x8 } -- start, cgms-a data
     * packet 2: { char-a, char-b } -- char-b is always 64
     * packet 3: { 0x8f, char-c }
     */

    data[0].addr = 0x6D;
    // bit 1: enable field-1 for CC action
    // bit 2: enable data-repeat
    data[0].data = (1 << 1) | (1 << 2);
    DcTvo2ProgramEncoder( tvo, data, 1 );

    /* first packet */
    data[0].addr = 0x77;
    data[0].data = 0x1;
    data[1].addr = 0x78;
    data[1].data = 0x8;
    DcTvo2ProgramEncoder( tvo, data, 2 );
    DcTvo2WaitForCC( tvo );

    /* second packet */
    data[0].addr = 0x77;
    data[0].data = char_a[idx];
    data[1].addr = 0x78;
    data[1].data = 0x40; // char_b
    DcTvo2ProgramEncoder( tvo, data, 2 );
    DcTvo2WaitForCC( tvo );

    /* third packet */
    data[0].addr = 0x77;
    data[0].data = 0x8f;
    data[1].addr = 0x78;
    data[1].data = char_c[idx];
    DcTvo2ProgramEncoder( tvo, data, 2 );
    DcTvo2WaitForCC( tvo );

    /* null packet */
    data[0].addr = 0x77;
    data[0].data = 0x0;
    data[1].addr = 0x78;
    data[1].data = 0x80;
    DcTvo2ProgramEncoder( tvo, data, 2 );
    DcTvo2WaitForCC( tvo );

    /* disable data repeat*/
    data[0].addr = 0x6D;
    // bit 1: enable field-1 for CC action and null packets
    data[0].data = (1 << 1);
    DcTvo2ProgramEncoder( tvo, data, 1 );
}
