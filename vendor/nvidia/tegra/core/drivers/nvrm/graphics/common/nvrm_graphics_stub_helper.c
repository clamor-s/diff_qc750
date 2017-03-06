/*
 * Copyright (c) 2008 - 2009 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvos.h"
#include "nvassert.h"
#include "nvrm_init.h"
#include "nvrm_ioctls.h"
#include "nvidlcmd.h"
#include "nvreftrack.h"


/* This includes the functions needed for the stub function, that return the
 * name of kernel device, and the magic IOCTL to use to communicate with that
 * device
 */

//###########################################################################
//############################### WIN32 #####################################
//###########################################################################

#if NVOS_IS_WINDOWS

#include <windows.h>

//===========================================================================
// DllMain()
//===========================================================================
BOOL WINAPI DllMain(HANDLE hinstDLL, 
                    DWORD dwReason, 
                    LPVOID lpvReserved)
{
    switch (dwReason) {
    case DLL_PROCESS_ATTACH: 
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH: 
    case DLL_THREAD_DETACH:
    case DLL_THREAD_ATTACH:
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

NvOsFileHandle NvRmGraphics_NvIdlGetIoctlFile( void )
{
    // nvrm_kernel and nvrm_graphics are packaged into the same driver,
    // just use the nvrm_kernel filehandle. Note that since we depend
    // on nvrm_kernel is is guaranteed that the file handle is not
    // closed before we are unloaded - therefore no need to refcount
    // the open file context

    return NvRm_NvIdlGetIoctlFile();
}

NvU32 NvRmGraphics_NvIdlGetIoctlCode( void )
{
    return NvRmIoctls_NvRmGraphics;  
}

// We need this here because the .export file exports this function.
NvError NvRmGraphics_Dispatch(
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
NvError NvRmGraphics_Dispatch(void *in, void *out, NvDispatchCtx* Ctx)
{
    return NvError_NotSupported;
}

#endif

//###########################################################################
//############################### COMMON ####################################
//###########################################################################

void NvRmGraphicsClose(NvRmDeviceHandle rm);
NvError NvRmGraphicsOpen(NvRmDeviceHandle rm);
NvError NvRmPowerSuspend(NvRmDeviceHandle rm);
NvError NvRmPowerResume(NvRmDeviceHandle rm);

void NvRmGraphicsClose(NvRmDeviceHandle rm)
{
    return;
}

NvError  NvRmGraphicsOpen(NvRmDeviceHandle rm)
{
    return 0;
}

NvError NvRmPowerSuspend(NvRmDeviceHandle rm)
{
    return 0;
}

NvError NvRmPowerResume(NvRmDeviceHandle rm)
{
    return 0;
}
