/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include <string.h>

#include "debug.h"
#include "ocr-policy-domain.h"
#include "policy-domain/fsim/fsim-policy.h"

static ocrPolicyCtx_t * cloneOcrPolicyCtxXE (ocrPolicyCtx_t * ctxIn) {
    ocrPolicyCtxXE_t* ctxOut = checkedMalloc (ctxOut, sizeof(ocrPolicyCtxXE_t));
    memcpy(ctxOut, ctxIn, sizeof(ocrPolicyCtxXE_t));
    return (ocrPolicyCtx_t*) ctxOut;
}

static ocrPolicyCtx_t * cloneOcrPolicyCtxCE (ocrPolicyCtx_t * ctxIn) {
    ocrPolicyCtxCE_t* ctxOut = checkedMalloc (ctxOut, sizeof(ocrPolicyCtxCE_t));
    memcpy(ctxOut, ctxIn, sizeof(ocrPolicyCtxCE_t));
    return (ocrPolicyCtx_t*) ctxOut;
}

static ocrPolicyCtx_t * cloneOcrPolicyCtxMasteredCE (ocrPolicyCtx_t * ctxIn) {
    ocrPolicyCtxMasteredCE_t* ctxOut = checkedMalloc (ctxOut, sizeof(ocrPolicyCtxMasteredCE_t));
    memcpy(ctxOut, ctxIn, sizeof(ocrPolicyCtxMasteredCE_t));
    return (ocrPolicyCtx_t*) ctxOut;
}

static void destructOcrPolicyCtxFSIM ( ocrPolicyCtx_t* self ) {
    free(self);
}

static ocrPolicyCtx_t * instantiateOcrPolicyCtxXE ( ocrPolicyCtxFactory_t *factory, ocrParamList_t *perInstance) {
    ocrPolicyCtxXE_t* derived = checkedMalloc (derived, sizeof(ocrPolicyCtxXE_t));
    ocrPolicyCtx_t * base = (ocrPolicyCtx_t *) derived;
    base->clone = cloneOcrPolicyCtxXE;
    base->destruct = destructOcrPolicyCtxFSIM;
    return (ocrPolicyCtx_t*) derived;
}

static ocrPolicyCtx_t * instantiateOcrPolicyCtxCE ( ocrPolicyCtxFactory_t *factory, ocrParamList_t *perInstance) {
    ocrPolicyCtxCE_t* derived = checkedMalloc (derived, sizeof(ocrPolicyCtxCE_t));
    ocrPolicyCtx_t * base = (ocrPolicyCtx_t *) derived;
    base->clone = cloneOcrPolicyCtxCE;
    base->destruct = destructOcrPolicyCtxFSIM;
    return (ocrPolicyCtx_t*) derived;
}

static ocrPolicyCtx_t * instantiateOcrPolicyCtxMasteredCE ( ocrPolicyCtxFactory_t *factory, ocrParamList_t *perInstance) {
    ocrPolicyCtxMasteredCE_t* derived = checkedMalloc (derived, sizeof(ocrPolicyCtxMasteredCE_t));
    ocrPolicyCtx_t * base = (ocrPolicyCtx_t *) derived;
    base->clone = cloneOcrPolicyCtxMasteredCE;
    base->destruct = destructOcrPolicyCtxFSIM;
    return (ocrPolicyCtx_t*) derived;
}

static void destructOcrPolicyCtxFactoryFSIM ( ocrPolicyCtxFactory_t* self ) {
}

ocrPolicyCtxFactory_t* newPolicyCtxFactoryXE ( ocrParamList_t* params ) {
    ocrPolicyCtxFactoryXE_t* derived = (ocrPolicyCtxFactoryXE_t*) checkedMalloc(derived, sizeof(ocrPolicyCtxFactoryXE_t));
    ocrPolicyCtxFactory_t * base = (ocrPolicyCtxFactory_t *) derived;
    base->instantiate = instantiateOcrPolicyCtxXE;
    base->destruct = destructOcrPolicyCtxFactoryFSIM;
    return base;
}

