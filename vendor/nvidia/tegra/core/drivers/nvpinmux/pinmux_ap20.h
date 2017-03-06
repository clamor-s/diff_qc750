/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef PINMUX_AP20_H
#define PINMUX_AP20_H

#include "ap20/arapb_misc.h"

#if defined(__cplusplus)
extern "C"
{
#endif

enum NvPinDrivePingroup {
    TEGRA_DRIVE_PINGROUP_AO1   = 0x868,
    TEGRA_DRIVE_PINGROUP_AT1   = 0x870,
    TEGRA_DRIVE_PINGROUP_DBG   = 0x894,
    TEGRA_DRIVE_PINGROUP_VI1   = 0x8bc,
    TEGRA_DRIVE_PINGROUP_VI2   = 0x8c0,
    TEGRA_DRIVE_PINGROUP_SDIO1 = 0x8e0,
    TEGRA_DRIVE_PINGROUP_DDC   = 0x8f0,
    TEGRA_DRIVE_PINGROUP_DUMMY = -1,
};

#define DEFAULT_DRIVE(_name)                            \
{                                                       \
    .PingroupDriveReg = TEGRA_DRIVE_PINGROUP_##_name,   \
    .Value = (((TEGRA_HSM_DISABLE) & 1UL) << 2)      |  \
            (((TEGRA_SCHMITT_ENABLE) & 1UL)  << 3)   |  \
            (((TEGRA_DRIVE_DIV_1) & 3UL) << 4)       |  \
            (((TEGRA_PULL_31) & 31UL) << 12)         |  \
            (((TEGRA_PULL_31) & 31UL) << 20)         |  \
            (((TEGRA_SLEW_SLOWEST) & 3UL) << 28)     |  \
            (((TEGRA_SLEW_SLOWEST) & 3UL) << 30)        \
}

#define SET_DRIVE(_name, _hsm, _schmitt, _drive, _pulldn_drive, \
        _pullup_drive, _pulldn_slew, _pullup_slew)              \
{                                                               \
    .PingroupDriveReg = TEGRA_DRIVE_PINGROUP_##_name,           \
    .Value = (((TEGRA_HSM_##_hsm) & 1UL) << 2)            |     \
            (((TEGRA_SCHMITT_##_schmitt) & 1UL)  << 3)    |     \
            (((TEGRA_DRIVE_##_drive) & 3UL) << 4)         |     \
            (((TEGRA_PULL_##_pulldn_drive) & 31UL) << 12) |     \
            (((TEGRA_PULL_##_pullup_drive) & 31UL) << 20) |     \
            (((TEGRA_SLEW_##_pulldn_slew) & 3UL) << 28)   |     \
            (((TEGRA_SLEW_##_pullup_slew) & 3UL) << 30)         \
}


#define  MUX_TRISTATE_CONFIG(_Pingroup, _Pinmux, _Tristate, _PinmuxReg, _TristateReg)     \
{                                                                                         \
    .Pinmux        =  APB_MISC_PP_PIN_MUX_##_PinmuxReg##_0_##_Pingroup##_SEL_##_Pinmux,   \
    .Pupd          =  0,                                                                  \
    .Tristate      =  APB_MISC_PP_TRISTATE_##_TristateReg##_0_Z_##_Pingroup##_##_Tristate,\
    .MuxReg        =  APB_MISC_PP_PIN_MUX_##_PinmuxReg##_0,                               \
    .PupdReg       =  0,                                                                  \
    .TristateReg   =  APB_MISC_PP_TRISTATE_##_TristateReg##_0,                            \
    .MuxShift      =  APB_MISC_PP_PIN_MUX_##_PinmuxReg##_0_##_Pingroup##_SEL_SHIFT,       \
    .PupdShift     =  0,                                                                  \
    .TristateShift =  APB_MISC_PP_TRISTATE_##_TristateReg##_0_Z_##_Pingroup##_SHIFT       \
}

#define  DEFAULT_CONFIG(_Pingroup, _Pinmux, _Pupd, _Tristate, _PinmuxReg, _PupdReg, _TristateReg) \
{                                                                                         \
    .Pinmux   =  APB_MISC_PP_PIN_MUX_##_PinmuxReg##_0_##_Pingroup##_SEL_##_Pinmux,        \
    .Pupd     =  APB_MISC_PP_PULLUPDOWN_##_PupdReg##_0_##_Pingroup##_PU_PD_##_Pupd,       \
    .Tristate =  APB_MISC_PP_TRISTATE_##_TristateReg##_0_Z_##_Pingroup##_##_Tristate,     \
    .MuxReg   =  APB_MISC_PP_PIN_MUX_##_PinmuxReg##_0,                                    \
    .PupdReg  =  APB_MISC_PP_PULLUPDOWN_##_PupdReg##_0,                                   \
    .TristateReg =  APB_MISC_PP_TRISTATE_##_TristateReg##_0,                              \
    .MuxShift    =  APB_MISC_PP_PIN_MUX_##_PinmuxReg##_0_##_Pingroup##_SEL_SHIFT,         \
    .PupdShift   =  APB_MISC_PP_PULLUPDOWN_##_PupdReg##_0_##_Pingroup##_PU_PD_SHIFT,      \
    .TristateShift =  APB_MISC_PP_TRISTATE_##_TristateReg##_0_Z_##_Pingroup##_SHIFT       \
}

typedef struct NvPingroupConfigRec
{
    NvU8  PadGroup;
    NvU8  Pinmux;
    NvU8  Pupd;
    NvU8  Tristate;
    NvU32 MuxReg;
    NvU32 MuxShift;
    NvU32 PupdReg;
    NvU32 PupdShift;
    NvU32 TristateReg;
    NvU32 TristateShift;
}NvPingroupConfig;

NvError NvPinmuxConfigTable(NvPingroupConfig *config, NvU32 len);

#if defined(__cplusplus)
}
#endif

#endif
