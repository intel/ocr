/**
 * @brief Simple implementation of a malloc wrapper
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_DATABLOCK_LOCKABLE

#include "ocr-hal.h"
#include "datablock/lockable/lockable-datablock.h"
#include "debug.h"
#include "ocr-comp-platform.h"
#include "ocr-datablock.h"
#include "ocr-errors.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"
#include "utils/ocr-utils.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#include "ocr-statistics-callbacks.h"
#endif

#define DEBUG_TYPE DATABLOCK

#define DB_LOCKED_NONE 0
#define DB_LOCKED_EW 1
#define DB_LOCKED_ITW 2

// Data-structure to store EDT waiting to be be granted access to the DB
typedef struct _dbWaiter_t {
    ocrGuid_t guid;
    u32 slot;
    u32 properties; // properties specified with the acquire request
    bool isInternal;
    struct _dbWaiter_t * next;
} dbWaiter_t;

// Forward declaraction
u8 lockableDestruct(ocrDataBlock_t *self);

// simple helper function to resolve the location of a guid
static ocrLocation_t fatGuidToLocation(ocrPolicyDomain_t * pd, ocrFatGuid_t fatGuid) {
    // at startup this code may be run outside of an EDT
    if (fatGuid.guid == NULL_GUID) {
        return pd->myLocation;
    } else {
        ocrLocation_t edtLoc = -1;
        u8 res = guidLocation(pd, fatGuid, &edtLoc);
        ASSERT(!res);
        return edtLoc;
    }
}

static ocrLocation_t guidToLocation(ocrPolicyDomain_t * pd, ocrGuid_t edtGuid) {
    // at startup this code may be run outside of an EDT
    if (edtGuid == NULL_GUID) {
        return pd->myLocation;
    } else {
        ocrFatGuid_t fatGuid;
        fatGuid.guid = edtGuid;
        fatGuid.metaDataPtr = NULL;
        ocrLocation_t edtLoc = -1;
        u8 res = guidLocation(pd, fatGuid, &edtLoc);
        ASSERT(!res);
        return edtLoc;
    }
}

//Warning: Calling context must own rself->lock
static dbWaiter_t * popEwWaiter(ocrDataBlock_t * self) {
    ocrDataBlockLockable_t * rself = (ocrDataBlockLockable_t*) self;
    ASSERT(rself->ewWaiterList != NULL);
    dbWaiter_t * waiter = rself->ewWaiterList;
    rself->ewWaiterList = waiter->next;
    return waiter;
}

//Warning: Calling context must own rself->lock
static dbWaiter_t * popItwWaiter(ocrDataBlock_t * self) {
    ocrDataBlockLockable_t * rself = (ocrDataBlockLockable_t*) self;
    ASSERT(rself->itwWaiterList != NULL);
    dbWaiter_t * waiter = rself->itwWaiterList;
    rself->itwWaiterList = waiter->next;
    return waiter;
}

//Warning: Calling context must own rself->lock
static dbWaiter_t * popRoWaiter(ocrDataBlock_t * self) {
    ocrDataBlockLockable_t * rself = (ocrDataBlockLockable_t*) self;
    ASSERT(rself->roWaiterList != NULL);
    dbWaiter_t * waiter = rself->roWaiterList;
    rself->roWaiterList = waiter->next;
    return waiter;
}

static bool lockButSelf(ocrDataBlockLockable_t *rself) {
    ocrWorker_t * worker;
    getCurrentEnv(NULL, &worker, NULL, NULL);
    bool unlock = true;
    if (rself->lock) {
        if (worker == rself->worker) {
            // fall-through
            unlock = false;
        } else {
            hal_lock32(&rself->lock);
        }
    } else {
        hal_lock32(&rself->lock);
        rself->worker = worker;
    }
    return unlock;
}

//Warning: This call must be protected with the self->lock in the calling context
static u8 lockableAcquireInternal(ocrDataBlock_t *self, void** ptr, ocrFatGuid_t edt, u32 edtSlot,
                  ocrDbAccessMode_t mode, bool isInternal, u32 properties) {
    ocrDataBlockLockable_t * rself = (ocrDataBlockLockable_t*) self;

    if(rself->attributes.freeRequested && (rself->attributes.numUsers == 0)) {
        // Most likely stemming from an error in the user-code
        // There's a race between the datablock being freed and having no
        // users with someone else trying to acquire the DB
        ASSERT(false && "OCR_EACCES");
        return OCR_EACCES;
    }

    //TODO DBX this is temporary until we get a real OB mode implemented
    if(properties & DB_PROP_RT_OBLIVIOUS) {
        *ptr = self->ptr;
        return 0;
    }

    if (mode == DB_MODE_RO) {
        if (rself->attributes.modeLock) {
            ocrPolicyDomain_t * pd = NULL;
            getCurrentEnv(&pd, NULL, NULL, NULL);
            dbWaiter_t * waiterEntry = (dbWaiter_t *) pd->fcts.pdMalloc(pd, sizeof(dbWaiter_t));
            waiterEntry->guid = edt.guid;
            waiterEntry->slot = edtSlot;
            waiterEntry->isInternal = isInternal;
            waiterEntry->properties = properties;
            waiterEntry->next = rself->roWaiterList;
            rself->roWaiterList = waiterEntry;
            *ptr = NULL; // not contractual, but should ease debug if misread
            return OCR_EBUSY;
        }
    }

    if (mode == DB_MODE_EW) {
        if ((rself->attributes.modeLock) || (rself->attributes.numUsers != 0)) {
            // The DB is already in use
            ocrPolicyDomain_t * pd = NULL;
            getCurrentEnv(&pd, NULL, NULL, NULL);
            dbWaiter_t * waiterEntry = (dbWaiter_t *) pd->fcts.pdMalloc(pd, sizeof(dbWaiter_t));
            waiterEntry->guid = edt.guid;
            waiterEntry->slot = edtSlot;
            waiterEntry->isInternal = isInternal;
            waiterEntry->properties = properties;
            waiterEntry->next = rself->ewWaiterList;
            rself->ewWaiterList = waiterEntry;
            *ptr = NULL; // not contractual, but should ease debug if misread
            return OCR_EBUSY;
        } else {
            rself->attributes.modeLock = DB_LOCKED_EW;
        }
    }

    if (mode == DB_MODE_ITW) {
        bool enque = false;
        if (rself->attributes.modeLock == DB_LOCKED_ITW) {
            ocrPolicyDomain_t * pd = NULL;
            getCurrentEnv(&pd, NULL, NULL, NULL);
            // check of DB already in ITW use by another location
            enque = (fatGuidToLocation(pd, edt) != rself->itwLocation);
        } else {
            enque = (rself->attributes.numUsers != 0) || (rself->attributes.modeLock == DB_LOCKED_EW);
        }
        if (enque) {
            // The DB is already in use, enque
            ocrPolicyDomain_t * pd = NULL;
            getCurrentEnv(&pd, NULL, NULL, NULL);
            dbWaiter_t * waiterEntry = (dbWaiter_t *) pd->fcts.pdMalloc(pd, sizeof(dbWaiter_t));
            waiterEntry->guid = edt.guid;
            waiterEntry->slot = edtSlot;
            waiterEntry->isInternal = isInternal;
            waiterEntry->properties = properties;
            waiterEntry->next = rself->itwWaiterList;
            rself->itwWaiterList = waiterEntry;
            *ptr = NULL; // not contractual, but should ease debug if misread
            return OCR_EBUSY;
        } else {
            // Ruled out all enque scenario, we can acquire.
            // Just check if we need to grab lock too
            if (rself->attributes.numUsers == 0) {
                ocrPolicyDomain_t * pd = NULL;
                getCurrentEnv(&pd, NULL, NULL, NULL);
                rself->attributes.modeLock = DB_LOCKED_ITW;
                rself->itwLocation = fatGuidToLocation(pd, edt);
            }
        }
    }

    rself->attributes.numUsers += 1;

    DPRINTF(DEBUG_LVL_VERB, "Acquiring DB @ 0x%lx (GUID: 0x%lx) from EDT (GUID: 0x%lx) (runtime acquire: %d) (mode: %d) (numUsers: %d) (modeLock: %d)\n",
            (u64)self->ptr, rself->base.guid, edt.guid, (u32)isInternal, (int) mode,
            rself->attributes.numUsers, rself->attributes.modeLock);

#ifdef OCR_ENABLE_STATISTICS
    {
        statsDB_ACQ(pd, edt.guid, (ocrTask_t*)edt.metaDataPtr, self->guid, self);
    }
#endif /* OCR_ENABLE_STATISTICS */
    *ptr = self->ptr;
    return 0;
}


