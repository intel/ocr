/* Copyright (c) 2012, Rice University

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1.  Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   2.  Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.
   3.  Neither the name of Intel Corporation
   nor the names of its contributors may be used to endorse or
   promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "ocr-macros.h"
#include "hc.h"
#include "ocr-policy-domain.h"
#include "debug.h"

void destructOcrPolicyCtxFactoryHC ( ocrPolicyCtxFactory_t* self ) {
}

ocrPolicyCtx_t * instantiateOcrPolicyCtxFactoryHC ( ocrPolicyCtxFactory_t *factory ) {
    ocrPolicyCtxHC_t* derived = checkedMalloc (derived, sizeof(ocrPolicyCtxHC_t));
    return (ocrPolicyCtx_t*) derived;
}

ocrPolicyCtxFactory_t* newPolicyContextFactoryHC ( ocrParamList_t* params ) {
    ocrPolicyCtxFactoryHC_t* derived = (ocrPolicyCtxFactoryHC_t*) checkedMalloc(derived, sizeof(ocrPolicyCtxFactoryHC_t));
    ocrPolicyCtxFactory_t * base = (ocrPolicyCtxFactory_t *) derived;
    base->instantiate = instantiateOcrPolicyCtxFactoryHC;
    base->destruct = destructOcrPolicyCtxFactoryHC;
    return base;
}

static void hcPolicyDomainStart(ocrPolicyDomain_t * policy) {
    // The PD should have been brought up by now and everything instantiated
    // WARNING: Threads start should be the last thing we do here after
    //          all data-structures have been initialized.
    u64 i = 0;
    u64 workerCount = policy->workerCount;
    u64 computeCount = policy->computeCount;
    u64 schedulerCount = policy->schedulerCount;
    ASSERT(workerCount == computeCount);

    // Start schedulers
    for(i = 0; i < schedulerCount; i++) {
        policy->schedulers[i]->fctPtrs->start(policy->schedulers[i], policy);
    }

    //TODO workers could be responsible for starting the underlying target
    // Note: it's important to first logically start all workers.
    // Once they are all up, start the runtime
    // Only start (N-1) workers as worker '0' is the current thread.
    for(i = 1; i < workerCount; i++) {
        policy->workers[i]->fctPtrs->start(policy->workers[i], policy);
    }

    // Need to associate thread and worker here, as current thread fall-through
    // in user code and may need to know which Worker it is associated to.
    // Handle target '0'
    policy->workers[0]->fctPtrs->start(policy->workers[0], policy);
}


static void hcPolicyDomainFinish(ocrPolicyDomain_t * policy) {
    // Note: As soon as worker '0' is stopped its thread is
    // free to fall-through in ocr_finalize() (see warning there)
    u64 i;
    for ( i = 0; i < policy->workerCount; ++i ) {
        policy->workers[i]->fctPtrs->stop(policy->workers[i]);
    }
}

static void hcPolicyDomainStop(ocrPolicyDomain_t * policy) {
    // WARNING: Do not add code here unless you know what you're doing !!
    // If we are here, it means an EDT called ocrFinish which
    // logically stopped workers and can make thread '0' executes this
    // code before joining the other threads.

    // Thread '0' joins the other (N-1) threads.
    u64 i;
    for (i = 1; i < policy->computeCount; i++) {
        policy->computes[i]->fctPtrs->stop(policy->computes[i]);
    }
}

static void hcPolicyDomainDestruct(ocrPolicyDomain_t * policy) {
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
    for ( i = 0; i < policy->workerCount; ++i ) {
        computes[i]->fctPtrs->destruct(computes[i]);
    }
    ocrWorkpile_t ** workpiles = policy->workpiles;
    for ( i = 0; i < policy->workerCount; ++i ) {
        workpiles[i]->fctPtrs->destruct(workpiles[i]);
    }
    ocrAllocator_t ** allocators = policy->allocators;
    for ( i = 0; i < policy->workerCount; ++i ) {
        allocators[i]->fctPtrs->destruct(allocators[i]);
    }
    ocrMemTarget_t ** memories = policy->memories;
    for ( i = 0; i < policy->workerCount; ++i ) {
        memories[i]->fctPtrs->destruct(memories[i]);
    }

    // Simple hc policies don't have neighbors
    ASSERT(policy->neighbors == NULL);

    // Destruct factories after instances as the instances
    // rely on a structure in the factory (the function pointers)
    policy->taskFactory->destruct(policy->taskFactory);
    policy->taskTemplateFactory->destruct(policy->taskTemplateFactory);
    policy->dbFactory->destruct(policy->dbFactory);
    policy->eventFactory->destruct(policy->eventFactory);

    //Anticipate those to be null-impl for some time
    ASSERT(policy->lockFactory == NULL);
    ASSERT(policy->atomicFactory == NULL);
    ASSERT(policy->costFunction == NULL);

    // Finish with those in case destruct implementation needs
    // to releaseGuids or access context for some reasons
    policy->contextFactory->destruct(policy->contextFactory);
    policy->guidProvider->fctPtrs->destruct(policy->guidProvider);

    free(policy);
}

// Mapping function many-to-one to map a set of schedulers to a policy instance
static void hcOcrModuleMapSchedulersToPolicy (ocrMappable_t * self, ocrMappableKind kind,
                                              u64 instanceCount, ocrMappable_t ** instances) {
    // Checking mapping conforms to what we're expecting in this implementation
    ASSERT(kind == OCR_SCHEDULER);

//    ocrPolicyDomain_t * policy = (ocrPolicyDomain_t *) self;
//    int i = 0;
    // TODO: Re-add?
    /*
    for ( i = 0; i < instanceCount; ++i ) {
        ocrScheduler_t* scheduler = (ocrScheduler_t*)instances[i];
        scheduler->domain = policy;
    }
    */
}

