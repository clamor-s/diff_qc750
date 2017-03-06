/*------------------------------------------------------------------------
 * 
 * HG debug heap
 * -------------
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
 * $Id: //hybrid/libs/malloc/main/src/hgDebugMalloc.c#9 $
 * $Date: 2007/02/09 $
 * $Author: tero $ *
 *
 *//**
 * \file
 * \brief       \c Debug heap for HG
 * \author      wili@hybrid.fi
 *
 *//*-------------------------------------------------------------------*/

#include "hgdebugmalloc.h"
#include "hgdefs.h"
#include "hgint32.h"

#include <stdio.h>              /* for sprintf()        */

#if (HG_COMPILER == HG_COMPILER_RVCT)
#   include "nvos.h"
#   define MEMCPY(source1, source2, size) NvOsMemcpy(source1, source2, size)
#   define MEMSET(source, value, size) NvOsMemset(source, value, size)
#else
#   include <string.h>
#   define MEMCPY(source1, source2, size) memcpy(source1, source2, size)
#   define MEMSET(source, value, size) memset(source, value, size)
#endif

typedef struct Allocation_s     Allocation;

#define MEMORY_INIT_PATTERN 0xdf                                        /*!< Pattern used for filling fresh memory allocations  */
#define MEMORY_FREE_PATTERN 0xe1                                        /*!< Pattern used for filling freed memory allocations  */

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Header for each allocation
 * \note        Need to keep the header 16-byte aligned
 *                      so that we don't break alignment requirements of the
 *                      application.
 *//*-------------------------------------------------------------------*/

struct Allocation_s
{
        
        HGDebugMalloc*                          m_pMemoryManager;       /*!< Pointer back to memory manager                                             */
        Allocation*                                     m_pPrev;                        /*!< Previous allocation in linked list                                 */
        Allocation*                                     m_pNext;                        /*!< Next allocation in linked list                                             */
        const char*                                     m_pName;                        /*!< Name of the allocation (or NULL if not defined)    */
        const char*                                     m_pFileName;            /*!< File where the allocation was made                                 */
        size_t                                          m_size;                         /*!< Size of the original allocation in bytes                   */
    HGint32                                             m_lineNumber;           /*!< Line where the allocation was made                                 */
        HGuint32                                        m_magic;                        /*!< Magic number for identifying correct allocations / trapping overwrites     */

#if (HG_PTR_SIZE == 8)
        HGuint8                                         m_padding[16-sizeof(size_t)];           /*!< Padding for native 64-bit systems                                  */
#endif

};

HG_CT_ASSERT ((sizeof(Allocation) & 15UL) == 0);        /* need to keep aligned by 16 */

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Debug heap
 *//*-------------------------------------------------------------------*/

struct HGDebugMalloc_s
{
        Allocation*                                     m_firstAllocation;      /*!< Doubly-linked list of allocations                                          */
        void*                                           m_heap;                         /*!< Underlying heap used for actual allocations                        */
        HGDebugMallocAllocFunc          m_allocFunc;            /*!< Function used for making memory allocations                        */
        HGDebugMallocReallocFunc        m_reallocFunc;          /*!< Function used for making memory reallocations                      */
        HGDebugMallocFreeFunc           m_freeFunc;                     /*!< Function used for making memory releases                           */
        HGDebugMallocPrintFunc          m_printFunc;            /*!< Function used for printing text                                            */
        size_t                                          m_maxHeapUse;           /*!< Maximum size the heap has ever been (bytes)                        */
        size_t                                          m_heapUse;                      /*!< Current heap use (bytes)                                                           */
        HGint32                                         m_maxAllocations;       /*!< Largest number of simultaneous allocs there has been       */
        HGint32                                         m_numAllocations;       /*!< Number of allocations                                                                      */
};

