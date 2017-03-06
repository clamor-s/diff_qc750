/**
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/*
 * set.c - State setting support for the nvbuildbct
 */

/*
 * TODO / Notes
 * - Add doxygen commentary
 */

#include "math.h"


#include "nvapputil.h"
#include "nvassert.h"
#include "ap20_set.h"
#include "ap20/nvboot_bct.h"
#include "ap20/nvboot_sdram_param.h"
#include "../nvbuildbct.h"
#include "ap20_data_layout.h"
#include "nvbct_customer_platform_data.h"
#include "../nvbuildbct_util.h"

#define NV_MAX(a, b) (((a) > (b)) ? (a) : (b))

void
Ap20ComputeRandomAesBlock(BuildBctContext *Context)
{
    NvU32 i;

    for (i = 0; i < NVBOOT_CMAC_AES_HASH_LENGTH; i++)
    {
        ((NvBootHash*)(Context->RandomBlock))->hash[i] = rand();
    }
}

int
Ap20SetRandomAesBlock(BuildBctContext *Context, NvU32 Value, NvU8* AesBlock)
{
    Context->RandomBlockType = (RandomAesBlockType)Value;

    switch (Context->RandomBlockType)
    {
        case RandomAesBlockType_Zeroes:
            NvOsMemset(&(Context->RandomBlock[0]), 0, CMAC_HASH_LENGTH_BYTES);
            break;

        case RandomAesBlockType_Literal:
            NvOsMemcpy(&(Context->RandomBlock[0]), AesBlock, CMAC_HASH_LENGTH_BYTES);
            break;

        case RandomAesBlockType_Random:
            /* This type will be recomputed whenever needed. */
            break;

        case RandomAesBlockType_RandomFixed:
            /* Compute this once now. */
            Ap20ComputeRandomAesBlock(Context);
            break;

        default:
            NV_ASSERT(!"Unknown random block type.");
            return 1;
    }

    return 0;
}

/*
 * SetBootLoader(): Processes commands to set a bootloader.
 */
int
Ap20SetBootLoader(BuildBctContext *Context,
              NvU8              *Filename,
              NvU32              LoadAddress,
              NvU32              EntryPoint)
{
    NvOsMemcpy(Context->NewBlFilename, Filename, MAX_BUFFER);
    Context->NewBlLoadAddress = LoadAddress;
    Context->NewBlEntryPoint = EntryPoint;
    Context->NewBlStartBlk = 0;
    Context->NewBlStartPg = 0;
    Ap20UpdateBl(Context, (UpdateEndState)0);

    return 0;
}

/*
 * SetSdmmcParam(): Processes commands to set Sdmmc parameters.
 */
int
Ap20SetSdmmcParam(BuildBctContext *Context,
                 NvU32              Index,
                 ParseToken         Token,
                 NvU32              Value)
{
    NvBootSdmmcParams *Params;
    NvBootConfigTable *Bct = NULL;
    Bct = (NvBootConfigTable*)(Context->NvBCT);

    NV_ASSERT(Context      != NULL);
    NV_ASSERT(Bct != NULL);

    Params = &(Bct->DevParams[Index].SdmmcParams);

    Bct->NumParamSets = NV_MAX(Bct->NumParamSets, Index + 1);

    /* The result of the TODO processing is to get a token & value. */
    switch (Token)
    {
        case Token_ClockDivider:
            Params->ClockDivider = Value;
            break;

        case Token_DataWidth:
            if((Value != 0) && (Value != 1) && (Value != 2))
            {
               NvAuPrintf("\nError: Value entered for DeviceParam[%d].SdmmcParams.DataWidth is not valid. Valid values are 1, 4 & 8. \n ",Index);
               NvAuPrintf("\nBCT file was not created. \n");
               exit(1);
            }
            Params->DataWidth = Value;
            break;
        case Token_MaxPowerClassSupported:
            Params->MaxPowerClassSupported = Value;
            break;
        default:
            NvAuPrintf("Unexpected token %d at line %d\n",
                       Token, __LINE__);

            return 1;
    }

    return 0;
}

/*
 * SetNandParam(): Processes commands to set Nand parameters.
 */
