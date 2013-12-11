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
#include "ocr-utils.h"

typedef enum _allocatorType_t {
#ifdef ENABLE_ALLOCATOR_TLSF
    allocatorTlsf_id,
#endif
    allocatorMax_id
} allocatorType_t;

const char * allocator_types[] = {
#ifdef ENABLE_ALLOCATOR_TLSF
    "tlsf",
#endif
    NULL
};

// TLSF allocator
#include "allocator/tlsf/tlsf-allocator.h"

// Add other allocators using the same pattern as above

inline ocrAllocatorFactory_t *newAllocatorFactory(allocatorType_t type, ocrParamList_t *typeArg) {
    switch(type) {
#ifdef ENABLE_ALLOCATOR_TLSF
    case allocatorTlsf_id:
        return newAllocatorFactoryTlsf(typeArg);
#endif
    case allocatorMax_id:
    default:
        ASSERT(0);
        return NULL;
    };
}

#endif /* __ALLOCATOR_ALL_H__ */