ocrPolicyCtxFactory_t* newPolicyCtxFactoryCE ( ocrParamList_t* params ) {
    ocrPolicyCtxFactoryCE_t* derived = (ocrPolicyCtxFactoryCE_t*) checkedMalloc(derived, sizeof(ocrPolicyCtxFactoryCE_t));
    ocrPolicyCtxFactory_t * base = (ocrPolicyCtxFactory_t *) derived;
    base->instantiate = instantiateOcrPolicyCtxCE;
    base->destruct = destructOcrPolicyCtxFactoryFSIM;
    return base;
}

ocrPolicyCtxFactory_t* newPolicyCtxFactoryMasteredCE ( ocrParamList_t* params ) {
    ocrPolicyCtxFactoryMasteredCE_t* derived = (ocrPolicyCtxFactoryMasteredCE_t*) checkedMalloc(derived, sizeof(ocrPolicyCtxFactoryMasteredCE_t));
    ocrPolicyCtxFactory_t * base = (ocrPolicyCtxFactory_t *) derived;
    base->instantiate = instantiateOcrPolicyCtxMasteredCE;
    base->destruct = destructOcrPolicyCtxFactoryFSIM;
    return base;
}

static void fsimNonMasteredPolicyDomainStart(ocrPolicyDomain_t * policy) {
    // The PD should have been brought up by now and everything instantiated
    // WARNING: Threads start should be the last thing we do here after
    //          all data-structures have been initialized.
    u64 i = 0;
    u64 workerCount = policy->workerCount;
    u64 computeCount = policy->computeCount;
    u64 schedulerCount = policy->schedulerCount;
    ASSERT(workerCount == computeCount);

    // Start the allocators
    for(i = 0; i < policy->allocatorCount; ++i) {
        policy->allocators[i]->fctPtrs->start(policy->allocators[i], policy);
    }

    // Start schedulers
    for(i = 0; i < schedulerCount; i++) {
        policy->schedulers[i]->fctPtrs->start(policy->schedulers[i], policy);
    }

    //TODO workers could be responsible for starting the underlying target
    // Note: it's important to first logically start all workers.
    // Once they are all up, start the runtime
    for(i = 0; i < workerCount; i++) {
        policy->workers[i]->fctPtrs->start(policy->workers[i], policy);
    }
}

static void fsimMasteredPolicyDomainStart(ocrPolicyDomain_t * policy) {
    // The PD should have been brought up by now and everything instantiated
    // WARNING: Threads start should be the last thing we do here after
    //          all data-structures have been initialized.
    u64 i = 0;
    u64 workerCount = policy->workerCount;
    u64 computeCount = policy->computeCount;
    u64 schedulerCount = policy->schedulerCount;
    ASSERT(workerCount == computeCount);

    // Start the allocators
    for(i = 0; i < policy->allocatorCount; ++i) {
        policy->allocators[i]->fctPtrs->start(policy->allocators[i], policy);
    }

    // Start schedulers
    for(i = 0; i < schedulerCount; i++) {
        policy->schedulers[i]->fctPtrs->start(policy->schedulers[i], policy);
    }

    //TODO workers could be responsible for starting the underlying target
    // Note: it's important to first logically start all workers.
    // Once they are all up, start the runtime
    for(i = 1; i < workerCount; i++) {
        policy->workers[i]->fctPtrs->start(policy->workers[i], policy);
    }

    // Need to associate thread and worker here, as current thread fall-through
    // in user code and may need to know which Worker it is associated to.
    // Handle target '0'
    policy->workers[0]->fctPtrs->start(policy->workers[0], policy);
}

static void fsimPolicyDomainFinish(ocrPolicyDomain_t * policy) {
    u64 i;
    //TODO we should ask the scheduler to finish too
    // Note: As soon as worker '0' is stopped its thread is
    // free to fall-through and continue shutting down the policy domain
    for ( i = 0; i < policy->workerCount; ++i ) {
        policy->workers[i]->fctPtrs->finish(policy->workers[i]);
    }
}

static void fsimNonMasteredPolicyDomainStop(ocrPolicyDomain_t * policy) {
    u64 i;
    for (i = 0; i < policy->workerCount; i++) {
        policy->workers[i]->fctPtrs->stop(policy->workers[i]);
    }
}

