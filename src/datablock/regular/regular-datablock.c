/**
 * @brief Simple implementation of a malloc wrapper
 * @authors Romain Cledat, Intel Corporation
 * Copyright (c) 2012, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of Intel Corporation nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

#include <errno.h>

#include "ocr-datablock.h"
#include "regular-datablock.h"
#include "debug.h"
#include "ocr-guid.h"
#include "ocr-utils.h"

#include "ocr-comp-platform.h"
#include "ocr-sync.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

#include <inttypes.h>
#include "ocr-policy-domain.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-macros.h"

void* regularAcquire(ocrDataBlock_t *self, ocrGuid_t edt, bool isInternal) {
    ocrDataBlockRegular_t *rself = (ocrDataBlockRegular_t*)self;

    DO_DEBUG(DEBUG_LVL_VERB) {
        PRINTF("VERB: Acquiring DB @ 0x%"PRIx64" (GUID: 0x%"PRIdPTR") from EDT 0x%"PRIdPTR" (isInternal %d)\n",
               (u64)self->ptr, rself->base.guid, edt, (u32)isInternal);
    }

    // Critical section
    rself->lock->fctPtrs->lock(rself->lock);
    if(rself->attributes.freeRequested) {
        rself->lock->fctPtrs->unlock(rself->lock);
        return NULL;
    }
    u32 idForEdt = ocrGuidTrackerFind(&(rself->usersTracker), edt);
    if(idForEdt > 63)
        idForEdt = ocrGuidTrackerTrack(&(rself->usersTracker), edt);
    else {
        DO_DEBUG(DEBUG_LVL_VVERB) {
            PRINTF("VVERB: EDT already had acquired DB (pos %d)\n", idForEdt);
        }
        rself->lock->fctPtrs->unlock(rself->lock);
        return self->ptr;
    }

    if(idForEdt > 63) {
        rself->lock->fctPtrs->unlock(rself->lock);
        return NULL;
    }
    rself->attributes.numUsers += 1;
    if(isInternal)
        rself->attributes.internalUsers += 1;

    rself->lock->fctPtrs->unlock(rself->lock);
    // End critical section
    DO_DEBUG(DEBUG_LVL_VERB) {
        PRINTF("VERB: Added EDT GUID 0x%"PRIx64" at position %d. Have %d users and %d internal\n",
               (u64)edt, idForEdt, rself->attributes.numUsers, rself->attributes.internalUsers);
    }
    return self->ptr;
}

u8 regularRelease(ocrDataBlock_t *self, ocrGuid_t edt,
                  bool isInternal) {

    ocrDataBlockRegular_t *rself = (ocrDataBlockRegular_t*)self;
    u32 edtId = ocrGuidTrackerFind(&(rself->usersTracker), edt);
    bool isTracked = true;

    DO_DEBUG(DEBUG_LVL_VERB) {
        PRINTF("VERB: Releasing DB @ 0x%"PRIx64" (GUID 0x%"PRIdPTR") from EDT 0x%"PRIdPTR" (%d) (internal: %d)\n",
               (u64)self->ptr, rself->base.guid, edt, edtId, (u32)isInternal);
    }
    // Start critical section
    rself->lock->fctPtrs->lock(rself->lock);
    if(edtId > 63 || rself->usersTracker.slots[edtId] != edt) {
        // We did not find it. The runtime may be
        // re-releasing it
        if(isInternal) {
            // This is not necessarily an error
            rself->attributes.internalUsers -= 1;
            isTracked = false;
        } else {
            // Definitely a problem here
            rself->lock->fctPtrs->unlock(rself->lock);
            return (u8)EACCES;
        }
    }

    if(isTracked) {
        ocrGuidTrackerRemove(&(rself->usersTracker), edt, edtId);
        rself->attributes.numUsers -= 1;
        if(isInternal) {
            rself->attributes.internalUsers -= 1;
        }
    }
    DO_DEBUG(DEBUG_LVL_VVERB) {
        PRINTF("VVERB: DB attributes: numUsers %d; internalUsers %d; freeRequested %d\n",
               rself->attributes.numUsers, rself->attributes.internalUsers, rself->attributes.freeRequested);
    }
    // Check if we need to free the block
    if(rself->attributes.numUsers == 0  &&
       rself->attributes.internalUsers == 0 &&
       rself->attributes.freeRequested == 1) {
        // We need to actually free the data-block
        rself->lock->fctPtrs->unlock(rself->lock);
        self->fctPtrs->destruct(self);
    } else {
        rself->lock->fctPtrs->unlock(rself->lock);
    }
    // End critical section

    return 0;
}

void regularDestruct(ocrDataBlock_t *self) {
    // We don't use a lock here. Maybe we should
    ocrDataBlockRegular_t *rself = (ocrDataBlockRegular_t*)self;
    ASSERT(rself->attributes.numUsers == 0);
    ASSERT(rself->attributes.freeRequested == 1);
    rself->lock->fctPtrs->destruct(rself->lock);

    ocrPolicyDomain_t *pd = getCurrentPD();
    ocrPolicyCtx_t *orgCtx = getCurrentWorkerContext();
    ocrPolicyCtx_t * ctx = orgCtx->clone(orgCtx);
    ctx->type = PD_MSG_GUID_REL;
    // Tell the allocator to free the data-block
    ocrAllocator_t *allocator = NULL;
    deguidify(getCurrentPD(), rself->base.allocator, (u64*)&allocator, NULL);

    DO_DEBUG(DEBUG_LVL_VERB) {
        PRINTF("VERB: Freeing DB @ 0x%"PRIx64" (GUID: 0x%"PRIdPTR")\n", (u64)self->ptr, rself->base.guid);
    }
    allocator->fctPtrs->free(allocator, self->ptr);

    // TODO: This is not pretty to be here but I can't put this in the ocrDbFree because
    // the semantics of ocrDbFree is that it will wait for all acquire/releases to have
    // completed so I have to release the block when it is really being freed. This current
    // OCR version does not really implement delayed freeing but may in the future.
#ifdef OCR_ENABLE_STATISTICS
    ocrStatsProcessDestruct(&(rself->base.statProcess));
#endif

    pd->inform(pd, self->guid, ctx);
    free(rself);
}

u8 regularFree(ocrDataBlock_t *self, ocrGuid_t edt) {
    ocrDataBlockRegular_t *rself = (ocrDataBlockRegular_t*)self;

    u32 id = ocrGuidTrackerFind(&(rself->usersTracker), edt);
    DO_DEBUG(DEBUG_LVL_VERB) {
        PRINTF("VERB: Requesting a free for DB @ 0x%"PRIx64" (GUID 0x%"PRIdPTR")\n",
               (u64)self->ptr, rself->base.guid);
    }
    // Begin critical section
    rself->lock->fctPtrs->lock(rself->lock);
    if(rself->attributes.freeRequested) {
        rself->lock->fctPtrs->unlock(rself->lock);
        return EPERM;
    }
    rself->attributes.freeRequested = 1;
    rself->lock->fctPtrs->unlock(rself->lock);
    // End critical section


    if(id < 64) {
        regularRelease(self, edt, false);
    } else {
        // We can call free without having acquired the block
        // Now check if we can actually free the block

        // Critical section
        rself->lock->fctPtrs->lock(rself->lock);
        if(rself->attributes.numUsers == 0 && rself->attributes.internalUsers == 0) {
            rself->lock->fctPtrs->unlock(rself->lock);
            regularDestruct(self);
        } else {
            rself->lock->fctPtrs->unlock(rself->lock);
        }
        // End critical section
    }

    return 0;
}

ocrDataBlock_t* newDataBlockRegular(ocrDataBlockFactory_t *factory, ocrGuid_t allocator,
                                    ocrGuid_t allocatorPD, u64 size, void* ptr,
                                    u16 properties, ocrParamList_t *perInstance) {

    ocrDataBlockRegular_t *result = (ocrDataBlockRegular_t*)
        checkedMalloc(result, sizeof(ocrDataBlockRegular_t));

    ocrPolicyDomain_t *pd = getCurrentPD();
    ocrPolicyCtx_t *ctx = getCurrentWorkerContext();

    result->base.guid = UNINITIALIZED_GUID;
    result->base.allocator = allocator;
    result->base.allocatorPD = allocatorPD;
    result->base.size = size;
    result->base.ptr = ptr;
    result->base.properties = properties;
    result->base.fctPtrs = &(factory->dataBlockFcts);


    guidify(pd, (u64)result, &(result->base.guid), OCR_GUID_DB);
    result->lock = pd->getLock(pd, ctx);

    result->attributes.flags = result->base.properties;
    result->attributes.numUsers = 0;
    result->attributes.freeRequested = 0;
    ocrGuidTrackerInit(&(result->usersTracker));


    DO_DEBUG(DEBUG_LVL_VERB) {
        PRINTF("VERB: Creating a datablock of size %"PRIu64" @ 0x%"PRIx64" (GUID: 0x%"PRIdPTR")\n",
               size, (u64)result->base.ptr, result->base.guid);
    }

    return (ocrDataBlock_t*)result;
}
/******************************************************/
/* OCR DATABLOCK REGULAR FACTORY                      */
/******************************************************/

static void destructRegularFactory(ocrDataBlockFactory_t *factory) {
    free(factory);
}

ocrDataBlockFactory_t *newDataBlockFactoryRegular(ocrParamList_t *perType) {
    ocrDataBlockFactory_t *base = (ocrDataBlockFactory_t*)
        checkedMalloc(base, sizeof(ocrDataBlockFactoryRegular_t));

    base->instantiate = &newDataBlockRegular;
    base->destruct = &destructRegularFactory;
    base->dataBlockFcts.destruct = &regularDestruct;
    base->dataBlockFcts.acquire = &regularAcquire;
    base->dataBlockFcts.release = &regularRelease;
    base->dataBlockFcts.free = &regularFree;

    return base;
}
