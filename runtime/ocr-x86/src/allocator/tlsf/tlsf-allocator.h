/**
 * @brief Allocator implemented using the TLSF algorithm
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __ALLOCATOR_TLSF_H__
#define __ALLOCATOR_TLSF_H__

#include "ocr-config.h"
#ifdef ENABLE_ALLOCATOR_TLSF

#include "ocr-allocator.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

typedef struct {
    ocrAllocatorFactory_t base;
} ocrAllocatorFactoryTlsf_t;

typedef struct _ocrAllocatorTlsf_t {
    ocrAllocator_t base;
    u32 lockForInit;        // This lock is used solely to serialize initialization of a shared pool by its multiple clients.
    u8  poolStorageOffset;  // Distance from poolAddr to storage address of the pool (which wasn't necessarily 8-byte aligned).
    u8  poolStorageSuffix;  // Bytes at end of storage space not usable for the pool.
    volatile u16 sliceCount;// Number of semi-private "slice" pools to carve out of the memory, to assign round-robin to clients
                            // of the allocator.  This value is zero when slicing is not used, as is the case with the lowest
                            // level memory pool(s).
    volatile s32 useCount1; // Count up the calls to tlsfBegin, down the calls to tlsfFinish.  (Only accurate in anchor CE).
                            // When the counter gets back down to zero, we know it is time to do the tlsfFinish logic.
    volatile s32 useCount2; // Count up the calls to tlsfStart, down the calls to tlsfStop.  (Only accurate in anchor CE).
                            // When the counter gets back down to zero, we know it is time to do the tlsfStop logic.
    volatile u64 sliceSize; // How many bytes each slice will accomodate.  NOTE:  This count INCLUDES the pool overhead space.
                            // To keep things simple, sliceSize must be a minimum of 256 bytes.  (TODO: perhaps change this)
    volatile u64 poolAddr;  // Address of the 8-byte-aligned net pool storage space.
    u64 poolSize;           // Net size of usable, aligned pool space.  Includes pool header; it is net of unaligned bytes trimmed
                            // from the beginning and end of the available storage span.
                            // Slices of the memory that would otherwise be in a monolithic pool for this allocator pool will
                            // instead be lopped off for handling as independent smaller pools. These slice pools will serve a
                            // smaller number of clients, accomplishing less contention, albeit at a cost of greater fragmentation
                            // and the ability to accomodate only a smaller size for any single data block.  There can still be a
                            // monolithic "remnant pool" left over, and in fact this is presently required (to keep things easier).
                            // The following variables relate to the slice functionality: sliceCount, sliceSize.

    //TODO:  Not used.  Delete u32 initializedBy;    // Identity of the agent that initialized the pool.
} ocrAllocatorTlsf_t;

typedef struct {
    paramListAllocatorInst_t base;
    u64 sliceCount;
    u64 sliceSize;
} paramListAllocatorTlsf_t;

extern ocrAllocatorFactory_t* newAllocatorFactoryTlsf(ocrParamList_t *perType);

void tlsfDeallocate(void* address);

#endif /* ENABLE_ALLOCATOR_TLSF */
#endif /* __TLSF_ALLOCATOR_H__ */