int
Ap20SetNandParam(BuildBctContext *Context,
             NvU32              Index,
             ParseToken         Token,
             NvU32              Value)
{
    NvBootNandParams *Params;
    NvBootConfigTable *Bct = NULL;

    Bct = (NvBootConfigTable*)(Context->NvBCT);

    NV_ASSERT(Context      != NULL);
    NV_ASSERT(Bct != NULL);

    Params = &(Bct->DevParams[Index].NandParams);

    Bct->NumParamSets = NV_MAX(Bct->NumParamSets, Index + 1);

    /* The result of the TODO processing is to get a token & value. */
    switch (Token)
    {
        case Token_ClockDivider:
            Params->ClockDivider = (NvU8)Value;
            break;

        case Token_NandTiming:
            Params->NandTiming = Value;
            break;

        case Token_NandTiming2:
            Params->NandTiming2 = Value;
            break;

        case Token_BlockSizeLog2:
            Params->BlockSizeLog2 = (NvU8)Value;
            break;

        case Token_PageSizeLog2:
            Params->PageSizeLog2 = (NvU8)Value;
            break;

        default:
            NvAuPrintf("Unexpected token %d at line %d\n",
                       Token, __LINE__);

            return 1;
    }

    return 0;
}

/*
 * SetNor(): Processes commands to set Nor parameters.
 */
int
Ap20SetNorParam(BuildBctContext *Context,
            NvU32              Index,
            ParseToken         Token,
            NvU32              Value)
{
    NvBootSnorParams *Params;
    NvBootConfigTable *Bct = NULL;
    Bct = (NvBootConfigTable*)(Context->NvBCT);

    NV_ASSERT(Context      != NULL);
    NV_ASSERT(Bct != NULL);

    Params = &(Bct->DevParams[Index].SnorParams);

    Bct->NumParamSets = NV_MAX(Bct->NumParamSets, Index + 1);

    /* The result of the TODO processing is to get a token & value. */
    switch (Token)
    {
        case Token_NorTiming:
    //        Params->NorTiming = Value;
            break;

        default:
            NvAuPrintf("Unexpected token %d at line %d\n",
                       Token, __LINE__);

            return 1;
    }

    return 0;
}


/*
 * SetSpiFlashParam(): Processes commands to set SpiFlash parameters.
 */
int
Ap20SetSpiFlashParam(BuildBctContext *Context,
                 NvU32              Index,
                 ParseToken         Token,
                 NvU32              Value)
{
    NvBootSpiFlashParams *Params;
    NvBootConfigTable *Bct = NULL;
    Bct = (NvBootConfigTable*)(Context->NvBCT);

    NV_ASSERT(Context      != NULL);
    NV_ASSERT(Bct != NULL);

    Params = &(Bct->DevParams[Index].SpiFlashParams);

    Bct->NumParamSets = NV_MAX(Bct->NumParamSets, Index + 1);

    /* The result of the TODO processing is to get a token & value. */
    switch (Token)
    {
        case Token_ClockDivider:
            if((Value != 9) && (Value != 11) && (Value != 16) && (Value != 22))
            {
               NvAuPrintf("\nError: Value entered for DeviceParam[%d].SpiFlashParams.ClockDivider is not valid. Valid values are 9, 11, 16 & 22. \n ",Index);
               NvAuPrintf("\nBCT file was not created. \n");
               exit(1);
            }
            Params->ClockDivider = (NvU8)Value;
            break;

        case Token_ClockSource:
            Params->ClockSource= (NvBootSpiClockSource)Value;
            break;

        case Token_ReadCommandTypeFast:
            Params->ReadCommandTypeFast = (NvBool)Value;
            break;

        default:
            NvAuPrintf("Unexpected token %d at line %d\n",
                       Token, __LINE__);

            return 1;
    }

    return 0;
}

#define SET_PARAM_U32(z) case Token_##z: Params->z = Value; break
#define SET_PARAM_ENUM(z,t) case Token_##z: Params->z = (t)Value; break
#define SET_PARAM_BOOL(z) case Token_##z: Params->z = (NvBool)Value; break

/*
 * SetSdramParam(): Sets an SDRAM parameter.
 */
