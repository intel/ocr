/**
 * @brief Implementation of an allocator based on the TLSF allocator
 * @authors Romain Cledat, Intel Corporation
 * @date 2012-09-21
 * Copyright (c) 2012, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of Intel Corporation nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

#include "tlsf-allocator.h"
#include "ocr-macros.h"
#include "ocr-mappable.h"
#include "ocr-types.h"
#include "ocr-utils.h"
#include "ocr-guid.h"
#include "ocr-config.h"
#include "debug.h"

#include "ocr-policy-domain-getter.h"

#include <string.h>
//#include <stdlib.h>
#include <inttypes.h>

/******************************************************/
/* OCR ALLOCATOR TLSF IMPLEMENTATION                  */
/******************************************************/

/**
 * @todo A lot of the weird notations here are remnants of an older
 * code base. They will be simplified but it should not impact
 * performance
 */

/*
 * Type used to store the "size" of a block
 * The minimum size of a block will be 4*sizeof(tlsfSize_t)
 * padded to be a multiple of 2^ALIGN_LOG2 bytes
 * NOTE: When changing this:
 *  - make sure the fls routine in tlsfMalloc.c exists for the size
 *  - change FL_MAX_LOG2 if required
 */
typedef u32 tlsfSize_t;
#define SIZE_TLSF_SIZE 32
#define SIZE_TLSF_SIZE_LOG2 5

/*
 * Configuration constants
 */
/*
 * VERY IMPORTANT: The maximum size of the memory supported
 * is *very* dependent on a few parameters:
 *  - tlsfSize_t: this encodes the "addresses" and size of
 *  each block. It must be able to contain the entire range
 *  of addresses
 *  - this is partly compensated by ELEMENT_SIZE_LOG2/BYTES which
 *  contains the number of bytes per "unit" of memory. This
 *  increases granularity to be able to address more with fewer
 *  bits. Therefore, tlsfSize_t needs only to encode
 *  the total size in ELEMENT_SIZE_BYTES unit.
 *  - FL_MAX_LOG2: The total size of memory is (1<<FL_MAX_LOG2) * ELEMENT_SIZE_BYTES
 */


/*
 * Number of bytes the "size" of a block
 * is a multiple of. A size of 1 means 1 word (4 bytes, 32 bits)
 * This is used to "extend" the range possible by trading some
 * granularity in allocation. The maximum size allocatable/addressable
 * will be ELEMENT_SIZE_BYTES*2^{sizeof(tlsfSize_t)*8} bytes
 * NOTE: Must be at least 2 bytes
 */
#define ELEMENT_SIZE_LOG2 2
#define ELEMENT_SIZE_BYTES 4

/*
 * Alignment of returned block
 * Mostly an 8 byte alignment
 */
#define ALIGN_LOG2  3   // 8 byte alignment
#define ALIGN_BYTES 8

/*
 * Alignment of full block (including headers)
 * Can be ALIGN_LOG2 or half of that if 2 fields
 * of tlsfSize_t will "pad" the full block to start
 * on an ALIGN_BYTES alignment
 */
#define ALIGN_BLOCK_LOG2 2 // 4 byte
#define ALIGN_BLOCK_BYTES 4

/*
 * Number of subdivisions in the second-level list
 * 2^SL_COUNT_LOG2 must be less than SIZE_TLSF_SIZE
 */
#define SL_COUNT_LOG2 3

/*
 * Support allocations/memory of size up to (1 << FL_MAX_LOG2) * ELEMENT_SIZE_BYTES
 * Note that bitmaps are of type tlsfSize_t so FL_MAX_LOG2 must be
 * less than or equal to SIZE_TLSF_SIZE.
 * For 64K memory: 14 (if ELEMENT_SIZE_LOG2 == 2)
 * For 1024K memory: 18 (if ELEMENT_SIZE_LOG2 == 2)
 * For 4096K memory: 20 (if ELEMENT_SIZE_LOG2 == 2)
 * For 16384K memory: 22 (if ELEMENT_SIZE_LOG2 == 2)
 * For 32768K memory: 23 (if ELEMENT_SIZE_LOG2 == 2)
 * For 65536K memory: 24 (if ELEMENT_SIZE_LOG2 == 2)
 * For 128MB memory: 25 (if ELEMENT_SIZE_LOG2 == 2)
 * For 256MB memory: 26 (if ELEMENT_SIZE_LOG2 == 2)
 * For 512MB memory: 27 (if ELEMENT_SIZE_LOG2 == 2)
 */
#define FL_MAX_LOG2 27


/* Size specific functions:
 * fls: Find last set: position of the MSB set to 1
 * ffs: find first set: position of the LSB set to 1
 */
// IMPORTANT: CHANGE THESE FUNCTIONS IF tlsfSize_t CHANGES
// ST_SIZE/LD_SIZE will be used to st/ld values of type tlsfSize_t
#if SIZE_TLSF_SIZE == 16
#define FLS fls16
#define ST_SIZE(addr, value) (*((u16*)(addr)) = (u16)(value))
#define LD_SIZE(var, addr)   ((var) = *((u16*)(addr)))
#elif SIZE_TLSF_SIZE == 32
#define FLS fls32
#define ST_SIZE(addr, value) (*((u32*)(addr)) = (u32)(value))
#define LD_SIZE(var, addr)   ((var) = *((u32*)(addr)))
#else
#error "Unknown size for tlsfSize_t"
#endif

#define MALLOC_COPY(dest,src,nbytes) memcpy(dest, src, nbytes)

// Usually no need of any fences on x86 for this
#define FENCE_LOAD
#define FENCE_STORE

/**
 * @brief Function to compute the address of a field
 *
 * Usage: ADDR_OF(type, ptr, field) <=> &(ptr->field)
 * where 'type' is the type of 'ptr'
 */
#define ADDR_OF(type, ptr, field) (ptr + offsetof(type, field))

/**
 * @brief Function to compute the address of an element of a field that
 * is an array
 *
 * Usage: ADDR_OF_1(type, ptr, fieldtype, field, idx) <=> &(ptr->field[idx])
 * where 'type' is the type of 'ptr' and 'field' is of type 'fieldtype[]'
 */
#define ADDR_OF_1(type, ptr, fieldtype, field, idx) (ADDR_OF(type, ptr, field) + sizeof(fieldtype)*idx)

/**
 * @brief Function to compute the address of an element of a field
 * that is a 2D array
 *
 * Usage: ADDR_OF_2(type, ptr, fieldtype, field, idx, dim2, idx2) <=> &(ptr->field[idx][idx2])
 * where 'type' is the type of 'ptr' and 'field' is of type 'fieldtype[][dim2]'
 */
#define ADDR_OF_2(type, ptr, fieldtype, field, idx, dim2, idx2)         \
    (ADDR_OF(type, ptr, field) + sizeof(fieldtype)*(idx*dim2 + idx2))



