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

#include "ocr-macros.h"
#include "hc.h"
#include "ocr-policy.h"

void hc_policy_domain_create(ocr_policy_domain_t * policy, void * configuration,
                             ocr_scheduler_t ** schedulers, ocr_worker_t ** workers,
                             ocr_executor_t ** executors, ocr_workpile_t ** workpiles,
                             ocrAllocator_t ** allocators, ocrLowMemory_t ** memories) {
    policy->schedulers = schedulers;
    policy->workers = workers;
    policy->executors = executors;
    policy->workpiles = workpiles;
    policy->allocators = allocators;
    policy->memories = memories;
}

void hc_policy_domain_start(ocr_policy_domain_t * policy) {
    // Create Task and Event Factories
    policy->taskFactories = (ocr_task_factory**) malloc(sizeof(ocr_task_factory*));
    policy->eventFactories = (ocr_event_factory**) malloc(sizeof(ocr_event_factory*));

    policy->taskFactories[0] = hc_task_factory_constructor();
    policy->eventFactories[0] = hc_event_factory_constructor();

    // WARNING: Threads start should be the last thing we do here after
    //          all data-structures have been initialized.
    size_t i = 0;
    size_t nb_workers = policy->nb_workers;
    size_t nb_executors = policy->nb_executors;
    // Only start (N-1) workers as worker '0' is the current thread.
    for(i = 1; i < nb_workers; i++) {
        policy->workers[i]->start(policy->workers[i]);

    }
    // Only start (N-1) threads as thread '0' is the current thread.
    for(i = 1; i < nb_executors; i++) {
        policy->executors[i]->start(policy->executors[i]);
    }
    // Handle thread '0'
    policy->workers[0]->start(policy->workers[0]);
    // Need to associate thread and worker here, as current thread fall-through
    // in user code and may need to know which Worker it is associated to.
    associate_executor_and_worker(policy->workers[0]);
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
    for (i = 1; i < policy->nb_executors; i++) {
        policy->executors[i]->stop(policy->executors[i]);
    }
}

void hc_policy_domain_destruct(ocr_policy_domain_t * policy) {
    ocr_task_factory** taskFactories = policy->taskFactories;
    taskFactories[0]->destruct(taskFactories[0]);
    free(taskFactories);

    ocr_event_factory** eventFactories = policy->eventFactories;
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
        ocr_scheduler_t* scheduler = ptr_instances[i];
        scheduler->domain = policy;
    }
}

ocr_task_factory* hc_policy_getTaskFactoryForUserTasks (ocr_policy_domain_t * policy) {
    return policy->taskFactories[0];
}

ocr_event_factory* hc_policy_getEventFactoryForUserEvents(ocr_policy_domain_t * policy) {
    return policy->eventFactories[0];
}

ocr_policy_domain_t * hc_policy_domain_constructor(size_t nb_workpiles,
                                                   size_t nb_workers,
                                                   size_t nb_executors,
                                                   size_t nb_schedulers) {
    ocr_policy_domain_t * policy = (ocr_policy_domain_t *) malloc(sizeof(ocr_policy_domain_t));
    // Get a GUID
    policy->guid = UNINITIALIZED_GUID;
    globalGuidProvider->getGuid(globalGuidProvider, &(policy->guid), (u64)policy, OCR_GUID_POLICY);

    ocr_module_t * module_base = (ocr_module_t *) policy;
    module_base->map_fct = hc_ocr_module_map_schedulers_to_policy;

    policy->nb_executors = nb_executors;
    policy->nb_workpiles = nb_workpiles;
    policy->nb_workers = nb_workers;
    policy->nb_executors = nb_executors;
    policy->nb_schedulers = nb_schedulers;
    policy->create = hc_policy_domain_create;
    policy->start = hc_policy_domain_start;
    policy->finish = hc_policy_domain_finish;
    policy->stop = hc_policy_domain_stop;
    policy->destruct = hc_policy_domain_destruct;
    policy->getAllocator = hc_policy_getAllocator;
    // no inter-policy domain for HC for now
    policy->take = policy_domain_take_assert;
    policy->give = policy_domain_give_assert;
    policy->handIn = policy_domain_handIn_assert;
    policy->handOut = policy_domain_handOut_assert;
    policy->receive = policy_domain_receive_assert;

    policy->getTaskFactoryForUserTasks = hc_policy_getTaskFactoryForUserTasks;
    policy->getEventFactoryForUserEvents = hc_policy_getEventFactoryForUserEvents;
    return policy;
}

