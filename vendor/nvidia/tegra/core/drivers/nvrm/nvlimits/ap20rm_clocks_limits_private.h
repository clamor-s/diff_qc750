/*
 * Copyright (c) 2009 - 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef INCLUDED_AP20RM_CLOCKS_LIMITS_PRIVATE_H
#define INCLUDED_AP20RM_CLOCKS_LIMITS_PRIVATE_H

#include "nvrm_clocks_limits_private.h"
#include "ap20/project_relocation_table.h"

#ifdef __cplusplus
extern "C"
{
#endif  /* __cplusplus */

// TODO: AP20 process corners and mapping to fuses

// Number of characterized AP20 process corners
#define NV_AP20_PROCESS_CORNERS (4)

// AP20 RAM timing controls low voltage threshold and setting
#define NV_AP20_SVOP_LOW_VOLTAGE (1010)
#define NV_AP20_SVOP_LOW_SETTING \
        (NV_DRF_NUM(APB_MISC, GP_ASDBGREG, CFG2TMC_RAM_SVOP_DP,  0) | \
         NV_DRF_NUM(APB_MISC, GP_ASDBGREG, CFG2TMC_RAM_SVOP_PDP, 0) | \
         NV_DRF_NUM(APB_MISC, GP_ASDBGREG, CFG2TMC_RAM_SVOP_REG, 3) | \
         NV_DRF_NUM(APB_MISC, GP_ASDBGREG, CFG2TMC_RAM_SVOP_SP,  0))

#define NV_AP20_SVOP_HIGH_SETTING \
        (NV_DRF_NUM(APB_MISC, GP_ASDBGREG, CFG2TMC_RAM_SVOP_DP,  0) | \
         NV_DRF_NUM(APB_MISC, GP_ASDBGREG, CFG2TMC_RAM_SVOP_PDP, 0) | \
         NV_DRF_NUM(APB_MISC, GP_ASDBGREG, CFG2TMC_RAM_SVOP_REG, 0) | \
         NV_DRF_NUM(APB_MISC, GP_ASDBGREG, CFG2TMC_RAM_SVOP_SP,  0))

/**
 * The tables below provide clock limits for AP20 modules for different core
 * voltages for each process corner. Note that EMC and MC are not included,
 * as memory scaling has board dependencies, and EMC scaling configurations
 * are retrieved by ODM query NvOdmQuerySdramControllerConfigGet(). The ODM
 * data is adjuasted based on process corner using EMC DQSIB offsets.
 */

