/*
 * Copyright (c) 2008 - 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVFLASH_UTIL__PRIV_H
#define INCLUDED_NVFLASH_UTIL__PRIV_H

#include "nvcommon.h"

#if defined(__cplusplus)
extern "C"
{
#endif


typedef struct NvFlashUtilHalRec
{
    NvBool (*MungeBct)( const char *bct_file, const char *tmp_file );
    NvU32 (*GetBctSize)(void);
    NvU32 (*GetBlEntry)(void);
    NvU32 (*GetBlAddress)(void);
    void (*GetDumpBit)(NvU8 * BitBuffer, const char*dumpOption);
    NvU32 devId;
} NvFlashUtilHal;

NvBool NvFlashUtilGetAp20Hal(NvFlashUtilHal *);
NvBool NvFlashUtilGetT30Hal(NvFlashUtilHal *);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef INCLUDED_NVFLASH_UTIL__PRIV_H */

