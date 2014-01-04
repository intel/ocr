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

#include HAL_FILE
#include "datablock/regular/regular-datablock.h"
#include "debug.h"
#include "ocr-comp-platform.h"
#include "ocr-datablock.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"
#include "utils/ocr-utils.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#include "ocr-statistics-callbacks.h"
#endif

// TODO: Need to get our own ERRNO file
#include <errno.h>

// TODO: This is for the PRI things. Need to get rid of them 
#include <inttypes.h>


#define DEBUG_TYPE DATABLOCK

// Forward declaraction
void regularDestruct(ocrDataBlock_t *self);

void* regularAcquire(ocrDataBlock_t *self, ocrFatGuid_t edt, bool isInternal) {
    
    ocrDataBlockRegular_t *rself = (ocrDataBlockRegular_t*)self;

    DPRINTF(DEBUG_LVL_VERB, "Acquiring DB @ 0x%"PRIx64" (GUID: 0x%"PRIdPTR") from EDT 0x%"PRIdPTR" (isInternal %d)\n",
                (u64)self->ptr, rself->base.guid, edt.guid, (u32)isInternal);

    // Critical section
    hal_lock32(&(rself->lock));
    if(rself->attributes.freeRequested) {
        hal_unlock32(&(rself->lock));
        return NULL;
    }
    u32 idForEdt = ocrGuidTrackerFind(&(rself->usersTracker), edt.guid);
    if(idForEdt > 63)
        idForEdt = ocrGuidTrackerTrack(&(rself->usersTracker), edt.guid);
    else {
        DPRINTF(DEBUG_LVL_VVERB, "EDT already had acquired DB (pos %d)\n", idForEdt);
        hal_unlock32(&(rself->lock));
        return self->ptr;
    }

    if(idForEdt > 63) {
        hal_unlock32(&(rself->lock));
        return NULL;
    }
    rself->attributes.numUsers += 1;
    if(isInternal)
        rself->attributes.internalUsers += 1;

    hal_unlock32(&(rself->lock));
    // End critical section
    DPRINTF(DEBUG_LVL_VERB, "Added EDT GUID 0x%"PRIx64" at position %d. Have %d users and %d internal\n",
            (u64)edt.guid, idForEdt, rself->attributes.numUsers, rself->attributes.internalUsers);

#ifdef OCR_ENABLE_STATISTICS
    {
        statsDB_ACQ(pd, edt.guid, (ocrTask_t*)edt.metaDataPtr, self->guid, self);
    }
#endif /* OCR_ENABLE_STATISTICS */
    return self->ptr;
}

