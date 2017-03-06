
#define NV_IDL_IS_STUB

/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvos.h"
#include "nvassert.h"
#include "nvidlcmd.h"
#include "nvrm_power.h"

static NvOsFileHandle gs_IoctlFile;
static NvU32 gs_IoctlCode;

static void NvIdlIoctlFile(void)
{
    if(!gs_IoctlFile)
    {
        gs_IoctlFile = NvRm_NvIdlGetIoctlFile();
        gs_IoctlCode = NvRm_NvIdlGetIoctlCode();
    }
}

#define OFFSET(s, e)  (NvU32)(void *)(&(((s*)0)->e))

#define supported_module(module) \
    ((NVRM_MODULE_ID_MODULE(module)==NvRmModuleID_Vi) || \
     (NVRM_MODULE_ID_MODULE(module)==NvRmModuleID_Csi) || \
     (NVRM_MODULE_ID_MODULE(module)==NvRmModuleID_Isp))

// NvRm Package
typedef enum
{
    NvRm_Invalid = 0,
    NvRm_nvrm_xpc,
    NvRm_nvrm_transport,
    NvRm_nvrm_memctrl,
    NvRm_nvrm_pcie,
    NvRm_nvrm_pwm,
    NvRm_nvrm_keylist,
    NvRm_nvrm_pmu,
    NvRm_nvrm_diag,
    NvRm_nvrm_pinmux,
    NvRm_nvrm_analog,
    NvRm_nvrm_owr,
    NvRm_nvrm_i2c,
    NvRm_nvrm_spi,
    NvRm_nvrm_interrupt,
    NvRm_nvrm_dma,
    NvRm_nvrm_power,
    NvRm_nvrm_gpio,
    NvRm_nvrm_module,
    NvRm_nvrm_memmgr,
    NvRm_nvrm_init,
    NvRm_Num,
    NvRm_Force32 = 0x7FFFFFFF,
} NvRm;

typedef struct NvRmPowerModuleClockControl_in_t
{
    NvU32 package_;
    NvU32 function_;
    NvRmDeviceHandle hRmDeviceHandle;
    NvRmModuleID ModuleId;
    NvU32 ClientId;
    NvBool Enable;
} NV_ALIGN(4) NvRmPowerModuleClockControl_in;

typedef struct NvRmPowerModuleClockControl_inout_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRmPowerModuleClockControl_inout;

typedef struct NvRmPowerModuleClockControl_out_t
{
    NvError ret_;
} NV_ALIGN(4) NvRmPowerModuleClockControl_out;

typedef struct NvRmPowerModuleClockControl_params_t
{
    NvRmPowerModuleClockControl_in in;
    NvRmPowerModuleClockControl_inout inout;
    NvRmPowerModuleClockControl_out out;
} NvRmPowerModuleClockControl_params;

typedef struct NvRmPowerModuleClockConfig_in_t
{
    NvU32 package_;
    NvU32 function_;
    NvRmDeviceHandle hRmDeviceHandle;
    NvRmModuleID ModuleId;
    NvU32 ClientId;
    NvRmFreqKHz MinFreq;
    NvRmFreqKHz MaxFreq;
    NvRmFreqKHz  * PrefFreqList;
    NvU32 PrefFreqListCount;
    NvU32 flags;
} NV_ALIGN(4) NvRmPowerModuleClockConfig_in;

typedef struct NvRmPowerModuleClockConfig_inout_t
{
    NvU32 dummy_;
} NV_ALIGN(4) NvRmPowerModuleClockConfig_inout;

typedef struct NvRmPowerModuleClockConfig_out_t
{
    NvError ret_;
    NvRmFreqKHz CurrentFreq;
} NV_ALIGN(4) NvRmPowerModuleClockConfig_out;

typedef struct NvRmPowerModuleClockConfig_params_t
{
    NvRmPowerModuleClockConfig_in in;
    NvRmPowerModuleClockConfig_inout inout;
    NvRmPowerModuleClockConfig_out out;
} NvRmPowerModuleClockConfig_params;

NvError NvRmKernelPowerResume(NvRmDeviceHandle hRmDeviceHandle)
{
    return NvError_AccessDenied;
}

