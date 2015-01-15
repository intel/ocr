/**
 * @brief Allocator implemented using the system malloc/free facilities.  Not for use on FSIM.
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __ALLOCATOR_MALLOCPROXY_H__
#define __ALLOCATOR_MALLOCPROXY_H__

#include "ocr-config.h"
#ifdef ENABLE_ALLOCATOR_MALLOCPROXY
#if defined(HAL_FSIM_CE) || defined(HAL_FSIM_XE)
#error "System Malloc is not available on FSIM.  Do not #define ENABLE_ALLOCATOR_MALLOCPROXY."
#endif

#include "ocr-allocator.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

typedef struct {
    ocrAllocatorFactory_t base;
} ocrAllocatorFactoryMallocProxy_t;

typedef struct _ocrAllocatorMallocProxy_t {
    ocrAllocator_t base;
} ocrAllocatorMallocProxy_t;

typedef struct {
    paramListAllocatorInst_t base;
} paramListAllocatorMallocProxy_t;

extern ocrAllocatorFactory_t* newAllocatorFactoryMallocProxy(ocrParamList_t *perType);

void mallocProxyDeallocate(void* address);

#endif /* ENABLE_ALLOCATOR_MALLOCPROXY */
#endif /* __MALLOCPROXY_ALLOCATOR_H__ */
