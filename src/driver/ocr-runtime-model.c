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

#include <stdlib.h>

#include "ocr-runtime-model.h"
#include "ocr-factory.h"

ocr_executor_kind ocr_executor_default_kind;
ocr_worker_kind ocr_worker_default_kind;
ocr_scheduler_kind ocr_scheduler_default_kind;
ocr_policy_kind ocr_policy_default_kind;
ocr_workpile_kind ocr_workpile_default_kind;

/**
 * Default policy has one scheduler and a configurable
 * number of workers, executors and workpiles
 */
ocr_model_policy_t * defaultOcrModelPolicy(size_t nb_workers,
        size_t nb_executors, size_t nb_workpiles) {

    // Default policy
    ocr_model_policy_t * defaultPolicy =
            (ocr_model_policy_t *) malloc(sizeof(ocr_model_policy_t));
    defaultPolicy->model.configuration = NULL;
    defaultPolicy->model.kind = ocr_policy_default_kind;
    defaultPolicy->model.nb_instances = 1;
    defaultPolicy->nb_scheduler_types = 1;

    // Default scheduler
    ocr_model_scheduler_t * defaultScheduler =
            (ocr_model_scheduler_t *) malloc(sizeof(ocr_model_scheduler_t));
    defaultScheduler->model.configuration = NULL;
    defaultScheduler->model.kind = ocr_scheduler_default_kind;
    defaultScheduler->model.nb_instances = 1;
    defaultScheduler->nb_worker_types = 1;
    defaultScheduler->nb_executor_types = 1;
    defaultScheduler->nb_workpile_types = 1;
    defaultPolicy->schedulers = defaultScheduler;

    // Default worker
    ocr_model_t * defaultWorker =
            (ocr_model_t *) malloc(sizeof(ocr_model_t));
    defaultWorker->configuration = NULL;
    defaultWorker->kind = ocr_worker_default_kind;
    defaultWorker->nb_instances = nb_workers;
    defaultScheduler->workers = defaultWorker;

    // Default executor
    ocr_model_t * defaultExecutor =
            (ocr_model_t *) malloc(sizeof(ocr_model_t));
    defaultExecutor->configuration = NULL;
    defaultExecutor->kind = ocr_executor_default_kind;
    defaultExecutor->nb_instances = nb_executors;
    defaultScheduler->executors = defaultExecutor;

    // Default workpile
    ocr_model_t * defaultWorkpile =
            (ocr_model_t *) malloc(sizeof(ocr_model_t));
    defaultWorkpile->configuration = NULL;
    defaultWorkpile->kind = ocr_workpile_default_kind;
    defaultWorkpile->nb_instances = nb_workpiles;
    defaultScheduler->workpiles = defaultWorkpile;

    return defaultPolicy;
}

void destroyOcrModelPolicy(ocr_model_policy_t * model) {
    if(model->nb_scheduler_types > 0) {
        size_t i;
        for(i=0; i < model->nb_scheduler_types; i++) {
            ocr_model_scheduler_t * scheduler = model->schedulers+i;
            if(scheduler->nb_worker_types > 0) {
                free(scheduler->workers);
            }
            if(scheduler->nb_executor_types > 0) {
                free(scheduler->executors);
            }
            if(scheduler->nb_workpile_types > 0) {
                free(scheduler->workpiles);
            }
            free(scheduler);
        }
    }
    free(model);
}

