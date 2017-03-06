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

#ifndef INCLUDED_NVMM_VideoDECBLOCK_H
#define INCLUDED_NVMM_VideoDECBLOCK_H

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
 * H.264 Decoder Block APIs
 */

NvError NvMMH264DecBlockOpen(
        NvMMBlockHandle *phBlock,
        NvMMInternalCreationParameters *pParams,
        NvOsSemaphoreHandle semaphore,
    NvMMDoWorkFunction *pDoWorkFunction);

void NvMMH264DecBlockClose(NvMMBlockHandle hBlock,
     NvMMInternalDestructionParameters *pParams);

NvError NvMMH264Dec2xBlockOpen(
    NvMMBlockHandle *phBlock,
    NvMMInternalCreationParameters *pParams,
    NvOsSemaphoreHandle semaphore,
    NvMMDoWorkFunction *pDoWorkFunction);

void NvMMH264Dec2xBlockClose(NvMMBlockHandle hBlock,
    NvMMInternalDestructionParameters *pParams);

NvError
NvMMH264DecBlockCapabilities(
    NvMMBlockType eType,
    NvMMLocale eLocale,
    void *pCaps);


/*
 * VC1 Decoder Block APIs
 */
NvError
NvMMVc1DecBlockOpen(
    NvMMBlockHandle *phBlock,
    NvMMInternalCreationParameters *pParams,
    NvOsSemaphoreHandle semaphore,
    NvMMDoWorkFunction *pDoWorkFunction);

NvError
NvMMVc1Dec2xBlockOpen(
    NvMMBlockHandle *phBlock,
    NvMMInternalCreationParameters *pParams,
    NvOsSemaphoreHandle semaphore,
    NvMMDoWorkFunction *pDoWorkFunction);
    
void NvMMVc1DecBlockClose(NvMMBlockHandle hBlock, NvMMInternalDestructionParameters *pParams);

void NvMMVc1Dec2xBlockClose(NvMMBlockHandle hBlock, NvMMInternalDestructionParameters *pParams);

NvError
NvMMVc1DecBlockCapabilities(
    NvMMBlockType eType,
    NvMMLocale eLocale,
    void *pCaps);


/*
 * JPEG Decoder Block APIs
 */

NvError NvMMJpegDecBlockOpen(
        NvMMBlockHandle *phBlock,
        NvMMInternalCreationParameters *pParams,
        NvOsSemaphoreHandle semaphore,
    NvMMDoWorkFunction *pDoWorkFunction);

void NvMMJpegDecBlockClose(NvMMBlockHandle hBlock, NvMMInternalDestructionParameters *pParams);

/*
 * MPEG4 Decoder Block APIs
 */
NvError NvMMMPEG4DecBlockOpen(
        NvMMBlockHandle *phBlock,
        NvMMInternalCreationParameters *pParams,
        NvOsSemaphoreHandle semaphore,
    NvMMDoWorkFunction *pDoWorkFunction);

void NvMMMPEG4DecBlockClose(NvMMBlockHandle hBlock, NvMMInternalDestructionParameters *pParams);
/*
 * Super JPEG Decoder Block APIs
 */
NvError NvMMSuperJpegDecBlockOpen(
    NvMMBlockHandle *phBlock,
    NvMMInternalCreationParameters *pParams,
    NvOsSemaphoreHandle semaphore,
    NvMMDoWorkFunction *pDoWorkFunction);

void NvMMSuperJpegDecBlockClose(NvMMBlockHandle hBlock, NvMMInternalDestructionParameters *pParams);


/*
 * JPEG Progressive Decoder Block APIs
 */
NvError NvMMJpegProgressiveDecBlockOpen(
    NvMMBlockHandle *phBlock,
    NvMMInternalCreationParameters *pParams,
    NvOsSemaphoreHandle semaphore,
    NvMMDoWorkFunction *pDoWorkFunction);

void NvMMJpegProgressiveDecBlockClose(NvMMBlockHandle hBlock, NvMMInternalDestructionParameters *pParams);


/*
 * MPEG2 Decoder Block APIs
 */
NvError NvMMMPEG2DecBlockOpen(
        NvMMBlockHandle *phBlock,
        NvMMInternalCreationParameters *pParams,
        NvOsSemaphoreHandle semaphore,
    NvMMDoWorkFunction *pDoWorkFunction);

void NvMMMPEG2DecBlockClose(NvMMBlockHandle hBlock, NvMMInternalDestructionParameters *pParams);


#if defined(__cplusplus)
}
#endif

/** @} */

#endif // INCLUDED_NVMM_VideoDECBLOCK_H
