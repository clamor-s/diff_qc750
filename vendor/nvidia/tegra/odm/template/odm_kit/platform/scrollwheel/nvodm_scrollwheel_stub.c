/*
 * Copyright (c) 2006 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvodm_scrollwheel.h"


NvOdmScrollWheelHandle
NvOdmScrollWheelOpen(
    NvOdmOsSemaphoreHandle hNotifySema,
    NvOdmScrollWheelEvent RegisterEvents)
{
    return NULL;
}

void NvOdmScrollWheelClose(NvOdmScrollWheelHandle hOdmScrollWheel)
{
}

NvOdmScrollWheelEvent NvOdmScrollWheelGetEvent(NvOdmScrollWheelHandle hOdmScrollWheel)
{
    return NvOdmScrollWheelEvent_None;
}

