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
 * nvboot_nand.h - Public definitions for using NAND as the second level boot
 * device.
 */

#ifndef INCLUDED_NVBOOT_NAND_INT_H
#define INCLUDED_NVBOOT_NAND_INT_H

#include "nvboot_error.h"
#include "nvboot_device_int.h"
#include "ap15/nvboot_nand_param.h"
#include "nvboot_nand_context_int.h"

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
NvBootNandGetParams(
    const NvU32 ParamIndex,
    NvBootNandParams **Params);

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
NvBootNandValidateParams(
    const NvBootNandParams *Params);

/**
 * Queries the block and page sizes for the device in units of log2(bytes).
 * Thus, a 1KB block size is reported as 10.
 *
 * @note NvBootNandInit() must have beeen called atleast once before this API
 *          and the call must have returned NvBootError_Success.
 *
 * @param Params Pointer to Param info.
 * @param BlockSizeLog2 returns block size in log2 scale.
 * @param PageSizeLog2 returns page size in log2 scale.
 *
 */
void
NvBootNandGetBlockSizes(
    const NvBootNandParams *Params,
    NvU32 *BlockSizeLog2,
    NvU32 *PageSizeLog2);

/**
 * Uses the data pointed to by DeviceParams to initialize an 8-bit
 * device for reading.  Note that the routine will likely be called
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
 * @retval NvBootError_HwTimeOut Device is not responding.
 * @retval NvBootError_DeviceUnsupported Device is not supported.
 */

/* provide an alias for the original name for backward compatibility */
#define NvBootNandInit  NvBootNandInit8Bit

NvBootError
NvBootNandInit8Bit(
    const NvBootNandParams *ParamData,
    NvBootNandContext *Context);

/**
 * Uses the data pointed to by DeviceParams to initialize a 16-bit
 * device for reading.  Note that the routine will likely be called
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
 * @retval NvBootError_HwTimeOut Device is not responding.
 * @retval NvBootError_DeviceUnsupported Device is not supported.
 */
NvBootError
NvBootNandInit16Bit(
    const NvBootNandParams *ParamData,
    NvBootNandContext *Context);

/**
 * Initiate the reading of a page of data into Dest.buffer.
 *
 * @param Block Block number to read from.
 * @param Page Page number in the block to read from.
 *          valid range is 0 <= Page < PagesPerBlock.
 * @param Dest Buffer to rad the data into. The buffer must be atleast 32 byte
 *          aligned.
 *
 * @retval NvBootError_Success No Error
 * @retval NvBootError_HwTimeOut Device is not responding.
 */
NvBootError
NvBootNandReadPage(
    const NvU32 Block,
    const NvU32 Page,
    NvU8 *Dest);

/**
 * Check the status of read operation that is launched with
 *  API NvBootNandReadPage, if it is pending.
 *
 * @retval NvBootDeviceStatus_ReadInProgress - Reading in progress.
 * @retval NvBootDeviceStatus_EccFailure - Data is corrupted.
 * @retval NvBootDeviceStatus_Idle - Read operation is complete.
 * @retval NvBootDeviceStatus_ReadFailure - Read operation failed.
 */
NvBootDeviceStatus NvBootNandQueryStatus(void);

/**
 * Shutdowns device and cleanup the state.
 *
 */
void NvBootNandShutdown(void);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_NVBOOT_NAND_INT_H */
