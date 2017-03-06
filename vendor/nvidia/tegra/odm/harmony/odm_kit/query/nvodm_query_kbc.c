/*
 * Copyright (c) 2007 - 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * @brief <b>NVIDIA Driver Development Kit:
 *                  ODM Kbc interface</b>
 *
 */

#include "nvodm_query_kbc.h"
#include "nvodm_query_kbc_gpio_def.h"
#include "nvodm_query_kbc_qwerty_def.h"

static NvU32 RowNumbers[] = {NvOdmKbcGpioPin_KBRow0, NvOdmKbcGpioPin_KBRow1};
static NvU32 ColNumbers[] = {NvOdmKbcGpioPin_KBCol0, NvOdmKbcGpioPin_KBCol0};

void
NvOdmKbcGetParameter(
        NvOdmKbcParameter Param,
        NvU32 SizeOfValue,
        void * pValue)
{
    NvU32 *pTempVar;
    switch (Param)
    {
        case NvOdmKbcParameter_DebounceTime:
            pTempVar = (NvU32 *)pValue;
            *pTempVar = 10;
            break;
        case NvOdmKbcParameter_RepeatCycleTime:
            pTempVar = (NvU32 *)pValue;
            *pTempVar = 100;
            break;
        default:
            break;
    }
}

NvU32 
NvOdmKbcGetKeyCode(
    NvU32 Row, 
    NvU32 Column,
    NvU32 RowCount,
    NvU32 ColumnCount)
{
    NvU32 CodeData;
    if (Row < KBC_QWERTY_FUNCTION_KEY_ROW_BASE)
    {
        CodeData = KBC_QWERTY_NORMAL_KEY_CODE_BASE + ((Row * ColumnCount) + Column);
    }    
    else
    {
        CodeData = KBC_QWERTY_FUNCTION_KEY_CODE_BASE + 
                        (((Row - KBC_QWERTY_FUNCTION_KEY_ROW_BASE) * ColumnCount) + Column);
    }
    return CodeData;
}

NvBool
NvOdmKbcIsSelectKeysWkUpEnabled(
    NvU32 **pRowNumber,
    NvU32 **pColNumber,
    NvU32 *NumOfKeys)
{
    *pRowNumber = &RowNumbers[0];
    *pColNumber = &ColNumbers[0];
    *NumOfKeys = 2;
    return NV_TRUE;
}