static u8 hcAllocateDb(ocrPolicyDomain_t *self, ocrGuid_t *guid, void** ptr, u64 size,
                       u16 properties, ocrGuid_t affinity, ocrInDbAllocator_t allocator,
                       ocrPolicyCtx_t *context) {

    // Currently a very simple model of just going through all allocators
    u64 i;
    void* result;
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

static u8 hcCreateEdt(ocrPolicyDomain_t *self, ocrGuid_t *guid,
               ocrTaskTemplate_t * edtTemplate, u32 paramc, u64* paramv,
               u32 depc, u16 properties, ocrGuid_t affinity,
               ocrGuid_t * outputEvent, ocrPolicyCtx_t *context) {
    //TODO what does context is supposed to tell me ?
    // REC: The context tells you who is creating the EDT for example
    // It can also be extended to contain information (in inform() for ex)
    //TODO would it make sense to trickle down 'self' in instantiate ?
    // REC: You have that in context
    ocrTask_t * base = self->taskFactory->instantiate(self->taskFactory, edtTemplate, paramc,
                                                      paramv, depc, properties, affinity,
                                                      outputEvent, NULL);
    *guid = base->guid;
    return 0;
}

static ocrLock_t* hcGetLock(ocrPolicyDomain_t *self, ocrPolicyCtx_t *context) {
    return self->lockFactory->instantiate(self->lockFactory, NULL);
}

static ocrAtomic64_t* hcGetAtomic64(ocrPolicyDomain_t *self, ocrPolicyCtx_t *context) {
    return self->atomicFactory->instantiate(self->atomicFactory, NULL);
}

static void hcInform(ocrPolicyDomain_t *self, ocrGuid_t obj, const ocrPolicyCtx_t *context) {
    if(context->type == PD_MSG_GUID_REL) {
        self->guidProvider->fctPtrs->releaseGuid(self->guidProvider, obj);
        return;
    }
    //TODO not yet implemented
    ASSERT(false);
}

static u8 hcGetGuid(ocrPolicyDomain_t *self, ocrGuid_t *guid, u64 val, ocrGuidKind type,
                    ocrPolicyCtx_t *ctx) {
    self->guidProvider->fctPtrs->getGuid(self->guidProvider, guid, val, type);
    return 0;
}

static u8 hcGetInfoForGuid(ocrPolicyDomain_t *self, ocrGuid_t guid, u64* val,
                           ocrGuidKind* type, ocrPolicyCtx_t *ctx) {
    self->guidProvider->fctPtrs->getVal(self->guidProvider, guid, val, type);
    return 0;
}

static u8 hcTakeEdt(ocrPolicyDomain_t *self, ocrCost_t *cost, u32 *count,
                ocrGuid_t *edts, ocrPolicyCtx_t *context) {
    self->schedulers[0]->fctPtrs->takeEdt(self->schedulers[0], cost, count, edts, context);
    // When takeEdt is successful, it means there either was
    // work in the current worker's workpile, or that the scheduler
    // did work-stealing across workpiles.
    return 0;
}

static u8 hcGiveEdt(ocrPolicyDomain_t *self, u32 count, ocrGuid_t *edts, ocrPolicyCtx_t *context) {
    self->schedulers[0]->fctPtrs->giveEdt(self->schedulers[0], count, edts, context);
    return 0;
}


ocrPolicyDomain_t * newPolicyDomainHc(ocrPolicyDomainFactory_t * policy,
                                      u64 schedulerCount, u64 workerCount, u64 computeCount,
                                      u64 workpileCount, u64 allocatorCount, u64 memoryCount,
                                      ocrTaskFactory_t *taskFactory, ocrTaskTemplateFactory_t *taskTemplateFactory,
                                      ocrDataBlockFactory_t *dbFactory, ocrEventFactory_t *eventFactory,
                                      ocrPolicyCtxFactory_t *contextFactory, ocrGuidProvider_t *guidProvider,
                                      ocrLockFactory_t* lockFactory, ocrAtomic64Factory_t* atomicFactory,
                                      ocrCost_t *costFunction, ocrParamList_t *perInstance) {

    //TODO missing guidProvider
    ocrPolicyDomainHc_t * derived = (ocrPolicyDomainHc_t *) checkedMalloc(policy, sizeof(ocrPolicyDomainHc_t));
    ocrPolicyDomain_t * base = (ocrPolicyDomain_t *) derived;

    ocrMappable_t * moduleBase = (ocrMappable_t *) policy;
    moduleBase->mapFct = hcOcrModuleMapSchedulersToPolicy;

    base->schedulerCount = schedulerCount;
    ASSERT(schedulerCount == 1); // Simplest HC PD implementation
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

    base->destruct = hcPolicyDomainDestruct;
    base->start = hcPolicyDomainStart;
    base->stop = hcPolicyDomainStop;
    base->finish = hcPolicyDomainFinish;
    base->allocateDb = hcAllocateDb;
    base->createEdt = hcCreateEdt;
    base->inform = hcInform;
    base->getGuid = hcGetGuid;
    base->getInfoForGuid = hcGetInfoForGuid;
    base->takeEdt = hcTakeEdt;
    base->takeDb = NULL;
    base->giveEdt = hcGiveEdt;
    base->giveDb = NULL;
    base->processResponse = NULL;
    base->getLock = hcGetLock;
    base->getAtomic64 = hcGetAtomic64;

    // no inter-policy domain for simple HC
    base->neighbors = NULL;

    //TODO populated by ini file factories. Need setters or something ?
    base->guidProvider = NULL;
    base->schedulers = NULL;
    base->workers = NULL;
    base->computes = NULL;
    base->workpiles = NULL;
    base->allocators = NULL;
    base->memories = NULL;

    base->guid = UNINITIALIZED_GUID;
    base->guidProvider->fctPtrs->getGuid(base->guidProvider, &(base->guid),
                                         (u64)base, OCR_GUID_POLICY);
    return base;
}

static void destructPolicyDomainFactoryHc(ocrPolicyDomainFactory_t * factory) {
    free(factory);
}

ocrPolicyDomainFactory_t * newPolicyDomainFactoryHc(ocrParamList_t *perType) {
    ocrPolicyDomainHcFactory_t* derived = (ocrPolicyDomainHcFactory_t*) checkedMalloc(derived, sizeof(ocrPolicyDomainHcFactory_t));
    ocrPolicyDomainFactory_t* base = (ocrPolicyDomainFactory_t*) derived;
    base->instantiate = newPolicyDomainHc;
    base->destruct =  destructPolicyDomainFactoryHc;
    return base;
}
