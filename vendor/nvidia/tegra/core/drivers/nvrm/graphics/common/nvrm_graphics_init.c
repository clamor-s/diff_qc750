/*
 * Copyright (c) 2007 - 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef NVRM_TRANSPORT_IN_KERNEL
#define NVRM_TRANSPORT_IN_KERNEL 0
#endif

#include "nvcommon.h"
#include "nvos.h"
#include "nvutil.h"
#include "nvassert.h"
#include "nvrm_init.h"
#include "nvrm_xpc.h"
#include "ap20/ap20rm_channel.h"
#include "ap20/ap20rm_hwcontext.h"
#include "nvrm_graphics_private.h"

/* No locking done. Should be take care off by the caller */

NvError NvRmGraphicsOpen(NvRmDeviceHandle rm)
{
    NvError err = NvSuccess;

    if (!(NVCPU_IS_X86 && NVOS_IS_WINDOWS))
    {
        err = NvRmPrivChannelInit(rm);
        if (NV_SHOW_ERROR(err))
            return err;
        NvRmPrivContextReadHandlerInit(rm);
        NvRmPrivHostInit(rm);

#if (NVRM_TRANSPORT_IN_KERNEL == 0)
        err = NvRmXpcInitArbSemaSystem(rm);
        if (NV_SHOW_ERROR(err))
            return err;
#endif
    }
    return err;
}

void
NvRmGraphicsClose(NvRmDeviceHandle handle)
{
    if (!handle)
        return;

    /* shutdown the channels */
    NvRmPrivChannelDeinit(handle);

    /* Host shutdown */
    NvRmPrivHostShutdown(handle);

    /* shut down context reading thread */
    NvRmPrivContextReadHandlerDeinit(handle);

    /* TODO: [ahatala 2010-06-01] */
    // NvRmPrivDeInitAvp( handle );
}