static void fsimMasteredPolicyDomainStop(ocrPolicyDomain_t * policy) {
    // In HC, we MUST call stop on the master worker first.
    // The master worker enters its work routine loop and will
    // be unlocked by ocrShutdown

    policy->workers[0]->fctPtrs->stop(policy->workers[0]);
    // WARNING: Do not add code here unless you know what you're doing !!
    // If we are here, it means an EDT called ocrShutdown which
    // logically finished workers and can make thread '0' executes this
    // code before joining the other threads.

    // Thread '0' joins the other (N-1) threads.

    u64 i;
    for (i = 1; i < policy->workerCount; i++) {
        policy->workers[i]->fctPtrs->stop(policy->workers[i]);
    }
}

static void fsimPolicyDomainDestruct(ocrPolicyDomain_t * policy) {
    // Destroying instances
    int i = 0;
    ocrScheduler_t ** schedulers = policy->schedulers;
    for ( i = 0; i < policy->schedulerCount; ++i ) {
        schedulers[i]->fctPtrs->destruct(schedulers[i]);
    }
    ocrWorker_t ** workers = policy->workers;
    for ( i = 0; i < policy->workerCount; ++i ) {
        workers[i]->fctPtrs->destruct(workers[i]);
    }
    ocrCompTarget_t ** computes = policy->computes;
    for ( i = 0; i < policy->computeCount; ++i ) {
        computes[i]->fctPtrs->destruct(computes[i]);
    }
    ocrWorkpile_t ** workpiles = policy->workpiles;
    for ( i = 0; i < policy->workpileCount; ++i ) {
        workpiles[i]->fctPtrs->destruct(workpiles[i]);
    }
    ocrAllocator_t ** allocators = policy->allocators;
    for ( i = 0; i < policy->allocatorCount; ++i ) {
        allocators[i]->fctPtrs->destruct(allocators[i]);
    }
    ocrMemTarget_t ** memories = policy->memories;
    for ( i = 0; i < policy->memoryCount; ++i ) {
        memories[i]->fctPtrs->destruct(memories[i]);
    }
    
    // Destruct factories after instances as the instances
    // rely on a structure in the factory (the function pointers)
    policy->taskFactory->destruct(policy->taskFactory);
    policy->taskTemplateFactory->destruct(policy->taskTemplateFactory);
    policy->dbFactory->destruct(policy->dbFactory);
    policy->eventFactory->destruct(policy->eventFactory);
    policy->lockFactory->destruct(policy->lockFactory);
    policy->atomicFactory->destruct(policy->atomicFactory);

    //Anticipate those to be null-impl for some time
    ASSERT(policy->costFunction == NULL);

    // Finish with those in case destruct implementation needs
    // to releaseGuids or access context for some reasons
    policy->contextFactory->destruct(policy->contextFactory);
    policy->guidProvider->fctPtrs->destruct(policy->guidProvider);

    free(policy);
}

static u8 fsimAllocateDb(ocrPolicyDomain_t *self, ocrGuid_t *guid, void** ptr, u64 size,
                       u16 properties, ocrGuid_t affinity, ocrInDbAllocator_t allocator,
                       ocrPolicyCtx_t *context) {

    // Currently a very simple model of just going through all allocators
    u64 i;
    void* result = NULL;
    for(i=0; i < self->allocatorCount; ++i) {
        result = self->allocators[i]->fctPtrs->allocate(self->allocators[i],
                                                        size);
        if(result) break;
    }
    // TODO: return error code. Requires our own errno to be clean
    ocrDataBlock_t *block = self->dbFactory->instantiate(self->dbFactory,
                                                         self->allocators[i]->guid, self->guid,
                                                         size, result, properties, NULL);
    *ptr = result;
    *guid = block->guid;
    return 0;
}

static u8 fsimCreateEdt(ocrPolicyDomain_t *self, ocrGuid_t *guid,
                      ocrTaskTemplate_t * edtTemplate, u32 paramc, u64* paramv,
                      u32 depc, u16 properties, ocrGuid_t affinity,
                      ocrGuid_t * outputEvent, ocrPolicyCtx_t *context) {

    ocrTask_t * base = self->taskFactory->instantiate(self->taskFactory, edtTemplate, paramc,
                                                      paramv, depc, properties, affinity,
                                                      outputEvent, NULL);
    // Check if the edt is ready to be scheduled
    if (base->depc == 0) {
        base->fctPtrs->schedule(base);
    }
    *guid = base->guid;
    return 0;
}

