/* Copyright (c) 2006-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an
 * express license agreement from NVIDIA Corporation is strictly prohibited.
 */


// nvmm_manager.c
//

#include "nvassert.h"
#include "nvrm_transport.h"
#include "nvrm_moduleloader.h"
#include "nvrm_hardware_access.h"
#include "nvmm_manager_internal.h"
#include "nvmm.h"
#include "nvmm_manager.h"
#include "nvmm_manager_common.h"
#include "list.h"
#include "nvreftrack.h"
#include "nvmm_videodecblock.h"
#include "nvmm_videodec_common.h"

#if NVOS_IS_LINUX
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/tegra_mediaserver.h>
#include "nvavp.h"
#endif

typedef struct NvmmManagerRec
{
    List                    BlockList;
    List                    NvmmClientCallback;
    NvmmManagerPowerState   CurrentPowerState;
    NvOsSemaphoreHandle     hSyncSema;
    NvOsSemaphoreHandle     hSema;
    NvU32                   nRefCount;
    NvmmMgr_ResourceRec     ResourceRec;
    NvRmDeviceHandle        hRmDevice;
    NvU32                   IRAMScratchSpaceCount[NvMMIRAMScratchType_Num];
    NvOsMutexHandle         Mutex;
#if !NVOS_IS_LINUX
    NvRmTransportHandle     hRmTransport;
    NvRmLibraryHandle       hRmLoader;
#else
    int                     FileDescriptor;
#endif
} NvmmMgrCtx;

NvMMServiceMsgInfo_AllocScratchIRAMResponse IRAMScratchSpace[NvMMIRAMScratchType_Num];
#if !NVOS_IS_LINUX
static NvError NvmmManagerSendMessage(void* pMessageBuffer, NvU32 MessageSize);
#endif

static NvmmMgrCtx gs_Ctx;
static int gs_IsUsingNewAvp = 0;

#if NVOS_IS_LINUX
static NvOsMutexHandle gs_Mutex = 0;

void __attribute__ ((constructor)) nvmm_manager_loadlib(void);
void __attribute__ ((destructor)) nvmm_manager_unloadlib(void);

void nvmm_manager_loadlib(void)
{
    NvAvpHandle hAVP = NULL;

    if (gs_Mutex == 0)
    {
        NvError e = NvOsMutexCreate(&gs_Mutex);
        if (e != NvSuccess)
            NvOsDebugPrintf("Error creating manager mutex\n");
        else
        {
            NvOsMemset(&gs_Ctx, 0, sizeof(NvmmMgrCtx));
            gs_Ctx.Mutex = gs_Mutex;
        }
    }

    if (NvAvpOpen(&hAVP) != NvSuccess)
        gs_IsUsingNewAvp = 0;
    else
    {
        gs_IsUsingNewAvp = !!(int)hAVP;
        NvAvpClose(hAVP);
    }
}

void nvmm_manager_unloadlib(void)
{
    if (gs_Mutex)
        NvOsMutexDestroy(gs_Mutex);
    gs_Ctx.Mutex = NULL;
    gs_Mutex = NULL;
}
#endif

int NvMMManagerIsUsingNewAVP(void)
{
    return gs_IsUsingNewAvp;
}

static void NvmmManagerFreeHwBlock(NvmmMgrBlockHandle hBlock)
{
#if NVOS_IS_LINUX
    union tegra_mediaserver_free_info info;

    info.in.tegra_mediaserver_resource_type = TEGRA_MEDIASERVER_RESOURCE_BLOCK;
    info.in.u.nvmm_block_handle = (int)hBlock;

    ioctl(gs_Ctx.FileDescriptor, TEGRA_MEDIASERVER_IOCTL_FREE, &info);
#endif
};

static NvError NvmmManagerAllocHwBlock(NvmmMgrBlockHandle hBlock,
                                       NvBool *ClearGreedyIramAlloc)
{
#if !NVOS_IS_LINUX
    // Don't set greedy IRAM allocation if either HWA Audio or Video decoder is already running
    *ClearGreedyIramAlloc =
        ((gs_Ctx.ResourceRec.NumHWAAudioDecInUse != 0) ||
         (gs_Ctx.ResourceRec.NumHWAVideoDecInUse != 0)) ? NV_TRUE : NV_FALSE;
    return NvSuccess;
#else
     union tegra_mediaserver_alloc_info info;

     NvOsMemset(&info, 0, sizeof(info));
     info.in.tegra_mediaserver_resource_type = TEGRA_MEDIASERVER_RESOURCE_BLOCK;
     info.in.u.block.nvmm_block_handle = (int)hBlock;

     if (ioctl(gs_Ctx.FileDescriptor, TEGRA_MEDIASERVER_IOCTL_ALLOC, &info) < 0)
     {
         return NvError_InsufficientMemory;
     }

    *ClearGreedyIramAlloc = (info.out.u.block.count == 1) ? NV_FALSE : NV_TRUE;
    return NvSuccess;
#endif
}

