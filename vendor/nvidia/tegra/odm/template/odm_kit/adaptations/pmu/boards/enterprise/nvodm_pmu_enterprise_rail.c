/*
 * Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
/**
 * @file
 * <b>NVIDIA Tegra ODM Kit: Implementation of public API of
 *                          Enterprise pmu driver interface.</b>
 *
 * @b Description: Implementation of  the power regulator api of public
 *                        interface for the pmu driver for enterprise.
 *
 */

#include "nvodm_pmu.h"
#include "nvodm_pmu_enterprise.h"
#include "enterprise_pmu.h"
#include "nvodm_pmu_enterprise_supply_info_table.h"

typedef struct EnterprisePmuRailMapRec {
    NvU32 PmuVddId;
    PmuChipDriverOpsHandle hPmuDriverOps;
    void *hPmuDriver;
} EnterprisePmuRailMap;

static EnterprisePmuRailMap s_EnterpriseRail_PmuBoard[EnterprisePowerRails_Num];

void EnterprisePmuGetCapabilities(NvU32 VddRail,
            NvOdmPmuVddRailCapabilities* pCapabilities)
{
    NV_ASSERT((VddRail != EnterprisePowerRails_Invalid) &&
                    (VddRail < EnterprisePowerRails_Num));
    NV_ASSERT(s_EnterpriseRail_PmuBoard[VddRail].PmuVddId != EnterprisePowerRails_Invalid);
    if (s_EnterpriseRail_PmuBoard[VddRail].PmuVddId == EnterprisePowerRails_Invalid)
    {
         NvOdmOsPrintf("%s(): The power rail %d is not mapped properly\n", __func__, VddRail);
         NvOdmOsMemset(pCapabilities, 0, sizeof(NvOdmPmuVddRailCapabilities));
         return;
    }

    s_EnterpriseRail_PmuBoard[VddRail].hPmuDriverOps->GetCaps(
            s_EnterpriseRail_PmuBoard[VddRail].hPmuDriver,
            s_EnterpriseRail_PmuBoard[VddRail].PmuVddId, pCapabilities);
}

NvBool EnterprisePmuGetVoltage(NvOdmPmuDeviceHandle hPmuDevice, NvU32 VddRail,
            NvU32* pMilliVolts)
{
    NV_ASSERT((VddRail != EnterprisePowerRails_Invalid) &&
                (VddRail < EnterprisePowerRails_Num));
    NV_ASSERT(s_EnterpriseRail_PmuBoard[VddRail].PmuVddId != EnterprisePowerRails_Invalid);
    if (s_EnterpriseRail_PmuBoard[VddRail].PmuVddId == EnterprisePowerRails_Invalid)
    {
         NvOdmOsPrintf("%s(): The power rail %d is not mapped properly\n", __func__, VddRail);
         return NV_FALSE;
    }

    return s_EnterpriseRail_PmuBoard[VddRail].hPmuDriverOps->GetVoltage(
            s_EnterpriseRail_PmuBoard[VddRail].hPmuDriver,
            s_EnterpriseRail_PmuBoard[VddRail].PmuVddId, pMilliVolts);
}

NvBool EnterprisePmuSetVoltage(NvOdmPmuDeviceHandle hPmuDevice, NvU32 VddRail,
            NvU32 MilliVolts, NvU32* pSettleMicroSeconds)
{
    NV_ASSERT((VddRail != EnterprisePowerRails_Invalid) &&
                (VddRail < EnterprisePowerRails_Num));
    NV_ASSERT(s_EnterpriseRail_PmuBoard[VddRail].PmuVddId != EnterprisePowerRails_Invalid);
    if (s_EnterpriseRail_PmuBoard[VddRail].PmuVddId == EnterprisePowerRails_Invalid)
    {
         NvOdmOsPrintf("%s(): The power rail %d is not mapped properly\n", __func__, VddRail);
         return NV_FALSE;
    }

    return s_EnterpriseRail_PmuBoard[VddRail].hPmuDriverOps->SetVoltage(
            s_EnterpriseRail_PmuBoard[VddRail].hPmuDriver,
            s_EnterpriseRail_PmuBoard[VddRail].PmuVddId, MilliVolts,
            pSettleMicroSeconds);
}

