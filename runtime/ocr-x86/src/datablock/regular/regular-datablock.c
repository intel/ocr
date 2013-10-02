/**
 * @brief Simple implementation of a malloc wrapper
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "datablock/regular/regular-datablock.h"
#include "debug.h"
#include "ocr-comp-platform.h"
#include "ocr-datablock.h"
#include "ocr-macros.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-policy-domain.h"
#include "ocr-sync.h"
#include "ocr-utils.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#include "ocr-statistics-callbacks.h"
#endif

#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>


#define DEBUG_TYPE DATABLOCK

void* regularAcquire(ocrDataBlock_t *self, ocrGuid_t edt, bool isInternal) {
    ocrDataBlockRegular_t *rself = (ocrDataBlockRegular_t*)self;

    DPRINTF(DEBUG_LVL_VERB, "Acquiring DB @ 0x%"PRIx64" (GUID: 0x%"PRIdPTR") from EDT 0x%"PRIdPTR" (isInternal %d)\n",
                (u64)self->ptr, rself->base.guid, edt, (u32)isInternal);

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
        DPRINTF(DEBUG_LVL_VVERB, "EDT already had acquired DB (pos %d)\n", idForEdt);
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
    DPRINTF(DEBUG_LVL_VERB, "Added EDT GUID 0x%"PRIx64" at position %d. Have %d users and %d internal\n",
            (u64)edt, idForEdt, rself->attributes.numUsers, rself->attributes.internalUsers);

#ifdef OCR_ENABLE_STATISTICS
    {
        statsDB_ACQ(getCurrentPD(), edt, NULL, self->guid, self);
    }
#endif /* OCR_ENABLE_STATISTICS */
    return self->ptr;
}

u8 regularRelease(ocrDataBlock_t *self, ocrGuid_t edt,
                  bool isInternal) {

    ocrDataBlockRegular_t *rself = (ocrDataBlockRegular_t*)self;
    u32 edtId = ocrGuidTrackerFind(&(rself->usersTracker), edt);
    bool isTracked = true;

    DPRINTF(DEBUG_LVL_VERB, "Releasing DB @ 0x%"PRIx64" (GUID 0x%"PRIdPTR") from EDT 0x%"PRIdPTR" (%d) (internal: %d)\n",
                (u64)self->ptr, rself->base.guid, edt, edtId, (u32)isInternal);
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
    DPRINTF(DEBUG_LVL_VVERB, "DB attributes: numUsers %d; internalUsers %d; freeRequested %d\n",
            rself->attributes.numUsers, rself->attributes.internalUsers, rself->attributes.freeRequested);
    // Check if we need to free the block
#ifdef OCR_ENABLE_STATISTICS
    {
        statsDB_REL(getCurrentPD(), edt, NULL, self->guid, self);
    }
#endif /* OCR_ENABLE_STATISTICS */
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

    DPRINTF(DEBUG_LVL_VERB, "Freeing DB @ 0x%"PRIx64" (GUID: 0x%"PRIdPTR")\n", (u64)self->ptr, rself->base.guid);
    allocator->fctPtrs->free(allocator, self->ptr);
    
#ifdef OCR_ENABLE_STATISTICS
    // This needs to be done before GUID is freed.
    {
        ocrGuid_t edtGuid = getCurrentEDT();
        statsDB_DESTROY(pd, edtGuid, NULL, self->allocator, NULL, self->guid, self);
    }
#endif /* OCR_ENABLE_STATISTICS */
    
    pd->inform(pd, self->guid, ctx);
    ctx->destruct(ctx);
    free(rself);
}

u8 regularFree(ocrDataBlock_t *self, ocrGuid_t edt) {
    ocrDataBlockRegular_t *rself = (ocrDataBlockRegular_t*)self;

    u32 id = ocrGuidTrackerFind(&(rself->usersTracker), edt);
    DPRINTF(DEBUG_LVL_VERB, "Requesting a free for DB @ 0x%"PRIx64" (GUID 0x%"PRIdPTR")\n",
            (u64)self->ptr, rself->base.guid);
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
    result->attributes.internalUsers = 0;
    result->attributes.freeRequested = 0;
    ocrGuidTrackerInit(&(result->usersTracker));

#ifdef OCR_ENABLE_STATISTICS
    ocrGuid_t edtGuid = getCurrentEDT();
    statsDB_CREATE(pd, edtGuid, NULL, allocator, NULL, result->base.guid, &(result->base));
#endif /* OCR_ENABLE_STATISTICS */
    
    DPRINTF(DEBUG_LVL_VERB, "Creating a datablock of size %"PRIu64" @ 0x%"PRIx64" (GUID: 0x%"PRIdPTR")\n",
            size, (u64)result->base.ptr, result->base.guid);

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
