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
/* Uncomment to get the traces */
/* #define MODULE_NAME  "SMX_HEAP" */

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#include "smx_heap.h"

/**
 * @item Topic
 * @name LIB HEAP Implementation Notes
 *
 * The implementation of this API has been copied from the file ker_mem_.c
 *
 * All kerMem occurences have replaced by SMXHeap.
 * All KER_MEM occurences have replaced by SMX_HEAP.
 */

/**
 * This module parameter check the integrity of the non allocated memory before
 * any allocation.
 */
#undef SMX_HEAP_FULL_DEBUG

#include "s_error.h"

#if defined(LINUX)
#define MD_VAR_NOT_USED(variable)  do{(void)(variable);}while(0);
#else
#define MD_VAR_NOT_USED(variable) (variable);
#endif

/**
 * Aligns a value on a specified boundary.
 *
 * The function returns the smallest value superior or equal to <i>value</i>
 * that is aligned on a <i>align</i>-byte boundary.
 *
 * @syntax  uint32_t mdMemAlignValue(uint32_t value, uint32_t align);
 *
 * @param   value    the value to align.
 *
 * @param   align    the boundary value (i.e. 2, 4, or 8).
 *
 * @return  the aligned value.
 */
#define mdMemAlignValue(value, align) \
         ((uint32_t)((uint32_t)(((uint32_t)(value)) + ((align) - 1)) & (uint32_t)(-((int32_t)(align)))))

/**
 * Adds a scalar offset to a pointer.
 *
 * @syntax  PVOID mdMemAddOffsetToPointer(PVOID pMemory, uint32_t offset)
 *
 * @param   pMemory  a pointer in memory.
 *
 * @param   offset   the offset to add, in bytes.
 *
 * @return  the pointer value shifted of <i>offset</i> bytes.
 */

#define mdMemAddOffsetToPointer(pMemory, offset) \
         ((void*)(((uint8_t*)(pMemory)) + ((uint32_t)(offset))))

/**
 * Removes an offset from a pointer.
 *
 * @syntax  PVOID mdMemSubstractOffsetToPointer(PVOID pMemory, uint32_t offset)
 *
 * @param   pMemory  a pointer in memory.
 *
 * @param   offset   the offset to remove, in bytes.
 *
 * @return   the pointer value shifted down of <i>offset</i> bytes.
 */
#define mdMemSubstractOffsetToPointer(pMemory, offset) \
         ((void*)(((uint8_t*)(pMemory)) - ((uint32_t)(offset))))

#define mdMemAlignPointer(pMemory, align) \
         ((void*)((((uint32_t)(pMemory)) + ((align) - 1)) & (-((int32_t)(align)))))

#define mdMemSubstractPointer(pMemoryA, pMemoryB) \
         ((int32_t)(((uint8_t*)(pMemoryA)) - ((uint8_t*)(pMemoryB))))

#ifdef NDEBUG
/* Compile-out the traces */
#define SMX_TRACE_ERROR(...)
#define SMX_TRACE_INFO(...)
#else
#ifdef ANDROID
#define LOG_TAG "HEAP"
#include <android/log.h>
#define SMX_TRACE_INFO(format, ...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, format, __VA_ARGS__)
#define SMX_TRACE_ERROR(format, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, format, __VA_ARGS__)
#else
static void SMX_TRACE_ERROR(const char* format, ...)
{
   va_list ap;
   va_start(ap, format);
   fprintf(stderr, "TRACE: ERROR: ");
   vfprintf(stderr, format, ap);
   fprintf(stderr, "\n");
   va_end(ap);
}

static void SMX_TRACE_INFO(const char* format, ...)
{
   va_list ap;
   va_start(ap, format);
   fprintf(stderr, "TRACE: ");
   vfprintf(stderr, format, ap);
   fprintf(stderr, "\n");
   va_end(ap);
}
#endif /* ANDROID */
#endif /* NDEBUG */

/* ------------------------------------------------------------------------
 * SMXHeapxxxEx operations (hidden for now - keep them internal)
------------------------------------------------------------------------ */

/**
 * Allocates a block of memory. The address of the allocated block is
 * aligned on a 4-byte boundary.
 *
 *
 * @param   hHeap    Handle to the heap from which the memory will be
 *                   allocated. This handle is returned by the SMXHeapCreate.
 *
 * @param   nSize    the size in bytes of the block to allocate. A zero value is
 *                   invalid.
 *
 * @param   nType    the type of block to allocate. The flags are the following:
 *                    - {SMX_HEAP_ZEROED} to fill the allocated memory with 0.
 *                    - {SMX_HEAP_SAFE} to allocate the memory in the safe part
 *                      of the current memory bank.
 *
 * @return  a pointer on the new block or NULL if the block cannot be allocated.
 **/
