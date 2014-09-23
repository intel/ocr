/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "ocr-config.h"
#ifdef ENABLE_POLICY_DOMAIN_HC

#include "debug.h"
#include "ocr-errors.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

#include "utils/profiler/profiler.h"

#include "policy-domain/hc/hc-policy.h"
#include "allocator/allocator-all.h"

//DIST-TODO cloning: hack to support edt templates
#include "task/hc/hc-task.h"
#include "event/hc/hc-event.h"

#define DEBUG_TYPE POLICY

void hcPolicyDomainBegin(ocrPolicyDomain_t * policy) {
    // The PD should have been brought up by now and everything instantiated

    u64 i = 0;
    u64 maxCount = 0;

    maxCount = policy->guidProviderCount;
    for(i = 0; i < maxCount; ++i) {
        policy->guidProviders[i]->fcts.begin(policy->guidProviders[i], policy);
    }

    maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fcts.begin(policy->allocators[i], policy);
    }

    maxCount = policy->schedulerCount;
    for(i = 0; i < maxCount; ++i) {
        policy->schedulers[i]->fcts.begin(policy->schedulers[i], policy);
    }

    maxCount = policy->commApiCount;
    for(i = 0; i < maxCount; i++) {
        policy->commApis[i]->fcts.begin(policy->commApis[i], policy);
    }

    // REC: Moved all workers to start here.
    // Note: it's important to first logically start all workers.
    // Once they are all up, start the runtime.
    // Workers should start the underlying target and platforms
    maxCount = policy->workerCount;
    for(i = 0; i < maxCount; i++) {
        policy->workers[i]->fcts.begin(policy->workers[i], policy);
    }

    // We have now started everyone so we swap the state
    // The expected behavior is 0 -> 1
    // This does not have to be a cmpswap but leaving
    // it that way to verify logic
    // Note that we swap here because start is allowed to use pdMalloc
    // for example (ie: the PD should at least be able to respond to queries
    // after begin is done)
    ocrPolicyDomainHc_t *rself = (ocrPolicyDomainHc_t*)policy;
    u32 oldState = hal_cmpswap32(&(rself->state), 0, 1);
    ASSERT(oldState == 0); // No EDT should be able
    // to run at this point since mainEDT
    // is started AFTER this function completes
}

void hcPolicyDomainStart(ocrPolicyDomain_t * policy) {
    // The PD should have been brought up by now and everything instantiated
    // This is a bit ugly but I can't find a cleaner solution:
    //   - we need to associate the environment with the
    //   currently running worker/PD so that we can use getCurrentEnv

    u64 i = 0;
    u64 maxCount = 0;

    maxCount = policy->guidProviderCount;
    for(i = 0; i < maxCount; ++i) {
        policy->guidProviders[i]->fcts.start(policy->guidProviders[i], policy);
    }

    guidify(policy, (u64)policy, &(policy->fguid), OCR_GUID_POLICY);

    maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fcts.start(policy->allocators[i], policy);
    }

    maxCount = policy->schedulerCount;
    for(i = 0; i < maxCount; ++i) {
        policy->schedulers[i]->fcts.start(policy->schedulers[i], policy);
    }

    maxCount = policy->commApiCount;
    for(i = 0; i < maxCount; i++) {
        policy->commApis[i]->fcts.start(policy->commApis[i], policy);
    }

    // Create and initialize the placer (work in progress)
    policy->placer = createLocationPlacer(policy);

    // REC: Moved all workers to start here.
    // Note: it's important to first logically start all workers.
    // Once they are all up, start the runtime.
    // Workers should start the underlying target and platforms
    maxCount = policy->workerCount;
    for(i = 0; i < maxCount; i++) {
        policy->workers[i]->fcts.start(policy->workers[i], policy);
    }
}

void hcPolicyDomainFinish(ocrPolicyDomain_t * policy) {
    // Finish everything in reverse order
    u64 i = 0;
    u64 maxCount = 0;

    // Worker 0 enters here
    ocrPolicyDomainHc_t *rself = (ocrPolicyDomainHc_t*)policy;
    DPRINTF(DEBUG_LVL_VERB,"Begin PD finish wait loop\n");
    u32 oldState = 0;
    // Wait for stop to complete before we start with finish
    do {
        oldState = hal_cmpswap32(&(rself->state), 10, 14);
    } while(oldState != 10);

    DPRINTF(DEBUG_LVL_VERB,"End PD finish wait loop\n");
    // Note: As soon as worker '0' is stopped; its thread is
    // free to fall-through and continue shutting down the
    // policy domain
    maxCount = policy->workerCount;
    for(i = 0; i < maxCount; i++) {
        policy->workers[i]->fcts.finish(policy->workers[i]);
    }

    maxCount = policy->commApiCount;
    for(i = 0; i < maxCount; i++) {
        policy->commApis[i]->fcts.finish(policy->commApis[i]);
    }

    maxCount = policy->schedulerCount;
    for(i = 0; i < maxCount; ++i) {
        policy->schedulers[i]->fcts.finish(policy->schedulers[i]);
    }

    maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fcts.finish(policy->allocators[i]);
    }

    maxCount = policy->guidProviderCount;
    for(i = 0; i < maxCount; ++i) {
        policy->guidProviders[i]->fcts.finish(policy->guidProviders[i]);
    }

}

void hcPolicyDomainStop(ocrPolicyDomain_t * policy) {
    u64 i = 0;
    u64 maxCount = 0;

    ocrPolicyDomainHc_t *rself = (ocrPolicyDomainHc_t*)policy;

    // We inform people that we want to stop the policy domain
    // We make sure that no one else is using the PD (in processMessage mostly)
    DPRINTF(DEBUG_LVL_VERB,"Begin PD stop wait loop\n");
    u32 oldState = 0, newState = 0;
    do {
        oldState = rself->state;
        newState = (oldState & 0xFFFFFFF0) | 2;
        newState = hal_cmpswap32(&(rself->state), oldState, newState);
    } while(oldState != newState);
    // Now we have set the "I want to shut-down" bit so now we need
    // to wait for the users to drain
    // We do something really stupid and just loop for now
    while(rself->state != 18);
    DPRINTF(DEBUG_LVL_VERB,"End PD stop wait loop\n");
    // Here we can start stopping the PD
    ASSERT(rself->state == 18); // We should have no users and have managed to set

    // Note: As soon as worker '0' is stopped; its thread is
    // free to fall-through from 'start' and call 'finish'.
    maxCount = policy->workerCount;
    for(i = 0; i < maxCount; i++) {
        policy->workers[i]->fcts.stop(policy->workers[i]);
    }

    maxCount = policy->commApiCount;
    for(i = 0; i < maxCount; i++) {
        policy->commApis[i]->fcts.stop(policy->commApis[i]);
    }

    maxCount = policy->schedulerCount;
    for(i = 0; i < maxCount; ++i) {
        policy->schedulers[i]->fcts.stop(policy->schedulers[i]);
    }

    maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fcts.stop(policy->allocators[i]);
    }

    // We could release our GUID here but not really required

    maxCount = policy->guidProviderCount;
    for(i = 0; i < maxCount; ++i) {
        policy->guidProviders[i]->fcts.stop(policy->guidProviders[i]);
    }

    // This does not need to be a cmpswap but keeping it
    // this way for now to make sure the logic is sound
    oldState = hal_cmpswap32(&(rself->state), 18, 26);
    ASSERT(oldState == 18);
}

