/*
 * Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
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
#include "tps6236x/nvodm_pmu_tps6236x_pmu_driver.h"

static Tps6236xBoardData s_tps6236x_bdata = {
      NV_FALSE, Tps6236xMode_3,
};
static NvOdmPmuRailProps s_tps6236x_rail_props[] = {
      {
           Tps6236xPmuSupply_Invalid, 0, 0, 0, 0, NV_FALSE, NV_FALSE, NV_FALSE, NULL,
      },
      {
           Tps6236xPmuSupply_VDD, CardhuPowerRails_VDD_CORE, 0, 0, 1100,
           NV_TRUE, NV_TRUE, NV_FALSE, &s_tps6236x_bdata,
      },
};


static PmuChipDriverOps s_tps6236xOps = {
    Tps6236xPmuGetCaps, Tps6236xPmuGetVoltage, Tps6236xPmuSetVoltage,
};

void *CardhuPmu_GetTps6236xMap(NvOdmPmuDeviceHandle hDevice,
    PmuChipDriverOpsHandle *hDriverOps, NvU32 *pRailMap,
    NvOdmBoardInfo *pProcBoard, NvOdmBoardInfo *pPmuBoard)
{
    NvOdmPmuDriverInfoHandle hPmuDriver = NULL;
    Tps6236xChipInfo CInfo;
    int vsels;

    CInfo.ChipId = Tps6236xChipId_TPS62361B;

    if (pProcBoard->BoardID == PROC_BOARD_E1198)
    {
        vsels = (((pProcBoard->SKU >> 1) & 0x2) | (pProcBoard->SKU & 0x1));
        switch(vsels) {
        case 1:
            switch(pProcBoard->Fab) {
            case BOARD_FAB_A00:
            case BOARD_FAB_A01:
            case BOARD_FAB_A02:
                CInfo.ChipId = Tps6236xChipId_TPS62361B;
                break;
            case BOARD_FAB_A03:
                CInfo.ChipId = Tps6236xChipId_TPS62365;
                break;
            default:
                NvOdmOsPrintf("%s(): Unexpected board fab: %d\n", __func__, pProcBoard->Fab);
                return NULL;
            }
            // VoltageMode already set
            break;
        case 2:
            CInfo.ChipId = Tps6236xChipId_TPS62366A;
            s_tps6236x_bdata.VoltageMode = Tps6236xMode_0;
            break;
        case 3:
            NvOdmOsPrintf("%s(): No driver support for E1198 with this SKU\n", __func__);
            return NULL;
        default:
            NvOdmOsPrintf("%s(): Illegal E1198 SKU: could not set voltage mode\n", __func__);
            return NULL;
        }
    }

    hPmuDriver = Tps6236xPmuDriverOpen(hDevice, &CInfo, s_tps6236x_rail_props,
                                Tps6236xPmuSupply_Num);
    if (!hPmuDriver)
    {
        NvOdmOsPrintf("%s(): The Tps6236xPmuDriverOpen() fails\n", __func__);
        return NULL;
    }

    pRailMap[CardhuPowerRails_VDD_CORE] = Tps6236xPmuSupply_VDD;

    *hDriverOps = &s_tps6236xOps;
    return hPmuDriver;
}