void leaf_place_policy_domain_start (ocr_policy_domain_t * policy) {
    // Create Task and Event Factories
    policy->taskFactories = (ocr_task_factory**) malloc(sizeof(ocr_task_factory*));
    policy->eventFactories = (ocr_event_factory**) malloc(sizeof(ocr_event_factory*));

    policy->taskFactories[0] = hc_task_factory_constructor();
    policy->eventFactories[0] = hc_event_factory_constructor();

    // WARNING: Threads start should be the last thing we do here after
    //          all data-structures have been initialized.
    size_t i = 0;
    size_t nb_workers = policy->nb_workers;
    size_t nb_executors = policy->nb_executors;
    for(i = 0; i < nb_workers; i++) {
        policy->workers[i]->start(policy->workers[i]);

    }
    for(i = 0; i < nb_executors; i++) {
        policy->executors[i]->start(policy->executors[i]);
    }
}

void mastered_leaf_place_policy_domain_start (ocr_policy_domain_t * policy) {
    // Create Task and Event Factories
    policy->taskFactories = (ocr_task_factory**) malloc(sizeof(ocr_task_factory*));
    policy->eventFactories = (ocr_event_factory**) malloc(sizeof(ocr_event_factory*));

    policy->taskFactories[0] = hc_task_factory_constructor();
    policy->eventFactories[0] = hc_event_factory_constructor();

    // WARNING: Threads start should be the last thing we do here after
    //          all data-structures have been initialized.
    size_t i = 0;
    size_t nb_workers = policy->nb_workers;
    size_t nb_executors = policy->nb_executors;
    // Only start (N-1) workers as worker '0' is the current thread.
    for(i = 1; i < nb_workers; i++) {
        policy->workers[i]->start(policy->workers[i]);

    }
    // Only start (N-1) threads as thread '0' is the current thread.
    for(i = 1; i < nb_executors; i++) {
        policy->executors[i]->start(policy->executors[i]);
    }
    // Handle thread '0'
    policy->workers[0]->start(policy->workers[0]);
    // Need to associate thread and worker here, as current thread fall-through
    // in user code and may need to know which Worker it is associated to.
    associate_executor_and_worker(policy->workers[0]);
}

void leaf_place_policy_domain_stop (ocr_policy_domain_t * policy) {
    size_t i;
    for (i = 0; i < policy->nb_executors; i++) {
        policy->executors[i]->stop(policy->executors[i]);
    }
}

void mastered_leaf_place_policy_domain_stop (ocr_policy_domain_t * policy) {
    // WARNING: Do not add code here unless you know what you're doing !!
    // If we are here, it means a codelet called ocrFinish which
    // logically stopped workers and can make thread '0' executes this
    // code before joining the other threads.

    // Thread '0' joins the other (N-1) threads.
    size_t i;
    for (i = 1; i < policy->nb_executors; i++) {
        policy->executors[i]->stop(policy->executors[i]);
    }
}

