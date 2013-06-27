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
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "ocr-policy-domain.h"

ocrPolicyDomainFactory_t * newPolicyDomainFactoryHcPlaced(ocrParamList_t *perType) {
	//TODO
	return NULL;
}

ocrPolicyDomainFactory_t * newPolicyDomainFactoryHcLeafPlace(ocrParamList_t *perType) {
	//TODO
	return NULL;
}

ocrPolicyDomainFactory_t * newPolicyDomainFactoryHcMasterLeafPlace(ocrParamList_t *perType) {
	//TODO
	return NULL;
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

void leaf_place_policy_domain_start (ocrPolicyDomain_t * policy) {
    // Create Task and Event Factories
    policy->taskFactories = (ocrTaskFactory_t**) malloc(sizeof(ocrTaskFactory_t*));
    policy->eventFactories = (ocrEventFactory_t**) malloc(sizeof(ocrEventFactory_t*));

    policy->taskFactories[0] = newTaskFactoryHc(NULL);
    policy->eventFactories[0] = newEventFactoryHc(NULL);

    // WARNING: Threads start should be the last thing we do here after
    //          all data-structures have been initialized.
    u64 i = 0;
    u64 workerCount = policy->workerCount;
    u64 computeCount = policy->computeCount;
    for(i = 0; i < workerCount; i++) {
        policy->workers[i]->fctPtrs->start(policy->workers[i]);

    }
    for(i = 0; i < computeCount; i++) {
        policy->computes[i]->fctPtrs->start(policy->computes[i]);
    }
}

void mastered_leaf_place_policy_domain_start (ocrPolicyDomain_t * policy) {
    // Create Task and Event Factories
    policy->taskFactories = (ocrTaskFactory_t**) malloc(sizeof(ocrTaskFactory_t*));
    policy->eventFactories = (ocrEventFactory_t**) malloc(sizeof(ocrEventFactory_t*));

    policy->taskFactories[0] = newTaskFactoryHc(NULL);
    policy->eventFactories[0] = newEventFactoryHc(NULL);

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

void leaf_place_policy_domain_stop (ocrPolicyDomain_t * policy) {
    u64 i;
    for (i = 0; i < policy->computeCount; i++) {
        policy->computes[i]->fctPtrs->stop(policy->computes[i]);
    }
}

void mastered_leaf_place_policy_domain_stop (ocrPolicyDomain_t * policy) {
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

ocrGuid_t leaf_policy_domain_handIn( ocrPolicyDomain_t * this, ocrPolicyDomain_t * takingPolicy, ocrGuid_t takingWorkerGuid ) {
    u64 nPredecessors = this->n_predecessors;
    u64 currIndex = 0;
    ocrGuid_t extracted = NULL_GUID;
    for ( currIndex = 0; currIndex < nPredecessors && NULL_GUID == extracted; ++currIndex ) {
        ocrPolicyDomain_t* currParent = this->predecessors[currIndex];
        extracted = currParent->handIn(currParent, this, takingWorkerGuid);
    }
    return extracted;
}

ocrGuid_t leaf_policy_domain_extract ( ocrPolicyDomain_t * this, ocrPolicyDomain_t * takingPolicy, ocrGuid_t takingWorkerGuid ) {
    // TODO sagnak BAD BAD hardcoding
    ocrScheduler_t* scheduler = this->schedulers[0];
    return scheduler->take(scheduler, takingWorkerGuid);
}

void leaf_place_policy_domain_constructor_helper ( ocrPolicyDomain_t * policy,
                                                   u64 workpileCount,
                                                   u64 workerCount,
                                                   u64 computeCount,
                                                   u64 schedulerCount) {
    // Get a GUID
    policy->guid = UNINITIALIZED_GUID;
    guidify(getCurrentPD(), (u64)policy, &(policy->guid), OCR_GUID_POLICY);

    ocrMappable_t * module_base = (ocrMappable_t *) policy;
    module_base->mapFct = hc_ocr_module_map_schedulers_to_policy;

    policy->computeCount = computeCount;
    policy->workpileCount = workpileCount;
    policy->workerCount = workerCount;
    policy->schedulerCount = schedulerCount;

    policy->create = hc_policy_domain_create;
    policy->finish = hc_policy_domain_finish;
    policy->destruct = hc_policy_domain_destruct;
    // no inter-policy domain for HC for now
    policy->handOut = policy_domain_handOut_assert;
    policy->receive = policy_domain_receive_assert;
    policy->handIn = leaf_policy_domain_handIn;
    policy->extract = leaf_policy_domain_extract;
}

ocrPolicyDomain_t * leaf_place_policy_domain_constructor(u64 workpileCount,
                                                           u64 workerCount,
                                                           u64 computeCount,
                                                           u64 schedulerCount) {
    ocrPolicyDomain_t * policy = (ocrPolicyDomain_t *) malloc(sizeof(ocrPolicyDomain_t));
    leaf_place_policy_domain_constructor_helper(policy, workpileCount, workerCount, computeCount, schedulerCount);
    policy->start = leaf_place_policy_domain_start; // mastered_leaf_place_policy_domain_start 
    policy->stop = leaf_place_policy_domain_stop; // mastered_leaf_place_policy_domain_stop 
    return policy;
}

ocrPolicyDomain_t * mastered_leaf_place_policy_domain_constructor(u64 workpileCount,
                                                   u64 workerCount,
                                                   u64 computeCount,
                                                   u64 schedulerCount) {
    ocrPolicyDomain_t * policy = (ocrPolicyDomain_t *) malloc(sizeof(ocrPolicyDomain_t));
    leaf_place_policy_domain_constructor_helper(policy, workpileCount, workerCount, computeCount, schedulerCount);
    policy->start = mastered_leaf_place_policy_domain_start;
    policy->stop = mastered_leaf_place_policy_domain_stop;
    return policy;
}

void ocr_module_map_nothing_to_place (void * self_module, ocrMappableKind kind,
                                               u64 nb_instances, void ** ptr_instances) {
    // Checking mapping conforms to what we're expecting in this implementation
    assert ( 0 && "We should not map anything on a place policy");
}

void place_policy_domain_create ( ocrPolicyDomain_t * policy, void * configuration,
        u64 schedulerCount, u64 workerCount, u64 computeCount,
        u64 workpileCount, u64 allocatorCount, u64 memoryCount,
        ocrTaskFactory_t *taskFactory, ocrDataBlockFactory_t *dbFactory,
        ocrEventFactory_t *eventFactory, ocrPolicyCtxFactory_t *contextFactory, 
        ocrCost_t *costFunction ) {
}

void place_policy_domain_destruct(ocrPolicyDomain_t * policy) {
}

ocrGuid_t place_policy_getAllocator(ocrPolicyDomain_t * policy, ocrLocation_t* location) {
    return NULL_GUID;
}

void place_policy_domain_start(ocrPolicyDomain_t * policy) {
}

void place_policy_domain_finish(ocrPolicyDomain_t * policy) {
}

void place_policy_domain_stop(ocrPolicyDomain_t * policy) {
}

ocrGuid_t place_policy_domain_handIn( ocrPolicyDomain_t * this, ocrPolicyDomain_t * takingPolicy, ocrGuid_t takingWorkerGuid ) {
    ocrGuid_t extracted = this->extract(this,takingPolicy,takingWorkerGuid);

    if ( NULL_GUID == extracted ) {
        u64 nPredecessors = this->n_predecessors;
        u64 currIndex = 0;
        for ( currIndex = 0; currIndex < nPredecessors && NULL_GUID == extracted; ++currIndex ) {
            ocrPolicyDomain_t* currParent = this->predecessors[currIndex];
            extracted = currParent->handIn(currParent,this,takingWorkerGuid);
        }
    }

    return extracted;
}

ocrGuid_t place_policy_domain_extract ( ocrPolicyDomain_t * this, ocrPolicyDomain_t * takingPolicy, ocrGuid_t takingWorkerGuid ) {
    ocrGuid_t extracted = NULL_GUID;
    u64 nSuccessors = this->n_successors;
    u64 currIndex = 0;
    for ( currIndex = 0; currIndex < nSuccessors && NULL_GUID == extracted; ++currIndex ) {
        ocrPolicyDomain_t* currChild = this->successors[currIndex];
        if ( currChild != takingPolicy ) {
            extracted = currChild->extract(currChild,this,takingWorkerGuid);
        }
    }
    return extracted;
}

ocrPolicyDomain_t * place_policy_domain_constructor () {
    ocrPolicyDomain_t * policy = (ocrPolicyDomain_t *) malloc(sizeof(ocrPolicyDomain_t));

    policy->guid = UNINITIALIZED_GUID;
    guidify(getCurrentPD(), (u64)policy, &(policy->guid), OCR_GUID_POLICY);

    ocrMappable_t * module_base = (ocrMappable_t *) policy;
    module_base->mapFct = ocr_module_map_nothing_to_place;

    policy->workpileCount = 0;
    policy->workerCount = 0;
    policy->schedulerCount = 0;
    policy->computeCount = 0;

    policy->create = place_policy_domain_create;
    policy->destruct = place_policy_domain_destruct;
    policy->getAllocator = place_policy_getAllocator;

    policy->start = place_policy_domain_start;
    policy->finish = place_policy_domain_finish;
    policy->stop = place_policy_domain_stop;

    policy->handOut = policy_domain_handOut_assert;
    policy->receive = policy_domain_receive_assert;
    policy->handIn = place_policy_domain_handIn;
    policy->extract = place_policy_domain_extract;

    return policy;
}