// A whole bunch of internal methods used only in this file.

#define myStaticAssert(e) extern char (*myStaticAssert(void))[sizeof(char[1-2*!(e)])]

#define _NULL 0ULL

#define GET_ADDRESS(header_addr)                                \
    (pgStart + (header_addr.address << ELEMENT_SIZE_LOG2))

#define SET_VALUE(header_addr, addr)                                    \
    do { (header_addr).address = 0ULL;                                  \
        (header_addr).value = ((u64)(addr) - pgStart) >> ELEMENT_SIZE_LOG2; \
    } while(0)


#define SET_NULL(header_addr) SET_VALUE(header_addr, pgStart)
#define IS_NULL(addr)         ((addr) == pgStart)


myStaticAssert(ELEMENT_SIZE_BYTES >= 2);
myStaticAssert(SL_COUNT_LOG2 < SIZE_TLSF_SIZE_LOG2);
myStaticAssert(FL_MAX_LOG2 <= SIZE_TLSF_SIZE);

/*
 * Some computed values:
 *  - SL_COUNT: Number of buckets in each SL list
 *  - FL_COUNT_SHIFT: For the first SL_COUNT, we do not need to maintain
 *  first level lists (since they all go in the "0th" one.
 *  - FL_COUNT: Number of buckets in FL (so number of SL bitmaps)
 *  - ZERO_LIST_SIZE: Size under which we are in SL 0
 */
enum computedVals {
    SL_COUNT        = (1 << SL_COUNT_LOG2),
    FL_COUNT_SHIFT  = (SL_COUNT_LOG2),
    FL_COUNT        = (FL_MAX_LOG2 - FL_COUNT_SHIFT + 1),
    ZERO_LIST_SIZE  = (1 << FL_COUNT_SHIFT),
};

/*
 * Header of a block (allocated or free) of memory. Note that not all
 * fields are valid at all times:
 *
 * Free block:
 * tlsfSize_t prevFreeBlock; <-- can NEVER be 0 or 1 (would indicate
 *  an occupied block). This should be fine as that would mean that
 *  the address of the previous block is 0x4 (in absolute) which is
 *  never going to happen...
 * tlsfSize_t sizeBlock;
 * tlsfSize_t nextFreeBlock; <-- This field aligned 2^ALIGN_LOG2 bytes
 * <SPACE>
 * tlsfSize_t sizeBlock; (used by next occupied block); must be at the very end
 *
 * Used block:
 * tlsfSize_t prevFreeBlock <- not used as that:
 *  - bit 0 indicates whether the previous block is free (1) or not (0)
 *  - the rest MUST be 0 to indicate an occupied block.
 * tlsfSize_t sizeBlock
 *
 * Note that prevFreeBlock and nextFreeBlock are actually pointers
 * but we only consider the lower tlsfSize_t bits. The rest are zero
 * since the total size of the memory is small.
 * */
typedef struct {
    tlsfSize_t prevFreeBlock; // __attribute__ (( aligned (ALIGN_BLOCK_BYTES) ));
    tlsfSize_t sizeBlock;

    tlsfSize_t nextFreeBlock; // __attribute__ (( aligned (ALIGN_BYTES) ));
} header_t;

// Union to hold both pointer to header and tlsfSize_t which
// are used inter-changeably
typedef union {
    u64 address;
    tlsfSize_t value;
} headerAddr_t;

// Some assertions to make sure things are OK
myStaticAssert(sizeof(header_t) == 3*SIZE_TLSF_SIZE/8);
myStaticAssert(2*sizeof(tlsfSize_t) >= ELEMENT_SIZE_BYTES);
myStaticAssert(offsetof(header_t, nextFreeBlock) % ELEMENT_SIZE_BYTES == 0);
myStaticAssert(ZERO_LIST_SIZE == SL_COUNT);
myStaticAssert(sizeof(char) == 1);

/* The pool (containing the bitmaps and the lists of
 * free blocks.
 */
typedef struct {
    tlsfSize_t flAvailOrNot;                // bitmap that indicates the presence
    // (1) or absence (0) of free blocks
    // in blocks[i][*]
    tlsfSize_t slAvailOrNot[FL_COUNT];      // Second level bitmaps
    tlsfSize_t blocks[FL_COUNT][SL_COUNT];  // Pointers to starts of free-lists

    // The padding works as follows:
    //  - ((FL_COUNT*(1+SL_COUNT) + 1)*(SIZE_TLSF_SIZE/8)) is the size of what is above in bytes
    //      + Call this quantity x
    //  - we know that x = k*ALIGN_BYTES + r and we want x + p = (k+1)*ALIGN_BYTES
    //  - Therefore p = ALIGN_BYTES - r except when r == 0 in which case p = 0
    //  r is (x & (ALIGN_BYTES - 1)
#define _R_SIZE_TEMP (((FL_COUNT*(1+SL_COUNT) + 1)*(SIZE_TLSF_SIZE/8)) & (ALIGN_BYTES - 1))
    u8 _padding[_R_SIZE_TEMP?(ALIGN_BYTES - _R_SIZE_TEMP):0];
#undef _R_SIZE_TEMP

    /* nullBlock will be used to mark NULL blocks. This is important
     * since prev/next blocks can never be NULL as that is also used
     * as the busy flag
     */
    header_t nullBlock; // Contains 3*tlsfSize_t
#define _R_SIZE_TEMP ((3*(SIZE_TLSF_SIZE/8)) & (ALIGN_BYTES - 1))
    u8 _padding2[_R_SIZE_TEMP?(ALIGN_BYTES - _R_SIZE_TEMP):0];
#undef _R_SIZE_TEMP
    header_t mainBlock; // Main memory block (this needs to be aligned ALIGN_BYTES)
} pool_t;

myStaticAssert(offsetof(pool_t, nullBlock) % ALIGN_BYTES == 0);
myStaticAssert(offsetof(pool_t, mainBlock) % ALIGN_BYTES == 0);


/* Some contants */
static const tlsfSize_t GoffsetToStart      = offsetof(header_t, nextFreeBlock);
static const tlsfSize_t GusedBlockOverhead  = (offsetof(header_t, nextFreeBlock)) >> ELEMENT_SIZE_LOG2;

// This computation just rounds it up to the closest multiple of ELEMENT_SIZE_BYTES
static const tlsfSize_t GminBlockRealSize   = (sizeof(header_t) + sizeof(tlsfSize_t) + ELEMENT_SIZE_BYTES - 1)
    >> ELEMENT_SIZE_LOG2;
static const tlsfSize_t GmaxBlockRealSize   = (tlsfSize_t)((1<<FL_MAX_LOG2) - 1);

static int myffs(tlsfSize_t val) {
    return FLS(val & (~val + 1));
}

/* Utility functions for blocks. All these functions that
 * a "me" block pointer as their first argument
 */
