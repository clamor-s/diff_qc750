/*
 * Copyright (c) 2007-2012 NVIDIA Corporation.  All Rights Reserved.
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

#include "ap20/arhost1x_uclass.h"
#include "ap20/class_ids.h"
#include "ap20/ap20rm_channel.h"
#include "ap20/armpe.h"

#define MAKE_REG_INFO(COUNT, REG_NAME) { COUNT, REG_NAME, #REG_NAME }
const NvRmMpeContext RmMpeContext[] = {
    MAKE_REG_INFO(    2, MPEA_BUFFER_FULL_READ_0),
    MAKE_REG_INFO(    2, MPEA_LENGTH_OF_STREAM_0),
    MAKE_REG_INFO(    2, MPEA_BUFFER_DEPLETION_RATE_0),
    MAKE_REG_INFO(    2, MPEA_ENC_FRAME_NUM_0),
    MAKE_REG_INFO(    1, MPEA_WIDTH_HEIGHT_0),
    MAKE_REG_INFO(    2, MPEA_VOL_CTRL_0),
    MAKE_REG_INFO(    1, MPEA_CROP_LR_OFFSET_0),
    MAKE_REG_INFO(    1, MPEA_CROP_TB_OFFSET_0),
    MAKE_REG_INFO(    1, MPEA_TIMESTAMP_RES_0),
    MAKE_REG_INFO(    1, MPEA_MCCIF_MPECSRD_HP_0),
    MAKE_REG_INFO(    1, MPEA_TIMEOUT_WCOAL_MPEB_0),
    MAKE_REG_INFO(    1, MPEA_TIMEOUT_WCOAL_MPEC_0),
    MAKE_REG_INFO(    1, MPEA_SEQ_PIC_CTRL_0),
    MAKE_REG_INFO(    1, MPEA_SEQ_PARAMETERS_0),
    MAKE_REG_INFO(    1, MPEA_PIC_PARAMETERS_0),
    MAKE_REG_INFO(    1, MPEA_PIC_PARAM_SET_ID_0),
    MAKE_REG_INFO(    1, MPEA_SEQ_PARAM_SET_ID_0),
    MAKE_REG_INFO(    1, MPEA_NUM_REF_IDX_ACTIVE_0),
    MAKE_REG_INFO(    1, MPEA_LOG2_MAX_FRAME_NUM_MINUS4_0),
    MAKE_REG_INFO(    1, MPEA_FRAME_CTRL_0),
    MAKE_REG_INFO(    1, MPEA_MOT_SEARCH_CTRL_0),
    MAKE_REG_INFO(    1, MPEA_MOT_SEARCH_RANGE_0),
    MAKE_REG_INFO(    1, MPEA_MOTSEARCH_EXIT_0),
    MAKE_REG_INFO(    1, MPEA_MOTSEARCH_BIAS1_0),
    MAKE_REG_INFO(    1, MPEA_MOTSEARCH_BIAS2_0),
    MAKE_REG_INFO(    1, MPEA_MOTSEARCH_BIAS3_0),
    MAKE_REG_INFO(    1, MPEA_MOTSEARCH_BIAS4_0),
    MAKE_REG_INFO(    1, MPEA_MOTSEARCH_BIAS5_0),
    MAKE_REG_INFO(    1, MPEA_MOTSEARCH_BIAS6_0),
    MAKE_REG_INFO(    1, MPEA_FRAME_TYPE_0),
    MAKE_REG_INFO(    1, MPEA_I_RATE_CTRL_0),
    MAKE_REG_INFO(    1, MPEA_P_RATE_CTRL_0),
    MAKE_REG_INFO(    1, MPEA_RC_ADJUSTMENT_0),
    MAKE_REG_INFO(    1, MPEA_BUFFER_DEPLETION_RATE_0),
    MAKE_REG_INFO(    1, MPEA_BUFFER_SIZE_0),
    MAKE_REG_INFO(    1, MPEA_INITIAL_DELAY_OFFSET_0),
    MAKE_REG_INFO(    1, MPEA_BUFFER_FULL_0),
    MAKE_REG_INFO(    1, MPEA_MIN_IFRAME_SIZE_0),
    MAKE_REG_INFO(    1, MPEA_MIN_PFRAME_SIZE_0),
    MAKE_REG_INFO(    1, MPEA_SUGGESTED_IFRAME_SIZE_0),
    MAKE_REG_INFO(    1, MPEA_SUGGESTED_PFRAME_SIZE_0),
    MAKE_REG_INFO(    1, MPEA_TARGET_BUFFER_I_SIZE_0),
    MAKE_REG_INFO(    1, MPEA_TARGET_BUFFER_P_SIZE_0),
    MAKE_REG_INFO(    1, MPEA_SKIP_THRESHOLD_0),
    MAKE_REG_INFO(    1, MPEA_UNDERFLOW_THRESHOLD_0),
    MAKE_REG_INFO(    1, MPEA_OVERFLOW_THRESHOLD_0),
    MAKE_REG_INFO(    1, MPEA_RC_QP_DQUANT_0),
    MAKE_REG_INFO(    1, MPEA_RC_BU_SIZE_0),
    MAKE_REG_INFO(    2, MPEA_GOP_PARAM_0),
    MAKE_REG_INFO(    1, MPEA_FORCE_I_FRAME_0),
    MAKE_REG_INFO(    1, MPEA_FRAME_PATTERN_0),
    MAKE_REG_INFO(    1, MPEA_INTERNAL_BIAS_MULTIPLIER_0),
    MAKE_REG_INFO(    1, MPEA_MISC_MODE_BIAS_0),
    MAKE_REG_INFO(    1, MPEA_INTRA_PRED_BIAS_0),
    MAKE_REG_INFO(    1, MPEA_INTRA_REF_CTRL_0),
    MAKE_REG_INFO(    1, MPEA_INTRA_REF_MIN_COUNTER_0),
    MAKE_REG_INFO(    1, MPEA_INTRA_REF_DELTA_COUNTER_0),
    MAKE_REG_INFO(    1, MPEA_QUANTIZATION_CONTROL_0),
    MAKE_REG_INFO(    1, MPEA_IPCM_CTRL_0),
    MAKE_REG_INFO(    1, MPEA_PACKET_HEC_0),
    MAKE_REG_INFO(    1, MPEA_PACKET_CTRL_H264_0),
    MAKE_REG_INFO(    1, MPEA_NUM_SLICE_GROUPS_0),
    MAKE_REG_INFO(    1, MPEA_DMA_BUFFER_ADDR_0),
    MAKE_REG_INFO(    1, MPEA_PIC_INIT_Q_0),
    MAKE_REG_INFO(    1, MPEA_MAX_MIN_QP_I_0),
    MAKE_REG_INFO(    1, MPEA_MAX_MIN_QP_P_0),
    MAKE_REG_INFO(    1, MPEA_SLICE_PARAMS_0),
    MAKE_REG_INFO(    1, MPEA_NUM_OF_UNITS_0),
    MAKE_REG_INFO(    1, MPEA_TOP_LEFT_0),
    MAKE_REG_INFO(    1, MPEA_BOTTOM_RIGHT_0),
    MAKE_REG_INFO(    1, MPEA_CHANGE_RATE_0),
    MAKE_REG_INFO(    1, MPEA_DMA_BUFFER_ADDR_0),
    MAKE_REG_INFO(    1, MPEA_PIC_INIT_Q_0),
    MAKE_REG_INFO(    1, MPEA_MAX_MIN_QP_I_0),
    MAKE_REG_INFO(    1, MPEA_MAX_MIN_QP_P_0),
    MAKE_REG_INFO(    1, MPEA_SLICE_PARAMS_0),
    MAKE_REG_INFO(    1, MPEA_NUM_OF_UNITS_0),
    MAKE_REG_INFO(    1, MPEA_TOP_LEFT_0),
    MAKE_REG_INFO(    1, MPEA_BOTTOM_RIGHT_0),
    MAKE_REG_INFO(    1, MPEA_CHANGE_RATE_0),
    MAKE_REG_INFO(    1, MPEA_DMA_BUFFER_ADDR_0),
    MAKE_REG_INFO(    1, MPEA_PIC_INIT_Q_0),
    MAKE_REG_INFO(    1, MPEA_MAX_MIN_QP_I_0),
    MAKE_REG_INFO(    1, MPEA_MAX_MIN_QP_P_0),
    MAKE_REG_INFO(    1, MPEA_SLICE_PARAMS_0),
    MAKE_REG_INFO(    1, MPEA_NUM_OF_UNITS_0),
    MAKE_REG_INFO(    1, MPEA_TOP_LEFT_0),
    MAKE_REG_INFO(    1, MPEA_BOTTOM_RIGHT_0),
    MAKE_REG_INFO(    1, MPEA_CHANGE_RATE_0),
    MAKE_REG_INFO(    1, MPEA_QPP_CTRL_0),
    MAKE_REG_INFO(    1, MPEA_IB_BUFFER_ADDR_MODE_0),
    MAKE_REG_INFO(    1, MPEA_IB_OFFSET_LUMA_0),
    MAKE_REG_INFO(    1, MPEA_IB_OFFSET_CHROMA_0),
    MAKE_REG_INFO(    1, MPEA_FIRST_IB_OFFSET_LUMA_0),
    MAKE_REG_INFO(    1, MPEA_FIRST_IB_OFFSET_CHROMA_0),
    MAKE_REG_INFO(    1, MPEA_FIRST_IB_V_SIZE_0),
    MAKE_REG_INFO(    1, MPEA_IB0_START_ADDR_Y_0),
    MAKE_REG_INFO(    1, MPEA_IB0_START_ADDR_U_0),
    MAKE_REG_INFO(    1, MPEA_IB0_START_ADDR_V_0),
    MAKE_REG_INFO(    1, MPEA_IB0_SIZE_0),
    MAKE_REG_INFO(    1, MPEA_IB0_LINE_STRIDE_0),
    MAKE_REG_INFO(    1, MPEA_IB0_BUFFER_STRIDE_LUMA_0),
    MAKE_REG_INFO(    1, MPEA_IB0_BUFFER_STRIDE_CHROMA_0),
    MAKE_REG_INFO(    1, MPEA_REF_BUFFER_ADDR_MODE_0),
    MAKE_REG_INFO(    1, MPEA_REF_Y_START_ADDR_0),
    MAKE_REG_INFO(    1, MPEA_REF_U_START_ADDR_0),
    MAKE_REG_INFO(    1, MPEA_REF_V_START_ADDR_0),
    MAKE_REG_INFO(    1, MPEA_REF_STRIDE_0),
    MAKE_REG_INFO(    1, MPEA_REF_BUFFER_LEN_0),
    MAKE_REG_INFO(    1, MPEA_REF_RD_MBROW_0),
    MAKE_REG_INFO(    1, MPEA_REF_WR_MBROW_0),
    MAKE_REG_INFO(    1, MPEA_AVE_BIT_LEN_0),
    MAKE_REG_INFO(    1, MPEA_MAX_PACKET_0),
    MAKE_REG_INFO(    1, MPEA_FRAME_CYCLE_COUNT_0),
    MAKE_REG_INFO(    1, MPEA_MB_CYCLE_COUNT_0),
    MAKE_REG_INFO(    1, MPEA_CLK_OVERRIDE_A_0),
    MAKE_REG_INFO(    1, MPEA_IPRED_ROW_ADDR_0),
    MAKE_REG_INFO(    1, MPEA_DBLK_PARAM_ADDR_0),
    MAKE_REG_INFO(    1, MPEA_DMA_BUFFER_STATUS_0),
    MAKE_REG_INFO(    1, MPEA_NUM_INTRAMB_0),
    MAKE_REG_INFO(    1, MPEA_DMA_BUFFER_SIZE_0),
    MAKE_REG_INFO(    1, MPEA_DMA_LIST_SIZE_0),
    MAKE_REG_INFO(    1, MPEA_FRAME_NUM_0),
    MAKE_REG_INFO(    1, MPEA_LENGTH_OF_MOTION_MODE_0),
    MAKE_REG_INFO(    1, MPEA_REF_BUFFER_IDX_0),
    MAKE_REG_INFO(    1, MPEA_REF_RD_MBROW_0),
    MAKE_REG_INFO(    1, MPEA_REF_WR_MBROW_0),
    MAKE_REG_INFO(    3, MPEA_FRAME_INDEX_0),
    MAKE_REG_INFO(    2, MPEA_FRAME_NUM_GOP_0),
    MAKE_REG_INFO(    1, MPEA_LOWER_BOUND_0),
    MAKE_REG_INFO(    1, MPEA_UPPER_BOUND_0),
    MAKE_REG_INFO(    1, MPEA_REMAINING_BITS_0),
    MAKE_REG_INFO(    1, MPEA_NUM_CODED_BU_0),
    MAKE_REG_INFO(    1, MPEA_PREVIOUS_QP_0),
    MAKE_REG_INFO(    1, MPEA_NUM_P_PICTURE_0),
    MAKE_REG_INFO(    1, MPEA_QP_SUM_0),
    MAKE_REG_INFO(    1, MPEA_TOTAL_ENERGY_0),
    MAKE_REG_INFO(    1, MPEA_A1_VALUE_0),
    MAKE_REG_INFO(    1, MPEA_CODED_FRAMES_0),
    MAKE_REG_INFO(    1, MPEA_P_AVE_HEADER_BITS_A_0),
    MAKE_REG_INFO(    1, MPEA_P_AVE_HEADER_BITS_B_0),
    MAKE_REG_INFO(    1, MPEA_PREV_FRAME_MAD_0),
    MAKE_REG_INFO(    1, MPEA_TOTAL_QP_FOR_P_PICTURE_0),
    MAKE_REG_INFO(    2, MPEA_CONTEXT_SAVE_MISC_0),
    MAKE_REG_INFO(    1, MPEA_TARGET_BUFFER_LEVEL_0),
    MAKE_REG_INFO(    1, MPEA_DELTA_P_0),
    MAKE_REG_INFO(    1, MPEA_IDR_PIC_ID_0),
    MAKE_REG_INFO(    1, MPEA_MAD_PIC_1_CBR_0),
    MAKE_REG_INFO(    1, MPEA_MX1_CBR_0),
    MAKE_REG_INFO(    1, MPEA_LENGTH_OF_STREAM_CBR_0),
    MAKE_REG_INFO(    -1, MPEA_BUFFER_FULL_0),
    MAKE_REG_INFO(    -1, MPEA_REPORTED_FRAME_0),
    MAKE_REG_INFO(    0, 0xffff) // end-of-array marker
};


// Number of registers to get from the read FIFO.
// This is used when context switch is not required.  Right now it's only one
// set class command.  This is needed because it is possible that another
// process was using 2D unit and left class set to 2D.  When we're back to the
// first process, we need to guarantee that class is back to MPE.
static NvRmMemHandle s_hContextPrepare = 0;
// The same context save command sequence is used for all contexts.
static NvRmMemHandle s_hContextSave = 0;
// Sizes are in words.  They are calculated once in
// NvRmContextMpeInit and never change.
static NvU32 s_ContextPrepareSize;
static NvU32 s_ContextSaveSize;
static NvU32 s_ContextRestoreSize;
static NvU32 s_ContextSaveRcRamSize;
static NvU32 s_ContextSaveIrfrRamSize;

// How many MPE contexts there are.  When it is zero, static blocks are freed
// as well.
static NvU32 s_MpeContextCount = 0;

#define INDOFF_MACRO(indbe, autoinc, offset, rwn)               \
    (NV_DRF_NUM(NV_CLASS_HOST, INDOFF, INDBE, (indbe)) |        \
     NV_DRF_DEF(NV_CLASS_HOST, INDOFF, AUTOINC, autoinc) |      \
     NV_DRF_DEF(NV_CLASS_HOST, INDOFF, INDMODID, MPE) |        \
     NV_DRF_NUM(NV_CLASS_HOST, INDOFF, INDROFFSET, (offset)) |  \
     NV_DRF_DEF(NV_CLASS_HOST, INDOFF, ACCTYPE, REG) |          \
     NV_DRF_DEF(NV_CLASS_HOST, INDOFF, RWN, rwn))


#define RAM_SIZE 692
#define IRFR_RAM_SIZE 408

extern NvOsMutexHandle g_hwcontext_mutex;

static NvError NvRmContextMpeInit( NvRmDeviceHandle hDevice, NvRmContext *ctx )
{
    NvError err;
    const NvRmMpeContext *rid;
    NvU32 Offset = 0;
    NvU32 count = 0;
    NvRmMemHandle h = 0; // Will go to RestoreHandle.
    NvRmMemHandle h1 = 0; // S/W work-around for h/w bug 526915
    NvU32 RegisterCount = 0;

    NvOsMutexLock( g_hwcontext_mutex );

    // Don't need to do it more than once.
    if ( s_hContextSave )
        goto initialized;

    /****First, figure out how much memory we will need.****/

    //6 for writing MPEA_RC_RAM_READ_CMD_0 register
    s_ContextSaveRcRamSize = 6;
    //4 for junk into MPEA_RC_RAM_READ_DATA_0 and 4 for reading actual data from MPEA_RC_RAM_READ_CMD_0.
    //692 is size of RC-RAM
    s_ContextSaveRcRamSize += RAM_SIZE*(4+4);

    //6 for writing MPEA_INTRA_REF_READ_CMD_0 register
    s_ContextSaveIrfrRamSize = 6;
    //4 for junk into MPEA_RC_RAM_READ_DATA_0 and 4 for reading actual data from MPEA_RC_RAM_READ_CMD_0.
    //408 is size of IRFR-RAM
    s_ContextSaveIrfrRamSize += IRFR_RAM_SIZE*(4+4);

    // Start with setting class to host and incrementing sync point base.
    s_ContextRestoreSize = 3;

    // Set class back to MPE.
    s_ContextRestoreSize++;

    // incrementing sync point value to trigger the reg read thread
    //s_ContextSaveSize = 2;

    // Set class to host.
    s_ContextSaveSize = 1;
    for( rid=RmMpeContext; rid->offset!=0xffff; rid++ )
    {
        NvS16 count = rid->count;
        if ( count == 0 )
            continue;
        if( rid->count == 1 || rid->count == 2  || rid->count == 3) // save/restore
        {
            count = 1;
            // Direct registers:
            // Three WRITE_WORDs from the next loop.
            s_ContextSaveSize += 3;
            // One opcode to restore direct registers.
            s_ContextRestoreSize++;
            // this many INDDATA bogus values
            s_ContextSaveSize += count;
        }
        else if(rid->count == 2) //save only
        {
            // Direct registers:
            // Three WRITE_WORDs from the next loop.
            s_ContextSaveSize += 3;
            // this many INDDATA bogus values
            s_ContextSaveSize += 1;
            count = 0;
        }
        else if(rid->count == -1) //restore only
        {
            // One opcode to restore direct registers.
            s_ContextRestoreSize++;
            count = 1;
        }
        //NV_ASSERT(count > 0);
        RegisterCount += count;
    }

    // Set class back to MPE.
    s_ContextSaveSize++;

    // In addition to opcodes, there are actual register values.
    s_ContextRestoreSize += RegisterCount;
    //and RC-RAM contents
    s_ContextRestoreSize += (RAM_SIZE*2)+2;
    //and IRFR-RAM contents
    s_ContextRestoreSize += (IRFR_RAM_SIZE*2)+2;

    // Sync point increment once registers are restored.
    s_ContextRestoreSize += 2;

    /**** Second, allocate memory and write context reading commands there. ****/

    err = NvRmMemHandleCreate(hDevice, &s_hContextSave,
        (s_ContextSaveSize+s_ContextSaveRcRamSize+s_ContextSaveIrfrRamSize)*4);
    if (err != NvSuccess)
        goto fail;

    err = NvRmMemAllocTagged(s_hContextSave, NULL, 0,
        32, NvOsMemAttribute_WriteCombined,
        NVRM_MEM_TAG_RM_MISC);
    if (err != NvSuccess)
        goto fail;

    // FIXME: need to unpin!
    NvRmMemPin( s_hContextSave );

