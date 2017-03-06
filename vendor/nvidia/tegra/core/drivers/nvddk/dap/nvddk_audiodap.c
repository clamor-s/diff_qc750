/*
 * Copyright (c) 2007 - 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvddk_audiodap_internal.h"
#include "nvrm_pinmux.h"


#include "nvddk_i2s.h"
#define NvAudioDap_Max 6
#define NvI2s_Instances 2

// Module debug: 0=disable, 1=enable
#define NVDDK_DAP_ENABLE_LOG         (0)
#define NVDAP_PREFIX                 "NVDAP: "

#if NVDDK_DAP_ENABLE_LOG
    #define NVDDK_DAP_LOG(x)         NvOsDebugPrintf x
#else
    #define NVDDK_DAP_LOG(x)         do {} while(0)
#endif

static NvDdkAudioDapHandle g_hDap = 0;
static NvS32 g_DapRefCnt = 0;

typedef struct NvDdkAudioDapRec
{
    NvRmDeviceHandle hRmDevice;

    NvU32  MiscBaseAddress;
    NvU32  MiscRegSize;

    NvOdmAudioPortCaps*   pCodecPortCaps;
    NvU32                 IsVdacPresent;
    NvU32                 IsCodecMaster;
    NvOdmDapPort          NumDapPorts;

    // Last Mixer line connection
    NvDdkAudioDapConnectionLine   CurrentDapConnection;
    // 5 daps port
    NvOdmQueryDapPortProperty *pDapPortDefaultProperty[NvAudioDap_Max];

    //2 instances of I2s
    NvOdmQueryI2sInterfaceProperty I2sDefaultInterfaceProps[NvI2s_Instances];

    NvOdmDapPort HiFiCodecDapPortIndex;
    NvOdmDapPort VoiceCodecDapPortIndex;
    NvOdmDapPort BBDapPortIndex;
    NvOdmDapPort BTDapPortIndex;
    NvOdmDapPort MediaDapPortIndex;
    NvOdmDapPort VoiceDapPortIndex;

    NvS32        HiFiCodecUseCnt;

    // DapTable
    NvAudioDap_MuxSelect *pDapMuxTable;

    NvU32       RmPowerClientId;
    NvS32       DapUseCnt;
    NvU32       IsDapPowerEnabled;
    NvU32       HiFiDapInUse;
    NvU32       CurrentConnectionIndex;

    // Keep the I2sDDkHandle & CodecHandle
    NvDdkI2sHandle hDdkI2s[NvI2s_Instances];
    NvOdmAudioCodecHandle hAudioCodec;
    NvU32 CodecInstanceID;

} NvDdkAudioDap;

typedef struct DasMiscCapabilityRec
{
    NvAudioDap_MuxSelect *pMuxTable;

}DasMiscCapability;


//*****************************************************************************
// EnableDapModulePower
//*****************************************************************************
//
static NvError EnableDapModulePower(NvDdkAudioDapHandle hDap)
{
    // Enable power
    NvError Status = NvSuccess;

    if (!hDap->DapUseCnt && !hDap->IsDapPowerEnabled)
    {
        Status =  NvRmPowerVoltageControl(hDap->hRmDevice, NVRM_MODULE_ID(NvRmPrivModuleID_System, 0),
                        hDap->RmPowerClientId,
                        NvRmVoltsUnspecified, NvRmVoltsUnspecified,
                        NULL, 0, NULL);
        hDap->IsDapPowerEnabled = NV_TRUE;
    }
    hDap->DapUseCnt++;

    //NvOsDebugPrintf(" DCnt+ %d \n",hDap->DapUseCnt);
    return Status;

}
//*****************************************************************************
// DisableDapModulePower
//*****************************************************************************
//
static NvError DisableDapModulePower(NvDdkAudioDapHandle hDap)
{
    // Disable power
    NvError Status = NvSuccess;

    if (hDap->DapUseCnt)
        hDap->DapUseCnt--;

    if (!hDap->DapUseCnt && hDap->IsDapPowerEnabled)
    {
        Status = NvRmPowerVoltageControl(hDap->hRmDevice, NVRM_MODULE_ID(NvRmPrivModuleID_System, 0),
                    hDap->RmPowerClientId, NvRmVoltsOff, NvRmVoltsOff,
                    NULL, 0, NULL);
        hDap->IsDapPowerEnabled = NV_FALSE;
    }
    else
    {
        if (!hDap->HiFiCodecUseCnt)
        {
            NVDDK_DAP_LOG((NVDAP_PREFIX "DisableDapModulePower Not Supported, HiFiCodecUseCnt is 0\n"));
            Status = NvError_NotSupported;
        }
    }

    //NvOsDebugPrintf("DCnt- %d \n",hDap->DapUseCnt);

    return Status;
}

//*****************************************************************************
// AudioDapSetMuxMode
//*****************************************************************************
//
static NvError AudioDapSetMuxMode(NvDdkAudioDapHandle hDap, NvU32 DapPortIndex, NvBool IsMasterMode)
{
    NvU32 RegValue = 0;
    NvU32 SrcMode  = hDap->pDapMuxTable[DapPortIndex].ModeDefaultMask <<
                            hDap->pDapMuxTable[DapPortIndex].ModeSelectShift;

    NvU32 RegBase = hDap->MiscBaseAddress + hDap->pDapMuxTable[DapPortIndex].RegisterOffset;
    NVDDK_DAP_LOG((NVDAP_PREFIX "[++AudioDapSetMuxMode] hDap:x%x DapPortIndex:x%x IsMasterMode:x%x\n", hDap, DapPortIndex, IsMasterMode ));

    // Nothing to do for the None Port
    if (DapPortIndex == NvOdmDapPort_None)
        return NvSuccess;

    // Read the default DAS/DAP Register
    RegValue = NV_READ32(RegBase);

    // Clear the Mode bits
    RegValue &= ~(SrcMode);

    if (IsMasterMode)
    {
        // set the destination dap value
        RegValue |= ( 1 & hDap->pDapMuxTable[DapPortIndex].ModeDefaultMask)
                                   << hDap->pDapMuxTable[DapPortIndex].ModeSelectShift;
    }

    DAS_REGISTER_WRITE32(RegBase, RegValue);
    // NvOsDebugPrintf("MuxModecall DAS RegBase 0x%x 0x%x \n", RegBase, RegValue);
    NVDDK_DAP_LOG((NVDAP_PREFIX "[--AudioDapSetMuxMode]\n"));
    return NvSuccess;
}

//*****************************************************************************
// AudioDapSetMuxCtrlReg
//*****************************************************************************
//
static NvError AudioDapSetMuxCtrlReg(NvDdkAudioDapHandle hDap, NvU32 SrcIndex, NvU32 DestIndex)
{
    NvU32 RegValue = 0, RegBase = 0;
    NvU32 DestMuxSelectField  = 0, DestMuxSelectValue  = 0;
    NVDDK_DAP_LOG((NVDAP_PREFIX "[++AudioDapSetMuxCtrlReg] hDap:%x SrcIndex:x%x DestIndex:x%x\n", hDap, SrcIndex, DestIndex));

    if (DestIndex == NvOdmDapPort_None)
    {
        DestIndex = SrcIndex;
        SrcIndex  = NvOdmDapPort_None;
    }

    DestMuxSelectField  = hDap->pDapMuxTable[DestIndex].MuxDefaultMask <<
                                    hDap->pDapMuxTable[DestIndex].MuxSelectShift;

    DestMuxSelectValue  = hDap->pDapMuxTable[SrcIndex].MuxValue;

    RegBase = hDap->MiscBaseAddress + hDap->pDapMuxTable[DestIndex].RegisterOffset;

    // Check whether the DestIndex is for Dacs , then we need to use DapToDacMuxValue
    if ( DestIndex > NvOdmDapPort_Dap5)
    {
        DestMuxSelectValue = SrcIndex - NvOdmDapPort_Dap1;
    }
    // Read the default DAS/DAP Register
    RegValue = NV_READ32(RegBase);

    // Clear the exiting selection bits
    RegValue &= ~(DestMuxSelectField);

    // set the destination value
    RegValue |= (DestMuxSelectValue & hDap->pDapMuxTable[DestIndex].MuxDefaultMask)
                                       << hDap->pDapMuxTable[DestIndex].MuxSelectShift;

    // Check the number of port and set the input source accordingly
    if ((hDap->NumDapPorts == NvOdmDapPort_Dap5) && (DestIndex > NvOdmDapPort_Dap5))
    {
        // Clear the existing selection bits
        DestMuxSelectField  = hDap->pDapMuxTable[DestIndex].Sdata2DefaultMask <<
                                    hDap->pDapMuxTable[DestIndex].Sdata2SelectShift;
        RegValue &= ~(DestMuxSelectField);

        DestMuxSelectField  = hDap->pDapMuxTable[DestIndex].Sdata1DefaultMask <<
                                    hDap->pDapMuxTable[DestIndex].Sdata1SelectShift;
        RegValue &= ~(DestMuxSelectField);

        RegValue |= (DestMuxSelectValue & hDap->pDapMuxTable[DestIndex].Sdata2DefaultMask)
                        << hDap->pDapMuxTable[DestIndex].Sdata2SelectShift;
        RegValue |= (DestMuxSelectValue & hDap->pDapMuxTable[DestIndex].Sdata1DefaultMask)
                        << hDap->pDapMuxTable[DestIndex].Sdata1SelectShift;
    }

    DAS_REGISTER_WRITE32(RegBase, RegValue);

    // NvOsDebugPrintf("MuxReg Call DAS RegBase 0x%x 0x%x \n", RegBase, RegValue);
    NVDDK_DAP_LOG((NVDAP_PREFIX "[--AudioDapSetMuxCtrlReg]\n"));
    return NvSuccess;
}

//*****************************************************************************
// AudioDapSetState
// Function to set or reset the dap pad state
//*****************************************************************************
//
#ifndef SET_KERNEL_PINMUX
static NvError AudioDapSetState(NvDdkAudioDapHandle hDap, NvU32 PortIndex, NvBool IsReset)
{
    //NvOsDebugPrintf(" Dap state for %d State %d \n'", PortIndex, IsReset);

    if (PortIndex>=NvOdmDapPort_Dap1 &&
        PortIndex<NvOdmDapPort_I2s1)

        NV_ASSERT_SUCCESS(NvRmSetOdmModuleTristate(hDap->hRmDevice, NvOdmIoModule_Dap,
            PortIndex - NvOdmDapPort_Dap1, IsReset));

    return NvSuccess;

}
#endif

//*****************************************************************************
// AudioDapDefaultSettings
//*****************************************************************************
//
NvError AudioDapDefaultSettings(NvDdkAudioDapHandle hDap, NvU32 DestPortIndex)
{
    // setting to Default Source
    NvOdmDapPort SrcPortIndex = NvOdmDapPort_I2s1;

    if ((DestPortIndex <= NvOdmDapPort_None) || (DestPortIndex > NvOdmDapPort_Dap5))
        return NvSuccess;

    if (hDap->pDapPortDefaultProperty[DestPortIndex]->DapDestination == NvOdmDapPort_None)
        return NvSuccess;

    SrcPortIndex = hDap->pDapPortDefaultProperty[DestPortIndex]->DapSource;

    AudioDapSetMuxCtrlReg(hDap, SrcPortIndex, DestPortIndex);

    // Set the Port to Slave Mode
    AudioDapSetMuxMode(hDap, DestPortIndex, NV_FALSE);

#ifndef SET_KERNEL_PINMUX
    AudioDapSetState(hDap, DestPortIndex, NV_TRUE);
#endif
    return NvSuccess;
}

//*****************************************************************************
// AudioDapSetConnection
// Function to setup the Das Registers and controllers
//*****************************************************************************
//
NvError AudioDapSetConnection(NvDdkAudioDapHandle hDap, NvBool IsEnable,
                                   NvU32 SrcIndex, NvU32 DestIndex, NvBool IsMaster)
{
    NvBool DapState = NV_FALSE;
    NVDDK_DAP_LOG((NVDAP_PREFIX "[++AudioDapSetConnection]\n"));

    // If both source and destination as None - return
    if ((SrcIndex == NvOdmDapPort_None) && (DestIndex == NvOdmDapPort_None))
    {
        return NvSuccess;
    }

    if (IsEnable)
    {
        // Src to Dest Index
        AudioDapSetMuxCtrlReg(hDap, SrcIndex, DestIndex);

        // If SrcIndex is None swap the Src and Dest
        if (SrcIndex == NvOdmDapPort_None)
            SrcIndex  = DestIndex;

        // Set the Mode
        AudioDapSetMuxMode(hDap, SrcIndex, IsMaster);

        if ((SrcIndex == NvOdmDapPort_None) || (DestIndex == NvOdmDapPort_None))
            DapState  = NV_TRUE; // tristate

#ifndef SET_KERNEL_PINMUX
        // Set the Dap state to normal
        AudioDapSetState(hDap, SrcIndex, DapState);
#endif
    }
    else
    {
        // If SrcIndex is None swap the Src and Dest
        if (SrcIndex == NvOdmDapPort_None)
            SrcIndex  = DestIndex;

        // Set to Default dap connection
        AudioDapDefaultSettings(hDap, SrcIndex);
    }

    NVDDK_DAP_LOG((NVDAP_PREFIX "[--AudioDapSetConnection]\n"));
    return NvSuccess;
}

//*****************************************************************************
// AudioDapSetCodecAnalogInDevice
//*****************************************************************************
//
static NvError AudioDapSetCodecAnalogInDevice(NvDdkAudioDapHandle hDap,
                                                NvOdmAudioConfigInOutSignal* pMixerConnection, NvU32 PortId)
{
    NvOdmAudioConfigInOutSignal AudioConfig;

    NvOsMemcpy(&AudioConfig, pMixerConnection, sizeof(NvOdmAudioConfigInOutSignal));

    AudioConfig.OutSignalType = pMixerConnection->OutSignalType;

    if (hDap->hAudioCodec)
    {
        // SetPowerState
        NvOdmAudioCodecSetPowerState(hDap->hAudioCodec,
                                  NVODMAUDIO_PORTNAME(NvOdmAudioPortType_Input, PortId),
                                  pMixerConnection->InSignalType,
                                  pMixerConnection->InSignalId,
                                  pMixerConnection->IsEnable);
        // SetInputPortIO
        NvOdmAudioCodecSetConfiguration(hDap->hAudioCodec,
                                     NVODMAUDIO_PORTNAME(NvOdmAudioPortType_Input, PortId),
                                     NvOdmAudioConfigure_InOutSignal,
                                     (void*)&AudioConfig);
    }

    return NvSuccess;
}

//*****************************************************************************
// AudioDapSetVoiceCodec
//*****************************************************************************
//
static NvError AudioDapSetVoiceCodec(NvDdkAudioDapHandle hDap, NvDdkAudioDapConnectionLine* pMixerConnection)
{
    NvError Status = NvSuccess;
    NvOdmAudioConfigInOutSignal AudioConfig;

    if (!hDap) return Status;

    NvOsMemset(&AudioConfig, 0, sizeof(AudioConfig));
    AudioConfig.InSignalType  = NvOdmAudioSignalType_Digital_I2s;
    AudioConfig.InSignalId    = 1;
    AudioConfig.OutSignalType = NvOdmAudioSignalType_Digital_BaseBand;
    AudioConfig.OutSignalId   = 0;
    AudioConfig.IsEnable      = pMixerConnection->IsEnable;

    if (hDap->hAudioCodec)
    {
        // SetInputPortIO
        NvOdmAudioCodecSetConfiguration(hDap->hAudioCodec,
                                     NVODMAUDIO_PORTNAME(NvOdmAudioPortType_Output, 1),
                                     NvOdmAudioConfigure_Vdac,
                                     (void*)&AudioConfig);

        // SetPowerState for vdac
        NvOdmAudioCodecSetPowerState(hDap->hAudioCodec,
                                  NVODMAUDIO_PORTNAME(NvOdmAudioPortType_Output, 1),
                                  NvOdmAudioSignalType_HeadphoneOut,
                                  0,
                                  pMixerConnection->IsEnable);
    }

    return Status;
}

//*****************************************************************************
// AudioDapSetInPortConnection
//*****************************************************************************
//
static NvError AudioDapSetInPortConnection(NvDdkAudioDapHandle hDap, NvOdmAudioSignalType SignalType,
                                                        NvU32 InPortId, NvU32 OutPortId, NvU32 IsEnable)
{
    // Set MicIn1-0 if headphone connected - else use MicIn2 -1 onboard
    NvOdmAudioConfigInOutSignal  MixerConnection;
    NvOsMemset(&MixerConnection, 0, sizeof(MixerConnection));
    MixerConnection.InSignalType  = SignalType;
    MixerConnection.InSignalId    = InPortId;
    MixerConnection.IsEnable      = IsEnable;
    MixerConnection.OutSignalType = NvOdmAudioSignalType_Digital_I2s;
    return AudioDapSetCodecAnalogInDevice(hDap, &MixerConnection, OutPortId);
}

//*****************************************************************************
// AudioDapSetCodecOperationMode
//*****************************************************************************
//
static NvError AudioDapSetCodecOperationMode(NvDdkAudioDapHandle hDap,
                                               NvOdmQueryDapPortConnectionLines* pPortConnection,
                                               NvBool IsEnable)
{
    NvU32 OutPortIndex       = 0;
    NvBool CodecMasterMode   = NV_TRUE;
    NVDDK_DAP_LOG((NVDAP_PREFIX "[++AudioDapSetCodecOperationMode]\n"));

    CodecMasterMode = (hDap->IsCodecMaster)? NV_TRUE: NV_FALSE;
    if (IsEnable)
    {
        // Check the Dapport Mode
        if (pPortConnection->IsSourceMaster)
        {
            CodecMasterMode = NV_FALSE;
        }
    }

    if ( (hDap->IsVdacPresent) && (pPortConnection->Destination == hDap->VoiceCodecDapPortIndex))
    {
       OutPortIndex = 1;
    }

    if (hDap->hAudioCodec)
        NvOdmAudioCodecSetOperationMode(hDap->hAudioCodec,
            NVODMAUDIO_PORTNAME(NvOdmAudioPortType_Output, OutPortIndex), CodecMasterMode);

    NVDDK_DAP_LOG((NVDAP_PREFIX "[--AudioDapSetCodecOperationMode]\n"));
    return NvSuccess;
}

//*****************************************************************************
// AudioDapSetCodecToDapProperty
//*****************************************************************************
//
static NvError AudioDapSetCodecToDapProperty(NvDdkAudioDapHandle hDap,
                                               NvOdmQueryDapPortConnectionLines* pPortConnection,
                                               NvBool IsEnable)
{
    NvOdmAudioWave AudioWave;
    NvError e = NvSuccess;
    NvU32 DapPortIndex = pPortConnection->Source, OutPortIndex = 0;
    NvOdmQueryDapPortProperty *pPortProperty = NULL;
    NVDDK_DAP_LOG((NVDAP_PREFIX "[++AudioDapSetCodecToDapProperty]\n"));

    if (!IsEnable)
    {
        DapPortIndex = pPortConnection->Destination;
    }

    if ((DapPortIndex <= NvOdmDapPort_None) || (DapPortIndex > NvOdmDapPort_Dap5))
        return NvSuccess;

    pPortProperty = hDap->pDapPortDefaultProperty[DapPortIndex];

    NvOsMemset(&AudioWave, 0, sizeof(AudioWave));
    AudioWave.IsSignedData          = NV_FALSE;
    AudioWave.IsInterleaved         = NV_TRUE;
    AudioWave.IsLittleEndian        = NV_FALSE;
    AudioWave.SamplingRateInHz      = pPortProperty->DapDeviceProperty.SamplingRateInHz;
    AudioWave.NumberOfBitsPerSample = pPortProperty->DapDeviceProperty.NumberOfBitsPerSample;
    AudioWave.NumberOfChannels      = pPortProperty->DapDeviceProperty.NumberOfChannels;
    AudioWave.DataFormat            = pPortProperty->DapDeviceProperty.DapPortCommunicationFormat;
    AudioWave.FormatType            = NvOdmAudioWaveFormat_Pcm;

    if ( (hDap->IsVdacPresent) && (pPortConnection->Destination == hDap->VoiceCodecDapPortIndex))
    {
       OutPortIndex = 1;
    }

    if (hDap->hAudioCodec)
    {
        NvBool IsSuccess = NvSuccess;
        IsSuccess = NvOdmAudioCodecSetConfiguration(hDap->hAudioCodec,
            NVODMAUDIO_PORTNAME(NvOdmAudioPortType_Output, OutPortIndex),
                        NvOdmAudioConfigure_Pcm, (void *)&AudioWave);
        if (!IsSuccess)
        {
            NVDDK_DAP_LOG((NVDAP_PREFIX "AudioDapSetCodecToDapProperty Not Supported, NvOdmAudioCodecSetConfiguration failed!\n"));
            e = NvError_NotSupported;
        }
    }

    NVDDK_DAP_LOG((NVDAP_PREFIX "[--AudioDapSetCodecToDapProperty]\n"));
    return e;
}

//*****************************************************************************
// AudioDapSetDacInterfaceProperty
//*****************************************************************************
//

static NvError AudioDapSetDacInterfaceProperty(NvDdkAudioDapHandle hDap,
                                          NvU32 I2sInstId,
                                          NvOdmQueryDapPortConnectionLines* pPortConnection,
                                          NvBool IsEnable)
{
    // Get the current dac property
    NvOdmQueryI2sInterfaceProperty CurIntProperty;
    NVDDK_DAP_LOG((NVDAP_PREFIX "[++AudioDapSetDacInterfaceProperty]\n"));

    NvDdkI2sGetInterfaceProperty(hDap->hDdkI2s[I2sInstId], (void*)&CurIntProperty);
    if (IsEnable)
    {
        CurIntProperty.Mode = NvOdmQueryI2sMode_Master;
    }
    else
    {
        // Need to get only the existing mode from the query table.
        // Set the Default Odm Query Value to I2s
        CurIntProperty.Mode = hDap->I2sDefaultInterfaceProps[I2sInstId].Mode;
    }
    NvDdkI2sSetInterfaceProperty(hDap->hDdkI2s[I2sInstId], (void*)&CurIntProperty);

    NVDDK_DAP_LOG((NVDAP_PREFIX "[--AudioDapSetDacInterfaceProperty]\n"));
    return NvSuccess;
}

//*****************************************************************************
// AudioDapSetCodecProperty
//*****************************************************************************
//
static NvError AudioDapSetCodecProperty(NvDdkAudioDapHandle hDap,
                                          NvOdmQueryDapPortConnectionLines* pPortConnection,
                                          NvBool IsEnable)
{
    NvError Status    = NvSuccess;

    Status = AudioDapSetCodecToDapProperty(hDap, pPortConnection, IsEnable);

    if (Status == NvSuccess)
    {
        // Check the DapDevice property -- set it to Master or Slave accordingly
        AudioDapSetCodecOperationMode(hDap, pPortConnection, IsEnable);
    }

    return Status;
}

//*****************************************************************************
// AudioDapSetConnectionLine
// Function to setup the Dap Connection
//*****************************************************************************
//
static NvError AudioDapSetConnectionLine(
    NvDdkAudioDapHandle hDap,
    NvDdkAudioDapConnectionLine* pMixerConnection)
{
    NvError Status  = NvSuccess;
    NvU32 Index  = 0, OutPortIndex = 0;

    NvOdmQueryDapPortConnectionLines PortConnectionLine;
    NvOdmQueryDapPortConnection* pDapPortConnectionTbl = NULL;
    NvU32 NumEntry;
    NvU32 I2sInstId = 0;
    NVDDK_DAP_LOG((NVDAP_PREFIX "[++AudioDapSetConnectionLine]\n"));

    if (!hDap || !pMixerConnection)
    {
        NVDDK_DAP_LOG((NVDAP_PREFIX "AudioDapSetConnectionLine Bad Parameter, hDap:x%x pMixerConnection:x%x\n", hDap, pMixerConnection));
        return NvError_BadParameter;
    }

    pDapPortConnectionTbl = (NvOdmQueryDapPortConnection *)
        NvOdmQueryDapPortGetConnectionTable(pMixerConnection->IndexToConnectionTable);

    if (!pDapPortConnectionTbl)
    {
        // Adding a checking to check for TestReserved Index
        if (pMixerConnection->IndexToConnectionTable ==
                                            NvOdmDapConnectionIndex_TestReserved)
        {
            pDapPortConnectionTbl = (NvOdmQueryDapPortConnection *)
                       ((NvU8*)pMixerConnection+ sizeof(NvDdkAudioDapConnectionLine));
        }
        if (!pDapPortConnectionTbl)
        {
            NVDDK_DAP_LOG((NVDAP_PREFIX "AudioDapSetConnectionLine Not Supported, pDapPortConnectionTbl is 0\n"));
            return NvError_NotSupported;
        }
    }
    NumEntry = pDapPortConnectionTbl->NumofEntires;
    if (!NumEntry)
    {
        NVDDK_DAP_LOG((NVDAP_PREFIX "AudioDapSetConnectionLine Not Supported, NumEntry is 0\n"));
        return NvError_NotSupported;
    }

    // Save the ConnectionIndex
    if (pMixerConnection->IsEnable)
        hDap->CurrentConnectionIndex = pMixerConnection->IndexToConnectionTable;

    // Set the Dap port Connection accordingly
    for(Index = 0; Index < NumEntry; Index++)
    {
        PortConnectionLine = pDapPortConnectionTbl->DapPortConnectionLines[Index];

        if ((PortConnectionLine.Source == NvOdmDapPort_I2s1) ||
            (PortConnectionLine.Source == NvOdmDapPort_I2s2))
        {
            I2sInstId = PortConnectionLine.Source - NvOdmDapPort_I2s1;
            if (!hDap->hDdkI2s[I2sInstId])
            {
                NVDDK_DAP_LOG((NVDAP_PREFIX "AudioDapSetConnectionLine Not Supported, hDap->hBlI2s[I2sInstId] is 0, I2sInstId:x%x\n", I2sInstId));
                return NvError_NotSupported;
            }
        }

        // Check whether we need to enable the voice codec
        if ( (hDap->IsVdacPresent) && (PortConnectionLine.Destination == hDap->VoiceCodecDapPortIndex))
        {
            AudioDapSetVoiceCodec(hDap, pMixerConnection);
            OutPortIndex = 1;
        }

        if ((PortConnectionLine.Destination == hDap->HiFiCodecDapPortIndex) &&
            (pMixerConnection->IsEnable))
            hDap->HiFiDapInUse = NV_TRUE;

        // Set the codec property if the Source or destination is connected to a codec
        if ( (PortConnectionLine.Destination == hDap->VoiceCodecDapPortIndex) ||
             (PortConnectionLine.Destination == hDap->HiFiCodecDapPortIndex))
        {
            // Make sure the MCLK for codec is enabled or not.
            // No need to call for the default music connection
            if (pMixerConnection->IndexToConnectionTable)
            {
                if (hDap->hAudioCodec)
                    NvOdmAudioCodecSetPowerMode(hDap->hAudioCodec,
                        (pMixerConnection->IsEnable)? NvOdmAudioCodecPowerMode_Normal: NvOdmAudioCodecPowerMode_Shutdown);

                if ((!hDap->IsCodecMaster) && (hDap->I2sDefaultInterfaceProps[I2sInstId].Mode))
                {
                    if (pMixerConnection->IsEnable)
                    {
                        Status = NvDdkI2sResume(hDap->hDdkI2s[I2sInstId]);
                    }
                    else
                    {
                        Status = NvDdkI2sSuspend(hDap->hDdkI2s[I2sInstId]);
                    }
                }
            }

            Status = AudioDapSetCodecProperty(hDap, &PortConnectionLine,
                                                pMixerConnection->IsEnable);
        }

        // if it is connected to a DAC , then set the i2s property accordingly
        if ( ((PortConnectionLine.Source == NvOdmDapPort_I2s1) ||
             (PortConnectionLine.Source == NvOdmDapPort_I2s2))
                       && (PortConnectionLine.IsSourceMaster))
        {
            // Set I2s As the Master in this connection
            // Make sure the Property is same as Codec one
            if ((pMixerConnection->IsEnable) && (pMixerConnection->IndexToConnectionTable))
            {
                Status = NvDdkI2sResume(hDap->hDdkI2s[I2sInstId]);
            }
            AudioDapSetDacInterfaceProperty(hDap,I2sInstId,&PortConnectionLine,pMixerConnection->IsEnable);
            if ((!pMixerConnection->IsEnable) && (pMixerConnection->IndexToConnectionTable))
                Status = NvDdkI2sSuspend(hDap->hDdkI2s[I2sInstId]);
        }

        // check port has priority and set the master port accordingly
        AudioDapSetConnection(hDap, pMixerConnection->IsEnable,
                 PortConnectionLine.Source,
                 PortConnectionLine.Destination,
                 PortConnectionLine.IsSourceMaster);

    }

    // Enable Mic or LineIn based on Rec Source request
    // if rec source specified
    if (pMixerConnection->SignalType != NvOdmAudioSignalType_None)
    {
        // check the signaltype and set.
        if ((pMixerConnection->SignalType == NvOdmAudioSignalType_MicIn) ||
             (pMixerConnection->SignalType == NvOdmAudioSignalType_LineIn) ||
             (pMixerConnection->SignalType == NvOdmAudioSignalType_Aux))
        {
            Status = AudioDapSetInPortConnection(hDap, pMixerConnection->SignalType,
                                                pMixerConnection->PortIndex, OutPortIndex, pMixerConnection->IsEnable);
        }
    }

    if (Status == NvSuccess)
        NvOsMemcpy(&hDap->CurrentDapConnection, pMixerConnection, sizeof(NvDdkAudioDapConnectionLine));

    // Set the powermode
    if ( (Status == NvSuccess) && hDap->CurrentConnectionIndex)
        NvDdkAudioDapPowerMode(hDap, !pMixerConnection->IsEnable);

    if ((PortConnectionLine.Destination == hDap->HiFiCodecDapPortIndex) &&
          (!pMixerConnection->IsEnable))
        hDap->HiFiDapInUse = NV_FALSE;

    NVDDK_DAP_LOG((NVDAP_PREFIX "[--AudioDapSetConnectionLine]\n"));
    return Status;
}
//*****************************************************************************
// AudioDapGetMuxTable
// Get appropriate Mux Table
//*****************************************************************************
//
static NvError AudioDapGetMuxTable(NvDdkAudioDapHandle hDap)
{
    NvError Status = NvSuccess;
    static DasMiscCapability MiscModCaps0;
    NvU32 *pMiscModCaps;

    // Set the Module capability whether it supports the ap15 or ap20
    NvRmModuleCapability s_MiscModcaps[] =
    { { 2, 0, 0, NvRmModulePlatform_Silicon, &MiscModCaps0 },
    };

    NvAudioDap_MuxSelect *Ap20MuxTableCap = AudioDapGetAp20Table(&hDap->NumDapPorts);

    MiscModCaps0.pMuxTable = Ap20MuxTableCap;

    Status = NvRmModuleGetCapabilities(hDap->hRmDevice, NVRM_MODULE_ID(NvRmModuleID_Misc, 0),
                        s_MiscModcaps, 5, (void **)&pMiscModCaps);

    if(Status ==  NvSuccess)
    {
        hDap->pDapMuxTable = (NvAudioDap_MuxSelect*)*pMiscModCaps;
        // To avoid system hang - set the  Table to Ap20Table
        if (!hDap->pDapMuxTable)
        {
            hDap->pDapMuxTable = Ap20MuxTableCap;
        }
    }

    return Status;
}

//*****************************************************************************
// AudioDapSetHiFiDapState
// Set/Reset the HifiDap State
//*****************************************************************************
//
static NvError AudioDapSetHiFiDapState(NvDdkAudioDapHandle hDap, NvBool IsTriStateMode)
{
    NvError Status = NvSuccess;

    if (hDap->HiFiDapInUse)
    {
        if (IsTriStateMode) //equivalent to poweroff
        {
            if(hDap->HiFiCodecUseCnt > 0)
                hDap->HiFiCodecUseCnt--;
        }
        else  //equivalent to poweron
            hDap->HiFiCodecUseCnt++;

        //NvOsDebugPrintf("HiFiCnt %d \n", hDap->HiFiCodecUseCnt);

        if ( (!IsTriStateMode) || (hDap->HiFiCodecUseCnt <= 0))
        {
            //Set mode to all Dapport used in default connection
            //NvOsDebugPrintf("setting the Dap1 in TriState %d \n", IsTriStateMode);
#ifndef SET_KERNEL_PINMUX
            Status = AudioDapSetState(hDap, hDap->HiFiCodecDapPortIndex, IsTriStateMode);
#endif
        }
    }
    return Status;
}

//*****************************************************************************
// AudioDapSetDefaultMusicPath
// connection for default Music path
//*****************************************************************************
//
static NvError AudioDapSetDefaultMusicPath(NvDdkAudioDapHandle hDap)
{
    NvDdkAudioDapConnectionLine MixerConnection;
    NvOsMemset(&MixerConnection, 0, sizeof(NvDdkAudioDapConnectionLine));

    MixerConnection.IsEnable     = NV_TRUE;
    MixerConnection.SignalType   = NvOdmAudioSignalType_None;

    // Default Music path Index is 0
    MixerConnection.IndexToConnectionTable = NvOdmDapConnectionIndex_Music_Path;

    AudioDapSetConnectionLine(hDap, &MixerConnection);

    return NvSuccess;
}


//*****************************************************************************
// External Functions
//*****************************************************************************
//

//**********************************************************************************
// NvDdkAudioDapPowerMode
// Function to set tristate/normal mode for the HifiCodec Dap Port based on powermode
//***********************************************************************************
//
NvError NvDdkAudioDapPowerMode(NvDdkAudioDapHandle hDap, NvBool IsTriStateMode)
{

    NvError Status = NvSuccess;

    Status = AudioDapSetHiFiDapState(hDap, IsTriStateMode);

    if (Status == NvSuccess)
    {
        if (IsTriStateMode) //equivalent to poweroff
        {
            Status = DisableDapModulePower(hDap);
        }
        else  //equivalent to poweron
        {
            Status = EnableDapModulePower(hDap);
        }
    }

    return Status;
}

//*****************************************************************************
// NvDdkAudioDapOpen
// Function to setup the Das Registers and controllers
//*****************************************************************************
//
NvError NvDdkAudioDapOpen(NvRmDeviceHandle hRmDevice,
                     void *hDriverI2s,
                     NvU32 I2sInstanceID,
                     NvOdmAudioCodecHandle hAudioCodec,
                     NvU32 CodecInstanceID,
                     NvDdkAudioDapHandle* phDap)
{
    NvError Status = NvSuccess;
    NvU32 MiscModulePhyAddr = 0 , i = 0;
    NvBool IsFound = NV_FALSE;
    NvDdkAudioDap* pDap = NULL;
    const NvOdmQueryI2sACodecInterfaceProp *pI2sCodecInt = NULL;
    NvOdmQueryI2sInterfaceProperty *pI2sInterfaceProps   = NULL;
    NVDDK_DAP_LOG((NVDAP_PREFIX "[++NvDdkAudioDapOpen]\n"));

    if ((g_DapRefCnt > 0) && g_hDap)
    {
        g_DapRefCnt++;
        g_hDap->hDdkI2s[I2sInstanceID]    = hDriverI2s;
        // Get the default I2s Interface Status.
        pI2sInterfaceProps =
                    (NvOdmQueryI2sInterfaceProperty *)NvOdmQueryI2sGetInterfaceProperty(I2sInstanceID);

        NvOsMemcpy(&g_hDap->I2sDefaultInterfaceProps[I2sInstanceID], pI2sInterfaceProps,
                        sizeof(NvOdmQueryI2sInterfaceProperty));

        *phDap = g_hDap;
        return Status;
    }

    pDap   = (NvDdkAudioDap*)NvOsAlloc(sizeof(NvDdkAudioDap));
    if(!pDap)
    {
        Status = NvError_InsufficientMemory;
        return Status;
    }

    NvOsMemset(pDap, 0, sizeof(NvDdkAudioDap));

    pDap->hRmDevice      = hRmDevice;
    /*Status = NvDdkI2sGetHandle(&pDap->hDdkI2s[0], 0);
    if(Status !=  NvSuccess)
    {
        goto DAS_FAILED;
    }
    Status = NvDdkI2sGetHandle(&pDap->hDdkI2s[1], 1);
    if(Status !=  NvSuccess)
    {
        goto DAS_FAILED;
    }*/
    pDap->hDdkI2s[I2sInstanceID]    = hDriverI2s;

    pDap->hAudioCodec     = hAudioCodec;
    pDap->CodecInstanceID = CodecInstanceID;

    // Get the physical base address of vde register.
    NvRmModuleGetBaseAddress(pDap->hRmDevice, NVRM_MODULE_ID(NvRmModuleID_Misc, 0),
                            &MiscModulePhyAddr, &pDap->MiscRegSize);

    // Get the virtual base address of misc dap register.
    Status = NvRmPhysicalMemMap(MiscModulePhyAddr, pDap->MiscRegSize, NVOS_MEM_READ_WRITE,
                 NvOsMemAttribute_Uncached, (void **)&pDap->MiscBaseAddress);

    if(Status !=  NvSuccess)
    {
        goto DAS_FAILED;
    }

    Status = AudioDapGetMuxTable(pDap);
    if(Status !=  NvSuccess)
    {
        goto DAS_FAILED;
    }

    // Query the portcaps for codec
    pDap->IsVdacPresent   = NV_FALSE;
    pDap->pCodecPortCaps  = (NvOdmAudioPortCaps*)NvOdmAudioCodecGetPortCaps(CodecInstanceID);
    if (pDap->pCodecPortCaps->MaxNumberOfOutputPort > 1)
    {
        pDap->IsVdacPresent = NV_TRUE;
    }

    pI2sCodecInt = NvOdmQueryGetI2sACodecInterfaceProperty(CodecInstanceID);
    if (pI2sCodecInt)
    {
        pDap->IsCodecMaster = pI2sCodecInt->IsCodecMasterMode;
    }

    // Set the Dap module Voltage Control
    pDap->RmPowerClientId = NVRM_POWER_CLIENT_TAG('D','A','P','*');
    Status = NvRmPowerRegister(pDap->hRmDevice, NULL, &pDap->RmPowerClientId);
    if(Status !=  NvSuccess)
        goto DAS_FAILED;

    for ( i = 0; i < 5; i++)
    {
        IsFound = NV_TRUE;
        pDap->pDapPortDefaultProperty[i] = (NvOdmQueryDapPortProperty *)NvOdmQueryDapPortGetProperty(i);
        // For low power consumption - default the values as follows
        // 0x7000009c = 0x00000248 // This sets DAP2, 3, 4 connected to DAC2
        // 0x70000014 = 0x00000700 // SDOUT2,3,4 are input

        if (!pDap->pDapPortDefaultProperty[i])
        {
            continue;
        }

        // Obtain the PortIndex for each Type
        switch(pDap->pDapPortDefaultProperty[i]->DapDestination)
        {
            case NvOdmDapPort_HifiCodecType:
                pDap->HiFiCodecDapPortIndex  = (NvOdmDapPort)i;
                break;
            case NvOdmDapPort_VoiceCodecType:
                pDap->VoiceCodecDapPortIndex = (NvOdmDapPort)i;
                break;
            case NvOdmDapPort_BlueTooth:
                pDap->BTDapPortIndex    = (NvOdmDapPort)i;
                break;
            case NvOdmDapPort_BaseBand:
                pDap->BBDapPortIndex    = (NvOdmDapPort)i;
                break;
            case NvOdmDapPort_MediaType:
                pDap->MediaDapPortIndex = (NvOdmDapPort)i;
                break;
            case NvOdmDapPort_VoiceType:
                pDap->VoiceDapPortIndex = (NvOdmDapPort)i;
                break;
            default:
                IsFound = NV_FALSE;
                break;
        }

        if (IsFound)
        {
            AudioDapDefaultSettings(pDap, i);
        }
    }

    // Get the default I2s Interface Status.
    pI2sInterfaceProps =
                       (NvOdmQueryI2sInterfaceProperty *)NvOdmQueryI2sGetInterfaceProperty(I2sInstanceID);

    NvOsMemcpy(&pDap->I2sDefaultInterfaceProps[I2sInstanceID], pI2sInterfaceProps,
                                                    sizeof(NvOdmQueryI2sInterfaceProperty));

    Status = AudioDapSetDefaultMusicPath(pDap);
    if(Status !=  NvSuccess)
    {
        goto DAS_FAILED;
    }

    Status = NvDdkAudioDapPowerMode(pDap, NV_FALSE);
    if(Status !=  NvSuccess)
        goto DAS_FAILED;

    *phDap = (NvDdkAudioDapHandle)pDap;
    g_hDap = pDap;
    g_DapRefCnt++;

    return Status;

