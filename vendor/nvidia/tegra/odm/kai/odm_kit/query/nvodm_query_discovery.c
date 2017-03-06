/*
 * Copyright (c) 2012 NVIDIA CORPORATION.  All rights reserved.
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
#include "subboards/nvodm_query_discovery_e1565_addresses.h"
#include "nvrm_drf.h"
#include "tegra_devkit_custopt.h"
#include "nvodm_keylist_reserved.h"
#include "nvbct.h"

#define NVODM_QUERY_I2C_INSTANCE_ID      4
#define NVODM_QUERY_MAX_EEPROMS          8      // Maximum number of EEPROMs per bus segment
#define NVODM_QUERY_I2C_EEPROM_ADDRESS   0xAC   // I2C device base address for EEPROM (7'h56)
#define NVODM_QUERY_BOARD_HEADER_START   0x04   // Offset to Part Number in EERPOM
#define NVODM_QUERY_I2C_CLOCK_SPEED      100    // kHz
#define I2C_AUDIOCODEC_ID                4

// Supported board list by this odms
#define BOARD_E1565            0x0F41

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

    /* AVDD_HDMI */
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_AVDD_HDMI_PLL, 0 },
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_AVDD_HDMI, 0 },
    { NvOdmIoModule_Vdd, 0x00, KaiPowerRails_VDD_HDMI_CON, 0 },
};

static NvOdmPeripheralConnectivity s_Peripherals[] =
{
#include "subboards/nvodm_query_discovery_e1565_peripherals.h"
    //  LVDS LCD Display
    {
        NV_ODM_GUID('L','V','D','S','W','S','V','G'),   // LVDS WSVGA panel
        s_LvdsDisplayAddresses,
        NV_ARRAY_SIZE(s_LvdsDisplayAddresses),
        NvOdmPeripheralClass_Display
    },

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

NvBool
NvOdmPeripheralGetBoardInfo(
    NvU16 BoardId,
    NvOdmBoardInfo *pBoardInfo)
{
    NvBool RetVal = NV_FALSE;
    NvOdmServicesI2cHandle hOdmI2c = NULL;
    NvU8 EepromInst, CurrentBoard;
    static NvU8 NumBoards = 0;
    static NvBool s_ReadBoardInfoDone = NV_FALSE;

    if (!s_ReadBoardInfoDone)
    {
        s_ReadBoardInfoDone = NV_TRUE;
        hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, NVODM_QUERY_I2C_INSTANCE_ID);
        if (!hOdmI2c)
        {
            // Exit
            pBoardInfo = NULL;
            return NV_FALSE;
        }

        for (EepromInst=0; EepromInst < NVODM_QUERY_MAX_EEPROMS; EepromInst++)
        {
            RetVal = NvOdmPeripheralReadPartNumber(
                  hOdmI2c, EepromInst, &s_BoardModuleTable[NumBoards]);

            if (RetVal == NV_TRUE) {
                NvOdmOsPrintf("0x%02x BoardInfo: 0x%04x:0x%04x:0x%02x:0x%02x:0x%02x\n",
                          EepromInst, s_BoardModuleTable[NumBoards].BoardID, s_BoardModuleTable[NumBoards].SKU,
                          s_BoardModuleTable[NumBoards].Fab, s_BoardModuleTable[NumBoards].Revision, s_BoardModuleTable[NumBoards].MinorRevision);
                NumBoards++;
            }
        }

        NvOdmI2cClose(hOdmI2c);
    }

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
            for (CurrentBoard=0; CurrentBoard < NumBoards; CurrentBoard++)
            {
               if (s_BoardModuleTable[CurrentBoard].BoardID == BOARD_E1565)
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
    return NV_FALSE;
}

static NvBool GetAudioCodecName(char *pCodecName, int MaxNameLen)
{
    NvOdmServicesI2cHandle hOdmI2c = NULL;

    hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, I2C_AUDIOCODEC_ID);
    if (!hOdmI2c) {
         NvOdmOsPrintf("%s(): The I2c service can not be open\n");
         return NV_FALSE;
    }

    NvOdmOsMemset(pCodecName, 0, MaxNameLen);

    // Read codec and fill the codec name
    NvOdmOsMemcpy(pCodecName, "rt5640", 6);

    NvOdmI2cClose(hOdmI2c);
    return NV_TRUE;
}

