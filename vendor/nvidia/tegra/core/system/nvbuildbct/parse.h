/**
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * This file contains Definitions for the nvbuildbct parsing code.
 */


#ifndef INCLUDED_PARSE_H
#define INCLUDED_PARSE_H

#include "nvbuildbct.h"

#if defined(__cplusplus)
extern "C"
{
#endif


/* Forward definitions */
typedef struct NvBootConfigTableRec BootConfigTable;

/*
 * Enum ParseToken is a superset of all
 * fields required by BCTs of all chips so far.
 */

typedef enum
{
    Token_None = 0,
    Token_AddressFormat,
    Token_AhbArbitrationXbarCtrl,
    Token_AhbArbitrationXbarCtrlRegValue,
    Token_AhbArbitrationXbarCtrlMemInitDone,
    Token_BadBlock,
    Token_BlockSize,
    Token_BootLoader,
    Token_ClockDivider,
    Token_ClockSource,
    Token_ClockRate,
    Token_DataWidth,
    Token_SdController,
    Token_DevType,
    Token_Attribute,
    Token_EmcAdrCfg1RegValue,
    Token_EmcCfg2RegValue,
    Token_EmcFbioDqsibDlyMsbRegValue,
    Token_EmcFbioQuseDlyMsbRegValue,
    Token_EmcMrw1RegValue,
    Token_EmcMrw2RegValue,
    Token_EmcMrw3RegValue,
    Token_EmcMrwResetCommandValue,
    Token_EmcMrwResetNInitWaitTimeValue,
    Token_EmcAdrCfgRegValue,
    Token_EmcAutoCalIntervalRegValue,
    Token_EmcAutoCalConfigRegValue,
    Token_EmcCfgRegValue,
    Token_EmcDbgRegValue,
    Token_EmcEmrsRegValue,
    Token_EmcFbioCfg1RegValue,
    Token_EmcFbioCfg5RegValue,
    Token_EmcFbioCfg6RegValue,
    Token_EmcFbioDqsibDlyRegValue,
    Token_EmcFbioQuseDlyRegValue,
    Token_EmcMrsRegValue,
    Token_EmcMrsResetDllValue,
    Token_McLowLatencyConfig,
    Token_EmcAct2Pden,
    Token_EmcAdrCfg1,
    Token_EmcAr2Pden,
    Token_EmcAutoCalStableTime,
    Token_EmcCfg2,
    Token_EmcCfgDigDll,
    Token_DelayAfterEmcPinProgram,
    Token_EmcConfigDigDll,
    Token_EmcCttTermCtrl,
    Token_EmcDllXformDqs,
    Token_EmcDllXformQUse,
    Token_EmcFbioDqsibDlyMsb,
    Token_EmcFbioQuseDlyMsb,
    Token_EmcFbioSpare,
    Token_EmcMrw1,
    Token_EmcMrw2,
    Token_EmcMrw3,
    Token_EmcMrwResetCommand,
    Token_EmcMrwResetNInitWait,
    Token_EmcOdtRead,
    Token_EmcOdtWrite,
    Token_EmcPChg2Pden,
    Token_EmcPdEx2Rd,
    Token_EmcPdEx2Wr,
    Token_EmcQRst,
    Token_EmcQSafe,
    Token_EmcQUse,
    Token_EmcQUseExtra,
    Token_EmcR2p,
    Token_EmcR2w,
    Token_EmcRas,
    Token_EmcRc,
    Token_EmcRdv,
    Token_EmcRefresh,
    Token_EmcRext,
    Token_EmcRfc,
    Token_EmcRp,
    Token_EmcRrd,
    Token_EmcRw2Pden,
    Token_EmcTcke,
    Token_EmcTClkStable,
    Token_EmcTClkStop,
    Token_EmcTfaw,
    Token_EmcTRefBw,
    Token_EmcTrpab,
    Token_EmcTxsr,
    Token_EmcW2p,
    Token_EmcW2r,
    Token_EmcWdv,
    Token_EmcRdRcd,
    Token_EmcWrRcd,
    Token_EmcBurstRefreshNum,
    Token_EmcZcalMrwCmd,
    Token_EmcZcalRefCnt,
    Token_EmcZcalWaitCnt,
    Token_McEmemAdrCfgRegValue,
    Token_McEmemArbCfg0RegValue,
    Token_McEmemArbCfg1RegValue,
    Token_McEmemArbCfg2RegValue,
    Token_McSecurityCfg0,
    Token_McSecurityCfg1,
    Token_MemoryType,
    Token_WarmBootWait,
    Token_DeviceParam,
    Token_EccSelection,
    Token_EmcAdrCfg,
    Token_EmcAutoCalInterval,
    Token_EmcAutoCalConfig,
    Token_EmcAutoCalWait,
    Token_EmcPinProgramWait,
    Token_EmcCfg,
    Token_EmcClockDivider,
    Token_EmcDbg,
    Token_EmcEmrs,
    Token_EmcFbioCfg1,
    Token_EmcFbioCfg5,
    Token_EmcFbioCfg6,
    Token_EmcFbioDqsibDly,
    Token_EmcFbioQuseDly,
    Token_EmcMrs,
    Token_EmcNopRegValue,
    Token_EmcPinRegValue,
    Token_EmcPreRegValue,
    Token_EmcRefCtrlRegValue,
    Token_EmcRefRegValue,
    Token_EmcTiming0RegValue,
    Token_EmcTiming1RegValue,
    Token_EmcTiming2RegValue,
    Token_EmcTiming3RegValue,
    Token_EmcTiming4RegValue,
    Token_EmcTiming5RegValue,
    Token_EmcTimingControlRegValue,
    Token_EmcMrsResetDll,
    Token_EmcMrwZqInitDev0,
    Token_EmcMrwZqInitDev1,
    Token_EmcMrwZqInitWait,
    Token_EmcMrsResetDllWait,
    Token_EmcEmrsEmr2,
    Token_EmcEmrsEmr3,
    Token_EmcEmrsDdr2DllEnable,
    Token_EmcMrsDdr2DllReset,
    Token_EmcEmrsDdr2OcdCalib,
    Token_EmcDdr2Wait,
    Token_EmcCfgClktrim0,
    Token_EmcCfgClktrim1,
    Token_EmcCfgClktrim2,
    Token_PmcDdrPwr,
    Token_EmcMrwExtra,
    Token_EmcWarmBootMrwExtra,
    Token_EmcWarmBootExtraModeRegWriteEnable,
    Token_EmcExtraModeRegWriteEnable,
    Token_EmcMrsExtra,
    Token_EmcWarmBootMrsExtra,
    Token_EmcExtraRefreshNum,
    Token_EmcClkenOverrideAllWarmBoot,
    Token_McClkenOverrideAllWarmBoot,
    Token_EmcCfgDigDllPeriodWarmBoot,
    Token_PmcNoIoPower,
    Token_ApbMiscGpXm2CfgAPadCtrl,
    Token_ApbMiscGpXm2CfgCPadCtrl,
    Token_ApbMiscGpXm2CfgCPadCtrl2,
    Token_ApbMiscGpXm2CfgDPadCtrl,
    Token_ApbMiscGpXm2CfgDPadCtrl2,
    Token_ApbMiscGpXm2ClkCfgPadCtrlv,
    Token_ApbMiscGpXm2CompPadCtrl,
    Token_ApbMiscGpXm2VttGenPadCtrl,
    Token_ApbMiscGpXm2ClkCfgPadCtrl,
    Token_EnableJtag,
    Token_EnableOnfiSupport,
    Token_HighSpeedMode,
    Token_InjectBadBlock,
    Token_InjectCrcError,
    Token_InjectDataError,
    Token_InjectEccError,
    Token_Key,
    Token_McClockDivider,
    Token_McEmemCfg,
    Token_McEmemCfgRegValue,
    Token_McLowLatencyCfg,
    Token_McLowLatencyCfgRegValue,
    Token_MaxPowerClassSupported,
    Token_McPll_PllM,
    Token_EmmcParams,
    Token_SdmmcParams,
    Token_NandParams,
    Token_NandTiming,
    Token_NandTiming2,
    Token_BlockSizeLog2,
    Token_PageSizeLog2,
    Token_NorParams,
    Token_NorTiming,
    Token_NumOfAddressCycles,
    Token_PageSize,
    Token_PartitionSize,
    Token_PllMChargePumpSetupControl,
    Token_PllMFeedbackDivider,
    Token_PllMInputDivider,
    Token_PllMLoopFilterSetupControl,
    Token_PllMPostDivider,
    Token_PllMStableTime,
    Token_RandomAesBlock,
    Token_RandomSeed,
    Token_ReadCommandTypeFast,
    Token_Redundancy,
    Token_Sdram,
    Token_SecurityMode,
    Token_SpiFlashParams,
    Token_SystemClockInMHz,
    Token_UpdateBct,
    Token_WriteImage,
    Token_Version,
    Token_UpdateOSImage,
    Token_InterleaveColumnCount,
    Token_OsMediaType,
    Token_OsMediaParam,
    Token_OsEmmcParams,
    Token_OsNandParams,
    Token_CustomerDataVersion,
    Token_EmcPinExtraWait,
    Token_EmcWext,
    Token_EmcPreRefreshReqCnt,
    Token_EmcMrsWaitCnt,
    Token_EmcDynSelfRefControl,
    Token_EmcSelDpdCtrl,
    Token_EmcDllXformDqs0,
    Token_EmcDllXformDqs1,
    Token_EmcDllXformDqs2,
    Token_EmcDllXformDqs3,
    Token_EmcDllXformDqs4,
    Token_EmcDllXformDqs5,
    Token_EmcDllXformDqs6,
    Token_EmcDllXformDqs7,
    Token_EmcDllXformQUse0,
    Token_EmcDllXformQUse1,
    Token_EmcDllXformQUse2,
    Token_EmcDllXformQUse3,
    Token_EmcDllXformQUse4,
    Token_EmcDllXformQUse5,
    Token_EmcDllXformQUse6,
    Token_EmcDllXformQUse7,
    Token_EmcDliTrimTxDqs0,
    Token_EmcDliTrimTxDqs1,
    Token_EmcDliTrimTxDqs2,
    Token_EmcDliTrimTxDqs3,
    Token_EmcDliTrimTxDqs4,
    Token_EmcDliTrimTxDqs5,
    Token_EmcDliTrimTxDqs6,
    Token_EmcDliTrimTxDqs7,
    Token_EmcDllXformDq0,
    Token_EmcDllXformDq1,
    Token_EmcDllXformDq2,
    Token_EmcDllXformDq3,
    Token_EmcZcalInterval,
    Token_EmcZcalInitDev0,
    Token_EmcZcalInitDev1,
    Token_EmcZcalInitWait,
    Token_McEmemArbTimingRas,
    Token_McEmemArbTimingFaw,
    Token_McEmemArbTimingRrd,
    Token_McEmemArbTimingRap2Pre,
    Token_McEmemArbTimingWap2Pre,
    Token_McEmemArbTimingR2R,
    Token_McEmemArbTimingW2W,
    Token_McEmemArbTimingR2W,
    Token_McEmemArbTimingW2R,
    Token_McEmemArbDaTurns,
    Token_McEmemArbDaCovers,
    Token_McEmemArbMisc0,
    Token_McEmemArbMisc1,
    Token_McEmemArbOverride,
    Token_EmcTimingControlWait,
    Token_EmcCtt,
    Token_EmcCttDuration,
    Token_EmcTxsrDll,
    Token_EmcCfgRsv,
    Token_EmcCmdQ,
    Token_EmcMc2EmcQ,
    Token_EmcCfgDigDllPeriod,
    Token_EmcDevSelect,
    Token_EmcZcalColdBootEnable,
    Token_EmcZcalWarmBootEnable,
    Token_EmcMrwLpddr2ZcalWarmBoot,
    Token_EmcZqCalDdr3WarmBoot,
    Token_EmcZcalWarmBootWait,
    Token_EmcXm2VttGenPadCtrl2,
    Token_McEmemArbRsv,
    Token_McClkenOverride,
    Token_EmcClockSource,
    Token_EmcClockUsePllMUD,
    Token_EmcWarmBootMrw1,
    Token_EmcWarmBootMrw2,
    Token_EmcWarmBootMrw3,
    Token_EmcMrsWarmBootEnable,
    Token_EmcWarmBootEmr2,
    Token_EmcWarmBootEmr3,
    Token_EmcWarmBootMrs,
    Token_EmcWarmBootEmrs,
    Token_PmcVddpSel,
    Token_PmcIoDpdReq,
    Token_PmcENoVttGen,
    Token_EmcXm2DqsPadCtrl3,

    Token_McEmemArbCfg,
    Token_McEmemArbOutstandingReq,
    Token_McEmemArbTimingRcd,
    Token_McEmemArbTimingRp,
    Token_McEmemArbTimingRc,
    Token_McEmemAdrCfg,
    Token_McEmemAdrCfgDev0,
    Token_McEmemAdrCfgDev1,
    Token_EmcXm2CmdPadCtrl,
    Token_EmcXm2CmdPadCtrl2,
    Token_EmcXm2DqsPadCtrl,
    Token_EmcXm2DqsPadCtrl2,
    Token_EmcXm2DqPadCtrl,
    Token_EmcXm2DqPadCtrl2,
    Token_EmcXm2ClkPadCtrl,
    Token_EmcXm2CompPadCtrl,
    Token_EmcXm2VttGenPadCtrl,
    Token_EmcXm2QUsePadCtrl,
    Token_EmcClkenOverride,
    Token_EmcTimingExtraDelay,
    Token_PmcDdrCfg,
    Token_McEmemArbRing1Throttle,
    Token_NandAsyncTiming0,
    Token_NandAsyncTiming1,
    Token_NandAsyncTiming2,
    Token_NandAsyncTiming3,
    Token_NandSDDRTiming0,
    Token_NandSDDRTiming1,
    Token_NandTDDRTiming0,
    Token_NandTDDRTiming1,
    Token_DisableSyncDDR,
    Token_NandFbioDqsibDlyByte,
    Token_NandFbioQuseDlyByte,
    Token_NandFbioCfgQuseLate,

    Token_SataParams,
    Token_SataClockSource,
    Token_SataClockDivider,
    Token_SataOobClockSource,
    Token_SataOobClockDivider,
    Token_SataMode,
    Token_TransferMode,
    Token_isSdramInitialized,
    Token_SataBuffersBase,
    Token_DataBufferBase,
    Token_PlleDivM,
    Token_PlleDivN,
    Token_PlleDivPl,
    Token_PlleDivPlCml,
    Token_PlleSSCoeffSscmax,
    Token_PlleSSCoeffSscinc,
    Token_PlleSSCoeffSscincintrvl,

    Token_Force32 = 0x7fffffff
} ParseToken;

typedef enum
{
    FieldType_None = 0,
    FieldType_Enum,
    FieldType_U32,
    FieldType_U8,
    FieldType_Bool,
    FiledType_Force32 = 0x7fffffff
} FieldType;


/* Forward declarations */
typedef int (*ProcessFunction)(BuildBctContext *Context,
                               ParseToken         Token,
                               char              *Remiainder,
                               char              *Prefix);

/*
 * BaseAddr is the base address of the destination of the structure into which
 * the function will write the parsed result (if successful).
 */
typedef int (*ProcessSubfieldFunction)(BuildBctContext *Context,
                                       NvU32              Index,
                                       ParseToken         Token,
                                       NvU32              Value);


/*
 * Structure definitions
 */


typedef struct
{
    char *Name;
    NvU32 Value;
} EnumItem;

typedef struct
{
    char      *Name;
    NvU32      Token;
    FieldType  Type;
    EnumItem  *EnumTable;
    NvBool             StatusZero;
    NvBool             StatusOne;
    NvBool             StatusTwo;
    NvBool             StatusThree;
} FieldItem;

typedef struct
{
    char            *Prefix;
    ParseToken       Token;
    ProcessFunction  Process;
    NvBool             Status;
} ParseItem;

typedef struct
{
    char                    *Prefix;
    ParseToken               Token;
    FieldItem               *FieldTable;
    ProcessSubfieldFunction  Process;
} ParseSubfieldItem;


#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_PARSE_H */

