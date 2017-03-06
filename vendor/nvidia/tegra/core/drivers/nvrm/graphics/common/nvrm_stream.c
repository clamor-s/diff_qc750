/*
 * Copyright (c) 2007 - 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvassert.h"
#include "nvrm_channel.h"
#include "nvrm_rmctrace.h"
#include "nvrm_stream.h"
#include "nvrm_hardware_access.h"
#include "nvrm_drf.h"
#include "nvrm_disasm.h"

#include "ap20/class_ids.h"
#include "ap20/arhost1x_uclass.h"
#include "ap20/ap20rm_channel.h"


#define NVRM_CMDBUF_GATHER_SIZE 256
#define NVRM_CMDBUF_RELOC_SIZE 1024
#define NVRM_CMDBUF_WAIT_SIZE 256
NV_CT_ASSERT(NVRM_CMDBUF_GATHER_SIZE >= NVRM_STREAM_GATHER_TABLE_SIZE*2 + 1);
NV_CT_ASSERT(NVRM_CMDBUF_RELOC_SIZE >= NVRM_STREAM_RELOCATION_TABLE_SIZE);
NV_CT_ASSERT(NVRM_CMDBUF_WAIT_SIZE >= NVRM_STREAM_WAIT_TABLE_SIZE);
NV_CT_ASSERT((NVRM_CMDBUF_GATHER_SIZE + NVRM_CMDBUF_RELOC_SIZE)
             <= NVRM_CHANNEL_SUBMIT_MAX_HANDLES);
NV_CT_ASSERT(NV_HOST1X_SYNCPT_NB_PTS == 32);
NV_CT_ASSERT(NV_HOST1X_SYNCPT_THRESH_WIDTH == 16);

/* 2 words for sync point increment, 8 words for the 3D wait base */
#define NVRM_CMDBUF_BOOKKEEPING (40)

typedef struct NvRmCmdBuf
{
    // This buffer will be managed as a ping-pong buffer.  We fill one half as we
    // wait for the hardware to drain the other half, preventing the hardware from
    // going idle.
    NvRmMemHandle hMem;
    void *pMem; /* if the memory is mappable */
    NvU32 memSize; /* size of the entire memory region */
    NvU32 current; /* offset from memory base */
    NvU32 fence; /* size of the memory */
    NvU32 last; /* offset of the last submit() */
    NvU32 pong; // 0 for first half of buffer, 1 for second half of buffer
    /* used to track buffer consumption by hardware */
    NvOsSemaphoreHandle sem;

    /* flushing stuff (don't put this on the stack...too big) */
    NvRmCommandBuffer GatherTable[NVRM_CMDBUF_GATHER_SIZE];
    NvRmCommandBuffer *pCurrentGather;

    NvRmChannelSubmitReloc RelocTable[NVRM_CMDBUF_RELOC_SIZE];
    NvRmChannelSubmitReloc *pCurrentReloc;
    NvU32 RelocShiftTable[NVRM_CMDBUF_RELOC_SIZE];
    NvU32 *pCurrentRelocShift;

    NvRmChannelWaitChecks WaitTable[NVRM_CMDBUF_WAIT_SIZE];
    NvRmChannelWaitChecks *pCurrentWait;
    NvU32 SyncPointsWaited;     // bitmask of referenced WAIT syncpt IDs

    NvRmDeviceHandle hDevice;
    NvRmChannelHandle hChannel;

    // The sync point id and value for the most recent submit to the HW
    NvU32 SyncPointID;
    NvU32 SyncPointValue;
    //Shadow the waitbase to avoid the costly kernel call, right now
    //only 3d module's waitbase is initialized.
    NvU32 SyncPointWaitBaseIDShadow[NvRmModuleID_Num];
    // The sync point id and value for the previous half of the ping-pong buf
    NvU32 PongSyncPointID;
    NvU32 PongSyncPointValue;

    NvU32 SubmitTimeout;

    /* number of syncpts written to stream */
    NvU32 SyncPointsUsed;
} NvRmCmdBuf;

struct NvRmStreamPrivateRec
{
    NvRmCmdBuf CmdBuf;

    /*** RM PRIVATE DATA STARTS HERE -- DO NOT ACCESS THE FOLLOWING FIELDS ***/

    // Opaque pointer to RM private data associated with this stream.  Anything
    // that RM does not need high-performance access to should be hidden behind
    // here, so that we don't have to recompile the world when it changes.
    NvRmStreamPrivate *pRmPriv;

    // This points to the base of the command buffer chunk that we are
    // currently filling in.
    NvData32 *pBase;

    // This points to the end of the region of the command buffer that we are
    // currently allowed to write to.  This could be the end of the memory
    // allocation, the point the HW is currently fetching from, or just an
    // artificial "auto-flush" point where we want to stop and kick off the
    // work we've queued up before continuing on.
    NvData32 *pFence;

    // This points to the current location we are writing to.  This should
    // always be >= pBase and <= pFence.
    NvData32 *pCurrent;

    // Memory pin/unpin, command buffer relocation, and gather tables.
    NvRmCmdBufRelocation *pCurrentReloc;
    NvRmCmdBufGather *pCurrentGather;
    NvRmCmdBufRelocation RelocTable[NVRM_STREAM_RELOCATION_TABLE_SIZE];
    NvRmCmdBufGather GatherTable[NVRM_STREAM_GATHER_TABLE_SIZE];

    // Track which syncpts (and their threshold values) that have been waited
    // for in the command stream. This is used to remove a wait that already
    // has been satisfied. This is necessary as with a 16bit HW compare it can
    // appear pending in the future (but from a 32bit compare, it's already
    // completed and wrapped).
    NvRmCmdBufWait *pCurrentWait;
    NvRmCmdBufWait WaitTable[NVRM_STREAM_WAIT_TABLE_SIZE];
    NvU32 SyncPointsWaited;

    // We keep track of how much command buffer space the user has asked for,
    // so that we can verify that they don't write too much data.  This also
    // doubles as a way for us to keep track of whether we are currently inside
    // a Get/Save pair.  (Get of zero is illegal, so if this is nonzero we know
    // we are inside a Get/Save pair.)
    NvU32 WordsRequested;

    // Same idea, but for the gather and reloc tables.  If the user writes past
    // this point, there is a bug in their code.
    NvRmCmdBufRelocation *pRelocFence;
    NvRmCmdBufGather *pGatherFence;
    NvRmCmdBufWait *pWaitFence;
    NvU32 CtxChanged;

