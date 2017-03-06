/*
 * Copyright (c) 2009-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited
 */

#include "bootloader.h"
#include "nvassert.h"
#include "aos.h"
#include "arapb_misc.h"
#include "arevp.h"
#include "arflow_ctlr.h"
#include "arpg.h"
#include "arfuse.h"
#include "nvuart.h"

#ifndef NV_TEST_LOADER
#include "nvbl_arm_processor.h"

#if  AES_KEYSCHED_LOCK_WAR_BUG_598910
#include "aes_keyschedule_lock.h"
#endif

#if SE_AES_KEYSCHED_READ_LOCK
#include "se_aes_keyschedule_lock.h"
#endif

#pragma arm

NvU32 s_ChipId;
volatile NvU32 s_bFirstBoot = 1;

extern NvAosChip s_Chip;
extern NvBool s_QuickTurnEmul;
extern NvBool s_Simulation;
extern NvBool s_FpgaEmulation;
extern NvU32  s_Netlist;
extern NvU32  s_NetlistPatch;

NvBool NvBlQueryGetNv3pServerFlag_AP20(void);
NvBool NvBlQueryGetNv3pServerFlag_T30(void);
void ColdBoot_AP20(void);
void ColdBoot_T30(void);

NvBool NvBlIsMmuEnabled(void)
{
    NvU32   cpsr;   // Current Processor Status Register
    NvU32   cr;     // CP15 Control Register

    // If the processor is in USER mode (a non-privileged mode) the MMU is assumed to
    // be on because the MRC instruction to definitively determine the state of the MMU
    // is not available to non-privileged callers. This is a safe assumption because
    // the only time the processor is in user mode is well after the kernel has
    // been started and the kernel virtual memory environment has been established.

    GET_CPSR(cpsr);
    if ((cpsr & PSR_MODE_MASK) != PSR_MODE_USR)
    {
        // Read the CP15 Control Register
        MRC(p15, 0, cr, c1, c0, 0);

        // Is the MMU on?
        if (!(cr & M_CP15_C1_C0_0_M))
        {
            // MMU is off.
            return NV_FALSE;
        }
    }

    // MMU is on.
    return NV_TRUE;
}


void NvBlAvpUnHaltCpu(void)
{
    NV_FLOW_REGW(FLOW_PA_BASE, HALT_CPU_EVENTS, 0);
}


NvBool NvBlQueryGetNv3pServerFlag()
{
    switch (s_ChipId)
    {
        case 0x20:
            return NvBlQueryGetNv3pServerFlag_AP20();

        case 0x30:
            return NvBlQueryGetNv3pServerFlag_T30();

        default:
            break;
    }

    // return false on any failure.
    return NV_FALSE;
}

