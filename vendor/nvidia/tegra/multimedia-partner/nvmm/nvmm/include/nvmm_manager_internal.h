/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_nvmm_mgr_internal_H
#define INCLUDED_nvmm_mgr_internal_H


#if defined(__cplusplus)
extern "C"
{
#endif

#include "nvcommon.h"
#include "nvos.h"
#include "nvmm_manager.h"
#include "nvrm_moduleloader.h"
#include "nvmm.h"
#include "nvmm_service_common.h"

extern NvBlockProfile BlockProfileTable[];

#define MAX_INSTANCE_HWA_AUDIO 1
#define MAX_INSTANCE_HWA_VIDEO 5
#define MAX_INSTANCE_ENCODER 2
#define MAX_INSTANCE_IMAGE_ENCODER 3

/*
 * @ingroup nvmm_manager
 * @{
 */

/**
 * @brief Resource type
 */
typedef enum
{
    /*Video*/
    NvBlockCat_VideoDecoder = 1,

    /*Audio*/
    NvBlockCat_AudioEncoder,
    NvBlockCat_AudioDecoder,

    /*Image*/
    NvBlockCat_ImageEncoder,
    NvBlockCat_ImageDecoder,

    NvBlockCat_Parser,
    NvBlockCat_Renderer,

    NvBlockCat_Other,

    /* TBD: other tracked resources. */
    NvBlockCat_Num,
    NvBlockCat_Force32 = 0x7FFFFFFF
} NvBlockCategory;

/**
 * @brief BLOCK Max resource types
 */
enum { MAX_RESOURCE_TYPES=10};


/**
 * @brief BLOCK clock
 */
typedef enum
{
    NvBlockClock_CPU = 0x1,
    NvBlockClock_AVP = 0x2,
    NvBlockClock_AHB = 0x4,
    NvBlockClock_APB = 0x8,
    NvBlockClock_VPIPE = 0x10,
    NvBlockClock_EMC = 0x20,

    /* TBD: other clocks. */
    NvBlockClock_Num,
    NvBlockClock_Force32 = 0x7FFFFFFF
} NvBlockClock;


/**
 * @brief BLOCK Max elements per use case
 */
enum { MAX_ELEMENTS_PER_USECASE=16};

/**
 * @brief Max resource allotments
 */
enum {MAX_RES_ALLOTMENTS=10};
// end cheats

// enumeration of all possible resource profile ids
// used by both resource profiles and use cases
typedef enum
{
    BP_NvMM_Base = 0x0001,

    BP_NvMM_FileReader_Default,
    BP_NvMM_DecAAC_Default,
    BP_NvMM_DecAAC_Cooperative,
    BP_NvMM_DecMP3_Default,
    BP_NvMM_DecWMA_Default,
    BP_NvMM_DecMPEG4_Default,
    BP_NvMM_DecH264_Default,
    BP_NvMM_DecVC1_Default,
    BP_NvMM_SinkAudio_Default,
    BP_NvMM_AudioMixer_Default,
    BP_NvMM_SinkVideo_Default,
    BP_NvMM_3gpFileReader_Default,

    BP_NvMM_Error_Default,
    BP_Force32 = 0x7fffffff
}BPType;





extern NvBlockProfile BlockResourceProfileTable[];
extern const NvU32 NumResourceProfiles;

typedef struct NvmmMgrBlockRec
{
    void *pBlockHandle;
    NvU32 Type;
    NvU8 RefCount;
    NvBlockProfile *pBP;
    NvMMBlockHandle hActualBlock;
    NvRmLibraryHandle hBlockRmLoader;
    void *hServiceAvp;
    NvRmLibraryHandle hServiceRmLoader;
} NvmmMgr_BP;

typedef struct
{
    NvU32 NumHWAAudioDecInUse;
    NvU32 NumHWAVideoDecInUse;
    NvU32 NumHWAImageDecInUse;
    NvU32 NumAudioEncodersInUse;
    NvU32 NumImageEncodersInUse;
} NvmmMgr_ResourceRec;


/******************************************************************************
 * Internal functions
 *****************************************************************************/

void
GetBPFromBPType(
    BPType BPType,
    NvBlockProfile **ppBP);

void
GetDefaultBPFromBlockType(
    NvU32 BlockType,
    NvBlockProfile **ppBP);

void
GetBlockFromBlockHandle(
     void* pBlockHandle,
    NvmmMgr_BP **ppBlock);


void CreateNewBlockRec(
    NvmmMgr_BP **ppBlock);

void
DecrementBlockRefCount(
    NvmmMgr_BP *pBlock);

NvU32
NvmmManagerGetBlockLocale(
    NvU32 eBlockType);

NvError
NvmmMgr_IramMemAlloc(
    NvMMIRAMScratchType Type,
    NvRmMemHandle *phMemHandle,
    NvU32 Size,
    NvU32 Allignment,
    NvU32 *pPhysicalAddress);

void
NvmmMgr_IramMemFree(
    NvRmMemHandle hMemHandle);

#if defined(__cplusplus)
}
#endif

#endif
