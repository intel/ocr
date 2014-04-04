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

extern const char * memplatform_types[];

// Malloc memory platform
#include "mem-platform/malloc/malloc-mem-platform.h"
#include "mem-platform/fsim/fsim-mem-platform.h"

// Add other memory platforms using the same pattern as above

ocrMemPlatformFactory_t *newMemPlatformFactory(memPlatformType_t type, ocrParamList_t *typeArg);

#endif /* __MEM_PLATFORM_ALL_H__ */