/**
 * @brief Setup a callback response message and acquire on behalf of the waiter
 **/
static void processAcquireCallback(ocrDataBlock_t *self, dbWaiter_t * waiter, ocrDbAccessMode_t waiterMode, u32 properties, ocrPolicyMsg_t * msg) {
    getCurrentEnv(NULL, NULL, NULL, msg);
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DB_ACQUIRE
    msg->type = PD_MSG_DB_ACQUIRE | PD_MSG_RESPONSE;
    PD_MSG_FIELD(guid.guid) = self->guid;
    PD_MSG_FIELD(guid.metaDataPtr) = self;
    PD_MSG_FIELD(edt.guid) = waiter->guid;
    PD_MSG_FIELD(edt.metaDataPtr) = NULL;
    PD_MSG_FIELD(edtSlot) = waiter->slot;
    PD_MSG_FIELD(size) = self->size;
    // In this implementation properties encodes the MODE + isInternal +
    // any additional flags set by the PD (such as the FETCH flag)
    PD_MSG_FIELD(properties) = properties;
    PD_MSG_FIELD(returnDetail) = 0;
    //NOTE: we still have the lock, calling the internal version
    u8 res = lockableAcquireInternal(self, &PD_MSG_FIELD(ptr), PD_MSG_FIELD(edt),
                                  PD_MSG_FIELD(edtSlot), waiterMode, waiter->isInternal,
                                  PD_MSG_FIELD(properties));
    // Not much we would be able to recover here
    ASSERT(!res);
#undef PD_MSG
#undef PD_TYPE
}