u64 addressForBlock(u64 me /* header_t* */) {
    u64 t;
    t = (me + GoffsetToStart);
    return t;
}

headerAddr_t blockForAddress(u64 pgStart, u64 me) {
    headerAddr_t res;

    SET_VALUE(res, (me - GoffsetToStart));
    return res;
}

// R from prevFreeBlock (tlsfSize_t)
static bool isBlockFree(u64 me /* const header_t* */) {
    tlsfSize_t temp;
    LD_SIZE(temp, ADDR_OF(header_t, me, prevFreeBlock));
    FENCE_LOAD;
    return ((temp & ~0x1) != 0);   // Checks if there is a non-null bit
    // (ie: a valid pointer) past the first bit
}

static bool isPrevBlockFree(u64 me /* const header_t* */) {
    if(isBlockFree(me)) {
        return false;
    } else {
        tlsfSize_t temp;
        LD_SIZE(temp, ADDR_OF(header_t, me, prevFreeBlock));
        FENCE_LOAD;
        return (temp & 0x1);
    }
}


static headerAddr_t getPrevBlock(u64 pgStart, u64 me /* const header_t* */) {
    headerAddr_t res;
    ASSERT(isPrevBlockFree(me));
    tlsfSize_t offset;
    LD_SIZE(offset, me - sizeof(tlsfSize_t));
    FENCE_LOAD;
    offset += GusedBlockOverhead;   // Gets sizeBlock2 in previous block (which is free)

    SET_VALUE(res, (me - (offset << ELEMENT_SIZE_LOG2)));
    return res;
}

static headerAddr_t getNextBlock(u64 pgStart, u64 me /* const header_t* */) {
    headerAddr_t res;
    tlsfSize_t offset;
    LD_SIZE(offset, ADDR_OF(header_t, me, sizeBlock));
    FENCE_LOAD;

    offset += GusedBlockOverhead;
    SET_VALUE(res, (me + (offset << ELEMENT_SIZE_LOG2)));

    return res;
}

static headerAddr_t getPrevFreeBlock(u64 me /* const header_t* */) {
    headerAddr_t res = { .address = 0ULL };
    ASSERT(isBlockFree(me)); /* getPrevFreeBlock not called on free block */

    LD_SIZE(res.value, ADDR_OF(header_t, me, prevFreeBlock));
    FENCE_LOAD;
    return res;
}

static headerAddr_t getNextFreeBlock(u64 me /* const header_t* */) {
    headerAddr_t res = { .address = 0ULL };
    ASSERT(isBlockFree(me)); /* getNextFreeBlock not called on free block" */

    LD_SIZE(res.value, ADDR_OF(header_t, me, nextFreeBlock));
    FENCE_LOAD;
    return res;
}

static void linkFreeBlocks(u64 pgStart, headerAddr_t first, headerAddr_t second) {

    ASSERT(isBlockFree(GET_ADDRESS(first)));  /* linkFreeBlocks arg1 not free */
    ASSERT(isBlockFree(GET_ADDRESS(second))); /* linkFreeBlocks arg2 not free */

    /* Consec blocks cannot be free */
    ASSERT(getNextBlock(pgStart, GET_ADDRESS(first)).value != second.value);

    /* linkFreeBlocks arg1 misaligned */
    ASSERT((GET_ADDRESS(first) & ((1<<ELEMENT_SIZE_LOG2) - 1)) == 0);

    /* linkFreeBlocks arg2 misaligned */
    ASSERT((GET_ADDRESS(second) & ((1<<ELEMENT_SIZE_LOG2) - 1)) == 0);

    ST_SIZE(ADDR_OF(header_t, GET_ADDRESS(first), nextFreeBlock), second.value);
    ST_SIZE(ADDR_OF(header_t, GET_ADDRESS(second), prevFreeBlock), first.value);
    FENCE_STORE;
}

// The previous block of a free block is *ALWAYS* used
// (otherwise, it would be coalesced)
static void markPrevBlockUsed(u64 me /* header_t* */) {
    ASSERT(!isBlockFree(me)); /* markPrevBlockUsed can only be called on used block */
    ST_SIZE(ADDR_OF(header_t, me, prevFreeBlock), 0);
    FENCE_STORE;
}

static void markPrevBlockFree(u64 me /* header_t* */) {
    ASSERT(!isBlockFree(me)); /* markPrevBlockFree can only be called on used block */
    ST_SIZE(ADDR_OF(header_t, me, prevFreeBlock), 1);
    FENCE_STORE;
}

static void markBlockUsed(u64 pgStart, u64 me/* header_t* */) {
    ST_SIZE(ADDR_OF(header_t, me, prevFreeBlock), 0);
    FENCE_STORE;

    headerAddr_t nextBlock = getNextBlock(pgStart, me);

    if(!isBlockFree(GET_ADDRESS(nextBlock)))
        markPrevBlockUsed(GET_ADDRESS(nextBlock));
}

static void markBlockFree(u64 pgStart, u64 me /* header_t* */) {
    // To mark a block as free, we need to duplicate it's size
    // at the end of the block so that the next block can find the beginning
    // of the free block.
    tlsfSize_t temp;
    LD_SIZE(temp, ADDR_OF(header_t, me, sizeBlock));
    FENCE_LOAD;
    u64 locationToWrite = me +
        ((temp + GusedBlockOverhead) << ELEMENT_SIZE_LOG2) - sizeof(tlsfSize_t);
    ST_SIZE(locationToWrite, temp);

    ST_SIZE(ADDR_OF(header_t, me, prevFreeBlock), 0xBEEF);  // Some value that is not 0 or 1 for now
    // will be updated when this
    // free block is actually inserted

    u64 nextBlockPtr = GET_ADDRESS(getNextBlock(pgStart, me));
    FENCE_STORE;

    if(!isBlockFree(nextBlockPtr))  // Usually, no two free blocks follow each other but it may happen temporarily
        markPrevBlockFree(nextBlockPtr);
}


/*
 * Allocation helpers (size and alignement constraints)
 */
static tlsfSize_t getRealSizeOfRequest(u64 size) {
    if((size >> ELEMENT_SIZE_LOG2) > GmaxBlockRealSize)
        return 0; // Too big to allocate

    if(size < ((GminBlockRealSize - GusedBlockOverhead) << ELEMENT_SIZE_LOG2)) {
        return GminBlockRealSize - GusedBlockOverhead;
    }

    // We want this to be aligned so that the total w/ the header is ALIGN_BYTES but
    // since the header may be smaller than ALIGN_BYTES, we do this weird computation
    // to take it into account
    size = ((size + ALIGN_BYTES - GoffsetToStart - 1) & ~(ALIGN_BYTES - 1))
        + GoffsetToStart;

    ASSERT(size % ELEMENT_SIZE_BYTES == 0);

    size >>= ELEMENT_SIZE_LOG2;
    ASSERT(size >= (GminBlockRealSize - GusedBlockOverhead));

    return size;
}

