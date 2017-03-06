/*
 * Copyright (c) 2008-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvodm_audiocodec.h"
#include "audiocodec_hal.h"
#include "nvodm_services.h"
#include "nvodm_query_discovery.h"
#define NUM_CODECS 1

#define INVALID_HANDLE ((NvOdmAudioCodecHandle)0)
#define HANDLE(x) (NvOdmAudioCodecHandle)(((x)&0x7fffffffUL) | 0x80000000UL)
#define IS_VALID_HANDLE(x) ( ((NvU32)(x)) & 0x80000000UL )
#define HANDLE_INDEX(x) (((NvU32)(x)) & 0x7fffffffUL)

#define AD1937_GUID NV_ODM_GUID('a','d','e','v','1','9','3','7')
#define MMSE_CODEC_GUID NV_ODM_GUID('x','x','d','i','r','a','n','a')

//Find out if the codec exists @ 0x0E
//HACK is needed because p852 config is used on LUPIN/SMEG and e1155 configs.
//The codecs/i2s interfaces used will be different.
static NvBool IsE1155Config()
{
    NvOdmI2cStatus I2cTransStatus = NvOdmI2cStatus_Timeout;
    NvOdmI2cTransactionInfo TransactionInfo[2];
    NvOdmServicesI2cHandle hOdmService;
    NvU32 I2CInstance =0;
    NvU8 ReadBuffer[2] = {0};
    NvU32 RegIndex = 0;
    hOdmService = NvOdmI2cOpen(NvOdmIoModule_I2c,I2CInstance);
    ReadBuffer[0] = (1 & 0xFF); //Read 1st offset

    TransactionInfo[0].Address = 0xE;
    TransactionInfo[0].Buf = ReadBuffer;
    TransactionInfo[0].Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo[0].NumBytes = 1;
    TransactionInfo[1].Address = (0xE | 0x1);
    TransactionInfo[1].Buf = ReadBuffer;
    TransactionInfo[1].Flags = 0;
    TransactionInfo[1].NumBytes = 1;
/* I2C Settings */
#define I2C_TIMEOUT   1000    // 1 seconds
#define I2C_SCLK      100     // 400KHz
    // Read data from PMU at the specified offset
    I2cTransStatus = NvOdmI2cTransaction(hOdmService, &TransactionInfo[0], 2,
            I2C_SCLK, I2C_TIMEOUT);
    return (NvBool)(I2cTransStatus == 0);
}

static NvOdmAudioCodec*
GetCodecByPseudoHandle(NvOdmAudioCodecHandle PseudoHandle)
{
    static NvOdmAudioCodec Codecs[NUM_CODECS];
    static NvBool first = NV_TRUE;

    if (first)
    {
        NvOdmOsMemset(Codecs, 0, sizeof(Codecs));
        first = NV_FALSE;
    }

    if (IS_VALID_HANDLE(PseudoHandle) &&
        (HANDLE_INDEX(PseudoHandle)<NUM_CODECS))
        return &Codecs[HANDLE_INDEX(PseudoHandle)];

    return NULL;
}

static NvOdmAudioCodec*
GetCodecByGuid(NvU64 OdmGuid)
{
    NvOdmAudioCodec *codec = NULL;
    codec = GetCodecByPseudoHandle(HANDLE(0));
    if (!codec->IsInited)
    {
        switch (OdmGuid)
        {
            case AD1937_GUID:
                AD1937InitCodecInterface(codec);
                break;
            case MMSE_CODEC_GUID:
                DiranaInitCodecInterface(codec);
                break;

        }
    }
    return codec;
}

static NvU64
GetCodecGuidByInstance(NvU32 InstanceId)
{
    static const NvU64 AudioCodecGuidList[] = {
        AD1937_GUID,
        MMSE_CODEC_GUID
    };

    NV_ASSERT(InstanceId < sizeof(AudioCodecGuidList)/sizeof(NvU64));

    if(!IsE1155Config())
        return AudioCodecGuidList[1];
    else
        return AudioCodecGuidList[0];
}

const NvOdmAudioPortCaps*
NvOdmAudioCodecGetPortCaps(NvU32 InstanceId)
{
    NvOdmAudioCodec *codec = GetCodecByGuid(GetCodecGuidByInstance(InstanceId));

    if (codec && codec->pfnGetPortCaps)
        return codec->pfnGetPortCaps();

    return NULL;
}

const NvOdmAudioOutputPortCaps*
NvOdmAudioCodecGetOutputPortCaps(
    NvU32 InstanceId,
    NvU32 OutputPortId)
{
    NvOdmAudioCodec *codec = GetCodecByGuid(GetCodecGuidByInstance(InstanceId));

    if (codec && codec->pfnGetOutputPortCaps)
        return codec->pfnGetOutputPortCaps(OutputPortId);

    return NULL;
}

const NvOdmAudioInputPortCaps*
NvOdmAudioCodecGetInputPortCaps(
    NvU32 InstanceId,
    NvU32 InputPortId)
{
    NvOdmAudioCodec *codec = GetCodecByGuid(GetCodecGuidByInstance(InstanceId));

    if (codec && codec->pfnGetInputPortCaps)
        return codec->pfnGetInputPortCaps(InputPortId);

    return NULL;
}