NvError NvRmKernelPowerSuspend(NvRmDeviceHandle hRmDeviceHandle)
{
    return NvError_AccessDenied;
}

void NvRmDfsSetLowVoltageThreshold(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmDfsVoltageRailId RailId,
    NvRmMilliVolts LowMv)
{
}

void NvRmDfsGetLowVoltageThreshold(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmDfsVoltageRailId RailId,
    NvRmMilliVolts *pLowMv,
    NvRmMilliVolts *pPresentMv)
{
}

NvError NvRmDfsLogBusyGetEntry(
    NvRmDeviceHandle hRmDeviceHandle,
    NvU32 EntryIndex,
    NvU32 *pSampleIndex,
    NvU32 *pClientId,
    NvU32 *pClientTag,
    NvRmDfsBusyHint *pBusyHint)
{
    return NvError_NotSupported;
}

NvError NvRmDfsLogStarvationGetEntry(
    NvRmDeviceHandle hRmDeviceHandle,
    NvU32 EntryIndex,
    NvU32 *pSampleIndex,
    NvU32 *pClientId,
    NvU32 *pClientTag,
    NvRmDfsStarvationHint *pStarvationHint)
{
    return NvError_NotSupported;
}

NvError NvRmDfsLogActivityGetEntry(
    NvRmDeviceHandle hRmDeviceHandle,
    NvU32 EntryIndex,
    NvU32 LogDomainsCount,
    NvU32 *pIntervalMs,
    NvU32 * pLp2TimeMs,
    NvU32 *pActiveCyclesList,
    NvRmFreqKHz *pAveragesList,
    NvRmFreqKHz *pFrequenciesList)
{
    return NvError_NotSupported;
}

NvError NvRmDfsLogGetMeanFrequencies(
    NvRmDeviceHandle hRmDeviceHandle,
    NvU32 LogMeanFreqListCount,
    NvRmFreqKHz *pLogMeanFreqList,
    NvU32 *pLogLp2TimeMs,
    NvU32 *pLogLp2Entries )
{
    return NvError_NotSupported;
}

void NvRmDfsLogStart(NvRmDeviceHandle hRmDeviceHandle)
{
}

NvError NvRmDfsGetProfileData(
    NvRmDeviceHandle hRmDeviceHandle,
    NvU32 DfsProfileCount,
    NvU32 *pSamplesNoList,
    NvU32 *pProfileTimeUsList,
    NvU32 *pDfsPeriodUs)
{
    return NvError_NotSupported;
}

NvError NvRmDfsSetAvHighCorner(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmFreqKHz DfsSystemHighKHz,
    NvRmFreqKHz DfsAvpHighKHz,
    NvRmFreqKHz DfsVpipeHighKHz)
{
    return NvSuccess;
}

NvError NvRmDfsSetCpuEmcHighCorner(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmFreqKHz DfsCpuHighKHz,
    NvRmFreqKHz DfsEmcHighKHz)
{
    return NvSuccess;
}

NvError NvRmDfsSetEmcEnvelope(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmFreqKHz DfsEmcLowCornerKHz,
    NvRmFreqKHz DfsEmcHighCornerKHz)
{
    return NvSuccess;
}

NvError NvRmDfsSetCpuEnvelope(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmFreqKHz DfsCpuLowCornerKHz,
    NvRmFreqKHz DfsCpuHighCornerKHz)
{
    return NvSuccess;
}

NvError NvRmDfsSetTarget(
    NvRmDeviceHandle hRmDeviceHandle,
    NvU32 DfsFreqListCount,
    const NvRmFreqKHz * pDfsTargetFreqList)
{
    return NvSuccess;
}

NvError NvRmDfsSetLowCorner(
    NvRmDeviceHandle hRmDeviceHandle,
    NvU32 DfsFreqListCount,
    const NvRmFreqKHz * pDfsLowFreqList)
{
    return NvSuccess;
}

NvError NvRmDfsSetState(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmDfsRunState NewDfsRunState)
{
    return NvSuccess;
}

NvError NvRmDfsGetClockUtilization(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmDfsClockId ClockId,
    NvRmDfsClockUsage *pClockUsage)
{
    return NvError_NotSupported;
}

