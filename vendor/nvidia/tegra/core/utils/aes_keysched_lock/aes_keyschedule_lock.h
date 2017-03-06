//
// Copyright (c) 2010 NVIDIA Corporation.  All Rights Reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation.  Any
// use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation
// is strictly prohibited.
//


#ifndef AES_KEYSCHEDULE_LOCK_H
#define AES_KEYSCHEDULE_LOCK_H


#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief Disables the key schedule reads on both AES engines.
 * NOTE : This function uses 624 bytes starting 200KB offset in IRAM. Before
 *        calling this function,the caller of this function must therefore skip
 *        or not place any data in this region of IRAM.
 *
 * NOTE : This function uses key slot #6(the 7th key slot) in each security
 *        engine in order to verify if the key schedule reads were indeed disabled
 *        in each security engine. Therefore, the caller of this function should
 *        ensure that it doesn't leave any keys programmed in the 7th key slot of
 *        either security engine before calling this function.
*/

void    NvAesDisableKeyScheduleRead(void);

#if defined(__cplusplus)
}
#endif

#endif // AES_KEYSCHEDULE_LOCK_H
