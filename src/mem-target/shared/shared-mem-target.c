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
#include "ocr-macros.h"
#include "ocr-mappable.h"
#include "ocr-mem-platform.h"
#include "ocr-mem-target.h"

#include <stdlib.h>



/******************************************************/
/* OCR MEM TARGET SHARED IMPLEMENTATION               */
/******************************************************/

void sharedMap(ocrMappable_t* self, ocrMappableKind kind, u64 instanceCount,
               ocrMappable_t **instances) {
    ASSERT(kind == OCR_MEM_PLATFORM);
    ASSERT(instanceCount == 1);
    ocrMemTarget_t *me = (ocrMemTarget_t*)self;
    me->memories = (ocrMemPlatform_t**)checkedMalloc(me->memories,
                                                     sizeof(ocrMemPlatform_t*));
    me->memories[0] = (ocrMemPlatform_t*)(instances[0]);
    me->memoryCount = instanceCount;
}

void sharedDestruct(ocrMemTarget_t *self) {
    free(self->memories);
    free(self);
}

struct _ocrPolicyDomain_t;
static void sharedStart(ocrMemTarget_t *self, struct _ocrPolicyDomain_t * PD ) { }

static void sharedStop(ocrMemTarget_t *self) { }

void* sharedAllocate(ocrMemTarget_t *self, u64 size) {
    return self->memories[0]->fctPtrs->allocate(
        self->memories[0], size);
}

void sharedFree(ocrMemTarget_t *self, void *addr) {
    return self->memories[0]->fctPtrs->free(
        self->memories[0], addr);
}

ocrMemTarget_t* newMemTargetShared(ocrMemTargetFactory_t * factory,
                                       ocrParamList_t *perInstance) {

    // TODO: This will be replaced by the runtime/GUID meta-data allocator
    // For now, we cheat and use good-old malloc which is kind of counter productive with
    // all the trouble we are going through to *not* use malloc...
    ocrMemTarget_t *result = (ocrMemTarget_t*)
        checkedMalloc(result, sizeof(ocrMemTargetShared_t));

    result->fctPtrs = &(factory->targetFcts);

    return result;
}

/******************************************************/
/* OCR MEM TARGET SHARED FACTORY                    */
/******************************************************/

static void destructMemTargetFactoryShared(ocrMemTargetFactory_t *factory) {
    free(factory);
}

ocrMemTargetFactory_t *newMemTargetFactoryShared(ocrParamList_t *perType) {
    ocrMemTargetFactory_t *base = (ocrMemTargetFactory_t*)
        checkedMalloc(base, sizeof(ocrMemTargetFactoryShared_t));
    base->instantiate = &newMemTargetShared;
    base->destruct = &destructMemTargetFactoryShared;
    base->targetFcts.destruct = &sharedDestruct;
    base->targetFcts.start = &sharedStart;
    base->targetFcts.stop = &sharedStop;
    base->targetFcts.allocate = &sharedAllocate;
    base->targetFcts.free = &sharedFree;
    return base;
}
