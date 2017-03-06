/*
 * Copyright (c) 2011-2012, NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_query.h"
#include "nvrm_module.h"
#include "nvodm_services.h"
#include "nvodm_query_discovery.h"
#include "pmu/boards/cardhu/nvodm_pmu_cardhu_supply_info_table.h"

#define CPLD_I2C_TRANS_TIMEOUT 5000
#define CPLD_I2C_TRANS_SPEED 100

#define NV_BOARD_E1187_ID   0x0B57
#define NV_BOARD_E1186_ID   0x0B56
#define NV_BOARD_E1198_ID   0x0B62
#define NV_BOARD_E1256_ID   0x0C38
#define NV_BOARD_E1257_ID   0x0C39
#define NV_BOARD_E1291_ID   0x0C5B
#define NV_BOARD_PM267_ID   0x0243
#define NV_BOARD_PM269_ID   0x0245
#define NV_BOARD_PM272_ID   0x0248
#define NV_BOARD_PM305_ID   0x0305
#define NV_BOARD_PM311_ID   0x030B

#define NV_DISPLAY_BOARD_E1506        0x0F06
/* SKU Information */
#define SKU_DCDC_TPS62361_SUPPORT     0x1
#define SKU_SLT_ULPI_SUPPORT          0x2
#define SKU_T30S_SUPPORT              0x4

static NvBool CpldRead8(
    NvOdmServicesI2cHandle hOdmI2c,
    NvU32 DevAddr,
    NvU32 SpeedKHz,
    NvU8 RegAddr,
    NvU8 *Data)
{
    NvU8 ReadBuffer=0;
    NvOdmI2cStatus status = NvOdmI2cStatus_Success;
    NvOdmI2cTransactionInfo TransactionInfo[2];

    // Write the PMU offset
    ReadBuffer = RegAddr & 0xFF;

    TransactionInfo[0].Address = DevAddr;
    TransactionInfo[0].Buf = &ReadBuffer;
    TransactionInfo[0].Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo[0].NumBytes = 1;
    TransactionInfo[1].Address = DevAddr | 1;
    TransactionInfo[1].Buf = &ReadBuffer;
    TransactionInfo[1].Flags = 0;
    TransactionInfo[1].NumBytes = 1;

    // Read data from PMU at the specified offset
    status = NvOdmI2cTransaction(hOdmI2c, &TransactionInfo[0], 2,
                SpeedKHz, CPLD_I2C_TRANS_TIMEOUT);

    if (status != NvOdmI2cStatus_Success)
    {
        switch (status)
        {
            case NvOdmI2cStatus_Timeout:
                NvOdmOsPrintf("%s(): Failed: Timeout\n", __func__);
                break;
             case NvOdmI2cStatus_SlaveNotFound:
                NvOdmOsPrintf("%s(): Failed: SlaveNotFound slave Add 0x%x\n",
                          __func__, DevAddr);
                break;
             default:
                NvOdmOsPrintf("%s() Failed: status 0x%x\n", status, __func__);
                break;
        }
        return NV_FALSE;
    }

    *Data = ReadBuffer;
    return NV_TRUE;
}

static NvBool CpldWrite8(
    NvOdmServicesI2cHandle hOdmI2c,
    NvU32 DevAddr,
    NvU32 SpeedKHz,
    NvU8 RegAddr,
    NvU8 Data)
{
    NvU8 WriteBuffer[2];
    NvOdmI2cStatus status = NvOdmI2cStatus_Success;
    NvOdmI2cTransactionInfo TransactionInfo = {0};

    WriteBuffer[0] = RegAddr & 0xFF;    // PMU offset
    WriteBuffer[1] = Data & 0xFF;    // written data

    TransactionInfo.Address = DevAddr;
    TransactionInfo.Buf = &WriteBuffer[0];
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 2;

    status = NvOdmI2cTransaction(hOdmI2c, &TransactionInfo, 1,
                      SpeedKHz, CPLD_I2C_TRANS_TIMEOUT);

    if (status == NvOdmI2cStatus_Success)
        return NV_TRUE;

    switch (status)
    {
        case NvOdmI2cStatus_Timeout:
            NvOdmOsPrintf("%s(): Failed: Timeout\n", __func__);
            break;
         case NvOdmI2cStatus_SlaveNotFound:
            NvOdmOsPrintf("%s(): Failed: SlaveNotFound slave Add 0x%x\n",
                           __func__, DevAddr);
            break;
         default:
            NvOdmOsPrintf("%s() Failed: status 0x%x\n", status, __func__);
            break;
    }
    return NV_FALSE;
}

