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
#include "ap20/ap20rm_channel.h"

#include "ap20/arhost1x_uclass.h"
#include "ap20/class_ids.h"

#include "t30/ar3d.h"

#include "ctx3d.c"

// Number of registers to get from the read FIFO.
// This is used when context switch is not required.  Right now it's only one
// set class command.  This is needed because it is possible that another
// process was using 2D unit and left class set to 2D.  When we're back to the
// first process, we need to guarantee that class is back to 3D.
static NvRmMemHandle s_hContextNoChange = 0;
// The same context save command sequence is used for all contexts.
static NvRmMemHandle s_hContextSave = 0;
// Sizes are in words.  They are calculated once in
// NvRmContext3DInit and never change.
static NvU32 s_ContextPrepareSize;
static NvU32 s_ContextSaveSize;
static NvU32 s_ContextRestoreSize;

// To switch to the new context switcher, change this to NV_TRUE.
NvBool s_Queueless = NV_FALSE;
NvBool s_IsT30 = NV_FALSE;

// How many 3D contexts there are.  When it is zero, static blocks are freed
// as well.
static NvU32 s_3DContextCount = 0;

#define INDOFF_MACRO(indbe, autoinc, offset, rwn)               \
    (NV_DRF_NUM(NV_CLASS_HOST, INDOFF, INDBE, (indbe)) |        \
     NV_DRF_DEF(NV_CLASS_HOST, INDOFF, AUTOINC, autoinc) |      \
     NV_DRF_DEF(NV_CLASS_HOST, INDOFF, INDMODID, GR3D) |        \
     NV_DRF_NUM(NV_CLASS_HOST, INDOFF, INDROFFSET, (offset)) |  \
     NV_DRF_DEF(NV_CLASS_HOST, INDOFF, ACCTYPE, REG) |          \
     NV_DRF_DEF(NV_CLASS_HOST, INDOFF, RWN, rwn))

extern NvOsMutexHandle g_hwcontext_mutex;

