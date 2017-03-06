/*
 * Copyright (c) 2007 - 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/* 
 * nvmm/manager/nvmm_powermanager.c
 */

#include "nvos.h"
#include "nvmm_debug.h"
#include "nvassert.h"
#include "nvmm_contextmanager.h"
#include "nvmm_manager.h"

#define MAX_BLOCKS_PER_PROCESS 64

typedef struct NvmmContextManagerRec
{
    NvOsSemaphoreHandle hPMSema;
    NvOsSemaphoreHandle hSyncSema;
    NvmmManagerHandle   hNvmmMgr;
    NvOsMutexHandle hMutex;
    NvOsThreadHandle hThread;
    NvMMBlockHandle BlockList[MAX_BLOCKS_PER_PROCESS];
    NvU32 BlockListIndex;
    NvU32 RefCount;
    NvmmPowerClientHandle hPowerClient;
    NvU32 PowerState;
    NvBool bShutDown;
    NvBool bInitialized;
    NvU32 bWait;
    NvBool bCloseInProcess;
} NvmmContextManager;

static NvmmContextManager gs_NvmmPM;
static NvError NvmmContextManagerChangeState(NvmmContextManager *pNvmmPM);
static void NvmmContextManagerCloseInternal(NvBool unregisterProcessClient);

void NvmmContextManagerThread(void *arg);
extern NvMMBlockType NvMMGetBlockType(NvMMBlockHandle hBlock);

NvError NvmmContextManagerOpen(NvmmManagerHandle hNvmmMgr)
{
    NvError e;
    NvmmContextManager *pNvmmPM = 0;
    NvOsMutexHandle nvmmMutex = NULL;
    NvU32 TempRefCount;

    if(gs_NvmmPM.bShutDown == NV_TRUE)
    {
        while(gs_NvmmPM.bCloseInProcess == NV_TRUE)
        {
            /* Use delay instead of yield incase other thread is lower priority */
            NV_DEBUG_PRINTF(("NvmmContextManagerOpen : Delaying the Caller -\r\n"));
            NvOsSleepMS(1);
        }
    }

    if (gs_NvmmPM.hMutex == NULL)
    {
        e = NvOsMutexCreate(&nvmmMutex);
        if (e != NvSuccess)
            return e;

        if (NvOsAtomicCompareExchange32((NvS32*)&gs_NvmmPM.hMutex, 0, (NvS32)nvmmMutex) != 0)
            NvOsMutexDestroy(nvmmMutex);
    }
    NvOsMutexLock(gs_NvmmPM.hMutex);

    pNvmmPM = &gs_NvmmPM;

    if (pNvmmPM->bInitialized)
    {
        if (pNvmmPM->RefCount)
        {
            pNvmmPM->RefCount++;
            NvOsMutexUnlock(gs_NvmmPM.hMutex);
            return NvSuccess;
        }
    }

    nvmmMutex = gs_NvmmPM.hMutex;
    TempRefCount = pNvmmPM->RefCount;
    NvOsMemset(pNvmmPM, 0, sizeof(NvmmContextManager));
    gs_NvmmPM.hMutex = nvmmMutex;
    gs_NvmmPM.RefCount = TempRefCount;

    NV_CHECK_ERROR_CLEANUP(
        NvOsSemaphoreCreate(&(pNvmmPM->hPMSema), 0)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvOsSemaphoreCreate(&(pNvmmPM->hSyncSema), 0)
    );

    NvOsMemset(pNvmmPM->BlockList, 0, sizeof(NvU32) * MAX_BLOCKS_PER_PROCESS);

    pNvmmPM->hNvmmMgr = hNvmmMgr;

    NV_CHECK_ERROR_CLEANUP(
        NvmmManagerRegisterProcessClient(pNvmmPM->hNvmmMgr, 
                                         pNvmmPM->hPMSema, 
                                         &pNvmmPM->hPowerClient)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvOsThreadCreate(NvmmContextManagerThread, 
                         (void *)pNvmmPM, 
                         &(pNvmmPM->hThread))
    );

   /* This will make NVMM context manager persistent. Resources will be be cleaned
      * during  process clean-up. Since NvMM context manager open/close are called
      * form multiple threads, it is difficult to synchronize them. So we keep it persistant
      * and when process terminates all resources will be cleaned.
       */
    pNvmmPM->RefCount += 2;
    pNvmmPM->bInitialized = NV_TRUE;
    pNvmmPM->bCloseInProcess = NV_FALSE;
    pNvmmPM->bWait = 0;

    NV_DEBUG_PRINTF(("Out NvmmContextManagerOpen %x\n", pNvmmPM));
    NvOsMutexUnlock(pNvmmPM->hMutex);
    return NvSuccess;

fail: 
    NvOsMutexUnlock(pNvmmPM->hMutex);
    NvOsSemaphoreDestroy(pNvmmPM->hPMSema);
    NvOsSemaphoreDestroy(pNvmmPM->hSyncSema);
    return e;
}

