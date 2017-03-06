/*
 * Copyright 2007-2011 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvassert.h"
#include "aos.h"
#include "nvrm_drf.h"
#include "ap20/arscu.h"

#define SCU_PA_BASE         0x50040000  // Base address for arscu.h registers

#define NV_SCU_REGR(reg)      AOS_REGR(SCU_PA_BASE + SCU_##reg##_0)
#define NV_SCU_REGW(reg, val) AOS_REGW(SCU_PA_BASE + SCU_##reg##_0, (val))

extern NvBool s_UseSCU;

void enableScu(void)
{
    NvU32 reg;

    if (!s_UseSCU)
        return;

    reg = NV_SCU_REGR(CONTROL);
    if (NV_DRF_VAL(SCU, CONTROL, SCU_ENABLE, reg) == 1)
    {
        /* SCU already enabled, return */
        return;
    }

    /* Invalidate all ways for all processors */
    NV_SCU_REGW(INVALID_ALL, 0xffff);

    /* Enable SCU - bit 0 */
    reg = NV_SCU_REGR(CONTROL);
    reg = NV_FLD_SET_DRF_NUM(SCU, CONTROL, SCU_ENABLE, 1, reg);
    NV_SCU_REGW(CONTROL, reg);
}

void disableScu(void)
{
    NvU32 reg;

    if (!s_UseSCU)
        return;

    reg = NV_SCU_REGR(CONTROL);
    if (NV_DRF_VAL(SCU, CONTROL, SCU_ENABLE, reg) == 0)
    {
        //Return if scu already disabled
        return;
    }

    /* Invalidate all ways for all processors */
    NV_SCU_REGW(INVALID_ALL, 0xffff);

    /* Disable SCU - bit 0 */
    reg = NV_SCU_REGR(CONTROL);
    reg = NV_FLD_SET_DRF_NUM(SCU, CONTROL, SCU_ENABLE, 0, reg);
    NV_SCU_REGW(CONTROL, reg);
}
