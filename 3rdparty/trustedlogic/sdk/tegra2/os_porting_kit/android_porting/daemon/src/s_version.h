/*
 * Copyright (c) 2011 Trusted Logic S.A.
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
#ifndef __S_VERSION_H__
#define __S_VERSION_H__

/*
 * Usage: define S_VERSION_BUILD on the compiler's command line.
 *
 * Then set:
 * - S_VERSION_OS
 * - S_VERSION_PLATFORM
 * - S_VERSION_MAIN
 * - S_VERSION_ENG is optional
 * - S_VERSION_PATCH is optional
 * - S_VERSION_BUILD = 0 if S_VERSION_BUILD not defined or empty
 */
#if defined(WIN32)
#define S_VERSION_OS "W"          /* "W" for Windows PC (XP, Vista…) */
#define S_VERSION_PLATFORM "X"    /* "X" for ix86 PC simulators */
#elif defined(ANDROID)
#define S_VERSION_OS "A"          /* "A" for Android */
#define S_VERSION_PLATFORM "A"    /* "A" for Tegra2 */
#elif defined(LINUX)
#define S_VERSION_OS "L"          /* "L" for Linux */
#define S_VERSION_PLATFORM "X"    /* "X" for ix86 PC simulators */
#else
#define S_VERSION_OS "X"          /* "X" for Secure-World */
#define S_VERSION_PLATFORM "A"    /* "A" for Tegra2 */
#endif

/*
 * This version number must be updated for each new release
 */
#define S_VERSION_MAIN  "01.09"
#define S_VERSION_RESOURCE 1,3,0,S_VERSION_BUILD

/*
* If this is a patch or engineering version use the following
* defines to set the version number. Else set these values to 0.
*/
#define S_VERSION_ENG 0
#define S_VERSION_PATCH 0

#ifdef S_VERSION_BUILD
/* TRICK: detect if S_VERSION is defined but empty */
#if 0 == S_VERSION_BUILD-0
#undef  S_VERSION_BUILD
#define S_VERSION_BUILD 0
#endif
#else
/* S_VERSION_BUILD is not defined */
#define S_VERSION_BUILD 0
#endif

#define __STRINGIFY(X) #X
#define __STRINGIFY2(X) __STRINGIFY(X)

#if S_VERSION_ENG != 0
#define _S_VERSION_ENG "e" __STRINGIFY2(S_VERSION_ENG)
#else
#define _S_VERSION_ENG ""
#endif

#if S_VERSION_PATCH != 0
#define _S_VERSION_PATCH "p" __STRINGIFY2(S_VERSION_PATCH)
#else
#define _S_VERSION_PATCH ""
#endif

#if !defined(NDEBUG) || defined(_DEBUG)
#define S_VERSION_VARIANT "D   "
#else
#define S_VERSION_VARIANT "    "
#endif

#define S_VERSION_STRING \
	"TFN" \
	S_VERSION_OS \
	S_VERSION_PLATFORM \
	S_VERSION_MAIN \
	_S_VERSION_ENG \
	_S_VERSION_PATCH \
	"."  __STRINGIFY2(S_VERSION_BUILD) " " \
	S_VERSION_VARIANT

#endif /* __S_VERSION_H__ */