int
Ap20SetSdramParam(BuildBctContext *Context,
              NvU32              Index,
              ParseToken         Token,
              NvU32              Value)
{
    NvBootSdramParams *Params;
    NvBootConfigTable *Bct = NULL;
    Bct = (NvBootConfigTable*)(Context->NvBCT);

    NV_ASSERT(Context      != NULL);
    NV_ASSERT(Bct != NULL);

    Params = &(Bct->SdramParams[Index]);

    Bct->NumSdramSets = NV_MAX(Bct->NumSdramSets, Index + 1);

    switch (Token)
    {
        SET_PARAM_U32(MemoryType);
        SET_PARAM_U32(PllMChargePumpSetupControl);
        SET_PARAM_U32(PllMLoopFilterSetupControl);
        SET_PARAM_U32(PllMInputDivider);
        SET_PARAM_U32(PllMFeedbackDivider);
        SET_PARAM_U32(PllMPostDivider);
        SET_PARAM_U32(PllMStableTime);
        SET_PARAM_U32(EmcClockDivider);
        SET_PARAM_U32(EmcAutoCalInterval);
        SET_PARAM_U32(EmcAutoCalConfig);
        SET_PARAM_U32(EmcAutoCalWait);
        SET_PARAM_U32(EmcPinProgramWait);
        SET_PARAM_U32(EmcRc);
        SET_PARAM_U32(EmcRfc);
        SET_PARAM_U32(EmcRas);
        SET_PARAM_U32(EmcRp);
        SET_PARAM_U32(EmcR2w);
        SET_PARAM_U32(EmcW2r);
        SET_PARAM_U32(EmcR2p);
        SET_PARAM_U32(EmcW2p);
        SET_PARAM_U32(EmcRdRcd);
        SET_PARAM_U32(EmcWrRcd);
        SET_PARAM_U32(EmcRrd);
        SET_PARAM_U32(EmcRext);
        SET_PARAM_U32(EmcWdv);
        SET_PARAM_U32(EmcQUseExtra);
        SET_PARAM_U32(EmcQUse);
        SET_PARAM_U32(EmcQRst);
        SET_PARAM_U32(EmcQSafe);
        SET_PARAM_U32(EmcRdv);
        SET_PARAM_U32(EmcRefresh);
        SET_PARAM_U32(EmcBurstRefreshNum);
        SET_PARAM_U32(EmcPdEx2Wr);
        SET_PARAM_U32(EmcPdEx2Rd);
        SET_PARAM_U32(EmcPChg2Pden);
        SET_PARAM_U32(EmcAct2Pden);
        SET_PARAM_U32(EmcAr2Pden);
        SET_PARAM_U32(EmcRw2Pden);
        SET_PARAM_U32(EmcTxsr);
        SET_PARAM_U32(EmcTcke);
        SET_PARAM_U32(EmcTfaw);
        SET_PARAM_U32(EmcTrpab);
        SET_PARAM_U32(EmcTClkStable);
        SET_PARAM_U32(EmcTClkStop);
        SET_PARAM_U32(EmcTRefBw);
        SET_PARAM_U32(EmcFbioCfg1);
        SET_PARAM_U32(EmcFbioDqsibDlyMsb);
        SET_PARAM_U32(EmcFbioDqsibDly);
        SET_PARAM_U32(EmcFbioQuseDlyMsb);
        SET_PARAM_U32(EmcFbioQuseDly);
        SET_PARAM_U32(EmcFbioCfg5);
        SET_PARAM_U32(EmcFbioCfg6);
        SET_PARAM_U32(EmcFbioSpare);
        SET_PARAM_U32(EmcMrsResetDllWait); // microseconds
        SET_PARAM_U32(EmcMrsResetDll);
        SET_PARAM_U32(EmcMrsDdr2DllReset);
        SET_PARAM_U32(EmcMrs);
        SET_PARAM_U32(EmcEmrsEmr2);
        SET_PARAM_U32(EmcEmrsEmr3);
        SET_PARAM_U32(EmcEmrsDdr2DllEnable);
        SET_PARAM_U32(EmcEmrsDdr2OcdCalib);
        SET_PARAM_U32(EmcEmrs);
        SET_PARAM_U32(EmcMrw1);
        SET_PARAM_U32(EmcMrw2);
        SET_PARAM_U32(EmcMrw3);
        SET_PARAM_U32(EmcMrwResetCommand);
        SET_PARAM_U32(EmcMrwResetNInitWait);
        SET_PARAM_U32(EmcAdrCfg1);
        SET_PARAM_U32(EmcAdrCfg);
        SET_PARAM_U32(McEmemCfg);
        SET_PARAM_U32(McLowLatencyConfig);
        SET_PARAM_U32(EmcCfg2);
        SET_PARAM_U32(EmcCfgDigDll);

        // Clock trimmers
        SET_PARAM_U32(EmcCfgClktrim0);
        SET_PARAM_U32(EmcCfgClktrim1);
        SET_PARAM_U32(EmcCfgClktrim2);
        SET_PARAM_U32(EmcCfg);

        SET_PARAM_U32(EmcDbg);
        SET_PARAM_U32(AhbArbitrationXbarCtrl);
        SET_PARAM_U32(EmcDllXformDqs);
        SET_PARAM_U32(EmcDllXformQUse);
        SET_PARAM_U32(WarmBootWait);
        SET_PARAM_U32(EmcCttTermCtrl);
        SET_PARAM_U32(EmcOdtWrite);
        SET_PARAM_U32(EmcOdtRead);
        SET_PARAM_U32(EmcZcalRefCnt);   // should be 0 for non-lpddr2  memory types
        SET_PARAM_U32(EmcZcalWaitCnt);
        SET_PARAM_U32(EmcZcalMrwCmd);

        SET_PARAM_U32(EmcMrwZqInitDev0);
        SET_PARAM_U32(EmcMrwZqInitDev1);
        SET_PARAM_U32(EmcMrwZqInitWait);   // microseconds


        SET_PARAM_U32(EmcDdr2Wait);        // microseconds

        // Pad controls
        SET_PARAM_U32(PmcDdrPwr);
        SET_PARAM_U32(ApbMiscGpXm2CfgAPadCtrl);
        SET_PARAM_U32(ApbMiscGpXm2CfgCPadCtrl2);
        SET_PARAM_U32(ApbMiscGpXm2CfgCPadCtrl);
        SET_PARAM_U32(ApbMiscGpXm2CfgDPadCtrl2);
        SET_PARAM_U32(ApbMiscGpXm2CfgDPadCtrl);

        SET_PARAM_U32(ApbMiscGpXm2ClkCfgPadCtrl);
        SET_PARAM_U32(ApbMiscGpXm2CompPadCtrl);
        SET_PARAM_U32(ApbMiscGpXm2VttGenPadCtrl);
        default:
            NvAuPrintf("Unexpected token %d at line %d\n",
                       Token, __LINE__);
            return 1;
    }

    return 0;
}