NvRmDfsRunState NvRmDfsGetState(NvRmDeviceHandle hRmDeviceHandle)
{
    return NvRmDfsRunState_Disabled;
}

NvError NvRmPowerActivityHint(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmModuleID ModuleId,
    NvU32 ClientId,
    NvU32 ActivityDurationMs)
{
    return NvSuccess;
}

NvError NvRmPowerStarvationHintMulti(
    NvRmDeviceHandle hRmDeviceHandle,
    NvU32 ClientId,
    const NvRmDfsStarvationHint *pMultiHint,
    NvU32 NumHints)
{
    return NvSuccess;
}

NvError NvRmPowerStarvationHint(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmDfsClockId ClockId,
    NvU32 ClientId,
    NvBool Starving)
{
    return NvSuccess;
}

NvError NvRmPowerBusyHintMulti(
    NvRmDeviceHandle hRmDeviceHandle,
    NvU32 ClientId,
    const NvRmDfsBusyHint *pMultiHint,
    NvU32 NumHints,
    NvRmDfsBusyHintSyncMode Mode)
{
    return NvSuccess;
}

NvError NvRmPowerBusyHint(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmDfsClockId ClockId,
    NvU32 ClientId,
    NvU32 BoostDurationMs,
    NvRmFreqKHz BoostKHz)
{
    return NvSuccess;
}

void NvRmListPowerAwareModules(
    NvRmDeviceHandle hRmDeviceHandle,
    NvU32 *pListSize,
    NvRmModuleID *pIdList,
    NvBool *pActiveList)
{
    *pListSize = 0;
}

NvError NvRmPowerVoltageControl(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmModuleID ModuleId,
    NvU32 ClientId,
    NvRmMilliVolts MinVolts,
    NvRmMilliVolts MaxVolts,
    const NvRmMilliVolts *PrefVoltageList,
    NvU32 PrefVoltageListCount,
    NvRmMilliVolts *CurrentVolts)
{
    if (CurrentVolts)
    {
        if (PrefVoltageListCount)
            *CurrentVolts = PrefVoltageList[0];
        else
            *CurrentVolts = MinVolts;
    }

    return NvSuccess;
}

static NvError do_kernel_clock_control(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmModuleID ModuleId,
    NvU32 ClientId,
    NvBool Enable)
{
    NvError err_;
    NvU32 i_size_ = OFFSET(NvRmPowerModuleClockControl_params, inout);
    NvU32 io_size_ = OFFSET(NvRmPowerModuleClockControl_params, out) -
        OFFSET(NvRmPowerModuleClockControl_params, inout);
    NvU32 o_size_ = sizeof(NvRmPowerModuleClockControl_params) -
        OFFSET(NvRmPowerModuleClockControl_params, out);
    NvRmPowerModuleClockControl_params p_;

    p_.in.package_ = NvRm_nvrm_power;
    p_.in.function_ = 8;
    p_.in.hRmDeviceHandle = hRmDeviceHandle;
    p_.in.ModuleId = ModuleId;
    p_.in.ClientId = ClientId;
    p_.in.Enable = Enable;

    NvIdlIoctlFile();

    err_ = NvOsIoctl(gs_IoctlFile, gs_IoctlCode, &p_, i_size_,
                     io_size_, o_size_);
    NV_ASSERT( err_ == NvSuccess );

    return p_.out.ret_;
}

NvError NvRmPowerModuleClockControl(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmModuleID ModuleId,
    NvU32 ClientId,
    NvBool Enable)
{

    if (supported_module(ModuleId)) {
        return do_kernel_clock_control(hRmDeviceHandle, ModuleId,
                                       ClientId, Enable);
    }

    if (NVRM_MODULE_ID_MODULE(ModuleId)!=NvRmModuleID_GraphicsHost)
        NvOsDebugPrintf("%s %s MOD[%u] INST[%u]\n", __func__,
                        (Enable)?"on" : "off",
                        NVRM_MODULE_ID_MODULE(ModuleId),
                        NVRM_MODULE_ID_INSTANCE(ModuleId));
    return NvSuccess;
}

