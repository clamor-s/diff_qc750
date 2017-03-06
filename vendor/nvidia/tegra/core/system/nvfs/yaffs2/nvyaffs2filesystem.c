/**
 * Copyright (c) 2008-2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#define NV_ENABLE_DEBUG_PRINTS 0

#include "nvos.h"
#include "nvassert.h"
#include "nvyaffs2filesystem.h"
#include "nvpartmgr_defs.h"

static NvOsMutexHandle s_NvYaffs2FSMutex;
static NvU8 *s_buffer = NULL;
static NvU32 s_bufferSize = 0, s_bytesToWrite = 0;
static NvU8 *g_ReadBuffer = 0;
static NvU32 Consumed = 0;
static NvU32 BlockSize =0;
static NvU64 CurrentPage = 0;

#define YAFFS_BUFFER_PAGES 1000

typedef struct NvYaffs2FileSystemRec
{
    NvFileSystem hFileSystem;
    NvPartitionHandle hPartition;
    NvDdkBlockDevHandle hBlkDevHandle;
    char PartName[NVPARTMGR_MOUNTPATH_NAME_LENGTH];
}NvYaffs2FileSystem;

typedef struct NvYaffs2FileSystemFileRec
{
    NvFileSystemFile hFileSystemFile;
    NvYaffs2FileSystem *pNvYaffs2FileSystem;
    NvU64 FileCurrPageOffset;
    NvU32 Flags;
}NvYaffs2FileSystemFile;


static NvYaffs2Layout gs_YaffsLayout =
{
    2048,
    16,
    48,
};

NvError NvYaffs2FileSystemInit(void)
{
    NvError e = NvSuccess;
    if (!s_NvYaffs2FSMutex)
    {
        NV_CHECK_ERROR_CLEANUP(NvOsMutexCreate(&s_NvYaffs2FSMutex));
    }

    return e;

fail:
    s_NvYaffs2FSMutex = NULL;
    return e;
}

void NvYaffs2FileSystemDeinit(void)
{
    NvOsMutexDestroy(s_NvYaffs2FSMutex);
    s_NvYaffs2FSMutex = 0;
}

static NvError NvFileSystemUnmount(NvFileSystemHandle hFileSystem)
{
    if (!hFileSystem)
    {
        return NvSuccess;
    }

    NvOsMutexLock(s_NvYaffs2FSMutex);
    NvOsFree(hFileSystem);
    NvOsMutexUnlock(s_NvYaffs2FSMutex);

    return NvSuccess;
}

static NvError NvFileSystemFormat(NvFileSystemHandle hFileSystem,
    NvU64 StartLogicalSector, NvU64 NumLogicalSectors,
    NvU32 PTendSector, NvBool IsPTpartition)
{
    NvError e = NvSuccess;
#if !NVOS_IS_WINDOWS
    NvYaffs2FileSystem *pNvYaffs2FileSystem = (NvYaffs2FileSystem*)hFileSystem;

    NV_CHECK_ERROR_CLEANUP(
        NvYaffs2PrivNandErasePartition(pNvYaffs2FileSystem->PartName)
    );

fail:
#endif
    return e;
}

static NvError
NvFileSystemFileClose(
    NvFileSystemFileHandle hFile)
{
    NvError e = NvSuccess;
#if !NVOS_IS_WINDOWS
    NvU64 CurrPage;
    NvYaffs2FileSystem *pNvYaffs2FileSystem = NULL;
    NvYaffs2FileSystemFile *pNvYaffs2FileSystemFile =
        (NvYaffs2FileSystemFile*)hFile;

    NvOsMutexLock(s_NvYaffs2FSMutex);
    pNvYaffs2FileSystem = pNvYaffs2FileSystemFile->pNvYaffs2FileSystem;
    //this fixes the situation in which only 1 read and 1 write operations is going on and 
    //not multuple write operations
    if ((s_bytesToWrite) && (pNvYaffs2FileSystemFile->Flags == NVOS_OPEN_WRITE))
    {
        NvU32 YaffsSize = gs_YaffsLayout.SkipSize +
            gs_YaffsLayout.PageSize + gs_YaffsLayout.TagSize;
        NvU32 DataPages =
            (s_bytesToWrite + YaffsSize-1) / YaffsSize;
        NvU32 DataBytes = DataPages * gs_YaffsLayout.PageSize;

        CurrPage = pNvYaffs2FileSystemFile->FileCurrPageOffset;

        NV_CHECK_ERROR_CLEANUP(
            NvYaffs2PrivNandYaffsWrite(pNvYaffs2FileSystem->PartName, &gs_YaffsLayout,
                s_buffer, DataBytes, &CurrPage)
        );
        s_bytesToWrite =0;
    }

fail:
    NvOsMutexUnlock(s_NvYaffs2FSMutex);
#endif
    NvOsFree(hFile);
    NvOsFree(s_buffer);
    s_buffer = 0;
    return e;
}



static NvError ReadBlock(
    NvYaffs2FileSystem *pNvYaffs2FileSystem,
    void *pBuffer,
    NvU32 BytesToRead)
{
    NvError e = NvSuccess;
    NvU32 DataPages;
    NvU32 DataBytes;

    DataPages = (BytesToRead + gs_YaffsLayout.PageSize-1) / gs_YaffsLayout.PageSize;
    DataBytes = DataPages * gs_YaffsLayout.PageSize;


    NV_CHECK_ERROR_CLEANUP(
        NvYaffs2PrivNandYaffsRead(pNvYaffs2FileSystem->PartName, &gs_YaffsLayout,
            pBuffer, DataBytes, &CurrentPage)
    );

fail:
    return e;
}

static NvError
NvFileSystemFileRead(
    NvFileSystemFileHandle hFile,
    void *pBuffer,
    NvU32 BytesToRead,
    NvU32 *BytesRead)
{
    NvError e = NvSuccess;
#if !NVOS_IS_WINDOWS
    NvYaffs2FileSystem *pNvYaffs2FileSystem = NULL;
    NvYaffs2FileSystemFile *pNvYaffs2FileSystemFile =
        (NvYaffs2FileSystemFile*)hFile;
    NvU32 read = 0;

    NV_ASSERT(hFile);
    NV_ASSERT(pBuffer);
    NV_ASSERT(BytesRead);

    NvOsMutexLock(s_NvYaffs2FSMutex);
    pNvYaffs2FileSystem =  pNvYaffs2FileSystemFile->pNvYaffs2FileSystem;
    if (!g_ReadBuffer)
    {
        NvDdkBlockDevHandle hBlkDev = pNvYaffs2FileSystem->hBlkDevHandle;
        NvDdkBlockDevInfo BlockDevInfo;

        hBlkDev->NvDdkBlockDevGetDeviceInfo(hBlkDev, &BlockDevInfo);
        BlockSize = BlockDevInfo.SectorsPerBlock * BlockDevInfo.BytesPerSector;
        g_ReadBuffer =  NvOsAlloc(BlockSize);

        if (!g_ReadBuffer)
        {
            e = NvError_InsufficientMemory;
            goto fail;
        }
        Consumed = BlockSize;
    }
    while (read < BytesToRead)
    {
        if (Consumed < BlockSize)
        {
            NvU32 avail = BlockSize - Consumed;
            NvU32 copy = (BytesToRead - read )< avail ? (BytesToRead - read) : avail;

            NvOsMemcpy((NvU8*)pBuffer + read, g_ReadBuffer + Consumed, copy);
            Consumed += copy;
            read += copy;

        }

        // Read complete blocks directly into the user's buffer
        while (Consumed  == BlockSize && (BytesToRead - read) >= BlockSize)
        {
            //if (read_block(ctx->partition, ctx->fd, data + read)) return -1;
            NV_CHECK_ERROR_CLEANUP(
                ReadBlock(pNvYaffs2FileSystem, ((NvU8 *)pBuffer+read), BlockSize)
            );
            read += BlockSize;
        }

        if (read >= BytesToRead) {
            goto fail;
        }

        // Read the next block into the buffer
        if (Consumed == BlockSize && read < BytesToRead) {
            NV_CHECK_ERROR_CLEANUP(
                ReadBlock(pNvYaffs2FileSystem, g_ReadBuffer, BlockSize)
            );
            Consumed = 0;
        }
    }

fail:
    NvOsMutexUnlock(s_NvYaffs2FSMutex);
#endif

    if (!e)
        *BytesRead = BytesToRead;
    else
        *BytesRead = 0;

    return e;
}

static NvError
NvFileSystemFileWrite(
    NvFileSystemFileHandle hFile,
    const void *pBuffer,
    NvU32 BytesToWrite,
    NvU32 *BytesWritten)
{
    NvError e = NvSuccess;
#if !NVOS_IS_WINDOWS
    NvYaffs2FileSystem *pNvYaffs2FileSystem = NULL;
    NvYaffs2FileSystemFile *pNvYaffs2FileSystemFile =
        (NvYaffs2FileSystemFile*)hFile;
    NvU32 YaffsSize = gs_YaffsLayout.SkipSize +
        gs_YaffsLayout.PageSize + gs_YaffsLayout.TagSize;
    NvU32 DataPages =
        (s_bufferSize + YaffsSize-1) / YaffsSize;
    NvU32 DataBytes = DataPages * gs_YaffsLayout.PageSize;
    NvU64 CurrPage;
    NvU32 bytes = BytesToWrite;
    NvU32 remainder = 0;
    NvU8 *tempBuf = (NvU8*)pBuffer;

    NvOsMutexLock(s_NvYaffs2FSMutex);
    pNvYaffs2FileSystem = pNvYaffs2FileSystemFile->pNvYaffs2FileSystem;

    //If we're using a massive buffer, then we assume this is
    //the fastboot write case (fastboot writes the entire
    //image in one shot.
    if (BytesToWrite > s_bufferSize)
    {
        DataPages = (BytesToWrite + YaffsSize-1) / YaffsSize;
        DataBytes = DataPages * gs_YaffsLayout.PageSize;

        CurrPage = 0;

        NV_CHECK_ERROR_CLEANUP(
            NvYaffs2PrivNandYaffsWrite(pNvYaffs2FileSystem->PartName, &gs_YaffsLayout,
                pBuffer, DataBytes, &CurrPage)
        );

        goto done;
    }

    if ((s_bytesToWrite+BytesToWrite) > s_bufferSize)
    {
        remainder = s_bytesToWrite + BytesToWrite - s_bufferSize;
        bytes -= remainder;
    }

    NvOsMemcpy(&s_buffer[s_bytesToWrite], pBuffer, bytes);

    s_bytesToWrite += BytesToWrite;
    if (s_bytesToWrite >= s_bufferSize)
    {
        CurrPage = pNvYaffs2FileSystemFile->FileCurrPageOffset;

        NV_CHECK_ERROR_CLEANUP(
            NvYaffs2PrivNandYaffsWrite(pNvYaffs2FileSystem->PartName, &gs_YaffsLayout,
                s_buffer, DataBytes, &CurrPage)
        );

        pNvYaffs2FileSystemFile->FileCurrPageOffset = CurrPage;

        if (remainder)
            NvOsMemcpy(s_buffer, &tempBuf[BytesToWrite-remainder], remainder);

        s_bytesToWrite = remainder;
    }

done:
fail:
    NvOsMutexUnlock(s_NvYaffs2FSMutex);
#endif

    if (!e)
        *BytesWritten = BytesToWrite;
    else
        *BytesWritten = 0;

    return e;
}


static NvError
NvFileSystemFileSeek(
    NvFileSystemFileHandle hFile,
    NvS64 ByteOffset,
    NvU32 Whence)
{
    NvError e = NvSuccess;
    NvYaffs2FileSystem *pNvYaffs2FileSystem = NULL;
    NvYaffs2FileSystemFile *pNvYaffs2FileSystemFile =
        (NvYaffs2FileSystemFile*)hFile;

    NvOsMutexLock(s_NvYaffs2FSMutex);
    NV_ASSERT(!(ByteOffset & (BlockSize -1)));
    NV_ASSERT(Whence == NvOsSeek_Set);

    pNvYaffs2FileSystem = pNvYaffs2FileSystemFile->pNvYaffs2FileSystem;

    CurrentPage = ByteOffset/gs_YaffsLayout.PageSize;
    Consumed = BlockSize;

    NV_CHECK_ERROR_CLEANUP(
        NvYaffs2PrivNandYaffsMapLba2Pba(pNvYaffs2FileSystem->PartName, &gs_YaffsLayout,
            CurrentPage, &CurrentPage)
    );

    pNvYaffs2FileSystemFile->FileCurrPageOffset = CurrentPage;
fail:
    return e;
}

static NvError
NvFileSystemFileIoctl(
     NvFileSystemFileHandle hFile,
     NvFileSystemIoctlType IoctlType,
     NvU32 InputSize,
     NvU32 OutputSize,
     const void * InputArgs,
     void *OutputArgs)
{
    NvError Err = NvSuccess;
    NV_ASSERT(InputArgs);
    switch (IoctlType)
    {
        case NvFileSystemIoctlType_WriteTagDataDisable:
        {
            NvFileSystemIoctl_WriteTagDataDisableInputArgs *WriteTagData =
                    (NvFileSystemIoctl_WriteTagDataDisableInputArgs *)InputArgs;
            NvYaffs2PrivNandYaffsSkipTagWrite(WriteTagData->TagDataWriteDisable);
        }
        break;
        default:
            Err = NvSuccess;
    }
    return Err;
}

static NvError
NvFileSystemFileOpen(
    NvFileSystemHandle hFileSystem,
    const char *FilePath,
    NvU32 Flags,
    NvFileSystemFileHandle *phFile)
{
    NvError e = NvSuccess;
#if !NVOS_IS_WINDOWS
    NvYaffs2FileSystemFile *pNvYaffs2FileSystemFile = NULL;
    NvU32 YaffsSize = gs_YaffsLayout.SkipSize +
        gs_YaffsLayout.PageSize + gs_YaffsLayout.TagSize;

    NvOsMutexLock(s_NvYaffs2FSMutex);
    pNvYaffs2FileSystemFile = NvOsAlloc(sizeof(NvYaffs2FileSystemFile));

    if (!pNvYaffs2FileSystemFile)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    pNvYaffs2FileSystemFile->hFileSystemFile.NvFileSystemFileClose = NvFileSystemFileClose;
    pNvYaffs2FileSystemFile->hFileSystemFile.NvFileSystemFileRead = NvFileSystemFileRead;
    pNvYaffs2FileSystemFile->hFileSystemFile.NvFileSystemFileWrite = NvFileSystemFileWrite;
    pNvYaffs2FileSystemFile->hFileSystemFile.NvFileSystemFileIoctl = NvFileSystemFileIoctl;
    pNvYaffs2FileSystemFile->hFileSystemFile.NvFileSystemFileSeek = NvFileSystemFileSeek;
    pNvYaffs2FileSystemFile->hFileSystemFile.NvFileSystemFtell = NULL;
    pNvYaffs2FileSystemFile->hFileSystemFile.NvFileSystemFileQueryFstat = NULL;

    pNvYaffs2FileSystemFile->FileCurrPageOffset = 0;
    pNvYaffs2FileSystemFile->pNvYaffs2FileSystem = (NvYaffs2FileSystem*)hFileSystem;
    pNvYaffs2FileSystemFile->Flags = Flags;
    *phFile = &pNvYaffs2FileSystemFile->hFileSystemFile;

    if(Flags == NVOS_OPEN_WRITE)
    {
        NV_CHECK_ERROR_CLEANUP(
            NvYaffs2PrivNandErasePartition( pNvYaffs2FileSystemFile->pNvYaffs2FileSystem->PartName)
        );
        s_bytesToWrite = 0;
    }
    else
    {
        if (g_ReadBuffer)
        {
            NvOsFree(g_ReadBuffer);
            g_ReadBuffer = 0;
            Consumed = BlockSize;
            CurrentPage = 0;
        }
    }


    s_bufferSize = YaffsSize * YAFFS_BUFFER_PAGES;
    s_buffer = NvOsAlloc(s_bufferSize);

    if (!s_buffer)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

fail:
    NvOsMutexUnlock(s_NvYaffs2FSMutex);
#endif
    return e;
}

static NvError
NvFileSystemFileQueryStat(
    NvFileSystemHandle hFileSystem,
    const char *FilePath,
    NvFileStat *pStat)
{
    NvError Err = NvSuccess;
    NvDdkBlockDevInfo BlockDevInfo;
    NvYaffs2FileSystem *pNvYaffs2FileSystem = (NvYaffs2FileSystem*)hFileSystem;
    NvDdkBlockDevHandle hBlkDev = NULL;


    NV_ASSERT(hFileSystem);
    NV_ASSERT(pStat);
    hBlkDev = pNvYaffs2FileSystem->hBlkDevHandle;
    hBlkDev->NvDdkBlockDevGetDeviceInfo(hBlkDev, &BlockDevInfo);
    if (!BlockDevInfo.TotalBlocks)
    {
        Err = NvError_BadParameter;
        NV_DEBUG_PRINTF(("NvFileSystemFileQueryStat Failed. Either device"
                " is not present or its not responding. \n"));
        goto Fail;
    }
    // File size is not supported in Basic File system so
    // give the patition size in bytes as file size
    pStat->Size = ((BlockDevInfo.BytesPerSector) *
            (pNvYaffs2FileSystem->hPartition->NumLogicalSectors));
    return NvSuccess;
Fail:
    pStat->Size = 0;
    return Err;
}

static NvError
NvFileSystemFileSystemQueryStat(
    NvFileSystemHandle hFileSystem,
    const char *FilePath,
    NvFileSystemStat *pStat)
{
    return NvError_NotSupported;
}

NvError
NvYaffs2FileSystemMount(
    NvPartitionHandle hPart,
    NvDdkBlockDevHandle hDevice,
    NvFsMountInfo *pMountInfo,
    NvU32 FileSystemAttr,
    NvFileSystemHandle *phFileSystem)
{
    NvError e = NvSuccess;
    NvYaffs2FileSystem *pNvYaffs2FileSystem = NULL;

    if (!hPart || !hDevice)
    {
        e = NvError_BadParameter;
        goto fail;
    }

    NvOsMutexLock(s_NvYaffs2FSMutex);
    pNvYaffs2FileSystem = NvOsAlloc(sizeof(NvYaffs2FileSystem));
    if (!pNvYaffs2FileSystem)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    pNvYaffs2FileSystem->hFileSystem.NvFileSystemUnmount =
                            NvFileSystemUnmount;
    pNvYaffs2FileSystem->hFileSystem.NvFileSystemFileOpen =
                            NvFileSystemFileOpen;
    pNvYaffs2FileSystem->hFileSystem.NvFileSystemFileQueryStat =
                            NvFileSystemFileQueryStat;
    pNvYaffs2FileSystem->hFileSystem.NvFileSystemFileSystemQueryStat =
                            NvFileSystemFileSystemQueryStat;

    pNvYaffs2FileSystem->hFileSystem.NvFileSystemFormat = NvFileSystemFormat;

    pNvYaffs2FileSystem->hPartition = hPart;
    pNvYaffs2FileSystem->hBlkDevHandle = hDevice;
    NvOsStrncpy(pNvYaffs2FileSystem->PartName, pMountInfo->MountPath,
        NVPARTMGR_MOUNTPATH_NAME_LENGTH);

    *phFileSystem = &pNvYaffs2FileSystem->hFileSystem;
    NvOsMutexUnlock(s_NvYaffs2FSMutex);
    return e;

fail:
    phFileSystem = NULL;
    NvOsMutexLock(s_NvYaffs2FSMutex);
    return e;
}