static NvBool InitTps80031BasedRailMapping(NvOdmPmuDeviceHandle hPmuDevice,
        NvOdmBoardInfo *pProcBoard, NvOdmBoardInfo *pPmuBoard)
{
    void *hPmuDriver;
    PmuChipDriverOpsHandle hDriverOps;
    NvU32 RailMap[EnterprisePowerRails_Num];
    int i;

    for (i =0; i < EnterprisePowerRails_Num; ++i)
        RailMap[i] = 0xFFFF;

    hPmuDriver = EnterprisePmu_GetTps80031Map(hPmuDevice, &hDriverOps, RailMap,
                        pProcBoard, pPmuBoard);
    if (!hPmuDriver)
    {
        NvOdmOsPrintf("%s(): Call EnterprisePmu_GetTps80031Map is failed\n", __func__);
        return NV_FALSE;
    }
    for (i =0; i < EnterprisePowerRails_Num; ++i)
    {
        if ((RailMap[i] == 0xFFFF) || (RailMap[i] == 0))
            continue;

        s_EnterpriseRail_PmuBoard[i].PmuVddId = RailMap[i];
        s_EnterpriseRail_PmuBoard[i].hPmuDriverOps = hDriverOps;
        s_EnterpriseRail_PmuBoard[i].hPmuDriver = hPmuDriver;
    }
    return NV_TRUE;
}

static NvBool InitApGpioBasedRailMapping(NvOdmPmuDeviceHandle hPmuDevice,
        NvOdmBoardInfo *pProcBoard, NvOdmBoardInfo *pPmuBoard)
{
    void *hPmuDriver;
    PmuChipDriverOpsHandle hDriverOps;
    NvU32 RailMap[EnterprisePowerRails_Num];
    int i;

    for (i =0; i < EnterprisePowerRails_Num; ++i)
        RailMap[i] = 0xFFFF;

    hPmuDriver = EnterprisePmu_GetApGpioRailMap(hPmuDevice, &hDriverOps, RailMap,
                    pProcBoard, pPmuBoard);
    if (!hPmuDriver)
    {
        NvOdmOsPrintf("%s(): Call EnterprisePmu_GetApGpioRailMap is failed\n", __func__);
        return NV_FALSE;
    }
    for (i =0; i < EnterprisePowerRails_Num; ++i)
    {
        if ((RailMap[i] == 0xFFFF) || (RailMap[i] == 0))
            continue;

        s_EnterpriseRail_PmuBoard[i].PmuVddId = RailMap[i];
        s_EnterpriseRail_PmuBoard[i].hPmuDriverOps = hDriverOps;
        s_EnterpriseRail_PmuBoard[i].hPmuDriver = hPmuDriver;
    }
    return NV_TRUE;
}

NvBool EnterprisePmuSetup(NvOdmPmuDeviceHandle hPmuDevice)
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
        NvOdmOsMemcpy(&PmuBoard, &ProcBoard, sizeof(NvOdmBoardInfo));
    else
    {
        NvOdmOsPrintf("%s(): Not able to get BoardInfo\n", __func__);
        return NV_FALSE;
    }

    NvOdmOsMemset(&s_EnterpriseRail_PmuBoard, 0, sizeof(s_EnterpriseRail_PmuBoard));

    for (i = 0; i < EnterprisePowerRails_Num; ++i)
        s_EnterpriseRail_PmuBoard[i].PmuVddId = EnterprisePowerRails_Invalid;

    InitTps80031BasedRailMapping(hPmuDevice, &ProcBoard, &PmuBoard);
    InitApGpioBasedRailMapping(hPmuDevice, &ProcBoard, &PmuBoard);

    hBatDriver = EnterprisePmu_Tps80031_BatterySetup(hPmuDevice, &ProcBoard,
                        &PmuBoard);
    if (!hBatDriver)
        NvOdmOsPrintf("%s(): Not able to do BatterySetup\n", __func__);

    return NV_TRUE;
}

void EnterprisePmuRelease(NvOdmPmuDeviceHandle hPmuDevice)
{
}