NvBool
NvOdmQueryGetBoardModuleInfo(
    NvOdmBoardModuleType mType,
    void *BoardModuleData,
    int DataLen)
{
    NvOdmBoardInfo *procBoardInfo;
    NvOdmBoardInfo BoardInfo;
    NvOdmPowerSupplyInfo *pSupplyInfo;
    NvU32 CustOpt;

    NvOdmPmuBoardInfo *pPmuInfo;
    NvOdmMaxCpuCurrentInfo *pCpuCurrInfo;

    /* DISPLAY table */
    static NvOdmDisplayBoardInfo displayBoardTable[] = {
        {
            NvOdmBoardDisplayType_LVDS,
            NV_FALSE,
            {
                BOARD_E1565,
            }
        },
    };
    NvOdmDisplayBoardInfo *pDisplayInfo;
    NvOdmAudioCodecBoardInfo *pAudioCodecBoard = NULL;

    unsigned int i;
    NvBool status;

    switch (mType)
    {
        case NvOdmBoardModuleType_ProcessorBoard:
            if (DataLen != sizeof(NvOdmBoardInfo))
                return NV_FALSE;

            status = NvOdmPeripheralGetBoardInfo(0, BoardModuleData);
            if (status)
            {
                procBoardInfo = (NvOdmBoardInfo *)BoardModuleData;
                NvOdmOsPrintf("The proc BoardInfo: 0x%04x:0x%04x:0x%02x:0x%02x:0x%02x\n",
                             procBoardInfo->BoardID, procBoardInfo->SKU,
                             procBoardInfo->Fab, procBoardInfo->Revision, procBoardInfo->MinorRevision);
            }
            return status;

        case NvOdmBoardModuleType_PowerSupply:
             pSupplyInfo = (NvOdmPowerSupplyInfo *)BoardModuleData;

             // Get the board Id
             NvOdmOsMemset(&BoardInfo, 0, sizeof(BoardInfo));
             status = NvOdmPeripheralGetBoardInfo(0, &BoardInfo);
             if (!status)
                 return NV_FALSE;

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

        case NvOdmBoardModuleType_PmuBoard:
            if (DataLen != sizeof(NvOdmPmuBoardInfo))
                return NV_FALSE;

            pPmuInfo = BoardModuleData;

            /* Default is 0 */
            pPmuInfo->core_edp_mv = 0;
            pPmuInfo->isPassBoardInfoToKernel = NV_FALSE;

            // Get the board Id
            status = NvOdmPeripheralGetBoardInfo(0, &BoardInfo);

            return NV_TRUE;

        case NvOdmBoardModuleType_DisplayBoard:
            if (DataLen != sizeof(NvOdmDisplayBoardInfo))
                return NV_FALSE;
            pDisplayInfo = BoardModuleData;
            for (i = 0; i < NV_ARRAY_SIZE(displayBoardTable); ++i)
            {
                status = NvOdmPeripheralGetBoardInfo(displayBoardTable[i].BoardInfo.BoardID, &BoardInfo);
                if (status)
                {
                    pDisplayInfo->DisplayType = displayBoardTable[i].DisplayType;
                    pDisplayInfo->IsPassBoardInfoToKernel = displayBoardTable[i].IsPassBoardInfoToKernel;

                    pDisplayInfo->BoardInfo.BoardID = BoardInfo.BoardID;
                    pDisplayInfo->BoardInfo.SKU = BoardInfo.SKU;
                    pDisplayInfo->BoardInfo.Fab = BoardInfo.Fab;
                    pDisplayInfo->BoardInfo.Revision = BoardInfo.Revision;
                    pDisplayInfo->BoardInfo.MinorRevision = BoardInfo.MinorRevision;

                    return NV_TRUE;
                }
            }
            /* Default is LVDS */
            pDisplayInfo->DisplayType = NvOdmBoardDisplayType_LVDS;
            return NV_TRUE;

       case NvOdmBoardModuleType_AudioCodec:
           pAudioCodecBoard = (NvOdmAudioCodecBoardInfo *)BoardModuleData;
           return GetAudioCodecName(pAudioCodecBoard->AudioCodecName,
                       sizeof(pAudioCodecBoard->AudioCodecName));

       case NvOdmBoardModuleType_CpuCurrent:
            pCpuCurrInfo = BoardModuleData;
            /* Default is 0 */
            pCpuCurrInfo->MaxCpuCurrentmA = 0;

            // Get the board Id
            status = NvOdmPeripheralGetBoardInfo(0, &BoardInfo);

            return NV_FALSE;

        default:
            break;
    }
    return NV_FALSE;
}