/* Two-level function to determine indices. This is pretty much
 * taken straight from the specs
 */
static void mappingInsert(tlsfSize_t realSize, int* flIndex, int* slIndex) {
    int tf, ts;

    if(realSize < ZERO_LIST_SIZE) {
        tf = 0;
        ts = realSize;
    } else {
        tf = FLS(realSize);
        ts = (realSize >> (tf - SL_COUNT_LOG2)) - (SL_COUNT);
        tf -= (FL_COUNT_SHIFT - 1);
    }
    *flIndex = tf;
    *slIndex = ts;
}

/* For allocations, we want to round-up to the next block size
 * so that we are sure that any block will work so we can
 * pick it in constant time.
 */
static void mappingSearch(tlsfSize_t realSize, int* flIndex, int* slIndex) {
    if(realSize >= ZERO_LIST_SIZE) {
        // If not, we don't need to do anything since the mappingInsert
        // will return the correct thing
        realSize += (1 << (FLS(realSize) - SL_COUNT_LOG2)) - 1;
    }
    mappingInsert(realSize, flIndex, slIndex);
}

/* Search for a suitable free block:
 *  - first search for a block in the fl/sl indicated.
 *  - if not found, search for higher sl (bigger) with same fl
 *  - if not found, search for higher fl and sl in that
 *
 *  Returns the header of a free block as well as flIndex and slIndex
 *  that block was taken from.
 */
static headerAddr_t findFreeBlockForRealSize(u64 pgStart, tlsfSize_t realSize,
                                              int* flIndex, int* slIndex) {

    int tf, ts;
    headerAddr_t res = { .address = 0ULL };
    tlsfSize_t flBitMap, slBitMap;

    mappingSearch(realSize, flIndex, slIndex);
    tf = *flIndex;
    ts = *slIndex;

    if(tf >= FL_COUNT) {
        SET_NULL(res); // This means the alloc size is too large
        return res;
    }

    LD_SIZE(slBitMap, ADDR_OF_1(pool_t, pgStart, tlsfSize_t, slAvailOrNot, tf));
    FENCE_LOAD;
    slBitMap &= (~0 << ts); // This takes all SL bins bigger or requal to ts
    if(slBitMap == 0) {
        // We don't have any non-zero block here so we look at the flAvailOrNot map
        LD_SIZE(flBitMap, ADDR_OF(pool_t, pgStart, flAvailOrNot));
        FENCE_LOAD;
        flBitMap &= (~0 << (tf + 1));
        if(flBitMap == 0) {
            SET_NULL(res);
            return res;
        }

        // Look for the first bit that is a one
        tf = myffs(flBitMap);
        ASSERT(tf > *flIndex);
        *flIndex = tf;

        // Now we get the slBitMap. There is definitely a 1 in there since there is a 1 in tf
        LD_SIZE(slBitMap, ADDR_OF_1(pool_t, pgStart, tlsfSize_t, slAvailOrNot, tf));
        FENCE_LOAD;
    }

    ASSERT(slBitMap != 0);

    ts = myffs(slBitMap);
    *slIndex = ts;

    LD_SIZE(res.value, ADDR_OF_2(pool_t, pgStart, tlsfSize_t, blocks, tf, SL_COUNT, ts));
    FENCE_LOAD;
    return res;
}


/* Helpers to manipulate the free lists */
static void removeFreeBlock(u64 pgStart, headerAddr_t freeBlock) {
    /* header_t* */ u64 freeBlockPtr = GET_ADDRESS(freeBlock);
    tlsfSize_t tempSz;

    ASSERT(isBlockFree(freeBlockPtr));

    int flIndex, slIndex;
    LD_SIZE(tempSz, ADDR_OF(header_t, freeBlockPtr, sizeBlock));
    FENCE_LOAD;
    mappingInsert(tempSz, &flIndex, &slIndex);

    // First remove it from the chain of free blocks
    headerAddr_t prevFreeBlock = getPrevFreeBlock(freeBlockPtr);
    headerAddr_t nextFreeBlock = getNextFreeBlock(freeBlockPtr);

    ASSERT(prevFreeBlock.value !=0 && isBlockFree(GET_ADDRESS(prevFreeBlock)));
    ASSERT(nextFreeBlock.value !=0 && isBlockFree(GET_ADDRESS(nextFreeBlock)));

    linkFreeBlocks(pgStart, prevFreeBlock, nextFreeBlock);

    // Check if we need to change the head of the blocks list
    LD_SIZE(tempSz, ADDR_OF_2(pool_t, pgStart, tlsfSize_t, blocks, flIndex, SL_COUNT, slIndex));
    FENCE_LOAD;
    if(tempSz == freeBlock.value) {
        ST_SIZE(ADDR_OF_2(pool_t, pgStart, tlsfSize_t, blocks, flIndex, SL_COUNT, slIndex),
                nextFreeBlock.value);

        // If the new head is nullBlock then we clear the bitmaps to
        // indicate that we have no more blocks available here
        if(GET_ADDRESS(nextFreeBlock) == ADDR_OF(pool_t, pgStart, nullBlock)) {
            LD_SIZE(tempSz, ADDR_OF_1(pool_t, pgStart, tlsfSize_t, slAvailOrNot, flIndex));
            FENCE_LOAD;
            tempSz &= ~(1 << slIndex); // Clear me bit
            ST_SIZE(ADDR_OF_1(pool_t, pgStart, tlsfSize_t, slAvailOrNot, flIndex), tempSz);

            // Check if slAvailOrNot is now 0
            if(tempSz == 0) {
                LD_SIZE(tempSz, ADDR_OF(pool_t, pgStart, flAvailOrNot));
                FENCE_LOAD;

                tempSz &= ~(1 << flIndex);
                ST_SIZE(ADDR_OF(pool_t, pgStart, flAvailOrNot), tempSz);
            }
        }
        FENCE_STORE;
    }
}