#define NV_AP20SS_SHMOO_VOLTAGES           950,   1000,    1100,    1200,    1225,    1275,    1300
#define NV_AP20SS_SCALED_CLK_LIMITS \
    { NV_DEVID_I2S      , 0,       1,  { 26000,  26000,   26000,   26000,   26000,   26000,   26000} }, \
    { NV_DEVID_SPDIF    , 0,       1,  {100000, 100000,  100000,  100000,  100000,  100000,  100000} }, \
    { NV_DEVID_I2C      , 0,       1,  { 26000,  26000,   26000,   26000,   26000,   26000,   26000} }, \
    { NV_DEVID_DVC      , 0,       1,  { 26000,  26000,   26000,   26000,   26000,   26000,   26000} }, \
    { NV_DEVID_OWR      , 0,       1,  { 26000,  26000,   26000,   26000,   26000,   26000,   26000} }, \
    { NV_DEVID_PWFM     , 0,       1,  {216000, 216000,  216000,  216000,  216000,  216000,  216000} }, \
    { NV_DEVID_XIO      , 0,       1,  {150000, 150000,  150000,  150000,  150000,  150000,  150000} }, \
    { NV_DEVID_TWC      , 0,       1,  {150000, 150000,  150000,  150000,  150000,  150000,  150000} }, \
    { NV_DEVID_HSMMC    , 0,       1,  { 44000,  52000,   52000,   52000,   52000,   52000,   52000} }, \
    { NV_DEVID_VFIR     , 0,       1,  { 72000,  72000,   72000,   72000,   72000,   72000,   72000} }, \
    { NV_DEVID_UART     , 0,       1,  { 72000,  72000,   72000,   72000,   72000,   72000,   72000} }, \
    { NV_DEVID_NANDCTRL , 0,       1,  {130000, 150000,  158000,  164000,  164000,  164000,  164000} }, \
    { NV_DEVID_SNOR     , 0,       1,  {     1,  80000,   92000,   92000,   92000,   92000,   92000} }, \
    { NV_DEVID_EIDE     , 0,       1,  {     1,      1,  100000,  100000,  100000,  100000,  100000} }, \
    { NV_DEVID_MIPI_HS  , 0,       1,  {     1,  40000,   40000,   40000,   40000,   60000,   60000} }, \
    { NV_DEVID_SDIO     , 0,       1,  { 44000,  52000,   52000,   52000,   52000,   52000,   52000} }, \
    { NV_DEVID_SPI      , 0,       1,  { 40000,  40000,   40000,   40000,   40000,   40000,   40000} }, \
    { NV_DEVID_SLINK    , 0,       1,  {160000, 160000,  160000,  160000,  160000,  160000,  160000} }, \
    { NV_DEVID_USB      , 0,       1,  {     1,      1,       1,  480000,  480000,  480000,  480000} }, \
    { NV_DEVID_PCIE     , 0,       1,  {     1,      1,       1,  250000,  250000,  250000,  250000} }, \
    { NV_DEVID_HOST1X   , 0,       1,  {104500, 133000,  166000,  166000,  166000,  166000,  166000} }, \
    { NV_DEVID_EPP      , 0,       1,  {133000, 171000,  247000,  300000,  300000,  300000,  300000} }, \
    { NV_DEVID_GR2D     , 0,       1,  {133000, 171000,  247000,  300000,  300000,  300000,  300000} }, \
    { NV_DEVID_GR3D     , 0,       1,  {114000, 161500,  247000,  304000,  304000,  333500,  333500} }, \
    { NV_DEVID_MPE      , 0,       1,  {104500, 152000,  228000,  300000,  300000,  300000,  300000} }, \
    { NV_DEVID_VI       , 0,       1,  { 85000, 100000,  150000,  150000,  150000,  150000,  150000} }, \
    { NV_DEVID_DISPLAY  , 0,       1,  {158000, 158000,  190000,  190000,  190000,  190000,  190000} }, \
    { NV_DEVID_DSI      , 0,       1,  {200000, 200000,  200000,  500000,  500000,  500000,  500000} }, \
    { NV_DEVID_TVO      , 0,       1,  {     1,      1,       1,  250000,  250000,  250000,  250000} }, \
    { NV_DEVID_HDMI     , 0,       1,  {     1,      1,       1,  148500,  148500,  148500,  148500} }, \
    { NV_DEVID_AVP,0, NVRM_BUS_MIN_KHZ,{ 95000, 133000,  190000,  222500,  240000,  247000,  262000} }, \
    { NV_DEVID_VDE      , 0,       1,  { 95000, 123500,  209000,  275500,  275500,  300000,  300000} }, \
    { NVRM_DEVID_CLK_SRC, 0,       1,  {480000, 600000,  800000, 1067000, 1067000, 1067000, 1067000} }