static NvError do_kernel_clock_config(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmModuleID ModuleId,
    NvU32 ClientId,
    NvRmFreqKHz MinFreq,
    NvRmFreqKHz MaxFreq,
    const NvRmFreqKHz *PrefFreqList,
    NvU32 PrefFreqListCount,
    NvRmFreqKHz *CurrentFreq,
    NvU32 flags)
{
    NvError err_;
    NvU32 i_size_ = OFFSET(NvRmPowerModuleClockConfig_params, inout);
    NvU32 io_size_ = OFFSET(NvRmPowerModuleClockConfig_params, out) -
        OFFSET(NvRmPowerModuleClockConfig_params, inout);
    NvU32 o_size_ = sizeof(NvRmPowerModuleClockConfig_params) -
        OFFSET(NvRmPowerModuleClockConfig_params, out);
    NvRmPowerModuleClockConfig_params p_;

    p_.in.package_ = NvRm_nvrm_power;
    p_.in.function_ = 7;
    p_.in.hRmDeviceHandle = hRmDeviceHandle;
    p_.in.ModuleId = ModuleId;
    p_.in.ClientId = ClientId;
    p_.in.MinFreq = MinFreq;
    p_.in.MaxFreq = MaxFreq;
    p_.in.PrefFreqList = ( NvRmFreqKHz * )PrefFreqList;
    p_.in.PrefFreqListCount = PrefFreqListCount;
    p_.in.flags = flags;

    NvIdlIoctlFile();

    err_ = NvOsIoctl(gs_IoctlFile, gs_IoctlCode, &p_, i_size_,
                     io_size_, o_size_);
    NV_ASSERT( err_ == NvSuccess );
    if(CurrentFreq)
    {
        *CurrentFreq = p_.out.CurrentFreq;
    }

    return p_.out.ret_;
}

NvError NvRmPowerModuleClockConfig(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmModuleID ModuleId,
    NvU32 ClientId,
    NvRmFreqKHz MinFreq,
    NvRmFreqKHz MaxFreq,
    const NvRmFreqKHz *PrefFreqList,
    NvU32 PrefFreqListCount,
    NvRmFreqKHz *CurrentFreq,
    NvU32 flags)
{

    if (supported_module(ModuleId)) {
        return do_kernel_clock_config(hRmDeviceHandle, ModuleId, ClientId,
                                      MinFreq, MaxFreq, PrefFreqList,
                                      PrefFreqListCount, CurrentFreq, flags);
    }

    if (CurrentFreq) {
        if (PrefFreqList)
            *CurrentFreq = PrefFreqList[0];
        else
            *CurrentFreq = MinFreq;
    }

    return NvSuccess;
}

NvRmFreqKHz NvRmPowerModuleGetMaxFrequency(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmModuleID ModuleId)
{
    if(ModuleId == NvRmModuleID_Mpe)
        return 250000;
    if(ModuleId == NvRmModuleID_Vde)
        return 250000;
    return 0;
}

NvRmFreqKHz NvRmPowerGetPrimaryFrequency(NvRmDeviceHandle hRmDeviceHandle)
{
    return 0;
}

NvError NvRmPowerGetState(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmPowerState *pState)
{
    *pState = NvRmPowerState_Active;
    return NvSuccess;
}

void NvRmPowerEventNotify(
    NvRmDeviceHandle hRmDeviceHandle,
    NvRmPowerEvent Event)
{
}

NvError NvRmPowerGetEvent(
    NvRmDeviceHandle hRmDeviceHandle,
    NvU32 ClientId,
    NvRmPowerEvent *pEvent)
{
    *pEvent = NvRmPowerEvent_NoEvent;
    return NvSuccess;
}

void NvRmPowerUnRegister(NvRmDeviceHandle hRmDeviceHandle, NvU32 ClientId)
{
}

NvError NvRmPowerRegister(
    NvRmDeviceHandle hRmDeviceHandle,
    NvOsSemaphoreHandle hEventSemaphore,
    NvU32 *pClientId)
{
    if (pClientId)
        *pClientId = 0xdeadbeef;

    return NvSuccess;
}
