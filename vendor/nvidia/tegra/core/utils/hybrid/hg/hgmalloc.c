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
 * outlined in signed agreement may result in irrepairable harm to
 * Hybrid Graphics and legal action against the party in breach.
 *
 * $Id: //hybrid/libs/malloc/main/src/hgMalloc.c#8 $
 * $Date: 2007/01/31 $
 * $Author: tero $ *
 *
 *//**
 * \file
 * \brief   \c Malloc()/free() implementations for HG
 * \author  wili@hybrid.fi
 *
 *          No debug functionality (detection of leaks, double-frees, 
 *          overwrites) is provided here. A separate wrapper
 *          library "hgDebugMalloc" handles all that (and works
 *          with arbitrary malloc implementations).
 *          
 *          Todo list and change log in separate file hgMalloc.txt
 *          (if you're reviewing this code, please read those first,
 *          as they contain explanations for some of the oddities).
 *
 * \note    [wili 24/Oct/04] Added system for controlling when to start using the fixed allocators
 * \note    [wili 24/Oct/04] Added system that determines how big superblocks we should allocate
 * \note        [wili 25/Oct/04] Falls way too often to system allocs when there's not many allocs around 
 *
 * \todo        [wili 26/Oct/04] Add a rule that keeps the last superblock alive to avoid performance problems when
 *                                                       only a few allocations exist (spurious system alloc/release)
 * \todo        [wili 26/Oct/04] Better rounding of small superblock allocs. Now wastes memory on allocs of 240 bytes etc.
 *                                                       because 2048 doesn't divide evenly with it. Maybe just have a couple of classes, i.e.,
 *                                                       < 64 byte allocs allocate N entries, others allocate M entries?
 *
 *//*-------------------------------------------------------------------*/

#include "hgmalloc.h"
#include "hgint32.h"                            /* for hgClz32()        */

#if (HG_COMPILER == HG_COMPILER_RVCT)
#   include "nvos.h"
#   define MEMCPY(source1, source2, size) NvOsMemcpy(source1, source2, size)
#   define MEMSET(source, value, size) NvOsMemset(source, value, size)
#else
#   include <string.h>
#   define MEMCPY(source1, source2, size) memcpy(source1, source2, size)
#   define MEMSET(source, value, size) memset(source, value, size)
#endif

/*@-predboolptr -boolops -predboolint@*/
/*@-retalias -dependenttrans -mustfreeonly -usereleased -shiftnegative -branchstate -usedef -compmempass -compdef -onlytrans -nullstate -kepttrans -temptrans -mustfreefresh @*/

/*--------------------------------------------------------------------
 * Some defines that alter the functionality.of the allocator
 *------------------------------------------------------------------*/

#define USE_SMALL_ALLOCATOR                     /*!< \internal Define to enable the use of the small allocator */

/*--------------------------------------------------------------------
 * Forward declarations of internal structures and enumerations
 *------------------------------------------------------------------*/

typedef struct  Block_s                                 Block;
typedef struct  FreeBlock_s             FreeBlock;
typedef struct  FixedBlock_s            FixedBlock;
typedef struct  FixedFreeBlock_s        FixedFreeBlock;
typedef struct  FixedSuperBlock_s       FixedSuperBlock;
typedef struct  SuperBlock_s            SuperBlock;

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Block type enumeration (2 bits)
 * \note    \c BLOCK_MASK is not made an enum so that we can force
 *          it to be unsigned (much less casts required to silent LINT).
 *//*-------------------------------------------------------------------*/

enum Type_s
{
    BLOCK_FREE      = 0,                        /*!< \internal Block is a free one (in standard heap)     */
    BLOCK_USED      = 1,                        /*!< \internal Block is a used one (in standard heap)     */
    BLOCK_SENTINEL  = 2,                        /*!< \internal Block is a sentinel                        */
    BLOCK_FIXED     = 3                         /*!< \internal Block is a part of the fixed-size heap     */
};

typedef enum    Type_s                                  Type;

/*--------------------------------------------------------------------
 * Internal constants
 *------------------------------------------------------------------*/

#define BLOCK_MASK                  3u                                          /*!< \internal Mask that can be used when manipulating the bits */
#define LOG2_ALIGNMENT              4                       /*!< \internal Log2 of alignment          */
#define ALIGNMENT                   (1 << LOG2_ALIGNMENT)   /*!< \internal Alignment for allocations  */
#define FREE_LIST_LUT_SIZE          192                     /*!< \internal Number of entries in the freeblock search LUT                                                                                        */
#define SMALL_ALLOC_FREE_LIST       0x1f                    /*!< \internal Free list used (only) for small allocs that are allocated using the standard allocator       */
#define FREE_LIST_COUNT             32                                          /*!< \internal Number of free lists used */

/*--------------------------------------------------------------------
 * Parameters that can be tuned
 *------------------------------------------------------------------*/

#define FIXED_SUPERBLOCK_COUNT      32                                          /*!< \internal Number of fixed-sized allocators */
#define FIXED_MINIMUM_ATTEMPTS      15                                          /*!< \internal Minimum number of attempts for a fixed slot until the slot is created    */
#define MINIMUM_ALLOC_SIZE          ((FIXED_SUPERBLOCK_COUNT+1) * ALIGNMENT)    /*!< \internal Minimum alloc size handled by the standard (non-fixed) allocator         */
#define MAXIMUM_ALLOC_SIZE          0x7fffffffu                                 /*!< \internal Largest memory allocation we can handle                                  */
#define FIXED_SUPERBLOCK_SIZE       2048                                        /*!< \internal Size (in bytes) for fixed-allocator superblocks                          */

/*--------------------------------------------------------------------
 * Derived constants
 *------------------------------------------------------------------*/

#define FIXED_SUPERBLOCK_ALLOC      (FIXED_SUPERBLOCK_SIZE + sizeof(FixedSuperBlock) + ALIGNMENT)           /*!< \internal Number of bytes to allocate for fixed superblocks        */
#define MINIMUM_SUPERBLOCK_SIZE     (sizeof(SuperBlock)+2*sizeof(Block)+sizeof(FreeBlock)+ALIGNMENT)        /*!< \internal Minimum superblock size                                  */
#define MINIMUM_SUPERBLOCK_ALLOC        (FIXED_SUPERBLOCK_ALLOC + sizeof(Block) * 2 + sizeof(FreeBlock))        /*!< \internal Smallest possible superblock                                                             */


#define MAXIMUM_SUPERBLOCK_ALLOC        (MINIMUM_SUPERBLOCK_ALLOC * 32)                                                                         /*!< \internal Maximum superblock size we allocate voluntarily  */

HG_CT_ASSERT (FIXED_MINIMUM_ATTEMPTS >= 0 && FIXED_MINIMUM_ATTEMPTS <= 255);                                                            /* Has to fit into an unsigned byte */

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Header for all blocks allocated using the standard
 *          allocator
 * \note    The header contains pointers to previous and next entries
 *          (in memory) as well as the \c Type information of the block. 
 *          The  header requires eight bytes for architectures with 
 *          32-bit integers.
 *//*-------------------------------------------------------------------*/

struct Block_s                                             
{
    HGuintptr               nextBlock;        /*!< Pointer to next entry (address-wise) */
    HGuintptr               prevBlock;        /*!< Pointer to previous entry (address-wise) (with type field encoded into lower 2 bits) */
};

HG_CT_ASSERT (sizeof(Block) == (size_t)8); 

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Memory allocation header for free blocks. This has
 *          the standard Header plus links to previous and next
 *          free blocks.
 *//*-------------------------------------------------------------------*/

struct FreeBlock_s 
{
    Block                   base;             /*!< Base class */ 
    FreeBlock*              prevFree;         /*!< Pointer to previous free block in free list    */
    FreeBlock*              nextFree;         /*!< Pointer to next free block in free list        */
};

HG_CT_ASSERT (sizeof(FreeBlock) == (size_t)16); 

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Header for superblocks used by the fixed-size allocator
 *//*-------------------------------------------------------------------*/

struct FixedSuperBlock_s
{
    FixedSuperBlock*        prevSuperBlock;   /*!< Previous free FixedSuperBlock of the same size                             */
    FixedSuperBlock*        nextSuperBlock;   /*!< Next free FixedSuperBlock of the same size                                 */
    FixedFreeBlock*         firstFree;        /*!< First free block (or HG_NULL if superblock is closed)                      */
    HGMemory*               memoryManager;    /*!< Pointer back to the memory manager                                         */
    HGint16                 fixedIndex;       /*!< Which allocator size this is?                                              */
    HGint16                 usedCount;        /*!< # of fixed-size blocks that are used (when zero we can free the header!)   */
};

HG_CT_ASSERT (sizeof(FixedSuperBlock) == (size_t)20);