    // String buffer for command disassembly
    char *DisasmBuffer;
    NvU32 DisasmBufferSize;
    NvBool DisasmLoaded;
};

static void NvRmPrivSetPong(NvRmCmdBuf *cmdbuf, NvU32 pong)
{
    if (pong == 1)
    {
        cmdbuf->pong = 1;
        cmdbuf->current = cmdbuf->memSize/2;
        cmdbuf->fence = cmdbuf->memSize;
    }
    else
    {
        NV_ASSERT(pong == 0);
        cmdbuf->pong = 0;
        cmdbuf->current = 0;
        cmdbuf->fence = cmdbuf->memSize/2;
    }
}

NvError
NvRmStreamGetError(NvRmStream *pStream ) {
    NvError err = pStream->ErrorFlag;
    pStream->ErrorFlag = NvSuccess;
    return err;
}

void
NvRmStreamSetError(NvRmStream *pStream, NvError err) {
    if(pStream->ErrorFlag == NvSuccess) {
        pStream->ErrorFlag = err;
    }
}

void NvRmStreamInitParamsSetDefaults( NvRmStreamInitParams* pInitParams)
{
    NvOsMemset( pInitParams, 0, sizeof(*pInitParams) );
    pInitParams->cmdBufSize = 32768;
}

NvError
NvRmStreamInit( NvRmDeviceHandle hDevice, NvRmChannelHandle hChannel,
    NvRmStream *pStream )
{
    NvRmStreamInitParams initParams;
    NvRmStreamInitParamsSetDefaults(&initParams);
    return NvRmStreamInitEx( hDevice, hChannel, &initParams, pStream );
}

NvError
NvRmStreamInitEx ( NvRmDeviceHandle hDevice, NvRmChannelHandle hChannel,
    const NvRmStreamInitParams* pInitParams, NvRmStream *pStream)
{
    NvError err;
    NvRmStreamPrivate *priv = NULL;
    NvRmCmdBuf *cmdbuf = NULL;
    const NvU32 granularity = 8*1024;   // cmdBufSize granularity

    /* handle NULL pInitParams case */
    NvRmStreamInitParams defaultParams;
    if (!pInitParams)
    {
        NvRmStreamInitParamsSetDefaults(&defaultParams);
        pInitParams = &defaultParams;
    }
    /* Validation of input params.
     * Check cmdBufSize is at least a multiple of 8KB.
     */
    if ((pInitParams->cmdBufSize < granularity) ||
        (0 != (pInitParams->cmdBufSize & (granularity-1))))
    {
        goto fail;
    }

    /* for simulation, each stream has a command buffer which is copied to at
     * GetSpace() time. Only when the command buffer is full or when Flush()
     * is called does the command buffer get submitted to the channel.
     *
     * the stream buffer (from GetSpace()) is operating system-allocated memory
     * since it must be dereferencable (mapped into the process' virtual
     * space).
     *
     * for cases where the command buffer is mappable (real hardware),
     * the stream pointers (pBase, pCurrent, etc) point into the mapped
     * command buffer.
     */

    /* zero out the stream */
    NvOsMemset( pStream, 0, sizeof(NvRmStream) );

    priv = NvOsAlloc( sizeof(*priv) );
    if( !priv )
    {
        goto fail;
    }
    cmdbuf = &priv->CmdBuf;

    NvOsMemset( priv, 0, sizeof(*priv) );

    // initialize necessary fields for proper cleanup if fail/bail-out later
    cmdbuf->SyncPointID = NVRM_INVALID_SYNCPOINT_ID;
    pStream->pRmPriv = (void *)priv;

    err = NvOsSemaphoreCreate(&cmdbuf->sem, 0);
    if( err != NvSuccess )
    {
        goto fail;
    }

    /* allocate/map the command buffer first. if the mapping fails, then
     * setup the stream buffer with normally-allocated OS memory.
     */
    cmdbuf->memSize = pInitParams->cmdBufSize;
    err = NvRmMemHandleCreate(hDevice, &cmdbuf->hMem, cmdbuf->memSize);
    if( err != NvSuccess )
    {
        goto fail;
    }

    err = NvRmMemAllocTagged(cmdbuf->hMem, NULL, 0, 32,
        NvOsMemAttribute_WriteCombined, NVRM_MEM_TAG_RM_MISC);
    if( err != NvSuccess )
    {
        goto fail;
    }

    /* try to map the command buffer.
     * NOTE: if the size changes here, be sure to update the size passed to
     * NvRmMemUnMap
     */
    err = NvRmMemMap( cmdbuf->hMem, 0, cmdbuf->memSize, NVOS_MEM_READ_WRITE,
        &cmdbuf->pMem );
    if( err != NvSuccess )
    {
        cmdbuf->pMem = 0;
        /* the map may fail, that's ok */
    }
    else
        NvRmMemCacheMaint( cmdbuf->hMem, cmdbuf->pMem, cmdbuf->memSize, NV_FALSE, NV_TRUE);

    NvRmPrivSetPong(cmdbuf, 0);
    cmdbuf->last = 0;
    cmdbuf->pCurrentGather = &cmdbuf->GatherTable[0];
    cmdbuf->pCurrentReloc = &cmdbuf->RelocTable[0];
    cmdbuf->pCurrentRelocShift = &cmdbuf->RelocShiftTable[0];
    cmdbuf->pCurrentWait = &cmdbuf->WaitTable[0];

    cmdbuf->hDevice= hDevice;
    cmdbuf->hChannel = hChannel;
    cmdbuf->SyncPointValue = 0;
    cmdbuf->SyncPointWaitBaseIDShadow[NvRmModuleID_3D] =
        NvRmChannelGetModuleWaitBase( hChannel, NvRmModuleID_3D, 0 );
    cmdbuf->PongSyncPointID = NVRM_INVALID_SYNCPOINT_ID;
    cmdbuf->PongSyncPointValue = 0;

    cmdbuf->SubmitTimeout = 0;

    pStream->Flags = 0;

    priv->DisasmBuffer = NULL;
    priv->DisasmBufferSize = 0;
    priv->DisasmLoaded = NV_FALSE;

    // Write to a malloc buffer and copy to the real command buffer
    // Allow for a single max-sized stream buffer plus the sync point
    // increment.
    // FIXME: should the buffer have this NVRM_CMDBUF_BOOKKEEPING padding?
    // They don't always go through the stream -- sometimes they go straight
    // to the command buffer.
    pStream->pBase = NvOsAlloc(
        NVRM_STREAM_BEGIN_MAX_WORDS*4 + NVRM_CMDBUF_BOOKKEEPING);
    if (!pStream->pBase)
    {
        goto fail;
    }
    pStream->pFence = pStream->pBase + NVRM_STREAM_BEGIN_MAX_WORDS;
    pStream->pCurrent = pStream->pBase;
    pStream->pCurrentReloc = &pStream->RelocTable[0];
    pStream->pCurrentGather = &pStream->GatherTable[0];
    pStream->pCurrentWait = &pStream->WaitTable[0];
    pStream->SyncPointsWaited = 0;

    pStream->SyncPointID = NVRM_INVALID_SYNCPOINT_ID;
    pStream->SyncPointsUsed = 0;
    pStream->LastEngineUsed = (NvRmModuleID)0;
    pStream->UseImmediate = NV_FALSE;

    return NvSuccess;

fail:
    NvRmStreamFree( pStream );  // also free up cmdbuf and its fields
    return NvError_RmStreamInitFailure;
}