#define NV_AP20ST_SHMOO_VOLTAGES           950,   1000,    1100,    1200,    1225,    1275,    1300
#define NV_AP20ST_SCALED_CLK_LIMITS \
    { NV_DEVID_I2S      , 0,       1,  { 26000,  26000,   26000,   26000,   26000,   26000,   26000} }, \
    { NV_DEVID_SPDIF    , 0,       1,  {100000, 100000,  100000,  100000,  100000,  100000,  100000} }, \
    { NV_DEVID_I2C      , 0,       1,  { 26000,  26000,   26000,   26000,   26000,   26000,   26000} }, \
    { NV_DEVID_DVC      , 0,       1,  { 26000,  26000,   26000,   26000,   26000,   26000,   26000} }, \
    { NV_DEVID_OWR      , 0,       1,  { 26000,  26000,   26000,   26000,   26000,   26000,   26000} }, \
    { NV_DEVID_PWFM     , 0,       1,  {216000, 216000,  216000,  216000,  216000,  216000,  216000} }, \
    { NV_DEVID_XIO      , 0,       1,  {150000, 150000,  150000,  150000,  150000,  150000,  150000} }, \
    { NV_DEVID_TWC      , 0,       1,  {150000, 150000,  150000,  150000,  150000,  150000,  150000} }, \
    { NV_DEVID_HSMMC    , 0,       1,  { 44000,  52000,   52000,   52000,   52000,   52000,   52000} }, \
    { NV_DEVID_VFIR     , 0,       1,  { 72000,  72000,   72000,   72000,   72000,   72000,   72000} }, \
    { NV_DEVID_UART     , 0,       1,  { 72000,  72000,   72000,   72000,   72000,   72000,   72000} }, \
    { NV_DEVID_NANDCTRL , 0,       1,  {130000, 150000,  158000,  164000,  164000,  164000,  164000} }, \
    { NV_DEVID_SNOR     , 0,       1,  {     1,  80000,   92000,   92000,   92000,   92000,   92000} }, \
    { NV_DEVID_EIDE     , 0,       1,  {     1,      1,  100000,  100000,  100000,  100000,  100000} }, \
    { NV_DEVID_MIPI_HS  , 0,       1,  {     1,  40000,   40000,   40000,   40000,   60000,   60000} }, \
    { NV_DEVID_SDIO     , 0,       1,  { 44000,  52000,   52000,   52000,   52000,   52000,   52000} }, \
    { NV_DEVID_SPI      , 0,       1,  { 40000,  40000,   40000,   40000,   40000,   40000,   40000} }, \
    { NV_DEVID_SLINK    , 0,       1,  {160000, 160000,  160000,  160000,  160000,  160000,  160000} }, \
    { NV_DEVID_USB      , 0,       1,  {     1,      1,       1,  480000,  480000,  480000,  480000} }, \
    { NV_DEVID_PCIE     , 0,       1,  {     1,      1,       1,  250000,  250000,  250000,  250000} }, \
    { NV_DEVID_HOST1X   , 0,       1,  {123500, 152000,  166000,  166000,  166000,  166000,  166000} }, \
    { NV_DEVID_EPP      , 0,       1,  {171000, 209000,  300000,  300000,  300000,  300000,  300000} }, \
    { NV_DEVID_GR2D     , 0,       1,  {171000, 209000,  300000,  300000,  300000,  300000,  300000} }, \
    { NV_DEVID_GR3D     , 0,       1,  {161500, 209000,  285000,  333500,  333500,  361000,  361000} }, \
    { NV_DEVID_MPE      , 0,       1,  {142500, 190000,  275500,  300000,  300000,  300000,  300000} }, \
    { NV_DEVID_VI       , 0,       1,  {100000, 100000,  150000,  150000,  150000,  150000,  150000} }, \
    { NV_DEVID_DISPLAY  , 0,       1,  {158000, 158000,  190000,  190000,  190000,  190000,  190000} }, \
    { NV_DEVID_DSI      , 0,       1,  {200000, 200000,  200000,  500000,  500000,  500000,  500000} }, \
    { NV_DEVID_TVO      , 0,       1,  {     1,      1,       1,  250000,  250000,  250000,  250000} }, \
    { NV_DEVID_HDMI     , 0,       1,  {     1,      1,       1,  148500,  148500,  148500,  148500} }, \
    { NV_DEVID_AVP,0, NVRM_BUS_MIN_KHZ,{123500, 159500,  207000,  240000,  240000,  264000,  277500} }, \
    { NV_DEVID_VDE      , 0,       1,  {123500, 152000,  237500,  300000,  300000,  300000,  300000} }, \
    { NVRM_DEVID_CLK_SRC, 0,       1,  {600000, 667000, 1067000, 1067000, 1067000, 1067000, 1067000} }

