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
#include "nvcommon.h"
#include "nvassert.h"
#include "pinmux_ap20.h"

static void NvPinmuxApplyConfig(NvPingroupConfig Pin)
{
    NvU32 Reg = 0;

    //pupd operation
    if (Pin.PupdReg > 0)
    {
        Reg = NV_READ32(NV_ADDRESS_MAP_APB_MISC_BASE + Pin.PupdReg);
        Reg &= ~(0x3 << Pin.PupdShift);
        Reg |= (Pin.Pupd << Pin.PupdShift);
        NV_WRITE32(NV_ADDRESS_MAP_APB_MISC_BASE + Pin.PupdReg, Reg);
    }

    //pinmux operation
    if (Pin.MuxReg > 0)
    {
        Reg = NV_READ32(NV_ADDRESS_MAP_APB_MISC_BASE + Pin.MuxReg);
        Reg &= ~(0x3 << Pin.MuxShift) ;
        Reg |= Pin.Pinmux << Pin.MuxShift ;
        NV_WRITE32(NV_ADDRESS_MAP_APB_MISC_BASE + Pin.MuxReg, Reg);
    }

    //tristate operation
    if (Pin.TristateReg > 0)
    {
        Reg = NV_READ32(NV_ADDRESS_MAP_APB_MISC_BASE + Pin.TristateReg);
        Reg &= ~(0x1 << Pin.TristateShift);
        Reg |= (Pin.Tristate << Pin.TristateShift);
        NV_WRITE32(NV_ADDRESS_MAP_APB_MISC_BASE + Pin.TristateReg, Reg);
    }
}

NvError NvPinmuxConfigTable(NvPingroupConfig *config, NvU32 len)
{
    NvU32 i;

    NV_ASSERT(config);
    NV_ASSERT(len > 0);

    if (!config)
        return NvError_BadParameter;

    for (i = 0; i < len; i++)
    {
        NvPinmuxApplyConfig(config[i]);
    }
    return NvSuccess;
}
