 /**
 * Copyright (c) 2007-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/*
 * NVIDIA APX ODM Kit for Wolfson 8903 Implementation
 *
 * Note: Please search "FIXME" for un-implemented part/might_be_wrong part
 */



#include "nvodm_services.h"
#include "nvodm_query.h"
#include "nvodm_audiocodec.h"
#include "audiocodec_hal.h"
#include "nvodm_query_discovery.h"

#define DIRANA_CODEC_GUID NV_ODM_GUID('x','x','d','i','r','a','n','a')

typedef NvU32 WCodecRegIndex;

typedef struct DiranaAudioCodecRec
{
    NvOdmServicesI2cHandle hOdmService;
    NvOdmQueryI2sACodecInterfaceProp WCodecInterface;
    NvU32   RegValues[173];
} DiranaAudioCodec, *DiranaAudioCodecHandle;

static DiranaAudioCodec s_Dirana = {0};       /* Unique audio codec for the whole system*/

static NvBool InitializeDigitalInterface(NvOdmPrivAudioCodecHandle hOdmAudioCodec);
static NvBool InitializeAnalogAudioPath(NvOdmPrivAudioCodecHandle hOdmAudioCodec);
static NvBool EnableInput( DiranaAudioCodecHandle hOdmAudioCodec, NvU32  IsMic, NvBool IsPowerOn);
static NvBool SetClockSourceOnCodec(NvOdmPrivAudioCodecHandle hOdmAudioCodec, NvBool IsEnable);

static NvBool
ReadDiranaRegister(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    WCodecRegIndex RegIndex,
    NvU32 *Data)
{
    *Data = 0;
    return (NvBool)(NV_TRUE);
}

static NvBool
WriteDiranaRegister(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    WCodecRegIndex RegIndex,
    NvU32 Data)
{
    return (NvBool)(NV_TRUE);
}

static NvBool
Dirana_ReadRegister(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    WCodecRegIndex RegIndex,
    NvU32 *Data)
{
    return ReadDiranaRegister(hOdmAudioCodec, RegIndex, Data);
}

static NvBool
Dirana_WriteRegister(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    WCodecRegIndex RegIndex,
    NvU32 Data)
{
    return WriteDiranaRegister(hOdmAudioCodec, RegIndex, Data);
}


static NvBool
SetHeadphoneOutVolume(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvU32 Channel,
    NvU32 LeftVolume,
    NvU32 RightVolume)
{
    NvBool IsSuccess = NV_TRUE;
    return IsSuccess;
}

static NvBool
SetLineInVolume(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvU32 Channel,
    NvU32 LeftVolume,
    NvU32 RightVolume)
{
    /* There are no registers to support this */
    return NV_TRUE;
}


static NvBool
SetHeadphoneOutMute(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvU32 Channel,
    NvBool IsMute)
{
    NvBool IsSuccess = NV_TRUE;
    return IsSuccess;
}


static NvBool
SetLineInMute(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvU32 Channel,
    NvBool IsMute)
{
    NvBool IsSuccess = NV_TRUE;
    return IsSuccess;
}

static NvBool SetMicrophoneInVolume(NvOdmPrivAudioCodecHandle hOdmAudioCodec, NvU32 MicGain)
{
    NvBool IsSuccess = NV_TRUE;
    return IsSuccess;
}

static NvBool SetMicrophoneInMute(NvOdmPrivAudioCodecHandle hOdmAudioCodec, NvBool IsMute)
{
    return NV_TRUE;
}

/*
 * Shuts down the audio codec driver.
 */
static NvBool ShutDownCodec(NvOdmPrivAudioCodecHandle hOdmAudioCodec)
{
    /* Quick Shutdown */
    NvBool IsSuccess=NV_TRUE;
    SetClockSourceOnCodec(hOdmAudioCodec,NV_FALSE);
    return IsSuccess;

}



static NvBool ResetCodec(NvOdmPrivAudioCodecHandle hOdmAudioCodec)
{
    /* Reset 8903 */
    NvBool IsSuccess=NV_TRUE;
    return IsSuccess;
}

