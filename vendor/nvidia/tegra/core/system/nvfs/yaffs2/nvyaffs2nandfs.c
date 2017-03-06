/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/** nvaboot_nandfs.c
 *
 *   Utility functions for operating on Linux partition types that are stored
 *   on NAND flash media (e.g., YAFFS).
 */

#include "nvcommon.h"
#include "nvddk_nand.h"
#include "nvpartmgr.h"
#include "nvstormgr.h"
#include "nvddk_blockdevmgr.h"
#include "nvos.h"
#include "nvassert.h"
#include "nvyaffs2filesystem.h"

#define NDFLASH_CS_MAX 8

#define NAND_RUN_TIME_BAD_BLOCK_MARKER          0x0
#define NAND_RUN_TIME_BAD_BLOCK_MARKER_OFFSET   0x1

/* This is a test code to infuse some bad blocks, in an otherwise perfect NAND
 * device, for testing bad block management code in the OS driver.
 * */
#define INFUSE_BAD_BLOCKS   0
// Format partition takes long time hence disabling bad block detection test
#define ENABLE_BAD_BLOCK_TEST 0

static NvBool gs_skipTagWrite = NV_FALSE;
static NvU8               *s_pTagBuff = NULL;

static NvError
NvYaffs2PrivGetStartPhysicalPage(
       NvU32         PartitionId,
       NvU64         LogicalPage,
       NvU64         *PhysicalPage);

static NvError NvYaffs2PrivNandYaffsVerify(
    NvDdkNandHandle           hNand,
    NvU32                     StartPhysicalPage,
    NvU32                     MaxPhysicalPage,
    const NvYaffs2Layout      *pLayout,
    const NvU8                *pBuff,
    NvU32                     DataBytes);


NvError
NvYaffs2PrivGetStartPhysicalPage(
       NvU32         PartitionId,
       NvU64         LogicalPage,
       NvU64         *PhysicalPage)
{
    NvDdkBlockDevIoctl_GetPartitionPhysicalSectorsInputArgs InArgs;
    NvDdkBlockDevIoctl_GetPartitionPhysicalSectorsOutputArgs OutArgs;
    NvFsMountInfo       minfo;
    NvDdkBlockDevHandle hDev = NULL;
    NvError             e;

    NV_CHECK_ERROR_CLEANUP(NvPartMgrGetFsInfo( PartitionId, &minfo ));

    NV_CHECK_ERROR_CLEANUP(
        NvDdkBlockDevMgrDeviceOpen( (NvDdkBlockDevMgrDeviceId)minfo.DeviceId,
            minfo.DeviceInstance, PartitionId, &hDev )
    );

    InArgs.PartitionLogicalSectorStart = (NvU32)LogicalPage;
    // FIXME: Since end physical unused we set stop logical sector same as start
    InArgs.PartitionLogicalSectorStop = InArgs.PartitionLogicalSectorStart;

    NV_CHECK_ERROR_CLEANUP(hDev->NvDdkBlockDevIoctl(hDev,
            NvDdkBlockDevIoctlType_GetPartitionPhysicalSectors,
            sizeof(NvDdkBlockDevIoctl_GetPartitionPhysicalSectorsInputArgs),
            sizeof(NvDdkBlockDevIoctl_GetPartitionPhysicalSectorsOutputArgs),
            &InArgs,
            &OutArgs)
    );

    *PhysicalPage = OutArgs.PartitionPhysicalSectorStart;
    // FIXME: if we need physical stop sector we may need to send
    // extra argument stop logical sector as argument

fail:
    if (hDev)
        hDev->NvDdkBlockDevClose( hDev );
    return e;
}


