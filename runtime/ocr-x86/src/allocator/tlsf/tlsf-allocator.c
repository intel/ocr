/**
 * @brief Implementation of an allocator based on the TLSF allocator
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

// In the past, "pool" ambigiously referred to either the entire domain of an allocator, or to the
// bookkeeping space it needed to manage it.  The word "block" is horribly overloaded and unspecific,
// both internally, and in its implications with regard to collections (aka "islands") of processors.
// Concomitant with introducing some rigorous type checking, more specific terminology is also
// introduced, to wit:
//
// An instance of a TLSF allocator generally manages the entire inventory of dynamically allocatable
// memory at one particular memory domain.  For example, all the non-statically allocated memory in
// one L1 scratch pad; or in L2 scratch pad; or L3; ...; or "MCDRAM" (i.e. an external memory typified
// by smaller size, higher bandwidth, and usually around the same latency as DRAM); or other external
// memory such as GDDR or NVM.  On platforms where the lower-level memory domains are caches rather
// than scratch pads, no TLSF allocator exists for those levels.  But where allocators do exist, they
// either manage their memory inventory as a single pool (as is best for L1, and perhaps L2), or as
// multiple "slice pools" and a single "remnant pool" (as is highly advised for the very large
// external memories utilized by many agents).  The advantage of slicing is that it reduces
// contention when multiple agents are seeking to do malloc/free operations simultaneously.  The
// remnant pool acts as a backstop to "catch" larger blocks, which works especially well if they
// are shared among multiple agents.  The advantage of a single monolithic pool is that it supports
// larger block requests and less fragmentation, but for the higher-up levels, excessive contention
// would make this a major choke point (imagine hundreds of thousands of simultaneous mallocs!).
//
// A "pool" is comprised of the overall bookkeeping structure followed by the "net" space that is
// actually available for parceling into the requested blocks.  The bookkeeping area is now called
// poolHdr_t, though the typedef for that struct is only able to portray the constant-sized components
// and it is necessary to annex onto this struct the storage for the variable-length components.
// (The size of this annex is determined when the pool is instantiated, after which its size is invariant.)
// The space in the pool available for parceling is called the "glebe" (hearkening to parish farmlands
// distributable to peasant farmers at the whim of the parish council).
//
// Blocks in the glebe, be they allocated or free, would better be called "packets", as they are
// comprised of two distinct parts: a [packet] header and the "payload".  However, "block" is rather
// entrenched in the code and comments, so in the context of this source code, we continue to use
// "block" for this construct.  Thus, a block consists of the blkHdr_t header struct, followed by
// the payload, which is the part that the user "sees" after a successful allocation request, and
// which he subsequently frees when he is done with it, at which time the entire block is returned
// to the glebe.
//
// The glebe is initially comprised of one really big free block spanning from the start of the
// glebe right up to a wee sentinel block at the end of the glebe.  The sentinel block exists for
// the purposes of facilitating the coalescent freeing of a block that occurs contiguously just
// before it.  This sentinel is not be confused with the nullBlock, which acts as the end-of-chain
// indicator for free-list chasing operations.
//
// The above constructs are depicted thusly:
//
//        *  Each allocator instance:  (zero to several "slice pools" followed by a "remnant pool"
//           =====================================================================================
//
//     slice 7  slice 6  slice 5  slice 4  slice 3  slice 2  slice 1  slice 0       remnant
//   +--------+--------+--------+--------+--------+--------+--------+--------+----------------------+
//   |        |        |        |        |        |        |        |        |                      |
//   |        |        |        |        |        |        |        |        |                      |
//   |        |        |        |        |        |        |        |        |                      |
//   +--------+--------+--------+--------+--------+--------+--------+--------+----------------------+
//   .        .
//   .        .     *  Every pool, including the remnant, is comprised of the following (not to scale):
//   .        .        ================================================================================
//   .        .
//   .        . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
//   .                                                                                      .
//   .                                                                                      .
//   . poolHdr_t      pool header annex      glebe                                          .
//   +--------------+----------------------+------------------------------------------------+
//   |              |                      |                                                |
//   | see notes at | see notes at         |                                                |
//   | poolHdr_t    | poolHdr_t            |                                                |
//   | typedef      | typedef              |                                                |
//   | for          | for                  |                                                |
//   | contents     | contents             |                                                |
//   |              |                      |                                                |
//   +--------------+----------------------+------------------------------------------------+
//                                         .                                                .
//                                         .                                                .
//                                         .                                                .
//                                         .    *  The glebe contains the blocks:           .
//   . . . . . . . . . . . . . . . . . . . .       ==============================           .
//   .                                                                                      .
//   .                                                                                      .
//   . used block    free block    used block        used blk  free block        used block .
//   +-------------+-------------+-----------------+---------+-----------------+------------+
//   |             |             |                 |         |                 |            |
//   |             |             |                 |         |                 |            |
//   +-------------+-------------+-----------------+---------+-----------------+------------+
//   .             .                                         .                 .
//   .             .                                         .                 .
//   .             .   * Blocks have a header, then space:   .                 .
//   .             .     =================================   .                 .
//   .             .                                         .                 .
//   . used block: . . . . . . . . . . . .         . . . . . . free block:     . . . . . . .
//   .                                   .         .                                       .
//   . blkHdr_t   payload                .         . blkHdr_t   free space          blkLen .
//   +----------+------------------------+         +----------+------------------------+---+
//   |          |                        |         |          |                        |   |
//   | see      |                        |         | see      |                        |   |
//   | blkHdr_t | user-visible datablock |         | blkHdr_t |                        |   |
//   | typedef  |                        |         | typedef  |                        |   |
//   |          |                        |         |          |                        |   |
//   +----------+------------------------+         +----------+------------------------+---+
//
//
// Terminology has also been clarified in regard to the usage of "previous" and "next".  Formerly,
// these terms sometimes referred to relative positions along a linked lists in some places, versus
// relative absolute location along the continuum of glebe address space in other places.  To make
// this more clear, for the list usage, "Previous" has been changed to "BkwdLink" and "Next" has
// been changed to "FrwdLink"; and for the spatial adjacency context, "Previous" has become "PrevNbr"
// and "Next" has become "NextNbr".
//
// For example:
//
//   NOTES:
//
//   *  "U" means Used block; "F" means Free block.  It is impossible for there to be two free blocks in a row.
//   *  NextNbr and PrevNbr are NOT actual fields in the block headers.  They are derived from fields that are there.
//      We use "access methods" to get and set these values, as we do for real header values.
//   *  The first and last blocks depicted are pseudo-blocks, i.e. sentinels, always present, and always marked Used.
//   *  The head of the free list comes from the pool header, and there is one free list for each "bucket" of free
//      block sizes.  But only TWO free lists are depicted, to keep this simple diagram from getting messy.  The
//      free list ends at the null block, also a pseudo-block.  Forward pointer from the last real block in the list
//      points to null block, but backward pointer from null block is undefined (since there is only ONE null block
//      serving as the termination sentinel for ALL free lists).  Likewise backward pointer from the first real
//      block points to the null block, but forward pointer from the null block is undefined.  Only the defined
//      pointers are depicted below.  End-of-chain, pointing into null block, is depicted as -->x.  In addition,
//      the pool header provides the head pointer to the free list, but there is no need to keep the tail.
//
//
//        +-------------------------------------------------------------------------+
//        |  poolHdr_t:                                                             |
//        |                                                                         |
//        |  Free block buckets.  Here is where the availBlkListHead comes from     |
//        |                 *                                             *         |
//        +-----------------|---------------------------------------------|---------+
//                          |                                             |
//                          +------------------------------+              +------+
//                                                         |                     |
//                        +----------------------------+   |                     |
//                        |                            |   |                     |
//            +---+      +|--+      +---+      +---+   |  +|--+      +---+      +|--+      +---+      +---+
//            |   |      |v  |      |   |      |   |   |  |v  |      |   |      |v  |      |   |      |   |
//  FwrdLink: |   |   x<--*  |      |   |      |   |   +---*  |      |   |   x<--*  |      |   |      |   |
//            |   |      |   |      |   |      |   |      |   |      |   |      |   |      |   |      |   |
//  NextNbr:  |*--------->*--------->*--------->*--------->*--------->*--------->*--------->*--------->   |
//            |   |      |   |      |   |      |   |      |   |      |   |      |   |      |   |      |   |
//            | U |      | F |      | U |      | U |      | F |      | U |      | F |      | U |      | U |
//            |   |      |   |      |   |      |   |      |   |      |   |      |   |      |   |      |   |
//  PrevNbr:  |*<---------*<---------*<---------*<---------*<---------*<---------*<---------*<---------*  |
//            |   |      |   |      |   |      |   |      |   |      |   |      |   |      |   |      |   |
//            |   |      |   |      |   |      |   |      |   |      |   |      |   |      |   |      |   |
//  BkwdLink: |   |      |*  |      |   |      |   |   x<--*  |      |   |   x<--*  |      |   |      |   |
//            |   |      ||  |      |   |      |   |      |^  |      |   |      |   |      |   |      |   |
//            +---+      +|--+      +---+      +---+      +|--+      +---+      +---+      +---+      +---+
//                        |                                |
//                        +--------------------------------+
//
// How initialization/termination of shared allocators is performed:
//
//     Definition:  anchor:  the "zero'th" agent of the collection of agents that share an allocator.  E.g. the
//     L4 is shared by all agents of a Chip, so the anchor agent is Block 0, Unit 0 of the current Chip.
//
//     tlsfBegin:  Only the anchor can run tlsfBegin; all other agents have to wait for the anchor to do it.
//     Also, all agents need to increment a counter (needed by tlsfFinish).  So implementation is that all
//     agents except the anchor wait for useCount1 to be non-zero before doing their tlsfBegin work (which
//     is nothing; there is no work for them to do except to bump that counter).  The anchor has to call
//     begin on the mem-target, and bump that counter.
//
//     tlsfStart: works much the same as tlsfBegin, except that it counts up useCount2; and except that the
//     non-anchors have to copy over a few variables from the anchor's allocator_t struct to their own.
//
//     tlsfStop: All agents count down the useCount2.  Non-anchor agents are then done.  The anchor has to
//     wait for that counter to reach zero, and then it calls stop on the mem-target.
//
//     tlsfFinish: All agents count down the useCount1.  Non-anchor agents are then done.  The anchor has to
//     wait for that counter to reach zero, and then it calls finish on the mem-target.
//
//     tlsfDestruct:  TODO!
//
//     The wait operations are presently implemented as while loop spins.  This might be really rotten
//     on FSIM, wasting lots of simulation time; and on real hardware it would waste power.  Therefore,
//     this is a FIXME!  Team advice solicited:  how should this be done?
//

#include "ocr-config.h"
#ifdef ENABLE_ALLOCATOR_TLSF

#include "ocr-hal.h"
#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-runtime-types.h"
#include "ocr-sysboot.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"
#include "tlsf-allocator.h"
#include "allocator/allocator-all.h"
#ifdef HAL_FSIM_CE
#include "rmd-map.h"
#endif

#define DEBUG_TYPE ALLOCATOR

//#ifndef ENABLE_BUILDER_ONLY

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#include "ocr-statistics-callbacks.h"
#endif

#ifdef ENABLE_VALGRIND
#include <valgrind/memcheck.h>
// The distinction between VALGRIND_xxx and VALGRIND_xxx1 used to be that the former supported
// the last few bytes of the blkHdr_t being used as the first part of the payload of in-use
// data blocks.  That is no longer the case, as that location now is used to hold the info
// necessary to be able to free the block back to the right pool, no matter who calls free.
//#define VALGRIND_DEFINED(addr) VALGRIND_MAKE_MEM_DEFINED((addr), sizeof(blkHdr_t) - sizeof(tlsfSize_t))
//#define VALGRIND_NOACCESS(addr) VALGRIND_MAKE_MEM_NOACCESS((addr), sizeof(blkHdr_t) - sizeof(tlsfSize_t))
#define VALGRIND_DEFINED(addr) VALGRIND_MAKE_MEM_DEFINED((addr), sizeof(blkHdr_t))
#define VALGRIND_NOACCESS(addr) VALGRIND_MAKE_MEM_NOACCESS((addr), sizeof(blkHdr_t))
#define VALGRIND_DEFINED1(addr) VALGRIND_MAKE_MEM_DEFINED((addr), sizeof(blkHdr_t))
#define VALGRIND_NOACCESS1(addr) VALGRIND_MAKE_MEM_NOACCESS((addr), sizeof(blkHdr_t))
#else
#define VALGRIND_DEFINED(addr)
#define VALGRIND_NOACCESS(addr)
#define VALGRIND_DEFINED1(addr)
#define VALGRIND_NOACCESS1(addr)
#endif

/******************************************************/
/* OCR ALLOCATOR TLSF IMPLEMENTATION                  */
/******************************************************/

/*
 * Configuration constants
 */

// As a stake in the ground to clean up the hard-to-read code with respect to alignment generality,
// ALL structures -- poolHdr_t struct, blkHdr_t structs, and user payloads -- will be on 8-byte
// alignment boundaries.
#define ALIGNMENT 8LL

/*
 * Number of subdivisions in the second-level list
 * Can be 2 to 5; 6 if slAvailOrNot is changed from u32 to u64.
 * 4 is the "sweet spot";  less results in more fragmentation;
 * more results in more pool header overhead.
 */
#define SL_COUNT_LOG2 4LL

/*
 * Support allocations/memory of size up to (1 << FL_MAX_LOG2) * ALIGNMENT
 * For 64KB memory: 13 (at ALIGNMENT == 8)
 * For  1MB memory: 17 (at ALIGNMENT == 8)
 * For  1GB memory: 27 (at ALIGNMENT == 8)
 * For  8EB memory: 60 (at ALIGNMENT == 8)
 */
#define FL_MAX_LOG2 60LL

/*
 * Some computed values:
 *  - SL_COUNT: Number of buckets in each SL list
 *  - FL_COUNT_SHIFT: For the first SL_COUNT, we do not need to maintain
 *  first level lists (since they all go in the "0th" one.
 *  - ZERO_LIST_SIZE: Size under which we are in SL 0
 */
