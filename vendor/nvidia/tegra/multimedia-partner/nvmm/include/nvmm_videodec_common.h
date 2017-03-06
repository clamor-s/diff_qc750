/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */


/* This NvMM block is provided as a reference to block writers. As such it is a demonstration of correct semantics
   not a template for the authoring of blocks. Actual implementation may vary. For instance it may create a worker
   thread rather than handle processing synchronously. */
#ifndef INCLUDED_NVMM_VIDEODEC_COMMON_H
#define INCLUDED_NVMM_VIDEODEC_COMMON_H

/**
 *  @defgroup nvmm NVIDIA Multimedia APIs
 */


/**
 *  @defgroup nvmm_modules Multimedia Codec APIs
 *
 *  @ingroup nvmm
 * @{
 */

#include "nvmm.h"
#include "nvmm_event.h"
#include "nvmm_queue.h"
#include "nvassert.h"
#include "nvos.h"
#include "nvrm_init.h"
#include "nvrm_interrupt.h"
#include "nvmm_block.h"
#include "nvmm_videodec.h"
#include "nvrm_avp_shrd_interrupt.h"
#include "nvrm_power.h"

#if defined(__cplusplus)
extern "C"
{
#endif


/*
 * INTERNAL TYPEDEFS/DEFINES
 *
 * A custom block may use these values to the extent
 * that its implementation resembles the VideoDecerence.
 * Even so, the values will likely differ in a custom
 * block.
 */

// these 2 macros indicate the maximum number of macro-blocks for
// resolutions D1 and 1080P
#define VIDEODEC_MBCNT_QVGA         300     // ((320>>4)  * (240>>4))
#define VIDEODEC_MBCNT_VGA          1200    // ((640>>4)  * (480>>4))
#define VIDEODEC_MBCNT_D1           1620    // ((720>>4)  * (576>>4))
#define VIDEODEC_MBCNT_720P         3600    // ((720>>4) * (1280>>4))
#define VIDEODEC_MBCNT_1080P        8160    // ((1920>>4) * (1088>>4))
#define VIDEODEC_MBCNT_2KX2K        16384   // ((2048>>4) * (2048>>4))
#define VIDEODEC_MBCNT_4KX4K        65536   // ((4096>>4) * (4096>>4))

#define VIDEODEC_WIDTH_VGA    640
#define VIDEODEC_HEIGHT_VGA   480

#define VIDEODEC_WIDTH_D1    720
#define VIDEODEC_HEIGHT_D1   480

#define VIDEO_MAX_BUFFER_SIZE ((4096*4096*3)>>2) //(4kx4k yuv frame size)/2
#define VIDEO_MIN_BUFFER_SIZE 1024
#define VIDEO_MAX_INPUT_BUFFERS 32
#define VIDEO_MIN_INPUT_BUFFERS 2
#define VIDEO_MAX_OUTPUT_BUFFERS 32
#define VIDEO_MIN_OUTPUT_BUFFERS 3
#define VIDEO_MAX_QUEUE_SIZE 20
#define INPUT_STREAM 0
#define OUTPUT_STREAM 1
#define VIDEODEC_ENABLE_PROFILING 0
#define NVMM_VDEC_MAX_FRAME_PROF_ENTRIES 4000
#define VDE_MIN_FREQ_REQUIRED_TO_DECODE_1080P       (220 * 1000)
// this is very specific to errorneous cases
#define NV_FRAME_ERROR_THRESHOLD 30;

/** Maximum time needed to wait for hw to finish decoding
this is common to all the decoders except JPEG decoder*/
#define DEC_VDE_TIME_OUT            200
#define JPEG_DEC_VDE_TIME_OUT       850
#define ERROR_VALUE                 0xDEADBEEF

// Chip ID's
#define NV_CHIP_ID_T30 0x30
#define NV_CHIP_ID_AP20 0x20
#define NV_CHIP_ID_AP15 0x15
#define NV_CHIP_ID_AP16 0x16

// Macro to enable/disable T30 specific codepaths. This will be removed eventually. FIXME.
#define VIDEODEC_T30_USE_BSEV_LL    1
#define VIDEODEC_T30_SXE_S          0

// Macro to enable/disable look ahead logic for vc1
#define VIDEODEC_ENABLE_LOOKAHEAD 1

// Macro to enable/disable look ahead logic for h264
#if NV_IS_AVP //
#define VIDEODEC_ENABLE_LOOKAHEAD_H264 1
#else
#define VIDEODEC_ENABLE_LOOKAHEAD_H264 0
#endif

// Macro to enable/disable VDE interrupts on AVP only, CPU it is disabled ; 1 => enable, 0 => disable
#if NV_IS_AVP
#define USE_VDE_INTERRUPTS 1 //
#else
#define USE_VDE_INTERRUPTS 0
#endif

// Timeout value for video decoder blocks
// This timeout value is used by the semaphore wait in nvmm_transport_block.c
// Kept as infinte because semaphore is expected to be signalled only on
// arrival of message from client and not otherwise
#define NVMM_VIDEODEC_TIMEOUT_MSEC NV_WAIT_INFINITE


// Threshold values for skipping the B frames
// Any value send from client will be mapped to these thresholds
#define SKIP_LEVEL1 25
#define SKIP_LEVEL2 50
#define SKIP_LEVEL3 75
// this value is used as reference i.e to skip how many no of frames from next LIMIT_FRAME_SKIP B frames
#define SKIP_LEVELS 4

// stores all details of physically allocated memory
typedef struct {
    NvRmMemHandle hMem;
    NvU32 PhysicalAddress;
    NvU32 VirtualAddress;
    NvU32 Size;
}NvVideoDecMemory;

#if VIDEODEC_T30_USE_BSEV_LL

/*
 *  BseV-LL structure
 *
 * ------------------------------------------
 *  31              | 30:0
 * ------------------------------------------
 *  LL structure size (= n + 1)
 * ------------------------------------------
 *  LL #0 start address (byte address)
 *  last (= 0)      | LL #0 size (in bytes)
 * ------------------------------------------
 *  |
 *  |
 * ------------------------------------------
 *  |
 *  |
 * ------------------------------------------
 *  LL #n start address (byte address)
 *  last (= 1)      | LL #n size (in bytes)
 * ------------------------------------------
 */

#define VDE_MAX_BSEV_LL_ENTRIES 32

// BseV LL struct entry
typedef struct
{
    NvU32 StartAddress;
    NvU32 Last_Size;
} BseV_LL_Entry;

// BseV LL struct
typedef struct
{
    NvU32           LL_NumEntries;
    BseV_LL_Entry   LL_Entry[VDE_MAX_BSEV_LL_ENTRIES];
} BseV_LL_Struct;

// BseV LL Context
typedef struct
{
    NvVideoDecMemory    LL_Struct;
    BseV_LL_Struct      *pLL_Struct;
    NvU32               idx_Curr_Entry;         // index of top un-consumed entry
    NvU32               sizeOfValidData;        // sum of size of all buffers present in the linklist
    NvMMQueueHandle     NvMMBufferQueue;
    NvU32               InpQPeekIndex;
    NvBool              RetainLastLLEntry;      // core decoder sets this flag TRUE if there is a possibility that it might need LastLLEntry again
    NvBool              LastLLEntryRetained;    // common layer uses this flag to maintain if it has a retained buffer
    NvBool              ResendRetainedLLEntry;  // core decoder sets this flag TRUE if it needs the retained buffer back
} BseV_LL_Ctxt;

void NvMMVideoDecPrint_LL_Struct(BseV_LL_Struct *pLL_Struct);

NvU32
NvMMVideoDecLLBytesConsumed(
    BseV_LL_Ctxt *pLL_Ctxt,
    NvU32 CurLLID,
    NvU32 CurByteAddr);

#endif // VIDEODEC_T30_USE_BSEV_LL

/*
 * Component function protottypes. Each video decoder comp has to be complined to this prototypes
 */

/*
 * Comp Open function. Comp has to allocate memory for ppNvVideoCompContext (Comp comntext). Also all
 * initializations for ppNvVideoCompContext has to be done here
 */
typedef NvError (*NvMMVideoCompOpen)(
    NvRmDeviceHandle hDevice,
    NvU32 VDERegBaseAdd,
    void **ppNvVideoCompContext
);

/*
 * Comp Close Function: Comp has to free all the memory allocated in function NvMMVideoCompOpen. Also make
 * *ppNvVideoCompContext to NULL
 */
typedef NvError (*NvMMVideoCompClose)(
    void **ppNvVideoCompContext
);

/*
 * Comp decoder Function: This is main decode function. Input biotstream will be provided in pInpBuff->pBuffer
 * Comp has to assign output buffer pointer to *ppOutBuff. If no output buffer is available then make it NULL.
 * If any meta data information has to be passed from comp to block then it can be passed through pMetaData. Else
 * make *pMetaData = NULL. Internally comp has to typecast *pMetaData to NvMMEventNewVideoStreamFormatInfo * and update
 * field of NvMMEventNewVideoStreamFormatInfo struct. Even new header info can be passed through this struct only.
 * Comp has to increment pInpBuff->startOfValidData with number of bytes consumed.
 */
typedef NvError (*NvMMVideoDecCompDecode)(
    void *pNvVideoCompContext,
    NvMMBuffer *pInpBuff,
    NvMMBuffer **ppOutBuff,
    void *pMetaData
);

/*
 * Comp get attribute function.
 */
typedef NvError (*NvMMVideoDecCompGetAttribute)(
    void *pNvVideoCompContext,
    NvMMVideoDecAttribute VideoDecoderAttribute,
    NvU32 AttributeSize,
    void *ppVideoDecoderAttribute_Param
);

/*
 * Comp set attribute function.
 */
typedef NvError (*NvMMVideoDecCompSetAttribute)(
    void *pNvVideoCompContext,
    NvMMVideoDecAttribute VideoDecoderAttribute,
    NvU32 AttributeSize,
    void *ppVideoDecoderAttribute_Param
);

/*
 * Comp DPB flush function.Comp has to flush output surface one by one in the display order
 */
typedef NvError (*NvMMVideoDecCompFlushDPB)(
    void *pNvVideoCompContext,
    NvMMBuffer **ppOutBuff);

/*
 * Component's interrupt-handler function
 */
typedef void (*NvMMVideoDecCompIntHandler)(
    void *pNvVideoCompContext
);

/*
 * Component's abort buffer function
 */
typedef NvU32 (*NvMMVideoDecCompAbortBuffer)(
    void *pNvVideoCompContext,
    NvMMBuffer **ppoutBufs
);

/*
 * Component's Change state
 */
typedef NvError (*NvMMVideoDecCompSetState)(
    void *pNvVideoCompContext,
    NvMMState eState
);


// Chip Features
typedef struct FeatureRec {
    NvBool ErrorResilienceSupport;
    NvBool DmaFromSdramToVramSupport;
    NvBool Vc1ApSupport;
    NvBool SxepSupport;
    NvBool CoalesceSupport;
    NvBool VldTablesInVramSupport;
    NvBool InterruptConfiguration1;
    NvBool InterruptConfiguration2;
    NvBool WAR10;
    NvBool WAR11;
    NvBool WAR12;  // WARxy represents WAR specific to chips with major version x and minor version y.
    NvBool H264HPSupport;
    NvBool MPEG2Support;
    NvBool LinkedListSupport;
} VidoeoDecFeature;
// Chip Capabilities
typedef struct {
    NvU32 Id;
    VidoeoDecFeature FeatureSupported;
} Capabilities;

typedef struct {
    NvBool ReqInpBuffIsOutOfQueueSize;
    NvU32 ReqInpBuffNum;
} InputBufferRequired;

/*
 * INTERNAL DATA STRUCTURE DEFINITIONS
 *
 * A custom block may use these structures to the
 * extent that its implementation resembles the
 * VideoDecerence. Even so, the fields will likely differ
 * in a custom block.
 */

/**
This Structure is used to decide next action at the common level for Look Ahead.
These fields are obtained by doing getAttribute to block.
*/
#if VIDEODEC_ENABLE_LOOKAHEAD || VIDEODEC_ENABLE_LOOKAHEAD_H264
typedef struct {
    NvU32 PeekNextEntryOfIPQ;
    NvU32 DeQInpBuffer;
    NvBool HwLockAtCommon;
} LookAheadStatus;
#endif

typedef struct {
#define VDE_DFS_BUFF_SIZE       10                  // Window size
#define FREQ_UP_COUNT           5                   // Wait for this time before trying to increase again
#define FREQ_DOWN_COUNT         VDE_DFS_BUFF_SIZE   // Wait for this time before trying to decrease again
#define THRESHOLD_UP            4                   // If o/p free buffers cross this, increase freq
#define THRESHOLD_UP_LONG_TERM  2.5                 // If avg for long time crosses this, increase freq
#define THRESHOLD_UP_SEVERE     5                   // If o/p free buffers cross this, increase freq irrespective of FREQ_UP_COUNT
#define THRESHOLD_DOWN          0.5                 // Decrease freq if avg o/p free buffers decrease below this
#define LOW_CORNER_HIGH         180000              // Low corner based on resolution
#define LOW_CORNER_MID          90000               // > 1280x720: HIGH, > 720x480: MID
#define LOW_CORNER_LOW          50000               // <= 720x480: LOW
    NvU32   MaxFreq;
    NvU32   LowCorner;
    NvU32   NumOutputSurfacesWithDec_Array[VDE_DFS_BUFF_SIZE];
    NvU32   idx;
    NvU32   CountUp;
    NvU32   CountDown;
    NvBool  bFreqIncreased;
    NvBool  bFreqDecreased;
#define GATHER_VDE_DFS_STATS    0   // TBD: Fix Stats Gathering
#if GATHER_VDE_DFS_STATS
    NvU32   NoFramesDecoded;
    NvU32   FreqSum;
    NvU32   NumFreqUp;
#endif
} VdeDfsStruct;

/** Context for VideoDec block

    First member of this struct must be the NvMMBlockContext base class,
    so base class functions (NvMMBlockXXX()) can downcast a VideoDecBlockContext*
    to NvMMBlock*.

    A custom block would add implementation specific members to
    this structure.
*/
typedef struct VideoDecBlockContext_ {
    NvMMBlockContext block; //!< NvMMBlock base class, must be first member to allow down-casting
    // Members for derived block follow here

    // Context of the Video Decoder handle
    void *hVideoDecCompContext;

    NvBool bVideoDecBlockShuttEventSent;
    NvBool bDoVideoDecBlockClose;
    NvBool HwLockReleased;

    NvMMVideoCompOpen CompOpen;
    NvMMVideoCompClose CompClose;
    NvMMVideoDecCompDecode CompDecode;
    NvMMVideoDecCompGetAttribute CompGetAttribute;
    NvMMVideoDecCompSetAttribute CompSetAttribute;
    NvMMVideoDecCompFlushDPB CompFlushDPB;
    NvMMVideoDecCompIntHandler CompSyncTokenIntHandler;
    NvMMVideoDecCompIntHandler CompBseIntHandler;
    NvMMVideoDecCompIntHandler CompSxeIntHandler;
    NvMMVideoDecCompAbortBuffer CompAbortBuffer;
    NvMMVideoDecCompSetState CompSetState;

    // These 2 variables are just put for debugging. Not really required
    NvU32 NoFramesDecoded;
    NvU32 NoFramesDisplayed;
    NvU32 VDEFrameLevelLocked;
    NvU32 VDERegBaseAdd;
    NvU32 VDERegbankSize;
    NvU8 InstanceName[16];
    NvBool NullWorkSpecificEventSent;
    NvMMEventNewVideoStreamFormatInfo StreamInfo;
    NvMMInternalCreationParameters Params;
    // list of IRQ numbers for each VDE interrupt that is enabled
    NvU32 IrqList[MAX_SHRDINT_INTERRUPTS];
    // flag to indicate if interrupts are registered or not
    NvBool bInterruptsRegistered;
    // codec type
    NvMMBlockType eType;
    NvOsInterruptHandle InterruptHandle;
    NvU32 VdeClientId;
    NvU32 chipID;                // To differentiate b/w various chip versions
    VidoeoDecFeature FeatureSupported;
    NvMMBuffer *pOffsetInBuf;

    // Frame Decode Profiling
    NvU32 ProfileFrameNumber;
    NvU32 ProfileStartTime, ProfileEndTime;
    NvU32 EnableVDECacheStat;
    NvU32 *ProfileDataAccumulator;

    //Queue for Timestamps
    NvMMQueueHandle timeStampQueue;
    NvU64 lastPTS;
    NvU32 skippedFrameCount;
    NvBool ptsNotAvailable;

    NvU32 DfsClient;
    NvU32 InputBusyHintIssueTime;
    NvU32 OutputBusyHintIssueTime;
    NvBool bBoost;
    NvRmDfsBusyHint pMultiHint[5];
    NvBool IsClockEnabled;
    NvU32  nBusyHints;
#if VIDEODEC_ENABLE_LOOKAHEAD || VIDEODEC_ENABLE_LOOKAHEAD_H264
    LookAheadStatus LABlockStatus;
#endif
    NvBool bOneInpBuff;
    NvU32 NewStreamFormatCount;
    NvRmFreqKHz VDecBusyHintsBoostKHz [NvRmDfsClockId_Num];

    NvU32 Frames_to_Skip;
    NvU32 Skip_Levels;
    NvU32 Frame_Skipped;
    // as soon as NvMM Buffer is available after decoding send it to client
    NvU32 Always_Skip_NvMM_DeferDelivery;
    InputBufferRequired InpBuffInfo;
    // Flag to indicate whether to set Low Corner frequency
    NvBool bSetLC;

#if VIDEODEC_T30_USE_BSEV_LL
    BseV_LL_Ctxt LL_Ctxt;
    NvVideoDecMemory DummyLLBuffer;
    NvBool PartialSliceSupport;
#endif
    NvBool bNewFrame;
    VdeDfsStruct VdeDfs;
} VideoDecBlockContext;


// Structure used for video decoder profiling
#if VIDEODEC_ENABLE_PROFILING

#define FRAME_LEVEL_PROFILING   1
#define FUNCTION_LEVEL_PROFILING   1

#define NV_MM_PROFILE_VIDEO_INDEX0 0
#define NV_MM_PROFILE_VIDEO_INDEX1 1
#define NV_MM_PROFILE_VIDEO_INDEX2 2
#define NV_MM_PROFILE_VIDEO_INDEX3 3
#define NV_MM_PROFILE_VIDEO_INDEX4 4
#define NV_MM_PROFILE_VIDEO_INDEX5 5
#define NV_MM_PROFILE_VIDEO_INDEX6 6
#define NV_MM_PROFILE_VIDEO_INDEX7 7
#define NV_MM_PROFILE_VIDEO_INDEX8 8
#define NV_MM_PROFILE_VIDEO_INDEX9 9

#define MAX_PROF_VIDEO_BLOCKS  8

typedef struct Profiling_st
{
    NvU32 accumulatedtime[MAX_PROF_VIDEO_BLOCKS];
    NvU32 no_of_times[MAX_PROF_VIDEO_BLOCKS];
    NvU32 last_run_time[MAX_PROF_VIDEO_BLOCKS];
    NvU32 ProfStats[MAX_PROF_VIDEO_BLOCKS];
}VideodecProfiling;

void NvMMVideoDecProfilingStart(NvU32 index);
void NvMMVideoDecProfilingEnd(NvU32 index);
void NvMMVideoDecProfilingInit(void);
#endif // end of #if VIDEODEC_ENABLE_PROFILING

/*
 * EXTERNAL MANDATED ENTRYPOINTS
 *
 * Every custom block must implement its version of
 * these 3 functions and add them to nvmm.c.
 */
#if USE_VDE_INTERRUPTS

#define MUTEX_LOCK_VDE() \
do \
{ \
    if (pContext->VDEFrameLevelLocked == NV_FALSE) \
    { \
        NvRmAvpShrdInterruptAquireHwBlock(NvRmModuleID_Vde, pContext->VdeClientId); \
        pContext->VDEFrameLevelLocked = NV_TRUE; \
        /* Reset VDE */ \
        NvRmModuleReset(pContext->block.hRmDevice, NvRmModuleID_Vde); \
        /* Enable low power  mode of VDE after reset is done */ \
        NvMMVideoDecEnableLowPowerMode(pContext); \
    } \
}while(0)

#define MUTEX_UNLOCK_VDE() \
    do \
{ \
    if (pContext->VDEFrameLevelLocked == NV_TRUE)  \
    { \
        NvRmAvpShrdInterruptReleaseHwBlock(NvRmModuleID_Vde, pContext->VdeClientId); \
        pContext->VDEFrameLevelLocked = NV_FALSE; \
    } \
}while(0)

#else

#define MUTEX_LOCK_VDE() \
do \
{ \
}while(0)

#define MUTEX_UNLOCK_VDE() \
do \
{ \
}while(0)


#endif


/* INTERNAL NVMMBLOCK METHOD PROTOTYPES */

NvError VideoDecBlockDPBFlush(VideoDecBlockContext *pContext);
NvError NvMMVideoDecAbortBuffers(NvMMBlockHandle hBlock, NvU32 StreamIndex);

NvError NvMMVideoDecBlockOpen(
    NvMMBlockHandle *phBlock,
    NvMMInternalCreationParameters *pParams,
    NvOsSemaphoreHandle semaphore,
    NvMMDoWorkFunction *pDoWorkFunction,
    NvMMBlockType Type);

// Function to enable Low-power mode for VDE
void NvMMVideoDecEnableLowPowerMode(VideoDecBlockContext *pContext);

//Function to enable VDE cache
void NvMMVideoDecPrintVDECacheStats(VideoDecBlockContext *pContext);

// Function to update dstRect when srcRect dimentions are lesser than dstRect or dstRect = [0,0,0,0]
void NvMMVideoDecDecideCropRect(NvRect srcRect, NvRect* dstRect);

// Common videodec module reset to be used by all video decoders for enabling safty reset
// after polling set of registers and also for enabling low power mode after module reset
void NvMMVideoDecModuleReset(VideoDecBlockContext *pContext);

/*
* this is used for skipping either B frame/non-ref frames.
* block will will call this function whether to skip the current frame or not.
*/
NvBool NvMMVideoSkipFrames(VideoDecBlockContext *pContext);

#if USE_VDE_INTERRUPTS
NvError NvMMVideoDecEnableInterrupts(VideoDecBlockContext *pContext);
void NvMMVideoDecDisableInterrupts(VideoDecBlockContext *pContext);
#endif

void NvMMVideoDecMutexLockVde(VideoDecBlockContext *pContext);
void NvMMVideoDecMutexUnlockVde(VideoDecBlockContext *pContext);

NvError NvMMVideoDecHwInit(VideoDecBlockContext *pContext);
void VideoDecBlockReportError(NvMMBlockContext block, NvError RetType);


#endif // INCLUDED_NVMM_VIDEODEC_COMMON_H


