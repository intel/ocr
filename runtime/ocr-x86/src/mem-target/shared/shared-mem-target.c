/**
 * @brief Simple implementation of a shared memory target
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_MEM_TARGET_SHARED

#include "debug.h"
#include "mem-target/shared/shared-mem-target.h"
#include "ocr-mem-platform.h"
#include "ocr-mem-target.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#include "ocr-statistics-callbacks.h"
#endif


/******************************************************/
/* OCR MEM TARGET SHARED IMPLEMENTATION               */
/******************************************************/

void sharedDestruct(ocrMemTarget_t *self) {
    ASSERT(self->memoryCount == 1);
    self->memories[0]->fctPtrs->destruct(self->memories[0]);
    runtimeChunkFree((u64)self->memories, NULL);
#ifdef OCR_ENABLE_STATISTICS
    ocrPolicyDomain_t *pd = NULL;
    getCurrentEnv(&pd, NULL, NULL);
    statsMEMTARGET_STOP(pd, self->guid, self);
#endif
    runtimeChunkFree((u64)self, NULL);    
}

struct _ocrPolicyDomain_t;
void sharedStart(ocrMemTarget_t *self, struct _ocrPolicyDomain_t * PD ) {
    // Get a GUID
    guidify(PD, (u64)self, &(self->guid), OCR_GUID_MEMTARGET);
#ifdef OCR_ENABLE_STATISTICS
    statsMEMTARGET_START(PD, self->guid, self);
#endif
    ASSERT(self->memoryCount == 1);
    self->memories[0]->fctPtrs->start(self->memories[0], PD);
}

void sharedStop(ocrMemTarget_t *self) {
    ASSERT(self->memoryCount == 1);
    self->memories[0]->fctPtrs->stop(self->memories[0]);
    
    // Destroy the GUID
    ocrPolicyDomain_t *pd = NULL;
    ocrPolicyMsg_t msg;
    
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_DESTROY
    msg.type = PD_MSG_GUID_DESTROY | PD_MSG_REQUEST;
    PD_MSG_FIELD(guid.guid) = self->guid;
    PD_MSG_FIELD(guid.metaDataPtr) = self;
    PD_MSG_FIELD(properties) = 0;
    pd->sendMessage(pd, &msg, false);
#undef PD_MSG
#undef PD_TYPE
    self->guid = NULL_GUID;
}

void sharedFinish(ocrMemTarget_t *self) {
    ASSERT(self->memoryCount == 1);
    self->memories[0]->fctPtrs->finish(self->memories[0]);
}

u8 sharedGetThrottle(ocrMemTarget_t *self, u64* value) {
    return 1;
}

u8 sharedSetThrottle(ocrMemTarget_t *self, u64 value) {
    return 1;
}

void sharedGetRange(ocrMemTarget_t *self, u64* startAddr,
                    u64* endAddr) {
    return self->memories[0]->fctPtrs->getRange(
        self->memories[0], startAddr, endAddr);
}

u8 sharedChunkAndTag(ocrMemTarget_t *self, u64 *startAddr,
                     u64 size, ocrMemoryTag_t oldTag, ocrMemoryTag_t newTag) {

    return self->memories[0]->fctPtrs->chunkAndTag(
        self->memories[0], startAddr, size, oldTag, newTag);
}

u8 sharedTag(ocrMemTarget_t *self, u64 startAddr, u64 endAddr,
             ocrMemoryTag_t newTag) {

    return self->memories[0]->fctPtrs->tag(self->memories[0], startAddr,
                                           endAddr, newTag);
}

u8 sharedQueryTag(ocrMemTarget_t *self, u64 *start, u64 *end,
                  ocrMemoryTag_t *resultTag, u64 addr) {

    return self->memories[0]->fctPtrs->queryTag(self->memories[0], start,
                                                end, resultTag, addr);
}

ocrMemTarget_t* newMemTargetShared(ocrMemTargetFactory_t * factory,
                                   ocrPhysicalLocation_t location,
                                   u64 size, ocrParamList_t *perInstance) {

    ocrMemTarget_t *result = (ocrMemTarget_t*)
        runtimeChunkAlloc(sizeof(ocrMemTargetShared_t), NULL);

    result->guid = UNINITIALIZED_GUID;
    result->location = location;

    result->size = size;
    result->startAddr = result->endAddr = 0ULL;
    result->memories = NULL;
    result->memoryCount = 0;
    result->fctPtrs = &(factory->targetFcts);

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
    base->targetFcts.finish = &sharedFinish;
    base->targetFcts.getThrottle = &sharedGetThrottle;
    base->targetFcts.setThrottle = &sharedSetThrottle;
    base->targetFcts.getRange = &sharedGetRange;
    base->targetFcts.chunkAndTag = &sharedChunkAndTag;
    base->targetFcts.tag = &sharedTag;
    base->targetFcts.queryTag = &sharedQueryTag;
    
    return base;
}

#endif /* ENABLE_MEM_TARGET_SHARED */