u8 lockableAcquire(ocrDataBlock_t *self, void** ptr, ocrFatGuid_t edt, u32 edtSlot,
                  ocrDbAccessMode_t mode, bool isInternal, u32 properties) {
    ocrDataBlockLockable_t *rself = (ocrDataBlockLockable_t*)self;
    ocrWorker_t * worker;
    getCurrentEnv(NULL, &worker, NULL, NULL);
    bool unlock = lockButSelf(rself);
    u8 res = lockableAcquireInternal(self, ptr, edt, edtSlot, mode, isInternal, properties);
    if (unlock) {
        rself->worker = NULL;
        hal_unlock32(&rself->lock);
    }
    return res;
}

u8 lockableRelease(ocrDataBlock_t *self, ocrFatGuid_t edt, bool isInternal) {
    ocrDataBlockLockable_t *rself = (ocrDataBlockLockable_t*)self;
    dbWaiter_t * waiter = NULL;
    DPRINTF(DEBUG_LVL_VERB, "Releasing DB @ 0x%lx (GUID 0x%lx) from EDT 0x%lx (runtime release: %d)\n",
            (u64)self->ptr, rself->base.guid, edt.guid, (u32)isInternal);
    // Start critical section
    hal_lock32(&(rself->lock));
    ocrWorker_t * worker;
    getCurrentEnv(NULL, &worker, NULL, NULL);
    rself->worker = worker;

    // The registered EDT can be different if a DB has been released by the user
    // and the runtime tries to release it afterwards. It could be that in
    // between the two releases, the slot is used for another EDT's DB.
    rself->attributes.numUsers -= 1;

    // catch errors when release is called one too many time
    ASSERT(rself->attributes.numUsers != (u32)-1);

    //IMPL: this is probably not very fair
    if (rself->attributes.numUsers == 0) {
        // last user of the DB, check what the transition looks like:
        // W -> W (pop the next EW waiter)
        // RO -> EW (pop the next EW waiter)
        // EW -> RO (pop all RO waiter)
        // RO -> RO (nothing to do)
        if (rself->attributes.modeLock) {
            // Last ITW writer. Most likely there are either more ITW on their way
            // or we're done writing and there are RO piling up or on their way.
            rself->attributes.modeLock = DB_LOCKED_NONE; // release and see what we got
            rself->itwLocation = -1;
            if (rself->roWaiterList != NULL) {
                waiter = popRoWaiter(self);
            }
        } else {
            // We're in RO, by all means there should be no RO waiter
            ASSERT(rself->roWaiterList == NULL);
            // Try to favor writers
        }

        if (waiter == NULL) {
            if (rself->itwWaiterList != NULL) {
                waiter = popItwWaiter(self);
                rself->attributes.modeLock = DB_LOCKED_ITW;
            } else if (rself->ewWaiterList != NULL) {
                waiter = popEwWaiter(self);
                rself->attributes.modeLock = DB_LOCKED_EW;
            }
        }

        if (rself->attributes.modeLock == DB_LOCKED_ITW) {
            // NOTE: by design there should be a waiter otherwise we would have exit the modeLock
            ASSERT(waiter != NULL);
            // Switching to ITW mode, now we can release all waiters that belong
            // to the same location the elected waiter is.
            ocrPolicyDomain_t * pd = NULL;
            ocrPolicyMsg_t msg;
            getCurrentEnv(&pd, NULL, NULL, NULL);
            // Setup: Have ITW lock + its right location for acquire
            ocrLocation_t itwLocation = guidToLocation(pd, waiter->guid);
            rself->itwLocation = itwLocation;
            dbWaiter_t * prev = waiter;
            do {
                dbWaiter_t * next = waiter->next;
                if (itwLocation == guidToLocation(pd, waiter->guid)) {
                    processAcquireCallback(self, waiter, DB_MODE_ITW, waiter->properties, &msg);
                    if (prev == waiter) { // removing head
                        prev = next;
                        rself->itwWaiterList = prev;
                    } else {
                        prev->next = next;
                    }
                    pd->fcts.pdFree(pd, waiter);
                    waiter = next;
                    //PERF: Would be nice to do that outside the lock but it incurs allocating
                    // an array of messages and traversing the list of waiters again
                    ASSERT(!pd->fcts.processMessage(pd, &msg, true));
                } else {
                    prev = waiter;
                    waiter = next;
                }
            } while (waiter != NULL);
            #ifdef OCR_ENABLE_STATISTICS
                {
                    statsDB_REL(getCurrentPD(), edt.guid, (ocrTask_t*)edt.metaDataPtr, self->guid, self);
                }
            #endif /* OCR_ENABLE_STATISTICS */
            rself->worker = NULL;
            hal_unlock32(&(rself->lock));
            return 0;
        } else if (rself->attributes.modeLock == DB_LOCKED_EW) {
            // EW: by design there should be a waiter otherwise we would have exit the modeLock
            ASSERT(waiter != NULL);
            ocrPolicyDomain_t * pd = NULL;
            ocrPolicyMsg_t msg;
            getCurrentEnv(&pd, NULL, NULL, &msg);
            rself->attributes.modeLock = DB_LOCKED_NONE; // technicality so that acquire sees the DB is not acquired
            // Acquire the DB on behalf of the next waiter (i.e. numUser++)
            processAcquireCallback(self, waiter, DB_MODE_EW, waiter->properties, &msg);
            // Will process asynchronous callback message outside of the critical section
            #ifdef OCR_ENABLE_STATISTICS
                {
                    statsDB_REL(getCurrentPD(), edt.guid, (ocrTask_t*)edt.metaDataPtr, self->guid, self);
                }
            #endif /* OCR_ENABLE_STATISTICS */
            rself->worker = NULL;
            hal_unlock32(&(rself->lock));
            pd->fcts.pdFree(pd, waiter);
            ASSERT(!pd->fcts.processMessage(pd, &msg, true));
            return 0;
        } else { // RO
            //NOTE: if waiter is NULL it means there was nobody queued up for any mode
            if (waiter != NULL) {
                // transition EW -> RO, release all RO waiters
                ocrPolicyDomain_t * pd = NULL;
                ocrPolicyMsg_t msg;
                getCurrentEnv(&pd, NULL, NULL, NULL);
                rself->roWaiterList = NULL;
                do {
                    processAcquireCallback(self, waiter, DB_MODE_RO, waiter->properties, &msg);
                    dbWaiter_t * next = waiter->next;
                    pd->fcts.pdFree(pd, waiter);
                    //PERF: Would be nice to do that outside the lock but it incurs allocating
                    // an array of messages and traversing the list of waiters again
                    ASSERT(!pd->fcts.processMessage(pd, &msg, true));
                    waiter = next;
                } while (waiter != NULL);
                ASSERT(rself->roWaiterList == NULL);
                #ifdef OCR_ENABLE_STATISTICS
                    {
                        statsDB_REL(getCurrentPD(), edt.guid, (ocrTask_t*)edt.metaDataPtr, self->guid, self);
                    }
                #endif /* OCR_ENABLE_STATISTICS */
                rself->worker = NULL;
                hal_unlock32(&(rself->lock));
                return 0;
            }
        }
    }
    DPRINTF(DEBUG_LVL_VVERB, "DB (GUID: 0x%lx) attributes: numUsers %d (including %d runtime users); freeRequested %d\n",
            self->guid, rself->attributes.numUsers, rself->attributes.internalUsers, rself->attributes.freeRequested);

#ifdef OCR_ENABLE_STATISTICS
    {
        statsDB_REL(getCurrentPD(), edt.guid, (ocrTask_t*)edt.metaDataPtr, self->guid, self);
    }
#endif /* OCR_ENABLE_STATISTICS */

    // Check if we need to free the block
    if(rself->attributes.numUsers == 0 &&
        rself->attributes.internalUsers == 0 &&
        rself->attributes.freeRequested == 1) {
        rself->worker = NULL;
        hal_unlock32(&(rself->lock));
        return lockableDestruct(self);
    }
    rself->worker = NULL;
    hal_unlock32(&(rself->lock));
    return 0;
}

