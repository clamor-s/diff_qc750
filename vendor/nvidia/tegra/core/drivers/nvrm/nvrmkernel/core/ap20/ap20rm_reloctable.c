/*
 * Copyright (c) 2007-2008 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvos.h"
#include "nvrm_init.h"
#include "common/nvrm_hwintf.h"
#include "ap20/project_relocation_table.h"
#include "ap15/ap15rm_private.h"

static NvU32 s_RelocationTable[] =
{
    NV_RELOCATION_TABLE_INIT
};

NvU32 *
NvRmPrivAp20GetRelocationTable( NvRmDeviceHandle hDevice )
{
    return s_RelocationTable;
}