static u8 fsimCreateEdtTemplate(ocrPolicyDomain_t *self, ocrGuid_t *guid,
                                ocrEdt_t func, u32 paramc, u32 depc, const char* funcName,
                                ocrPolicyCtx_t *context) {


    ocrTaskTemplate_t *base = self->taskTemplateFactory->instantiate(self->taskTemplateFactory,
                                                                     func, paramc, depc, funcName, NULL);
    *guid = base->guid;
    return 0;
}

static u8 fsimCreateEvent(ocrPolicyDomain_t *self, ocrGuid_t *guid,
                        ocrEventTypes_t type, bool takesArg, ocrPolicyCtx_t *context) {


    ocrEvent_t *base = self->eventFactory->instantiate(self->eventFactory,
                                                              type, takesArg, NULL);
    *guid = base->guid;
    return 0;
}

static u8 fsimWaitForEvent(ocrPolicyDomain_t *self, ocrGuid_t workerGuid,
                       ocrGuid_t yieldingEdtGuid, ocrGuid_t eventToYieldForGuid,
                       ocrGuid_t * returnGuid, ocrPolicyCtx_t *context) {
    return self->schedulers[0]->fctPtrs->yield(self->schedulers[0], workerGuid, yieldingEdtGuid, eventToYieldForGuid, returnGuid, context);
}

static ocrLock_t* fsimGetLock(ocrPolicyDomain_t *self, ocrPolicyCtx_t *context) {
    return self->lockFactory->instantiate(self->lockFactory, NULL);
}

static ocrAtomic64_t* fsimGetAtomic64(ocrPolicyDomain_t *self, ocrPolicyCtx_t *context) {
    return self->atomicFactory->instantiate(self->atomicFactory, NULL);
}

static ocrPolicyCtx_t* fsimGetContext(ocrPolicyDomain_t *self) {
    return self->contextFactory->instantiate(self->contextFactory, NULL);
}

static void fsimInform(ocrPolicyDomain_t *self, ocrGuid_t obj, const ocrPolicyCtx_t *context) {
    if(context->type == PD_MSG_GUID_REL) {
        self->guidProvider->fctPtrs->releaseGuid(self->guidProvider, obj);
        return;
    }
    //TODO not yet implemented
    ASSERT(false);
}

static u8 fsimGetGuid(ocrPolicyDomain_t *self, ocrGuid_t *guid, u64 val, ocrGuidKind type,
                    ocrPolicyCtx_t *ctx) {
    self->guidProvider->fctPtrs->getGuid(self->guidProvider, guid, val, type);
    return 0;
}

static u8 fsimGetInfoForGuid(ocrPolicyDomain_t *self, ocrGuid_t guid, u64* val,
                           ocrGuidKind* type, ocrPolicyCtx_t *ctx) {
    self->guidProvider->fctPtrs->getVal(self->guidProvider, guid, val, type);
    return 0;
}

static u8 fsimTakeEdt(ocrPolicyDomain_t *self, ocrCost_t *cost, u32 *count,
                ocrGuid_t *edts, ocrPolicyCtx_t *context) {
    self->schedulers[0]->fctPtrs->takeEdt(self->schedulers[0], cost, count, edts, context);
    // When takeEdt is successful, it means there either was
    // work in the current worker's workpile, or that the scheduler
    // did work-stealing across workpiles.
    return 0;
}


static u8 xeGiveEdt(ocrPolicyDomain_t *self, u32 count, ocrGuid_t *edts, ocrPolicyCtx_t *context) {
    if ( PD_MSG_EDT_GIVE == context->type && PD_MSG_INJECT_EDT == context->type) {
        self->schedulers[0]->fctPtrs->giveEdt(self->schedulers[0], count, edts, context);
    } else {
        assert ( PD_MSG_GIVE_ME_WORK == context->type && "XE can only be forwarding with PD_MSG_GIVE_ME_WORK if not giving by PD_MSG_EDT_GIVE and PD_MSG_INJECT");
        // TODO sagnak, XE's first neighbour is its parent
        ocrPolicyDomain_t* parent = self->neighbors[0];
        parent->giveEdt(parent, count, edts, context);
    }
    return 0;
}

