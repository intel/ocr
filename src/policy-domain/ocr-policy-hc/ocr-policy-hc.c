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

void hc_policy_domain_create(ocr_policy_domain_t * policy, void * configuration,
                             ocrScheduler_t ** schedulers, ocrWorker_t ** workers,
                             ocrCompTarget_t ** compTargets, ocrWorkpile_t ** workpiles,
                             ocrAllocator_t ** allocators, ocrMemPlatform_t ** memories) {
    policy->schedulers = schedulers;
    policy->workers = workers;
    policy->compTargets = compTargets;
    policy->workpiles = workpiles;
    policy->allocators = allocators;
    policy->memories = memories;
}

void hc_policy_domain_start(ocr_policy_domain_t * policy) {
    // Create Task and Event Factories
    policy->taskFactories = (ocrTaskFactory_t**) malloc(sizeof(ocrTaskFactory_t*));
    policy->eventFactories = (ocrEventFactory_t**) malloc(sizeof(ocrEventFactory_t*));

    policy->taskFactories[0] = newTaskFactoryHc(NULL);
    policy->eventFactories[0] = newEventFactoryHc(NULL);

    // WARNING: Threads start should be the last thing we do here after
    //          all data-structures have been initialized.
    size_t i = 0;
    size_t nb_workers = policy->nb_workers;
    size_t nb_comp_targets = policy->nb_comp_targets;
    // Only start (N-1) workers as worker '0' is the current thread.
    for(i = 1; i < nb_workers; i++) {
        policy->workers[i]->start(policy->workers[i]);

    }
    // Only start (N-1) threads as thread '0' is the current thread.
    for(i = 1; i < nb_comp_targets; i++) {
        policy->compTargets[i]->start(policy->compTargets[i]);
    }
    // Handle thread '0'
    policy->workers[0]->start(policy->workers[0]);
    // Need to associate thread and worker here, as current thread fall-through
    // in user code and may need to know which Worker it is associated to.
    associate_comp_platform_and_worker(policy->workers[0]);
}

void hc_policy_domain_finish(ocr_policy_domain_t * policy) {
    // Note: As soon as worker '0' is stopped its thread is
    // free to fall-through in ocr_finalize() (see warning there)
    size_t i;
    for ( i = 0; i < policy->nb_workers; ++i ) {
        policy->workers[i]->stop(policy->workers[i]);
    }
}

void hc_policy_domain_stop(ocr_policy_domain_t * policy) {
    // WARNING: Do not add code here unless you know what you're doing !!
    // If we are here, it means a codelet called ocrFinish which
    // logically stopped workers and can make thread '0' executes this
    // code before joining the other threads.

    // Thread '0' joins the other (N-1) threads.
    size_t i;
    for (i = 1; i < policy->nb_comp_targets; i++) {
        policy->compTargets[i]->stop(policy->compTargets[i]);
    }
}

void hc_policy_domain_destruct(ocr_policy_domain_t * policy) {
    ocrTaskFactory_t** taskFactories = policy->taskFactories;
    taskFactories[0]->destruct(taskFactories[0]);
    free(taskFactories);

    ocrEventFactory_t** eventFactories = policy->eventFactories;
    eventFactories[0]->destruct(eventFactories[0]);
    free(eventFactories);

}

ocrGuid_t hc_policy_getAllocator(ocr_policy_domain_t * policy, ocrLocation_t* location) {
    return policy->allocators[0]->guid;
}

// Mapping function many-to-one to map a set of schedulers to a policy instance
void hc_ocr_module_map_schedulers_to_policy (void * self_module, ocr_module_kind kind,
                                             size_t nb_instances, void ** ptr_instances) {
    // Checking mapping conforms to what we're expecting in this implementation
    assert(kind == OCR_SCHEDULER);

    ocr_policy_domain_t * policy = (ocr_policy_domain_t *) self_module;
    int i = 0;
    for ( i = 0; i < nb_instances; ++i ) {
        ocrScheduler_t* scheduler = ptr_instances[i];
        scheduler->domain = policy;
    }
}

ocrTaskFactory_t* hc_policy_getTaskFactoryForUserTasks (ocr_policy_domain_t * policy) {
    return policy->taskFactories[0];
}

ocrEventFactory_t* hc_policy_getEventFactoryForUserEvents(ocr_policy_domain_t * policy) {
    return policy->eventFactories[0];
}