void addFreeBlock(u64 pgStart, headerAddr_t freeBlock) {
    /* header_t* */ u64 freeBlockPtr = GET_ADDRESS(freeBlock);
    int flIndex, slIndex;
    tlsfSize_t tempSz;

    LD_SIZE(tempSz, ADDR_OF(header_t, freeBlockPtr, sizeBlock));
    FENCE_LOAD;
    mappingInsert(tempSz, &flIndex, &slIndex);

    headerAddr_t currentHead = { .address = 0ULL }, temp = { .address = 0ULL };

    LD_SIZE(currentHead.value, ADDR_OF_2(pool_t, pgStart, tlsfSize_t, blocks, flIndex, SL_COUNT, slIndex));
    FENCE_LOAD;

    ASSERT(GET_ADDRESS(currentHead));
    ASSERT(freeBlockPtr);
    ASSERT(freeBlockPtr != ADDR_OF(pool_t, pgStart, nullBlock));

    // Set the links properly
    SET_VALUE(temp, ADDR_OF(pool_t, pgStart, nullBlock));

    ST_SIZE(ADDR_OF(header_t, freeBlockPtr, prevFreeBlock), temp.value);
    FENCE_STORE;
    linkFreeBlocks(pgStart, freeBlock, currentHead);

    ST_SIZE(ADDR_OF(header_t, freeBlockPtr, prevFreeBlock), temp.value);
    ST_SIZE(ADDR_OF(header_t, freeBlockPtr, nextFreeBlock), currentHead.value);
    ST_SIZE(ADDR_OF(header_t, GET_ADDRESS(currentHead), prevFreeBlock), freeBlock.value);   // Does not matter
    // if the head was the
    // nullBlock,
    // that value is ignored anyways

    // Update the bitmaps
    ST_SIZE(ADDR_OF_2(pool_t, pgStart, tlsfSize_t, blocks, flIndex, SL_COUNT, slIndex), freeBlock.value);

    LD_SIZE(tempSz, ADDR_OF(pool_t, pgStart, flAvailOrNot));
    FENCE_LOAD;

    LD_SIZE(tempSz, ADDR_OF_1(pool_t, pgStart, tlsfSize_t, slAvailOrNot, flIndex));
    FENCE_LOAD;
    if(!(tempSz & (1 << slIndex))) {
        // Here it wasn't stored so we definitely have to update

        tempSz |= (1 << slIndex);
        ST_SIZE(ADDR_OF_1(pool_t, pgStart, tlsfSize_t, slAvailOrNot, flIndex), tempSz);

        LD_SIZE(tempSz, ADDR_OF(pool_t, pgStart, flAvailOrNot));
        FENCE_LOAD;
        if(!(tempSz & (1 << flIndex))) {
            tempSz |= (1 << flIndex);
            ST_SIZE(ADDR_OF(pool_t, pgStart, flAvailOrNot), tempSz);
        }
    }

    FENCE_STORE;
}

/* Split origBlock so that origBlock is realSize long and
 * returns a pointer to the remaining block which is free
 */
static headerAddr_t splitBlock(u64 pgStart, headerAddr_t origBlock, tlsfSize_t realSize) {
    headerAddr_t remainingBlock = { .address = 0ULL };

    /* header_t* */ u64 origBlockPtr = GET_ADDRESS(origBlock);
    tlsfSize_t origBlockSize;
    LD_SIZE(origBlockSize, ADDR_OF(header_t, origBlockPtr, sizeBlock));
    FENCE_LOAD;
    ASSERT(origBlockSize > realSize + GminBlockRealSize);

    tlsfSize_t remainingSize = origBlockSize - realSize - GusedBlockOverhead;

    SET_VALUE(remainingBlock,
              (origBlockPtr + GoffsetToStart + (realSize << ELEMENT_SIZE_LOG2)));

    // Take care of the remaining block (set its size and mark it as free)
    ST_SIZE(ADDR_OF(header_t, GET_ADDRESS(remainingBlock), sizeBlock), remainingSize);
    FENCE_STORE;
    markBlockFree(pgStart, GET_ADDRESS(remainingBlock));

    // Set the size of the original block properly
    ST_SIZE(ADDR_OF(header_t, origBlockPtr, sizeBlock), realSize);
    FENCE_STORE;

    return remainingBlock;
}

/* Absorbs nextBlock into freeBlock making one much
 * larger freeBlock. nextBlock must be physically next
 * to freeBlock
 */
static void absorbNext(u64 pgStart, headerAddr_t freeBlock, headerAddr_t nextBlock) {
    /* header_t * */ u64 freeBlockPtr = GET_ADDRESS(freeBlock);
    /* header_t * */ u64 nextBlockPtr = GET_ADDRESS(nextBlock);
    tlsfSize_t tempSz, tempSz2;

    ASSERT(isBlockFree(freeBlockPtr));
    ASSERT(isBlockFree(nextBlockPtr));
    ASSERT(getNextBlock(pgStart, freeBlockPtr).value == nextBlock.value);

    LD_SIZE(tempSz, ADDR_OF(header_t, freeBlockPtr, sizeBlock));
    LD_SIZE(tempSz2, ADDR_OF(header_t, nextBlockPtr, sizeBlock));
    FENCE_LOAD;
    tempSz += tempSz2 + GusedBlockOverhead; // freeBlockPtr->sizeBlock += nextBlockPtr->sizeBlock + GusedBlockOverhead
    ST_SIZE(ADDR_OF(header_t, freeBlockPtr, sizeBlock), tempSz);
    FENCE_STORE;
    markBlockFree(pgStart, freeBlockPtr); // will update the other field
}

/* Merge with the previous block if it is free (toBeFreedBlock must
 * be used so isPrevBlockFree can return true)
 * Returns the resulting free block (either toBeFreedBlock marked as free
 * or the larger block)
 */
static headerAddr_t mergePrevious(u64 pgStart, headerAddr_t toBeFreedBlock) {
    /* header_t * */ u64 toBeFreedBlockPtr = GET_ADDRESS(toBeFreedBlock);

    ASSERT(!isBlockFree(toBeFreedBlockPtr));
    if(isPrevBlockFree(toBeFreedBlockPtr)) {
        // Get the previous block
        headerAddr_t prevBlock = getPrevBlock(pgStart, toBeFreedBlockPtr);
        // Remove it from the free-lists (since it will be made bigger)
        removeFreeBlock(pgStart, prevBlock);

        // Now we mark the toBeFreedBlock as free and merge it with the other one
        markBlockFree(pgStart, toBeFreedBlockPtr);
        absorbNext(pgStart, prevBlock, toBeFreedBlock);
        toBeFreedBlock.value = prevBlock.value;
    } else {
        markBlockFree(pgStart, toBeFreedBlockPtr);
    }

    ASSERT(isBlockFree(toBeFreedBlockPtr));
    return toBeFreedBlock;
}

/* Merges a block with its block if that one is free as well. The input
 * block must be free to start with
 */
static headerAddr_t mergeNext(u64 pgStart, headerAddr_t freeBlock) {
    ASSERT(isBlockFree(GET_ADDRESS(freeBlock)));
    headerAddr_t nextBlock = getNextBlock(pgStart, GET_ADDRESS(freeBlock));
    if(isBlockFree(GET_ADDRESS(nextBlock))) {
        removeFreeBlock(pgStart, nextBlock);
        absorbNext(pgStart, freeBlock, nextBlock);
    }

    return freeBlock;
}