static NvBool CpldUpdate8(
    NvOdmServicesI2cHandle hOdmI2c,
    NvU32 DevAddr,
    NvU32 SpeedKHz,
    NvU8 RegAddr,
    NvU8 Value,
    NvU8 Mask)
{
    NvBool Status;
    NvU8 Data;

    Status = CpldRead8(hOdmI2c, DevAddr, SpeedKHz, RegAddr, &Data);
    if (!Status)
        goto End;

    Data = (Data & ~Mask) | (Value & Mask);
    Status = CpldWrite8(hOdmI2c, DevAddr, SpeedKHz, RegAddr, Data);

End:
    return Status;
}

static void NvOdmUartbCPLDProgram(void)
{
    NvOdmBoardInfo BoardInfo;
    NvOdmServicesI2cHandle hOdmI2c;
    NvOdmI2cTransactionInfo TransactionInfo;
    NvU8  DataBuffer[3];
    NvOdmI2cStatus status = NvOdmI2cStatus_WriteFailed;

    NvOdmPeripheralGetBoardInfo(0, &BoardInfo);

    if (((BoardInfo.SKU & SKU_SLT_ULPI_SUPPORT) &&
        ((BoardInfo.BoardID == NV_BOARD_E1186_ID) ||
        (BoardInfo.BoardID == NV_BOARD_E1187_ID) ||
        (BoardInfo.BoardID == NV_BOARD_PM269_ID))) ||
        (BoardInfo.BoardID == NV_BOARD_E1256_ID) ||
        (BoardInfo.BoardID == NV_BOARD_E1257_ID))
    {
        // Enabling Uart b for verbier boards (e1186/7, pm269) having ULPI
        // rework done as evident from 2nd bit of SKU using CPLD program 
        hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, 0x4);
        if (!hOdmI2c)
        {
            NvOdmOsPrintf("I2C open failed - ERROR \n");
            return;
        }

        DataBuffer[0] = 0x12;
        DataBuffer[1] = 0x00;

        TransactionInfo.Address = 0xD2;
        TransactionInfo.Buf = DataBuffer;
        TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
        TransactionInfo.NumBytes = 2;

        status = NvOdmI2cTransaction(hOdmI2c, &TransactionInfo, 1,
                CPLD_I2C_TRANS_SPEED, CPLD_I2C_TRANS_TIMEOUT);
        if (status != NvOdmI2cStatus_Success)
            NvOdmOsPrintf("UARTB CPLD Program - ERROR \n");

        DataBuffer[0] = 0x11;
        DataBuffer[1] = 0x92;

        status = NvOdmI2cTransaction(hOdmI2c, &TransactionInfo, 1,
                CPLD_I2C_TRANS_SPEED, 10);
        if (status != NvOdmI2cStatus_Success)
           NvOdmOsPrintf("UARTB CPLD Program - ERROR \n");

        NvOdmI2cClose(hOdmI2c);
    }
    else if ((BoardInfo.BoardID == NV_BOARD_E1186_ID) ||
                (BoardInfo.BoardID == NV_BOARD_E1198_ID))
    {
        // Routing the UARTB signal to the IrDA transceiver tfdu6103, on Verbier
        // boards (E1186 and E1198) which do not have the ULPI rework done
        hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, 0x4);
        if (!hOdmI2c)
        {
            NvOdmOsPrintf("I2C open failed - ERROR \n");
            return;
        }

        DataBuffer[0] = 0x11;
        DataBuffer[1] = 0x48;
        DataBuffer[2] = 0x26;

        TransactionInfo.Address = 0xD2;
        TransactionInfo.Buf = DataBuffer;
        TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
        TransactionInfo.NumBytes = 3;

        status = NvOdmI2cTransaction(hOdmI2c, &TransactionInfo, 1,
                CPLD_I2C_TRANS_SPEED, CPLD_I2C_TRANS_TIMEOUT);
        if (status != NvOdmI2cStatus_Success)
            NvOdmOsPrintf("UARTB CPLD Program - ERROR \n");

        NvOdmI2cClose(hOdmI2c);
    }
}

