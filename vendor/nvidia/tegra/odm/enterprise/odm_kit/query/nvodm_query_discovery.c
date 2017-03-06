/*
 * Copyright (c) 2008-2012, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * <b>NVIDIA APX ODM Kit::
 *         Implementation of the ODM Peripheral Discovery API</b>
 *
 * @b Description: The peripheral connectivity database implementation.
 */

#include "nvcommon.h"
#include "nvodm_query_gpio.h"
#include "nvodm_modules.h"
#include "nvodm_query_discovery.h"
#include "include/nvodm_imager_guids.h"
#include "include/nvodm_query_discovery_imager.h"
#include "nvodm_query_kbc_gpio_def.h"
#include "subboards/nvodm_query_discovery_e1205_addresses.h"
#include "nvrm_drf.h"
#include "tegra_devkit_custopt.h"
#include "nvodm_keylist_reserved.h"
#include "nvbct.h"


#define NVODM_QUERY_I2C_INSTANCE_ID      4
#define NVODM_QUERY_MAX_BUS_SEGMENTS     4      // # of Bus Segments defined by I2C extender
#define NVODM_QUERY_MAX_EEPROMS          8      // Maximum number of EEPROMs per bus segment
#define NVODM_QUERY_I2C_EXTENDER_ADDRESS 0x42   // I2C bus extender address (7'h21)
#define NVODM_QUERY_I2C_EEPROM_ADDRESS   0xA0   // I2C device base address for EEPROM (7'h50)
#define NVODM_QUERY_BOARD_HEADER_START   0x04   // Offset to Part Number in EERPOM
#define NVODM_QUERY_I2C_CLOCK_SPEED      100    // kHz


#define BOARD_E1197   0x0B61
#define BOARD_E1205   0x0C05
#define BOARD_E1239   0x0C27
#define BOARD_E1513   0x0F0D
#define BOARD_FAB_A03 0x03

#define SKU_CPU_6AMP_SUPPORT     0x0400  // Bit 2 in MSB

// The following are used to store entries read from EEPROMs at runtime.
static NvOdmBoardInfo s_BoardModuleTable[NVODM_QUERY_MAX_EEPROMS];
static const NvOdmIoAddress s_SdioAddresses[] =
{
    { NvOdmIoModule_Sdio, 0x0, 0x0, 0 },
    { NvOdmIoModule_Sdio, 0x2, 0x0, 0 },
    { NvOdmIoModule_Sdio, 0x3, 0x0, 0 },
};

static const NvOdmIoAddress s_HdmiAddresses[] =
{
    { NvOdmIoModule_Hdmi, 0, 0, 0 },

    // Display Data Channel (DDC) for Extended Display Identification
    // Data (EDID)
    { NvOdmIoModule_I2c, 0x03, 0xA0, 0 },

    // HDCP ROM
    { NvOdmIoModule_I2c, 0x03, 0x74, 0 },
};

static const NvOdmIoAddress s_CrtAddresses[] =
{
    { NvOdmIoModule_Crt, 0, 0, 0 },

    // Display Data Channel (DDC) for Extended Display Identification
    // Data (EDID)
    { NvOdmIoModule_I2c, 0x00, 0xA0, 0 },
};

static const NvOdmIoAddress s_ffaVideoDacAddresses[] =
{
    { NvOdmIoModule_Tvo, 0x00, 0x00, 0 },
};

static const NvOdmIoAddress s_CrtDdcMuxerAddresses[] =
{
    { NvOdmIoModule_I2c, 0x00, 0xE0, 0x1},
};

static NvOdmPeripheralConnectivity s_Peripherals[] =
{
#include "subboards/nvodm_query_discovery_e1205_peripherals.h"

    //  Sdio module
    {
        NV_ODM_GUID('s','d','i','o','_','m','e','m'),
        s_SdioAddresses,
        NV_ARRAY_SIZE(s_SdioAddresses),
        NvOdmPeripheralClass_Other,
    },

    // HDMI module
    {
        NV_ODM_GUID('f','f','a','2','h','d','m','i'),
        s_HdmiAddresses,
        NV_ARRAY_SIZE(s_HdmiAddresses),
        NvOdmPeripheralClass_Display
    },

    // CRT module
    {
        NV_ODM_GUID('f','f','a','_','_','c','r','t'),
        s_CrtAddresses,
        NV_ARRAY_SIZE(s_CrtAddresses),
        NvOdmPeripheralClass_Display
    },

    // TV Out Video Dac
    {
        NV_ODM_GUID('f','f','a','t','v','o','u','t'),
        s_ffaVideoDacAddresses,
        NV_ARRAY_SIZE(s_ffaVideoDacAddresses),
        NvOdmPeripheralClass_Display
    },

    // CRT DDC muxer
    {
        NV_ODM_GUID('c','r','t','d','d','c','m','u'),
        s_CrtDdcMuxerAddresses,
        NV_ARRAY_SIZE(s_CrtDdcMuxerAddresses),
        NvOdmPeripheralClass_Display
    },

};