static
NvBool NvYaffs2PrivNandIsRuntimeBadBlock(
        NvDdkNandHandle hNand,
        NvU32 Device,
        NvU32 BlockIdx,
        NvDdkNandDeviceInfo *pNandDevInfo,
        NvU8 *pPageBuffer,
        NvU8 *pTagBuffer)
{
    NvU32 StartPage[NDFLASH_CS_MAX];
    NvU32 i;
    NvU8 BadBlockMarker;
#if ENABLE_BAD_BLOCK_TEST
    NvU32 NumPages;
    NvError e;
    NvU32 NumberOfBlocks = 1;
    NvU32 TestPattern;
    NvU32 checkAgain = 2;
    NvU32 page;
#endif

#if INFUSE_BAD_BLOCKS
    /* Add one bad block for every 64 blocks */
    if (!(BlockIdx % 64))
    {
        return NV_TRUE;
    }
#endif


    for (i=0; i<NDFLASH_CS_MAX; i++)
        StartPage[i] = ~0UL;

    StartPage[Device] = BlockIdx * pNandDevInfo->PagesPerBlock;
    NvDdkNandReadSpare(hNand, Device, StartPage, &BadBlockMarker, 1, 1);
    if (BadBlockMarker != 0xff)
        return NV_TRUE;

#if ENABLE_BAD_BLOCK_TEST
    while (checkAgain)
    {
        StartPage[Device] = BlockIdx * pNandDevInfo->PagesPerBlock;
        NV_CHECK_ERROR_CLEANUP(
            NvDdkNandErase(hNand, Device, StartPage, &NumberOfBlocks)
        );

        /* Erase is all 0xffffffff */
        TestPattern = 0xffffffff;
        for (page=0; page<pNandDevInfo->PagesPerBlock; page++)
        {
            NvOsMemset(pPageBuffer, ~TestPattern, pNandDevInfo->PageSize);
            NvOsMemset(pTagBuffer, ~TestPattern, pNandDevInfo->TagSize);
            StartPage[Device] = BlockIdx * pNandDevInfo->PagesPerBlock + page;
            NumPages = 1;
            NV_CHECK_ERROR_CLEANUP(
                    NvDdkNandRead(hNand, Device, StartPage, pPageBuffer,
                        pTagBuffer, &NumPages, NV_TRUE )
            );
            for (i=0; i< pNandDevInfo->PageSize; i+=4)
            {
                if (*(NvU32 *)(pPageBuffer + i) != TestPattern)
                {
                    return NV_TRUE;
                }
            }
            for (i=0; i< pNandDevInfo->TagSize; i++)
            {
                if (pTagBuffer[i] != (NvU8)TestPattern)
                {
                    return NV_TRUE;
                }
            }
        }

        if (checkAgain)
        {
            TestPattern = 0xa5a5a5a5;
        } else
        {
            TestPattern = 0x5a5a5a5a;
        }

        /* Clear with TestPattern, read back and veriify */
        NvOsMemset(pPageBuffer, TestPattern, pNandDevInfo->PageSize);
        NvOsMemset(pTagBuffer, TestPattern, pNandDevInfo->TagSize);
        for (page=0; page<pNandDevInfo->PagesPerBlock; page++)
        {
            StartPage[Device] = BlockIdx * pNandDevInfo->PagesPerBlock + page;
            NumPages = 1;
            NV_CHECK_ERROR_CLEANUP(
                    NvDdkNandWrite(hNand, Device, StartPage, pPageBuffer,
                        pTagBuffer, &NumPages)
            );
        }
        for (page=0; page<pNandDevInfo->PagesPerBlock; page++)
        {
            NvOsMemset(pPageBuffer, ~TestPattern, pNandDevInfo->PageSize);
            NvOsMemset(pTagBuffer, ~TestPattern, pNandDevInfo->TagSize);
            StartPage[Device] = BlockIdx * pNandDevInfo->PagesPerBlock + page;
            NumPages = 1;
            NV_CHECK_ERROR_CLEANUP(
                    NvDdkNandRead(hNand, Device, StartPage, pPageBuffer,
                        pTagBuffer, &NumPages, NV_TRUE)
            );
            for (i=0; i< pNandDevInfo->PageSize; i+=4)
            {
                if (*(NvU32 *)(pPageBuffer + i) != TestPattern)
                {
                    return NV_TRUE;
                }
            }
            for (i=0; i< pNandDevInfo->TagSize; i++)
            {
                if (pTagBuffer[i] != (NvU8)TestPattern)
                {
                    return NV_TRUE;
                }
            }
        }
        checkAgain--;
    }
#endif // ENABLE_BAD_BLOCK_TEST

    return NV_FALSE;
#if ENABLE_BAD_BLOCK_TEST
 fail:
    return NV_TRUE;
#endif
}

