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
    allocatorNull_id,
    allocatorMax_id
} allocatorType_t;

extern const char * allocator_types[];

// TLSF allocator
#include "allocator/tlsf/tlsf-allocator.h"
#include "allocator/null/null-allocator.h"

// Add other allocators using the same pattern as above

ocrAllocatorFactory_t *newAllocatorFactory(allocatorType_t type, ocrParamList_t *typeArg);

#endif /* __ALLOCATOR_ALL_H__ */
