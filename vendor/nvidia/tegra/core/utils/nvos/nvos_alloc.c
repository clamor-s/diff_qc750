/*
 * Copyright (c) 2007 - 2009 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvos_alloc.h"
#include "nvos.h"
#include "nvassert.h"

#if NVOS_TRACE || NV_DEBUG
#undef NvOsAlloc
#undef NvOsFree
#undef NvOsRealloc
#endif

#if NVOS_RESTRACKER_COMPILED

#define NVOS_TRACKER_DIE() NV_ASSERT(!"ResTracker dying!")

typedef NvU64                       Guardband;
typedef struct AllocHeaderRec       AllocHeader;

/* Information about allocations.
 *
 * Structure for user allocations is:
 *
 *   [AllocHeader + PreGuard + <buffer...> + PostGuard]
 */

struct AllocHeaderRec
{
    size_t              size;
    Guardband           preguard;
};


static const Guardband s_preGuard   = 0xc0decafef00dfeedULL;
static const Guardband s_postGuard  = 0xfeedbeefdeadbabeULL;
static const Guardband s_freedGuard  = 0xdeaddeaddeaddeadULL;

static AllocHeader*       GetAllocHeader          (NvU8* userptr);
static NvBool             VerifyPtr               (NvU8* userptr);

/*
 * Returns NV_TRUE if the internal integrity of buffer pointed to by
 * userptr is sane.
 */

NvBool VerifyPtr(NvU8* userptr)
{
    AllocHeader* header = ((AllocHeader*)userptr) - 1;

    if (header->size > 0x7FFFFFFF)
        NvOsDebugPrintf("WARNING: Allocation size exceeds 2 gigs. Possibly compromised allocation (userptr: 0x%08x, size %d)\n", userptr, header->size);        

    if (NvOsMemcmp(&header->preguard, &s_freedGuard, sizeof(Guardband)) == 0)
    {
        NvOsDebugPrintf("Allocation already freed (userptr: 0x%08x)\n", userptr);
        return NV_FALSE;
    }
    if (NvOsMemcmp(&header->preguard, &s_preGuard, sizeof(Guardband)) != 0)
    {
        NvOsDebugPrintf("Allocation pre-guard compromised (userptr: 0x%08x)\n", userptr);
        return NV_FALSE;
    }
    if (NvOsMemcmp((userptr + header->size), &s_postGuard, sizeof(Guardband)) != 0)
    {
        NvOsDebugPrintf("Allocation post-guard compromised (userptr: 0x%08x)\n", userptr);
        return NV_FALSE;
    }
        
    return NV_TRUE;
}

/*
 * Handle userptr-to-AllocHeader mapping.
 */

AllocHeader* GetAllocHeader(NvU8* userptr)
{
    AllocHeader* header = ((AllocHeader*)userptr) - 1;

    if (NvOsMemcmp(&header->preguard, &s_preGuard, sizeof(Guardband)) == 0)
        return header;
    return NULL;
}


/*
 * malloc wrapper.
 */

