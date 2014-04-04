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
    u64 addr, totalSize, poolAddr, poolSize;
    u32 lock; /**< Currently needs a lock. The idea is to have multiple allocators but currently only one */
} ocrAllocatorTlsf_t;

extern ocrAllocatorFactory_t* newAllocatorFactoryTlsf(ocrParamList_t *perType);

#endif /* ENABLE_ALLOCATOR_TLSF */
#endif /* __TLSF_ALLOCATOR_H__ */
