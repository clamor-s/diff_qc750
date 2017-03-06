/*
 * Copyright (c) 2006 - 2009 NVIDIA Corporation.  All rights reserved.
 * 
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file nvboot_fuse.h
 *
 * Defines the parameters and data structures related to fuses.
 */

#ifndef INCLUDED_NVBOOT_FUSE_H
#define INCLUDED_NVBOOT_FUSE_H

#if defined(__cplusplus)
extern "C"
{
#endif

#define NVBOOT_DEVICE_KEY_BYTES (4)


/*
 * NvBootFuseBootDevice -- Peripheral device where Boot Loader is stored
 *
 * AP15 A02 split the original NAND encoding in NAND x8 and NAND x16.
 * For backwards compatibility the original enum was maintained and 
 * is implicitly x8 and aliased with the explicit x8 enum.
 *
 * This enum *MUST* match the equivalent list in nvboot_devmgr.h for
 * all valid values and None, Undefined not present in nvboot_devmgr.h
 */
typedef enum
{
    NvBootFuseBootDevice_Sdmmc,
    NvBootFuseBootDevice_SnorFlash,
    NvBootFuseBootDevice_SpiFlash,
    NvBootFuseBootDevice_NandFlash,
    NvBootFuseBootDevice_NandFlash_x8  = NvBootFuseBootDevice_NandFlash,
    NvBootFuseBootDevice_NandFlash_x16 = NvBootFuseBootDevice_NandFlash,
    NvBootFuseBootDevice_MobileLbaNand,
    NvBootFuseBootDevice_MuxOneNand,
    NvBootFuseBootDevice_Max, /* Must appear after the last legal item */
    NvBootFuseBootDevice_Force32 = 0x7fffffff
} NvBootFuseBootDevice;
  
#if defined(__cplusplus)
}
#endif

#endif // INCLUDED_NVBOOT_FUSE_H