/*-------------------------------------------------------------------*//*!
 * \brief   Creates a Debug Memory manager
 * \param       heap            Pointer to underlying memory manager class (may be \c NULL)
 * \param       allocFunc       Function used for performing memory allocations
 * \param       reallocFunc     Function used for performing memory reallocations
 * \param       freeFunc        Function used for releasing memory
 * \param       printFunc       Function used for printing (may be \c NULL)
 * \return      Pointer to Debug memory manager or \c NULL on failure
 * \note        The function fails if one of the allocation functions
 *                      is \c NULL or there is not enough memory to create the
 *                      Debug memory manager
 *//*-------------------------------------------------------------------*/

HGDebugMalloc* HGDebugMalloc_init (
        void*                                           heap, 
        HGDebugMallocAllocFunc          allocFunc, 
        HGDebugMallocReallocFunc        reallocFunc, 
        HGDebugMallocFreeFunc           freeFunc, 
        HGDebugMallocPrintFunc          printFunc)
{
        HGDebugMalloc*  pMemory;

        /* note thap "heap" and "printFunc" can be NULL */
        if (!allocFunc || !reallocFunc || !freeFunc)
                return HG_NULL;

        pMemory = (HGDebugMalloc*)allocFunc (heap, sizeof(HGDebugMalloc));
        if (!pMemory)
                return HG_NULL;

        MEMSET (pMemory, 0, (int)sizeof(HGDebugMalloc));

        pMemory->m_heap                         = heap;
        pMemory->m_allocFunc            = allocFunc;
        pMemory->m_reallocFunc          = reallocFunc;
        pMemory->m_freeFunc                     = freeFunc;
        pMemory->m_printFunc            = printFunc;

        return pMemory;
}

/*-------------------------------------------------------------------*//*!
 * \brief   Destroys a memory manager
 * \param       pMemory Pointer to Debug memory manager
 * \note        All pending memory allocations are released here.
 *//*-------------------------------------------------------------------*/

void HGDebugMalloc_exit (
        HGDebugMalloc* pMemory)
{
        HG_ASSERT (pMemory);

        if (pMemory)
        {
                HGDebugMalloc_check (pMemory);                                          /* run a consistency check */

                /* Leak detection */
                if (pMemory->m_firstAllocation && pMemory->m_printFunc)
                {
                        pMemory->m_printFunc ("HGDebugMalloc_exit(): leaks detected! Call HGDebugMalloc_dump() for info!\n");
                }

                /* Release remaing (leaked) memory allocations */
                while (pMemory->m_firstAllocation)
                {
                        HGDebugMalloc_free (pMemory, (void*)(pMemory->m_firstAllocation + 1));  /* release the memory */
                }

                HGDebugMalloc_check (pMemory);

                /* release the memory manager */
                pMemory->m_freeFunc (pMemory->m_heap, pMemory);
        }
}

/*-------------------------------------------------------------------*//*!
 * \brief   Performs a memory allocation
 * \param       pMemory                 Pointer to Debug memory manager
 * \param       bytes                   Number of bytes of memory to allocate
 * \param       allocationName  String containing name of allocation
 * \param       fileName                String containing file name where allocation is made (usually \c __FILE__)
 * \param       lineNumber              Integer containing line number where allocation is made (usually \c __LINE__)
 * \return      Pointer to allocated memory or \c NULL on failure
 * \note        All of the debugging strings passed in can be NULL. Note that
 *                      *NO* copies of the strings are made internally, i.e., the user
 *                      is responsible for releasing them -- however, they cannot
 *                      be released as long as the memory allocation is alive. Therefore
 *                      one should only pass constant strings.
 * \note        The allocated memory is initialized with the byte pattern dfdfdfdf...
 *//*-------------------------------------------------------------------*/

