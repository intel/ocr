/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __ALLOCATOR_NULL_H__
#define __ALLOCATOR_NULL_H__

#include "ocr-config.h"
#ifdef ENABLE_ALLOCATOR_NULL

#include "ocr-allocator.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

typedef struct {
    ocrAllocatorFactory_t base;
} ocrAllocatorFactoryNull_t;

typedef struct {
    ocrAllocator_t base;
} ocrAllocatorNull_t;

extern ocrAllocatorFactory_t* newAllocatorFactoryNull(ocrParamList_t *perType);

#endif /* ENABLE_ALLOCATOR_NULL */
#endif /* __NULL_ALLOCATOR_H__ */