/*-------------------------------------------------------------------*//*!
 * \internal 
 * \brief   Header for blocks allocated by the fixed-size allocator
 * \note    Note that this is a copy of \c Block's first DWORD! 
 *//*-------------------------------------------------------------------*/

struct FixedBlock_s                    
{
    HGuintptr               pSuperBlock;      /*!< Pointer back to super block. Two lowest bits contain Type (same as prevBlock field in Block) */
};

HG_CT_ASSERT (sizeof(FixedBlock) == (size_t)4);

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Free entry in fixed allocator. Same as \c FixedBlock except 
 *          that we have an additional pointer to next free block of 
 *          the same size.
 *//*-------------------------------------------------------------------*/

struct FixedFreeBlock_s 
{
    FixedBlock              base;             /*!< Base class                                             */
    FixedFreeBlock*         nextFree;         /*!< Next free block in the same fixed superblock (or NULL) */
};

HG_CT_ASSERT (sizeof(FixedFreeBlock) == (size_t)8);

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief       Super block structure
 *//*-------------------------------------------------------------------*/

struct SuperBlock_s
{
    SuperBlock*             prevSuperBlock;   /*!< Pointer to previous superblock                                         */
    SuperBlock*             nextSuperBlock;   /*!< Pointer to next superblock                                             */ 
    void*                   allocation;       /*!< Original memory allocation                                             */
    HGuintptr               allocationSize;   /*!< Size of original memory allocation in bytes (needed for book-keeping)  */
};

HG_CT_ASSERT (sizeof(SuperBlock) == (size_t)16);

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Memory manager
 *//*-------------------------------------------------------------------*/

struct HGMemory_s
{
    HGMemoryAllocFunc       allocFunc;                                  /*!< Callback function for OS memory allocations            */
    HGMemoryReallocFunc     reallocFunc;                                /*!< Callback function for OS memory reallocations          */
    HGMemoryFreeFunc        freeFunc;                                   /*!< Callback function for OS memory freeing                */

        void*                                   userHeap;
        HGMemoryHeapAllocFunc   heapAllocFunc;                          /*!< Callback function for user heap memory allocations.        */
        HGMemoryHeapReallocFunc heapReallocFunc;                        /*!< Callback function for user heap memory reallocations.      */
        HGMemoryHeapFreeFunc    heapFreeFunc;                           /*!< Callback function for user heap memory freeing.            */

    SuperBlock*             superBlockHead;                             /*!< Head of linked list of superblocks                     */
    size_t                  memoryReserved;                             /*!< Number of bytes allocated from the operating system    */
    size_t                  memoryUsed;                                 /*!< Number of bytes used by user allocations               */
    HGuint32                freeListMask;                               /*!< Free list bit mask indicating which entries are free   */

    /* large members */
    FreeBlock*              freeList[FREE_LIST_COUNT];  /*!< Free lists for different sized free entries            */

#ifdef USE_SMALL_ALLOCATOR
    FixedSuperBlock*        fixedSuperBlocks[FIXED_SUPERBLOCK_COUNT]; /*!< Superblocks for fixed-size allocations                     */
    HGuint8                 fixedAttempts[FIXED_SUPERBLOCK_COUNT];    /*!< How many attempts have there been to allocate this size?   */
#endif
};                      

/*-------------------------------------------------------------------*//*!
 * \defgroup API Public API
 *//*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*//*!
 * \defgroup callback Callback functions provided by the user
 *//*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*//*!
 * \ingroup API
 * \typedef HGMemory
 * \brief   Memory manager
 *//*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*//*!
 * \ingroup callback
 * \typedef HGMemoryAllocFunc
 * \brief   User-provided callback function used for allocating memory
 *          from the operating system
 *//*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*//*!
 * \ingroup callback
 * \typedef HGMemoryReallocFunc
 * \brief   User-provided callback function used for re-allocating 
 *          memory from the operating system
 *//*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*//*!
 * \ingroup callback
 * \typedef HGMemoryFreeFunc
 * \brief   User-provided callback function used for releasing allocated
 *          memory back to the operating system
 *//*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*//*!
 * \internal
 * \hideinitializer
 * \brief       Lookup table used for determining the free list
 *              for a given allocation size. See \c getFreeListIndex()
 *              for further details.
 * \note        0x1f is now reserved for very small allocs that don't
 *              have a proper list allocator..
 *//*-------------------------------------------------------------------*/

static const HGuint8 s_freeListLUT[FREE_LIST_LUT_SIZE] = /*@-type@*/
{
    0x1e, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 
    0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 
    0x0f, 0x0f, 0x0e, 0x0e, 0x0d, 0x0d, 0x0d, 0x0d,
    0x0c, 0x0c, 0x0c, 0x0c, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01
}; /*@=type@*/

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Returns type of the block
 * \param   pBlock    Pointer to block (non-NULL)
 * \return  Type enumeration
 * \sa      setType()
 *//*-------------------------------------------------------------------*/

