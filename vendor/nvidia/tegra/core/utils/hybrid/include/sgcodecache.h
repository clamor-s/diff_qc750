#ifndef __SGCODECACHE_H
#define __SGCODECACHE_H

#if !defined(__SGDEFS_H)
#   include "sgdefs.h"
#endif

#ifndef SG_CODECACHE_HASH_SIZE
#   define SG_CODECACHE_HASH_SIZE 64
#endif

typedef struct sgCacheEntry_s sgCacheEntry;

typedef void* (*sgCCMalloc)(size_t size);
typedef void  (*sgCCFree)(void* ptr);
typedef void  (*sgCCICacheFlush)(void* start, size_t nBytes);

typedef struct
{
    HGint32 nHits;          /*!< # of cache hits  */
    HGint32 nAccesses;      /*!< # of times accessed */
    HGint32 nAdds;          /*!< # of new entry additions */
    HGint32 nDefrags;       /*!< # of defrags */
} sgCodeCacheStats;

typedef struct
{
    /** Pointer to added entry data in the cache */
    const void* cachedData;
    /** HG_FALSE if resident loops in the cache are still valid (e.g.,
        no removes or defrags were done).  HG_TRUE means existing
        pointers to cached items have become invalid because of the
        insert. */
    HGint32     invalidated;
} sgCacheAddResult;

/*-------------------------------------------------------------------*//*!
 * \brief Class for performing code caching
 *//*-------------------------------------------------------------------*/

typedef struct
{
    void*            arena;         /*!< Data arena                 */
    sgCacheEntry*    entries;       /*!< Pointer to entry array     */
    sgCacheEntry*    firstUsed;     /*!< Ptr to first used entry    */
    sgCacheEntry*    firstFree;     /*!< Ptr to first free entry    */
    sgCacheEntry*    firstUnused;   /*!< Ptr to first unused entry  */
    /** Hash table for speeding up cache find */
    sgCacheEntry*    hashtab[SG_CODECACHE_HASH_SIZE];
    sgCCMalloc       malloc;        /*!< Func ptr to malloc memory */
    sgCCFree         free;          /*!< Func ptr to free malloc'd memory */
    sgCCICacheFlush  iCacheFlush;   /*!< Instruction cache flush func */
    HGuint32         timeStamp; /*!< Time stamp of the cache    */
    HGint32          arenaSize; /*!< Arena size in bytes        */
    HGint32          entryCount;    /*!< Total number of entries    */
    HGint32          maxSigLength;  /*!< Maximum length of a signature */
    sgCodeCacheStats stats;
} sgCodeCache;

HG_HEADER_CT_ASSERT(SGCODECACHE_H, sizeof(sgCodeCache) == 
                    64 + SG_CODECACHE_HASH_SIZE*sizeof(void*));

HGbool sgCodeCache_init(sgCodeCache*    cache,
                        sgCCMalloc      malloc,
                        sgCCFree        free,
                        sgCCICacheFlush cacheFlush,
                        void*           arena,
                        int             arenaSize,
                        int             entryCount,
                        int             maxSignatureSize);

void        sgCodeCache_free(sgCodeCache* cache);

const void* sgCodeCache_find(sgCodeCache* cache,
                             const void*   sig,
                             int           sigLength);

sgCacheAddResult sgCodeCache_add(sgCodeCache* cache,
                                 const void*  data,
                                 int          dataSize,
                                 const void*  sig,
                                 int          sigLength);

void sgCodeCache_resetStats(sgCodeCache* cache);
sgCodeCacheStats sgCodeCache_stats(const sgCodeCache* cache);

#endif /* __SGCODECACHE_H */
