/*
 * Copyright (c) 2009-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
#include "nvcommon.h"
#include "nvassert.h"
#include "nvintrhandler.h"
#include "nvintrhandler_apbdma.h"
#include "nvrm_module.h"
#include "nvrm_interrupt.h"
#include "ap20/arapbdma.h"
#include "nvrm_processor.h"

static NvIntrSubIrq ApbDmaInst = {0x0,0x0, 0x0};
static NvU32 *s_pApbDmaIntrIrqIndex = NULL;
static NvU32 s_MaxApbDmaIrqEntry = 0;
static NvBool s_IsChannelBasedIntRegistration =  NV_FALSE;

static void InitApbDma(NvRmIntrHandle hRmDevice)
{
    ApbDmaInst.Instances = NvRmModuleGetNumInstances(hRmDevice,
                                                NvRmPrivModuleID_ApbDma);
    ApbDmaInst.LowerIrqIndex = NvRmGetIrqForLogicalInterrupt(hRmDevice,
                    NVRM_MODULE_ID(NvRmPrivModuleID_ApbDma, 0), 0);
    ApbDmaInst.UpperIrqIndex = ApbDmaInst.LowerIrqIndex + ApbDmaInst.Instances
                                        - 1;
}

static void
ApbDmaGetHighLowIrqIndex(
    NvRmIntrHandle hRmDevice,
    NvU32 *pLowerIrq,
    NvU32 *pUpperIrq)
{
    *pLowerIrq = ApbDmaInst.LowerIrqIndex;
    *pUpperIrq = ApbDmaInst.UpperIrqIndex;
}

void NvIntrGetApbDmaInterface(NvIntr *pNvIntr, NvU32 Id)
{
    InitApbDma(pNvIntr->hRmIntrDevice);
    pNvIntr->NvApbDmaGetLowHighIrqIndex = ApbDmaGetHighLowIrqIndex;

    if (Id == 0x20)
        NvIntrGetApbDmaInstIrqTableAp20(pNvIntr, &s_pApbDmaIntrIrqIndex,
                                                        &s_MaxApbDmaIrqEntry);
    else
        NvIntrGetApbDmaInstIrqTableT30(pNvIntr, &s_pApbDmaIntrIrqIndex,
                                                        &s_MaxApbDmaIrqEntry);
    s_IsChannelBasedIntRegistration =  NV_FALSE;

}