static NvBool NvOdmVddCoreInit(void)
{
     NvOdmPmuBoardInfo PmuInfo;

     NvBool Status;
     NvU32 SettlingTime;
     NvU32 VddId;
     static NvOdmServicesPmuHandle hPmu = NULL;

     Status = NvOdmQueryGetBoardModuleInfo(NvOdmBoardModuleType_PmuBoard,
                    &PmuInfo, sizeof(PmuInfo));
     if (!Status)
          return NV_FALSE;

     if (!PmuInfo.core_edp_mv)
          return NV_TRUE;

     VddId = CardhuPowerRails_VDD_CORE;

     if (!hPmu)
          hPmu = NvOdmServicesPmuOpen();

     if (!hPmu)
     {
          NvOdmOsDebugPrintf("PMU Device can not open\n");
          return NV_FALSE;
     }

     NvOdmOsDebugPrintf("Setting VddId=%d to %d mV\n", VddId, PmuInfo.core_edp_mv);
     NvOdmServicesPmuSetVoltage(hPmu, VddId, PmuInfo.core_edp_mv, &SettlingTime);
     if (SettlingTime)
         NvOdmOsWaitUS(SettlingTime);

     return NV_TRUE;
}

// Progarm EN_VDDIO_VID_OC* to enabling  enable +5V to the HDMI for pm269.
static void NvOdmHdmiCPLDProgram(void)
{
    NvOdmBoardInfo BoardInfo;
    NvOdmPeripheralGetBoardInfo(0, &BoardInfo);

    if ((BoardInfo.BoardID == NV_BOARD_PM269_ID) ||
        (BoardInfo.BoardID == NV_BOARD_PM272_ID) ||
        (BoardInfo.BoardID == NV_BOARD_PM305_ID) ||
        (BoardInfo.BoardID == NV_BOARD_PM311_ID))
    {
        NvOdmServicesI2cHandle hOdmHdmiI2c;
        NvOdmI2cTransactionInfo HdmiTransactionInfo;
        NvU8  DataBuffer[2];
        NvOdmI2cStatus status = NvOdmI2cStatus_WriteFailed;

        hOdmHdmiI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, 0x4);
        if (!hOdmHdmiI2c)
        {
            NvOdmOsPrintf("I2C open failed - ERROR \n");
            return;
        }

        DataBuffer[0] = 0x09;
        DataBuffer[1] = 0x90;

        HdmiTransactionInfo.Address = 0xD2;
        HdmiTransactionInfo.Buf = DataBuffer;
        HdmiTransactionInfo.Flags = NVODM_I2C_IS_WRITE;
        HdmiTransactionInfo.NumBytes = 2;

        status = NvOdmI2cTransaction(hOdmHdmiI2c, &HdmiTransactionInfo, 1,
                CPLD_I2C_TRANS_SPEED, CPLD_I2C_TRANS_TIMEOUT);
        if (status != NvOdmI2cStatus_Success)
            NvOdmOsPrintf("HDMI CPLD Program - ERROR \n");

        DataBuffer[0] = 0x1b;
        DataBuffer[1] = 0x01;

        HdmiTransactionInfo.Address = 0xD2;
        HdmiTransactionInfo.Buf = DataBuffer;
        HdmiTransactionInfo.Flags = NVODM_I2C_IS_WRITE;
        HdmiTransactionInfo.NumBytes = 2;

        status = NvOdmI2cTransaction(hOdmHdmiI2c, &HdmiTransactionInfo, 1,
                CPLD_I2C_TRANS_SPEED, CPLD_I2C_TRANS_TIMEOUT);
        if (status != NvOdmI2cStatus_Success)
            NvOdmOsPrintf("HDMI CPLD Program - ERROR \n");

        NvOdmI2cClose(hOdmHdmiI2c);
    }
}

