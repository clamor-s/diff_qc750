#ifndef __HGDEBUGMALLOC_H
#define __HGDEBUGMALLOC_H
/*------------------------------------------------------------------------
 * 
 * HG debug heap
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
 * $Id: //hybrid/libs/malloc/main/include/hgDebugMalloc.h#4 $
 * $Date: 2007/01/16 $
 * $Author: antti $ *
 *
 *//**
 * \file	
 * \brief	Public interface for \c HGDebugMalloc package
 * \author	wili@hybrid.fi
 *//*-------------------------------------------------------------------*/

#if !defined (__HGPUBLICDEFS_H)
#	include "hgpublicdefs.h"
#endif

#if !defined (__HGMALLOCDEFS_H)
#	include "hgmallocdefs.h"
#endif

/*------------------------------------------------------------------------
 * Callback functions provided by the user
 *----------------------------------------------------------------------*/

typedef void* (*HGDebugMallocAllocFunc)(void* heap, size_t bytes);				
typedef void* (*HGDebugMallocReallocFunc)(void* heap, void* ptr, size_t bytes);	
typedef void  (*HGDebugMallocFreeFunc)(void* heap, void* ptr);					
typedef void  (*HGDebugMallocPrintFunc)(const char* string);

typedef struct HGDebugMalloc_s HGDebugMalloc;	

/*------------------------------------------------------------------------
 * Debug malloc functions
 *----------------------------------------------------------------------*/

HG_EXTERN_C_BLOCK_BEGIN 

HGDebugMalloc*	HGDebugMalloc_init				(void* heap, HGDebugMallocAllocFunc allocFunc, HGDebugMallocReallocFunc reallocFunc, HGDebugMallocFreeFunc freeFunc, HGDebugMallocPrintFunc printFunc);
void			HGDebugMalloc_exit				(HGDebugMalloc* pMemory);							
void*			HGDebugMalloc_malloc			(HGDebugMalloc* pMemory, size_t bytes, const char* allocationName, const char* fileName, HGint32 lineNumber);				
void*			HGDebugMalloc_realloc			(HGDebugMalloc* pMemory, void* ptr, size_t bytes);	
void			HGDebugMalloc_free				(HGDebugMalloc* pMemory, void* ptr);				
void			HGDebugMalloc_dump				(const HGDebugMalloc* pMemory);
void			HGDebugMalloc_check				(const HGDebugMalloc* pMemory);
void			HGDebugMalloc_checkPointer		(const HGDebugMalloc* pMemory, const void* ptr);
void			HGDebugMalloc_getPointerInfo	(const HGDebugMalloc* pMemory, const void* ptr, HGint32* size, const char** name, const char** file, HGint32* line);
HGint32			HGDebugMalloc_allocationCount	(const HGDebugMalloc* pMemory);
HGint32			HGDebugMalloc_allocationInfo	(const HGDebugMalloc* pMemory, HGint32* sizes, HGint32* lines, HGint32 len);
HGint32			HGDebugMalloc_fullAllocInfo		(const HGDebugMalloc* pMemory, HGint32* sizes, const char** names, const char** files, HGint32* lines, HGint32 len);
HGint32			HGDebugMalloc_maxAllocationCount(const HGDebugMalloc* pMemory);
size_t			HGDebugMalloc_memUsed			(const HGDebugMalloc* pMemory);
size_t			HGDebugMalloc_maxMemUsed		(const HGDebugMalloc* pMemory);

			
HG_EXTERN_C_BLOCK_END

/*----------------------------------------------------------------------*/
#endif /* __HGDEBUGMALLOC_H */