NvError
NvmmManagerOpen(NvmmManagerHandle *phNvmmMgr)
{
    NvError e;

#if !NVOS_IS_LINUX
    if (gs_Ctx.Mutex == NULL)
    {
         NvOsMutexHandle NvmmMgrMutex = NULL;
         e = NvOsMutexCreate(&NvmmMgrMutex);
         if (e != NvSuccess)
             return e;

        if (NvOsAtomicCompareExchange32((NvS32*)&gs_Ctx.Mutex, 0, (NvS32)NvmmMgrMutex) != 0)
             NvOsMutexDestroy(NvmmMgrMutex);
    }
#endif
    NvOsMutexLock(gs_Ctx.Mutex);

    // Check if nvmm manager is already open
    if(gs_Ctx.nRefCount > 0)
    {
        gs_Ctx.nRefCount++;
        *phNvmmMgr = &gs_Ctx;
        NvOsMutexUnlock(gs_Ctx.Mutex);
        return NvSuccess;
    }

    NV_CHECK_ERROR_CLEANUP(NvRmOpen(&gs_Ctx.hRmDevice, 0));

    NV_CHECK_ERROR_CLEANUP(ListCreate(&gs_Ctx.BlockList));

    NvOsMemset (&gs_Ctx.ResourceRec, 0, sizeof(NvmmMgr_ResourceRec));

    NV_CHECK_ERROR_CLEANUP(ListCreate(&gs_Ctx.NvmmClientCallback));

    NV_CHECK_ERROR_CLEANUP(NvOsSemaphoreCreate(&gs_Ctx.hSyncSema, 0));

    NV_CHECK_ERROR_CLEANUP(NvOsSemaphoreCreate(&gs_Ctx.hSema, 0));

    gs_Ctx.CurrentPowerState = NvmmManagerPowerState_FullOn;

#if !NV_DEF_ENVIRONMENT_SUPPORTS_SIM
#if !NVOS_IS_LINUX
    NV_CHECK_ERROR_CLEANUP(NvRmTransportOpen(gs_Ctx.hRmDevice,
                                             "NVMM_MANAGER_SRV",
                                             gs_Ctx.hSema,
                                             &gs_Ctx.hRmTransport));

    NV_CHECK_ERROR_CLEANUP(NvRmTransportSetQueueDepth(gs_Ctx.hRmTransport,
                                                      30,
                                                      256));

    NV_CHECK_ERROR_CLEANUP(NvRmLoadLibrary(gs_Ctx.hRmDevice,
                                           "nvmm_manager.axf",
                                           &gs_Ctx,
                                           sizeof(NvU32),
                                           &gs_Ctx.hRmLoader));

    NV_CHECK_ERROR_CLEANUP(NvRmTransportConnect(gs_Ctx.hRmTransport, 50000));
#else

    if (!NvMMManagerIsUsingNewAVP())
    {
        gs_Ctx.FileDescriptor = open("/dev/tegra_mediaserver", 0);
        if (gs_Ctx.FileDescriptor < 0)
        {
            gs_Ctx.FileDescriptor = 0;
            e = NvError_NotInitialized;
            goto fail;
        }
    }
    else
    {
       gs_Ctx.FileDescriptor = 0;
    }

#endif
#endif

    *phNvmmMgr = &gs_Ctx;
    gs_Ctx.nRefCount = 1;
    NvOsMutexUnlock(gs_Ctx.Mutex);
    return NvSuccess;

fail:

#if !NVOS_IS_LINUX
    if (gs_Ctx.hRmTransport)
        NvRmTransportClose(gs_Ctx.hRmTransport);
    if (gs_Ctx.hRmLoader)
         NvRmFreeLibrary(gs_Ctx.hRmLoader);
#else
    if (0 < gs_Ctx.FileDescriptor)
    {
        close(gs_Ctx.FileDescriptor);
    }
#endif
    ListDestroy(&gs_Ctx.BlockList);
    ListDestroy(&gs_Ctx.NvmmClientCallback);
    NvOsSemaphoreDestroy(gs_Ctx.hSyncSema);
    NvOsSemaphoreDestroy(gs_Ctx.hSema);
    NvRmClose(gs_Ctx.hRmDevice);
    NvOsMutexUnlock(gs_Ctx.Mutex);
#if !NVOS_IS_LINUX
    NvOsMutexDestroy(gs_Ctx.Mutex);
    gs_Ctx.Mutex = NULL;
#endif

    return e;
}


void
NvmmManagerClose(NvmmManagerHandle hNvmmMgr, NvBool bForceClose)
{
    if (hNvmmMgr != &gs_Ctx)
    {
         NV_ASSERT(0);
         return;
    }

    NvOsMutexLock(gs_Ctx.Mutex);
    if (!gs_Ctx.nRefCount)
    {
        NvOsMutexUnlock(gs_Ctx.Mutex);
        return;
    }

    gs_Ctx.nRefCount--;
    if (gs_Ctx.nRefCount==0)
    {
#if !NVOS_IS_LINUX
        if (gs_Ctx.hRmTransport)
            NvRmTransportClose(gs_Ctx.hRmTransport);
        if (gs_Ctx.hRmLoader)
             NvRmFreeLibrary(gs_Ctx.hRmLoader);
#else
        if (0 < gs_Ctx.FileDescriptor)
        {
            close(gs_Ctx.FileDescriptor);
        }
#endif

        ListDestroy(&gs_Ctx.BlockList);
        ListDestroy(&gs_Ctx.NvmmClientCallback);
        NvOsSemaphoreDestroy(gs_Ctx.hSyncSema);
        NvOsSemaphoreDestroy(gs_Ctx.hSema);
        NvOsMutexUnlock(gs_Ctx.Mutex);
#if !NVOS_IS_LINUX
        NvOsMutexDestroy(gs_Ctx.Mutex);
        gs_Ctx.Mutex = NULL;
#endif
        NvRmClose(gs_Ctx.hRmDevice);

        return;
    }

    NvOsMutexUnlock(gs_Ctx.Mutex);

    return;
}

