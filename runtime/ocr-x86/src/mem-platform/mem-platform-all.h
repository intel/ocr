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
#include "ocr-config.h"
#include "ocr-mem-platform.h"
#include "utils/ocr-utils.h"

typedef enum _memPlatformType_t {
    memPlatformMalloc_id,
    memPlatformFsim_id,
    memPlatformMax_id
} memPlatformType_t;

const char * memplatform_types[] = {
    "malloc",
    "fsim",
    NULL
};

// Malloc memory platform
#include "mem-platform/malloc/malloc-mem-platform.h"
#include "mem-platform/fsim/fsim-mem-platform.h"

// Add other memory platforms using the same pattern as above

inline ocrMemPlatformFactory_t *newMemPlatformFactory(memPlatformType_t type, ocrParamList_t *typeArg) {
    switch(type) {
#ifdef ENABLE_MEM_PLATFORM_MALLOC
    case memPlatformMalloc_id:
        return newMemPlatformFactoryMalloc(typeArg);
#endif
#ifdef ENABLE_MEM_PLATFORM_FSIM
    case memPlatformFsim_id:
        return newMemPlatformFactoryFsim(typeArg);
#endif
    default:
        ASSERT(0);
        return NULL;
    };
}

#endif /* __MEM_PLATFORM_ALL_H__ */