static void NvmmContextManagerCloseInternal(NvBool unregisterProcessClient)
{
    NvmmContextManager *pNvmmPM = &gs_NvmmPM;

    pNvmmPM->bCloseInProcess = NV_TRUE;

    if (pNvmmPM->hMutex )
        NvOsMutexLock( pNvmmPM->hMutex );
    pNvmmPM->RefCount--;

    /* do deinit if refcount is zero */
    if( pNvmmPM->RefCount == 0 && pNvmmPM->bInitialized)
    {
        NV_DEBUG_PRINTF(("Closing NvmmContextManager ...."));
        pNvmmPM->bShutDown = NV_TRUE;
        NvOsSemaphoreSignal(pNvmmPM->hPMSema);
        NvOsThreadJoin(pNvmmPM->hThread);

        if (unregisterProcessClient == NV_TRUE)
        {
            NvmmManagerUnregisterProcessClient(pNvmmPM->hPowerClient);
        }

        NvOsSemaphoreDestroy(pNvmmPM->hPMSema);
        NvOsSemaphoreDestroy(pNvmmPM->hSyncSema);

        if (pNvmmPM->hMutex )
        {
            NvOsMutexUnlock( pNvmmPM->hMutex );
            NvOsMutexDestroy(pNvmmPM->hMutex);
        }
        /* This takes care of bCloseInProcess = NV_FALSE flag */
        NvOsMemset(pNvmmPM, 0, sizeof(NvmmContextManager));
        NV_DEBUG_PRINTF(("...done\n"));
        return;
    }

    if (pNvmmPM->hMutex )
    {
        NvOsMutexUnlock( pNvmmPM->hMutex );
        // If the service is not initialized, just kill the hMutex that was created in Open
        if (!pNvmmPM->bInitialized && pNvmmPM->RefCount == 0)
        {
            NvOsMutexDestroy(pNvmmPM->hMutex);
            pNvmmPM->hMutex = 0;
        }
    }
}

void NvmmContextManagerClose(void)
{
    NvmmContextManagerCloseInternal(NV_TRUE);
}

NvError NvmmContextManagerRegisterBlock(NvMMBlockHandle hBlock)
{
    NvU32 i;
    NvmmContextManager *pNvmmPM = &gs_NvmmPM;

    if (!pNvmmPM->bInitialized)
    {
        return NvError_NotInitialized;
    }

    NvOsMutexLock( pNvmmPM->hMutex );
    for (i = 0; i < MAX_BLOCKS_PER_PROCESS; i++)
    {
        if (pNvmmPM->BlockList[i] == NULL)
        {
            break;
        }
    }
    if (i >= MAX_BLOCKS_PER_PROCESS)
    {
        NvOsMutexUnlock( pNvmmPM->hMutex );
        return NvError_InsufficientMemory;
    }

    pNvmmPM->BlockList[i] = hBlock;
    NvOsMutexUnlock( pNvmmPM->hMutex );
    return NvSuccess;
}