static u8 fsimGiveEdt(ocrPolicyDomain_t *self, u32 count, ocrGuid_t *edts, ocrPolicyCtx_t *context) {
    self->schedulers[0]->fctPtrs->giveEdt(self->schedulers[0], count, edts, context);
    return 0;
}

ocrPolicyDomain_t * newPolicyDomainXE(ocrPolicyDomainFactory_t * policy,
                                      u64 schedulerCount, u64 workerCount, u64 computeCount,
                                      u64 workpileCount, u64 allocatorCount, u64 memoryCount,
                                      ocrTaskFactory_t *taskFactory, ocrTaskTemplateFactory_t *taskTemplateFactory,
                                      ocrDataBlockFactory_t *dbFactory, ocrEventFactory_t *eventFactory,
                                      ocrPolicyCtxFactory_t *contextFactory, ocrGuidProvider_t *guidProvider,
                                      ocrLockFactory_t* lockFactory, ocrAtomic64Factory_t* atomicFactory,
                                      ocrQueueFactory_t *queueFactory,
#ifdef OCR_ENABLE_STATISTICS
                                      ocrStats_t *statsObject,
#endif
                                      ocrCost_t *costFunction, ocrParamList_t *perInstance) {

    ocrPolicyDomainXE_t * derived = (ocrPolicyDomainXE_t *) checkedMalloc(policy, sizeof(ocrPolicyDomainXE_t));
    ocrPolicyDomain_t * base = (ocrPolicyDomain_t *) derived;

    base->schedulerCount = schedulerCount;
    ASSERT(schedulerCount == 1); // Simplest PD implementation
    base->workerCount = workerCount;
    base->computeCount = computeCount;
    base->workpileCount = workpileCount;
    base->allocatorCount = allocatorCount;
    base->memoryCount = memoryCount;

    base->taskFactory = taskFactory;
    base->taskTemplateFactory = taskTemplateFactory;
    base->dbFactory = dbFactory;
    base->eventFactory = eventFactory;
    base->contextFactory = contextFactory;
    base->guidProvider = guidProvider;
    base->lockFactory = lockFactory;
    base->atomicFactory = atomicFactory;
    base->costFunction = costFunction;

    base->destruct = fsimPolicyDomainDestruct;
    base->start = fsimNonMasteredPolicyDomainStart;
    base->stop = fsimNonMasteredPolicyDomainStop;
    base->finish = fsimPolicyDomainFinish;
    base->allocateDb = fsimAllocateDb;
    base->createEdt = fsimCreateEdt;
    base->createEdtTemplate = fsimCreateEdtTemplate;
    base->createEvent = fsimCreateEvent;
    base->inform = fsimInform;
    base->getGuid = fsimGetGuid;
    base->getInfoForGuid = fsimGetInfoForGuid;
    base->waitForEvent = fsimWaitForEvent;
    base->takeEdt = fsimTakeEdt;
    base->takeDb = NULL;
    base->giveEdt = xeGiveEdt;
    base->giveDb = NULL;
    base->processResponse = NULL;
    base->getLock = fsimGetLock;
    base->getAtomic64 = fsimGetAtomic64;
    base->getContext = fsimGetContext;

    base->neighbors = NULL;
    base->neighborCount = 0;

    //TODO populated by ini file factories. Need setters or something ?
    base->schedulers = NULL;
    base->workers = NULL;
    base->computes = NULL;
    base->workpiles = NULL;
    base->allocators = NULL;
    base->memories = NULL;

    base->guid = UNINITIALIZED_GUID;
    guidify(base, (u64)base, &(base->guid), OCR_GUID_POLICY);
    return base;
}

