
#define NV_IDL_IS_STUB

/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvos.h"
#include "nvassert.h"
#include "nvidlcmd.h"
#include "nvrm_interrupt.h"

NvU32 NvRmGetIrqCountForLogicalInterrupt(
    NvRmDeviceHandle hRmDevice,
    NvRmModuleID ModuleID)
{
    return 1;
}

NvU32 NvRmGetIrqForLogicalInterrupt(
    NvRmDeviceHandle hRmDevice,
    NvRmModuleID ModuleID,
    NvU32 Index)
{
    switch (NVRM_MODULE_ID_MODULE(ModuleID))
    {
    case NvRmModuleID_GraphicsHost:
        if (Index==0)
            return 97;  /* MPCORE_SYNCPT */
        else if (Index==1)
            return 99; /* MPCORE_GENERAL */
        break;
    case NvRmModuleID_Display:
        if (NVRM_MODULE_ID_INSTANCE(ModuleID)==0)
            return 105; /* DISPLAY_GENERAL */
        else if (NVRM_MODULE_ID_INSTANCE(ModuleID)==1)
            return 106;
        break;
    }

    NvOsDebugPrintf("%s: MOD[%u] INST[%u] Index[%u] not found\n",
                    __func__, NVRM_MODULE_ID_MODULE(ModuleID),
                    NVRM_MODULE_ID_INSTANCE(ModuleID), Index);
    return -1;
}
