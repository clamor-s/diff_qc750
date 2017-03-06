/**
 * Copyright (c) 2008-2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
/**
 * @file nvyaffs2filesystem.h
 *
 * NvYaffs2FileSystem is NVIDIA's Yaffs2 File System.
 *
 */

#ifndef YAFFS2FS_H
#define YAFFS2FS_H

#include "nvfsmgr.h"
#include "nvddk_blockdev.h"
#include "nverror.h"
#include "nvfs.h"


#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * Defines how data is organized in a YAFFS-formatted buffer.
 */
typedef struct NvYaffs2LayoutRec
{
    /// Specifies NAND page size in bytes of the YAFFS data.
    NvU16 PageSize;
    /// Specifies the amount of tag bytes accompanying each page.
    NvU8  TagSize;
    /// Specifies the amount of padding bytes between page N's tag data and page N+1's data.
    NvU8  SkipSize;
} NvYaffs2Layout;

/**
 * Initializes Yaffs2 File system driver
 *
 * @retval NvSuccess Global resources have been allocated and initialized.
 * @retval NvError_DeviceNotFound File system is not present or not supported.
 * @retval NvError_InsufficientMemory Can't allocate memory.
 */
NvError
NvYaffs2FileSystemInit(void);

/**
 * Mounts Yaffs2 file system
 *
 * @param hPart handle for partition where file system is mounted
 * @param hDevice handle for device where partition is located
 * @param FileSystemAttr attribute value interpreted by driver
 *      It is ignored in basic file system
 * @param phFileSystem address of Basic file system driver instance handle
 *
 * @retval NvError_Success No Error
 * @retval NvError_InvalidParameter Illegal parameter value
 * @NvError_InsufficientMemory Memory allocation failed
 */

NvError
NvYaffs2FileSystemMount(
    NvPartitionHandle hPart,
    NvDdkBlockDevHandle hDevice,
    NvFsMountInfo *pMountInfo,
    NvU32 FileSystemAttr,
    NvFileSystemHandle *phFileSystem);

/**
 * Shutdown Yaffs2 File System driver
 */
void
NvYaffs2FileSystemDeinit(void);

/**
 * Erases the partition NvPartitionName. Used to erase partitions without
 * falling back to NvFlash.
 *
 * @param NvPartitionName Name of the partition to erase.
 * @retval NvSuccess If successful, or the appropriate error code.
 */
NvError NvYaffs2PrivNandErasePartition(
    const char   *NvPartitionName);

/**
 * Enables/disable skipping of tag data while writing.
 *
 * @param SkipTagWrite If SkipTagWrite is true, then data stream while writing will be
 * treated as page data only, no tag data.
 * If SkipTagWrite is false, then data stream will seen as page data + tag data.
 */
void NvYaffs2PrivNandYaffsSkipTagWrite(NvBool SkipTagWrite);

/**
 * Flashes a YAFFS-formatted partition image to the partition NvPartitionName.
 *
 * @param NvPartitionName Name of the partition to flash.
 * @param pLayout Organization of the NAND page and tag data in the YAFFS image.
 * @param pBuff A pointer to the data to be flashed.
 * @param DataBytes Length of the data (just counting the actual NAND pages) to
 *   be flashed. Must be an even multiple of the page size.
 * @param NextPageToWrite A pointer to the next page to write.
 * @retval NvSuccess If successful, or the appropriate error code.
 */
NvError NvYaffs2PrivNandYaffsWrite(
    const char               *NvPartitionName,
    const NvYaffs2Layout     *pLayout,
    const NvU8               *pBuff,
    NvU32                    DataBytes,
    NvU64                    *NextPageToWrite);


/**
 * Reads  the YAFFS-formatted partition in MTD format (only data).
 *
 * @param NvPartitionName Name of the partition to flash.
 * @param pLayout Organization of the NAND page and tag data in the YAFFS image.
 * @param pBuff A pointer to store the data.
 * @param DataBytes Length of the data (just counting the actual NAND pages) to
 *   be read. Must be an even multiple of the page size.
 * @param NextPageToRead A pointer to the next page to read.
 * @retval NvSuccess If successful, or the appropriate error code.
 */
NvError NvYaffs2PrivNandYaffsRead(
    const char               *NvPartitionName,
    const NvYaffs2Layout     *pLayout,
    NvU8               *pBuff,
    NvU32                    DataBytes,
    NvU64                    *NextPageToRead);

/**
 * Converts logical block address to phyusical block address.
 *
 * @param NvPartitionName Name of the partition to flash.
 * @param pLayout Organization of the NAND page and tag data in the YAFFS image.
 * @param logicalpage The logical page.
 * @param physicalpage The correspondig physical page
 * @retval NvSuccess If successful, or the appropriate error code.
 */
NvError NvYaffs2PrivNandYaffsMapLba2Pba(
    const char               *NvPartitionName,
    const NvYaffs2Layout     *pLayout,
    NvU64               logicalpage,
    NvU64                *physicalpage);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef YAFFS2FS_H */
