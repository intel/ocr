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
    u64 poolStorageAddr;  // Address of system storage (not necessarily 8-byte aligned) for start of pool space.
                          // Formerly called "poolAddr".  Possibly not needed, if no real need to perform tlsfFinish.
    u64 poolStorageSize;  // Amount of system storage allocated to pool, including possible leading and trailing
                          // bytes outside of aligned range.  Formerly called "poolSize".  Ibid about not needed.
    u64 poolAddr;         // Address of the 8-byte-aligned net pool storage space.  Formerly called "addr".
    u64 poolSize;         // Net size of usable, aligned pool space.  Formerly called "totalSize".
                          // Slices of the memory that would otherwise be in a monolithic pool for this allocator pool will
                          // instead be lopped off for handling as independent smaller pools. These slice pools will serve a
                          // smaller number of clients, accomplishing less contention, albeit at a cost of greater fragmentation
                          // and the ability to accomodate only a smaller size for any single data block.  There can still be a
                          // monolithic "remnant pool" left over, and in fact this is presently required (to keep things easier).
                          // The following variables relate to the slice functionality.
    u64 sliceSize;        // How many bytes each slice will accomodate.  NOTE:  This count INCLUDES the pool overhead space.
                          // To keep things simple, sliceSize must be a minimum of 256 bytes.  (TODO: perhaps change this)
    u32 sliceCount;       // Number of semi-private "slice" pools to carve out of the memory, to assign round-robin to clients
                          // of the allocator.  This value is zero when slicing is not used, as is the case with the lowest
                          // level memory pool(s).

    u32 initializedBy;    // Identity of the agent that initialized the pool.
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