static NvBool SetCodecPower(NvOdmPrivAudioCodecHandle hOdmAudioCodec, NvBool IsPowerOn)
{
    NvBool IsSuccess = NV_TRUE;
    return IsSuccess;
}


static NvBool InitializeDigitalInterface(NvOdmPrivAudioCodecHandle hOdmAudioCodec)
{
    NvBool IsSuccess = NV_TRUE;
    return IsSuccess;
}

static NvBool InitializeHeadphoneOut(NvOdmPrivAudioCodecHandle hOdmAudioCodec)
{
    NvBool IsSuccess = NV_TRUE;
    return IsSuccess;
}


static NvBool InitializeAnalogAudioPath(NvOdmPrivAudioCodecHandle hOdmAudioCodec)
{
    NvBool IsSuccess = NV_TRUE;
    return IsSuccess;
}

static NvBool SetDacMute(NvOdmPrivAudioCodecHandle hOdmAudioCodec, NvBool IsMute)
{
    return SetHeadphoneOutMute(hOdmAudioCodec,0,IsMute);
}

/*
 * Sets the PCM size for the audio data.
 */
static NvBool
SetPcmSize(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioPortType PortType,
    NvU32 PcmSize)
{
    NvBool IsSuccess = NV_TRUE;
    return IsSuccess;
}

/*
 * Sets the sampling rate for the audio playback and record path.
 */
static NvBool
SetSamplingRate(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioPortType PortType,
    NvU32 SamplingRate)
{
    NvBool IsSuccess = NV_TRUE;
    return IsSuccess;
}

/*
 * Sets the input analog line to the input to ADC.
 */
static NvBool
SetInputPortIO(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvU32 PortId,
    NvOdmAudioConfigInOutSignal *pConfigIO)
{
    NvBool IsSuccess = NV_TRUE;
    return IsSuccess;
}

/*
 * Sets the output analog line from the DAC.
 */
static NvBool
SetOutputPortIO(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvU32 PortId,
    NvOdmAudioConfigInOutSignal *pConfigIO)
{
    NvBool IsSuccess = NV_TRUE;
    return IsSuccess;
}

/*
 * Sets the codec bypass.
 */
static NvBool
SetBypass(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioSignalType InputAnalogLineType,
    NvU32 InputLineId,
    NvOdmAudioSignalType OutputAnalogLineType,
    NvU32 OutputLineId,
    NvU32 IsEnable)
{
    /* Do Codec Bypass */
    return NV_TRUE;
}


/*
 * Sets the side attenuation.
 */
static NvBool
SetSideToneAttenuation(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvU32 SideToneAtenuation,
    NvBool IsEnabled)
{
    return NV_TRUE;
}

/*
 * Sets the power status of the specified devices of the audio codec.
 */
static NvBool
SetPowerStatus(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioSignalType SignalType,
    NvU32 SignalId,
    NvOdmAudioSpeakerType Channel,
    NvBool IsPowerOn)
{
    return NV_TRUE;
}

static NvBool OpenDiranaCodec(NvOdmPrivAudioCodecHandle hOdmAudioCodec)
{
    NvBool IsSuccess = NV_TRUE;

    //I2C programming will be done by MMSE
    SetClockSourceOnCodec(hOdmAudioCodec,NV_TRUE);

    return IsSuccess;
}

static const NvOdmAudioPortCaps * ACDiranaGetPortCaps(void)
{
    static const NvOdmAudioPortCaps s_PortCaps =
        {
            1,    // MaxNumberOfInputPort;
            1     // MaxNumberOfOutputPort;
        };
    return &s_PortCaps;
}