void* SMXHeapAllocEx(
      IN  S_HANDLE    hHeap,
      IN  uint32_t           nSize,
      IN  uint8_t           nType);

/**
 * Reallocates the specified block.
 *
 * If <tt>pBlock</tt> is null, the current block is a block of size zero. If
 * <tt>nNewSize</tt> is zero, the block is freed and the function returns
 * <tt>NULL</tt>.
 *
 * If <tt>nNewSize</tt> is less or equal to the current size of the block, the
 * block is trucated, the content of the block is left unchanged and the
 * function returns <tt>pBlock</tt>.
 *
 * If <tt>nNewSize</tt> is greater than the current size of the block, the size
 * of the block is increased. The whole content of the block is copied at the
 * beginning of the new block. If possible, the block is enlarged in place and
 * the function retuns <tt>pBlock</tt>. If this is not possible, a new block
 * is allocated with the new size, the content of the current block is copied,
 * the current block is freed and the function retuns the pointer on the new
 * block.
 *
 * @param   hHeap    Handle to the heap from which the memory will be
 *                   re-allocated. This handle is returned by the SMXHeapCreate.
 *
 * @param   pBlock  The curent block. This value may be null.
 *
 * @param   nNewSize The new size of the block. This value may be zero.
 *
 * @param   nType    The flags are the following:
 *                    - {SMX_HEAP_ZEROED} if this flag is set, the new extra area
 *                      allocated at the top of the block is filled with zeros.
 *
 * @return  The pointer on the resized block or <tt>NULL</tt> if
 *          <tt>nNewSize</tt> is zero or if an error is detected.
 **/
void* SMXHeapReallocEx(
      IN  S_HANDLE    hHeap,
      IN  void*         pBlock,
      IN  uint32_t           nNewSize,
      IN  uint8_t           nType);

/**
 * Frees a block of memory allocated by {SMXHeapAlloc} or {SMXHeapAllocEx}.
 *
 * @param   hHeap    Handle to the heap whose memory block is to be freed.
 *                   This handle is returned by the SMXHeapCreate.
 *
 * @param    pBlock  A pointer on the block to free. This value may be null.
 *
 * @param   nType    the type of operation. The flags are the following:
 *                    - {SMX_HEAP_ZEROED} to fill the block with zeros before
 *                      releasing the memory.
 **/
void SMXHeapFreeEx(
      IN  S_HANDLE    hHeap,
      IN  void*         pBlock,
      IN  uint8_t           nType);

/*-----------------------------------------------------------------------------
 * Debug operations (hidden for now - keep them internal)
 *----------------------------------------------------------------------------*/

#define SMXHeapDebugDump(pMemBase, nType) MD_NULL_FUNCTION
#define KER_ASSERT_VALID_BLOCK(pMemBase, pBlock, nType) MD_NULL_FUNCTION
#define KER_ASSERT_VALID_BUFFER(pMemBase, pBuffer, length, nType) MD_NULL_FUNCTION
#define KER_ASSERT_VALID_POINTER(pMemBase, pPointer, nType) MD_NULL_FUNCTION
#define SMXHeapIsMemorySafe(pMemBase) ( S_SUCCESS )


/* ------------------------------------------------------------------------
                           Internal definitions & structures
------------------------------------------------------------------------ */

/** The safe block flag */
#define  SMX_HEAP_SAFE     0x01

/** The zeroed block flag */
#define  SMX_HEAP_ZEROED   0x02

/** Tag used to identify an allocated block */
#define SMX_HEAP_FLAG_ALLOCATED  ((uint16_t)0x0100)

/** Tag used to identify the last block */
#define SMX_HEAP_FLAG_LAST       ((uint16_t)0x0200)

/** Tag used to identify a chained free block */
#define SMX_HEAP_FLAG_IN_CHAIN   ((uint16_t)0x0800)


/** Internal header of a memory block */
typedef struct SMX_HEAP_HEADER
{
   uint32_t flags;          /* The flags of the block  */
   uint32_t blockSize;      /* The size of the block, in bytes */
} SMX_HEAP_HEADER, * PSMX_HEAP_HEADER;

#define SMX_HEAP_FILL_CHAR    ((uint8_t)0x55)

typedef struct SMX_HEAP_STRUCTURE
{
   void* pRealBase;
   void* pFirstFree;

} SMX_HEAP_STRUCTURE, * PSMX_HEAP_STRUCTURE;

