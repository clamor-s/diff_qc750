/*
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_CRC32_H
#define INCLUDED_CRC32_H

#if defined(__cplusplus)
extern "C"
{
#endif

#include "nvcommon.h"
/**
 * Computes CRC32 value of the submitted buffer
 *
 * @params CrcInitVal Initial CRC value (If Any) otherwise zero.
 * @params buf Buffer for which the CRC32 needs to be calculated
 * @params size Size of the buffer
 *
 * @retval Returns the CRC32 value of the buffer
 */
NvU32 NvComputeCrc32(NvU32 CrcInitVal, const NvU8 *buf, NvU32 size);

#if defined(__cplusplus)
}
#endif

#endif // INCLUDED_CRC32_H