/* Pool construction */
static void initializePool(u64 pgStart, tlsfSize_t poolRealSize) {

    headerAddr_t mainBlockAddr, nullBlockAddr;

    SET_VALUE(nullBlockAddr, ADDR_OF(pool_t, pgStart, nullBlock));
    SET_VALUE(mainBlockAddr, ADDR_OF(pool_t, pgStart, mainBlock));

    // Initialize the bitmaps to 0

    ST_SIZE(ADDR_OF(pool_t, pgStart, flAvailOrNot), 0);
    int i, j;
    for(i=0; i<FL_COUNT; ++i) {
        ST_SIZE(ADDR_OF_1(pool_t, pgStart, tlsfSize_t, slAvailOrNot, i), 0);
        for(j=0; j<SL_COUNT; ++j) {
            ST_SIZE(ADDR_OF_2(pool_t, pgStart, tlsfSize_t, blocks, i, SL_COUNT, j), nullBlockAddr.value);
        }
    }

    // Initialize the main block properly
    // We do something a little special in the main block: we take a little bit
    // of it at the end to create a zero-sized block that is used
    // to act as a sentinel. We therefore need GusedBlockOverhead space in the
    // end (the 2* is because we also account for our own overhead)
    ST_SIZE(ADDR_OF(header_t, ADDR_OF(pool_t, pgStart, mainBlock), sizeBlock),
            poolRealSize - 2*GusedBlockOverhead);

    ST_SIZE(ADDR_OF(header_t, ADDR_OF(pool_t, pgStart, mainBlock), nextFreeBlock), nullBlockAddr.value);

    // This code is from markBlockFree (it marks mainBlock as free)
    // We paste it here to remove some of the actions on next/prev block
    // which bother valgrind
    {
        u64 locationToWrite = ADDR_OF(pool_t, pgStart, mainBlock) +
            ((poolRealSize - GusedBlockOverhead) << ELEMENT_SIZE_LOG2) - sizeof(tlsfSize_t);
        ST_SIZE(locationToWrite, poolRealSize - 2*GusedBlockOverhead);

        ST_SIZE(ADDR_OF(header_t, ADDR_OF(pool_t, pgStart, mainBlock), prevFreeBlock), 0xBEEF);  // Some value that is not 0 or 1 for now
    }
    FENCE_STORE;

    // Add the sentinel
    u64 sentinel = (ADDR_OF(pool_t, pgStart, mainBlock) +
                    ((poolRealSize - GusedBlockOverhead) << ELEMENT_SIZE_LOG2));

    ST_SIZE(ADDR_OF(header_t, sentinel, sizeBlock), 0);
    ST_SIZE(ADDR_OF(header_t, sentinel, prevFreeBlock), 1); // This is mark prevBlockFree for sentinel

    // Initialize the nullBlock properly
    ST_SIZE(ADDR_OF(header_t, ADDR_OF(pool_t, pgStart, nullBlock), sizeBlock), 0);
    ST_SIZE(ADDR_OF(header_t, ADDR_OF(pool_t, pgStart, nullBlock), prevFreeBlock), nullBlockAddr.value);
    ST_SIZE(ADDR_OF(header_t, ADDR_OF(pool_t, pgStart, nullBlock), nextFreeBlock), nullBlockAddr.value);
    FENCE_STORE;

    // Add the big free block
    addFreeBlock(pgStart, mainBlockAddr);
}

static u32 tlsfInit(u64 pgStart, u64 size) {
    tlsfSize_t realSizeForPool = size >> ELEMENT_SIZE_LOG2;
    /* The memory will be layed out as follows:
     *  - at location: the pool structure is used
     *  - the first free block starts right after that (aligned)
     */

    tlsfSize_t poolHeaderSize = sizeof(pool_t) + ALIGN_BLOCK_BYTES - 1;
    poolHeaderSize = (poolHeaderSize & ~(ALIGN_BLOCK_BYTES - 1));

    // Now we have a poolHeaderSize that is big enough to contain the pool
    // and right after it, we can start the big block.

    tlsfSize_t poolRealSize = realSizeForPool -
        ((poolHeaderSize + ELEMENT_SIZE_BYTES - 1) >> ELEMENT_SIZE_LOG2);

    if(poolRealSize < GminBlockRealSize || poolRealSize > GmaxBlockRealSize) {
        DO_DEBUG(DEBUG_LVL_WARN) {
            PRINTF("WARN: Space mismatch allocating TLSF pool at 0x%"PRIx64" of sz %d (user sz: %d)\n",
                   pgStart, (u32)poolRealSize, (u32)(poolRealSize << ELEMENT_SIZE_LOG2));
            PRINTF("WARN: Sz must be at least %d and at most %d\n", GminBlockRealSize, GmaxBlockRealSize);
        }
        return -1; // Can't allocate pool
    }

    DO_DEBUG(DEBUG_LVL_INFO) {
        PRINTF("INFO: Allocating a TLSF pool at 0x%"PRIx64" of sz %d (user sz: %d)\n",
               pgStart, (u32)poolRealSize, (u32)(poolRealSize << ELEMENT_SIZE_LOG2));
    }

    initializePool(pgStart, poolRealSize);

    return 0;
}

//static u32 tlsfResize(u64 pgStart, u64 newsize) {
//    return -1;
//    // TODO: Not implemented yet
//}

static u64 tlsfMalloc(u64 pgStart, u64 size) {
    u64 result = 0ULL;
    tlsfSize_t allocSize, returnedSize;
    int flIndex, slIndex;
    headerAddr_t freeBlock, remainingBlock;

    allocSize = getRealSizeOfRequest(size);

    if(allocSize == 0 && size != 0) {
        DO_DEBUG(DEBUG_LVL_VERB) {
            PRINTF("VERB: tlsfMalloc @0x%"PRIx64" returning NULL for too large size %"PRIu64"\n",
                   pgStart, size);
        }
        return _NULL;
    }

    freeBlock = findFreeBlockForRealSize(pgStart, allocSize, &flIndex, &slIndex);
    DO_DEBUG(DEBUG_LVL_VERB) {
        PRINTF("VERB: tslf_malloc @0x%"PRIx64" found a free block at 0x%"PRIx64"\n", pgStart,
               addressForBlock(GET_ADDRESS(freeBlock)));
    }
    /* header_t * */ u64 freeBlockPtr = GET_ADDRESS(freeBlock);

    if(IS_NULL(freeBlockPtr)) {
        DO_DEBUG(DEBUG_LVL_VERB) {
            PRINTF("VERB: tlsfMalloc @ 0x%"PRIx64" returning NULL for size %"PRIu64"\n",
                   pgStart, size);
        }
        return _NULL;
    }
    removeFreeBlock(pgStart, freeBlock);
    LD_SIZE(returnedSize, ADDR_OF(header_t, freeBlockPtr, sizeBlock));
    FENCE_LOAD;

    if(returnedSize > allocSize + GminBlockRealSize) {
        remainingBlock = splitBlock(pgStart, freeBlock, allocSize);
        DO_DEBUG(DEBUG_LVL_VERB) {
            PRINTF("VERB: tlsfMalloc @0x%"PRIx64" split block and re-added to free list 0x%"PRIx64"\n", pgStart,
                   addressForBlock(GET_ADDRESS(remainingBlock)));
        }
        addFreeBlock(pgStart, remainingBlock);
    }

    markBlockUsed(pgStart, freeBlockPtr);

    result = addressForBlock(freeBlockPtr);
    DO_DEBUG(DEBUG_LVL_VERB) {
        PRINTF("VERB: tlsfMalloc @ 0x%"PRIx64" returning 0x%"PRIx64" for size %"PRIu64"\n",
               pgStart, result, size);
    }
    return result;
}

