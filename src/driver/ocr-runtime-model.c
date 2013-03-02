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
#include <assert.h>

#include "ocr-macros.h"
#include "ocr-executor.h"
#include "ocr-low-workers.h"
#include "ocr-scheduler.h"
#include "ocr-policy.h"
#include "ocr-runtime-model.h"
#include "ocr-config.h"

/**!
 * Helper function to build a module mapping
 */
static ocr_module_mapping_t build_ocr_module_mapping(ocr_mapping_kind kind, ocr_module_kind from, ocr_module_kind to) {
    ocr_module_mapping_t res;
    res.kind = kind;
    res.from = from;
    res.to = to;
    return res;
}

/**!
 * Function pointer type to define ocr modules mapping functions
 */
typedef void (*ocr_map_module_fct) (ocr_module_kind from, ocr_module_kind to,
        size_t nb_instances_from,
        ocr_module_t ** instances_from,
        size_t nb_instances_to,
        ocr_module_t ** instances_to);

/**!
 * One-to-many mapping function.
 * Maps a single instance of kind 'from' to 'n' instances of kind 'to'.
 * ex: maps a single scheduler to several workers.
 */
static void map_modules_one_to_many(ocr_module_kind from, ocr_module_kind to,
        size_t nb_instances_from,
        ocr_module_t ** instances_from,
        size_t nb_instances_to,
        ocr_module_t ** instances_to) {
    assert(nb_instances_from == 1);
    size_t i;
    for(i=0; i < nb_instances_to; i++) {
        instances_to[i]->map_fct(instances_to[i], from, 1, (void **) &instances_from[0]);
    }
}

/**!
 * One-to-one mapping function.
 * Maps each instance of kind 'from' to each instance of kind 'to' one to one.
 * ex: maps each executor to each worker in a one-one fashion.
 */
static void map_modules_one_to_one(ocr_module_kind from, ocr_module_kind to,
        size_t nb_instances_from,
        ocr_module_t ** instances_from,
        size_t nb_instances_to,
        ocr_module_t ** instances_to) {
    assert(nb_instances_from == nb_instances_to);
    size_t i;
    for(i=0; i < nb_instances_to; i++) {
        instances_to[i]->map_fct(instances_to[i], from, 1, (void **) &instances_from[i]);
    }
}

/**!
 * Many-to-one mapping function.
 * Maps all instance of kind 'from' to each instance of kind 'to'.
 * ex: maps all workpiles to each individual scheduler available.
 */

static void map_modules_many_to_one(ocr_module_kind from, ocr_module_kind to,
        size_t nb_instances_from,
        ocr_module_t ** instances_from,
        size_t nb_instances_to,
        ocr_module_t ** instances_to) {
    size_t i;
    for(i=0; i < nb_instances_to; i++) {
        instances_to[i]->map_fct(instances_to[i], from, nb_instances_from, (void **) instances_from);
    }
}

/**!
 * Given a mapping kind, returns a function pointer
 * to the implementation of the mapping kind.
 */
static ocr_map_module_fct get_module_mapping_function (ocr_mapping_kind mapping_kind) {
    switch(mapping_kind) {
    case MANY_TO_ONE_MAPPING:
        return map_modules_many_to_one;
    case ONE_TO_ONE_MAPPING:
        return map_modules_one_to_one;
    case ONE_TO_MANY_MAPPING:
        return map_modules_one_to_many;
    default:
        assert(false && "Unknown module mapping function");
        break;
    }
    return NULL;
}


/**!
 * Data-structure that stores ocr modules instances of a policy domain.
 * Used as an internal data-structure when building policy domains so
 * that generic code can be written when there's a need to find a particular
 * module's instance backing array.
 */
typedef struct {
    ocr_module_kind kind;
    size_t nb_instances;
    void ** instances;
} ocr_module_instance;

/**!
 * Utility function that goes over modules_kinds and looks for
 * a particular 'kind' of module. Sets pointers to its
 * number of instances and instances backing array.
 */
static void resolve_module_instances(ocr_module_kind kind, size_t nb_module_kind,
        ocr_module_instance * modules_kinds, size_t * nb_instances, void *** instances) {
    size_t i;
    for(i=0; i < nb_module_kind; i++) {
        if (kind == modules_kinds[i].kind) {
            *nb_instances = modules_kinds[i].nb_instances;
            *instances = modules_kinds[i].instances;
            return;
        }
    }
    assert(false && "Cannot resolve modules instances");
}