#if !NVOS_IS_LINUX
static NvError NvmmManagerSendMessage(void* pMessageBuffer, NvU32 MessageSize)
{
    NvmmManagerHandle pNvmmManager = &gs_Ctx;
    NvError Status = NvSuccess;
    Status = NvRmTransportSendMsg(pNvmmManager->hRmTransport,
                                  pMessageBuffer,
                                  MessageSize,
                                  NV_WAIT_INFINITE);
    return Status;
}
#endif

NvError NvmmManagerIRAMScratchAlloc(NvmmManagerHandle hNvmmMgr,
                                    void *pResponseStruct,
                                    NvU32 CodecType,
                                    NvU32 Size)
{
    NvError Status = NvSuccess;
    NvU32 CurrentIRAMScratchCount;
    NvMMServiceMsgInfo_AllocScratchIRAMResponse *pResponse =
        (NvMMServiceMsgInfo_AllocScratchIRAMResponse *)pResponseStruct;
    NvMMIRAMScratchType ScratchType = (NvMMIRAMScratchType)CodecType;

    if (hNvmmMgr != &gs_Ctx)
    {
        NV_ASSERT(0);
        return NvError_BadParameter;
    }

    CurrentIRAMScratchCount = gs_Ctx.IRAMScratchSpaceCount[ScratchType];

    if (CurrentIRAMScratchCount == 0)
    {
        Status = NvmmMgr_IramMemAlloc(ScratchType, &IRAMScratchSpace[ScratchType].hMem,
                                      Size, 256,
                                      &IRAMScratchSpace[ScratchType].pPhyAddr);
        if (Status != NvSuccess)
        {
            goto AllocFail;
        }

        IRAMScratchSpace[ScratchType].Length = Size;
    }

    pResponse->pPhyAddr = IRAMScratchSpace[ScratchType].pPhyAddr;
    pResponse->hMem = IRAMScratchSpace[ScratchType].hMem;
    pResponse->Length = IRAMScratchSpace[ScratchType].Length;

    gs_Ctx.IRAMScratchSpaceCount[ScratchType]++;

    return Status;

AllocFail:

    pResponse->hMem = NULL;
    pResponse->pVirtAddr = 0;
    pResponse->pPhyAddr = 0;
    pResponse->Length = 0;
    return Status;
}

NvError NvmmManagerIRAMScratchFree(NvmmManagerHandle hNvmmMgr,
                                   NvU32 CodecType)
{
    NvError Status = NvSuccess;
    NvU32 CurrentIRAMScratchCount;
    NvMMIRAMScratchType ScratchType = (NvMMIRAMScratchType)CodecType;

    if (hNvmmMgr != &gs_Ctx)
    {
        NV_ASSERT(0);
        return NvError_BadParameter;
    }

    if (gs_Ctx.IRAMScratchSpaceCount[ScratchType] != 0)
    {
        gs_Ctx.IRAMScratchSpaceCount[ScratchType]--;
    }

    CurrentIRAMScratchCount = gs_Ctx.IRAMScratchSpaceCount[ScratchType];

    if (CurrentIRAMScratchCount == 0)
    {
        if (IRAMScratchSpace[ScratchType].hMem)
        {
#if !NVOS_IS_LINUX
            NvRmMemUnpin(IRAMScratchSpace[ScratchType].hMem);
            NvRmMemHandleFree(IRAMScratchSpace[ScratchType].hMem);
#else
            union tegra_mediaserver_free_info info;

            info.in.tegra_mediaserver_resource_type = TEGRA_MEDIASERVER_RESOURCE_IRAM;
            info.in.u.iram_rm_handle = (int)IRAMScratchSpace[ScratchType].hMem;

            ioctl(gs_Ctx.FileDescriptor, TEGRA_MEDIASERVER_IOCTL_FREE, &info);
#endif
            IRAMScratchSpace[ScratchType].pPhyAddr = 0;
            IRAMScratchSpace[ScratchType].hMem = NULL;
            IRAMScratchSpace[ScratchType].Length = 0;
        }
    }

    return Status;
}