static void tlsfFree(u64 pgStart, u64 ptr) {
    headerAddr_t bl;

    DO_DEBUG(DEBUG_LVL_VERB) {
        PRINTF("VERB: tlsfFree @ 0x%"PRIx64" going to free 0x%"PRIx64"\n",
               pgStart, ptr);
    }

    bl = blockForAddress(pgStart, ptr);

    bl = mergePrevious(pgStart, bl);
    bl = mergeNext(pgStart, bl);
    addFreeBlock(pgStart, bl);
}

static u64 tlsfRealloc(u64 pgStart, u64 ptr, u64 size) {
    // First deal with corner cases:
    //  - non-zero size with null pointer is like malloc
    //  - zero size with ptr like free

    if(ptr && size == 0) {
        tlsfFree(pgStart, ptr);
        return _NULL;
    }
    if(!ptr) {
        return tlsfMalloc(pgStart, size);
    }

    u64 result;
    result = _NULL;
    headerAddr_t bl, nextBlock;
    /* header_t * */ u64 blockAddr, nextBlockAddr;
    tlsfSize_t realReqSize, realAvailSize, tempSz;

    bl = blockForAddress(pgStart, ptr);
    blockAddr = GET_ADDRESS(bl);
    nextBlock = getNextBlock(pgStart, blockAddr);
    nextBlockAddr = GET_ADDRESS(nextBlock);

    LD_SIZE(realAvailSize, ADDR_OF(header_t, nextBlockAddr, sizeBlock));
    LD_SIZE(tempSz, ADDR_OF(header_t, blockAddr, sizeBlock));
    FENCE_LOAD;
    realAvailSize += tempSz + GusedBlockOverhead; // realAvailSize = blockAddr->sizeBlock
    // + nextBlockAddr->sizeBlock + GusedBlockOverhead
    realReqSize = getRealSizeOfRequest(size);

    if(realReqSize > tempSz || (!isBlockFree(nextBlockAddr) || realReqSize > realAvailSize)) {
        // We need to reallocate and copy
        // Note, does not matter if pgStart was already truncated, it is an idempotent operation
        result = tlsfMalloc(pgStart, size);
        if(result) {
            u64 sizeToCopy = tempSz<realReqSize?tempSz:realReqSize;
            MALLOC_COPY((void*)result, (void*)ptr, sizeToCopy << ELEMENT_SIZE_LOG2);

            tlsfFree(pgStart, ptr); // Free the original block
        }
    } else {
        if(realReqSize > tempSz) {
            // This means we need to extend to the other block
            removeFreeBlock(pgStart, nextBlock);
            ST_SIZE(ADDR_OF(header_t, blockAddr, sizeBlock), realAvailSize);
            FENCE_STORE;
            markBlockUsed(pgStart, blockAddr);
        } else {
            realAvailSize = tempSz;
        }
        // We can trim to just the size used to create a new
        // free block and reduce internal fragmentation
        if(realAvailSize > realReqSize + GminBlockRealSize) {
            headerAddr_t remainingBlock = splitBlock(pgStart, bl, realReqSize);
            addFreeBlock(pgStart, remainingBlock);
        }
        result = ptr;
    }

    return result;
}

// Helper methods
static void tlsfMap(ocrMappable_t* self, ocrMappableKind kind, u64 instanceCount,
                    ocrMappable_t** ptrInstances) {

    ocrAllocatorTlsf_t *rself = (ocrAllocatorTlsf_t*)self;
    ASSERT(instanceCount == 1); // Currently only support one underlying memory
    ASSERT(rself->base.memoryCount == 0 && rself->base.memories == NULL); // Called only once
    rself->base.memoryCount = instanceCount;
    // TODO: Is this the best way...
    rself->base.memories = (ocrMemTarget_t**)checkedMalloc(rself->base.memories,
                                                           sizeof(ocrMemTarget_t*));
    rself->base.memories[0] = ((ocrMemTarget_t**)ptrInstances)[0];

    // Do the allocation
    rself->addr = rself->poolAddr = (u64)(rself->base.memories[0]->fctPtrs->allocate(
                                              rself->base.memories[0], rself->totalSize));
    ASSERT(rself->addr);
    RESULT_ASSERT(tlsfInit(rself->addr, rself->totalSize), ==, 0);
}

static void tlsfDestruct(ocrAllocator_t *self) {
    ocrAllocatorTlsf_t *rself = (ocrAllocatorTlsf_t*)self;
    if(self->memoryCount)
        self->memories[0]->fctPtrs->free(self->memories[0], (void*)rself->addr);
    rself->lock->fctPtrs->destruct(rself->lock);
    free(self->memories);

    ocrPolicyDomain_t *pd = getCurrentPD();
    ocrPolicyCtx_t *orgCtx = getCurrentWorkerContext();
    ocrPolicyCtx_t * ctx = orgCtx->clone(orgCtx);
    ctx->type = PD_MSG_GUID_REL;
    pd->inform(pd, self->guid, ctx);
    free(rself);
}

static void tlsfStart(ocrAllocator_t *self, ocrPolicyDomain_t * PD ) { }

static void tlsfStop(ocrAllocator_t *self) { }

static void* tlsfAllocate(ocrAllocator_t *self, u64 size) {
    ocrAllocatorTlsf_t *rself = (ocrAllocatorTlsf_t*)self;
    rself->lock->fctPtrs->lock(rself->lock);
    void* toReturn = (void*)tlsfMalloc(rself->addr, size);
    rself->lock->fctPtrs->unlock(rself->lock);
    return toReturn;
}

static void tlsfDeallocate(ocrAllocator_t *self, void* address) {
    ocrAllocatorTlsf_t *rself = (ocrAllocatorTlsf_t*)self;
    rself->lock->fctPtrs->lock(rself->lock);
    tlsfFree(rself->addr, (u64)address);
    rself->lock->fctPtrs->unlock(rself->lock);
}

static void* tlsfReallocate(ocrAllocator_t *self, void* address, u64 size) {
    ocrAllocatorTlsf_t *rself = (ocrAllocatorTlsf_t*)self;
    rself->lock->fctPtrs->lock(rself->lock);
    void* toReturn = (void*)(tlsfRealloc(rself->addr, (u64)address, size));
    rself->lock->fctPtrs->unlock(rself->lock);
    return toReturn;
}