/*****************************************************************************/

static const NvOdmPeripheralConnectivity*
NvApGetAllPeripherals (NvU32 *pNum)
{
    if (!pNum) {
        return NULL;
    }

    *pNum = NV_ARRAY_SIZE(s_Peripherals);
    return (const NvOdmPeripheralConnectivity *)s_Peripherals;
}

// This implements a simple linear search across the entire set of currently-
// connected peripherals to find the set of GUIDs that Match the search
// criteria.  More clever implementations are possible, but given the
// relatively small search space (max dozens of peripherals) and the relative
// infrequency of enumerating peripherals, this is the easiest implementation.
const NvOdmPeripheralConnectivity *
NvOdmPeripheralGetGuid(NvU64 SearchGuid)
{
    const NvOdmPeripheralConnectivity *pAllPeripherals;
    NvU32 NumPeripherals;
    NvU32 i;

    pAllPeripherals = NvApGetAllPeripherals(&NumPeripherals);

    if (!pAllPeripherals || !NumPeripherals) {
        return NULL;
    }

    for (i=0; i<NumPeripherals; i++) {
        if (SearchGuid == pAllPeripherals[i].Guid) {
            return &pAllPeripherals[i];
        }
    }

    return NULL;
}

static NvBool
IsBusMatch(
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
                IsMatch = (pSearchVals[j] == (NvU32)(pPeriph->AddressList[i].Interface));
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
        } while (IsMatch && j<NumAttrs && pSearchAttrs[j]!=NvOdmPeripheralSearch_IoModule);

        if (IsMatch)
        {
            return NV_TRUE;
        }
    }
    return NV_FALSE;
}

static NvBool
IsPeripheralMatch(
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

NvU32
NvOdmPeripheralEnumerate(
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

    for (i=0, Matches=0; i<NumPeripherals && (Matches < NumGuids || !pGuidList); i++)
    {
        if ( !NumAttrs || IsPeripheralMatch(&pAllPeripherals[i], pSearchAttrs, pSearchVals, NumAttrs) )
        {
            if (pGuidList)
                pGuidList[Matches] = pAllPeripherals[i].Guid;

            Matches++;
        }
    }

    return Matches;
}


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
        return Error;

    *pData = ReadBuffer[0];
    return Error;
}


static NvBool
NvOdmPeripheralReadPartNumber(
    NvOdmServicesI2cHandle hOdmI2c,
    NvU8 EepromInst,
    NvOdmBoardInfo *pBoardInfo)
{
    NvOdmI2cStatus Error;
    NvU32 i;
    NvU8 I2cAddr, Offset;
    NvU8 ReadBuffer[sizeof(NvOdmBoardInfo)];

    NvOdmOsMemset(ReadBuffer, 0, sizeof(ReadBuffer));

    // EepromInst*2, since 7-bit addressing
    I2cAddr = NVODM_QUERY_I2C_EEPROM_ADDRESS + (EepromInst << 1);

    /**
     * Offset to the board number entry in EEPROM.
     */
    Offset = NVODM_QUERY_BOARD_HEADER_START;

    for (i=0; i<sizeof(NvOdmBoardInfo); i++)
    {
        Error = NvOdmPeripheralI2cRead8(
            hOdmI2c, I2cAddr, Offset+i, (NvU8 *)&ReadBuffer[i]);
        if (Error != NvOdmI2cStatus_Success)
            return NV_FALSE;
    }
    NvOdmOsMemcpy(pBoardInfo, &ReadBuffer[0], sizeof(NvOdmBoardInfo));
    return NV_TRUE;
}

