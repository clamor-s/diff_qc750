/*
 * Copyright (c) 2011-2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvavp.h"

NvError NvAvpAudioOpen(NvAvpHandle *phAvp)
{
    return NvSuccess;
}

NvError NvAvpAudioEnableClocks(
    NvAvpHandle hAvp,
    NvU32 ModuleId)

{
    return NvSuccess;
}

NvError NvAvpAudioDisableClocks(
    NvAvpHandle hAvp,
    NvU32 ModuleId)

{
    return NvSuccess;
}

NvError NvAvpOpen(NvAvpHandle *phAvp)
{
    return NvSuccess;
}

NvError NvAvpClose(NvAvpHandle hAvp)
{
    return NvSuccess;
}

NvError NvAvpSubmitBuffer(
    NvAvpHandle hAvp,
    const NvAvpCommandBuffer *pCommandBuf,
    const NvAvpRelocationEntry *pRelocations,
    NvU32 NumRelocations,
    NvRmFence *pPbFence
)
{
    return NvSuccess;
}

NvError NvAvpSubmitBufferNew(
    NvAvpHandle hAvp,
    const NvAvpCommandBuffer *pCommandBuf,
    const NvAvpRelocationEntry *pRelocations,
    NvU32 NumRelocations,
    NvRmFence *pPbFence,
    NvU32 flags
)
{
    return NvSuccess;
}

NvError NvAvpGetSyncPointID(
    NvAvpHandle hAvp,
    NvU32 *pSyncPointID)
{
    return NvSuccess;
}

NvError NvAvpGetClockRate(
    NvAvpHandle hAvp,
    NvU32 ModuleId,
    NvU32 *pRate)
{
    return NvSuccess;
}

NvError NvAvpSetClockRate(
    NvAvpHandle hAvp,
    NvU32 ModuleId,
    NvU32 Rate,
    NvU32 *pFreqSet)
{
    return NvSuccess;
}

NvError NvAvpWakeAvp(
    NvAvpHandle hAvp)
{
    return NvSuccess;
}

NvError NvAvpForceClockStayOn(
    NvAvpHandle hAvp,
    NvU32 flag)
{
    return NvSuccess;
}
