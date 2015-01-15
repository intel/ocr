/**
 * @brief OCR memory allocators
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __ALLOCATOR_ALL_H__
#define __ALLOCATOR_ALL_H__

#include "debug.h"
#include "ocr-allocator.h"
#include "ocr-config.h"
#include "utils/ocr-utils.h"

typedef enum _allocatorType_t {
    allocatorTlsf_id,
#ifdef ENABLE_ALLOCATOR_MALLOCPROXY
    allocatorMallocProxy_id,
#endif
    allocatorNull_id,
    // As other types are added, add switch cases in allocater-all.c
    allocatorMax_id
} allocatorType_t;

// When a block is freed, the Policy Domain extracts an index of the type of allocator that
// allocated the block from a "pool header descriptor" in the block header.  That descriptor
// also contains the address of the pool header itself.  So this descriptor aggregates to
// both the type and instance of the allocator to which the block needs to be returned.  As
// such, the number of bits available for the allocator type index is only those left over
// from expressing the pool address.  I.e., with allocators working at a granularity of
// eight-byte aligned allocations, there are three bits available to distinguish allocator
// type.  Thus, there can only be up to eight allocator types, not counting the Null allocator.
// These masks support extracting the allocator type and pool address from the block header.
#define POOL_HEADER_TYPE_MASK (7L)
#define POOL_HEADER_ADDR_MASK (~(POOL_HEADER_TYPE_MASK))

extern const char * allocator_types[];

#include "allocator/tlsf/tlsf-allocator.h"    // TLSF allocator
#ifdef ENABLE_ALLOCATOR_MALLOCPROXY
// System malloc allocator with our own wrapper, for platforms other than FSIM, for fall-back testing
#include "allocator/mallocproxy/mallocproxy-allocator.h"
#endif
#include "allocator/null/null-allocator.h"

ocrAllocatorFactory_t *newAllocatorFactory(allocatorType_t type, ocrParamList_t *typeArg);
void allocatorFreeFunction(void* blockPayloadAddr);

#endif /* __ALLOCATOR_ALL_H__ */