#define SMX_HEAP_FREE_CHAIN_POINTER(pBlockHeader) \
   ((void**)static_SMXHeapHeaderToFreeContent(pBlockHeader))


static __inline S_HANDLE mdConvertPointerToHandle(void* pPointer, uint32_t nScrambler)
{
   return (S_HANDLE)(pPointer == NULL)?
            S_HANDLE_NULL:
            (((uint32_t)pPointer) ^ 0x55555555 ^ (nScrambler << 1));
}

static __inline void* mdConvertHandleToPointer(S_HANDLE hHandle, uint32_t nScrambler)
{
   return (void*)((hHandle == S_HANDLE_NULL)?
            NULL:
            (void*)(hHandle ^ 0x55555555 ^ (nScrambler << 1)));
}


#define  SMXHeapConvertHandleToPointer(H)  mdConvertHandleToPointer(H, 0xDEADBEEF)
#define  SMXHeapConvertPointerToHandle(P)  mdConvertPointerToHandle(P, 0xDEADBEEF)


/* ------------------------------------------------------------------------
                        Internal functions
------------------------------------------------------------------------ */

/**
 * Returns the total block size.
 *
 * @param   pBlockHeader The current block.
 *
 * @return  The total block size in bytes.
 **/
static  uint32_t static_SMXHeapGetBlockTotalSize(PSMX_HEAP_HEADER pBlockHeader)
{
   register uint32_t length;

   length = pBlockHeader->blockSize;
   length = sizeof(SMX_HEAP_HEADER) + mdMemAlignValue(length, 8);

   return length;
}

/* ------------------------------------------------------------------------ */

/**
 * Computes the total size for an allocated block.
 *
 * @param   length   The length, in bytes of the block data.
 *
 * @return  Total size in bytes of the block.
 **/
static  uint32_t static_SMXHeapComputeUsedTotalSize(uint32_t length)
{
   return (sizeof(SMX_HEAP_HEADER) + mdMemAlignValue(length, 8));
}

/* ------------------------------------------------------------------------ */

/**
 * Returns the next block.
 *
 * @param   pBlockHeader The current block.
 *
 * @return  The next block or NULL if the current block is the last block.
 **/
static  PSMX_HEAP_HEADER static_SMXHeapGetNextBlock(
      PSMX_HEAP_STRUCTURE pMemoryBase,
      PSMX_HEAP_HEADER pBlockHeader)
{
   return (PSMX_HEAP_HEADER)(pBlockHeader == NULL)?
      mdMemAddOffsetToPointer(pMemoryBase, sizeof(SMX_HEAP_STRUCTURE)):
      (((pBlockHeader->flags & SMX_HEAP_FLAG_LAST) != 0)?
         NULL:
         mdMemAddOffsetToPointer(
            pBlockHeader, static_SMXHeapGetBlockTotalSize(pBlockHeader)));
}

/* ------------------------------------------------------------------------ */

/**
 * Converts a pointer on a header into a pointer on the block content
 * for chaining purposes
 */
static  void* static_SMXHeapHeaderToFreeContent(PSMX_HEAP_HEADER pBlockHeader)
{
   assert((pBlockHeader->flags & SMX_HEAP_FLAG_ALLOCATED) == 0);
   return mdMemAddOffsetToPointer(pBlockHeader,sizeof(SMX_HEAP_HEADER));
}

/* ------------------------------------------------------------------------ */

/**
 * Converts a pointer on the block content into a pointer on the corresponding
 * block header.
 *
 * @param   pBlockContent  the pointer on the block content.
 *
 * @return  the corresponding pointer on the block header.
 */
static  PSMX_HEAP_HEADER static_SMXHeapContentToHeader(void* pBlockContent)
{
   return (PSMX_HEAP_HEADER)mdMemSubstractOffsetToPointer(
      pBlockContent, sizeof(SMX_HEAP_HEADER));
}

/* ------------------------------------------------------------------------ */

/**
 * Fills a used block. The type of header is set by the flags.
 *
 * @param   pBlockHeader  The pointer on the block to fill.
 *
 * @param   length  The length in bytes of the block content.
 *
 * @param   flags  The flag mask
 *
 * @return  The pointer on the content of the block.
 **/
static  void* static_SMXHeapFillUsedBlock(void* pBlockHeader, uint32_t length, uint16_t flags)
{
   void* pContent;

   /* Then, create the block */
   ((PSMX_HEAP_HEADER)pBlockHeader)->flags = flags;
   ((PSMX_HEAP_HEADER)pBlockHeader)->blockSize = length;

   pContent = mdMemAddOffsetToPointer(pBlockHeader, sizeof(SMX_HEAP_HEADER));

   return pContent;
}