void
NvRmStreamFree(NvRmStream *pStream)
{
    NvRmStreamPrivate *priv;

    if( !pStream )
    {
        return;
    }

    priv = pStream->pRmPriv;

    if( priv )
    {
        NvRmCmdBuf *cmdbuf = &priv->CmdBuf;
        if ( cmdbuf->SyncPointID != NVRM_INVALID_SYNCPOINT_ID )
        {
            /* make sure the stream is flushed */
            NvRmChannelSyncPointWait( cmdbuf->hDevice,
                cmdbuf->SyncPointID,
                cmdbuf->SyncPointValue,
                cmdbuf->sem );
        }
        NvOsSemaphoreDestroy( cmdbuf->sem );
        NvRmMemUnmap(cmdbuf->hMem, cmdbuf->pMem, cmdbuf->memSize);
        NvRmMemHandleFree(cmdbuf->hMem);

        NvOsFree( priv->DisasmBuffer );

        if ( priv->DisasmLoaded )
            NvRmDisasmLibraryUnload();

        NvOsFree( priv );
        pStream->pRmPriv = NULL;
    }
    NvOsFree(pStream->pBase);
}

static NvData32 *NvRmPrivAppendSyncPointBaseIncr( NvRmCmdBuf * cmdbuf,
    NvData32 *pCurrent, NvU32 count )
{
    NVRM_STREAM_PUSH_U(pCurrent,
        NVRM_CH_OPCODE_SET_CLASS(NV_HOST1X_CLASS_ID, 0, 0));
    NVRM_STREAM_PUSH_U(pCurrent,
        NVRM_CH_OPCODE_NONINCR( NV_CLASS_HOST_INCR_SYNCPT_BASE_0, 1));
    NVRM_STREAM_PUSH_U(pCurrent,
        NV_DRF_NUM(NV_CLASS_HOST, INCR_SYNCPT_BASE, BASE_INDX,
                    cmdbuf->SyncPointWaitBaseIDShadow[NvRmModuleID_3D] ) |
        NV_DRF_NUM(NV_CLASS_HOST, INCR_SYNCPT_BASE, OFFSET, count));
    NVRM_STREAM_PUSH_U(pCurrent,
        NVRM_CH_OPCODE_SET_CLASS(NV_GRAPHICS_3D_CLASS_ID, 0, 0));
    return pCurrent;
}

/**
 * copies the stream buffer to the command buffer.
 */
