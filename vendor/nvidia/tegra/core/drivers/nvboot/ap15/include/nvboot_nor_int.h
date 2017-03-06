/**
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/*
 * nvboot_nor.h - Public definitions for using NOR as the second level boot
 * device.
 */

#ifndef INCLUDED_NVBOOT_NOR_INT_H
#define INCLUDED_NVBOOT_NOR_INT_H

#include "nvboot_error.h"
#include "nvboot_device_int.h"
#include "ap15/nvboot_nor_param.h"
#include "nvboot_nor_context_int.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * Returns a pointer to a device-specific structure of device parameters
 * in the ROM.  Later, the init routine will use them to configure the device.
 *
 * @param ParamIndex Parma Index that comes from Fuse values.
 * @param Params double pointer to retunr Param info based on
 *          the param index value.
 *
 */
void
NvBootNorGetParams(
    const NvU32 ParamIndex,
    NvBootNorParams **Params);

/**
 * Checks the contents of the parameter structure and returns NV_TRUE
 * if the parameters are all legal, NV_FLASE otherwise.
 *
 *@param Params Pointer to Param info that needs validation.
 *
 * @retval NV_TRUE The parameters are valid.
 * @retval NV_FALSE The parameters would cause an error if used.
 */
NvBool
NvBootNorValidateParams(
    const NvBootNorParams *Params);

/**
 * Queries the block and page sizes for the device in units of log2(bytes).
 * Thus, a 1KB block size is reported as 10.
 *
 * @param Params Pointer to Param info.
 * @param BlockSizeLog2 returns block size in log2 scale.
 * @param PageSizeLog2 returns page size in log2 scale.
 *
 */
void
NvBootNorGetBlockSizes(
    const NvBootNorParams *Params,
    NvU32 *BlockSizeLog2,
    NvU32 *PageSizeLog2);

/**
 * Uses the data pointed to by DeviceParams to initialize
 * the device for reading.  Note that the routine will likely be called
 * multiple times - once for initially gaining access to the device,
 * and once to use better parameters stored in the device itself.
 *
 * DriverContext is provided as space for storing device-specific global state.
 * Drivers should keep this pointer around for reference during later calls.
 *
 * @param ParamData Pointer to Param info to initialize the Nand with.
 * @param Context Pointer to memory, where nand state is saved to.
 *
 * @retval NvBootError_Success No Error
 */
NvBootError
NvBootNorInit(
    const NvBootNorParams *ParamData,
    NvBootNorContext *Context);

/**
 * Initiate the reading of a page of data into Dest.buffer.
 *
 * @param Block Block number to read from.
 * @param Page Page number in the block to read from.
 *          valid range is 0 <= Page < PagesPerBlock.
 * @param Dest Buffer to rad the data into.
 *
 * @retval NvBootError_Success No Error
 */
NvBootError
NvBootNorReadPage(
    const NvU32 Block,
    const NvU32 Page,
    NvU8 *Dest);

/**
 * Check the status of read operation that is launched with
 *  API NvBootNandReadPage, if it is pending.
 *
 * @retval NvBootDeviceStatus_Idle - Read operation is complete.
 */
NvBootDeviceStatus NvBootNorQueryStatus(void);

/**
 * Shutdowns device and cleanup the state.
 *
 */
void NvBootNorShutdown(void);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_NVBOOT_NOR_INT_H */
