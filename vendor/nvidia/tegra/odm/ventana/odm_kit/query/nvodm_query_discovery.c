/*
 * Copyright (c) 2009-2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvodm_query_gpio.h"
#include "nvodm_modules.h"
#include "nvodm_query_discovery.h"
#include "nvodm_query.h"
#include "nvrm_drf.h"
#include "include/nvodm_query_discovery_imager.h"
#include "subboards/nvodm_query_discovery_pm275_addresses.h"

static NvOdmPeripheralConnectivity s_Peripherals_Default[] =
{
#include "subboards/nvodm_query_discovery_pm275_peripherals.h"
};

static NvOdmBoardInfo s_BoardInfo;

#define NVODM_QUERY_I2C_CLOCK_SPEED      100    // kHz

#define NVODM_QUERY_ENTRY_HEADER_SIZE    0x30   // Size of EERPOM "Entry Header"
#define NVODM_QUERY_BOARD_HEADER_START   0x04   // Offset to Part Number in EERPOM

#define NVODM_QUERY_I2C_EEPROM_ADDRESS   0xA0   // I2C device base address for EEPROM (7'h50)

static NvOdmI2cStatus
NvOdmPeripheralI2cRead8(
    NvOdmServicesI2cHandle hOdmI2c,
    NvU8 I2cAddr,
    NvU8 Offset,
    NvU8 *pData)
{
    NvU8 ReadBuffer[1];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;

    ReadBuffer[0] = Offset;

    TransactionInfo.Address = I2cAddr;
    TransactionInfo.Buf = ReadBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 1;

    Error = NvOdmI2cTransaction(
        hOdmI2c, &TransactionInfo, 1, NVODM_QUERY_I2C_CLOCK_SPEED, NV_WAIT_INFINITE);
    if (Error != NvOdmI2cStatus_Success)
    {
        return Error;
    }

    NvOdmOsMemset(ReadBuffer, 0, sizeof(ReadBuffer));

    TransactionInfo.Address = (I2cAddr | 0x1);
    TransactionInfo.Buf = ReadBuffer;
    TransactionInfo.Flags = 0;
    TransactionInfo.NumBytes = 1;

    // Read data from ROM at the specified offset
    Error = NvOdmI2cTransaction(
        hOdmI2c, &TransactionInfo, 1, NVODM_QUERY_I2C_CLOCK_SPEED, NV_WAIT_INFINITE);
    if (Error != NvOdmI2cStatus_Success)
    {
        return Error;
    }
    *pData = ReadBuffer[0];
    return Error;
}

static NvBool
NvOdmPeripheralReadBoardInfo(
    NvOdmServicesI2cHandle hOdmI2c,
    NvOdmBoardInfo *pBoardInfo)
{
    NvOdmI2cStatus Error;
    NvU32 i;
    NvU8 I2cAddr, Offset;
    NvU8 ReadBuffer[sizeof(NvOdmBoardInfo)];

    NvOdmOsMemset(ReadBuffer, 0, sizeof(ReadBuffer));

    I2cAddr = NVODM_QUERY_I2C_EEPROM_ADDRESS;

    /* Offset to the board number entry in EEPROM. */
    Offset = NVODM_QUERY_BOARD_HEADER_START;

    for (i=0; i<sizeof(NvOdmBoardInfo); i++)
    {
        Error = NvOdmPeripheralI2cRead8(
            hOdmI2c, I2cAddr, Offset+i, (NvU8 *)&ReadBuffer[i]);
        if (Error != NvOdmI2cStatus_Success)
        {
            return NV_FALSE;
        }
    }

    NvOdmOsMemcpy(pBoardInfo, &ReadBuffer[0], sizeof(NvOdmBoardInfo));
    return NV_TRUE;
}

NvBool NvOdmPeripheralGetBoardInfo(
    NvU16 BoardId,
    NvOdmBoardInfo *pBoardInfo)
{
    NvBool RetVal = NV_FALSE;
    NvOdmServicesI2cHandle hOdmI2c = NULL;
    static NvBool s_BoardInfoFound = NV_FALSE;

    if (!s_BoardInfoFound)
    {
        hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c_Pmu, 0);
        if (!hOdmI2c)
        {
            return NV_FALSE;
        }

        RetVal = NvOdmPeripheralReadBoardInfo(hOdmI2c, &s_BoardInfo);
        if (RetVal == NV_TRUE)
        {
            s_BoardInfoFound = NV_TRUE;
        }

        NvOdmI2cClose(hOdmI2c);
    }

    if (s_BoardInfoFound)
    {
        pBoardInfo->BoardID  = s_BoardInfo.BoardID;
        pBoardInfo->SKU      = s_BoardInfo.SKU;
        pBoardInfo->Fab      = s_BoardInfo.Fab;
        pBoardInfo->Revision = s_BoardInfo.Revision;
        pBoardInfo->MinorRevision = s_BoardInfo.MinorRevision;
        return NV_TRUE;
    }

    return NV_FALSE;
}

static const NvOdmPeripheralConnectivity *NvApGetAllPeripherals(NvU32 *pNum)
{
    *pNum = NV_ARRAY_SIZE(s_Peripherals_Default);
    return s_Peripherals_Default;
}