static void
NvRmPrivCopyToCmdbuf( NvRmStream *pStream )
{
    NvRmCmdBuf *cmdbuf = &pStream->pRmPriv->CmdBuf;
    NvRmCmdBufGather *gather;
    NvRmCmdBufRelocation *reloc;
    NvRmCmdBufWait *wait;
    NvRmCommandBuffer *buf;
    NvRmChannelSubmitReloc *r;
    NvU32 *rShift;
    NvRmChannelWaitChecks *w;
    NvData32 *cur;
    NvU32 size;

    // FIXME: make clients specify the sync point id
    if (pStream->SyncPointID == NVRM_INVALID_SYNCPOINT_ID)
        pStream->SyncPointID = 0;

    // If necessary, stick an increment syncpt-base on the end of the stream
    // command buffer before copying it
    if ((pStream->SyncPointID == NVRM_SYNCPOINT_3D
            || pStream->SyncPointID == NVRM_SYNCPOINT_3D_HIPRIO)
        && pStream->SyncPointsUsed)
    {
        pStream->pCurrent = NvRmPrivAppendSyncPointBaseIncr(
            cmdbuf, pStream->pCurrent, pStream->SyncPointsUsed);
    }

    // Compute size in bytes -- could be larger than
    // NVRM_STREAM_BEGIN_MAX_WORDS due to extra stuff we stick on
    size = (NvU32)pStream->pCurrent - (NvU32)pStream->pBase;
    NV_ASSERT((size / 4) <= NVRM_STREAM_BEGIN_MAX_WORDS+4);

    // If necessary, copy the stream buffer to the command buffer
    if (size)
    {
        if (cmdbuf->pMem)
        {
            NvOsMemcpy((NvU8 *)cmdbuf->pMem + cmdbuf->current, pStream->pBase,
                size);
        }
        else
        {
            NvRmMemWrite(cmdbuf->hMem, cmdbuf->current, pStream->pBase, size);
        }
    }

    /* handle gathers - walk the gather table - split up the command buffer
     * into chunks before and after each gather:
     *
     *  +--+ <-+   +--+  <-- cur
     *  |  |   |   |  |
     *  +--+   +-- |==| GatherTable[0], gather->current
     *             |  |
     *  +--+ <---- |==| GatherTable[1], gather->current
     *  |  |       |  |
     *  +--+       |  |
     *             +--+
     */
    buf = cmdbuf->pCurrentGather;
    gather = &pStream->GatherTable[0];
    cur = pStream->pBase;
    while (gather != pStream->pCurrentGather)
    {
        if (cur < gather->pCurrent)
        {
            // Data block before the next gather
            buf->hMem = cmdbuf->hMem;
            buf->Offset = cmdbuf->current + (NvU32)cur - (NvU32)pStream->pBase;
            buf->Words = gather->pCurrent - cur;
            buf++;
            cur = gather->pCurrent;
        }

        // Insert the gather
        NV_ASSERT(cur == gather->pCurrent);
        buf->hMem = gather->hMem;
        buf->Offset = gather->Offset;
        buf->Words = gather->Words;
        buf++;
        gather++;
    }
    if (cur != pStream->pCurrent)
    {
        // Data block after the last gather
        buf->hMem = cmdbuf->hMem;
        buf->Offset = cmdbuf->current + (NvU32)cur - (NvU32)pStream->pBase;
        buf->Words = pStream->pCurrent - cur;
        buf++;
        cur = pStream->pCurrent;
    }
    cmdbuf->pCurrentGather = buf;

    // Copy the relocs from the stream reloc table into the cmdbuf reloc table,
    // adjusting their target from being a pointer to being an [hMem, Offset]
    // pair
    r = cmdbuf->pCurrentReloc;
    rShift = cmdbuf->pCurrentRelocShift;
    reloc = &pStream->RelocTable[0];
    while (reloc != pStream->pCurrentReloc)
    {
        NvU32 offset = (NvU32)reloc->pTarget - (NvU32)pStream->pBase;
        r->hCmdBufMem = cmdbuf->hMem;
        r->CmdBufOffset = cmdbuf->current + offset;
        r->hMemTarget = reloc->hMem;
        r->TargetOffset = reloc->Offset;
        *rShift = reloc->Shift;
        r++;
        rShift++;
        reloc++;
    }
    cmdbuf->pCurrentReloc = r;
    cmdbuf->pCurrentRelocShift = rShift;

    // copy wait checks from stream to cmdbuf
    w = cmdbuf->pCurrentWait;
    wait = &pStream->WaitTable[0];
    while (wait != pStream->pCurrentWait)
    {
        NvU32 offset = ((NvU32)wait->pCurrent - (NvU32)pStream->pBase);
        w->hCmdBufMem = cmdbuf->hMem;
        w->CmdBufOffset = cmdbuf->current + offset;
        w->SyncPointID = wait->Wait.SyncPointID;
        w->Threshold = wait->Wait.Value;
        w++;
        wait++;
    }
    cmdbuf->pCurrentWait = w;
    cmdbuf->SyncPointsWaited |= pStream->SyncPointsWaited;
    pStream->SyncPointsWaited = 0;

    // Reset stream pointers to beginnings of their respective tables
    pStream->pCurrent = pStream->pBase;
    pStream->pCurrentGather = &pStream->GatherTable[0];
    pStream->pCurrentReloc = &pStream->RelocTable[0];
    pStream->pCurrentWait = &pStream->WaitTable[0];

    // Update the cmdbuf state
    cmdbuf->current += size;
    NV_ASSERT(cmdbuf->current <= cmdbuf->fence);
    cmdbuf->SyncPointID = pStream->SyncPointID;

    if( !pStream->ClientManaged )
    {
        cmdbuf->SyncPointsUsed += pStream->SyncPointsUsed;
    }
    else
    {
        /* always use value from client */
        cmdbuf->SyncPointsUsed = pStream->SyncPointsUsed;
    }
    pStream->SyncPointsUsed = 0;
}

// This function returns true if there is space in the command buffer to copy
// over everything that's currently in the stream buffer.
static NvBool NvRmPrivCheckSpace(NvRmStream *pStream,
    NvU32 Words, NvU32 Gathers, NvU32 Relocs, NvU32 Waits)
{
    NvRmCmdBuf *cmdbuf = &pStream->pRmPriv->CmdBuf;
    NvU32 StreamGathers, StreamRelocs, StreamWaitChecks, size;

    NV_ASSERT(cmdbuf->fence >= cmdbuf->current);

    // Check for space in the cmdbuf gather, reloc and wait tables
    // Note that N gathers in the stream table can expand to 2*N+1 in the
    // cmdbuf.
    // Also need to save one for the syncpt increment
    StreamGathers = pStream->pCurrentGather - &pStream->GatherTable[0];
    StreamGathers += Gathers;
    if (cmdbuf->pCurrentGather + StreamGathers*2 + 1 >
        &cmdbuf->GatherTable[NVRM_CMDBUF_GATHER_SIZE-1])
    {
        return NV_FALSE;
    }

    StreamRelocs = pStream->pCurrentReloc - &pStream->RelocTable[0];
    StreamRelocs += Relocs;
    if (cmdbuf->pCurrentReloc + StreamRelocs >
        &cmdbuf->RelocTable[NVRM_CMDBUF_RELOC_SIZE])
    {
        return NV_FALSE;
    }

    StreamWaitChecks = pStream->pCurrentWait - &pStream->WaitTable[0];
    StreamWaitChecks += Waits;
    if (cmdbuf->pCurrentWait + StreamWaitChecks >
        &cmdbuf->WaitTable[NVRM_CMDBUF_WAIT_SIZE])
    {
        return NV_FALSE;
    }

    // Always add the full amount of bookkeeping space -- this is slightly
    // wasteful for non-3D, but avoids a branch, so probably a win on net
    // Also reserve space to be filled by a pre-flush callback if any
    size = (pStream->pCurrent - pStream->pBase) * sizeof(NvU32);
    size += NVRM_CMDBUF_BOOKKEEPING * (Words ? 2 : 1);
    size += Words * sizeof(NvU32);
    size += Relocs * sizeof(NvU32);
    size += pStream->PreFlushWords * sizeof(NvU32);
    if (cmdbuf->current + size > cmdbuf->fence)
        return NV_FALSE;

    return NV_TRUE;
}

static NvData32 *
NvRmStreamGetSpace(NvRmStream *pStream,
    NvU32 Words, NvU32 Gathers, NvU32 Relocs, NvU32 Waits)
{
    NV_ASSERT( pStream );

    // Is there room in the cmdbuf for everything currently in the stream buf?
    if (NvRmPrivCheckSpace(pStream, Words, Gathers, Relocs, Waits))
    {
        // Yes -- go ahead and copy it over, at which point the stream bufs
        // will be empty and we can safely refill them.
        NvRmPrivCopyToCmdbuf(pStream);
    }
    else
    {
        // No.  Force a full flush of the stream, which will clear everything
        // out of the stream buffers, leaving them empty for us to refill.
        NvRmStreamFlush(pStream, NULL);
    }

    return pStream->pBase;
}