NvError
NvmmManagerRegisterBlock(
    NvmmManagerHandle hNvmmMgr,
    NvmmMgrBlockHandle *phBlock,
    NvU32 BlockType,
    NvmmUCType uctype,
    NvBlockProfile *pBP)
{
    NvError err = NvError_Success;

    NvmmMgr_BP *pBlock;
    NvBlockProfile *pDefaultBP = NULL;
    NvBool HwBlockAllocated = NV_FALSE;

    if (hNvmmMgr != &gs_Ctx)
    {
         NV_ASSERT(0);
         return NvError_BadParameter;
    }

    NvOsMutexLock(hNvmmMgr->Mutex);
    CreateNewBlockRec(&pBlock);
    if (NULL == pBlock)
    {
         err = NvError_ResourceError;
         goto cleanup;
    }

    // fill in fields
    pBlock->pBlockHandle = pBlock;

    pBlock->pBP = (NvBlockProfile *)NvOsAlloc(sizeof(NvBlockProfile));
    if (NULL == pBlock->pBP) goto cleanup;


    if ((BlockType == NvMMBlockType_DecH264) ||
        (BlockType == NvMMBlockType_DecVC1) ||
        (BlockType == NvMMBlockType_DecMPEG2))
    {
        NvRmModuleID ModuleId;
        // VidoeoDecFeature FeatureSupported;
        static Capabilities Cap15, Cap20, Cap16, Cap30;
        Capabilities *capabilities;
        NvRmModuleCapability caps[] =
            {
                {1, 2, 0, NvRmModulePlatform_Silicon, (void *)&Cap20},
                {1, 0, 0, NvRmModulePlatform_Silicon, (void *)&Cap15},
                {1, 1, 0, NvRmModulePlatform_Silicon, (void *)&Cap16},
                {1, 3, 0, NvRmModulePlatform_Silicon, (void *)&Cap30},
            };
        // Feature10 represents features supported by the chip whose major version is 1 and minor version is 0
        VidoeoDecFeature Feature10 = {0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0};
        VidoeoDecFeature Feature11 = {0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0};
        VidoeoDecFeature Feature12 = {1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0};
        VidoeoDecFeature Feature13 = {1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1};

        ModuleId = NVRM_MODULE_ID(NvRmModuleID_Vde, 0);

        // for AP20
        Cap20.FeatureSupported = Feature12;
        // for AP15 A01 and A02
        Cap15.FeatureSupported = Feature10;
        // for AP16
        Cap16.FeatureSupported = Feature11;
        // for T30
        Cap30.FeatureSupported = Feature13;

        err = NvRmModuleGetCapabilities(gs_Ctx.hRmDevice, ModuleId, caps,
                        NV_ARRAY_SIZE(caps), (void **)&capabilities);
        if (err != NvSuccess)
            goto cleanup;

        if (capabilities != &Cap15 && capabilities != &Cap16 && capabilities != &Cap20)
        {
            if (BlockType == NvMMBlockType_DecVC1)
                BlockType = NvMMBlockType_DecVC1_2x;
            else if (BlockType == NvMMBlockType_DecH264)
                BlockType = NvMMBlockType_DecH264_2x;
        }

        if (BlockType == NvMMBlockType_DecMPEG2 &&
            (capabilities == &Cap15 || capabilities == &Cap16 || capabilities == &Cap20))
        {
            BlockType = NvMMBlockType_DecMPEG2VideoVld;
        }

        // Use default BP
        GetDefaultBPFromBlockType(BlockType, &pDefaultBP);

        if (capabilities->FeatureSupported.SxepSupport == 0)
            pDefaultBP->BlockFlags = NvBlockFlag_Default | NvBlockFlag_HWA;
    }
    else
    {
        // Use default BP
        GetDefaultBPFromBlockType(BlockType, &pDefaultBP);
    }

    // Copy the record from the Block Profile table to the handle
    NvOsMemcpy(pBlock->pBP, pDefaultBP, sizeof(NvBlockProfile));

    pBlock->pBP->BlockType = BlockType;
    pBlock->pBP->pBlockHandle = pBlock;
    pBlock->pBP->Locale = NvmmManagerGetBlockLocale(BlockType);

    // Allocate HW resources

    if (pBlock->pBP->Locale == NvMMLocale_AVP)
    {
        NvBool clearGreeyIramAlloc = 0;
        err = NvmmManagerAllocHwBlock(pBlock, &clearGreeyIramAlloc);
        if (err == NvSuccess)
        {
            HwBlockAllocated = NV_TRUE;
        }

        if (clearGreeyIramAlloc)
            pBlock->pBP->BlockFlags &= ~NvBlockFlag_UseGreedyIramAlloc;

        if (NvBlockFlag_UseCustomStack & pBlock->pBP->BlockFlags)
        {
            err = NvmmMgr_IramMemAlloc(0, &pBlock->pBP->hMemHandle,
                                       pBlock->pBP->StackSize,
                                       4,
                                       &(pBlock->pBP->StackPtr));
        }
    }

    // Apply local concurrency policies
    if ((BlockType == NvMMBlockType_DecAAC) ||
        (BlockType == NvMMBlockType_DecMP3) ||
        (BlockType == NvMMBlockType_DecWMA) ||
        (BlockType == NvMMBlockType_DecWMAPRO))
    {
        if (gs_Ctx.ResourceRec.NumHWAAudioDecInUse >= MAX_INSTANCE_HWA_AUDIO)
        {
            if ((NvBlockFlag_HWA & pBlock->pBP->BlockFlags) &&
                (NvBlockFlag_CPU_OK & pBlock->pBP->BlockFlags))
            {
                 pBlock->pBP->Locale = NvMMLocale_CPU;

                if (BlockType == NvMMBlockType_DecWMA)
                {
                     pBlock->pBP->BlockFlags |= NvBlockFlag_UseNewBlockType;
                     pBlock->pBP->BlockType = NvMMBlockType_DecWMAPRO;
                }
                else if (BlockType == NvMMBlockType_DecAAC)
                {
                     pBlock->pBP->BlockFlags |= NvBlockFlag_UseNewBlockType;
                     pBlock->pBP->BlockType = NvMMBlockType_DecEAACPLUS;
                }
                else if (BlockType == NvMMBlockType_DecMP3)
                {
                     pBlock->pBP->BlockFlags |= NvBlockFlag_UseNewBlockType;
                     pBlock->pBP->BlockType = NvMMBlockType_DecMP3SW;
                }
            }
            else
            {
                 err = NvError_BadParameter;
                 goto cleanup;
            }
        }
        else
        {
            gs_Ctx.ResourceRec.NumHWAAudioDecInUse++;
        }
    }
    else if ((BlockType == NvMMBlockType_DecH264)    ||
             (BlockType == NvMMBlockType_DecH264_2x) ||
             (BlockType == NvMMBlockType_DecVC1)     ||
             (BlockType == NvMMBlockType_DecVC1_2x)  ||
             (BlockType == NvMMBlockType_DecMPEG2)   ||
             (BlockType == NvMMBlockType_DecMPEG4))
    {
        if (gs_Ctx.ResourceRec.NumHWAVideoDecInUse >= MAX_INSTANCE_HWA_VIDEO)
        {
             err = NvError_ResourceError;
             goto cleanup;
        }
        else
            gs_Ctx.ResourceRec.NumHWAVideoDecInUse++;
    }
    else if ((BlockType == NvMMBlockType_EncAMRNB) ||
             (BlockType == NvMMBlockType_EncAMRWB) ||
             (BlockType == NvMMBlockType_EncAAC)   ||
             (BlockType == NvMMBlockType_EncMP3)   ||
             (BlockType == NvMMBlockType_EncSBC)   ||
             (BlockType == NvMMBlockType_EnciLBC))
    {
        if (gs_Ctx.ResourceRec.NumAudioEncodersInUse >= MAX_INSTANCE_ENCODER)
        {
             err = NvError_ResourceError;
             goto cleanup;
        }
        else
            gs_Ctx.ResourceRec.NumAudioEncodersInUse++;
    }
    else if (BlockType == NvMMBlockType_EncJPEG)
    {
        if (gs_Ctx.ResourceRec.NumImageEncodersInUse >= MAX_INSTANCE_IMAGE_ENCODER)
        {
             err = NvError_ResourceError;
             goto cleanup;
        }
        else
            gs_Ctx.ResourceRec.NumImageEncodersInUse++;
    }

    // If we switched the block to SW, free any HW resources.
    if (pBlock->pBP->Locale == NvMMLocale_CPU)
    {
        if (pBlock->pBP->hMemHandle)
        {
            NvmmMgr_IramMemFree(pBlock->pBP->hMemHandle);
            pBlock->pBP->hMemHandle = 0;
        }

        if (HwBlockAllocated)
        {
            NvmmManagerFreeHwBlock(pBlock);
            HwBlockAllocated = NV_FALSE;
        }
    }

    NvOsMemcpy(pBP, pBlock->pBP, sizeof(NvBlockProfile));
    NvOsMutexUnlock(hNvmmMgr->Mutex);

    *phBlock = pBlock;
    return NvError_Success;

cleanup:

    if (pBlock)
    {
        if (pBlock->pBP)
        {
            if (pBlock->pBP->hMemHandle)
                NvmmMgr_IramMemFree(pBlock->pBP->hMemHandle);

            if (HwBlockAllocated)
                NvmmManagerFreeHwBlock(pBlock);

            NvOsFree(pBlock->pBP);
        }
    }
    NvOsMutexUnlock(hNvmmMgr->Mutex);
    return err;
}