#define NV_AP20FT_SHMOO_VOLTAGES           950,   1000,    1100,    1200,    1225,    1275,    1300
#define NV_AP20FT_SCALED_CLK_LIMITS \
    { NV_DEVID_I2S      , 0,       1,  { 26000,  26000,   26000,   26000,   26000,   26000,   26000} }, \
    { NV_DEVID_SPDIF    , 0,       1,  {100000, 100000,  100000,  100000,  100000,  100000,  100000} }, \
    { NV_DEVID_I2C      , 0,       1,  { 26000,  26000,   26000,   26000,   26000,   26000,   26000} }, \
    { NV_DEVID_DVC      , 0,       1,  { 26000,  26000,   26000,   26000,   26000,   26000,   26000} }, \
    { NV_DEVID_OWR      , 0,       1,  { 26000,  26000,   26000,   26000,   26000,   26000,   26000} }, \
    { NV_DEVID_PWFM     , 0,       1,  {216000, 216000,  216000,  216000,  216000,  216000,  216000} }, \
    { NV_DEVID_XIO      , 0,       1,  {150000, 150000,  150000,  150000,  150000,  150000,  150000} }, \
    { NV_DEVID_TWC      , 0,       1,  {150000, 150000,  150000,  150000,  150000,  150000,  150000} }, \
    { NV_DEVID_HSMMC    , 0,       1,  { 44000,  52000,   52000,   52000,   52000,   52000,   52000} }, \
    { NV_DEVID_VFIR     , 0,       1,  { 72000,  72000,   72000,   72000,   72000,   72000,   72000} }, \
    { NV_DEVID_UART     , 0,       1,  { 72000,  72000,   72000,   72000,   72000,   72000,   72000} }, \
    { NV_DEVID_NANDCTRL , 0,       1,  {130000, 150000,  158000,  164000,  164000,  164000,  164000} }, \
    { NV_DEVID_SNOR     , 0,       1,  {     1,  80000,   92000,   92000,   92000,   92000,   92000} }, \
    { NV_DEVID_EIDE     , 0,       1,  {     1,      1,  100000,  100000,  100000,  100000,  100000} }, \
    { NV_DEVID_MIPI_HS  , 0,       1,  {     1,  40000,   40000,   40000,   40000,   60000,   60000} }, \
    { NV_DEVID_SDIO     , 0,       1,  { 44000,  52000,   52000,   52000,   52000,   52000,   52000} }, \
    { NV_DEVID_SPI      , 0,       1,  { 40000,  40000,   40000,   40000,   40000,   40000,   40000} }, \
    { NV_DEVID_SLINK    , 0,       1,  {160000, 160000,  160000,  160000,  160000,  160000,  160000} }, \
    { NV_DEVID_USB      , 0,       1,  {     1,      1,       1,  480000,  480000,  480000,  480000} }, \
    { NV_DEVID_PCIE     , 0,       1,  {     1,      1,       1,  250000,  250000,  250000,  250000} }, \
    { NV_DEVID_HOST1X   , 0,       1,  {161500, 166000,  166000,  166000,  166000,  166000,  166000} }, \
    { NV_DEVID_EPP      , 0,       1,  {237500, 275500,  300000,  300000,  300000,  300000,  300000} }, \
    { NV_DEVID_GR2D     , 0,       1,  {237500, 275500,  300000,  300000,  300000,  300000,  300000} }, \
    { NV_DEVID_GR3D     , 0,       1,  {218500, 256500,  323000,  380000,  380000,  400000,  400000} }, \
    { NV_DEVID_MPE      , 0,       1,  {190000, 237500,  300000,  300000,  300000,  300000,  300000} }, \
    { NV_DEVID_VI       , 0,       1,  {100000, 100000,  150000,  150000,  150000,  150000,  150000} }, \
    { NV_DEVID_DISPLAY  , 0,       1,  {158000, 158000,  190000,  190000,  190000,  190000,  190000} }, \
    { NV_DEVID_DSI      , 0,       1,  {200000, 200000,  200000,  500000,  500000,  500000,  500000} }, \
    { NV_DEVID_TVO      , 0,       1,  {     1,      1,       1,  250000,  250000,  250000,  250000} }, \
    { NV_DEVID_HDMI     , 0,       1,  {     1,      1,       1,  148500,  148500,  148500,  148500} }, \
    { NV_DEVID_AVP,0, NVRM_BUS_MIN_KHZ,{152000, 180500,  229500,  260000,  260000,  285000,  300000} }, \
    { NV_DEVID_VDE      , 0,       1,  {152000, 209000,  285000,  300000,  300000,  300000,  300000} }, \
    { NVRM_DEVID_CLK_SRC, 0,       1,  {667000, 800000, 1067000, 1067000, 1067000, 1067000, 1067000} }

