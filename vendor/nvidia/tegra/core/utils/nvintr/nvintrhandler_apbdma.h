/*
 * Copyright (c) 2009-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVINTRHANDLER_APBDMA_H
#define INCLUDED_NVINTRHANDLER_APBDMA_H

void
NvIntrGetApbDmaInstIrqTableAp20(
    NvIntr *pNvIntr,
    NvU32 **pIrqIndexList,
    NvU32 *pMaxIrqIndex);

void
NvIntrGetApbDmaInstIrqTableT30(
    NvIntr *pNvIntr,
    NvU32 **pIrqIndexList,
    NvU32 *pMaxIrqIndex);

#endif//INCLUDED_NVINTRHANDLER_APBDMA_H
