/*
 * Copyright (c) 2007 NVIDIA Corporation.  All Rights Reserved.
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
#include "nvstormgr_ioctls.h"

static NvOsFileHandle s_NvStorMgrFile = NULL;

#if NVOS_IS_WINDOWS

#include <windows.h>

BOOL WINAPI DllMain(HANDLE hinstDLL, 
                    DWORD dwReason, 
                    LPVOID lpvReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH: 
        if (NvOsFopen("NFS1:", NVOS_OPEN_READ | NVOS_OPEN_WRITE, &s_NvStorMgrFile)
            != NvSuccess)
        {
            return FALSE;
        }
        break;

    case DLL_PROCESS_DETACH: 
        NvOsFclose(s_NvStorMgrFile);
        s_NvStorMgrFile = NULL;
        break;

    case DLL_THREAD_DETACH:
    case DLL_THREAD_ATTACH:
        break;

    default:
        return FALSE;
    }

    return TRUE;
}
#else
#error Need to figure out what to do for non-Windows
#endif



/* This includes the functions needed for the stub function, that return the name of
 * kernel device, and the magic IOCTL to use to communicate with that device
 */

NvOsFileHandle NvStorManager_NvIdlGetIoctlFile( void )
{
    return s_NvStorMgrFile;
}


NvU32 NvStorManager_NvIdlGetIoctlCode( void )
{
    return NvStorMgrIoctls_Generic;
}

