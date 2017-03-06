/*
 * Copyright (c) 2009 - 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvassert.h"
#include "ap20rm_clocks_limits_private.h"
#include "t30rm_clocks_limits_private.h"
#include "ap15rm_private.h"
#include "ap20/arapb_misc.h"

NvError
NvRmPrivChipShmooDataInit(
    NvRmDeviceHandle hRmDevice,
    NvRmChipFlavor* pChipFlavor)
{
    // Read chip ID from h/w and dispatch initialization
    NvU32 ChipId = NV_REGR(hRmDevice, NVRM_MODULE_ID(NvRmModuleID_Misc, 0),
        APB_MISC_GP_HIDREV_0);
    ChipId = NV_DRF_VAL(APB_MISC_GP, HIDREV, CHIPID, ChipId);

    switch (ChipId)
    {
        case 0x20:
            NvRmPrivAp20FlavorInit(hRmDevice, pChipFlavor);
            break;
        case 0x30:
            NvRmPrivT30FlavorInit(hRmDevice, pChipFlavor);
            break;
        default:
            NV_ASSERT(!"Unsupported chip ID");
            return NvError_NotSupported;
    }
    return NvSuccess;
}