// Progarm EN_VDD_SDMMC1* and G_VI_OE to enabling voltage on sdmmc1
static void NvOdmSdmmc1PowerCpldProgram(void)
{
    NvOdmBoardInfo BoardInfo;
    NvOdmPeripheralGetBoardInfo(0, &BoardInfo);

    if ((BoardInfo.BoardID == NV_BOARD_PM269_ID) ||
         (BoardInfo.BoardID == NV_BOARD_E1257_ID) ||
         (BoardInfo.BoardID == NV_BOARD_PM305_ID) ||
         (BoardInfo.BoardID == NV_BOARD_PM311_ID))
    {
        NvOdmServicesI2cHandle hOdmI2c;
        NvBool status;

        hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, 0x4);
        if (!hOdmI2c)
        {
            NvOdmOsPrintf("I2C open failed - ERROR \n");
            return;
        }

        // Make EN_VDD_SDMMC1 = 1
        status = CpldUpdate8(hOdmI2c, 0xD2, CPLD_I2C_TRANS_SPEED,
                         0x1A, (1 << 6), (1 << 6));

        // Make G_VI_1_DEMUX_SEL = 0
        if (status)
           status = CpldUpdate8(hOdmI2c, 0xD2, CPLD_I2C_TRANS_SPEED,
                         0x9, 0, (1 << 4));

        NvOdmOsPrintf("The enabling SDMMC1 Power rail is %s.\n",
                      (status)? "PASSED" : "FAILED");
        NvOdmI2cClose(hOdmI2c);
    }
}

static void NvOdmE1506CPLDProgram(void)
{
    NvOdmBoardInfo BoardInfo;
    NvBool IsE1506;

    IsE1506 = NvOdmPeripheralGetBoardInfo(NV_DISPLAY_BOARD_E1506, &BoardInfo);
    if (IsE1506)
    {
        NvOdmServicesI2cHandle hOdmI2c;
        NvBool status;

        hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, 0x4);
        if (!hOdmI2c)
        {
            NvOdmOsPrintf("I2C open failed - ERROR \n");
            return;
        }

        // Make G_LCD_DEMUX_SEL = 0
        status = CpldUpdate8(hOdmI2c, 0xD2, CPLD_I2C_TRANS_SPEED,
                         0xA, 0, (1 << 1));

        NvOdmI2cClose(hOdmI2c);
    }
}

static NvBool NvOdmPlatformConfigure_BlStart(void)
{
    NvOdmUartbCPLDProgram();
    return NV_TRUE;
}

static NvBool NvOdmPlatformConfigure_OsLoad(void)
{
    NvOdmVddCoreInit();
    NvOdmHdmiCPLDProgram();
    NvOdmE1506CPLDProgram();
    NvOdmSdmmc1PowerCpldProgram();
    return NV_TRUE;
}

NvBool NvOdmPlatformConfigure(int Config)
{
    if (Config == NvOdmPlatformConfig_OsLoad)
         return NvOdmPlatformConfigure_OsLoad();

    if (Config == NvOdmPlatformConfig_BlStart)
         return NvOdmPlatformConfigure_BlStart();

    return NV_TRUE;
}