NvError NvYaffs2PrivNandYaffsVerify(
    NvDdkNandHandle           hNand,
    NvU32                     StartPhysicalPage,
    NvU32                     MaxPhysicalPage,
    const NvYaffs2Layout      *pLayout,
    const NvU8                *pBuff,
    NvU32                     DataBytes)
{
    NvU8               *pPageBuffer = NULL;
    NvDdkNandDeviceInfo NandDevInfo;
    NvU32               StartPage[NDFLASH_CS_MAX];
    NvU32               PagesPerDevice;
    NvU32               CurrPage;
    NvU32               Remain;
    NvU32               MaxPage;
    NvU32               NumPagesToRead = 1;
    NvU32               i;
    NvError             e;

    if (!hNand || !pBuff || !DataBytes)
        return NvError_BadParameter;

    for (i=0; i<NDFLASH_CS_MAX; i++)
        StartPage[i] = ~0UL;

    NV_CHECK_ERROR_CLEANUP(NvDdkNandGetDeviceInfo(hNand, 0, &NandDevInfo));

    if (pLayout->PageSize != NandDevInfo.PageSize ||
        pLayout->TagSize > NandDevInfo.TagSize)
    {
        e = NvError_FileWriteFailed;
        goto fail;
    }

    PagesPerDevice = NandDevInfo.PagesPerBlock * NandDevInfo.NoOfBlocks;

    if (!s_pTagBuff)
    {
        s_pTagBuff= (NvU8*)NvOsAlloc((NandDevInfo.TagSize + 63) & ~63);
        if (!s_pTagBuff)
        {
            e = NvError_InsufficientMemory;
            goto fail;
        }
    }
    pPageBuffer = (NvU8*)NvOsAlloc(NandDevInfo.PageSize);
    if (!pPageBuffer)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    NvOsMemset(s_pTagBuff, 0xff, NandDevInfo.TagSize);

    CurrPage = StartPhysicalPage;
    MaxPage = MaxPhysicalPage;
    Remain = (DataBytes+NandDevInfo.PageSize-1)/NandDevInfo.PageSize;

    while (Remain && CurrPage<MaxPage)
    {
        NvU32 Device    = CurrPage / PagesPerDevice;
        NvU32 PageIdx   = CurrPage - (Device*PagesPerDevice);
        NvU32 CurrBlock = PageIdx / NandDevInfo.PagesPerBlock;
        NvU32 PageOfs   = PageIdx - (CurrBlock*NandDevInfo.PagesPerBlock);
        NandBlockInfo BlockInfo;
        NvU8 BadBlockMarker;
        NvU8 BadBlockMarkerOffset  = NAND_RUN_TIME_BAD_BLOCK_MARKER_OFFSET;

        BlockInfo.IsFactoryGoodBlock = NV_FALSE;
        BlockInfo.IsBlockLocked = NV_FALSE;
        BlockInfo.pTagBuffer = s_pTagBuff;

        NvDdkNandGetBlockInfo(hNand, Device, CurrBlock, &BlockInfo, NV_FALSE);
        /* Skip factory bad blocks */
        if (!BlockInfo.IsFactoryGoodBlock)
        {
            CurrPage += (NandDevInfo.PagesPerBlock - PageOfs);
            continue;
        }

        /* Skip run-time bad blocks */
        StartPage[Device] = PageIdx;
        NvDdkNandReadSpare(hNand, Device, StartPage, &BadBlockMarker,
                        BadBlockMarkerOffset, 1);
        StartPage[Device] = ~0UL;
        if (BadBlockMarker != 0xFF)
        {
            CurrPage += (NandDevInfo.PagesPerBlock - PageOfs);
            continue;
        }

        /* Block is good Read and verify that the data is written correctly */
        for ( ; Remain && PageOfs<NandDevInfo.PagesPerBlock &&
                  CurrPage<MaxPage; )
        {
            NumPagesToRead = 1;
            NvOsMemset(s_pTagBuff, 0xff, NandDevInfo.TagSize);
            StartPage[Device] = PageIdx;
            NV_CHECK_ERROR_CLEANUP(
                NvDdkNandRead(hNand, Device, StartPage, pPageBuffer,
                    s_pTagBuff, &NumPagesToRead, NV_TRUE)
            );

            for (i=0; i < pLayout->PageSize; i++)
            {
                if (pPageBuffer[i] != pBuff[i])
                {
                    e = NvError_NandOperationFailed;
                    goto fail;
                }
            }

            for (i=0; i < pLayout->TagSize; i++)
            {
                if (s_pTagBuff[i] != pBuff[i + pLayout->PageSize])
                {
                    e = NvError_NandOperationFailed;
                    goto fail;
                }
            }

            PageOfs++;
            PageIdx++;
            CurrPage++;
            Remain--;
            pBuff += (pLayout->TagSize + pLayout->SkipSize + pLayout->PageSize);
        }
        StartPage[Device] = ~0UL;
    }

    if (Remain)
        e = NvError_FileWriteFailed;
    else
        e = NvSuccess;
 fail:
    NvOsFree(pPageBuffer);

    return e;
}