ocr_policy_domain_t * hc_policy_domain_constructor(size_t nb_workpiles,
                                                   size_t nb_workers,
                                                   size_t nb_comp_targets,
                                                   size_t nb_schedulers) {
    ocr_policy_domain_t * policy = (ocr_policy_domain_t *) malloc(sizeof(ocr_policy_domain_t));
    // Get a GUID
    policy->guid = UNINITIALIZED_GUID;
    globalGuidProvider->getGuid(globalGuidProvider, &(policy->guid), (u64)policy, OCR_GUID_POLICY);

    ocr_module_t * module_base = (ocr_module_t *) policy;
    module_base->map_fct = hc_ocr_module_map_schedulers_to_policy;
    policy->nb_comp_targets = nb_comp_targets;
    policy->nb_workpiles = nb_workpiles;
    policy->nb_workers = nb_workers;
    policy->nb_schedulers = nb_schedulers;
    policy->create = hc_policy_domain_create;
    policy->start = hc_policy_domain_start;
    policy->finish = hc_policy_domain_finish;
    policy->stop = hc_policy_domain_stop;
    policy->destruct = hc_policy_domain_destruct;
    policy->getAllocator = hc_policy_getAllocator;
    // no inter-policy domain for HC for now
    policy->handOut = policy_domain_handOut_assert;
    policy->receive = policy_domain_receive_assert;
    policy->handIn = policy_domain_handIn_assert;
    policy->extract = policy_domain_extract_assert;

    policy->getTaskFactoryForUserTasks = hc_policy_getTaskFactoryForUserTasks;
    policy->getEventFactoryForUserEvents = hc_policy_getEventFactoryForUserEvents;
    return policy;
}

void leaf_place_policy_domain_start (ocr_policy_domain_t * policy) {
    // Create Task and Event Factories
    policy->taskFactories = (ocrTaskFactory_t**) malloc(sizeof(ocrTaskFactory_t*));
    policy->eventFactories = (ocrEventFactory_t**) malloc(sizeof(ocrEventFactory_t*));

    policy->taskFactories[0] = newTaskFactoryHc(NULL);
    policy->eventFactories[0] = newEventFactoryHc(NULL);

    // WARNING: Threads start should be the last thing we do here after
    //          all data-structures have been initialized.
    size_t i = 0;
    size_t nb_workers = policy->nb_workers;
    size_t nb_comp_targets = policy->nb_comp_targets;
    for(i = 0; i < nb_workers; i++) {
        policy->workers[i]->start(policy->workers[i]);

    }
    for(i = 0; i < nb_comp_targets; i++) {
        policy->compTargets[i]->start(policy->compTargets[i]);
    }
}

void mastered_leaf_place_policy_domain_start (ocr_policy_domain_t * policy) {
    // Create Task and Event Factories
    policy->taskFactories = (ocrTaskFactory_t**) malloc(sizeof(ocrTaskFactory_t*));
    policy->eventFactories = (ocrEventFactory_t**) malloc(sizeof(ocrEventFactory_t*));

    policy->taskFactories[0] = newTaskFactoryHc(NULL);
    policy->eventFactories[0] = newEventFactoryHc(NULL);

    // WARNING: Threads start should be the last thing we do here after
    //          all data-structures have been initialized.
    size_t i = 0;
    size_t nb_workers = policy->nb_workers;
    size_t nb_comp_targets = policy->nb_comp_targets;
    // Only start (N-1) workers as worker '0' is the current thread.
    for(i = 1; i < nb_workers; i++) {
        policy->workers[i]->start(policy->workers[i]);

    }
    // Only start (N-1) threads as thread '0' is the current thread.
    for(i = 1; i < nb_comp_targets; i++) {
        policy->compTargets[i]->start(policy->compTargets[i]);
    }
    // Handle thread '0'
    policy->workers[0]->start(policy->workers[0]);
    // Need to associate thread and worker here, as current thread fall-through
    // in user code and may need to know which Worker it is associated to.
    associate_comp_platform_and_worker(policy->workers[0]);
}

void leaf_place_policy_domain_stop (ocr_policy_domain_t * policy) {
    size_t i;
    for (i = 0; i < policy->nb_comp_targets; i++) {
        policy->compTargets[i]->stop(policy->compTargets[i]);
    }
}

