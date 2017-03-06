/*
 * Copyright (c) 2008-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#ifndef INCLUDED_NVBOOTARGS_H
#define INCLUDED_NVBOOTARGS_H

/**
 * @file
 * <b>NVIDIA Boot Arguments</b>
 *
 * @b Description: Defines the basic boot argument structure and keys for use
 * with NvOsBootArgGet and NvOsBootArgSet.
 */

/**
 * @defgroup boot_arguments_group Boot Arguments
 *
 * Defines the basic boot argument structure and keys for use with
 * NvOsBootArgGet() and NvOsBootArgSet().
 * @{
 */

#include "nvcommon.h"

/**
 * The maximum number of memory handles that may be preserved across the
 * bootloader-to-OS transition.  @see NvRmBootArg_PreservedMemHandle.
 */
#define NV_BOOTARGS_MAX_PRESERVED_MEMHANDLES 3
#define CMDLINE_BUF_SIZE 40

/**
 * Compressed kernel image will be loaded at this address
 */
#define ZIMAGE_START_ADDR (0xA00000UL)  /**< Offset @ 10MB */

/**
 * Size of the memory reserved for loading compressed kernel image. Because the
 * decompressor of the kernel decompresses the kernel image and uses the memory
 * from the beginning, this allocation has 32MB to spare. With this margin
 * we avoid disturbing the already loaded ramdisk @32MB offset.
 */
#define AOS_RESERVE_FOR_KERNEL_IMAGE_SIZE (0x1600000) /**< 22MB */
#define RAMDISK_MAX_SIZE                  (0x1400000) /**< 20MB */