ocrPolicyDomain_t * newPolicyDomainCE(ocrPolicyDomainFactory_t * policy,
                                      u64 schedulerCount, u64 workerCount, u64 computeCount,
                                      u64 workpileCount, u64 allocatorCount, u64 memoryCount,
                                      ocrTaskFactory_t *taskFactory, ocrTaskTemplateFactory_t *taskTemplateFactory,
                                      ocrDataBlockFactory_t *dbFactory, ocrEventFactory_t *eventFactory,
                                      ocrPolicyCtxFactory_t *contextFactory, ocrGuidProvider_t *guidProvider,
                                      ocrLockFactory_t* lockFactory, ocrAtomic64Factory_t* atomicFactory,
                                      ocrQueueFactory_t *queueFactory,
#ifdef OCR_ENABLE_STATISTICS
                                      ocrStats_t *statsObject,
#endif
                                      ocrCost_t *costFunction, ocrParamList_t *perInstance) {

    ocrPolicyDomainCE_t * derived = (ocrPolicyDomainCE_t *) checkedMalloc(policy, sizeof(ocrPolicyDomainCE_t));
    ocrPolicyDomain_t * base = (ocrPolicyDomain_t *) derived;

    base->schedulerCount = schedulerCount;
    ASSERT(schedulerCount == 1); // Simplest PD implementation
    base->workerCount = workerCount;
    base->computeCount = computeCount;
    base->workpileCount = workpileCount;
    base->allocatorCount = allocatorCount;
    base->memoryCount = memoryCount;

    base->taskFactory = taskFactory;
    base->taskTemplateFactory = taskTemplateFactory;
    base->dbFactory = dbFactory;
    base->eventFactory = eventFactory;
    base->contextFactory = contextFactory;
    base->guidProvider = guidProvider;
    base->lockFactory = lockFactory;
    base->atomicFactory = atomicFactory;
    base->costFunction = costFunction;

    base->destruct = fsimPolicyDomainDestruct;
    base->start = fsimNonMasteredPolicyDomainStart;
    base->stop = fsimNonMasteredPolicyDomainStop;
    base->finish = fsimPolicyDomainFinish;
    base->allocateDb = fsimAllocateDb;
    base->createEdt = fsimCreateEdt;
    base->createEdtTemplate = fsimCreateEdtTemplate;
    base->createEvent = fsimCreateEvent;
    base->inform = fsimInform;
    base->getGuid = fsimGetGuid;
    base->getInfoForGuid = fsimGetInfoForGuid;
    base->waitForEvent = fsimWaitForEvent;
    base->takeEdt = fsimTakeEdt;
    base->takeDb = NULL;
    base->giveEdt = fsimGiveEdt;
    base->giveDb = NULL;
    base->processResponse = NULL;
    base->getLock = fsimGetLock;
    base->getAtomic64 = fsimGetAtomic64;
    base->getContext = fsimGetContext;

    base->neighbors = NULL;
    base->neighborCount = 0;

    //TODO populated by ini file factories. Need setters or something ?
    base->schedulers = NULL;
    base->workers = NULL;
    base->computes = NULL;
    base->workpiles = NULL;
    base->allocators = NULL;
    base->memories = NULL;

    base->guid = UNINITIALIZED_GUID;
    guidify(base, (u64)base, &(base->guid), OCR_GUID_POLICY);
    return base;
}

