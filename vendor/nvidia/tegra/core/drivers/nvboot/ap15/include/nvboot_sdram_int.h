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
 * @file nvboot_sdram.h
 *
 * NvBootSdram interface for NvBOOT
 *
 * NvBootSdram is NVIDIA's interface for SDRAM configuration and control.
 *
 *
 * For frozen/cold boot, the process is --
 * 1. NvBootSdramReleaseBus
 * 2. NvBootSdramInitFromParams
 * 3. NvBootSdramQueryTotalSize
 *
 *
 */

#ifndef INCLUDED_NVBOOT_SDRAM_INT_H
#define INCLUDED_NVBOOT_SDRAM_INT_H

#include "nvboot_error.h"
#include "ap15/nvboot_sdram_param.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/**
  * Initializes the SDRAM controller according to the given configuration
  * parameters.  Intended for frozen/cold boot use.
  *
  * Generally the steps are --
  * 1. Program and enable PLL
  * 2. Wait for the PLL to get locked
  * 3. Enable clock and disable reset to emc
  *
  * @param ParamData pointer to configuration parameters
  *
  */
void
NvBootSdramInit(const NvBootSdramParams *ParamData);


/**
  * Initializes the SDRAM controller according to the given configuration
  * parameters.  Intended for warm boot0 boot use.
  *
  * Generally the steps are --
  * 1. Program and enable PLL
  * 2. Wait for the PLL to get locked
  * 3. Enable clock and disable reset to emc
  * 4. Write the desired EMC and MC registers in the correct sequence
  *
  * @param ParamData pointer to configuration parameters
  *
  */
void
NvBootSdramInitWarmBoot0(const NvBootSdramParams *ParamData);



  /**
    * Detects and reports total amount of SDRAM memory installed on the system.
    * Size is reported as log2 of the total amount of SDRAM memory in bytes. This API
    * can be called only after SDRAM is initialized successfully.
    *
    * @retval  log2 of the total amount of SDRAM memory in bytes
    * @retval 0 If the SDRAM was not initiazlied and we are not able write/read into/from SDRAM
    *
    */

NvU32
NvBootSdramQueryTotalSize( void );


/**
  * Clock the SDRAM chip so that any operation that may have been in progress
  * when the chip was reset is allowed to complete, thus releasing the data
  * bus.  Since the SDRAM and NOR flash devices share a common data bus, omitting
  * this process would possibly prevent the NOR flash device from driving the
  * bus and thereby prevent the NOR flash device from being accessed.
  *
  * Use only for frozen/cold boot.  For WB0, the SDRAM controller will have been
  * shut down properly so this routine isn't needed; furthermore, enabling the
  * clock enable might interfer with proper operation of the SDRAM (which is
  * currently in Self Refresh mode.
  *
  * Generally, the steps are --
  * 1. configure the SDRAM controller to drive the SDRAM's clock signal from
  *    the chip's oscillator input
  * 2. enable the clock output and clock enable signals to the SDRAM chip
  * 3. allow the SDRAM chip to be clocked for the length of the maximum read
  *    operation (roughly the max burst size, i.e., 16 cycles)
  * 4. disable the clock output and the clock enable signals to the SDRAM chip
  *
  *
  * @return NvBootError_Success
  *          - if Bus release is success
  * @return NvBootError_NotInitialized
  *          - if SDRAm context is not intialized
  */
NvBootError
NvBootSdramReleaseBus(void);


#if defined(__cplusplus)
}
#endif

#endif // INCLUDED_NVBOOT_SDRAM_INT_H