u8 lockableDestruct(ocrDataBlock_t *self) {
    ocrDataBlockLockable_t *rself = (ocrDataBlockLockable_t*)self;
    // Any of these wrong would indicate a race between free and DB's consumers
    ASSERT(rself->attributes.numUsers == 0);
    ASSERT(rself->attributes.internalUsers == 0);
    ASSERT(rself->attributes.freeRequested == 1);
    ASSERT(rself->roWaiterList == NULL);
    ASSERT(rself->ewWaiterList == NULL);
    ASSERT(rself->itwWaiterList == NULL);
    ASSERT(rself->lock == 0);

    DPRINTF(DEBUG_LVL_VERB, "Freeing DB (GUID: 0x%lx)\n", self->guid);
    ocrPolicyDomain_t *pd = NULL;
    ocrTask_t *task = NULL;
    ocrPolicyMsg_t msg;
    getCurrentEnv(&pd, NULL, &task, &msg);

#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_MEM_UNALLOC
    msg.type = PD_MSG_MEM_UNALLOC | PD_MSG_REQUEST;
    PD_MSG_FIELD(ptr) = self->ptr;
    PD_MSG_FIELD(allocatingPD.guid) = self->allocatingPD;
    PD_MSG_FIELD(allocatingPD.metaDataPtr) = NULL;
    PD_MSG_FIELD(allocator.guid) = self->allocator;
    PD_MSG_FIELD(allocator.metaDataPtr) = NULL;
    PD_MSG_FIELD(type) = DB_MEMTYPE;
    PD_MSG_FIELD(properties) = 0;
    RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, false));


