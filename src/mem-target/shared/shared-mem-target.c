/**
 * @brief Simple implementation of a shared memory target
 * @authors Romain Cledat, Intel Corporation
 * Copyright (c) 2012, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *      * Neither the name of the Georgia Institute of Technology nor the
 *        names of its contributors may be used to endorse or promote products
 *        derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ROMAIN CLEDAT OR INTEL CORPORATION BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

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
