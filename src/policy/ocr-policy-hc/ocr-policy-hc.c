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
    if (taskFactory == NULL) {
        taskFactory = hc_task_factory_constructor();
    }
    if (eventFactory == NULL) {
        eventFactory = hc_event_factory_constructor();
    }

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
    // Current thread is thread '0' mapped to worker '0'
    // Now that the thread went through all the user code it
    // can begin its worker scheduling routine
    ocr_worker_t * worker_zero = policy->workers[0];
    worker_zero->routine(worker_zero);

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
    taskFactory->destruct(taskFactory);
    eventFactory->destruct(eventFactory);
}

ocrGuid_t hc_policy_getAllocator(ocr_policy_domain_t * policy, ocrLocation_t* location) {
    return guidify((ocrAllocator_t *)(policy->allocators[0]));
}

ocr_policy_domain_t * hc_policy_domain_constructor(size_t nb_workpiles,
        size_t nb_workers,
        size_t nb_executors,
        size_t nb_schedulers) {
    ocr_policy_domain_t * policy = (ocr_policy_domain_t *) malloc(sizeof(ocr_policy_domain_t));
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
    return policy;
}
