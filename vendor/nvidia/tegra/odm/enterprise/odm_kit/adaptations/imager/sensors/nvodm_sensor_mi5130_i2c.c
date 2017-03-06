/*
 * Copyright (c) 2011 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

///
// When brute forcing the i2c settings for initialization,
// pll setting and resolutions, use these tables.
// We support full resolution and quarter resolution, and
// Full speed (A02 or better APX2500) or clock limited speed.

static DevCtrlReg16 InitSequence_2592x1944[] = {
    {0x0100, 0x0001}, // mode_select =
    {0x0104, 0x0000}, // grouped parameter hold off
    {0x0202, NVODM_SENSOR_DEFAULT_EXPOS_COARSE}, // coarse_int time default
    {0x301A, 0x10CC}, // reset register
    {0x3040, 0x0024}, // read mode 0x6C
    {NVODM_SENSOR_REG_ANALOG_GAIN, NVODM_SENSOR_MIN_GAIN},
    {0x3088, 0x6FFB}, // undocumented register (addendum)
    {0x308E, 0x2020}, // undocumented - fixes column fixed pattern noise
    {0x309E, 0x4400}, // undocumented - column fixed pattern noise: Errata1
    {0x309A, 0xA500}, // undocumented register (micron tools?)
    {0x30D4, 0x9080}, // undocumented register (addendum)
    {0x30EA, 0x3F06}, // undocumented register (addendum)
    {0x3154, 0x1482}, // undocumented register (addendum)
    {0x3156, 0x1C81}, // undocumented register (micron tools?)
    {0x3158, 0x97C7}, // undocumented register (addendum)
    {0x315A, 0x97C6}, // undocumented register (addendum)
    {0x3162, 0x074C}, // undocumented register (addendum)
    {0x3164, 0x0756}, // undocumented register (addendum)
    {0x3166, 0x0760}, // undocumented register (addendum)
    {0x316E, 0x8488}, // undocumented register (addendum)
    {0x3172, 0x0003}, // undocumented register (addendum)
    {0x3016, NVODM_SENSOR_ROWSPEED}, // row speed
    {0x0304, NVODM_SENSOR_PLL_PRE_DIV}, // pre_pll_clk_div (default 2)
    {0x0306, NVODM_SENSOR_PLL_MULT}, // pll_multiplier (default 50)
    {0x0300, NVODM_SENSOR_PLL_VT_PIX_DIV}, // vt_pix_clk_div (default 5)
    {0x0302, NVODM_SENSOR_PLL_VT_SYS_DIV}, // vt_sys_clk_div (default 1)
    {0x0308, NVODM_SENSOR_PLL_OP_PIX_DIV}, // op_pix_clk_div (default 10)
    {0x030A, NVODM_SENSOR_PLL_OP_SYS_DIV}, // op_sys_clk_div (default 1)
    {0x3162, 0x074C}, // global reset end

    {0xFFFE, 100}, // sleep  for pll to finish

    {NVODM_SENSOR_REG_EXPOSURE_COARSE, 0x02fa},

    {NVODM_SENSOR_REG_GROUP_PARAM_HOLD, 0x0000},
    {NVODM_SENSOR_REG_GROUP_PARAM_HOLD, 0x0001},
    {NVODM_SENSOR_REG_FRAME_LENGTH_LINES, 0x0808},
    {NVODM_SENSOR_REG_LINE_LENGTH_PCK, 0x14fc},
    {NVODM_SENSOR_REG_X_ODD_INC, 0x0001},
    {NVODM_SENSOR_REG_Y_ODD_INC, 0x0001},

    {NVODM_SENSOR_REG_X_OUTPUT_SIZE, 0x0a20},
    {NVODM_SENSOR_REG_Y_OUTPUT_SIZE, 0x07a8},
    {NVODM_SENSOR_REG_X_ADDR_START, 0x0008},
    {NVODM_SENSOR_REG_Y_ADDR_START, 0x0008},
    {NVODM_SENSOR_REG_X_ADDR_END, 0x0a27},
    {NVODM_SENSOR_REG_Y_ADDR_END, 0x079f},
    {NVODM_SENSOR_REG_FRAME_LENGTH_LINES, 0x0808},
    {NVODM_SENSOR_REG_GROUP_PARAM_HOLD, 0x0000},

    {0xFFFE, 100}, // sleep 100ms

    /* End-of-Sequence marker */
    {0xFFFF, 0x0000}
};