static NvBool
NvOdmPeripheralSetBusSegment(
    NvOdmServicesI2cHandle hOdmI2c,
    NvU16 BusSegment)
{
    NvU8 WriteBuffer[2];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;
    NvU8 Data;

    Data = (NvU8)(BusSegment & 0xF);

    WriteBuffer[0] = 0x03;  // Register (0x03)
    WriteBuffer[1] = 0x00;  // Data

    TransactionInfo.Address = NVODM_QUERY_I2C_EXTENDER_ADDRESS;
    TransactionInfo.Buf = WriteBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 2;

    Error = NvOdmI2cTransaction(
    hOdmI2c, &TransactionInfo, 1, NVODM_QUERY_I2C_CLOCK_SPEED, 100);
    if (Error != NvOdmI2cStatus_Success)
       return NV_FALSE;

    WriteBuffer[0] = 0x01;  // Register (0x01)
    WriteBuffer[1] = Data;  // Data (bus segment 0 thru 3)

    Error = NvOdmI2cTransaction(
       hOdmI2c, &TransactionInfo, 1, NVODM_QUERY_I2C_CLOCK_SPEED, 100);

    if (Error != NvOdmI2cStatus_Success)
      return NV_FALSE;

    return NV_TRUE;
}

NvBool
NvOdmPeripheralGetBoardInfo(
    NvU16 BoardId,
    NvOdmBoardInfo *pBoardInfo)
{
    NvBool RetVal = NV_FALSE;
    NvBool RetExpVal = NV_FALSE;
    NvOdmServicesI2cHandle hOdmI2c = NULL;
    NvU8 EepromInst, CurrentBoard;
    static NvU8 NumBoards = 0;
    static NvBool s_ReadBoardInfoDone = NV_FALSE;
    NvU16 MainProcessorBoard[] = {BOARD_E1205, BOARD_E1197, BOARD_E1239};
    NvU32 procBCount;
    NvU32 BusSegment;

    hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, NVODM_QUERY_I2C_INSTANCE_ID);
    if (!hOdmI2c)
    {
        // Exit
        pBoardInfo = NULL;
        return NV_FALSE;
    }

    if (!s_ReadBoardInfoDone)
    {
        s_ReadBoardInfoDone = NV_TRUE;

        for (BusSegment=0; BusSegment < NVODM_QUERY_MAX_BUS_SEGMENTS; BusSegment++)
        {
           RetExpVal = NvOdmPeripheralSetBusSegment(hOdmI2c, BusSegment);

           for (EepromInst=0; EepromInst < NVODM_QUERY_MAX_EEPROMS; EepromInst++)
           {
             RetVal = NvOdmPeripheralReadPartNumber(
                   hOdmI2c, EepromInst, &s_BoardModuleTable[NumBoards]);

            if (RetVal == NV_TRUE)
              NumBoards++;
           }

           if (!RetExpVal)
              break;
        }
    }
    //Bus segment needs to be 0 by default
    NvOdmPeripheralSetBusSegment(hOdmI2c, 0);
    NvOdmI2cClose(hOdmI2c);

    if (NumBoards)
    {
       if (BoardId)
       {
          // Linear search for given BoardId; if found, return entry
          for (CurrentBoard=0; CurrentBoard < NumBoards; CurrentBoard++)
          {
             if (s_BoardModuleTable[CurrentBoard].BoardID == BoardId)
             {
                // Match found
                pBoardInfo->BoardID  = s_BoardModuleTable[CurrentBoard].BoardID;
                pBoardInfo->SKU      = s_BoardModuleTable[CurrentBoard].SKU;
                pBoardInfo->Fab      = s_BoardModuleTable[CurrentBoard].Fab;
                pBoardInfo->Revision = s_BoardModuleTable[CurrentBoard].Revision;
                pBoardInfo->MinorRevision = s_BoardModuleTable[CurrentBoard].MinorRevision;
                return NV_TRUE;
               }
            }
         }
         else
         {
            for (procBCount = 0; procBCount < NV_ARRAY_SIZE(MainProcessorBoard); ++procBCount)
            {
                for (CurrentBoard=0; CurrentBoard < NumBoards; CurrentBoard++)
                {
                   if (s_BoardModuleTable[CurrentBoard].BoardID == MainProcessorBoard[procBCount])
                   {
                      // Match found
                      pBoardInfo->BoardID  = s_BoardModuleTable[CurrentBoard].BoardID;
                      pBoardInfo->SKU      = s_BoardModuleTable[CurrentBoard].SKU;
                      pBoardInfo->Fab      = s_BoardModuleTable[CurrentBoard].Fab;
                      pBoardInfo->Revision = s_BoardModuleTable[CurrentBoard].Revision;
                      pBoardInfo->MinorRevision = s_BoardModuleTable[CurrentBoard].MinorRevision;
                      return NV_TRUE;
                   }
               }
            }
         }
    }
    return NV_FALSE;
}

