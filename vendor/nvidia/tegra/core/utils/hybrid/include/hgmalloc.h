#ifndef __HGMALLOC_H
#define __HGMALLOC_H
/*------------------------------------------------------------------------
 * 
 * HG malloc replacement
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
 * $Id: //hybrid/libs/malloc/main/include/hgMalloc.h#3 $
 * $Date: 2006/01/04 $
 * $Author: sampo $ *
 *
 *//**
 * \file	
 * \brief	Public interface for \c HGMalloc package
 * \author	wili@hybrid.fi
 *//*-------------------------------------------------------------------*/

#if !defined (__HGDEFS_H)
#	include "hgdefs.h"
#endif

#if !defined (__HGMALLOCDEFS_H)
#	include "hgmallocdefs.h"
#endif

/*------------------------------------------------------------------------
 * Callback functions provided by the user
 *----------------------------------------------------------------------*/

typedef void* (*HGMemoryAllocFunc)(size_t bytes);
typedef void* (*HGMemoryReallocFunc)(void* ptr, size_t bytes);
typedef void  (*HGMemoryFreeFunc)(void* ptr);

typedef void* (*HGMemoryHeapAllocFunc)(void* heap, size_t bytes);
typedef void* (*HGMemoryHeapReallocFunc)(void* heap, void* ptr, size_t bytes);
typedef void  (*HGMemoryHeapFreeFunc)(void* heap, void* ptr);

typedef struct HGMemory_s HGMemory;								

/*------------------------------------------------------------------------
 * Memory manager API
 *----------------------------------------------------------------------*/

HG_EXTERN_C_BLOCK_BEGIN 

/*@null@*/ HGMemory*					HGMemory_init				(HGMemoryAllocFunc allocFunc, HGMemoryReallocFunc reallocFunc, HGMemoryFreeFunc freeFunc);		/* Init memory manager		*/
/*@null@*/ HGMemory*					HGMemory_initWithUserHeap	(void* userHeap, HGMemoryHeapAllocFunc heapAllocFunc, HGMemoryHeapReallocFunc heapReallocFunc, HGMemoryHeapFreeFunc heapFreeFunc);	/* Init memory manager with an external heap object. */
HGbool									HGMemory_exit				(HGMemory* HG_RESTRICT pMemoryManager);							/* Destroy memory manager (can be used for region deallocation)	*/
/*@null@*/ void* HG_ATTRIBUTE_MALLOC	HGMemory_malloc				(HGMemory* HG_RESTRICT pMemoryManager, size_t bytes);				/* Allocate memory			*/
/*@null@*/ void*						HGMemory_realloc			(HGMemory* HG_RESTRICT pMemoryManager, /*@null@*/ void* ptr, size_t bytes);	/* Reallocate memory		*/
void									HGMemory_free				(HGMemory* HG_RESTRICT pMemoryManager, /*@null@*/ void* ptr);		/* Free memory				*/
size_t									HGMemory_msize				(/*@null@*/ const void* ptr);								/* Return size of allocated memory block							*/
size_t									HGMemory_memUsed			(const HGMemory* HG_RESTRICT pMemoryManager);						/* Number of bytes of memory used by the user allocs				*/
size_t									HGMemory_memReserved		(const HGMemory* HG_RESTRICT pMemoryManager);						/* Number of bytes of memory reserved from the OS by the allocator	*/
HGbool									HGMemory_isEmpty			(const HGMemory* HG_RESTRICT pMemoryManager);						/* Is the number of allocations exactly zero?						*/
void									HGMemory_checkConsistency	(const HGMemory* HG_RESTRICT pMemoryManager);						/* Consistency checking (debug build)								*/

HG_EXTERN_C_BLOCK_END
/*----------------------------------------------------------------------*/
#endif /* __HGMALLOC_H */

