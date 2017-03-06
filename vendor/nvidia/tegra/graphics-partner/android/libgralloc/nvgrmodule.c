/*
 * Copyright (c) 2009-2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvgralloc.h"
#include "nvgr_scratch.h"
#include "nvassert.h"
#include <errno.h>

static int
NvGrDevOpen(
    const hw_module_t* module,
    const char* name,
    hw_device_t** device)
{
    NvGrModule* m = (NvGrModule*)module;
    int idx = 0;

    if (NvOsStrcmp(name, GRALLOC_HARDWARE_GPU0) == 0)
        idx = NvGrDevIdx_Alloc;
    else if (NvOsStrcmp(name, GRALLOC_HARDWARE_FB0) == 0)
        idx = NvGrDevIdx_Fb0;
    else
        return -EINVAL;

    pthread_mutex_lock(&m->Lock);

    if (m->DevRef[idx] == 0)
    {
        int ret;

        switch (idx) {
        case NvGrDevIdx_Alloc:
            ret = NvGrAllocDevOpen(m, &m->Dev[idx]);
            break;
        case NvGrDevIdx_Fb0:
            *device = NULL;
            ret = 0;
            break;
        default:
            ret = -EINVAL;
            break;
        }

        if (ret)
        {
            pthread_mutex_unlock(&m->Lock);
            return ret;
        }
    }

    m->DevRef[idx]++;
    pthread_mutex_unlock(&m->Lock);
    *device = m->Dev[idx];
    return 0;
}

NvBool
NvGrDevUnref(NvGrModule* m, int idx)
{
    NvBool ret = NV_FALSE;

    pthread_mutex_lock(&m->Lock);
    if (--m->DevRef[idx] == 0)
    {
        ret = NV_TRUE;
        m->Dev[idx] = NULL;
    }
    pthread_mutex_unlock(&m->Lock);

    return ret;
}


NvError
NvGrModuleRef(NvGrModule* m)
{
    pthread_mutex_lock(&m->Lock);

    if (m->RefCount == 0) {
        NvError err;
        int ret;

        err = NvRmOpen(&m->Rm, 0);

        if (err != NvSuccess) {
            goto exit;
        }

        ret = NvGr2dInit(m);
        if (ret != 0) {
            NvRmClose(m->Rm);
            goto exit;
        }

        ret = NvGrScratchInit(m);
        if (ret != 0) {
            NvGr2dDeInit(m);
            NvRmClose(m->Rm);
            goto exit;
        }
    }

    m->RefCount++;

exit:
    pthread_mutex_unlock(&m->Lock);

    return NvSuccess;
}

void
NvGrModuleUnref(NvGrModule* m)
{
    pthread_mutex_lock(&m->Lock);

    if (--m->RefCount == 0)
    {
        NvGrScratchDeInit(m);
        NvGr2dDeInit(m);
        NvRmClose(m->Rm);
        m->Rm = NULL;
    }

    pthread_mutex_unlock(&m->Lock);
}

//
// Globals for the HW module interface
//

static hw_module_methods_t NvGrMethods =
{
    .open = NvGrDevOpen
};

NvGrModule HAL_MODULE_INFO_SYM =
{
    .Base =
    {
        .common =
        {
            .tag = HARDWARE_MODULE_TAG,
            .version_major = 1,
            .version_minor = 0,
            .id = GRALLOC_HARDWARE_MODULE_ID,
            .name = "NVIDIA Tegra Graphics Memory Allocator",
            .author = "NVIDIA",
            .methods = &NvGrMethods
        },
        .registerBuffer = NvGrRegisterBuffer,
        .unregisterBuffer = NvGrUnregisterBuffer,
        .lock = NvGrLock,
        .unlock = NvGrUnlock
    },

    .addfence = NvGrAddFence,
    .getfences = NvGrGetFences,
    .set_stereo_info = NvGrSetStereoInfo,
    .set_source_crop = NvGrSetSourceCrop,
#if NVGR_DEBUG_LOCKS
    .clear_lock_usage = NvGrClearLockUsage,
#endif
    .scratch_open = NvGrScratchOpen,
    .scratch_close = NvGrScratchClose,
    .alloc_scratch = NvGrAllocInternal,
    .free_scratch = NvGrFreeInternal,
    .copy_buffer = NvGr2dCopyBuffer,
    .copy_edges = NvGr2dCopyEdges,
    .blit = NvGr2dBlit,
    .clear = NvGr2DClear,
    .composite = NvGr2dComposite,
    .module_ref = NvGrModuleRef,
    .module_unref = NvGrModuleUnref,
    .dump_buffer = NvGrDumpBuffer,
    .Lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER,
    .RefCount = 0,
    .Rm = NULL,
};