NvBool
NvOdmQueryGetBoardModuleInfo(
    NvOdmBoardModuleType mType,
    void *BoardModuleData,
    int DataLen)
{
    NvU32 CustOpt;
    NvOdmPowerSupplyInfo *pSupplyInfo;
    NvOdmBoardInfo BoardInfo;
    NvOdmMaxCpuCurrentInfo *pCpuCurrInfo;
    NvOdmCameraBoardInfo *pCameraInfo;

    NvBool status;
    switch (mType)
    {
        case NvOdmBoardModuleType_ProcessorBoard:
            if (DataLen != sizeof(NvOdmBoardInfo))
                return NV_FALSE;
            return NvOdmPeripheralGetBoardInfo(0, BoardModuleData);

       case NvOdmBoardModuleType_PowerSupply:
            pSupplyInfo = (NvOdmPowerSupplyInfo *)BoardModuleData;

            // Get the board Id
            NvOdmOsMemset(&BoardInfo, 0, sizeof(BoardInfo));
            status = NvOdmPeripheralGetBoardInfo(0, &BoardInfo);
            if (!status)
                return NV_FALSE;

            if (BoardInfo.BoardID == BOARD_E1197) {
                pSupplyInfo->SupplyType = NvOdmBoardPowerSupply_Adapter;
                return NV_TRUE;
            }

            CustOpt = GetBctKeyValue();
            switch (NV_DRF_VAL(TEGRA_DEVKIT, BCT_CUSTOPT, POWERSUPPLY, CustOpt))
            {
                 case TEGRA_DEVKIT_BCT_CUSTOPT_0_POWERSUPPLY_BATTERY:
                      pSupplyInfo->SupplyType = NvOdmBoardPowerSupply_Battery;
                      return NV_TRUE;

                 default:
                      pSupplyInfo->SupplyType = NvOdmBoardPowerSupply_Adapter;
                      return NV_TRUE;
            }
            break;

       case NvOdmBoardModuleType_CpuCurrent:
            pCpuCurrInfo = BoardModuleData;
            /* Default is 0 */
            pCpuCurrInfo->MaxCpuCurrentmA = 0;

            NvOdmOsMemset(&BoardInfo, 0, sizeof(BoardInfo));
            // Get the board Id
            status = NvOdmPeripheralGetBoardInfo(0, &BoardInfo);
            if (!status)
                return status;

            // E1205 with MSB bit 2 clear-> Support 6Amp
            if (BoardInfo.BoardID == BOARD_E1205)
            {
                 if (!(BoardInfo.SKU & SKU_CPU_6AMP_SUPPORT)
                             || (BoardInfo.Fab >= BOARD_FAB_A03))
                 {
                      pCpuCurrInfo->MaxCpuCurrentmA = 6000;
                      return NV_TRUE;
                 }
            }
            else if (BoardInfo.BoardID == BOARD_E1239)
            {
                pCpuCurrInfo->MaxCpuCurrentmA = 6000;
                return NV_TRUE;
            }
            // E1197 with MSB bit 2 set -> Support 6Amp
            else if (BoardInfo.BoardID == BOARD_E1197)
            {
                 if (BoardInfo.SKU & SKU_CPU_6AMP_SUPPORT)
                 {
                      pCpuCurrInfo->MaxCpuCurrentmA = 6000;
                      return NV_TRUE;
                 }
            }
            return NV_FALSE;

        case NvOdmBoardModuleType_CameraBoard:
            if (DataLen != sizeof(NvOdmCameraBoardInfo))
                return NV_FALSE;
            pCameraInfo = BoardModuleData;
            pCameraInfo->IsPassBoardInfoToKernel = NV_FALSE;

            status = NvOdmPeripheralGetBoardInfo(BOARD_E1513, &BoardInfo);
            if (status)
            {
                pCameraInfo->IsPassBoardInfoToKernel = NV_TRUE;;
                pCameraInfo->BoardInfo.BoardID = BoardInfo.BoardID;
                pCameraInfo->BoardInfo.SKU = BoardInfo.SKU;
                pCameraInfo->BoardInfo.Fab = BoardInfo.Fab;
                pCameraInfo->BoardInfo.Revision = BoardInfo.Revision;
                pCameraInfo->BoardInfo.MinorRevision = BoardInfo.MinorRevision;
            }
            return NV_TRUE;

        default:
            break;
    }
    return NV_FALSE;
}
