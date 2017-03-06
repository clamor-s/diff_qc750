/*
 * Copyright (c) 2007 - 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvrm_module.h"
#include "nvrm_drf.h"
#include "ap20/arclk_rst.h"
#include "ap15rm_clocks.h"
#include "nvrm_structure.h"


#define CLOCK_ENABLE( rm, offset, field, EnableState ) \
    do { \
        regaddr = (CLK_RST_CONTROLLER_##offset##_0); \
        NvOsMutexLock((rm)->CarMutex); \
        reg = NV_REGR((rm), \
            NVRM_MODULE_ID( NvRmPrivModuleID_ClockAndReset, 0 ), regaddr); \
        reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, offset, field, \
            EnableState, reg); \
        NV_REGW((rm), NVRM_MODULE_ID( NvRmPrivModuleID_ClockAndReset, 0 ), \
            regaddr, reg); \
        NvOsMutexUnlock((rm)->CarMutex); \
    } while( 0 )

void
Ap15EnableTvDacClock(
    NvRmDeviceHandle hDevice,
    ModuleClockState ClockState)
{
    NvU32 reg;
    NvU32 regaddr;

    CLOCK_ENABLE( hDevice, CLK_OUT_ENB_H, CLK_ENB_TVDAC, ClockState );
}

void
NvRmPrivAp15ClockConfigEx(
    NvRmDeviceHandle hDevice,
    NvRmModuleID Module,
    NvU32 ClkSourceOffset,
    NvU32 flags)
{
    NvU32 reg;

    if ((Module == NvRmModuleID_Vi) &&
        (!(flags & NvRmClockConfig_SubConfig)) &&
         (flags & (NvRmClockConfig_InternalClockForPads |
                   NvRmClockConfig_ExternalClockForPads |
                   NvRmClockConfig_InternalClockForCore |
                   NvRmClockConfig_ExternalClockForCore)))
    {
#ifdef CLK_RST_CONTROLLER_CLK_SOURCE_VI_0_PD2VI_CLK_SEL_FIELD
        reg = NV_REGR( hDevice,
            NVRM_MODULE_ID( NvRmPrivModuleID_ClockAndReset, 0 ),
            ClkSourceOffset);

        /* Default is pads use External and Core use internal */
        reg = NV_FLD_SET_DRF_NUM(
            CLK_RST_CONTROLLER, CLK_SOURCE_VI, PD2VI_CLK_SEL, 0, reg);
        reg = NV_FLD_SET_DRF_NUM(
            CLK_RST_CONTROLLER, CLK_SOURCE_VI, VI_CLK_SEL, 0, reg);

        /* This is an invalid setting. */
        NV_ASSERT(!((flags & NvRmClockConfig_InternalClockForPads) &&
                    (flags & NvRmClockConfig_ExternalClockForCore)));

        if (flags & NvRmClockConfig_InternalClockForPads)
            reg = NV_FLD_SET_DRF_NUM(
                CLK_RST_CONTROLLER, CLK_SOURCE_VI, PD2VI_CLK_SEL, 1, reg);
        if (flags & NvRmClockConfig_ExternalClockForCore)
            reg = NV_FLD_SET_DRF_NUM(
                CLK_RST_CONTROLLER, CLK_SOURCE_VI, VI_CLK_SEL, 1, reg);

        NV_REGW(hDevice, NVRM_MODULE_ID( NvRmPrivModuleID_ClockAndReset, 0 ),
            ClkSourceOffset, reg);
#endif
    }
    if (Module == NvRmModuleID_I2s)
    {
        reg = NV_REGR( hDevice,
            NVRM_MODULE_ID( NvRmPrivModuleID_ClockAndReset, 0 ),
            ClkSourceOffset);

        if (flags & NvRmClockConfig_ExternalClockForCore)
        {
            // Set I2S in slave mode (field definition is the same for I2S1
            // and I2S2)
            reg = NV_FLD_SET_DRF_NUM(
                CLK_RST_CONTROLLER, CLK_SOURCE_I2S1, I2S1_MASTER_CLKEN, 0, reg);
            NV_REGW(hDevice,
                NVRM_MODULE_ID( NvRmPrivModuleID_ClockAndReset, 0 ),
                ClkSourceOffset, reg);
        }
        else if (flags & NvRmClockConfig_InternalClockForCore)
        {
            // Set I2S in master mode (field definition is the same for I2S1
            // and I2S2)
            reg = NV_FLD_SET_DRF_NUM(
                CLK_RST_CONTROLLER, CLK_SOURCE_I2S1, I2S1_MASTER_CLKEN, 1, reg);
            NV_REGW(hDevice,
                NVRM_MODULE_ID( NvRmPrivModuleID_ClockAndReset, 0 ),
                ClkSourceOffset, reg);
        }
    }
}

void NvRmPrivAp15SimPllInit(NvRmDeviceHandle hRmDevice)
{
    NvU32 RegData;

    //Enable the plls in simulation. We can just use PLLC as the template
    //and replicate across pllM and pllP since the offsets are the same.
    RegData = NV_DRF_NUM (CLK_RST_CONTROLLER, PLLC_BASE, PLLC_DIVP, 0) 
          | NV_DRF_NUM (CLK_RST_CONTROLLER, PLLC_BASE, PLLC_DIVM, 0) 
          | NV_DRF_NUM (CLK_RST_CONTROLLER, PLLC_BASE, PLLC_DIVN, 0)
          | NV_DRF_DEF (CLK_RST_CONTROLLER, PLLC_BASE, PLLC_BYPASS, DISABLE)
          | NV_DRF_DEF (CLK_RST_CONTROLLER, PLLC_BASE, PLLC_ENABLE, ENABLE) ;

    NV_REGW(hRmDevice, NVRM_MODULE_ID( NvRmPrivModuleID_ClockAndReset, 0 ),
            CLK_RST_CONTROLLER_PLLM_BASE_0, RegData);
    NV_REGW(hRmDevice, NVRM_MODULE_ID( NvRmPrivModuleID_ClockAndReset, 0 ),
            CLK_RST_CONTROLLER_PLLC_BASE_0, RegData);
    NV_REGW(hRmDevice, NVRM_MODULE_ID( NvRmPrivModuleID_ClockAndReset, 0 ),
            CLK_RST_CONTROLLER_PLLP_BASE_0, RegData);
}

void
NvRmPrivAp15OscPllRefFreqInit(
    NvRmDeviceHandle hRmDevice,
    NvRmFreqKHz* pOscKHz,
    NvRmFreqKHz* pPllRefKHz)
{
        /* OSC_FREQ:  0,     1,     2,     3  */
        static const NvRmFreqKHz s_Ap15OscFreqKHz[] =
                { 13000, 19200, 12000, 26000 };

        NvU32 reg = NV_REGR(hRmDevice,
            NVRM_MODULE_ID(NvRmPrivModuleID_ClockAndReset, 0),
            CLK_RST_CONTROLLER_OSC_CTRL_0);
        reg = NV_DRF_VAL(CLK_RST_CONTROLLER, OSC_CTRL, OSC_FREQ, reg);

        NV_ASSERT(reg < NV_ARRAY_SIZE(s_Ap15OscFreqKHz));
        *pOscKHz = *pPllRefKHz = s_Ap15OscFreqKHz[reg];
}
