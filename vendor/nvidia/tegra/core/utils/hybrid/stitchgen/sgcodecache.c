/*------------------------------------------------------------------------
 * 
 * OpenVG
 * -------------------------------
 *
 * (C) 2002-2003 Hybrid Graphics, Ltd.
 * All Rights Reserved.
 *
 * This file consists of unpublished, proprietary source code of
 * Hybrid Graphics, and is considered Confidential Information for
 * purposes of non-disclosure agreement. Disclosure outside the terms
 * outlined in signed agreement may result in irrepairable harm to
 * Hybrid Graphics and legal action against the party in breach.
 *
 *//**
 * \file
 * \brief Cache class for pixel pipeline loops
 *//*-------------------------------------------------------------------*/

#include "sgcodecache.h"
#include "hgdefs.h"
#include "hgint32.h"

#if (HG_COMPILER == HG_COMPILER_RVCT)
#   include "nvos.h"
#   define MEMCPY(source1, source2, size) NvOsMemcpy(source1, source2, size)
#   define MEMSET(source, value, size) NvOsMemset(source, value, size)
#   define MEMCMP(source1, source2, size) NvOsMemcmp(source1, source2, size)
#   define MEMMOVE(destination, source, size) NvOsMemmove(destination, source, size)
#else
#   include <string.h>
#   define MEMCPY(source1, source2, size) memcpy(source1, source2, size)
#   define MEMSET(source, value, size) memset(source, value, size)
#   define MEMCMP(source1, source2, size) memcmp(source1, source2, size)
#   define MEMMOVE(destination, source, size) memmove(destination, source, size)
#endif

#define CACHE_HASH_SIZE SG_CODECACHE_HASH_SIZE

/* Note: Need to alloc cache signatures that there's free space after
   data[] array. */
typedef struct
{
    int     length;
    HGuint8 data[1];
} CacheSignature;


/*-------------------------------------------------------------------*//*!
 * \brief   Single loop entry in the cache
 *//*-------------------------------------------------------------------*/

struct sgCacheEntry_s
{
    sgCacheEntry* prev;     /*!< Pointer to previous entry   */
    sgCacheEntry* next;     /*!< Pointer to next cache entry */
    sgCacheEntry* hashNext; /*!< Next entry in the hash slot */
    HGint32        dataOffset;  /*!< Offset into arena           */
    HGint32        dataSize;    /*!< Bytes of code               */
    HGuint32       timeStamp;   /*!< Last usage time stamp       */
    CacheSignature signature;   /*!< Signature for the data      */
};

HG_HEADER_CT_ASSERT(SGCODECACHE_H, 
                    sizeof(sgCacheEntry) == 24 + sizeof(CacheSignature));

typedef union
{
    void*       ptr;
    const void* cptr;
} ConstCastU;

#if !defined(CC_DISABLE_PROFILING)
#   define INC_STAT(cache, v) (cache->stats.v)++
#else
#   define INC_STAT(cache, v) HG_NULL_STATEMENT
#endif

#define ALIGN_TO_PTR_SIZE(x) ((x)+sizeof(void*)-1) & (~(sizeof(void*)-1))

HG_INLINE int entryStride(int maxSigSize)
{
    return ALIGN_TO_PTR_SIZE(sizeof(sgCacheEntry)+maxSigSize-1);
}