NvError NvYaffs2PrivNandYaffsMapLba2Pba(
    const char               *NvPartitionName,
    const NvYaffs2Layout     *pLayout,
    NvU64               logicalpage,
    NvU64                *physicalpage)
{
    NvRmDeviceHandle    hRm = NULL;
    NvDdkNandHandle     hNand = NULL;
    NvDdkNandDeviceInfo NandDevInfo;
    NvPartInfo          Partition;
    NvU32               StartPage[NDFLASH_CS_MAX];
    NvU32               PartitionId;
    NvU32               PagesPerDevice;
    NvU64               CurrPage;
    NvU64               MaxPage;
    NvU64               Remain;
    NvU32               i;
    NvError             e;

    if (!physicalpage)
        return NvError_BadParameter;

    for (i=0; i<NDFLASH_CS_MAX; i++)
        StartPage[i] = ~0UL;

    NV_CHECK_ERROR_CLEANUP(NvPartMgrGetIdByName(NvPartitionName, &PartitionId));
    NV_CHECK_ERROR_CLEANUP(NvPartMgrGetPartInfo(PartitionId, &Partition));

    CurrPage = Partition.StartLogicalSectorAddress;

    //Convert the logical to a physical address
    NV_CHECK_ERROR_CLEANUP(
        NvYaffs2PrivGetStartPhysicalPage(PartitionId,
           CurrPage, &CurrPage)
    );

    //This is not strictly correct
    MaxPage = CurrPage + Partition.NumLogicalSectors;

    NV_CHECK_ERROR_CLEANUP(NvRmOpen(&hRm, 0));
    NV_CHECK_ERROR_CLEANUP(NvDdkNandOpen(hRm, &hNand));
    NV_CHECK_ERROR_CLEANUP(NvDdkNandGetDeviceInfo(hNand, 0, &NandDevInfo));

    if (pLayout->PageSize != NandDevInfo.PageSize ||
        pLayout->TagSize > NandDevInfo.TagSize)
    {
        e = NvError_FileReadFailed;
        goto fail;
    }

    if (Partition.StartLogicalSectorAddress & (NandDevInfo.PagesPerBlock - 1))
    {
        /* A partiton should always start on block boundary */
        e = NvError_BadParameter;
        goto fail;
    }

    PagesPerDevice = NandDevInfo.PagesPerBlock * NandDevInfo.NoOfBlocks;

    if (!s_pTagBuff)
    {
        s_pTagBuff = (NvU8*)NvOsAlloc((NandDevInfo.TagSize + 63) & ~63);
        if (!s_pTagBuff)
        {
            e = NvError_InsufficientMemory;
            goto fail;
        }
    }
    NvOsMemset(s_pTagBuff, 0xff, NandDevInfo.TagSize);

    Remain = logicalpage;

    while (Remain && CurrPage<MaxPage)
    {
        NvU32 Device    = (NvU32)(CurrPage / PagesPerDevice);
        NvU32 PageIdx   = (NvU32)(CurrPage - (Device*PagesPerDevice));
        NvU32 CurrBlock = PageIdx / NandDevInfo.PagesPerBlock;
        NandBlockInfo BlockInfo;
        NvU8 BadBlockMarker;
        NvU8 BadBlockMarkerOffset  = NAND_RUN_TIME_BAD_BLOCK_MARKER_OFFSET;

        BlockInfo.IsFactoryGoodBlock = NV_FALSE;
        BlockInfo.IsBlockLocked = NV_FALSE;
        BlockInfo.pTagBuffer = s_pTagBuff;

        NvDdkNandGetBlockInfo(hNand, Device, CurrBlock, &BlockInfo, NV_FALSE);
        /* Skip factory bad blocks */
        if (!BlockInfo.IsFactoryGoodBlock)
        {
            CurrPage += NandDevInfo.PagesPerBlock;
            continue;
        }

        /* Skip run-time bad blocks */
        StartPage[Device] = PageIdx;
        BadBlockMarker = 0;
        NvDdkNandReadSpare(hNand, Device, StartPage, &BadBlockMarker,
                        BadBlockMarkerOffset, 1);
        StartPage[Device] = ~0UL;
        if (BadBlockMarker != 0xFF)
        {
            CurrPage += NandDevInfo.PagesPerBlock;
            continue;
        }

        /* Block is good */
        if(Remain < NandDevInfo.PagesPerBlock)
        {
            CurrPage += Remain;
            Remain = 0;
        }
        else
        {
            CurrPage += NandDevInfo.PagesPerBlock;
            Remain -= NandDevInfo.PagesPerBlock;
        }

    }

    if (Remain)
        e = NvError_FileReadFailed;
    else
        e = NvSuccess;

    if (e == NvSuccess)
    {
        *physicalpage = CurrPage;
    }

 fail:
    NvDdkNandClose(hNand);
    NvRmClose(hRm);

    return e;
}