NvError
NvmmManagerUpdateBlockInfo(NvmmMgrBlockHandle hBlock,
                           void *hActualBlock,
                           void *hBlockRmLoader,
                           void *hServiceAvp,
                           void *hServiceRmLoader)
{
    NvError err = NvError_Success;
    NvmmMgr_BP *pBlock;
    NvmmManagerHandle hNvmmMgr = &gs_Ctx;

    NvOsMutexLock(hNvmmMgr->Mutex);
    GetBlockFromBlockHandle(hBlock, &pBlock);
    if(NULL == pBlock)
    {
         NvOsMutexUnlock(hNvmmMgr->Mutex);
         return NvError_ResourceError;
    }
    else
    {
         pBlock->hBlockRmLoader   = (NvRmLibraryHandle)hBlockRmLoader;
         pBlock->hActualBlock     = (NvMMBlockHandle)hActualBlock;
         pBlock->hServiceAvp      = hServiceAvp;
         pBlock->hServiceRmLoader = (NvRmLibraryHandle)hServiceRmLoader;

#if NVOS_IS_LINUX
         if (pBlock->pBP->Locale == NvMMLocale_AVP)
         {
             union tegra_mediaserver_update_block_info info;

             info.in.nvmm_block_handle = (int)hBlock;
             info.in.avp_block_handle = (int)hActualBlock;
             info.in.avp_block_library_handle = (int)hBlockRmLoader;
             info.in.service_handle = (int)hServiceAvp;
             info.in.service_library_handle = (int)hServiceRmLoader;

             ioctl(gs_Ctx.FileDescriptor, TEGRA_MEDIASERVER_IOCTL_UPDATE_BLOCK_INFO, &info);
         }
#endif
    }

    NvOsMutexUnlock(hNvmmMgr->Mutex);
    return err;
}