ocr_policy_domain_t * instantiateModel(ocr_model_policy_t * model) {
    // Compute total number of workers, executors and workpiles
    int nb_scheduler_types = model->nb_scheduler_types;
    int total_nb_schedulers = 0;
    int total_nb_workers = 0;
    int total_nb_executors = 0;
    int total_nb_workpiles = 0;
    size_t i = 0;
    for(i=0; i < nb_scheduler_types; i++) {
        ocr_model_scheduler_t * current_scheduler = model->schedulers+i;
        total_nb_schedulers += current_scheduler->model.nb_instances;
        size_t j = 0;
        for(j=0; j < current_scheduler->nb_worker_types; j++) {
            total_nb_workers += current_scheduler->workers[j].nb_instances;
        }
        for(j=0; j < current_scheduler->nb_executor_types; j++) {
            total_nb_executors += current_scheduler->executors[j].nb_instances;
        }
        for(j=0; j < current_scheduler->nb_workpile_types; j++) {
            total_nb_workpiles += current_scheduler->workpiles[j].nb_instances;
        }
    }

    // Create an instance of the policy domain
    ocr_policy_domain_t * policyDomain = newPolicy(model->model.kind,
            total_nb_workpiles, total_nb_workers,
            total_nb_executors, total_nb_schedulers);

    // Allocate memory for ocr components
    // Components instances are grouped on one big chunk of memory
    ocr_scheduler_t ** schedulers = (ocr_scheduler_t **)
                        malloc(sizeof(ocr_scheduler_t *) * total_nb_schedulers);
    ocr_worker_t ** workers = (ocr_worker_t **)
                        malloc(sizeof(ocr_worker_t *) * total_nb_workers);
    ocr_executor_t ** executors = (ocr_executor_t **)
                        malloc(sizeof(ocr_executor_t *) * total_nb_executors);
    ocr_workpile_t ** workpiles = (ocr_workpile_t **)
                        malloc(sizeof(ocr_workpile_t *) * total_nb_workpiles);

    size_t workpile_head = 0;
    size_t workpile_tail = 0;
    size_t executor_tail = 0;
    size_t worker_tail = 0;
    size_t scheduler_tail = 0;

    /* For each scheduler type, create the specified number of instances  */
    size_t scheduler_type_i;
    size_t scheduler_instance_i;
    for(scheduler_type_i=0; scheduler_type_i < nb_scheduler_types; scheduler_type_i++) {
        ocr_model_scheduler_t * scheduler = model->schedulers + scheduler_type_i;
        for(scheduler_instance_i=0;
                scheduler_instance_i < scheduler->model.nb_instances; scheduler_instance_i++) {
            int nb_workpile_types = scheduler->nb_workpile_types;
            int nb_worker_types = scheduler->nb_worker_types;
            int nb_executor_types = scheduler->nb_executor_types;
            size_t type = 0;
            // Create Workpiles for this scheduler type
            for(type=0; type < nb_workpile_types; type++) {
                size_t nb_instances = scheduler->workpiles[type].nb_instances;
                size_t instance = 0;
                for(instance = 0; instance < nb_instances; instance++) {
                    workpiles[workpile_tail] = newWorkpile(scheduler->workpiles[type].kind);
                    workpiles[workpile_tail]->create(workpiles[workpile_tail], scheduler->workpiles[type].configuration);
                    workpile_tail++;
                }
            }

            // Create Schedulers
            schedulers[scheduler_tail] = newScheduler(scheduler->model.kind);
            schedulers[scheduler_tail]->create(schedulers[scheduler_tail], scheduler->model.configuration,
                    (workpile_tail - workpile_head), &workpiles[workpile_head]);

            // Create Workers for this scheduler type
            for(type = 0; type < nb_worker_types; type++) {
                size_t nb_instances = scheduler->workers[type].nb_instances;
                size_t instance = 0;
                for(instance = 0; instance < nb_instances; instance++) {
                    workers[worker_tail] = newWorker(scheduler->workers[type].kind);
                    workers[worker_tail]->create(workers[worker_tail], scheduler->workers[type].configuration,
                            worker_tail /*worker id*/, schedulers[scheduler_tail]);
                    worker_tail++;
                }
            }

            // Create Executors for this scheduler type
            for(type = 0; type < nb_executor_types; type++) {
                size_t nb_instances = scheduler->executors[type].nb_instances;
                size_t instance = 0;
                for(instance = 0; instance < nb_instances; instance++) {
                    executors[executor_tail] = newExecutor(scheduler->executors[type].kind);
                    //TODO the routine thing is a hack. Threads should pick workers from a worker pool
                    executors[executor_tail]->create(executors[executor_tail], scheduler->executors[type].configuration,
                            workers[executor_tail]->routine, workers[executor_tail]);
                    executor_tail++;
                }
            }
            scheduler_tail++;
            workpile_head = workpile_tail;
            worker_head = worker_tail;
            executor_head = executor_tail;
            scheduler_head = scheduler_tail;
        }
    }
    policyDomain->create(policyDomain, NULL, schedulers,
            workers, executors, workpiles);
    return policyDomain;
}
