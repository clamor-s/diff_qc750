/*
 * Copyright (c) 2006 - 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file nvboot_pads.h
 *
 * Pads and other low level power function for NvBoot
 *
 * NvBootPads is NVIDIA's interface for control of IO pads and power related functions,
 * including pin mux control.
 *
 */

#ifndef INCLUDED_NVBOOT_PADS_INT_H
#define INCLUDED_NVBOOT_PADS_INT_H

#include "nvcommon.h"
#include "nvboot_error.h"
#include "ap15/arapbpm.h"
#include "ap15/nvboot_fuse.h"

#define NVBOOT_PADS_PWR_DET_DELAY (6)

#if defined(__cplusplus)
extern "C"
{
#endif

/* Set of IO rails supported in the API */
/* Note that the enum are power of two to allow the use of bitmap of IO rails */
/* The bit index are aligned to the HW bit index */
/* there is no MIPI in power detect, only for no io */
typedef enum
{
    NvBootPadsIoVoltageId_None = 0x0,
    NvBootPadsIoVoltageId_Ao   = (0x1 << APBDEV_PMC_PWR_DET_0_AO_SHIFT  ),  // bit 0
    NvBootPadsIoVoltageId_At3  = (0x1 << APBDEV_PMC_PWR_DET_0_AT3_SHIFT ),  //     1
    NvBootPadsIoVoltageId_Dbg  = (0x1 << APBDEV_PMC_PWR_DET_0_DBG_SHIFT ),  //     2
    NvBootPadsIoVoltageId_Dlcd = (0x1 << APBDEV_PMC_PWR_DET_0_DLCD_SHIFT),  //     3
    NvBootPadsIoVoltageId_Dvi  = (0x1 << APBDEV_PMC_PWR_DET_0_DVI_SHIFT ),  //     4
    NvBootPadsIoVoltageId_I2s  = (0x1 << APBDEV_PMC_PWR_DET_0_I2S_SHIFT ),  //     5
    NvBootPadsIoVoltageId_Lcd  = (0x1 << APBDEV_PMC_PWR_DET_0_LCD_SHIFT ),  //     6
    NvBootPadsIoVoltageId_Mem  = (0x1 << APBDEV_PMC_PWR_DET_0_MEM_SHIFT ),  //     7
    NvBootPadsIoVoltageId_Sd   = (0x1 << APBDEV_PMC_PWR_DET_0_SD_SHIFT  ),  //     8
    NvBootPadsIoVoltageId_Mipi = (0x1 << APBDEV_PMC_NO_IOPOWER_0_MIPI_SHIFT),  //  9
    NvBootPadsIoVolatgeId_Force32 = 0x7FFFFFF
} NvBootPadsIOVoltageId;

#define NVBOOT_PADS_ALL_IO_RAILS_BITMAP ( \
    ( (NvU32)NvBootPadsIoVoltageId_Ao      \
    | (NvU32)NvBootPadsIoVoltageId_At3     \
    | (NvU32)NvBootPadsIoVoltageId_Dbg     \
    | (NvU32)NvBootPadsIoVoltageId_Dlcd    \
    | (NvU32)NvBootPadsIoVoltageId_Dvi     \
    | (NvU32)NvBootPadsIoVoltageId_I2s     \
    | (NvU32)NvBootPadsIoVoltageId_Lcd     \
    | (NvU32)NvBootPadsIoVoltageId_Mem     \
    | (NvU32)NvBootPadsIoVoltageId_Sd      \
    | (NvU32)NvBootPadsIoVoltageId_Mipi  )

#define NVBOOT_PADS_UP_DRIVER_STRENGTH_1_8V (22)
#define NVBOOT_PADS_DN_DRIVER_STRENGTH_1_8V (18)

/*
 * NvBootPadsEnableIoVolatgeSampling()
 *
 * IO pads are finicky, they contain voltage shifter that should
 * only be used when needed, otherwise they consume power.
 * So each IO rail (except MIPI) has a power level detector, the
 * output of which indicates if the level shifter are needed or not.
 *
 * But the power level detector themselves consume power!! So the
 * output of the power detector is sampled then the power detector
 * disabled.
 *
 * At AO reset, the power detector and the sample control are both
 * active.  Two API functions are defined to either disable or enable
 * the IO sampling mechanism for a set of IO rails.
 *
 */
NvBootError
NvBootPadsEnableIoVolatgeSampling(NvU32 IoRailsBitmap);

/*
 * NvBootPadsDisableIoVolatgeSampling():
 */
NvBootError
NvBootPadsDisableIoVolatgeSampling(NvU32 IoRailsBitmap);

/*
 * Set up of correct path between a controller and external world
 * Also set the correct IO driver strength for interfaces that may operate
 * at 1.8 V (NAND and HSMMC)
 *
 * @param identification of the boot device
 *
 * @retval NvBootError_ValidationFailure of passing an incorrect device
 * @retval NvBootError_Success otherwise
 */
NvBootError
NvBootPadsConfigureForBootPeripheral(NvBootFuseBootDevice BootDevice) ;

#if defined(__cplusplus)
}
#endif

#endif // INCLUDED_NVBOOT_PADS_INT_H
