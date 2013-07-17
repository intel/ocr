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

#include "ocr-allocator.h"
#include "ocr-sync.h"
#include "ocr-types.h"
#include "ocr-utils.h"

typedef struct {
    ocrAllocatorFactory_t base;
} ocrAllocatorFactoryTlsf_t;

typedef struct {
    ocrAllocator_t base;
    u64 addr, totalSize, poolAddr, poolSize;
    ocrLock_t* lock; /**< Currently needs a lock. The idea is to have multiple allocators but currently only one */
} ocrAllocatorTlsf_t;

extern ocrAllocatorFactory_t* newAllocatorFactoryTlsf(ocrParamList_t *perType);
#endif /* __TLSF_ALLOCATOR_H__ */
