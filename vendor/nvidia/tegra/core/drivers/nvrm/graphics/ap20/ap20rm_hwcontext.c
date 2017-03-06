/*
 * Copyright (c) 2007 - 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvrm_drf.h"
#include "nvassert.h"
#include "ap20/ap20rm_hwcontext.h"
#include "ap20/arhost1x.h"
#include "ap20/arhost1x_uclass.h"
#include "ap20/arhost1x_channel.h"
#include "ap20/class_ids.h"
#include "ap20/arhost1x_sync.h"
#include "ap20/ap20rm_channel.h"
#include "ap20/armpe.h"

// FIXME: repeats code from ap15rm_channel.c.
#define CHANNEL_REGR( ch, reg ) \
    NV_READ32( (ch)->ChannelRegs + ((HOST1X_CHANNEL_##reg##_0)/4) )

static NvOsSemaphoreHandle s_ContextReadSem;
static NvOsSemaphoreHandle s_ContextReadSemMpe;

NvOsMutexHandle g_hwcontext_mutex = NULL;

typedef struct RmContextQueue_t
{
    NvRmContextHandle hContext;
    NvRmMemHandle hHostWait;
    NvU32 SyncPointID;
    struct RmContextQueue_t *next;
    NvRmChHw *HwState;
} RmContextQueue;

// FIXME: assuming single channel for now.
static RmContextQueue *s_ContextQueueHead = NULL;
static RmContextQueue *s_ContextQueueTail = NULL;

static RmContextQueue s_QueueElement;
static RmContextQueue *s_ContextQueueSingleElement = NULL;
static NvRmContext s_Context;

// Context queue mutex.
static NvOsMutexHandle s_HwContextMutex;
extern NvBool s_IsT30;




static void
NvRmPrivSetContextSingle( NvRmChannelHandle hChannel,
    NvRmContextHandle hContext, NvU32 SyncPointID, NvU32 SyncPointValue )
{
    NvRmDeviceHandle hDevice = hChannel->hDevice;

    if( s_ContextQueueSingleElement ) {
        NvRmContextHandle hCtx = s_ContextQueueSingleElement->hContext; (void)hCtx;
        NV_ASSERT(s_ContextQueueSingleElement->SyncPointID == SyncPointID);
        NV_ASSERT(s_ContextQueueSingleElement->HwState == hChannel->HwState);
        NV_ASSERT(hCtx->ContextRestoreOffset == hContext->ContextRestoreOffset);
        NV_ASSERT(hCtx->ContextRestoreSize == hContext->ContextRestoreSize);
        NV_ASSERT(hCtx->ContextRegisters == hContext->ContextRegisters);
    }
    else
    {
        RmContextQueue *ctx = s_ContextQueueSingleElement = &s_QueueElement;

        s_Context = *hContext;
        ctx->hContext = &s_Context;
        ctx->hHostWait = 0;
        ctx->SyncPointID = SyncPointID;
        ctx->HwState = hChannel->HwState;
    }

    if( hContext->Engine == NvRmModuleID_3D && !s_IsT30)
    NvRmPrivSyncPointSchedule( hDevice, SyncPointID, SyncPointValue,
                                    s_ContextReadSem );
    else if( hContext->Engine == NvRmModuleID_Mpe )
        NvRmPrivSyncPointSchedule( hDevice, SyncPointID, SyncPointValue,
                                    s_ContextReadSemMpe );
}

// SyncPointValue is the value we'll be waiting for.
static void
NvRmPrivAddContext( NvRmChannelHandle hChannel, NvRmContextHandle hContext,
    NvRmContextHandle newContext,
    NvRmMemHandle hHostWait,
    NvU32 SyncPointID, NvU32 SyncPointValue )
{
    NvRmDeviceHandle hDevice = hChannel->hDevice;
    RmContextQueue *ctx;

    ctx = NvOsAlloc( sizeof(RmContextQueue) );
    if( !ctx )
    {
        NV_ASSERT(!"NvRmPrivAddContext: NvOsAlloc failed");
        return;// NvError_InsufficientMemory;
    }

    ctx->hContext = hContext;
    ctx->hHostWait = hHostWait;
    ctx->SyncPointID = SyncPointID;
    ctx->HwState = hChannel->HwState;

    NvOsMutexLock( s_HwContextMutex );
    ctx->hContext->RefCount++;
    if ( s_ContextQueueTail )
    {
        s_ContextQueueTail->next = ctx;
        s_ContextQueueTail = ctx;
        s_ContextQueueTail->next = NULL;
    }
    else
    {
        ctx->next = NULL;
        s_ContextQueueHead = s_ContextQueueTail = ctx;
    }
    NvOsMutexUnlock( s_HwContextMutex );

    // This is bad.  The semaphore should be a field in hContext structure,
    // then we will not special cases.
    if(hContext->Engine == NvRmModuleID_3D)
    NvRmPrivSyncPointSchedule( hDevice, SyncPointID, SyncPointValue,
                                    s_ContextReadSem );
    else if(hContext->Engine == NvRmModuleID_Mpe)
        NvRmPrivSyncPointSchedule( hDevice, SyncPointID, SyncPointValue,
                                    s_ContextReadSemMpe );
}

static NvU32
NvRmPrivGetHostWaitCommands( NvRmChannelHandle hChannel, NvRmMemHandle *pMem,
    NvU32 ClassID, NvU32 SyncPointID, NvU32 SyncPointValue, NvRmModuleID ModuleID )
{
    NvError err;
    NvU32 base;
    NvU32 TotalSyncPointIncr = 0;

#define CONTEXT_SAVE_FIRST_CHUNK_SIZE 7
    // Second chunk is NvRmContest::hContestSave.
#define CONTEXT_SAVE_THIRD_CHUNK_SIZE 6
    NvU32 commands[CONTEXT_SAVE_FIRST_CHUNK_SIZE+CONTEXT_SAVE_THIRD_CHUNK_SIZE];
    NvU32 *cptr = commands;

    // Words 0-6 are executed right before register reads.

    // set class to the unit to flush
    *cptr++ = NVRM_CH_OPCODE_SET_CLASS( ClassID, 0, 0) ;
    // sync point increment
    *cptr++ = NVRM_CH_OPCODE_NONINCR( 0x0, 1 );
    *cptr++ = (1UL << 8) | (NvU8)(SyncPointID & 0xff);
    // set class to host
    *cptr++ = NVRM_CH_OPCODE_SET_CLASS( NV_HOST1X_CLASS_ID, 0, 0 );
    // wait for SyncPointValue+1
    *cptr++ = NVRM_CH_OPCODE_NONINCR( NV_CLASS_HOST_WAIT_SYNCPT_0, 1 );
    *cptr++ =
        NV_DRF_NUM( NV_CLASS_HOST, WAIT_SYNCPT, INDX, SyncPointID ) |
        NV_DRF_NUM( NV_CLASS_HOST, WAIT_SYNCPT, THRESH, SyncPointValue+1 );
    // set class back to the unit
    *cptr++ = NVRM_CH_OPCODE_SET_CLASS( ClassID, 0, 0 );
    NV_ASSERT( cptr-commands == CONTEXT_SAVE_FIRST_CHUNK_SIZE );

    // the reg read cmd buffer set the sync point to SyncPointValue+2
    // Words 7-12 are executed after reads.
    if(ModuleID == NvRmModuleID_3D)
        TotalSyncPointIncr = 3;
    else if(ModuleID == NvRmModuleID_Mpe)
        TotalSyncPointIncr = 2;

    *cptr++ = NVRM_CH_OPCODE_SET_CLASS( NV_HOST1X_CLASS_ID, 0, 0 );
    // wait for SyncPointValue+3
    *cptr++ = NVRM_CH_OPCODE_NONINCR( NV_CLASS_HOST_WAIT_SYNCPT_0, 1 );
    *cptr++ =
        NV_DRF_NUM( NV_CLASS_HOST, WAIT_SYNCPT, INDX, SyncPointID ) |
        NV_DRF_NUM( NV_CLASS_HOST, WAIT_SYNCPT, THRESH, SyncPointValue+TotalSyncPointIncr );

    // increment sync point base by three.
    base = NvRmChannelGetModuleWaitBase( hChannel, ModuleID, 0 );
            *cptr++ = NVRM_CH_OPCODE_NONINCR( NV_CLASS_HOST_INCR_SYNCPT_BASE_0, 1 );
            *cptr++ =
                NV_DRF_NUM(NV_CLASS_HOST, INCR_SYNCPT_BASE, BASE_INDX, base ) |
                NV_DRF_NUM(NV_CLASS_HOST, INCR_SYNCPT_BASE, OFFSET, TotalSyncPointIncr);

    // set class back to the unit
    *cptr++ = NVRM_CH_OPCODE_SET_CLASS( ClassID, 0, 0 );
    NV_ASSERT( cptr-commands ==
        CONTEXT_SAVE_FIRST_CHUNK_SIZE+CONTEXT_SAVE_THIRD_CHUNK_SIZE);

    err = NvRmMemHandleCreate(hChannel->hDevice, pMem, sizeof(commands));
    NV_ASSERT(err == NvSuccess);

    // FIXME: need better error handling.  This silly thing is here so that
    // ARM compiler does not complain about err being set and not used in the
    // release build.
    if (err != NvSuccess)
        return 0;

    // FIXME: we should have pre-allocated set instead.
    err = NvRmMemAllocTagged(*pMem, NULL, 0, 32,
        NvOsMemAttribute_Uncached,
        NVRM_MEM_TAG_RM_MISC);
    NV_ASSERT(err == NvSuccess);

    NvRmMemPin( *pMem );

    NvRmMemWrite( *pMem, 0, commands, sizeof(commands) );

    return SyncPointValue+TotalSyncPointIncr;
}

NvU32
NvRmPrivCheckForContextChange( NvRmChannelHandle hChannel,
    NvRmContextHandle hContext, NvRmModuleID ModuleID,
    NvU32 SyncPointID, NvU32 SyncPointValue, NvBool DryRun )
{
    NvU32 ret = 0;
    NvRmContext *lastContext;

    // If the new context is NULL, it means we're not interested in storing
    // and restoring it.  We'll be able to switch back to the last context
    // without extra register saves and restores.
    if( !hContext )
        return ret;

    lastContext = NVRM_CHANNEL_CONTEXT_GET(hChannel, ModuleID);

    if( hContext == lastContext ) {
        // It can happen that the previous submit operation did not want to
        // store and restore context, but it did change the current class.
        // This restores the class back to what the current context expects.

        if( !DryRun ) {
            NvRmCommandBuffer cmdBufs[1];

            cmdBufs[0].hMem = hContext->hContextNoChange;
            cmdBufs[0].Offset = 0;
            cmdBufs[0].Words = hContext->ContextNoChangeSize;

            NvRmPrivChannelSubmitCommandBufs( hChannel, cmdBufs, 1,
                NULL, NULL, 0,
                SyncPointID, SyncPointValue, NV_FALSE );
        }
        return ret;
    }

    // Save the context if we care.
    if( lastContext && lastContext != FAKE_POWERDOWN_CONTEXT )
    {
        if(ModuleID == NvRmModuleID_3D)
            ret = 3;
        else if(ModuleID == NvRmModuleID_Mpe)
            ret = 2;

        if( !DryRun ) {
            if( lastContext->Queueless )
            {
                NvRmCommandBuffer cmdBufs[2];

                SyncPointValue += lastContext->QueuelessSyncpointIncrement;
                NV_ASSERT(lastContext->QueuelessSyncpointIncrement == 3);
                NvRmPrivSetContextSingle( hChannel, lastContext, SyncPointID, SyncPointValue-1 );

                cmdBufs[0].hMem = lastContext->hContextSetAddress;
                cmdBufs[0].Offset = 0;
                cmdBufs[0].Words = lastContext->ContextSetAddressSize;

                cmdBufs[1].hMem = lastContext->hContextSave;
                cmdBufs[1].Offset = 0;
                cmdBufs[1].Words = lastContext->ContextSaveSize;

                NvRmPrivChannelSubmitCommandBufs( hChannel, cmdBufs,
                    NV_ARRAY_SIZE(cmdBufs), NULL, NULL, 0,
                    SyncPointID, SyncPointValue,
                    NV_FALSE );
            }
            else
            {
                NvRmMemHandle HostWaitCommands;
                NvRmCommandBuffer cmdBufs[3];

                SyncPointValue = NvRmPrivGetHostWaitCommands( hChannel,
                    &HostWaitCommands, lastContext->ClassID,
                    SyncPointID, SyncPointValue, ModuleID );
                NvRmPrivAddContext( hChannel, lastContext, hContext,
                    HostWaitCommands, SyncPointID, SyncPointValue-1 );

                cmdBufs[0].hMem = HostWaitCommands;
                cmdBufs[0].Offset = 0;
                cmdBufs[0].Words = CONTEXT_SAVE_FIRST_CHUNK_SIZE;

                cmdBufs[1].hMem = lastContext->hContextSave;
                cmdBufs[1].Offset = 0;
                cmdBufs[1].Words = lastContext->ContextSaveSize;

                cmdBufs[2].hMem = HostWaitCommands;
                cmdBufs[2].Offset = CONTEXT_SAVE_FIRST_CHUNK_SIZE*4;
                cmdBufs[2].Words = CONTEXT_SAVE_THIRD_CHUNK_SIZE;

                NvRmPrivChannelSubmitCommandBufs( hChannel, cmdBufs,
                    NV_ARRAY_SIZE(cmdBufs), NULL, NULL, 0,
                    SyncPointID, SyncPointValue,
                    NV_FALSE );
            }

            // Mark the context as valid.
            lastContext->IsValid = NV_TRUE;
            lastContext->SyncPointID = SyncPointID;
            lastContext->SyncPointValue = SyncPointValue;
        }
    }

    // If the new context is valid, restore it.
    NV_ASSERT(hContext);
    if( hContext != FAKE_POWERDOWN_CONTEXT && hContext->IsValid )
    {
        ret++;

        if( !DryRun ) {
            NvRmCommandBuffer cmdBufs[1];

            cmdBufs[0].hMem = hContext->hContextRestore;
            cmdBufs[0].Offset = 0;
            cmdBufs[0].Words = hContext->ContextRestoreSize;

            // hContext->SyncPointID was initialized at context creation time.
            NV_ASSERT( hContext->SyncPointID == SyncPointID );
            // FIXME: locking.  Mustn't fight with assigning stuff to
            // lastContext in the previous if.
            SyncPointValue++;
            hContext->SyncPointValue = SyncPointValue;

            NvRmPrivChannelSubmitCommandBufs( hChannel, cmdBufs, 1,
                NULL, NULL, 0,
                SyncPointID, SyncPointValue, NV_FALSE );
        }
    }

    if( !DryRun )
        NVRM_CHANNEL_CONTEXT_SET(hChannel, ModuleID, hContext);

    return ret;
}




static volatile NvBool s_ShutdownContextReadThread;
static NvOsThreadHandle s_hContextReadThread;

static void NvRmPrivContextReadThread( void *args )
{
    NvRmDeviceHandle rm = (NvRmDeviceHandle)args;

    /* avoid a warning */
    rm = rm;

    for( ;; )
    {
        NvRmChHw *hw;
        NvRmContextHandle hCtx;
        NvU32 SyncPointID;

        NvOsSemaphoreWait( s_ContextReadSem );

        if( !s_ContextQueueSingleElement )
        {
            NvOsMutexLock( s_HwContextMutex );

            // This thread will be shut down only when all context reads have made
            // it through; otherwise the host will stall on data pending in OUTF.
            if( s_ShutdownContextReadThread && s_ContextQueueHead == NULL )
            {
                if( !s_ContextQueueSingleElement )
                    NvOsMutexUnlock( s_HwContextMutex );
                break;
            }
            NV_ASSERT( s_ContextQueueHead );

            hCtx = s_ContextQueueHead->hContext;
            hw = s_ContextQueueHead->HwState;
            SyncPointID = s_ContextQueueHead->SyncPointID;
        }
        else
        {
            hCtx = s_ContextQueueSingleElement->hContext;
            hw = s_ContextQueueSingleElement->HwState;
            SyncPointID = s_ContextQueueSingleElement->SyncPointID;
        }

        HOST_CLOCK_ENABLE( rm );

        {
            NvU32 temp = 0;
            NvU32 Offset = hCtx->ContextRestoreOffset*4;
            NvRmMemHandle hRestore;
            const NvRm3DContext *rid;
            NvU32 *vaddr;
            const NvU32 MapSize = hCtx->ContextRestoreSize*4-Offset;

#define READ_CONTEXT() \
            do { \
            for( rid = hCtx->ContextRegisters; rid->offset != 0xffff; rid++ )\
            {                                                                \
                NvS16 count = rid->count;                                    \
                if( count == 0 )                                             \
                    continue;                                                \
                                                                             \
                WRITE_WORD( count > 0 ?                                      \
                    NVRM_CH_OPCODE_INCR( rid->offset, count ) :              \
                    NVRM_CH_OPCODE_NONINCR( rid->offset, -count ));          \
                                                                             \
                /* For indirect registers, the first one is the address, */  \
                /* program it with zero. */                                  \
                if( (rid+1)->count < 0)                                      \
                {                                                            \
                    NV_ASSERT( count == 1 );                                 \
                    WRITE_WORD(0);                                           \
                    continue;                                                \
                }                                                            \
                                                                             \
                count = count > 0 ? count : -count;                          \
                                                                             \
                if ((count & 7) == 0)                                        \
                while( count )                                               \
                {                                                            \
                    NvU32 word1, word2, word3, word4;                        \
                    NvS32 FifoTries = 100;                                   \
                    while( temp < 8 )                                        \
                    {                                                        \
                        temp = CHANNEL_REGR(hw, FIFOSTAT);                   \
                        temp = NV_DRF_VAL(HOST1X_CHANNEL, FIFOSTAT,          \
                            OUTFENTRIES, temp);                              \
                                                                             \
                        if (FifoTries > 0)                                   \
                            FifoTries--;                                     \
                        else if (FifoTries == 0) {                           \
                            NvOsDebugPrintf("Output FIFO does not refill, "  \
                                "context read is stuck.");                   \
                            FifoTries--;                                     \
                        }                                                    \
                        else                                                 \
                            NvOsThreadYield();                               \
                    }                                                        \
                    temp -= 8;                                               \
                    count -= 8;                                              \
                                                                             \
                    word1 = CHANNEL_REGR(hw, INDDATA);                       \
                    word2 = CHANNEL_REGR(hw, INDDATA);                       \
                    word3 = CHANNEL_REGR(hw, INDDATA);                       \
                    word4 = CHANNEL_REGR(hw, INDDATA);                       \
                    WRITE_WORD(word1);                                       \
                    WRITE_WORD(word2);                                       \
                    WRITE_WORD(word3);                                       \
                    WRITE_WORD(word4);                                       \
                    word1 = CHANNEL_REGR(hw, INDDATA);                       \
                    word2 = CHANNEL_REGR(hw, INDDATA);                       \
                    word3 = CHANNEL_REGR(hw, INDDATA);                       \
                    word4 = CHANNEL_REGR(hw, INDDATA);                       \
                    WRITE_WORD(word1);                                       \
                    WRITE_WORD(word2);                                       \
                    WRITE_WORD(word3);                                       \
                    WRITE_WORD(word4);                                       \
                }                                                            \
                else                                                         \
                while( count-- )                                             \
                {                                                            \
                    NvU32 word;                                              \
                    NvS32 FifoTries = 100;                                   \
                    while( temp == 0 )                                       \
                    {                                                        \
                        temp = CHANNEL_REGR(hw, FIFOSTAT);                   \
                        temp = NV_DRF_VAL(HOST1X_CHANNEL, FIFOSTAT,          \
                            OUTFENTRIES, temp);                              \
                                                                             \
                        if (FifoTries > 0)                                   \
                            FifoTries--;                                     \
                        else if (FifoTries == 0) {                           \
                            NvOsDebugPrintf("Output FIFO does not refill, "  \
                                "context read is stuck.");                   \
                            FifoTries--;                                     \
                        }                                                    \
                        else                                                 \
                            NvOsThreadYield();                               \
                    }                                                        \
                    temp--;                                                  \
                                                                             \
                    word = CHANNEL_REGR(hw, INDDATA);                        \
                    WRITE_WORD(word);                                        \
                }                                                            \
            }                                                                \
                                                                             \
            /* Sync point increment to help with freeing the context. */     \
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR( 0x0, 1 ) );                  \
            WRITE_WORD( ((1UL << 8) | (NvU8)(SyncPointID & 0xff) ));         \
            } while (0)

            // In queueless mode the first register read returns the memory
            // handle to store the context in.
            {
                NvU32 m;
                while( temp < 8 )
                {
                    temp = CHANNEL_REGR(hw, FIFOSTAT);
                    temp = NV_DRF_VAL(HOST1X_CHANNEL, FIFOSTAT, OUTFENTRIES,
                        temp);
                }
                m = CHANNEL_REGR(hw, INDDATA);
                if (hCtx->Queueless)
                    hRestore = (NvRmMemHandle)m;
                else
                    hRestore = hCtx->hContextRestore;
            }

            if( NvRmMemMap(hRestore, Offset, MapSize, NVOS_MEM_WRITE,
                    (void **) &vaddr) == NvSuccess )
            {
#define WRITE_WORD(data) \
                do { \
                    *vaddr++ = data; \
                    Offset += 4; \
                } while (0)
                READ_CONTEXT();
#undef WRITE_WORD
                NvRmMemUnmap(hRestore, vaddr-MapSize/4, MapSize);
            }
            else {
#define WRITE_WORD(data) \
            do { \
                NvRmMemWr32( hRestore, Offset, data ); \
                Offset += 4; \
            } while (0)
                READ_CONTEXT();
#undef WRITE_WORD
            }
            NV_ASSERT( Offset == hCtx->ContextRestoreSize*4 );
        }

        {
            NvU32 reg = NV_DRF_NUM( HOST1X_SYNC, SYNCPT_CPU_INCR,
                    SYNCPT_CPU_INCR_VECTOR,
                    1 << SyncPointID );
            HOST1X_SYNC_REGW( HOST1X_SYNC_SYNCPT_CPU_INCR_0, reg );
        }

        HOST_CLOCK_DISABLE( rm );

        if( !s_ContextQueueSingleElement )
        {
            // Free the queue element and its buffers.
            NvRmMemUnpin(s_ContextQueueHead->hHostWait);
            NvRmMemHandleFree( s_ContextQueueHead->hHostWait );

            hCtx->RefCount--;
            if ( !s_ContextQueueHead->next ) {
                s_ContextQueueTail = NULL;
                NV_ASSERT(hCtx->RefCount==0);
            }

            {
                RmContextQueue *NewQueueHead = s_ContextQueueHead->next;
                NvOsFree( s_ContextQueueHead );
                s_ContextQueueHead = NewQueueHead;
            }

            NvOsMutexUnlock( s_HwContextMutex );
        }
    }
}