/* ------------------------------------------------------------------------ */

/**
 * Fills a free block
 *
 * @param   pHeader  The pointer on the block to fill.
 *
 * @param   totalSize  The total size in bytes of the free block.
 *
 * @param   flags  The flag mask
 **/
static  void static_SMXHeapFillFreeBlock(void* pHeader, uint32_t totalSize, uint16_t flags)
{
   uint32_t size;

   assert(totalSize == mdMemAlignValue(totalSize, 8));
   assert((flags & SMX_HEAP_FLAG_ALLOCATED) == 0);

   size = totalSize - sizeof(SMX_HEAP_HEADER);
   ((PSMX_HEAP_HEADER)pHeader)->flags = flags;
   ((PSMX_HEAP_HEADER)pHeader)->blockSize = size;

}

/* ------------------------------------------------------------------------ */

/**
 * rebuilds the list of free blocks
 * => merge consecutive free blocks
 */
static void static_SMXHeapRebuildFreeBlockList(PSMX_HEAP_STRUCTURE pMemoryBase)
{
   void** pLastNodeChain = &(pMemoryBase->pFirstFree);
   PSMX_HEAP_HEADER pBlockHeader = NULL;

   while((pBlockHeader = static_SMXHeapGetNextBlock(pMemoryBase, pBlockHeader)) != NULL)
   {
      if((pBlockHeader->flags & SMX_HEAP_FLAG_ALLOCATED) == 0)
      {
         uint32_t totalSize = static_SMXHeapGetBlockTotalSize(pBlockHeader);
         bool atEnd=false;
         PSMX_HEAP_HEADER pStartOfFreeBlock=pBlockHeader;
         PSMX_HEAP_HEADER pLastOfFreeBlock=pBlockHeader;
         while(pBlockHeader != NULL)
         {
            pBlockHeader=static_SMXHeapGetNextBlock(pMemoryBase, pBlockHeader);
            if(pBlockHeader==NULL)
            {
               atEnd=true;
               break;
            }
            if((pBlockHeader->flags & SMX_HEAP_FLAG_ALLOCATED) != 0)
            {
               break;
            }
            totalSize+=static_SMXHeapGetBlockTotalSize(pBlockHeader);
            pLastOfFreeBlock=pBlockHeader;
         }
         static_SMXHeapFillFreeBlock(pStartOfFreeBlock,
                                    totalSize,
                                    (uint16_t)(atEnd?SMX_HEAP_FLAG_LAST:0));

         /* content set to next free block reference */
         if(pStartOfFreeBlock->blockSize >= sizeof(void*))
         {
            *pLastNodeChain=(void*)pStartOfFreeBlock;
            /* update chain of new free block */
            *(SMX_HEAP_FREE_CHAIN_POINTER(pStartOfFreeBlock))=
               *SMX_HEAP_FREE_CHAIN_POINTER(pLastOfFreeBlock);
            pLastNodeChain=
               SMX_HEAP_FREE_CHAIN_POINTER(pStartOfFreeBlock);
            pStartOfFreeBlock->flags |= SMX_HEAP_FLAG_IN_CHAIN;
         }
         if(atEnd)
         {
            break;
         }
      }
   }
   *pLastNodeChain=NULL;
   return;
}

/* ------------------------------------------------------------------------
                     Public functions - SMXHeapxxx
------------------------------------------------------------------------- */

