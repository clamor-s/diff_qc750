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

#ifndef __SMX_HEAP_H__
#define __SMX_HEAP_H__


#include "s_type.h"


#ifdef __cplusplus
extern "C" {
#endif
#if 0
}  /* balance curly quotes */
#endif


/** The memory type associated with the free blocks */
#define SMX_HEAP_TYPE_FREE       0

/** The memory type associated with the normal blocks */
#define SMX_HEAP_TYPE_NORMAL     1

/** The memory type associated with all the allocated blocks */
#define SMX_HEAP_TYPE_ALLOCATED  2

/** The memory type associated with all the blocks */
#define SMX_HEAP_TYPE_ALL        3



/**
 * Initializes a heap object from an already allocated memory area.
 *
 * This API is basically an allocator API based on a pool of memory
 * already pre-allocated by the caller.
 *
 * @param   pAddressBase   The base address. A NULL pointer is invalid.
 *
 * @param   nHeapSize   The size in bytes of the allocated memory area.
 *                      A zero value is invalid.
 *
 * @param   phHeap      A pointer to the placeholder which receives a handle
 *                      to the newly initialized heap.  The content of this
 *                      placeholder is set to {S_HANDLE_NULL} upon error.
 *
 * @return  {S_SUCCESS} upon successful completion,
 *          or an appropriate error code upon failure.
 **/
S_RESULT SMXHeapInit(
      IN  void*         pBaseAddress,
      IN  uint32_t           nHeapSize,
      OUT S_HANDLE*   phHeap);


/**
 * Uninitializes a heap object.
 *
 * Upon successful return, <tt>hHeap</tt> is invalid and cannot be used
 * anymore.
 *
 * This function does nothing and returns {TRUE} if <tt>hHeap</tt> is set to
 * {S_HANDLE_NULL}.
 *
 * @param   hHeap    The heap to uninitialize.
 *
 * @return  {S_SUCCESS} upon successful completion,
 *          or an appropriate error code upon failure.
 **/
S_RESULT SMXHeapUninit(
      IN  S_HANDLE    hHeap);


/**
 * Allocates a block of memory from a heap. The address of the allocated block
 * is aligned on a 4-byte boundary. A block allocated by {SMXHeapAlloc} must
 * be freed by {SMXHeapFree}.
 *
 * @param   hHeap    Handle to the heap from which the memory will be
 *                   allocated. This handle is returned by the SMXHeapInit.
 *
 * @param   nSize    Number of bytes to be allocated. A zero value is invalid.
 *
 * @return  A pointer to the allocated memory block or
 *          NULL if the block cannot be allocated.
 **/
void* SMXHeapAlloc(
      IN  S_HANDLE    hHeap,
      IN  uint32_t           nSize);


/**
 * Reallocates a block of memory from a heap.
 * This function enables you to resize a memory block.
 *
 * If <tt>pBlock</tt> is NULL, SMXHeapRealloc(hHeap, pBlock, nNewSize)
 * is equivalent to SMXHeapAlloc(hHeap, nNewSize). In particular, if
 * nNewSize is 0, the function returns NULL.
 *
 * If <tt>pBlock</tt> is not NULL and <tt>nNewSize</tt> is 0, then
 * SMXHeapRealloc(hHeap, pBlock, nNewSize) is equivalent to 
 * SMXHeapFree(hHeap, pBlock) and returns NULL.
 *
 * If <tt>nNewSize</tt> is less or equal to the current size of the block,
 * the block is trucated, the content of the block is left unchanged and
 * the function returns <tt>pBlock</tt>.
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
 *                   re-allocated. This handle is returned by the SMXHeapInit.
 *
 * @param   pBlock   Pointer to the block of memory that the function
 *                   reallocates. This value may be null or returned by an
 *                   earlier call to the SMXHeapAlloc or SMXHeapRealloc
 *                   function.
 *
 * @param   nNewSize New size of the memory block, in bytes.
 *                   This value may be zero. A memory block's size can be
 *                   increased or decreased by using this function.
 *
 * @return  A pointer to the reallocated memory block or
 *          NULL if <tt>nNewSize</tt> is zero or if an error is detected.
 **/
void* SMXHeapRealloc(
      IN  S_HANDLE    hHeap,
      IN  void*         pBlock,
      IN  uint32_t           nNewSize);


/**
 * Frees a memory block allocated from a heap by the SMXHeapAlloc or
 * SMXHeapRealloc function.
 *
 * This function does nothing if pBlock is set to NULL.
 *
 * @param   hHeap    Handle to the heap whose memory block is to be freed.
 *                   This handle is returned by the SMXHeapInit.
 *
 * @param   pBlock   Pointer to the memory block to be freed.
 *                   This pointer is returned by an earlier call
 *                   to the SMXHeapAlloc or SMXHeapRealloc function.
 **/
void SMXHeapFree(
      IN  S_HANDLE    hHeap,
      IN  void*         pBlock);

#if 0
{  /* balance curly quotes */
#endif
#ifdef __cplusplus
}  /* closes extern "C" */
#endif


#endif  /* !defined(__SMX_HEAP_H__) */