NvOdmAudioCodecHandle
NvOdmAudioCodecOpen(NvU32 InstanceId, void* hCodecNotifySem)
{
    NvOdmAudioCodec *codec = GetCodecByGuid(GetCodecGuidByInstance(InstanceId));
    const NvOdmQueryI2sACodecInterfaceProp *pI2sCodecInt = NvOdmQueryGetI2sACodecInterfaceProperty(InstanceId);
    if (codec && codec->pfnOpen && pI2sCodecInt)
    {
        if(codec->pfnOpen(codec->hCodecPrivate, pI2sCodecInt, hCodecNotifySem))
            return HANDLE(InstanceId);
    }

    return INVALID_HANDLE;
}


void
NvOdmAudioCodecClose(NvOdmAudioCodecHandle hAudioCodec)
{
    NvOdmAudioCodec *codec = GetCodecByPseudoHandle(hAudioCodec);

    if (codec && codec->pfnClose)
        codec->pfnClose(codec->hCodecPrivate);
}

const NvOdmAudioWave*
NvOdmAudioCodecGetPortPcmCaps(
    NvOdmAudioCodecHandle hAudioCodec,
    NvOdmAudioPortName PortName,
    NvU32 *pValidListCount)
{
    NvOdmAudioCodec *codec = GetCodecByPseudoHandle(hAudioCodec);

    if (codec && codec->pfnGetPortPcmCaps)
        return codec->pfnGetPortPcmCaps(codec->hCodecPrivate, PortName, pValidListCount);

    return NULL;

}

NvBool
NvOdmAudioCodecSetPortPcmProps(
    NvOdmAudioCodecHandle hAudioCodec,
    NvOdmAudioPortName PortName,
    NvOdmAudioWave *pWaveParams)
{
    NvOdmAudioCodec *codec = GetCodecByPseudoHandle(hAudioCodec);

    if (codec && codec->pfnSetPortPcmProps)
        return codec->pfnSetPortPcmProps(codec->hCodecPrivate, PortName, pWaveParams);

    return NV_FALSE;
}

void
NvOdmAudioCodecSetVolume(
    NvOdmAudioCodecHandle hAudioCodec,
    NvOdmAudioPortName PortName,
    NvOdmAudioVolume *pVolume,
    NvU32 ListCount)
{
    NvOdmAudioCodec *codec = GetCodecByPseudoHandle(hAudioCodec);

    if (codec && codec->pfnSetVolume)
        codec->pfnSetVolume(codec->hCodecPrivate, PortName, pVolume, ListCount);
}

 void
 NvOdmAudioCodecSetMuteControl(
    NvOdmAudioCodecHandle hAudioCodec,
    NvOdmAudioPortName PortName,
    NvOdmAudioMuteData *pMuteParam,
    NvU32 ListCount)
 {
     NvOdmAudioCodec *codec = GetCodecByPseudoHandle(hAudioCodec);

     if (codec && codec->pfnSetMuteControl)
         codec->pfnSetMuteControl(codec->hCodecPrivate, PortName, pMuteParam, ListCount);
 }

 NvBool
 NvOdmAudioCodecSetConfiguration(
    NvOdmAudioCodecHandle hAudioCodec,
    NvOdmAudioPortName PortName,
    NvOdmAudioConfigure ConfigType,
    void *pConfigData)
 {
     NvOdmAudioCodec *codec = GetCodecByPseudoHandle(hAudioCodec);

     if (codec && codec->pfnSetConfiguration)
         return codec->pfnSetConfiguration(codec->hCodecPrivate, PortName, ConfigType, pConfigData);

     return NV_FALSE;
 }

void
 NvOdmAudioCodecGetConfiguration(
    NvOdmAudioCodecHandle hAudioCodec,
    NvOdmAudioPortName PortName,
    NvOdmAudioConfigure ConfigType,
    void *pConfigData)
{
    NvOdmAudioCodec *codec = GetCodecByPseudoHandle(hAudioCodec);

    if (codec && codec->pfnGetConfiguration)
        codec->pfnGetConfiguration(codec->hCodecPrivate, PortName, ConfigType, pConfigData);

}

void
NvOdmAudioCodecSetPowerState(
    NvOdmAudioCodecHandle hAudioCodec,
    NvOdmAudioPortName PortName,
    NvOdmAudioSignalType SignalType,
    NvU32 SignalId,
    NvBool IsPowerOn)
{
    NvOdmAudioCodec *codec = GetCodecByPseudoHandle(hAudioCodec);

    if (codec && codec->pfnSetPowerState)
        codec->pfnSetPowerState(codec->hCodecPrivate, PortName, SignalType, SignalId, IsPowerOn);
}

void
NvOdmAudioCodecSetPowerMode(
    NvOdmAudioCodecHandle hAudioCodec,
    NvOdmAudioCodecPowerMode PowerMode)
{
    NvOdmAudioCodec *codec = GetCodecByPseudoHandle(hAudioCodec);

    if (codec && codec->pfnSetPowerMode)
        codec->pfnSetPowerMode(codec->hCodecPrivate, PowerMode);
}

void
NvOdmAudioCodecSetOperationMode(
    NvOdmAudioCodecHandle hAudioCodec,
    NvOdmAudioPortName PortName,
    NvBool IsMaster)
{
    NvOdmAudioCodec *codec = GetCodecByPseudoHandle(hAudioCodec);

    if (codec && codec->pfnSetOperationMode)
        codec->pfnSetOperationMode(codec->hCodecPrivate, PortName, IsMaster);
}