void* HGDebugMalloc_malloc (
        HGDebugMalloc*  pMemory, 
        size_t                  bytes, 
        const char*             allocationName, 
        const char*             fileName, 
        HGint32                 lineNumber)
{
        if (!pMemory)
        {
                HG_ASSERT (!"HGDebugMalloc_malloc() - NULL pMemory!");
                return HG_NULL;
        } else
        {
                size_t          allocSize   = (sizeof(Allocation) + bytes + 4 * sizeof(HGuint8));
                Allocation* pAllocation = (Allocation*)pMemory->m_allocFunc(pMemory->m_heap, allocSize);
                HGuint8*        pData;

                if (!pAllocation)
                        return HG_NULL;

                pAllocation->m_pMemoryManager = pMemory;
                pAllocation->m_pName              = allocationName;
                pAllocation->m_pFileName      = fileName;
                pAllocation->m_lineNumber         = lineNumber;
                pAllocation->m_magic              = 0xdeadbabeu ^ ((HGuint32)((HGuintptr)pAllocation));
                pAllocation->m_size           = bytes;

                pAllocation->m_pPrev          = HG_NULL;
                pAllocation->m_pNext              = pMemory->m_firstAllocation;
                if (pMemory->m_firstAllocation)
                        pMemory->m_firstAllocation->m_pPrev = pAllocation;
                pMemory->m_firstAllocation    = pAllocation;


                if (++pMemory->m_numAllocations > pMemory->m_maxAllocations)
                        pMemory->m_maxAllocations = pMemory->m_numAllocations;

                pMemory->m_heapUse += bytes;
                if (pMemory->m_heapUse > pMemory->m_maxHeapUse)
                        pMemory->m_maxHeapUse = pMemory->m_heapUse;

                pData = (HGuint8*)(pAllocation + 1);

                if (bytes)
                        MEMSET (pData, MEMORY_INIT_PATTERN , (size_t)bytes);    
                
                /* init rear guard. Note that we purposefully write the data a byte at a time */
                pData[bytes+0] = 0xde;
                pData[bytes+1] = 0xad;
                pData[bytes+2] = 0xba;
                pData[bytes+3] = 0xbe;

                return (void*)(pData);                          /* skip header */
        }
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief               Checks that an allocation is ok
 * \param               pMemory         Pointer to Debug memory manager
 * \param               pAllocation     Pointer to allocation
 * \note                Only performed in debug build
 *//*-------------------------------------------------------------------*/

static void checkAllocation (
        const HGDebugMalloc*    pMemory,
        const Allocation*               pAllocation)
{
        const HGuint8* pRear = (const HGuint8*)(pAllocation + 1) + (pAllocation->m_size);

        HG_UNREF (pMemory);
        HG_UNREF (pRear);

        HG_ASSERT (pAllocation->m_magic == ((HGuint32)((HGuintptr)pAllocation) ^ 0xdeadbabe) && "checkAllocation(): double release or invalid pointer!");
        HG_ASSERT (pAllocation->m_pMemoryManager == pMemory && "checkAllocation(): wrong memory manager (overwrite?)");
        HG_ASSERT (pRear[0] == 0xde && "checkAllocation(): memory overwrite (rear guard)");
        HG_ASSERT (pRear[1] == 0xad && "checkAllocation(): memory overwrite (rear guard)");
        HG_ASSERT (pRear[2] == 0xba && "checkAllocation(): memory overwrite (rear guard)");
        HG_ASSERT (pRear[3] == 0xbe && "checkAllocation(): memory overwrite (rear guard)");
        HG_ASSERT (pMemory->m_numAllocations > 0);
        HG_ASSERT (pMemory->m_heapUse        >= pAllocation->m_size);

        if (pAllocation->m_pNext)
                HG_ASSERT (pAllocation->m_pNext->m_pPrev == pAllocation);
        
        if (pAllocation->m_pPrev)
                HG_ASSERT (pAllocation->m_pPrev->m_pNext == pAllocation);
        else
                HG_ASSERT (pAllocation == pMemory->m_firstAllocation);
}

/*-------------------------------------------------------------------*//*!
 * \brief               Checks that an allocation is ok
 * \param               pMemory         Pointer to Debug memory manager
 * \param               ptr                     Pointer to memory block
 * \note                Check only performed in debug build.
 *//*-------------------------------------------------------------------*/

void HGDebugMalloc_checkPointer (
        const HGDebugMalloc*    pMemory,
        const void*                             ptr)
{
        if (ptr)
        {
                checkAllocation (pMemory, ((const Allocation*)(ptr)) - 1);
        }
}

/*-------------------------------------------------------------------*//*!
 * \brief   Releases a block of memory
 * \param       pMemory         Pointer to debug memory manager
 * \param       ptr                     Pointer to a block of memory to be released (may be \c NULL)
 * \note        The function detects if the allocation has been made with
 *                      the debug memory manager (i.e., traps invalid releases). It
 *                      also performs some elementary overwrite checking.
 * \note        The released memory is filled with the byte pattern e1e1e1e1...
 *//*-------------------------------------------------------------------*/

void HGDebugMalloc_free (
        HGDebugMalloc*  pMemory, 
        void*                   ptr)
{
        HG_ASSERT (pMemory);

        if (!ptr || !pMemory)
                return;
        else
        {
                Allocation* pAllocation = ((Allocation*)(ptr)) - 1;             /* get the header */

                checkAllocation (pMemory, pAllocation);

                if (pAllocation->m_pPrev)
                {
                        HG_ASSERT (pMemory->m_firstAllocation != pAllocation);
                        pAllocation->m_pPrev->m_pNext = pAllocation->m_pNext;
                }
                else
                {
                        HG_ASSERT (pMemory->m_firstAllocation == pAllocation);
                        pMemory->m_firstAllocation = pAllocation->m_pNext;
                }

                if (pAllocation->m_pNext)
                        pAllocation->m_pNext->m_pPrev = pAllocation->m_pPrev;

                pMemory->m_numAllocations--;
                pMemory->m_heapUse -= pAllocation->m_size;

                MEMSET (pAllocation, MEMORY_FREE_PATTERN, (size_t)(sizeof(Allocation) + pAllocation->m_size + 4 * sizeof(HGuint8)));
        
                /* release the memory */
                HG_ASSERT (pMemory->m_freeFunc);
                pMemory->m_freeFunc (pMemory->m_heap, pAllocation);
        }
}

/*-------------------------------------------------------------------*//*!
 * \brief   Performs a consistency check of the heap
 * \param       pMemory         Pointer to debug memory manager
 * \note        The function walks through the entire heap, performing
 *                      overwrite and other consistency checks. This may take
 *                      some time (as all active allocations need to be tested).
 *//*-------------------------------------------------------------------*/

void HGDebugMalloc_check        (
        const HGDebugMalloc* pMemory)
{
        HG_ASSERT (pMemory);
        if (pMemory)
        {
                const Allocation*       pAllocation = pMemory->m_firstAllocation;
#ifdef HG_DEBUG
                const Allocation*       prev            = HG_NULL;
#endif
                int                                     numAllocs   = 0;
                size_t                          memUsed     = 0;

                while (pAllocation)                                                     /* walk through the heap                        */
                {
                        checkAllocation (pMemory, pAllocation); /* make sure that allocation is ok      */
                        HG_ASSERT (pAllocation->m_pPrev == prev);

                        numAllocs++;
                        memUsed         += pAllocation->m_size;
#ifdef HG_DEBUG
                        prev            = pAllocation;
#endif
                        pAllocation = pAllocation->m_pNext;
                }

                /* make sure that our numbers match */
                HG_ASSERT (numAllocs == pMemory->m_numAllocations && "HGDebugMalloc_check(): overwrite somewhere?");
                HG_ASSERT (memUsed   == pMemory->m_heapUse        && "HGDebugMalloc_check(): overwrite somewhere?");
        }
}

/*-------------------------------------------------------------------*//*!
 * \brief   Returns number of active allocations
 * \param       pMemory         Pointer to debug memory manager
 * \return      Number of active allocations (0 = heap is empty)
 *//*-------------------------------------------------------------------*/

HGint32 HGDebugMalloc_allocationCount (
        const HGDebugMalloc* pMemory)
{
        HG_ASSERT (pMemory);
        return pMemory->m_numAllocations;
}

/*-------------------------------------------------------------------*//*!
 * \brief   Returns largest number of active allocations in the life time
 *                      of the memory manager
 * \param       pMemory         Pointer to debug memory manager
 * \return      Largeswt number of active allocations
 *//*-------------------------------------------------------------------*/

HGint32 HGDebugMalloc_maxAllocationCount (
        const HGDebugMalloc* pMemory)
{
        HG_ASSERT (pMemory);
        return pMemory->m_maxAllocations;
}

/*-------------------------------------------------------------------*//*!
 * \brief   Returns number of bytes of memory currently allocated
 * \param       pMemory         Pointer to debug memory manager
 * \return      Number of bytes of memory allocated
 * \note        This figure does not take into account the extra memory
 *                      allocations made by the debug memory manager itself.
 *//*-------------------------------------------------------------------*/

size_t HGDebugMalloc_memUsed (
        const HGDebugMalloc* pMemory)
{
        return pMemory->m_heapUse;
}

/*-------------------------------------------------------------------*//*!
 * \brief   Returns maximum memory usage of the heap during its life time
 * \param       pMemory         Pointer to debug memory manager
 * \return      Maximum memory usage of the heap during its life time
 * \note        This figure does not take into account the extra memory
 *                      allocations made by the debug memory manager itself.
 *//*-------------------------------------------------------------------*/

size_t HGDebugMalloc_maxMemUsed (
        const HGDebugMalloc* pMemory)
{
        return pMemory->m_maxHeapUse;
}

/*-------------------------------------------------------------------*//*!
 * \brief   Returns some info of the allocation.
 * \param       pMemory         Pointer to debug memory manager
 * \param   ptr         Pointer to the allocation.
 * \param   size        (out) size of the allocation in bytes
 * \param   name        (out) name of the allocation
 * \param   file        (out) file name of the source file from which this
 *                      allocation was made.
 * \param   line        (out) the source line number in the file from which 
 *                      this allocation was made.
 *//*-------------------------------------------------------------------*/

void HGDebugMalloc_getPointerInfo (const HGDebugMalloc* pMemory, 
                                   const void*          ptr, 
                                   HGint32*             size, 
                                   const char**         name, 
                                   const char**         file, 
                                   HGint32*             line)
{
        if (pMemory)
        {
        const Allocation* pAllocation = ((const Allocation*)(ptr)) - 1;
                        
        if (size)
            *size = pAllocation->m_size;                        
        
        if (line)
            *line = pAllocation->m_lineNumber;
                
        if (name)
            *name = pAllocation->m_pName;

        if (file)
            *file = pAllocation->m_pFileName;
        }
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns some info about the currently active allocations.
 * \return      Number of entries actually written.
 *//*-------------------------------------------------------------------*/
HGint32 HGDebugMalloc_allocationInfo(const HGDebugMalloc*  pMemory,
                                                                         HGint32*                          sizes,
                                                                         HGint32*                          lines,
                                                                         HGint32                           len)

{
        int index = 0;
        HG_ASSERT(pMemory);
        if (pMemory)
        {
                const Allocation*       pAllocation = pMemory->m_firstAllocation;
                while (pAllocation && index < len)
                {
                        if (sizes)
                                sizes[index] = pAllocation->m_size;
                        if (lines)
                                lines[index] = pAllocation->m_lineNumber;
                        index++;
                        pAllocation = pAllocation->m_pNext;
                }               
        }
        return index;
}

/*-------------------------------------------------------------------*//*!
 * \brief       Returns detailed info about the currently active allocations.
 * \return      Number of entries actually written.
 *//*-------------------------------------------------------------------*/
HGint32 HGDebugMalloc_fullAllocInfo(const HGDebugMalloc* pMemory,
                                                                        HGint32*                         sizes,
                                                                        const char**             names,
                                                                        const char**             files,
                                                                        HGint32*                         lines,
                                                                        HGint32                          len)

{
        int index = 0;
        HG_ASSERT(pMemory);
        if (pMemory)
        {
                const Allocation*       pAllocation = pMemory->m_firstAllocation;
                while (pAllocation && index < len)
                {
                        if (sizes)
                                sizes[index] = pAllocation->m_size;
                        if (names)
                                names[index] = pAllocation->m_pName;
                        if (files)
                                files[index] = pAllocation->m_pFileName;
                        if (lines)
                                lines[index] = pAllocation->m_lineNumber;
                        index++;
                        pAllocation = pAllocation->m_pNext;
                }               
        }
        return index;
}


/*-------------------------------------------------------------------*//*!
 * \brief   Shows a detailed listing of all active memory allocations
 * \param       pMemory         Pointer to debug memory manager
 * \note        Print func needs to be set for the HGDebugMalloc instance
 *//*-------------------------------------------------------------------*/

void HGDebugMalloc_dump (
        const HGDebugMalloc* pMemory)
{
        HG_ASSERT (pMemory);

        HGDebugMalloc_check (pMemory);

        if (pMemory && pMemory->m_printFunc)
        {
                const Allocation*       pAllocation = pMemory->m_firstAllocation;

                pMemory->m_printFunc ("Size      Line      File                               Name\n");
                pMemory->m_printFunc ("--------------------------------------------------------------------------------\n");

                while (pAllocation)                                                     /* walk through the heap                        */
                {
                        char string[1000];

                        const char* filename  = pAllocation->m_pFileName;
                        const char* allocName = pAllocation->m_pName;

/*                      MEMSET ((HGuint8*)string, (HGuint8)' ', 384);           /\* fill with spaces    *\/ */

/*: alloc '%s' leaked %i bytes\n", */
/*
                                        allocName ? allocName : "<unnamed>",
                                        pAllocation->m_size);
*/
#                       if(HG_COMPILER==HG_COMPILER_MSC) && (_MSC_VER >= 1400)
                                sprintf_s(string, 1000,"%s:%i: ", filename ? filename : "<unknown>",  pAllocation->m_lineNumber);
#                       else
                                sprintf(string, "%s:%i: ", filename ? filename : "<unknown>",  pAllocation->m_lineNumber);
#                       endif
                        pMemory->m_printFunc(string);

                        if(allocName)
#                       if(HG_COMPILER==HG_COMPILER_MSC) && (_MSC_VER >= 1400)
                                sprintf_s(string, 1000, "named alloc '%s' %lu bytes (%p)\n",
                                                allocName, (unsigned long)pAllocation->m_size, (const void*)(pAllocation+1));
                        else
                                sprintf_s(string, 1000, "%lu bytes (%p)\n", (unsigned long)pAllocation->m_size, (const void*)(pAllocation+1));
#                       else
                                sprintf(string, "named alloc '%s' %lu bytes (%p)\n",
                                                allocName, (unsigned long)pAllocation->m_size, (const void*)(pAllocation+1));
                        else
                                sprintf(string, "%lu bytes (%p)\n", (unsigned long)pAllocation->m_size, (const void*)(pAllocation+1));
#                       endif

                        pMemory->m_printFunc(string);
                        
                        pAllocation = pAllocation->m_pNext;
                }

                /* \todo: print mem use, # of allocs, max memory use, max allocs */

        }
}

void* HGDebugMalloc_realloc (HGDebugMalloc* pMemory, 
                                                         void* ptr, 
                                                         size_t bytes)
{
        /* TODO Proper implementation */
        Allocation* pAllocation = ((Allocation*)(ptr)) - 1;             /* get the header */
        void* new = HG_NULL;

        HG_ASSERT(pMemory);

        if (bytes > 0)
        {
                if (ptr)
                {
                        new = HGDebugMalloc_malloc(pMemory, 
                                                                           bytes, 
                                                                           pAllocation->m_pName,
                                                                           pAllocation->m_pFileName,
                                                                           pAllocation->m_lineNumber);
                        if (new)
                        {
                                MEMCPY(new, ptr, hgMin32(bytes, pAllocation->m_size));
                        }
                }
                else
                {
                        new = HGDebugMalloc_malloc(pMemory,
                                                                           bytes,
                                                                           "Realloc",
                                                                           __FILE__,
                                                                           __LINE__);
                }
        }

        
        if (ptr && (new || bytes == 0))
                HGDebugMalloc_free(pMemory, ptr);
        
        return new;
}


/*----------------------------------------------------------------------*/