void mastered_leaf_place_policy_domain_stop (ocr_policy_domain_t * policy) {
    // WARNING: Do not add code here unless you know what you're doing !!
    // If we are here, it means a codelet called ocrFinish which
    // logically stopped workers and can make thread '0' executes this
    // code before joining the other threads.

    // Thread '0' joins the other (N-1) threads.
    size_t i;
    for (i = 1; i < policy->nb_comp_targets; i++) {
        policy->compTargets[i]->stop(policy->compTargets[i]);
    }
}

ocrGuid_t leaf_policy_domain_handIn( ocr_policy_domain_t * this, ocr_policy_domain_t * takingPolicy, ocrGuid_t takingWorkerGuid ) {
    size_t nPredecessors = this->n_predecessors;
    size_t currIndex = 0;
    ocrGuid_t extracted = NULL_GUID;
    for ( currIndex = 0; currIndex < nPredecessors && NULL_GUID == extracted; ++currIndex ) {
        ocr_policy_domain_t* currParent = this->predecessors[currIndex];
        extracted = currParent->handIn(currParent, this, takingWorkerGuid);
    }
    return extracted;
}

ocrGuid_t leaf_policy_domain_extract ( ocr_policy_domain_t * this, ocr_policy_domain_t * takingPolicy, ocrGuid_t takingWorkerGuid ) {
    // TODO sagnak BAD BAD hardcoding
    ocrScheduler_t* scheduler = this->schedulers[0];
    return scheduler->take(scheduler, takingWorkerGuid);
}

void leaf_place_policy_domain_constructor_helper ( ocr_policy_domain_t * policy,
                                                   size_t nb_workpiles,
                                                   size_t nb_workers,
                                                   size_t nb_comp_targets,
                                                   size_t nb_schedulers) {
    // Get a GUID
    policy->guid = UNINITIALIZED_GUID;
    globalGuidProvider->getGuid(globalGuidProvider, &(policy->guid), (u64)policy, OCR_GUID_POLICY);

    ocr_module_t * module_base = (ocr_module_t *) policy;
    module_base->map_fct = hc_ocr_module_map_schedulers_to_policy;

    policy->nb_comp_targets = nb_comp_targets;
    policy->nb_workpiles = nb_workpiles;
    policy->nb_workers = nb_workers;
    policy->nb_schedulers = nb_schedulers;

    policy->create = hc_policy_domain_create;
    policy->finish = hc_policy_domain_finish;
    policy->destruct = hc_policy_domain_destruct;
    policy->getAllocator = hc_policy_getAllocator;
    // no inter-policy domain for HC for now
    policy->handOut = policy_domain_handOut_assert;
    policy->receive = policy_domain_receive_assert;
    policy->handIn = leaf_policy_domain_handIn;
    policy->extract = leaf_policy_domain_extract;

    policy->getTaskFactoryForUserTasks = hc_policy_getTaskFactoryForUserTasks;
    policy->getEventFactoryForUserEvents = hc_policy_getEventFactoryForUserEvents;
}

ocr_policy_domain_t * leaf_place_policy_domain_constructor(size_t nb_workpiles,
                                                           size_t nb_workers,
                                                           size_t nb_comp_targets,
                                                           size_t nb_schedulers) {
    ocr_policy_domain_t * policy = (ocr_policy_domain_t *) malloc(sizeof(ocr_policy_domain_t));
    leaf_place_policy_domain_constructor_helper(policy, nb_workpiles, nb_workers, nb_comp_targets, nb_schedulers);
    policy->start = leaf_place_policy_domain_start; // mastered_leaf_place_policy_domain_start 
    policy->stop = leaf_place_policy_domain_stop; // mastered_leaf_place_policy_domain_stop 
    return policy;
}

ocr_policy_domain_t * mastered_leaf_place_policy_domain_constructor(size_t nb_workpiles,
                                                   size_t nb_workers,
                                                   size_t nb_comp_targets,
                                                   size_t nb_schedulers) {
    ocr_policy_domain_t * policy = (ocr_policy_domain_t *) malloc(sizeof(ocr_policy_domain_t));
    leaf_place_policy_domain_constructor_helper(policy, nb_workpiles, nb_workers, nb_comp_targets, nb_schedulers);
    policy->start = mastered_leaf_place_policy_domain_start;
    policy->stop = mastered_leaf_place_policy_domain_stop;
    return policy;
}

