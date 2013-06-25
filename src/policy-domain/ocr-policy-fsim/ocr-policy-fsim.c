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

#include "fsim.h"
#include "ocr-policy-domain.h"

void fsim_policy_domain_create(ocrPolicyDomain_t * policy, void * configuration,
                               ocrScheduler_t ** schedulers, ocrWorker_t ** workers,
                               ocrCompTarget_t ** computes, ocrWorkpile_t ** workpiles,
                               ocrAllocator_t ** allocators, ocrMemPlatform_t ** memories) {
    policy->schedulers = schedulers;
    policy->workers = workers;
    policy->computes = computes;
    policy->workpiles = workpiles;
    policy->allocators = allocators;
    policy->memories = memories;
}

static inline void start_task_event_factories ( ocrPolicyDomain_t * policy ) {
    // Create Task and Event Factories
    policy->taskFactories = (ocrTaskFactory_t**) malloc(sizeof(ocrTaskFactory_t*)*2);
    policy->eventFactories = (ocrEventFactory_t**) malloc(sizeof(ocrEventFactory_t*));

    policy->taskFactories[0] = newTaskFactoryFsim(NULL);
    policy->taskFactories[1] = newTaskFactoryFsimMessage(NULL);
    policy->eventFactories[0] = newEventFactoryHc(NULL);
}

void xe_policy_domain_start(ocrPolicyDomain_t * policy) {
    start_task_event_factories(policy);
    // WARNING: Threads start should be the last thing we do here after
    //          all data-structures have been initialized.
    u64 i = 0;
    u64 workerCount = policy->workerCount;
    u64 computeCount = policy->computeCount;
    for(i = 0; i < workerCount; i++) {
        policy->workers[i]->start(policy->workers[i]);

    }
    for(i = 0; i < computeCount; i++) {
        policy->computes[i]->start(policy->computes[i]);
    }
}

void ce_policy_domain_start(ocrPolicyDomain_t * policy) {
    start_task_event_factories(policy);

    // WARNING: Threads start should be the last thing we do here after
    //          all data-structures have been initialized.
    u64 i = 0;
    u64 workerCount = policy->workerCount;
    u64 computeCount = policy->computeCount;
    for(i = 0; i < workerCount; i++) {
        policy->workers[i]->start(policy->workers[i]);

    }
    for(i = 0; i < computeCount; i++) {
        policy->computes[i]->start(policy->computes[i]);
    }
}

void ce_mastered_policy_domain_start(ocrPolicyDomain_t * policy) {
    start_task_event_factories(policy);

    // WARNING: Threads start should be the last thing we do here after
    //          all data-structures have been initialized.
    u64 i = 0;
    u64 workerCount = policy->workerCount;
    u64 computeCount = policy->computeCount;

    // DIFFERENT FROM OTHER POLICY DOMAINS
    // Only start (N-1) workers as worker '0' is the current thread.
    for(i = 1; i < workerCount; i++) {
        policy->workers[i]->start(policy->workers[i]);
    }
    // Only start (N-1) threads as thread '0' is the current thread.
    for(i = 1; i < computeCount; i++) {
        policy->computes[i]->start(policy->computes[i]);
    }

    // Handle thread '0'
    policy->workers[0]->start(policy->workers[0]);
    // Need to associate thread and worker here, as current thread fall-through
    // in user code and may need to know which Worker it is associated to.
    associate_comp_platform_and_worker(policy->workers[0]);
}

void fsim_policy_domain_finish(ocrPolicyDomain_t * policy) {
    u64 i;
    for ( i = 0; i < policy->workerCount; ++i ) {
        policy->workers[i]->stop(policy->workers[i]);
    }

    for ( i = 0; i < policy->computeCount; ++i) {
        policy->computes[i]->stop(policy->computes[i]);
    }
}

void fsim_mastered_policy_domain_finish(ocrPolicyDomain_t * policy) {
    u64 i;
    for ( i = 0; i < policy->workerCount; ++i ) {
        policy->workers[i]->stop(policy->workers[i]);
    }

    for ( i = 1; i < policy->computeCount; ++i) {
        policy->computes[i]->stop(policy->computes[i]);
    }
}

void fsim_mastered_policy_domain_stop(ocrPolicyDomain_t * policy) {
}

void fsim_policy_domain_stop(ocrPolicyDomain_t * policy) {
}

void fsim_policy_domain_destruct(ocrPolicyDomain_t * policy) {
    ocrTaskFactory_t** taskFactories = policy->taskFactories;
    // TODO: sagnak should be OK to hardcode
    // there is 2 task factories and a single event factory by type/design
    taskFactories[0]->destruct(taskFactories[0]);
    taskFactories[1]->destruct(taskFactories[1]);
    free(taskFactories);

    ocrEventFactory_t** eventFactories = policy->eventFactories;
    eventFactories[0]->destruct(eventFactories[0]);
    free(eventFactories);
}

ocrGuid_t fsim_policy_getAllocator(ocrPolicyDomain_t * policy, ocrLocation_t* location) {
    return policy->allocators[0]->guid;
}