void hcPolicyDomainDestruct(ocrPolicyDomain_t * policy) {
    // Destroying instances
    u64 i = 0;
    u64 maxCount = 0;

    ocrPolicyDomainHc_t* rself = (ocrPolicyDomainHc_t*)policy;
    ASSERT(rself->state == 14);

    // Note: As soon as worker '0' is stopped; its thread is
    // free to fall-through and continue shutting down the
    // policy domain

    maxCount = policy->workerCount;
    for(i = 0; i < maxCount; i++) {
        policy->workers[i]->fcts.destruct(policy->workers[i]);
    }

    maxCount = policy->commApiCount;
    for(i = 0; i < maxCount; i++) {
        policy->commApis[i]->fcts.destruct(policy->commApis[i]);
    }

    maxCount = policy->schedulerCount;
    for(i = 0; i < maxCount; ++i) {
        policy->schedulers[i]->fcts.destruct(policy->schedulers[i]);
    }

    //TODO Need a scheme to deallocate neighbors
    //ASSERT(policy->neighbors == NULL);

    // Destruct factories

    maxCount = policy->taskFactoryCount;
    for(i = 0; i < maxCount; ++i) {
        policy->taskFactories[i]->destruct(policy->taskFactories[i]);
    }

    maxCount = policy->eventFactoryCount;
    for(i = 0; i < maxCount; ++i) {
        policy->eventFactories[i]->destruct(policy->eventFactories[i]);
    }

    maxCount = policy->taskTemplateFactoryCount;
    for(i = 0; i < maxCount; ++i) {
        policy->taskTemplateFactories[i]->destruct(policy->taskTemplateFactories[i]);
    }

    maxCount = policy->dbFactoryCount;
    for(i = 0; i < maxCount; ++i) {
        policy->dbFactories[i]->destruct(policy->dbFactories[i]);
    }

    //Anticipate those to be null-impl for some time
    ASSERT(policy->costFunction == NULL);

    // Destroy these last in case some of the other destructs make use of them
    maxCount = policy->guidProviderCount;
    for(i = 0; i < maxCount; ++i) {
        policy->guidProviders[i]->fcts.destruct(policy->guidProviders[i]);
    }

    maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fcts.destruct(policy->allocators[i]);
    }

    // Destroy self
    runtimeChunkFree((u64)policy->workers, NULL);
    runtimeChunkFree((u64)policy->commApis, NULL);
    runtimeChunkFree((u64)policy->schedulers, NULL);
    runtimeChunkFree((u64)policy->allocators, NULL);
    runtimeChunkFree((u64)policy->taskFactories, NULL);
    runtimeChunkFree((u64)policy->taskTemplateFactories, NULL);
    runtimeChunkFree((u64)policy->dbFactories, NULL);
    runtimeChunkFree((u64)policy->eventFactories, NULL);
    runtimeChunkFree((u64)policy->guidProviders, NULL);
    runtimeChunkFree((u64)policy, NULL);
}

static void localDeguidify(ocrPolicyDomain_t *self, ocrFatGuid_t *guid) {
    START_PROFILE(pd_hc_localDeguidify);
    ASSERT(self->guidProviderCount == 1);
    if(guid->guid != NULL_GUID && guid->guid != UNINITIALIZED_GUID) {
        if(guid->metaDataPtr == NULL) {
            self->guidProviders[0]->fcts.getVal(self->guidProviders[0], guid->guid,
                                                (u64*)(&(guid->metaDataPtr)), NULL);
        }
    }
    RETURN_PROFILE();
}

// In all these functions, we consider only a single PD. In other words, in CE, we
// deal with everything locally and never send messages

// allocateDatablock:  Utility used by hcAllocateDb and hcMemAlloc, just below.
static void* allocateDatablock (ocrPolicyDomain_t *self,
                                u64                size,
                                u64                prescription,
                                u64               *allocatorIdx) {
    void* result;
    u64 hints = 0; // Allocator hint
    u64 idx;  // Index into the allocators array to select the allocator to try.
    ASSERT (self->allocatorCount > 0);
    do {
        hints = (prescription & 1)?(OCR_ALLOC_HINT_NONE):(OCR_ALLOC_HINT_REDUCE_CONTENTION);
        prescription >>= 1;
        idx = prescription & 7;  // Get the index of the allocator to use.
        prescription >>= 3;
        if ((idx > self->allocatorCount) || (self->allocators[idx] == NULL)) {
            continue;  // Skip this allocator if it doesn't exist.
        }
        result = self->allocators[idx]->fcts.allocate(self->allocators[idx], size, hints);

        if (result) {
            *allocatorIdx = idx;
            return result;
        }
    } while (prescription != 0);
    return NULL;
}

static u8 hcAllocateDb(ocrPolicyDomain_t *self, ocrFatGuid_t *guid, void** ptr, u64 size,
                       u32 properties, ocrFatGuid_t affinity, ocrInDbAllocator_t allocator,
                       u64 prescription) {
    // This function allocates a data block for the requestor, who is either this computing agent or a
    // different one that sent us a message.  After getting that data block, it "guidifies" the results
    // which, by the way, ultimately causes hcMemAlloc (just below) to run.
    //
    // Currently, the "affinity" and "allocator" arguments are ignored, and I expect that these will
    // eventually be eliminated here and instead, above this level, processed into the "prescription"
    // variable, which has been added to this argument list.  The prescription indicates an order in
    // which to attempt to allocate the block to a pool.
    u64 idx;
    void* result = allocateDatablock (self, size, prescription, &idx);
    if (result) {
        ocrDataBlock_t *block = self->dbFactories[0]->instantiate(
            self->dbFactories[0], self->allocators[idx]->fguid, self->fguid,
            size, result, properties, NULL);
        *ptr = result;
        (*guid).guid = block->guid;
        (*guid).metaDataPtr = block;
        return 0;
    } else {
        DPRINTF(DEBUG_LVL_WARN, "hcAllocateDb returning NULL for size %ld\n", (u64) size);
        return OCR_ENOMEM;
    }
}

static u8 hcMemAlloc(ocrPolicyDomain_t *self, ocrFatGuid_t* allocator, u64 size,
                     ocrMemType_t memType, void** ptr, u64 prescription) {
    // Like hcAllocateDb, this function also allocates a data block.  But it does NOT guidify
    // the results.  The main usage of this function is to allocate space for the guid needed
    // by hcAllocateDb; so if this function also guidified its results, you'd be in an infinite
    // guidification loop!
    //
    // The prescription indicates an order in which to attempt to allocate the block to a pool.
    void* result;
    u64 idx;
    ASSERT (memType == GUID_MEMTYPE || memType == DB_MEMTYPE);
    result = allocateDatablock (self, size, prescription, &idx);
    if (result) {
        *ptr = result;
        *allocator = self->allocators[idx]->fguid;
        return 0;
    } else {
        DPRINTF(DEBUG_LVL_WARN, "hcMemAlloc returning NULL for size %ld\n", (u64) size);
        return OCR_ENOMEM;
    }
}

static u8 hcMemUnAlloc(ocrPolicyDomain_t *self, ocrFatGuid_t* allocator,
                       void* ptr, ocrMemType_t memType) {
#if 1
    allocatorFreeFunction(ptr);
    return 0;
#else
    u64 i;
    ASSERT (memType == GUID_MEMTYPE || memType == DB_MEMTYPE);
    if (memType == DB_MEMTYPE) {
        for(i=0; i < self->allocatorCount; ++i) {
            if(self->allocators[i]->fguid.guid == allocator->guid) {
                allocator->metaDataPtr = self->allocators[i]->fguid.metaDataPtr;
                self->allocators[i]->fcts.free(self->allocators[i], ptr);
                return 0;
            }
        }
        return OCR_EINVAL;
    } else if (memType == GUID_MEMTYPE) {
        ASSERT (self->allocatorCount >= 0);
        self->allocators[self->allocatorCount-1]->fcts.free(self->allocators[self->allocatorCount-1], ptr);
        return 0;
    } else {
        ASSERT (false);
    }
    return OCR_EINVAL;
#endif
}