void *NvOsAllocLeak(size_t size, const char *filename, int line)
{
    void*         rv          = NULL;
    AllocHeader*  allocation;

    if (!NvOsResourceTrackingEnabled())
        return NvOsAllocInternal(size);

    if (size == 0)
        return NULL;

    // Let resource tracker decide if this alloc failed or not.  Used
    // in stress testers for increased test coverage for error
    // conditions.

    if (!NvOsAllocSucceedsSim(NvOsAllocHookType_Alloc))
        return NULL;

    /* Do the actual allocation. */
    allocation = NvOsAllocInternal(sizeof(AllocHeader) + size + sizeof(Guardband));

    if (allocation)
    {
        NvU8*           userptr = (NvU8*)(allocation + 1);

        /* Make sure we can store metadata for the next allocation, or
         * bail out. */

        if (NvOsTrackResource(NvOsResourceType_NvOsAlloc, userptr, filename, line))
        {

            /* Write backreference, write guardbands, and fill only
             * the beginning of buffer with 0xcc (because memsetting
             * newly allocated pages can be slow due to lots of
             * pagefaults and we want to catch the most likely case:
             * someone allocating a struct and assuming the fields are
             * zero in our thin debug malloc). */

            NvOsMemcpy(&allocation->preguard, &s_preGuard,    sizeof(Guardband));
            NvOsMemcpy(userptr + size,        &s_postGuard,   sizeof(Guardband));

            allocation->size = size;

            if (size <= 32)
            {
                NvOsMemset(userptr, 0xcc, size);
            }
            else
            {
                NvOsMemset(userptr, 0xcc, 16);
                NvOsMemset(userptr + size - 16, 0xcc, 16);
            }
            
            NvOsIncResTrackerStat(NvOsResTrackerStat_NumAllocs, 1, NvOsResTrackerStat_NumPeakAllocs);
            NvOsIncResTrackerStat(NvOsResTrackerStat_NumBytes, size, NvOsResTrackerStat_NumPeakBytes);
            NvOsIncResTrackerStat(NvOsResTrackerStat_TrackerBytes, 
                                  sizeof(AllocHeader) + sizeof(Guardband), 
                                  NvOsResTrackerStat_TrackerPeakBytes);            
            rv = userptr;
            NV_ASSERT(VerifyPtr(userptr));
        }
        else
            NvOsFreeInternal(allocation);
    }

    return rv;
}

/*
 * realloc wrapper.
 */

void *NvOsReallocLeak(
                void *userptr,
                size_t size,
                const char *filename,
                int line)
{
    void*       rv = NULL;

    // TODO [kraita 21/Nov/08] What if: Enable -> realloc A -> Disable -> realloc A. Expecting crash and burn.
    if (!NvOsResourceTrackingEnabled())
        return NvOsReallocInternal(userptr, size);

    // TODO there's probably a better way to handle Realloc failures
    if (!NvOsAllocSucceedsSim(NvOsAllocHookType_Realloc))
        return NULL;

    if (!userptr)
        return NvOsAllocLeak(size, filename, line);

    if (size == 0)
    {
        NvOsFreeLeak(userptr, filename, line);
        return NULL;
    }

    if (!NvOsIsTrackedResource(userptr))
    {
        NvOsDebugPrintf("Resource tracking enabled, but realloc received non-tracked pointer!\n");
        NVOS_TRACKER_DIE();
    }

    if (!VerifyPtr(userptr))
    {
        NvOsResourceIsCorrupt(userptr);        
        NVOS_TRACKER_DIE();        
    }   
    else
    {
        AllocHeader* OldHeader = GetAllocHeader(userptr);
        size_t       OldSize   = OldHeader->size;
        AllocHeader* NewPointer;

        /* Realloc and resource tracking database have to be updated
         * automically. Otherwise other threads can allocate released
         * pointer and get the resource tracked in conflict with OS
         * allocs
         */
        NvOsLockResourceTracker();
        NewPointer = NvOsReallocInternal(OldHeader, sizeof(AllocHeader) + size + sizeof(Guardband));
        if (NewPointer)
        {
            rv = (NvU8*)(NewPointer + 1);
            NvOsUpdateResourcePointer(userptr, rv);
        }
        NvOsUnlockResourceTracker();

        NV_ASSERT((NewPointer && rv) || (!NewPointer && !rv));
            
        if (NewPointer)
        {
            /* Update stats first. */
            
            NvOsIncResTrackerStat(NvOsResTrackerStat_NumBytes, 
                                  (NvS64)size - (NvS64)OldSize, 
                                  NvOsResTrackerStat_NumPeakBytes);

            NewPointer->size       = size;

            /* If the size grew, pad max last 16 bytes of the new
             * chunk with 0xcc. */

            if (size > OldSize)
            {
                size_t PadLength = NV_MIN(16, size - OldSize);

                NvOsMemset((NvU8*)rv + size - PadLength, 0xcc, PadLength);
            }

            /* Rewrite post guardband because size has changed. */

            NvOsMemcpy((NvU8*)rv + size, &s_postGuard, sizeof(Guardband));

            NV_ASSERT(GetAllocHeader(rv) == NewPointer);
            NV_ASSERT(VerifyPtr(rv));
        }
    }

    return rv;
}