#ifdef OCR_ENABLE_STATISTICS
    // This needs to be done before GUID is freed.
    {
        statsDB_DESTROY(pd, task->guid, task, self->allocator, NULL, self->guid, self);
    }
#endif /* OCR_ENABLE_STATISTICS */
#undef PD_TYPE
#define PD_TYPE PD_MSG_GUID_DESTROY
    getCurrentEnv(NULL, NULL, NULL, &msg);
    msg.type = PD_MSG_GUID_DESTROY | PD_MSG_REQUEST;
    // These next two statements may be not required. Just to be safe
    PD_MSG_FIELD(guid.guid) = self->guid;
    PD_MSG_FIELD(guid.metaDataPtr) = self;
    PD_MSG_FIELD(properties) = 1; // Free metadata
    RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, false));
#undef PD_MSG
#undef PD_TYPE
    return 0;
}

u8 lockableFree(ocrDataBlock_t *self, ocrFatGuid_t edt) {
    ocrDataBlockLockable_t *rself = (ocrDataBlockLockable_t*)self;
    DPRINTF(DEBUG_LVL_VERB, "Requesting a free for DB @ 0x%lx (GUID: 0x%lx)\n",
            (u64)self->ptr, rself->base.guid);
    hal_lock32(&(rself->lock));
    if(rself->attributes.freeRequested) {
        hal_unlock32(&(rself->lock));
        return OCR_EPERM;
    }
    rself->attributes.freeRequested = 1;
    if(rself->attributes.numUsers == 0 && rself->attributes.internalUsers == 0) {
        hal_unlock32(&(rself->lock));
        return lockableDestruct(self);
    }
    hal_unlock32(&(rself->lock));
    return 0;
}

u8 lockableRegisterWaiter(ocrDataBlock_t *self, ocrFatGuid_t waiter, u32 slot,
                         bool isDepAdd) {
    ASSERT(0);
    return OCR_ENOSYS;
}

u8 lockableUnregisterWaiter(ocrDataBlock_t *self, ocrFatGuid_t waiter, u32 slot,
                           bool isDepRem) {
    ASSERT(0);
    return OCR_ENOSYS;
}