S_RESULT SMXHeapInit(
      IN  void*         pBaseAddress,
      IN  uint32_t           nHeapSize,
      OUT S_HANDLE*   phHeap)
{
   PSMX_HEAP_HEADER pFirstBlock;
   void** pFirstBlockContent;
   PSMX_HEAP_STRUCTURE pMemoryBase;

   void* pRealBase = pBaseAddress;
   uint32_t   nTotalSize = nHeapSize;
   uint32_t   nRealSize;

   /*
    * Check parameters.
    */
   if (phHeap == NULL)
   {
      return S_ERROR_BAD_PARAMETERS;
   }

   *phHeap = S_HANDLE_NULL;

   if ((pBaseAddress == NULL) || (nHeapSize < 24))
   {
      return S_ERROR_BAD_PARAMETERS;
   }

   nHeapSize -= 8;  /* reserve the last 8 bytes */

   /* Truncate the size to a multiple of 8 */
   nTotalSize &= 0xFFFFFFF8;

   pMemoryBase = (PSMX_HEAP_STRUCTURE)mdMemAlignPointer(pRealBase, 8);
   if(pMemoryBase != pRealBase)
   {
      nTotalSize -= 8; /* truncate the size for alignment */
   }
   if (nTotalSize < sizeof(SMX_HEAP_STRUCTURE) +
                     sizeof(SMX_HEAP_HEADER) +
                     8)
   {
      SMX_TRACE_ERROR("SMXHeapInit: The memory size is too small (%d bytes)",
                  nTotalSize);
      return S_ERROR_GENERIC;
   }

   pMemoryBase->pRealBase = pRealBase;

   nRealSize = nTotalSize - sizeof(SMX_HEAP_STRUCTURE);
   nRealSize &= 0xFFFFFFF8;

   pFirstBlock =
      (PSMX_HEAP_HEADER)mdMemAddOffsetToPointer(pMemoryBase,
                                                sizeof(SMX_HEAP_STRUCTURE));

   /* first block is always at least 4 bytes long, we can chain it */
   static_SMXHeapFillFreeBlock(
      pFirstBlock,
      nRealSize, SMX_HEAP_FLAG_LAST | SMX_HEAP_FLAG_IN_CHAIN);

   pMemoryBase->pFirstFree=pFirstBlock;

   pFirstBlockContent=SMX_HEAP_FREE_CHAIN_POINTER(pFirstBlock);
   /* no next free block */
   *pFirstBlockContent=NULL;

   *phHeap = SMXHeapConvertPointerToHandle(pMemoryBase);

   SMX_TRACE_INFO("SMXHeapInit[0x%x]: Heap Size of 0x%x bytes",
               *phHeap, nTotalSize);

#ifdef SMX_HEAP_TEST_MANAGER
   static_SMXHeapRunTests(*phHeap);
#endif

   return S_SUCCESS;
}

/* ------------------------------------------------------------------------ */

S_RESULT SMXHeapUninit(
      IN  S_HANDLE    hHeap)
{
   S_RESULT       nError = S_SUCCESS;
   PSMX_HEAP_STRUCTURE  pMemoryBase = (PSMX_HEAP_STRUCTURE)
                                       SMXHeapConvertHandleToPointer(hHeap);
   PSMX_HEAP_HEADER pBlockHeader = NULL;

   if (pMemoryBase == NULL)
   {
      SMX_TRACE_ERROR("SMXHeapUninit[0x%x]: Invalid handle",
                  hHeap);
      return S_ERROR_BAD_PARAMETERS;
   }

   assert(pMemoryBase->pRealBase != NULL);
   /* invalidate heap structure => no more alloc possible */
   pBlockHeader = static_SMXHeapGetNextBlock(pMemoryBase, pBlockHeader);
   static_SMXHeapFillFreeBlock(
      pBlockHeader,
      0, SMX_HEAP_FLAG_LAST);
   pMemoryBase->pFirstFree=NULL;

   SMX_TRACE_INFO("SMXHeapUninit[0x%x]: Status=%d",
               hHeap, nError);

   return nError;
}

/* ------------------------------------------------------------------------ */

void* SMXHeapAlloc(
      IN  S_HANDLE    hHeap,
      IN  uint32_t           nSize)
{
   return SMXHeapAllocEx(hHeap, nSize, 0);
}

/* ------------------------------------------------------------------------ */

void* SMXHeapRealloc(
      IN  S_HANDLE    hHeap,
      IN  void*         pBlock,
      IN  uint32_t           nNewSize)
{
   return SMXHeapReallocEx(hHeap, pBlock, nNewSize, 0);
}

/* ------------------------------------------------------------------------ */

void SMXHeapFree(
      IN  S_HANDLE    hHeap,
      IN  void*         pBlock)
{
   SMXHeapFreeEx(hHeap, pBlock, 0);
}

/* ------------------------------------------------------------------------ */