// Mapping function one-to-one to map a scheduler to a policy instance
void fsim_ocr_module_map_schedulers_to_policy (void * self_module, ocrMappableKind kind,
                                               u64 nb_instances, void ** ptr_instances) {
    // Checking mapping conforms to what we're expecting in this implementation
    assert(kind == OCR_SCHEDULER);
    assert(nb_instances == 1);

    ocrPolicyDomain_t * policy = (ocrPolicyDomain_t *) self_module;
    ocrScheduler_t* scheduler = ptr_instances[0];
    scheduler->domain = policy;
}

ocrTaskFactory_t* fsim_policy_getTaskFactoryForUserTasks (ocrPolicyDomain_t * policy) {
    return policy->taskFactories[0];
}

ocrEventFactory_t* fsim_policy_getEventFactoryForUserEvents(ocrPolicyDomain_t * policy) {
    return policy->eventFactories[0];
}

static inline void fsim_policy_domain_constructor_helper ( ocrPolicyDomain_t * policy, u64 workpileCount,
                                                           u64 workerCount,
                                                           u64 computeCount,
                                                           u64 schedulerCount) {
    // Get a GUID
    policy->guid = UNINITIALIZED_GUID;
    guidify(getCurrentPD(), &(policy->guid), (u64)policy, OCR_GUID_POLICY);

    ocrMappable_t * module_base = (ocrMappable_t *) policy;
    module_base->mapFct = fsim_ocr_module_map_schedulers_to_policy;

    policy->workpileCount = workpileCount;
    policy->workerCount = workerCount;
    policy->schedulerCount = schedulerCount;
    policy->computeCount = computeCount;

    policy->create = fsim_policy_domain_create;
    policy->destruct = fsim_policy_domain_destruct;
    policy->getAllocator = fsim_policy_getAllocator;

    policy->getTaskFactoryForUserTasks = fsim_policy_getTaskFactoryForUserTasks;
    policy->getEventFactoryForUserEvents = fsim_policy_getEventFactoryForUserEvents;

    policy->handIn = policy_domain_handIn_assert;
    policy->extract = policy_domain_extract_assert;
}

void xe_policy_domain_hand_out ( ocrPolicyDomain_t * thisPolicy, ocrGuid_t giverWorkerGuid, ocrGuid_t givenTaskGuid ) {
    ocrPolicyDomain_t* ceDomain = thisPolicy->predecessors[0];
    ceDomain->receive (ceDomain, giverWorkerGuid, givenTaskGuid);
}

void ce_policy_domain_receive ( ocrPolicyDomain_t * thisPolicy, ocrGuid_t giverWorkerGuid, ocrGuid_t givenTaskGuid ) {
    ocrPolicyDomain_t* ceDomain = (ocrPolicyDomain_t *) thisPolicy;

    // TODO sagnak, oooh nasty hardcoding
    ocrScheduler_t* ceScheduler = ceDomain->schedulers[0];
    ceScheduler->give(ceScheduler, giverWorkerGuid, givenTaskGuid);
}

ocrPolicyDomain_t * xe_policy_domain_constructor (u64 workpileCount,
                                                    u64 workerCount,
                                                    u64 computeCount,
                                                    u64 schedulerCount) {

    ocrPolicyDomain_t * policy = (ocrPolicyDomain_t *) malloc(sizeof(ocrPolicyDomain_t));

    fsim_policy_domain_constructor_helper(policy, workpileCount, workerCount, computeCount, schedulerCount);

    policy->start = xe_policy_domain_start;
    policy->finish = fsim_policy_domain_finish;
    policy->stop = fsim_policy_domain_stop;

    policy->handOut = xe_policy_domain_hand_out;
    policy->receive = policy_domain_receive_assert;

    return policy;
}

ocrPolicyDomain_t * ce_policy_domain_constructor (u64 workpileCount,
                                                    u64 workerCount,
                                                    u64 computeCount,
                                                    u64 schedulerCount) {

    ocrPolicyDomain_t * policy = (ocrPolicyDomain_t *) malloc(sizeof(ocrPolicyDomain_t));

    fsim_policy_domain_constructor_helper(policy, workpileCount, workerCount, computeCount, schedulerCount);

    policy->start = ce_policy_domain_start;
    policy->finish = fsim_policy_domain_finish;
    policy->stop = fsim_policy_domain_stop;

    policy->handOut = policy_domain_handOut_assert;
    policy->receive = ce_policy_domain_receive;

    return policy;
}

ocrPolicyDomain_t * ce_mastered_policy_domain_constructor (u64 workpileCount,
                                                             u64 workerCount,
                                                             u64 computeCount,
                                                             u64 schedulerCount) {

    ocrPolicyDomain_t * policy = (ocrPolicyDomain_t *) malloc(sizeof(ocrPolicyDomain_t));

    fsim_policy_domain_constructor_helper(policy, workpileCount, workerCount, computeCount, schedulerCount);

    policy->start = ce_mastered_policy_domain_start;
    policy->finish = fsim_mastered_policy_domain_finish;
    policy->stop = fsim_mastered_policy_domain_stop;

    policy->handOut = policy_domain_handOut_assert;
    policy->receive = ce_policy_domain_receive;

    return policy;
}
