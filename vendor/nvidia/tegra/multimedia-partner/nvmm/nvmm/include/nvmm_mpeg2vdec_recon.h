/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * @brief <b>nVIDIA Driver Development Kit:
 *           MPEG2 Video Decoder Reconstruction Block </b>
 *
 * @b Description: Defines the NvMM interfaces to the Mpeg2 Video Decoder Reconstruction Block.
 */

#ifndef INCLUDED_NVMM_MPEG2VDEC_RECON_H
#define INCLUDED_NVMM_MPEG2VDEC_RECON_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "nvcommon.h"
#include "nvmm.h"
#include "nvmm_event.h"
#include "nvmm_queue.h"
#include "nvassert.h"
#include "nvos.h"
#include "nvrm_init.h"
#include "nvrm_interrupt.h"
#include "nvmm_block.h"

#define MPEG2VDEC_RECON_MAX_BUFFER_SIZE 52*1024
#define MPEG2VDEC_RECON_MIN_BUFFER_SIZE 1024
#define MPEG2VDEC_RECON_MAX_INPUT_BUFFERS 8
#define MPEG2VDEC_RECON_MIN_INPUT_BUFFERS 6
#define MPEG2VDEC_RECON_MAX_OUTPUT_BUFFERS 32
#define MPEG2VDEC_RECON_MIN_OUTPUT_BUFFERS 14
#define MPEG2VDEC_RECON_OUTPUT_BUFFER_BYTE_ALIGNMENT 1024
#define MPEG2VDEC_RECON_INPUT_BUFFER_BYTE_ALIGNMENT 256

NvError 
NvMMMpeg2VDecReconBlockOpen(
    NvMMBlockHandle *phBlock, 
    NvMMInternalCreationParameters *pParams, 
    NvOsSemaphoreHandle semaphore, 
    NvMMDoWorkFunction *pDoWorkFunction);

void
NvMMMpeg2VDecReconBlockClose(
    NvMMBlockHandle hBlock, 
    NvMMInternalDestructionParameters *pParams);

#ifdef __cplusplus
}
#endif

#endif // INCLUDED_NVMM_MPEG2VDEC_RECON_H
