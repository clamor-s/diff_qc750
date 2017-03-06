#ifndef __HGMALLOCDEFS_H
#define __HGMALLOCDEFS_H
/*------------------------------------------------------------------------
 * 
 * HG allocation functionality
 * ---------------------
 *
 * (C) 2003-2004 Hybrid Graphics, Ltd.
 * All Rights Reserved.
 *
 * This file consists of unpublished, proprietary source code of
 * Hybrid Graphics, and is considered Confidential Information for
 * purposes of non-disclosure agreement. Disclosure outside the terms
 * outlined in signed agreement may result in irreparable harm to
 * Hybrid Graphics and legal action against the party in breach.
 *
 * $Id: //hybrid/libs/malloc/main/include/hgMallocDefs.h#3 $
 * $Date: 2006/01/04 $
 * $Author: sampo $ *
 *
 *//**
 * \file	
 * \brief	Common defines for hgMalloc/hgDebugMalloc
 * \author	sampo@hybrid.fi
 *//*-------------------------------------------------------------------*/

#ifndef __HGDEFS_H
#	include "hgdefs.h"
#endif

#define HG_MALLOC_VERSION_MAJOR 0
#define HG_MALLOC_VERSION_MINOR 0
#define HG_MALLOC_VERSION_PATCH 0

/* Used for stringifying version numbers: */ 
#define xstr(s) str(s) 
#define str(s) #s 

/*-------------------------------------------------------------------*//*!
 * \brief	Return HGMalloc's version number as a C string
 *//*-------------------------------------------------------------------*/

HG_INLINE const char* hgMallocGetVersion(void)
{
	const char* const s = 
		xstr(HG_MALLOC_VERSION_MAJOR) "." 
		xstr(HG_MALLOC_VERSION_MINOR) "." 
		xstr(HG_MALLOC_VERSION_PATCH); 
	return s; 
}

#endif