void* SMXHeapAllocEx(
      IN  S_HANDLE    hHeap,
      IN  uint32_t           nSize,
      IN  uint8_t           nType)
{
   PSMX_HEAP_HEADER pBlockHeader, pSelectedBlock, pNextFreeBlock;
   bool bFirstPass = true;
   uint16_t flags = SMX_HEAP_FLAG_ALLOCATED;
   uint32_t neededLength, availableLength;
   void* pContent;
   void** pLastNodeChain;

   PSMX_HEAP_STRUCTURE  pMemoryBase = (PSMX_HEAP_STRUCTURE)
                                       SMXHeapConvertHandleToPointer(hHeap);
   if (pMemoryBase == NULL)
   {
      SMX_TRACE_ERROR("SMXHeapAllocEx[0x%x]: Invalid handle", hHeap);
      return NULL;
   }

#ifdef SMX_HEAP_FULL_DEBUG
   assert(SMXHeapIsMemorySafe(pMemoryBase) == S_SUCCESS);
#endif

   if(nSize == 0)
   {
      return NULL;
   }

   neededLength = static_SMXHeapComputeUsedTotalSize(nSize);

second_pass:

   pLastNodeChain = &(pMemoryBase->pFirstFree);

   assert(pLastNodeChain != NULL);

   pBlockHeader = (PSMX_HEAP_HEADER)(*pLastNodeChain);

   while(pBlockHeader != NULL)
   {
      PSMX_HEAP_HEADER pLastSeenInChain = pBlockHeader;

      availableLength = static_SMXHeapGetBlockTotalSize(pBlockHeader);
      pSelectedBlock = pBlockHeader;

      pNextFreeBlock=
         (PSMX_HEAP_HEADER)(*SMX_HEAP_FREE_CHAIN_POINTER(pBlockHeader));

      /* merge consecutive free block */
      while((pBlockHeader = static_SMXHeapGetNextBlock(pMemoryBase, pBlockHeader)) != NULL)
      {
         if((pBlockHeader->flags & SMX_HEAP_FLAG_ALLOCATED) != 0)
         {
            /* if enough free size, keep this block */
            if (availableLength < neededLength)
            {
               pSelectedBlock = NULL;
            }
            break;
         }

         availableLength += static_SMXHeapGetBlockTotalSize(pBlockHeader);
         /* if consecutive free block , update next free block */
         if(pBlockHeader == pNextFreeBlock)
         {
            pNextFreeBlock = (PSMX_HEAP_HEADER)
               (*SMX_HEAP_FREE_CHAIN_POINTER(pBlockHeader));
            pLastSeenInChain=pBlockHeader;
         }
      }
      if (availableLength < neededLength
         && pBlockHeader == NULL)
      {
         goto block_not_found;
      }
      if(pSelectedBlock != NULL)
      {
         goto block_found;
      }
      else
      {
         pBlockHeader=pNextFreeBlock;
         pLastNodeChain=SMX_HEAP_FREE_CHAIN_POINTER(pLastSeenInChain);
      }
   }

block_not_found:

   if(bFirstPass)
   {
      bFirstPass = false;
      static_SMXHeapRebuildFreeBlockList(pMemoryBase);
      goto second_pass;
   }

   SMX_TRACE_ERROR("SMXHeapAllocEx: Out of memory size=%d",nSize);
   return NULL;

block_found:

   /* First create the new free block, if needed */
   if(availableLength > neededLength + sizeof(SMX_HEAP_HEADER))
   {
      PSMX_HEAP_HEADER pNewFreeBlock = mdMemAddOffsetToPointer(pSelectedBlock,
                                                               neededLength);
      static_SMXHeapFillFreeBlock(
         pNewFreeBlock,
         availableLength - neededLength,
         (pBlockHeader==NULL?SMX_HEAP_FLAG_LAST:0));
      if(pNewFreeBlock->blockSize >= sizeof(void*))
      {
         /* put created free block in free blocks chain */
         pNewFreeBlock->flags |= SMX_HEAP_FLAG_IN_CHAIN;
         *(SMX_HEAP_FREE_CHAIN_POINTER(pNewFreeBlock))=pNextFreeBlock;
         *pLastNodeChain=pNewFreeBlock;
      }
      else
      {
         /* skip block in list */
         *pLastNodeChain = pNextFreeBlock;
      }
   }
   else
   {
      nSize+=availableLength-neededLength;
      /* Else propagate the "last" flag */
      if(pBlockHeader==NULL)
      {
         flags |= SMX_HEAP_FLAG_LAST;
      }
      /* remove free block(s) from chain */
      *pLastNodeChain=pNextFreeBlock;
   }

   /* Then, create the block */
   pContent = static_SMXHeapFillUsedBlock(pSelectedBlock, nSize, flags);

   /* Then fill the memory */
   if((nType & SMX_HEAP_ZEROED) != 0)
   {
      memset(pContent, 0, nSize);
   }

   SMX_TRACE_INFO("SMXHeapAllocEx: The following block is allocated: %p", pContent);

   return pContent;
}

/* ------------------------------------------------------------------------ */