static volatile NvBool s_ShutdownContextReadThreadMpe;
static NvOsThreadHandle s_hContextReadThreadMpe;

//word1[0] = MPEA_BUFFER_FULL_READ_0
//word1[1] = MPEA_LENGTH_OF_STREAM_0
//word1[2] = MPEA_BUFFER_DEPLETION_RATE_0
//word1[3] = MPEA_ENC_FRAME_NUM_0
//word1[4] = h264_mode
//word1[5] = gop_flag
//word1[6] = frame_num_gop
//word1[7] = ctx_save_misc

//restore[0] = MPEA_BUFFER_FULL_0
//restore[1] = MPEA_REPORTED_FRAME_0


#define MAX(a,b) (((a)>(b))?(a):(b))
#define RC_RAM_SIZE 692
#define IRFR_RAM_SIZE 408
#define GOP_LENGTH_MINUS1 74

static void NvRmPrivContextReadThreadMpe( void *args )
{
    NvRmDeviceHandle rm = (NvRmDeviceHandle)args;

    for( ;; )
    {
        NvRmChHw *hw;
        NvRmContextHandle hCtx;

        NvOsSemaphoreWait( s_ContextReadSemMpe );
        NvOsMutexLock( s_HwContextMutex );

        // This thread will be shut down only when all context reads have made
        // it through; otherwise the host will stall on data pending in OUTF.
        if( s_ShutdownContextReadThreadMpe && s_ContextQueueHead == NULL )
        {
            NvOsMutexUnlock( s_HwContextMutex );
            break;
        }
        NV_ASSERT( s_ContextQueueHead );

        hCtx = s_ContextQueueHead->hContext;
        hw = s_ContextQueueHead->HwState;

        HOST_CLOCK_ENABLE( rm );

        if (1)
        {
            NvU32 temp = 0;
            NvU32 Offset = hCtx->ContextRestoreOffset*4;
            NvRmMemHandle hRestore = hCtx->hContextRestore;
            NvRmMemHandle hRestoreBugFix = hCtx->hContextMpeHwBugFix;

            const NvRmMpeContext *rid;
            NvU32 word1[8], restore[2];
            NvU32 i, j = 0, n = 0;
            NvU32 h264Mode=0;

            NvU32 frm_num_gop = 0, gop_flag = 0xff;

#define CSIM_OR_QT 0
#if !(CSIM_OR_QT)
            NvU32 *vaddr;
            const NvU32 MapSize = hCtx->ContextRestoreSize*4-Offset;
            NvU32 err = NvRmMemMap(hRestore, Offset, MapSize, NVOS_MEM_WRITE,
                                   (void **) &vaddr);
            NV_ASSERT(err == NvSuccess); (void)err;
#define WRITE_WORD(data) \
            do { \
                *vaddr++ = data; \
                Offset += 4; \
            } while (0)
#else
//NvRmMemMap fails on csim so use NvRmMemWr32 instead.
#define WRITE_WORD(data) \
    do { \
        NvRmMemWr32( hRestore, Offset, data ); \
        Offset += 4; \
    } while (0)
#endif //CSIM_OR_QT

            //Then save and restore the set of registers.
            for( rid = hCtx->ContextRegistersMpe, i = 0; rid->offset != 0xffff; rid++,i++ )
            {
                int newBufferFull;
                NvU32 repFrame;
                NvS16 count = rid->count;
                if( count == 0 )
                    continue;
                if( rid->count == 1)
                {
                    NvU32 word;
                    WRITE_WORD(NVRM_CH_OPCODE_INCR( rid->offset, count ));

                    while( temp == 0 )
                    {
                        temp = CHANNEL_REGR(hw, FIFOSTAT);
                        temp = NV_DRF_VAL(HOST1X_CHANNEL, FIFOSTAT, OUTFENTRIES, temp);
                    }
                    temp--;
                    word = CHANNEL_REGR(hw, INDDATA);
                    WRITE_WORD(word);

                }
                else if( rid->count == 2 )
                {
                    NvU32 word;
                    count = 1;
                    WRITE_WORD(NVRM_CH_OPCODE_INCR( rid->offset, count ));
                    while( temp == 0 )
                    {
                        temp = CHANNEL_REGR(hw, FIFOSTAT);
                        temp = NV_DRF_VAL(HOST1X_CHANNEL, FIFOSTAT, OUTFENTRIES, temp);
                    }
                    temp--;
                    word = CHANNEL_REGR(hw, INDDATA);
                    word1[n++] = word;
                    WRITE_WORD(word);
                }
                else if( rid->count == 3)
                {
                    NvU32 word;
                    count = 1;
                    WRITE_WORD(NVRM_CH_OPCODE_INCR( rid->offset, count ));

                    while( temp == 0 )
                    {
                        temp = CHANNEL_REGR(hw, FIFOSTAT);
                        temp = NV_DRF_VAL(HOST1X_CHANNEL, FIFOSTAT, OUTFENTRIES, temp);
                    }
                    temp--;
                    word = CHANNEL_REGR(hw, INDDATA);
                    {
                        NvU32 bufferFullRead = NV_DRF_VAL( MPEA, BUFFER_FULL_READ,      BUFFER_FULL_READ,       word1[0] );
                        NvU32 byteLen        = NV_DRF_VAL( MPEA, LENGTH_OF_STREAM,      STREAM_BYTE_LEN,        word1[1] );
                        NvU32 drain          = NV_DRF_VAL( MPEA, BUFFER_DEPLETION_RATE, BUFFER_DRAIN,           word1[2] );
                        repFrame             = NV_DRF_VAL( MPEA, ENC_FRAME_NUM,         ENC_FRAME_NUM,          word1[3] );
                        h264Mode             = NV_DRF_VAL( MPEA, VOL_CTRL,              H264_ENABLE,            word1[4] );

                        if(h264Mode)
                            byteLen = byteLen/8; //register value in bits(contvert to bytes)

                        newBufferFull = bufferFullRead + byteLen - 4*drain; //bytes
                        newBufferFull = MAX(0, newBufferFull);
                        restore[0] = newBufferFull;
                        restore[1] = repFrame;
                    }

                    if(restore[1] == 0) //(repFrame == 0)
                        word = NV_FLD_SET_DRF_NUM(MPEA, FRAME_INDEX, P_FRAME_INDEX, 0, word);
                    WRITE_WORD(word);
                }
                else if( rid->count == -1 )
                {
                    count = 1;
                    WRITE_WORD(NVRM_CH_OPCODE_INCR( rid->offset, count ));
                    WRITE_WORD(restore[j]);
                    j++;
                }
            }
            /* s/w work around for h/w bug 526915 */
            if(h264Mode)
            {
                NvU32 Save, RcRamOffset, data1;
                NvU32 add_to_modify, upt_filt_ptr;

                Save = NvRmMemRd32( hRestoreBugFix, 1*4);
                gop_flag = NV_DRF_VAL( MPEA, GOP_PARAM, GOP_FLAG, word1[5]);
                if(gop_flag == 0)
                {
                    frm_num_gop = NV_DRF_VAL( MPEA, FRAME_NUM_GOP, FRAME_NUM_GOP, word1[6]);
                    if((frm_num_gop == GOP_LENGTH_MINUS1) && (Save == 0))
                    {
                        upt_filt_ptr = NV_DRF_VAL(MPEA, CONTEXT_SAVE_MISC, UPT_FILT_PTRS, word1[7]);
                        upt_filt_ptr = upt_filt_ptr >> 5; //upt_filt_ptr[9:5]
                        if(upt_filt_ptr == 0)
                            add_to_modify = 19;
                        else
                            add_to_modify = upt_filt_ptr - 1;

                        RcRamOffset = 612 + 2*add_to_modify;
                        NvRmMemWr32( hRestoreBugFix, 0*4, RcRamOffset );
                        data1 = 1; //save
                        NvRmMemWr32( hRestoreBugFix, 1*4, data1 );
                        data1 = 0; //restore
                        NvRmMemWr32( hRestoreBugFix, 2*4, data1 );
                    }
                    else if((frm_num_gop == 0) && (Save == 1))
                    {
                        data1 = 0; //save
                        NvRmMemWr32( hRestoreBugFix, 1*4, data1 );
                        data1 = 1; //restore
                        NvRmMemWr32( hRestoreBugFix, 2*4, data1 );

                    }
                }
            }
            //First save and restore RC-RAM contents.
            if(h264Mode)
            {
                NvU32 addr, tempRamVal=0;
                NvU32 k = 0, Save = 0, Restore = 0;
                NvU32 ram_offset, data1;
                NvU32 tmp1, tmp2, tmp3;//, tmp4;
                NvU32 mod_val1, mod_val2;

                ram_offset = NvRmMemRd32( hRestoreBugFix, 0*4);
                Save = NvRmMemRd32( hRestoreBugFix, 1*4);
                Restore = NvRmMemRd32( hRestoreBugFix, 2*4);

                addr = MPEA_RC_RAM_LOAD_CMD_0;
                WRITE_WORD(NVRM_CH_OPCODE_INCR( addr, 1 )); //+1
                WRITE_WORD( RC_RAM_SIZE ); //+1

                addr = MPEA_RC_RAM_LOAD_DATA_0;
                for(k=0; k < RC_RAM_SIZE; k++)
                {
                    while( temp == 0 )
                    {
                        temp = CHANNEL_REGR(hw, FIFOSTAT);
                        temp = NV_DRF_VAL(HOST1X_CHANNEL, FIFOSTAT, OUTFENTRIES, temp);
                    }
                    temp--;
                    tempRamVal = CHANNEL_REGR(hw, INDDATA);

                    if( (Save==1) && (Restore==0) )
                    {
                        if(k == ram_offset)
                            NvRmMemWr32( hRestoreBugFix, 3*4, tempRamVal );//WRITE_WORD_BUG_FIX(4, tempRamVal);
                        else if(k == ram_offset+1)
                            NvRmMemWr32( hRestoreBugFix, 4*4, tempRamVal );//WRITE_WORD_BUG_FIX(5, tempRamVal);
                    }
                    else if( (Save==0) && (Restore==1) )
                    {
                        if(k == ram_offset)
                            NvRmMemWr32( hRestoreBugFix, 5*4, tempRamVal );//WRITE_WORD_BUG_FIX(6, tempRamVal);
                        else if(k == ram_offset+1)
                            NvRmMemWr32( hRestoreBugFix, 6*4, tempRamVal );//WRITE_WORD_BUG_FIX(7, tempRamVal);

                        tmp1 = NvRmMemRd32( hRestoreBugFix, 3*4);
                        tmp2 = NvRmMemRd32( hRestoreBugFix, 4*4);
                        tmp3 = NvRmMemRd32( hRestoreBugFix, 5*4);
                        //tmp4 = NvRmMemRd32( hRestoreBugFix, 6*4);
                        mod_val1 = (tmp1 & 0xff000000) | (tmp3 & 0x00ffffff);
                        mod_val2 = tmp2;
                        if(k == ram_offset)
                            tempRamVal = mod_val1;
                        else if(k == ram_offset+1)
                            tempRamVal = mod_val2;
                        data1 = 0;
                        NvRmMemWr32( hRestoreBugFix, 1*4, data1 );//WRITE_WORD_BUG_FIX(1, 0); //save flag
                        NvRmMemWr32( hRestoreBugFix, 2*4, data1 );//WRITE_WORD_BUG_FIX(2, 0); //restore flag
                    }

                    WRITE_WORD(NVRM_CH_OPCODE_NONINCR( addr, 1 ));
                    WRITE_WORD( tempRamVal );
                }
                temp=0;
            }
            else
            {
                NvU32 addr, tempRamVal=0;
                NvU32 k = 0;
                addr = MPEA_RC_RAM_LOAD_CMD_0;
                WRITE_WORD(NVRM_CH_OPCODE_INCR( addr, 1 )); //+1
                WRITE_WORD( RC_RAM_SIZE ); //+1

                addr = MPEA_RC_RAM_LOAD_DATA_0;
                for(k=0; k < RC_RAM_SIZE; k++)
                {
                    while( temp == 0 )
                    {
                        temp = CHANNEL_REGR(hw, FIFOSTAT);
                        temp = NV_DRF_VAL(HOST1X_CHANNEL, FIFOSTAT, OUTFENTRIES, temp);
                    }
                    temp--;
                    tempRamVal = CHANNEL_REGR(hw, INDDATA);
                    WRITE_WORD(NVRM_CH_OPCODE_NONINCR( addr, 1 ));
                    WRITE_WORD( tempRamVal );
                }
                temp=0;
            }
            //Second save and restore IRFR-RAM contents.
            if(1)
            {
                NvU32 addr, tempRamVal=0;
                NvU32 k = 0;
                addr = MPEA_INTRA_REF_LOAD_CMD_0;
                WRITE_WORD(NVRM_CH_OPCODE_INCR( addr, 1 )); //+1
                WRITE_WORD( IRFR_RAM_SIZE ); //+1

                addr = MPEA_INTRA_REF_LOAD_DATA_0;
                for(k=0; k < IRFR_RAM_SIZE; k++)
                {
                    while( temp == 0 )
                    {
                        temp = CHANNEL_REGR(hw, FIFOSTAT);
                        temp = NV_DRF_VAL(HOST1X_CHANNEL, FIFOSTAT, OUTFENTRIES, temp);
                    }
                    temp--;
                    tempRamVal = CHANNEL_REGR(hw, INDDATA);
                    WRITE_WORD(NVRM_CH_OPCODE_NONINCR( addr, 1 ));
                    WRITE_WORD( tempRamVal );
                }
                temp=0;
            }

            // Sync point increment to help with freeing the context.
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR( 0x0, 1 ) );
            WRITE_WORD( ((1UL << 8) | (NvU8)(s_ContextQueueHead->SyncPointID & 0xff) ));
#undef WRITE_WORD
            NV_ASSERT( Offset == hCtx->ContextRestoreSize*4 );

#if !(CSIM_OR_QT)
            NvRmMemUnmap(hRestore, vaddr-MapSize/4, MapSize);
#endif
        }

        {
            NvU32 s = s_ContextQueueHead->SyncPointID;
            NvU32 reg = NV_DRF_NUM( HOST1X_SYNC, SYNCPT_CPU_INCR,
                    SYNCPT_CPU_INCR_VECTOR,
                    1 << s );
            HOST1X_SYNC_REGW( HOST1X_SYNC_SYNCPT_CPU_INCR_0, reg );
        }
        HOST_CLOCK_DISABLE( rm );

        // Free the queue element and its buffers.
        NvRmMemUnpin(s_ContextQueueHead->hHostWait);
        NvRmMemHandleFree( s_ContextQueueHead->hHostWait );

        hCtx->RefCount--;
        if ( !s_ContextQueueHead->next ) {
            s_ContextQueueTail = NULL;
            NV_ASSERT(hCtx->RefCount==0);
        }

        {
            RmContextQueue *NewQueueHead = s_ContextQueueHead->next;
            NvOsFree( s_ContextQueueHead );
            s_ContextQueueHead = NewQueueHead;
        }

        NvOsMutexUnlock( s_HwContextMutex );
    }
}

