/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_ALLOCATOR_NULL

#include "ocr-hal.h"
#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"
#include "utils/ocr-utils.h"
#include "null-allocator.h"

void nullDestruct(ocrAllocator_t *self) {
    runtimeChunkFree((u64)self, NULL);
}

void nullBegin(ocrAllocator_t *self, ocrPolicyDomain_t * PD ) {
}

void nullStart(ocrAllocator_t *self, ocrPolicyDomain_t * PD ) {
    // Get a GUID
    guidify(PD, (u64)self, &(self->fguid), OCR_GUID_ALLOCATOR);
    self->pd = PD;
}

void nullStop(ocrAllocator_t *self) {
    self->fguid.guid = NULL_GUID;
}

void nullFinish(ocrAllocator_t *self) {
}

void* nullAllocate(ocrAllocator_t *self, u64 size) {
    return NULL;
}

void nullDeallocate(ocrAllocator_t *self, void* address) {
}

void* nullReallocate(ocrAllocator_t *self, void* address, u64 size) {
    return NULL;
}

// Method to create the NULL allocator
ocrAllocator_t * newAllocatorNull(ocrAllocatorFactory_t * factory, ocrParamList_t *perInstance) {

    ocrAllocator_t *base = (ocrAllocator_t*)
        runtimeChunkAlloc(sizeof(ocrAllocatorNull_t), NULL);
    factory->initialize(factory, base, perInstance);
    return (ocrAllocator_t *) base;
}
    
void initializeAllocatorNull(ocrAllocatorFactory_t * factory, ocrAllocator_t * self, ocrParamList_t * perInstance){
    initializeAllocatorOcr(factory, self, perInstance);
}

/******************************************************/
/* OCR ALLOCATOR NULL FACTORY                         */
/******************************************************/

static void destructAllocatorFactoryNull(ocrAllocatorFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrAllocatorFactory_t * newAllocatorFactoryNull(ocrParamList_t *perType) {
    ocrAllocatorFactory_t* base = (ocrAllocatorFactory_t*)
        runtimeChunkAlloc(sizeof(ocrAllocatorFactoryNull_t), (void *)1);
    ASSERT(base);
    base->instantiate = &newAllocatorNull;
    base->initialize = &initializeAllocatorNull;
    base->destruct =  &destructAllocatorFactoryNull;
    base->allocFcts.destruct = FUNC_ADDR(void (*)(ocrAllocator_t*), nullDestruct);
    base->allocFcts.begin = FUNC_ADDR(void (*)(ocrAllocator_t*, ocrPolicyDomain_t*), nullBegin);
    base->allocFcts.start = FUNC_ADDR(void (*)(ocrAllocator_t*, ocrPolicyDomain_t*), nullStart);
    base->allocFcts.stop = FUNC_ADDR(void (*)(ocrAllocator_t*), nullStop);
    base->allocFcts.finish = FUNC_ADDR(void (*)(ocrAllocator_t*), nullFinish);
    base->allocFcts.allocate = FUNC_ADDR(void* (*)(ocrAllocator_t*, u64), nullAllocate);
    base->allocFcts.free = FUNC_ADDR(void (*)(ocrAllocator_t*, void*), nullDeallocate);
    base->allocFcts.reallocate = FUNC_ADDR(void* (*)(ocrAllocator_t*, void*, u64), nullReallocate);
    return base;
}

#endif /* ENABLE_NULL_ALLOCATOR */
