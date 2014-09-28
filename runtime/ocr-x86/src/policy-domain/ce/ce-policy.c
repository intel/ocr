/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

// TODO:  The prescription needs to be derived from the affinity, and needs to default to something sensible.
// TODO:  All this prescription logic needs to be moved out of the PD, into an abstract allocator level.  See and resolve TRAC #145.
#define PRESCRIPTION 0xFEDCBA9876544321LL   // L1 remnant, L2 slice, L2 remnant, L3 slice (twice), L3 remnant, remaining levels each slice-then-remnant.
#define PRESCRIPTION_HINT_MASK 0x1
#define PRESCRIPTION_HINT_NUMBITS 1
#define PRESCRIPTION_LEVEL_MASK 0x7
#define PRESCRIPTION_LEVEL_NUMBITS 3
#define NUM_MEM_LEVELS_SUPPORTED 8

#include "ocr-config.h"
#ifdef ENABLE_POLICY_DOMAIN_CE

#include "debug.h"
#include "ocr-errors.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

#include "ocr-comm-platform.h"
#include "policy-domain/ce/ce-policy.h"
#include "allocator/allocator-all.h"

#ifdef HAL_FSIM_CE
#include "rmd-map.h"
#endif

#define DEBUG_TYPE POLICY

extern void ocrShutdown(void);

void cePolicyDomainBegin(ocrPolicyDomain_t * policy) {
    DPRINTF(DEBUG_LVL_VVERB, "cePolicyDomainBegin called on policy at 0x%lx\n", (u64) policy);
    // The PD should have been brought up by now and everything instantiated

    u64 i = 0;
    u64 maxCount = 0;

    maxCount = policy->commApiCount;
    for(i = 0; i < maxCount; i++) {
        policy->commApis[i]->fcts.begin(policy->commApis[i], policy);
    }

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

    policy->placer = NULL;

    // REC: Moved all workers to start here.
    // Note: it's important to first logically start all workers.
    // Once they are all up, start the runtime.
    // Workers should start the underlying target and platforms
    maxCount = policy->workerCount;
    for(i = 0; i < maxCount; i++) {
        policy->workers[i]->fcts.begin(policy->workers[i], policy);
    }

#ifdef HAL_FSIM_CE
    // Neighbor discovery
    // FIXME: Severely restrictive, only valid within a unit. Extend: trac #231
    u32 ncount = 0;
    for(i = 0; i < MAX_NUM_BLOCK && ncount < policy->neighborCount; i++) {
        // Enumerate CEs
        if(policy->myLocation != MAKE_CORE_ID(0, 0, 0, 0, i, ID_AGENT_CE))
            // If I'm not the enumerated CE, add as my neighbor
            policy->neighbors[ncount++] = MAKE_CORE_ID(0, 0, 0, 0, i, ID_AGENT_CE);
    }
#endif

    ocrPolicyDomainCe_t * cePolicy = (ocrPolicyDomainCe_t*)policy;
    //FIXME: This is a hack to get the shutdown reportees.
    //This should be replaced by a registration protocol for children to parents.
    //Trac bug: #134
    if (policy->myLocation == policy->parentLocation) {
        cePolicy->shutdownMax = cePolicy->xeCount + policy->neighborCount;
    } else {
        cePolicy->shutdownMax = cePolicy->xeCount;
    }
}

void cePolicyDomainStart(ocrPolicyDomain_t * policy) {
    DPRINTF(DEBUG_LVL_VVERB, "cePolicyDomainStart called on policy at 0x%lx\n", (u64) policy);
    // The PD should have been brought up by now and everything instantiated
    // This is a bit ugly but I can't find a cleaner solution:
    //   - we need to associate the environment with the
    //   currently running worker/PD so that we can use getCurrentEnv

    ocrPolicyDomainCe_t* cePolicy = (ocrPolicyDomainCe_t*) policy;
    u64 i = 0;
    u64 maxCount = 0;
    u64 low, high, level, engineIdx;

    maxCount = policy->allocatorCount;
    low = 0;
    for(i = 0; i < maxCount; ++i) {
        level = policy->allocators[low]->memories[0]->level;
        high = i+1;
        if (high == maxCount || policy->allocators[high]->memories[0]->level != level) {
            // One or more allocators for some level were provided in the config file.  Their indices in
            // the array of allocators span from low(inclusive) to high (exclusive).  From this information,
            // initialize the allocatorIndexLookup table for that level, for all agents.
            if (high - low == 1) {
                // All agents in the block use the same allocator (usage conjecture: on TG,
                // this is used for all but L1.)
                for (engineIdx = 0; engineIdx <= cePolicy->xeCount; engineIdx++) {
                    policy->allocatorIndexLookup[engineIdx*NUM_MEM_LEVELS_SUPPORTED+level] = low;
                }
            } else if (high - low == 2) {
                // All agents except the last in the block (i.e. the XEs) use the same allocator.
                // The last (i.e. the CE) uses a different allocator.  (usage conjecture: on TG,
                // this is might be used to experiment with XEs sharing a common SPAD that is
                // different than what the CE uses.  This is presently just for academic interst.)
                for (engineIdx = 0; engineIdx < cePolicy->xeCount; engineIdx++) {
                    policy->allocatorIndexLookup[engineIdx*NUM_MEM_LEVELS_SUPPORTED+level] = low;
                }
                policy->allocatorIndexLookup[engineIdx*NUM_MEM_LEVELS_SUPPORTED+level] = low+1;
            } else if (high - low == cePolicy->xeCount+1) {
                // All agents in the block use different allocators (usage conjecture: on TG,
                // this is used for L1.)
                for (engineIdx = 0; engineIdx <= cePolicy->xeCount; engineIdx++) {
                    policy->allocatorIndexLookup[engineIdx*NUM_MEM_LEVELS_SUPPORTED+level] = low+engineIdx;
                }
            } else {
                DPRINTF(DEBUG_LVL_WARN, "I don't know how to spread allocator[%ld:%ld] (level %ld) across %ld XEs plus the CE\n", (u64) low, (u64) high-1, (u64) level, (u64) cePolicy->xeCount);
                ASSERT (0);
            }
            low = high;
        }
    }

    maxCount = policy->guidProviderCount;
    for(i = 0; i < maxCount; ++i) {
        policy->guidProviders[i]->fcts.start(policy->guidProviders[i], policy);
    }

    maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fcts.start(policy->allocators[i], policy);
    }

    guidify(policy, (u64)policy, &(policy->fguid), OCR_GUID_POLICY);

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
    runtimeChunkFree((u64)policy->allocatorIndexLookup, NULL);
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

    ocrPolicyDomainCe_t* derived = (ocrPolicyDomainCe_t*) self;

    // We need the block-relative index of the engine (aka agent).  That index for XE's ranges from
    // 0 to xeCount-1, and the engine index for the CE is next, i.e. it equals xeCount.  We derive
    // the index from the system-wide absolute location of the agent.  If a time comes when different
    // chips in a system might have different numbers of XEs per block, this code might break, and
    // have to be reworked.

    ocrLocation_t blockRelativeEngineIndex =
        location -         // Absolute location of the agent for which the allocation is being done.
        self->myLocation + // Absolute location of the CE doing the allocation.
        derived->xeCount;  // Offset so that first XE is 0, last is xeCount-1, and CE is xeCount.
    ASSERT(blockRelativeEngineIndex >= 0);
    ASSERT(blockRelativeEngineIndex <= derived->xeCount);
    return blockRelativeEngineIndex;
}