ocrPolicyDomain_t * newPolicyDomainMasteredCE(ocrPolicyDomainFactory_t * policy,
                                              u64 schedulerCount, u64 workerCount, u64 computeCount,
                                              u64 workpileCount, u64 allocatorCount, u64 memoryCount,
                                              ocrTaskFactory_t *taskFactory, ocrTaskTemplateFactory_t *taskTemplateFactory,
                                              ocrDataBlockFactory_t *dbFactory, ocrEventFactory_t *eventFactory,
                                              ocrPolicyCtxFactory_t *contextFactory, ocrGuidProvider_t *guidProvider,
                                              ocrLockFactory_t* lockFactory, ocrAtomic64Factory_t* atomicFactory,
                                              ocrQueueFactory_t* queueFactory,
#ifdef OCR_ENABLE_STATISTICS
                                              ocrStats_t* statsObject,
#endif
                                              ocrCost_t *costFunction, ocrParamList_t *perInstance) {

    ocrPolicyDomainMasteredCE_t * derived = (ocrPolicyDomainMasteredCE_t *) checkedMalloc(policy, sizeof(ocrPolicyDomainMasteredCE_t));
    ocrPolicyDomain_t * base = (ocrPolicyDomain_t *) derived;

    base->schedulerCount = schedulerCount;
    ASSERT(schedulerCount == 1); // Simplest PD implementation
    base->workerCount = workerCount;
    base->computeCount = computeCount;
    base->workpileCount = workpileCount;
    base->allocatorCount = allocatorCount;
    base->memoryCount = memoryCount;

    base->taskFactory = taskFactory;
    base->taskTemplateFactory = taskTemplateFactory;
    base->dbFactory = dbFactory;
    base->eventFactory = eventFactory;
    base->contextFactory = contextFactory;
    base->guidProvider = guidProvider;
    base->lockFactory = lockFactory;
    base->atomicFactory = atomicFactory;
    base->costFunction = costFunction;

    base->destruct = fsimPolicyDomainDestruct;
    base->start = fsimMasteredPolicyDomainStart;
    base->stop = fsimMasteredPolicyDomainStop;
    base->finish = fsimPolicyDomainFinish;
    base->allocateDb = fsimAllocateDb;
    base->createEdt = fsimCreateEdt;
    base->createEdtTemplate = fsimCreateEdtTemplate;
    base->createEvent = fsimCreateEvent;
    base->inform = fsimInform;
    base->getGuid = fsimGetGuid;
    base->getInfoForGuid = fsimGetInfoForGuid;
    base->waitForEvent = fsimWaitForEvent;
    base->takeEdt = fsimTakeEdt;
    base->takeDb = NULL;
    base->giveEdt = fsimGiveEdt;
    base->giveDb = NULL;
    base->processResponse = NULL;
    base->getLock = fsimGetLock;
    base->getAtomic64 = fsimGetAtomic64;
    base->getContext = fsimGetContext;

    base->neighbors = NULL;
    base->neighborCount = 0;

    //TODO populated by ini file factories. Need setters or something ?
    base->schedulers = NULL;
    base->workers = NULL;
    base->computes = NULL;
    base->workpiles = NULL;
    base->allocators = NULL;
    base->memories = NULL;

    base->guid = UNINITIALIZED_GUID;
    guidify(base, (u64)base, &(base->guid), OCR_GUID_POLICY);
    return base;
}

static void destructPolicyDomainFactoryFSIM(ocrPolicyDomainFactory_t * factory) {
    free(factory);
}

ocrPolicyDomainFactory_t * newPolicyDomainFactoryXE(ocrParamList_t *perType) {
    ocrPolicyDomainFactoryXE_t* derived = (ocrPolicyDomainFactoryXE_t*) checkedMalloc(derived, sizeof(ocrPolicyDomainFactoryXE_t));
    ocrPolicyDomainFactory_t* base = (ocrPolicyDomainFactory_t*) derived;
    base->instantiate = newPolicyDomainXE;
    base->destruct =  destructPolicyDomainFactoryFSIM;
    return base;
}

ocrPolicyDomainFactory_t * newPolicyDomainFactoryCE(ocrParamList_t *perType) {
    ocrPolicyDomainFactoryCE_t* derived = (ocrPolicyDomainFactoryCE_t*) checkedMalloc(derived, sizeof(ocrPolicyDomainFactoryCE_t));
    ocrPolicyDomainFactory_t* base = (ocrPolicyDomainFactory_t*) derived;
    base->instantiate = newPolicyDomainCE;
    base->destruct =  destructPolicyDomainFactoryFSIM;
    return base;
}

ocrPolicyDomainFactory_t * newPolicyDomainFactoryMasteredCE(ocrParamList_t *perType) {
    ocrPolicyDomainFactoryMasteredCE_t* derived = (ocrPolicyDomainFactoryMasteredCE_t*) checkedMalloc(derived, sizeof(ocrPolicyDomainFactoryMasteredCE_t));
    ocrPolicyDomainFactory_t* base = (ocrPolicyDomainFactory_t*) derived;
    base->instantiate = newPolicyDomainMasteredCE;
    base->destruct =  destructPolicyDomainFactoryFSIM;
    return base;
}