static NvError NvRmContext3DInit( NvRmDeviceHandle hDevice, NvRmContext *ctx )
{
    NvError err;
    const NvRm3DContext *rid;
    NvRmMemHandle h = 0; // Will go to RestoreHandle.
    NvU32* map = NULL;
    NvU32* temp = NULL;
    NvU32* ptr;
    NvU32 RegisterCount = 0;
    // FIXME: get a channel handle
    NvU32 base = NvRmChannelGetModuleWaitBase( 0, NvRmModuleID_3D, 0 );

    NvOsMutexLock( g_hwcontext_mutex );

    // Don't need to do it more than once.
    if ( s_hContextSave )
        goto initialized;

    // First, figure out how much memory we will need.
    // Start with setting class to host and incrementing sync point base.
    s_ContextRestoreSize = 3;

    // Set class back to 3D.
    s_ContextRestoreSize++;

    // Program zero into AR3D_PSEQ_QUAD_ID_0.
    s_ContextRestoreSize++;

    // incrementing sync point value to trigger the reg read thread
    s_ContextSaveSize = 2;

    // Set class to host.
    s_ContextSaveSize += 1;

    // Read QRAST_DEBUG.
    s_ContextSaveSize += 3;

    for( rid=Rm3DContext; rid->offset!=0xffff; rid++ )
    {
        NvS16 count = rid->count;
        if ( count == 0 )
            continue;

        if ( (rid+1)->count >= 0 )
        {
            // Direct registers:
            // Three WRITE_WORDs from the next loop.
            s_ContextSaveSize += 3;
            if( s_Queueless && s_IsT30 )
                s_ContextSaveSize += 3;
            // One opcode to restore direct registers.
            s_ContextRestoreSize++;
        }
        else
        {
            // Indirect registers.
            // Seven WRITE_WORDs from the next loop.
            s_ContextSaveSize += 7;
            if( s_Queueless && s_IsT30 )
                s_ContextSaveSize += 6;
            // Do non-incrementing reads from the next register.
            rid++;
            NV_ASSERT(rid->offset != 0xffff);
            count = -rid->count;

            // One opcode, one zero value and one more opcode to restore
            // indirect registers.
            s_ContextRestoreSize += 3;
        }

        NV_ASSERT(count > 0);
        RegisterCount += count;

        // this many INDDATA bogus values
        s_ContextSaveSize += count;
    }

    // Set class back to 3D.
    s_ContextSaveSize++;

    if( s_Queueless )
    {
        // Seven commands to do the host wait.
        s_ContextSaveSize += 7;
        // Six commands to do another wait.
        s_ContextSaveSize += 6;
    }

    if( s_Queueless && s_IsT30 )
        s_ContextSaveSize -= 2;

    // In addition to opcodes, there are actual register values.
    s_ContextRestoreSize += RegisterCount;

    // Sync point increment once registers are restored.
    s_ContextRestoreSize += 2;

    // Second, allocate memory and write context reading commands there.
    err = NvRmMemHandleCreate(hDevice, &s_hContextSave,
        s_ContextSaveSize*4);
    if (err != NvSuccess)
        goto fail;

    err = NvRmMemAllocTagged(s_hContextSave, NULL, 0,
        32, NvOsMemAttribute_WriteCombined,
        NVRM_MEM_TAG_RM_MISC);
    if (err != NvSuccess)
        goto fail;
    err = NvRmMemMap(s_hContextSave, 0, s_ContextSaveSize*4,
                     NVOS_MEM_WRITE, (void**)&map);
    if (err != NvSuccess)
    {
        temp = NvOsAlloc(s_ContextSaveSize*4);
        if (!temp)
        {
            err = NvError_InsufficientMemory;
            goto fail;
        }
        ptr = temp;
    }
    else
    {
        ptr = map;
    }

#define WRITE_WORD(data) *(ptr++) = (data)

    if( s_Queueless )
    {
        // set class to the unit to flush
        WRITE_WORD( NVRM_CH_OPCODE_SET_CLASS( NV_GRAPHICS_3D_CLASS_ID, 0, 0 )) ;

        if (s_IsT30)
        {
            WRITE_WORD(
                NVRM_CH_OPCODE_NONINCR( AR3D_GLOBAL_MEMORY_OUTPUT_READS_0, 1 ));
            WRITE_WORD( AR3D_GLOBAL_MEMORY_OUTPUT_READS_0_READ_DEST_MEMORY
                << AR3D_GLOBAL_MEMORY_OUTPUT_READS_0_READ_DEST_SHIFT );

            // sync point increment
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR( 0x0, 1 ));
            WRITE_WORD( 0x100 | ctx->SyncPointID );
            //WRITE_WORD( NV_DRF_NUM( NVRM, INCR_SYNCPT, INDX, ctx->SyncPointID ));
        }
        else
        {
            // sync point increment
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR( 0x0, 1 ));
            WRITE_WORD( 0x100 | ctx->SyncPointID );
            // set class to host
            WRITE_WORD( NVRM_CH_OPCODE_SET_CLASS( NV_HOST1X_CLASS_ID, 0, 0 ));
            // wait for SyncPointValue+1
            WRITE_WORD(
                NVRM_CH_OPCODE_NONINCR( NV_CLASS_HOST_WAIT_SYNCPT_BASE_0, 1 ));
            WRITE_WORD(
                NV_DRF_NUM(NV_CLASS_HOST, WAIT_SYNCPT_BASE, INDX,
                    ctx->SyncPointID) |
                NV_DRF_NUM(NV_CLASS_HOST, WAIT_SYNCPT_BASE, BASE_INDX, base) |
                NV_DRF_NUM(NV_CLASS_HOST, WAIT_SYNCPT_BASE, OFFSET, 1));
            // set class back to the unit
            WRITE_WORD(
                NVRM_CH_OPCODE_SET_CLASS( NV_GRAPHICS_3D_CLASS_ID, 0, 0 ));
        }
    }

    // XXX Unfortunately hardcoding the sync point ID prevents us from using
    // sync points other than NVRM_SYNCPOINT_3D.

    //incrementing sync point value to trigger reg read thread
    NV_ASSERT(ctx->SyncPointID); //0 is reserved
    WRITE_WORD( NVRM_CH_OPCODE_NONINCR( 0, 1 ) );
    //inc sync point with immediate mode, since the cmd buffer
    //before this one already idle the pipe
    WRITE_WORD( ctx->SyncPointID );

    WRITE_WORD( NVRM_CH_OPCODE_SET_CLASS( NV_HOST1X_CLASS_ID, 0, 0 ));

    if( !s_IsT30)
    {
        WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDOFF_0, 1) );

        // Right now csim uses QRAST_DEBUG for something else, so to test stuff
        // use SURFADDR_15 instead.  According to MichaelM it is fixed at
        // changelist 4698115.
        WRITE_WORD( INDOFF_MACRO(0, ENABLE, AR3D_QR_QRAST_DEBUG_0, READ) );
        //WRITE_WORD( INDOFF_MACRO(0, ENABLE, AR3D_GLOBAL_SURFADDR_15, READ) );

        WRITE_WORD( NVRM_CH_OPCODE_IMM(NV_CLASS_HOST_INDDATA_0, 0x66));
    }

    for ( rid=Rm3DContext; rid->offset!=0xffff; rid++ )
    {
        NvS16 count = rid->count;
        NvU32 bogus;
        if (count == 0)
            continue;

        if( (rid+1)->count >= 0 )
        {
            if( s_Queueless && s_IsT30 )
            {
                WRITE_WORD( NVRM_CH_OPCODE_SET_CLASS( NV_GRAPHICS_3D_CLASS_ID,
                        AR3D_DW_MEMORY_OUTPUT_DATA_0, 1 ));
                WRITE_WORD( NVRM_CH_OPCODE_INCR( rid->offset, count ));

                WRITE_WORD(NVRM_CH_OPCODE_SET_CLASS( NV_HOST1X_CLASS_ID, 0, 0));
            }
            // Direct registers.
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDOFF_0, 1) );
            WRITE_WORD( INDOFF_MACRO(0, ENABLE, rid->offset, READ) );
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDDATA_0, count));
            bogus = 0x77;
        }
        else
        {
            if( s_Queueless && s_IsT30 )
            {
                WRITE_WORD(
                    NVRM_CH_OPCODE_SET_CLASS( NV_GRAPHICS_3D_CLASS_ID, 0, 0 ));
                WRITE_WORD(
                    NVRM_CH_OPCODE_NONINCR( AR3D_DW_MEMORY_OUTPUT_DATA_0, 3 ));
                WRITE_WORD( NVRM_CH_OPCODE_INCR( rid->offset, rid->count ));
                WRITE_WORD( 0 );
                WRITE_WORD(
                    NVRM_CH_OPCODE_NONINCR( (rid+1)->offset, -(rid+1)->count ));
                WRITE_WORD(
                    NVRM_CH_OPCODE_SET_CLASS( NV_HOST1X_CLASS_ID, 0, 0 ));
            }
            // Indirect registers.
            // Write zero to the first register.
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDOFF_0, 1) );
            WRITE_WORD( INDOFF_MACRO(0xf, ENABLE, rid->offset, WRITE) );
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDDATA_0, 1) );
            WRITE_WORD( 0 );

            // Do non-incrementing reads from the next register.
            rid++;
            NV_ASSERT(rid->offset != 0xffff);
            count = -rid->count;

            WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDOFF_0, 1) );
            WRITE_WORD( INDOFF_MACRO(0, DISABLE, rid->offset, READ) );
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDDATA_0, count));
            bogus = 0x88;
        }

        NV_ASSERT(count > 0);

        while (count--)
            WRITE_WORD( bogus );
    }
    WRITE_WORD( NVRM_CH_OPCODE_SET_CLASS( NV_GRAPHICS_3D_CLASS_ID, 0, 0 ) );

    if( s_Queueless && s_IsT30 )
    {
        // Tracking sync point increment.
        WRITE_WORD( NVRM_CH_OPCODE_NONINCR( AR3D_DW_MEMORY_OUTPUT_DATA_0, 2 ));
        WRITE_WORD( NVRM_CH_OPCODE_NONINCR( 0, 1 ));
        WRITE_WORD( 0x100 | ctx->SyncPointID );
    }

    if( s_Queueless )
    {
#define THREE 3
        WRITE_WORD( NVRM_CH_OPCODE_SET_CLASS( NV_HOST1X_CLASS_ID, 0, 0 ));

        if (s_IsT30)
        {
            // Increment sync point value to match what would be done in the
            // interrupt handler.
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR( 0x0, 1 ) );
            WRITE_WORD( ctx->SyncPointID );
        }
        else
        {
            // wait for SyncPointValue+3
            WRITE_WORD(
                NVRM_CH_OPCODE_NONINCR( NV_CLASS_HOST_WAIT_SYNCPT_BASE_0, 1 ));
            WRITE_WORD(
                NV_DRF_NUM(NV_CLASS_HOST, WAIT_SYNCPT_BASE, INDX,
                    ctx->SyncPointID) |
                NV_DRF_NUM(NV_CLASS_HOST, WAIT_SYNCPT_BASE, BASE_INDX, base) |
                NV_DRF_NUM(NV_CLASS_HOST, WAIT_SYNCPT_BASE, OFFSET, THREE) );
        }

        // increment sync point base by three.
        WRITE_WORD(
            NVRM_CH_OPCODE_NONINCR( NV_CLASS_HOST_INCR_SYNCPT_BASE_0, 1 ));
        WRITE_WORD(
            NV_DRF_NUM(NV_CLASS_HOST, INCR_SYNCPT_BASE, BASE_INDX, base ) |
            NV_DRF_NUM(NV_CLASS_HOST, INCR_SYNCPT_BASE, OFFSET, THREE) );

        // set class back to the unit
        WRITE_WORD( NVRM_CH_OPCODE_SET_CLASS( NV_GRAPHICS_3D_CLASS_ID, 0, 0 ));
    }

