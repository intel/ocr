/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "ocr-config.h"
#ifdef ENABLE_POLICY_DOMAIN_CE

#include "debug.h"
#include "ocr-errors.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

#include "policy-domain/ce/ce-policy.h"
#include "allocator/allocator-all.h"

#define DEBUG_TYPE POLICY

#define ENGINE_INDEX_OF_CE (8)

void cePolicyDomainBegin(ocrPolicyDomain_t * policy) {
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
}

void cePolicyDomainStart(ocrPolicyDomain_t * policy) {
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

    // REC: Moved all workers to start here.
    // Note: it's important to first logically start all workers.
    // Once they are all up, start the runtime.
    // Workers should start the underlying target and platforms
    maxCount = policy->workerCount;
    for(i = 0; i < maxCount; i++) {
        policy->workers[i]->fcts.start(policy->workers[i], policy);
    }
}

void cePolicyDomainFinish(ocrPolicyDomain_t * policy) {
    // Finish everything in reverse order
    u64 i = 0;
    u64 maxCount = 0;

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

void cePolicyDomainStop(ocrPolicyDomain_t * policy) {

    // Finish everything in reverse order
    // In CE, we MUST call stop on the master worker first.
    // The master worker enters its work routine loop and will
    // be unlocked by ocrShutdown
    u64 i = 0;
    u64 maxCount = 0;

    // Note: As soon as worker '0' is stopped; its thread is
    // free to fall-through and continue shutting down the
    // policy domain
    maxCount = policy->workerCount;
    for(i = 0; i < maxCount; i++) {
        policy->workers[i]->fcts.stop(policy->workers[i]);
    }
    // WARNING: Do not add code here unless you know what you're doing !!
    // If we are here, it means an EDT called ocrShutdown which
    // logically finished workers and can make thread '0' executes this
    // code before joining the other threads.

    // Thread '0' joins the other (N-1) threads.

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
}

void cePolicyDomainDestruct(ocrPolicyDomain_t * policy) {
    // Destroying instances
    u64 i = 0;
    u64 maxCount = 0;

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

    maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fcts.destruct(policy->allocators[i]);
    }

    // Simple ce policies don't have neighbors
    ASSERT(policy->neighbors == NULL);

    // Destruct factories
    maxCount = policy->taskFactoryCount;
    for(i = 0; i < maxCount; ++i) {
        policy->taskFactories[i]->destruct(policy->taskFactories[i]);
    }

    maxCount = policy->taskTemplateFactoryCount;
    for(i = 0; i < maxCount; ++i) {
        policy->taskTemplateFactories[i]->destruct(policy->taskTemplateFactories[i]);
    }

    maxCount = policy->dbFactoryCount;
    for(i = 0; i < maxCount; ++i) {
        policy->dbFactories[i]->destruct(policy->dbFactories[i]);
    }

    maxCount = policy->eventFactoryCount;
    for(i = 0; i < maxCount; ++i) {
        policy->eventFactories[i]->destruct(policy->eventFactories[i]);
    }

    //Anticipate those to be null-impl for some time
    ASSERT(policy->costFunction == NULL);

    // Destroy these last in case some of the other destructs make use of them
    maxCount = policy->guidProviderCount;
    for(i = 0; i < maxCount; ++i) {
        policy->guidProviders[i]->fcts.destruct(policy->guidProviders[i]);
    }

    // Destroy self
    runtimeChunkFree((u64)policy->workers, NULL);
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
    ASSERT(self->guidProviderCount == 1);
    if(guid->guid != NULL_GUID && guid->guid != UNINITIALIZED_GUID) {
        if(guid->metaDataPtr == NULL) {
            self->guidProviders[0]->fcts.getVal(self->guidProviders[0], guid->guid,
                                                (u64*)(&(guid->metaDataPtr)), NULL);
        }
    }
}

static u8 getEngineIndex(ocrPolicyDomain_t *self, ocrLocation_t location) {

    if(location == self->myLocation) {
        return ENGINE_INDEX_OF_CE; // Get the old behavior back
    }
    // This is an XE
    ASSERT(location < ENGINE_INDEX_OF_CE);
    return location;
}

// In all these functions, we consider only a single PD. In other words, in CE, we
// deal with everything locally and never send messages

