/*
 * Copyright (c) 2008-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * <b>NVIDIA Tegra ODM Kit:
 *         Abstraction layer stub for audio codec adaptations</b>
 */

#ifndef INCLUDED_NVODM_AUDIOCODEC_ADAPTATION_HAL_H
#define INCLUDED_NVODM_AUDIOCODEC_ADAPTATION_HAL_H

#include "nvcommon.h"
#include "nvodm_audiocodec.h"
#include "nvodm_query.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void *NvOdmPrivAudioCodecHandle;

typedef NvBool (*pfnAudioOpen)(NvOdmPrivAudioCodecHandle, const NvOdmQueryI2sACodecInterfaceProp *, void* hCodecNotifySem);
typedef void (*pfnAudioClose)(NvOdmPrivAudioCodecHandle);
typedef const NvOdmAudioPortCaps* (*pfnAudioGetPortCaps)(void);
typedef const NvOdmAudioOutputPortCaps* (*pfnAudioGetOutputPortCaps)(NvU32);
typedef const NvOdmAudioInputPortCaps* (*pfnAudioGetInputPortCaps)(NvU32);
typedef const NvOdmAudioWave* (*pfnAudioGetPortPcmCaps)(NvOdmPrivAudioCodecHandle, NvOdmAudioPortName, NvU32*);
typedef NvBool (*pfnAudioSetPortPcmProps)(NvOdmPrivAudioCodecHandle, NvOdmAudioPortName, NvOdmAudioWave*);
typedef void (*pfnAudioSetVolume)(NvOdmPrivAudioCodecHandle, NvOdmAudioPortName, NvOdmAudioVolume*, NvU32);
typedef void (*pfnAudioSetMuteControl)(NvOdmPrivAudioCodecHandle, NvOdmAudioPortName, NvOdmAudioMuteData*, NvU32);
typedef NvBool (*pfnAudioSetConfiguration)(NvOdmPrivAudioCodecHandle, NvOdmAudioPortName, NvOdmAudioConfigure, void*);
typedef void (*pfnAudioGetConfiguration)(NvOdmPrivAudioCodecHandle, NvOdmAudioPortName, NvOdmAudioConfigure, void*);
typedef void (*pfnAudioSetPowerState)(NvOdmPrivAudioCodecHandle, NvOdmAudioPortName, NvOdmAudioSignalType, NvU32, NvBool);
typedef void (*pfnAudioSetPowerMode)(NvOdmPrivAudioCodecHandle, NvOdmAudioCodecPowerMode);
typedef void (*pfnAudioSetOperationMode)(NvOdmPrivAudioCodecHandle, NvOdmAudioPortName, NvBool IsMaster);

typedef struct NvOdmAudioCodecRec
{
    pfnAudioOpen              pfnOpen;
    pfnAudioClose             pfnClose;
    pfnAudioGetPortCaps       pfnGetPortCaps;
    pfnAudioGetOutputPortCaps pfnGetOutputPortCaps;
    pfnAudioGetInputPortCaps  pfnGetInputPortCaps;
    pfnAudioGetPortPcmCaps    pfnGetPortPcmCaps;
    pfnAudioSetPortPcmProps   pfnSetPortPcmProps;
    pfnAudioSetVolume         pfnSetVolume;
    pfnAudioSetMuteControl    pfnSetMuteControl;
    pfnAudioSetConfiguration  pfnSetConfiguration;
    pfnAudioGetConfiguration  pfnGetConfiguration;
    pfnAudioSetPowerState     pfnSetPowerState;
    pfnAudioSetPowerMode      pfnSetPowerMode;
    pfnAudioSetOperationMode  pfnSetOperationMode;

    NvOdmPrivAudioCodecHandle hCodecPrivate;
    NvBool                    IsInited;
} NvOdmAudioCodec;

void AD1937InitCodecInterface(NvOdmAudioCodec *pCodec);

#ifdef __cplusplus
}
#endif

#endif