HG_INLINE Type getType (
    const Block* pBlock) /*@modifies@*/            
{ 
    HG_ASSERT (pBlock != HG_NULL);
    
    return (Type)(pBlock->prevBlock & BLOCK_MASK);                           
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Returns predecessor of the block
 * \param   pBlock    Pointer to block (non-NULL)
 * \return  Pointer to predecessor
 * \sa      getNext()
 * \note    The predecessor is the previous block _physically in memory_
 *//*-------------------------------------------------------------------*/

HG_INLINE Block* getPrev (
    const Block* pBlock) /*@modifies@*/            
{  
    HG_ASSERT (pBlock != HG_NULL);
    
    return HG_REINTERPRET_CAST (Block*, pBlock->prevBlock & ~BLOCK_MASK);   
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Returns successor of the block
 * \param   pBlock    Pointer to block (non-NULL)
 * \return  Pointer to successor
 * \sa      getPrev()
 * \note    The successor is the next block _physically in memory_
 *//*-------------------------------------------------------------------*/

HG_INLINE Block* getNext (
    const Block* pBlock) /*@modifies@*/            
{ 
    HG_ASSERT (pBlock != HG_NULL);
    
    return HG_REINTERPRET_CAST (Block*, pBlock->nextBlock);     
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Sets the type of the block
 * \param   pBlock    Pointer to block
 * \param   type      Type enumeration
 * \sa      getType()
 *//*-------------------------------------------------------------------*/

HG_INLINE void setType (
    Block*  pBlock, 
    Type    type)               
{ 
    HG_ASSERT (pBlock != HG_NULL);
    HG_ASSERT (!((HGuintptr)type & ~BLOCK_MASK));
    
    pBlock->prevBlock = (pBlock->prevBlock & ~(HGuintptr)BLOCK_MASK) + (HGuintptr)(type); 
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Sets the 'prev' pointer of a block
 * \param   pBlock    Pointer to block
 * \param   pPrev     Pointer to previous block
 * \sa      getPrev(), setNext()
 * \note    This does not perform any true linked list management. The
 *          function just writes to the 'prev' field of the block.
 *//*-------------------------------------------------------------------*/

HG_INLINE void setPrev (
    Block*  pBlock, 
    Block*  pPrev)  
{ 
    HG_ASSERT (pBlock != HG_NULL);
    HG_ASSERT (!((HGuintptr)(pPrev) & BLOCK_MASK)); 
    
    pBlock->prevBlock = (pBlock->prevBlock & BLOCK_MASK) + (HGuintptr)(pPrev); 
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Sets the 'next' pointer of a block
 * \param   pBlock    Pointer to block
 * \param   pNext     Pointer to next block
 * \sa      getNext(), setPrev()
 * \note    This does not perform any true linked list management. The
 *          function just writes to the 'next' field of the block.
 *//*-------------------------------------------------------------------*/

HG_INLINE void setNext (
    Block*  pBlock, 
    Block*  pNext)  
{
    HG_ASSERT (pBlock != HG_NULL);

    pBlock->nextBlock = (HGuintptr)(pNext);  
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Sets all parameters of the block header
 * \param   pBlock  Pointer to block
 * \param   type    Type of the block
 * \param   pPrev   Pointer to previous block physically in memory
 * \param   pNext   Pointer to next block physically in memory
 *//*-------------------------------------------------------------------*/

HG_INLINE void constructHeader (
    Block*  pBlock, 
    Type    type, 
    /*@null@*/Block*  pPrev, 
    /*@null@*/Block*  pNext)
{
    HG_ASSERT (pBlock != HG_NULL);
    HG_ASSERT ((HGuintptr)(type) <= (HGuintptr)BLOCK_MASK);
    HG_ASSERT (!((HGuintptr)(pPrev) & BLOCK_MASK));

    pBlock->nextBlock = HG_REINTERPRET_CAST (HGuintptr, (pNext));
    pBlock->prevBlock = HG_REINTERPRET_CAST (HGuintptr, (pPrev)) + HG_STATIC_CAST (HGuintptr,type);
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Returns pointer to \c FixedSuperBlock of a fixed block
 * \param   pFixedBlock Pointer to fixed block
 * \return  Pointer to the superblock where this block belongs
 * \sa      setFixedSuperBlock()
 *//*-------------------------------------------------------------------*/

HG_INLINE FixedSuperBlock* getFixedSuperBlock (
    const FixedBlock*   pFixedBlock) /*@modifies@*/                            
{ 
    HG_ASSERT (pFixedBlock != HG_NULL);

    return HG_REINTERPRET_CAST (FixedSuperBlock*,(pFixedBlock->pSuperBlock & ~BLOCK_MASK));  
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Determines if a pointer is aligned by \c ALIGNMENT
 * \param   ptr     Pointer (may be NULL)
 * \return  \c HG_TRUE if the pointer is properly aligned, \c HG_FALSE 
 *          otherwise
 *//*-------------------------------------------------------------------*/

#ifdef HG_DEBUG
HG_INLINE HGbool isPointerAligned (
    /*@null@*/ const void* ptr) /*@modifies@*/            
{
    return (HGbool)( ( HG_REINTERPRET_CAST(HGuintptr,ptr) & (ALIGNMENT - 1) ) == 0);
}
#endif

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Rounds a size upwards so that it is aligned by \c ALIGNMENT
 * \param   bytes   Integer value (size)
 * \return  Aligned integer value
 *//*-------------------------------------------------------------------*/

HG_INLINE size_t align (
    size_t bytes) /*@modifies@*/            
{
    return (size_t)((bytes + ALIGNMENT - 1ul) & ~(ALIGNMENT - 1ul));
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Returns size of block in bytes
 * \param   pBlock    Pointer to block (must be a valid pointer!)
 * \return  Size of the block in bytes (including block header)
 * \note    The size is not encoded in the block header directly. However,
 *          it can be computed from the distance between the "next" block
 *          and the current one.
 *//*-------------------------------------------------------------------*/

HG_INLINE size_t getBlockSize (
    const Block*   pBlock) /*@modifies@*/              
{
    HG_ASSERT (pBlock);

#ifdef USE_SMALL_ALLOCATOR
    if (getType(pBlock) == BLOCK_FIXED)
    {
        const FixedBlock* pFixedBlock;
        FixedSuperBlock*  pSuperBlock;

        pFixedBlock = HG_REINTERPRET_CAST(const FixedBlock*,(const HGuint8*)(pBlock)+sizeof(HGuintptr));
        pSuperBlock = getFixedSuperBlock(pFixedBlock);

        return ((pSuperBlock->fixedIndex+1) << LOG2_ALIGNMENT);
    }
    else
#endif /* USE_SMALL_ALLOCATOR */
    {
        HG_ASSERT (getNext(pBlock));

        return (size_t)(
            HG_REINTERPRET_CAST(const HGuint8*, getNext(pBlock)) -
            HG_REINTERPRET_CAST(const HGuint8*, pBlock)
            );
    }

}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Returns free list index for a block of certain size
 * \param   size   Size of the block in bytes
 * \return  Freelist index in range [0,31] or SMALL_ALLOC_FREE_LIST if the 
 *          block does not belong into any free list (too small)
 *//*-------------------------------------------------------------------*/

HG_INLINE int getFreeListIndex  (
    size_t size) /*@modifies@*/            
{
    if (size < (size_t)MINIMUM_ALLOC_SIZE)
        return SMALL_ALLOC_FREE_LIST;

    size = (size - MINIMUM_ALLOC_SIZE) >> 8;

    return (size >= (size_t)FREE_LIST_LUT_SIZE) ? 0x00 : (int)s_freeListLUT[size];
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Returns free list index of a free block
 * \param   pFreeBlock  Pointer to free block
 * \return  Freelist index in range [0,31]
 * \sa      getFreeListIndex()
 *//*-------------------------------------------------------------------*/

HG_INLINE int getBlockFreeListIndex (
    const FreeBlock* pFreeBlock) /*@modifies@*/            
{
    HG_ASSERT (pFreeBlock != HG_NULL);

    return getFreeListIndex (getBlockSize(&(pFreeBlock->base)));
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Unlink block from a freelist
 * \param   pMemoryManager  Pointer to memory manager
 * \param   pFreeBlock      Pointer to free block
 * \sa      linkBlockToFreeList()
 * \note    Leaves nextFree and prevFree members invalid (on purpose)
 *//*-------------------------------------------------------------------*/

HG_INLINE void HG_REGCALL unlinkBlockFromFreeList (
    HGMemory*   HG_RESTRICT pMemoryManager,
    FreeBlock*  HG_RESTRICT pFreeBlock)
{
    FreeBlock* HG_RESTRICT  prev = pFreeBlock->prevFree;
    FreeBlock* HG_RESTRICT  next = pFreeBlock->nextFree;
    
    HG_ASSERT (pMemoryManager && pFreeBlock);

    if (prev)
        prev->nextFree = next;
    else
    {
        int freeListIndex = getBlockFreeListIndex(pFreeBlock);  

        HG_ASSERT (freeListIndex >= 0 && freeListIndex < FREE_LIST_COUNT);
        HG_ASSERT (pMemoryManager->freeList[freeListIndex] == pFreeBlock);

        pMemoryManager->freeList[freeListIndex] = next;
        
        if (!next)
            pMemoryManager->freeListMask &= ~(1u << freeListIndex);           
    }

    if (next)
        next->prevFree = prev;
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Link block to freelist of corresponding size
 * \param   pMemoryManager  Pointer to memory manager (non-NULL)
 * \param   pFreeBlock      Pointer to free block (non-NULL)
 * \sa      unlinkBlockFromFreeList()
 *//*-------------------------------------------------------------------*/

HG_INLINE void HG_REGCALL linkBlockToFreeList (
    HGMemory*   HG_RESTRICT pMemoryManager,
    FreeBlock*  HG_RESTRICT pFreeBlock)
{
    int         freeListIndex = getBlockFreeListIndex(pFreeBlock);
    FreeBlock*  next;

    HG_ASSERT (pMemoryManager && pFreeBlock);
    HG_ASSERT (freeListIndex >= 0 && freeListIndex < FREE_LIST_COUNT);

    next                                       = pMemoryManager->freeList[freeListIndex];
    pMemoryManager->freeList[freeListIndex]    = pFreeBlock;         
    pMemoryManager->freeListMask              |= (1u << freeListIndex);

    pFreeBlock->nextFree = next;
    pFreeBlock->prevFree = HG_NULL;

    if (next)
        next->prevFree = pFreeBlock;
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Debug function for checking the validity of the current 
 *          free lists
 * \param   pMemoryManager  Pointer to Memory
 * \note    This function walks through the entire heap and can be
 *          quite slow. Use only during debugging.
 *//*-------------------------------------------------------------------*/

HG_INLINE void validateFreeList (
    const HGMemory* pMemoryManager)
{
#if defined (HG_DEBUG)
    const FreeBlock*    pFreeBlock;
    int                 i;

    for (i = 0; i < FREE_LIST_COUNT; i++)
    for (pFreeBlock = pMemoryManager->freeList[i]; pFreeBlock; pFreeBlock = pFreeBlock->nextFree)
    {
        const Block* prev = getPrev(&pFreeBlock->base);
        const Block* next = getNext(&pFreeBlock->base);

        HG_ASSERT (pFreeBlock != HG_NULL);
        HG_ASSERT (getType(&pFreeBlock->base) == BLOCK_FREE);     
        HG_ASSERT (getPrev(&pFreeBlock->base) != HG_NULL);
        HG_ASSERT (getNext(&pFreeBlock->base) != HG_NULL);
        HG_ASSERT (getNext(prev) == HG_STATIC_CAST(const Block*,pFreeBlock));
        HG_ASSERT (getPrev(next) == HG_STATIC_CAST(const Block*,pFreeBlock));
        HG_ASSERT (getType(prev) == BLOCK_USED || getType(prev) == BLOCK_SENTINEL);
        HG_ASSERT (getType(next) == BLOCK_USED || getType(next) == BLOCK_SENTINEL);
    }
#else
    HG_UNREF(pMemoryManager);
#endif
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Destroys a superblock
 * \param   pMemoryManager  Pointer to memory manager
 * \param   pFreeBlock      Pointer to the 'free area' of the superblock
 * \note    Calls freeToOS() to release the memory of the superblock
 * \note    This is the only place where memory is released to the
 *          operating system.
 *//*-------------------------------------------------------------------*/

HG_INLINE void destroySuperBlock (
    HGMemory*   HG_RESTRICT pMemoryManager,
    FreeBlock*  HG_RESTRICT pFreeBlock)
{
    /*--------------------------------------------------------------------
     * Find superblock pointer by stepping backwards in the list. Then
     * perform some sanity checks
     *------------------------------------------------------------------*/

    SuperBlock*                         pSuperBlock = HG_REINTERPRET_CAST(SuperBlock*,HG_REINTERPRET_CAST(Block*,pFreeBlock) - 1) - 1;               
    SuperBlock* HG_RESTRICT prev                = pSuperBlock->prevSuperBlock;
    SuperBlock* HG_RESTRICT next                = pSuperBlock->nextSuperBlock;

    /*--------------------------------------------------------------------
     * If we're keeping the last superblock alive and this is the
         * last superblock, just return.
         *------------------------------------------------------------------*/

    HG_ASSERT (pFreeBlock != HG_NULL);
    HG_ASSERT (pMemoryManager != HG_NULL);
    HG_ASSERT (pMemoryManager->superBlockHead != HG_NULL);
    HG_ASSERT (getType(&pFreeBlock->base)             == BLOCK_FREE);
    HG_ASSERT (getType(getPrev(&pFreeBlock->base))    == BLOCK_SENTINEL);
    HG_ASSERT (getType(getNext(&pFreeBlock->base))    == BLOCK_SENTINEL);

    /*--------------------------------------------------------------------
     * Unlink the free block from the free list
     *------------------------------------------------------------------*/

    pMemoryManager->memoryReserved -= pSuperBlock->allocationSize;         

    unlinkBlockFromFreeList (pMemoryManager, pFreeBlock);

    /*--------------------------------------------------------------------
     * Unlink the superblock from the superblock list (can't use existing 
     * linked list code, sorry).
     *------------------------------------------------------------------*/

    if (prev)
        prev->nextSuperBlock = next;
    else
    {
        HG_ASSERT (pSuperBlock == pMemoryManager->superBlockHead);
        pMemoryManager->superBlockHead = next;
    }

    if (next)
        next->prevSuperBlock = prev;
   
    /*--------------------------------------------------------------------
     * Release memory back to the operating system
     *------------------------------------------------------------------*/

        if (pMemoryManager->freeFunc)
            pMemoryManager->freeFunc (pSuperBlock->allocation);
        else
        {
                HG_ASSERT (pMemoryManager->userHeap);
                pMemoryManager->heapFreeFunc (pMemoryManager->userHeap, pSuperBlock->allocation);
        }
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Creates a new superblock
 * \param   pMemoryManager      Pointer to Memory
 * \param   size                Size of memory area in bytes
 * \return  HG_TRUE on success, HG_FALSE on failure 
 * \note    This is the only place where memory is allocated from the
 *          operating system.
 *//*-------------------------------------------------------------------*/

HG_INLINE HGbool createSuperBlock (
    HGMemory*   HG_RESTRICT pMemoryManager,
    size_t      size)
{
    /*--------------------------------------------------------------------
     * Attempt to allocate memory for the superblock from the operating
     * system.
     *------------------------------------------------------------------*/

    SuperBlock*     pSuperBlock;                                        /* pointer to superblock                    */          
    SuperBlock*     next;                                                                                               /* pointer to next superblock in list           */
    HGuint8*        pData;                                              /* pointer to superblock's data area        */
    size_t          dataSize;                                           /* size of superblock's data area           */
    void*           allocation;                                                                                 /* pointer to the allocated memory          */

    HG_ASSERT (size >= MINIMUM_SUPERBLOCK_SIZE);

    /*--------------------------------------------------------------------
     * Try to get memory from the system.
     *------------------------------------------------------------------*/

        if (pMemoryManager->allocFunc)
                allocation = pMemoryManager->allocFunc (size);
        else
        {
                HG_ASSERT (pMemoryManager->heapAllocFunc);
                allocation = pMemoryManager->heapAllocFunc (pMemoryManager->userHeap, size);
        }

    if (!allocation)                                                                                                    /* memory allocation failed?                            */
        return HG_FALSE;

    /*--------------------------------------------------------------------
     * Perform alignment of allocation and size (to guarantee properly 
         * aligned allocations later on).
     *------------------------------------------------------------------*/

    pSuperBlock = HG_REINTERPRET_CAST(SuperBlock*,(HG_REINTERPRET_CAST(HGuintptr,allocation) + (HGuintptr)(ALIGNMENT-1)) &~ (HGuintptr)(ALIGNMENT-1));
    pData       = HG_REINTERPRET_CAST(HGuint8*,(pSuperBlock+1));              /* skip past superblock header */
    dataSize    = (size_t)( (HGuintptr)(((HG_REINTERPRET_CAST(HGuint8*,allocation) + size)-pData)) &~ (HGuintptr)(ALIGNMENT-1) );
    next        = pMemoryManager->superBlockHead;

    HG_ASSERT (dataSize >= 2*sizeof(Block)+sizeof(FreeBlock));    /* minimum size! */

    /*--------------------------------------------------------------------
     * Setup the superblock's members and link it to linked list.
     *------------------------------------------------------------------*/

    pSuperBlock->allocation     = allocation;                   /* pointer to _original_ data                       */
    pSuperBlock->allocationSize = (HGuintptr)size;              /* original allocation size (just for book-keeping) */
    pSuperBlock->prevSuperBlock = HG_NULL;                      /* link superblock into global linked list          */
    pSuperBlock->nextSuperBlock = next;

    if (next)
        next->prevSuperBlock = pSuperBlock;

    pMemoryManager->superBlockHead     = pSuperBlock;
    pMemoryManager->memoryReserved    += size;

    /*--------------------------------------------------------------------
     * Create sentinel blocks and a free block. 
     *------------------------------------------------------------------*/

    {
        Block*     headSentinel = HG_REINTERPRET_CAST(Block*,pData);
        Block*     tailSentinel = HG_REINTERPRET_CAST(Block*,(pData + dataSize - sizeof(Block)));
        FreeBlock* freeBlock    = HG_REINTERPRET_CAST(FreeBlock*,(headSentinel + 1));

        constructHeader     (headSentinel,          BLOCK_SENTINEL, HG_NULL,            &freeBlock->base);
        constructHeader     (tailSentinel,          BLOCK_SENTINEL, &freeBlock->base,   HG_NULL);
        constructHeader     (&freeBlock->base,      BLOCK_FREE,     headSentinel,       tailSentinel);
        
        linkBlockToFreeList (pMemoryManager, freeBlock);
    }

    return HG_TRUE; /* success */
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Finds the best-fitting free block
 * \param   pMemoryManager      Pointer to Memory
 * \param   searchMask          Bit mask indicating which of the 32 free lists to search
 * \param   allocSize           Minimum block size in bytes
 * \return  Pointer to the free block or HG_NULL if none was found
 * \note    The function finds the smallest free memory block that
 *          satisfies the size requirement. 
 *//*-------------------------------------------------------------------*/

HG_INLINE /*@null@*/ FreeBlock* findFreeBlock (
    HGMemory*   HG_RESTRICT pMemoryManager,
    HGuint32    searchMask,
    size_t      allocSize)
{
    FreeBlock* pBestBlock = HG_NULL;                               /* pointer to free block    */

    /*--------------------------------------------------------------------
     * Keep looking as long as a free block hasn't been found. 
     *------------------------------------------------------------------*/

    while (searchMask)
    {
        size_t          bestSize        = (size_t)MAXIMUM_ALLOC_SIZE;               /* size of best-fitting block so far            */              
        int             freeListIndex   = 31 - hgClz32(searchMask);                 /* bit trickery to find next larger free list   */
        FreeBlock*      pBlock          = pMemoryManager->freeList[freeListIndex];

        searchMask &= ~(1u << freeListIndex);                                       /* remove slot from the search mask             */
        
        HG_ASSERT (pBlock != HG_NULL);                     

        /*----------------------------------------------------------------
         * Iterate through the free blocks in the free list. Find the
         * smallest block that is larger than 'allocSize'. Early-exit
         * if the size is very close to that of allocSize.
         *--------------------------------------------------------------*/

        do
        {
            size_t blockSize = getBlockSize(&pBlock->base);

            HG_ASSERT (getType(&pBlock->base) == BLOCK_FREE);

            if (blockSize >= allocSize && blockSize < bestSize)                                         /* valid and better that earlier ones?     */
            {
                pBestBlock  = pBlock;
                bestSize    = blockSize;
                
                if (bestSize < (allocSize + sizeof(FreeBlock)))                                         /* can't really get any better than this    */
                    return pBestBlock;
            }

            pBlock = pBlock->nextFree;

        } while (pBlock);

        /*--------------------------------------------------------------------
         * If we've found a free block, there's no point to search the next
         * free list (as all blocks in it are guaranteed to be larger
         * than pBestBlock) -> exit.
         *------------------------------------------------------------------*/

        if (pBestBlock)
                        break;
    }

    return pBestBlock;     
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Computes size of next allocatable superblock
 * \param   pMemoryManager      Pointer to memory manager
 * \param   bytes               Minimum number we *need* to allocate
 * \return  Number of bytes we *should* allocate
 *//*-------------------------------------------------------------------*/

HG_INLINE size_t computeSuperBlockAllocationSize (
    HGMemory*   HG_RESTRICT pMemoryManager,
    size_t      bytes)
{
    /*--------------------------------------------------------------------
     * Figure out optimal size for the superblock
     *------------------------------------------------------------------*/

        size_t optimal = pMemoryManager->memoryUsed;

        if (optimal > MAXIMUM_SUPERBLOCK_ALLOC)
                optimal = MAXIMUM_SUPERBLOCK_ALLOC;

    /*--------------------------------------------------------------------
     * Absolute minimum
     *------------------------------------------------------------------*/

    if (bytes < MINIMUM_SUPERBLOCK_ALLOC)
        bytes = MINIMUM_SUPERBLOCK_ALLOC;
  
        return bytes > optimal ? bytes : optimal;
}

/*-------------------------------------------------------------------*//*!
 * \ingroup API
 * \brief   Allocates memory from the memory manager (replacement
 *          for C system library function malloc()).
 * \param   pMemoryManager      Pointer to memory manager
 * \param   bytes               Number of bytes of memory to allocate
 * \return  Pointer to memory block or HG_NULL if allocation fails
 *          (out of memory error)
 * \sa      HGMemory_realloc(), HGMemory_free()
 * \note    A request for 0 bytes will generate a unique memory
 *          address (i.e., it will allocate memory!)
 * \note    See documentation of system library function malloc() for
 *          exact behavior
 *//*-------------------------------------------------------------------*/

void* HG_ATTRIBUTE_MALLOC HGMemory_malloc (
    HGMemory* HG_RESTRICT pMemoryManager,
    size_t      bytes)
{
        int             ctr;                                                                                                    /* loop counter -- as we may run the code twice */

    HG_ASSERT (pMemoryManager != HG_NULL);

    /*--------------------------------------------------------------------
     * Trap out-of-range allocations here so that we don't need to
     * clutter the rest of the code with checks.
     *------------------------------------------------------------------*/

    if (bytes > (size_t)MAXIMUM_ALLOC_SIZE)
        return HG_NULL;

    /*--------------------------------------------------------------------
     * Actual memory allocation loop
     *------------------------------------------------------------------*/
    
    for (ctr = 0; ctr < 2; ctr++)
    {
 #ifdef USE_SMALL_ALLOCATOR
                size_t  alignedSize = align (bytes + sizeof(FixedBlock));       /* size of the allocation after alignment               */


        /*--------------------------------------------------------------------
         * Handle small allocations using the fixed-size allocator. This
         * also handles zero-size allocs. This code could be refactored
         * into a separate function, but then GCC does a pretty bad job.
                 * Note that this code is executed only on the first pass.
         *------------------------------------------------------------------*/
    
        if (((alignedSize >> LOG2_ALIGNMENT) <= (size_t)FIXED_SUPERBLOCK_COUNT) && ctr == 0)
        {
            int                 superBlockIndex     = (int)((alignedSize >> LOG2_ALIGNMENT) - 1);                      /* which fixed allocator are we using? */                                
            FixedSuperBlock*    pFixedSuperBlock    = pMemoryManager->fixedSuperBlocks[superBlockIndex];
            int                 attempts            = (int)pMemoryManager->fixedAttempts[superBlockIndex];

            HG_ASSERT (alignedSize <= (size_t)FIXED_SUPERBLOCK_SIZE);
            HG_ASSERT (alignedSize >= sizeof(FixedFreeBlock));               
            HG_ASSERT (alignedSize == align(alignedSize));
            HG_ASSERT (superBlockIndex >= 0 && superBlockIndex < FIXED_SUPERBLOCK_COUNT);   
            HG_ASSERT (attempts <= FIXED_MINIMUM_ATTEMPTS);

            /*--------------------------------------------------------------------
             * Increase the attempt count for this small alloc. If we haven't
             * reached yet the threshold until we switch to use the small allocs,
             * we'll direct this allocation into the standard memory manager
             * (unless of course a pool exists).
             *------------------------------------------------------------------*/

            if (attempts < FIXED_MINIMUM_ATTEMPTS)
                pMemoryManager->fixedAttempts[superBlockIndex] = (HGuint8)(attempts + 1);

            /*--------------------------------------------------------------------
             * If there's no superblock available, create one (this code is 
             * executed quite rarely and isn't really that performance critical
             * -- so we just concentrate on minimizing the size).
             *------------------------------------------------------------------*/

            if (!pFixedSuperBlock && attempts == FIXED_MINIMUM_ATTEMPTS)
            {
                /*--------------------------------------------------------------------
                 * Allocate memory for the fixed superblock. Note that this causes
                 * a recursive call to HGMemory_malloc(). However, the allocation
                 * size is guaranteed to be so big that it won't come back to
                 * this code, i.e., we do not have problems with infinite
                 * recursion or anything like that.
                 *------------------------------------------------------------------*/

                pFixedSuperBlock = HG_REINTERPRET_CAST(FixedSuperBlock*, HGMemory_malloc (pMemoryManager, FIXED_SUPERBLOCK_ALLOC));

                if (pFixedSuperBlock) /* allocation succeeded */
                {
                                        HGuint8*                        pBlocks         = (HGuint8*)(pFixedSuperBlock+1) + (ALIGNMENT-((sizeof(FixedSuperBlock)+sizeof(FixedBlock)) & (ALIGNMENT-1)));
                                        int                                     numBlocks       = FIXED_SUPERBLOCK_SIZE / alignedSize;
                                        FixedFreeBlock**        nextPtr         = &pFixedSuperBlock->firstFree;
                                        int                                     i;

                    pFixedSuperBlock->memoryManager             = pMemoryManager;
                    pFixedSuperBlock->fixedIndex                = (HGint16)superBlockIndex;
                    pFixedSuperBlock->prevSuperBlock    = HG_NULL;
                    pFixedSuperBlock->nextSuperBlock    = HG_NULL;
                    pFixedSuperBlock->usedCount                 = 0;

                    pMemoryManager->fixedSuperBlocks[superBlockIndex] = pFixedSuperBlock;

                    /*--------------------------------------------------------------------
                     * Organize the free blocks into a singly-linked list
                     *------------------------------------------------------------------*/

                                        for (i = 0; i < numBlocks; i++)
                                        {
                                                FixedFreeBlock* block = HG_REINTERPRET_CAST(FixedFreeBlock*, pBlocks + i * alignedSize);
                                                HG_ASSERT((HG_REINTERPRET_CAST(HGuintptr, pFixedSuperBlock) & BLOCK_MASK) == 0);
                                                block->base.pSuperBlock = HG_REINTERPRET_CAST(HGuintptr, pFixedSuperBlock) + (HGuintptr)BLOCK_FIXED;
                                                *nextPtr = block;
                                                nextPtr = &block->nextFree;
                                        }
                                        *nextPtr = HG_NULL;
                                }
                        }

            /*--------------------------------------------------------------------
             * If we have a superblock, perform allocation from it.
             *------------------------------------------------------------------*/

            if (pFixedSuperBlock)
            {
                FixedFreeBlock* pFreeBlock = pFixedSuperBlock->firstFree;

                HG_ASSERT (pFixedSuperBlock && getFixedSuperBlock(&(pFreeBlock->base)) == pFixedSuperBlock);

                pFixedSuperBlock->firstFree = pFreeBlock->nextFree;
                pFixedSuperBlock->usedCount++;

                /*--------------------------------------------------------------------
                 * If we allocate the last free block of the superblock, remove the 
                 * superblock from the free superblock list. This way further allocations 
                 * do not need to search through completely used superblocks.
                 *------------------------------------------------------------------*/

                if (!pFixedSuperBlock->firstFree)                             
                {
                    FixedSuperBlock* next = pFixedSuperBlock->nextSuperBlock;

                    HG_ASSERT(!pFixedSuperBlock->prevSuperBlock);                 
        
                    pMemoryManager->fixedSuperBlocks[superBlockIndex] = next;
        
                    if (next)
                        next->prevSuperBlock = HG_NULL;
        
                    pFixedSuperBlock->prevSuperBlock = HG_NULL;               
                    pFixedSuperBlock->nextSuperBlock = HG_NULL;               
                }

                /*--------------------------------------------------------------------
                 * Return pointer to the memory.
                 *------------------------------------------------------------------*/

                return (void*)(HG_REINTERPRET_CAST(FixedBlock*,pFreeBlock) + 1);
            }

            /* FALLTHRU to standard allocator */
        }

    #endif /* USE_SMALL_ALLOCATOR */

        {
                        size_t      allocSize;      /* aligned size including header                        */
                        size_t      remainder;      /* bytes remaining in original free block after alloc   */
            FreeBlock*  pFreeBlock;     /* pointer to free block    */
            HGuint32    searchMask;     /* mask indicating which free lists to search */

            /*--------------------------------------------------------------------
             * Use the standard allocator to perform the allocation
             *
             * Align the block size (including the header) to ALIGNMENT.
             *------------------------------------------------------------------*/

            allocSize = align (bytes + sizeof(Block));

            /*--------------------------------------------------------------------
             * Find a free block that can hold the allocation
             *------------------------------------------------------------------*/

            searchMask  = ( (1u << getFreeListIndex(allocSize)) << 1) - 1u;
            searchMask &= pMemoryManager->freeListMask;                                                                                 /* don't search in empty slots */
            pFreeBlock  = findFreeBlock (pMemoryManager, searchMask, allocSize);

            /*--------------------------------------------------------------------
             * If we managed to find a suitable free block...
             *------------------------------------------------------------------*/

            if (pFreeBlock)
            {
                /*--------------------------------------------------------------------
                 * Remove the free block from the free list
                 * \todo We only need to do this if the free list changes. Should
                 *       we check for this?
                 *------------------------------------------------------------------*/

                HG_ASSERT (pFreeBlock && getType(&pFreeBlock->base) == BLOCK_FREE);

                unlinkBlockFromFreeList (pMemoryManager, pFreeBlock);   

                /*--------------------------------------------------------------------
                 * Check if the allocation would shrink the remaining free block 
                 * smaller than a FreeHeader structure -> in such a case we delete 
                 * the FreeBlock entry (the block couldn't be used for anything
                 * anyway).
                 * \todo Shouldn't we do this if there's only a few bytes left
                 *       in the Block (as it's not really useful for anything?).
                 *------------------------------------------------------------------*/

                HG_ASSERT ( getBlockSize(&pFreeBlock->base) >= allocSize);
                remainder = getBlockSize(&pFreeBlock->base) -  allocSize;                     /* how many bytes remain of the free block? */

                if (remainder < sizeof(FreeBlock))                                            /* if we would consume the entire block... */
                {
                    pMemoryManager->memoryUsed += getBlockSize(&pFreeBlock->base);
                    setType (&pFreeBlock->base, BLOCK_USED);                                  /* set block type to "used" */
                    return HG_REINTERPRET_CAST(Block*,pFreeBlock) + 1;                        /* we're done.. */
                }

                /*--------------------------------------------------------------------
                 * OK. There's still some memory to serve from this block.. So we 
                 * keep the free block in its place but insert a new "used" block to
                 * the very end of the free block's memory area.
                 *------------------------------------------------------------------*/

                {
                    Block* used = HG_REINTERPRET_CAST(Block*,(HG_REINTERPRET_CAST(HGuint8*,pFreeBlock) + remainder));
                    Block* next = getNext(&pFreeBlock->base);

                    HG_ASSERT (next != HG_NULL);
    
                    constructHeader (used, BLOCK_USED, &pFreeBlock->base, next);
                    setPrev         (next, used);
                    setNext         (&pFreeBlock->base, used);
    
                    pMemoryManager->memoryUsed += getBlockSize (used);
    
                    HG_ASSERT (getType(&pFreeBlock->base) == BLOCK_FREE);

                    /*--------------------------------------------------------------------
                     * Move block to the head in order to speed up 
                     * searches in the future. 
                     *------------------------------------------------------------------*/

                    linkBlockToFreeList (pMemoryManager, pFreeBlock);

                    /*--------------------------------------------------------------------
                     * Return pointer to the memory block
                     *------------------------------------------------------------------*/

                    return (void*)(used + 1);                                
                }
            }
        }


        /*--------------------------------------------------------------------
         * If we failed the first time, create a new superblock that is 
         * guaranteed to hold the allocation. If the superblock cannot be 
         * created (OS refuses to give more memory), we exit (return NULL). 
         * Otherwise, we *will* succeed in the next allocateInternal() 
         * call.
         *
         * We first try to allocate an amount of memory that we be 
         * "nice to have". If that fails, we try to allocate the very
         * minimum we need for satisfying this request.
         *------------------------------------------------------------------*/

                HG_ASSERT (ctr == 0); /* can come here exactly one.. */

        {
            HGint32 attempt;
            size_t  blockSize = align (bytes + sizeof(Block)) + MINIMUM_SUPERBLOCK_SIZE;

            for (attempt = 0; attempt < 2; attempt++)
            {
                size_t attemptSize = (attempt == 0) ? computeSuperBlockAllocationSize (pMemoryManager, blockSize) : blockSize;

                if (createSuperBlock(pMemoryManager, attemptSize))
                    /*@innerbreak@*/break;
            }

            if (attempt == 2)   /* If neither allocation worked, exit (we can't serve the memory!) */
                break;
        }
    }

    /*--------------------------------------------------------------------
     * Operating system couldn't satisfy our allocation!
     *------------------------------------------------------------------*/

    return HG_NULL;                                     
}

/*-------------------------------------------------------------------*//*!
 * \ingroup API
 * \brief   Releases memory previously allocated using the memory
 *          manager
 * \param   pMemoryManager  Pointer to memory manager
 * \param   ptr             Pointer to data to be freed (may be NULL)   
 * \sa      HGMemory_malloc(), HGMemory_realloc()
 * \note    See documentation of system library function free() for
 *          exact behavior
 * \assert  The function asserts in the debug build if the block has
 *          not been allocated from the memory manager.
 *//*-------------------------------------------------------------------*/

void HGMemory_free (
    HGMemory*   HG_RESTRICT pMemoryManager,
    void*       ptr)
{
    Block* pBlock;
        
    /*--------------------------------------------------------------------
     * Handle freeing of NULL pointer silently
     *------------------------------------------------------------------*/

    HG_ASSERT (isPointerAligned(ptr));
    HG_ASSERT (pMemoryManager != HG_NULL);

    if (!ptr)                                                   
        return;

        pBlock = HG_REINTERPRET_CAST(Block*,ptr) - 1;  /* get block header */

    /*--------------------------------------------------------------------
     * Is the block is allocated using the fixed allocator?
     *------------------------------------------------------------------*/

#ifdef USE_SMALL_ALLOCATOR
    if (getType(pBlock) == BLOCK_FIXED)
    {
        /*--------------------------------------------------------------------
         * Locate the superblock and memory manager used by the block
         *------------------------------------------------------------------*/

        FixedFreeBlock*         pFreeBlock      = HG_REINTERPRET_CAST(FixedFreeBlock*,(HGuint8*)(pBlock)+sizeof(HGuintptr) );
        FixedSuperBlock*    pSuperBlock     = getFixedSuperBlock(&(pFreeBlock->base));                                        
        HGint32             fixedIndex      = (HGint32)pSuperBlock->fixedIndex;

/*      pMemoryManager  = pSuperBlock->memoryManager; */ /* we could locate it this way as well */
    
        HG_ASSERT (pSuperBlock && pMemoryManager);

        /*--------------------------------------------------------------------
         * If the superblock becomes empty, destroy it (unlink and release
         * memory back to the operating system).
         *------------------------------------------------------------------*/

        if (--pSuperBlock->usedCount == 0)
        {
            FixedSuperBlock* prev       = pSuperBlock->prevSuperBlock;
            FixedSuperBlock* next       = pSuperBlock->nextSuperBlock;

            /*--------------------------------------------------------------------
             * Unlink superblock from linked list of fixed superblocks
             *------------------------------------------------------------------*/

            HG_ASSERT (prev || next || pMemoryManager->fixedSuperBlocks[fixedIndex] == pSuperBlock);

            if (prev) 
                prev->nextSuperBlock = next;
            else
            {
                HG_ASSERT (pSuperBlock == pMemoryManager->fixedSuperBlocks[fixedIndex]);     
                pMemoryManager->fixedSuperBlocks[fixedIndex] = next;

                /*--------------------------------------------------------------------
                 * If this was the last superblock of this size, reset the 
                 * attempt counters
                 *------------------------------------------------------------------*/

                if (!next)
                    pMemoryManager->fixedAttempts[fixedIndex] = (HGuint8)0;
            }

            if (next) 
                next->prevSuperBlock = prev;

            /*--------------------------------------------------------------------
             * Note that even though the call to HGMemory_free() causes 
             * recursion, the execution will not re-enter fixedFree(), as
             * it is handled by the large-block removal.
                         * \todo: could we avoid the recursion by doing a fallthru
                         *        to the large alloc code below?
             *------------------------------------------------------------------*/
        
            HGMemory_free (pMemoryManager, pSuperBlock);
        
        } else /* superblock does not become empty */
        {
            HG_ASSERT (pSuperBlock->usedCount > 0);                                 

            /*--------------------------------------------------------------------
             * If the superblock was full before this release (i.e. it did not belong
             * to the "free list of superblocks"), add the superblock to the head of
             * "free list" of the given size.
             *------------------------------------------------------------------*/

            if (!pSuperBlock->firstFree)
            {
                FixedSuperBlock* next = pMemoryManager->fixedSuperBlocks[fixedIndex];

                HG_ASSERT (!pSuperBlock->prevSuperBlock && !pSuperBlock->nextSuperBlock && pMemoryManager->fixedSuperBlocks[fixedIndex] != pSuperBlock);

                pMemoryManager->fixedSuperBlocks[fixedIndex] = pSuperBlock;
                
                pSuperBlock->prevSuperBlock = HG_NULL;
                pSuperBlock->nextSuperBlock = next;
            
                if (next)
                    next->prevSuperBlock = pSuperBlock;
            }

            /*--------------------------------------------------------------------
             * Add block to free list of the superblock
             *------------------------------------------------------------------*/

            pFreeBlock->nextFree      = pSuperBlock->firstFree;                               
            pSuperBlock->firstFree    = pFreeBlock;
        }
    } else
#endif    
    {
        /*--------------------------------------------------------------------
         * The block is allocated using the standard allocator.
         * \todo prefetch the headers for next/prev (all of the time goes
         *       in the memory accesses when dealing with large memory
         *       allocations).
         *------------------------------------------------------------------*/

        FreeBlock* block = HG_REINTERPRET_CAST(FreeBlock*, pBlock);       
        FreeBlock* prev  = HG_REINTERPRET_CAST(FreeBlock*, getPrev(&block->base));      
        Block*     next  = getNext(&block->base);

        pMemoryManager->memoryUsed -= getBlockSize(pBlock);
    
                HG_ASSERT (pMemoryManager->memoryUsed >= sizeof(HGMemory));
        HG_ASSERT (getType(pBlock) == BLOCK_USED && "HGMemory_free(): Releasing an already freed block!");  /*lint !e506 */

        /*--------------------------------------------------------------------
         * Now all we have to do is to perform coalescing of the blocks. There
         * are four different cases (based on status of left/right blocks). 
         * Note that we _know_ there are valid next and prev pointers because 
         * of the sentinels used.
         *------------------------------------------------------------------*/

        if (getType(next) == BLOCK_FREE)                                                        
        {
            FreeBlock* old = HG_REINTERPRET_CAST(FreeBlock*, next);
            next = getNext(next);
            unlinkBlockFromFreeList (pMemoryManager, old);
        }

        HG_ASSERT (next != HG_NULL);

        if (getType(&prev->base) == BLOCK_FREE)
        {
            unlinkBlockFromFreeList (pMemoryManager, prev);
            setNext                 (&prev->base, next);

            block = prev;               /* change to operate on previous block */
        } else
        {
            
            constructHeader (&block->base, BLOCK_FREE, &prev->base, next);
            setNext         (&prev->base, &block->base);
        }

        setPrev             (next, &block->base);
        linkBlockToFreeList (pMemoryManager, block);        

        /*--------------------------------------------------------------------
         * If list became empty (only sentinels around the free block),
         * destroy the superblock.
         *------------------------------------------------------------------*/

        if (getType(getPrev(&block->base)) == BLOCK_SENTINEL && 
            getType(getNext(&block->base)) == BLOCK_SENTINEL)
        {
            destroySuperBlock (pMemoryManager, block);    
        }
    }
}

/*-------------------------------------------------------------------*//*!
 * \ingroup API
 * \brief   Reallocates memory
 * \param   pMemoryManager  Pointer to Memory manager
 * \param   ptr             Pointer to block of memory allocated earlier
 *                          or \c HG_NULL
 * \param   bytes           New size of the memory block
 * \sa      HGMemory_malloc(), HGMemory_free()
 * \return  Pointer to the new memory block or \c HG_NULL if allocation 
 *          fails (out of memory error)
 * \note    See documentation for C standard library function realloc()
 *          for exact behavior.
 * \note    This function behaves correctly but the implementation is
 *          far from optimal. 
 *//*-------------------------------------------------------------------*/

void* HGMemory_realloc (
    HGMemory* HG_RESTRICT pMemoryManager,
    void*       ptr,
    size_t      bytes)
{
    void*       pNew = HG_NULL;
    Block*      pBlock;         
    size_t      oldSize;                    /* size of old memory allocation in bytes */

    HG_ASSERT (pMemoryManager != HG_NULL);

    /*--------------------------------------------------------------------
     * If 'ptr' is NULL, the function behaves as ordinary malloc.
     *------------------------------------------------------------------*/

    if (!ptr)
        return HGMemory_malloc (pMemoryManager, bytes);

    /*--------------------------------------------------------------------
     * A request of zero bytes frees the original block and returns NULL.
     *------------------------------------------------------------------*/

    if (bytes)
    {
        /*--------------------------------------------------------------------
         * Determine size of the old allocation
         *------------------------------------------------------------------*/

        pBlock = HG_REINTERPRET_CAST(Block*,ptr) - 1; /* get header */

        HG_ASSERT (((getType(pBlock) & BLOCK_USED) != 0) && "HGMemory_realloc(): Invalid input pointer"); /*lint !e506 */
    
        oldSize = getBlockSize(pBlock);

#ifdef USE_SMALL_ALLOCATOR
        if (getType(pBlock) == BLOCK_FIXED)
        {
            size_t alignedSize = align(bytes + sizeof(FixedBlock));
 
            /*-------------------------------------------------------------
             * If the new block size would equal the old block size,
             * we can just return the old block.
             *-----------------------------------------------------------*/

            if (oldSize == alignedSize)
                return ptr;

            /*-------------------------------------------------------------
             * We don't know the exact allocation size, so set oldSize
             * as the tightest upper bound.
             *-----------------------------------------------------------*/

            oldSize -= sizeof(FixedBlock);
        }
        else
#endif /* USE_SMALL_ALLOCATOR */
        {
            HG_ASSERT (getType(pBlock) == BLOCK_USED);
            
            oldSize -= sizeof(Block);

            /*--------------------------------------------------------------------
             * If the size doesn't change, just return pointer to the old block
             * \todo [wili 6/Nov/03]    There are also cases where reallocating 
             *                          doesn't make any difference so we should 
             *                          detect them here.
             *------------------------------------------------------------------*/

            if (oldSize == bytes)
                return ptr;
        }

        /*--------------------------------------------------------------------
         * If the size changes, we need to allocate additional memory and
         * copy contents of the old block to the new one.
         * \todo [wili 6/Nov/03]    When contracting (and we're not dealing
         *                          with a fixed-size allocator), we should 
         *                          actually try to change the size of the
         *                          allocation.
         * \todo [wili 6/Nov/03]    If the alloc was originally directed
         *                          to the OS heap, we should attempt to
         *                          expand the OS allocation (need a callback
         *                          function for this as well), i.e.,
         *                          call realloc().
         *------------------------------------------------------------------*/

        pNew = HGMemory_malloc (pMemoryManager, bytes);

        /*--------------------------------------------------------------------
         * If memory allocation failed, return either a pointer to the old
         * block (new size is smaller than old size) or HG_NULL.
         *------------------------------------------------------------------*/

        if (!pNew)  
            return (bytes <= oldSize) ? ptr : HG_NULL;

        /*--------------------------------------------------------------------
         * Copy data from old allocation to the new one
         *------------------------------------------------------------------*/

        if (oldSize)
            MEMCPY (pNew, ptr, oldSize < bytes ? oldSize : bytes);
    }

    /*--------------------------------------------------------------------
     * Release old memory allocation and return pointer to new block
     *------------------------------------------------------------------*/

    HGMemory_free (pMemoryManager, ptr);

    return pNew;
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Sets up a previously allocated memory manager.
 * \param       pMemoryManager Pointer to memory manager.
 *//*-------------------------------------------------------------------*/

static void HGMemory_setup (
    HGMemory* HG_RESTRICT pMemoryManager)
{
    /*--------------------------------------------------------------------
     * Clear the structure
     *------------------------------------------------------------------*/

        MEMSET (pMemoryManager, 0, sizeof(HGMemory));

    /*--------------------------------------------------------------------
     * We need to take the initial allocation into account to the
     * statistics
     *------------------------------------------------------------------*/

    pMemoryManager->memoryUsed     = sizeof(HGMemory);
    pMemoryManager->memoryReserved = sizeof(HGMemory);
}

/*-------------------------------------------------------------------*//*!
 * \ingroup API
 * \brief   Creates a new memory manager
 * \param   allocFunc   Callback function for performing OS memory allocs
 * \param   reallocFunc Callback function for performing OS memory reallocs (can be NULL)
 * \param   freeFunc    Callback function for performing OS memory releases
 * \return  Pointer to memory manager or NULL on failure (invalid input params)
 * \sa      HGMemory_exit()
 *//*-------------------------------------------------------------------*/

HGMemory* HGMemory_init (
    HGMemoryAllocFunc   allocFunc, 
    HGMemoryReallocFunc reallocFunc, 
    HGMemoryFreeFunc    freeFunc)
{
    HGMemory* m;

    /*--------------------------------------------------------------------
     * Validate input parameters. Note that we allow NULL reallocFuncs!
     *------------------------------------------------------------------*/

    if (!allocFunc || !freeFunc)
        return HG_NULL;

    /*--------------------------------------------------------------------
     * Allocate memory from OS for the memory manager itself; exit
     * on failure.
     *------------------------------------------------------------------*/

    m = (HGMemory*)allocFunc (sizeof(HGMemory));    
    if (!m)
        return HG_NULL;

    /*--------------------------------------------------------------------
     * Setup the structure
     *------------------------------------------------------------------*/

        HGMemory_setup(m);

    /*--------------------------------------------------------------------
     * Copy function pointers
     *------------------------------------------------------------------*/

        m->allocFunc      = allocFunc;
    m->reallocFunc    = reallocFunc;
    m->freeFunc       = freeFunc;

    return m;
}

/*-------------------------------------------------------------------*//*!
 * \ingroup API
 * \brief   Creates a new memory manager using an external heap.
 * \param   userHeap        Pointer to the user heap object.
 * \param   heapAllocFunc   Callback function for performing heap memory allocs
 * \param   heapReallocFunc Callback function for performing heap memory reallocs (can be NULL)
 * \param   heapFreeFunc    Callback function for performing heap memory releases
 * \return  Pointer to memory manager or NULL on failure (invalid input params)
 * \sa      HGMemory_exit()
 *//*-------------------------------------------------------------------*/

HGMemory* HGMemory_initWithUserHeap (
    void*                   userHeap,
    HGMemoryHeapAllocFunc   heapAllocFunc, 
    HGMemoryHeapReallocFunc heapReallocFunc, 
    HGMemoryHeapFreeFunc    heapFreeFunc)
{
    HGMemory* m;

    /*--------------------------------------------------------------------
     * Validate input parameters. Note that we allow NULL reallocFuncs!
     *------------------------------------------------------------------*/

    if (!heapAllocFunc || !heapFreeFunc)
        return HG_NULL;

    /*--------------------------------------------------------------------
     * Allocate memory from OS for the memory manager itself; exit
     * on failure.
     *------------------------------------------------------------------*/

    m = (HGMemory*)heapAllocFunc (userHeap, sizeof(HGMemory));    
    if (!m)
        return HG_NULL;

    /*--------------------------------------------------------------------
     * Setup the structure
     *------------------------------------------------------------------*/

        HGMemory_setup(m);

    /*--------------------------------------------------------------------
     * Copy function pointers
     *------------------------------------------------------------------*/

        m->userHeap         = userHeap;
        m->heapAllocFunc    = heapAllocFunc;
    m->heapReallocFunc  = heapReallocFunc;
    m->heapFreeFunc     = heapFreeFunc;

    return m;
}

/*-------------------------------------------------------------------*//*!
 * \ingroup API
 * \brief   Shut down the memory manager
 * \param   pMemoryManager  Pointer to Memory manager (non-\c NULL)
 * \return  \c HG_TRUE if there were memory leaks, \c HG_FALSE otherwise
 * \note    All memory ever allocated by the memory manager is released
 *          to the operating system at this point. 
 * \sa      HGMemory_init()
 *//*-------------------------------------------------------------------*/

HGbool HGMemory_exit (
    HGMemory* HG_RESTRICT pMemoryManager)
{
    SuperBlock*         pSuperBlock = pMemoryManager->superBlockHead;
    HGbool              anyLeaks    = (HGbool)(pMemoryManager->memoryUsed != sizeof(HGMemory));

    HG_ASSERT (pMemoryManager);

    /*--------------------------------------------------------------------
     * Release memory allocations of all remaining superblocks back to 
     * the operating system.
     *------------------------------------------------------------------*/
    
    while (pSuperBlock)
    {
        SuperBlock* next = pSuperBlock->nextSuperBlock;
                if (pMemoryManager->freeFunc)
                pMemoryManager->freeFunc (pSuperBlock->allocation);
                else
                {
                        HG_ASSERT(pMemoryManager->heapFreeFunc);
                        pMemoryManager->heapFreeFunc (pMemoryManager->userHeap, pSuperBlock->allocation);
                }
        pSuperBlock = next;
    }
 
    /*--------------------------------------------------------------------
     * Release the Memory manager object itself
     *------------------------------------------------------------------*/

        if (pMemoryManager->freeFunc)
            pMemoryManager->freeFunc (pMemoryManager);
        else
        {
                HG_ASSERT(pMemoryManager->heapFreeFunc);
                pMemoryManager->heapFreeFunc (pMemoryManager->userHeap, pMemoryManager);
        }

    /*--------------------------------------------------------------------
     * Report memory leaks
     *------------------------------------------------------------------*/

    return anyLeaks;                                
}

/*-------------------------------------------------------------------*//*!
 * \ingroup API
 * \brief   Returns size of allocated memory block
 * \param   ptr Pointer to memory block (may be \c HG_NULL)
 * \return  Size of the memory block in bytes. Note that this may not
 *          be the size of the original allocation, but includes
 *          alignment and other allocation overhead.
 * \assert  The function asserts in debug build that 'ptr' has been 
 *          allocated from the memory manager. In release build an
 *          invalid msize() call _may_ crash
 *//*-------------------------------------------------------------------*/

size_t HGMemory_msize (
    const void* ptr)
{
/*@-nullptrarith*/
    const Block*    pBlock  = HG_REINTERPRET_CAST(const Block*,ptr) - 1;                /* get the header           */
    HGbool          ok      = (HGbool)(ptr && ((getType(pBlock) & BLOCK_USED) != 0));        /* valid used or fixed block? */
    
    HG_ASSERT(ok);                                                                      /* assert in debug build..  */
        
    return ok ? getBlockSize(pBlock) : 0;                                               /* this takes the header into account */
/*@=nullptrarith*/
}

/*-------------------------------------------------------------------*//*!
 * \ingroup API
 * \brief   Performs a consistency check of the memory manager 
 * \param   pMemoryManager  Pointer to memory manager
 * \note    The consistency check is only executed in the debug build
 *          (raises assertions if the memory manager is broken).
 *//*-------------------------------------------------------------------*/

void HGMemory_checkConsistency (
    const HGMemory* HG_RESTRICT pMemoryManager)
{
    validateFreeList (pMemoryManager);
}

/*-------------------------------------------------------------------*//*!
 * \ingroup API
 * \brief   Returns the number of bytes used by the user's allocations
 * \param   pMemoryManager  Pointer to memory manager
 * \return  Total umber of bytes of memory used by the user's allocations
 * \sa      HGMemory_memReserved()
 *//*-------------------------------------------------------------------*/

size_t HGMemory_memUsed (
    const HGMemory* HG_RESTRICT pMemoryManager)
{
    HG_ASSERT (pMemoryManager != HG_NULL);

    return pMemoryManager->memoryUsed;
}

/*-------------------------------------------------------------------*//*!
 * \ingroup API
 * \brief   Returns the number of bytes reserved by the memory manager
 *          from the operating system
 * \param   pMemoryManager  Pointer to memory manager
 * \return  Number of bytes of memory reserved from the operating system
 * \sa      HGMemory_memUsed()
 *//*-------------------------------------------------------------------*/

size_t HGMemory_memReserved (
    const HGMemory* HG_RESTRICT pMemoryManager)
{
    HG_ASSERT (pMemoryManager != HG_NULL);
    
    return pMemoryManager->memoryReserved;
}

/*-------------------------------------------------------------------*//*!
 * \ingroup API
 * \brief   Returns a boolean indicating whether the allocator
 *          is empty or not
 * \param   pMemoryManager  Pointer to memory manager
 * \return  HG_TRUE if allocator is empty, HG_FALSE otherwise
 * \sa      HGMemory_memReserved()
 *//*-------------------------------------------------------------------*/

HGbool HGMemory_isEmpty (
    const HGMemory* HG_RESTRICT pMemoryManager)
{
    HG_ASSERT (pMemoryManager != HG_NULL);
    
    return (HGbool)(pMemoryManager->memoryReserved == sizeof(HGMemory)); /* only the allocator itself remains */
}
/*----------------------------------------------------------------------*/