#undef WRITE_WORD

    if (temp)
    {
        NvRmMemWrite(s_hContextSave, 0, temp, s_ContextSaveSize*4);
        NvOsFree(temp);
    }
    else
    {
        NvRmMemUnmap(s_hContextSave, map, s_ContextSaveSize*4);
    }

    // FIXME: need to unpin!
    NvRmMemPin( s_hContextSave );

    // Workaround for setting class.
    s_ContextPrepareSize = 1;
    err = NvRmMemHandleCreate(hDevice, &s_hContextNoChange,
        s_ContextPrepareSize*4);
    if (err != NvSuccess)
        goto fail;

    err = NvRmMemAllocTagged(s_hContextNoChange, NULL, 0,
        32, NvOsMemAttribute_WriteCombined,
        NVRM_MEM_TAG_RM_MISC);
    if (err != NvSuccess)
        goto fail;

    NvRmMemWr32( s_hContextNoChange, 0,
        NVRM_CH_OPCODE_SET_CLASS( NV_GRAPHICS_3D_CLASS_ID, 0, 0 ));

    // FIXME: need to unpin!
    NvRmMemPin( s_hContextNoChange );

initialized:
    ctx->Queueless = s_Queueless;
    ctx->QueuelessSyncpointIncrement = THREE;

    err = NvRmMemHandleCreate(hDevice, &h, s_ContextRestoreSize*4);
    if( err != NvSuccess )
        goto fail;

    err = NvRmMemAllocTagged(h, NULL, 0,
        32, NvOsMemAttribute_WriteCombined,
        NVRM_MEM_TAG_RM_MISC);
    if( err != NvSuccess )
        goto fail;

    {
        NvU32 commands[5];
        NvU32 *cptr = commands;

        // set class to host
        *cptr++ = NVRM_CH_OPCODE_SET_CLASS( NV_HOST1X_CLASS_ID, 0, 0 );
        // increment sync point base
        // TODO Unhardcode sync point base number.
        *cptr++ = NVRM_CH_OPCODE_NONINCR( NV_CLASS_HOST_INCR_SYNCPT_BASE_0, 1 );
        *cptr++ = NV_DRF_NUM(NV_CLASS_HOST, INCR_SYNCPT_BASE, BASE_INDX, base )
                | NV_DRF_NUM(NV_CLASS_HOST, INCR_SYNCPT_BASE, OFFSET, 1);

        // set class to 3D
        *cptr++ = NVRM_CH_OPCODE_SET_CLASS( NV_GRAPHICS_3D_CLASS_ID, 0, 0 );
        // program PSEQ_QUAD_ID
        *cptr++ = NVRM_CH_OPCODE_IMM( AR3D_PSEQ_QUAD_ID_0, 0 );
        NV_ASSERT( cptr-commands == NV_ARRAY_SIZE(commands) );

        NvRmMemWrite( h, 0, commands, sizeof(commands) );
    }

    // FIXME: need to unpin!
    NvRmMemPin( h );

    ctx->hContextNoChange = s_hContextNoChange;
    ctx->ContextNoChangeSize = s_ContextPrepareSize;
    ctx->hContextSave = s_hContextSave;
    ctx->ContextSaveSize = s_ContextSaveSize;
    ctx->hContextRestore = h;
    // Set class to host (0), increment sync point base (1,2), set class to 3D
    // (3) and program PSEQ_QUAD_ID (4).
    ctx->ContextRestoreOffset = 5;
    ctx->ContextRestoreSize = s_ContextRestoreSize;

    if( s_Queueless )
    {
        NvU32 commands[3];
        NvU32 *cptr = commands;

        *cptr++ = NVRM_CH_OPCODE_SET_CLASS( NV_GRAPHICS_3D_CLASS_ID, 0, 0 );

        if( s_IsT30 )
        {
            *cptr++ = NVRM_CH_OPCODE_NONINCR( AR3D_DW_MEMORY_OUTPUT_ADDRESS_0, 1);
            *cptr++ = NvRmMemGetAddress(ctx->hContextRestore, 0) + ctx->ContextRestoreOffset*4;
        }
        else
        {
            *cptr++ = NVRM_CH_OPCODE_NONINCR( AR3D_QR_QRAST_DEBUG_0, 1 );
            //*cptr++ = NVRM_CH_OPCODE_NONINCR( AR3D_GLOBAL_SURFADDR_15, 1 );
            *cptr++ = (NvU32)ctx->hContextRestore;
        }

        NV_ASSERT( cptr-commands == NV_ARRAY_SIZE(commands) );
        ctx->ContextSetAddressSize = NV_ARRAY_SIZE(commands);

        err = NvRmMemHandleCreate( hDevice, &ctx->hContextSetAddress,
            NV_ARRAY_SIZE(commands)*4 );
        if( err != NvSuccess )
            goto fail;

        err = NvRmMemAllocTagged( ctx->hContextSetAddress, NULL, 0, 32,
            NvOsMemAttribute_WriteCombined,
            NVRM_MEM_TAG_RM_MISC);
        if( err != NvSuccess )
            goto fail;

        NvRmMemWrite( ctx->hContextSetAddress, 0, commands, sizeof(commands) );
        NvRmMemPin( ctx->hContextSetAddress );
    }

    s_3DContextCount++;

    NvOsMutexUnlock( g_hwcontext_mutex );
    return err;