enum computedVals {
    SL_COUNT        = (1LL << SL_COUNT_LOG2),
    FL_COUNT_SHIFT  = (SL_COUNT_LOG2),
    ZERO_LIST_SIZE  = (1LL << FL_COUNT_SHIFT),
};

/*
 * Header of a block (allocated or free) of memory.
 *
 *
 * Free block:  (freeBlkHdr_t)
 *
 *   u64 oFreeBlkBkwdLink;  <-- Conceptually a LINK in the backward direction to
 *      previous free block.  But to reduce confusion about address map scopes,
 *      implemented as an offset from the start of the pool to the block.  Can't
 *      be 0 or 1, as these would indicate that THIS block is occupied.
 *   u64 payloadSize;     <-- Size of this free block NOT including
 *      the size of this header.
 *   u64 oFreeBlkFrwdLink;  <-- Conceptually a LINK in the forward direction to
 *       next free block.  Implemented as an offset from the start of the pool,
 *       like oFreeBlkBkwdLink.  This field aligned to ALIGNMENT bytes.
 *   Immediately after this header:
 *       <FREE SPACE>
 *   u64 payloadSize; (used by contiguously next block); must be at the very end
 *
 *
 * Used block:  (usedBlkHdr_t)
 *
 *   u64 aggregatedFreeBlockIndicators;
 *      - bit 0:  whether the contiguously previous block is free (1) or not (0)
 *      - the rest MUST be 0 to indicate that THIS block is occupied.
 *   u64 payloadSize;     <-- Size of this in-use block NOT including
 *      the size of this header. I.e., size of the "payload".
 *   u64 poolHeaderDescr;
 *      - low 3 bits index to the type of allocator in use.  This TLSF
 *        allocator is one of those index values, and other [potential]
 *        allocators are other values.  This is used to index into the
 *        appropriate "free" function.
 *      - remaining bits are the offset to the poolHdr_t from which the
 *        block was taken.
 *
 *
 * Generic block -- A block the type of which is either in need of determination, or
 * for which the specific type does not presently matter:  (gnrcBlkHdr_t)
 *
 *   bit 0:     isPrevNbrBlkFree:  <--        1 = Free; 0 = In Use
 *   bits 1:63  isThisBlkFree:     <-- non-zero = Free; 0 = In Use
 *   u64        payloadSize;       <-- Size of this free block NOT including this header.
 *   u64        typeDependentInfo; <-- Depends on if the block is in use or free.
 *
 * */
typedef struct blkHdr_t {
#ifdef ENABLE_ALLOCATOR_CHECKSUM
    u64 checksum;                           // Checksum, for detecting block header corruption.
#endif
    union {
        u64 oFreeBlkBkwdLink;               // If THIS block header is for a free block, this value is defined.
        u64 aggregatedFreeBlockIndicators;  // Handy for initializing these fields together.
        struct {
            u64 isPrevNbrBlkFree:1;         // zero == in use;  non-zero == free
            u64 isThisBlkFree:63;           // zero == in use;  non-zero == free
        };
    };
    u64 payloadSize;                        // Relevant for both free and in-use blocks.
    union {
        u64 oFreeBlkFrwdLink;               // Relevant for free blocks.
        u64 poolHeaderDescr;                // Relevant for in-use blocks.
    };
} blkHdr_t;

typedef void blkPayload_t;   // Strongly type-check the ptr-to-void that comprises the ptr-to-payload results.

/* The pool (containing the bitmaps and the lists of free blocks.
 *
 * This struct has been reordered to attain the goal of allowing the size of the
 * large 2D "blocks" array to be kept as small as possible.  This is necessary to
 * keep from wasting precious L1 on buckets that aren't even needed.  Moreover, the
 * 1D "slAvailOrNot" array is changed to an additional column of the "blocks" array
 * so that its length too can be minimized.
 */
typedef struct {
#ifdef ENABLE_ALLOCATOR_CHECKSUM
    u64 annexChecksum;  // Checksum, for detecting pool header annex corruption.
    u64 checksum;       // Checksum, for detecting pool header corruption.
#endif
    u32 lock;           // Lock to serialize shared access to pool's management data structures.
    u32 flCount;        // Number of first-level buckets.  This is invariant after constructor runs.
    u32 offsetToGlebe;  // Offset in bytes from start of this struct to the start of the glebe.
    u32 currSliceNum;   // Round-robin counter for slice assignment (only used from poolHdr_t of remnant).
    u64 flAvailOrNot;   // bitmap that indicates the presence (1) or absence (0) of free blocks in blocks[i][*]
    blkHdr_t nullBlock; // Used to mark NULL blocks.  (This contains three u64 elements.)
    // Variably-sized elements annexed onto the end of the above:
    // u32        slAvailOrNot[flCount];            // Second level bitmaps
    // <one u32 of padding, if flCount == odd>
    // u32 or u64 availBlocks[flCount][SL_COUNT];   // Index into mainBlock for free list of appropriately-sized blocks.
    // blkHdr_t   glebeHdr;                         // Main memory block (this needs to be aligned and
                                                    // must be last before the net pool space to be managed)
    // <the "glebe", i.e. the net pool space to manage>
    // blkHdr_t   sentinel;                         // Trailing fake in-use datablock, to act as bookend.
} poolHdr_t;

// The following function exist for the purposes of abstracting load and store operations
// so that they can expand to appropriate (different) code for different levels of the
// memory hierarchy on FSIM-CE (and FSIM-XE, if any allocators run on XEs, which is not
// presently the case at the time of this writing).  Ultimately, FSIM will probably be
// modified to do this seamlessly, but presently, the CPU load/store instructions only
// work if the data is in L1.  Some poolHdr_t and blkHdr_t structs are in other memory
// levels.

// FLS: Find last set: position of the MSB set to 1
#define FLS fls64

// Usually no need of any fences on x86 for this.

// If we were C++, the following would be "access methods", to "private" data of poolHdr_t class:

#ifdef ENABLE_ALLOCATOR_CHECKSUM
static inline u32 GET_offsetToGlebe (poolHdr_t * pPool);
static inline u64 calcChecksum (void * addr, u32 len) {
    u32 i;
    u64 checksum = 0xFeedF00dDeadBeefLL;
    u64 shiftCount = 3;
    for (i = sizeof(u64); i < len; i += sizeof(u64)) {
        checksum ^= *((u64 *) (((u64) addr) + i)) >> shiftCount;
        checksum ^= *((u64 *) (((u64) addr) + i)) << (64 - shiftCount);
        shiftCount += 3;
        if (shiftCount == 60) shiftCount = 3;
    }
    return checksum;
}

static inline void setChecksum (void * addr, u32 len) {
    *((u64 *) addr) = calcChecksum (addr, len);
}

static inline void checkChecksum (void * addr, u32 len, u32 linenum, char * structType) {
    if (calcChecksum(addr, len) != *((u64 *) addr)) {
        DPRINTF (DEBUG_LVL_WARN, "Checksum failure in pool data structure %s, detected at %s line %d\n", structType, __FILE__, linenum);
        u32 i;
        for (i = 0; i < len; i += sizeof(u64)) {
            DPRINTF(DEBUG_LVL_WARN, "    0x%lx = 0x%lx\n", (((u64) addr) + i), *((u64 *) (((u64) addr) + i)));
        }
        ASSERT(0);
    }
}
#else
#define setChecksum(addr, len)
#define checkChecksum(addr, len, linenum, structType)
#endif

static inline u32 GET_flCount (poolHdr_t * pPool) {
    u32 temp;
    GET32(temp,((u64)(&(pPool->flCount))));
    return temp;
}

static inline u32 GET_lock (poolHdr_t * pPool) {
    u32 temp;
    GET32(temp,((u64)(&(pPool->lock))));
    return temp;
}

static inline u32 GET_offsetToGlebe (poolHdr_t * pPool) {
    u32 temp;
    GET32(temp,((u64)(&(pPool->offsetToGlebe))));
    return temp;
}

static inline blkHdr_t * GET_glebeAsInitialBlock (poolHdr_t * pPool) {
    return ((blkHdr_t *) (((u64) pPool) + ((u64) GET_offsetToGlebe(pPool))));
}

static inline u32 GET_currSliceNum (poolHdr_t * pPool) {
    u32 temp;
    GET32(temp,((u64)(&(pPool->currSliceNum))));
    return temp;
}

static inline u64 GET_flAvailOrNot(poolHdr_t * pPool) {
    u64 temp;
    GET64(temp,((u64)(&(pPool->flAvailOrNot))));
    return temp;
}

static inline u64 GET_slAvailOrNot (poolHdr_t * pPool, u32 firstLvlIdx) {
    u32 temp;
    GET32(temp,(((u64)(pPool))+sizeof(poolHdr_t)+(firstLvlIdx*sizeof(u32))));
    return (u64)temp;
}

static inline blkHdr_t * GET_availBlkListHead (poolHdr_t * pPool, u32 firstLvlIdx, u32 secondLvlIdx) {
    u32 flCount = GET_flCount(pPool);
    if (flCount <= 26) {
        // Use 32-bit offsets for avail_blocks values when full pool size is small enough.
        flCount += flCount&1;  // If flCount is odd, we need to skip over one DWORD of padding to reach the availBlkListHead array.
        u32 temp;
        GET32(temp,(((u64)(pPool))+sizeof(poolHdr_t)+(flCount*sizeof(u32))+((firstLvlIdx*SL_COUNT+secondLvlIdx)*sizeof(u32))));
        return (blkHdr_t *) (((u64) pPool) + ((u64) temp));
    } else {
        // Use 64-bit offsets for avail_blocks for large memory pools.
        flCount += flCount&1;  // If flCount is odd, we need to skip over one DWORD of padding to reach the availBlkListHead array.
        u64 temp;
        GET64(temp,(((u64)(pPool))+sizeof(poolHdr_t)+(flCount*sizeof(u32))+((firstLvlIdx*SL_COUNT+secondLvlIdx)*sizeof(u64))));
        return (blkHdr_t *) (((u64) pPool) + temp);
    }
}

static inline void SET_flCount (poolHdr_t * pPool, u32 value) {
    SET32((u64) (&(pPool->flCount)), value);
}

static inline void SET_lock (poolHdr_t * pPool, u32 value) {
    SET32((u64) (&(pPool->lock)), value);
}

static inline void SET_offsetToGlebe (poolHdr_t * pPool, u32 value) {
    SET32((u64) (&(pPool->offsetToGlebe)), value);
}

static inline void SET_currSliceNum (poolHdr_t * pPool, u32 value) {
    SET32((u64) (&(pPool->currSliceNum)), value);
}

static inline void SET_flAvailOrNot (poolHdr_t * pPool, u64 value) {
    SET64((u64) (&(pPool->flAvailOrNot)), value);
}