static u8 hcCreateEdt(ocrPolicyDomain_t *self, ocrFatGuid_t *guid,
                      ocrFatGuid_t  edtTemplate, u32 *paramc, u64* paramv,
                      u32 *depc, u32 properties, ocrFatGuid_t affinity,
                      ocrFatGuid_t * outputEvent, ocrTask_t * currentEdt,
                      ocrFatGuid_t parentLatch) {


    ocrTaskTemplate_t *taskTemplate = (ocrTaskTemplate_t*)edtTemplate.metaDataPtr;
    DPRINTF(DEBUG_LVL_VVERB, "Creating EDT with template GUID 0x%lx (0x%lx) (paramc=%d; depc=%d)"
            " and have paramc=%d; depc=%d\n", edtTemplate.guid, edtTemplate.metaDataPtr,
            taskTemplate->paramc, taskTemplate->depc, *paramc, *depc);
    ASSERT(((taskTemplate->paramc == EDT_PARAM_UNK) && *paramc != EDT_PARAM_DEF) ||
           (taskTemplate->paramc != EDT_PARAM_UNK && (*paramc == EDT_PARAM_DEF ||
                   taskTemplate->paramc == *paramc)));
    ASSERT(((taskTemplate->depc == EDT_PARAM_UNK) && *depc != EDT_PARAM_DEF) ||
           (taskTemplate->depc != EDT_PARAM_UNK && (*depc == EDT_PARAM_DEF ||
                   taskTemplate->depc == *depc)));

    if(*paramc == EDT_PARAM_DEF) {
        *paramc = taskTemplate->paramc;
    }
    if(*depc == EDT_PARAM_DEF) {
        *depc = taskTemplate->depc;
    }
    // If paramc are expected, double check paramv is not NULL
    if((*paramc > 0) && (paramv == NULL)) {
        ASSERT(0);
        return OCR_EINVAL;
    }

    ocrTask_t * base = self->taskFactories[0]->instantiate(
                           self->taskFactories[0], edtTemplate, *paramc, paramv,
                           *depc, properties, affinity, outputEvent, currentEdt,
                           parentLatch, NULL);

    (*guid).guid = base->guid;
    (*guid).metaDataPtr = base;
    return 0;
}

static u8 hcCreateEdtTemplate(ocrPolicyDomain_t *self, ocrFatGuid_t *guid,
                              ocrEdt_t func, u32 paramc, u32 depc, const char* funcName) {


    ocrTaskTemplate_t *base = self->taskTemplateFactories[0]->instantiate(
                                  self->taskTemplateFactories[0], func, paramc, depc, funcName, NULL);
    (*guid).guid = base->guid;
    (*guid).metaDataPtr = base;
    return 0;
}

static u8 hcCreateEvent(ocrPolicyDomain_t *self, ocrFatGuid_t *guid,
                        ocrEventTypes_t type, bool takesArg) {

    ocrEvent_t *base = self->eventFactories[0]->instantiate(
                           self->eventFactories[0], type, takesArg, NULL);
    (*guid).guid = base->guid;
    (*guid).metaDataPtr = base;
    return 0;
}