// This implements a simple linear search across the entire set of currently-
// connected peripherals to find the set of GUIDs that Match the search
// criteria.  More clever implementations are possible, but given the
// relatively small search space (max dozens of peripherals) and the relative
// infrequency of enumerating peripherals, this is the easiest implementation.
const NvOdmPeripheralConnectivity *NvOdmPeripheralGetGuid(NvU64 SearchGuid)
{
    const NvOdmPeripheralConnectivity *pAllPeripherals;
    NvU32 NumPeripherals;
    NvU32 i;

    pAllPeripherals = NvApGetAllPeripherals(&NumPeripherals);

    if (!pAllPeripherals || !NumPeripherals)
        return NULL;

    for (i=0; i<NumPeripherals; i++)
    {
        if (SearchGuid == pAllPeripherals[i].Guid)
            return &pAllPeripherals[i];
    }

    return NULL;
}

static NvBool IsBusMatch(
    const NvOdmPeripheralConnectivity *pPeriph,
    const NvOdmPeripheralSearch *pSearchAttrs,
    const NvU32 *pSearchVals,
    NvU32 offset,
    NvU32 NumAttrs)
{
    NvU32 i, j;
    NvBool IsMatch = NV_FALSE;

    for (i=0; i<pPeriph->NumAddress; i++)
    {
        j = offset;
        do
        {
            switch (pSearchAttrs[j])
            {
            case NvOdmPeripheralSearch_IoModule:
                IsMatch = (pSearchVals[j] ==
                           (NvU32)(pPeriph->AddressList[i].Interface));
                break;
            case NvOdmPeripheralSearch_Address:
                IsMatch = (pSearchVals[j] == pPeriph->AddressList[i].Address);
                break;
            case NvOdmPeripheralSearch_Instance:
                IsMatch = (pSearchVals[j] == pPeriph->AddressList[i].Instance);
                break;
            case NvOdmPeripheralSearch_PeripheralClass:
            default:
                NV_ASSERT(!"Bad Query!");
                break;
            }
            j++;
        } while (IsMatch && j<NumAttrs &&
                 pSearchAttrs[j]!=NvOdmPeripheralSearch_IoModule);

        if (IsMatch)
        {
            return NV_TRUE;
        }
    }
    return NV_FALSE;
}

static NvBool IsPeripheralMatch(
    const NvOdmPeripheralConnectivity *pPeriph,
    const NvOdmPeripheralSearch *pSearchAttrs,
    const NvU32 *pSearchVals,
    NvU32 NumAttrs)
{
    NvU32 i;
    NvBool IsMatch = NV_TRUE;

    for (i=0; i<NumAttrs && IsMatch; i++)
    {
        switch (pSearchAttrs[i])
        {
        case NvOdmPeripheralSearch_PeripheralClass:
            IsMatch = (pSearchVals[i] == (NvU32)(pPeriph->Class));
            break;
        case NvOdmPeripheralSearch_IoModule:
            IsMatch = IsBusMatch(pPeriph, pSearchAttrs, pSearchVals, i, NumAttrs);
            break;
        case NvOdmPeripheralSearch_Address:
        case NvOdmPeripheralSearch_Instance:
            // In correctly-formed searches, these parameters will be parsed by
            // IsBusMatch, so we ignore them here.
            break;
        default:
            NV_ASSERT(!"Bad search attribute!");
            break;
        }
    }
    return IsMatch;
}

NvU32 NvOdmPeripheralEnumerate(
    const NvOdmPeripheralSearch *pSearchAttrs,
    const NvU32 *pSearchVals,
    NvU32 NumAttrs,
    NvU64 *pGuidList,
    NvU32 NumGuids)
{
    const NvOdmPeripheralConnectivity *pAllPeripherals;
    NvU32 NumPeripherals;
    NvU32 Matches;
    NvU32 i;

    pAllPeripherals = NvApGetAllPeripherals(&NumPeripherals);

    if (!pAllPeripherals || !NumPeripherals)
    {
        return 0;
    }

    if (!pSearchAttrs || !pSearchVals)
    {
        NumAttrs = 0;
    }
    for (i=0, Matches=0; i<NumPeripherals &&
             (Matches < NumGuids || !pGuidList); i++)
    {
        if ( !NumAttrs || IsPeripheralMatch(&pAllPeripherals[i],
                                            pSearchAttrs, pSearchVals,
                                            NumAttrs) )
        {
            if (pGuidList)
                pGuidList[Matches] = pAllPeripherals[i].Guid;
            Matches++;
        }
    }
    return Matches;
}

NvBool
NvOdmQueryGetBoardModuleInfo(
    NvOdmBoardModuleType mType,
    void *BoardModuleData,
    int DataLen)
{
    switch (mType)
    {
        case NvOdmBoardModuleType_ProcessorBoard:
            if (DataLen != sizeof(NvOdmBoardInfo))
                return NV_FALSE;
            return NvOdmPeripheralGetBoardInfo(0, BoardModuleData);

        default:
            break;
    }
    return NV_FALSE;
}
