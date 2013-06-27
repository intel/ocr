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

void hc_policy_domain_start(ocrPolicyDomain_t * policy) {
    //TODO this is still the old implementation

    // Create Task and Event Factories
    policy->taskFactory = newTaskFactoryHc(NULL);
    policy->eventFactory = newEventFactoryHc(NULL);

    // WARNING: Threads start should be the last thing we do here after
    //          all data-structures have been initialized.
    u64 i = 0;
    u64 workerCount = policy->workerCount;
    u64 computeCount = policy->computeCount;
    // Only start (N-1) workers as worker '0' is the current thread.
    for(i = 1; i < workerCount; i++) {
        policy->workers[i]->fctPtrs->start(policy->workers[i]);

    }
    // Only start (N-1) threads as thread '0' is the current thread.
    for(i = 1; i < computeCount; i++) {
        policy->computes[i]->fctPtrs->start(policy->computes[i]);
    }
    // Handle thread '0'
    policy->workers[0]->fctPtrs->start(policy->workers[0]);
    // Need to associate thread and worker here, as current thread fall-through
    // in user code and may need to know which Worker it is associated to.
    associate_comp_platform_and_worker(policy->workers[0]);
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
    // If we are here, it means a codelet called ocrFinish which
    // logically stopped workers and can make thread '0' executes this
    // code before joining the other threads.

    // Thread '0' joins the other (N-1) threads.
    u64 i;
    for (i = 1; i < policy->computeCount; i++) {
        policy->computes[i]->fctPtrs->stop(policy->computes[i]);
    }
}

void hcPolicyDomainDestruct(ocrPolicyDomain_t * policy) {
    policy->taskFactory->destruct(policy->taskFactory);
    policy->taskTemplateFactory->destruct(policy->taskTemplateFactory);
    policy->dbFactory->destruct(policy->dbFactory);
    policy->eventFactory->destruct(policy->eventFactory);
    policy->contextFactory->destruct(policy->contextFactory);

    //Anticipate those to be null-impl for some time
    ASSERT(policy->lockFactory == NULL);
    ASSERT(policy->atomicFactory == NULL);
    ASSERT(policy->costFunction == NULL);

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
    policy->guidProvider->fctPtrs->destruct(policy->guidProvider);
    // Simple hc policies don't have neighbors
    ASSERT(policy->neighbors == NULL);
}

// Mapping function many-to-one to map a set of schedulers to a policy instance
static void hc_ocr_module_map_schedulers_to_policy (ocrMappable_t * self_module, ocrMappableKind kind,
                                             u64 nb_instances, ocrMappable_t ** ptr_instances) {
    // Checking mapping conforms to what we're expecting in this implementation
    assert(kind == OCR_SCHEDULER);

    ocrPolicyDomain_t * policy = (ocrPolicyDomain_t *) self_module;
    int i = 0;
    for ( i = 0; i < nb_instances; ++i ) {
        ocrScheduler_t* scheduler = (ocrScheduler_t*)ptr_instances[i];
        scheduler->domain = policy;
    }
}

ocrTask_t* (*instantiate)(struct _ocrTaskFactory_t * factory, ocrTaskTemplate_t * edtTemplate,
    u64 * params, void ** paramv, u16 properties, ocrGuid_t * outputEvent);

u8 hcCreateEdt(ocrPolicyDomain_t *self, ocrGuid_t *guid,
     ocrTaskTemplate_t * edtTemplate, u64 * params, void ** paramv, 
     u16 properties, ocrGuid_t * outputEvent, ocrHint_t *hint, ocrPolicyCtx_t *context) {
    //TODO what does context is supposed to tell me ?
    //TODO would it make sense to trickle down 'self' in instantiate ?
    ocrTask_t * base = self->taskFactory->instantiate(self->taskFactory, edtTemplate, params, paramv, properties, outputEvent);
    *guid = base->guid;
    return 0;
}

ocrPolicyDomain_t * newPolicyDomainHc(ocrPolicyDomainFactory_t * policy, void * configuration,
        u64 schedulerCount, u64 workerCount, u64 computeCount,
        u64 workpileCount, u64 allocatorCount, u64 memoryCount,
        ocrTaskFactory_t *taskFactory, ocrTaskTemplateFactory_t *taskTemplateFactory, 
        ocrDataBlockFactory_t *dbFactory, ocrEventFactory_t *eventFactory, 
        ocrPolicyCtxFactory_t *contextFactory, ocrCost_t *costFunction ) {
    
    //TODO missing guidProvider 
    ocrPolicyDomainHc_t * derived = (ocrPolicyDomainHc_t *) checkedMalloc(policy, sizeof(ocrPolicyDomainHc_t));
    ocrPolicyDomain_t * base = (ocrPolicyDomain_t *) derived;

    ocrMappable_t * module_base = (ocrMappable_t *) policy;
    module_base->mapFct = hc_ocr_module_map_schedulers_to_policy;

    base->schedulerCount = schedulerCount;
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

    base->destruct = hcPolicyDomainDestruct;
    base->start = hc_policy_domain_start;
    base->stop = hcPolicyDomainStop;
    base->finish = hcPolicyDomainFinish;
    base->allocateDb = NULL;
    base->createEdt = hcCreateEdt;
    base->inform = NULL;
    base->getGuid = NULL;
    base->getInfoForGuid = NULL;
    base->takeEdt = NULL;
    base->takeDb = NULL;
    base->giveEdt = NULL;
    base->giveDb = NULL;
    base->processResponse = NULL;
    base->getLock = NULL;
    base->getAtomic64 = NULL;

    // no inter-policy domain for simple HC
    base->neighbors = NULL;
    
    //TODO populated by ini file factories. Need setters or something ?
    base->schedulers = NULL;
    base->workers = NULL;
    base->computes = NULL;
    base->workpiles = NULL;
    base->allocators = NULL;
    base->memories = NULL;

    // TODO Once we have a handle on a guidProvider get one for the PD
    base->guid = UNINITIALIZED_GUID;
    guidify(getCurrentPD(), (u64)base, &(base->guid), OCR_GUID_POLICY);
    return base;
}

void destructPolicyDomainHc(ocrPolicyDomainFactory_t * factory) {
    // nothing to do
}

ocrPolicyDomainFactory_t * newPolicyDomainFactoryHc(ocrParamList_t *perType) {
    ocrPolicyDomainHcFactory_t* derived = (ocrPolicyDomainHcFactory_t*) checkedMalloc(derived, sizeof(ocrPolicyDomainHcFactory_t));
    ocrPolicyDomainFactory_t* base = (ocrPolicyDomainFactory_t*) derived;
    base->instantiate = newPolicyDomainHc;
    base->destruct =  destructPolicyDomainHc;
    return base;
}