NvError NvYaffs2PrivNandYaffsRead(
    const char               *NvPartitionName,
    const NvYaffs2Layout     *pLayout,
    NvU8               *pBuff,
    NvU32                    DataBytes,
    NvU64                    *NextPageToRead)
{
    NvRmDeviceHandle    hRm = NULL;
    NvDdkNandHandle     hNand = NULL;
    NvDdkNandDeviceInfo NandDevInfo;
    NvPartInfo          Partition;
    NvU32               StartPage[NDFLASH_CS_MAX];
    NvU32               PartitionId;
    NvU32               PagesPerDevice;
    NvU64               CurrPage;
    NvU64               MaxPage;
    NvU32               Remain;
    NvU32               NumPagesToRead = 1;
    NvU32               i;
    NvError             e;

    if (!NvPartitionName || !pBuff || !DataBytes || !NextPageToRead)
        return NvError_BadParameter;

    for (i=0; i<NDFLASH_CS_MAX; i++)
        StartPage[i] = ~0UL;

    NV_CHECK_ERROR_CLEANUP(NvPartMgrGetIdByName(NvPartitionName, &PartitionId));
    NV_CHECK_ERROR_CLEANUP(NvPartMgrGetPartInfo(PartitionId, &Partition));

    CurrPage = Partition.StartLogicalSectorAddress;

    //Convert the logical to a physical address
    NV_CHECK_ERROR_CLEANUP(
        NvYaffs2PrivGetStartPhysicalPage(PartitionId,
           CurrPage, &CurrPage)
    );

    //This is not strictly correct
    MaxPage = CurrPage + Partition.NumLogicalSectors;

    if (*NextPageToRead)
        CurrPage = (NvU32)*NextPageToRead;

    NV_CHECK_ERROR_CLEANUP(NvRmOpen(&hRm, 0));
    NV_CHECK_ERROR_CLEANUP(NvDdkNandOpen(hRm, &hNand));
    NV_CHECK_ERROR_CLEANUP(NvDdkNandGetDeviceInfo(hNand, 0, &NandDevInfo));

    if (pLayout->PageSize != NandDevInfo.PageSize ||
        pLayout->TagSize > NandDevInfo.TagSize)
    {
        e = NvError_FileReadFailed;
        goto fail;
    }

    if (Partition.StartLogicalSectorAddress & (NandDevInfo.PagesPerBlock - 1))
    {
        /* A partiton should always start on block boundary */
        e = NvError_BadParameter;
        goto fail;
    }

    PagesPerDevice = NandDevInfo.PagesPerBlock * NandDevInfo.NoOfBlocks;

    if(!s_pTagBuff)
    {
        s_pTagBuff = (NvU8*)NvOsAlloc((NandDevInfo.TagSize + 63) & ~63);
        if (!s_pTagBuff)
        {
            e = NvError_InsufficientMemory;
            goto fail;
        }
    }
    NvOsMemset(s_pTagBuff, 0xff, NandDevInfo.TagSize);

    Remain = (DataBytes+NandDevInfo.PageSize-1)/NandDevInfo.PageSize;

    while (Remain && CurrPage<MaxPage)
    {
        NvU32 Device    = (NvU32)(CurrPage / PagesPerDevice);
        NvU32 PageIdx   = (NvU32)(CurrPage - (Device*PagesPerDevice));
        NvU32 CurrBlock = PageIdx / NandDevInfo.PagesPerBlock;
        NvU32 PageOfs   = PageIdx - (CurrBlock*NandDevInfo.PagesPerBlock);
        NandBlockInfo BlockInfo;
        NvU8 BadBlockMarker;
        NvU8 BadBlockMarkerOffset  = NAND_RUN_TIME_BAD_BLOCK_MARKER_OFFSET;

        NvU32 r, p, c;
        NvU8 *b;

        BlockInfo.IsFactoryGoodBlock = NV_FALSE;
        BlockInfo.IsBlockLocked = NV_FALSE;
        BlockInfo.pTagBuffer = s_pTagBuff;

        NvDdkNandGetBlockInfo(hNand, Device, CurrBlock, &BlockInfo, NV_FALSE);
        /* Skip factory bad blocks */
        if (!BlockInfo.IsFactoryGoodBlock)
        {
            CurrPage += (NandDevInfo.PagesPerBlock - PageOfs);
            continue;
        }

        /* Skip run-time bad blocks */
        StartPage[Device] = PageIdx;
        BadBlockMarker = 0;
        NvDdkNandReadSpare(hNand, Device, StartPage, &BadBlockMarker,
                        BadBlockMarkerOffset, 1);
        StartPage[Device] = ~0UL;
        if (BadBlockMarker != 0xFF)
        {
            CurrPage += (NandDevInfo.PagesPerBlock - PageOfs);
            continue;
        }

        /* Block is good start the write. If write fails, abandon the entire
         * block and skip over to next block.
         *
         * Shadow the variables to roll back, in case the write fails.
         */
        r = Remain;
        p = PageOfs;
        c = (NvU32)CurrPage;
        b = pBuff;
        for ( ; r && p <NandDevInfo.PagesPerBlock &&
                  r < MaxPage; )
        {
            NumPagesToRead = 1;
            NvOsMemset(s_pTagBuff, 0xff, NandDevInfo.TagSize);
            StartPage[Device] = PageIdx;
            e = NvDdkNandRead(hNand, Device, StartPage, b,
                    s_pTagBuff, &NumPagesToRead, NV_FALSE);
            if (e != NvSuccess)
            {
                    /* Mark this block bad by writting the marker to the
                     * spare area of the first page of this block and
                     * skip over to next block
                     * */
                    BadBlockMarker = NAND_RUN_TIME_BAD_BLOCK_MARKER;
                    StartPage[Device] = (NvU32)(CurrPage - (Device*PagesPerDevice));
                    //NvDdkNandWriteSpare(hNand, Device, StartPage, &BadBlockMarker,
                        //BadBlockMarkerOffset, 1);
                    //CurrPage += (NandDevInfo.PagesPerBlock - PageOfs);
                    while(1);
                    goto fail;;
            }

            p++;
            PageIdx++;
            c++;

            r--;
            b += pLayout->PageSize;
        }

        Remain = r;
        PageOfs = p;
        CurrPage = c;
        pBuff = b;

    }

    if (Remain)
        e = NvError_FileReadFailed;
    else
        e = NvSuccess;

    if (e == NvSuccess)
    {
        *NextPageToRead = (NvU64)CurrPage;
    }

 fail:
    NvDdkNandClose(hNand);
    NvRmClose(hRm);
    return e;
}