static void
NvRmPrivAppendIncr(  NvRmStream *pStream )
{
    NvRmCmdBuf *cmdbuf = &pStream->pRmPriv->CmdBuf;
    NvData32 reg[6];
    NvU32 CopySize;
    NvU32 OriginalCurrent = cmdbuf->current;
    NvRmCommandBuffer *buf;

    cmdbuf->SyncPointsUsed++;

    reg[0].u = NVRM_CH_OPCODE_NONINCR( NVRM_INCR_SYNCPT_0, 1 );

    /* INCR_SYNCPT format is:
     *
     * 15:8 - condition
     *   OP_DONE is 1: module has reached bottom of pipe
     *   IMMEDIATE is 0: command buffer is at the top of the pipe
     * 7:0 - sync point id
     */
    if( pStream->UseImmediate )
    {
        reg[1].u = NV_DRF_DEF( NVRM, INCR_SYNCPT, COND, IMMEDIATE )
            | NV_DRF_NUM( NVRM, INCR_SYNCPT, INDX,
                (cmdbuf->SyncPointID & 0xff) );
    }
    else
    {
        reg[1].u = NV_DRF_DEF( NVRM, INCR_SYNCPT, COND, OP_DONE )
            | NV_DRF_NUM( NVRM, INCR_SYNCPT, INDX,
                (cmdbuf->SyncPointID & 0xff) );
    }
    if (cmdbuf->SyncPointID == NVRM_SYNCPOINT_3D
        || cmdbuf->SyncPointID == NVRM_SYNCPOINT_3D_HIPRIO)
    {
        (void)NvRmPrivAppendSyncPointBaseIncr(cmdbuf, &reg[2], 1);
        CopySize = 24;
    }
    else
    {
        CopySize = 8;
    }

    if (cmdbuf->pMem)
    {
        NvOsMemcpy((NvU8 *)cmdbuf->pMem + cmdbuf->current, &reg, CopySize);
    }
    else
    {
        NvRmMemWrite(cmdbuf->hMem, cmdbuf->current, &reg, CopySize);
    }
    cmdbuf->current += CopySize;
    NV_ASSERT( cmdbuf->current <= cmdbuf->fence );

    NV_ASSERT(cmdbuf->pCurrentGather != &cmdbuf->GatherTable[0]);
    buf = cmdbuf->pCurrentGather-1;
    if ((buf->hMem == cmdbuf->hMem) &&
        (buf->Offset + buf->Words*4 == OriginalCurrent))
    {
        // Can append to the last existing gather
        buf->Words += (cmdbuf->current - OriginalCurrent) >> 2;
    }
    else
    {
        // Must append a new gather
        buf++;
        buf->hMem = cmdbuf->hMem;
        buf->Offset = OriginalCurrent;
        buf->Words = (cmdbuf->current - OriginalCurrent) >> 2;
        cmdbuf->pCurrentGather = buf+1;
    }
}

static void
NvRmPrivDisasm(NvRmStream *pStream)
{
    char str[200];
    NvS32 offset;
    NvStreamDisasmContext *context = NULL;
    NvRmStreamPrivate *priv = pStream->pRmPriv;
    NvRmCmdBuf *cmdbuf = &priv->CmdBuf;
    NvRmCommandBuffer *buf = &cmdbuf->GatherTable[0];

    if (priv->DisasmLoaded == NV_FALSE)
    {
        priv->DisasmLoaded = NvRmDisasmLibraryLoad();
        if (priv->DisasmLoaded == NV_FALSE)
            goto fail;
    }

    if (priv->DisasmBuffer == NULL)
    {
        #define NVRM_DISASM_BUFFER_SIZE 2048
        priv->DisasmBufferSize = NVRM_DISASM_BUFFER_SIZE;
        priv->DisasmBuffer = NvOsAlloc(priv->DisasmBufferSize);
        if (priv->DisasmBuffer == NULL)
            goto fail;
    }

    offset = NvOsSnprintf(priv->DisasmBuffer, priv->DisasmBufferSize,
                          "NVRM STREAM DISASM:\n");

    if (offset < 0 || (NvU32) offset >= priv->DisasmBufferSize)
        goto fail;

    context = NvRmDisasmCreate();

    if (context == NULL)
        goto fail;

    NvRmDisasmInitialize(context);

    while (buf != cmdbuf->pCurrentGather)
    {
        NvU32 i;

        for (i=0; i<buf->Words; i++)
        {
            NvS32 length;
            NvU32 data;

            data = NvRmMemRd32(buf->hMem, buf->Offset + i*4);
            length = NvRmDisasmPrint(context,
                                     data,
                                     str,
                                     NV_ARRAY_SIZE(str));

            if (length < 0)
                return;

            if (priv->DisasmBufferSize < (NvU32) (offset + length + 2))
            {
                size_t newSize = priv->DisasmBufferSize * 2 + length + 2;
                char *newBuffer = NvOsAlloc(newSize);

                if (newBuffer == NULL)
                    return;

                NvOsMemcpy(newBuffer, priv->DisasmBuffer, offset);
                NvOsFree(priv->DisasmBuffer);
                priv->DisasmBufferSize = newSize;
                priv->DisasmBuffer = newBuffer;
            }

            NvOsMemcpy(priv->DisasmBuffer + offset, str, length);
            offset += length;
            priv->DisasmBuffer[offset++] = '\n';
            priv->DisasmBuffer[offset] = 0;
        }

        buf++;
    }

    NvOsDebugString(priv->DisasmBuffer);

fail:
    if (context != NULL)
        NvRmDisasmDestroy(context);
}