/*
 * SetValue(): General handler for setting values in config files.
 */
int Ap20SetValue(BuildBctContext *Context, ParseToken Token, NvU32 Value)
{
    NvBootConfigTable *Bct = NULL;
    Bct = (NvBootConfigTable*)(Context->NvBCT);

    NV_ASSERT(Context != NULL);

    switch (Token)
    {

    case Token_Attribute:
        Context->NewBlAttribute = Value;
        break;

    case Token_BlockSize:
        Context->BlockSize     = Value;
        Context->BlockSizeLog2 = Log2(Value);

        if (Context->Memory != NULL)
        {
            NvAuPrintf("Error: Too late to change block size.\n");
            return 1;
        }

        if (Value != (NvU32)(1 << Context->BlockSizeLog2))
        {
            NvAuPrintf("Error: Block size must be a power of 2.\n");
            return 1;
        }

        Context->PagesPerBlock = 1 << (Context->BlockSizeLog2 -
                                       Context->PageSizeLog2);

        break;

    case Token_EnableJtag:
        NV_ASSERT(Bct != NULL);
//        Bct->EnableJtag = (NvBool) Value;
        break;

    case Token_PageSize:
        Context->PageSize     = Value;
        Context->PageSizeLog2 = Log2(Value);

        if (Context->Memory != NULL)
        {
            NvAuPrintf("Error: Too late to change block size.\n");
            return 1;
        }

        if (Value != (NvU32)(1 << Context->PageSizeLog2))
        {
            NvAuPrintf("Error: Page size must be a power of 2.\n");
            return 1;
        }

        Context->PagesPerBlock = 1 << (Context->BlockSizeLog2 -
                                       Context->PageSizeLog2);

        break;

    case Token_PartitionSize:
        if (Context->Memory != NULL)
        {
            NvAuPrintf("Error: Too late to change block size.\n");
            return 1;
        }

        Context->PartitionSize = Value;
        break;

    case Token_Redundancy:
        Context->Redundancy = Value;
        break;

    case Token_Version:
        Context->Version = Value;
        Context->VersionSetBeforeBL = NV_TRUE;
        break;

    case Token_CustomerDataVersion:
        {
            NvBctCustomerPlatformData* pCustomerPlatformData;
            pCustomerPlatformData = (NvBctCustomerPlatformData *)((NvBootConfigTable*)Context->NvBCT)->CustomerData;
            pCustomerPlatformData->CustomerDataVersion = NV_CUSTOMER_DATA_VERSION;
        }
        break;

    default:
        NvAuPrintf("Unknown token %d at line %d\n", Token, __LINE__);
        return 1;
    }

    return 0;
}