#define WRITE_WORD(data) \
    do { \
        NvRmMemWr32( s_hContextSave, Offset, data ); \
        Offset += 4; \
    } while (0)

    //incrementing sync point value to trigger reg read thread
    /*NV_ASSERT(ctx->SyncPointID); //0 is reserved
    WRITE_WORD( NVRM_CH_OPCODE_NONINCR( 0x0, 1 ) );
    WRITE_WORD( (NvU8)(ctx->SyncPointID & 0xff) );*/

    WRITE_WORD( NVRM_CH_OPCODE_SET_CLASS( NV_HOST1X_CLASS_ID, 0, 0 ) );
    for ( rid=RmMpeContext; rid->offset!=0xffff; rid++ )
    {
        NvS16 count = rid->count;
        NvU32 bogus;
        if (count == 0)
            continue;

        if(rid->count >= 0)
        {
            count = 1;
            // Direct registers.
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDOFF_0, 1) );
            WRITE_WORD( INDOFF_MACRO(0, ENABLE, rid->offset, READ) );
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDDATA_0, count));
            bogus = 0x77;
            while (count--)
                WRITE_WORD( bogus );
        }
        //NV_ASSERT(count > 0);
    }
    WRITE_WORD( NVRM_CH_OPCODE_SET_CLASS( NV_VIDEO_ENCODE_MPEG_CLASS_ID, 0, 0 ) );
    NV_ASSERT( s_ContextSaveSize*4 == Offset );

    //Special: for RC-RAM contents.
    {
        //Write to MPEA_RC_RAM_READ_CMD_0 reg only once, else MPE will reset the rd counter everytime.
        NvU32 addr = MPEA_RC_RAM_READ_CMD_0;
        NvU32 bogus = 0x99;
        count = RAM_SIZE; //these many words to read from RC-RAM.
        WRITE_WORD( NVRM_CH_OPCODE_SET_CLASS( NV_HOST1X_CLASS_ID, 0, 0 ) );
        WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDOFF_0, 1) );
        WRITE_WORD( INDOFF_MACRO(0xf, DISABLE, addr, WRITE) );
        WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDDATA_0, 1) );
        WRITE_WORD( count );

        //Program MPEA_RC_RAM_READ_DATA_0 to read RC data from RAM.
        //Writing junk data to avoid cached problem with register memory.
        //Then read actual value.
        addr = MPEA_RC_RAM_READ_DATA_0;
        while(count--)
        {
            //4 more, for actual reads.
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDOFF_0, 1) );
            WRITE_WORD( INDOFF_MACRO(0, DISABLE, addr, READ) );
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDDATA_0, 1));
            WRITE_WORD( bogus );
            //FIXME:
            //4 for WAR: Write this junk word before reading to avoid caching/uncaching of register memory.
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDOFF_0, 1) );
            WRITE_WORD( INDOFF_MACRO(0xf, DISABLE, addr, WRITE) );
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDDATA_0, 1));
            WRITE_WORD( bogus );
        }
        WRITE_WORD( NVRM_CH_OPCODE_SET_CLASS( NV_VIDEO_ENCODE_MPEG_CLASS_ID, 0, 0 ) );
        count=0;
    } //end of RC-RAM reading.

    NV_ASSERT( (s_ContextSaveSize+s_ContextSaveRcRamSize)*4 == Offset );

    //Special: for IRFR-RAM contents.
    {
        //Write to MPEA_INTRA_REF_READ_CMD_0 reg only once, else MPE will reset the rd counter everytime.
        NvU32 addr = MPEA_INTRA_REF_READ_CMD_0;
        NvU32 bogus = 0x99;
        count = IRFR_RAM_SIZE; //these many words to read from IRFR_RAM_SIZE.
        WRITE_WORD( NVRM_CH_OPCODE_SET_CLASS( NV_HOST1X_CLASS_ID, 0, 0 ) );
        WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDOFF_0, 1) );
        WRITE_WORD( INDOFF_MACRO(0xf, DISABLE, addr, WRITE) );
        WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDDATA_0, 1) );
        WRITE_WORD( count );

        //Program MPEA_INTRA_REF_READ_DATA_0 to read RC data from RAM.
        //Writing junk data to avoid cached problem with register memory.
        //Then read actual value.
        addr = MPEA_INTRA_REF_READ_DATA_0;
        while(count--)
        {
            //4 more, for actual reads.
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDOFF_0, 1) );
            WRITE_WORD( INDOFF_MACRO(0, DISABLE, addr, READ) );
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDDATA_0, 1));
            WRITE_WORD( bogus );
            //FIXME:
            //4 for WAR: Write this junk word before reading to avoid caching/uncaching of register memory.
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDOFF_0, 1) );
            WRITE_WORD( INDOFF_MACRO(0xf, DISABLE, addr, WRITE) );
            WRITE_WORD( NVRM_CH_OPCODE_NONINCR(NV_CLASS_HOST_INDDATA_0, 1));
            WRITE_WORD( bogus );
        }
        WRITE_WORD( NVRM_CH_OPCODE_SET_CLASS( NV_VIDEO_ENCODE_MPEG_CLASS_ID, 0, 0 ) );
        count=0;
    } //end of IRFR-RAM reading.

