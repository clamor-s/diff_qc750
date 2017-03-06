/*
 * Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef __OEMCRYPTO_STUB_H__
#define __OEMCRYPTO_STUB_H__

#include "s_type.h"

/** Error Codes */
#define OEMCRYPTO_ERROR_NOT_INITIALIZED        0x00000014
#define OEMCRYPTO_ERROR_ILLEGAL_ARGUMENT        0x00000015
#define OEMCRYPTO_ERROR_GENERIC        0x00000016
#define OEMCRYPTO_ERROR_ILLEGAL_STATE        0x00000017
#define OEMCRYPTO_ERROR_OUT_OF_MEMORY        0x00000018
#define OEMCRYPTO_ERROR_SESSION_NOT_OPEN        0x00000019

/** Sepcial value for invalid session handles */
#define OEMCRYPTO_KEY_SESSION_HANDLE_INVALID        0x00000000

/** OEMCRYPTO Session Handle type */
typedef uint32_t OEMCRYPTO_SESSION_HANDLE;

#endif /* __OEMCRYPTO_STUB_H__ */
