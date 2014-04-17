/**
 * @brief OCR low-level memory allocator
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __MEM_PLATFORM_ALL_H__
#define __MEM_PLATFORM_ALL_H__

#include "debug.h"
#include "ocr-mem-platform.h"
#include "ocr-utils.h"

typedef enum _memPlatformType_t {
    memPlatformMalloc_id,
    memPlatformMax_id
} memPlatformType_t;

const char * memplatform_types[] = {
    "malloc",
    NULL
};

// Malloc memory platform
#include "mem-platform/malloc/malloc-mem-platform.h"

// Add other memory platforms using the same pattern as above

inline ocrMemPlatformFactory_t *newMemPlatformFactory(memPlatformType_t type, ocrParamList_t *typeArg) {
    switch(type) {
    case memPlatformMalloc_id:
        return newMemPlatformFactoryMalloc(typeArg);
    default:
        ASSERT(0);
        return NULL;
    };
}

#endif /* __MEM_PLATFORM_ALL_H__ */
#if 0
extern ocrMemPlatform_t * newMemPlatformMalloc(ocrMemPlatformFactory_t * factory, void * perTypeConfig, void * perInstanceConfig);

ocrMemPlatform_t* newMemPlatform(ocrMemPlatformKind type, void * perTypeConfig, void * perInstanceConfig) {
    if(type == OCR_MEMPLATFORM_DEFAULT) type = ocrMemPlatformDefaultKind;
    switch(type) {
    case OCR_MEMPLATFORM_MALLOC:
        return newMemPlatformMalloc(NULL, perTypeConfig, perInstanceConfig);
        break;
    default:
        ASSERT(0);
    }
    return NULL;
}
#endif
