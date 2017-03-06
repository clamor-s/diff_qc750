/*
 * Copyright (c) 2007-2009 NVIDIA Corporation.  All Rights Reserved.
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
#include "nvidlcmd.h"
#include "nvrm_memmgr.h"

NvOsFileHandle NvRm_NvIdlGetIoctlFile( void )
{
    NV_ASSERT(!"Not supported");
    return NULL;
}

int NvRm_MemmgrGetIoctlFile(void)
{
    NV_ASSERT(!"Not supported");
    return 0;
}