#undef WRITE_WORD

    NV_ASSERT( (s_ContextSaveIrfrRamSize+s_ContextSaveRcRamSize+s_ContextSaveSize)*4 == Offset );

    // Workaround for setting class when previous context and present context are same.
    s_ContextPrepareSize = 1;
    err = NvRmMemHandleCreate(hDevice, &s_hContextPrepare,
        s_ContextPrepareSize*4);
    if (err != NvSuccess)
        goto fail;

    err = NvRmMemAllocTagged(s_hContextPrepare, NULL, 0,
        32, NvOsMemAttribute_WriteCombined,
        NVRM_MEM_TAG_RM_MISC);
    if (err != NvSuccess)
        goto fail;

    // FIXME: need to unpin!
    NvRmMemPin( s_hContextPrepare );

    NvRmMemWr32( s_hContextPrepare, 0,
        NVRM_CH_OPCODE_SET_CLASS( NV_VIDEO_ENCODE_MPEG_CLASS_ID, 0, 0 ));

initialized:
    //h1 is for s/w war of h/w bug 526915.
    err = NvRmMemHandleCreate(hDevice, &h1, 7*4);
    if( err != NvSuccess )
        goto fail;

    err = NvRmMemAllocTagged(h1, NULL, 0,
        32, NvOsMemAttribute_WriteCombined,
        NVRM_MEM_TAG_RM_MISC);
    if( err != NvSuccess )
        goto fail;

    // FIXME: need to unpin!
    NvRmMemPin( h1 );
    {
        NvU32 data1 = 0;
        NvRmMemWr32( h1, 0*4, data1 );
        NvRmMemWr32( h1, 1*4, data1 );
        NvRmMemWr32( h1, 2*4, data1 );
        NvRmMemWr32( h1, 3*4, data1 );
        NvRmMemWr32( h1, 4*4, data1 );
        NvRmMemWr32( h1, 5*4, data1 );
        NvRmMemWr32( h1, 6*4, data1 );
    }
    //end for h1

    //h will go to restore handle in context thread.
    err = NvRmMemHandleCreate(hDevice, &h, s_ContextRestoreSize*4);
    if( err != NvSuccess )
        goto fail;

    err = NvRmMemAllocTagged(h, NULL, 0,
        32, NvOsMemAttribute_WriteCombined,
        NVRM_MEM_TAG_RM_MISC);
    if( err != NvSuccess )
        goto fail;

    // FIXME: need to unpin!
    NvRmMemPin( h );

    {
        NvU32 commands[4];
        NvU32 *cptr = commands;
        NvU32 base;

        // FIXME: get a channel handle
        base = NvRmChannelGetModuleWaitBase( 0, NvRmModuleID_Mpe, 0 );

        // set class to host
        *cptr++ = NVRM_CH_OPCODE_SET_CLASS( NV_HOST1X_CLASS_ID, 0, 0 );
        // increment sync point base
        // TODO Unhardcode sync point base number.
        *cptr++ = NVRM_CH_OPCODE_NONINCR( NV_CLASS_HOST_INCR_SYNCPT_BASE_0, 1 );
        *cptr++ = NV_DRF_NUM(NV_CLASS_HOST, INCR_SYNCPT_BASE, BASE_INDX, base )
                | NV_DRF_NUM(NV_CLASS_HOST, INCR_SYNCPT_BASE, OFFSET, 1);

        // set class to MPE
        *cptr++ = NVRM_CH_OPCODE_SET_CLASS( NV_VIDEO_ENCODE_MPEG_CLASS_ID, 0, 0 );
        NV_ASSERT( cptr-commands == NV_ARRAY_SIZE(commands) );

        NvRmMemWrite( h, 0, commands, sizeof(commands) );
    }

    ctx->hContextNoChange = s_hContextPrepare;
    ctx->ContextNoChangeSize = s_ContextPrepareSize;
    ctx->hContextSave = s_hContextSave;
    ctx->ContextSaveSize = s_ContextSaveSize+s_ContextSaveRcRamSize+s_ContextSaveIrfrRamSize;
    ctx->hContextRestore = h;
    ctx->hContextMpeHwBugFix = h1;

    // Set class to host (0), increment sync point base (1,2), and set class to MPE (3).
    ctx->ContextRestoreOffset = 4;
    ctx->ContextRestoreSize = s_ContextRestoreSize;

    s_MpeContextCount++;

    NvOsMutexUnlock( g_hwcontext_mutex );
    return err;

