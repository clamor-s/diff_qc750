/*
 * Copyright (c) 2007 - 2009 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvassert.h"
#include "nvos.h"
#include "nvmm_kernel_ioctls.h"
#include "nvidlcmd.h"
#include "nvreftrack.h"

/* This includes the functions needed for the stub function, that return the
 * name of kernel device, and the magic IOCTL to use to communicate with that
 * device.
 */

//###########################################################################
//############################### WIN32 #####################################
//###########################################################################
#if NVOS_IS_WINDOWS

#include <windows.h>

static NvOsFileHandle s_NvMMFile = NULL;

BOOL WINAPI DllMain(HANDLE hinstDLL, 
                    DWORD dwReason, 
                    LPVOID lpvReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH: 
        if (NvOsFopen("NMM1:", NVOS_OPEN_READ | NVOS_OPEN_WRITE, &s_NvMMFile) != NvSuccess)
        {
            return FALSE;
        }
        break;

    case DLL_PROCESS_DETACH: 
        NvOsFclose(s_NvMMFile);
        s_NvMMFile = NULL;
        break;

    case DLL_THREAD_DETACH:
    case DLL_THREAD_ATTACH:
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

NvOsFileHandle NvMM_NvIdlGetIoctlFile( void )
{
    return s_NvMMFile;
}

NvU32 NvMM_NvIdlGetIoctlCode( void )
{
    return NvMMKernelIoctls_Generic;  
}

// We need this here because the .export file exports this function.
NvError NvMM_Dispatch(
                void *InBuffer,
                NvU32 InSize,
                void *OutBuffer,
                NvU32 OutSize,
                NvDispatchCtx* Ctx)
{
    return NvError_NotSupported;
}

//###########################################################################
//############################### LINUX #####################################
//###########################################################################
#elif NVOS_IS_LINUX
// We need this here because the .export file exports this function.
NvError NvMM_Dispatch(void *in, void *out, NvDispatchCtx* Ctx)
{
    return NvError_NotSupported;
}

#else
#error unsupported os
#endif

