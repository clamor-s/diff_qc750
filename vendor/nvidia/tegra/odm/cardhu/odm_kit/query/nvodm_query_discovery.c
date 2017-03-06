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
#include "subboards/nvodm_query_discovery_e1291_addresses.h"
#include "subboards/nvodm_query_discovery_e1294_addresses.h"
#include "subboards/nvodm_query_discovery_e1211_addresses.h"
#include "nvodm_services.h"

#define NVODM_QUERY_I2C_INSTANCE_ID      4
#define NVODM_QUERY_MAX_BUS_SEGMENTS     4      // # of Bus Segments defined by I2C extender
#define NVODM_QUERY_MAX_EEPROMS          8      // Maximum number of EEPROMs per bus segment
#define NVODM_QUERY_I2C_EXTENDER_ADDRESS 0x42   // I2C bus extender address (7'h21)
#define NVODM_QUERY_I2C_EEPROM_ADDRESS   0xA0   // I2C device base address for EEPROM (7'h50)
#define NVODM_QUERY_BOARD_HEADER_START   0x04   // Offset to Part Number in EERPOM
#define NVODM_QUERY_I2C_CLOCK_SPEED      100    // kHz
#define I2C_AUDIOCODEC_ID                4

// Supported board list by this odms
#define BOARD_E1187   0x0B57
#define BOARD_E1186   0x0B56
#define BOARD_E1198   0x0B62
#define BOARD_E1291   0x0C5B
#define BOARD_PM267   0x0243
#define BOARD_PM269   0x0245
#define BOARD_PM305   0x0305
#define BOARD_PM311   0x030B
#define BOARD_E1256   0x0C38
#define BOARD_E1257   0x0C39

#define BOARD_PMU_1208   0xC08
#define BOARD_PMU_PM298  0x262
#define BOARD_PMU_PM299  0x263

#define PMU_DCDC_TPS62361_SUPPORT 0x1
#define SKU_CPU_10AMP_SUPPORT     0x0400  // Bit 2 in MSB

#define BOARD_FAB_A00  0x0
#define BOARD_FAB_A01  0x1
#define BOARD_FAB_A02  0x2
#define BOARD_FAB_A03  0x3
#define BOARD_FAB_A04  0x4

// The following are used to store entries read from EEPROMs at runtime.
static NvOdmBoardInfo s_BoardModuleTable[NVODM_QUERY_MAX_EEPROMS];
static const NvOdmIoAddress s_SonyPanelAddresses[] =
{
    { NvOdmIoModule_Gpio, (NvU32)'m'-'a', 3, 0},   /* Reset */
    { NvOdmIoModule_Gpio, (NvU32)'b'-'a', 2, 0},   /* On/off */
    { NvOdmIoModule_Gpio, (NvU32)'n'-'a', 4, 0},   /* ModeSelect */
    { NvOdmIoModule_Gpio, (NvU32)'j'-'a', 3, 0},   /* hsync */
    { NvOdmIoModule_Gpio, (NvU32)'j'-'a', 4, 0},   /* vsync */
    { NvOdmIoModule_Gpio, (NvU32)'c'-'a', 1, 0},   /* PowerEnable */
    { NvOdmIoModule_Gpio, 0xFFFFFFFF, 0xFFFFFFFF, 0 }, /* slin */
    { NvOdmIoModule_Display, 0, 0, 0 },
};
static const NvOdmIoAddress s_SdioAddresses[] =
{
    { NvOdmIoModule_Sdio, 0x0, 0x0, 0 },
    { NvOdmIoModule_Sdio, 0x2, 0x0, 0 },
    { NvOdmIoModule_Sdio, 0x3, 0x0, 0 },
    // Need to power up this rail for supporting SD card
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_SDMMC1, 0 },
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
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDDIO_HDMI, 0 },
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_HDMI_PLL, 0 },
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_HDMI, 0 },
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_HDMI_CON, 0 },
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_DIS_5V, 0 },
};

static const NvOdmIoAddress s_CrtAddresses[] =
{
    { NvOdmIoModule_Crt, 0, 0, 0 },

    // Display Data Channel (DDC) for Extended Display Identification
    // Data (EDID)
    { NvOdmIoModule_I2c, 0x00, 0xA0, 0 },

    /* tvdac rail */
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_VDAC, 0 },
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_VDD_HDMI_CON, 0 },
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_DIS_5V, 0 },
};

static const NvOdmIoAddress s_ffaVideoDacAddresses[] =
{
    { NvOdmIoModule_Tvo, 0x00, 0x00, 0 },

    /* tvdac rail */
    { NvOdmIoModule_Vdd, 0x00, CardhuPowerRails_AVDD_VDAC, 0 },
};

static const NvOdmIoAddress s_CrtDdcMuxerAddresses[] =
{
    { NvOdmIoModule_I2c, 0x00, 0xE0, 0x1},
};

