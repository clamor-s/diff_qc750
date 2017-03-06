/*
 * Copyright (c) 2008 - 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
 
/**
 * @file          Nvodm_Kbc.c
 * @brief         <b>KBC odm implementation</b>
 *
 * @Description : Implementation of the odm KBC API
 */
#include "nvodm_kbc.h"
#include "../../query/nvodm_query_kbc_qwerty_def.h"

NvU32
NvOdmKbcFilterKeys(
    NvU32 *pRows,
    NvU32 *pCols,
    NvU32 NumOfKeysPressed)
{
    NvBool IsFunctionKeyFound = NV_FALSE;
    NvU32 KeyIndex;
    
    for (KeyIndex = 0; KeyIndex < NumOfKeysPressed; ++KeyIndex)
    {
        if ((pRows[KeyIndex] == KBC_QWERTY_FUNCTION_KEY_ROW_NUMBER) &&
                (pCols[KeyIndex] == KBC_QWERTY_FUNCTION_KEY_COLUMN_NUMBER))
        {
            IsFunctionKeyFound = NV_TRUE;
            break;
        }
    }
    if (!IsFunctionKeyFound)
        return NumOfKeysPressed;

    // Add function row base to treat as special case
    for (KeyIndex = 0; KeyIndex < NumOfKeysPressed; ++KeyIndex)
        pRows[KeyIndex] += KBC_QWERTY_FUNCTION_KEY_ROW_BASE;
        
    return NumOfKeysPressed;
}