void* SMXHeapReallocEx(
      IN  S_HANDLE    hHeap,
      IN  void*         pBlock,
      IN  uint32_t           nNewSize,
      IN  uint8_t           nType)
{
   PSMX_HEAP_HEADER pBlockHeader, pNextBlockHeader, pNextInChain;
   void** pPrecInChain;
   uint32_t oldSize, availableSize, newTotalSize;
   uint16_t flags;
   void* pNewBlock;
   bool bEndReached;
   PSMX_HEAP_STRUCTURE  pMemoryBase;

   if(pBlock == NULL)
   {
      return SMXHeapAllocEx(hHeap, nNewSize, nType);
   }

   pMemoryBase = (PSMX_HEAP_STRUCTURE)SMXHeapConvertHandleToPointer(hHeap);
   if (pMemoryBase == NULL)
   {
      SMX_TRACE_ERROR("SMXHeapReallocEx[0x%x]: Invalid handle", hHeap);
      return NULL;
   }

   assert(nType == 0 || nType == SMX_HEAP_ZEROED);

#ifdef SMX_HEAP_FULL_DEBUG
   assert(SMXHeapIsMemorySafe(pMemoryBase) == S_SUCCESS);
#endif

   if(nNewSize == 0)
   {
      SMXHeapFreeEx(hHeap, pBlock , 0);
      return NULL;
   }

   pBlockHeader = static_SMXHeapContentToHeader(pBlock);

   SMX_TRACE_INFO("SMXHeapReallocEx: Reallocating the following block for %d bytes:", nNewSize);

   flags = pBlockHeader->flags;

   /* Test if the block is not free */
   assert((flags & SMX_HEAP_FLAG_ALLOCATED) != 0);

   oldSize = pBlockHeader->blockSize;
   if(oldSize == nNewSize)
   {
      return pBlock;
   }
   else
   {
      void** ppFirstFree;

      ppFirstFree = &(pMemoryBase->pFirstFree);
      pPrecInChain=ppFirstFree;
      while(*pPrecInChain != NULL &&
            mdMemSubstractPointer(pBlockHeader,*pPrecInChain) > 0)
      {
         pPrecInChain=
            SMX_HEAP_FREE_CHAIN_POINTER((PSMX_HEAP_HEADER)(*pPrecInChain));
      }
      assert(pPrecInChain != NULL);


      if(oldSize > nNewSize)
      {
         availableSize = static_SMXHeapGetBlockTotalSize(pBlockHeader);

         /* First, truncate the block */
         static_SMXHeapFillUsedBlock(pBlockHeader, nNewSize, flags);

         newTotalSize = static_SMXHeapGetBlockTotalSize(pBlockHeader);

         if(availableSize > newTotalSize + sizeof(SMX_HEAP_HEADER))
         {
            PSMX_HEAP_HEADER pNewFreeBlock=
               mdMemAddOffsetToPointer(pBlockHeader, newTotalSize);

            pBlockHeader->flags &= ~SMX_HEAP_FLAG_LAST;

            static_SMXHeapFillFreeBlock(pNewFreeBlock,
                                       availableSize - newTotalSize,
                                       (uint16_t)(flags & SMX_HEAP_FLAG_LAST));

            if(pNewFreeBlock->blockSize >= sizeof(void*))
            {
               *(SMX_HEAP_FREE_CHAIN_POINTER(pNewFreeBlock)) =
                  *pPrecInChain;
               *pPrecInChain=pNewFreeBlock;
               pNewFreeBlock->flags |= SMX_HEAP_FLAG_IN_CHAIN;
            }
         }
         else
         {
            pBlockHeader->blockSize+=availableSize-newTotalSize;
         }

#ifdef SMX_HEAP_FULL_DEBUG
         assert(SMXHeapIsMemorySafe(pMemoryBase) == S_SUCCESS);
#endif
         return pBlock;
      }
   }

   newTotalSize = static_SMXHeapComputeUsedTotalSize(nNewSize);

   availableSize = static_SMXHeapGetBlockTotalSize(pBlockHeader);
   bEndReached = true;
   pNextInChain = *pPrecInChain;
   pNextBlockHeader = pBlockHeader;
   /* try to enlarge with consecutive free block */
   while((pNextBlockHeader = static_SMXHeapGetNextBlock(pMemoryBase, pNextBlockHeader)) != NULL)
   {
      if((pNextBlockHeader->flags & SMX_HEAP_FLAG_ALLOCATED) != 0)
      {
         bEndReached = false;
         break;
      }
      availableSize += static_SMXHeapGetBlockTotalSize(pNextBlockHeader);
      if((pNextBlockHeader->flags & SMX_HEAP_FLAG_IN_CHAIN) != 0)
      {
         assert(pNextBlockHeader==pNextInChain);
         pNextInChain = (PSMX_HEAP_HEADER)
            (*SMX_HEAP_FREE_CHAIN_POINTER(pNextBlockHeader));
      }
   }

   if(newTotalSize <= availableSize)
   {
      /* The size of the block is increased */

      if(bEndReached != false)
      {
         flags |= SMX_HEAP_FLAG_LAST;
      }

      pBlock = static_SMXHeapFillUsedBlock(pBlockHeader, nNewSize, flags);

      if(nType == SMX_HEAP_ZEROED)
      {
         memset(mdMemAddOffsetToPointer(pBlock, oldSize), 0, nNewSize - oldSize);
      }

      if(availableSize > newTotalSize + sizeof(SMX_HEAP_HEADER))
      {
         PSMX_HEAP_HEADER pNewFreeBlock =
            mdMemAddOffsetToPointer(pBlockHeader, newTotalSize);

         pBlockHeader->flags &= ~SMX_HEAP_FLAG_LAST;

         static_SMXHeapFillFreeBlock(pNewFreeBlock,
            availableSize - newTotalSize,
            (uint16_t)(flags & SMX_HEAP_FLAG_LAST));
         if(pNewFreeBlock->blockSize >= sizeof(void*))
         {
            /* insert free block in chain */
            *SMX_HEAP_FREE_CHAIN_POINTER(pNewFreeBlock)=
               pNextInChain;
            *pPrecInChain=pNewFreeBlock;
            pNewFreeBlock->flags |= SMX_HEAP_FLAG_IN_CHAIN;
         }
         else
         {
            /* new freeblock is too short for keeping chaining
             * continue chain to next block */
            *pPrecInChain=pNextInChain;
         }
      }
      else
      {
         pBlockHeader->blockSize+=availableSize-newTotalSize;
         *pPrecInChain=pNextInChain;
      }

#ifdef SMX_HEAP_FULL_DEBUG
      assert(SMXHeapIsMemorySafe(pMemoryBase) == S_SUCCESS);
#endif
      return pBlock;
   }

   if((pNewBlock = SMXHeapAllocEx(hHeap, nNewSize, 0)) == NULL)
   {
      SMX_TRACE_ERROR("SMXHeapReallocEx: Cannot reallocate a new block of size %d bytes", nNewSize);
      return NULL;
   }

   memcpy(pNewBlock, pBlock, oldSize);

   SMXHeapFree(hHeap, pBlock);

   if(nType == SMX_HEAP_ZEROED)
   {
      memset(mdMemAddOffsetToPointer(pNewBlock, oldSize), 0, nNewSize - oldSize);
   }

   return pNewBlock;
}