fail:
    NvRmMemHandleFree( h );

    if( s_3DContextCount == 0 )
    {
        NvOsDebugPrintf("Freeing stuff!\n");
        NvRmMemHandleFree( s_hContextNoChange );
        NvRmMemHandleFree( s_hContextSave );
        s_hContextNoChange = s_hContextSave = 0;
    }

    NvOsMutexUnlock( g_hwcontext_mutex );
    NV_ASSERT(!"NvRmContext3DInit");
    return err;
}

NvError NvRmContext3DAlloc( NvRmDeviceHandle hDevice,
    NvRmContextHandle *phContext )
{
    NvError ret;
    NvRmContext *ctx;

    ctx = NvOsAlloc( sizeof(*ctx) );
    if( !ctx )
    {
        ret = NvError_InsufficientMemory;
        goto fail;
    }

    NvOsMemset( ctx, 0, sizeof(*ctx) );
    ctx->Engine = NvRmModuleID_3D;
    ctx->ClassID = NV_GRAPHICS_3D_CLASS_ID;
    ctx->ContextRegisters = Rm3DContext;
    ctx->IsValid = NV_FALSE;
    ctx->SyncPointID = NVRM_SYNCPOINT_3D;

    ret = NvRmContext3DInit( hDevice, ctx );
    if( ret != NvSuccess )
    {
        goto fail;
    }

    *phContext = ctx;

    NV_ASSERT(ctx->QueuelessSyncpointIncrement == 3);
    return NvSuccess;

fail:
    NvOsFree( ctx );
    return ret;
}

void NvRmContext3DFree( NvRmContextHandle hContext )
{
    NV_ASSERT( s_3DContextCount > 0 );
#if NV_DEBUG
    NvRmMemWr32( hContext->hContextRestore, 0,
        NVRM_CH_OPCODE_SET_CLASS( 0x333, 0, 0 ));
#endif

    NvRmMemUnpin( hContext->hContextRestore );
    NvRmMemHandleFree( hContext->hContextRestore );
    hContext->hContextRestore = (NvRmMemHandle)0xcccccccc;

    NvRmMemUnpin( hContext->hContextSetAddress );
    NvRmMemHandleFree( hContext->hContextSetAddress );
    hContext->hContextSetAddress = (NvRmMemHandle)0xcccccccc;

    NvOsFree( hContext );

    s_3DContextCount--;

    if( s_3DContextCount == 0)
    {
        NvRmMemHandleFree( s_hContextNoChange );
        NvRmMemHandleFree( s_hContextSave );
        s_hContextNoChange = s_hContextSave = 0;
    }
}