DAS_FAILED:
    NvDdkAudioDapClose(pDap);
    pDap = 0;
    Status = NvError_NotSupported;

    NVDDK_DAP_LOG((NVDAP_PREFIX "[--NvDdkAudioDapOpen]\n"));
    return Status;
}


//*****************************************************************************
// NvDdkAudioDapClose
//*****************************************************************************
//
void NvDdkAudioDapClose(NvDdkAudioDapHandle hDap)
{
    NVDDK_DAP_LOG((NVDAP_PREFIX "[++NvDdkAudioDapClose]\n"));
    if (hDap)
    {
        if (g_DapRefCnt)
            g_DapRefCnt--;

        if (g_DapRefCnt <= 0)
        {
            if(hDap->MiscBaseAddress)
            {
                NvRmPhysicalMemUnmap((void*)hDap->MiscBaseAddress,
                     hDap->MiscRegSize);
                hDap->MiscBaseAddress = 0;
            }

            // Unregister for the power manager.
            if (hDap->RmPowerClientId)
            NvRmPowerUnRegister(hDap->hRmDevice, hDap->RmPowerClientId);

            NvOsFree(hDap);
            g_DapRefCnt = 0;
            g_hDap = NULL;
        }
    }

    NVDDK_DAP_LOG((NVDAP_PREFIX "[--NvDdkAudioDapClose]\n"));
}

