/*
 * Copyright (c) 2008-2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "ap20/nvboot_bit.h"
#include "ap20/nvboot_version.h"
#include "nvassert.h"
#include "nvos.h"
#include "nvbit_private.h"

static NvBool NvBitPrivIsValidBitAp20(
    NvBootInfoTable *pBit)
{
    if (((pBit->BootRomVersion == NVBOOT_VERSION(2, 3))
      || (pBit->BootRomVersion == NVBOOT_VERSION(2, 2))
      || (pBit->BootRomVersion == NVBOOT_VERSION(2, 1)))
    &&   (pBit->DataVersion    == NVBOOT_VERSION(2, 1))
    &&   (pBit->RcmVersion     == NVBOOT_VERSION(2, 1))
    &&   (pBit->PrimaryDevice  == NvBootDevType_Irom))
    {
        // There is a valid Boot Information Table.
        return NV_TRUE;
    }

    return NV_FALSE;
}

NvError NvBitPrivGetDataSizeAp20(
    NvBitHandle hBit,
    NvBitDataType DataType,
    NvU32 *Size,
    NvU32 *NumInstances)
{
    NV_ASSERT(hBit);
    NV_ASSERT(Size);
    NV_ASSERT(NumInstances);
    switch(DataType)
    {
        case NvBitDataType_BlStatus:
            *Size = sizeof(NvU32);
            *NumInstances = NVBOOT_MAX_BOOTLOADERS;
            break;

        case NvBitDataType_BlUsedForEccRecovery:
            *Size = sizeof(NvU32);
            *NumInstances = NVBOOT_MAX_BOOTLOADERS;
            break;

        default:
            return NvError_NotImplemented;
    }
    return NvSuccess;
}

static NvError NvBitPrivGetDataAp20(
    NvBitHandle hBit,
    NvBitDataType DataType,
    NvU32 *Size,
    NvU32 *Instance,
    void *Data)
{
    if (!hBit)
        return NvError_BadParameter;

    if (!Size || !Instance)
        return NvError_InvalidAddress;

    if (!Data)
        return NvError_InvalidAddress;

    switch(DataType)
    {
        default:
            return NvError_NotImplemented;
    }
    //return NvSuccess;
}

#define NVBIT_SOC(FN)  FN##Ap20
#define NV_BIT_LOCATION 0x40000000UL
#include "nvbit_common.c"
#undef NVBIT_LOCATION
#undef NVBIT_SOC

