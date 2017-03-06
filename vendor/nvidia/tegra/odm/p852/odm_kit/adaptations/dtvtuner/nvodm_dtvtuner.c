/*
 * Copyright (c) 2006-2011 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvodm_services.h"
#include "nvodm_query_discovery.h"
#include "nvodm_dtvtuner.h"
#include "nvodm_dtvtuner_int.h"
#include "nvodm_dtvtuner_murata.h"

#define CURRENT_TUNER_GUID DTV_MURATA_057_GUID

/** Open tv tuner.
 *  Will open the current tuner.
 *  @return Device handle on success, or NULL on failure.
*/
NvOdmDtvtunerHandle NvOdmDtvtunerOpen(void)
{
    NvOdmDtvtunerContext *pDC = NULL;
    NvBool errorCode = NV_FALSE;

    pDC = NvOdmOsAlloc(sizeof(NvOdmDtvtunerContext));
    if (pDC)
    {
        NvOdmOsMemset(pDC, 0, sizeof(NvOdmDtvtunerContext));
    }
    else
    {
        NVODMTVTUNER_PRINTF(("ERROR NvOdmDtvtunerOpen: NvOdmOsAlloc fail\n"));
        errorCode = NV_TRUE;
        goto EXIT;
    }

    pDC->CurrnetGUID = CURRENT_TUNER_GUID;

    // Have GUID, now get tuner connectivity info:
    pDC->HWctx.pPeripheralConnectivity = NvOdmPeripheralGetGuid(pDC->CurrnetGUID);
    if( !pDC->HWctx.pPeripheralConnectivity )
    {
        NVODMTVTUNER_PRINTF(("ERROR NvOdmDtvtunerOpen: NvOdmPeripheralGetGuid fail!\n"));
        errorCode = NV_TRUE;
        goto EXIT;
    }

    pDC->CurrentPowerLevel = NvOdmDtvtunerPowerLevel_Off;
    switch (pDC->CurrnetGUID)
    {
        case DTV_MURATA_057_GUID:
            //Murata is connected via VIP:
            pDC->HWctx.hPmu = NvOdmServicesPmuOpen();
            if (!pDC->HWctx.hPmu) {
                errorCode = NV_TRUE;
                goto EXIT;
            }
            pDC->SetPowerLevel = NvOdmMurataSetPowerLevel;
            pDC->GetCap = NvOdmMurataGetCap;
            pDC->Init = NvOdmMurataInit;
            pDC->SetSegmentChannel = NvOdmMurataSetSegmentChannel;
            pDC->SetChannel = NvOdmMurataSetChannel;
            pDC->GetSignalStat = NvOdmMurataGetSignalStat;
            break;
        default:
            break;
    }

EXIT:
    if (errorCode)
    {
        if (pDC) {
            NvOdmServicesPmuClose(pDC->HWctx.hPmu);
            NvOdmI2cClose(pDC->HWctx.hI2C);
            NvOdmI2cClose(pDC->HWctx.hI2CPMU);
            NvOdmOsFree(pDC);
        }
        return NULL;
    }
    else
        return pDC;
}

/** Close tv tuner.
    @param phDtvtuner Pointer to tv tuner device handle, will be reset to NULL, \a *phDtvtuner can be NULL
*/
void NvOdmDtvtunerClose(NvOdmDtvtunerHandle *phDtvtuner)
{
    NvOdmDtvtunerContext *pDC = *phDtvtuner;

    if (pDC) {
        NvOdmServicesPmuClose(pDC->HWctx.hPmu);
        NvOdmI2cClose(pDC->HWctx.hI2C);
        NvOdmI2cClose(pDC->HWctx.hI2CPMU);
        NvOdmOsFree(pDC);
    }
    pDC = NULL;
}

/** Sets tv tuner to on, off, or suspend.
 * @param hDtvtuner TV tuner device handle
 * @param PowerLevel Specifies the power level to set.
*/
void NvOdmDtvtunerSetPowerLevel(
        NvOdmDtvtunerHandle hDtvtuner,
        NvOdmDtvtunerPowerLevel PowerLevel)
{
    if (!hDtvtuner) return;
    if (!hDtvtuner->SetPowerLevel) return;
    if(hDtvtuner->CurrentPowerLevel == PowerLevel) return;

    hDtvtuner->SetPowerLevel(&hDtvtuner->HWctx, PowerLevel);
    hDtvtuner->CurrentPowerLevel = PowerLevel;
}

/** Changes a tuner device setting.
 * @param hDtvtuner Dtvtuner device handle
 * @param Param A parameter ID.
 * @param SizeOfValue The byte size of buffer in \a value.
 * @param pValue A pointer to the buffer to hold parameter value.
 * @return NV_TRUE if successful, or NV_FALSE otherwise, such as if the
 * parameter is not supported or the parameter is not set.
 */