/**
 * Default policy has one scheduler and a configurable
 * number of workers, executors and workpiles
 */
ocr_model_policy_t * defaultOcrModelPolicy(size_t nb_schedulers, size_t nb_workers,
        size_t nb_executors, size_t nb_workpiles) {

    // Default policy
    ocr_model_policy_t * defaultPolicy =
            checked_malloc(defaultPolicy, sizeof(ocr_model_policy_t));
    defaultPolicy->model.kind = ocr_policy_default_kind;
    defaultPolicy->model.nb_instances = 1;
    defaultPolicy->model.configuration = NULL;
    defaultPolicy->nb_scheduler_types = 1;
    defaultPolicy->nb_worker_types = 1;
    defaultPolicy->nb_executor_types = 1;
    defaultPolicy->nb_workpile_types = 1;

    // Default scheduler
    ocr_model_t * defaultScheduler =
            checked_malloc(defaultScheduler, sizeof(ocr_model_t));
    defaultScheduler->kind = ocr_scheduler_default_kind;
    defaultScheduler->nb_instances = nb_schedulers;
    defaultScheduler->configuration = NULL;
    defaultPolicy->schedulers = defaultScheduler;

    // Default worker
    ocr_model_t * defaultWorker =
            checked_malloc(defaultWorker, sizeof(ocr_model_t));
    defaultWorker->kind = ocr_worker_default_kind;
    defaultWorker->nb_instances = nb_workers;
    defaultWorker->configuration = NULL;
    defaultPolicy->workers = defaultWorker;

    // Default executor
    ocr_model_t * defaultExecutor =
            checked_malloc(defaultExecutor, sizeof(ocr_model_t));
    defaultExecutor->kind = ocr_executor_default_kind;
    defaultExecutor->nb_instances = nb_executors;
    defaultExecutor->configuration = NULL;
    defaultPolicy->executors = defaultExecutor;

    // Default workpile
    ocr_model_t * defaultWorkpile =
            checked_malloc(defaultWorkpile, sizeof(ocr_model_t));
    defaultWorkpile->kind = ocr_workpile_default_kind;
    defaultWorkpile->nb_instances = nb_workpiles;
    defaultWorkpile->configuration = NULL;
    defaultPolicy->workpiles = defaultWorkpile;

    // Defines how ocr modules are bound together
    size_t nb_module_mappings = 4;
    ocr_module_mapping_t * defaultMapping =
            checked_malloc(defaultMapping, sizeof(ocr_module_mapping_t) * nb_module_mappings);
    // Note: this doesn't bind modules magically. You need to have a mapping function defined
    //       and set in the targeted implementation (see ocr_scheduler_hc implementation for reference).
    //       These just make sure the mapping functions you have defined are called
    defaultMapping[0] = build_ocr_module_mapping(MANY_TO_ONE_MAPPING, OCR_WORKPILE, OCR_SCHEDULER);
    defaultMapping[1] = build_ocr_module_mapping(ONE_TO_ONE_MAPPING, OCR_WORKER, OCR_EXECUTOR);
    defaultMapping[2] = build_ocr_module_mapping(ONE_TO_MANY_MAPPING, OCR_SCHEDULER, OCR_WORKER);
    defaultMapping[3] = build_ocr_module_mapping(MANY_TO_ONE_MAPPING, OCR_MEMORY, OCR_ALLOCATOR);
    defaultPolicy->nb_mappings = nb_module_mappings;
    defaultPolicy->mappings = defaultMapping;

    // Default memory
    ocr_model_t *defaultMemory =
            checked_malloc(defaultMemory, sizeof(ocr_model_t));
    defaultMemory->configuration = NULL;
    defaultMemory->kind = OCR_LOWMEMORY_DEFAULT;
    defaultMemory->nb_instances = 1;
    defaultPolicy->numMemTypes = 1;
    defaultPolicy->memories = defaultMemory;

    // Default allocator
    ocrAllocatorModel_t *defaultAllocator =
            checked_malloc(defaultAllocator, sizeof(ocrAllocatorModel_t));
    defaultAllocator->model.configuration = NULL;
    defaultAllocator->model.kind = OCR_ALLOCATOR_DEFAULT;
    defaultAllocator->model.nb_instances = 1;
    defaultAllocator->sizeManaged = gHackTotalMemSize;

    defaultPolicy->numAllocTypes = 1;
    defaultPolicy->allocators = defaultAllocator;

    return defaultPolicy;
}

