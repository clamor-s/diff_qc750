/*
 * Copyright (c) 2008 - 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file nvbit_private.h
 * @brief <b> Nv Boot Information Table Management Framework.</b>
 *
 * @b Description: This file declares chip specific private extension API's for interacting with
 *    the Boot Information Table (BIT).
 */

#ifndef INCLUDED_NVBIT_PRIVATE_H
#define INCLUDED_NVBIT_PRIVATE_H

#include "nvbit.h"

#if defined(__cplusplus)
extern "C"
{
#endif

NvError NvBitInitAp15(NvBitHandle *phBit);
NvError NvBitInitAp20(NvBitHandle *phBit);
NvError NvBitInitT30(NvBitHandle *phBit);

NvError 
NvBitGetDataAp15(
    NvBitHandle hBit,
    NvBitDataType DataType,
    NvU32 *Size,
    NvU32 *Instance,
    void *Data);
NvError 
NvBitGetDataAp20(
    NvBitHandle hBit,
    NvBitDataType DataType,
    NvU32 *Size,
    NvU32 *Instance,
    void *Data);
NvError
NvBitGetDataT30(
    NvBitHandle hBit,
    NvBitDataType DataType,
    NvU32 *Size,
    NvU32 *Instance,
    void *Data);

NvError 
NvBitPrivGetDataSizeAp15(
    NvBitHandle hBit,
    NvBitDataType DataType,
    NvU32 *Size,
    NvU32 *NumInstances);
NvError
NvBitPrivGetDataSizeAp20(
    NvBitHandle hBit,
    NvBitDataType DataType,
    NvU32 *Size,
    NvU32 *NumInstances);

NvError
NvBitPrivGetDataSizeT30(
    NvBitHandle hBit,
    NvBitDataType DataType,
    NvU32 *Size,
    NvU32 *NumInstances);

#if defined(__cplusplus)
}
#endif  /* __cplusplus */

#endif // INCLUDED_NVBIT_PRIVATE_H