static const NvOdmAudioOutputPortCaps *
ACDiranaGetOutputPortCaps(
    NvU32 OutputPortId)
{
    static const NvOdmAudioOutputPortCaps s_OutputPortCaps =
            {
                1,          // MaxNumberOfDigitalInput;
                1,          // MaxNumberOfLineOut;
                1,          // MaxNumberOfHeadphoneOut;
                2,          // MaxNumberOfSpeakers;
                NV_FALSE,   // IsShortCircuitCurrLimitSupported;
                NV_FALSE,   // IsNonLinerVolumeSupported;
                0,          // MaxVolumeInMilliBel;
                0,          // MinVolumeInMilliBel;
            };

    if (OutputPortId == 0)
        return &s_OutputPortCaps;
    return NULL;
}


static const NvOdmAudioInputPortCaps *
ACDiranaGetInputPortCaps(
    NvU32 InputPortId)
{
    static const NvOdmAudioInputPortCaps s_InputPortCaps =
            {
                3,          // MaxNumberOfLineIn;
                1,          // MaxNumberOfMicIn;
                0,          // MaxNumberOfCdIn;
                0,          // MaxNumberOfVideoIn;
                0,          // MaxNumberOfMonoInput;
                1,          // MaxNumberOfOutput;
                NV_FALSE,   // IsSideToneAttSupported;
                NV_FALSE,   // IsNonLinerGainSupported;
                0,          // MaxVolumeInMilliBel;
                0           // MinVolumeInMilliBel;
            };

    if (InputPortId == 0)
        return &s_InputPortCaps;
    return NULL;
}


static NvBool ACDiranaOpen(NvOdmPrivAudioCodecHandle hOdmAudioCodec,
                        const NvOdmQueryI2sACodecInterfaceProp *pI2sCodecInt,
                        void* hCodecNotifySem )
{
    OpenDiranaCodec(hOdmAudioCodec);
    return NV_TRUE;
}

static void ACDiranaClose(NvOdmPrivAudioCodecHandle hOdmAudioCodec)
{
    ShutDownCodec(hOdmAudioCodec);
}


static const NvOdmAudioWave *
ACDiranaGetPortPcmCaps(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioPortName PortName,
    NvU32 *pValidListCount)
{
    static const NvOdmAudioWave s_AudioPcmProps[] =
    {
        // NumberOfChannels;
        // IsSignedData;
        // IsLittleEndian;
        // IsInterleaved;
        // NumberOfBitsPerSample;
        // SamplingRateInHz;
        // DatabitsPerLRCLK
        // DataFormat
        // NvOdmAudioWaveFormat FormatType;
        {2, NV_FALSE, NV_FALSE, NV_FALSE, 16, 8000, 32, NvOdmQueryI2sDataCommFormat_I2S, NvOdmAudioWaveFormat_Pcm},
        {2, NV_FALSE, NV_FALSE, NV_FALSE, 20, 32000, 32, NvOdmQueryI2sDataCommFormat_I2S, NvOdmAudioWaveFormat_Pcm},
        {2, NV_FALSE, NV_FALSE, NV_FALSE, 24, 44100, 32, NvOdmQueryI2sDataCommFormat_I2S, NvOdmAudioWaveFormat_Pcm},
        {2, NV_FALSE, NV_FALSE, NV_FALSE, 32, 48000, 32, NvOdmQueryI2sDataCommFormat_I2S, NvOdmAudioWaveFormat_Pcm},
        {2, NV_FALSE, NV_FALSE, NV_FALSE, 32, 88200, 32, NvOdmQueryI2sDataCommFormat_I2S, NvOdmAudioWaveFormat_Pcm},
        {2, NV_FALSE, NV_FALSE, NV_FALSE, 32, 96000, 32, NvOdmQueryI2sDataCommFormat_I2S, NvOdmAudioWaveFormat_Pcm},
    };

    *pValidListCount = sizeof(s_AudioPcmProps)/sizeof(s_AudioPcmProps[0]);
    return &s_AudioPcmProps[0];
}

static NvBool
ACDiranaSetPortPcmProps(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioPortName PortName,
    NvOdmAudioWave *pWaveParams)
{
    return NV_TRUE;
}