NvError
NvmmManagerUnregisterBlock(
    NvmmMgrBlockHandle hBlock,
    NvBool bForceClose)
{
    NvmmMgr_BP *pBlock;
    NvmmManagerHandle hNvmmMgr = &gs_Ctx;

    NvOsMutexLock(hNvmmMgr->Mutex);

    GetBlockFromBlockHandle(hBlock, &pBlock);
    if(NULL == pBlock)
    {
        NvOsMutexUnlock(hNvmmMgr->Mutex);
        return NvError_ResourceError;
    }

    if( (pBlock->pBP->BlockType == NvMMBlockType_DecAAC) ||
        (pBlock->pBP->BlockType == NvMMBlockType_DecMP3) ||
        (pBlock->pBP->BlockType == NvMMBlockType_DecWMA) ||
        (pBlock->pBP->BlockType == NvMMBlockType_DecWMAPRO))
    {
        if(pBlock->pBP->Locale == NvMMLocale_AVP)
            gs_Ctx.ResourceRec.NumHWAAudioDecInUse--;
    }
    else if ( (pBlock->pBP->BlockType == NvMMBlockType_DecH264)  ||
              (pBlock->pBP->BlockType == NvMMBlockType_DecH264_2x)   ||
              (pBlock->pBP->BlockType == NvMMBlockType_DecVC1)   ||
              (pBlock->pBP->BlockType == NvMMBlockType_DecVC1_2x)   ||
              (pBlock->pBP->BlockType == NvMMBlockType_DecMPEG2) ||
              (pBlock->pBP->BlockType == NvMMBlockType_DecMPEG4) )
    {
        gs_Ctx.ResourceRec.NumHWAVideoDecInUse--;
    }
    else if ( (pBlock->pBP->BlockType == NvMMBlockType_EncAMRNB) ||
              (pBlock->pBP->BlockType == NvMMBlockType_EncAMRWB) ||
              (pBlock->pBP->BlockType == NvMMBlockType_EncAAC)   ||
              (pBlock->pBP->BlockType == NvMMBlockType_EncMP3)   ||
              (pBlock->pBP->BlockType == NvMMBlockType_EncSBC)   ||
              (pBlock->pBP->BlockType == NvMMBlockType_EnciLBC))
    {
        gs_Ctx.ResourceRec.NumAudioEncodersInUse--;
    }
    else if ( (pBlock->pBP->BlockType == NvMMBlockType_EncJPEG) )
    {
        gs_Ctx.ResourceRec.NumImageEncodersInUse--;
    }

#if !NVOS_IS_LINUX
    if((bForceClose == NV_TRUE) && (pBlock->pBP->Locale == NvMMLocale_AVP)&&
       (pBlock->hActualBlock != NULL))
    {
#if !NV_DEF_ENVIRONMENT_SUPPORTS_SIM
        NvmmManagerAbnormalTermMsg AbnormalTermMsg;
        NvmmMgr_BP *ppBlock;
        NvBool done = NV_FALSE;
        AbnormalTermMsg.eMsgType = NvmmManagerMsgType_AbnormalTerm;
        AbnormalTermMsg.hBlock = pBlock->hActualBlock;
        NvmmManagerSendMessage(&AbnormalTermMsg, sizeof(NvmmManagerAbnormalTermMsg));

        if(pBlock->hServiceRmLoader)
        {
            NvRmFreeLibrary(pBlock->hServiceRmLoader);

            ListReset(&gs_Ctx.BlockList);
            while (!done)
            {
                ListGetNextNode(&gs_Ctx.BlockList, (void *)&ppBlock);
                if (!ppBlock)
                {
                    done = NV_TRUE;
                    break;
                }
                else
                {
                    if(ppBlock->hServiceAvp == pBlock->hServiceAvp)
                    {
                        ppBlock->hServiceAvp = NULL;
                        ppBlock->hServiceRmLoader = NULL;
                    }
                }
            }
        }

        NvRmFreeLibrary(pBlock->hBlockRmLoader);
#endif
    }

#else
    if (pBlock->pBP->Locale == NvMMLocale_AVP)
    {
        NvmmManagerFreeHwBlock(pBlock);
    }
#endif

    if (pBlock->pBP->hMemHandle)
    {
        NvmmMgr_IramMemFree(pBlock->pBP->hMemHandle);
    }

    NvOsFree(pBlock->pBP);
    DecrementBlockRefCount(pBlock);
    NvOsMutexUnlock(hNvmmMgr->Mutex);
    return NvSuccess;
}


NvError
NvmmManagerRegisterProcessClient(
    NvmmManagerHandle hNvmmMgr,
    NvOsSemaphoreHandle hEventSemaphore,
    NvmmPowerClientHandle *pClient )
{
    NvError err;
    NvOsSemaphoreHandle hSema = NULL;

    if (hNvmmMgr != &gs_Ctx)
    {
        NV_ASSERT(0);
        return NvError_BadParameter;
    }

    NvOsMutexLock(hNvmmMgr->Mutex);

    if (hEventSemaphore != NULL)
    {
        err = NvOsSemaphoreClone(hEventSemaphore, &hSema);
        if (err != NvSuccess)
        {
            NvOsMutexUnlock(hNvmmMgr->Mutex);
            return err;
        }
    }
    err = ListAddNode(&gs_Ctx.NvmmClientCallback, hSema);
    if (err != NvSuccess)
    {
        NvOsMutexUnlock(hNvmmMgr->Mutex);
        return err;
    }
    *pClient = (NvmmPowerClientHandle)hSema;

     NvOsMutexUnlock(hNvmmMgr->Mutex);
    return err;
}

NvError NvmmManagerUnregisterProcessClient(
    NvmmPowerClientHandle pClient )
{
    NvError err;
    NvmmManagerHandle hNvmmMgr = &gs_Ctx;

    NvOsMutexLock(hNvmmMgr->Mutex);
    err = ListDeleteNode(&gs_Ctx.NvmmClientCallback, pClient);
    if (err != NvSuccess)
    {
        NvOsMutexUnlock(hNvmmMgr->Mutex);
        return err;
    }
    NvOsSemaphoreDestroy((NvOsSemaphoreHandle)pClient);

    NvOsMutexUnlock(hNvmmMgr->Mutex);
    return err;
}

NvError
NvmmManagerChangePowerState(
    NvmmManagerPowerState NewPowerState)
{
    switch (NewPowerState)
    {
    case NvmmManagerPowerState_FullOn:
    case NvmmManagerPowerState_Off:
        {
            NvOsSemaphoreHandle hEventSemaphore = NULL;
            NvBool done = NV_FALSE;

            gs_Ctx.CurrentPowerState = NewPowerState;

            ListReset(&gs_Ctx.NvmmClientCallback);
            while (!done)
            {
                ListGetNextNode(&gs_Ctx.NvmmClientCallback, (void *)&hEventSemaphore);
                if (!hEventSemaphore)
                {
                    done = NV_TRUE;
                }
                else
                {
                    NvOsSemaphoreSignal(hEventSemaphore);
                    NvOsSemaphoreWait(gs_Ctx.hSyncSema);
                }
            }
        }
        break;
    default:
        return NvError_NotSupported;
    }
    return NvSuccess;
}