static DevCtrlReg16 InitSequence_1296x968[] = {
    {0x0100, 0x0001}, // mode_select =
    {0x0104, 0x0000}, // grouped parameter hold off
    {0x0202, NVODM_SENSOR_DEFAULT_EXPOS_COARSE}, // coarse_int time default
    {0x301A, 0x10CC}, // reset register
    {0x3040, 0x0024}, // read mode 0x6C
    {NVODM_SENSOR_REG_ANALOG_GAIN, NVODM_SENSOR_MIN_GAIN},
    {0x3088, 0x6FFB}, // undocumented register (addendum)
    {0x308E, 0x2020}, // undocumented - fixes column fixed pattern noise
    {0x309E, 0x4400}, // undocumented - column fixed pattern noise: Errata1
    {0x309A, 0xA500}, // undocumented register (micron tools?)
    {0x30D4, 0x9080}, // undocumented register (addendum)
    {0x30EA, 0x3F06}, // undocumented register (addendum)
    {0x3154, 0x1482}, // undocumented register (addendum)
    {0x3156, 0x1C81}, // undocumented register (micron tools?)
    {0x3158, 0x97C7}, // undocumented register (addendum)
    {0x315A, 0x97C6}, // undocumented register (addendum)
    {0x3162, 0x074C}, // undocumented register (addendum)
    {0x3164, 0x0756}, // undocumented register (addendum)
    {0x3166, 0x0760}, // undocumented register (addendum)
    {0x316E, 0x8488}, // undocumented register (addendum)
    {0x3172, 0x0003}, // undocumented register (addendum)
    {0x3016, NVODM_SENSOR_ROWSPEED}, // row speed
    {0x0304, NVODM_SENSOR_PLL_PRE_DIV}, // pre_pll_clk_div (default 2)
    {0x0306, NVODM_SENSOR_PLL_MULT}, // pll_multiplier (default 50)
    {0x0300, NVODM_SENSOR_PLL_VT_PIX_DIV}, // vt_pix_clk_div (default 5)
    {0x0302, NVODM_SENSOR_PLL_VT_SYS_DIV}, // vt_sys_clk_div (default 1)
    {0x0308, NVODM_SENSOR_PLL_OP_PIX_DIV}, // op_pix_clk_div (default 10)
    {0x030A, NVODM_SENSOR_PLL_OP_SYS_DIV}, // op_sys_clk_div (default 1)
    {0x3162, 0x074C}, // global reset end

    {0xFFFE, 100}, // sleep 100 ms

    {NVODM_SENSOR_REG_EXPOSURE_COARSE, 0x2fa},
    {NVODM_SENSOR_REG_GROUP_PARAM_HOLD, 0x0000},
    {NVODM_SENSOR_REG_GROUP_PARAM_HOLD, 0x0001},
    {NVODM_SENSOR_REG_FRAME_LENGTH_LINES, 1067},
    {NVODM_SENSOR_REG_LINE_LENGTH_PCK, 3770},
    {NVODM_SENSOR_REG_X_ODD_INC, 0x0003},
    {NVODM_SENSOR_REG_Y_ODD_INC, 0x0003},

    {NVODM_SENSOR_REG_X_OUTPUT_SIZE, 1296},
    {NVODM_SENSOR_REG_Y_OUTPUT_SIZE, 976},
    {NVODM_SENSOR_REG_X_ADDR_START, 8},
    {NVODM_SENSOR_REG_Y_ADDR_START, 4},
    {NVODM_SENSOR_REG_X_ADDR_END, 2597},
    {NVODM_SENSOR_REG_Y_ADDR_END, 1953},
    {NVODM_SENSOR_REG_FRAME_LENGTH_LINES, 1067},
    {NVODM_SENSOR_REG_GROUP_PARAM_HOLD, 0x0000},

    {0xFFFE, 100}, // sleep 100 ms
    /* End-of-Sequence marker */
    {0xFFFF, 0x0000}
};


