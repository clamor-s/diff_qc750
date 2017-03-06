/*
 * Copyright (c) 2009 - 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */


#include "nvcommon.h"
#include "nvrm_drf.h"
#include "nvrm_module.h"
#include "nvrm_hardware_access.h"
#include "nvaboot_private.h"
#include "ap20/arapbpm.h"
#include "ap20/arapb_misc.h"
#include "ap20/arclk_rst.h"
#include "ap20/aremc.h"
#include "ap20/nvboot_bct.h"
#include "ap20/nvboot_bit.h"
#include "ap20/nvboot_pmc_scratch_map.h"

/** SCRATCH_REGS() - PMC scratch registers (list of SCRATCH_REG() macros).
    SCRATCH_REG(s) - PMC scratch register name:

    @param s Scratch register name (APBDEV_PMC_s)
 */
#define SCRATCH_REGS()          \
        SCRATCH_REG(SCRATCH2)   \
        SCRATCH_REG(SCRATCH4)   \
        SCRATCH_REG(SCRATCH24)  \
        /* End-of-List*/

#define SCRATCH_REG(s) static NvU32 s = 0;
SCRATCH_REGS()
#undef SCRATCH_REG

#define REGS() \
        /* CLK_RST Group */ \
        REG(SCRATCH2, CLK_RST_CONTROLLER, OSC_CTRL, XOBP) \
        REG(SCRATCH2, CLK_RST_CONTROLLER, PLLM_BASE, PLLM_DIVM) \
        REG(SCRATCH2, CLK_RST_CONTROLLER, PLLM_BASE, PLLM_DIVN) \
        REG(SCRATCH2, CLK_RST_CONTROLLER, PLLM_BASE, PLLM_DIVP) \
        REG(SCRATCH2, CLK_RST_CONTROLLER, PLLM_MISC, PLLM_CPCON) \
        REG(SCRATCH2, CLK_RST_CONTROLLER, PLLM_MISC, PLLM_LFCON) \
        /**/ \
        /* EMC Group */ \
        REG2(SCRATCH4, EMC, FBIO_SPARE, CFG_FBIO_SPARE_WB0) \
        /* APB_MISC Group */ \
        REG3(SCRATCH2, APB_MISC, GP_XM2CFGAPADCTRL, CFG2TMC_XM2CFGA_PREEMP_EN) \
        REG3(SCRATCH2, APB_MISC, GP_XM2CFGDPADCTRL, CFG2TMC_XM2CFGD_SCHMT_EN) \
        /**/ \
        /* BCT SdramParams Group*/ \
        RAM(SCRATCH2, MEMORY_TYPE, MemoryType) \
        /**/ \
        RAM(SCRATCH4, EMC_CLOCK_DIVIDER, EmcClockDivider) \
        CONSTANT(SCRATCH4, PLLM_STABLE_TIME, ~0) /* Stuff the maximum value (see bug 584658) */ \
        CONSTANT(SCRATCH4, PLLX_STABLE_TIME, ~0) /* Stuff the maximum value (see bug 584658) */ \
        /**/ \
        RAM(SCRATCH24, EMC_AUTO_CAL_WAIT, EmcAutoCalWait) \
        RAM(SCRATCH24, EMC_PIN_PROGRAM_WAIT, EmcPinProgramWait) \
        RAM(SCRATCH24, WARMBOOT_WAIT, WarmBootWait)

