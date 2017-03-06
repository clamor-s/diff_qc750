/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvmm_block_avpentry.h"

NvOsThreadHandle AvpBlockThreadHandle;
BlockSideCreationParameters CreationalParams;

NvError AxfModuleMain(NvRmModuleLoaderReason Reason, void *Args, NvU32 ArgsSize)
{
    NvError Status = NvSuccess;
    switch(Reason)
    {
        case NvRmModuleLoaderReason_Attach:
        {
            NvOsMemset(&CreationalParams, 0, sizeof(BlockSideCreationParameters));
            NvOsMemcpy(&CreationalParams, Args, sizeof(BlockSideCreationParameters));
            Status = NvOsSemaphoreCreate(&CreationalParams.BlockEventSema, 0);
            NV_ASSERT(Status == NvSuccess);

            Status = NvOsSemaphoreCreate(&CreationalParams.ScheduleBlockWorkerThreadSema, 0);
            NV_ASSERT(Status == NvSuccess);

            Status = NvOsAVPThreadCreate(BlockSideWorkerThread,
                                      (void *)&CreationalParams,
                                      &CreationalParams.blockSideWorkerHandle,
                                      (void *)CreationalParams.StackPtr,
                                      CreationalParams.StackSize);

            AvpBlockThreadHandle = CreationalParams.blockSideWorkerHandle;
            NvOsSemaphoreWait(CreationalParams.ScheduleBlockWorkerThreadSema);
        }
        break;

        case NvRmModuleLoaderReason_Detach:
            NvOsThreadJoin(AvpBlockThreadHandle);
            if (CreationalParams.ScheduleBlockWorkerThreadSema)
                NvOsSemaphoreDestroy(CreationalParams.ScheduleBlockWorkerThreadSema);
        break;
    }
    return Status;
}