static void
ACDiranaSetVolume(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioPortName PortName,
    NvOdmAudioVolume *pVolume,
    NvU32 ListCount)
{
}


static void
ACDiranaSetMuteControl(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioPortName PortName,
    NvOdmAudioMuteData *pMuteParam,
    NvU32 ListCount)
{
}


static NvBool
ACDiranaSetConfiguration(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioPortName PortName,
    NvOdmAudioConfigure ConfigType,
    void *pConfigData)
{
    return NV_TRUE;
}

static void
ACDiranaGetConfiguration(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioPortName PortName,
    NvOdmAudioConfigure ConfigType,
    void *pConfigData)
{
}

static NvBool EnableInput(
        DiranaAudioCodecHandle hOdmAudioCodec,
        NvU32  IsMic,  /* 0 for LineIn, 1 for Mic */
        NvBool IsPowerOn)
{
    NvBool IsSuccess = NV_TRUE;
    NvU32  CtrlReg = 0;
    /* What else needs to be done to enable?, Figureout and add when necessary*/
    return IsSuccess;
}

static void
ACDiranaSetPowerState(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioPortName PortName,
    NvOdmAudioSignalType SignalType,
    NvU32 SignalId,
    NvBool IsPowerOn)
{
    //NvOsDebugPrintf("Set Power State %d \n",IsPowerOn);
}

static void
ACDiranaSetPowerMode(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioCodecPowerMode PowerMode)
{
    //NvOsDebugPrintf("Set Power Mode %d \n",PowerMode);
}

void DiranaInitCodecInterface(NvOdmAudioCodec *pCodecInterface)
{
    pCodecInterface->pfnGetPortCaps = ACDiranaGetPortCaps;
    pCodecInterface->pfnGetOutputPortCaps = ACDiranaGetOutputPortCaps;
    pCodecInterface->pfnGetInputPortCaps = ACDiranaGetInputPortCaps;
    pCodecInterface->pfnGetPortPcmCaps = ACDiranaGetPortPcmCaps;
    pCodecInterface->pfnOpen = ACDiranaOpen;
    pCodecInterface->pfnClose = ACDiranaClose;
    pCodecInterface->pfnSetVolume = ACDiranaSetVolume;
    pCodecInterface->pfnSetMuteControl = ACDiranaSetMuteControl;
    pCodecInterface->pfnSetConfiguration = ACDiranaSetConfiguration;
    pCodecInterface->pfnGetConfiguration = ACDiranaGetConfiguration;
    pCodecInterface->pfnSetPowerState = ACDiranaSetPowerState;
    pCodecInterface->pfnSetPowerMode = ACDiranaSetPowerMode;

    pCodecInterface->hCodecPrivate = &s_Dirana;
    pCodecInterface->IsInited = NV_TRUE;
}

#define NVODM_CODEC_DIRANA_MAX_CLOCKS 1
static NvBool SetClockSourceOnCodec(NvOdmPrivAudioCodecHandle hOdmAudioCodec, NvBool IsEnable)
{
    const NvOdmPeripheralConnectivity *pPerConnectivity = NULL;
    NvU32 ClockInstances[NVODM_CODEC_DIRANA_MAX_CLOCKS];
    NvU32 ClockFrequencies[NVODM_CODEC_DIRANA_MAX_CLOCKS];
    NvU32 NumClocks;

    // Get the peripheral connectivity information
    pPerConnectivity = NvOdmPeripheralGetGuid(DIRANA_CODEC_GUID);
    if (pPerConnectivity == NULL)
        return NV_FALSE;
    if (IsEnable)
    {
        if (!NvOdmExternalClockConfig(
                DIRANA_CODEC_GUID, NV_FALSE,
                ClockInstances, ClockFrequencies, &NumClocks))
            return NV_FALSE;
    }
    else
    {
        if (!NvOdmExternalClockConfig(
                DIRANA_CODEC_GUID, NV_TRUE,
                ClockInstances, ClockFrequencies, &NumClocks));
            return NV_FALSE;
    }
    return NV_TRUE;
}

