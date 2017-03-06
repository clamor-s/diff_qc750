/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
#include "nvcommon.h"
#include "nvintrhandler.h"
#include "nvintrhandler_apbdma.h"

static const NvU32 s_ApbDmaIrqIndex[] = {0x3A};

void
NvIntrGetApbDmaInstIrqTableAp20(
    NvIntr *pNvIntr,
    NvU32 **pIrqIndexList,
    NvU32  *pMaxIrqIndex)
{
    *pIrqIndexList = (NvU32 *)&s_ApbDmaIrqIndex[0];
    *pMaxIrqIndex = sizeof(s_ApbDmaIrqIndex)/sizeof(s_ApbDmaIrqIndex[0]);
}