void bootloader( void )
{
    NvU32 reg;
    NvU32 major;
    NvU32 minor;

    if(s_bFirstBoot)
    {
#ifdef SET_KERNEL_PINMUX
        // initialize pinmuxes as in kernel on avp side of bootloader
        NvOdmPinmuxInit();
#endif
        // FIXME this init results in garbage prints, need to fix this
        NvAvpUartInit(PLLP_FIXED_FREQ_KHZ_432000);
    }

    reg = AOS_REGR( MISC_PA_BASE + APB_MISC_GP_HIDREV_0 );
    s_ChipId = NV_DRF_VAL(APB_MISC, GP_HIDREV, CHIPID, reg);
    major = NV_DRF_VAL(APB_MISC, GP_HIDREV, MAJORREV, reg);
    minor = NV_DRF_VAL(APB_MISC, GP_HIDREV, MINORREV, reg);

    //Disable key schedule reads
    //WAR for Bug#598910
#if  AES_KEYSCHED_LOCK_WAR_BUG_598910
    if(*(volatile NvU32*)(PG_UP_PA_BASE + PG_UP_TAG_0) != PG_UP_TAG_0_PID_CPU)
    {
        NvAesDisableKeyScheduleRead();
    }
#endif//AES_KEYSCHED_LOCK_WAR_BUG_598910

#if SE_AES_KEYSCHED_READ_LOCK
    if(*(volatile NvU32*)(PG_UP_PA_BASE + PG_UP_TAG_0) != PG_UP_TAG_0_PID_CPU)
    {
        NvLockSeAesKeySchedule();
    }
#endif

    if ((s_ChipId != 0x20) &&
        (NV_FUSE_REGR(FUSE_PA_BASE, SECURITY_MODE)))
    {
        // set debug capabilities
        reg  = AOS_REGR(SECURE_BOOT_PA_BASE + NVBL_SECURE_BOOT_PFCFG_OFFSET);
        reg &= NVBL_SECURE_BOOT_JTAG_CLEAR;
        reg |= NVBL_SECURE_BOOT_JTAG_VAL;
        AOS_REGW(SECURE_BOOT_PA_BASE + NVBL_SECURE_BOOT_PFCFG_OFFSET, reg);
    }
#ifndef BUILD_FOR_COSIM
    // enable JTAG
    reg = NV_DRF_DEF(APB_MISC, PP_CONFIG_CTL, TBE, ENABLE)
        | NV_DRF_DEF(APB_MISC, PP_CONFIG_CTL, JTAG, ENABLE);
    AOS_REGW(MISC_PA_BASE + APB_MISC_PP_CONFIG_CTL_0, reg);
#endif

    s_QuickTurnEmul = NV_FALSE;
    s_FpgaEmulation = NV_FALSE;
    s_Simulation = NV_FALSE;

    reg = AOS_REGR( MISC_PA_BASE + APB_MISC_GP_EMU_REVID_0);

    s_Netlist = NV_DRF_VAL(APB_MISC, GP_EMU_REVID, NETLIST, reg);
    s_NetlistPatch = NV_DRF_VAL(APB_MISC, GP_EMU_REVID, PATCH, reg);

    if( major == 0 )
    {
        if( s_Netlist == 0 )
        {
            s_Simulation = NV_TRUE;
        }
        else
        {
            if( minor == 0 )
            {
                s_QuickTurnEmul = NV_TRUE;
            }
            else
            {
                s_FpgaEmulation = NV_TRUE;
            }
        }
    }

    if( s_bFirstBoot )
    {
        /* need to set this before cold-booting, otherwise we'll end up in
         * an infinite loop.
         */
        s_bFirstBoot = 0;
        switch( s_ChipId ) {
        case 0x20:
            s_Chip = NvAosChip_Ap20;
            ColdBoot_AP20();
            break;
        case 0x30:
            s_Chip = NvAosChip_T30;
            ColdBoot_T30();
            break;
        default:
            NV_ASSERT( !"unknown chipid" );
            break;
        }
    }

    return;
}
#endif

void NvBlAvpSetCpuResetVector(NvU32 reset)
{
    NV_EVP_REGW(EVP_PA_BASE, CPU_RESET_VECTOR, reset);
}

void NvBlAvpStallUs(NvU32 MicroSec)
{
    NvU32           Reg;            // Flow controller register
    NvU32           Delay;          // Microsecond delay time
    NvU32           MaxUs;          // Maximum flow controller delay

    // Get the maxium delay per loop.
    MaxUs = NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, ZERO, 0xFFFFFFFF);

    while (MicroSec)
    {
        Delay     = (MicroSec > MaxUs) ? MaxUs : MicroSec;
        MicroSec -= Delay;

        Reg = NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, ZERO, Delay)
            | NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, uSEC, 1)
            | NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, MODE, 2);

        NV_FLOW_REGW(FLOW_PA_BASE, HALT_COP_EVENTS, Reg);
    }
}

void NvBlAvpStallMs(NvU32 MilliSec)
{
    NvU32           Reg;            // Flow controller register
    NvU32           Delay;          // Millisecond delay time
    NvU32           MaxMs;          // Maximum flow controller delay

    // Get the maxium delay per loop.
    MaxMs = NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, ZERO, 0xFFFFFFFF);

    while (MilliSec)
    {
        Delay     = (MilliSec > MaxMs) ? MaxMs : MilliSec;
        MilliSec -= Delay;

        Reg = NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, ZERO, Delay)
            | NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, MSEC, 1)
            | NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, MODE, 2);

        NV_FLOW_REGW(FLOW_PA_BASE, HALT_COP_EVENTS, Reg);
    }
}

void NvBlAvpHalt(void)
{
    NvU32   Reg;    // Scratch reg

    for (;;)
    {
        Reg = NV_DRF_DEF(FLOW_CTLR, HALT_COP_EVENTS, MODE, FLOW_MODE_STOP)
            | NV_DRF_NUM(FLOW_CTLR, HALT_COP_EVENTS, JTAG, 1);
        NV_FLOW_REGW(FLOW_PA_BASE, HALT_COP_EVENTS, Reg);
    }
}

