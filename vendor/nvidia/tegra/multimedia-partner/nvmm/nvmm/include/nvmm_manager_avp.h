/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_nvmm_manager_avp_H
#define INCLUDED_nvmm_manager_avp_H

#if defined(__cplusplus)
extern "C"
{
#endif

#include "nvrm_memmgr.h"
#include "nvrm_init.h"

/* Unique id to register with NVRM for nvmm manager function library */
static const NvU32 NvmmManagerFxnTableId = 0x6e766d6d;  // 'nvmm'

/* Nvmm manager handle */
typedef struct NvmmManagerRec *NvmmManagerHandle;

/* Nvmm manager call back reasons */
typedef enum
{
    NvmmManagerCallBack_AbnormalTerm = 1,
    NvmmManagerCallBack_Force32 = 0x7FFFFFFF

} NvmmManagerCallBackReason;

/* Nvmm manager client types */
typedef enum
{
    NvmmManagerClientType_NvmmBlock = 1,
    NvmmManagerClientType_Force32 = 0x7FFFFFFF

} NvmmManagerClientType;

/* Call back signature to register with Nvmm manager */
typedef NvError (*NvmmManagerClientCallBack) (NvmmManagerCallBackReason eReason, void *Args, NvU32 ArgsSize);

/* Nvmm manager function library */
typedef struct NvmmManagerFxnTableRec
{
    NvError (*NvmmManagerRegisterBlock)(NvMMBlockHandle hBlock, 
                                        NvmmManagerClientCallBack pfBlockCallBack,
                                        void *Args, 
                                        NvU32 ArgsSize,
                                        void *hUniqueHandle);

    NvError (*NvmmManagerUnregisterBlock)(NvMMBlockHandle hBlock, 
                                          void *hUniqueHandle);

} NvmmManagerFxnTable;

/** @} */

#if defined(__cplusplus)
}
#endif

#endif