static void
NvRmPrivFlush(NvRmStream *pStream)
{
    NvRmCmdBuf *cmdbuf = &pStream->pRmPriv->CmdBuf;
    NvError streamError = pStream->ErrorFlag;

    NV_ASSERT( cmdbuf->hMem );
    NV_ASSERT( cmdbuf->fence >= cmdbuf->current );
    NV_ASSERT( cmdbuf->sem );
    NV_ASSERT( cmdbuf->pCurrentGather >= &cmdbuf->GatherTable[0] );
    NV_ASSERT( cmdbuf->pCurrentGather <=
            &cmdbuf->GatherTable[ NVRM_CMDBUF_GATHER_SIZE ] );
    NV_ASSERT( cmdbuf->pCurrentReloc >= &cmdbuf->RelocTable[0] );
    NV_ASSERT( cmdbuf->pCurrentReloc <=
            &cmdbuf->RelocTable[ NVRM_CMDBUF_RELOC_SIZE ] );
    NV_ASSERT( cmdbuf->pCurrentWait >= &cmdbuf->WaitTable[0] );
    NV_ASSERT( cmdbuf->pCurrentWait <=
            &cmdbuf->WaitTable[ NVRM_CMDBUF_WAIT_SIZE ] );

    // If there's nothing to submit, don't submit.
    if ((cmdbuf->current == cmdbuf->last) &&
        (cmdbuf->pCurrentGather == &cmdbuf->GatherTable[0]) &&
        (cmdbuf->pCurrentReloc == &cmdbuf->RelocTable[0]) &&
        (cmdbuf->pCurrentWait == &cmdbuf->WaitTable[0]))
    {
        return;
    }

    // Update submit buffer timeout on any changes */
    if (pStream->SubmitTimeout != cmdbuf->SubmitTimeout)
    {
        cmdbuf->SubmitTimeout = pStream->SubmitTimeout;
        NvRmChannelSetSubmitTimeout(cmdbuf->hChannel, cmdbuf->SubmitTimeout);
    }

    // Let's call the callback only if user has requested to be notified
    // before a flush, and there actually is room. The second condition
    // might be false if callback was registered while command buffer was
    // full. Even in this case, callback will be called for the next flush
    if (pStream->pPreFlushCallback &&
        pStream->PreFlushWords*sizeof(NvU32) <= cmdbuf->fence - cmdbuf->current)
    {
        // Do not call NvRmStreamGetSpace (or NVRM_STREAM_BEGIN) here to
        // avoid nasty recursion. Moreover, NvRmPrivCheckSpace can return
        // false negative here.

        NvU32 bytesWritten;
        NvData32 *pDataStart = pStream->pPreFlushData;

        pStream->pPreFlushCallback(pStream);

        bytesWritten = (NvU32)pStream->pPreFlushData - (NvU32)pDataStart;
        NV_ASSERT(bytesWritten <= pStream->PreFlushWords * sizeof(NvU32));
        NV_ASSERT(bytesWritten + cmdbuf->current <= cmdbuf->fence);

        // If necessary, copy the written bytes to the command buffer
        if (bytesWritten)
        {
            if (cmdbuf->pMem)
            {
                NvOsMemcpy((NvU8 *)cmdbuf->pMem + cmdbuf->current,
                           pDataStart, bytesWritten);
            }
            else
            {
                NvRmMemWrite(cmdbuf->hMem,
                             cmdbuf->current,
                             pDataStart, bytesWritten);
            }

            // Update the cmdbuf state
            cmdbuf->current += bytesWritten;

            // Reset pPreFlushData pointer
            pStream->pPreFlushData = pDataStart;
        }
    }

    NV_ASSERT( pStream->SyncPointID != NVRM_INVALID_SYNCPOINT_ID );

    /* add a sync point increment for pushbuffer tracking.
     * this must not append if the client sets the disable bit.
     */
    if( !pStream->ClientManaged )
    {
        NvRmPrivAppendIncr( pStream );
    }

    pStream->CtxChanged = 0;

    // After failed submission new submissions can lead to undefined results.
    if(streamError == NvSuccess)
    {
        // Write back command buffer data
        if (cmdbuf->pMem)
            NvRmMemCacheMaint(cmdbuf->hMem, (NvU8 *)cmdbuf->pMem + cmdbuf->last,
                    cmdbuf->current - cmdbuf->last, NV_TRUE, NV_TRUE);

        if (pStream->Flags & NvRmStreamFlag_Disasm)
        {
            NvRmPrivDisasm(pStream);
        }

        /* finally submit data to the channel.
         * pinning/unpinning will be done by the channel submit logic.
         *
         * the number of buffers may be zero if the client needs to wait for
         * previous submission.
         *
         * If submission fails, we set error flag in stream.
         */
        streamError = NvRmChannelSubmit(cmdbuf->hChannel,
                cmdbuf->GatherTable,
                cmdbuf->pCurrentGather - &cmdbuf->GatherTable[0],
                cmdbuf->RelocTable,
                cmdbuf->RelocShiftTable,
                cmdbuf->pCurrentReloc - &cmdbuf->RelocTable[0],
                cmdbuf->WaitTable,
                cmdbuf->pCurrentWait - &cmdbuf->WaitTable[0],
                (pStream->NullKickoff) ? 0 : pStream->hContext,
                pStream->pContextShadowCopy, pStream->ContextShadowCopySize,
                pStream->NullKickoff, pStream->LastEngineUsed,
                cmdbuf->SyncPointID, cmdbuf->SyncPointsUsed, cmdbuf->SyncPointsWaited,
                &pStream->CtxChanged, &cmdbuf->SyncPointValue);
    }

    if(streamError == NvSuccess) {
        // Inform our client what range of sync points they actually got
        if (pStream->pSyncPointBaseCallback)
        {
            pStream->pSyncPointBaseCallback(
                pStream,
                cmdbuf->SyncPointValue - cmdbuf->SyncPointsUsed,
                cmdbuf->SyncPointsUsed);
        }
    }
    else {
        NvRmStreamSetError(pStream, streamError);
        if (pStream->pSyncPointBaseCallback)
        {
            pStream->pSyncPointBaseCallback(
                pStream,
                cmdbuf->SyncPointValue,
                0);
        }
    }

    cmdbuf->SyncPointsUsed = 0;
    cmdbuf->SyncPointsWaited = 0;
    cmdbuf->pCurrentWait = &cmdbuf->WaitTable[0];

    // Keep track of the last time we submitted to the hardware
    cmdbuf->last = cmdbuf->current;

    // Once we've called ChannelSubmit, we can reset the cmdbuf gather and
    // reloc tables.
    cmdbuf->pCurrentGather = &cmdbuf->GatherTable[0];
    cmdbuf->pCurrentReloc = &cmdbuf->RelocTable[0];
    cmdbuf->pCurrentRelocShift = &cmdbuf->RelocShiftTable[0];
}