#if defined(__cplusplus)
extern "C"
{
#endif

/* accessor for various boot arg classes, see NvOsBootArg* */
typedef enum
{
    NvBootArgKey_Rm = 0x1,
    NvBootArgKey_Display,
    NvBootArgKey_Framebuffer,
    NvBootArgKey_ChipShmoo,
    NvBootArgKey_ChipShmooPhys,
    NvBootArgKey_Carveout,
    NvBootArgKey_WarmBoot,
    NvBootArgKey_PreservedMemHandle_0 = 0x10000,
    NvBootArgKey_PreservedMemHandle_Num = (NvBootArgKey_PreservedMemHandle_0 +
                                         NV_BOOTARGS_MAX_PRESERVED_MEMHANDLES),
    NvBootArgKey_Force32 = 0x7FFFFFFF,
} NvBootArgKey;

/**
 * Resource Manager boot args.
 *
 * Nothing here yet.
 */
typedef struct NvBootArgsRmRec
{
    NvU32 reserved;
} NvBootArgsRm;

/**
 * Carveout boot args, which define the physical memory location of the GPU
 * carved-out memory region(s).
 */
typedef struct NvBootArgsCarveoutRec
{
    NvUPtr base;
    NvU32 size;
} NvBootArgsCarveout;

/**
 * Warmbootloader boot args. This structure only contains
 * a mem handle key to preserve the warm bootloader
 * across the bootloader->os transition
 */
typedef struct NvBootArgsWarmbootRec
{
    /* The key used for accessing the preserved memory handle */
    NvU32 MemHandleKey;
} NvBootArgsWarmboot;

/**
 * PreservedMemHandle boot args, indexed by PreservedMemHandle_0 + n.
 * All values n from 0 to the first value which does not return NvSuccess will
 * be quered at RM initialization in the OS environment.  If present, a new
 * memory handle for the physical region specified will be created.
 * This allows physical memory allocations (e.g., for framebuffers) to persist
 * between the bootloader and operating system.  Only carveout and IRAM
 * allocations may be preserved with this interface.
 */
typedef struct NvBootArgsPreservedMemHandleRec
{
    NvUPtr  Address;
    NvU32   Size;
} NvBootArgsPreservedMemHandle;


/**
 * Display boot args, indexed by NvBootArgKey_Display.
 *
 * The bootloader may have a splash screen. This will flag which controller
 * and device was used for the splash screen so the device will not be
 * reinitialized (which causes visual artifacts).
 */
typedef struct NvBootArgsDisplayRec
{
    /* which controller is initialized */
    NvU32 Controller;

    /* index into the ODM device list of the boot display device */
    NvU32 DisplayDeviceIndex;

    /* set to NV_TRUE if the display has been initialized */
    NvBool bEnabled;
} NvBootArgsDisplay;

/**
 * Framebuffer boot args, indexed by NvBootArgKey_Framebuffer
 *
 * A framebuffer may be shared between the bootloader and the
 * operating system display driver.  When this key is present,
 * a preserved memory handle for the framebuffer must also
 * be present, to ensure that no display corruption occurs
 * during the transition.
 */
typedef struct NvBootArgsFramebufferRec
{
    /*  The key used for accessing the preserved memory handle */
    NvU32 MemHandleKey;
    /*  Total memory size of the framebuffer */
    NvU32 Size;
    /*  Color format of the framebuffer, cast to a U32  */
    NvU32 ColorFormat;
    /*  Width of the framebuffer, in pixels  */
    NvU16 Width;
    /*  Height of each surface in the framebuffer, in pixels  */
    NvU16 Height;
    /*  Pitch of a framebuffer scanline, in bytes  */
    NvU16 Pitch;
    /*  Surface layout of the framebuffer, cast to a U8 */
    NvU8  SurfaceLayout;
    /*  Number of contiguous surfaces of the same height in the
     *  framebuffer, if multi-buffering.  Each surface is
     *  assumed to begin at Pitch * Height bytes from the
     *  previous surface.  */
    NvU8  NumSurfaces;
    /* Flags for future expandability.
     * Current allowable flags are:
     * zero - default
     * NV_BOOT_ARGS_FB_FLAG_TEARING_EFFECT - use a tearing effect signal in
     *      combination with a trigger from the display software to generate
     *      a frame of pixels for the display device.
     */
    NvU32 Flags;
#define NVBOOTARG_FB_FLAG_TEARING_EFFECT (0x1)

} NvBootArgsFramebuffer;

/**
 * Chip chatcterization shmoo data indexed by NvBootArgKey_ChipShmoo
 */
typedef struct NvBootArgsChipShmooRec
{
    // The key used for accessing the preserved memory handle of packed
    // charcterization tables
    NvU32 MemHandleKey;

    // Offset and size of each unit in the packed buffer
    NvU32 CoreShmooVoltagesListOffset;
    NvU32 CoreShmooVoltagesListSize;

    NvU32 CoreScaledLimitsListOffset;
    NvU32 CoreScaledLimitsListSize;

    NvU32 OscDoublerListOffset;
    NvU32 OscDoublerListSize;

    NvU32 SKUedLimitsOffset;
    NvU32 SKUedLimitsSize;

    NvU32 CpuShmooVoltagesListOffset;
    NvU32 CpuShmooVoltagesListSize;

    NvU32 CpuScaledLimitsOffset;
    NvU32 CpuScaledLimitsSize;

    // Misc charcterization settings
    NvU16 CoreCorner;
    NvU16 CpuCorner;
    NvU32 Dqsib;
    NvU32 SvopLowVoltage;
    NvU32 SvopLowSetting;
    NvU32 SvopHighSetting;
} NvBootArgsChipShmoo;

/**
 * Chip chatcterization shmoo data indexed by NvBootArgKey_ChipShmooPhys
 */
typedef struct NvBootArgsChipShmooPhysRec
{
    NvU32 PhysShmooPtr;
    NvU32 Size;
} NvBootArgsChipShmooPhys;

#define NVBOOTARG_NUM_PRESERVED_HANDLES (NvBootArgKey_PreservedMemHandle_Num - \
                                         NvBootArgKey_PreservedMemHandle_0)

/**
 * OS-agnostic bootarg structure.
 */
typedef struct NvBootArgsRec
{
    NvBootArgsRm RmArgs;
    NvBootArgsDisplay DisplayArgs;
    NvBootArgsFramebuffer FramebufferArgs;
    NvBootArgsChipShmoo ChipShmooArgs;
    NvBootArgsChipShmooPhys ChipShmooPhysArgs;
    NvBootArgsWarmboot WarmbootArgs;
    NvBootArgsPreservedMemHandle MemHandleArgs[NVBOOTARG_NUM_PRESERVED_HANDLES];
} NvBootArgs;
/** @} */
#if defined(__cplusplus)
}
#endif

#endif // INCLUDED_NVBOOTARGS_H
