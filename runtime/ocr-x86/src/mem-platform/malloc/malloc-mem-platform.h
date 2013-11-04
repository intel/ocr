/**
 * @brief Simple Malloc allocator
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __MEM_PLATFORM_MALLOC_H__
#define __MEM_PLATFORM_MALLOC_H__

#include "debug.h"
#include "external/rangeTracker.h"
#include "ocr-mem-platform.h"
#include "ocr-types.h"
#include "ocr-utils.h"

typedef struct {
    ocrMemPlatformFactory_t base;
} ocrMemPlatformFactoryMalloc_t;

typedef struct {
    ocrMemPlatform_t base;
    rangeTracker_t rangeTracker;
    u32 lock;
} ocrMemPlatformMalloc_t;

extern ocrMemPlatformFactory_t* newMemPlatformFactoryMalloc(ocrParamList_t *perType);

#endif /* __MEM_PLATFORM_MALLOC_H__ */
