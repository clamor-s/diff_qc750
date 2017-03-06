/*
 * Copyright (c) 2009-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvcommon.h"
#include "ap20/arictlr.h"
#include "ap20/artimer.h"
#include "ap20/arclk_rst.h"
#include "nvrm_arm_cp.h"
#include "nvintrhandler.h"
#include "nvrm_module.h"
#include "nvrm_drf.h"
#include "nvrm_processor.h"

#define PPI_INTR_ID_TIMER_0     (0)

#define INTERRUPT_NUM_CONTROLLERS 3

#define NV_INTR_REGR(rm,inst,reg) \
    NV_REGR(rm, NVRM_MODULE_ID( NvRmPrivModuleID_Interrupt, inst ), \
        ICTLR_##reg##_0 )

#define NV_INTR_REGW(rm,inst,reg,data) \
    NV_REGW(rm, NVRM_MODULE_ID( NvRmPrivModuleID_Interrupt, inst ), \
        ICTLR_##reg##_0, data)

static NvU32 NvIntrTimerIrqSim( void )
{
    return PPI_INTR_ID_TIMER_0;
}

static void NvIntrSetInterruptSim( NvRmIntrHandle hRmDevice,
                    NvU32 irq, NvBool val )
{
    if( irq == 0xFFFF || irq >= 160 )
    {
        NV_ASSERT(0);
        return;
    }

    if(val)
    {
        NV_INTR_REGW(hRmDevice, (irq / NVRM_IRQS_PER_INTRCTLR),
                            CPU_IER_SET, (1 << (irq % NVRM_IRQS_PER_INTRCTLR)));
    }
    else
    {
        NV_INTR_REGW(hRmDevice, (irq / NVRM_IRQS_PER_INTRCTLR),
                            CPU_IER_CLR, (1 << (irq % NVRM_IRQS_PER_INTRCTLR)));

    }
}

static NvU32 NvIntrDecoderSim( NvRmIntrHandle hRmDevice )
{
    NvU32 Virq = 0;
    NvU32 instance = 0;

    for (instance = 0;instance < INTERRUPT_NUM_CONTROLLERS; instance++)
    {
        Virq = NV_INTR_REGR(hRmDevice, instance, VIRQ_CPU);
        if (Virq)
        {
            Virq = 31 - CountLeadingZeros(Virq);
            NV_INTR_REGW(hRmDevice,((Virq+(NVRM_IRQS_PER_INTRCTLR * instance))
                                / NVRM_IRQS_PER_INTRCTLR), CPU_IER_CLR, 1 << Virq);
            Virq += (NVRM_IRQS_PER_INTRCTLR * instance);
            return Virq;
        }
    }
    return NVRM_IRQ_INVALID;
}

void NvIntrGetInterfaceSim(NvIntr *pNvIntr, NvU32 ChipId)
{
    pNvIntr->NvSetInterrupt = NvIntrSetInterruptSim;
    pNvIntr->NvDecodeInterrupt = NvIntrDecoderSim;
    pNvIntr->NvTimerIrq = NvIntrTimerIrqSim;
    pNvIntr->NvMaxMainIntIrq = INTERRUPT_NUM_CONTROLLERS * NVRM_IRQS_PER_INTRCTLR;
}
