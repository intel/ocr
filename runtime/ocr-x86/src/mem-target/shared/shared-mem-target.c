/**
 * @brief Simple implementation of a shared memory target
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "debug.h"
#include "mem-target/shared/shared-mem-target.h"
#include "ocr-mem-platform.h"
#include "ocr-mem-target.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"

#include <stdlib.h>



/******************************************************/
/* OCR MEM TARGET SHARED IMPLEMENTATION               */
/******************************************************/

void sharedDestruct(ocrMemTarget_t *self) {
    runtimeChunkFree((u64)self->memories, NULL);
    runtimeChunkFree((u64)self, NULL);
}

struct _ocrPolicyDomain_t;
static void sharedStart(ocrMemTarget_t *self, struct _ocrPolicyDomain_t * PD ) {
    self->memories[0]->fctPtrs->start(
        self->memories[0], PD);
}

static void sharedStop(ocrMemTarget_t *self) {
    self->memories[0]->fctPtrs->stop(self->memories[0]);
}

static u8 sharedGetThrottle(ocrMemTarget_t *self, u64* value) {
    return 1;
}

static u8 sharedSetThrottle(ocrMemTarget_t *self, u64 value) {
    return 1;
}

static void sharedGetRange(ocrMemTarget_t *self, u64* startAddr,
                           u64* endAddr) {
    return self->memories[0]->fctPtrs->getRange(
        self->memories[0], startAddr, endAddr);
}

static u8 sharedChunkAndTag(ocrMemTarget_t *self, u64 *startAddr,
                            u64 size, ocrMemoryTag_t oldTag, ocrMemoryTag_t newTag) {

    return self->memories[0]->fctPtrs->chunkAndTag(
        self->memories[0], startAddr, size, oldTag, newTag);
}

static u8 sharedTag(ocrMemTarget_t *self, u64 startAddr, u64 endAddr,
                    ocrMemoryTag_t newTag) {

    return self->memories[0]->fctPtrs->tag(self->memories[0], startAddr,
                                           endAddr, newTag);
}

static u8 sharedQueryTag(ocrMemTarget_t *self, u64 *start, u64 *end,
                         ocrMemoryTag_t *resultTag, u64 addr) {

    return self->memories[0]->fctPtrs->queryTag(self->memories[0], start,
                                                end, resultTag, addr);
}

ocrMemTarget_t* newMemTargetShared(ocrMemTargetFactory_t * factory,
                                   u64 size, ocrParamList_t *perInstance) {

    ocrMemTarget_t *result = (ocrMemTarget_t*)
        runtimeChunkAlloc(sizeof(ocrMemTargetShared_t), NULL);

    guidify(getCurrentPD(), (u64)result, &(result->guid), OCR_GUID_MEM_TARGET);
    result->size = size;
    result->startAddr = result->endAddr = 0ULL;
    result->memories = NULL;
    result->memoryCount = 0;
    result->fctPtrs = &(factory->targetFcts);
#ifdef OCR_ENABLE_STATISTICS
    statsMEMTARGET_START(getCurrentPD(), result->guid, result);
#endif
    return result;
}

/******************************************************/
/* OCR MEM TARGET SHARED FACTORY                    */
/******************************************************/

static void destructMemTargetFactoryShared(ocrMemTargetFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrMemTargetFactory_t *newMemTargetFactoryShared(ocrParamList_t *perType) {
    ocrMemTargetFactory_t *base = (ocrMemTargetFactory_t*)
        runtimeChunkAlloc(sizeof(ocrMemTargetFactoryShared_t), NULL);
    base->instantiate = &newMemTargetShared;
    base->destruct = &destructMemTargetFactoryShared;
    base->targetFcts.destruct = &sharedDestruct;
    base->targetFcts.start = &sharedStart;
    base->targetFcts.stop = &sharedStop;
    base->targetFcts.getThrottle = &sharedGetThrottle;
    base->targetFcts.setThrottle = &sharedSetThrottle;
    base->targetFcts.getRange = &sharedGetRange;
    base->targetFcts.chunkAndTag = &sharedChunkAndTag;
    base->targetFcts.tag = &sharedTag;
    base->targetFcts.queryTag = &sharedQueryTag;
    
    return base;
}