NvError
NvmmManagerGetPowerState(
    NvmmManagerHandle hNvmmMgr,
    NvU32 *pPowerState)
{
    *pPowerState = (NvU32)gs_Ctx.CurrentPowerState;
    return NvSuccess;
}

NvError
NvmmManagerNotify(
    NvmmManagerHandle hNvmmMgr)
{
    NvOsSemaphoreSignal(gs_Ctx.hSyncSema);
    return NvSuccess;
}

void
GetDefaultBPFromBlockType(
    NvU32 BlockType,
    NvBlockProfile **ppBP)
{
    NvU8 i;

    *ppBP = NULL;
    for(i=0;i<NumResourceProfiles;i++)
    {
        if (BlockType == BlockProfileTable[i].BlockType  &&
            NvBlockFlag_Default & BlockProfileTable[i].BlockFlags )
        {
            *ppBP = &BlockProfileTable[i];
            return;
        }
    }

    //No profile found assign default
    for(i=0;i<NumResourceProfiles;i++)
    {
        if (NvMMBlockType_UnKnown == BlockProfileTable[i].BlockType  &&
            NvBlockFlag_Default & BlockProfileTable[i].BlockFlags )
        {
            *ppBP = &BlockProfileTable[i];
            return;
        }
    }

    return;
}

void
GetBlockFromBlockHandle(
    void* pBlockHandle,
    NvmmMgr_BP **ppBlock)
{
    *ppBlock = ListFindNode(&gs_Ctx.BlockList, pBlockHandle);
    return;
}


void CreateNewBlockRec(NvmmMgr_BP **ppBlock)
{
    *ppBlock = NULL;
    *ppBlock = (NvmmMgr_BP*)NvOsAlloc(sizeof(NvmmMgr_BP));
    if (NULL == *ppBlock) return;

    ListAddNode(&gs_Ctx.BlockList,(void*)*ppBlock);

    (*ppBlock)->RefCount = 1;
}

void
DecrementBlockRefCount(NvmmMgr_BP *pBlock)
{
    pBlock->RefCount--;
    if (0 == pBlock->RefCount)
    {
        ListDeleteNode(&gs_Ctx.BlockList, pBlock->pBlockHandle);
        NvOsFree(pBlock);
    }
}

NvU32 NvmmManagerGetBlockLocale(NvU32 eBlockType)
{
    NvU32 eLocale;

#if NVCPU_IS_X86 && NVOS_IS_WINDOWS
    eLocale = NvMMLocale_CPU;
    return eLocale;
#else

    switch(eBlockType)
    {
        case NvMMBlockType_DecMPEG2VideoVld:
        #if (NV_IS_DECMPEG2VIDEOVLD_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECMPEG2VIDEOVLD_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecAAC:
        #if (NV_IS_DECAAC_LOCALE == NV_BLOCK_LOCALE_ALL)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECAAC_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECAAC_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecADTS:
        #if (NV_IS_DECADTS_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECADTS_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;
        case NvMMBlockType_DecEAACPLUS:
        #if (NV_IS_DECAACPLUS_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECAACPLUS_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecMPEG4:
        #if (NV_IS_DECMPEG4_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECMPEG4_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #elif (NV_IS_DECMPEG4_LOCALE == NV_BLOCK_LOCALE_ALL)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_EncAMRNB:
        #if (NV_IS_ENCAMRNB_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_ENCAMRNB_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_EnciLBC:
        #if (NV_IS_ENCILBC_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_ENCILBC_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecAMRNB:
        #if (NV_IS_DECAMRNB_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECAMRNB_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecMP2:
        #if (NV_IS_DECMP2_LOCALE == NV_BLOCK_LOCALE_ALL)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECMP2_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECMP2_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecMP3:
        #if (NV_IS_DECMP3_LOCALE == NV_BLOCK_LOCALE_ALL)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECMP3_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECMP3_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecMP3SW:
        #if (NV_IS_DECMP3SW_LOCALE == NV_BLOCK_LOCALE_ALL)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECMP3SW_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECMP3SW_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecJPEG:
        #if (NV_IS_DECJPEG_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECJPEG_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecVC1:
        #if (NV_IS_DECVC1_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECVC1_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecVC1_2x:
        #if (NV_IS_DECVC1_2X_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECVC1_2X_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecH264:
        #if (NV_IS_DECH264_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECH264_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #elif (NV_IS_DECH264_LOCALE == NV_BLOCK_LOCALE_ALL)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecH264_2x:
        #if (NV_IS_DECH264_2X_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECH264_2X_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecSuperJPEG:
            eLocale = NvMMLocale_CPU;
            break;
        case NvMMBlockType_DecJPEGProgressive:
            eLocale = NvMMLocale_CPU;
            break;

        case NvMMBlockType_DecMPEG2:
        #if (NV_IS_DECMPEG2_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECMPEG2_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #elif (NV_IS_DECMPEG2_LOCALE == NV_BLOCK_LOCALE_ALL)
            eLocale = NvMMLocale_AVP;
        #endif
            break;

        case NvMMBlockType_DecH264AVP:
            eLocale = NvMMLocale_AVP;
            break;

        case NvMMBlockType_DecMPEG4AVP:
            eLocale = NvMMLocale_AVP;
            break;

        case NvMMBlockType_EncJPEG:
        #if (NV_IS_ENCJPEG_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_ENCJPEG_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_EncAMRWB:
        #if (NV_IS_ENCAMRWB_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_ENCAMRWB_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecAMRWB:
        #if (NV_IS_DECAMRWB_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECAMRWB_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecILBC:
        #if (NV_IS_DECILBC_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECILBC_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecWAV:
        #if (NV_IS_DECWAV_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECWAV_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_EncWAV:
        #if (NV_IS_ENCWAV_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_ENCWAV_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_AudioMixer:
        #if (NV_IS_AUDIOMIXER_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_AUDIOMIXER_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_SinkAudio:
        case NvMMBlockType_SourceAudio:
            eLocale = NvMMLocale_CPU;
            break;

        case NvMMBlockType_SrcUSBCamera:
        #if (NV_IS_SRCUSBCAMERA_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_SRCUSBCAMERA_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_SrcCamera:
        #if (NV_IS_SRCCAMERA_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_SRCCAMERA_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_RingTone:
        #if (NV_IS_RINGTONE_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_RINGTONE_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DigitalZoom:
        #if (NV_IS_DIGITALZOOM_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DIGITALZOOM_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_EncAAC:
        #if (NV_IS_ENCAAC_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_ENCAAC_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecWMA:
        #if (NV_IS_DECWMA_LOCALE == NV_BLOCK_LOCALE_ALL)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECWMA_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECWMA_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecWMAPRO:
        #if (NV_IS_DECWMAPRO_LOCALE == NV_BLOCK_LOCALE_ALL)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECWMAPRO_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECWMAPRO_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;
        case NvMMBlockType_DecWMALSL:
        #if (NV_IS_DECWMALSL_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECWMALSL_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;
        case NvMMBlockType_DecWMASUPER:
        #if (NV_IS_DECWMASUPER_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECWMASUPER_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;
        case NvMMBlockType_DecSUPERAUDIO:
        #if (NV_IS_DECSUPERAUDIO_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECSUPERAUDIO_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;
        case NvMMBlockType_DecOGG:
        #if (NV_IS_DECOGG_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECOGG_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecBSAC:
        #if (NV_IS_DECBSAC_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECBSAC_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_SuperParser:
        #if (NV_IS_SUPERPARSER_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_SUPERPARSER_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_3gppWriter:
        #if (NV_IS_3GPWRITER_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_3GPWRITER_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        case NvMMBlockType_DecMPEG2VideoRecon:
        #if (NV_IS_DECMPEG2VIDEORECON_LOCALE == NV_BLOCK_LOCALE_AVP)
            eLocale = NvMMLocale_AVP;
        #elif (NV_IS_DECMPEG2VIDEORECON_LOCALE == NV_BLOCK_LOCALE_CPU)
            eLocale = NvMMLocale_CPU;
        #endif
            break;

        default:
            eLocale = NvMMLocale_Force32;
            break;
    }
    return eLocale;
#endif
}