///
// Fastest possible assumes 24MHz input clock and generates 64MHz output clock
///
static DevCtrlReg16 InitSequence_2592x1944_fast[] = {
    {0x0100, 0x0001}, // mode_select =
    {0x0104, 0x0000}, // grouped parameter hold off
    {0x0202, NVODM_SENSOR_DEFAULT_EXPOS_COARSE}, // coarse_int time default
    {0x301A, 0x10CC}, // reset register
    {0x3040, 0x0024}, // read mode 0x6C
    {NVODM_SENSOR_REG_ANALOG_GAIN, NVODM_SENSOR_MIN_GAIN},
    {0x3088, 0x6FFB}, // undocumented register (addendum)
    {0x308E, 0x2020}, // undocumented - fixes column fixed pattern noise
    {0x309E, 0x4400}, // undocumented - column fixed pattern noise: Errata1
    {0x309A, 0xA500}, // undocumented register (micron tools?)
    {0x30D4, 0x9080}, // undocumented register (addendum)
    {0x30EA, 0x3F06}, // undocumented register (addendum)
    {0x3154, 0x1482}, // undocumented register (addendum)
    {0x3156, 0x1C81}, // undocumented register (micron tools?)
    {0x3158, 0x97C7}, // undocumented register (addendum)
    {0x315A, 0x97C6}, // undocumented register (addendum)
    {0x3162, 0x074C}, // undocumented register (addendum)
    {0x3164, 0x0756}, // undocumented register (addendum)
    {0x3166, 0x0760}, // undocumented register (addendum)
    {0x316E, 0x8488}, // undocumented register (addendum)
    {0x3172, 0x0003}, // undocumented register (addendum)
    {0x3016, NVODM_SENSOR_ROWSPEED}, // row speed
    {0x0304, NVODM_SENSOR_PLL_FAST_PRE_DIV}, // pre_pll_clk_div (default 2)
    {0x0306, NVODM_SENSOR_PLL_FAST_MULT}, // pll_multiplier (default 50)
    {0x0300, NVODM_SENSOR_PLL_FAST_VT_PIX_DIV}, // vt_pix_clk_div (default 5)
    {0x0302, NVODM_SENSOR_PLL_FAST_VT_SYS_DIV}, // vt_sys_clk_div (default 1)
    {0x0308, NVODM_SENSOR_PLL_FAST_OP_PIX_DIV}, // op_pix_clk_div (default 10)
    {0x030A, NVODM_SENSOR_PLL_FAST_OP_SYS_DIV}, // op_sys_clk_div (default 1)
    {0x3162, 0x074C}, // global reset end

    {0xFFFE, 100}, // sleep  for pll to finish

    {NVODM_SENSOR_REG_EXPOSURE_COARSE, 0x02fa},

    {NVODM_SENSOR_REG_GROUP_PARAM_HOLD, 0x0000},
    {NVODM_SENSOR_REG_GROUP_PARAM_HOLD, 0x0001},
    {NVODM_SENSOR_REG_FRAME_LENGTH_LINES, 0x0808},
    {NVODM_SENSOR_REG_LINE_LENGTH_PCK, 0x14fc},
    {NVODM_SENSOR_REG_X_ODD_INC, 0x0001},
    {NVODM_SENSOR_REG_Y_ODD_INC, 0x0001},

    {NVODM_SENSOR_REG_X_OUTPUT_SIZE, 0x0a20},
    {NVODM_SENSOR_REG_Y_OUTPUT_SIZE, 0x07a8},
    {NVODM_SENSOR_REG_X_ADDR_START, 0x0008},
    {NVODM_SENSOR_REG_Y_ADDR_START, 0x0008},
    {NVODM_SENSOR_REG_X_ADDR_END, 0x0a27},
    {NVODM_SENSOR_REG_Y_ADDR_END, 0x079f},
    {NVODM_SENSOR_REG_FRAME_LENGTH_LINES, 0x0808},
    {NVODM_SENSOR_REG_GROUP_PARAM_HOLD, 0x0000},

    {0xFFFE, 100}, // sleep 100ms

    /* End-of-Sequence marker */
    {0xFFFF, 0x0000}
};


