/*
 * Copyright (c) 2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
#ifndef CODEC_H
#define CODEC_H

#include "nvos.h"

/**
 * Codec
 */
typedef struct tagCodec* Codec;

/**
 * Codec mode
 */
typedef enum tagCodecMode
{
    CodecMode_None    = 0,
    CodecMode_I2s1    = 1,
    CodecMode_I2s2    = 2,
    CodecMode_Tdm1    = 3,
    CodecMode_Tdm2    = 4,
    CodecMode_Force32 = 0x7FFFFFFF
} CodecMode;

/**
 * Open Codec
 */
NvError
CodecOpen(
    Codec*    phCodec,
    CodecMode mode,
    int       sampleRate
    );

/**
 * Close Codec
 */
void
CodecClose(
    Codec hCodec
    );

/**
 * Dump Codec Registers
 */
NvError
CodecDump(
    Codec hCodec
    );

#endif /* CODEC_H */