ocrDataBlock_t* newDataBlockLockable(ocrDataBlockFactory_t *factory, ocrFatGuid_t allocator,
                                    ocrFatGuid_t allocPD, u64 size, void* ptr,
                                    u32 flags, ocrParamList_t *perInstance) {
    ocrPolicyDomain_t *pd = NULL;
    ocrTask_t *task = NULL;
    ocrPolicyMsg_t msg;

    getCurrentEnv(&pd, NULL, &task, &msg);

#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_CREATE
    msg.type = PD_MSG_GUID_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(guid.guid) = NULL_GUID;
    PD_MSG_FIELD(guid.metaDataPtr) = NULL;
    PD_MSG_FIELD(size) = sizeof(ocrDataBlockLockable_t);
    PD_MSG_FIELD(kind) = OCR_GUID_DB;
    PD_MSG_FIELD(properties) = 0;

    RESULT_PROPAGATE2(pd->fcts.processMessage(pd, &msg, true), NULL);

    ocrDataBlockLockable_t *result = (ocrDataBlockLockable_t*)PD_MSG_FIELD(guid.metaDataPtr);
    result->base.guid = PD_MSG_FIELD(guid.guid);
#undef PD_MSG
#undef PD_TYPE

    ASSERT(result);
    result->base.allocator = allocator.guid;
    result->base.allocatingPD = allocPD.guid;
    result->base.size = size;
    result->base.ptr = ptr;
    result->base.fctId = factory->factoryId;
    // Only keep flags that represent the nature of
    // the DB as opposed to one-time usage creation flags
    result->base.flags = (flags & DB_PROP_SINGLE_ASSIGNMENT);
    result->lock = 0;
    result->attributes.flags = result->base.flags;
    result->attributes.numUsers = 0;
    result->attributes.internalUsers = 0;
    result->attributes.freeRequested = 0;
    result->attributes.modeLock = DB_LOCKED_NONE;
    result->ewWaiterList = NULL;
    result->roWaiterList = NULL;
    result->itwWaiterList = NULL;
    result->itwLocation = -1;
    result->worker = NULL;
    ocrGuidTrackerInit(&(result->usersTracker));

#ifdef OCR_ENABLE_STATISTICS
    statsDB_CREATE(pd, task->guid, task, allocator.guid,
                   (ocrAllocator_t*)allocator.metaDataPtr, result->base.guid,
                   &(result->base));
#endif /* OCR_ENABLE_STATISTICS */

    DPRINTF(DEBUG_LVL_VERB, "Creating a datablock of size %lu @ 0x%lx (GUID: 0x%lx)\n",
            size, (u64)result->base.ptr, result->base.guid);

    return (ocrDataBlock_t*)result;
}

/******************************************************/
/* OCR DATABLOCK LOCKABLE FACTORY                      */
/******************************************************/

void destructLockableFactory(ocrDataBlockFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrDataBlockFactory_t *newDataBlockFactoryLockable(ocrParamList_t *perType, u32 factoryId) {
    ocrDataBlockFactory_t *base = (ocrDataBlockFactory_t*)
                                  runtimeChunkAlloc(sizeof(ocrDataBlockFactoryLockable_t), NULL);

    base->instantiate = FUNC_ADDR(ocrDataBlock_t* (*)
                                  (ocrDataBlockFactory_t*, ocrFatGuid_t, ocrFatGuid_t,
                                   u64, void*, u32, ocrParamList_t*), newDataBlockLockable);
    base->destruct = FUNC_ADDR(void (*)(ocrDataBlockFactory_t*), destructLockableFactory);
    base->fcts.destruct = FUNC_ADDR(u8 (*)(ocrDataBlock_t*), lockableDestruct);
    base->fcts.acquire = FUNC_ADDR(u8 (*)(ocrDataBlock_t*, void**, ocrFatGuid_t, u32, ocrDbAccessMode_t, bool, u32), lockableAcquire);
    base->fcts.release = FUNC_ADDR(u8 (*)(ocrDataBlock_t*, ocrFatGuid_t, bool), lockableRelease);
    base->fcts.free = FUNC_ADDR(u8 (*)(ocrDataBlock_t*, ocrFatGuid_t), lockableFree);
    base->fcts.registerWaiter = FUNC_ADDR(u8 (*)(ocrDataBlock_t*, ocrFatGuid_t,
                                                 u32, bool), lockableRegisterWaiter);
    base->fcts.unregisterWaiter = FUNC_ADDR(u8 (*)(ocrDataBlock_t*, ocrFatGuid_t,
                                                   u32, bool), lockableUnregisterWaiter);
    base->factoryId = factoryId;

    return base;
}
#endif /* ENABLE_DATABLOCK_LOCKABLE */