// allocateDatablock:  Utility used by ceAllocateDb and ceMemAlloc, just below.
static void* allocateDatablock (ocrPolicyDomain_t *self,
                                u64                size,
                                u64                engineIndex,
                                u64                prescription,
                                u64               *allocatorIdx) {
    void* result;
    u64 hints = 0; // Allocator hint
    u64 idx;  // Index into the allocators array to select the allocator to try.
    ASSERT (self->allocatorCount > 0);

    do {
        hints = (prescription & 1)?(OCR_ALLOC_HINT_NONE):(OCR_ALLOC_HINT_REDUCE_CONTENTION);
        // TODO: REC HACK. This code assumes that the CE has all allocators which is
        // not the case for FSim right now. If only one allocator, ignore
        // prescription
        // In general, we should probably return something if we can whatever
        // the prescription says...
        if(self->allocatorCount == 1) {
            result = self->allocators[0]->fcts.allocate(self->allocators[0], size, hints);
            if(result) {
                *allocatorIdx = 0;
                return result;
            } else {
                prescription >>= 4; // Move to the next setting
                continue;   // This allows us to fail allocating to a slice pool (which is probably always the case with L1 since
                            // it doesn't make much sense to slice it), but still try the remnant pool.
            }
        }

        prescription >>= 1;
        idx = prescription & 7;  // Get the index of the allocator to use.

        prescription >>= 3;
        if (idx == 0) {
            idx = engineIndex;   // Zero selects an L1.  Which?  The one belonging to the requesting engine.
        } else {
            idx += ENGINE_INDEX_OF_CE;  // Non-zero selects something higher than L1.  Adjust index to get past ALL the L1s in the Block.
        }

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

static u8 ceAllocateDb(ocrPolicyDomain_t *self, ocrFatGuid_t *guid, void** ptr, u64 size,
                       u32 properties, u64 engineIndex,
                       ocrFatGuid_t affinity, ocrInDbAllocator_t allocator,
                       u64 prescription) {
    // This function allocates a data block for the requestor, who is either this computing agent or a
    // different one that sent us a message.  After getting that data block, it "guidifies" the results
    // which, by the way, ultimately causes ceMemAlloc (just below) to run.
    //
    // Currently, the "affinity" and "allocator" arguments are ignored, and I expect that these will
    // eventually be eliminated here and instead, above this level, processed into the "prescription"
    // variable, which has been added to this argument list.  The prescription indicates an order in
    // which to attempt to allocate the block to a pool.
    u64 idx;
    void* result = allocateDatablock (self, size, engineIndex, prescription, &idx);
    if (result) {
        ocrDataBlock_t *block = self->dbFactories[0]->instantiate(
            self->dbFactories[0], self->allocators[idx]->fguid, self->fguid,
            size, result, properties, NULL);
        *ptr = result;
        (*guid).guid = block->guid;
        (*guid).metaDataPtr = block;
        return 0;
    } else {
        DPRINTF(DEBUG_LVL_WARN, "ceAllocateDb returning NULL for size %ld\n", (u64) size);
        return OCR_ENOMEM;
    }
}

static u8 ceMemAlloc(ocrPolicyDomain_t *self, ocrFatGuid_t* allocator, u64 size,
                     u64 engineIndex, ocrMemType_t memType, void** ptr,
                     u64 prescription) {
    // Like hcAllocateDb, this function also allocates a data block.  But it does NOT guidify
    // the results.  The main usage of this function is to allocate space for the guid needed
    // by ceAllocateDb; so if this function also guidified its results, you'd be in an infinite
    // guidification loop!
    void* result;
    u64 idx;
    ASSERT (memType == GUID_MEMTYPE || memType == DB_MEMTYPE);
    result = allocateDatablock (self, size, engineIndex, prescription, &idx);
    if (result) {
        *ptr = result;
        *allocator = self->allocators[idx]->fguid;
        return 0;
    } else {
        DPRINTF(DEBUG_LVL_WARN, "ceMemAlloc returning NULL for size %ld\n", (u64) size);
        return OCR_ENOMEM;
    }
}

static u8 ceMemUnalloc(ocrPolicyDomain_t *self, ocrFatGuid_t* allocator,
                       void* ptr, ocrMemType_t memType) {
#if 1
    allocatorFreeFunction(ptr);
#else
    // Look for the allocator that has a guid that matches the one provided in the unalloc
    // request message.  (This pre-supposes that the caller properly conveys this information
    // from the response gotten back on the alloc request, to the unalloc request.  This might
    // be an error-prone design.  TODO: consider a more error-resistent design.)
    u64 i;
    ASSERT (memType == GUID_MEMTYPE || memType == DB_MEMTYPE);
    if (memType == DB_MEMTYPE) {
        for(i=0; i < self->allocatorCount; ++i) {
            if(self->allocators[i]->fguid.guid == allocator->guid) {
                allocator->metaDataPtr = self->allocators[i]->fguid.metaDataPtr;
                self->allocators[i]->fcts.free(->allocators[i], ptr);
                return 0;
            }
        }
        return OCR_EINVAL;
    } else if (memType == GUID_MEMTYPE) {
        ASSERT (self->allocatorCount > 0);
        self->allocators[self->allocatorCount-1]->fcts.free(ptr);
        return 0;
    } else {
        ASSERT (false);
    }
#endif
    return 0;
}

static u8 ceCreateEdt(ocrPolicyDomain_t *self, ocrFatGuid_t *guid,
                      ocrFatGuid_t  edtTemplate, u32 *paramc, u64* paramv,
                      u32 *depc, u32 properties, ocrFatGuid_t affinity,
                      ocrFatGuid_t * outputEvent) {


    ocrTaskTemplate_t *taskTemplate = (ocrTaskTemplate_t*)edtTemplate.metaDataPtr;

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
        *depc, properties, affinity, outputEvent, NULL);

    (*guid).guid = base->guid;
    (*guid).metaDataPtr = base;
    return 0;
}

static u8 ceCreateEdtTemplate(ocrPolicyDomain_t *self, ocrFatGuid_t *guid,
                              ocrEdt_t func, u32 paramc, u32 depc, const char* funcName) {


    ocrTaskTemplate_t *base = self->taskTemplateFactories[0]->instantiate(
        self->taskTemplateFactories[0], func, paramc, depc, funcName, NULL);
    (*guid).guid = base->guid;
    (*guid).metaDataPtr = base;
    return 0;
}

static u8 ceCreateEvent(ocrPolicyDomain_t *self, ocrFatGuid_t *guid,
                        ocrEventTypes_t type, bool takesArg) {

    ocrEvent_t *base = self->eventFactories[0]->instantiate(
        self->eventFactories[0], type, takesArg, NULL);
    (*guid).guid = base->guid;
    (*guid).metaDataPtr = base;
    return 0;
}

static void convertDepAddToSatisfy(ocrPolicyDomain_t *self, ocrFatGuid_t dbGuid,
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
    RESULT_ASSERT(self->fcts.processMessage(self, &msg, false), ==, 0);
#undef PD_MSG
#undef PD_TYPE
}

#ifdef OCR_ENABLE_STATISTICS
static ocrStats_t* ceGetStats(ocrPolicyDomain_t *self) {
    return self->statsObject;
}
#endif

static u8 ceProcessResponse(ocrPolicyDomain_t *self, ocrPolicyMsg_t *msg, u32 properties) {
    if((((ocrPolicyDomainCe_t*)self)->shutdownCount) &&
         (msg->srcLocation != self->myLocation) &&
         (msg->type != PD_MSG_MGT_SHUTDOWN)) {
        msg->type &= ~PD_MSG_TYPE_ONLY;
        msg->type |= PD_MSG_MGT_SHUTDOWN;
    }
    if (msg->srcLocation == self->myLocation) {
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |=  PD_MSG_RESPONSE;
    } else {
        msg->destLocation = msg->srcLocation;
        msg->srcLocation = self->myLocation;
        if (msg->type & PD_MSG_REQ_RESPONSE) {
            msg->type &= ~PD_MSG_REQUEST;
            msg->type |=  PD_MSG_RESPONSE;
            RESULT_ASSERT(self->fcts.sendMessage(self, msg->destLocation, msg, NULL, properties), ==, 0);
        }
    }
    return 0;
}

u8 cePolicyDomainProcessMessage(ocrPolicyDomain_t *self, ocrPolicyMsg_t *msg, u8 isBlocking) {

    u8 returnCode = 0;
    switch(msg->type & PD_MSG_TYPE_ONLY) {
    // Some messages are not handled at all
    case PD_MSG_DB_DESTROY:
    case PD_MSG_WORK_EXECUTE: case PD_MSG_DEP_UNREGSIGNALER:
    case PD_MSG_DEP_UNREGWAITER: case PD_MSG_DEP_DYNADD:
    case PD_MSG_DEP_DYNREMOVE:
    case PD_MSG_SAL_TERMINATE: case PD_MSG_MGT_REGISTER:
    case PD_MSG_MGT_UNREGISTER:
    {
        DPRINTF(DEBUG_LVL_WARN, "CE PD does not handle call of type 0x%x\n",
                (u32)(msg->type & PD_MSG_TYPE_ONLY));
    }

    // Most messages are handled locally
    case PD_MSG_DB_CREATE: {
        START_PROFILE(pd_ce_DbCreate);
#define PD_MSG msg
#define PD_TYPE PD_MSG_DB_CREATE
        // TODO: Add properties whether DB needs to be acquired or not
        // This would impact where we do the PD_MSG_MEM_ALLOC for example
        // For now we deal with both USER and RT dbs the same way
        ASSERT((PD_MSG_FIELD(dbType) == USER_DBTYPE) || (PD_MSG_FIELD(dbType) == RUNTIME_DBTYPE));
// TODO:  The prescription needs to be derived from the affinity, and needs to default to something sensible.
#define PRESCRIPTION 0xFEDCBA9876543210LL
        PD_MSG_FIELD(properties) = ceAllocateDb(
            self, &(PD_MSG_FIELD(guid)), &(PD_MSG_FIELD(ptr)), PD_MSG_FIELD(size),
            PD_MSG_FIELD(properties), msg->srcLocation,
            PD_MSG_FIELD(affinity), PD_MSG_FIELD(allocator), PRESCRIPTION);
        if(PD_MSG_FIELD(properties) == 0) {
            ocrDataBlock_t *db= PD_MSG_FIELD(guid.metaDataPtr);
            ASSERT(db);
            // TODO: Check if properties want DB acquired
            ASSERT(db->fctId == self->dbFactories[0]->factoryId);
            PD_MSG_FIELD(properties) = self->dbFactories[0]->fcts.acquire(
                db, &(PD_MSG_FIELD(ptr)), PD_MSG_FIELD(edt), false);
        } else {
            // Cannot acquire
            PD_MSG_FIELD(ptr) = NULL;
        }
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_DB_ACQUIRE: {
        START_PROFILE(pd_ce_DbAcquire);
        // Call the appropriate acquire function
#define PD_MSG msg
#define PD_TYPE PD_MSG_DB_ACQUIRE
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        localDeguidify(self, &(PD_MSG_FIELD(edt)));
        ocrDataBlock_t *db = (ocrDataBlock_t*)(PD_MSG_FIELD(guid.metaDataPtr));
        ASSERT(db->fctId == self->dbFactories[0]->factoryId);
        //ASSERT(!(msg->type & PD_MSG_REQ_RESPONSE));
        PD_MSG_FIELD(properties) = self->dbFactories[0]->fcts.acquire(
            db, &(PD_MSG_FIELD(ptr)), PD_MSG_FIELD(edt), PD_MSG_FIELD(properties) & 1);
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_DB_RELEASE: {
        START_PROFILE(pd_ce_DbRelease);
        // Call the appropriate release function
#define PD_MSG msg
#define PD_TYPE PD_MSG_DB_RELEASE
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        localDeguidify(self, &(PD_MSG_FIELD(edt)));
        ocrDataBlock_t *db = (ocrDataBlock_t*)(PD_MSG_FIELD(guid.metaDataPtr));
        //ASSERT(db->fctId == self->dbFactories[0]->factoryId);
        //ASSERT(!(msg->type & PD_MSG_REQ_RESPONSE));
        PD_MSG_FIELD(properties) =
            self->dbFactories[0]->fcts.release(db, PD_MSG_FIELD(edt), PD_MSG_FIELD(properties) & 1);
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_DB_FREE: {
        START_PROFILE(pd_ce_DbFree);
        // Call the appropriate free function
#define PD_MSG msg
#define PD_TYPE PD_MSG_DB_FREE
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        localDeguidify(self, &(PD_MSG_FIELD(edt)));
        ocrDataBlock_t *db = (ocrDataBlock_t*)(PD_MSG_FIELD(guid.metaDataPtr));
        ASSERT(db->fctId == self->dbFactories[0]->factoryId);
        ASSERT(!(msg->type & PD_MSG_REQ_RESPONSE));
        PD_MSG_FIELD(properties) =
            self->dbFactories[0]->fcts.free(db, PD_MSG_FIELD(edt));
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_MEM_ALLOC: {
        START_PROFILE(pd_ce_MemAlloc);
        u64 engineIndex = getEngineIndex(self, msg->srcLocation);
#define PD_MSG msg
#define PD_TYPE PD_MSG_MEM_ALLOC
        PD_MSG_FIELD(allocatingPD) = self->fguid;
        PD_MSG_FIELD(properties) = ceMemAlloc(
            self, &(PD_MSG_FIELD(allocator)), PD_MSG_FIELD(size),
            engineIndex, PD_MSG_FIELD(type), &(PD_MSG_FIELD(ptr)),
            PRESCRIPTION);
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_MEM_UNALLOC: {
        START_PROFILE(pd_ce_MemUnalloc);
#define PD_MSG msg
#define PD_TYPE PD_MSG_MEM_UNALLOC
        PD_MSG_FIELD(allocatingPD.metaDataPtr) = self;
        PD_MSG_FIELD(properties) = ceMemUnalloc(
            self, &(PD_MSG_FIELD(allocator)), PD_MSG_FIELD(ptr), PD_MSG_FIELD(type));
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_WORK_CREATE: {
        START_PROFILE(pd_ce_WorkCreate);
#define PD_MSG msg
#define PD_TYPE PD_MSG_WORK_CREATE
        localDeguidify(self, &(PD_MSG_FIELD(templateGuid)));
        localDeguidify(self, &(PD_MSG_FIELD(affinity)));
        ocrFatGuid_t *outputEvent = NULL;
        if(PD_MSG_FIELD(outputEvent.guid) == UNINITIALIZED_GUID) {
            outputEvent = &(PD_MSG_FIELD(outputEvent));
        }
        ASSERT(PD_MSG_FIELD(workType) == EDT_WORKTYPE);
        PD_MSG_FIELD(properties) = ceCreateEdt(
            self, &(PD_MSG_FIELD(guid)), PD_MSG_FIELD(templateGuid),
            &PD_MSG_FIELD(paramc), PD_MSG_FIELD(paramv), &PD_MSG_FIELD(depc),
            PD_MSG_FIELD(properties), PD_MSG_FIELD(affinity), outputEvent);
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
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
        PD_MSG_FIELD(properties) = self->taskFactories[0]->fcts.destruct(task);
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_EDTTEMP_CREATE: {
        START_PROFILE(pd_ce_EdtTempCreate);
#define PD_MSG msg
#define PD_TYPE PD_MSG_EDTTEMP_CREATE
        PD_MSG_FIELD(properties) = ceCreateEdtTemplate(
            self, &(PD_MSG_FIELD(guid)), PD_MSG_FIELD(funcPtr), PD_MSG_FIELD(paramc),
            PD_MSG_FIELD(depc), PD_MSG_FIELD(funcName));
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_EDTTEMP_DESTROY: {
        START_PROFILE(pd_ce_EdtTempDestroy);
#define PD_MSG msg
#define PD_TYPE PD_MSG_EDTTEMP_DESTROY
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        ocrTaskTemplate_t *tTemplate = (ocrTaskTemplate_t*)(PD_MSG_FIELD(guid.metaDataPtr));
        ASSERT(tTemplate->fctId == self->taskTemplateFactories[0]->factoryId);
        PD_MSG_FIELD(properties) = self->taskTemplateFactories[0]->fcts.destruct(tTemplate);
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_EVT_CREATE: {
        START_PROFILE(pd_ce_EvtCreate);
#define PD_MSG msg
#define PD_TYPE PD_MSG_EVT_CREATE
        PD_MSG_FIELD(properties) = ceCreateEvent(self, &(PD_MSG_FIELD(guid)),
                                                 PD_MSG_FIELD(type), PD_MSG_FIELD(properties) & 1);
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_EVT_DESTROY: {
        START_PROFILE(pd_ce_EvtDestroy);
#define PD_MSG msg
#define PD_TYPE PD_MSG_EVT_DESTROY
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        ocrEvent_t *evt = (ocrEvent_t*)PD_MSG_FIELD(guid.metaDataPtr);
        ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
        PD_MSG_FIELD(properties) = self->eventFactories[0]->fcts[evt->kind].destruct(evt);
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_EVT_GET: {
        START_PROFILE(pd_ce_EvtGet);
#define PD_MSG msg
#define PD_TYPE PD_MSG_EVT_GET
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        ocrEvent_t *evt = (ocrEvent_t*)PD_MSG_FIELD(guid.metaDataPtr);
        ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
        PD_MSG_FIELD(data) = self->eventFactories[0]->fcts[evt->kind].get(evt);
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_GUID_CREATE: {
        START_PROFILE(pd_ce_GuidCreate);
#define PD_MSG msg
#define PD_TYPE PD_MSG_GUID_CREATE
        if(PD_MSG_FIELD(size) != 0) {
            // Here we need to create a metadata area as well
            PD_MSG_FIELD(properties) = self->guidProviders[0]->fcts.createGuid(
                self->guidProviders[0], &(PD_MSG_FIELD(guid)), PD_MSG_FIELD(size),
                PD_MSG_FIELD(kind));
        } else {
            // Here we just need to associate a GUID
            ocrGuid_t temp;
            PD_MSG_FIELD(properties) = self->guidProviders[0]->fcts.getGuid(
                self->guidProviders[0], &temp, (u64)PD_MSG_FIELD(guid.metaDataPtr),
                PD_MSG_FIELD(kind));
            PD_MSG_FIELD(guid.guid) = temp;
        }
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_GUID_INFO: {
        START_PROFILE(pd_ce_GuidInfo);
#define PD_MSG msg
#define PD_TYPE PD_MSG_GUID_INFO
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        if(PD_MSG_FIELD(properties) & KIND_GUIDPROP) {
            PD_MSG_FIELD(properties) = self->guidProviders[0]->fcts.getKind(
                self->guidProviders[0], PD_MSG_FIELD(guid.guid), &(PD_MSG_FIELD(kind)));
            if(PD_MSG_FIELD(properties) == 0)
                PD_MSG_FIELD(properties) = KIND_GUIDPROP | WMETA_GUIDPROP | RMETA_GUIDPROP;
        } else {
            PD_MSG_FIELD(properties) = WMETA_GUIDPROP | RMETA_GUIDPROP;
        }
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_GUID_DESTROY: {
        START_PROFILE(pd_ce_GuidDestroy);
#define PD_MSG msg
#define PD_TYPE PD_MSG_GUID_DESTROY
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        PD_MSG_FIELD(properties) = self->guidProviders[0]->fcts.releaseGuid(
            self->guidProviders[0], PD_MSG_FIELD(guid), PD_MSG_FIELD(properties) & 1);
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_COMM_TAKE: {
        START_PROFILE(pd_ce_Take);
#define PD_MSG msg
#define PD_TYPE PD_MSG_COMM_TAKE
        ASSERT(PD_MSG_FIELD(type) == OCR_GUID_EDT);
        PD_MSG_FIELD(properties) = self->schedulers[0]->fcts.takeEdt(
            self->schedulers[0], &(PD_MSG_FIELD(guidCount)),
            PD_MSG_FIELD(guids));
        DPRINTF(DEBUG_LVL_VERB, "[CE] Sending EDT to XE%lu: guid: %lu metadata: %p\n",
                (u64)msg->srcLocation, (PD_MSG_FIELD(guids))->guid,
                (PD_MSG_FIELD(guids))->metaDataPtr);
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_COMM_GIVE: {
        START_PROFILE(pd_ce_Give);
#define PD_MSG msg
#define PD_TYPE PD_MSG_COMM_GIVE
        ASSERT(PD_MSG_FIELD(type) == OCR_GUID_EDT);
        DPRINTF(DEBUG_LVL_INFO, "[CE] Received Edt from %lu guid: %lx metadata: %p\n",
                (u64)msg->srcLocation, (PD_MSG_FIELD(guids))->guid,
                (PD_MSG_FIELD(guids))->metaDataPtr);
        PD_MSG_FIELD(properties) = self->schedulers[0]->fcts.giveEdt(
            self->schedulers[0], &(PD_MSG_FIELD(guidCount)),
            PD_MSG_FIELD(guids));
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_DEP_ADD: {
        START_PROFILE(pd_ce_DepAdd);
#define PD_MSG msg
#define PD_TYPE PD_MSG_DEP_ADD
        // We first get information about the source and destination
        ocrGuidKind srcKind, dstKind;
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(source.guid),
            (u64*)(&(PD_MSG_FIELD(source.metaDataPtr))), &srcKind);
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(dest.guid),
            (u64*)(&(PD_MSG_FIELD(dest.metaDataPtr))), &dstKind);

        ocrFatGuid_t src = PD_MSG_FIELD(source);
        ocrFatGuid_t dest = PD_MSG_FIELD(dest);
        if(srcKind == OCR_GUID_DB) {
            // This is equivalent to an immediate satisfy
            convertDepAddToSatisfy(self, src, dest, PD_MSG_FIELD(slot));
        } else {
            if(srcKind == OCR_GUID_EVENT) {
                ocrEvent_t *evt = (ocrEvent_t*)(src.metaDataPtr);
                ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
                self->eventFactories[0]->fcts[evt->kind].registerWaiter(
                    evt, dest, PD_MSG_FIELD(slot), true);
            } else {
                // Some sanity check
                ASSERT(srcKind == OCR_GUID_EDT);
            }
            if(dstKind == OCR_GUID_EDT) {
                ocrTask_t *task = (ocrTask_t*)(dest.metaDataPtr);
                ASSERT(task->fctId == self->taskFactories[0]->factoryId);
                self->taskFactories[0]->fcts.registerSignaler(task, src, PD_MSG_FIELD(slot), true);
            } else if(dstKind == OCR_GUID_EVENT) {
                ocrEvent_t *evt = (ocrEvent_t*)(dest.metaDataPtr);
                ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
                self->eventFactories[0]->fcts[evt->kind].registerSignaler(
                    evt, src, PD_MSG_FIELD(slot), true);
            } else {
                ASSERT(0); // Cannot have other types of destinations
            }
        }

#ifdef OCR_ENABLE_STATISTICS
        // TODO: Fixme
        statsDEP_ADD(pd, getCurrentEDT(), NULL, signalerGuid, waiterGuid, NULL, slot);
#endif
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_DEP_REGSIGNALER: {
        START_PROFILE(pd_ce_DepRegSignaler);
#define PD_MSG msg
#define PD_TYPE PD_MSG_DEP_REGSIGNALER
        // We first get information about the signaler and destination
        ocrGuidKind signalerKind, dstKind;
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(signaler.guid),
            (u64*)(&(PD_MSG_FIELD(signaler.metaDataPtr))), &signalerKind);
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(dest.guid),
            (u64*)(&(PD_MSG_FIELD(dest.metaDataPtr))), &dstKind);

        ocrFatGuid_t signaler = PD_MSG_FIELD(signaler);
        ocrFatGuid_t dest = PD_MSG_FIELD(dest);

        switch(dstKind) {
        case OCR_GUID_EVENT: {
            ocrEvent_t *evt = (ocrEvent_t*)(dest.metaDataPtr);
            ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
            self->eventFactories[0]->fcts[evt->kind].registerSignaler(
                evt, signaler, PD_MSG_FIELD(slot), false);
            break;
        }
        case OCR_GUID_EDT: {
            ocrTask_t *edt = (ocrTask_t*)(dest.metaDataPtr);
            ASSERT(edt->fctId == self->taskFactories[0]->factoryId);
            self->taskFactories[0]->fcts.registerSignaler(
                edt, signaler, PD_MSG_FIELD(slot), false);
            break;
        }
        default:
            ASSERT(0); // No other things we can register signalers on
        }
#ifdef OCR_ENABLE_STATISTICS
        // TODO: Fixme
        statsDEP_ADD(pd, getCurrentEDT(), NULL, signalerGuid, waiterGuid, NULL, slot);
#endif
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_DEP_REGWAITER: {
        START_PROFILE(pd_ce_DepRegWaiter);
#define PD_MSG msg
#define PD_TYPE PD_MSG_DEP_REGWAITER
// We first get information about the signaler and destination
        ocrGuidKind waiterKind, dstKind;
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(waiter.guid),
            (u64*)(&(PD_MSG_FIELD(waiter.metaDataPtr))), &waiterKind);
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(dest.guid),
            (u64*)(&(PD_MSG_FIELD(dest.metaDataPtr))), &dstKind);

        ocrFatGuid_t waiter = PD_MSG_FIELD(waiter);
        ocrFatGuid_t dest = PD_MSG_FIELD(dest);

        ASSERT(dstKind == OCR_GUID_EVENT); // Waiters can only wait on events
        ocrEvent_t *evt = (ocrEvent_t*)(dest.metaDataPtr);
        ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
        self->eventFactories[0]->fcts[evt->kind].registerWaiter(
            evt, waiter, PD_MSG_FIELD(slot), false);
#ifdef OCR_ENABLE_STATISTICS
        // TODO: Fixme
        statsDEP_ADD(pd, getCurrentEDT(), NULL, signalerGuid, waiterGuid, NULL, slot);
#endif
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_DEP_SATISFY: {
        START_PROFILE(pd_ce_DepSatisfy);
#define PD_MSG msg
#define PD_TYPE PD_MSG_DEP_SATISFY
        ocrGuidKind dstKind;
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(guid.guid),
            (u64*)(&(PD_MSG_FIELD(guid.metaDataPtr))), &dstKind);

        ocrFatGuid_t dst = PD_MSG_FIELD(guid);
        if(dstKind == OCR_GUID_EVENT) {
            ocrEvent_t *evt = (ocrEvent_t*)(dst.metaDataPtr);
            ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
            self->eventFactories[0]->fcts[evt->kind].satisfy(
                evt, PD_MSG_FIELD(payload), PD_MSG_FIELD(slot));
        } else {
            if(dstKind == OCR_GUID_EDT) {
                ocrTask_t *edt = (ocrTask_t*)(dst.metaDataPtr);
                ASSERT(edt->fctId == self->taskFactories[0]->factoryId);
                self->taskFactories[0]->fcts.satisfy(
                    edt, PD_MSG_FIELD(payload), PD_MSG_FIELD(slot));
            } else {
                ASSERT(0); // We can't satisfy anything else
            }
        }
#ifdef OCR_ENABLE_STATISTICS
        // TODO: Fixme
        statsDEP_ADD(pd, getCurrentEDT(), NULL, signalerGuid, waiterGuid, NULL, slot);
#endif
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_MGT_SHUTDOWN: {
        START_PROFILE(pd_ce_Shutdown);
        u32 neighborCount = self->neighborCount;
        ocrPolicyDomainCe_t * cePolicy = (ocrPolicyDomainCe_t *)self;
        if (msg->type & PD_MSG_REQUEST) {
            // This triggers the shutdown of the machine
            ASSERT(!(msg->type & PD_MSG_RESPONSE));
            ASSERT(msg->srcLocation != self->myLocation);
            cePolicy->shutdownCount++; //FIXME: Assumes block local shutdown message from XE
            DPRINTF(DEBUG_LVL_INFO, "CE received shutdown from XE %lu; shutdown %d/%d\n", (u64)msg->srcLocation, cePolicy->shutdownCount, neighborCount);
        } else {
            ASSERT(msg->type & PD_MSG_RESPONSE);
            ++(cePolicy->shutdownCount);
        }

        if (cePolicy->shutdownCount == neighborCount)
            self->fcts.stop(self);
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_MGT_FINISH: {
        self->fcts.finish(self);
        returnCode = ceProcessResponse(self, msg, 0);
        break;
    }

    case PD_MSG_SAL_PRINT: {
        DPRINTF(DEBUG_LVL_WARN, "CE PD does not yet implement PRINT (FIXME)\n");
        ASSERT(0);
    }

    case PD_MSG_SAL_READ: {
        DPRINTF(DEBUG_LVL_WARN, "CE PD does not yet implement READ (FIXME)\n");
        ASSERT(0);
    }

    case PD_MSG_SAL_WRITE: {
        DPRINTF(DEBUG_LVL_WARN, "CE PD does not yet implement WRITE (FIXME)\n");
        ASSERT(0);
    }

    default:
        // Not handled
        DPRINTF(DEBUG_LVL_WARN, "Unknown message type 0x%x\n", (u32)(msg->type & PD_MSG_TYPE_ONLY));
        ASSERT(0);
    }
    return returnCode;
}

u8 cePdSendMessage(ocrPolicyDomain_t* self, ocrLocation_t target, ocrPolicyMsg_t *message,
                   ocrMsgHandle_t **handle, u32 properties) {
    return self->commApis[0]->fcts.sendMessage(self->commApis[0], target, message, handle, properties);
}

u8 cePdPollMessage(ocrPolicyDomain_t *self, ocrMsgHandle_t **handle) {
    return self->commApis[0]->fcts.pollMessage(self->commApis[0], handle);
}

u8 cePdWaitMessage(ocrPolicyDomain_t *self,  ocrMsgHandle_t **handle) {
    return self->commApis[0]->fcts.waitMessage(self->commApis[0], handle);
}

void* cePdMalloc(ocrPolicyDomain_t *self, u64 size) {
    u64 i;
    void* result;
    // TODO: REC HACK. Same as for allocateDatablock. If only one allocator, use it!!
    if(self->allocatorCount == 1) {
        result = self->allocators[0]->fcts.allocate(self->allocators[0], size,
                                                    OCR_ALLOC_HINT_REDUCE_CONTENTION);
        if(result == NULL) {
            // Try the other way
            result = self->allocators[0]->fcts.allocate(self->allocators[0], size,
                                                        OCR_ALLOC_HINT_NONE);
        }

        if(result == NULL) {
            DPRINTF(DEBUG_LVL_WARN, "cePdMalloc returning NULL for size %ld\n", size);
        }
        return result;
    }

    for(i = ENGINE_INDEX_OF_CE; i < self->allocatorCount; i++) {
        result = self->allocators[i]->fcts.allocate(self->allocators[i], size,
            OCR_ALLOC_HINT_REDUCE_CONTENTION);   // Staticly reduce contention (Possibly refine later)
        if(result) break;
        result = self->allocators[i]->fcts.allocate(self->allocators[i], size,
            OCR_ALLOC_HINT_NONE);   // Staticly use common pool (Possibly refine later)
        if(result) break;
    }

    if(i < self->allocatorCount) {
        return result;
    }

    DPRINTF(DEBUG_LVL_WARN, "cePdMalloc returning NULL for size %ld\n", (u64) size);
    return NULL;
}

void cePdFree(ocrPolicyDomain_t *self, void* addr) {
    allocatorFreeFunction(addr);
}

ocrPolicyDomain_t * newPolicyDomainCe(ocrPolicyDomainFactory_t * factory,
#ifdef OCR_ENABLE_STATISTICS
                                      ocrStats_t *statsObject,
#endif
                                      ocrCost_t *costFunction, ocrParamList_t *perInstance) {

    ocrPolicyDomainCe_t * derived = (ocrPolicyDomainCe_t *) runtimeChunkAlloc(sizeof(ocrPolicyDomainCe_t), NULL);
    ocrPolicyDomain_t * base = (ocrPolicyDomain_t *) derived;

    ASSERT(base);

#ifdef OCR_ENABLE_STATISTICS
    factory->initialize(factory, base, statsObject, costFunction, perInstance);
#else
    factory->initialize(factory, base, costFunction, perInstance);
#endif

    return base;
}

void initializePolicyDomainCe(ocrPolicyDomainFactory_t * factory, ocrPolicyDomain_t* self,
#ifdef OCR_ENABLE_STATISTICS
                              ocrStats_t *statObject,
#endif
                              ocrCost_t *costFunction, ocrParamList_t *perInstance) {
#ifdef OCR_ENABLE_STATISTICS
    self->statsObject = statsObject;
#endif

    initializePolicyDomainOcr(factory, self, perInstance);
    ocrPolicyDomainCe_t* derived = (ocrPolicyDomainCe_t*) self;

    // TODO: If this is in the base class, it should be in the base param list
    self->neighborCount = ((paramListPolicyDomainCeInst_t*)perInstance)->neighborCount;
    derived->shutdownCount = 0;
}

static void destructPolicyDomainFactoryCe(ocrPolicyDomainFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrPolicyDomainFactory_t * newPolicyDomainFactoryCe(ocrParamList_t *perType) {
    ocrPolicyDomainFactory_t* base = (ocrPolicyDomainFactory_t*) runtimeChunkAlloc(sizeof(ocrPolicyDomainFactoryCe_t), (void *)1);
    ASSERT(base); // Check allocation

    // Set factory's methods
#ifdef OCR_ENABLE_STATISTICS
    base->instantiate = FUNC_ADDR(ocrPolicyDomain_t*(*)(ocrPolicyDomainFactory_t*,ocrStats_t*,
                                  ocrCost_t *,ocrParamList_t*), newPolicyDomainCe);
    base->initialize = FUNC_ADDR(void(*)(ocrPolicyDomainFactory_t*,ocrPolicyDomain_t*,
                                         ocrStats_t*,ocrCost_t *,ocrParamList_t*), initializePolicyDomainCe);
#endif
    base->instantiate = &newPolicyDomainCe;
    base->initialize = &initializePolicyDomainCe;
    base->destruct = &destructPolicyDomainFactoryCe;

    // Set future PDs' instance  methods
    base->policyDomainFcts.destruct = FUNC_ADDR(void(*)(ocrPolicyDomain_t*), cePolicyDomainDestruct);
    base->policyDomainFcts.begin = FUNC_ADDR(void(*)(ocrPolicyDomain_t*), cePolicyDomainBegin);
    base->policyDomainFcts.start = FUNC_ADDR(void(*)(ocrPolicyDomain_t*), cePolicyDomainStart);
    base->policyDomainFcts.stop = FUNC_ADDR(void(*)(ocrPolicyDomain_t*), cePolicyDomainStop);
    base->policyDomainFcts.finish = FUNC_ADDR(void(*)(ocrPolicyDomain_t*), cePolicyDomainFinish);
    base->policyDomainFcts.processMessage = FUNC_ADDR(u8(*)(ocrPolicyDomain_t*,ocrPolicyMsg_t*,u8), cePolicyDomainProcessMessage);

    base->policyDomainFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrPolicyDomain_t*, ocrLocation_t,
                                                          ocrPolicyMsg_t *, ocrMsgHandle_t**, u32),
                                                   cePdSendMessage);
    base->policyDomainFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrPolicyDomain_t*, ocrMsgHandle_t**), cePdPollMessage);
    base->policyDomainFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrPolicyDomain_t*, ocrMsgHandle_t**), cePdWaitMessage);

    base->policyDomainFcts.pdMalloc = FUNC_ADDR(void*(*)(ocrPolicyDomain_t*,u64), cePdMalloc);
    base->policyDomainFcts.pdFree = FUNC_ADDR(void(*)(ocrPolicyDomain_t*,void*), cePdFree);
#ifdef OCR_ENABLE_STATISTICS
    base->policyDomainFcts.getStats = FUNC_ADDR(ocrStats_t*(*)(ocrPolicyDomain_t*),ceGetStats);
#endif

    return base;
}

#endif /* ENABLE_POLICY_DOMAIN_CE */
