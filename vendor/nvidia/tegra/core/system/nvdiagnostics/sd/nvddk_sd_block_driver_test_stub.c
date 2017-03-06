/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvddk_sd_block_driver_test.h"
#include "nvos.h"
#include "nverror.h"

NvError SdDiagInit(NvU32 Instance)
{
    NvOsDebugPrintf("SdDiagInitt not supported\n");
    return NvError_NotImplemented;
}

NvError SdDiagTest(NvU32 Type)
{
    return NvError_NotImplemented;
}

NvError SdDiagVerifyWriteRead(void)
{
    NvOsDebugPrintf("SdDiagVerifyWriteRead not supported\n");
    return NvError_NotImplemented;
}

NvError SdDiagVerifyWriteReadFull(void)
{
    NvOsDebugPrintf("SdDiagVerifyWriteReadFull not supported\n");
    return NvError_NotImplemented;
}

NvError SdDiagVerifyWriteReadSpeed(void)
{
    NvOsDebugPrintf("SdDiagVerifyWriteReadSpeed not supported\n");
    return NvError_NotImplemented;
}

NvError SdDiagVerifyWriteReadSpeed_BootArea(void)
{
    NvOsDebugPrintf("SdDiagVerifyWriteReadSpeed_BootArea not supported\n");
    return NvError_NotImplemented;
}

NvError SdDiagVerifyErase(void)
{
    NvOsDebugPrintf("SdDiagVerifyEraset not supported\n");
    return NvError_NotImplemented;
}

void SdDiagClose(void)
{
    NvOsDebugPrintf("SdDiagClose not supported\n");
}