u8 regularRelease(ocrDataBlock_t *self, ocrFatGuid_t edt,
                  bool isInternal) {
    
    ocrDataBlockRegular_t *rself = (ocrDataBlockRegular_t*)self;
    u32 edtId = ocrGuidTrackerFind(&(rself->usersTracker), edt.guid);
    bool isTracked = true;

    DPRINTF(DEBUG_LVL_VERB, "Releasing DB @ 0x%"PRIx64" (GUID 0x%"PRIdPTR") from EDT 0x%"PRIdPTR" (%d) (internal: %d)\n",
                (u64)self->ptr, rself->base.guid, edt.guid, edtId, (u32)isInternal);
    // Start critical section
    hal_lock32(&(rself->lock));
    if(edtId > 63 || rself->usersTracker.slots[edtId] != edt.guid) {
        // We did not find it. The runtime may be
        // re-releasing it
        if(isInternal) {
            // This is not necessarily an error
            rself->attributes.internalUsers -= 1;
            isTracked = false;
        } else {
            // Definitely a problem here
            hal_unlock32(&(rself->lock));
            return (u8)EACCES;
        }
    }

    if(isTracked) {
        ocrGuidTrackerRemove(&(rself->usersTracker), edt.guid, edtId);
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
        statsDB_REL(getCurrentPD(), edt.guid, (ocrTask_t*)edt.metaDataPtr, self->guid, self);
    }
#endif /* OCR_ENABLE_STATISTICS */
    if(rself->attributes.numUsers == 0  &&
       rself->attributes.internalUsers == 0 &&
       rself->attributes.freeRequested == 1) {
        // We need to actually free the data-block
        hal_unlock32(&(rself->lock));
        regularDestruct(self);
    } else {
        hal_unlock32(&(rself->lock));
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
    ocrTask_t *task = NULL;
    ocrPolicyMsg_t msg;
    getCurrentEnv(&pd, NULL, &task, &msg);
    
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_MEM_DESTROY
    msg.type = PD_MSG_MEM_DESTROY | PD_MSG_REQUEST;
    PD_MSG_FIELD(guid.guid) = self->guid;
    PD_MSG_FIELD(guid.metaDataPtr) = self;
    PD_MSG_FIELD(allocatingPD.guid) = self->allocatingPD;
    PD_MSG_FIELD(allocatingPD.metaDataPtr) = NULL;
    PD_MSG_FIELD(allocator.guid) = self->allocator;
    PD_MSG_FIELD(allocator.metaDataPtr) = NULL;
    PD_MSG_FIELD(ptr) = self->ptr;
    pd->sendMessage(pd, &msg, false);
    
    
#ifdef OCR_ENABLE_STATISTICS
    // This needs to be done before GUID is freed.
    {
        statsDB_DESTROY(pd, task->guid, task, self->allocator, NULL, self->guid, self);
    }
#endif /* OCR_ENABLE_STATISTICS */

#undef PD_TYPE
#define PD_TYPE PD_MSG_GUID_DESTROY
    msg.type = PD_MSG_GUID_DESTROY | PD_MSG_REQUEST;
    // These next two statements may be not required. Just to be safe
    PD_MSG_FIELD(guid.guid) = self->guid;
    PD_MSG_FIELD(guid.metaDataPtr) = self;
    PD_MSG_FIELD(properties) = 1; // Free metadata
    pd->sendMessage(pd, &msg, false);
    
#undef PD_MSG
#undef PD_TYPE
}

u8 regularFree(ocrDataBlock_t *self, ocrFatGuid_t edt) {
    ocrDataBlockRegular_t *rself = (ocrDataBlockRegular_t*)self;
    
    u32 id = ocrGuidTrackerFind(&(rself->usersTracker), edt.guid);
    DPRINTF(DEBUG_LVL_VERB, "Requesting a free for DB @ 0x%"PRIx64" (GUID 0x%"PRIdPTR")\n",
            (u64)self->ptr, rself->base.guid);
    // Begin critical section
    hal_lock32(&(rself->lock));
    if(rself->attributes.freeRequested) {
        hal_unlock32(&(rself->lock));
        return EPERM;
    }
    rself->attributes.freeRequested = 1;
    hal_unlock32(&(rself->lock));
    // End critical section


    if(id < 64) {
        regularRelease(self, edt, false);
    } else {
        // We can call free without having acquired the block
        // Now check if we can actually free the block

        // Critical section
        hal_lock32(&(rself->lock));
        if(rself->attributes.numUsers == 0 && rself->attributes.internalUsers == 0) {
            hal_unlock32(&(rself->lock));
            regularDestruct(self);
        } else {
            hal_unlock32(&(rself->lock));
        }
        // End critical section
    }

    return 0;
}

ocrDataBlock_t* newDataBlockRegular(ocrDataBlockFactory_t *factory, ocrFatGuid_t allocator,
                                    ocrFatGuid_t allocPD, u64 size, void* ptr,
                                    u32 properties, ocrParamList_t *perInstance) {
    ocrPolicyDomain_t *pd = NULL;
    ocrTask_t *task = NULL;
    ocrPolicyMsg_t msg;

    getCurrentEnv(&pd, NULL, &task, &msg);
    
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_CREATE
    msg.type = PD_MSG_GUID_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(guid.guid) = NULL_GUID;
    PD_MSG_FIELD(guid.metaDataPtr) = NULL;
    PD_MSG_FIELD(size) = sizeof(ocrDataBlockRegular_t);
    PD_MSG_FIELD(kind) = OCR_GUID_DB;
    PD_MSG_FIELD(properties) = 0;

    if(pd->processMessage(pd, &msg, true) != 0) {
        DPRINTF(DEBUG_LVL_WARN, "Failure to create a data-block (could not create GUID and metadata for it\n");
        ASSERT(0); // For debugging
        return NULL; // Couldn't allocate GUID
    }

    ocrDataBlockRegular_t *result = (ocrDataBlockRegular_t*)PD_MSG_FIELD(guid.metaDataPtr);
    result->base.guid = PD_MSG_FIELD(guid.guid);
#undef PD_MSG
#undef PD_TYPE

    ASSERT(result);
    result->base.allocator = allocator.guid;
    result->base.allocatingPD = allocPD.guid;
    result->base.size = size;
    result->base.ptr = ptr;
    result->base.properties = properties;
    result->base.funcId = factory->factoryId;
    
    result->lock =0;
    result->attributes.flags = result->base.properties;
    result->attributes.numUsers = 0;
    result->attributes.internalUsers = 0;
    result->attributes.freeRequested = 0;
    ocrGuidTrackerInit(&(result->usersTracker));

#ifdef OCR_ENABLE_STATISTICS
    statsDB_CREATE(pd, task->guid, task, allocator.guid,
                   (ocrAllocator_t*)allocator.metaDataPtr, result->base.guid,
                   &(result->base));
#endif /* OCR_ENABLE_STATISTICS */
    
    DPRINTF(DEBUG_LVL_VERB, "Creating a datablock of size %"PRIu64" @ 0x%"PRIx64" (GUID: 0x%"PRIdPTR")\n",
            size, (u64)result->base.ptr, result->base.guid);

    return (ocrDataBlock_t*)result;
}
/******************************************************/
/* OCR DATABLOCK REGULAR FACTORY                      */
/******************************************************/

static void destructRegularFactory(ocrDataBlockFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrDataBlockFactory_t *newDataBlockFactoryRegular(ocrParamList_t *perType, u32 factoryId) {
    ocrDataBlockFactory_t *base = (ocrDataBlockFactory_t*)
        runtimeChunkAlloc(sizeof(ocrDataBlockFactoryRegular_t), NULL);

    base->instantiate = &newDataBlockRegular;
    base->destruct = &destructRegularFactory;
    base->fcts.destruct = &regularDestruct;
    base->fcts.acquire = &regularAcquire;
    base->fcts.release = &regularRelease;
    base->fcts.free = &regularFree;
    base->factoryId = factoryId;

    return base;
}
#endif /* ENABLE_DATABLOCK_REGULAR */