static inline void SET_nullBlock_pFreeBlkBkwdLink (poolHdr_t * pPool, blkHdr_t * pBlk) {
    blkHdr_t * pNullBlock = &(pPool->nullBlock);
    checkChecksum(&pNullBlock->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    u64 locToWrite = (u64) &(pNullBlock->oFreeBlkBkwdLink);
    SET64(locToWrite, (((u64) pBlk) - ((u64) pPool)));
    setChecksum(&pNullBlock->checksum, sizeof(blkHdr_t));
}

static inline void SET_nullBlock_payloadSize (poolHdr_t * pPool, u64 value) {
    blkHdr_t * pNullBlock = &(pPool->nullBlock);
    checkChecksum(&pNullBlock->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    u64 locToWrite = (u64) &(pNullBlock->payloadSize);
    SET64(locToWrite, value);
    setChecksum(&pNullBlock->checksum, sizeof(blkHdr_t));
}

static inline void SET_nullBlock_pFreeBlkFrwdLink (poolHdr_t * pPool, blkHdr_t * pBlk) {
    blkHdr_t * pNullBlock = &(pPool->nullBlock);
    checkChecksum(&pNullBlock->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    u64 locToWrite = (u64) &(pNullBlock->oFreeBlkFrwdLink);
    SET64(locToWrite, (((u64) pBlk) - ((u64) pPool)));
    setChecksum(&pNullBlock->checksum, sizeof(blkHdr_t));
}

static inline void SET_slAvailOrNot (poolHdr_t * pPool, u32 firstLvlIdx, u64 value) {
    ASSERT (value <= 0xFFFFFFFFLL);
    u32 temp=(u32)value;
    SET32((((u64)(pPool))+sizeof(poolHdr_t)+(firstLvlIdx*sizeof(u32))),temp);
}

static inline void SET_availBlkListHead (poolHdr_t * pPool, u32 firstLvlIdx, u32 secondLvlIdx, blkHdr_t * pBlk) {
    u64 value = ((u64) pBlk) - ((u64) pPool);
    u32 flCount = GET_flCount(pPool);
    if (flCount <= 26) {
        // Use 32-bit offsets for avail_blocks values when full pool size is small enough.
        flCount += flCount&1;  // If flCount is odd, we need to skip over one DWORD of padding to reach the availBlkListHead array.
        ASSERT (value <= 0xFFFFFFFFLL);
        SET32((((u64) pPool)+sizeof(poolHdr_t)+(flCount*sizeof(u32))+((firstLvlIdx*SL_COUNT+secondLvlIdx)*sizeof(u32))), (u32) value);
    } else {
        // Use 64-bit offsets for avail_blocks for large memory pools.
        flCount += flCount&1;  // If flCount is odd, we need to skip over one DWORD of padding to reach the availBlkListHead array.
        SET64((((u64) pPool)+sizeof(poolHdr_t)+(flCount*sizeof(u32))+((firstLvlIdx*SL_COUNT+secondLvlIdx)*sizeof(u64))), value);
    }
}

// If we were C++, the following would be "access methods", to "private" data of various blkHdr_t classes:

static inline u64 GET_aggregatedFreeBlockIndicators (blkHdr_t * pBlk) {
    checkChecksum(&pBlk->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    u64 temp;
    GET64(temp,((u64)(&(pBlk->aggregatedFreeBlockIndicators))));
    return temp;
}

static inline bool GET_isThisBlkFree (blkHdr_t * pBlk) {
    checkChecksum(&pBlk->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    return (GET_aggregatedFreeBlockIndicators((blkHdr_t *) pBlk) & (~1LL)) != 0;
}

static inline blkHdr_t * GET_pFreeBlkBkwdLink (poolHdr_t * pPool, blkHdr_t * pBlk) {
    checkChecksum(&pBlk->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    ASSERT (GET_isThisBlkFree(pBlk));   // Should NOT try to access oFreeBlkBkwdLink from a header of an in-use block.
    u64 temp;
    u64 locToRead = (u64) (&(pBlk->oFreeBlkBkwdLink));
    GET64(temp, locToRead);
    return (blkHdr_t *) (((u64) pPool) + temp);
}

static inline u64 GET_payloadSize (blkHdr_t * pBlk) {
    checkChecksum(&pBlk->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    u64 temp;
    GET64(temp,((u64)(&(pBlk->payloadSize))));
    return temp;
}

static inline blkHdr_t * GET_pFreeBlkFrwdLink (poolHdr_t * pPool, blkHdr_t * pBlk) {
    checkChecksum(&pBlk->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    ASSERT (GET_isThisBlkFree(pBlk));   // Should NOT try to access oFreeBlkFrwdLink from a header of an in-use block.
    u64 temp;
    u64 locToRead = (u64) (&(pBlk->oFreeBlkFrwdLink));
    GET64(temp, locToRead);
    return (blkHdr_t *) (((u64) pPool) + temp);
}

static inline u64 GET_poolHeaderDescr (blkHdr_t * pBlk) {
    checkChecksum(&pBlk->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    ASSERT (!GET_isThisBlkFree(pBlk));   // Should NOT try to access poolHeaderDescr from a header of a free block.
    u64 temp;
    GET64(temp,((u64)(&(pBlk->poolHeaderDescr))));
    return temp;
}

static inline bool GET_isPrevNbrBlkFree (blkHdr_t * pBlk) {
    checkChecksum(&pBlk->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    if(GET_isThisBlkFree(pBlk)) {
        return false;  // If THIS block is free, it would have been combined with the spatially contiguous preceeding
                       // block if that block was also free.  Therefore, it must NOT be free.
    } else {
        return (GET_aggregatedFreeBlockIndicators((blkHdr_t *) pBlk) & 1LL) != 0;
    }
}

static inline void SET_pFreeBlkBkwdLink (poolHdr_t * pPool, blkHdr_t * pBlk, blkHdr_t * pPrevBlk, bool overrideAsserts) {
    checkChecksum(&pBlk->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    if (! overrideAsserts) {
        ASSERT (GET_isThisBlkFree(pBlk));       // Should NOT try to access oFreeBlkBkwdLink from a header of an in-use block.
    }
    u64 locToWrite = (u64) (&(pBlk->oFreeBlkBkwdLink));
    SET64(locToWrite, (((u64) pPrevBlk) - ((u64) pPool)));
    setChecksum(&pBlk->checksum, sizeof(blkHdr_t));
}

static inline void SET_payloadSize (blkHdr_t * pBlk, u64 value) {
    checkChecksum(&pBlk->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    SET64((u64) (&(pBlk->payloadSize)),value);
    setChecksum(&pBlk->checksum, sizeof(blkHdr_t));
}

static inline void SET_pFreeBlkFrwdLink (poolHdr_t * pPool, blkHdr_t * pBlk, blkHdr_t * pNextBlk) {
    checkChecksum(&pBlk->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    ASSERT (GET_isThisBlkFree(pBlk));       // Should NOT try to access oFreeBlkFrwdLink from a header of an in-use block.
    ASSERT (GET_isThisBlkFree(pNextBlk));   // Should NOT set oFreeBlkFrwdLink of this block to point to an in-use block.
    u64 locToWrite = (u64) (&(pBlk->oFreeBlkFrwdLink));
    SET64(locToWrite, (((u64) pNextBlk) - ((u64) pPool)));
    setChecksum(&pBlk->checksum, sizeof(blkHdr_t));
}

static inline void SET_aggregatedFreeBlockIndicators (blkHdr_t * pBlk, u64 value) {
    checkChecksum(&pBlk->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    SET64((u64) (&(pBlk->aggregatedFreeBlockIndicators)), value);
    setChecksum(&pBlk->checksum, sizeof(blkHdr_t));
}

static inline void SET_poolHeaderDescr (blkHdr_t * pBlk, u64 value) {
    checkChecksum(&pBlk->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    ASSERT (!GET_isThisBlkFree(pBlk));   // Should NOT try to access poolHeaderDescr from a header of a free block.
    SET64((u64) (&(pBlk->poolHeaderDescr)), value);
    setChecksum(&pBlk->checksum, sizeof(blkHdr_t));
}

static inline void SET_isPrevNbrBlkFree (blkHdr_t * pBlk, bool value) {
    checkChecksum(&pBlk->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    ASSERT(!GET_isThisBlkFree(pBlk)); /* SET_isPrevNbrBlkFree can only be called on used block */
    u64 temp = GET_aggregatedFreeBlockIndicators(pBlk);
    temp = (temp & (~1LL)) | (value ? 1LL : 0LL);
    SET_aggregatedFreeBlockIndicators(pBlk, temp);
    setChecksum(&pBlk->checksum, sizeof(blkHdr_t));
}

static void markPrevNbrBlockUsed(blkHdr_t * pBlk) {
    checkChecksum(&pBlk->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    SET_isPrevNbrBlkFree(pBlk, 0);
    setChecksum(&pBlk->checksum, sizeof(blkHdr_t));
}

static void markPrevNbrBlockFree(blkHdr_t * pBlk) {
    checkChecksum(&pBlk->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    SET_isPrevNbrBlkFree(pBlk, 1);
    setChecksum(&pBlk->checksum, sizeof(blkHdr_t));
}

// A whole bunch of internal methods used only in this file.

#define myStaticAssert(e) extern char (*myStaticAssert(void))[sizeof(char[1-2*!(e)])]

#define _NULL 0ULL

myStaticAssert(ALIGNMENT == 8LL);
myStaticAssert((ALIGNMENT-1) == POOL_HEADER_TYPE_MASK);
myStaticAssert(SL_COUNT_LOG2 < 5);
myStaticAssert(FL_MAX_LOG2 <= 64);

// Some assertions to make sure things are OK
#ifdef ENABLE_ALLOCATOR_CHECKSUM
myStaticAssert(sizeof(blkHdr_t) == 4*sizeof(u64));
#else
myStaticAssert(sizeof(blkHdr_t) == 3*sizeof(u64));
#endif
myStaticAssert(sizeof(blkHdr_t) % ALIGNMENT == 0);
myStaticAssert(sizeof(poolHdr_t) % ALIGNMENT == 0);
myStaticAssert(offsetof(poolHdr_t, nullBlock) % ALIGNMENT == 0);
myStaticAssert(ZERO_LIST_SIZE == SL_COUNT);
myStaticAssert(sizeof(char) == 1);

/* Some contants */
static const u64 GoffsetFromBlkHdrToPayload = (sizeof(blkHdr_t));
static const u64 GminBlockSizeIncludingHdr  = sizeof(blkHdr_t) + sizeof(u64);
static const u64 GminPayloadSize = sizeof(blkHdr_t);

static u32 myffs(u64 val) {
    return FLS(val & (~val + 1LL));
}

// More utility functions for blocks.

// Given the address of the block's header (at the start of the block), return the address of the payload.
static inline blkPayload_t * payloadAddressForBlock(blkHdr_t * pBlk) {    // Return address of block's payload, given address of header.
    return ((blkPayload_t *) (((u64) pBlk) + sizeof(blkHdr_t)));
}

static inline blkHdr_t * mapPayloadAddrToBlockAddr(blkPayload_t * payload) {  // Return address of block's header, given address of payload.
    return ((blkHdr_t *) (((u64) payload) - sizeof(blkHdr_t)));
}

static inline blkHdr_t * getPrevNbrBlock(blkHdr_t * pCurrBlk) {
    checkChecksum(&pCurrBlk->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    ASSERT(GET_isPrevNbrBlkFree(pCurrBlk));
    u64 distanceToHeaderOfPrevNbrBlock;
#ifdef ENABLE_VALGRIND
    // Weird case so I'll just hack it for now
    VALGRIND_MAKE_MEM_DEFINED(((u64) pCurrBlk) - sizeof(u64), sizeof(u64));
#endif
    GET64 (distanceToHeaderOfPrevNbrBlock, (((u64) pCurrBlk)-sizeof(u64)));
#ifdef ENABLE_VALGRIND
    VALGRIND_MAKE_MEM_NOACCESS(((u64) pCurrBlk) - sizeof(u64), sizeof(u64));
#endif
    distanceToHeaderOfPrevNbrBlock += sizeof(blkHdr_t);
    blkHdr_t * pNbrBlk = (blkHdr_t *) (((u64) pCurrBlk) - distanceToHeaderOfPrevNbrBlock);
    checkChecksum(&pNbrBlk->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    return pNbrBlk;
}

static inline blkHdr_t * getNextNbrBlock(blkHdr_t * pCurrBlk) {
    checkChecksum(&pCurrBlk->checksum, sizeof(blkHdr_t), __LINE__, "blkHdr_t");
    u64 offset = GET_payloadSize(pCurrBlk);
    offset += sizeof(blkHdr_t);
    blkHdr_t * pNbrBlk = (blkHdr_t *) (((u64) pCurrBlk) + offset);
    return pNbrBlk;
}

// For valgrind, assumes first and second are properly setup
static void linkFreeBlocks(poolHdr_t * pPool, blkHdr_t * pFirstBlk, blkHdr_t * pSecondBlk) {
    ASSERT(GET_isThisBlkFree(pFirstBlk));  /* linkFreeBlocks arg1 not free */
    ASSERT(GET_isThisBlkFree(pSecondBlk)); /* linkFreeBlocks arg2 not free */

    /* Consec blocks cannot be free */
    ASSERT(getNextNbrBlock(pFirstBlk) != pSecondBlk);

    /* linkFreeBlocks arg1 misaligned */
    ASSERT((((u64) pFirstBlk) & (ALIGNMENT-1LL)) == 0LL);

    /* linkFreeBlocks arg2 misaligned */
    ASSERT((((u64) pSecondBlk) & (ALIGNMENT-1LL)) == 0LL);

    SET_pFreeBlkFrwdLink(pPool, pFirstBlk, pSecondBlk);
    SET_pFreeBlkBkwdLink(pPool, pSecondBlk, pFirstBlk, false);
}

static void markBlockUsed(poolHdr_t * pPool, blkHdr_t * pBlk) {
    SET_aggregatedFreeBlockIndicators(pBlk, 0LL);
    SET_poolHeaderDescr(pBlk, (((((u64) pPool) - ((u64) pBlk))) | allocatorTlsf_id));

    blkHdr_t * pNextBlock = getNextNbrBlock(pBlk);
    VALGRIND_DEFINED(pNextBlock);
    if(!GET_isThisBlkFree(pNextBlock))
        markPrevNbrBlockUsed(pNextBlock);
    VALGRIND_NOACCESS(pNextBlock);
}

static void markBlockFree(poolHdr_t * pPool, blkHdr_t * pBlk) {
    // To mark a block as free, we need to duplicate it's size at the end of
    // the block so that the contiguously next block can find the beginning of the free block.
    u64 payloadSize = GET_payloadSize(pBlk);
    u64 locationToWrite = ((u64) pBlk) + sizeof(blkHdr_t) + payloadSize - sizeof(u64);
#ifdef ENABLE_VALGRIND
    // Weird case so I'll just hack it for now
    VALGRIND_MAKE_MEM_DEFINED(locationToWrite, sizeof(u64));
#endif
    SET64 (locationToWrite, payloadSize);
#ifdef ENABLE_VALGRIND
    VALGRIND_MAKE_MEM_NOACCESS(locationToWrite, sizeof(u64));
#endif

    SET_pFreeBlkBkwdLink(pPool, pBlk, ((blkHdr_t *) 0xBEEFLL), true); // Write some value that is not 0 or 1 for now;
        // will be updated when this free block is actually inserted

    blkHdr_t * pNextBlock = getNextNbrBlock(pBlk);
    VALGRIND_DEFINED((u64) pNextBlock);

    if(!GET_isThisBlkFree(pNextBlock))  // Usually, no two free blocks follow each other but it may happen temporarily
        markPrevNbrBlockFree(pNextBlock);

    VALGRIND_NOACCESS((u64) pNextBlock);
}


/*
 * Allocation helpers (size and alignement constraints)
 */
static u64 getRealSizeOfRequest(u64 size) {
    if (size < sizeof(u64)) size = sizeof(u64);
    size = (size + ALIGNMENT - 1LL) & (~(ALIGNMENT-1LL));
    return size;
}

/* Two-level function to determine indices. This is pretty much
 * taken straight from the specs
 */
static void mappingInsert(u64 payloadSizeInBytes, u32* flIndex, u32* slIndex) {
    u32 tf, ts;
    u64 sizeInElements = payloadSizeInBytes / ALIGNMENT;

    if(sizeInElements < ZERO_LIST_SIZE) {
        tf = 0;
        ts = sizeInElements;
    } else {
        tf = FLS(sizeInElements);
        ts = (sizeInElements >> (tf - SL_COUNT_LOG2)) - (SL_COUNT);
        tf -= (FL_COUNT_SHIFT - 1LL);
    }
    *flIndex = tf;
    *slIndex = ts;
}

/* For allocations, we want to round-up to the next block size
 * so that we are sure that any block will work so we can
 * pick it in constant time.
 */
static void mappingSearch(u64 payloadSize, u32* flIndex, u32* slIndex) {
    u64 sizeInElements = payloadSize / ALIGNMENT;
    if(sizeInElements >= ZERO_LIST_SIZE) {
        // If not, we don't need to do anything since the mappingInsert
        // will return the correct thing
        sizeInElements += (1LL << (FLS(sizeInElements) - SL_COUNT_LOG2)) - 1LL;
    }
    payloadSize = sizeInElements * ALIGNMENT;
    mappingInsert(payloadSize, flIndex, slIndex);
}

/* Search for a suitable free block:
 *  - first search for a block in the fl/sl indicated.
 *  - if not found, search for higher sl (bigger) with same fl
 *  - if not found, search for higher fl and sl in that
 *
 *  Returns the header of a free block as well as flIndex and slIndex
 *  that block was taken from.
 */
static blkHdr_t * findFreeBlockForRealSize(
    poolHdr_t * pPool, u64 realSize, u32* flIndex, u32* slIndex) {

    blkHdr_t * pRes;
    u32 tf, ts;
    u64 flBitMap, slBitMap, flBucketCount;

    mappingSearch(realSize, flIndex, slIndex);
    tf = *flIndex;
    ts = *slIndex;

    flBucketCount = GET_flCount(pPool);
    if(tf >= flBucketCount) {
        return _NULL;
    }

    slBitMap = GET_slAvailOrNot(pPool, tf);
    slBitMap &= (~0LL << ts); // This takes all SL bins bigger or equal to ts
    if(slBitMap == 0) {
        // We don't have any non-zero block here so we look at the flAvailOrNot map
        flBitMap = GET_flAvailOrNot(pPool);
        flBitMap &= (~0LL << (tf + 1LL));
        if(flBitMap == 0) {
            return _NULL;
        }

        // Look for the first bit that is a one
        tf = myffs(flBitMap);
        ASSERT(tf > *flIndex);
        *flIndex = tf;

        // Now we get the slBitMap. There is definitely a 1 in there since there is a 1 in tf
        slBitMap = GET_slAvailOrNot(pPool, tf);
    }

    ASSERT(slBitMap != 0);

    ts = myffs(slBitMap);
    *slIndex = ts;

    pRes = GET_availBlkListHead(pPool,tf,ts);
    return pRes;
}


/* Helpers to manipulate the free lists */
// For valgrind, assumes freeBlock is OK
static void removeFreeBlock(poolHdr_t * pPool, blkHdr_t * pFreeBlk) {
    u64 tempSz;
    ASSERT(GET_isThisBlkFree(pFreeBlk));
    u32 flIndex, slIndex;
    tempSz = GET_payloadSize(pFreeBlk);
    mappingInsert(tempSz, &flIndex, &slIndex);

    // First remove it from the chain of free blocks
    blkHdr_t * pFreeBlkBkwdLink = GET_pFreeBlkBkwdLink(pPool, pFreeBlk);
    blkHdr_t * pFreeBlkFrwdLink = GET_pFreeBlkFrwdLink(pPool, pFreeBlk);

    VALGRIND_DEFINED1(pFreeBlkBkwdLink);
    VALGRIND_DEFINED(pFreeBlkFrwdLink);

    ASSERT(pFreeBlkBkwdLink != _NULL && GET_isThisBlkFree(pFreeBlkBkwdLink));
    ASSERT(pFreeBlkFrwdLink != _NULL && GET_isThisBlkFree(pFreeBlkFrwdLink));

    linkFreeBlocks(pPool, pFreeBlkBkwdLink, pFreeBlkFrwdLink);

    // Check if we need to change the head of the blocks list
    if(pFreeBlk == GET_availBlkListHead(pPool, flIndex, slIndex)) {
        SET_availBlkListHead (pPool, flIndex, slIndex, pFreeBlkFrwdLink);

        // If the new head is nullBlock then we clear the bitmaps to
        // indicate that we have no more blocks available here
        if(pFreeBlkFrwdLink == &(pPool->nullBlock)) {
            tempSz = GET_slAvailOrNot(pPool, flIndex);
            tempSz &= ~(((u64) 1LL) << slIndex); // Clear me bit
            SET_slAvailOrNot(pPool, flIndex, tempSz);

            // Check if slAvailOrNot is now 0
            if(tempSz == 0) {
                tempSz = GET_flAvailOrNot(pPool);

                tempSz &= ~(1LL << flIndex);
                SET_flAvailOrNot(pPool, tempSz);
            }
        }
    }

    VALGRIND_NOACCESS1(pFreeBlkBkwdLink);
    VALGRIND_NOACCESS(pFreeBlkFrwdLink);
}

// Assumes freeBlock OK for valgrind
void addFreeBlock(poolHdr_t * pPool, blkHdr_t * pFreeBlock) {
    u32 flIndex, slIndex;
    u64 tempSz;

    tempSz = GET_payloadSize(pFreeBlock);
    mappingInsert(tempSz, &flIndex, &slIndex);

    blkHdr_t * pCurrentHead;

    pCurrentHead = GET_availBlkListHead(pPool, flIndex, slIndex);

    ASSERT(pCurrentHead != _NULL);
    ASSERT(pFreeBlock != _NULL);
    ASSERT(pFreeBlock != &(pPool->nullBlock));

    VALGRIND_DEFINED(pCurrentHead);
    // Set the links properly
    SET_pFreeBlkBkwdLink(pPool, pFreeBlock, &(pPool->nullBlock), false);
    linkFreeBlocks(pPool, pFreeBlock, pCurrentHead);

    SET_pFreeBlkBkwdLink(pPool, pFreeBlock, &(pPool->nullBlock), false);
    SET_pFreeBlkFrwdLink(pPool, pFreeBlock, pCurrentHead);
    SET_pFreeBlkBkwdLink(pPool, pCurrentHead, pFreeBlock, false); // Does not matter if head was the nullBlock; that value is ignored anyway

    // Update the bitmaps
    SET_availBlkListHead (pPool, flIndex, slIndex, pFreeBlock);

    tempSz = GET_flAvailOrNot(pPool);

    tempSz = GET_slAvailOrNot(pPool, flIndex);
    if(!(tempSz & (1LL << slIndex))) {
        // Here it wasn't stored so we definitely have to update

        tempSz |= (1LL << slIndex);
        SET_slAvailOrNot(pPool, flIndex, tempSz);

        tempSz = GET_flAvailOrNot(pPool);
        if(!(tempSz & (1LL << flIndex))) {
            tempSz |= (1LL << flIndex);
            SET_flAvailOrNot(pPool, tempSz);
        }
    }

    VALGRIND_NOACCESS(pCurrentHead);
}

/* Split origBlock so that origBlock is allocSize long and
 * returns a pointer to the remaining block which is free
 */
// Assumes origBlock OK for valgrind
static blkHdr_t * splitBlock(poolHdr_t * pPool, blkHdr_t * pOrigBlock, u64 allocSize) {
    blkHdr_t * pRemainingBlock;
    u64 origBlockSize;
    origBlockSize = GET_payloadSize(pOrigBlock);
    ASSERT(origBlockSize > allocSize + GminBlockSizeIncludingHdr);

    u64 remainingSize = origBlockSize - allocSize - sizeof(blkHdr_t);

    pRemainingBlock = ((blkHdr_t *) (((u64) pOrigBlock) + GoffsetFromBlkHdrToPayload + allocSize));
    setChecksum(&pRemainingBlock->checksum, sizeof(blkHdr_t));

    VALGRIND_DEFINED(pRemainingBlock);
    // Take care of the remaining block (set its size and mark it as free)
    SET_payloadSize(pRemainingBlock, remainingSize);
    markBlockFree(pPool, pRemainingBlock);

    // Set the size of the original block properly
    SET_payloadSize(pOrigBlock, allocSize);

    VALGRIND_NOACCESS(pRemainingBlock);
    return pRemainingBlock;
}

/* Absorbs nextBlock into freeBlock making one much larger freeBlock. nextBlock
 * must be physically next to freeBlock
 */
// Assumes freeBlock and nextBlock OK for valgrind
static void absorbNext(poolHdr_t * pPool, blkHdr_t * pFreeBlock, blkHdr_t * pNextBlock) {
    ASSERT(GET_isThisBlkFree(pFreeBlock));
    ASSERT(GET_isThisBlkFree(pNextBlock));
    ASSERT(getNextNbrBlock(pFreeBlock) == pNextBlock);
    u64 tempSz = GET_payloadSize(pFreeBlock) + GET_payloadSize(pNextBlock) + sizeof(blkHdr_t);
    SET_payloadSize(pFreeBlock, tempSz);
    markBlockFree(pPool, pFreeBlock); // will update the other field
}

/* Merge with the contiguously previous block if it is free (blockToBeFreed must be used
 * so GET_isPrevNbrBlkFree can return true) Returns the resulting free block (either
 * blockToBeFreed marked as free or the larger block)
 */
// Assume blockToBeFreed OK for valgrind
#ifndef ENABLE_ALLOCATOR_LEAK_FREED_DATABLOCKS // Suppress if leaking datablocks, so compiler doesn't gripe.
static blkHdr_t * mergePrevNbr(poolHdr_t * pPool, blkHdr_t * pBlockToBeFreed) {
    ASSERT(!GET_isThisBlkFree(pBlockToBeFreed));
    if(GET_isPrevNbrBlkFree(pBlockToBeFreed)) {
        // Get the previous block
        blkHdr_t * pPrevBlock = getPrevNbrBlock(pBlockToBeFreed);
        VALGRIND_DEFINED1(pPrevBlock);
        // Remove it from the free-lists (since it will be made bigger)
        removeFreeBlock(pPool, pPrevBlock);

        // Now we mark the BlockToBeFreed as free and merge it with the other one
        markBlockFree(pPool, pBlockToBeFreed);
        absorbNext(pPool, pPrevBlock, pBlockToBeFreed);
        pBlockToBeFreed = pPrevBlock;
        ASSERT(GET_isThisBlkFree(pBlockToBeFreed));
        VALGRIND_NOACCESS1(pPrevBlock);
    } else {
        markBlockFree(pPool, pBlockToBeFreed);
        ASSERT(GET_isThisBlkFree(pBlockToBeFreed));
    }

    return pBlockToBeFreed;
}
#endif

/* Merges a block with the block contiguously next if that one is free as well. The input
 * block must be free to start with
 */
static blkHdr_t * mergeNextNbr(poolHdr_t * pPool, blkHdr_t * pFreeBlock) {
    ASSERT(GET_isThisBlkFree(pFreeBlock));
    blkHdr_t * pNextBlock = getNextNbrBlock(pFreeBlock);
    VALGRIND_DEFINED1(pNextBlock);
    if(GET_isThisBlkFree(pNextBlock)) {
        removeFreeBlock(pPool, pNextBlock);
        absorbNext(pPool, pFreeBlock, pNextBlock);
    }
    VALGRIND_NOACCESS1(pNextBlock);
    return pFreeBlock;
}

static u32 tlsfInit(poolHdr_t * pPool, u64 size) {
    /* The memory will be layed out as follows:
     *  - at location: the poolHdr_t structure is used
     *  - the typedef of that structure only has the fixed-length data included.  The variable-length
     *    data (variable at init time, invariant thereafter) has to be "annexed" onto that.
     *  - then the glebe, i.e. net pool space, i.e. the first free block starts right after that (aligned)
     */

// Figure out how much additional space needs to be annexed onto the end of the poolHdr_t struct
// for the first-level bucket bit-masks and second-level block lists.

    size &= ~(ALIGNMENT-1);
    u64 poolHeaderSize = sizeof(poolHdr_t);  // This size will increase as we add first-level buckets.
    u64 sizeRemainingAfterPoolHeader =
        size -             // From the gross pool size ...
        poolHeaderSize -   // ... subtract the size of the poolHdr_t ...
        sizeof(blkHdr_t);  // ... and the size of the blkHdr_t that starts the single, huge whole-glebe block.
    u64 flBucketCount = 0;
    u64 poolSizeSpannedByFlBuckets = (1LL << (FL_COUNT_SHIFT-1)) * ALIGNMENT;
    while (poolSizeSpannedByFlBuckets < sizeRemainingAfterPoolHeader) {
        flBucketCount++;
        poolHeaderSize = sizeof(poolHdr_t) +                                     // Non-variable-sized parts of pool header
                         (sizeof(u32) * (flBucketCount + (flBucketCount & 1))) + // Space for slAvailOrNot array
                         (sizeof(u32) * SL_COUNT * flBucketCount);               // Space for sl_avail 2D array.
        sizeRemainingAfterPoolHeader = size - poolHeaderSize - sizeof(blkHdr_t);
        poolSizeSpannedByFlBuckets <<= 1;
        if (flBucketCount == 26) break;
    }

    while (poolSizeSpannedByFlBuckets < sizeRemainingAfterPoolHeader) {
        flBucketCount++;
        poolHeaderSize = sizeof(poolHdr_t) +                                     // Non-variable-sized parts of pool header
                         (sizeof(u32) * (flBucketCount + (flBucketCount & 1))) + // Space for slAvailOrNot array
                         (sizeof(u64) * SL_COUNT * flBucketCount);               // Space for sl_avail 2D array.
        sizeRemainingAfterPoolHeader = size - poolHeaderSize - sizeof(blkHdr_t);
        poolSizeSpannedByFlBuckets <<= 1;
    }

    blkHdr_t * pNullBlock = &(pPool->nullBlock);
    setChecksum(pNullBlock, sizeof(blkHdr_t));                       // Init checksum, even though nullBlock is presently garbage.
    SET_lock         (pPool, 0);
    SET_flCount      (pPool, flBucketCount);
    SET_offsetToGlebe(pPool, poolHeaderSize);
    SET_currSliceNum (pPool, 0);
    poolHeaderSize += sizeof(blkHdr_t);
    // Now we have a poolHeaderSize that is big enough to contain the pool and right after it, we can start the glebe.
    sizeRemainingAfterPoolHeader = size - poolHeaderSize;
    if(sizeRemainingAfterPoolHeader < GminBlockSizeIncludingHdr) {
        DPRINTF(DEBUG_LVL_WARN, "Not enough space provided to make a meaningful TLSF pool at pPool=0x%lx.", (u64)pPool);
        DPRINTF(DEBUG_LVL_WARN, "Provision of %ld bytes nets a glebe (net pool size, after pool overhead) of %ld bytes\n",
            (u64) size, (u64)sizeRemainingAfterPoolHeader);
        return -1; // Can't allocate pool
    }
    DPRINTF(DEBUG_LVL_INFO,"Allocating a TLSF pool at 0x%lx of %ld bytes (glebe size, i.e. net size after pool overhead)\n",
        (u64)pPool, (u64)sizeRemainingAfterPoolHeader);

    blkHdr_t * pGlebe = GET_glebeAsInitialBlock(pPool);
    setChecksum(pGlebe, sizeof(blkHdr_t));
    SET_flAvailOrNot(pPool, 0); // Initialize the bitmaps to 0
    u32 i, j;
    for(i=0; i<flBucketCount; ++i) {
        SET_slAvailOrNot(pPool, i, 0);
        for(j=0; j<SL_COUNT; ++j) {
           SET_availBlkListHead (pPool, i, j, pNullBlock);
        }
    }

    // Initialize the glebe properly
    // We do something a little special in the glebe: we take a little bit
    // of it at the end to create a block with an empty payload that is used
    // to act as a sentinel. We therefore need space for a block header in the
    // end (the 2* is because we also account for our own block header overhead)
    SET_aggregatedFreeBlockIndicators (pGlebe, 0xBEEF0LL);
    SET_aggregatedFreeBlockIndicators (pNullBlock, 0xBEEF0LL);
    SET_payloadSize(pGlebe, sizeRemainingAfterPoolHeader - 2LL*sizeof(blkHdr_t));
    SET_pFreeBlkFrwdLink(pPool, pGlebe, pNullBlock);

    // This code is from markBlockFree (it marks the glebe as a single free block).  We paste
    // it here to remove some of the actions on next/prev block which bother valgrind
    u64 locationToWrite = ((u64) pGlebe) + (sizeRemainingAfterPoolHeader - sizeof(blkHdr_t)) - sizeof(u64);
    SET64 (locationToWrite, sizeRemainingAfterPoolHeader - (2LL*sizeof(blkHdr_t)));
    setChecksum(pGlebe, sizeof(blkHdr_t));
    SET_pFreeBlkBkwdLink(pPool, pGlebe, ((blkHdr_t *) 0xBEEFLL), true);  // Write some value that is not 0 or 1 for now

    // Add the sentinel
    blkHdr_t * pSentinelBlk = (blkHdr_t *) (locationToWrite + sizeof(u64));
    setChecksum(pSentinelBlk, sizeof(blkHdr_t));
    SET_payloadSize(pSentinelBlk, 0);
    SET_aggregatedFreeBlockIndicators(pSentinelBlk, 1);  // This is mark prevBlockFree for sentinel
    checkChecksum(pSentinelBlk, sizeof(blkHdr_t), __LINE__, "pSentinelBlk");

    // Initialize the nullBlock properly
    SET_nullBlock_payloadSize(pPool, 0);
    SET_nullBlock_pFreeBlkBkwdLink(pPool, pNullBlock);
    SET_nullBlock_pFreeBlkFrwdLink(pPool, pNullBlock);

    // Add the glebe, i.e. the big free block
    addFreeBlock(pPool, pGlebe);

    hal_lock32(&(pPool->lock));
    setChecksum(&pPool->checksum,      sizeof(poolHdr_t)-sizeof(u64));
    setChecksum(&pPool->annexChecksum, GET_offsetToGlebe(pPool));
    hal_unlock32(&(pPool->lock));

    VALGRIND_NOACCESS(pSentinelBlk);

    return 0;
}

//static u32 tlsfResize(u64 pgStart, u64 newsize) {
//    return -1;
//    // TODO: Not implemented yet
//}

void tlsf_walk_heap(poolHdr_t * pPool/*, tlsf_walkerAction action, void* extra*/);
static blkPayload_t * tlsfMalloc(poolHdr_t * pPool, u64 size)
{
    blkPayload_t * pResult = 0ULL;
    u64 payloadSize, returnedSize;
    u32 flIndex, slIndex;
    blkHdr_t * pAvailableBlock;
    blkHdr_t * pRemainingBlock;

    payloadSize = getRealSizeOfRequest(size);
    if(size > 0 && payloadSize == 0) {
        DPRINTF(DEBUG_LVL_INFO, "tlsfMalloc returning NULL for request size too large (%ld bytes) on pool at 0x%lx\n",
               size, (u64) pPool);
        return _NULL;
    }

    pAvailableBlock = findFreeBlockForRealSize(pPool, payloadSize, &flIndex, &slIndex);
    if (pAvailableBlock == NULL) {
        DPRINTF(DEBUG_LVL_INFO, "tlsfMalloc @0x%lx could not accomodate a block of size 0x%lx / %ld\n",
            (u64) pPool, (u64) payloadSize, (u64) payloadSize);
    } else {
        DPRINTF(DEBUG_LVL_INFO, "tlsfMalloc @0x%lx found a free block at 0x%lx for block of size 0x%lx / %ld\n",
            (u64) pPool, (u64) pAvailableBlock, (u64) payloadSize, (u64) payloadSize);
    }

    if (pAvailableBlock == _NULL) {
        // No longer print this warning here.  It is on the caller's head to decide if it should generate the
        // warning.  The other possibility is that the caller might try allocating the block in a different pool.
        //DPRINTF(DEBUG_LVL_WARN, "tlsfMalloc @ 0x%lx returning NULL for size %ld\n",
        //        (u64) pPool, size);
        return _NULL;
    }
    VALGRIND_DEFINED1(pAvailableBlock);
    removeFreeBlock(pPool, pAvailableBlock);
    returnedSize = GET_payloadSize(pAvailableBlock);
    if(returnedSize > payloadSize + GminBlockSizeIncludingHdr) {
        pRemainingBlock = splitBlock(pPool, pAvailableBlock, payloadSize);
        VALGRIND_DEFINED1(pRemainingBlock);
        DPRINTF(DEBUG_LVL_VVERB, "tlsfMalloc @0x%lx split block and re-added to free list 0x%lx\n",
            (u64) pPool, (u64) (pRemainingBlock));
        addFreeBlock(pPool, pRemainingBlock);
        VALGRIND_NOACCESS1(pRemainingBlock);
    } else {
        DPRINTF(DEBUG_LVL_VVERB, "tlsfMalloc. Widow, too small to utilize.  Payload size ask: %ld, getting %ld.  Contents:\n",
            (u64) payloadSize, (u64) returnedSize);
        u32 i;
        pResult = payloadAddressForBlock(pAvailableBlock);
        for (i = payloadSize; i < returnedSize; i += sizeof(u64)) {
            DPRINTF(DEBUG_LVL_VVERB, "    0x%lx = \n", ((u64) pResult) + i);
            DPRINTF(DEBUG_LVL_VVERB, "                    0x%lx\n", *((u64 *) (((u64) pResult) + i)));
        }
    }
    markBlockUsed(pPool, pAvailableBlock);
    VALGRIND_NOACCESS1(pAvailableBlock);
    pResult = payloadAddressForBlock(pAvailableBlock);
    returnedSize = GET_payloadSize(pAvailableBlock);
    DPRINTF(DEBUG_LVL_INFO, "tlsfMalloc @ 0x%lx returning 0x%lx for size %ld, returnedSize = 0x%lx, pResult=0x%lx\n",
        (u64) pPool, (u64) pResult, (u64) size, (u64) returnedSize, (u64) pResult);
#ifdef ENABLE_ALLOCATOR_INIT_NEW_DB_PAYLOAD
    u32 i;
    for (i = 0; i < returnedSize; i+= sizeof(u64)) {
        *((u64 *) (((u64) pResult) + i)) = ENABLE_ALLOCATOR_INIT_NEW_DB_PAYLOAD;
    }
#endif
    return pResult;
}

static void tlsfFree(poolHdr_t * pPool, blkPayload_t * pPayload) {
    blkHdr_t * pBlk = mapPayloadAddrToBlockAddr (pPayload);
    u64 payloadSize = GET_payloadSize(pBlk);
    ASSERT ((payloadSize & (ALIGNMENT-1)) == 0);
#ifdef ENABLE_ALLOCATOR_LEAK_FREED_DATABLOCKS
#define detail1 "LEAKED"
#else
#define detail1 " "
#endif
#ifdef ENABLE_ALLOCATOR_TRASH_FREED_DB_PAYLOAD
#define detail2 "OVERWRITTEN"
#else
#define detail2 " "
#endif
    DPRINTF(DEBUG_LVL_INFO, "tlsfFree         pool @ 0x%lx: free 0x%lx to 0x%lx, payloadSize=%ld/0x%lx, %s %s\n",
            (u64) pPool, (u64) pBlk, ((u64) pPayload)+payloadSize, (u64) payloadSize, (u64) payloadSize, detail1, detail2);
#undef detail1
#undef detail2
#ifdef ENABLE_ALLOCATOR_TRASH_FREED_DB_PAYLOAD
    u64 i;
    for (i = 0; i < payloadSize; i+=sizeof(u64)) {
        *((u64*)(((u64)pPayload)+i)) = ENABLE_ALLOCATOR_TRASH_FREED_DB_PAYLOAD;
    }
#endif
#ifdef ENABLE_ALLOCATOR_LEAK_FREED_DATABLOCKS
#ifdef ENABLE_VALGRIND
// Have Valgrind scream bloody murder if a "leaked" block is subsequently accessed.
    VALGRIND_MAKE_MEM_NOACCESS(pPayload, payloadSize);
#endif
#else // Don't leak, but really free the datablock
#ifdef ENABLE_VALGRIND
    blkHdr_t * pBlkTemp = pBlk;
    VALGRIND_DEFINED1(pBlk);
    pBlk = mergePrevNbr(pPool, pBlk);
    VALGRIND_NOACCESS1(pBlkTemp);
    pBlkTemp = pBlk;
    VALGRIND_DEFINED1(pBlk);
    pBlk = mergeNextNbr(pPool, pBlk);
    VALGRIND_NOACCESS1(pBlkTemp);
    VALGRIND_DEFINED1(pBlk);
    addFreeBlock(pPool, pBlk);
    VALGRIND_NOACCESS1(pBlk);
#else // Don't enable valgrind
    pBlk = mergePrevNbr(pPool, pBlk);
    pBlk = mergeNextNbr(pPool, pBlk);
    addFreeBlock(pPool, pBlk);
#endif // valgrind conditional
#endif // leakage conditional
    DPRINTF(DEBUG_LVL_INFO, "tlsfFree done on pool @ 0x%lx: free 0x%lx to 0x%lx, payloadSize=%ld/0x%lx\n",
            (u64) pPool, (u64) pBlk, ((u64) pPayload)+payloadSize, (u64) payloadSize, (u64) payloadSize);
}

static blkPayload_t * tlsfRealloc(poolHdr_t * pPool, blkPayload_t * pOldBlkPayload, u64 size) {
#if defined(ENABLE_ALLOCATOR_CHECKSUM) | \
    defined(ENABLE_ALLOCATOR_INIT_NEW_DB_PAYLOAD) | \
    defined(ENABLE_ALLOCATOR_TRASH_FREED_DB_PAYLOAD) | \
    defined(ENABLE_ALLOCATOR_LEAK_FREED_DATABLOCKS)
    // TODO:  At such time as this function is enabled for usage, deal with setting/checking checksums
    // and with the other ENABLE_ALLOCATOR... flags.  E.g., if the LEAK and the TRASH flags are set, it
    // would probably be appropriat to always do the realloc as a copy operation, and trash the old block
    static int warningGiven = 0;
    if (warningGiven == 0) {
        DPRINTF(DEBUG_LVL_WARN, "tlsfRealloc:  allocator checksuming, leaking, and/or payload [re]initing not implemented.\n");
        warningGiven = 1;
    }
#endif
    // First deal with corner cases:
    //  - non-zero size with null pointer is like malloc
    //  - zero size with ptr like free

    if(pOldBlkPayload != _NULL && size == 0) {
        tlsfFree(pPool, pOldBlkPayload);
        return _NULL;
    }
    if(pOldBlkPayload == _NULL) {
        return tlsfMalloc(pPool, size);
    }

    blkPayload_t * pResultPayload;
    blkHdr_t * pOldBlk = mapPayloadAddrToBlockAddr (pOldBlkPayload);
    VALGRIND_DEFINED(pOldBlk);
    blkHdr_t * pNextNbrBlk = getNextNbrBlock(pOldBlk);
    VALGRIND_DEFINED1(pNextNbrBlk);
    u64 annexSizeAvailable = GET_isThisBlkFree(pNextNbrBlk) ?  // Is the next contiguous block free?
        GET_payloadSize(pNextNbrBlk) + sizeof(blkHdr_t) :      // If so, its total size is available for annexation;
        0;                                                     // but if not, then no space is available for annexation.
    u64 existingPayloadSize = GET_payloadSize(pOldBlk);
    u64 maximumInPlacePayloadSize = existingPayloadSize + annexSizeAvailable;
    u64 newPayloadSize = getRealSizeOfRequest(size);
    if (newPayloadSize > maximumInPlacePayloadSize) {          // Needed size exceeds available?
        blkPayload_t * pNewBlkPayload = tlsfMalloc(pPool, newPayloadSize);
        if (pNewBlkPayload) {
            hal_memCopy(pNewBlkPayload, pOldBlkPayload, existingPayloadSize, false);
            tlsfFree(pPool, pOldBlkPayload);
        }
        pResultPayload = pNewBlkPayload;
    } else {
        if (newPayloadSize > existingPayloadSize) {
            // This means we need to extend to the other block
            removeFreeBlock(pPool, pNextNbrBlk);
            SET_payloadSize(pOldBlk, maximumInPlacePayloadSize);
            markBlockUsed(pPool, pOldBlk);
        } else {
            maximumInPlacePayloadSize = existingPayloadSize;
        }
        // We can trim to just the size used to create a new
        // free block and reduce internal fragmentation
        if(maximumInPlacePayloadSize > newPayloadSize + sizeof(blkHdr_t)) {
            blkHdr_t * pRemainingBlock = splitBlock(pPool, pOldBlk, newPayloadSize);
#ifdef ENABLE_VALGRIND
            VALGRIND_DEFINED1(pRemainingBlock);
            blkHdr_t * pBlkTemp = pRemainingBlock;
            pRemainingBlock = mergeNextNbr(pPool, pRemainingBlock);
            VALGRIND_NOACCESS1(pBlkTemp);
            VALGRIND_DEFINED1(pRemainingBlock);
            addFreeBlock(pPool, pRemainingBlock);
            VALGRIND_NOACCESS1(pRemainingBlock);
#else
            pRemainingBlock = mergeNextNbr(pPool, pRemainingBlock);
            addFreeBlock(pPool, pRemainingBlock);
#endif
        }
        pResultPayload = pOldBlkPayload;
    }

    VALGRIND_NOACCESS(pOldBlk);
    VALGRIND_NOACCESS1(pNextNbrBlk);
    return pResultPayload;
}

static ocrAllocator_t * getAnchorCE (ocrAllocator_t * self) {
// TODO:  given the address of this agent's ocrAllocator_t, this function returns the address of the
// same struct in the "anchor" agent.  ("Anchor" means the "zeroeth" agent of as far out the hierarchy
// as required.  E.g., for an L4 (shared by all blocks on a chip), the anchor is the CE of Block 0,
// Unit 0, Chip 0, current Board, current Rack, current Chassis, ....
//
// Of great concern is that this function presumes that the placement of such meta-data as the ocrAllocator_t
// for all agents will be identical.  As the project evolves, that is NOT a certainty.  For the moment, it
// is correct, but the code will need to be reworked if that changes in the future.
//
// While one might suppose that meta-data layout might remain consistent across all agents of a given
// type, it is rather unlikely that their layout will be the same for XEs as for CEs.  Therefore, at such
// time as the XEs are made responsible for allocating/reallocating/deallocating their own datablocks,
// they will need to manage their own L1s, and (imporantly for this discussion) then be able to climb
// the hierarchy to reach pools shared with other agents.  All of those pools will be shared with CEs,
// and all anchor ocrAllocator_t structs will be in a CE's domain.  As such, at least that much reworking
// of this code is inevitable, except that it can be finnessed by implementing a middle ground:  make the
// XEs responsible for attempting to allocate to their own L1, and if that fails, they can send a message
// (as they are doing now) to ask for that service from their CE.  Also, make the XEs able to deallocate
// datablocks that they successfully allocated (to their own L1), but send all other deallocation
// requests to the CE.  And realloc: tedious details TBD, but not rocket surgery.
//
    ocrAllocator_t * anchorCE = (ocrAllocator_t *) (self);
    ASSERT(self->memoryCount == 1);
#ifdef HAL_FSIM_CE
    u32 level = self->memories[0]->level;
    switch (level) {
    case 1:    // Agent level  (i.e. agent's scratch pad, managed by the CE of the agent's block.
    case 2:    // Block level  (i.e. block-shared memory, or block's private slice of a higher-level memory.
        anchorCE = (ocrAllocator_t *) (self);
        break;
    case 3:    // Unit level   (i.e. unit-shared memory, or unit's private slice of a higher-level memory.
        anchorCE = (ocrAllocator_t *) (((u64) self) | (UR_CE_BASE(0)));
        break;
    case 4:    // Chip level   (i.e. chip-shared memory, or chip's private slice of a higher-level memory.
        anchorCE = (ocrAllocator_t *) (((u64) self) | (CR_CE_BASE(0,0)));
        break;
    case 5:    // Board level  (i.e. board-shared memory, or board's private slice of a higher-level memory.
        anchorCE = (ocrAllocator_t *) (((u64) self) | (DR_CE_BASE(0,0,0)));
        break;
    case 6:    // Rack level   (i.e. rack-shared memory
        anchorCE = (ocrAllocator_t *) (((u64) self) | (RR_CE_BASE(0,0,0,0)));
        break;
    default:
        ASSERT(0);
    }
#endif
    return anchorCE;
}

void tlsfDestruct(ocrAllocator_t *self) {
    DPRINTF(DEBUG_LVL_INFO, "Entered tlsfDesctruct on allocator 0x%lx\n", (u64) self);
    ASSERT(self->memoryCount == 1);
    self->memories[0]->fcts.destruct(self->memories[0]);
    // TODO: Should we do this? It is the clean thing to do but may
    // cause mismatch between way it was created and freed
    runtimeChunkFree((u64)self->memories, NULL);

    runtimeChunkFree((u64)self, NULL);
    DPRINTF(DEBUG_LVL_INFO, "Leaving tlsfDesctruct on allocator 0x%lx\n", (u64) self);
}

static bool isAnchor (ocrAllocatorTlsf_t * rself, ocrAllocatorTlsf_t * rAnchorCE) {
    bool result;
    hal_lock32 (&(rAnchorCE->lockForInit));
    { // Bracket the critical section code
        result = (rself->lockForInit != 0);
    } // End bracketing of the critical section code
    hal_unlock32(&(rAnchorCE->lockForInit));
    return result;
}


void tlsfBegin(ocrAllocator_t *self, ocrPolicyDomain_t * PD ) {
    ocrAllocator_t * anchorCE = getAnchorCE(self);
    DPRINTF(DEBUG_LVL_INFO, "Entered tlsfBegin on allocator 0x%lx, anchorCE allocator is 0x%lx\n", (u64) self, (u64) anchorCE);
    u32 i;
    ASSERT(self->memoryCount == 1);
    ocrAllocatorTlsf_t * rAnchorCE = (ocrAllocatorTlsf_t *) anchorCE;
    ocrAllocatorTlsf_t * rself = (ocrAllocatorTlsf_t *) self;

    CHECK_AND_SET_MODULE_STATE(TLSF-ALLOCATOR, self, BEGIN);
    if (isAnchor(rself, rAnchorCE)) {
        hal_lock32(&(rAnchorCE->lockForInit));
        { // Bracket the critical section code
            if (rself->useCount1 == 0) {  // The reason for this is that on platforms other than FSIM,
                                          // all agents that share a pool also share the allocator_t
                                          // struct, and hence think they are the anchor.  Only the
                                          // first to get this lock should truly init the mem-target
                                          // and pool structures.
                self->memories[0]->fcts.begin(self->memories[0], PD);
                u64 poolAddr;
                RESULT_ASSERT(self->memories[0]->fcts.chunkAndTag(
                    self->memories[0], &poolAddr, rself->poolSize,
                    USER_FREE_TAG, USER_USED_TAG), ==, 0);
                rself->poolAddr = poolAddr;
                // Adjust alignment if required
                u64 fiddlyBits = ((u64) rself->poolAddr) & (ALIGNMENT - 1LL);
                if (fiddlyBits == 0) {
                    rself->poolStorageOffset = 0;
                } else {
                    rself->poolStorageOffset = ALIGNMENT - fiddlyBits;
                    rself->poolAddr += rself->poolStorageOffset;
                    rself->poolSize -= rself->poolStorageOffset;
                }
                rself->poolStorageSuffix = rself->poolSize & (ALIGNMENT-1LL);
                rself->poolSize &= ~(ALIGNMENT-1LL);
                DPRINTF(DEBUG_LVL_VERB,
                    "TLSF Allocator @ 0x%llx/0x%llx got pool at address 0x%llx of size %lld, offset from storage addr by %lld\n",
                    (u64) rself, (u64) self,
                    (u64) (rself->poolAddr), (u64) (rself->poolSize), (u64) (rself->poolStorageOffset));
#ifdef OCR_ENABLE_STATISTICS
                statsALLOCATOR_START(PD, self->guid, anchorCE, self->memories[0]->guid, self->memories[0]);
#endif
                ASSERT(((rself->sliceCount+2)*rself->sliceSize)<=rself->poolSize); // Remnant has to be at least as large as two slices.  (Note:  we might want to implement the option of there being NO remnant!)
                for (i = 0; i < rself->sliceCount; i++) {
                    DPRINTF(DEBUG_LVL_VVERB, "TLSF Allocator at 0x%llx initializing slice %d"
                        " at address 0x%llx of size %lld", self, i, rself->poolAddr, rself->sliceSize);
                    RESULT_ASSERT(tlsfInit(((poolHdr_t *) (rself->poolAddr)), rself->sliceSize), ==, 0);
#ifdef ENABLE_VALGRIND
                    VALGRIND_CREATE_MEMPOOL(((poolHdr_t *) (rself->poolAddr)), 0, false);
                    VALGRIND_MAKE_MEM_NOACCESS(((poolHdr_t *) (rself->poolAddr)), rself->sliceSize);
#endif
                    rself->poolAddr += rself->sliceSize;
                    rself->poolSize -= rself->sliceSize;
                }
                RESULT_ASSERT(tlsfInit(((poolHdr_t *) (rself->poolAddr)), rself->poolSize), ==, 0);
#ifdef ENABLE_VALGRIND
                VALGRIND_CREATE_MEMPOOL(((poolHdr_t *) (rself->poolAddr)), 0, false);
                VALGRIND_MAKE_MEM_NOACCESS(((poolHdr_t *) (rself->poolAddr)), rself->poolSize);
#endif
            }
            rself->useCount1++;
        } // End bracketing of the critical section code
        hal_unlock32(&(rAnchorCE->lockForInit));
    } else {
        while(rAnchorCE->useCount1 == 0) continue;
#ifdef OCR_ENABLE_STATISTICS
        // TODO:  Is this needed for non-anchor agents?
        // statsALLOCATOR_START(PD, self->guid, anchorCE, self->memories[0]->guid, self->memories[0]);
#endif
        rself->sliceCount = rAnchorCE->sliceCount;
        rself->sliceSize  = rAnchorCE->sliceSize;
        rself->poolAddr   = rAnchorCE->poolAddr;
        hal_lock32(&(rAnchorCE->lockForInit));
        { // Bracket the critical section code
            rAnchorCE->useCount1++;
        } // End bracketing of the critical section code
        hal_unlock32(&(rAnchorCE->lockForInit));
    }
    DPRINTF(DEBUG_LVL_INFO, "Leaving tlsfBegin on allocator 0x%lx\n", (u64) self);
}

void tlsfStart(ocrAllocator_t *self, ocrPolicyDomain_t * PD ) {
    DPRINTF(DEBUG_LVL_INFO, "Entered tlsfStart on allocator 0x%lx\n", (u64) self);
    ASSERT(self->memoryCount == 1);
    ocrAllocator_t * anchorCE = getAnchorCE(self);
    ocrAllocatorTlsf_t * rAnchorCE = (ocrAllocatorTlsf_t *) anchorCE;
    ocrAllocatorTlsf_t * rself = (ocrAllocatorTlsf_t *) self;
    // Get a GUID

    CHECK_AND_SET_MODULE_STATE(TLSF-ALLOCATOR, self, START);
    if (isAnchor(rself, rAnchorCE)) {
        hal_lock32(&(rAnchorCE->lockForInit));
        { // Bracket the critical section code
            if (rself->useCount2 == 0) {  // The reason for this is that on platforms other than FSIM,
                                          // all agents that share a pool also share the allocator_t
                                          // struct, and hence think they are the anchor.  Only the
                                          // first to get this lock should truly init the mem-target.
                self->memories[0]->fcts.start(self->memories[0], PD);
//printf ("tlsfStart calling guidify\n");
                guidify(PD, (u64)self, &(self->fguid), OCR_GUID_ALLOCATOR);
            }
            rself->useCount2++;
        } // End bracketing of the critical section code
        hal_unlock32(&(rAnchorCE->lockForInit));
    } else {
        while(rAnchorCE->useCount2 == 0) continue;
        hal_lock32(&(rAnchorCE->lockForInit));
        { // Bracket the critical section code
            rAnchorCE->useCount2++;
        } // End bracketing of the critical section code
        hal_unlock32(&(rAnchorCE->lockForInit));
    }
    self->pd = PD;
    DPRINTF(DEBUG_LVL_INFO, "Leaving tlsfStart on allocator 0x%lx\n", (u64) self);
}

void tlsfStop(ocrAllocator_t *self) {
    DPRINTF(DEBUG_LVL_INFO, "Entered tlsfStop on allocator 0x%lx\n", (u64) self);
    PD_MSG_STACK(msg);
    getCurrentEnv(&(self->pd), NULL, NULL, &msg);
    ocrAllocator_t * anchorCE = getAnchorCE(self);
    ocrAllocatorTlsf_t * rAnchorCE = (ocrAllocatorTlsf_t *) anchorCE;
    ocrAllocatorTlsf_t * rself = (ocrAllocatorTlsf_t *) self;
    ASSERT(self->memoryCount == 1);

    CHECK_AND_SET_MODULE_STATE(TLSF-ALLOCATOR, self, STOP);
    hal_lock32(&(rAnchorCE->lockForInit));
    { // Bracket the critical section code
        rAnchorCE->useCount2--;
    } // End bracketing of the critical section code
    hal_unlock32(&(rAnchorCE->lockForInit));
    if (isAnchor(rself, rAnchorCE)) {
        // On platforms other than FSIM, ALL agents share a common allocator_t struct, and so
        // think of themselves as the anchor.  Any ONE of them can do the finalization, AFTER
        // all of them have decremented the useCount to zero.
        while(rself->useCount2 > 0) continue;
        hal_lock32(&(rAnchorCE->lockForInit));
        { // Bracket the critical section code
            if (rself->useCount2 == 0) {
#ifdef OCR_ENABLE_STATISTICS
//TODO:  Should this be done by non-anchor agents too?
                statsALLOCATOR_STOP(self->pd, self->guid, self, self->memories[0]->guid, self->memories[0]);
#endif
                self->memories[0]->fcts.stop(self->memories[0]);
                rself->useCount2 = -1;  // Assure that other agents don't also perform the finalization steps.
            }
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_DESTROY
            msg.type = PD_MSG_GUID_DESTROY | PD_MSG_REQUEST;
            PD_MSG_FIELD_I(guid) = self->fguid;
            PD_MSG_FIELD_I(properties) = 0;
            self->pd->fcts.processMessage(self->pd, &msg, false);
#undef PD_MSG
#undef PD_TYPE
            self->fguid.guid = NULL_GUID;
        } // End bracketing of the critical section code
        hal_unlock32(&(rAnchorCE->lockForInit));
    }

    DPRINTF(DEBUG_LVL_INFO, "Leaving tlsfStop on allocator 0x%lx\n", (u64) self);
}

void tlsfFinish(ocrAllocator_t *self) {
    DPRINTF(DEBUG_LVL_INFO, "Entered tlsfFinish on allocator 0x%lx\n", (u64) self);
    ocrAllocator_t * anchorCE = getAnchorCE(self);
    ocrAllocatorTlsf_t * rAnchorCE = (ocrAllocatorTlsf_t *) anchorCE;
    ocrAllocatorTlsf_t * rself = (ocrAllocatorTlsf_t *) self;
    ASSERT(self->memoryCount == 1);

    CHECK_AND_SET_MODULE_STATE(TLSF-ALLOCATOR, self, FINISH);
    hal_lock32(&(rAnchorCE->lockForInit));
    { // Bracket the critical section code
        rAnchorCE->useCount1--;
    } // End bracketing of the critical section code
    hal_unlock32(&(rAnchorCE->lockForInit));
    if (isAnchor(rself, rAnchorCE)) {
        // On platforms other than FSIM, ALL agents share a common allocator_t struct, and so
        // think of themselves as the anchor.  Any ONE of them can do the finalization, AFTER
        // all of them have decremented the useCount to zero.
        while(rself->useCount1 > 0) continue;
        hal_lock32(&(rAnchorCE->lockForInit));
        { // Bracket the critical section code
            if (rself->useCount1 == 0) {
                RESULT_ASSERT(rAnchorCE->base.memories[0]->fcts.tag(
                    rself->base.memories[0],
                    rself->poolAddr - rself->sliceSize * rself->sliceCount - rself->poolStorageOffset,
                    rself->poolAddr + rself->poolSize + rself->poolStorageSuffix,
                    USER_FREE_TAG), ==, 0);
#ifdef ENABLE_VALGRIND
                u64 poolAddr = rself->poolAddr - (((u64) rself->sliceCount)*((u64) rself->sliceSize));
                u32 i;
                for (i = 0; i < rself->sliceCount; i++) {
                    VALGRIND_DESTROY_MEMPOOL(poolAddr);
                    VALGRIND_MAKE_MEM_DEFINED(poolAddr, rself->sliceSize);
                    poolAddr += srelf->sliceSize;
                }

                VALGRIND_DESTROY_MEMPOOL(poolAddr);
                VALGRIND_MAKE_MEM_DEFINED(poolAddr, rAnchorCE->poolSize);
#endif
                self->memories[0]->fcts.finish(self->memories[0]);
            }
        } // End bracketing of the critical section code
        hal_unlock32(&(rAnchorCE->lockForInit));
    }
    DPRINTF(DEBUG_LVL_INFO, "Leaving tlsfFinish on allocator 0x%lx\n", (u64) self);
}

void* tlsfAllocate(
    ocrAllocator_t *self,   // Allocator to attempt block allocation
    u64 size,               // Size of desired block, in bytes
    u64 hints) {            // Allocator-dependent hints; TLSF supports reduced contention
    ocrAllocatorTlsf_t *rself = (ocrAllocatorTlsf_t*)self;

    bool useRemnant = !(hints & OCR_ALLOC_HINT_REDUCE_CONTENTION);
    poolHdr_t * pPool = (poolHdr_t *) (rself->poolAddr); // Addr of remnant pool (pool shared by ALL clients of allocator)

    if (useRemnant == 0) {  // Attempt to allocate the requested block to a semi-private slice pool picked in round-robin fashion.
        if (rself->sliceCount == 0) return _NULL; // Slicing is NOT implemented on this pool.  Return failure status.
        if (rself->sliceSize < size) return _NULL; // Don't bother trying if the requested block is bigger than the pool supports.
        // Attempt allocations to slice pools on an entirely round-robin basis, so that any number of concurrent allocations up to
        // the number of slices can be supported with NO throttling on their locks.  In the following code, the increment and
        // wrap-around of the currSliceNum round-robin rotor is NOT thread safe, but it doesn't really need to be.  In the rare
        // occurrence of a race on that variable, getting it "wrong" doesn't hurt anything, and it's cheaper just to "let it ride".
#ifdef ENABLE_VALGRIND
        VALGRIND_MAKE_MEM_DEFINED((u64) pPool, sizeof(poolHdr_t));
#endif
#ifdef ENABLE_ALLOCATOR_CHECKSUM
// One difference when running with checksumming versus otherwise is that we have to be thread-safe about how we get the
// sliceNum to try next.  Otherwise, a different thread can whack the sliceNum such that it doesn't get reflected into
// the checksum correctly.  In this one way, checksumming doesn't give us an accurate picture of what is going on in
// the production runs.  (But this is considered a low-risk difference.)
        hal_lock32(&(pPool->lock));
        checkChecksum(&pPool->checksum,      sizeof(poolHdr_t)-sizeof(u64), __LINE__, "poolHdr_t");
#endif
        u64 sliceNum = GET_currSliceNum(pPool) + 1LL; // Read of thread-unsafe read-modify-write operation, but not a problem.
        u64 slicePoolOffset = sliceNum * rself->sliceSize;
        if (sliceNum == rself->sliceCount) sliceNum = 0;
        SET_currSliceNum (pPool, sliceNum); // Write of thread-unsafe read-modify-write operation, but not a problem.
#ifdef ENABLE_ALLOCATOR_CHECKSUM
        setChecksum(&pPool->checksum,      sizeof(poolHdr_t)-sizeof(u64));
        hal_unlock32(&(pPool->lock));
#endif
#ifdef ENABLE_VALGRIND
        VALGRIND_MAKE_MEM_NOACCESS((u64) pPool, sizeof(poolHdr_t));
#endif
        pPool = ((poolHdr_t *) (((u64) pPool) - slicePoolOffset)); // Adjust pool pointer to point to this slice instead of remnant.
    }

#ifdef ENABLE_VALGRIND
    VALGRIND_MAKE_MEM_DEFINED((u64) pPool, sizeof(poolHdr_t));
    u64 sizeOfPoolHdr = GET_offsetToGlebe(pPool) + sizeof(blkHdr_t);
    VALGRIND_MAKE_MEM_NOACCESS((u64) pPool, sizeof(poolHdr_t));
    VALGRIND_MAKE_MEM_DEFINED((u64) pPool, sizeOfPoolHdr);
#endif
    hal_lock32(&(pPool->lock));
    DPRINTF(DEBUG_LVL_INFO, "TLSF Allocate called with allocator 0x%llx, pool 0x%llx, for size %lld and useRemnant %d\n",
            self, pPool, size, useRemnant);
    checkChecksum(&pPool->checksum,      sizeof(poolHdr_t)-sizeof(u64), __LINE__, "poolHdr_t");
    checkChecksum(&pPool->annexChecksum, GET_offsetToGlebe(pPool),      __LINE__, "poolHdr_t");
    void* toReturn = (void*)tlsfMalloc(pPool, size);
    setChecksum(&pPool->checksum,      sizeof(poolHdr_t)-sizeof(u64));
    setChecksum(&pPool->annexChecksum, GET_offsetToGlebe(pPool));
#if 0
    if (((u64) toReturn) != 0) {
        u64 addr = ((u64) toReturn) - sizeof(blkHdr_t);
        u32 i;
        for (i = 0; i < sizeof(blkHdr_t); i += sizeof(u64)) {
            DPRINTF(DEBUG_LVL_WARN, "    0x%lx = 0x%lx\n", (((u64) addr) + i), *((u64 *) (((u64) addr) + i)));
        }
    }
#endif
    hal_unlock32(&(pPool->lock));
#ifdef ENABLE_VALGRIND
    if (toReturn) VALGRIND_MEMPOOL_ALLOC((u64) pPool, toReturn, size);
    VALGRIND_MAKE_MEM_NOACCESS((u64) pPool, sizeOfPoolHdr);
#endif
    return toReturn;
}

void tlsfDeallocate(void* address) {
    blkHdr_t * pBlock = mapPayloadAddrToBlockAddr(address);
#ifdef ENABLE_VALGRIND
    VALGRIND_MAKE_MEM_DEFINED((u64) pBlock, sizeof(blkHdr_t));
#endif
    poolHdr_t * pPool = (poolHdr_t *) (((u64) pBlock) + (GET_poolHeaderDescr(pBlock) & POOL_HEADER_ADDR_MASK));
#ifdef ENABLE_VALGRIND
    VALGRIND_MAKE_MEM_NOACCESS((u64) pBlock, sizeof(blkHdr_t));
    VALGRIND_MAKE_MEM_DEFINED((u64) pPool, sizeof(poolHdr_t));
#endif
    hal_lock32(&(pPool->lock));
    checkChecksum(&pPool->checksum,      sizeof(poolHdr_t)-sizeof(u64), __LINE__, "poolHdr_t");
    checkChecksum(&pPool->annexChecksum, GET_offsetToGlebe(pPool),      __LINE__, "poolHdr_t");
#ifdef ENABLE_VALGRIND
    u64 sizeOfPoolHdrWithAnnex = GET_offsetToGlebe(pPool) + sizeof(blkHdr_t);
    VALGRIND_MAKE_MEM_NOACCESS((u64) pPool, sizeof(poolHdr_t));
    VALGRIND_MAKE_MEM_DEFINED((u64) pPool, sizeOfPoolHdrWithAnnex);
#endif
    tlsfFree(pPool, (blkPayload_t *) address);
#ifdef ENABLE_VALGRIND
    VALGRIND_MEMPOOL_FREE((u64) pPool, address);
#endif
    setChecksum(&pPool->checksum,      sizeof(poolHdr_t)-sizeof(u64));
    setChecksum(&pPool->annexChecksum, GET_offsetToGlebe(pPool));
    hal_unlock32(&(pPool->lock));
#ifdef ENABLE_VALGRIND
    VALGRIND_MAKE_MEM_NOACCESS((u64) pPool, sizeOfPoolHdrWithAnnex);
#endif
}

void* tlsfReallocate(
    ocrAllocator_t *self,   // Allocator to attempt block allocation
    void * pCurrBlkPayload, // Address of existing block.  (NOT necessarily allocated to this Allocator instance, nor even in an allocator of this type.)
    u64 size,               // Size of desired block, in bytes
    u64 hints) {            // Allocator-dependent hints; TLSF supports reduced contention

    if (pCurrBlkPayload == _NULL) {  // Handle corner case, where a non-existent "existing" block means this is just a plain ole malloc.
        return tlsfAllocate (self, size, hints);
    }
    ASSERT (size != 0);     // Caller has to handle the oddball corner case where size is zero, meaning what we really want to do is a "free".

    bool useRemnant = !(hints & OCR_ALLOC_HINT_REDUCE_CONTENTION);
    blkHdr_t * pExistingBlock = mapPayloadAddrToBlockAddr(pCurrBlkPayload);
#ifdef ENABLE_VALGRIND
    VALGRIND_MAKE_MEM_DEFINED((u64) pExistingBlock, sizeof(blkHdr_t));
#endif
    poolHdr_t * pPoolOfExistingBlock =
        (poolHdr_t *) (((u64) pExistingBlock) + (GET_poolHeaderDescr(pExistingBlock) & POOL_HEADER_ADDR_MASK));
    ocrAllocatorTlsf_t *rself = (ocrAllocatorTlsf_t*)self;
    poolHdr_t * pRemnantPool = (poolHdr_t *) (((ocrAllocatorTlsf_t*)self)->poolAddr); // Addr of remnant pool (pool shared by ALL clients of allocator)
    poolHdr_t * pFirstSlicePool = (poolHdr_t *) (((u64) pRemnantPool) - (((u64) rself->sliceCount) * ((u64) rself->sliceSize)));
    if (pPoolOfExistingBlock <= pRemnantPool && pPoolOfExistingBlock >= pFirstSlicePool) {
        // Existing block is in one of the pools of this allocator, either in one of its slices or in its remnant.  Ignoring the useRemnant flag, attempt
        // to reallocate it to the same pool that it is already in.  Failing that, we will try reallocating it to the remnant.
        hal_lock32(&(pPoolOfExistingBlock->lock));
        checkChecksum(&pPoolOfExistingBlock->checksum,      sizeof(poolHdr_t)-sizeof(u64),           __LINE__, "poolHdr_t");
        checkChecksum(&pPoolOfExistingBlock->annexChecksum, GET_offsetToGlebe(pPoolOfExistingBlock), __LINE__, "poolHdr_t");
#ifdef ENABLE_VALGRIND
        u64 sizeOfPoolHdr = GET_offsetToGlebe(pPoolOfExistingBlock) + sizeof(blkHdr_t);
        VALGRIND_MAKE_MEM_DEFINED(pPoolOfExistingBlock, sizeOfPoolHdr);
#endif
        blkPayload_t* pNewBlockPayload = tlsfRealloc(pPoolOfExistingBlock, (blkPayload_t *)pCurrBlkPayload, size);
#ifdef ENABLE_VALGRIND
        VALGRIND_MEMPOOL_CHANGE(pPoolOfExistingBlock, pCurrBlkPayload, pNewBlockPayload, size);
        VALGRIND_MAKE_MEM_NOACCESS(pPoolOfExistingBlock, sizeOfPoolHdr);
#endif
        setChecksum(&pPoolOfExistingBlock->checksum,      sizeof(poolHdr_t)-sizeof(u64));
        setChecksum(&pPoolOfExistingBlock->annexChecksum, GET_offsetToGlebe(pPoolOfExistingBlock));
        hal_unlock32(&(pPoolOfExistingBlock->lock));
        if (pNewBlockPayload != _NULL) return (void *) pNewBlockPayload; // If we succeeded in reallocating it in its existing pool, return the successful result.
        if (pPoolOfExistingBlock == pRemnantPool) return _NULL;  // If the existing pool was the remnant, return failure so that the caller can try a different memory level.  Else, drop through to try remnant.
        useRemnant = 1;
    }

    poolHdr_t * pPool = pRemnantPool;
    if (useRemnant == 0) {  // Attempt to allocate the requested block to a semi-private slice pool picked in round-robin fashion.
        if (rself->sliceCount == 0) return _NULL;  // Slicing is NOT implemented on this pool.  Return failure status.
        if (rself->sliceSize < size) return _NULL; // Don't bother trying if the requested block is bigger than the pool supports.
#ifdef ENABLE_VALGRIND
        VALGRIND_MAKE_MEM_DEFINED((u64) pPool, sizeof(poolHdr_t));
#endif
        u64 sliceNum = GET_currSliceNum(pPool) + 1LL; // Read of thread-unsafe read-modify-write operation, but not a problem.
        u64 slicePoolOffset = sliceNum * rself->sliceSize;
        if (sliceNum == rself->sliceCount) sliceNum = 0;
        SET_currSliceNum (pPool, sliceNum); // Write of thread-unsafe read-modify-write operation, but not a problem.
#ifdef ENABLE_VALGRIND
        VALGRIND_MAKE_MEM_NOACCESS((u64) pPool, sizeof(poolHdr_t));
#endif
        pPool = ((poolHdr_t *) (((u64) pPool) - slicePoolOffset)); // Adjust pool pointer to point to this slice instead of remnant.
    }

#ifdef ENABLE_VALGRIND
    VALGRIND_MAKE_MEM_DEFINED((u64) pPool, sizeof(poolHdr_t));
    u64 sizeOfPoolHdr = GET_offsetToGlebe(pPool) + sizeof(blkHdr_t);
    VALGRIND_MAKE_MEM_NOACCESS((u64) pPool, sizeof(poolHdr_t));
    VALGRIND_MAKE_MEM_DEFINED((u64) pPool, sizeOfPoolHdr);
#endif
    hal_lock32(&(pPool->lock));
    checkChecksum(&pPool->checksum,      sizeof(poolHdr_t)-sizeof(u64), __LINE__, "poolHdr_t");
    checkChecksum(&pPool->annexChecksum, GET_offsetToGlebe(pPool),      __LINE__, "poolHdr_t");
    blkPayload_t * pNewBlockPayload = tlsfMalloc(pPool, size);
    setChecksum(&pPool->checksum,      sizeof(poolHdr_t)-sizeof(u64));
    setChecksum(&pPool->annexChecksum, GET_offsetToGlebe(pPool));
    hal_unlock32(&(pPool->lock));

    if (pNewBlockPayload != _NULL) {
        u64 sizeOfOldBlock = GET_payloadSize(pExistingBlock);
        u64 sizeOfNewBlock = GET_payloadSize(mapPayloadAddrToBlockAddr((blkPayload_t *) pNewBlockPayload));
        u64 sizeToCopy = sizeOfOldBlock < sizeOfNewBlock ? sizeOfOldBlock : sizeOfNewBlock;
        hal_memCopy(pNewBlockPayload, ((blkPayload_t *) pCurrBlkPayload), sizeToCopy, false);
#ifdef ENABLE_VALGRIND
        VALGRIND_MEMPOOL_ALLOC((u64) pPool, pNewBlockPayload, size);
        VALGRIND_MAKE_MEM_NOACCESS((u64) pPool, sizeOfPoolHdr);
#endif
        allocatorFreeFunction(pCurrBlkPayload);  // Free up the old block.  (Note that this might call an entirely different allocator instance, and even a different allocator type.)
    }
    return pNewBlockPayload;
}

//#endif /* ENABLE_BUILDER_ONLY */

// Method to create the TLSF allocator
ocrAllocator_t * newAllocatorTlsf(ocrAllocatorFactory_t * factory, ocrParamList_t *perInstance) {

    ocrAllocatorTlsf_t *result = (ocrAllocatorTlsf_t*)
        runtimeChunkAlloc(sizeof(ocrAllocatorTlsf_t), PERSISTENT_CHUNK);
    ocrAllocator_t * base = (ocrAllocator_t *) result;
    SET_MODULE_STATE(TLSF-ALLOCATOR, base, NEW);
    factory->initialize(factory, base, perInstance);
    return (ocrAllocator_t *) result;
}

void initializeAllocatorTlsf(ocrAllocatorFactory_t * factory, ocrAllocator_t * self, ocrParamList_t * perInstance) {
    initializeAllocatorOcr(factory, self, perInstance);

    ocrAllocatorTlsf_t *derived = (ocrAllocatorTlsf_t *)self;
    paramListAllocatorTlsf_t *perInstanceReal = (paramListAllocatorTlsf_t*)perInstance;

    if (perInstanceReal->sliceCount > 0xFFFF) {
        DPRINTF(DEBUG_LVL_WARN, "Number of slice pools per allocator limited to 65535.  Reduced\n");
        derived->sliceCount        = 0xFFFF;
    } else {
        derived->sliceCount        = (u16) perInstanceReal->sliceCount;
    }
    derived->sliceSize         = perInstanceReal->sliceSize;
    derived->useCount1         = 0;
    derived->useCount2         = 0;
    derived->poolAddr          = 0ULL;
    derived->poolSize          = perInstanceReal->base.size;
    derived->poolStorageOffset = 0;
    derived->poolStorageSuffix = 0;
    derived->lockForInit       = 0;
    DPRINTF(DEBUG_LVL_INFO, "TLSF Allocator instance @ 0x%llx initialized with "
            "sliceCount: %lld, sliceSize: %lld, poolSize: %lld",
            self, derived->sliceCount, derived->sliceSize, derived->poolSize);
}

/******************************************************/
/* OCR ALLOCATOR TLSF FACTORY                         */
/******************************************************/

static void destructAllocatorFactoryTlsf(ocrAllocatorFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrAllocatorFactory_t * newAllocatorFactoryTlsf(ocrParamList_t *perType) {
    ocrAllocatorFactory_t* base = (ocrAllocatorFactory_t*)
        runtimeChunkAlloc(sizeof(ocrAllocatorFactoryTlsf_t), NONPERSISTENT_CHUNK);
    ASSERT(base);
    base->instantiate = &newAllocatorTlsf;
    base->initialize = &initializeAllocatorTlsf;
    base->destruct = &destructAllocatorFactoryTlsf;
    base->allocFcts.destruct = FUNC_ADDR(void (*)(ocrAllocator_t*), tlsfDestruct);
    base->allocFcts.begin = FUNC_ADDR(void (*)(ocrAllocator_t*, ocrPolicyDomain_t*), tlsfBegin);
    base->allocFcts.start = FUNC_ADDR(void (*)(ocrAllocator_t*, ocrPolicyDomain_t*), tlsfStart);
    base->allocFcts.stop = FUNC_ADDR(void (*)(ocrAllocator_t*), tlsfStop);
    base->allocFcts.finish = FUNC_ADDR(void (*)(ocrAllocator_t*), tlsfFinish);
    base->allocFcts.allocate = FUNC_ADDR(void* (*)(ocrAllocator_t*, u64, u64), tlsfAllocate);
    //base->allocFcts.free = FUNC_ADDR(void (*)(void*), tlsfDeallocate);
    base->allocFcts.reallocate = FUNC_ADDR(void* (*)(ocrAllocator_t*, void*, u64), tlsfReallocate);
    return base;
}
// Deactivated for now. Need to clean-up
#if 0 && defined(OCR_DEBUG)
/* Some debug functions to see what is going on */


static void printBlock(void *block_, void* extra) {
    blkHdr_t *bl = (blkHdr_t*)block_;
    u32 *count = (u32*)extra;
    fprintf(stderr, "\tBlock %d starts at 0x%lx (user: 0x%lx) of size %d (user: %d) %s\n",
            *count, bl, (char*)bl + (GusedBlockOverhead << ELEMENT_SIZE_LOG2),
            bl->payloadSize, (bl->payloadSize << ELEMENT_SIZE_LOG2),
            GET_isThisBlkFree(bl)?"free":"used");
    *count += 1;
}

typedef struct flagVerifier_t {
    u32 countConsecutiveFrees;
    BOOL isPrevFree;
    u32 countErrors;
} flagVerifier_t;

static void verifyFlags(void *block_, void* extra) {
    blkHdr_t *bl = (blkHdr_t*)block_;
    flagVerifier_t* verif = (flagVerifier_t*)extra;

    if(GET_isPrevNbrBlkFree(bl) != verif->isPrevFree) {
        verif->countErrors += 1;
        fprintf(stderr, "Mismatch in free flag for block 0x%x\n", bl);
    }
    if(GET_isThisBlkFree(bl)) {
        verif->countConsecutiveFrees += 1;
        if(verif->countConsecutiveFrees > 1) {
            fprintf(stderr, "Blocks did not coalesce (count of %d at 0x%x)\n",
                    verif->countConsecutiveFrees, bl);
            verif->countErrors += 1;
        }
        verif->isPrevFree = TRUE;
    } else {
        verif->countConsecutiveFrees = 0;
        verif->isPrevFree = FALSE;
    }
}

void tlsf_walk_heap(u64 pgStart, tlsf_walkerAction action, void* extra) {
    // poolHdr_t *poolToUse = (poolHdr_t*)pgStart;
    if(action == 0) {
        action = (tlsf_walkerAction)printBlock;
    }
    blkHdr_t *bl = (blkHdr_t *) ADDR_OF_MAIN_BLOCK;     // poolToUse->pMainBlock;

    while(bl && !(bl->payloadSize == 0)) {
        action(bl, extra);
        bl = GET_ADDRESS(getNextNbrBlock(bl));
    }
}

/* Will check to make sure the heap is properly maintained
 *
 * Return the number of errors
 */
u32 tlsf_check_heap(u64 pgStart, unsigned u32 *freeRemaining) {
    poolHdr_t *poolToUse = (poolHdr_t*)pgStart;
    u32 errCount = 0;
    unsigned u32 countFree = 0;
    flagVerifier_t verif = {0, FALSE, 0};

    tlsf_walk_heap(pgStart, verifyFlags, &verif);
    if(verif.countErrors) {
        errCount += verif.countErrors;
        fprintf(stderr, "Mismatch in free flags or coalescing\n");
    }

    // Check the bitmaps
    u64 flBucketCount;
    flBucketCount = GET_flCount(pgStart);
    for(u32 i=0; i < flBucketCount; ++i) {
        bool hasFlAvail = poolToUse->flAvailOrNot & (1LL << i);
        for(u32 j=0; j < SL_COUNT; ++j) {
            bool hasSlAvail = poolToUse->slAvailOrNot[i] & (1LL << j);

            headerAddr_t blockHead;
            blockHead.value = poolToUse->blocks[i][j];

            if(!hasFlAvail && hasSlAvail) {
                ++errCount;
                fprintf(stderr, "FL and SL lists do not match for (%d, %d)\n", i, j);
            }

            if(!hasSlAvail && (GET_ADDRESS(blockHead) != &(poolToUse->nullBlock))) {
                ++errCount;
                fprintf(stderr, "Empty list does not start with nullBLock for (%d, %d)\n", i, j);
            }
            if(hasSlAvail) {
                // We now check all the blocks
                blkHdr_t *blockHeadPtr = GET_ADDRESS(blockHead);
                while(blockHeadPtr != &(poolToUse->nullBlock)) {
                    if(!GET_isThisBlkFree(blockHeadPtr)) {
                        fprintf(stderr, "Block 0x%x should be free for (%d, %d)\n", blockHeadPtr, i, j);
                        ++errCount;
                    }
                    if(GET_isThisBlkFree(GET_ADDRESS(getNextNbrBlock(blockHeadPtr)))) {
                        fprintf(stderr, "Block 0x%x should have coalesced with next for (%d, %d)\n", blockHeadPtr, i, j);
                        ++errCount;
                    }
                    if(!GET_isPrevNbrBlkFree(GET_ADDRESS(getNextNbrBlock(blockHeadPtr)))) {
                        fprintf(stderr, "Block 0x%x is not prevFree for (%d, %d)\n", blockHeadPtr, i, j);
                        ++errCount;
                    }
                    if(blockHeadPtr != GET_ADDRESS(getPrevNbrBlock(GET_ADDRESS(getNextNbrBlock(blockHeadPtr))))) {

                        fprintf(stderr, "Block 0x%x cannot be back-reached for (%d, %d)\n", blockHeadPtr, i, j);
                        ++errCount;
                    }
                    if(blockHeadPtr->payloadSize < (GminBlockRealSize - GusedBlockOverhead)
                            || blockHeadPtr->payloadSize > GmaxBlockRealSize) {

                        fprintf(stderr, "Block 0x%x has illegal size for (%d, %d)\n", blockHeadPtr, i, j);
                        ++errCount;
                    }

                    u32 tf, ts;
                    mappingInsert(blockHeadPtr->payloadSize, &tf, &ts);
                    if(tf != i || ts != j) {
                        fprintf(stderr, "Block 0x%x is in wrong bucket for (%d, %d)\n", blockHeadPtr, i, j);
                        ++errCount;
                    }
                    countFree += blockHeadPtr->payloadSize;
                    blockHead = GET_pFreeBlkFrwdLink(pPool, blockHeadPtr);
                    blockHeadPtr = GET_ADDRESS(blockHead);
                }
            }
        }
    }
    *freeRemaining = countFree << ELEMENT_SIZE_LOG2;
    return errCount;
}
#endif /* OCR_DEBUG */

#endif /* ENABLE_TLSF_ALLOCATOR */