static DevCtrlReg16 InitSequence_1296x968_fast[] = {
    {0x0100, 0x0001}, // mode_select =
    {0x0104, 0x0000}, // grouped parameter hold off
    {0x0202, NVODM_SENSOR_DEFAULT_EXPOS_COARSE}, // coarse_int time default
    {0x301A, 0x10CC}, // reset register
    {0x3040, 0x0024}, // read mode 0x6C
    {NVODM_SENSOR_REG_ANALOG_GAIN, NVODM_SENSOR_MIN_GAIN},
    {0x3088, 0x6FFB}, // undocumented register (addendum)
    {0x308E, 0x2020}, // undocumented - fixes column fixed pattern noise
    {0x309E, 0x4400}, // undocumented - column fixed pattern noise: Errata1
    {0x309A, 0xA500}, // undocumented register (micron tools?)
    {0x30D4, 0x9080}, // undocumented register (addendum)
    {0x30EA, 0x3F06}, // undocumented register (addendum)
    {0x3154, 0x1482}, // undocumented register (addendum)
    {0x3156, 0x1C81}, // undocumented register (micron tools?)
    {0x3158, 0x97C7}, // undocumented register (addendum)
    {0x315A, 0x97C6}, // undocumented register (addendum)
    {0x3162, 0x074C}, // undocumented register (addendum)
    {0x3164, 0x0756}, // undocumented register (addendum)
    {0x3166, 0x0760}, // undocumented register (addendum)
    {0x316E, 0x8488}, // undocumented register (addendum)
    {0x3172, 0x0003}, // undocumented register (addendum)
    {0x3016, NVODM_SENSOR_ROWSPEED}, // row speed
    {0x0304, NVODM_SENSOR_PLL_FAST_PRE_DIV}, // pre_pll_clk_div (default 2)
    {0x0306, NVODM_SENSOR_PLL_FAST_MULT}, // pll_multiplier (default 50)
    {0x0300, NVODM_SENSOR_PLL_FAST_VT_PIX_DIV}, // vt_pix_clk_div (default 5)
    {0x0302, NVODM_SENSOR_PLL_FAST_VT_SYS_DIV}, // vt_sys_clk_div (default 1)
    {0x0308, NVODM_SENSOR_PLL_FAST_OP_PIX_DIV}, // op_pix_clk_div (default 10)
    {0x030A, NVODM_SENSOR_PLL_FAST_OP_SYS_DIV}, // op_sys_clk_div (default 1)
    {0x3162, 0x074C}, // global reset end

    {0xFFFE, 100}, // sleep 100 ms

    {NVODM_SENSOR_REG_EXPOSURE_COARSE, 0x2fa},
    {NVODM_SENSOR_REG_GROUP_PARAM_HOLD, 0x0000},
    {NVODM_SENSOR_REG_GROUP_PARAM_HOLD, 0x0001},
    {NVODM_SENSOR_REG_FRAME_LENGTH_LINES, 1067},
    {NVODM_SENSOR_REG_LINE_LENGTH_PCK, 3770},
    {NVODM_SENSOR_REG_X_ODD_INC, 0x0003},
    {NVODM_SENSOR_REG_Y_ODD_INC, 0x0003},

    {NVODM_SENSOR_REG_X_OUTPUT_SIZE, 1296},
    {NVODM_SENSOR_REG_Y_OUTPUT_SIZE, 976},
    {NVODM_SENSOR_REG_X_ADDR_START, 8},
    {NVODM_SENSOR_REG_Y_ADDR_START, 4},
    {NVODM_SENSOR_REG_X_ADDR_END, 2597},
    {NVODM_SENSOR_REG_Y_ADDR_END, 1953},
    {NVODM_SENSOR_REG_FRAME_LENGTH_LINES, 1067},
    {NVODM_SENSOR_REG_GROUP_PARAM_HOLD, 0x0000},

    {0xFFFE, 100}, // sleep 100 ms
    /* End-of-Sequence marker */
    {0xFFFF, 0x0000}
};


