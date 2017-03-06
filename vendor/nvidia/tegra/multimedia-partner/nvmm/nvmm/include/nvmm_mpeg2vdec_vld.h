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

#ifndef INCLUDED_NVMM_MPEG2VDEC_BLOCK_H
#define INCLUDED_NVMM_MPEG2VDEC_BLOCK_H

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


#define VIDEO_MPEG2_MAX_BUFFER_SIZE (310*1024)
#define VIDEO_MPEG2_MIN_BUFFER_SIZE 16*1024
#define VIDEO_MPEG2_MAX_INPUT_BUFFERS 8
#define VIDEO_MPEG2_MIN_INPUT_BUFFERS 4
#define VIDEO_MPEG2_MAX_OUTPUT_BUFFERS 8
#define VIDEO_MPEG2_MIN_OUTPUT_BUFFERS 6
#define VIDEO_MPEG2_MAX_QUEUE_SIZE 20
#define INPUT_STREAM 0 
#define OUTPUT_STREAM 1


// Timeout value for video decoder blocks
// This timeout value is used by the semaphore wait in nvmm_transport_block.c 
// Kept as infinte because semaphore is expected to be signalled only on 
// arrival of message from client and not otherwise
#define NVMM_VIDEODEC_TIMEOUT_MSEC NV_WAIT_INFINITE

/*
 * Component function protottypes. Each video decoder comp has to be complined to this prototypes
 */


/*
 * INTERNAL DATA STRUCTURE DEFINITIONS
 *
 * A custom block may use these structures to the
 * extent that its implementation resembles the
 * VideoDecerence. Even so, the fields will likely differ
 * in a custom block.
 */

/** Context for VideoDec block

    First member of this struct must be the NvMMBlockContext base class,
    so base class functions (NvMMBlockXXX()) can downcast a VideoDecBlockContext*
    to NvMMBlock*.

    A custom block would add implementation specific members to
    this structure.
*/
typedef struct MPEG2VDecVLDBlockContext_ {
    NvMMBlockContext block; //!< NvMMBlock base class, must be first member to allow down-casting

    // Context of the Video Decoder handle
    void *hMpeg2VldCtx;

    NvMMBuffer  InputBuffer;
    NvU32   RemainingBytesForLastBuffer;
    NvBool  MPEG2ResoucesAllocated;
    NvBool  bBlockError;
    NvU32   Width;
    NvU32   Height;
    double  FrameRate;
    NvU64   LastTimeStamp;
    NvU64   StartFrameTimeStamp;
    NvU64   CurrentBufferTimeStamp;
    NvS32   NumberOfOutputEntries;

    // These 2 variables are just put for debugging. Not really required
    NvU32 NoFramesDecoded;
    NvU32 NoFramesDisplayed;
    NvMMEventNewVideoStreamFormatInfo StreamInfo;
} MPEG2VDecVLDBlockContext;

NvError NvMMMPEG2VDecAbortBuffers(NvMMBlockHandle hBlock, NvU32 StreamIndex);

NvError NvMMMPEG2VDecVldBlockOpen(
    NvMMBlockHandle *phBlock,
    NvMMInternalCreationParameters *pParams,
    NvOsSemaphoreHandle semaphore,
    NvMMDoWorkFunction *pDoWorkFunction);

void MPEG2VDecBlockReportError(NvMMBlockContext block, NvError RetType);

void NvMMMPEG2VDecVldBlockClose(NvMMBlockHandle hBlock, 
     NvMMInternalDestructionParameters *pParams);

#if defined(__cplusplus)
}
#endif

/** @} */

#endif // INCLUDED_NVMM_MPEG2VDEC_BLOCK_H