NvData32 *NvRmStreamBegin(NvRmStream *pStream,
    NvU32 Words, NvU32 Waits, NvU32 Relocs, NvU32 Gathers)
{
    NvData32 *pb;

    NV_ASSERT( pStream );
    NV_ASSERT( Words + Gathers > 0 );
    NV_ASSERT( Words <= NVRM_STREAM_BEGIN_MAX_WORDS );
    NV_ASSERT( Relocs <= NVRM_STREAM_RELOCATION_TABLE_SIZE );
    NV_ASSERT( Gathers <= NVRM_STREAM_GATHER_TABLE_SIZE );
    NV_ASSERT( Waits <= NVRM_STREAM_WAIT_TABLE_SIZE );

    NV_ASSERT(pStream->WordsRequested == 0);
    pb = pStream->pCurrent;
    if ((pb + Words + Relocs > pStream->pFence) ||
        (pStream->pCurrentReloc + Relocs >
            &pStream->RelocTable[NVRM_STREAM_RELOCATION_TABLE_SIZE]) ||
        (pStream->pCurrentGather + Gathers >
            &pStream->GatherTable[NVRM_STREAM_GATHER_TABLE_SIZE]) ||
        (pStream->pCurrentWait + Waits >
            &pStream->WaitTable[NVRM_STREAM_WAIT_TABLE_SIZE]) ||
        (!NvRmPrivCheckSpace(pStream, Words, Gathers, Relocs, Waits))) {

        pb = NvRmStreamGetSpace(pStream, Words, Gathers, Relocs, Waits);
    }

    NV_ASSERT(pb + Words + Relocs <= pStream->pFence);
    NV_DEBUG_CODE(pStream->WordsRequested = Words + Relocs;)
    NV_DEBUG_CODE(pStream->pRelocFence = pStream->pCurrentReloc + Relocs;)
    NV_DEBUG_CODE(pStream->pGatherFence = pStream->pCurrentGather + Gathers;)
    NV_DEBUG_CODE(pStream->pWaitFence = pStream->pCurrentWait + Waits;)

    return pb;
}

void NvRmStreamEnd(NvRmStream *pStream, NvData32 *pCurrent)
{
    NV_ASSERT((NvU32)(pCurrent - pStream->pCurrent) <= pStream->WordsRequested);
    NV_DEBUG_CODE((pStream)->WordsRequested = 0;)
    pStream->pCurrent = pCurrent;
}

NvData32 *NvRmStreamPushSetClass(NvRmStream *pStream, NvData32 *pCurrent,
    NvRmModuleID ModuleID, NvU32 ClassID)
{
    NVRM_STREAM_PUSH_U(pCurrent, NVRM_CH_OPCODE_SET_CLASS(ClassID, 0, 0));
    pStream->LastEngineUsed = ModuleID;
    pStream->LastClassUsed = ClassID;
    return pCurrent;
}

NvData32 *NvRmStreamPushIncr(NvRmStream *pStream, NvData32 *pCurrent,
    NvU32 SyncPointID, NvU32 Reg, NvU32 Cond, NvBool StreamSyncPoint)
{
    NV_ASSERT(pStream->LastEngineUsed != NvRmModuleID_GraphicsHost);
    NV_ASSERT(pStream->LastClassUsed != NV_HOST1X_CLASS_ID);

    NVRM_STREAM_PUSH_U(pCurrent,
        NVRM_CH_OPCODE_NONINCR(Reg, 1));
    NVRM_STREAM_PUSH_U(pCurrent,
        NV_DRF_NUM(NV_CLASS_HOST, INCR_SYNCPT, COND, Cond) |
        NV_DRF_NUM(NV_CLASS_HOST, INCR_SYNCPT, INDX, SyncPointID));

    if (StreamSyncPoint)
        pStream->SyncPointsUsed++;
    return pCurrent;
}

NvData32 *NvRmStreamPushWait(NvRmStream *pStream, NvData32 *pCurrent,
    NvRmFence Fence)
{
    NvRmModuleID moduleid = pStream->LastEngineUsed;
    NvU32 classid = pStream->LastClassUsed;

    pCurrent = NvRmStreamPushSetClass(pStream, pCurrent,
                    NvRmModuleID_GraphicsHost, NV_HOST1X_CLASS_ID);
    NVRM_STREAM_PUSH_U(pCurrent,
        NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_WAIT_SYNCPT_0, 1));
    NVRM_STREAM_PUSH_U(pCurrent,
        NV_DRF_NUM(NV_CLASS_HOST, WAIT_SYNCPT, INDX, Fence.SyncPointID) |
        NV_DRF_NUM(NV_CLASS_HOST, WAIT_SYNCPT, THRESH, Fence.Value));
    pCurrent = NvRmStreamPushWaitCheck(pStream, pCurrent, Fence);
    pCurrent = NvRmStreamPushSetClass(pStream, pCurrent, moduleid, classid);

    return pCurrent;
}

NvData32 *NvRmStreamPushWaits(NvRmStream *pStream, NvData32 *pCurrent,
    int NumFences, NvRmFence *Fences)
{
    NvRmModuleID moduleid = pStream->LastEngineUsed;
    NvU32 classid = pStream->LastClassUsed;
    int i;

    pCurrent = NvRmStreamPushSetClass(pStream, pCurrent,
                    NvRmModuleID_GraphicsHost, NV_HOST1X_CLASS_ID);
    NVRM_STREAM_PUSH_U(pCurrent,
        NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_WAIT_SYNCPT_0, NumFences));
    for (i = 0; i < NumFences; i++) {
        NvRmFence *fence = &Fences[i];
        NVRM_STREAM_PUSH_U(pCurrent,
            NV_DRF_NUM(NV_CLASS_HOST, WAIT_SYNCPT, INDX, fence->SyncPointID) |
            NV_DRF_NUM(NV_CLASS_HOST, WAIT_SYNCPT, THRESH, fence->Value));
        pCurrent = NvRmStreamPushWaitCheck(pStream, pCurrent, *fence);
    }
    pCurrent = NvRmStreamPushSetClass(pStream, pCurrent, moduleid, classid);

    return pCurrent;
}

