/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef INCLUDED_SD_DIAG_TESTS_H
#define INCLUDED_SD_DIAG_TESTS_H

#include "nverror.h"
#include "nvcommon.h"

#if defined(__cplusplus)
extern "C"
{
#endif


#define NV_DIAG_SD_SECTOR_SIZE       4096
#define NV_DIAG_SD_UNIT_SIZE       1024
#define NV_MICROSEC_IN_SEC                 1000000
#define NV_BYTES_IN_KB                     1024
#define NV_BYTES_IN_MB                     (NV_BYTES_IN_KB * NV_BYTES_IN_KB)
#define NV_WRITE_READ_VERIFY_NEEDED_FOR_SPEED_TEST      1

typedef enum
{
    SdDiagType_WriteRead = 0,
    SdDiagType_WriteReadFull,
    SdDiagType_WriteReadSpeed,
    SdDiagType_WriteReadSpeed_BootArea,
    SdDiagType_Erase,
    SdDiagType_Num,
    SdDiagType_Force32 = 0x7FFFFFFF
} SdDiagTests;

/**
 * Initializes the Sd Block driver, and opens the sd controller for the given
 * instance.
 *
 * @param Instance The Instance of the SD Controller.
 *
 * @retval NvSuccess. Block driver sucessfully initialized.
 * @retval NvError_InsufficientMemory. Failed to allocate memory.
 * @retval NvError_DeviceNotFound Indicates device is either not present, not
 *            responding, or not supported.
 */
NvError SdDiagInit(NvU32 Instance);

/**
 * Executes the requested Sd Diagnostic Test
 *
 * @param Type Type of the test. SdDiagVerifyWriteRead,
 *             SdDiagVerifyWriteReadFull etc
 *
 * @retval NvSuccess. Block driver sucessfully initialized.
 * @retval NvError_InsufficientMemory. Failed to allocate memory.
 * @retval NvError_DeviceNotFound Indicates device is either not present, not
 *            responding, or not supported.
 */
NvError SdDiagTest(NvU32 Type);

/**
 * Verifies the Sd Memory partially, by checking Write and Read data.
 *
 * @retval NvSuccess. Block driver sucessfully initialized.
 * @retval NvError_InsufficientMemory. Failed to allocate memory.
 * @retval NvError_NotImplemented If it is a stub function.
 */
NvError SdDiagVerifyWriteRead(void);


/**
 * Verifies the Sd Memory Fully, by checking Write and Read data
 *
 * @retval NvSuccess.Write Read Verify Test Successful.
 * @retval NvError_InsufficientMemory. Failed to allocate memory.
 * @retval NvError_NotImplemented If it is a stub function.
 */
NvError SdDiagVerifyWriteReadFull(void);


/**
 * Verifies the Sd Memory by checking Write and Read data
 *
 * @retval NvSuccess.Write Read Verify Test Successful.
 * @retval NvError_InsufficientMemory. Failed to allocate memory.
 * @retval NvError_NotImplemented If it is a stub function.
 */
NvError SdDiagVerifyWriteReadSpeed(void);


/**
 * Verifies the Write and Read Speed of Sd Memory  Boot area, .
 *
 * @retval NvSuccess. Write Read Full Verify Test Successful.
 * @retval NvError_InsufficientMemory. Failed to allocate memory.
 * @retval NvError_NotImplemented If it is a stub function.
 */
NvError SdDiagVerifyWriteReadSpeed_BootArea(void);


/**
 * Verifies the Sd Memory Erase and the Erase time.
 *
 * @retval NvSuccess. Erase and Erase Time Verify Test Successful.
 * @retval NvError_InsufficientMemory. Failed to allocate memory.
 * @retval NvError_NotImplemented If it is a stub function.
 * @retval NvError_NotSupported Opcode is not recognized.
 */
NvError SdDiagVerifyErase(void);


/**
 * Closes the SD Controller Instance and the block driver.
 *
 */
void SdDiagClose(void);


#if defined(__cplusplus)
}
#endif

#endif // INCLUDED_SD_DIAG_TESTS_H