static u8 convertDepAddToSatisfy(ocrPolicyDomain_t *self, ocrFatGuid_t dbGuid,
                                 ocrFatGuid_t destGuid, u32 slot) {

    ocrPolicyMsg_t msg;
    getCurrentEnv(NULL, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DEP_SATISFY
    msg.type = PD_MSG_DEP_SATISFY | PD_MSG_REQUEST;
    PD_MSG_FIELD(guid) = destGuid;
    PD_MSG_FIELD(payload) = dbGuid;
    PD_MSG_FIELD(slot) = slot;
    PD_MSG_FIELD(properties) = 0;
    RESULT_PROPAGATE(self->fcts.processMessage(self, &msg, false));
#undef PD_MSG
#undef PD_TYPE
    return 0;
}
#ifdef OCR_ENABLE_STATISTICS
static ocrStats_t* hcGetStats(ocrPolicyDomain_t *self) {
    return self->statsObject;
}
#endif

static u8 hcGrabPd(ocrPolicyDomainHc_t *rself) {
    START_PROFILE(pd_hc_GrabPd);
    u32 newState = rself->state;
    u32 oldState;
    if((newState & 0xF) == 1) {
        do {
            // Try to grab it
            oldState = newState;
            newState += 16; // Increment the user count by 1, skips the bottom 4 bits
            newState = hal_cmpswap32(&(rself->state), oldState, newState);
            if(newState == oldState) {
                RETURN_PROFILE(0);
            } else {
                if(newState & 0x2) {
                    // The PD is shutting down
                    RETURN_PROFILE(OCR_EAGAIN);
                }
                // Some other thread incremented the reader count so
                // we try again
                ASSERT((newState & 0xF) == 1); // Just to make sure
            }
        } while(true);
    } else {
        RETURN_PROFILE(OCR_EAGAIN);
    }
}

static void hcReleasePd(ocrPolicyDomainHc_t *rself) {
    START_PROFILE(pd_hc_ReleasePd);
    u32 oldState = 0;
    u32 newState = rself->state;
    do {
        ASSERT(newState > 16); // We must at least be a user
        oldState = newState;
        newState -= 16;
        newState = hal_cmpswap32(&(rself->state), oldState, newState);
    } while(newState != oldState);
    RETURN_PROFILE();
}

u8 hcPolicyDomainProcessMessage(ocrPolicyDomain_t *self, ocrPolicyMsg_t *msg, u8 isBlocking) {

    START_PROFILE(pd_hc_ProcessMessage);
    u8 returnCode = 0;

    returnCode = hcGrabPd((ocrPolicyDomainHc_t*)self);
    if(returnCode) {
        RETURN_PROFILE(returnCode);
    }

    // This assert checks the call's parameters are correct
    // - Synchronous processMessage calls always deal with a REQUEST.
    // - Asynchronous message processing allows for certain type of message
    //   to have a RESPONSE processed.
    ASSERT(((msg->type & PD_MSG_REQUEST) && !(msg->type & PD_MSG_RESPONSE))
        || ((msg->type & PD_MSG_RESPONSE) && ((msg->type & PD_MSG_TYPE_ONLY) == PD_MSG_DB_ACQUIRE)))

    switch(msg->type & PD_MSG_TYPE_ONLY) {
    case PD_MSG_DB_CREATE: {
        START_PROFILE(pd_hc_DbCreate);
#define PD_MSG msg
#define PD_TYPE PD_MSG_DB_CREATE
        // TODO: Add properties whether DB needs to be acquired or not
        // This would impact where we do the PD_MSG_MEM_ALLOC for ex
        // For now we deal with both USER and RT dbs the same way
        ASSERT(PD_MSG_FIELD(dbType) == USER_DBTYPE || PD_MSG_FIELD(dbType) == RUNTIME_DBTYPE);
#define PRESCRIPTION 0x10LL
        PD_MSG_FIELD(returnDetail) = hcAllocateDb(self, &(PD_MSG_FIELD(guid)),
                                  &(PD_MSG_FIELD(ptr)), PD_MSG_FIELD(size),
                                  PD_MSG_FIELD(properties),
                                  PD_MSG_FIELD(affinity),
                                  PD_MSG_FIELD(allocator),
                                  PRESCRIPTION);
        if(PD_MSG_FIELD(returnDetail) == 0) {
            ocrDataBlock_t *db = PD_MSG_FIELD(guid.metaDataPtr);
            ASSERT(db);
            // TODO: Check if properties want DB acquired
            ASSERT(db->fctId == self->dbFactories[0]->factoryId);
            PD_MSG_FIELD(returnDetail) = self->dbFactories[0]->fcts.acquire(
                db, &(PD_MSG_FIELD(ptr)), PD_MSG_FIELD(edt), EDT_SLOT_NONE, DB_MODE_ITW, false, (u32) DB_MODE_ITW);
        } else {
            // Cannot acquire
            PD_MSG_FIELD(ptr) = NULL;
        }
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_DB_DESTROY: {
        // Should never ever be called. The user calls free and internally
        // this will call whatever it needs (most likely PD_MSG_MEM_UNALLOC)
        // This would get called when DBs move for example
        ASSERT(0);
        break;
    }

    case PD_MSG_DB_ACQUIRE: {
        START_PROFILE(pd_hc_DbAcquire);
        if (msg->type & PD_MSG_REQUEST) {
        #define PD_MSG msg
        #define PD_TYPE PD_MSG_DB_ACQUIRE
            localDeguidify(self, &(PD_MSG_FIELD(guid)));
            //DIST-TODO rely on the call to set the fatguid ptr to NULL and not crash if edt acquiring is not local
            localDeguidify(self, &(PD_MSG_FIELD(edt)));
            ocrDataBlock_t *db = (ocrDataBlock_t*)(PD_MSG_FIELD(guid.metaDataPtr));
            ASSERT(db->fctId == self->dbFactories[0]->factoryId);
            PD_MSG_FIELD(returnDetail) = self->dbFactories[0]->fcts.acquire(
                db, &(PD_MSG_FIELD(ptr)), PD_MSG_FIELD(edt), PD_MSG_FIELD(edtSlot),
                (ocrDbAccessMode_t) (PD_MSG_FIELD(properties) & (u32)DB_ACCESS_MODE_MASK),
                PD_MSG_FIELD(properties) & DB_PROP_RT_ACQUIRE, PD_MSG_FIELD(properties));
            //DIST-TODO db: modify the acquire call if we agree on changing the api
            PD_MSG_FIELD(size) = db->size;
            // conserve acquire's msg properties and add the DB's one.
            PD_MSG_FIELD(properties) |= db->flags;
            // Acquire message can be asynchronously responded to
            if (PD_MSG_FIELD(returnDetail) == OCR_EBUSY) {
                // Processing not completed
                returnCode = OCR_EPEND;
            } else {
                // Something went wrong in dbAcquire
                ASSERT(PD_MSG_FIELD(returnDetail) == 0);
                msg->type &= ~PD_MSG_REQUEST;
                msg->type |= PD_MSG_RESPONSE;
            }
        #undef PD_MSG
        #undef PD_TYPE
        } else {
            ASSERT(msg->type & PD_MSG_RESPONSE);
            // asynchronous callback on acquire, reading response
        #define PD_MSG msg
        #define PD_TYPE PD_MSG_DB_ACQUIRE
            ocrFatGuid_t edtFGuid = PD_MSG_FIELD(edt);
            ocrFatGuid_t dbFGuid = PD_MSG_FIELD(guid);
            u32 edtSlot = PD_MSG_FIELD(edtSlot);
            localDeguidify(self, &edtFGuid);
            // At this point the edt MUST be local as well as the db data pointer.
            ocrTask_t* task = (ocrTask_t*) edtFGuid.metaDataPtr;
            PD_MSG_FIELD(returnDetail) = self->taskFactories[0]->fcts.dependenceResolved(task, dbFGuid.guid, PD_MSG_FIELD(ptr), edtSlot);
        #undef PD_MSG
        #undef PD_TYPE
        }
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_DB_RELEASE: {
        // Call the appropriate release function
        START_PROFILE(pd_hc_DbRelease);
#define PD_MSG msg
#define PD_TYPE PD_MSG_DB_RELEASE
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        localDeguidify(self, &(PD_MSG_FIELD(edt)));
        ocrDataBlock_t *db = (ocrDataBlock_t*)(PD_MSG_FIELD(guid.metaDataPtr));
        ASSERT(db->fctId == self->dbFactories[0]->factoryId);
        //DIST-TODO db: release is a blocking two-way message to make sure it executed at destination
        PD_MSG_FIELD(returnDetail) = self->dbFactories[0]->fcts.release(
            db, PD_MSG_FIELD(edt), PD_MSG_FIELD(properties) & DB_PROP_RT_ACQUIRE);
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_DB_FREE: {
        // Call the appropriate free function
        START_PROFILE(pd_hc_DbFree);
#define PD_MSG msg
#define PD_TYPE PD_MSG_DB_FREE
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        localDeguidify(self, &(PD_MSG_FIELD(edt)));
        ocrDataBlock_t *db = (ocrDataBlock_t*)(PD_MSG_FIELD(guid.metaDataPtr));
        ASSERT(db->fctId == self->dbFactories[0]->factoryId);
        ASSERT(!(msg->type & PD_MSG_REQ_RESPONSE));
        PD_MSG_FIELD(returnDetail) = self->dbFactories[0]->fcts.free(
            db, PD_MSG_FIELD(edt));
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        // msg->type |= PD_MSG_RESPONSE;
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_MEM_ALLOC: {
        START_PROFILE(pd_hc_MemAlloc);
#define PD_MSG msg
#define PD_TYPE PD_MSG_MEM_ALLOC
        PD_MSG_FIELD(allocatingPD.metaDataPtr) = self;
        PD_MSG_FIELD(returnDetail) = hcMemAlloc(
            self, &(PD_MSG_FIELD(allocator)), PD_MSG_FIELD(size),
            PD_MSG_FIELD(type), &(PD_MSG_FIELD(ptr)), PRESCRIPTION);
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_MEM_UNALLOC: {
        START_PROFILE(pd_hc_MemUnalloc);
#define PD_MSG msg
#define PD_TYPE PD_MSG_MEM_UNALLOC
        PD_MSG_FIELD(allocatingPD.metaDataPtr) = self;
        PD_MSG_FIELD(returnDetail) = hcMemUnAlloc(
            self, &(PD_MSG_FIELD(allocator)), PD_MSG_FIELD(ptr), PD_MSG_FIELD(type));
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_WORK_CREATE: {
        START_PROFILE(pd_hc_WorkCreate);
#define PD_MSG msg
#define PD_TYPE PD_MSG_WORK_CREATE
        localDeguidify(self, &(PD_MSG_FIELD(templateGuid)));
        ASSERT(PD_MSG_FIELD(templateGuid.metaDataPtr) != NULL);
        localDeguidify(self, &(PD_MSG_FIELD(affinity)));
        localDeguidify(self, &(PD_MSG_FIELD(currentEdt)));
        localDeguidify(self, &(PD_MSG_FIELD(parentLatch)));
        ocrFatGuid_t *outputEvent = NULL;
        if(PD_MSG_FIELD(outputEvent.guid) == UNINITIALIZED_GUID) {
            outputEvent = &(PD_MSG_FIELD(outputEvent));
        }
        ASSERT((PD_MSG_FIELD(workType) == EDT_USER_WORKTYPE) || (PD_MSG_FIELD(workType) == EDT_RT_WORKTYPE));
        PD_MSG_FIELD(returnDetail) = hcCreateEdt(
                self, &(PD_MSG_FIELD(guid)), PD_MSG_FIELD(templateGuid),
                &(PD_MSG_FIELD(paramc)), PD_MSG_FIELD(paramv), &(PD_MSG_FIELD(depc)),
                PD_MSG_FIELD(properties), PD_MSG_FIELD(affinity), outputEvent,
                (ocrTask_t*)(PD_MSG_FIELD(currentEdt).metaDataPtr), PD_MSG_FIELD(parentLatch));
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_WORK_EXECUTE: {
        ASSERT(0); // Not used for this PD
        break;
    }

    case PD_MSG_WORK_DESTROY: {
        START_PROFILE(pd_hc_WorkDestroy);
#define PD_MSG msg
#define PD_TYPE PD_MSG_WORK_DESTROY
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        ocrTask_t *task = (ocrTask_t*)PD_MSG_FIELD(guid.metaDataPtr);
        ASSERT(task);
        ASSERT(task->fctId == self->taskFactories[0]->factoryId);
        PD_MSG_FIELD(returnDetail) = self->taskFactories[0]->fcts.destruct(task);
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_EDTTEMP_CREATE: {
        START_PROFILE(pd_hc_EdtTempCreate);
#define PD_MSG msg
#define PD_TYPE PD_MSG_EDTTEMP_CREATE
        PD_MSG_FIELD(returnDetail) = hcCreateEdtTemplate(
            self, &(PD_MSG_FIELD(guid)),
            PD_MSG_FIELD(funcPtr), PD_MSG_FIELD(paramc),
            PD_MSG_FIELD(depc), PD_MSG_FIELD(funcName));

        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_EDTTEMP_DESTROY: {
        START_PROFILE(pd_hc_EdtTempDestroy);
#define PD_MSG msg
#define PD_TYPE PD_MSG_EDTTEMP_DESTROY
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        ocrTaskTemplate_t *tTemplate = (ocrTaskTemplate_t*)(PD_MSG_FIELD(guid.metaDataPtr));
        ASSERT(tTemplate->fctId == self->taskTemplateFactories[0]->factoryId);
        PD_MSG_FIELD(returnDetail) = self->taskTemplateFactories[0]->fcts.destruct(tTemplate);
#undef PD_MSG
#undef PD_TYPE
        msg->type &= (~PD_MSG_REQUEST);
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_EVT_CREATE: {
        START_PROFILE(pd_hc_EvtCreate);
#define PD_MSG msg
#define PD_TYPE PD_MSG_EVT_CREATE
        PD_MSG_FIELD(returnDetail) = hcCreateEvent(
            self, &(PD_MSG_FIELD(guid)),
            PD_MSG_FIELD(type), PD_MSG_FIELD(properties) & 1);
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_EVT_DESTROY: {
        START_PROFILE(pd_hc_EvtDestroy);
#define PD_MSG msg
#define PD_TYPE PD_MSG_EVT_DESTROY
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        ocrEvent_t *evt = (ocrEvent_t*)PD_MSG_FIELD(guid.metaDataPtr);
        ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
        PD_MSG_FIELD(returnDetail) = self->eventFactories[0]->fcts[evt->kind].destruct(evt);
#undef PD_MSG
#undef PD_TYPE
        msg->type &= (~PD_MSG_REQUEST);
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_EVT_GET: {
        START_PROFILE(pd_hc_EvtGet);
#define PD_MSG msg
#define PD_TYPE PD_MSG_EVT_GET
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        ocrEvent_t *evt = (ocrEvent_t*)PD_MSG_FIELD(guid.metaDataPtr);
        ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
        PD_MSG_FIELD(data) = self->eventFactories[0]->fcts[evt->kind].get(evt);
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_GUID_CREATE: {
        START_PROFILE(pd_hc_GuidCreate);
#define PD_MSG msg
#define PD_TYPE PD_MSG_GUID_CREATE
        if(PD_MSG_FIELD(size) != 0) {
            // Here we need to create a metadata area as well
            PD_MSG_FIELD(returnDetail) = self->guidProviders[0]->fcts.createGuid(
                self->guidProviders[0], &(PD_MSG_FIELD(guid)), PD_MSG_FIELD(size),
                PD_MSG_FIELD(kind));
        } else {
            // Here we just need to associate a GUID
            ocrGuid_t temp;
            PD_MSG_FIELD(returnDetail) = self->guidProviders[0]->fcts.getGuid(
                self->guidProviders[0], &temp, (u64)PD_MSG_FIELD(guid.metaDataPtr),
                PD_MSG_FIELD(kind));
            PD_MSG_FIELD(guid.guid) = temp;
        }
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_GUID_INFO: {
        START_PROFILE(pd_hc_GuidInfo);
#define PD_MSG msg
#define PD_TYPE PD_MSG_GUID_INFO
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        if(PD_MSG_FIELD(properties) & KIND_GUIDPROP) {
            PD_MSG_FIELD(returnDetail) = self->guidProviders[0]->fcts.getKind(
                self->guidProviders[0], PD_MSG_FIELD(guid.guid), &(PD_MSG_FIELD(kind)));
            if(PD_MSG_FIELD(returnDetail) == 0)
                PD_MSG_FIELD(returnDetail) = KIND_GUIDPROP
                    | WMETA_GUIDPROP | RMETA_GUIDPROP;
        } else if (PD_MSG_FIELD(properties) & LOCATION_GUIDPROP) {
            PD_MSG_FIELD(returnDetail) = self->guidProviders[0]->fcts.getLocation(
                self->guidProviders[0], PD_MSG_FIELD(guid.guid), &(PD_MSG_FIELD(location)));
            if(PD_MSG_FIELD(returnDetail) == 0)
                PD_MSG_FIELD(returnDetail) = LOCATION_GUIDPROP
                    | WMETA_GUIDPROP | RMETA_GUIDPROP;
        } else {
            PD_MSG_FIELD(returnDetail) = WMETA_GUIDPROP | RMETA_GUIDPROP;
        }

#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_GUID_METADATA_CLONE:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_GUID_METADATA_CLONE
        ocrFatGuid_t fatGuid = PD_MSG_FIELD(guid);
        ocrGuidKind kind = OCR_GUID_NONE;
        guidKind(self, fatGuid, &kind);
        //IMPL: For now only support edt template cloning

        switch(kind) {
            case OCR_GUID_EDT_TEMPLATE:
                localDeguidify(self, &(PD_MSG_FIELD(guid)));
                //These won't support flat serialization
#ifdef OCR_ENABLE_STATISTICS
                ASSERT(false && "no statistics support in distributed edt templates");
#endif
#ifdef OCR_ENABLE_EDT_NAMING
                ASSERT(false && "no serialization of edt template string");
#endif
                PD_MSG_FIELD(size) = sizeof(ocrTaskTemplateHc_t);
                break;
            case OCR_GUID_AFFINITY:
                localDeguidify(self, &(PD_MSG_FIELD(guid)));
                PD_MSG_FIELD(size) = sizeof(ocrAffinity_t);
                break;
            default:
                ASSERT(false && "Unsupported GUID kind cloning");
        }
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
        break;
    }

    case PD_MSG_GUID_DESTROY: {
        START_PROFILE(pd_hc_GuidDestroy);
#define PD_MSG msg
#define PD_TYPE PD_MSG_GUID_DESTROY
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        PD_MSG_FIELD(returnDetail) = self->guidProviders[0]->fcts.releaseGuid(
            self->guidProviders[0], PD_MSG_FIELD(guid), PD_MSG_FIELD(properties) & 1);
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_COMM_TAKE: {
        START_PROFILE(pd_hc_Take);
#define PD_MSG msg
#define PD_TYPE PD_MSG_COMM_TAKE
        if (PD_MSG_FIELD(type) == OCR_GUID_EDT) {
            PD_MSG_FIELD(returnDetail) = self->schedulers[0]->fcts.takeEdt(
                self->schedulers[0], &(PD_MSG_FIELD(guidCount)),
                PD_MSG_FIELD(guids));
            // For now, we return the execute function for EDTs
            PD_MSG_FIELD(extra) = (u64)(self->taskFactories[0]->fcts.execute);
            // We also consider that the task to be executed is local so we
            // return it's fully deguidified value (TODO: this may need revising)
            u64 i = 0, maxCount = PD_MSG_FIELD(guidCount);
            for( ; i < maxCount; ++i) {
                localDeguidify(self, &(PD_MSG_FIELD(guids)[i]));
            }
        } else {
            ASSERT(PD_MSG_FIELD(type) == OCR_GUID_COMM);
            PD_MSG_FIELD(returnDetail) = self->schedulers[0]->fcts.takeComm(
                self->schedulers[0], &(PD_MSG_FIELD(guidCount)),
                PD_MSG_FIELD(guids), PD_MSG_FIELD(properties));
        }
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_COMM_GIVE: {
        START_PROFILE(pd_hc_Give);
#define PD_MSG msg
#define PD_TYPE PD_MSG_COMM_GIVE
        if (PD_MSG_FIELD(type) == OCR_GUID_EDT) {
            PD_MSG_FIELD(returnDetail) = self->schedulers[0]->fcts.giveEdt(
                self->schedulers[0], &(PD_MSG_FIELD(guidCount)),
                PD_MSG_FIELD(guids));
        } else {
            ASSERT(PD_MSG_FIELD(type) == OCR_GUID_COMM);
            PD_MSG_FIELD(returnDetail) = self->schedulers[0]->fcts.giveComm(
                self->schedulers[0], &(PD_MSG_FIELD(guidCount)),
                PD_MSG_FIELD(guids), PD_MSG_FIELD(properties));
        }
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
        EXIT_PROFILE;
        break;
    }


    case PD_MSG_DEP_ADD: {
        START_PROFILE(pd_hc_AddDep);
#define PD_MSG msg
#define PD_TYPE PD_MSG_DEP_ADD
        // We first get information about the source and destination
        ocrGuidKind srcKind, dstKind;
        //NOTE: In distributed the metaDataPtr is set to NULL_GUID since
        //the guid provider doesn't fetch remote metaDataPtr yet. It's ok
        //(but fragile) because the HC event/task does not try to use it
        //Querying the kind through the PD's interface should be ok as it's
        //the problem of the guid provider to give this information
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(source.guid),
            (u64*)(&(PD_MSG_FIELD(source.metaDataPtr))), &srcKind);
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(dest.guid),
            (u64*)(&(PD_MSG_FIELD(dest.metaDataPtr))), &dstKind);

        ocrFatGuid_t src = PD_MSG_FIELD(source);
        ocrFatGuid_t dest = PD_MSG_FIELD(dest);
        ocrDbAccessMode_t mode = (PD_MSG_FIELD(properties) & DB_PROP_MODE_MASK); //lower bits is the mode //TODO not pretty

        if (srcKind == NULL_GUID) {
            //NOTE: Handle 'NULL_GUID' case here to be safe although
            //we've already caught it in ocrAddDependence for performance
            // This is equivalent to an immediate satisfy
            PD_MSG_FIELD(returnDetail) = convertDepAddToSatisfy(
                self, src, dest, PD_MSG_FIELD(slot));
        } else if (srcKind == OCR_GUID_DB) {
            if (dstKind & OCR_GUID_EVENT) {
                PD_MSG_FIELD(returnDetail) = convertDepAddToSatisfy(
                    self, src, dest, PD_MSG_FIELD(slot));
            } else {
                // NOTE: We could use convertDepAddToSatisfy since adding a DB dependence
                // is equivalent to satisfy. However, we want to go through the register
                // function to make sure the access mode is recorded.
                ASSERT(dstKind == OCR_GUID_EDT);
                ocrPolicyMsg_t registerMsg;
                getCurrentEnv(NULL, NULL, NULL, &registerMsg);
                ocrFatGuid_t sourceGuid = PD_MSG_FIELD(source);
                ocrFatGuid_t destGuid = PD_MSG_FIELD(dest);
                u32 slot = PD_MSG_FIELD(slot);
            #undef PD_MSG
            #undef PD_TYPE
            #define PD_MSG (&registerMsg)
            #define PD_TYPE PD_MSG_DEP_REGSIGNALER
                registerMsg.type = PD_MSG_DEP_REGSIGNALER | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
                // Registers sourceGuid (signaler) onto destGuid
                PD_MSG_FIELD(signaler) = sourceGuid;
                PD_MSG_FIELD(dest) = destGuid;
                PD_MSG_FIELD(slot) = slot;
                PD_MSG_FIELD(mode) = mode;
                PD_MSG_FIELD(properties) = true; // Specify context is add-dependence
                RESULT_PROPAGATE(self->fcts.processMessage(self, &registerMsg, true));
            #undef PD_MSG
            #undef PD_TYPE
            #define PD_MSG msg
            #define PD_TYPE PD_MSG_DEP_ADD
            }
        } else {
            // Only left with events as potential source
            ASSERT(srcKind & OCR_GUID_EVENT);
            //OK if srcKind is at current location
            u8 needSignalerReg = 0;
            ocrPolicyMsg_t registerMsg;
            getCurrentEnv(NULL, NULL, NULL, &registerMsg);
            ocrFatGuid_t sourceGuid = PD_MSG_FIELD(source);
            ocrFatGuid_t destGuid = PD_MSG_FIELD(dest);
            u32 slot = PD_MSG_FIELD(slot);
        #undef PD_MSG
        #undef PD_TYPE
        #define PD_MSG (&registerMsg)
        #define PD_TYPE PD_MSG_DEP_REGWAITER
            // Requires response to determine if we need to register signaler too
            registerMsg.type = PD_MSG_DEP_REGWAITER | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
            // Registers destGuid (waiter) onto sourceGuid
            PD_MSG_FIELD(waiter) = destGuid;
            PD_MSG_FIELD(dest) = sourceGuid;
            PD_MSG_FIELD(slot) = slot;
            PD_MSG_FIELD(properties) = true; // Specify context is add-dependence
            RESULT_PROPAGATE(self->fcts.processMessage(self, &registerMsg, true));
            needSignalerReg = PD_MSG_FIELD(returnDetail);
        #undef PD_MSG
        #undef PD_TYPE
        #define PD_MSG msg
        #define PD_TYPE PD_MSG_DEP_ADD
            PD_MSG_FIELD(properties) = needSignalerReg;
            //PERF: property returned by registerWaiter allows to decide
            // whether or not a registerSignaler call is needed.
            //TODO this is not done yet so some calls are pure waste
            if(!PD_MSG_FIELD(properties)) {
                // Cannot have other types of destinations
                ASSERT((dstKind == OCR_GUID_EDT) || (dstKind & OCR_GUID_EVENT));
                ocrPolicyMsg_t registerMsg;
                getCurrentEnv(NULL, NULL, NULL, &registerMsg);
                ocrFatGuid_t sourceGuid = PD_MSG_FIELD(source);
                ocrFatGuid_t destGuid = PD_MSG_FIELD(dest);
                u32 slot = PD_MSG_FIELD(slot);
            #undef PD_MSG
            #undef PD_TYPE
            #define PD_MSG (&registerMsg)
            #define PD_TYPE PD_MSG_DEP_REGSIGNALER
                registerMsg.type = PD_MSG_DEP_REGSIGNALER | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
                // Registers sourceGuid (signaler) onto destGuid
                PD_MSG_FIELD(signaler) = sourceGuid;
                PD_MSG_FIELD(dest) = destGuid;
                PD_MSG_FIELD(slot) = slot;
                PD_MSG_FIELD(mode) = mode;
                PD_MSG_FIELD(properties) = true; // Specify context is add-dependence
                RESULT_PROPAGATE(self->fcts.processMessage(self, &registerMsg, true));
            #undef PD_MSG
            #undef PD_TYPE
            #define PD_MSG msg
            #define PD_TYPE PD_MSG_DEP_ADD
           }
        }

#ifdef OCR_ENABLE_STATISTICS
        // TODO: Fixme
        statsDEP_ADD(pd, getCurrentEDT(), NULL, signalerGuid, waiterGuid, NULL, slot);
#endif
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_DEP_REGSIGNALER: {
        START_PROFILE(pd_hc_RegSignaler);
#define PD_MSG msg
#define PD_TYPE PD_MSG_DEP_REGSIGNALER
        // We first get information about the signaler and destination
        ocrGuidKind signalerKind, dstKind;
        //NOTE: In distributed the metaDataPtr is set to NULL_GUID since
        //the guid provider doesn't fetch remote metaDataPtr yet. It's ok
        //(but fragile) because the HC event/task does not try to use it
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(signaler.guid),
            (u64*)(&(PD_MSG_FIELD(signaler.metaDataPtr))), &signalerKind);
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(dest.guid),
            (u64*)(&(PD_MSG_FIELD(dest.metaDataPtr))), &dstKind);

        ocrFatGuid_t signaler = PD_MSG_FIELD(signaler);
        ocrFatGuid_t dest = PD_MSG_FIELD(dest);
        bool isAddDep = PD_MSG_FIELD(properties);

        if (dstKind & OCR_GUID_EVENT) {
            ocrEvent_t *evt = (ocrEvent_t*)(dest.metaDataPtr);
            ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
            PD_MSG_FIELD(returnDetail) = self->eventFactories[0]->fcts[evt->kind].registerSignaler(
                evt, signaler, PD_MSG_FIELD(slot), PD_MSG_FIELD(mode), isAddDep);
        } else if (dstKind == OCR_GUID_EDT) {
            ocrTask_t *edt = (ocrTask_t*)(dest.metaDataPtr);
            ASSERT(edt->fctId == self->taskFactories[0]->factoryId);
            PD_MSG_FIELD(returnDetail) = self->taskFactories[0]->fcts.registerSignaler(
                edt, signaler, PD_MSG_FIELD(slot), PD_MSG_FIELD(mode), isAddDep);
        } else {
            ASSERT(0); // No other things we can register signalers on
        }
#ifdef OCR_ENABLE_STATISTICS
        // TODO: Fixme
        statsDEP_ADD(pd, getCurrentEDT(), NULL, signalerGuid, waiterGuid, NULL, slot);
#endif
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_DEP_REGWAITER: {
        START_PROFILE(pd_hc_RegWaiter);
#define PD_MSG msg
#define PD_TYPE PD_MSG_DEP_REGWAITER
        // We first get information about the signaler and destination
        ocrGuidKind dstKind;
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(dest.guid),
            (u64*)(&(PD_MSG_FIELD(dest.metaDataPtr))), &dstKind);
        ocrFatGuid_t waiter = PD_MSG_FIELD(waiter);
        ocrFatGuid_t dest = PD_MSG_FIELD(dest);
        bool isAddDep = PD_MSG_FIELD(properties);
        if (dstKind & OCR_GUID_EVENT) {
            ocrEvent_t *evt = (ocrEvent_t*)(dest.metaDataPtr);
            ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
            PD_MSG_FIELD(returnDetail) = self->eventFactories[0]->fcts[evt->kind].registerWaiter(
                evt, waiter, PD_MSG_FIELD(slot), isAddDep);
        } else {
            ASSERT(dstKind & OCR_GUID_DB);
            // When an EDT want to register to a DB, for instance to get EW access.
            ocrDataBlock_t *db = (ocrDataBlock_t*)(dest.metaDataPtr);
            ASSERT(db->fctId == self->dbFactories[0]->factoryId);
            PD_MSG_FIELD(returnDetail) = self->dbFactories[0]->fcts.registerWaiter(
                db, waiter, PD_MSG_FIELD(slot), isAddDep);
        }
#ifdef OCR_ENABLE_STATISTICS
        // TODO: Fixme
        statsDEP_ADD(pd, getCurrentEDT(), NULL, signalerGuid, waiterGuid, NULL, slot);
#endif
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
        EXIT_PROFILE;
        break;
    }


    case PD_MSG_DEP_SATISFY: {
        START_PROFILE(pd_hc_Satisfy);
#define PD_MSG msg
#define PD_TYPE PD_MSG_DEP_SATISFY
        // make sure this is one-way
        ASSERT(!(msg->type & PD_MSG_REQ_RESPONSE));
        ocrGuidKind dstKind;
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(guid.guid),
            (u64*)(&(PD_MSG_FIELD(guid.metaDataPtr))), &dstKind);

        ocrFatGuid_t dst = PD_MSG_FIELD(guid);
        if(dstKind & OCR_GUID_EVENT) {
            ocrEvent_t *evt = (ocrEvent_t*)(dst.metaDataPtr);
            ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
            PD_MSG_FIELD(returnDetail) = self->eventFactories[0]->fcts[evt->kind].satisfy(
                evt, PD_MSG_FIELD(payload), PD_MSG_FIELD(slot));
        } else {
            if(dstKind == OCR_GUID_EDT) {
                ocrTask_t *edt = (ocrTask_t*)(dst.metaDataPtr);
                ASSERT(edt->fctId == self->taskFactories[0]->factoryId);
                PD_MSG_FIELD(returnDetail) = self->taskFactories[0]->fcts.satisfy(
                    edt, PD_MSG_FIELD(payload), PD_MSG_FIELD(slot));
            } else {
                ASSERT(0); // We can't satisfy anything else
            }
        }
#ifdef OCR_ENABLE_STATISTICS
        // TODO: Fixme
        statsDEP_ADD(pd, getCurrentEDT(), NULL, signalerGuid, waiterGuid, NULL, slot);
#endif
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_DEP_UNREGSIGNALER: {
        // Never used for now
        ASSERT(0);
        break;
    }

    case PD_MSG_DEP_UNREGWAITER: {
        // Never used for now
        ASSERT(0);
        break;
    }

    case PD_MSG_MGT_MONITOR_PROGRESS: {
    #define PD_MSG (msg)
    #define PD_TYPE PD_MSG_MGT_MONITOR_PROGRESS
      // Delegate to scheduler
      PD_MSG_FIELD(properties) = self->schedulers[0]->fcts.monitorProgress(self->schedulers[0],
                                  (ocrMonitorProgress_t) PD_MSG_FIELD(properties) & 0xFF, PD_MSG_FIELD(monitoree));
    #undef PD_MSG
    #undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
      break;
    }

    case PD_MSG_DEP_DYNADD: {
        START_PROFILE(pd_hc_DynAdd);
        ocrTask_t *curTask = NULL;
        getCurrentEnv(NULL, NULL, &curTask, NULL);
#define PD_MSG msg
#define PD_TYPE PD_MSG_DEP_DYNADD
        // Check to make sure that the EDT is only doing this to
        // itself
        // Also, this should only happen when there is an actual EDT
        ASSERT(curTask &&
               curTask->guid == PD_MSG_FIELD(edt.guid));

        ASSERT(curTask->fctId == self->taskFactories[0]->factoryId);
        PD_MSG_FIELD(returnDetail) = self->taskFactories[0]->fcts.notifyDbAcquire(curTask, PD_MSG_FIELD(db));
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        // msg->type |= PD_MSG_RESPONSE;
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_DEP_DYNREMOVE: {
        START_PROFILE(pd_hc_DynRemove);
        ocrTask_t *curTask = NULL;
        getCurrentEnv(NULL, NULL, &curTask, NULL);
#define PD_MSG msg
#define PD_TYPE PD_MSG_DEP_DYNREMOVE
        // Check to make sure that the EDT is only doing this to
        // itself
        // Also, this should only happen when there is an actual EDT
        ASSERT(curTask &&
               curTask->guid == PD_MSG_FIELD(edt.guid));

        ASSERT(curTask->fctId == self->taskFactories[0]->factoryId);
        PD_MSG_FIELD(returnDetail) = self->taskFactories[0]->fcts.notifyDbRelease(curTask, PD_MSG_FIELD(db));
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        // msg->type |= PD_MSG_RESPONSE;
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_SAL_PRINT: {
        ASSERT(0);
        break;
    }

    case PD_MSG_SAL_READ: {
        ASSERT(0);
        break;
    }

    case PD_MSG_SAL_WRITE: {
        ASSERT(0);
        break;
    }

    case PD_MSG_SAL_TERMINATE: {
        ASSERT(0);
        break;
    }
    case PD_MSG_MGT_SHUTDOWN: {
        START_PROFILE(pd_hc_Shutdown);
        msg->type |= PD_MSG_REQ_RESPONSE; // HC needs this.
#define PD_MSG msg
#define PD_TYPE PD_MSG_MGT_SHUTDOWN
        if (self->shutdownCode == 0)
            self->shutdownCode = PD_MSG_FIELD(errorCode);
        ASSERT(msg->type & PD_MSG_REQ_RESPONSE);
        self->fcts.stop(self);
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_MGT_FINISH: {
        START_PROFILE(pd_hc_Finish);
        self->fcts.finish(self);
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |= PD_MSG_RESPONSE;
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_MGT_REGISTER: {
        // Only one PD at this time
        ASSERT(0);
        break;
    }

    case PD_MSG_MGT_UNREGISTER: {
        // Only one PD at this time
        ASSERT(0);
        break;
    }

    default:
        // Not handled
        ASSERT(0);
    }

    if ((msg->type & PD_MSG_RESPONSE) && (msg->type & PD_MSG_REQ_RESPONSE)) {
        // response is issued:
        // flip required response bit
        msg->type &= ~PD_MSG_REQ_RESPONSE;
	    // flip src and dest locations
        ocrLocation_t src = msg->srcLocation;
        msg->srcLocation = msg->destLocation;
        msg->destLocation = src;
    } // when (!PD_MSG_REQ_RESPONSE) we were processing an asynchronous processMessage's RESPONSE

    // This code is not needed but just shows how things would be handled (probably
    // done by sub-functions)
    if(isBlocking && (msg->type & PD_MSG_REQ_RESPONSE)) {
        ASSERT(msg->type & PD_MSG_RESPONSE); // If we were blocking and needed a response
        // we need to make sure there is one
    }

    // Now "release" the policy domain
    hcReleasePd((ocrPolicyDomainHc_t*)self);

    RETURN_PROFILE(returnCode);
}

u8 hcPdSendMessage(ocrPolicyDomain_t* self, ocrLocation_t target, ocrPolicyMsg_t *message,
                   ocrMsgHandle_t **handle, u32 properties) {
    ASSERT(0);
    return 0;
}

u8 hcPdPollMessage(ocrPolicyDomain_t *self, ocrMsgHandle_t **handle) {
    ASSERT(0);
    return 0;
}

u8 hcPdWaitMessage(ocrPolicyDomain_t *self,  ocrMsgHandle_t **handle) {
    ASSERT(0);
    return 0;
}

void* hcPdMalloc(ocrPolicyDomain_t *self, u64 size) {
    START_PROFILE(pd_hc_PdMalloc);
    if(hcGrabPd((ocrPolicyDomainHc_t*)self))
        RETURN_PROFILE(NULL);

    // Just try in the first allocator
    void* toReturn = NULL;
    toReturn = self->allocators[0]->fcts.allocate(self->allocators[0], size, 0);
    ASSERT(toReturn != NULL);
    hcReleasePd((ocrPolicyDomainHc_t*)self);
    RETURN_PROFILE(toReturn);
}

void hcPdFree(ocrPolicyDomain_t *self, void* addr) {
    START_PROFILE(pd_hc_PdFree);
    // May result in leaks but better than the alternative...
    if(hcGrabPd((ocrPolicyDomainHc_t*)self))
        RETURN_PROFILE();

    allocatorFreeFunction(addr);

    hcReleasePd((ocrPolicyDomainHc_t*)self);
    RETURN_PROFILE();
}

ocrPolicyDomain_t * newPolicyDomainHc(ocrPolicyDomainFactory_t * factory,
#ifdef OCR_ENABLE_STATISTICS
                                      ocrStats_t *statsObject,
#endif
                                      ocrCost_t *costFunction, ocrParamList_t *perInstance) {

    ocrPolicyDomainHc_t * derived = (ocrPolicyDomainHc_t *) runtimeChunkAlloc(sizeof(ocrPolicyDomainHc_t), PERSISTENT_CHUNK);
    ocrPolicyDomain_t * base = (ocrPolicyDomain_t *) derived;
    ASSERT(base);
#ifdef OCR_ENABLE_STATISTICS
    factory->initialize(factory, base, statsObject, costFunction, perInstance);
#else
    factory->initialize(factory, base, costFunction, perInstance);
#endif

    return base;
}

void initializePolicyDomainHc(ocrPolicyDomainFactory_t * factory, ocrPolicyDomain_t* self,
#ifdef OCR_ENABLE_STATISTICS
                              ocrStats_t *statsObject,
#endif
                              ocrCost_t *costFunction, ocrParamList_t *perInstance) {
#ifdef OCR_ENABLE_STATISTICS
    self->statsObject = statsObject;
#endif

    initializePolicyDomainOcr(factory, self, perInstance);

    ocrPolicyDomainHc_t* derived = (ocrPolicyDomainHc_t*) self;
    //DIST-TODO ((paramListPolicyDomainHcInst_t*)perInstance)->rank;
    derived->rank = ((paramListPolicyDomainHcInst_t*)perInstance)->rank;
    derived->state = 0;
}

static void destructPolicyDomainFactoryHc(ocrPolicyDomainFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrPolicyDomainFactory_t * newPolicyDomainFactoryHc(ocrParamList_t *perType) {
    ocrPolicyDomainFactory_t* base = (ocrPolicyDomainFactory_t*) runtimeChunkAlloc(sizeof(ocrPolicyDomainFactoryHc_t), NONPERSISTENT_CHUNK);
    // Set factory's methods
#ifdef OCR_ENABLE_STATISTICS
    base->instantiate = FUNC_ADDR(ocrPolicyDomain_t*(*)(ocrPolicyDomainFactory_t*,ocrStats_t*,
                                  ocrCost_t *,ocrParamList_t*), newPolicyDomainHc);
    base->initialize = FUNC_ADDR(void(*)(ocrPolicyDomainFactory_t*,ocrPolicyDomain_t*,
                                         ocrStats_t*,ocrCost_t *,ocrParamList_t*), initializePolicyDomainHc);
#endif

    base->instantiate = &newPolicyDomainHc;
    base->initialize = &initializePolicyDomainHc;
    base->destruct = &destructPolicyDomainFactoryHc;

    // Set future PDs' instance  methods
    base->policyDomainFcts.destruct = FUNC_ADDR(void(*)(ocrPolicyDomain_t*), hcPolicyDomainDestruct);
    base->policyDomainFcts.begin = FUNC_ADDR(void(*)(ocrPolicyDomain_t*), hcPolicyDomainBegin);
    base->policyDomainFcts.start = FUNC_ADDR(void(*)(ocrPolicyDomain_t*), hcPolicyDomainStart);
    base->policyDomainFcts.stop = FUNC_ADDR(void(*)(ocrPolicyDomain_t*), hcPolicyDomainStop);
    base->policyDomainFcts.finish = FUNC_ADDR(void(*)(ocrPolicyDomain_t*), hcPolicyDomainFinish);
    base->policyDomainFcts.processMessage = FUNC_ADDR(u8(*)(ocrPolicyDomain_t*,ocrPolicyMsg_t*,u8), hcPolicyDomainProcessMessage);

    base->policyDomainFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrPolicyDomain_t*, ocrLocation_t, ocrPolicyMsg_t *, ocrMsgHandle_t**, u32),
                                         hcPdSendMessage);
    base->policyDomainFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrPolicyDomain_t*, ocrMsgHandle_t**), hcPdPollMessage);
    base->policyDomainFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrPolicyDomain_t*, ocrMsgHandle_t**), hcPdWaitMessage);

    base->policyDomainFcts.pdMalloc = FUNC_ADDR(void*(*)(ocrPolicyDomain_t*,u64), hcPdMalloc);
    base->policyDomainFcts.pdFree = FUNC_ADDR(void(*)(ocrPolicyDomain_t*,void*), hcPdFree);
#ifdef OCR_ENABLE_STATISTICS
    base->policyDomainFcts.getStats = FUNC_ADDR(ocrStats_t*(*)(ocrPolicyDomain_t*),hcGetStats);
#endif

    return base;
}

#endif /* ENABLE_POLICY_DOMAIN_HC */