HG_STATICF sgCacheEntry* entryAt(sgCodeCache* cache, int ndx)
{
    void* p = (HGuint8*)cache->entries + ndx*entryStride(cache->maxSigLength);
    HG_ASSERT(((HGuintptr)p & (sizeof(void*)-1)) == 0);
    return (sgCacheEntry*)p;
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Const cast of a void* pointer.
 * \param   ptr Input const pointer.
 * \return  Returns the non-const pointer.
 *//*-------------------------------------------------------------------*/

HG_INLINE void* grConstCast (const void* ptr)
{
    ConstCastU u;
    u.cptr = ptr;
    return u.ptr;
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Function for matching two signatures
 * \param   a   Pointer to first signature
 * \param   b   Pointer to second signature
 * \return  true if signatures match, false otherwise
 *//*-------------------------------------------------------------------*/

HG_INLINE int matchSignatures(const void* aData, int aLength,
                              const void* bData, int bLength)
{
    HG_ASSERT(aData && bData);
    
    if(aLength != bLength)
        return HG_FALSE;

    return !MEMCMP(aData, bData, aLength);
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Finds oldest used cache entry and returns pointer to it
 * \param   cache       Pointer to cache
 * \return  Pointer to oldest cache entry or 0 if cache is empty
 *//*-------------------------------------------------------------------*/

HG_INLINE sgCacheEntry* getOldestUsed(const sgCodeCache* cache)
{
    HG_ASSERT(cache);
    if(cache->firstUsed)
    {
        const sgCacheEntry* oldest = cache->firstUsed;
        const sgCacheEntry* entry  = oldest->next;
        while(entry)
        {
            if(entry->timeStamp < oldest->timeStamp)
                oldest = entry;
            entry = entry->next;
        }
        return (sgCacheEntry*)grConstCast(oldest);
    }
    return 0;
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Computes total free memory of all 'free entries'
 * \param   cache       Pointer to cache
 * \return  Total number of bytes of free memory in the free entries
 *//*-------------------------------------------------------------------*/

HG_INLINE int getFreeMemory(const sgCodeCache* cache)
{
    int totalFree = 0;
    const sgCacheEntry* entry;
    for(entry = cache->firstFree; entry; entry = entry->next)
        totalFree += entry->dataSize;

    return totalFree;
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Performs self-checking of the cache
 * \param   cache       Pointer to cache
 * \note    This function is only implemented in the debug build
 *//*-------------------------------------------------------------------*/


HG_INLINE void selfCheck (const sgCodeCache* cache)
{
#if defined(DEBUG)
    const sgCacheEntry* entry;
    int                  totalAccountedBytes    = getFreeMemory (cache);
    int                  totalAccountedEntries  = 0;

    /* Make sure we have not lost any bytes of memory */
    for(entry = cache->firstUsed; entry; entry = entry->next)
        totalAccountedBytes += entry->dataSize;
    HG_ASSERT (totalAccountedBytes == cache->arenaSize);

    /* Make sure we have not lost any entries */
    for(entry = cache->firstUsed; entry; entry = entry->next)
        totalAccountedEntries++;
    for(entry = cache->firstFree; entry; entry = entry->next)
        totalAccountedEntries++;
    for(entry = cache->firstUnused; entry; entry = entry->next)
        totalAccountedEntries++;
    HG_ASSERT (totalAccountedEntries == cache->entryCount);

#else
    HG_UNREF(cache);
#endif
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Moves an sgCacheEntry from one linked list to another
 * \param   entry       Pointer to cache entry
 * \param   newList     Address of new list head pointer
 * \param   oldList     Address of old list head pointer
 *//*-------------------------------------------------------------------*/

HG_STATICF void changeList(sgCacheEntry* entry, 
                       sgCacheEntry** newList, 
                       sgCacheEntry** oldList)
{
    HG_ASSERT (entry);

    if(entry->prev)
        entry->prev->next = entry->next;
    else
    {
        HG_ASSERT (*oldList == entry);
        *oldList = entry->next;
    }
    if(entry->next)
        entry->next->prev = entry->prev;

    entry->prev = 0;
    entry->next = *newList;
    if (entry->next)
        entry->next->prev = entry;
    *newList = entry;
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Merges a free block with its neighboring free blocks
 * \param   cache       Pointer to cache
 * \param   entry       Pointer to cache entry (MUST BE A FREE BLOCK!)
 *//*-------------------------------------------------------------------*/

HG_STATICF void mergeFreeBlock(sgCodeCache* cache, sgCacheEntry* entry)
{
    /*--------------------------------------------------------------------
     * Perform merging (we do it twice so that we can handle the case
     * where we have free-entry-free, which collapses into a single
     * free entry).
     *------------------------------------------------------------------*/

    int i;
    HG_ASSERT (cache && entry);

    for(i = 0; i < 2; i++)
    {
        sgCacheEntry* a;

        for(a = cache->firstFree; a; a = a->next)
        if(a != entry)
        {
            /* Following block */
            if(a->dataOffset == (entry->dataOffset + entry->dataSize))
            {
                entry->dataSize += a->dataSize;
                changeList(a, &cache->firstUnused, &cache->firstFree);
                break;
            }

            /* Previous block */
            if(entry->dataOffset == (a->dataOffset + a->dataSize))
            {
                a->dataSize += entry->dataSize;
                changeList(entry,&cache->firstUnused,&cache->firstFree);
                entry = a;
                break;
            }
        }
    }
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Removes a used entry from the cache and moves the block
 *          to the free list. If the used entry size is 0 (!), we
 *          move it to the unused list.
 * \param   cache       Pointer to cache
 * \param   entry       Pointer to cache entry
 * \note    Performs free block merging
 *//*-------------------------------------------------------------------*/

HG_INLINE void removeEntry(sgCodeCache* cache, sgCacheEntry* entry)
{
    HG_ASSERT(cache && entry);

    selfCheck(cache);

    if(entry->dataSize == 0)
    {
        changeList(entry, &cache->firstUnused, &cache->firstUsed);
    } else
    {
        changeList(entry, &cache->firstFree, &cache->firstUsed);
        selfCheck(cache);
        mergeFreeBlock(cache, entry);
    }
    selfCheck       (cache);
}

HG_INLINE int hashFunc(const void* sigData, int length)
{
    HG_ASSERT(length >= 1);

    if(length > 4)
    {
        HGuint32  f;

        HG_ASSERT(((HGuintptr)sigData & (sizeof(void*)-1)) == 0);

        f = *((const HGuint32*)sigData);

        return f ^ (hgRol32(f, 5)) ^ (hgRol32(f, 9));
    }

    return *((const HGint8*)sigData);
}

HG_INLINE void hashEntry(sgCacheEntry** hashtab, sgCacheEntry* e)
{
    int slot = hashFunc(&e->signature.data[0], e->signature.length) & (CACHE_HASH_SIZE-1);
    e->hashNext = hashtab[slot];
    hashtab[slot] = e;
}

HG_STATICF void rehash(sgCodeCache* cache)
{
    sgCacheEntry*  e;
    sgCacheEntry** hashtab = cache->hashtab;

    MEMSET(hashtab, 0, sizeof(sgCacheEntry*) * CACHE_HASH_SIZE);

    for(e = cache->firstUsed; e; e = e->next)
        hashEntry(hashtab, e);
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Defragments the cache (moves all used data to be contiguous)
 * \param   cache       Pointer to cache
 *//*-------------------------------------------------------------------*/

HG_INLINE void defrag(sgCodeCache* cache)
{
    HG_ASSERT (cache);
    HG_ASSERT (cache->firstFree);
    
    /*--------------------------------------------------------------------
     * Keep on looping as long as we have more than one free block.
     *------------------------------------------------------------------*/

    while(cache->firstFree->next)
    {
        /*----------------------------------------------------------------
         * Find "earlier" free block (this guarantees that there's a
         * used block after it
         *--------------------------------------------------------------*/

        sgCacheEntry* freeEntry =
            cache->firstFree->dataOffset < cache->firstFree->next->dataOffset ?
            cache->firstFree :
            cache->firstFree->next;

        sgCacheEntry* usedEntry = cache->firstUsed;

        HG_ASSERT (usedEntry);

        for(;;)
        {
            if (usedEntry->dataOffset ==
                (freeEntry->dataOffset + freeEntry->dataSize))
                break;
            usedEntry = usedEntry->next;
        }

        HG_ASSERT (usedEntry);  /* Huh? There *must* be such an entry */


        /*----------------------------------------------------------------
         * We move the loop data of the used entry to the free entry
         * and then "swap" the free and used blocks (the free area is
         * now at the end of the combined memory area). Then we merge
         * the free block with its neighbors.
         *--------------------------------------------------------------*/
        {
            HGuint8* dst = (HGuint8*)(cache->arena) + freeEntry->dataOffset;
            MEMMOVE(dst,
                    (const HGuint8*)(cache->arena) + usedEntry->dataOffset,
                    usedEntry->dataSize);
            cache->iCacheFlush(dst, usedEntry->dataSize);
            usedEntry->dataOffset = freeEntry->dataOffset;
            freeEntry->dataOffset = usedEntry->dataOffset +
                                    usedEntry->dataSize;
            mergeFreeBlock (cache, freeEntry);
        }
        selfCheck (cache);
    }
    
    INC_STAT(cache, nDefrags);
}

/*-------------------------------------------------------------------*//*!
 * \internal
 * \brief   Locates best free entry for 'dataSize' bytes
 * \param   cache       Pointer to cache
 * \param   dataSize    Number of bytes in data
 * \return  Pointer to best free entry or 0 if one cannot be found
 *          (cache must then be either defragmented or entries must
 *          be removed).
 * \note    Uses first-fit policy
 *//*-------------------------------------------------------------------*/

HG_STATICF sgCacheEntry* findFreeEntry(sgCodeCache* cache, int dataSize)
{
    sgCacheEntry* entry;

    for(entry = cache->firstFree; entry; entry = entry->next)
    {
        if(entry->dataSize >= dataSize) /* Valid entry.. */
            return entry;
    }

    return HG_NULL;
}

/*-------------------------------------------------------------------*//*!
 * \brief   Initialize or invalidate a cache structure
 * \param   cache       Pointer to cache
 * \param   mallocF     Function to use for mallocing
 * \param   freeF       Function to use for freeing malloc'd memory
 * \param   arena       Pointer to memory block where loops are stored
 * \param   arenaSize   Number of bytes in the arena block
 * \param   entryCount  Number of entries in 'entries' array
 * \param   maxSignatureSize Maximum size of a cache signature
 * \return  true if everything went OK, false otherwise.  Do not call
 *          sgCodeCache_free for the cache if this function returns
 *          false.
 * \note    'entries' and 'arena' should not be modified by anything
 *          but the sgCodeCache class as long as the cache is used
 * \note    The entryCount should be at least two for the cache
 *          to be operational (doesn't crash if less than two, but
 *          won't be able to store anything).
 *//*-------------------------------------------------------------------*/

HGbool sgCodeCache_init(sgCodeCache*    cache, 
                         sgCCMalloc      mallocF,
                         sgCCFree        freeF, 
                         sgCCICacheFlush cacheFlush,
                         void*            arena, 
                         int              arenaSize, 
                         int              entryCount,
                         int              maxSignatureSize)
{
    int i;

    HG_ASSERT(cache);
    HG_ASSERT(arenaSize >= 0 && entryCount >= 0);
    HG_ASSERT(cacheFlush);
    HG_ASSERT(arena || !arenaSize);

    MEMSET(cache, 0, sizeof(sgCodeCache));

    if(entryCount > 0)
    {
        sgCacheEntry* entries = mallocF(entryStride(maxSignatureSize)*entryCount);

        if(!entries)
            return HG_FALSE;

        MEMSET(entries, 0, entryStride(maxSignatureSize)*entryCount);

        cache->malloc              = mallocF;
        cache->free                = freeF;
        cache->iCacheFlush         = cacheFlush;
        cache->arena               = arena;
        cache->entries             = entries;
        cache->arenaSize           = (HGint32)arenaSize;
        cache->maxSigLength        = maxSignatureSize;
        cache->entryCount          = (HGint32)entryCount;
        cache->firstFree           = cache->entries;
        cache->firstFree->dataSize = arenaSize;

        if(entryCount >= 2)
        {
            cache->firstUnused = entryAt(cache, 1);
            for(i = 1; i < entryCount-1; i++)
                entryAt(cache, i)->next = entryAt(cache, i+1);
        }
    }

    selfCheck (cache);
    return HG_TRUE;
}

void sgCodeCache_free(sgCodeCache* cache)
{
    HG_ASSERT(cache);
    HG_ASSERT(cache->entries);
    cache->free(cache->entries);
}

sgCodeCacheStats sgCodeCache_stats(const sgCodeCache* cache)
{
    return cache->stats;
}

void sgCodeCache_resetStats(sgCodeCache* cache)
{
    MEMSET(&cache->stats, 0, sizeof(sgCodeCacheStats));
}

/*-------------------------------------------------------------------*//*!
 * \brief   Locates a loop from the cache based on its signature
 * \param   cache       Pointer to cache
 * \param   sigData     Pointer to bytes
 * \param   sigLength   Length of signature
 * \return  Pointer to corresponding loop or 0 if no match
 * \note    Also updates internal time stamp of the entry
 *//*-------------------------------------------------------------------*/

const void* sgCodeCache_find(sgCodeCache* cache, const void* sigData, int sigLength)
{
    int slot;
    sgCacheEntry* e;

    HG_ASSERT(cache);
    HG_ASSERT(cache->maxSigLength >= sigLength);

    INC_STAT(cache, nAccesses);

    slot = hashFunc(sigData, sigLength) & (CACHE_HASH_SIZE-1);

    for(e = cache->hashtab[slot]; e; e = e->hashNext)
    {
        if(matchSignatures(&e->signature.data, e->signature.length,
                           sigData, sigLength))
        {
            e->timeStamp = ++(cache->timeStamp);
            INC_STAT(cache, nHits);
            return (const void*)((HGuint8*)(cache->arena) + e->dataOffset);
        }
    }

    /* No match: */
    return HG_NULL;
}

/*-------------------------------------------------------------------*//*!
 * \brief   Creates a new cache entry, copies loop into the arena
 *          and returns a pointer to the copied data
 * \param   cache           Pointer to cache
 * \param   signature       Pointer to signature
 * \param   data            Loop data
 * \param   dataSize        Number of bytes in loop data
 * \return  Pointer to copy of the loop or NULL on failure (loop
 *          cannot be fit into the cache!!!)
 * \note    This function may kill old entries from the cache so no
 *          pointers to other loops should be held anywhere!!
 * \note    Caller of the function must ensure data alignment (keep
 *          dataSize as multiplies of alignment!)
 *//*-------------------------------------------------------------------*/

sgCacheAddResult sgCodeCache_add(sgCodeCache* cache,
                                 const void*  data,
                                 int          dataSize,
                                 const void*  sigData,
                                 int          sigLength)        
{
    sgCacheAddResult res;
    HGbool           needsRehash = HG_FALSE;
    sgCacheEntry*    best;

    HG_ASSERT(cache && sigData && data);
    HG_ASSERT(dataSize >= 0 && sigLength > 0);
    HG_ASSERT(cache->maxSigLength >= sigLength);

    selfCheck (cache);

    res.invalidated = HG_FALSE;
    res.cachedData  = HG_NULL;

    /* Data cannot be fit into the cache under any circumstances? */
    if(dataSize > cache->arenaSize || cache->entryCount < 2)
        return res;

    INC_STAT(cache, nAdds);

    /*--------------------------------------------------------------------
     * Kick out data from the cache until the total free memory
     * exceeds the size of the allocation and we have at least one
     * "unused" block.
     *------------------------------------------------------------------*/

    while(getFreeMemory (cache) < dataSize ||
          !cache->firstUnused || !cache->firstFree)
    {
        sgCacheEntry* oldest = getOldestUsed(cache);
        removeEntry(cache, oldest);
        needsRehash = HG_TRUE;
    }

    HG_ASSERT (cache->firstFree);

    /*--------------------------------------------------------------------
     * Find best block that can accommodate the allocation.  If
     * failed, defragment the cache (after this we *know* we have a
     * free block large enough).
     *------------------------------------------------------------------*/

    best = findFreeEntry(cache, dataSize);
    if (!best)
    {
        needsRehash = HG_TRUE;

        defrag(cache);
        best = findFreeEntry (cache, dataSize);
        HG_ASSERT(best);
    }

    res.invalidated = needsRehash;

    HG_ASSERT(cache->firstUnused);

    /* Rehash query hash if items were removed or otherwise
       modified: */
    if(needsRehash)
        rehash(cache);

    /*--------------------------------------------------------------------
     * Shrink the free block (we need one unused block) and copy the
     * data into the used area. If the free block becomes
     * non-existent, move it to the "non-used" list.
     *------------------------------------------------------------------*/

    {
        sgCacheEntry* block  = cache->firstUnused;
        int           offset = best->dataSize - dataSize;
        HGuint8*      dst    = (HGuint8*)cache->arena + best->dataOffset + offset;

        block->timeStamp  = ++(cache->timeStamp);
        block->dataOffset = best->dataOffset + offset;
        block->dataSize   = dataSize;

        MEMCPY(dst, (const HGuint8*)(data), dataSize);

        block->signature.length = sigLength;
        MEMCPY(&block->signature.data, sigData, sigLength);

        changeList(block, &cache->firstUsed, &cache->firstUnused);

        best->dataSize = offset;

        if(best->dataSize == 0)
            changeList(best, &cache->firstUnused, &cache->firstFree);

        cache->iCacheFlush(dst, dataSize);
        selfCheck(cache);

        hashEntry(cache->hashtab, block);

        res.cachedData = (const void*)dst;
        return res;
    }
}

/*-----------------------------------------------------------------------*/