NvBool NvOdmDtvtunerSetParameter(
        NvOdmDtvtunerHandle hDtvtuner,
        NvOdmDtvtunerParameter Param,
        NvU32 SizeOfValue,
        const void *pValue)
{
    NvBool status = NV_FALSE;

    if (!hDtvtuner) return NV_FALSE;

    switch (Param)
    {
    case NvOdmDtvtunerParameter_ISDBTInit:
        {
            const NvOdmDtvtunerISDBTInit *init = (NvOdmDtvtunerISDBTInit*)pValue;

            if ((!hDtvtuner->Init) ||
                (SizeOfValue != sizeof(NvOdmDtvtunerISDBTInit)))
                return NV_FALSE;

            status = hDtvtuner->Init(&hDtvtuner->HWctx, init->seg, init->channel);
            if(status == NV_TRUE)
            {
                hDtvtuner->CurrentSeg = init->seg;
                hDtvtuner->CurrentChannel = init->channel;
            }
        }
        break;
    case NvOdmDtvtunerParameter_ISDBTSetSegChPid:
        {
            const NvOdmDtvtunerISDBTSetSegChPid *segchpid = (NvOdmDtvtunerISDBTSetSegChPid*)pValue;

            if ((!hDtvtuner->SetSegmentChannel) ||
                (SizeOfValue != sizeof(NvOdmDtvtunerISDBTSetSegChPid)))
                return NV_FALSE;
            if (hDtvtuner->CurrentSeg == segchpid->seg)
            {
                //Tuner responds faster if skip seg setting:
                status = hDtvtuner->SetChannel(&hDtvtuner->HWctx, segchpid->channel, segchpid->PID);
            }
            else
            {
                status = hDtvtuner->SetSegmentChannel(&hDtvtuner->HWctx, segchpid->seg, segchpid->channel, segchpid->PID);
            }
            if(status == NV_TRUE)
            {
                hDtvtuner->CurrentSeg = segchpid->seg;
                hDtvtuner->CurrentChannel = segchpid->channel;
            }
        }
        break;
    default:
        status = NV_FALSE; // parameter not supported
        break;
    }
    return status;
}

/** Queries a variable-sized dtvtuner device parameter.
 * @param hDtvtuner Dtvtuner device handle
 * @param Param The parameter ID.
 * @param SizeOfValue The byte size of buffer in \a value.
 * @param pValue A pointer to the buffer to receive the parameter value.
 * @return NV_TRUE if successful, NV_FALSE otherwise.
 */
NvBool NvOdmDtvtunerGetParameter(
        NvOdmDtvtunerHandle hDtvtuner,
        NvOdmDtvtunerParameter Param,
        NvU32 SizeOfValue,
        void *pValue)
{
    NvBool status = NV_FALSE;

    if (!hDtvtuner) return NV_FALSE;

    switch (Param)
    {
    case NvOdmDtvtunerParameter_Cap:
        if ((!hDtvtuner->GetCap) ||
            (SizeOfValue != sizeof(NvOdmDtvtunerCap)))
            return NV_FALSE;
        status = hDtvtuner->GetCap(&hDtvtuner->HWctx, pValue);;
        break;

    case NvOdmDtvtunerParameter_DtvtunerStat:
        if ((!hDtvtuner->GetSignalStat) ||
            (SizeOfValue != sizeof(NvOdmDtvtunerStat)))
            return NV_FALSE;
        status = hDtvtuner->GetSignalStat(&hDtvtuner->HWctx, pValue);
        break;

    default:
        status = NV_FALSE; // parameter not supported
        break;
    }
    return status;
}

/**
 * I2C Helper functions
 */

NvOdmI2cStatus NvOdmTVI2cWrite8(
    NvOdmServicesI2cHandle hI2C,
    NvU8 devAddr,
    NvU8 regAddr,
    NvU8 Data)
{
    NvU8 WriteBuffer[2];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;

    WriteBuffer[0] = regAddr;
    WriteBuffer[1] = Data;

    TransactionInfo.Address = devAddr;
    TransactionInfo.Buf = WriteBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 2;

    Error = NvOdmI2cTransaction(hI2C, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NVODMTVTUNER_PRINTF(("NvOdmTVI2cWrite8 Fail: reg 0x%x data 0x%x\n", regAddr, Data));
        return Error;
    }
    return Error;
}

NvOdmI2cStatus NvOdmTVI2cRead8(
    NvOdmServicesI2cHandle hI2C,
    NvU8 devAddr,
    NvU8 regAddr,
    NvU8 *Data)
{
    NvU8 ReadBuffer[1];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;

    ReadBuffer[0] = regAddr;

    TransactionInfo.Address = devAddr;
    TransactionInfo.Buf = ReadBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 1;

    Error = NvOdmI2cTransaction(hI2C, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NVODMTVTUNER_PRINTF(("NvOdmTVI2cRead8 Fail\n"));
        return Error;
    }

    NvOdmOsMemset(ReadBuffer, 0, sizeof(ReadBuffer));

    TransactionInfo.Address = (devAddr | 0x1);
    TransactionInfo.Buf = ReadBuffer;
    TransactionInfo.Flags = 0;
    TransactionInfo.NumBytes = 1;

    // Read data from the specified offset
    Error = NvOdmI2cTransaction(hI2C, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NVODMTVTUNER_PRINTF(("NvOdmTVI2cRead8: Receive Failed\n"));
        return Error;
    }
    *Data = ReadBuffer[0];
    return Error;
}

/** Reference Implementation for the NvOdm TV Tuner */