#define NV_AP20FF_SHMOO_VOLTAGES           950,   1000,    1100,    1200,    1225,    1275,    1300
#define NV_AP20FF_SCALED_CLK_LIMITS \
    { NV_DEVID_I2S      , 0,       1,  { 26000,  26000,   26000,   26000,   26000,   26000,   26000} }, \
    { NV_DEVID_SPDIF    , 0,       1,  {100000, 100000,  100000,  100000,  100000,  100000,  100000} }, \
    { NV_DEVID_I2C      , 0,       1,  { 26000,  26000,   26000,   26000,   26000,   26000,   26000} }, \
    { NV_DEVID_DVC      , 0,       1,  { 26000,  26000,   26000,   26000,   26000,   26000,   26000} }, \
    { NV_DEVID_OWR      , 0,       1,  { 26000,  26000,   26000,   26000,   26000,   26000,   26000} }, \
    { NV_DEVID_PWFM     , 0,       1,  {216000, 216000,  216000,  216000,  216000,  216000,  216000} }, \
    { NV_DEVID_XIO      , 0,       1,  {150000, 150000,  150000,  150000,  150000,  150000,  150000} }, \
    { NV_DEVID_TWC      , 0,       1,  {150000, 150000,  150000,  150000,  150000,  150000,  150000} }, \
    { NV_DEVID_HSMMC    , 0,       1,  { 44000,  52000,   52000,   52000,   52000,   52000,   52000} }, \
    { NV_DEVID_VFIR     , 0,       1,  { 72000,  72000,   72000,   72000,   72000,   72000,   72000} }, \
    { NV_DEVID_UART     , 0,       1,  { 72000,  72000,   72000,   72000,   72000,   72000,   72000} }, \
    { NV_DEVID_NANDCTRL , 0,       1,  {130000, 150000,  158000,  164000,  164000,  164000,  164000} }, \
    { NV_DEVID_SNOR     , 0,       1,  {     1,  80000,   92000,   92000,   92000,   92000,   92000} }, \
    { NV_DEVID_EIDE     , 0,       1,  {     1,      1,  100000,  100000,  100000,  100000,  100000} }, \
    { NV_DEVID_MIPI_HS  , 0,       1,  {     1,  40000,   40000,   40000,   40000,   60000,   60000} }, \
    { NV_DEVID_SDIO     , 0,       1,  { 44000,  52000,   52000,   52000,   52000,   52000,   52000} }, \
    { NV_DEVID_SPI      , 0,       1,  { 40000,  40000,   40000,   40000,   40000,   40000,   40000} }, \
    { NV_DEVID_SLINK    , 0,       1,  {160000, 160000,  160000,  160000,  160000,  160000,  160000} }, \
    { NV_DEVID_USB      , 0,       1,  {     1,      1,       1,  480000,  480000,  480000,  480000} }, \
    { NV_DEVID_PCIE     , 0,       1,  {     1,      1,       1,  250000,  250000,  250000,  250000} }, \
    { NV_DEVID_HOST1X   , 0,       1,  {166000, 166000,  166000,  166000,  166000,  166000,  166000} }, \
    { NV_DEVID_EPP      , 0,       1,  {266000, 300000,  300000,  300000,  300000,  300000,  300000} }, \
    { NV_DEVID_GR2D     , 0,       1,  {266000, 300000,  300000,  300000,  300000,  300000,  300000} }, \
    { NV_DEVID_GR3D     , 0,       1,  {247000, 285000,  351500,  400000,  400000,  400000,  400000} }, \
    { NV_DEVID_MPE      , 0,       1,  {228000, 266000,  300000,  300000,  300000,  300000,  300000} }, \
    { NV_DEVID_VI       , 0,       1,  {100000, 100000,  150000,  150000,  150000,  150000,  150000} }, \
    { NV_DEVID_DISPLAY  , 0,       1,  {158000, 158000,  190000,  190000,  190000,  190000,  190000} }, \
    { NV_DEVID_DSI      , 0,       1,  {200000, 200000,  200000,  500000,  500000,  500000,  500000} }, \
    { NV_DEVID_TVO      , 0,       1,  {     1,      1,       1,  250000,  250000,  250000,  250000} }, \
    { NV_DEVID_HDMI     , 0,       1,  {     1,      1,       1,  148500,  148500,  148500,  148500} }, \
    { NV_DEVID_AVP,0, NVRM_BUS_MIN_KHZ,{171000, 218500,  256500,  292500,  292500,  300000,  300000} }, \
    { NV_DEVID_VDE      , 0,       1,  {171000, 218500,  300000,  300000,  300000,  300000,  300000} }, \
    { NVRM_DEVID_CLK_SRC, 0,       1,  {800000,1067000, 1067000, 1067000, 1067000, 1067000, 1067000} }