// Method to create the TLSF allocator
static ocrAllocator_t * newAllocatorTlsf(ocrAllocatorFactory_t * factory, ocrParamList_t *perInstance) {

    ocrAllocatorTlsf_t *result = (ocrAllocatorTlsf_t*)
        checkedMalloc(result, sizeof(ocrAllocatorTlsf_t));
    result->base.guid = UNINITIALIZED_GUID;
    guidify(getCurrentPD(), (u64) result, &(result->base.guid), OCR_GUID_ALLOCATOR);
    result->base.fctPtrs = &(factory->allocFcts);
    result->base.memories = NULL;
    result->base.memoryCount = 0;

    paramListAllocatorInst_t *perInstanceReal = (paramListAllocatorInst_t*)perInstance;

    result->addr = result->poolAddr = 0ULL;
    result->totalSize = result->poolSize = perInstanceReal->size;

    ocrPolicyDomain_t *pd = getCurrentPD();
    ocrPolicyCtx_t *ctx = getCurrentWorkerContext();

    result->lock = pd->getLock(pd, ctx);

    result->base.module.mapFct = &tlsfMap;

    return (ocrAllocator_t*)result;
}

/******************************************************/
/* OCR ALLOCATOR TLSF FACTORY                         */
/******************************************************/

static void destructAllocatorFactoryTlsf(ocrAllocatorFactory_t * factory) {
    free(factory);
}

ocrAllocatorFactory_t * newAllocatorFactoryTlsf(ocrParamList_t *perType) {
    ocrAllocatorFactory_t* base = (ocrAllocatorFactory_t*)
        checkedMalloc(base, sizeof(ocrAllocatorFactoryTlsf_t));
    base->instantiate = newAllocatorTlsf;
    base->destruct =  &destructAllocatorFactoryTlsf;
    base->allocFcts.destruct = &tlsfDestruct;
    base->allocFcts.start = &tlsfStart;
    base->allocFcts.stop = &tlsfStop;
    base->allocFcts.allocate = &tlsfAllocate;
    base->allocFcts.free = &tlsfDeallocate;
    base->allocFcts.reallocate = &tlsfReallocate;
    return base;
}
// Deactivated for now. Need to clean-up
#if 0 && defined(OCR_DEBUG)
/* Some debug functions to see what is going on */


static void printBlock(void *block_, void* extra) {
    header_t *bl = (header_t*)block_;
    int *count = (int*)extra;
    fprintf(stderr, "\tBlock %d starts at 0x%lx (user: 0x%lx) of size %d (user: %d) %s\n",
            *count, bl, (char*)bl + (GusedBlockOverhead << ELEMENT_SIZE_LOG2),
            bl->sizeBlock, (bl->sizeBlock << ELEMENT_SIZE_LOG2),
            isBlockFree(bl)?"free":"used");
    *count += 1;
}

typedef struct flagVerifier_t {
    int countConsecutiveFrees;
    BOOL isPrevFree;
    int countErrors;
} flagVerifier_t;

static void verifyFlags(void *block_, void* extra) {
    header_t *bl = (header_t*)block_;
    flagVerifier_t* verif = (flagVerifier_t*)extra;

    if(isPrevBlockFree(bl) != verif->isPrevFree) {
        verif->countErrors += 1;
        fprintf(stderr, "Mismatch in free flag for block 0x%x\n", bl);
    }
    if(isBlockFree(bl)) {
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
    pool_t *poolToUse = (pool_t*)pgStart;
    if(action == 0) {
        action = (tlsf_walkerAction)printBlock;
    }
    header_t *bl = &(poolToUse->mainBlock);

    while(bl && !(bl->sizeBlock == 0)) {
        action(bl, extra);
        bl = GET_ADDRESS(getNextBlock(pgStart, bl));
    }
}

/* Will check to make sure the heap is properly maintained
 *
 * Return the number of errors
 */
int tlsf_check_heap(u64 pgStart, unsigned int *freeRemaining) {
    pool_t *poolToUse = (pool_t*)pgStart;
    int errCount = 0;
    unsigned int countFree = 0;
    flagVerifier_t verif = {0, FALSE, 0};

    tlsf_walk_heap(pgStart, verifyFlags, &verif);
    if(verif.countErrors) {
        errCount += verif.countErrors;
        fprintf(stderr, "Mismatch in free flags or coalescing\n");
    }

    // Check the bitmaps
    for(int i=0; i < FL_COUNT; ++i) {
        bool hasFlAvail = poolToUse->flAvailOrNot & (1 << i);
        for(int j=0; j < SL_COUNT; ++j) {
            bool hasSlAvail = poolToUse->slAvailOrNot[i] & (1 << j);

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
                header_t *blockHeadPtr = GET_ADDRESS(blockHead);
                while(blockHeadPtr != &(poolToUse->nullBlock)) {
                    if(!isBlockFree(blockHeadPtr)) {
                        fprintf(stderr, "Block 0x%x should be free for (%d, %d)\n", blockHeadPtr, i, j);
                        ++errCount;
                    }
                    if(isBlockFree(GET_ADDRESS(getNextBlock(pgStart, blockHeadPtr)))) {
                        fprintf(stderr, "Block 0x%x should have coalesced with next for (%d, %d)\n", blockHeadPtr, i, j);
                        ++errCount;
                    }
                    if(!isPrevBlockFree(GET_ADDRESS(getNextBlock(pgStart, blockHeadPtr)))) {
                        fprintf(stderr, "Block 0x%x is not prevFree for (%d, %d)\n", blockHeadPtr, i, j);
                        ++errCount;
                    }
                    if(blockHeadPtr != GET_ADDRESS(getPrevBlock(pgStart, GET_ADDRESS(getNextBlock(pgStart, blockHeadPtr))))) {

                        fprintf(stderr, "Block 0x%x cannot be back-reached for (%d, %d)\n", blockHeadPtr, i, j);
                        ++errCount;
                    }
                    if(blockHeadPtr->sizeBlock < (GminBlockRealSize - GusedBlockOverhead)
                       || blockHeadPtr->sizeBlock > GmaxBlockRealSize) {

                        fprintf(stderr, "Block 0x%x has illegal size for (%d, %d)\n", blockHeadPtr, i, j);
                        ++errCount;
                    }

                    int tf, ts;
                    mappingInsert(blockHeadPtr->sizeBlock, &tf, &ts);
                    if(tf != i || ts != j) {
                        fprintf(stderr, "Block 0x%x is in wrong bucket for (%d, %d)\n", blockHeadPtr, i, j);
                        ++errCount;
                    }
                    countFree += blockHeadPtr->sizeBlock;
                    blockHead = getNextFreeBlock(blockHeadPtr);
                    blockHeadPtr = GET_ADDRESS(blockHead);
                }
            }
        }
    }
    *freeRemaining = countFree << ELEMENT_SIZE_LOG2;
    return errCount;
}
#endif /* OCR_DEBUG */