void NvRmPrivContextReadHandlerInit( NvRmDeviceHandle rm )
{
    NvError err;

    err = NvOsMutexCreate(&g_hwcontext_mutex);
    if( err != NvSuccess )
    {
        goto failed;
    }

    err = NvOsSemaphoreCreate(&s_ContextReadSem, 0);
    if( err != NvSuccess )
    {
        NV_ASSERT( !"Could not create s_ContextReadSem" );
        goto failed;
    }

    err = NvOsSemaphoreCreate(&s_ContextReadSemMpe, 0);
    if( err != NvSuccess )
    {
        NV_ASSERT( !"Could not create s_ContextReadSemMpe" );
        goto failed;
    }

    err = NvOsMutexCreate( &s_HwContextMutex );
    if( err != NvSuccess )
    {
        NV_ASSERT( !"Could not create s_HwContextMutex" );
        goto failed;
    }

    if( !s_IsT30 )
    {
        s_ShutdownContextReadThread = NV_FALSE;
        err = NvOsInterruptPriorityThreadCreate(NvRmPrivContextReadThread, rm,
            &s_hContextReadThread);
        if( err != NvSuccess )
        {
            NV_ASSERT( !"Could not start rm-host-ctx thread" );
            goto failed;
        }
    }

    s_ShutdownContextReadThreadMpe = NV_FALSE;
    err = NvOsInterruptPriorityThreadCreate(NvRmPrivContextReadThreadMpe, rm,
        &s_hContextReadThreadMpe);
    if( err != NvSuccess )
    {
        NV_ASSERT( !"Could not start rm-host-ctx thread" );
        goto failed;
    }

    return;

failed:
    NvOsMutexDestroy(s_HwContextMutex);
    NvOsSemaphoreDestroy(s_ContextReadSem);
    NvOsSemaphoreDestroy(s_ContextReadSemMpe);
    NvOsMutexDestroy(g_hwcontext_mutex);
}