/**
 * FSIM XE policy domain has:
 * one XE scheduler for all XEs
 * one worker for each XEs 
 * one executor for each XEs 
 * one workpile for each XEs
 * one memory for all XEs
 * one allocator for all XEs
 */

ocr_model_policy_t * createXeModelPolicies ( size_t nb_xes ) {
    size_t nb_xe_schedulers = 1;
    size_t nb_xe_workers = 1;
    size_t nb_xe_executors = 1;
    size_t nb_xe_workpiles = 1;
    size_t nb_xe_memories = 1;
    size_t nb_xe_allocators = 1;
    
    // there are #XE instances of a model 
    ocr_model_policy_t * xePolicyModel = (ocr_model_policy_t *) malloc(sizeof(ocr_model_policy_t));
    xePolicyModel->model.kind = OCR_POLICY_XE;
    xePolicyModel->model.nb_instances = nb_xes;
    xePolicyModel->model.configuration = NULL;
    xePolicyModel->nb_scheduler_types = 1;
    xePolicyModel->nb_worker_types = 1;
    xePolicyModel->nb_executor_types = 1;
    xePolicyModel->nb_workpile_types = 1;
    xePolicyModel->numMemTypes = 1;
    xePolicyModel->numAllocTypes = 1;

    // XE scheduler
    ocr_model_t * xeScheduler = (ocr_model_t *) malloc(sizeof(ocr_model_t));
    xeScheduler->kind = ocr_scheduler_xe_kind;
    xeScheduler->nb_instances = nb_xe_schedulers;
    xeScheduler->configuration = NULL;
    xePolicyModel->schedulers = xeScheduler;

    // XE worker
    ocr_model_t * xeWorker = (ocr_model_t *) malloc(sizeof(ocr_model_t));
    xeWorker->kind = ocr_worker_xe_kind;
    xeWorker->nb_instances = nb_xe_workers;
    xeWorker->configuration = NULL;
    xePolicyModel->workers = xeWorker;

    // XE executor
    ocr_model_t * xeExecutor = (ocr_model_t *) malloc(sizeof(ocr_model_t));
    xeExecutor->kind = ocr_executor_xe_kind;
    xeExecutor->nb_instances = nb_xe_executors;
    xeExecutor->configuration = NULL;
    xePolicyModel->executors = xeExecutor;

    // XE workpile
    ocr_model_t * xeWorkpile = (ocr_model_t *) malloc(sizeof(ocr_model_t));
    xeWorkpile->kind = ocr_workpile_xe_kind;
    xeWorkpile->nb_instances = nb_xe_workpiles;
    xeWorkpile->configuration = NULL;
    xePolicyModel->workpiles = xeWorkpile;

    // XE memory
    ocr_model_t *xeMemory = (ocr_model_t*)malloc(sizeof(ocr_model_t));
    xeMemory->configuration = NULL;
    xeMemory->kind = ocrLowMemoryXEKind;
    xeMemory->nb_instances = nb_xe_memories;
    xePolicyModel->memories = xeMemory;

    // XE allocator
    ocrAllocatorModel_t *xeAllocator = (ocrAllocatorModel_t*)malloc(sizeof(ocrAllocatorModel_t));
    xeAllocator->model.configuration = NULL;
    xeAllocator->model.kind = ocrAllocatorXEKind;
    xeAllocator->model.nb_instances = nb_xe_allocators;
    xeAllocator->sizeManaged = gHackTotalMemSize;
    xePolicyModel->allocators = xeAllocator;

    // Defines how ocr modules are bound together
    size_t nb_module_mappings = 4;
    ocr_module_mapping_t * xeMapping =
	(ocr_module_mapping_t *) malloc(sizeof(ocr_module_mapping_t) * nb_module_mappings);
    // Note: this doesn't bind modules magically. You need to have a mapping function defined
    //       and set in the targeted implementation (see ocr_scheduler_hc implementation for reference).
    //       These just make sure the mapping functions you have defined are called
    xeMapping[0] = build_ocr_module_mapping(MANY_TO_ONE_MAPPING, OCR_WORKPILE, OCR_SCHEDULER);
    xeMapping[1] = build_ocr_module_mapping(ONE_TO_ONE_MAPPING, OCR_WORKER, OCR_EXECUTOR);
    xeMapping[2] = build_ocr_module_mapping(ONE_TO_MANY_MAPPING, OCR_SCHEDULER, OCR_WORKER);
    xeMapping[3] = build_ocr_module_mapping(MANY_TO_ONE_MAPPING, OCR_MEMORY, OCR_ALLOCATOR);
    xePolicyModel->nb_mappings = nb_module_mappings;
    xePolicyModel->mappings = xeMapping;

    return xePolicyModel;
}
/**
 * FSIM CE policy domains has:
 * one CE scheduler for all CEs
 * one worker for each CEs 
 * one executor for each CEs 
 * one workpile for each CEs
 * one memory for all CEs
 * one allocator for all CEs
 */

