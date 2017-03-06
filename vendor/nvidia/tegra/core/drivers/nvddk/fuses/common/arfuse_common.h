/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_NVDDK_ARFUSE_COMMON_H
#define INCLUDED_NVDDK_ARFUSE_COMMON_H

#if defined(__cplusplus)
extern "C"
{
#endif

#ifndef _MK_SHIFT_CONST
   #define _MK_SHIFT_CONST(_constant_) _constant_
#endif
#ifndef _MK_MASK_CONST
   #define _MK_MASK_CONST(_constant_) _constant_
#endif
#ifndef _MK_ENUM_CONST
   #define _MK_ENUM_CONST(_constant_) (_constant_ ## UL)
#endif
#ifndef _MK_ADDR_CONST
   #define _MK_ADDR_CONST(_constant_) _constant_
#endif

#define NV_ADDRESS_MAP_APB_FUSE_BASE        0x7000F800
#define NV_ADDRESS_MAP_CAR_BASE             0x60006000

// Register FUSE_FA_0
#define FUSE_FA_0                       _MK_ADDR_CONST(0x148)
#define FUSE_PRIVATE_KEY0_NONZERO_0                     _MK_ADDR_CONST(0x50)
#define FUSE_PRIVATE_KEY1_NONZERO_0                     _MK_ADDR_CONST(0x54)
#define FUSE_PRIVATE_KEY2_NONZERO_0                     _MK_ADDR_CONST(0x58)
#define FUSE_PRIVATE_KEY3_NONZERO_0                     _MK_ADDR_CONST(0x5c)
#define FUSE_PRIVATE_KEY4_NONZERO_0                     _MK_ADDR_CONST(0x60)

// Register FUSE_SKU_INFO_0
#define FUSE_SKU_INFO_0                 _MK_ADDR_CONST(0x110)
#define FUSE_SKU_INFO_0_SKU_INFO_RANGE                  7:0

#define NV_ADDRESS_MAP_FUSE_BASE            0x7000F800

// Register FUSE_PACKAGE_INFO_0
#define FUSE_PACKAGE_INFO_0                     _MK_ADDR_CONST(0x1fc)

// Register FUSE_PRIVATE_KEY0_0
#define FUSE_PRIVATE_KEY0_0                     _MK_ADDR_CONST(0x1a4)

// Register FUSE_PRIVATE_KEY4_0
#define FUSE_PRIVATE_KEY4_0                     _MK_ADDR_CONST(0x1b4)

#define FUSE_SECURITY_MODE_0                            _MK_ADDR_CONST(0x1a0)

// Register FUSE_PRODUCTION_MODE_0
#define FUSE_PRODUCTION_MODE_0                  _MK_ADDR_CONST(0x100)

// Register FUSE_RESERVED_ODM0_0
#define FUSE_RESERVED_ODM0_0                    _MK_ADDR_CONST(0x1c8)

// Register FUSE_RESERVED_SW_0
#define FUSE_RESERVED_SW_0                      _MK_ADDR_CONST(0x1c0)
#define FUSE_RESERVED_SW_0_BOOT_DEVICE_SELECT_RANGE       2:0
#define FUSE_RESERVED_SW_0_SW_RESERVED_RANGE                    7:6
#define FUSE_RESERVED_SW_0_SKIP_DEV_SEL_STRAPS_RANGE      3:3

#define FUSE_ARM_DEBUG_DIS_0                    _MK_ADDR_CONST(0x1b8)

#define FUSE_BOOT_DEVICE_INFO_0                 _MK_ADDR_CONST(0x1bc)

#define APB_MISC_GP_HIDREV_0                    _MK_ADDR_CONST(0x804)
#define APB_MISC_GP_HIDREV_0_CHIPID_RANGE                       15:8

#define CLK_RST_CONTROLLER_MISC_CLK_ENB_0                       _MK_ADDR_CONST(0x48)
#define CLK_RST_CONTROLLER_MISC_CLK_ENB_0_CFG_ALL_VISIBLE_RANGE                 28:28

#define FUSE_BOOT_DEVICE_INFO_0_BOOT_DEVICE_CONFIG_RANGE 13:0

#ifdef __cplusplus
}
#endif

#endif
