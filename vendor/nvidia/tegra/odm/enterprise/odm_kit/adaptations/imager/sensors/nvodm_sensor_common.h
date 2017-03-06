/*
 * Copyright (c) 2007-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVODM_SENSOR_COMMON_H
#define INCLUDED_NVODM_SENSOR_COMMON_H

#if defined(__cplusplus)
extern "C"
{
#endif

typedef struct SMIAScalerSettingsRec
{
    NvU16 FrameLengthLines;
    NvU16 LineLengthPck;
    NvU16 XAddrStart;
    NvU16 YAddrStart;
    NvU16 XAddrEnd;
    NvU16 YAddrEnd;
    NvU16 XOutputSize;
    NvU16 YOutputSize;
    NvU8  XOddInc;
    NvU8  YOddInc;
    NvU8  ScalingMode;
    NvU8  ScaleM;
} SMIAScalerSettings;

#if defined(__cplusplus)
}
#endif

/** @} */

#endif // INCLUDED_NVODM_SENSOR_MI5130_H

