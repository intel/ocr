/**
 * @brief Simple implementation of a malloc wrapper
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