NvError NvmmContextManagerUnregisterBlock(NvMMBlockHandle hBlock)
{
    NvU32 i;
    NvmmContextManager *pNvmmPM = &gs_NvmmPM;
    
    if (!pNvmmPM->bInitialized)
    {
        return NvError_NotInitialized;
    }

    NvOsMutexLock( pNvmmPM->hMutex );
    for (i = 0; i < MAX_BLOCKS_PER_PROCESS; i++)
    {
        if (pNvmmPM->BlockList[i] == hBlock)
        {
            pNvmmPM->BlockList[i] = NULL;
        }
    }

    NvOsMutexUnlock( pNvmmPM->hMutex );
    return NvSuccess;
}

NvBool NvmmContextManagerNotify()
{
    NvmmContextManager *pNvmmPM = &gs_NvmmPM;
    if (pNvmmPM->bWait)
    {
        pNvmmPM->bWait--;
        NvOsSemaphoreSignal(pNvmmPM->hSyncSema);
        return NV_TRUE;
    }
    return NV_FALSE;
}

void NvmmContextManagerThread(void *arg)
{
    NvmmContextManager *pNvmmPM = (NvmmContextManager *)arg;

    while (!pNvmmPM->bShutDown)
    {
        NvOsSemaphoreWait(pNvmmPM->hPMSema);
        if (!pNvmmPM->bShutDown)
        {
            NvmmManagerGetPowerState(pNvmmPM->hNvmmMgr, &pNvmmPM->PowerState);
            NvmmContextManagerChangeState(pNvmmPM);
        }
    }
}

static NvError NvmmContextManagerChangeState(NvmmContextManager *pNvmmPM)
{
    switch (pNvmmPM->PowerState)
    {
    case NvmmManagerPowerState_Off:
    case NvmmManagerPowerState_FullOn:
    {
        NvU32 i;
        NvU32 numBlocks = 0;

        NvOsMutexLock( pNvmmPM->hMutex );
        for(i = 0; i < MAX_BLOCKS_PER_PROCESS; i++)
        {
            NvMMBlockHandle hBlock = pNvmmPM->BlockList[i];
            if (hBlock)
            {
                NvMMAttrib_PowerStateInfo PowerStateInfo;

                NvOsMemset(&PowerStateInfo, 0, sizeof(NvMMAttrib_PowerStateInfo));
                numBlocks++;

                PowerStateInfo.PowerState = pNvmmPM->PowerState;
                pNvmmPM->bWait++;
                hBlock->SetAttribute(hBlock, 
                                     NvMMAttribute_PowerState,
                                     NvMMSetAttrFlag_Notification,
                                     sizeof(NvMMAttrib_PowerStateInfo),
                                     &PowerStateInfo);
            }
        }
        for(i = 0; i < numBlocks; i++)
        {
            NvOsSemaphoreWait(pNvmmPM->hSyncSema);
        }

        NvmmManagerNotify( pNvmmPM->hNvmmMgr );
        NvOsMutexUnlock( pNvmmPM->hMutex );
    }
        break;
    default: 
        break;
    }
    return NvSuccess;
}

/* When dlclose() is called, free up the extra refcount */
#ifdef __GNUC__
int myfini(void) __attribute__ ((destructor));
int myfini(void)
{
    /*
     * Shouldn't somebody else have already unregistered process client before
     * this shared library is unloaded? If this shared library registered
     * anything in its thread context, it ought to be unregistered before
     * library itself gets unloaded.
     */
    NvmmContextManagerCloseInternal(NV_FALSE);
    return 0;
}
#else
// TODO: Clean this up for other compilers.
// Maybe we can get an NvOs version of _fini?
#endif