/**
 * The tables below provides clock limits for AP20 CPU for different CPU
 * voltages, for all process corners
 */

// A01 and A02 tables
#define NV_AP20SS_CPU_VOLTAGES               750,    825,    900,     975,    1050,    1100
#define NV_AP20SS_SCALED_CPU_CLK_LIMITS \
    { NV_DEVID_CPU, 0, NVRM_BUS_MIN_KHZ, {314000, 456000, 608000,  760000,  912000, 1000000} }

#define NV_AP20ST_CPU_VOLTAGES               750,    825,    900,     975,    1050,    1100
#define NV_AP20ST_SCALED_CPU_CLK_LIMITS \
    { NV_DEVID_CPU, 0, NVRM_BUS_MIN_KHZ, {314000, 456000, 618000,  770000,  922000, 1007000} }

#define NV_AP20FT_CPU_VOLTAGES               750,    825,    875,     925,     975,    1100
#define NV_AP20FT_SCALED_CPU_CLK_LIMITS \
    { NV_DEVID_CPU, 0, NVRM_BUS_MIN_KHZ, {494000, 675000, 817000,  922000, 1036000, 1226000} }

#define NV_AP20FF_CPU_VOLTAGES               750,    775,    800,     875,     975,    1100
#define NV_AP20FF_SCALED_CPU_CLK_LIMITS \
    { NV_DEVID_CPU, 0, NVRM_BUS_MIN_KHZ, {730000, 760000, 845000, 1000000, 1160000, 1250000} }

// A03+ tables
#define NV_AP20SS_A03CPU_VOLTAGES            750,    800,    850,     900,     950,    1025,    1050,    1125
#define NV_AP20SS_SCALED_A03CPU_CLK_LIMITS \
    { NV_DEVID_CPU, 0, NVRM_BUS_MIN_KHZ, {380000, 503000, 655000,  798000,  902000, 1000000, 1100000, 1200000} }

#define NV_AP20ST_A03CPU_VOLTAGES            750,    800,    850,     875,     950,    1000,    1050,    1100
#define NV_AP20ST_SCALED_A03CPU_CLK_LIMITS \
    { NV_DEVID_CPU, 0, NVRM_BUS_MIN_KHZ, {389000, 503000, 655000,  760000,  950000, 1015000, 1100000, 1216000} }

#define NV_AP20FT_A03CPU_VOLTAGES            750,    800,    850,     900,     950,    1000,    1050,    1100
#define NV_AP25FT_SCALED_A03CPU_CLK_LIMITS \
    { NV_DEVID_CPU, 0, NVRM_BUS_MIN_KHZ, {598000, 680000, 769000,  902000, 1026000, 1140000, 1225000, 1301000} }
#define NV_AP20FT_SCALED_A03CPU_CLK_LIMITS \
    { NV_DEVID_CPU, 0, NVRM_BUS_MIN_KHZ, {598000, 750000, 893000, 1007000, 1092000, 1160000, 1244000, 1301000} }