static NvOdmPeripheralConnectivity s_Peripherals[] =
{
#include "subboards/nvodm_query_discovery_e1291_peripherals.h"
#include "subboards/nvodm_query_discovery_e1294_peripherals.h"
#include "subboards/nvodm_query_discovery_e1211_peripherals.h"

    // LCD module
    {
        NV_ODM_GUID('f','f','a','_','l','c','d','0'),
        s_SonyPanelAddresses,
        NV_ARRAY_SIZE(s_SonyPanelAddresses),
        NvOdmPeripheralClass_Display,
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
    NvU16 MainProcessorBoard[] = {BOARD_E1187, BOARD_E1186, BOARD_E1198,
                                  BOARD_E1291, BOARD_PM269, BOARD_E1256,
                                  BOARD_PM305, BOARD_PM311, BOARD_E1257};
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

            if (RetVal == NV_TRUE) {
                NvOdmOsPrintf("The Bus seg %d:0x%02x BoardInfo: 0x%04x:0x%04x:0x%02x:0x%02x:0x%02x\n",
                          BusSegment, EepromInst, s_BoardModuleTable[NumBoards].BoardID, s_BoardModuleTable[NumBoards].SKU,
                          s_BoardModuleTable[NumBoards].Fab, s_BoardModuleTable[NumBoards].Revision, s_BoardModuleTable[NumBoards].MinorRevision);
                NumBoards++;
            }
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
    NvOdmOsMemcpy(pCodecName, "wm8903", 6);

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

    NvOdmPmuBoardInfo *pPmuInfo;
    NvOdmMaxCpuCurrentInfo *pCpuCurrInfo;

    /* DISPLAY table */
    static NvOdmDisplayBoardInfo displayBoardTable[] = {
        {
            NvOdmBoardDisplayType_LVDS,
            NV_TRUE,
            {
                0x0C2F,
            }
        },
        {
            NvOdmBoardDisplayType_LVDS,
            NV_FALSE,
            {
                0x0C5B,
            }
        },
        {
            NvOdmBoardDisplayType_DSI,
            NV_FALSE,
            {
                0x0C0D,
            }
        },
        {
            NvOdmBoardDisplayType_LVDS,
            NV_TRUE,
            {
                0x030D,
            }
        },
        {
            NvOdmBoardDisplayType_DSI,
            NV_TRUE,
            {
                0x0F06,
            }
        }
    };
    NvOdmDisplayBoardInfo *pDisplayInfo;
    NvOdmAudioCodecBoardInfo *pAudioCodecBoard = NULL;

    unsigned int i;
    NvBool status;
    NvU32 PackageInfo;
    NvU32 SKU;

    switch (mType)
    {
        case NvOdmBoardModuleType_ProcessorBoard:
            if (DataLen != sizeof(NvOdmBoardInfo))
                return NV_FALSE;

            // Here we are supporting the processor board and pmu combination
            // such that if dc-dc converter is available then return the processor
            // board sku such that bit 0. For E1291 and E1198, PMU is
            // on board and so board -id is programmed part of processor board id.
            // In PM269/E1186/E1187, pmu board is external to processor board and
            // so sku of pmu is updated based on the dc-dc converter. In this case
            // we update the sku (bit 0) of processor board to return the pmu
            // sku bit 0 information.

            status = NvOdmPeripheralGetBoardInfo(0, BoardModuleData);
            if (status)
            {
                procBoardInfo = (NvOdmBoardInfo *)BoardModuleData;
                NvOdmOsPrintf("The proc BoardInfo: 0x%04x:0x%04x:0x%02x:0x%02x:0x%02x\n",
                             procBoardInfo->BoardID, procBoardInfo->SKU,
                             procBoardInfo->Fab, procBoardInfo->Revision, procBoardInfo->MinorRevision);
                // If pmu is on processor board, return the board info as it is.
                if ((procBoardInfo->BoardID == BOARD_E1291) ||
                         (procBoardInfo->BoardID == BOARD_E1198))
                    return status;

                // PMU module is not in processor board then look for the
                // pmu board sku id and if bit 0 is set then set the processor
                // board sku bit 0.
                status = NvOdmPeripheralGetBoardInfo(BOARD_PMU_1208, &BoardInfo);
                if (!status)
                     status = NvOdmPeripheralGetBoardInfo(BOARD_PMU_PM298, &BoardInfo);
                if (!status)
                     status = NvOdmPeripheralGetBoardInfo(BOARD_PMU_PM299, &BoardInfo);
                if (status)
                {
                      NvOdmOsDebugPrintf("The pmu Board Id 0x%04x and sku 0x%04x\n",
                             BoardInfo.BoardID, BoardInfo.SKU);
                      if ((BoardInfo.SKU & 1) == 1)
                           procBoardInfo->SKU |= 0x1;
                }
                status = NV_TRUE;
            }
            return status;

        case NvOdmBoardModuleType_PmuBoard:
            if (DataLen != sizeof(NvOdmPmuBoardInfo))
                return NV_FALSE;

            pPmuInfo = BoardModuleData;

            /* Default is 0 */
            pPmuInfo->core_edp_mv = 0;
            pPmuInfo->isPassBoardInfoToKernel = NV_FALSE;

            // Get the board Id
            status = NvOdmPeripheralGetBoardInfo(0, &BoardInfo);

            // E1291- SKU:Fab - 1000:3 and 1001:x
            if ((BoardInfo.BoardID == BOARD_E1291) &&
                 ((BoardInfo.SKU & PMU_DCDC_TPS62361_SUPPORT)  ||
                           ((BoardInfo.Fab >= BOARD_FAB_A03))))
            {
                 pPmuInfo->core_edp_mv = 1300;
                 return NV_TRUE;
            }

            // E1198- All SKUs
            if (BoardInfo.BoardID == BOARD_E1198)
            {
                int tps62361_support = 0;
                int vsels;
                switch(BoardInfo.Fab) {
                    case BOARD_FAB_A00:
                    case BOARD_FAB_A01:
                    case BOARD_FAB_A02:
                        if (BoardInfo.SKU & PMU_DCDC_TPS62361_SUPPORT)
                            tps62361_support = 1;
                        break;
                    case BOARD_FAB_A03:

                        vsels = ((BoardInfo.SKU >> 1) & 2) | (BoardInfo.SKU & 1);
                        switch(vsels) {
                        case 1:
                        case 2:
                        case 3:
                            tps62361_support = 1;
                            break;
                        }
                }

                if (tps62361_support == 1)
                {
                    status = NvOdmServicesFuseGet(NvOdmServiceFuseType_SKU, &SKU);
                    if ((status == NV_TRUE) && (SKU == 0xA0))
                    {
                        status = NvOdmServicesFuseGet(NvOdmServiceFuseType_PackageInfo,
                                                   &PackageInfo);
                        if ((status == NV_TRUE) && (PackageInfo == 0x1))
                        {
                            pPmuInfo->core_edp_mv = 1350;
                            return NV_TRUE;
                        }
                    }
                }
                pPmuInfo->core_edp_mv = 1300;
                return NV_TRUE;
            }

            // For PMU 1208: 1000:3 and 1001:x
            // For PMU PM299: 1001:x and 1000:x
            // For PMU PM298: x
            status = NvOdmPeripheralGetBoardInfo(BOARD_PMU_1208, &BoardInfo);
            if (!status)
                  status = NvOdmPeripheralGetBoardInfo(BOARD_PMU_PM298, &BoardInfo);
            if (!status)
                  status = NvOdmPeripheralGetBoardInfo(BOARD_PMU_PM299, &BoardInfo);
            if (status)
            {
                NvOdmOsPrintf("The PMU BoardInfo: 0x%04x:0x%04x:0x%02x:0x%02x:0x%02x\n",
                          BoardInfo.BoardID, BoardInfo.SKU,
                          BoardInfo.Fab, BoardInfo.Revision, BoardInfo.MinorRevision);

                if (((BoardInfo.BoardID == BOARD_PMU_1208) &&
                                (BoardInfo.Fab == BOARD_FAB_A03)) ||
                                    (BoardInfo.SKU & PMU_DCDC_TPS62361_SUPPORT))
                    pPmuInfo->core_edp_mv = 1300;

                if ((BoardInfo.BoardID == BOARD_PMU_1208) &&
                                (BoardInfo.Fab == BOARD_FAB_A04)) {

                    status = NvOdmServicesFuseGet(NvOdmServiceFuseType_SKU, &SKU);
                    if ((status == NV_TRUE) && (SKU == 0xA0))
                    {
                        status = NvOdmServicesFuseGet(NvOdmServiceFuseType_PackageInfo,
                                                &PackageInfo);
                        if ((status == NV_TRUE) && (PackageInfo == 0x1))
                            pPmuInfo->core_edp_mv = 1350;
                    }
		}

                if (BoardInfo.BoardID == BOARD_PMU_PM299)
                    pPmuInfo->core_edp_mv = 1300;

                if (BoardInfo.BoardID == BOARD_PMU_PM298)
                    pPmuInfo->core_edp_mv = 1300;

                pPmuInfo->isPassBoardInfoToKernel = NV_TRUE;
                pPmuInfo->BoardInfo.BoardID = BoardInfo.BoardID;
                pPmuInfo->BoardInfo.SKU = BoardInfo.SKU;
                pPmuInfo->BoardInfo.Fab = BoardInfo.Fab;
                pPmuInfo->BoardInfo.Revision = BoardInfo.Revision;
                pPmuInfo->BoardInfo.MinorRevision = BoardInfo.MinorRevision;
            }
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

            // Check the 'e' bit (MSB bit 2) for 10Amp support on different boards
            if (((BoardInfo.BoardID == BOARD_PM269) && (BoardInfo.Fab == BOARD_FAB_A03)) ||
                ((BoardInfo.BoardID == BOARD_E1198) && (BoardInfo.Fab >= BOARD_FAB_A02)) ||
                 (BoardInfo.BoardID == BOARD_E1291) ||
                 (BoardInfo.BoardID == BOARD_E1256))
            {
                 if (BoardInfo.SKU & SKU_CPU_10AMP_SUPPORT)
                 {
                      pCpuCurrInfo->MaxCpuCurrentmA = 10000;
                      return NV_TRUE;
                 }
            }
            return NV_FALSE;

        default:
            break;
    }
    return NV_FALSE;
}
