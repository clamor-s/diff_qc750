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
#include "nvintrhandler_gpio.h"

/**
 * Defines Ap20 GPIO interrupt locations.
 */
typedef enum
{
    NvIrqGpioAp20_0 = 0x40,
    NvIrqGpioAp20_1 = 0x41,
    NvIrqGpioAp20_2 = 0x42,
    NvIrqGpioAp20_3 = 0x43,
    NvIrqGpioAp20_4 = 0x57,
    NvIrqGpioAp20_5 = 0x77,
    NvIrqGpioAp20_6 = 0x79,
} NvIrqGpioAp20;

static const NvU32 s_IrqIndex[] = {NvIrqGpioAp20_0, NvIrqGpioAp20_1, NvIrqGpioAp20_2,
                                    NvIrqGpioAp20_3, NvIrqGpioAp20_4, NvIrqGpioAp20_5,
                                    NvIrqGpioAp20_6};

void NvIntrGetGpioInstIrqTableAp20(NvU32 **pIrqIndexList, NvU32 *pMaxIrqIndex)
{

    *pIrqIndexList = (NvU32 *)&s_IrqIndex[0];
    *pMaxIrqIndex = sizeof(s_IrqIndex)/sizeof(s_IrqIndex[0]);
}