// In all these functions, we consider only a single PD. In other words, in CE, we
// deal with everything locally and never send messages

// allocateDatablock:  Utility used by ceAllocateDb and ceMemAlloc, just below.
static void* allocateDatablock (ocrPolicyDomain_t *self,
                                u64                size,
                                u64                engineIndex,
                                u64                prescription,
                                u64               *allocatorIndexReturn) {
    void* result;
    u64 allocatorHints;  // Allocator hint
    u64 levelIndex; // "Level" of the memory hierarchy, taken from the "prescription" to pick from the allocators array the allocator to try.
    s8 allocatorIndex;

    ASSERT (self->allocatorCount > 0);

    do {
        allocatorHints = (prescription & PRESCRIPTION_HINT_MASK) ?
            (OCR_ALLOC_HINT_NONE) :              // If the hint is non-zero, pick this (e.g. for tlsf, tries "remnant" pool.)
            (OCR_ALLOC_HINT_REDUCE_CONTENTION);  // If the hint is zero, pick this (e.g. for tlsf, tries a round-robin-chosen "slice" pool.)
        prescription >>= PRESCRIPTION_HINT_NUMBITS;
        levelIndex = prescription & PRESCRIPTION_LEVEL_MASK;
        prescription >>= PRESCRIPTION_LEVEL_NUMBITS;
        allocatorIndex = self->allocatorIndexLookup[engineIndex*NUM_MEM_LEVELS_SUPPORTED+levelIndex]; // Lookup index of allocator to use for requesting engine (aka agent) at prescribed memory hierarchy level.
        if ((allocatorIndex < 0) ||
            (allocatorIndex >= self->allocatorCount) ||
            (self->allocators[allocatorIndex] == NULL)) continue;  // Skip this allocator if it doesn't exist.
        result = self->allocators[allocatorIndex]->fcts.allocate(self->allocators[allocatorIndex], size, allocatorHints);

        if (result) {
#ifdef HAL_FSIM_CE
            if((u64)result <= CE_MSR_BASE && (u64)result >= BR_CE_BASE) {
                result = (void *)DR_CE_BASE(CHIP_FROM_ID(self->myLocation),
                                            UNIT_FROM_ID(self->myLocation),
                                            BLOCK_FROM_ID(self->myLocation))
                                            + (u64)(result - BR_CE_BASE);
                u64 check = MAKE_CORE_ID(0,
                                         0,
                                         ((((u64)result >> MAP_CHIP_SHIFT) & ((1ULL<<MAP_CHIP_LEN) - 1)) - 1),
                                         ((((u64)result >> MAP_UNIT_SHIFT) & ((1ULL<<MAP_UNIT_LEN) - 1)) - 2),
                                         ((((u64)result >> MAP_BLOCK_SHIFT) & ((1ULL<<MAP_BLOCK_LEN) - 1)) - 2),
                                         ID_AGENT_CE);
                ASSERT(check==self->myLocation);
            }
#endif
            DPRINTF (DEBUG_LVL_VVERB, "Success allocationg %5ld-byte block to allocator %ld (level %ld) for engine %ld (%2ld) -- 0x%lx\n",
            (u64) size, (u64) allocatorIndex, (u64) levelIndex, (u64) engineIndex, (u64) self->myLocation, (u64) (((u64*) result)[-1]));
            *allocatorIndexReturn = allocatorIndex;
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
#ifdef HAL_FSIM_CE
    u64 base = DR_CE_BASE(CHIP_FROM_ID(self->myLocation),
                          UNIT_FROM_ID(self->myLocation),
                          BLOCK_FROM_ID(self->myLocation));
#define LOCAL_MASK 0xFFFFFULL  // trac #222
    if(((u64)ptr < CE_MSR_BASE) || ((~LOCAL_MASK & (u64)ptr) == base)) {
        allocatorFreeFunction(ptr); // CE's spad
    } else {
        ocrPolicyMsg_t msg;
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_MEM_UNALLOC
        msg.type = PD_MSG_MEM_UNALLOC | PD_MSG_REQUEST;
        msg.destLocation = MAKE_CORE_ID(0,
                                        0,
                                        ((((u64)ptr >> MAP_CHIP_SHIFT) & ((1ULL<<MAP_CHIP_LEN) - 1)) - 1),
                                        ((((u64)ptr >> MAP_UNIT_SHIFT) & ((1ULL<<MAP_UNIT_LEN) - 1)) - 2),
                                        ((((u64)ptr >> MAP_BLOCK_SHIFT) & ((1ULL<<MAP_BLOCK_LEN) - 1)) - 2),
                                        ID_AGENT_CE);
        msg.srcLocation = self->myLocation;
        PD_MSG_FIELD(ptr) = ((void *) ptr);
        PD_MSG_FIELD(type) = memType;
        while(self->fcts.sendMessage(self, msg.destLocation, &msg, NULL, 0)) {
           ocrPolicyMsg_t myMsg;
           ocrMsgHandle_t myHandle;
           myHandle.msg = &myMsg;
           ocrMsgHandle_t *handle = &myHandle;
           while(!self->fcts.pollMessage(self, &handle))
               RESULT_ASSERT(self->fcts.processMessage(self, handle->response, true), ==, 0);
        }
#undef PD_MSG
#undef PD_TYPE
    }
#else
    allocatorFreeFunction(ptr);
#endif
    return 0;
}

static u8 ceCreateEdt(ocrPolicyDomain_t *self, ocrFatGuid_t *guid,
                      ocrFatGuid_t  edtTemplate, u32 *paramc, u64* paramv,
                      u32 *depc, u32 properties, ocrFatGuid_t affinity,
                      ocrFatGuid_t * outputEvent, ocrTask_t * currentEdt,
                      ocrFatGuid_t parentLatch) {


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
        *depc, properties, affinity, outputEvent, currentEdt, parentLatch, NULL);

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
static ocrStats_t* ceGetStats(ocrPolicyDomain_t *self) {
    return self->statsObject;
}
#endif

static u8 ceProcessResponse(ocrPolicyDomain_t *self, ocrPolicyMsg_t *msg, u32 properties) {
    if (msg->srcLocation == self->myLocation) {
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |=  PD_MSG_RESPONSE;
    } else {
        msg->destLocation = msg->srcLocation;
        msg->srcLocation = self->myLocation;
        if((msg->type & PD_CE_CE_MESSAGE) && (msg->type & PD_MSG_REQUEST)) {
             // FIXME: Ugly, but no other way to destruct the message
             self->commApis[0]->commPlatform->fcts.destructMessage(self->commApis[0]->commPlatform, msg);
        }

        if((((ocrPolicyDomainCe_t*)self)->shutdownMode) &&
           (msg->type != PD_MSG_MGT_SHUTDOWN)) {
            msg->type &= ~PD_MSG_TYPE_ONLY;
            msg->type |= PD_MSG_MGT_SHUTDOWN;
            properties |= BLOCKING_SEND_MSG_PROP;
        }

        if (msg->type & PD_MSG_REQ_RESPONSE) {
            msg->type &= ~PD_MSG_REQUEST;
            msg->type |=  PD_MSG_RESPONSE;
#ifdef HAL_FSIM_CE
            if((msg->destLocation & ID_AGENT_CE) == ID_AGENT_CE) {
                // This is a CE->CE message
                ocrPolicyMsg_t toSend;
                u64 fullMsgSize = 0, marshalledSize = 0;
                ocrPolicyMsgGetMsgSize(msg, &fullMsgSize, &marshalledSize);
                msg->size = fullMsgSize;
                ocrPolicyMsgMarshallMsg(msg, (u8 *)&toSend, MARSHALL_DUPLICATE);

                while(self->fcts.sendMessage(self, toSend.destLocation, &toSend, NULL, properties)) {
                    ocrPolicyMsg_t myMsg;
                    ocrMsgHandle_t myHandle;
                    myHandle.msg = &myMsg;
                    ocrMsgHandle_t *handle = &myHandle;
                    while(!self->fcts.pollMessage(self, &handle))
                        RESULT_ASSERT(self->fcts.processMessage(self, handle->response, true), ==, 0);
                }
            } else {
                // This is a CE->XE message
                RESULT_ASSERT(self->fcts.sendMessage(self, msg->destLocation, msg, NULL, properties), ==, 0);
            }
#else
            RESULT_ASSERT(self->fcts.sendMessage(self, msg->destLocation, msg, NULL, properties), ==, 0);
#endif
        }
    }
    return 0;
}

u8 cePolicyDomainProcessMessage(ocrPolicyDomain_t *self, ocrPolicyMsg_t *msg, u8 isBlocking) {

    u8 returnCode = 0;

    DPRINTF(DEBUG_LVL_VERB, "Going to processs message of type 0x%x\n",
            msg->type & PD_MSG_TYPE_ONLY);

    switch((u32)(msg->type & PD_MSG_TYPE_ONLY)) {
    // Some messages are not handled at all
    case PD_MSG_DB_DESTROY:
    case PD_MSG_WORK_EXECUTE: case PD_MSG_DEP_UNREGSIGNALER:
    case PD_MSG_DEP_UNREGWAITER: case PD_MSG_DEP_DYNADD:
    case PD_MSG_DEP_DYNREMOVE:
    case PD_MSG_GUID_METADATA_CLONE:
    case PD_MSG_SAL_TERMINATE: case PD_MSG_MGT_REGISTER:
    case PD_MSG_MGT_UNREGISTER:
    case PD_MSG_MGT_MONITOR_PROGRESS:
    {
        DPRINTF(DEBUG_LVL_WARN, "CE PD does not handle call of type 0x%x\n",
                (u32)(msg->type & PD_MSG_TYPE_ONLY));
        ASSERT(0);
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
        DPRINTF(DEBUG_LVL_VVERB, "DB_CREATE request from 0x%lx for size %lu\n",
                msg->srcLocation, PD_MSG_FIELD(size));
// TODO:  The prescription needs to be derived from the affinity, and needs to default to something sensible.
        u64 engineIndex = getEngineIndex(self, msg->srcLocation);
        PD_MSG_FIELD(returnDetail) = ceAllocateDb(
            self, &(PD_MSG_FIELD(guid)), &(PD_MSG_FIELD(ptr)), PD_MSG_FIELD(size),
            PD_MSG_FIELD(properties), engineIndex,
            PD_MSG_FIELD(affinity), PD_MSG_FIELD(allocator), PRESCRIPTION);
        if(PD_MSG_FIELD(returnDetail) == 0) {
            ocrDataBlock_t *db= PD_MSG_FIELD(guid.metaDataPtr);
            ASSERT(db);
            // TODO: Check if properties want DB acquired
            ASSERT(db->fctId == self->dbFactories[0]->factoryId);
            PD_MSG_FIELD(returnDetail) = self->dbFactories[0]->fcts.acquire(
                db, &(PD_MSG_FIELD(ptr)), PD_MSG_FIELD(edt), EDT_SLOT_NONE,
                DB_MODE_ITW, false, (u32)DB_MODE_ITW);
        } else {
            // Cannot acquire
            PD_MSG_FIELD(ptr) = NULL;
        }
        DPRINTF(DEBUG_LVL_VVERB, "DB_CREATE response for size %lu: GUID: 0x%lx; PTR: 0x%lx)\n",
                PD_MSG_FIELD(size), PD_MSG_FIELD(guid.guid), PD_MSG_FIELD(ptr));
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
        if (msg->type & PD_MSG_REQUEST) {
            localDeguidify(self, &(PD_MSG_FIELD(guid)));
            localDeguidify(self, &(PD_MSG_FIELD(edt)));
            ocrDataBlock_t *db = (ocrDataBlock_t*)(PD_MSG_FIELD(guid.metaDataPtr));
            DPRINTF(DEBUG_LVL_VVERB, "DB_ACQUIRE request from 0x%lx for GUID 0x%lx\n",
                    msg->srcLocation, PD_MSG_FIELD(guid.guid));
            ASSERT(db->fctId == self->dbFactories[0]->factoryId);
            //ASSERT(!(msg->type & PD_MSG_REQ_RESPONSE));
            PD_MSG_FIELD(returnDetail) = self->dbFactories[0]->fcts.acquire(
                db, &(PD_MSG_FIELD(ptr)), PD_MSG_FIELD(edt), PD_MSG_FIELD(edtSlot),
                (ocrDbAccessMode_t)(PD_MSG_FIELD(properties) & (u32)DB_ACCESS_MODE_MASK),
                PD_MSG_FIELD(properties) & DB_PROP_RT_ACQUIRE, PD_MSG_FIELD(properties));
            PD_MSG_FIELD(size) = db->size;
            PD_MSG_FIELD(properties) |= db->flags;
            // NOTE: at this point the DB may not have been acquired.
            // In that case PD_MSG_FIELD(returnDetail) == OCR_EBUSY
            // The XE checks for this return code.
            DPRINTF(DEBUG_LVL_VVERB, "DB_ACQUIRE response for GUID 0x%lx: PTR: 0x%lx\n",
                    PD_MSG_FIELD(guid.guid), PD_MSG_FIELD(ptr));
            returnCode = ceProcessResponse(self, msg, 0);
        } else {
            // This a callback response to an acquire that was pending. The CE just need
            // to call the task's dependenceResolve to iterate the task's acquire frontier
            ASSERT(msg->type & PD_MSG_RESPONSE);
            // asynchronous callback on acquire, reading response
            ocrFatGuid_t edtFGuid = PD_MSG_FIELD(edt);
            ocrFatGuid_t dbFGuid = PD_MSG_FIELD(guid);
            u32 edtSlot = PD_MSG_FIELD(edtSlot);
            localDeguidify(self, &edtFGuid);
            // At this point the edt MUST be local as well as the db data pointer.
            ocrTask_t* task = (ocrTask_t*) edtFGuid.metaDataPtr;
            PD_MSG_FIELD(returnDetail) = self->taskFactories[0]->fcts.dependenceResolved(task, dbFGuid.guid, PD_MSG_FIELD(ptr), edtSlot);
        }
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
        ASSERT(db->fctId == self->dbFactories[0]->factoryId);
        //ASSERT(!(msg->type & PD_MSG_REQ_RESPONSE));
        DPRINTF(DEBUG_LVL_VVERB, "DB_RELEASE req/resp from 0x%lx for GUID 0x%lx\n",
                msg->srcLocation, PD_MSG_FIELD(guid.guid));
        PD_MSG_FIELD(returnDetail) =
            self->dbFactories[0]->fcts.release(db, PD_MSG_FIELD(edt), PD_MSG_FIELD(properties) & DB_PROP_RT_ACQUIRE);
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
        DPRINTF(DEBUG_LVL_VVERB, "DB_FREE req/resp from 0x%lx for GUID 0x%lx\n",
                msg->srcLocation, PD_MSG_FIELD(guid.guid));
        ASSERT(db->fctId == self->dbFactories[0]->factoryId);
        ASSERT(!(msg->type & PD_MSG_REQ_RESPONSE));
        PD_MSG_FIELD(returnDetail) =
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
        DPRINTF(DEBUG_LVL_VVERB, "MEM_ALLOC request from 0x%lx for size %lu\n",
                msg->srcLocation, PD_MSG_FIELD(size));
        PD_MSG_FIELD(returnDetail) = ceMemAlloc(
            self, &(PD_MSG_FIELD(allocator)), PD_MSG_FIELD(size),
            engineIndex, PD_MSG_FIELD(type), &(PD_MSG_FIELD(ptr)),
            PRESCRIPTION);
        DPRINTF(DEBUG_LVL_VVERB, "MEM_ALLOC response for size %lu: PTR: 0x%lx\n",
                PD_MSG_FIELD(size), PD_MSG_FIELD(ptr));
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
        DPRINTF(DEBUG_LVL_VVERB, "MEM_UNALLOC req/resp from 0x%lx for PTR 0x%lx\n",
                msg->srcLocation, PD_MSG_FIELD(ptr));
        PD_MSG_FIELD(returnDetail) = ceMemUnalloc(
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
        localDeguidify(self, &(PD_MSG_FIELD(currentEdt)));
        localDeguidify(self, &(PD_MSG_FIELD(parentLatch)));
        ocrFatGuid_t *outputEvent = NULL;
        if(PD_MSG_FIELD(outputEvent.guid) == UNINITIALIZED_GUID) {
            outputEvent = &(PD_MSG_FIELD(outputEvent));
        }
        ASSERT((PD_MSG_FIELD(workType) == EDT_USER_WORKTYPE) || (PD_MSG_FIELD(workType) == EDT_RT_WORKTYPE));
        DPRINTF(DEBUG_LVL_VVERB, "WORK_CREATE request from 0x%lx\n",
                msg->srcLocation);
        PD_MSG_FIELD(returnDetail) = ceCreateEdt(
            self, &(PD_MSG_FIELD(guid)), PD_MSG_FIELD(templateGuid),
            &PD_MSG_FIELD(paramc), PD_MSG_FIELD(paramv), &PD_MSG_FIELD(depc),
            PD_MSG_FIELD(properties), PD_MSG_FIELD(affinity), outputEvent,
            (ocrTask_t*)(PD_MSG_FIELD(currentEdt).metaDataPtr), PD_MSG_FIELD(parentLatch));
        DPRINTF(DEBUG_LVL_VVERB, "WORK_CREATE response: GUID: 0x%lx\n",
                PD_MSG_FIELD(guid.guid));
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
        DPRINTF(DEBUG_LVL_VVERB, "WORK_DESTROY req/resp from 0x%lx for GUID 0x%lx\n",
                msg->srcLocation, PD_MSG_FIELD(guid.guid));
        ASSERT(task->fctId == self->taskFactories[0]->factoryId);
        PD_MSG_FIELD(returnDetail) = self->taskFactories[0]->fcts.destruct(task);
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
        DPRINTF(DEBUG_LVL_VVERB, "EDTTEMP_CREATE request from 0x%lx\n",
                msg->srcLocation);
        PD_MSG_FIELD(returnDetail) = ceCreateEdtTemplate(
            self, &(PD_MSG_FIELD(guid)), PD_MSG_FIELD(funcPtr), PD_MSG_FIELD(paramc),
            PD_MSG_FIELD(depc), PD_MSG_FIELD(funcName));
        DPRINTF(DEBUG_LVL_VVERB, "EDTTEMP_CREATE response: GUID: 0x%lx\n",
                PD_MSG_FIELD(guid.guid));
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
        DPRINTF(DEBUG_LVL_VVERB, "EDTTEMP_DESTROY req/resp from 0x%lx for GUID 0x%lx\n",
                msg->srcLocation, PD_MSG_FIELD(guid.guid));
        ASSERT(tTemplate->fctId == self->taskTemplateFactories[0]->factoryId);
        PD_MSG_FIELD(returnDetail) = self->taskTemplateFactories[0]->fcts.destruct(tTemplate);
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
        DPRINTF(DEBUG_LVL_VVERB, "EVT_CREATE request from 0x%lx of type %u\n",
                msg->srcLocation, PD_MSG_FIELD(type));
        PD_MSG_FIELD(returnDetail) = ceCreateEvent(self, &(PD_MSG_FIELD(guid)),
                                                   PD_MSG_FIELD(type), PD_MSG_FIELD(properties) & 1);
        DPRINTF(DEBUG_LVL_VVERB, "EVT_CREATE response for type %u: GUID: 0x%lx\n",
                PD_MSG_FIELD(type), PD_MSG_FIELD(guid.guid));
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
        DPRINTF(DEBUG_LVL_VVERB, "EVT_DESTROY req/resp from 0x%lx for GUID 0x%lx\n",
                msg->srcLocation, PD_MSG_FIELD(guid.guid));
        ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
        PD_MSG_FIELD(returnDetail) = self->eventFactories[0]->fcts[evt->kind].destruct(evt);
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
        DPRINTF(DEBUG_LVL_VVERB, "EVT_GET request from 0x%lx for GUID 0x%lx\n",
                msg->srcLocation, PD_MSG_FIELD(guid.guid));
        ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
        PD_MSG_FIELD(data) = self->eventFactories[0]->fcts[evt->kind].get(evt);
        DPRINTF(DEBUG_LVL_VVERB, "EVT_GET response for GUID 0x%lx: GUID: 0x%lx\n",
                PD_MSG_FIELD(guid.guid), PD_MSG_FIELD(data.guid));
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
            DPRINTF(DEBUG_LVL_VVERB, "GUID_CREATE(new) request from 0x%lx\n",
                    msg->srcLocation);
            PD_MSG_FIELD(returnDetail) = self->guidProviders[0]->fcts.createGuid(
                self->guidProviders[0], &(PD_MSG_FIELD(guid)), PD_MSG_FIELD(size),
                PD_MSG_FIELD(kind));
        } else {
            // Here we just need to associate a GUID
            ocrGuid_t temp;
            DPRINTF(DEBUG_LVL_VVERB, "GUID_CREATE(exist) request from 0x%lx\n",
                    msg->srcLocation);
            PD_MSG_FIELD(returnDetail) = self->guidProviders[0]->fcts.getGuid(
                self->guidProviders[0], &temp, (u64)PD_MSG_FIELD(guid.metaDataPtr),
                PD_MSG_FIELD(kind));
            PD_MSG_FIELD(guid.guid) = temp;
        }
        DPRINTF(DEBUG_LVL_VVERB, "GUID_CREATE response: GUID: 0x%lx\n",
                PD_MSG_FIELD(guid.guid));
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
        DPRINTF(DEBUG_LVL_VVERB, "GUID_INFO request from 0x%lx for GUID 0x%lx\n",
                msg->srcLocation, PD_MSG_FIELD(guid.guid));
        if(PD_MSG_FIELD(properties) & KIND_GUIDPROP) {
            PD_MSG_FIELD(returnDetail) = self->guidProviders[0]->fcts.getKind(
                self->guidProviders[0], PD_MSG_FIELD(guid.guid), &(PD_MSG_FIELD(kind)));
            if(PD_MSG_FIELD(returnDetail) == 0)
                PD_MSG_FIELD(returnDetail) = KIND_GUIDPROP | WMETA_GUIDPROP | RMETA_GUIDPROP;
        } else if (PD_MSG_FIELD(properties) & LOCATION_GUIDPROP) {
            PD_MSG_FIELD(returnDetail) = self->guidProviders[0]->fcts.getLocation(
                self->guidProviders[0], PD_MSG_FIELD(guid.guid), &(PD_MSG_FIELD(location)));
            if(PD_MSG_FIELD(returnDetail) == 0) {
                PD_MSG_FIELD(returnDetail) = LOCATION_GUIDPROP | WMETA_GUIDPROP | RMETA_GUIDPROP;
            }
        } else {
            PD_MSG_FIELD(returnDetail) = WMETA_GUIDPROP | RMETA_GUIDPROP;
        }
        // TODO print more useful stuff
        DPRINTF(DEBUG_LVL_VVERB, "GUID_INFO response\n");
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
        DPRINTF(DEBUG_LVL_VVERB, "GUID_DESTROY req/resp from 0x%lx for GUID 0x%lx\n",
                msg->srcLocation, PD_MSG_FIELD(guid.guid));
        PD_MSG_FIELD(returnDetail) = self->guidProviders[0]->fcts.releaseGuid(
            self->guidProviders[0], PD_MSG_FIELD(guid), PD_MSG_FIELD(properties) & 1);
        returnCode = ceProcessResponse(self, msg, 0);
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_COMM_TAKE: {
        START_PROFILE(pd_ce_Take);
        // TAKE protocol on multiblock TG:
        //
        // When a XE is out of work it requests the CE for work.
        // If the CE is out of work, it pings a random neighbor CE for work.
        // So, a CE can receive a TAKE request from either a XE or another CE.
        // For a CE, the reaction to a TAKE request is currently the same,
        // whether it came from a XE or another CE, which is:
        //
        // Ask your own scheduler first.
        // If no work was found locally, start to ping other CE's for new work.
        // The protocol makes sure that an empty CE does not request back
        // to a requesting CE. It also makes sure (through local flags), that
        // it does not resend a request to another CE to which it had already
        // posted one earlier.
        //
        // This protocol may cause an exponentially growing number
        // of search messages until it fills up the system or finds work.
        // It may or may not be the fastest way to grab work, but definitely
        // not energy efficient nor communication-wise cheap. Also, who receives
        // what work is totally random.
        //
        // This implementation will serve as the initial baseline for future
        // improvements.
#define PD_MSG msg
#define PD_TYPE PD_MSG_COMM_TAKE
        ASSERT(PD_MSG_FIELD(type) == OCR_GUID_EDT);
        // A TAKE request can come from either a CE or XE
        if (msg->type & PD_MSG_REQUEST) {
            DPRINTF(DEBUG_LVL_VVERB, "COMM_TAKE request from 0x%lx\n",
                msg->srcLocation);

            //First check if my own scheduler can give out work
            PD_MSG_FIELD(returnDetail) = self->schedulers[0]->fcts.takeEdt(
                self->schedulers[0], &(PD_MSG_FIELD(guidCount)), PD_MSG_FIELD(guids));

            //If my scheduler does not have work, (i.e guidCount == 0)
            //then we need to start looking for work on other CE's.
            ocrPolicyDomainCe_t * cePolicy = (ocrPolicyDomainCe_t*)self;
            static s32 throttlecount = 1;
            // FIXME: very basic self-throttling mechanism
            if ((PD_MSG_FIELD(guidCount) == 0) && (cePolicy->shutdownMode == false) && (--throttlecount <= 0)) {
                throttlecount = 200*(self->neighborCount-1);
                //Try other CE's
                u32 i;
                for (i = 0; i < self->neighborCount; i++) {
                    //Check if I already have an active work request pending on a CE
                    //If not, then we post one
                    if ((cePolicy->ceCommTakeActive[i] == 0) && (msg->srcLocation != self->neighbors[i])) {
#undef PD_MSG
#define PD_MSG (&ceMsg)
                        ocrPolicyMsg_t ceMsg;
                        getCurrentEnv(NULL, NULL, NULL, &ceMsg);
                        ocrFatGuid_t fguid[CE_TAKE_CHUNK_SIZE] = {{0}};
                        ceMsg.destLocation = self->neighbors[i];
                        ceMsg.type = PD_MSG_COMM_TAKE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE | PD_CE_CE_MESSAGE;
                        PD_MSG_FIELD(guids) = &(fguid[0]);
                        PD_MSG_FIELD(guidCount) = CE_TAKE_CHUNK_SIZE;
                        PD_MSG_FIELD(properties) = 0;
                        PD_MSG_FIELD(type) = OCR_GUID_EDT;
                        if(self->fcts.sendMessage(self, ceMsg.destLocation, &ceMsg, NULL, 0) == 0)
                            cePolicy->ceCommTakeActive[i] = 1;
#undef PD_MSG
#define PD_MSG msg
                    }
                }
            }

            // Respond to the requester
            // If my own scheduler had extra work, we respond with work
            // If not, then we respond with no work (but we've already put out work requests by now)
            if(PD_MSG_FIELD(guidCount)) {
                  if(PD_MSG_FIELD(guids[0]).metaDataPtr == NULL)
                      localDeguidify(self, &(PD_MSG_FIELD(guids[0])));
            }
            returnCode = ceProcessResponse(self, msg, PD_MSG_FIELD(properties));
        } else { // A TAKE response has to be from another CE responding to my own request
            ASSERT(msg->type & PD_MSG_RESPONSE);
            u32 i, j;
            for (i = 0; i < self->neighborCount; i++) {
                if (msg->srcLocation == self->neighbors[i]) {
                    ocrPolicyDomainCe_t * cePolicy = (ocrPolicyDomainCe_t*)self;
                    cePolicy->ceCommTakeActive[i] = 0;
                    if (PD_MSG_FIELD(guidCount) > 0) {
                        //If my work request was responded with work,
                        //then we store it in our own scheduler
                        ASSERT(PD_MSG_FIELD(type) == OCR_GUID_EDT);
                        ocrFatGuid_t *fEdtGuids = PD_MSG_FIELD(guids);

                        DPRINTF(DEBUG_LVL_INFO, "Received %lu Edt(s) from 0x%lx: ",
                                PD_MSG_FIELD(guidCount), (u64)msg->srcLocation);
                        for (j = 0; j < PD_MSG_FIELD(guidCount); j++) {
                            DPRINTF(DEBUG_LVL_INFO, "(guid:0x%lx metadata: %p) ", fEdtGuids[j].guid, fEdtGuids[j].metaDataPtr);
                        }
                        DPRINTF(DEBUG_LVL_INFO, "\n");

                        PD_MSG_FIELD(returnDetail) = self->schedulers[0]->fcts.giveEdt(
                            self->schedulers[0], &(PD_MSG_FIELD(guidCount)), fEdtGuids);
                    }
                    break;
                }
            }
            ASSERT(i < self->neighborCount);
        }
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
        DPRINTF(DEBUG_LVL_VVERB, "COMM_GIVE req/resp from 0x%lx with GUID 0x%lx\n",
                msg->srcLocation, (PD_MSG_FIELD(guids))->guid);
        PD_MSG_FIELD(returnDetail) = self->schedulers[0]->fcts.giveEdt(
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
        DPRINTF(DEBUG_LVL_VVERB, "DEP_ADD req/resp from 0x%lx for 0x%lx -> 0x%lx\n",
                msg->srcLocation, PD_MSG_FIELD(source.guid), PD_MSG_FIELD(dest.guid));
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
        ocrDbAccessMode_t mode = (PD_MSG_FIELD(properties) & DB_PROP_MODE_MASK); //lower 3-bits is mode //TODO not pretty
        if (srcKind == NULL_GUID) {
            //NOTE: Handle 'NULL_GUID' case here to be safe although
            //we've already caught it in ocrAddDependence for performance
            // This is equivalent to an immediate satisfy
            PD_MSG_FIELD(returnDetail) = convertDepAddToSatisfy(
                self, src, dest, PD_MSG_FIELD(slot));
        } else if (srcKind == OCR_GUID_DB) {
            if(dstKind & OCR_GUID_EVENT) {
                // This is equivalent to an immediate satisfy
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
            if(srcKind & OCR_GUID_EVENT) {
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
                self->taskFactories[0]->fcts.registerSignaler(task, src, PD_MSG_FIELD(slot),
                    PD_MSG_FIELD(properties) & DB_PROP_MODE_MASK, true);
            } else if(dstKind & OCR_GUID_EVENT) {
                ocrEvent_t *evt = (ocrEvent_t*)(dest.metaDataPtr);
                ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
                self->eventFactories[0]->fcts[evt->kind].registerSignaler(
                    evt, src, PD_MSG_FIELD(slot), PD_MSG_FIELD(properties) & DB_PROP_MODE_MASK, true);
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
        DPRINTF(DEBUG_LVL_VVERB, "DEP_REGSIGNALER req/resp from 0x%lx for 0x%lx -> 0x%lx\n",
                msg->srcLocation, PD_MSG_FIELD(signaler.guid), PD_MSG_FIELD(dest.guid));
        ocrGuidKind signalerKind, dstKind;
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
            self->eventFactories[0]->fcts[evt->kind].registerSignaler(
                evt, signaler, PD_MSG_FIELD(slot), PD_MSG_FIELD(mode), isAddDep);
        } else if(dstKind == OCR_GUID_EDT) {
            ocrTask_t *edt = (ocrTask_t*)(dest.metaDataPtr);
            ASSERT(edt->fctId == self->taskFactories[0]->factoryId);
            self->taskFactories[0]->fcts.registerSignaler(
                edt, signaler, PD_MSG_FIELD(slot), PD_MSG_FIELD(mode), isAddDep);
        } else {
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
        DPRINTF(DEBUG_LVL_VVERB, "DEP_REGWAITER req/resp from 0x%lx for 0x%lx -> 0x%lx\n",
                msg->srcLocation, PD_MSG_FIELD(dest.guid), PD_MSG_FIELD(waiter.guid));
        ocrGuidKind waiterKind, dstKind;
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(waiter.guid),
            (u64*)(&(PD_MSG_FIELD(waiter.metaDataPtr))), &waiterKind);
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(dest.guid),
            (u64*)(&(PD_MSG_FIELD(dest.metaDataPtr))), &dstKind);

        ocrFatGuid_t waiter = PD_MSG_FIELD(waiter);
        ocrFatGuid_t dest = PD_MSG_FIELD(dest);

        ASSERT(dstKind & OCR_GUID_EVENT); // Waiters can only wait on events
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
        DPRINTF(DEBUG_LVL_VVERB, "DEP_SATISFY from 0x%lx for GUID 0x%lx with 0x%lx\n",
                msg->srcLocation, PD_MSG_FIELD(guid.guid), PD_MSG_FIELD(payload.guid));
        ocrGuidKind dstKind;
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(guid.guid),
            (u64*)(&(PD_MSG_FIELD(guid.metaDataPtr))), &dstKind);

        ocrFatGuid_t dst = PD_MSG_FIELD(guid);
        if(dstKind & OCR_GUID_EVENT) {
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

    case 0: // FIXME: Treat null messages as shutdown, till #134 is fixed
    case PD_MSG_MGT_SHUTDOWN: {
        START_PROFILE(pd_ce_Shutdown);
#define PD_MSG msg
#define PD_TYPE PD_MSG_MGT_SHUTDOWN
        //u32 neighborCount = self->neighborCount;
        ocrPolicyDomainCe_t * cePolicy = (ocrPolicyDomainCe_t *)self;
        if (cePolicy->shutdownMode == false) {
            cePolicy->shutdownMode = true;
        }
        if (msg->srcLocation == self->myLocation && self->myLocation != self->parentLocation) {
            msg->destLocation = self->parentLocation;
            RESULT_ASSERT(self->fcts.sendMessage(self, msg->destLocation, msg, NULL, BLOCKING_SEND_MSG_PROP), ==, 0);
        } else {
            if (msg->type & PD_MSG_REQUEST) {
                // This triggers the shutdown of the machine
                ASSERT(!(msg->type & PD_MSG_RESPONSE));
                ASSERT(msg->srcLocation != self->myLocation);
                if (self->shutdownCode == 0)
                    self->shutdownCode = PD_MSG_FIELD(errorCode);
                cePolicy->shutdownCount++;
                DPRINTF (DEBUG_LVL_VERB, "MSG_SHUTDOWN REQ from Agent 0x%lx; shutdown %u/%u\n",
                    msg->srcLocation, cePolicy->shutdownCount, cePolicy->shutdownMax);
            } else {
                ASSERT(msg->type & PD_MSG_RESPONSE);
                DPRINTF (DEBUG_LVL_VERB, "MSG_SHUTDOWN RESP from Agent 0x%lx; shutdown %u/%u\n",
                    msg->srcLocation, cePolicy->shutdownCount, cePolicy->shutdownMax);
            }

            if (cePolicy->shutdownCount == cePolicy->shutdownMax) {
                self->fcts.stop(self);
                if (self->myLocation != self->parentLocation) {
                    ocrShutdown();
                }
            }
        }
#undef PD_MSG
#undef PD_TYPE
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
        DPRINTF(DEBUG_LVL_WARN, "Unknown message type 0x%x\n",
                msg->type & PD_MSG_TYPE_ONLY);
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
    void* result;
    u64 engineIndex = getEngineIndex(self, self->myLocation);
//TODO: Unlike other datablock allocation contexts, cePdMalloc is not going to get hints, so this part can be stripped/simplified. Maybe use an order value directly hard-coded in this function.
    u64 prescription = PRESCRIPTION;
    u64 allocatorHints;  // Allocator hint
    u64 levelIndex; // "Level" of the memory hierarchy, taken from the "prescription" to pick from the allocators array the allocator to try.
    s8 allocatorIndex;
    do {
        allocatorHints = (prescription & PRESCRIPTION_HINT_MASK) ?
            (OCR_ALLOC_HINT_NONE) :              // If the hint is non-zero, pick this (e.g. for tlsf, tries "remnant" pool.)
            (OCR_ALLOC_HINT_REDUCE_CONTENTION);  // If the hint is zero, pick this (e.g. for tlsf, tries a round-robin-chosen "slice" pool.)
        prescription >>= PRESCRIPTION_HINT_NUMBITS;
        levelIndex = prescription & PRESCRIPTION_LEVEL_MASK;
        prescription >>= PRESCRIPTION_LEVEL_NUMBITS;
        allocatorIndex = self->allocatorIndexLookup[engineIndex*NUM_MEM_LEVELS_SUPPORTED+levelIndex]; // Lookup index of allocator to use for requesting engine (aka agent) at prescribed memory hierarchy level.
        if ((allocatorIndex < 0) ||
            (allocatorIndex >= self->allocatorCount) ||
            (self->allocators[allocatorIndex] == NULL)) continue;  // Skip this allocator if it doesn't exist.
        result = self->allocators[allocatorIndex]->fcts.allocate(self->allocators[allocatorIndex], size, allocatorHints);
#ifdef HAL_FSIM_CE
        if (result) {
            if((u64)result <= CE_MSR_BASE && (u64)result >= BR_CE_BASE) {
                result = (void *)DR_CE_BASE(CHIP_FROM_ID(self->myLocation),
                                            UNIT_FROM_ID(self->myLocation),
                                            BLOCK_FROM_ID(self->myLocation)) +
                                            (u64)(result - BR_CE_BASE);
                u64 check = MAKE_CORE_ID(0,
                                         0,
                                         ((((u64)result >> MAP_CHIP_SHIFT) & ((1ULL<<MAP_CHIP_LEN) - 1)) - 1),
                                         ((((u64)result >> MAP_UNIT_SHIFT) & ((1ULL<<MAP_UNIT_LEN) - 1)) - 2),
                                         ((((u64)result >> MAP_BLOCK_SHIFT) & ((1ULL<<MAP_BLOCK_LEN) - 1)) - 2),
                                         ID_AGENT_CE);
                ASSERT(check==self->myLocation);
            }
            return result;
        }
#endif
        if (result) return result;
    } while (prescription != 0);

    DPRINTF(DEBUG_LVL_WARN, "cePdMalloc returning NULL for size %ld\n", (u64) size);
    return NULL;
}

void cePdFree(ocrPolicyDomain_t *self, void* addr) {
#ifdef HAL_FSIM_CE
    u64 base = DR_CE_BASE(CHIP_FROM_ID(self->myLocation),
                          UNIT_FROM_ID(self->myLocation),
                          BLOCK_FROM_ID(self->myLocation));
    if(((u64)addr < CE_MSR_BASE) || ((base & (u64)addr) == base)) {  // trac #222
        allocatorFreeFunction(addr);
    } else {
        ocrPolicyMsg_t msg;
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_MEM_UNALLOC
        msg.type = PD_MSG_MEM_UNALLOC | PD_MSG_REQUEST;
        msg.destLocation = MAKE_CORE_ID(0,
                                        0,
                                        ((((u64)addr >> MAP_CHIP_SHIFT) & ((1ULL<<MAP_CHIP_LEN) - 1)) - 1),
                                        ((((u64)addr >> MAP_UNIT_SHIFT) & ((1ULL<<MAP_UNIT_LEN) - 1)) - 2),
                                        ((((u64)addr >> MAP_BLOCK_SHIFT) & ((1ULL<<MAP_BLOCK_LEN) - 1)) - 2),
                                        ID_AGENT_CE);
        msg.srcLocation = self->myLocation;
        PD_MSG_FIELD(ptr) = ((void *) addr);
        PD_MSG_FIELD(type) = 0;
        while(self->fcts.sendMessage(self, msg.destLocation, &msg, NULL, 0)) {
           ocrPolicyMsg_t myMsg;
           ocrMsgHandle_t myHandle;
           myHandle.msg = &myMsg;
           ocrMsgHandle_t *handle = &myHandle;
           while(!self->fcts.pollMessage(self, &handle))
               RESULT_ASSERT(self->fcts.processMessage(self, handle->response, true), ==, 0);
        }
#undef PD_MSG
#undef PD_TYPE
    }
#else
    allocatorFreeFunction(addr);
#endif
}

ocrPolicyDomain_t * newPolicyDomainCe(ocrPolicyDomainFactory_t * factory,
#ifdef OCR_ENABLE_STATISTICS
                                      ocrStats_t *statsObject,
#endif
                                      ocrCost_t *costFunction, ocrParamList_t *perInstance) {

    ocrPolicyDomainCe_t * derived = (ocrPolicyDomainCe_t *) runtimeChunkAlloc(sizeof(ocrPolicyDomainCe_t), PERSISTENT_CHUNK);
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
    u64 i;
#ifdef OCR_ENABLE_STATISTICS
    self->statsObject = statsObject;
#endif

    initializePolicyDomainOcr(factory, self, perInstance);
    ocrPolicyDomainCe_t* derived = (ocrPolicyDomainCe_t*) self;

    derived->xeCount = ((paramListPolicyDomainCeInst_t*)perInstance)->xeCount;
    self->neighborCount = ((paramListPolicyDomainCeInst_t*)perInstance)->neighborCount;
    self->allocatorIndexLookup = (s8 *) runtimeChunkAlloc((derived->xeCount+1) * NUM_MEM_LEVELS_SUPPORTED, PERSISTENT_CHUNK);
    for (i = 0; i < ((derived->xeCount+1) * NUM_MEM_LEVELS_SUPPORTED); i++) self->allocatorIndexLookup[i] = -1;

    derived->shutdownCount = 0;
    derived->shutdownMax = 0;
    derived->shutdownMode = false;

    derived->ceCommTakeActive = (bool*)runtimeChunkAlloc(sizeof(bool) *  self->neighborCount, 0);
}

static void destructPolicyDomainFactoryCe(ocrPolicyDomainFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrPolicyDomainFactory_t * newPolicyDomainFactoryCe(ocrParamList_t *perType) {
    ocrPolicyDomainFactory_t* base = (ocrPolicyDomainFactory_t*) runtimeChunkAlloc(sizeof(ocrPolicyDomainFactoryCe_t), NONPERSISTENT_CHUNK);
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