void CEModelPoliciesHelper ( ocr_model_policy_t * cePolicyModel ) {
    size_t nb_ce_schedulers = 1;
    size_t nb_ce_workers = 1;
    size_t nb_ce_executors = 1;
    size_t nb_ce_workpiles = 1;
    size_t nb_ce_memories = 1;
    size_t nb_ce_allocators = 1;
    
    cePolicyModel->nb_scheduler_types = 1;
    cePolicyModel->nb_worker_types = 1;
    cePolicyModel->nb_executor_types = 1;
    cePolicyModel->nb_workpile_types = 1;
    cePolicyModel->numMemTypes = 1;
    cePolicyModel->numAllocTypes = 1;

    // CE scheduler
    ocr_model_t * ceScheduler = (ocr_model_t *) malloc(sizeof(ocr_model_t));
    ceScheduler->kind = ocr_scheduler_ce_kind;
    ceScheduler->nb_instances = nb_ce_schedulers;
    ceScheduler->configuration = NULL;
    cePolicyModel->schedulers = ceScheduler;

    // CE worker
    ocr_model_t * ceWorker = (ocr_model_t *) malloc(sizeof(ocr_model_t));
    ceWorker->kind = ocr_worker_ce_kind;
    ceWorker->nb_instances = nb_ce_workers;
    ceWorker->configuration = NULL;
    cePolicyModel->workers = ceWorker;

    // CE executor
    ocr_model_t * ceExecutor = (ocr_model_t *) malloc(sizeof(ocr_model_t));
    ceExecutor->kind = ocr_executor_ce_kind;
    ceExecutor->nb_instances = nb_ce_executors;
    ceExecutor->configuration = NULL;
    cePolicyModel->executors = ceExecutor;

    // CE workpile
    ocr_model_t * ceWorkpile = (ocr_model_t *) malloc(sizeof(ocr_model_t));
    ceWorkpile->kind = ocr_workpile_ce_kind;
    ceWorkpile->nb_instances = nb_ce_workpiles;
    ceWorkpile->configuration = NULL;
    cePolicyModel->workpiles = ceWorkpile;

    // CE memory
    ocr_model_t *ceMemory = (ocr_model_t*)malloc(sizeof(ocr_model_t));
    ceMemory->configuration = NULL;
    ceMemory->kind = ocrLowMemoryCEKind;
    ceMemory->nb_instances = nb_ce_memories;
    cePolicyModel->memories = ceMemory;

    // CE allocator
    ocrAllocatorModel_t *ceAllocator = (ocrAllocatorModel_t*)malloc(sizeof(ocrAllocatorModel_t));
    ceAllocator->model.configuration = NULL;
    ceAllocator->model.kind = ocrAllocatorXEKind;
    ceAllocator->model.nb_instances = nb_ce_allocators;
    ceAllocator->sizeManaged = gHackTotalMemSize;
    cePolicyModel->allocators = ceAllocator;

    // Defines how ocr modules are bound together
    size_t nb_module_mappings = 4;
    ocr_module_mapping_t * ceMapping =
	(ocr_module_mapping_t *) malloc(sizeof(ocr_module_mapping_t) * nb_module_mappings);
    // Note: this doesn't bind modules magically. You need to have a mapping function defined
    //       and set in the targeted implementation (see ocr_scheduler_hc implementation for reference).
    //       These just make sure the mapping functions you have defined are called
    ceMapping[0] = build_ocr_module_mapping(MANY_TO_ONE_MAPPING, OCR_WORKPILE, OCR_SCHEDULER);
    ceMapping[1] = build_ocr_module_mapping(ONE_TO_ONE_MAPPING, OCR_WORKER, OCR_EXECUTOR);
    ceMapping[2] = build_ocr_module_mapping(ONE_TO_MANY_MAPPING, OCR_SCHEDULER, OCR_WORKER);
    ceMapping[3] = build_ocr_module_mapping(MANY_TO_ONE_MAPPING, OCR_MEMORY, OCR_ALLOCATOR);
    cePolicyModel->nb_mappings = nb_module_mappings;
    cePolicyModel->mappings = ceMapping;


}

