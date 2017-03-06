/*
 * Copyright (c) 2009 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvassert.h"
#include "nvrm_minikernel.h"
#include "ap15rm_private.h"
#include "nvrm_structure.h"
#include "common/nvrm_hwintf.h"
#include "common/nvrm_chiplib.h"

NvRmDevice gs_Rm;

NvRmDeviceHandle NvRmPrivGetRmDeviceHandle(void)
{
    return &gs_Rm;
}

#if NV_DEF_ENVIRONMENT_SUPPORTS_SIM
NvBool
NvRmIsSimulation(void)
{
    NvRmDevice *rm = NvRmPrivGetRmDeviceHandle();

    return (rm->cfg.Chiplib[0] != '\0');
}
#endif

static NvRmModuleInstance *
get_instance( NvRmDeviceHandle rm, NvRmModuleID aperture )
{
    NvRmModuleTable *tbl;
    NvRmModuleInstance *inst;
    NvU32 DeviceId;
    NvU32 Module   = NVRM_MODULE_ID_MODULE( aperture );
    NvU32 Instance = NVRM_MODULE_ID_INSTANCE( aperture );
    NvU32 Bar      = NVRM_MODULE_ID_BAR( aperture );
    NvU32 idx;

    tbl = NvRmPrivGetModuleTable( rm );

    NV_ASSERT( tbl->Modules[aperture].Index != NVRM_MODULE_INVALID );

    inst = tbl->ModInst + tbl->Modules[Module].Index;

    DeviceId = inst->DeviceId;

    // find the right instance and bar
    idx = 0;
    while( inst->DeviceId == DeviceId )
    {
        if( idx == Instance && inst->Bar == Bar )
        {
            break;
        }
        if( inst->Bar == 0 )
        {
            idx++;
        }

        inst++;
    }

    return inst;
}

NvU32 NvRegr( NvRmDeviceHandle rm, NvRmModuleID aperture, NvU32 offset )
{
    NvRmModuleInstance *inst;
    void *addr;

    inst = get_instance( rm, aperture );
    addr = (void *)((NvUPtr)inst->VirtAddr + offset);

    return NV_READ32( addr );
}

void NvRegw( NvRmDeviceHandle rm, NvRmModuleID aperture, NvU32 offset,
    NvU32 data )
{
    NvRmModuleInstance *inst;
    void *addr;

    inst = get_instance( rm, aperture );
    addr = (void *)((NvUPtr)inst->VirtAddr + offset);

    NV_WRITE32( addr, data );
}