#define NV_AP20FF_A03CPU_VOLTAGES            750,    775,    800,     850,     875,     950,    1000,    1050
#define NV_AP20FF_SCALED_A03CPU_CLK_LIMITS \
    { NV_DEVID_CPU, 0, NVRM_BUS_MIN_KHZ, {730000, 760000, 845000,  940000, 1000000, 1130000, 1216000, 1310000} }

/**
 * Core and CPU speedo threshollds for all process corners
 */

// Core:                            SS,  ST,  FT, FF
#define NV_AP20_A02_SPEEDO_LIMITS  166, 196, 225, 0xFFFFFFFFUL
#define NV_AP20_A03_SPEEDO_LIMITS  166, 196, 225, 0xFFFFFFFFUL
#define NV_AP25_A03_SPEEDO_LIMITS  166, 196, 225, 0xFFFFFFFFUL

// CPU:                                 SS,  ST,  FT, FF
#define NV_AP20_A02_CPU_SPEEDO_LIMITS  316, 367, 421, 0xFFFFFFFFUL
#define NV_AP20_A03_CPU_SPEEDO_LIMITS  304, 369, 420, 0xFFFFFFFFUL
#define NV_AP25_A03_CPU_SPEEDO_LIMITS  317, 332, 384, 0xFFFFFFFFUL


/**
 * AP20 EMC DQSIB offsets in order of process calibration settings
 * - specify packed digital DLL override equation parameters
 */
#define EMC_DIG_DLL_OVERRIDE_0_LOW_KHZ_MULT_RANGE    7:0
#define EMC_DIG_DLL_OVERRIDE_0_LOW_KHZ_OFFS_RANGE   15:8
#define EMC_DIG_DLL_OVERRIDE_0_HIGH_KHZ_MULT_RANGE  23:16
#define EMC_DIG_DLL_OVERRIDE_0_HIGH_KHZ_OFFS_RANGE  31:24
#define EMC_DIG_DLL_OVERRIDE_PACK(low_mult, low_offs, high_mult, high_offs) \
        (NV_DRF_NUM(EMC, DIG_DLL_OVERRIDE, LOW_KHZ_MULT, (low_mult))   | \
         NV_DRF_NUM(EMC, DIG_DLL_OVERRIDE, LOW_KHZ_OFFS, (low_offs))   | \
         NV_DRF_NUM(EMC, DIG_DLL_OVERRIDE, HIGH_KHZ_MULT, (high_mult)) | \
         NV_DRF_NUM(EMC, DIG_DLL_OVERRIDE, HIGH_KHZ_OFFS, (high_offs)))

#define NV_AP20_DQSIB_OFFSETS \
    EMC_DIG_DLL_OVERRIDE_PACK( 69, 105, 38, 139), /* 0 - SS corner */ \
    EMC_DIG_DLL_OVERRIDE_PACK( 97, 147, 47, 173), /* 1 - ST corner */ \
    EMC_DIG_DLL_OVERRIDE_PACK(110, 174, 53, 196), /* 2 - FT corner */ \
    EMC_DIG_DLL_OVERRIDE_PACK(110, 184, 55, 208), /* 3 - FF corner */

#define NV_AP25_DQSIB_OFFSETS \
    EMC_DIG_DLL_OVERRIDE_PACK( 69, 105, 38, 139), /* 0 - SS corner */ \
    EMC_DIG_DLL_OVERRIDE_PACK( 97, 147, 47, 173), /* 1 - ST corner */ \
    EMC_DIG_DLL_OVERRIDE_PACK(100, 174, 16, 111), /* 2 - FT corner */ \
    EMC_DIG_DLL_OVERRIDE_PACK( 66, 184, 24, 150), /* 3 - FF corner */


/**
 * AP20 OSC Doubler tap delays for each supported frequency in order
 * of process calibration settings
 */
#define NV_AP20_OSC_DOUBLER_CONFIGURATIONS \
/*     Osc kHz  SS  ST  FT  FF   */ \
    {  12000, {  7,  7,  7,  7 } }, \
    {  13000, {  7,  7,  7,  7 } },


/**
 * This table provides maximum limits for selected AP20 modules depended on
 * AP20 SKUs. Table entries are ordered by SKU number.
 */