ocr_model_policy_t * createCeModelPolicies ( size_t nb_ces ) {
    ocr_model_policy_t * cePolicyModel = (ocr_model_policy_t *) malloc(sizeof(ocr_model_policy_t));

    cePolicyModel->model.kind = ocr_policy_ce_kind;
    cePolicyModel->model.nb_instances = nb_ces;
    cePolicyModel->model.configuration = NULL;
    CEModelPoliciesHelper(cePolicyModel); 
    return cePolicyModel;
}

ocr_model_policy_t * createCeMasteredModelPolicy ( ) {
    ocr_model_policy_t * cePolicyModel = (ocr_model_policy_t *) malloc(sizeof(ocr_model_policy_t));

    cePolicyModel->model.kind = ocr_policy_ce_mastered_kind;
    cePolicyModel->model.nb_instances = 1;
    cePolicyModel->model.configuration = NULL;
    CEModelPoliciesHelper(cePolicyModel); 
    return cePolicyModel;
}


void destructOcrModelPolicy(ocr_model_policy_t * model) {
    if (model->schedulers != NULL) {
        free(model->schedulers);
    }
    if (model->workers != NULL) {
        free(model->workers);
    }
    if (model->executors != NULL) {
        free(model->executors);
    }
    if (model->workpiles != NULL) {
        free(model->workpiles);
    }
    if (model->mappings != NULL) {
        free(model->mappings);
    }
    if(model->memories != NULL) {
        free(model->memories);
    }
    if(model->allocators != NULL) {
        free(model->allocators);
    }
    free(model);
}

static inline void create_configure_all_schedulers ( ocr_scheduler_t ** all_schedulers, int nb_component_types, ocr_model_t* components ) {
    size_t type = 0, idx = 0, instance = 0;
    for (; type < nb_component_types; ++type) {
        // For each type, create the number of instances asked for.
	ocr_model_t curr_component_model = components[type];
        for ( instance = 0; instance < curr_component_model.nb_instances; ++instance, ++idx ) {
            // Call the factory method based on the model's type kind.
            all_schedulers[idx] = newScheduler(curr_component_model.kind);
            all_schedulers[idx]->create(all_schedulers[idx], curr_component_model.configuration);
        }
    }
}

static inline void create_configure_all_workpiles ( ocr_workpile_t ** all_workpiles, int nb_component_types, ocr_model_t* components ) {
    size_t type = 0, idx = 0, instance = 0;
    for (; type < nb_component_types; ++type) {
        // For each type, create the number of instances asked for.
	ocr_model_t curr_component_model = components[type];
        for ( instance = 0; instance < curr_component_model.nb_instances; ++instance, ++idx ) {
            // Call the factory method based on the model's type kind.
            all_workpiles[idx] = newWorkpile(curr_component_model.kind);
            all_workpiles[idx]->create(all_workpiles[idx], curr_component_model.configuration);
        }
    }
}