NvData32 *NvRmStreamPushWaitLast(NvRmStream *pStream, NvData32 *pCurrent,
    NvU32 SyncPointID, NvU32 WaitBase, NvU32 Reg, NvU32 Cond)
{
    NvRmModuleID moduleid = pStream->LastEngineUsed;
    NvU32 classid = pStream->LastClassUsed;

    NV_ASSERT(pStream->LastEngineUsed != 0);
    NV_ASSERT(pStream->LastEngineUsed == NvRmModuleID_3D);

    pCurrent = NvRmStreamPushIncr(pStream, pCurrent, SyncPointID, Reg, Cond, 1);
    pCurrent = NvRmStreamPushSetClass(pStream, pCurrent,
        NvRmModuleID_GraphicsHost, NV_HOST1X_CLASS_ID);
    NVRM_STREAM_PUSH_U(pCurrent,
        NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_WAIT_SYNCPT_BASE_0, 1));
    NVRM_STREAM_PUSH_U(pCurrent,
        NV_DRF_NUM(NV_CLASS_HOST, WAIT_SYNCPT_BASE, INDX, SyncPointID) |
        NV_DRF_NUM(NV_CLASS_HOST, WAIT_SYNCPT_BASE, BASE_INDX, WaitBase) |
        NV_DRF_NUM(NV_CLASS_HOST, WAIT_SYNCPT_BASE, OFFSET,
                pStream->SyncPointsUsed));
    pCurrent = NvRmStreamPushSetClass(pStream, pCurrent, moduleid, classid);

    return pCurrent;
}

NvData32 *NvRmStreamPushReloc(NvRmStream *pStream, NvData32 *pCurrent,
    NvRmMemHandle hMem, NvU32 Offset, NvU32 RelocShift)
{
    NvRmCmdBufRelocation *pReloc = pStream->pCurrentReloc;
    NV_ASSERT(pReloc <= pStream->pRelocFence);
    pReloc->pTarget = pCurrent;
    pReloc->hMem = hMem;
    pReloc->Offset = Offset;
    pReloc->Shift = RelocShift;
    pStream->pCurrentReloc = pReloc + 1;
    (pCurrent++)->u = 0xDEADBEEF;

    return pCurrent;
}

NvData32 *NvRmStreamPushGather(NvRmStream *pStream, NvData32 *pCurrent,
    NvRmMemHandle hMem, NvU32 Offset, NvU32 Words)
{
    NvRmCmdBufGather *pGather = pStream->pCurrentGather;
    NV_ASSERT(pGather <= pStream->pGatherFence);
    pGather->pCurrent = pCurrent;
    pGather->hMem = hMem;
    pGather->Offset = Offset;
    pGather->Words = Words;
    pStream->pCurrentGather = pGather + 1;

    return pCurrent;
}

NvData32 *NvRmStreamPushWaitCheck(NvRmStream *pStream, NvData32 *pCurrent,
    NvRmFence Fence)
{
    NvRmCmdBufWait *pWait = pStream->pCurrentWait;
    NV_ASSERT(pWait <= pStream->pWaitFence);
    pWait->pCurrent = pCurrent - 1;
    pWait->Wait.SyncPointID = Fence.SyncPointID;
    pWait->Wait.Value = Fence.Value;
    pStream->pCurrentWait = pWait + 1;
    pStream->SyncPointsWaited |= (1 << (Fence.SyncPointID));

    return pCurrent;
}

void NvRmStreamFlush(NvRmStream *pStream, NvRmFence *pFence)
{
    NvRmCmdBuf *cmdbuf = &pStream->pRmPriv->CmdBuf;

    NV_ASSERT( pStream->LastEngineUsed != (NvRmModuleID)0 );
    NV_ASSERT( pStream->WordsRequested == 0 );

    // We need to copy the contents of the stream buf over into the cmdbuf.  Do
    // we have space to do this?
    if (!NvRmPrivCheckSpace(pStream, 0, 0, 0, 0))
    {
        // No.  First, submit the stuff already in the cmdbuf to the hardware.
        NvRmPrivFlush(pStream);
        if (!NvRmPrivCheckSpace(pStream, 0, 0, 0, 0))
        {
            // We still don't have space.  This means we've run out of this
            // half of the ping-pong buffer and need to flip over to the
            // other side.
            if (cmdbuf->PongSyncPointID != NVRM_INVALID_SYNCPOINT_ID)
            {
                NvRmChannelSyncPointWait(
                    cmdbuf->hDevice,
                    cmdbuf->PongSyncPointID,
                    cmdbuf->PongSyncPointValue,
                    cmdbuf->sem);
            }
            NvRmPrivSetPong(cmdbuf, !cmdbuf->pong);
            cmdbuf->last = cmdbuf->current;
            cmdbuf->PongSyncPointID = cmdbuf->SyncPointID;
            cmdbuf->PongSyncPointValue = cmdbuf->SyncPointValue;

            // Now, we surely *must* have space to copy the stream buf over...
            NV_ASSERT(NvRmPrivCheckSpace(pStream, 0, 0, 0, 0));
        }
    }

    // Now we can submit
    NvRmPrivCopyToCmdbuf(pStream);
    NvRmPrivFlush(pStream);

    if (pFence)
    {
        pFence->SyncPointID = pStream->SyncPointID;
        pFence->Value = cmdbuf->SyncPointValue;
    }
}

NvError
NvRmStreamRead3DRegister(NvRmStream *pStream, NvU32 Offset, NvU32* Value)
{
    NvRmCmdBuf *cmdbuf = &pStream->pRmPriv->CmdBuf;

    return NvRmChannelRead3DRegister(cmdbuf->hChannel, Offset, Value);
}

NvError
NvRmStreamSetPriorityEx(NvRmStream *pStream, NvU32 Priority,
        NvU32 SyncPointIndex, NvU32 WaitBaseIndex,
        NvU32 *SyncPointID, NvU32 *WaitBase)
{
    NvError err;
    NvRmCmdBuf *cmdbuf = &pStream->pRmPriv->CmdBuf;

    err = NvRmChannelSetPriority(cmdbuf->hChannel, Priority,
        SyncPointIndex, WaitBaseIndex, SyncPointID, WaitBase);
    if (err == NvSuccess) {
        cmdbuf->SyncPointWaitBaseIDShadow[NvRmModuleID_3D] =
            NvRmChannelGetModuleWaitBase(cmdbuf->hChannel, NvRmModuleID_3D, 0);
    }

    return err;
}

NvError
NvRmStreamSetPriority(NvRmStream *pStream, NvU32 Priority)
{
    return NvRmStreamSetPriorityEx(pStream, Priority, 0, 0, NULL, NULL);
}
