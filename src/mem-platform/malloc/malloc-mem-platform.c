/**
 * @brief Simple implementation of a malloc wrapper
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "debug.h"
#include "mem-platform/malloc/malloc-mem-platform.h"
#include "ocr-macros.h"
#include "ocr-mappable.h"
#include "ocr-mem-platform.h"

#include <stdlib.h>

/******************************************************/
/* OCR MEM PLATFORM MALLOC IMPLEMENTATION             */
/******************************************************/

void mallocDestruct(ocrMemPlatform_t *self) {
    free(self);
}

struct _ocrPolicyDomain_t;
static void mallocStart(ocrMemPlatform_t *self, struct _ocrPolicyDomain_t * PD ) { }

static void mallocStop(ocrMemPlatform_t *self) { }

void* mallocAllocate(ocrMemPlatform_t *self, u64 size) {
    return malloc(size);
}

void mallocFree(ocrMemPlatform_t *self, void *addr) {
    free(addr);
}

ocrMemPlatform_t* newMemPlatformMalloc(ocrMemPlatformFactory_t * factory,
                                       ocrParamList_t *perInstance) {

    // TODO: This will be replaced by the runtime/GUID meta-data allocator
    // For now, we cheat and use good-old malloc which is kind of counter productive with
    // all the trouble we are going through to *not* use malloc...
    ocrMemPlatform_t *result = (ocrMemPlatform_t*)
        checkedMalloc(result, sizeof(ocrMemPlatformMalloc_t));

    result->fctPtrs = &(factory->platformFcts);

    return result;
}

/******************************************************/
/* OCR MEM PLATFORM MALLOC FACTORY                    */
/******************************************************/

static void destructMemPlatformFactoryMalloc(ocrMemPlatformFactory_t *factory) {
    free(factory);
}

ocrMemPlatformFactory_t *newMemPlatformFactoryMalloc(ocrParamList_t *perType) {
    ocrMemPlatformFactory_t *base = (ocrMemPlatformFactory_t*)
        checkedMalloc(base, sizeof(ocrMemPlatformFactoryMalloc_t));

    base->instantiate = &newMemPlatformMalloc;
    base->destruct = &destructMemPlatformFactoryMalloc;
    base->platformFcts.destruct = &mallocDestruct;
    base->platformFcts.start = &mallocStart;
    base->platformFcts.stop = &mallocStop;
    base->platformFcts.allocate = &mallocAllocate;
    base->platformFcts.free = &mallocFree;

    return base;
}
