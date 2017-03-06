/*
 * Copyright (c) 2011-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_pmu.h"
#include "nvodm_pmu_cardhu.h"
#include "cardhu_pmu.h"
#include "nvodm_pmu_cardhu_supply_info_table.h"
#include "nvodm_services.h"
#include "ricoh583/nvodm_pmu_ricoh583_pmu_driver.h"
#include "max77663/nvodm_pmu_max77663_pmu_driver.h"
#include "tps6591x/nvodm_pmu_tps6591x_pmu_driver.h"

typedef struct CardhuPmuRailMapRec {
    NvU32 PmuVddId;
    PmuChipDriverOpsHandle hPmuDriverOps;
    void *hPmuDriver;
} CardhuPmuRailMap;

static CardhuPmuRailMap s_CardhuRail_PmuBoard[CardhuPowerRails_Num];

void CardhuPmuGetCapabilities(NvU32 VddRail,
            NvOdmPmuVddRailCapabilities* pCapabilities)
{
    PmuDebugPrintf("%s(): The power rail %d \n", __func__, VddRail);
    NV_ASSERT((VddRail != CardhuPowerRails_Invalid) &&
                    (VddRail < CardhuPowerRails_Num));
    NV_ASSERT(s_CardhuRail_PmuBoard[VddRail].PmuVddId != CardhuPowerRails_Invalid);
    if (s_CardhuRail_PmuBoard[VddRail].PmuVddId == CardhuPowerRails_Invalid)
    {
         NvOdmOsPrintf("%s(): The power rail %d is not mapped properly\n",
                        __func__, VddRail);
         NvOdmOsMemset(pCapabilities, 0, sizeof(NvOdmPmuVddRailCapabilities));
         return;
    }

    s_CardhuRail_PmuBoard[VddRail].hPmuDriverOps->GetCaps(
             s_CardhuRail_PmuBoard[VddRail].hPmuDriver,
            s_CardhuRail_PmuBoard[VddRail].PmuVddId, pCapabilities);
}

NvBool CardhuPmuGetVoltage(NvOdmPmuDeviceHandle hPmuDevice, NvU32 VddRail,
            NvU32* pMilliVolts)
{
    PmuDebugPrintf("%s(): The power rail %d \n", __func__, VddRail);
    NV_ASSERT((VddRail != CardhuPowerRails_Invalid) &&
                (VddRail < CardhuPowerRails_Num));
    NV_ASSERT(s_CardhuRail_PmuBoard[VddRail].PmuVddId != CardhuPowerRails_Invalid);
    if (s_CardhuRail_PmuBoard[VddRail].PmuVddId == CardhuPowerRails_Invalid)
    {
         NvOdmOsPrintf("%s(): The power rail %d is not mapped properly\n",
                       __func__, VddRail);
         return NV_FALSE;
    }

    return s_CardhuRail_PmuBoard[VddRail].hPmuDriverOps->GetVoltage(
            s_CardhuRail_PmuBoard[VddRail].hPmuDriver,
            s_CardhuRail_PmuBoard[VddRail].PmuVddId, pMilliVolts);
}

NvBool CardhuPmuSetVoltage(NvOdmPmuDeviceHandle hPmuDevice, NvU32 VddRail,
            NvU32 MilliVolts, NvU32* pSettleMicroSeconds)
{
    PmuDebugPrintf("%s(): The power rail %d 0x%02x\n", __func__, VddRail,
                  MilliVolts);
    NV_ASSERT((VddRail != CardhuPowerRails_Invalid) &&
                (VddRail < CardhuPowerRails_Num));
    NV_ASSERT(s_CardhuRail_PmuBoard[VddRail].PmuVddId != CardhuPowerRails_Invalid);
    if (s_CardhuRail_PmuBoard[VddRail].PmuVddId == CardhuPowerRails_Invalid)
    {
         NvOdmOsPrintf("%s(): The power rail %d is not mapped properly\n",
                      __func__, VddRail);
         return NV_FALSE;
    }

    return s_CardhuRail_PmuBoard[VddRail].hPmuDriverOps->SetVoltage(
            s_CardhuRail_PmuBoard[VddRail].hPmuDriver,
            s_CardhuRail_PmuBoard[VddRail].PmuVddId, MilliVolts,
            pSettleMicroSeconds);
}

static NvBool InitTps6591xBasedRailMapping(NvOdmPmuDeviceHandle hPmuDevice,
        NvOdmBoardInfo *pProcBoard, NvOdmBoardInfo *pPmuBoard)
{
    void *hPmuDriver;
    PmuChipDriverOpsHandle hDriverOps;
    NvU32 RailMap[CardhuPowerRails_Num];
    int i;

    for (i =0; i < CardhuPowerRails_Num; ++i)
        RailMap[i] = 0xFFFF;

    hPmuDriver = CardhuPmu_GetTps6591xMap(hPmuDevice, &hDriverOps, RailMap,
                        pProcBoard, pPmuBoard);
    if (!hPmuDriver)
    {
        NvOdmOsPrintf("%s(): Call CardhuPmu_GetTps6591xMap is failed\n",
                    __func__);
        return NV_FALSE;
    }
    for (i =0; i < CardhuPowerRails_Num; ++i)
    {
        if ((RailMap[i] == 0xFFFF) || (RailMap[i] == 0))
            continue;

        s_CardhuRail_PmuBoard[i].PmuVddId = RailMap[i];
        s_CardhuRail_PmuBoard[i].hPmuDriverOps = hDriverOps;
        s_CardhuRail_PmuBoard[i].hPmuDriver = hPmuDriver;
    }
    return NV_TRUE;
}

static NvBool InitRicoh583BasedRailMapping(NvOdmPmuDeviceHandle hPmuDevice,
        NvOdmBoardInfo *pProcBoard, NvOdmBoardInfo *pPmuBoard)
{
    void *hPmuDriver;
    PmuChipDriverOpsHandle hDriverOps;
    NvU32 RailMap[CardhuPowerRails_Num];
    int i;

    for (i =0; i < CardhuPowerRails_Num; ++i)
        RailMap[i] = 0xFFFF;

    hPmuDriver = CardhuPmu_GetRicoh583RailMap(hPmuDevice, &hDriverOps, RailMap,
                        pProcBoard, pPmuBoard);
    if (!hPmuDriver)
    {
        NvOdmOsPrintf("%s(): Call CardhuPmu_GetRicoh583RailMap is failed\n",
                    __func__);
        return NV_FALSE;
    }
    for (i =0; i < CardhuPowerRails_Num; ++i)
    {
        if ((RailMap[i] == 0xFFFF) || (RailMap[i] == 0))
            continue;

        s_CardhuRail_PmuBoard[i].PmuVddId = RailMap[i];
        s_CardhuRail_PmuBoard[i].hPmuDriverOps = hDriverOps;
        s_CardhuRail_PmuBoard[i].hPmuDriver = hPmuDriver;
    }
    return NV_TRUE;
}

static NvBool InitTps6236xBasedRailMapping(NvOdmPmuDeviceHandle hPmuDevice,
        NvOdmBoardInfo *pProcBoard, NvOdmBoardInfo *pPmuBoard)
{
    void *hPmuDriver;
    PmuChipDriverOpsHandle hDriverOps;
    NvU32 RailMap[CardhuPowerRails_Num];
    int i;

    for (i =0; i < CardhuPowerRails_Num; ++i)
        RailMap[i] = 0xFFFF;

    hPmuDriver = CardhuPmu_GetTps6236xMap(hPmuDevice, &hDriverOps, RailMap,
                        pProcBoard, pPmuBoard);
    if (!hPmuDriver)
    {
        NvOdmOsPrintf("%s(): Call CardhuPmu_GetTps6236xMap is failed\n",
                    __func__);
        return NV_FALSE;
    }
    for (i =0; i < CardhuPowerRails_Num; ++i)
    {
        if ((RailMap[i] == 0xFFFF) || (RailMap[i] == 0))
            continue;

        s_CardhuRail_PmuBoard[i].PmuVddId = RailMap[i];
        s_CardhuRail_PmuBoard[i].hPmuDriverOps = hDriverOps;
        s_CardhuRail_PmuBoard[i].hPmuDriver = hPmuDriver;
    }
    return NV_TRUE;
}

static NvBool InitMax77663BasedRailMapping(NvOdmPmuDeviceHandle hPmuDevice,
        NvOdmBoardInfo *pProcBoard, NvOdmBoardInfo *pPmuBoard)
{
    void *hPmuDriver;
    PmuChipDriverOpsHandle hDriverOps;
    NvU32 RailMap[CardhuPowerRails_Num];
    int i;

    for (i =0; i < CardhuPowerRails_Num; ++i)
        RailMap[i] = 0xFFFF;

    hPmuDriver = CardhuPmu_GetMax77663RailMap(hPmuDevice, &hDriverOps, RailMap,
                        pProcBoard, pPmuBoard);
    if (!hPmuDriver)
    {
        NvOdmOsPrintf("%s(): Call CardhuPmu_GetMax77663RailMap is failed\n",
                    __func__);
        return NV_FALSE;
    }
    for (i =0; i < CardhuPowerRails_Num; ++i)
    {
        if ((RailMap[i] == 0xFFFF) || (RailMap[i] == 0))
            continue;

        s_CardhuRail_PmuBoard[i].PmuVddId = RailMap[i];
        s_CardhuRail_PmuBoard[i].hPmuDriverOps = hDriverOps;
        s_CardhuRail_PmuBoard[i].hPmuDriver = hPmuDriver;
    }
    return NV_TRUE;
}

static NvBool InitApGpioBasedRailMapping(NvOdmPmuDeviceHandle hPmuDevice,
        NvOdmBoardInfo *pProcBoard, NvOdmBoardInfo *pPmuBoard)
{
    void *hPmuDriver;
    PmuChipDriverOpsHandle hDriverOps;
    NvU32 RailMap[CardhuPowerRails_Num];
    int i;

    for (i =0; i < CardhuPowerRails_Num; ++i)
        RailMap[i] = 0xFFFF;

    hPmuDriver = CardhuPmu_GetApGpioRailMap(hPmuDevice, &hDriverOps, RailMap,
                    pProcBoard, pPmuBoard);
    if (!hPmuDriver)
    {
        NvOdmOsPrintf("%s(): Call CardhuPmu_GetApGpioRailMap is failed\n", __func__);
        return NV_FALSE;
    }
    for (i =0; i < CardhuPowerRails_Num; ++i)
    {
        if ((RailMap[i] == 0xFFFF) || (RailMap[i] == 0))
            continue;

        s_CardhuRail_PmuBoard[i].PmuVddId = RailMap[i];
        s_CardhuRail_PmuBoard[i].hPmuDriverOps = hDriverOps;
        s_CardhuRail_PmuBoard[i].hPmuDriver = hPmuDriver;
    }
    return NV_TRUE;
}

NvBool CardhuPmuSetup(NvOdmPmuDeviceHandle hPmuDevice)
{
    int i, vsels;
    NvOdmBoardInfo ProcBoard;
    NvOdmBoardInfo PmuBoard;
    NvBool status;

    NvOdmOsMemset(&ProcBoard, 0, sizeof(NvOdmBoardInfo));
    NvOdmOsMemset(&PmuBoard, 0, sizeof(NvOdmBoardInfo));

    /* Get processor Board */
    status = NvOdmPeripheralGetBoardInfo(0, &ProcBoard);
    if (status)
    {
        if ((ProcBoard.BoardID == PROC_BOARD_E1198) ||
                    (ProcBoard.BoardID == PROC_BOARD_E1291))
            NvOdmOsMemcpy(&PmuBoard, &ProcBoard, sizeof(NvOdmBoardInfo));
        else
        {
            status = NvOdmPeripheralGetBoardInfo(BOARD_PMU_E1208, &PmuBoard);
            if (!status)
                status = NvOdmPeripheralGetBoardInfo(BOARD_PMU_PM298, &PmuBoard);
            if (!status)
                status = NvOdmPeripheralGetBoardInfo(BOARD_PMU_PM299, &PmuBoard);
        }
    }

    NvOdmOsMemset(&s_CardhuRail_PmuBoard, 0, sizeof(s_CardhuRail_PmuBoard));

    for (i =0; i < CardhuPowerRails_Num; ++i)
        s_CardhuRail_PmuBoard[i].PmuVddId = CardhuPowerRails_Invalid;

    if ((ProcBoard.BoardID == PROC_BOARD_E1198) ||
            (ProcBoard.BoardID == PROC_BOARD_E1291) ||
                (PmuBoard.BoardID == BOARD_PMU_E1208))
        InitTps6591xBasedRailMapping(hPmuDevice, &ProcBoard, &PmuBoard);
    else if (PmuBoard.BoardID == BOARD_PMU_PM298)
        InitMax77663BasedRailMapping(hPmuDevice, &ProcBoard, &PmuBoard);
    else if (PmuBoard.BoardID == BOARD_PMU_PM299)
        InitRicoh583BasedRailMapping(hPmuDevice, &ProcBoard, &PmuBoard);
    else
        NvOdmOsPrintf("BoardId is not supported ProcBoard 0x%04x and Pmu Board 0x%04x\n",
              ProcBoard.BoardID, PmuBoard.BoardID);

    if (ProcBoard.BoardID == PROC_BOARD_E1198)
    {
        switch(ProcBoard.Fab) {
        case BOARD_FAB_A00:
        case BOARD_FAB_A01:
        case BOARD_FAB_A02:
            if ((ProcBoard.SKU & SKU_DCDC_TPS62361_SUPPORT) ||
                   (PmuBoard.SKU & SKU_DCDC_TPS62361_SUPPORT))
                InitTps6236xBasedRailMapping(hPmuDevice, &ProcBoard, &PmuBoard);
            break;
        case BOARD_FAB_A03:
            vsels = (((ProcBoard.SKU >> 1) & 0x2) | (ProcBoard.SKU & 0x1));
            switch(vsels) {
            case 1:
            case 2:
            case 3:
                InitTps6236xBasedRailMapping(hPmuDevice, &ProcBoard, &PmuBoard);
                break;
            }
            break;
        }
    } else if ((ProcBoard.SKU & SKU_DCDC_TPS62361_SUPPORT) ||
                   (PmuBoard.SKU & SKU_DCDC_TPS62361_SUPPORT))
    {
        InitTps6236xBasedRailMapping(hPmuDevice, &ProcBoard, &PmuBoard);
    }

    /* GPIO based power rail control */
    InitApGpioBasedRailMapping(hPmuDevice, &ProcBoard, &PmuBoard);

    return NV_TRUE;
}

