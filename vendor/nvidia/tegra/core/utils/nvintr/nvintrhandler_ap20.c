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
#include "nvassert.h"
#include "nvrm_drf.h"
#include "nvrm_arm_cp.h"
#include "nvintrhandler.h"
#include "nvrm_module.h"
#include "ap20/arfic_dist.h"
#include "ap20/arfic_proc_if.h"

#define AP20_PPI_INTR_ID_TIMER_0     (0 + 32)

#define NV_INTR_REGR(rm,inst,reg) \
    NV_REGR(rm, NVRM_MODULE_ID(NvRmPrivModuleID_ArmPerif, 0), \
        FIC_DIST_##reg##_0 + (inst) * 4)

#define NV_INTR_REGW(rm,inst,reg,data) \
    NV_REGW(rm, NVRM_MODULE_ID(NvRmPrivModuleID_ArmPerif, 0), \
        FIC_DIST_##reg##_0 + (inst) * 4, data)

#define NV_CIF_REGW(rm, inst, reg, data) \
    NV_REGW(rm, NVRM_MODULE_ID(NvRmPrivModuleID_ArmPerif, 0), \
        FIC_PROC_IF_##reg##_0, data)

#define NV_CIF_REGR(rm, inst, reg) \
    NV_REGR(rm, NVRM_MODULE_ID(NvRmPrivModuleID_ArmPerif, 0), \
        FIC_PROC_IF_##reg##_0)

static NvBool IsSecure(void);
static NvBool IsTrustzone(void);
static void MarkInterrupt(NvRmIntrHandle hRmDeviceIntr, NvU32 irq);

static NvBool IsSecure(void)
{
#if ENABLE_SECURITY
    return NV_TRUE;
#else
    return NV_FALSE;
#endif
}

static NvBool IsTrustzone(void)
{
    return TRUSTZONE_ENABLED;
}

static void MarkInterrupt(NvRmIntrHandle hRmDeviceIntr, NvU32 irq)
{
    if (IsTrustzone())
    {
        NvU32 reg;

        reg = NV_INTR_REGR(hRmDeviceIntr, (irq / NVRM_IRQS_PER_INTRCTLR),
                            INTERRUPT_SECURITY_0);
        if (IsSecure())
        {
            reg &= ~(1 << (irq % NVRM_IRQS_PER_INTRCTLR));
        }
        else
        {
            reg |= (1 << (irq % NVRM_IRQS_PER_INTRCTLR));
        }
        NV_INTR_REGW(hRmDeviceIntr, (irq / NVRM_IRQS_PER_INTRCTLR),
            INTERRUPT_SECURITY_0, reg);
    }
}

static NvU32 NvIntrTimerIrqAp20( void )
{
    return AP20_PPI_INTR_ID_TIMER_0;
}

static void NvIntrSetInterruptAp20(NvRmIntrHandle hRmDeviceIntr,
                    NvU32 irq, NvBool val)
{
    MarkInterrupt(hRmDeviceIntr, irq);
    if(val)
    {
        NV_INTR_REGW(hRmDeviceIntr, (irq / NVRM_IRQS_PER_INTRCTLR),
                        ENABLE_SET_0, (1 << (irq % NVRM_IRQS_PER_INTRCTLR)));
    }
    else
    {
        NV_INTR_REGW(hRmDeviceIntr, (irq / NVRM_IRQS_PER_INTRCTLR),
                        ENABLE_CLEAR_0, (1 << (irq % NVRM_IRQS_PER_INTRCTLR)));
    }
}

static NvU32 NvIntrDecoderAp20( NvRmIntrHandle hRmDeviceIntr )
{
    NvU32 Virq = 0;

    Virq = (NV_CIF_REGR(hRmDeviceIntr, 0, INT_ACK)
                & FIC_PROC_IF_INT_ACK_0_ACK_INTID_NO_OUTSTANDING_INTR);

    if (Virq != FIC_PROC_IF_INT_ACK_0_ACK_INTID_NO_OUTSTANDING_INTR)
    {
        /* Disable the main IRQ */
        NV_INTR_REGW(hRmDeviceIntr, (Virq / NVRM_IRQS_PER_INTRCTLR),
            ENABLE_CLEAR_0, (1 << (Virq % NVRM_IRQS_PER_INTRCTLR)));

        // FIXME: need a delay here on FPGA of 50us

        /* Done with the interrupt. So write to the EOI register. */
        NV_CIF_REGW(hRmDeviceIntr, 0, EOI, Virq);
    }
    return Virq;
}

void NvIntrGetInterfaceAp20( NvIntr *pNvIntr, NvU32 ChipId)
{
    pNvIntr->NvSetInterrupt = NvIntrSetInterruptAp20;
    pNvIntr->NvDecodeInterrupt = NvIntrDecoderAp20;
    pNvIntr->NvTimerIrq = NvIntrTimerIrqAp20;

    // Ap20 have 4 and T30 have the 5 interrupt registers.
    if (ChipId == 0x20)
        pNvIntr->NvMaxMainIntIrq = (4 * NVRM_IRQS_PER_INTRCTLR) + 32;
    else
        pNvIntr->NvMaxMainIntIrq = (5 * NVRM_IRQS_PER_INTRCTLR) + 32;
}