void leaf_place_policy_domain_constructor_helper ( ocr_policy_domain_t * policy,
                                                   size_t nb_workpiles,
                                                   size_t nb_workers,
                                                   size_t nb_executors,
                                                   size_t nb_schedulers) {
    // Get a GUID
    policy->guid = UNINITIALIZED_GUID;
    globalGuidProvider->getGuid(globalGuidProvider, &(policy->guid), (u64)policy, OCR_GUID_POLICY);

    ocr_module_t * module_base = (ocr_module_t *) policy;
    module_base->map_fct = hc_ocr_module_map_schedulers_to_policy;

    policy->nb_executors = nb_executors;
    policy->nb_workpiles = nb_workpiles;
    policy->nb_workers = nb_workers;
    policy->nb_executors = nb_executors;
    policy->nb_schedulers = nb_schedulers;

    policy->create = hc_policy_domain_create;
    policy->finish = hc_policy_domain_finish;
    policy->destruct = hc_policy_domain_destruct;
    policy->getAllocator = hc_policy_getAllocator;
    // no inter-policy domain for HC for now
    policy->take = policy_domain_take_assert;
    policy->give = policy_domain_give_assert;
    policy->handIn = policy_domain_handIn_assert;
    policy->handOut = policy_domain_handOut_assert;
    policy->receive = policy_domain_receive_assert;

    policy->getTaskFactoryForUserTasks = hc_policy_getTaskFactoryForUserTasks;
    policy->getEventFactoryForUserEvents = hc_policy_getEventFactoryForUserEvents;
}

ocr_policy_domain_t * leaf_place_policy_domain_constructor(size_t nb_workpiles,
                                                           size_t nb_workers,
                                                           size_t nb_executors,
                                                           size_t nb_schedulers) {
    ocr_policy_domain_t * policy = (ocr_policy_domain_t *) malloc(sizeof(ocr_policy_domain_t));
    leaf_place_policy_domain_constructor_helper(policy, nb_workpiles, nb_workers, nb_executors, nb_schedulers);
    policy->start = leaf_place_policy_domain_start; // mastered_leaf_place_policy_domain_start 
    policy->stop = leaf_place_policy_domain_stop; // mastered_leaf_place_policy_domain_stop 
    return policy;
}

ocr_policy_domain_t * mastered_leaf_place_policy_domain_constructor(size_t nb_workpiles,
                                                   size_t nb_workers,
                                                   size_t nb_executors,
                                                   size_t nb_schedulers) {
    ocr_policy_domain_t * policy = (ocr_policy_domain_t *) malloc(sizeof(ocr_policy_domain_t));
    leaf_place_policy_domain_constructor_helper(policy, nb_workpiles, nb_workers, nb_executors, nb_schedulers);
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
                               ocr_scheduler_t ** schedulers, ocr_worker_t ** workers,
                               ocr_executor_t ** executors, ocr_workpile_t ** workpiles,
                               ocrAllocator_t ** allocators, ocrLowMemory_t ** memories) {
    policy->schedulers = NULL;
    policy->workers = NULL;
    policy->executors = NULL;
    policy->workpiles = NULL;
    policy->allocators = NULL;
    policy->memories = NULL;
}

void place_policy_domain_destruct(ocr_policy_domain_t * policy) {
}

ocrGuid_t place_policy_getAllocator(ocr_policy_domain_t * policy, ocrLocation_t* location) {
    return NULL_GUID;
}

ocr_task_factory* place_policy_getTaskFactoryForUserTasks (ocr_policy_domain_t * policy) {
    assert ( 0 && "We should not ask for a task factory from a place policy");
    return NULL;
}

ocr_event_factory* place_policy_getEventFactoryForUserEvents (ocr_policy_domain_t * policy) {
    assert ( 0 && "We should not ask for an event factory from a place policy");
    return NULL;
}

void place_policy_domain_start(ocr_policy_domain_t * policy) {
}

void place_policy_domain_finish(ocr_policy_domain_t * policy) {
}

void place_policy_domain_stop(ocr_policy_domain_t * policy) {
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
    policy->nb_executors = 0;

    policy->create = place_policy_domain_create;
    policy->destruct = place_policy_domain_destruct;
    policy->getAllocator = place_policy_getAllocator;

    policy->getTaskFactoryForUserTasks = place_policy_getTaskFactoryForUserTasks;
    policy->getEventFactoryForUserEvents = place_policy_getEventFactoryForUserEvents;

    policy->take = policy_domain_take_assert;
    policy->give = policy_domain_give_assert;
    policy->handIn = policy_domain_handIn_assert;

    policy->start = place_policy_domain_start;
    policy->finish = place_policy_domain_finish;
    policy->stop = place_policy_domain_stop;

    policy->handOut = policy_domain_handOut_assert;
    policy->receive = policy_domain_receive_assert;

    return policy;
}