static inline void create_configure_all_workers ( ocr_worker_t** all_workers, int nb_component_types, ocr_model_t* components, ocr_policy_domain_t* policyDomain ) {
    size_t type = 0, idx = 0, instance = 0;
    for (; type < nb_component_types; ++type) {
        // For each type, create the number of instances asked for.
	ocr_model_t curr_component_model = components[type];
	curr_component_model.configuration = policyDomain;
	for ( instance = 0; instance < curr_component_model.nb_instances; ++instance, ++idx ) {
            // Call the factory method based on the model's type kind.
            all_workers[idx] = newWorker(curr_component_model.kind);
            //TODO handle worker index
            all_workers[idx]->create(all_workers[idx], curr_component_model.configuration, idx);
        }
    }
}

static inline void create_configure_all_executors ( ocr_executor_t ** all_executors, int nb_component_types, ocr_model_t* components ) {
    size_t type = 0, idx = 0, instance = 0;
    for (; type < nb_component_types; ++type) {
        // For each type, create the number of instances asked for.
	ocr_model_t curr_component_model = components[type];
        for ( instance = 0; instance < curr_component_model.nb_instances; ++instance, ++idx ) {
            // Call the factory method based on the model's type kind.
            all_executors[idx] = newExecutor(curr_component_model.kind);
            all_executors[idx]->create(all_executors[idx], curr_component_model.configuration);
        }
    }
}

static inline void create_configure_all_allocators ( ocrAllocator_t ** all_allocators, int nb_component_types, ocrAllocatorModel_t* components ) {
    size_t type = 0, idx = 0, instance = 0;
    for (; type < nb_component_types; ++type) {
        // For each type, create the number of instances asked for.
	ocrAllocatorModel_t curr_component_model = components[type];
        for ( instance = 0; instance < curr_component_model.model.nb_instances; ++instance, ++idx ) {
            // Call the factory method based on the model's type kind.
            all_allocators[idx] = newAllocator(curr_component_model.model.kind);
            all_allocators[idx]->create(all_allocators[idx], curr_component_model.sizeManaged, curr_component_model.model.configuration);
        }
    }
}

static inline void create_configure_all_memories ( ocrLowMemory_t ** all_memories, int nb_component_types, ocr_model_t* components ) {
    size_t type = 0, idx = 0, instance = 0;
    for (; type < nb_component_types; ++type) {
        // For each type, create the number of instances asked for.
	ocr_model_t curr_component_model = components[type];
        for ( instance = 0; instance < curr_component_model.nb_instances; ++instance, ++idx ) {
            // Call the factory method based on the model's type kind.
            all_memories[idx] = newLowMemory(curr_component_model.kind);
            all_memories[idx]->create(all_memories[idx],curr_component_model.configuration);
        }
    }
}

/**!
 * Given a policy domain model, go over its modules and instantiate them.
 */