/* ------------------------------------------------------------------------ */

void SMXHeapFreeEx(
      IN  S_HANDLE    hHeap,
      IN  void*         pBlock,
      IN  uint8_t           nType)
{
   PSMX_HEAP_HEADER pBlockHeader;
   PSMX_HEAP_STRUCTURE  pMemoryBase;

   assert(nType == 0 || nType == SMX_HEAP_ZEROED);

   if (pBlock == NULL) return;

   pMemoryBase = (PSMX_HEAP_STRUCTURE)SMXHeapConvertHandleToPointer(hHeap);
   if (pMemoryBase == NULL)
   {
      SMX_TRACE_ERROR("SMXHeapFreeEx[0x%x]: Invalid handle", hHeap);
      return;
   }

   pBlockHeader = static_SMXHeapContentToHeader(pBlock);

   SMX_TRACE_INFO("SMXHeapFreeEx: Free the following block: %p", pBlock);

   /* Test if the block is not free */
   assert((pBlockHeader->flags & SMX_HEAP_FLAG_ALLOCATED) != 0);

   if(nType == SMX_HEAP_ZEROED)
   {
      memset(pBlock, 0, pBlockHeader->blockSize);
   }

   static_SMXHeapFillFreeBlock(
      pBlockHeader,
      static_SMXHeapGetBlockTotalSize(pBlockHeader),
      (uint16_t)(pBlockHeader->flags & SMX_HEAP_FLAG_LAST));

   if(pBlockHeader->blockSize >= sizeof(void*))
   {
      void** pPrecInChain = &(pMemoryBase->pFirstFree);

      while(*pPrecInChain != NULL &&
            mdMemSubstractPointer(pBlockHeader,*pPrecInChain) > 0)
      {
         pPrecInChain=
            SMX_HEAP_FREE_CHAIN_POINTER((PSMX_HEAP_HEADER)(*pPrecInChain));
      }

      assert(pPrecInChain != NULL);
      /* pBlockHeader content take next free block in chain */
      *SMX_HEAP_FREE_CHAIN_POINTER(pBlockHeader)=*pPrecInChain;
      *pPrecInChain=pBlockHeader;
      pBlockHeader->flags |= SMX_HEAP_FLAG_IN_CHAIN;
   }
}

/* ------------------------------------------------------------------------ */
