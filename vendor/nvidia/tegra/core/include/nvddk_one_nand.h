/*
 * Copyright (c) 2008-2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * <b> NVIDIA Driver Development Kit: One NAND Driver APIs</b>
 *
 * @b Description: This file defines the interface for NvDDK one NAND module.
 */


#ifndef INCLUDED_NVDDK_ONE_NAND_H
#define INCLUDED_NVDDK_ONE_NAND_H


#include "nvcommon.h"
#include "nvos.h"
#include "nvddk_blockdev.h"
#include "nvddk_nand.h"

#if defined(__cplusplus)
 extern "C"
 {
#endif  /* __cplusplus */

#define DEBUG_PRINTS 0 

/**
 * @defgroup nvbdk_ddk_onenand One NAND Driver APIs
 *
 * This is the interface for NvDDK one NAND module.
 *
 * @ingroup nvddk_modules
 * @{
 */

 /** 
  * @brief Opaque handle for NvDdkOneNand context
  */
typedef struct NvDdkOneNandRec *NvDdkOneNandHandle;


/**
 * Initializes the one NAND driver and returns a created handle to the client.
 * Only one instance of the handle can be created.
 * 
 * @pre The client must call this API first before calling any further APIs.
 *
 * @param hDevice Handle to RM device.
 * @param Instance Instance ID for the nand controller.
 * @param phOneNand A pointer to the handle where the created handle will be stored.
 *
 * @retval NvSuccess Initialization is successful.
 * @retval NvError_AlreadyAllocated The driver is already in use.
 */
NvError
NvDdkOneNandOpen(
    NvRmDeviceHandle hDevice, 
    NvU8 Instance,
    NvDdkOneNandHandle *phOneNand);

/**
 * Closes the one NAND driver and frees the handle.
 *
 * @param hOneNand Handle to the one NAND driver.
 *
 */
void NvDdkOneNandClose(NvDdkOneNandHandle hOneNand);

/**
 * Reads data from the one NAND device. 
 *
 * @param hOneNand Handle to the one NAND driver.
 * 
 * @param StartDeviceNum The device number, which read operation has to be 
 *      started from. It starts from value '0'.
 * @param pPageNumbers A pointer to an array containing page numbers for
 *      each NAND Device. If there are (n + 1) NAND Devices, then 
 *      array size should be (n + 1).
 *      - pPageNumbers[0] gives page number to access in NAND Device 0.
 *      - pPageNumbers[1] gives page number to access in NAND Device 1.
 *      - ....................................
 *      - pPageNumbers[n] gives page number to access in NAND Device n.
 *      
 *      If NAND device 'n' should not be accessed, fill \a pPageNumbers[n] as 
 *      0xFFFFFFFF.
 *      If the read starts from NAND device 'n', all the page numbers 
 *      in the array should correspond to the same row, even though we don't 
 *      access the same row pages for '0' to 'n-1' Devices.
 * @param pDataBuffer A pointer to read the page data into. The size of buffer 
 *      should be (*pNoOfPages * PageSize).
 * @param pTagBuffer A pointer to read the tag data into. The size of buffer 
 *      should be (*pNoOfPages * TagSize).
 * @param pNoOfPages The number of pages to read. This count should include 
 *      only valid page count. Consder that total NAND devices present is 4,
 *      Need to read 1 page from Device1 and 1 page from Device3. In this case,
 *      \a StartDeviceNum should be 1 and Number of pages should be 2.
 *      \a pPageNumbers[0] and \a pPageNumbers[2] should have 0xFFFFFFFF.
 *      \a pPageNumbers[1] and \a pPageNumbers[3] should have valid page numbers.
 *      The same pointer returns the number of pages read successfully.
 * @param IgnoreEccError If set to NV_TRUE, It ignores the ECC error and 
 *      continues to read the subsequent pages with out aborting read operation.
 *      This is required during bad block replacements.
 *
 * @retval NvSuccess NAND read operation completed successfully.
 * @retval NvError_NandReadEccFailed Indicates nand read encountered ECC 
 *      errors that cannot be corrected.
 * @retval NvError_NandOperationFailed NAND read operation failed.
 */
NvError
NvDdkOneNandRead(
    NvDdkOneNandHandle hOneNand,
    NvU8 StartDeviceNum,
    NvU32* pPageNumbers,
    NvU8* const pDataBuffer,
    NvU8* const pTagBuffer,
    NvU32 *pNoOfPages,
    NvBool IgnoreEccError);

/**
 * Writes data from the one NAND device. 
 *
 * @param hOneNand Handle to the one NAND driver.
 * 
 * @param StartDeviceNum The device number, which read operation has to be 
 *      started from. It starts from value '0'.
 * @param pPageNumbers A pointer to an array containing page numbers for
 *      each NAND device. If there are (n + 1) NAND devices, then 
 *      array size should be (n + 1).
 *      - pPageNumbers[0] gives page number to access in NAND Device 0.
 *      - pPageNumbers[1] gives page number to access in NAND Device 1.
 *      - ....................................
 *      - pPageNumbers[n] gives page number to access in NAND Device n.
 *      
 *      If NAND device 'n' should not be accessed, fill \a pPageNumbers[n] as 
 *      0xFFFFFFFF.
 *      If the read starts from NAND device 'n', all the page numbers 
 *      in the array should correspond to the same row, even though we don't 
 *      access the same row pages for '0' to 'n-1' Devices.
 * @param pDataBuffer A pointer to read the page data into. The size of buffer 
 *      should be (*pNoOfPages * PageSize).
 * @param pTagBuffer A pointer to read the tag data into. The size of buffer 
 *      should be (*pNoOfPages * TagSize).
 * @param pNoOfPages The number of pages to read. This count should include 
 *      only valid page count. Consder that total NAND devices present is 4,
 *      Need to read 1 page from Device1 and 1 page from Device3. In this case,
 *      \a StartDeviceNum should be 1 and Number of pages should be 2.
 *      \a pPageNumbers[0] and \a pPageNumbers[2] should have 0xFFFFFFFF.
 *      \a pPageNumbers[1] and \a pPageNumbers[3] should have valid page numbers.
 *      The same pointer returns the number of pages read successfully.
 *
 * @retval NvSuccess NAND read operation completed successfully.
 * @retval NvError_NandReadEccFailed Indicates NAND read encountered ECC 
 *      errors that cannot be corrected.
 * @retval NvError_NandOperationFailed NAND read operation failed.
 */
NvError
NvDdkOneNandWrite(
    NvDdkOneNandHandle hOneNand,
    NvU8 StartDeviceNum,
    NvU32* pPageNumbers,
    const NvU8* pDataBuffer,
    const NvU8* pTagBuffer,
    NvU32 *pNoOfPages);

/**
 * Erases the NAND flash. 
 *
 * @param hOneNand Handle to the one NAND driver.
 * 
 * @param hOneNand Handle to the NAND, which is returned by NvDdkNandOpen().
 * @param StartDeviceNum The device number, which erase operation has to be 
 *      started from. It starts from value '0'.
 * @param pPageNumbers A pointer to an array containing page numbers for
 *      each NAND device. If there are (n + 1) NAND devices, then 
 *      array size should be (n + 1).
 *      - pPageNumbers[0] gives page number to access in NAND Device 0.
 *      - pPageNumbers[1] gives page number to access in NAND Device 1.
 *      - ....................................
 *      - pPageNumbers[n] gives page number to access in NAND Device n.
 *      
 *      If NAND Device 'n' should not be accessed, fill \a pPageNumbers[n] as 
 *      0xFFFFFFFF.
 *      If the read starts from NAND device 'n', all the page numbers 
 *      in the array should correspond to the same row, even though we don't 
 *      access the same row pages for '0' to 'n-1' Devices.
 * @param pNumberOfBlocks The number of blocks to erase. This count should include 
 *      only valid block count. Consder that total NAND devices present is 4,
 *      Need to erase 1 block from Device1 and 1 block from Device3. In this case,
 *      \a StartDeviceNum should be 1 and number of blocks should be 2.
 *      \a pPageNumbers[0] and \a pPageNumbers[2] should have 0xFFFFFFFF.
 *      \a pPageNumbers[1] and \a pPageNumbers[3] should have valid page numbers
 *      corresponding to blocks.
 *      The same pointer returns the number of blocks erased successfully.
 *
 * @retval NvSuccess Operation completed successfully.
 * @retval NvError_NandOperationFailed Operation failed.
 */
NvError
NvDdkOneNandErase(
    NvDdkOneNandHandle hOneNand,
    NvU8 StartDeviceNum,
    NvU32* pPageNumbers,
    NvU32* pNumberOfBlocks);

/**
 * Gets the one NAND flash device information.
 *
 * @param hOneNand Handle to the one NAND driver.
 * @param DeviceNumber NAND flash device number.
 * @param pDeviceInfo Returns the device information.
 *
 * @retval NvSuccess Operation completed successfully.
 * @retval NvError_NandOperationFailed NAND operation failed.
 */
NvError
NvDdkOneNandGetDeviceInfo(
    NvDdkOneNandHandle hOneNand,
    NvU8 DeviceNumber,
    NvDdkNandDeviceInfo* pDeviceInfo);

/**
 * Gives the block specific information such as tag information, lock status, block good/bad.
 * @param hOneNand Handle to the one NAND driver.
 * @param DeviceNumber Device number in which the requested block exists.
 * @param BlockNumber Requested physical block number.
 * @param pBlockInfo Return the block information.
*/
void
NvDdkOneNandGetBlockInfo(
    NvDdkOneNandHandle hOneNand,
    NvU32 DeviceNumber,
    NvU32 BlockNumber,
    NandBlockInfo* pBlockInfo);

/**
 * Returns the details of the locked apertures, like device number, starting 
 * page number, ending page number of the region locked.
 *
 * @param hOneNand Handle to the one NAND driver.
 * @param pFlashLockParams A pointer to first array element of \a LockParams type 
 * with eight elements in the array. 
 * Check if \a pFlashLockParams[i].DeviceNumber == 0xFF, then that aperture is free to 
 * use for locking.
 */
void 
NvDdkOneNandGetLockedRegions(
    NvDdkOneNandHandle hOneNand,
    LockParams* pFlashLockParams);

/**
 * Locks the specified NAND flash pages.
 *
 * @param hOneNand Handle to the one NAND driver.
 * @param pFlashLockParams A pointer to the range of pages to be locked.
 */
void 
NvDdkOneNandSetFlashLock(
    NvDdkOneNandHandle hOneNand,
    LockParams* pFlashLockParams);

/**
 * Releases all regions that were locked using NvDdkNandSetFlashLock().
 *
 * @param hOneNand Handle to the one NAND driver.
 */
void NvDdkOneNandReleaseFlashLock(NvDdkOneNandHandle hOneNand);


/**
 * Disable the SNOR controller. The clock is disabled and the RM is 
 * notified that the driver is ready to be suspended.
 *
 * @param hOneNand Handle to the one NAND driver.
 */
NvError NvDdkOneNandSuspend(NvDdkOneNandHandle hOneNand);

/**
 * Resumes the SNOR controller from suspended state. 
 *
 * @param hOneNand Handle to the one NAND driver.
 *
 * @retval NvSuccess The driver is successfully resumed.
 */
NvError NvDdkOneNandResume(NvDdkOneNandHandle hOneNand);

/**
 * It allocates the required resources, powers on the device, 
 * and prepares the device for I/O operations.
 * Client gets a valid handle only if the device is found.
 * The same handle must be used for further operations.
 * The device can be opened by only one client at a time.
 *
 * @pre This method must be called once before using other 
 *           NvDdkBlockDev APIs.
 *
 * @param Instance Instance of specific device.
 * @param MinorInstance Minor instance of specific device.
 * @param phBlockDev Returns a pointer to device handle.
 *
 * @retval NvSuccess Device is present and ready for I/O operations.
 * @retval NvError_DeviceNotFound Device is neither present nor responding 
 *                                   nor supported.
 * @retval NvError_DeviceInUse The requested device is already in use.
 * @retval NvError_InsufficientMemory Cannot allocate memory.
 */
NvError
NvDdkOneNandBlockDevOpen(
    NvU32 Instance,
    NvU32 MinorInstance,
    NvDdkBlockDevHandle *phBlockDev);

NvError NvDdkOneNandBlockDevInit(NvRmDeviceHandle hDevice);

void NvDdkOneNandBlockDevDeinit(void);

#if defined(__cplusplus)
 }
#endif  /* __cplusplus */
 
 /** @} */
 
#endif  /* INCLUDED_NVDDKONENAND_H */