void NvYaffs2PrivNandYaffsSkipTagWrite(NvBool SkipTagWrite)
{
    gs_skipTagWrite = SkipTagWrite;
}

/**
 * NvYaffs2PrivNandYaffsWrite
 *   Writes a YAFFS image to NAND.  The layout of the YAFFS image (page size,
 *   tag area) must match the parameters of the targetted NAND device */

NvError NvYaffs2PrivNandYaffsWrite(
    const char               *NvPartitionName,
    const NvYaffs2Layout     *pLayout,
    const NvU8               *pBuff,
    NvU32                    DataBytes,
    NvU64                    *NextPageToWrite)
{
    NvRmDeviceHandle    hRm = NULL;
    NvDdkNandHandle     hNand = NULL;
    NvU8               *pPageBuffer = NULL;
    NvDdkNandDeviceInfo NandDevInfo;
    NvPartInfo          Partition;
    NvU32               StartPage[NDFLASH_CS_MAX];
    NvU32               PartitionId;
    NvU32               PagesPerDevice;
    NvU64               CurrPage;
    NvU64               StartPhysicalPage;
    NvU64               MaxPage;
    NvU64               MaxPhysicalPage;
    NvU32               Remain;
    NvU32               NumPagesToWrite = 1;
    NvU32               i;
    NvError             e;
    const               NvU8 *pUserBuffer = pBuff;

    if (!NvPartitionName || !pBuff || !DataBytes || !NextPageToWrite)
        return NvError_BadParameter;

    for (i=0; i<NDFLASH_CS_MAX; i++)
        StartPage[i] = ~0UL;

    NV_CHECK_ERROR_CLEANUP(NvPartMgrGetIdByName(NvPartitionName, &PartitionId));
    NV_CHECK_ERROR_CLEANUP(NvPartMgrGetPartInfo(PartitionId, &Partition));

    CurrPage = Partition.StartLogicalSectorAddress;

    //Convert the logical to a physical address
    NV_CHECK_ERROR_CLEANUP(
        NvYaffs2PrivGetStartPhysicalPage(PartitionId,
           CurrPage, &CurrPage)
    );

    //This is not strictly correct
    MaxPage = CurrPage + Partition.NumLogicalSectors;

    if (*NextPageToWrite)
        CurrPage = *NextPageToWrite;

    StartPhysicalPage = CurrPage;
    MaxPhysicalPage = MaxPage;

    NV_CHECK_ERROR_CLEANUP(NvRmOpen(&hRm, 0));
    NV_CHECK_ERROR_CLEANUP(NvDdkNandOpen(hRm, &hNand));
    NV_CHECK_ERROR_CLEANUP(NvDdkNandGetDeviceInfo(hNand, 0, &NandDevInfo));

    if (pLayout->PageSize != NandDevInfo.PageSize ||
        pLayout->TagSize > NandDevInfo.TagSize)
    {
        e = NvError_FileWriteFailed;
        goto fail;
    }

    if (Partition.StartLogicalSectorAddress & (NandDevInfo.PagesPerBlock - 1))
    {
        /* A partiton should always start on block boundary */
        e = NvError_BadParameter;
        goto fail;
    }

    PagesPerDevice = NandDevInfo.PagesPerBlock * NandDevInfo.NoOfBlocks;

    if(!s_pTagBuff)
    {
        s_pTagBuff = (NvU8*)NvOsAlloc((NandDevInfo.TagSize + 63) & ~63);
        if (!s_pTagBuff)
        {
            e = NvError_InsufficientMemory;
            goto fail;
        }
    }

    NvOsMemset(s_pTagBuff, 0xff, NandDevInfo.TagSize);

    Remain = (DataBytes+NandDevInfo.PageSize-1)/NandDevInfo.PageSize;

    while (Remain && CurrPage<MaxPage)
    {
        NvU32 Device    = (NvU32)(CurrPage / PagesPerDevice);
        NvU32 PageIdx   = (NvU32)(CurrPage - (Device*PagesPerDevice));
        NvU32 CurrBlock = PageIdx / NandDevInfo.PagesPerBlock;
        NvU32 PageOfs   = PageIdx - (CurrBlock*NandDevInfo.PagesPerBlock);
        NandBlockInfo BlockInfo;
        NvU8 BadBlockMarker;
        NvU8 BadBlockMarkerOffset  = NAND_RUN_TIME_BAD_BLOCK_MARKER_OFFSET;

        NvU32 r, p, c;
        const NvU8 *b;

        BlockInfo.IsFactoryGoodBlock = NV_FALSE;
        BlockInfo.IsBlockLocked = NV_FALSE;
        BlockInfo.pTagBuffer = s_pTagBuff;

        NvDdkNandGetBlockInfo(hNand, Device, CurrBlock, &BlockInfo, NV_FALSE);
        /* Skip factory bad blocks */
        if (!BlockInfo.IsFactoryGoodBlock)
        {
            CurrPage += (NandDevInfo.PagesPerBlock - PageOfs);
            continue;
        }

        /* Skip run-time bad blocks */
        StartPage[Device] = PageIdx;
        BadBlockMarker = 0;
        NvDdkNandReadSpare(hNand, Device, StartPage, &BadBlockMarker,
                        BadBlockMarkerOffset, 1);
        StartPage[Device] = ~0UL;
        if (BadBlockMarker != 0xFF)
        {
            CurrPage += (NandDevInfo.PagesPerBlock - PageOfs);
            continue;
        }

        /* Block is good start the write. If write fails, abandon the entire
         * block and skip over to next block.
         *
         * Shadow the variables to roll back, in case the write fails.
         */
        r = Remain;
        p = PageOfs;
        c = (NvU32)CurrPage;
        b = pBuff;
        for ( ; r && p <NandDevInfo.PagesPerBlock &&
                  r < MaxPage; )
        {
            NumPagesToWrite = 1;
            NvOsMemset(s_pTagBuff, 0xff, NandDevInfo.TagSize);
            StartPage[Device] = PageIdx;
            if(!gs_skipTagWrite)
                NvOsMemcpy(s_pTagBuff, &b[pLayout->PageSize], pLayout->TagSize);
            e = NvDdkNandWrite(hNand, Device, StartPage, b,
                    s_pTagBuff, &NumPagesToWrite);
            if (e != NvSuccess)
            {
                if (e == NvError_NandProgramFailed)
                {
                    /* Mark this block bad by writting the marker to the
                     * spare area of the first page of this block and
                     * skip over to next block
                     * */
                    BadBlockMarker = NAND_RUN_TIME_BAD_BLOCK_MARKER;
                    StartPage[Device] = (NvU32)(CurrPage - (Device*PagesPerDevice));
                    NvDdkNandWriteSpare(hNand, Device, StartPage, &BadBlockMarker,
                        BadBlockMarkerOffset, 1);
                    CurrPage += (NandDevInfo.PagesPerBlock - PageOfs);
                    goto skiptoNextBlock;
                } else
                {
                    goto fail;
                }
            }

            p++;
            PageIdx++;
            c++;
            r--;
            if(gs_skipTagWrite)
                b += pLayout->PageSize;
            else
                b += (pLayout->TagSize + pLayout->SkipSize + pLayout->PageSize);
        }

        Remain = r;
        PageOfs = p;
        CurrPage = c;
        pBuff = b;

skiptoNextBlock:
        StartPage[Device] = ~0UL;
    }

    if (Remain)
        e = NvError_FileWriteFailed;
    else
        e = NvSuccess;

    if (e == NvSuccess)
    {
        *NextPageToWrite = CurrPage;
        NV_CHECK_ERROR_CLEANUP(NvYaffs2PrivNandYaffsVerify(hNand,(NvU32)StartPhysicalPage,
                (NvU32)MaxPhysicalPage, pLayout, pUserBuffer, DataBytes));
    }

 fail:
    NvDdkNandClose(hNand);
    NvRmClose(hRm);
     NvOsFree(pPageBuffer);
    gs_skipTagWrite = NV_FALSE;
    return e;
}

