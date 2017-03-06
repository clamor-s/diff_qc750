/**
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/*
 * data_layout.c - Code to manage the layout of data in the boot device.
 *
 */
/*
 * This file manages the locations of data in the boot device.  As such, it
 * serves as a prototype of sorts for logic that would prove useful in a
 * recovery mode app that updates the structures in the boot device.
 */

#include "ap20_data_layout.h"

#include "ap20_set.h" /* For ComputeRandomAesBlock, ReadBlImage */
#include "nvassert.h"
#include "nvapputil.h"
#include "ap20/nvboot_version.h"
#include "ap20/nvboot_bct.h"
#include "../nvbuildbct_util.h"

/* Function prototypes */

static void  UpdateRandomAesBlock(BuildBctContext *Context);

static void
InsertPadding(NvU8 *Data, NvU32 Length);

static void
WritePadding(NvU8 *Data, NvU32 Length);

static void WriteBct(BuildBctContext *Context, NvU32 Block, NvU32 BctSlot);

static void
SetBlData(BuildBctContext *Context,
          NvU32              Instance,
          NvU32              StartBlock,
          NvU32              StartPage,
          NvU32              Length);

static void FillBootloaderData(BuildBctContext *Context, UpdateEndState EndState);

static void BeginUpdate (BuildBctContext *Context, UpdateEndState EndState);
static void FinishUpdate(BuildBctContext *Context, UpdateEndState EndState);

static void
UpdateRandomAesBlock(BuildBctContext *Context)
{
    switch (Context->RandomBlockType)
    {
        case RandomAesBlockType_Zeroes:
        case RandomAesBlockType_Literal:
        case RandomAesBlockType_RandomFixed:
            /* The random block was updated when parsed. */
            break;

        case RandomAesBlockType_Random:
            Ap20ComputeRandomAesBlock(Context);
            break;

        default:
            NV_ASSERT(!"Unknown random block type.");
            return;
    }
}

static void
InsertPadding(NvU8 *Data, NvU32 Length)
{
    NvU32        AesBlocks;
    NvU32        Remaining;

    AesBlocks = ICeilLog2(Length, NVBOOT_AES_BLOCK_SIZE_LOG2);
    Remaining = (AesBlocks << NVBOOT_AES_BLOCK_SIZE_LOG2) - Length;

    WritePadding(Data + Length, Remaining);
}

static void
WritePadding(NvU8 *p, NvU32 Remaining)
{
    NvU8 RefVal = 0x80;

    while (Remaining)
    {
        *p++ = RefVal;
        Remaining--;
        RefVal = 0x00;
    }
}


static void
WriteBct(BuildBctContext *Context, NvU32 Block, NvU32 BctSlot)
{
    NvU32        PagesRemaining;
    NvU32        Page;
    NvU32        PagesPerBct;
    NvU8        *Buffer = NULL;

    NV_ASSERT(Context);

    PagesPerBct = ICeilLog2(Context->NvBCTLength, Context->PageSizeLog2);
    PagesRemaining = PagesPerBct;
    Page = BctSlot * PagesPerBct;

    /* Create a local copy of the BCT data */
    Buffer = NvOsAlloc(Context->NvBCTLength);
    if (Buffer == NULL) goto fail;
    NvOsMemset(Buffer, 0, Context->NvBCTLength);

    NvOsMemcpy(Buffer, Context->NvBCT, Context->NvBCTLength);

    InsertPadding(Buffer, Context->NvBCTLength);
    /* Write the BCT data to the storage device, picking up ECC errors */
    NvOsFwrite(Context->RawFile,
                       Buffer,
                       Context->NvBCTLength);

fail:
    /* Cleanup */
  if(Buffer)
 {
    NvOsFree(Buffer);
    Buffer = NULL;
 }
}

/**** TODO: Add hash ****/
static void
SetBlData(BuildBctContext *Context,
          NvU32              Instance,
          NvU32              StartBlock,
          NvU32              StartPage,
          NvU32              Length)
{
    NvBootConfigTable *Bct = NULL;

    NV_ASSERT(Context);

    Bct = (NvBootConfigTable*)(Context->NvBCT);

    Bct->BootLoader[Instance].Version     = Context->Version;
    Bct->BootLoader[Instance].StartBlock  = Context->NewBlStartBlk;
    Bct->BootLoader[Instance].StartPage   = Context->NewBlStartPg;
    Bct->BootLoader[Instance].Length      = Length;
    Bct->BootLoader[Instance].LoadAddress = Context->NewBlLoadAddress;
    Bct->BootLoader[Instance].EntryPoint  = Context->NewBlEntryPoint;
}


