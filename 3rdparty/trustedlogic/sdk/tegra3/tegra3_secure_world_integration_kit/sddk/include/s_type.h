/*
 * Copyright (c) 2001-2009 Trusted Logic S.A.
 * All Rights Reserved.
 *
 * This software is the confidential and proprietary information of
 * Trusted Logic S.A. ("Confidential Information"). You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered
 * into with Trusted Logic S.A.
 *
 * TRUSTED LOGIC S.A. MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. TRUSTED LOGIC S.A. SHALL
 * NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
 * MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

/**
 * Definition of the machine-specific integer types
 **/
#ifndef __S_TYPE_H__
#define __S_TYPE_H__

/* C99 integer types */
#if (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L) &&(!defined(ANDROID))

#include <limits.h>

/* Figure out if a 64-bit integer types is available */
#if \
    defined(_MSC_VER) || \
    defined(__SYMBIAN32__) || \
    defined(_WIN32_WCE) || \
    (defined(ULLONG_MAX) && ULLONG_MAX == 0xFFFFFFFFFFFFFFFFULL) || \
    (defined(ULONG_LONG_MAX) && ULONG_LONG_MAX == 0xFFFFFFFFFFFFFFFFULL)
typedef unsigned long long uint64_t;
typedef long long int64_t;
#else
#define __S_TYPE_INT64_UNDEFINED
#endif

#if UINT_MAX == 0xFFFFFFFF
typedef unsigned int uint32_t;
typedef int int32_t;
#elif ULONG_MAX == 0xFFFFFFFF
typedef unsigned long uint32_t;
typedef long int32_t;
#else
#error This compiler is not supported.
#endif

#if USHRT_MAX == 0xFFFF
typedef unsigned short uint16_t;
typedef short  int16_t;
#else
#error This compiler is not supported.
#endif

#if UCHAR_MAX == 0xFF
typedef unsigned char uint8_t;
typedef signed char   int8_t;
#else
#error This compiler is not supported.
#endif

#if !defined(__cplusplus)
typedef unsigned char bool;
#define false ( (bool)0 )
#define true  ( (bool)1 )
#endif

#else  /* !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L */

#include <stdbool.h>
#include <stdint.h>

#endif  /* !(!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L) */

#include <stddef.h>

#ifndef NULL
#  ifdef __cplusplus
#     define NULL  0
#  else
#     define NULL  ((void *)0)
#  endif
#endif

#define IN
#define OUT

/*
 * Definition of other common types
 */

typedef uint32_t S_RESULT;
typedef S_RESULT TEEC_Result;
typedef S_RESULT SM_ERROR;

typedef uint32_t S_HANDLE;
typedef S_HANDLE SM_HANDLE;
#define S_HANDLE_NULL ((S_HANDLE)0)
#define SM_HANDLE_INVALID  S_HANDLE_NULL

/** Definition of an UUID (from RFC 4122 http://www.ietf.org/rfc/rfc4122.txt) */
typedef struct S_UUID
{
   uint32_t time_low;
   uint16_t time_mid;
   uint16_t time_hi_and_version;
   uint8_t clock_seq_and_node[8];
}S_UUID;
typedef S_UUID TEEC_UUID;
typedef S_UUID SM_UUID;

/* DLL Import/Export directives */

#if defined(WIN32) || defined(__ARMCC_VERSION) || defined(__WINSCW__) || defined(_WIN32_WCE)
#  define S_DLL_EXPORT __declspec(dllexport)
#  define S_DLL_IMPORT __declspec(dllimport)
#  define S_NO_RETURN  __declspec(noreturn)
#elif defined(__GNUC__)
#  define S_DLL_EXPORT __attribute__ ((visibility ("default")))
#  define S_DLL_IMPORT __attribute__ ((visibility ("default")))
#  define S_NO_RETURN  __attribute__ ((noreturn))
#else
#  define S_DLL_EXPORT
#  define S_DLL_IMPORT
#  define S_NO_RETURN
#endif

#if defined(_MSC_VER)
#define __func__ __FUNCTION__
#endif

#endif /* __S_TYPE_H__ */