NvError NvmmMgr_IramMemAlloc(
    NvMMIRAMScratchType Type,
    NvRmMemHandle *phMemHandle,
    NvU32 Size,
    NvU32 Alignment,
    NvU32 *pPhysicalAddress)
{
    NvError Status = NvSuccess;
    NvRmMemHandle hMemHandle;

    static const NvRmHeap heaps[] =
    {
        NvRmHeap_IRam
    };

#if !NVOS_IS_LINUX
    Status = NvRmMemHandleCreate(gs_Ctx.hRmDevice, &hMemHandle, Size);
    if (Status != NvSuccess)
    {
        return Status;
    }

    Status = NvRmMemAlloc(hMemHandle, heaps, NV_ARRAY_SIZE(heaps),
                         Alignment, NvOsMemAttribute_Uncached);
    if (Status != NvSuccess)
    {
        NvRmMemHandleFree(hMemHandle);
        return Status;
    }

    *pPhysicalAddress = NvRmMemPin(hMemHandle);
    *phMemHandle = hMemHandle;
#else
    if (Type) {
        union tegra_mediaserver_alloc_info info;

        info.in.tegra_mediaserver_resource_type = TEGRA_MEDIASERVER_RESOURCE_IRAM;
        info.in.u.iram.tegra_mediaserver_iram_type =  TEGRA_MEDIASERVER_IRAM_SHARED;
        info.in.u.iram.alignment = Alignment;
        info.in.u.iram.size = Size;

        if (ioctl(gs_Ctx.FileDescriptor, TEGRA_MEDIASERVER_IOCTL_ALLOC, &info) < 0)
        {
            return NvError_InsufficientMemory;
        }

        *phMemHandle = (NvRmMemHandle)info.out.u.iram.rm_handle;
        *pPhysicalAddress = info.out.u.iram.physical_address;
    }

    else
    {
        Status = NvRmMemHandleCreate(gs_Ctx.hRmDevice, &hMemHandle, Size);
        if (Status != NvSuccess)
        {
            return Status;
        }

        Status = NvRmMemAlloc(hMemHandle, heaps, NV_ARRAY_SIZE(heaps),
                             Alignment, NvOsMemAttribute_Uncached);
        if (Status != NvSuccess)
        {
            NvRmMemHandleFree(hMemHandle);
            return Status;
        }

        *pPhysicalAddress = NvRmMemPin(hMemHandle);
        *phMemHandle = hMemHandle;
    }
#endif

    return Status;
}

void NvmmMgr_IramMemFree(NvRmMemHandle hMemHandle)
{
    NvRmMemUnpin(hMemHandle);
    NvRmMemHandleFree(hMemHandle);
}

