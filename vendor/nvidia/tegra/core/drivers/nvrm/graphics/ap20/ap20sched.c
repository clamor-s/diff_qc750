/*
 * Copyright (c) 2007-2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "ap20/class_ids.h"
#include "ap20/arhost1x_uclass.h"

#include "nvrm_drf.h"
#include "nvrm_sched.h"
#include "nvrm_sched_private.h"
#include "nvassert.h"

NvData32 *NvSchedPushIncr(NvSchedClient *sc,
    NvData32 *pCurrent,
    NvU32 Cond,
    NvU32 *VirtualFence)
{
    NVRM_STREAM_PUSH_INCR(&sc->stream, pCurrent,
        sc->pt[0].SyncPointID, 0x0, Cond, NV_TRUE);

    if (VirtualFence)
        *VirtualFence = sc->pt[0].NextSyncPoint;

    sc->pt[0].NextSyncPoint++;

    return pCurrent;
}

NvData32 *NvSchedPushHostWaitLast(NvSchedClient *sc, NvData32 *pCurrent,
    NvBool opDoneFlag)
{
    const struct NvSchedVirtualSyncPointRec *ptRec = &sc->pt[0];

    NvU32 cond =  opDoneFlag ?
        NV_CLASS_HOST_INCR_SYNCPT_0_COND_OP_DONE :
        NV_CLASS_HOST_INCR_SYNCPT_0_COND_RD_DONE;

    /* Add SETCLASS here now until graphics has been converted to do it */
    NVRM_STREAM_PUSH_SETCLASS(&sc->stream, pCurrent,
        NvRmModuleID_3D, NV_GRAPHICS_3D_CLASS_ID);
    NVRM_STREAM_PUSH_WAIT_LAST(&sc->stream, pCurrent,
        ptRec->SyncPointID, (NvU32)sc->pClientPriv,
        0, cond);
    sc->pt[0].NextSyncPoint++;

    if (sc->pSyncptIncr)
        sc->pSyncptIncr(sc->SyncptIncrData);

    return pCurrent;
}

void NvSchedHostWaitLast(NvSchedClient *sc, NvSchedVirtualSyncPoint *points,
                         int count, NvBool opDoneFlag)
{
    NvData32 *pCurrent;

    NV_ASSERT(count == 1 && *points == 0);
    NVRM_STREAM_BEGIN(&sc->stream, pCurrent, 7);
    pCurrent = NvSchedPushHostWaitLast(sc, pCurrent, opDoneFlag);
    NVRM_STREAM_END(&sc->stream, pCurrent);
}

void NvSchedFlushAndCpuWait(NvSchedClient *sc)
{
    NvRmFence fence;
    NvRmStreamFlush(&sc->stream, &fence);
    NvRmFenceWait(sc->hDevice, &fence, NV_WAIT_INFINITE);
}

void NvSchedCpuWaitLast(NvSchedClient *sc,
                        NvSchedVirtualSyncPoint *points, int count)
{
    NV_ASSERT(count == 1 && *points == 0);
    NvSchedFlushAndCpuWait(sc);
}

NvError NvSchedClientPrivInit(NvRmDeviceHandle hDevice, NvRmChannelHandle hChannel,
                          NvRmModuleID ModuleID,
                          NvSchedClient *sc)
{
    NV_ASSERT(sc);//sc should not be null
    //only 3d module's waitbase ID is needed
    sc->pClientPriv = (void *)NvRmChannelGetModuleWaitBase( hChannel, NvRmModuleID_3D, 0 );
    return NvSuccess;
}

void NvSchedClientPrivClose(NvSchedClient *sc)
{
    return;
}