//    CPU     Avp     Vde     MC      EMC2x    3D      DispA   DispB    mV   Cpu mV   SKU
#define NV_AP20_SKUED_LIMITS \
    {{1000000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1225, 1100 },  0}, /* Dev (AP20 Unrestricted) */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1200, 1000 },  1}, /* Dev (AP20 Reserved)     */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1200, 1000 },  2}, /* Dev (AP20 Reserved)     */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1200, 1000 },  3}, /* Dev (AP20 Reserved)     */ \
    {{1000000, 240000, 240000, 300000,  600000, 300000, 190000, 190000, 1225, 1100 },  4}, /* T240                    */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1200, 1000 },  5}, /* Dev (AP20 Reserved)     */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1200, 1000 },  6}, /* Dev (AP20 Reserved)     */ \
    {{ 750000, 240000, 240000, 300000,  600000, 300000, 190000, 190000, 1225,  975 },  7}, /* AP20                    */ \
    {{1000000, 240000, 240000, 400000,  800000, 333500, 190000, 190000, 1275, 1100 },  8}, /* T20                     */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1200, 1000 },  9}, /* Dev (AP20 Reserved)     */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1200, 1000 }, 10}, /* Dev (AP20 Reserved)     */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1200, 1000 }, 11}, /* Dev (AP20 Reserved)     */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1200, 1000 }, 12}, /* Dev (AP20 Reserved)     */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1200, 1000 }, 13}, /* Dev (AP20 Reserved)     */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1200, 1000 }, 14}, /* Dev (AP20 Reserved)     */ \
    {{1000000, 240000, 240000, 300000,  600000, 300000, 190000, 190000, 1225, 1100 }, 15}, /* AP20H                   */ \
    {{ 750000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1225,  975 }, 16}, /* T20L                    */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1200, 1000 }, 17}, /* Dev (AP20 Reserved)     */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1200, 1000 }, 18}, /* Dev (AP20 Reserved)     */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1200, 1000 }, 19}, /* Dev (AP20 Reserved)     */ \
    {{1200000, 300000, 300000, 400000,  800000, 400000, 190000, 190000, 1300, 1125 }, 20}, /* T25SE                   */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1200, 1000 }, 21}, /* Dev (AP20 Reserved)     */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1200, 1000 }, 22}, /* Dev (AP20 Reserved)     */ \
    {{1200000, 300000, 300000, 400000,  800000, 400000, 190000, 190000, 1300, 1125 }, 23}, /* AP25                    */ \
    {{1200000, 300000, 300000, 400000,  800000, 400000, 190000, 190000, 1300, 1125 }, 24}, /* T25                     */ \
    {{1200000, 300000, 300000, 400000,  800000, 400000, 190000, 190000, 1300, 1125 }, 27}, /* AP25E                   */ \
    {{1200000, 300000, 300000, 400000,  800000, 400000, 190000, 190000, 1300, 1125 }, 28}, /* T25E                    */ \
    {{ 600000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1250, 1025 }, 32}, /* T20                     */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1250, 1025 }, 36}, /* T20                     */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1250, 1025 }, 40}, /* T20                     */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1250, 1025 }, 44}, /* T20-A03                 */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1250, 1025 }, 96}, /* T20                     */ \
    {{ 900000, 240000, 240000, 400000,  800000, 300000, 190000, 190000, 1200, 1000 },255}  /* Dev (AP20 Reserved)     */

#define IsAp25Speedo(sku) \
    (((sku) == 23) || \
     ((sku) == 24) || \
     ((sku) == 20) || \
     ((sku) == 27) || \
     ((sku) == 28))

/**
 * Initializes SoC characterization data base
 * 
 * @param hRmDevice The RM device handle.
 * @param pChipFlavor a pointer to the chip "flavor" structure
 *  that this function fills in
 */
void NvRmPrivAp20FlavorInit(
    NvRmDeviceHandle hRmDevice,
    NvRmChipFlavor* pChipFlavor);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  // INCLUDED_AP20RM_CLOCKS_LIMITS_PRIVATE_H
