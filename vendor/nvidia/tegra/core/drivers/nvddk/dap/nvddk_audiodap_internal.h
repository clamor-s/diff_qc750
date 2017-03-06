/*
* Copyright (c) 2007 - 2012 NVIDIA Corporation.  All rights reserved.
*
* NVIDIA Corporation and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA Corporation is strictly prohibited.
*/
#ifndef INCLUDED_NVDDK_DAP_INTERNAL_H
#define INCLUDED_NVDDK_DAP_INTERNAL_H

#include "nvrm_drf.h"
#include "nvrm_hardware_access.h"
#include "nvrm_power.h"
#include "nvassert.h"
#include "nvodm_modules.h"
#include "nvrm_pinmux.h"
#include "nvodm_query_pinmux.h"
#include "nvddk_audiodap.h"

#define DAS_REGISTER_WRITE32(pAddr,value) \
    NV_WRITE32((pAddr), (value))

#define DAS_REGISTER_READ32(pAddr) \
    NV_READ32(pAddr)

typedef struct NvAudioDap_MuxSelectRec
{
    NvOdmDapPort PortType;
    NvU32      RegisterOffset;
    NvU32      MuxDefaultMask;
    NvU32      MuxSelectShift;
    NvU32      ModeDefaultMask;
    NvU32      ModeSelectShift;
    NvU32      Sdata1DefaultMask;
    NvU32      Sdata1SelectShift;
    NvU32      Sdata2DefaultMask;
    NvU32      Sdata2SelectShift;
    NvU32      MuxValue;
}NvAudioDap_MuxSelect;

// Prototypes for controlling Das
NvError AudioDapDefaultSettings(NvDdkAudioDapHandle hDap, NvU32 DapPortIndex);
NvError AudioDapSetConnection(NvDdkAudioDapHandle hDap, NvBool IsEnable,
                               NvU32 SrcIndex, NvU32 DestIndex, NvBool IsMaster);
NvAudioDap_MuxSelect*  AudioDapGetAp20Table(NvOdmDapPort *NumOfDapPorts);
#endif //INCLUDED_NVDDK_DAP_INTERNAL_H