#define NV_PMC_REGR(pBase, reg)\
    NV_READ32( (((NvUPtr)(pBase)) + APBDEV_PMC_##reg##_0))
#define NV_PMC_REGW(pBase, reg, val)\
    NV_WRITE32((((NvUPtr)(pBase)) + APBDEV_PMC_##reg##_0), (val))

//------------------------------------------------------------------------------
// Boot ROM PMC scratch map name remapping to fix broken names (see bug 542815).
//------------------------------------------------------------------------------

//Correct name->Broken name from nvboot_pmc_scratch_map.h
#define APBDEV_PMC_SCRATCH2_0_CLK_RST_CONTROLLER_OSC_CTRL_0_XOBP_RANGE\
    APBDEV_PMC_SCRATCH2_0_CLK_RST_OSC_CTRL_XOBP_RANGE
#define APBDEV_PMC_SCRATCH2_0_CLK_RST_CONTROLLER_PLLM_BASE_0_PLLM_DIVM_RANGE\
    APBDEV_PMC_SCRATCH2_0_CLK_RST_PLLM_BASE_PLLM_DIVM_RANGE
#define APBDEV_PMC_SCRATCH2_0_CLK_RST_CONTROLLER_PLLM_BASE_0_PLLM_DIVN_RANGE\
    APBDEV_PMC_SCRATCH2_0_CLK_RST_PLLM_BASE_PLLM_DIVN_RANGE
#define APBDEV_PMC_SCRATCH2_0_CLK_RST_CONTROLLER_PLLM_BASE_0_PLLM_DIVP_RANGE\
    APBDEV_PMC_SCRATCH2_0_CLK_RST_PLLM_BASE_PLLM_DIVP_RANGE
#define APBDEV_PMC_SCRATCH2_0_CLK_RST_CONTROLLER_PLLM_MISC_0_PLLM_CPCON_RANGE\
    APBDEV_PMC_SCRATCH2_0_CLK_RST_PLLM_MISC_CPCON_RANGE
#define APBDEV_PMC_SCRATCH2_0_CLK_RST_CONTROLLER_PLLM_MISC_0_PLLM_LFCON_RANGE\
    APBDEV_PMC_SCRATCH2_0_CLK_RST_PLLM_MISC_LFCON_RANGE

#define BIT_BASE_ADDR 0x40000000

void NvAbootPrivAp20Reset(NvAbootHandle hAboot)
{
    NvU32 reg = NV_DRF_DEF(APBDEV_PMC, CNTRL, MAIN_RST, ENABLE);
    NV_REGW(hAboot->hRm, NVRM_MODULE_ID( NvRmModuleID_Pmif, 0 ),
        APBDEV_PMC_CNTRL_0, reg);
}

NvError NvAbootPrivAp20SaveSdramParams(NvAbootHandle hAboot)
{
    NvU32                   Reg;            //Module register contents
    NvU32                   Val;            //Register field contents
    NvU32                   RamCode;        //BCT RAM selector
    volatile void           *g_pPMC;
    NvBctHandle             hBct = NULL;
    NvU32                   Size, Instance;
    NvError                 e = NvSuccess;
    NvBootSdramParams       SdramParams;

    Size = 0;
    Instance = 0;

    //Read strap register and extract the RAM selection code.
    //NOTE: The boot ROM limits the RamCode values to the range 0-3.
    Reg = NV_REGR(hAboot->hRm, NVRM_MODULE_ID(NvRmModuleID_Misc, 0),
        APB_MISC_PP_STRAPPING_OPT_A_0);
    RamCode = NV_DRF_VAL(APB_MISC, PP_STRAPPING_OPT_A, RAM_CODE, Reg) & 3;

    NV_CHECK_ERROR_CLEANUP(NvBctInit(&Size, NULL, &hBct));

    NV_CHECK_ERROR_CLEANUP(
        NvBctGetData(hBct, NvBctDataType_SdramConfigInfo,
            &Size, &Instance, NULL)
    );

    if (RamCode > Instance)
    {
        e = NvError_BadParameter;
        goto fail;
    }

    NV_CHECK_ERROR_CLEANUP(
        NvBctGetData(hBct, NvBctDataType_SdramConfigInfo,
            &Size, &RamCode, (NvU8 *)&SdramParams)
    );

    // REG(s,d,r,f)
    //  s = destination Scratch register
    //  d = Device name
    //  r = Register name
    //  f = register Field
    #define REG(s,d,r,f)  \
        Reg = NV_REGR(hAboot->hRm,\
        NVRM_MODULE_ID( NvRmPrivModuleID_ClockAndReset, 0 ),\
        d##_##r##_0);\
        Val = NV_DRF_VAL(d,r,f,Reg); \
        s = NV_FLD_SET_SDRF_NUM(s,d,r,f,Val);

    #define REG2(s,d,r,f)  \
        Reg = NV_REGR(hAboot->hRm,\
        NVRM_MODULE_ID( NvRmPrivModuleID_ExternalMemoryController, 0 ),\
        d##_##r##_0);\
        Val = NV_DRF_VAL(d,r,f,Reg); \
        s = NV_FLD_SET_SDRF_NUM(s,d,r,f,Val);

    #define REG3(s,d,r,f)  \
        Reg = NV_REGR(hAboot->hRm,\
        NVRM_MODULE_ID( NvRmModuleID_Misc, 0 ),\
        d##_##r##_0);\
        Val = NV_DRF_VAL(d,r,f,Reg); \
        s = NV_FLD_SET_SDRF_NUM(s,d,r,f,Val);

    //RAM(s,f,n)
    //s = destination Scratch register
    //f = register Field
    //v = bct Variable
    #define RAM(s,f,v) \
        s = NV_FLD_SET_SF_NUM(s,f,SdramParams.v);

    //Define the transformation macro that will stuff a PMC scratch register
    //with a constant value.

    //CONSTANT(s,f,n)
    //s = destination Scratch register
    //f = register Field
    //v = constant Value
    #define CONSTANT(s,f,v) \
        s = NV_FLD_SET_SF_NUM(s,f,v);

    //Instantiate all of the register transformations.
    REGS()
    #undef RAM
    #undef CONSTANT

    //Generate writes to the PMC scratch registers to copy the local
    //variables to the actual registers.
    #define SCRATCH_REG(s)\
        NV_REGW(hAboot->hRm, NVRM_MODULE_ID( NvRmModuleID_Pmif, 0 ),\
        APBDEV_PMC_##s##_0, s);
    SCRATCH_REGS()
    #undef SCRATCH_REG
 fail:
    NvBctDeinit(hBct);
    return e;
}
