/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_nvmm_manager_common_H
#define INCLUDED_nvmm_manager_common_H

#if defined(__cplusplus)
extern "C"
{
#endif

#include "nvmm.h"

/* Nvmm manager maximum message size */
#define MSG_BUFFER_SIZE 256

/* Nvmm manager message types */
typedef enum
{
    NvmmManagerMsgType_AbnormalTerm = 1,
    NvmmManagerMsgType_Force32 = 0x7FFFFFFF

} NvmmManagerMsgType;

/* Nvmm manager termination message info */
typedef struct NvmmManagerAbnormalTermMsgRec
{
    NvmmManagerMsgType eMsgType;
    NvMMBlockHandle hBlock;
} NvmmManagerAbnormalTermMsg;

/** @} */

#if defined(__cplusplus)
}
#endif

#endif
