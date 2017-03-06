/*
 * Copyright (c) 2007 - 2009 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#if NV_IS_AVP
#define NV_DEF_RMC_TRACE                0   // NO TRACING FOR AVP
#endif

#include "nvcommon.h"
#include "nvassert.h"
#include "nvrm_hardware_access.h"
#include "nvrm_module_private.h"
#include "nvrm_rmctrace.h"
#include "nvrm_chiplib.h"
#include "nvrm_hwintf.h"

// FIXME:  This file needs to be split up, when we build user/kernel 
//         The NvRegr/NvRegw should thunk to the kernel since the rm
//         handle is not usable in user space.
//
//         NvRmPhysicalMemMap/NvRmPhysicalMemUnmap need to be in user space.
//

static NvRmModuleInstance *
get_instance( NvRmDeviceHandle rm, NvRmModuleID aperture )
{
    NvRmModuleTable *tbl;
    NvRmModuleInstance *inst;
    NvU32 Module   = NVRM_MODULE_ID_MODULE( aperture );
    NvU32 Instance = NVRM_MODULE_ID_INSTANCE( aperture );
    NvU32 Bar      = NVRM_MODULE_ID_BAR( aperture );
    NvU32 DeviceId;
    NvU32 idx = 0;

    tbl = NvRmPrivGetModuleTable( rm );

    inst = tbl->ModInst + tbl->Modules[Module].Index;
    NV_ASSERT( inst );
    NV_ASSERT( inst < tbl->LastModInst );

    DeviceId = inst->DeviceId;

    // find the right instance and bar
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

    NV_ASSERT( inst->DeviceId == DeviceId );
    NV_ASSERT( inst->VirtAddr );

    return inst;
}

NvU32 NvRegr( NvRmDeviceHandle rm, NvRmModuleID aperture, NvU32 offset )
{
    NvRmModuleInstance *inst;
    void *addr;

#if NV_DEF_RMC_TRACE
    NvRmRmcFile *file;
#endif

    inst = get_instance( rm, aperture );
    addr = (void *)((NvUPtr)inst->VirtAddr + offset);

#if NV_DEF_RMC_TRACE
    if( NvRmGetRmcFile( rm, &file ) == NvSuccess )
    {
        NVRM_RMC_TRACE(( file, "OnDevice NULL\n" ));
        NVRM_RMC_TRACE(( file, "RegisterRead 0x%.8x\n",
            inst->PhysAddr + offset ));
        NVRM_RMC_TRACE(( file, "OnDevice HOST1X\n" ));
    }
#endif

    return NV_READ32( addr );
}

void NvRegw( NvRmDeviceHandle rm, NvRmModuleID aperture, NvU32 offset,
    NvU32 data )
{
    NvRmModuleInstance *inst;
    void *addr;
#if NV_DEF_RMC_TRACE
    NvRmRmcFile *file;
#endif

    inst = get_instance( rm, aperture );
    addr = (void *)((NvUPtr)inst->VirtAddr + offset);

#if NV_DEF_RMC_TRACE
    if( NvRmGetRmcFile( rm, &file ) == NvSuccess )
    {
        NVRM_RMC_TRACE(( file, "OnDevice NULL\n" ));
        NVRM_RMC_TRACE(( file, "RegisterWrite 0x%.8x 0x%.8x\n",
            inst->PhysAddr + offset, data ));
        NVRM_RMC_TRACE(( file, "OnDevice HOST1X\n" ));
    }
#endif

    NV_WRITE32( addr, data );
}

NvU8 NvRegr08( NvRmDeviceHandle rm, NvRmModuleID aperture, NvU32 offset )
{
    NvRmModuleInstance *inst;
    void *addr;
#if NV_DEF_RMC_TRACE
    NvRmRmcFile *file;
#endif

    inst = get_instance( rm, aperture );
    addr = (void *)((NvUPtr)inst->VirtAddr + offset);

#if NV_DEF_RMC_TRACE
    if( NvRmGetRmcFile( rm, &file ) == NvSuccess )
    {
        NVRM_RMC_TRACE(( file, "OnDevice NULL\n" ));
        NVRM_RMC_TRACE(( file, "RegisterRead 0x%.8x\n",
            inst->PhysAddr + offset ));
        NVRM_RMC_TRACE(( file, "OnDevice HOST1X\n" ));
    }
#endif

    return NV_READ8( addr );
}


void NvRegw08( NvRmDeviceHandle rm, NvRmModuleID aperture, NvU32 offset,
    NvU8 data )
{
    NvRmModuleInstance *inst;
    void *addr;
#if NV_DEF_RMC_TRACE
    NvRmRmcFile *file;
#endif

    inst = get_instance( rm, aperture );
    addr = (void *)((NvUPtr)inst->VirtAddr + offset);

#if NV_DEF_RMC_TRACE
    if( NvRmGetRmcFile( rm, &file ) == NvSuccess )
    {
        NVRM_RMC_TRACE(( file, "OnDevice NULL\n" ));
        NVRM_RMC_TRACE(( file, "RegisterWrite 0x%.8x 0x%.8x\n",
            inst->PhysAddr + offset, data ));
        NVRM_RMC_TRACE(( file, "OnDevice HOST1X\n" ));
    }
#endif

    NV_WRITE08( addr, data );
}

void NvRegrm( NvRmDeviceHandle rm, NvRmModuleID aperture, NvU32 num,
    const NvU32 *offsets, NvU32 *values )
{
    void *addr;
    NvRmModuleInstance *inst;
    NvU32 i;

#if NV_DEF_RMC_TRACE
    NvRmRmcFile *file;
#endif

    inst = get_instance( rm, aperture );

    for( i = 0; i < num; i++ )
    {
        addr = (void *)((NvUPtr)inst->VirtAddr + offsets[i]);

#if NV_DEF_RMC_TRACE
        if( NvRmGetRmcFile( rm, &file ) == NvSuccess )
        {
            NVRM_RMC_TRACE(( file, "OnDevice NULL\n" ));
            NVRM_RMC_TRACE(( file, "RegisterRead 0x%.8x\n",
                inst->PhysAddr + offsets[i] ));
            NVRM_RMC_TRACE(( file, "OnDevice HOST1X\n" ));
        }
#endif

        values[i] = NV_READ32( addr );
    }
}

void NvRegwm( NvRmDeviceHandle rm, NvRmModuleID aperture, NvU32 num,
    const NvU32 *offsets, const NvU32 *values )
{
    void *addr;
    NvRmModuleInstance *inst;
    NvU32 i;

#if NV_DEF_RMC_TRACE
    NvRmRmcFile *file;
#endif

    inst = get_instance( rm, aperture );

    for( i = 0; i < num; i++ )
    {
        addr = (void *)((NvUPtr)inst->VirtAddr + offsets[i]);

#if NV_DEF_RMC_TRACE
        if( NvRmGetRmcFile( rm, &file ) == NvSuccess )
        {
            NVRM_RMC_TRACE(( file, "OnDevice NULL\n" ));
            NVRM_RMC_TRACE(( file, "RegisterWrite 0x%.8x 0x%.8x\n",
                inst->PhysAddr + offsets[i], values[i] ));
            NVRM_RMC_TRACE(( file, "OnDevice HOST1X\n" ));
        }
#endif

        NV_WRITE32( addr, values[i] );
    }
}

void NvRegwb( NvRmDeviceHandle rm, NvRmModuleID aperture, NvU32 num,
    NvU32 offset, const NvU32 *values )
{
    void *addr;
    NvRmModuleInstance *inst;

#if NV_DEF_RMC_TRACE
    NvU32 i;
    NvRmRmcFile *file;
#endif

    inst = get_instance( rm, aperture );
    addr = (void *)((NvUPtr)inst->VirtAddr + offset);
    NV_WRITE( addr, values, (num << 2) );

#if NV_DEF_RMC_TRACE
    for( i = 0; i < num; i++ )
    {
        if( NvRmGetRmcFile( rm, &file ) == NvSuccess )
        {
            NVRM_RMC_TRACE(( file, "OnDevice NULL\n" ));
            NVRM_RMC_TRACE(( file, "RegisterWrite 0x%.8x 0x%.8x\n",
                inst->PhysAddr + offset + (i<<2), values[i] ));
            NVRM_RMC_TRACE(( file, "OnDevice HOST1X\n" ));
        }
    }
#endif
}

void NvRegrb( NvRmDeviceHandle rm, NvRmModuleID aperture, NvU32 num,
    NvU32 offset, NvU32 *values )
{
    void *addr;
    NvRmModuleInstance *inst;

#if NV_DEF_RMC_TRACE
    NvU32 i;
    NvRmRmcFile *file;
#endif

    inst = get_instance( rm, aperture );
    addr = (void *)((NvUPtr)inst->VirtAddr + offset);
    NV_READ( values, addr, (num << 2 ) );

#if NV_DEF_RMC_TRACE
    for( i = 0; i < num; i++ )
    {
        if( NvRmGetRmcFile( rm, &file ) == NvSuccess )
        {
            NVRM_RMC_TRACE(( file, "OnDevice NULL\n" ));
            NVRM_RMC_TRACE(( file, "RegisterRead 0x%.8x\n",
                inst->PhysAddr + offset + (i<<2)));
            NVRM_RMC_TRACE(( file, "OnDevice HOST1X\n" ));
        }
    }
#endif
}
