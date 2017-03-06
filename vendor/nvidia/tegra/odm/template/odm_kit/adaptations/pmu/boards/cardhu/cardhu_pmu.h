/*
 * Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_CARDHU_PMU_H
#define INCLUDED_CARDHU_PMU_H

#include "nvodm_query_discovery.h"
#include "nvodm_pmu.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/* Processor Board  ID */
#define PROC_BOARD_E1187    0x0B57
#define PROC_BOARD_E1186    0x0B56
#define PROC_BOARD_E1198    0x0B62
#define PROC_BOARD_E1256    0x0C38
#define PROC_BOARD_E1291    0x0C5B
#define PROC_BOARD_PM269    0x0245

/* PMU Board ID */
#define BOARD_PMU_E1208     0x0C08
#define BOARD_PMU_PM298     0x0262
#define BOARD_PMU_PM299     0x0263

/* SKU Information */
#define SKU_DCDC_TPS62361_SUPPORT       0x1
#define SKU_SLT_ULPI_SUPPORT            0x2
#define SKU_T30S_SUPPORT                0x4

/* Board Fab version */
#define BOARD_FAB_A00                   0x0
#define BOARD_FAB_A01                   0x1
#define BOARD_FAB_A02                   0x2
#define BOARD_FAB_A03                   0x3
#define BOARD_FAB_A04                   0x4

typedef struct PmuChipDriverOpsRec {
    void (*GetCaps)(void *pDriverData, NvU32 vddRail,
            NvOdmPmuVddRailCapabilities* pCapabilities);
    NvBool (*GetVoltage)(void *pDriverData,
            NvU32 vddRail, NvU32* pMilliVolts);
    NvBool (*SetVoltage) (void *pDriverData,
            NvU32 vddRail, NvU32 MilliVolts, NvU32* pSettleMicroSeconds);
} PmuChipDriverOps, *PmuChipDriverOpsHandle;

void
*CardhuPmu_GetTps6591xMap(
    NvOdmPmuDeviceHandle hDevice,
    PmuChipDriverOpsHandle *hDriverOps,
    NvU32 *pRailMap,
    NvOdmBoardInfo *pProcBoard,
    NvOdmBoardInfo *pPmuBoard);

void
*CardhuPmu_GetTps6236xMap(
    NvOdmPmuDeviceHandle hDevice,
    PmuChipDriverOpsHandle *hDriverOps,
    NvU32 *pRailMap,
    NvOdmBoardInfo *pProcBoard,
    NvOdmBoardInfo *pPmuBoard);

void
*CardhuPmu_GetApGpioRailMap(
    NvOdmPmuDeviceHandle hPmuDevice,
    PmuChipDriverOpsHandle *hDriverOps,
    NvU32 *pRailMap,
    NvOdmBoardInfo *pProcBoard,
    NvOdmBoardInfo *pPmuBoard);

void
*CardhuPmu_GetRicoh583RailMap(
    NvOdmPmuDeviceHandle hPmuDevice,
    PmuChipDriverOpsHandle *hDriverOps,
    NvU32 *pRailMap,
    NvOdmBoardInfo *pProcBoard,
    NvOdmBoardInfo *pPmuBoard);

void
*CardhuPmu_GetMax77663RailMap(
    NvOdmPmuDeviceHandle hPmuDevice,
    PmuChipDriverOpsHandle *hDriverOps,
    NvU32 *pRailMap,
    NvOdmBoardInfo *pProcBoard,
    NvOdmBoardInfo *pPmuBoard);

#if defined(__cplusplus)
}
#endif

#endif /* INCLUDED_CARDHU_PMU_H */
