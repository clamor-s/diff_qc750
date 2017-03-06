/**
 * Copyright (c) 2009 - 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NV3P_SERVER_PRIVATE_H
#define INCLUDED_NV3P_SERVER_PRIVATE_H

#include "nvcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct Nv3pServerUtilsHalRec
{
    NvError
    (*ConvertBootToRmDeviceType)(
        NvBootDevType bootDevId,
        NvDdkBlockDevMgrDeviceId *blockDevId);
    NvError
    (*Convert3pToRmDeviceType)(
        Nv3pDeviceType DevNv3p,
        NvRmModuleID *pDevNvRm);
    NvBool
    (*ValidateRmDevice)(
         NvBootDevType BootDevice,
         NvRmModuleID RmDeviceId);
    void
    (*UpdateBct)(
        NvU8* data,
        NvU32 BctSection);

} Nv3pServerUtilsHal;

NvBool Nv3pServerUtilsGetAp20Hal(Nv3pServerUtilsHal * pHal);
NvBool Nv3pServerUtilsGetT30Hal(Nv3pServerUtilsHal * pHal);

#ifdef __cplusplus
}
#endif

#endif
