/* Copyright (c) 2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */


#ifndef SE_AES_KEYSCHEDULE_LOCK_H
#define SE_AES_KEYSCHEDULE_LOCK_H


#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief Disables the key schedule reads for SE engine
 */

void NvLockSeAesKeySchedule(void);

#if defined(__cplusplus)
}
#endif

#endif
