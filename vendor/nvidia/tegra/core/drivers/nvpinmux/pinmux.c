/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "pinmux.h"
#include "nvassert.h"

NvError NvPinmuxDriveConfigTable(NvPinDrivePingroupConfig *config, NvU32 len)
{
    NvU32 i;

    NV_ASSERT(config);
    NV_ASSERT(len > 0);

    if (!config)
        return NvError_BadParameter;

    for (i = 0; i < len; i++)
    {
        NV_WRITE32((NV_ADDRESS_MAP_APB_MISC_BASE + config[i].PingroupDriveReg),
                config[i].Value);
    }
    return NvSuccess;
}