fail:
    NvRmMemHandleFree( h );
    NvRmMemHandleFree( h1 );

    if( s_MpeContextCount == 0 )
    {
        NvRmMemHandleFree( s_hContextPrepare );
        NvRmMemHandleFree( s_hContextSave );
        s_hContextPrepare = s_hContextSave = 0;
    }

    NvOsMutexUnlock( g_hwcontext_mutex );
    NV_ASSERT(!"NvRmContextMpeInit");
    return err;
}

NvError NvRmContextMpeAlloc( NvRmDeviceHandle hDevice,
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

    ctx->Engine = NvRmModuleID_Mpe;
    ctx->ClassID = NV_VIDEO_ENCODE_MPEG_CLASS_ID;

    ctx->ContextRegistersMpe = RmMpeContext;
    ctx->IsValid = NV_FALSE;
    ctx->SyncPointID = NVRM_SYNCPOINT_MPE;

    ret = NvRmContextMpeInit( hDevice, ctx );
    if( ret != NvSuccess )
    {
        goto fail;
    }

    *phContext = ctx;

    return NvSuccess;

fail:
    NvOsFree( ctx );
    return ret;
}

void NvRmContextMpeFree( NvRmContextHandle hContext )
{
    NV_ASSERT( s_MpeContextCount > 0 );
#if NV_DEBUG
    NvRmMemWr32( hContext->hContextRestore, 0,
        NVRM_CH_OPCODE_SET_CLASS( 0x333, 0, 0 ));
#endif

    NvRmMemUnpin( hContext->hContextRestore );
    NvRmMemHandleFree( hContext->hContextRestore );
    hContext->hContextRestore = (NvRmMemHandle)0xcccccccc;

    NvRmMemUnpin( hContext->hContextMpeHwBugFix );
    NvRmMemHandleFree( hContext->hContextMpeHwBugFix );
    hContext->hContextMpeHwBugFix = (NvRmMemHandle)0xcccccccc;

    NvOsFree( hContext );

    s_MpeContextCount--;

    if( s_MpeContextCount == 0)
    {
        NvRmMemUnpin( s_hContextPrepare );
        NvRmMemHandleFree( s_hContextPrepare );
        NvRmMemUnpin( s_hContextSave );
        NvRmMemHandleFree( s_hContextSave );
        s_hContextPrepare = s_hContextSave = 0;
    }
}