//*****************************************************************************
// NvDdkAudioDapSetConnection
//*****************************************************************************
//
NvError NvDdkAudioDapSetConnection(NvDdkAudioDapHandle hDap, NvDdkAudioDapConnectionLine* pMixerConnection)
{
    NvError Status         = NvSuccess;
    NVDDK_DAP_LOG((NVDAP_PREFIX "[++NvDdkAudioDapSetConnection]\n"));

    if (!hDap)
    {
        NVDDK_DAP_LOG((NVDAP_PREFIX "NvDdkAudioDapSetConnection Not Supported, hDap:x%x\n", hDap));
        return NvError_NotSupported;
    }

    if (pMixerConnection->IndexToConnectionTable != NvOdmDapConnectionIndex_Unknown)
    {
        AudioDapSetConnectionLine(hDap, pMixerConnection);
    }
    else
    {
        // Check the SignalType and set according.
        if (pMixerConnection->SignalType == NvOdmAudioSignalType_Speakers)
        {
            NvOdmAudioMuteData  MuteInfo;
            NvOsMemset(&MuteInfo, 0, sizeof(NvOdmAudioMuteData));
            MuteInfo.SignalType = NvOdmAudioSignalType_Speakers;
            MuteInfo.IsMute     = pMixerConnection->IsEnable;

            if (hDap->hAudioCodec)
            {
                // Enable or disable the Internal speaker
                NvOdmAudioCodecSetMuteControl(hDap->hAudioCodec,
                                    NVODMAUDIO_PORTNAME(NvOdmAudioPortType_Output, 0),
                                    &MuteInfo,1);
            }
        }
        else if ( (pMixerConnection->SignalType == NvOdmAudioSignalType_MicIn) ||
                  (pMixerConnection->SignalType == NvOdmAudioSignalType_LineIn)||
                  (pMixerConnection->SignalType == NvOdmAudioSignalType_Aux))
        {
            AudioDapSetInPortConnection(hDap, pMixerConnection->SignalType,
                                            pMixerConnection->PortIndex, 0, pMixerConnection->IsEnable);
        }
    }

    NVDDK_DAP_LOG((NVDAP_PREFIX "[--NvDdkAudioDapSetConnection]\n"));
    return Status;
}

//*****************************************************************************
// NvDdkAudioDapGetConnection
//*****************************************************************************
//
NvError NvDdkAudioDapGetConnection(NvDdkAudioDapHandle hDap, NvDdkAudioDapConnectionLine* pProperty)
{
    NvError Status = NvSuccess;
    NVDDK_DAP_LOG((NVDAP_PREFIX "[++NvDdkAudioDapGetConnection]\n"));
    NvOsMemcpy(pProperty, &hDap->CurrentDapConnection, sizeof(NvDdkAudioDapConnectionLine));
    NVDDK_DAP_LOG((NVDAP_PREFIX "[--NvDdkAudioDapGetConnection]\n"));
    return Status;
}



