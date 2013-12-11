/**
 * @brief Simple implementation of a malloc wrapper
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_DATABLOCK_REGULAR

#include "datablock/regular/regular-datablock.h"
#include "debug.h"
#include "ocr-comp-platform.h"
#include "ocr-datablock.h"
#include "ocr-macros.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"
#include "ocr-sys.h"
#include "ocr-utils.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#include "ocr-statistics-callbacks.h"
#endif

// TODO: Need to get our own ERRNO file
#include <errno.h>
//#include <stdlib.h>

// TODO: This is for the PRI things. Need to get rid of them 
#include <inttypes.h>


#define DEBUG_TYPE DATABLOCK

void* regularAcquire(ocrDataBlock_t *self, ocrGuid_t edt, bool isInternal) {
    ocrPolicyDomain_t *pd = NULL;
    getCurrentEnv(&pd, NULL, NULL);
    ocrSys_t *sys = pd->getSys();
    
    ocrDataBlockRegular_t *rself = (ocrDataBlockRegular_t*)self;

    DPRINTF(DEBUG_LVL_VERB, "Acquiring DB @ 0x%"PRIx64" (GUID: 0x%"PRIdPTR") from EDT 0x%"PRIdPTR" (isInternal %d)\n",
                (u64)self->ptr, rself->base.guid, edt, (u32)isInternal);

    // Critical section
    sys->lock32(sys, &(rself->lock));
    if(rself->attributes.freeRequested) {
        sys->unlock32(sys, &(rself->lock);
        return NULL;
    }
    u32 idForEdt = ocrGuidTrackerFind(&(rself->usersTracker), edt);
    if(idForEdt > 63)
        idForEdt = ocrGuidTrackerTrack(&(rself->usersTracker), edt);
    else {
        DPRINTF(DEBUG_LVL_VVERB, "EDT already had acquired DB (pos %d)\n", idForEdt);
        sys->unlock32(sys, &(rself->lock));
        return self->ptr;
    }

    if(idForEdt > 63) {
        sys->unlock32(sys, &(rself->lock));
        return NULL;
    }
    rself->attributes.numUsers += 1;
    if(isInternal)
        rself->attributes.internalUsers += 1;

    sys->unlock32(sys, &(rself->lock));
    // End critical section
    DPRINTF(DEBUG_LVL_VERB, "Added EDT GUID 0x%"PRIx64" at position %d. Have %d users and %d internal\n",
            (u64)edt, idForEdt, rself->attributes.numUsers, rself->attributes.internalUsers);

#ifdef OCR_ENABLE_STATISTICS
    {
        statsDB_ACQ(pd, edt, NULL, self->guid, self);
    }
#endif /* OCR_ENABLE_STATISTICS */
    return self->ptr;
}

u8 regularRelease(ocrDataBlock_t *self, ocrGuid_t edt,
                  bool isInternal) {

    ocrPolicyDomain_t *pd = NULL;
    getCurrentEnv(&pd, NULL, NULL);
    ocrSys_t *sys = pd->getSys();
    
    ocrDataBlockRegular_t *rself = (ocrDataBlockRegular_t*)self;
    u32 edtId = ocrGuidTrackerFind(&(rself->usersTracker), edt);
    bool isTracked = true;

    DPRINTF(DEBUG_LVL_VERB, "Releasing DB @ 0x%"PRIx64" (GUID 0x%"PRIdPTR") from EDT 0x%"PRIdPTR" (%d) (internal: %d)\n",
                (u64)self->ptr, rself->base.guid, edt, edtId, (u32)isInternal);
    // Start critical section
    sys->lock32(sys, &(rself->lock));
    if(edtId > 63 || rself->usersTracker.slots[edtId] != edt) {
        // We did not find it. The runtime may be
        // re-releasing it
        if(isInternal) {
            // This is not necessarily an error
            rself->attributes.internalUsers -= 1;
            isTracked = false;
        } else {
            // Definitely a problem here
            sys->unlock32(sys, &(rself->lock));
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
        sys->unlock(sys, &(rself->lock));
        self->fctPtrs->destruct(self);
    } else {
        sys->unlock(sys, &(rself->lock));
    }
    // End critical section

    return 0;
}

void regularDestruct(ocrDataBlock_t *self) {
    // We don't use a lock here. Maybe we should
    ocrDataBlockRegular_t *rself = (ocrDataBlockRegular_t*)self;
    ASSERT(rself->attributes.numUsers == 0);
    ASSERT(rself->attributes.freeRequested == 1);
    ASSERT(rself->lock == 0);
    
    ocrPolicyDomain_t *pd = NULL;
    ocrPolicyMsg_t msg;
    getCurrentEnv(&pd, &msg, NULL);
    
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_MEM_DESTROY
    msg.type = PD_MSG_MEM_DESTROY | PD_MSG_REQUEST;
    PD_MSG_FIELD(guid.guid) = self->guid;
    PD_MSG_FIELD(guid.metaDataPtr) = self;
    PD_MSG_FIELD(allocatingPD) = self->allocatorPD;
    PD_MSG_FIELD(allocator) = self->allocator;
    PD_MSG_FIELD(ptr) = self->ptr;
    pd->sendMessage(pd, &msg, false);
    
    
#ifdef OCR_ENABLE_STATISTICS
    // This needs to be done before GUID is freed.
    {
        ocrGuid_t edtGuid = getCurrentEDT();
        statsDB_DESTROY(pd, edtGuid, NULL, self->allocator, NULL, self->guid, self);
    }
#endif /* OCR_ENABLE_STATISTICS */
    
#define PD_TYPE PD_MSG_GUID_DESTROY
    msg.type = PD_MSG_GUID_DESTROY | PD_MSG_REQUEST;
    pd->sendMessage(pd, &msg, false);

    // Now free the metadata
    // TODO: How to do this:
    // - Do we tie metadata to GUID (and the metadata gets destroyed with the GUID. This is the FSim model)
    // - Do we always allocate metadata local to the PD (what happens if multiple PDs want to
    // refer to the same data-block)?
    // - Other ideas?
        
#undef PD_MSG
#undef PD_TYPE
    

    //free(rself);
}

u8 regularFree(ocrDataBlock_t *self, ocrGuid_t edt) {
    ocrDataBlockRegular_t *rself = (ocrDataBlockRegular_t*)self;

    ocrPolicyDomain_t *pd = NULL;
    getCurrentEnv(&pd, NULL, NULL);
    ocrSys_t *sys = pd->getSys();
    
    u32 id = ocrGuidTrackerFind(&(rself->usersTracker), edt);
    DPRINTF(DEBUG_LVL_VERB, "Requesting a free for DB @ 0x%"PRIx64" (GUID 0x%"PRIdPTR")\n",
            (u64)self->ptr, rself->base.guid);
    // Begin critical section
    sys->lock32(sys, &(rself->lock));
    if(rself->attributes.freeRequested) {
        sys->unlock32(sys, &(rself->lock));
        return EPERM;
    }
    rself->attributes.freeRequested = 1;
    sys->unlock32(sys,&(rself->lock));
    // End critical section


    if(id < 64) {
        regularRelease(self, edt, false);
    } else {
        // We can call free without having acquired the block
        // Now check if we can actually free the block

        // Critical section
        sys->lock32(sys, &(rself->lock));
        if(rself->attributes.numUsers == 0 && rself->attributes.internalUsers == 0) {
            sys->unlock32(sys, &(rself->lock));
            regularDestruct(self);
        } else {
            sys->unlock32(sys, &(rself->lock));
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
    result->lock =0;

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
#endif /* ENABLE_DATABLOCK_REGULAR */