ocr_policy_domain_t ** instantiateModel(ocr_model_policy_t * model) {

    // Compute total number of workers, executors and workpiles, allocators and memories
    int total_nb_schedulers = 0;
    int total_nb_workers = 0;
    int total_nb_executors = 0;
    int total_nb_workpiles = 0;
    u64 totalNumMemories = 0;
    u64 totalNumAllocators = 0;
    size_t j = 0;
    for(j=0; j < model->nb_scheduler_types; j++) {
        total_nb_schedulers += model->schedulers[j].nb_instances;
    }
    for(j=0; j < model->nb_worker_types; j++) {
        total_nb_workers += model->workers[j].nb_instances;
    }
    for(j=0; j < model->nb_executor_types; j++) {
        total_nb_executors += model->executors[j].nb_instances;
    }
    for(j=0; j < model->nb_workpile_types; j++) {
        total_nb_workpiles += model->workpiles[j].nb_instances;
    }
    for(j=0; j < model->numAllocTypes; ++j) {
        totalNumAllocators += model->allocators[j].model.nb_instances;
    }
    for(j=0; j < model->numMemTypes; ++j) {
        totalNumMemories += model->memories[j].nb_instances;
    }

    int n_policy_domains = model->model.nb_instances;
    ocr_policy_domain_t ** policyDomains = (ocr_policy_domain_t **) malloc( sizeof(ocr_policy_domain_t*) * n_policy_domains );

    int idx;
    for ( idx = 0; idx < n_policy_domains; ++idx ) {
	// Create an instance of the policy domain
	policyDomains[idx] = newPolicy(model->model.kind,
		total_nb_workpiles, total_nb_workers,
		total_nb_executors, total_nb_schedulers);

	// Allocate memory for ocr components
	// Components instances are grouped into one big chunk of memory
	ocr_scheduler_t ** all_schedulers = (ocr_scheduler_t **) malloc(sizeof(ocr_scheduler_t *) * total_nb_schedulers);
	ocr_worker_t ** all_workers = (ocr_worker_t **) malloc(sizeof(ocr_worker_t *) * total_nb_workers);
	ocr_executor_t ** all_executors = (ocr_executor_t **) malloc(sizeof(ocr_executor_t *) * total_nb_executors);
	ocr_workpile_t ** all_workpiles = (ocr_workpile_t **) malloc(sizeof(ocr_workpile_t *) * total_nb_workpiles);
	ocrAllocator_t ** all_allocators = (ocrAllocator_t **) malloc(sizeof(ocrAllocator_t*) * totalNumAllocators);
	ocrLowMemory_t ** all_memories = (ocrLowMemory_t **) malloc(sizeof(ocrLowMemory_t*) * totalNumMemories);

	//
	// Build instances of each ocr modules
	//
	//TODO would be nice to make creation more generic

	create_configure_all_schedulers ( all_schedulers, model->nb_scheduler_types, model->schedulers);
	create_configure_all_workers ( all_workers, model->nb_worker_types, model->workers, policyDomains[idx]);
	create_configure_all_executors ( all_executors, model->nb_executor_types, model->executors);
	create_configure_all_workpiles ( all_workpiles, model->nb_workpile_types, model->workpiles);
	create_configure_all_allocators ( all_allocators, model->numAllocTypes, model->allocators);
	create_configure_all_memories ( all_memories, model->numMemTypes, model->memories);


	policyDomains[idx]->create(policyDomains[idx], NULL, all_schedulers,
		all_workers, all_executors, all_workpiles,
		all_allocators, all_memories);

	// This is only needed because we want to be able to
	// write generic code to find instances' backing arrays
	// given a module kind.
	size_t nb_ocr_modules = 6;
	ocr_module_instance modules_kinds[nb_ocr_modules];
	modules_kinds[0].kind = OCR_WORKER;
	modules_kinds[0].nb_instances = total_nb_workers;
	modules_kinds[0].instances = (void **) all_workers;

	modules_kinds[1].kind = OCR_EXECUTOR;
	modules_kinds[1].nb_instances = total_nb_executors;
	modules_kinds[1].instances = (void **) all_executors;

	modules_kinds[2].kind = OCR_WORKPILE;
	modules_kinds[2].nb_instances = total_nb_workpiles;
	modules_kinds[2].instances = (void **) all_workpiles;

	modules_kinds[3].kind = OCR_SCHEDULER;
	modules_kinds[3].nb_instances = total_nb_schedulers;
	modules_kinds[3].instances = (void **) all_schedulers;

	modules_kinds[4].kind = OCR_ALLOCATOR;
	modules_kinds[4].nb_instances = totalNumAllocators;
	modules_kinds[4].instances = (void**)all_allocators;

	modules_kinds[5].kind = OCR_MEMORY;
	modules_kinds[5].nb_instances = totalNumMemories;
	modules_kinds[5].instances = (void**)all_memories;

	//
	// Bind instances of each ocr components through mapping functions
	//   - Binding is the last thing we do as we may need information set
	//     during the 'create' phase
	//
	size_t instance, nb_instances_from, nb_instances_to;
	void ** from_instances;
	void ** to_instances;
	for ( instance = 0; instance < model->nb_mappings; ++instance ) {
	    ocr_module_mapping_t * mapping = (model->mappings + instance);
	    // Resolve modules instances we want to map
	    resolve_module_instances(mapping->from, nb_ocr_modules, modules_kinds, &nb_instances_from, &from_instances);
	    resolve_module_instances(mapping->to, nb_ocr_modules, modules_kinds, &nb_instances_to, &to_instances);
	    // Resolve mapping function to use and call it
	    ocr_map_module_fct map_fct = get_module_mapping_function(mapping->kind);

	    map_fct(mapping->from, mapping->to,
		    nb_instances_from, (ocr_module_t **) from_instances,
		    nb_instances_to, (ocr_module_t **) to_instances);
	}

    }
    return policyDomains;
}