/**
 * NvYaffs2PrivNandErasePartition
 *  Erases a partition
 */
NvError NvYaffs2PrivNandErasePartition(
    const char   *NvPartitionName)
{
    NvRmDeviceHandle    hRm = NULL;
    NvPartInfo          Partition;
    NvDdkNandHandle     hNand = NULL;
    NvDdkNandDeviceInfo NandDevInfo;
    NvU32               PartitionId;
    NvError             e;
    NvU32               CurrBlock;
    NvU32               MaxBlock;
    NvU64               CurrPage;
    NvU64               MaxPage;
    NvU32               PageNumbers[NDFLASH_CS_MAX];
    NvU32               i;
     NvU8                *pPageBuffer = NULL;

    if (!NvPartitionName)
        return NvError_BadParameter;

    for (i=0; i<NV_ARRAY_SIZE(PageNumbers); i++)
        PageNumbers[i] = ~0UL;

    NV_CHECK_ERROR_CLEANUP(NvPartMgrGetIdByName(NvPartitionName, &PartitionId));
    NV_CHECK_ERROR_CLEANUP(NvPartMgrGetPartInfo(PartitionId, &Partition));

    CurrPage = Partition.StartLogicalSectorAddress;

    //Convert the logical to a physical address
    NV_CHECK_ERROR_CLEANUP(
        NvYaffs2PrivGetStartPhysicalPage(PartitionId,
            CurrPage, &CurrPage)
    );

    //This is not strictly correct
    MaxPage = CurrPage + Partition.NumLogicalSectors;

    NV_CHECK_ERROR_CLEANUP(NvRmOpen(&hRm, 0));
    NV_CHECK_ERROR_CLEANUP(NvDdkNandOpen(hRm, &hNand));
    NV_CHECK_ERROR_CLEANUP(NvDdkNandGetDeviceInfo(hNand, 0, &NandDevInfo));

    CurrBlock = (NvU32)(CurrPage/NandDevInfo.PagesPerBlock);
    MaxBlock = (NvU32)(MaxPage + (NandDevInfo.PagesPerBlock-1)) /
        NandDevInfo.PagesPerBlock;

    if(!s_pTagBuff)
    {
        s_pTagBuff = (NvU8 *)NvOsAlloc((NandDevInfo.TagSize + 63) & ~63);
        if (!s_pTagBuff)
        {
            e = NvError_InsufficientMemory;
            goto fail;
        }
    }
    pPageBuffer = (NvU8*)NvOsAlloc(NandDevInfo.PageSize);
    if (!pPageBuffer)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    while (CurrBlock<MaxBlock)
    {
        NvU32 Device    = CurrBlock / NandDevInfo.NoOfBlocks;
        NvU32 BlockIdx  = CurrBlock - (Device*NandDevInfo.NoOfBlocks);
        NvU32 NumErase;
        NandBlockInfo BlockInfo;
        NvU8 BadBlockMarker = NAND_RUN_TIME_BAD_BLOCK_MARKER;
        NvU8 BadBlockMarkerOffset  = NAND_RUN_TIME_BAD_BLOCK_MARKER_OFFSET;

        BlockInfo.IsFactoryGoodBlock = NV_FALSE;
        BlockInfo.IsBlockLocked = NV_FALSE;
        BlockInfo.pTagBuffer = s_pTagBuff;

        NvDdkNandGetBlockInfo(hNand, Device, BlockIdx, &BlockInfo, NV_FALSE);
        if (BlockInfo.IsFactoryGoodBlock)
        {
            PageNumbers[Device] = BlockIdx * NandDevInfo.PagesPerBlock;
            /* Is runtime bad? */
            if (NvYaffs2PrivNandIsRuntimeBadBlock(hNand, Device, BlockIdx,
                    &NandDevInfo, pPageBuffer, s_pTagBuff))
            {
                NvDdkNandWriteSpare(hNand, Device, PageNumbers, &BadBlockMarker,
                        BadBlockMarkerOffset, 1);
            } else
            {
                NumErase = 1;
                e = NvDdkNandErase(hNand, Device, PageNumbers, &NumErase);
                NV_ASSERT(NumErase==1);
                if (e != NvSuccess)
                {
                    if (e == NvError_NandEraseFailed)
                    {
                        /* Mark the block as runtime bad */
                        NvDdkNandWriteSpare(hNand, Device, PageNumbers, &BadBlockMarker,
                                BadBlockMarkerOffset, 1);
                    } else
                    {
                        goto fail;
                    }
                }
            }
        }
        PageNumbers[Device] = ~0UL;
        CurrBlock++;
    }

 fail:
    NvDdkNandClose(hNand);
    NvRmClose(hRm);
    NvOsFree(pPageBuffer);
    return e;
}