void ocr_module_map_nothing_to_place (void * self_module, ocr_module_kind kind,
                                               size_t nb_instances, void ** ptr_instances) {
    // Checking mapping conforms to what we're expecting in this implementation
    assert ( 0 && "We should not map anything on a place policy");
}

void place_policy_domain_create (ocr_policy_domain_t * policy, void * configuration,
                               ocrScheduler_t ** schedulers, ocrWorker_t ** workers,
                               ocrCompTarget_t ** compTargets, ocrWorkpile_t ** workpiles,
                               ocrAllocator_t ** allocators, ocrMemPlatform_t ** memories) {
    policy->schedulers = NULL;
    policy->workers = NULL;
    policy->compTargets = NULL;
    policy->workpiles = NULL;
    policy->allocators = NULL;
    policy->memories = NULL;
}

void place_policy_domain_destruct(ocr_policy_domain_t * policy) {
}

ocrGuid_t place_policy_getAllocator(ocr_policy_domain_t * policy, ocrLocation_t* location) {
    return NULL_GUID;
}

ocrTaskFactory_t* place_policy_getTaskFactoryForUserTasks (ocr_policy_domain_t * policy) {
    assert ( 0 && "We should not ask for a task factory from a place policy");
    return NULL;
}

ocrEventFactory_t* place_policy_getEventFactoryForUserEvents (ocr_policy_domain_t * policy) {
    assert ( 0 && "We should not ask for an event factory from a place policy");
    return NULL;
}

void place_policy_domain_start(ocr_policy_domain_t * policy) {
}

void place_policy_domain_finish(ocr_policy_domain_t * policy) {
}

void place_policy_domain_stop(ocr_policy_domain_t * policy) {
}

ocrGuid_t place_policy_domain_handIn( ocr_policy_domain_t * this, ocr_policy_domain_t * takingPolicy, ocrGuid_t takingWorkerGuid ) {
    ocrGuid_t extracted = this->extract(this,takingPolicy,takingWorkerGuid);

    if ( NULL_GUID == extracted ) {
        size_t nPredecessors = this->n_predecessors;
        size_t currIndex = 0;
        for ( currIndex = 0; currIndex < nPredecessors && NULL_GUID == extracted; ++currIndex ) {
            ocr_policy_domain_t* currParent = this->predecessors[currIndex];
            extracted = currParent->handIn(currParent,this,takingWorkerGuid);
        }
    }

    return extracted;
}

ocrGuid_t place_policy_domain_extract ( ocr_policy_domain_t * this, ocr_policy_domain_t * takingPolicy, ocrGuid_t takingWorkerGuid ) {
    ocrGuid_t extracted = NULL_GUID;
    size_t nSuccessors = this->n_successors;
    size_t currIndex = 0;
    for ( currIndex = 0; currIndex < nSuccessors && NULL_GUID == extracted; ++currIndex ) {
        ocr_policy_domain_t* currChild = this->successors[currIndex];
        if ( currChild != takingPolicy ) {
            extracted = currChild->extract(currChild,this,takingWorkerGuid);
        }
    }
    return extracted;
}

ocr_policy_domain_t * place_policy_domain_constructor () {
    ocr_policy_domain_t * policy = (ocr_policy_domain_t *) malloc(sizeof(ocr_policy_domain_t));

    policy->guid = UNINITIALIZED_GUID;
    globalGuidProvider->getGuid(globalGuidProvider, &(policy->guid), (u64)policy, OCR_GUID_POLICY);

    ocr_module_t * module_base = (ocr_module_t *) policy;
    module_base->map_fct = ocr_module_map_nothing_to_place;

    policy->nb_workpiles = 0;
    policy->nb_workers = 0;
    policy->nb_schedulers = 0;
    policy->nb_comp_targets = 0;

    policy->create = place_policy_domain_create;
    policy->destruct = place_policy_domain_destruct;
    policy->getAllocator = place_policy_getAllocator;

    policy->getTaskFactoryForUserTasks = place_policy_getTaskFactoryForUserTasks;
    policy->getEventFactoryForUserEvents = place_policy_getEventFactoryForUserEvents;

    policy->start = place_policy_domain_start;
    policy->finish = place_policy_domain_finish;
    policy->stop = place_policy_domain_stop;

    policy->handOut = policy_domain_handOut_assert;
    policy->receive = policy_domain_receive_assert;
    policy->handIn = place_policy_domain_handIn;
    policy->extract = place_policy_domain_extract;

    return policy;
}