void
NvRmPrivContextReadHandlerDeinit( NvRmDeviceHandle rm )
{
    s_ShutdownContextReadThread = NV_TRUE;
    s_ShutdownContextReadThreadMpe = NV_TRUE;
    NvOsSemaphoreSignal( s_ContextReadSem );
    NvOsSemaphoreSignal( s_ContextReadSemMpe );
    NvOsThreadJoin( s_hContextReadThread );
    NvOsThreadJoin( s_hContextReadThreadMpe );
    NvOsMutexDestroy(g_hwcontext_mutex);
}

void NvRmPrivContextFree( NvRmContextHandle hContext )
{
    NvRmChannelHandle ch = hContext->hChannel;
    NvOsSemaphoreHandle sem = 0;
    NvError err;

    if( hContext->Engine != NvRmModuleID_3D
        && hContext->Engine != NvRmModuleID_Mpe )
    {
        return;
    }

    // FIXME: if we allow several functions in this file executing
    // simultaneously, we need to add proper locking.  Another process might
    // decide to store this context one last time and hence update
    // hContext->SyncPointValue.

    // If the context has not yet been used, the channel is NULL.
    if( ch )
    {
        err = NvOsSemaphoreCreate(&sem, 0);
        NV_ASSERT(err == NvSuccess);
        if( err != NvSuccess )
            return;

        NvRmChannelSyncPointWait( ch->hDevice,
            hContext->SyncPointID, hContext->SyncPointValue, sem );

        NvOsSemaphoreDestroy( sem );
    }

    NV_DEBUG_CODE(NvOsMutexLock( s_HwContextMutex ));
    NV_ASSERT( hContext->RefCount == 0 );
    NV_DEBUG_CODE(NvOsMutexUnlock( s_HwContextMutex ));

    switch( hContext->Engine ) {
        case NvRmModuleID_3D:   NvRmContext3DFree( hContext );  break;
        case NvRmModuleID_Mpe:  NvRmContextMpeFree( hContext ); break;
        default: NV_ASSERT(!"Context freeing not implemented for this unit");
    }
}