void CardhuPmuRelease(NvOdmPmuDeviceHandle hPmuDevice)
{
}

NvBool CardhuPmuPowerOff(NvOdmPmuDeviceHandle hPmuDevice)
{
    NvOdmBoardInfo ProcBoard;
    NvOdmBoardInfo PmuBoard;
    NvBool status;

    NV_ASSERT(hPmuDevice);
    NvOdmOsMemset(&ProcBoard, 0, sizeof(NvOdmBoardInfo));
    NvOdmOsMemset(&PmuBoard, 0, sizeof(NvOdmBoardInfo));

    /* Get processor Board */
    status = NvOdmPeripheralGetBoardInfo(0, &ProcBoard);
    if(!status)
        return NV_FALSE;

    if ((ProcBoard.BoardID == PROC_BOARD_E1198) ||
            (ProcBoard.BoardID == PROC_BOARD_E1291))
        NvOdmOsMemcpy(&PmuBoard, &ProcBoard, sizeof(NvOdmBoardInfo));
    else
        status = NvOdmPeripheralGetBoardInfo(BOARD_PMU_E1208, &PmuBoard);

    if ((ProcBoard.BoardID == PROC_BOARD_E1198) ||
            (ProcBoard.BoardID == PROC_BOARD_E1291) ||
            (PmuBoard.BoardID == BOARD_PMU_E1208))
        Tps6591xPmuPowerOff(s_CardhuRail_PmuBoard[1].hPmuDriver);

    //shouldn't reach here
    NV_ASSERT(0);
    return NV_FALSE;
}