static void
FillBootloaderData(BuildBctContext *Context, UpdateEndState EndState)
{
    NvU32  BlActualSize = 0; /* In bytes */
    NvU32 CurrentBlock = 0;
    NvU32 CurrentPage = 0;
    NvBootConfigTable *Bct = NULL;

    Bct = (NvBootConfigTable*)(Context->NvBCT);

     SetBlData(Context,
                  0,// assuming only one bootloader is active at the moment
                  CurrentBlock,
                  CurrentPage,
                  BlActualSize);

}

static void
InitBadBlockTable(BuildBctContext *Context)
{
    NvU32                BytesPerEntry;
    NvBootBadBlockTable *Table;
    NvBootConfigTable*Bct = NULL;

    Bct = (NvBootConfigTable*)(Context->NvBCT);


    NV_ASSERT(Context      != NULL);
    NV_ASSERT(Bct != NULL);

    Table = &(Bct->BadBlockTable);

    /* Validate context values. */
    if ((Context->PartitionSize % Context->BlockSize) != 0)
    {
        NvAuPrintf("Partition size is not an integral number of blocks\n");
        return;
    }

    /* Initialize the bad block table. */
    Table->BlockSizeLog2        = (NvU8)(Context->BlockSizeLog2);
    BytesPerEntry               = ICeil(Context->PartitionSize,
                                           NVBOOT_BAD_BLOCK_TABLE_SIZE);
    Table->VirtualBlockSizeLog2 = (NvU8) NV_MAX(CeilLog2(BytesPerEntry),
                                                Table->BlockSizeLog2);
    Table->EntriesUsed          = ICeilLog2(Context->PartitionSize,
                                            Table->VirtualBlockSizeLog2);

    if (EnableDebug)
    {
        NvAuPrintf("InitBadBlockTable():\n");
        NvAuPrintf("  BCT->BlockSizeLog2 = %d (%d bytes)\n",
                   Bct->BlockSizeLog2,
                   1 << Bct->BlockSizeLog2);

        NvAuPrintf("  BCT->PartitionSize = %d\n",
                    Bct->PartitionSize);

        NvAuPrintf("  BBT->BlockSizeLog2 = %d (%d bytes)\n",
                    Bct->BadBlockTable.BlockSizeLog2,
                   1 <<  Bct->BadBlockTable.BlockSizeLog2);

        NvAuPrintf("  BBT->VirtualBlockSizeLog2 = %d (%d bytes)\n",
                    Bct->BadBlockTable.VirtualBlockSizeLog2,
                   1 <<  Bct->BadBlockTable.VirtualBlockSizeLog2);

        NvAuPrintf("  BBT->EntriesUsed = %d\n",
                    Bct->BadBlockTable.EntriesUsed);
    }
}

static void
BeginUpdate(BuildBctContext *Context, UpdateEndState EndState)
{
    NvU32 PagesPerBct;
    NvU32 PagesPerBlock;

    NvBootConfigTable *Bct = NULL;
    Bct = (NvBootConfigTable*)(Context->NvBCT);

    NV_ASSERT(Context);

    /* Ensure that the BCT block & page data is current. */
    if (EnableDebug)
        NvAuPrintf("BeginUpdate(): BCT data: b=%d p=%d\n",
                   Bct->BlockSizeLog2,
                   Bct->PageSizeLog2);

    Bct->BootDataVersion = NVBOOT_BOOTDATA_VERSION;
    Bct->BlockSizeLog2   = Context->BlockSizeLog2;
    Bct->PageSizeLog2    = Context->PageSizeLog2;
    Bct->PartitionSize   = Context->PartitionSize;
    FillBootloaderData(Context, EndState);
    PagesPerBct = ICeilLog2(Context->NvBCTLength, Context->PageSizeLog2);
    PagesPerBlock = (1 << (Context->BlockSizeLog2 - Context->PageSizeLog2));
    /* Update the random AES block and store in the BCT. */
    UpdateRandomAesBlock(Context);
    NvOsMemcpy(&(Bct->RandomAesBlock),
               &(Context->RandomBlock),
               sizeof(NvBootHash));

    /* Ensure the bad block table data is up to date. */
    InitBadBlockTable(Context);

     /* Fill the reserved data w/the padding pattern. */
    WritePadding(Bct->Reserved, NVBOOT_BCT_RESERVED_SIZE);

    WriteBct(Context, 0, 0);
}


static void
FinishUpdate(BuildBctContext *Context, UpdateEndState EndState)
{
    Context->BctWritten = NV_TRUE;
}

/*
 * For now, ignore end state.
 */
void
Ap20UpdateBct(BuildBctContext *Context, UpdateEndState EndState)
{
    BeginUpdate(Context, EndState);
    FinishUpdate(Context, EndState);
}

/*
 * For now, ignore end state.
 */
void
Ap20UpdateBl(BuildBctContext *Context, UpdateEndState EndState)
{
    FillBootloaderData(Context, EndState);
}

