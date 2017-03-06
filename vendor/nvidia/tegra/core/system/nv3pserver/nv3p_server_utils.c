/*
 * Copyright (c) 2008 - 2011 NVIDIA Corporation.  All rights reserved.
 * 
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvddk_blockdevmgr.h"
#include "nv3p.h"
#include "nv3p_server_utils.h"
#include "nv3p_server_private.h"

//Fixme: Added ap20 supported devices to NvRmModuleID enum and extend this API to 
//support devices on AP20
NvError
NvBl3pConvertBootToRmDeviceType(
    NvBootDevType bootDevId,
    NvDdkBlockDevMgrDeviceId *blockDevId)
{

    Nv3pServerUtilsHal Hal;
    if ( Nv3pServerUtilsGetAp20Hal(&Hal) ||  Nv3pServerUtilsGetT30Hal(&Hal))
    {
        return Hal.ConvertBootToRmDeviceType(bootDevId,blockDevId);
    }

    return NvError_NotSupported;
}

/* ------------------------------------------------------------------------ */
//Fixme: Added ap20 supported devices to NvRmModuleID enum and extend this API to 
//support devices on AP20
NvError
NvBl3pConvert3pToRmDeviceType(
    Nv3pDeviceType DevNv3p,
    NvRmModuleID *pDevNvRm)
{
    Nv3pServerUtilsHal Hal;
    if ( Nv3pServerUtilsGetAp20Hal(&Hal) || Nv3pServerUtilsGetT30Hal(&Hal))
    {
        return Hal.Convert3pToRmDeviceType(DevNv3p,pDevNvRm);
    }
    return NvError_NotSupported;
}


NvBool
NvBlValidateRmDevice
    (NvBootDevType BootDevice,
    NvRmModuleID RmDeviceId)
{
    Nv3pServerUtilsHal Hal;
    if ( Nv3pServerUtilsGetAp20Hal(&Hal) || Nv3pServerUtilsGetT30Hal(&Hal))
    {
        return Hal.ValidateRmDevice(BootDevice,RmDeviceId);
    }

        return NV_FALSE;
}


void
NvBlUpdateBct(
    NvU8* data,
    NvU32 BctSection)
{
    Nv3pServerUtilsHal Hal;
    if ( Nv3pServerUtilsGetAp20Hal(&Hal) || Nv3pServerUtilsGetT30Hal(&Hal))
    {
        Hal.UpdateBct(data,BctSection);
    }
}
