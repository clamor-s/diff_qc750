/*
 * Copyright (c) 2012 NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_pmu.h"
#include "nvodm_pmu_kai.h"
#include "kai_pmu.h"
#include "nvodm_pmu_kai_supply_info_table.h"
#include "nvodm_services.h"
#include "max77663/nvodm_pmu_max77663_pmu_driver.h"

typedef struct KaiPmuRailMapRec {
    NvU32 PmuVddId;
    PmuChipDriverOpsHandle hPmuDriverOps;
    void *hPmuDriver;
} KaiPmuRailMap;

static KaiPmuRailMap s_KaiRail_PmuBoard[KaiPowerRails_Num];

void KaiPmuGetCapabilities(NvU32 VddRail,
            NvOdmPmuVddRailCapabilities* pCapabilities)
{
    PmuDebugPrintf("%s(): The power rail %d \n", __func__, VddRail);
    NV_ASSERT((VddRail != KaiPowerRails_Invalid) &&
                    (VddRail < KaiPowerRails_Num));
    NV_ASSERT(s_KaiRail_PmuBoard[VddRail].PmuVddId != KaiPowerRails_Invalid);
    if (s_KaiRail_PmuBoard[VddRail].PmuVddId == KaiPowerRails_Invalid)
    {
         NvOdmOsPrintf("%s(): The power rail %d is not mapped properly\n",
                        __func__, VddRail);
         NvOdmOsMemset(pCapabilities, 0, sizeof(NvOdmPmuVddRailCapabilities));
         return;
    }

    s_KaiRail_PmuBoard[VddRail].hPmuDriverOps->GetCaps(
             s_KaiRail_PmuBoard[VddRail].hPmuDriver,
            s_KaiRail_PmuBoard[VddRail].PmuVddId, pCapabilities);
}

NvBool KaiPmuGetVoltage(NvOdmPmuDeviceHandle hPmuDevice, NvU32 VddRail,
            NvU32* pMilliVolts)
{
    PmuDebugPrintf("%s(): The power rail %d \n", __func__, VddRail);
    NV_ASSERT((VddRail != KaiPowerRails_Invalid) &&
                (VddRail < KaiPowerRails_Num));
    NV_ASSERT(s_KaiRail_PmuBoard[VddRail].PmuVddId != KaiPowerRails_Invalid);
    if (s_KaiRail_PmuBoard[VddRail].PmuVddId == KaiPowerRails_Invalid)
    {
         NvOdmOsPrintf("%s(): The power rail %d is not mapped properly\n",
                       __func__, VddRail);
         return NV_FALSE;
    }

    return s_KaiRail_PmuBoard[VddRail].hPmuDriverOps->GetVoltage(
            s_KaiRail_PmuBoard[VddRail].hPmuDriver,
            s_KaiRail_PmuBoard[VddRail].PmuVddId, pMilliVolts);
}

NvBool KaiPmuSetVoltage(NvOdmPmuDeviceHandle hPmuDevice, NvU32 VddRail,
            NvU32 MilliVolts, NvU32* pSettleMicroSeconds)
{
    PmuDebugPrintf("%s(): The power rail %d 0x%02x\n", __func__, VddRail,
                  MilliVolts);
    NV_ASSERT((VddRail != KaiPowerRails_Invalid) &&
                (VddRail < KaiPowerRails_Num));
    NV_ASSERT(s_KaiRail_PmuBoard[VddRail].PmuVddId != KaiPowerRails_Invalid);
    if (s_KaiRail_PmuBoard[VddRail].PmuVddId == KaiPowerRails_Invalid)
    {
         NvOdmOsPrintf("%s(): The power rail %d is not mapped properly\n",
                      __func__, VddRail);
         return NV_FALSE;
    }

    return s_KaiRail_PmuBoard[VddRail].hPmuDriverOps->SetVoltage(
            s_KaiRail_PmuBoard[VddRail].hPmuDriver,
            s_KaiRail_PmuBoard[VddRail].PmuVddId, MilliVolts,
            pSettleMicroSeconds);
}

static NvBool InitMax77663BasedRailMapping(NvOdmPmuDeviceHandle hPmuDevice,
        NvOdmBoardInfo *pProcBoard, NvOdmBoardInfo *pPmuBoard)
{
    void *hPmuDriver;
    PmuChipDriverOpsHandle hDriverOps;
    NvU32 RailMap[KaiPowerRails_Num];
    int i;

    for (i =0; i < KaiPowerRails_Num; ++i)
        RailMap[i] = 0xFFFF;

    hPmuDriver = KaiPmu_GetMax77663RailMap(hPmuDevice, &hDriverOps, RailMap,
                        pProcBoard, pPmuBoard);
    if (!hPmuDriver)
    {
        NvOdmOsPrintf("%s(): Call KaiPmu_GetMax77663RailMap is failed\n",
                    __func__);
        return NV_FALSE;
    }
    for (i =0; i < KaiPowerRails_Num; ++i)
    {
        if ((RailMap[i] == 0xFFFF) || (RailMap[i] == 0))
            continue;

        s_KaiRail_PmuBoard[i].PmuVddId = RailMap[i];
        s_KaiRail_PmuBoard[i].hPmuDriverOps = hDriverOps;
        s_KaiRail_PmuBoard[i].hPmuDriver = hPmuDriver;
    }
    return NV_TRUE;
}

static NvBool InitApGpioBasedRailMapping(NvOdmPmuDeviceHandle hPmuDevice,
        NvOdmBoardInfo *pProcBoard, NvOdmBoardInfo *pPmuBoard)
{
    void *hPmuDriver;
    PmuChipDriverOpsHandle hDriverOps;
    NvU32 RailMap[KaiPowerRails_Num];
    int i;

    for (i =0; i < KaiPowerRails_Num; ++i)
        RailMap[i] = 0xFFFF;

    hPmuDriver = KaiPmu_GetApGpioRailMap(hPmuDevice, &hDriverOps, RailMap,
                    pProcBoard, pPmuBoard);
    if (!hPmuDriver)
    {
        NvOdmOsPrintf("%s(): Call KaiPmu_GetApGpioRailMap is failed\n", __func__);
        return NV_FALSE;
    }
    for (i =0; i < KaiPowerRails_Num; ++i)
    {
        if ((RailMap[i] == 0xFFFF) || (RailMap[i] == 0))
            continue;

        s_KaiRail_PmuBoard[i].PmuVddId = RailMap[i];
        s_KaiRail_PmuBoard[i].hPmuDriverOps = hDriverOps;
        s_KaiRail_PmuBoard[i].hPmuDriver = hPmuDriver;
    }
    return NV_TRUE;
}

NvBool KaiPmuSetup(NvOdmPmuDeviceHandle hPmuDevice)
{
    int i;
    NvOdmBoardInfo ProcBoard;
    NvOdmBoardInfo PmuBoard;
    NvBool status;
    void *hBatDriver;

    NvOdmOsMemset(&ProcBoard, 0, sizeof(NvOdmBoardInfo));
    NvOdmOsMemset(&PmuBoard, 0, sizeof(NvOdmBoardInfo));

    /* Get processor Board */
    status = NvOdmPeripheralGetBoardInfo(0, &ProcBoard);
    if (status)
    {
        if (ProcBoard.BoardID == PROC_BOARD_E1565)
            NvOdmOsMemcpy(&PmuBoard, &ProcBoard, sizeof(NvOdmBoardInfo));
    }

    NvOdmOsMemset(&s_KaiRail_PmuBoard, 0, sizeof(s_KaiRail_PmuBoard));

    for (i =0; i < KaiPowerRails_Num; ++i)
        s_KaiRail_PmuBoard[i].PmuVddId = KaiPowerRails_Invalid;

    if (ProcBoard.BoardID == PROC_BOARD_E1565)
        InitMax77663BasedRailMapping(hPmuDevice, &ProcBoard, &PmuBoard);
    else
        NvOdmOsPrintf("BoardId is not supported ProcBoard 0x%04x and Pmu Board 0x%04x\n",
              ProcBoard.BoardID, PmuBoard.BoardID);

    /* GPIO based power rail control */
    InitApGpioBasedRailMapping(hPmuDevice, &ProcBoard, &PmuBoard);

    hBatDriver = KaiPmu_BatterySetup(hPmuDevice, &ProcBoard,
                        &PmuBoard);
    if (!hBatDriver)
        NvOdmOsPrintf("%s(): Not able to do BatterySetup\n", __func__);

    return NV_TRUE;
}

void KaiPmuRelease(NvOdmPmuDeviceHandle hPmuDevice)
{
}

NvBool KaiPmuPowerOff(NvOdmPmuDeviceHandle hDevice)
{
    NV_ASSERT(hDevice);
    Max77663PmuPowerOff(s_KaiRail_PmuBoard[1].hPmuDriver);
    //shouldn't reach here
    NV_ASSERT(0);
    return NV_FALSE;
}