/*
 * free() wrapper.
 */

void NvOsFreeLeak(void *userptr, const char *filename, int line)
{
    if (!NvOsResourceTrackingEnabled())
    {
        NvOsFreeInternal(userptr);
        return;
    }

    // Not much that the allc fail simulator does with this callback,
    // but let's have it here for completeness.
    if (!NvOsAllocSucceedsSim(NvOsAllocHookType_Free))
        return;

    if (!userptr)
        return;

    if (!NvOsIsTrackedResource(userptr))
    {
        NvOsDebugPrintf("Resource tracking enabled, but free received non-tracked pointer!\n");
        NVOS_TRACKER_DIE();
    }

    if (!VerifyPtr(userptr))
    {
        NvOsResourceIsCorrupt(userptr);        
        NVOS_TRACKER_DIE();        
    }   
    else
    {
        /* Get header struct */
        AllocHeader* header = GetAllocHeader(userptr);
    
        NV_ASSERT(header);

        /* Looks good. Update stats */
    
        NvOsDecResTrackerStat(NvOsResTrackerStat_NumAllocs, 1);
        NvOsDecResTrackerStat(NvOsResTrackerStat_NumBytes, header->size);
        NvOsDecResTrackerStat(NvOsResTrackerStat_TrackerBytes, sizeof(AllocHeader) + sizeof(Guardband));

        /* Write over guards */
        NvOsMemcpy(&header->preguard, &s_freedGuard, sizeof(Guardband));
        NvOsMemcpy((NvU8*)userptr + header->size, &s_freedGuard, sizeof(Guardband));

        /* Finally give up the freed block. 
         * NOTE: TrackedResource must be released before free! Otherwise multi-threaded
         * code can allocate the same pointer again and get resource tracker into
         * inconsistent state.
         */
        NvOsReleaseTrackedResource(userptr);
        NvOsFreeInternal(header);
    }
}

/*
 *  Get the size of an alloc.
 */
size_t NvOsGetAllocSize(void* ptr)
{
    AllocHeader* header = GetAllocHeader(ptr);
    if (header)
        return header->size;
    return 0;
}

/* Need this in the export table, will be called if we mix release application
 *  and debug NvOs.*/
static char message[] = "Release app, no line data"; 
 
void *NvOsAlloc(size_t size)
    { return NvOsAllocLeak(size, message, 0); }
void *NvOsRealloc( void *ptr, size_t size )
    { return NvOsReallocLeak(ptr, size, message, 0); }
void NvOsFree( void *ptr )
    { NvOsFreeLeak(ptr, message, 0); }

#else // NVOS_RESTRACKER_COMPILED

void *NvOsAlloc(size_t size)
{
    return NvOsAllocInternal(size);
}

void *NvOsRealloc(void *ptr, size_t size)
{
    return NvOsReallocInternal(ptr, size);
}

void NvOsFree(void *ptr)
{
    NvOsFreeInternal(ptr);
}

/* Need this in the export table, will be called if we mix debug application
 *  and release NvOs.*/
#if !(NV_DEBUG)
// If NV_DEBUG, we already have declarations.
void *NvOsAllocLeak( size_t size, const char *f, int l );
void *NvOsReallocLeak( void *ptr, size_t size, const char *f, int l );
void NvOsFreeLeak( void *ptr, const char *f, int l );
#endif

void *NvOsAllocLeak(size_t size, const char *f, int l)
    { return NvOsAllocInternal(size); }
void *NvOsReallocLeak( void *ptr, size_t size, const char *f, int l )
    { return NvOsReallocInternal(ptr, size); }
void NvOsFreeLeak( void *ptr, const char *f, int l )
    { NvOsFreeInternal(ptr); }

#endif // NVOS_RESTRACKER_COMPILED
