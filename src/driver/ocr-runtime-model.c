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
#include "ocr-comp-target.h"
#include "ocr-worker.h"
#include "ocr-scheduler.h"
#include "ocr-policy-domain.h"
#include "ocr-runtime-model.h"
#include "ocr-config.h"

// External declaration for models
extern ocr_model_policy_t * ocrModelInitFsim(char * mdFile);
extern ocr_model_policy_t * ocrModelInitFlat(char * mdFile);
extern ocr_model_policy_t * ocrModelInitThor(char * mdFile);

ocr_model_policy_t * ocrInitPolicyModel(ocr_policy_model_kind policyModelKind, char * mdFile) {
    switch(policyModelKind) {
    case OCR_POLICY_MODEL_FLAT:
        return ocrModelInitFlat(mdFile);
    case OCR_POLICY_MODEL_FSIM:
        return ocrModelInitFsim(mdFile);
    case OCR_POLICY_MODEL_THOR:
        return ocrModelInitThor(mdFile);
    default:
        assert(false && "Unrecognized policy model");
        break;
    }

    return NULL;
}

/**!
 * Helper function to build a module mapping
 */
ocr_module_mapping_t build_ocr_module_mapping(ocr_mapping_kind kind, ocr_module_kind from, ocr_module_kind to) {
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
 * ex: maps each comp-target to each worker in a one-one fashion.
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

ocr_model_t* newModel ( int kind, int nInstances, void * per_type_configuration, void ** per_instance_configuration ) {
    ocr_model_t * model = (ocr_model_t *) malloc(sizeof(ocr_model_t));
    model->kind = kind;
    model->nb_instances = nInstances;
    model->per_type_configuration = per_type_configuration;
    model->per_instance_configuration = per_instance_configuration;
    return model;
}


void destructOcrModelPolicy(ocr_model_policy_t * model) {
    if (model->schedulers != NULL) {
        free(model->schedulers);
    }
    if (model->workers != NULL) {
        free(model->workers);
    }
    if (model->compTargets != NULL) {
        free(model->compTargets);
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

static  void create_configure_all_schedulers ( ocrScheduler_t ** all_schedulers, int n_policy_domains, int nb_component_types, ocr_model_t* components ) {
    size_t idx = 0, index_policy_domain = 0;
    for (; index_policy_domain < n_policy_domains; ++index_policy_domain ) {
        size_t type = 0, instance = 0;
        for (; type < nb_component_types; ++type) {
            // For each type, create the number of instances asked for.
            ocr_model_t curr_model = components[type];
            for ( instance = 0; instance < curr_model.nb_instances; ++instance, ++idx ) {
                // Call the factory method based on the model's type kind.
                all_schedulers[idx] = newScheduler(curr_model.kind, curr_model.per_type_configuration,
                                            (curr_model.per_instance_configuration) ? curr_model.per_instance_configuration[idx]: NULL );
            }
        }
    }
}

static  void create_configure_all_workpiles ( ocr_workpile_t ** all_workpiles, int n_policy_domains, int nb_component_types, ocr_model_t* components ) {
    size_t idx = 0, index_policy_domain = 0;
    for (; index_policy_domain < n_policy_domains; ++index_policy_domain ) {
        size_t type = 0, instance = 0;
        for (; type < nb_component_types; ++type) {
            // For each type, create the number of instances asked for.
            ocr_model_t curr_model = components[type];
            for ( instance = 0; instance < curr_model.nb_instances; ++instance, ++idx ) {
                // Call the factory method based on the model's type kind.
                all_workpiles[idx] = newWorkpile(curr_model.kind);
                all_workpiles[idx]->create(all_workpiles[idx], curr_model.per_type_configuration);
            }
        }
    }
}

static  void create_configure_all_workers ( ocr_worker_t** all_workers, int n_policy_domains, int nb_component_types, ocr_model_t* components ) {
    size_t idx = 0, index_policy_domain = 0;
    for (; index_policy_domain < n_policy_domains; ++index_policy_domain ) {
        size_t type = 0, instance = 0;
        for (; type < nb_component_types; ++type) {
            // For each type, create the number of instances asked for.
            ocr_model_t curr_model = components[type];
            for ( instance = 0; instance < curr_model.nb_instances; ++instance, ++idx ) {
                // Call the factory method based on the model's type kind.
                all_workers[idx] = newWorker(curr_model.kind);
                all_workers[idx]->create(all_workers[idx], curr_model.per_type_configuration,
                                         (curr_model.per_instance_configuration) ? curr_model.per_instance_configuration[idx]: NULL );
            }
        }
    }
}

static  void create_configure_all_comp_targets ( ocr_comp_target_t ** all_comp_targets, int n_policy_domains, int nb_component_types, ocr_model_t* components ) {
    size_t idx = 0, index_policy_domain = 0;
    for (; index_policy_domain < n_policy_domains; ++index_policy_domain ) {
        size_t type = 0, instance = 0;
        for (; type < nb_component_types; ++type) {
            // For each type, create the number of instances asked for.
            ocr_model_t curr_component_model = components[type];
            for ( instance = 0; instance < curr_component_model.nb_instances; ++instance, ++idx ) {
                // Call the factory method based on the model's type kind.
                all_comp_targets[idx] = newCompTarget(curr_component_model.kind);
                all_comp_targets[idx]->create(all_comp_targets[idx], curr_component_model.per_type_configuration);
            }
        }
    }
}

static  void create_configure_all_allocators ( ocrAllocator_t ** all_allocators, int n_policy_domains, int nb_component_types, ocrAllocatorModel_t* components ) {
    size_t idx = 0, index_policy_domain = 0;
    for (; index_policy_domain < n_policy_domains; ++index_policy_domain ) {
        size_t type = 0, instance = 0;
        for (; type < nb_component_types; ++type) {
            // For each type, create the number of instances asked for.
            ocrAllocatorModel_t curr_component_model = components[type];
            for ( instance = 0; instance < curr_component_model.model.nb_instances; ++instance, ++idx ) {
                // Call the factory method based on the model's type kind.
                all_allocators[idx] = newAllocator(curr_component_model.model.kind);
                all_allocators[idx]->create(all_allocators[idx], curr_component_model.sizeManaged, curr_component_model.model.per_type_configuration);
            }
        }
    }
}

static  void create_configure_all_memories ( ocrMemPlatform_t ** all_memories, int n_policy_domains, int nb_component_types, ocr_model_t* components ) {
    size_t idx = 0, index_policy_domain = 0;
    for (; index_policy_domain < n_policy_domains; ++index_policy_domain ) {
        size_t type = 0, instance = 0;
        for (; type < nb_component_types; ++type) {
            // For each type, create the number of instances asked for.
            ocr_model_t curr_component_model = components[type];
            for ( instance = 0; instance < curr_component_model.nb_instances; ++instance, ++idx ) {
                // Call the factory method based on the model's type kind.
                all_memories[idx] = newMemPlatform(curr_component_model.kind);
                all_memories[idx]->create(all_memories[idx],curr_component_model.per_type_configuration);
            }
        }
    }
}

/**!
 * Given a policy domain model, go over its modules and instantiate them.
 */
ocr_policy_domain_t ** instantiateModel(ocr_model_policy_t * model) {

    // Compute total number of workers, comp-targets and workpiles, allocators and memories
    int per_policy_domain_total_nb_schedulers = 0;
    int per_policy_domain_total_nb_workers = 0;
    int per_policy_domain_total_nb_comp_targets = 0;
    int per_policy_domain_total_nb_workpiles = 0;
    u64 totalNumMemories = 0;
    u64 totalNumAllocators = 0;
    size_t j = 0;
    for ( j = 0; j < model->nb_scheduler_types; ++j ) {
        per_policy_domain_total_nb_schedulers += model->schedulers[j].nb_instances;
    }
    for ( j = 0; j < model->nb_worker_types; ++j ) {
        per_policy_domain_total_nb_workers += model->workers[j].nb_instances;
    }
    for ( j = 0; j < model->nb_comp_target_types; ++j ) {
        per_policy_domain_total_nb_comp_targets += model->compTargets[j].nb_instances;
    }
    for ( j = 0; j < model->nb_workpile_types; ++j ) {
        per_policy_domain_total_nb_workpiles += model->workpiles[j].nb_instances;
    }
    for ( j = 0; j < model->numAllocTypes; ++j ) {
        totalNumAllocators += model->allocators[j].model.nb_instances;
    }
    for ( j = 0; j < model->numMemTypes; ++j ) {
        totalNumMemories += model->memories[j].nb_instances;
    }

    int n_policy_domains = model->model.nb_instances;
    ocr_policy_domain_t ** policyDomains = (ocr_policy_domain_t **) malloc( sizeof(ocr_policy_domain_t*) * n_policy_domains );

    // Allocate memory for ocr components
    // Components instances are grouped into one big chunk of memory
    ocrScheduler_t** all_schedulers = (ocrScheduler_t**) checked_malloc(all_schedulers, sizeof(ocrScheduler_t*) * per_policy_domain_total_nb_schedulers * n_policy_domains );
    ocr_worker_t   ** all_workers    = (ocr_worker_t   **) checked_malloc(all_workers, sizeof(ocr_worker_t   *) * per_policy_domain_total_nb_workers    * n_policy_domains );
    ocr_comp_target_t ** all_comp_targets  = (ocr_comp_target_t **) checked_malloc(all_comp_targets, sizeof(ocr_comp_target_t *) * per_policy_domain_total_nb_comp_targets  * n_policy_domains );
    ocr_workpile_t ** all_workpiles  = (ocr_workpile_t **) checked_malloc(all_workpiles, sizeof(ocr_workpile_t *) * per_policy_domain_total_nb_workpiles  * n_policy_domains );
    ocrAllocator_t ** all_allocators = (ocrAllocator_t **) checked_malloc(all_allocators, sizeof(ocrAllocator_t *) * totalNumAllocators * n_policy_domains );
    ocrMemPlatform_t ** all_memories   = (ocrMemPlatform_t **) checked_malloc(all_memories, sizeof(ocrMemPlatform_t *) * totalNumMemories * n_policy_domains );

    //
    // Build instances of each ocr modules
    //
    //TODO would be nice to make creation more generic

    create_configure_all_schedulers ( all_schedulers, n_policy_domains, model->nb_scheduler_types, model->schedulers);
    create_configure_all_workers    ( all_workers   , n_policy_domains, model->nb_worker_types, model->workers );
    create_configure_all_comp_targets  ( all_comp_targets , n_policy_domains, model->nb_comp_target_types, model->compTargets);
    create_configure_all_workpiles  ( all_workpiles , n_policy_domains, model->nb_workpile_types, model->workpiles);
    create_configure_all_allocators ( all_allocators, n_policy_domains, model->numAllocTypes, model->allocators);
    create_configure_all_memories   ( all_memories  , n_policy_domains, model->numMemTypes, model->memories);


    int idx;
    for ( idx = 0; idx < n_policy_domains; ++idx ) {

        ocrScheduler_t** schedulers = (ocrScheduler_t**) &(all_schedulers[ idx * per_policy_domain_total_nb_schedulers ]);
        ocr_worker_t   ** workers    = (ocr_worker_t   **) &(all_workers   [ idx * per_policy_domain_total_nb_workers    ]);
        ocr_comp_target_t ** compTargets  = (ocr_comp_target_t **) &(all_comp_targets [ idx * per_policy_domain_total_nb_comp_targets  ]);
        ocr_workpile_t ** workpiles  = (ocr_workpile_t **) &(all_workpiles [ idx * per_policy_domain_total_nb_workpiles  ]);
        ocrAllocator_t ** allocators = (ocrAllocator_t **) &(all_allocators[ idx * totalNumAllocators                    ]);
        ocrMemPlatform_t ** memories   = (ocrMemPlatform_t **) &(all_memories  [ idx * totalNumMemories                      ]);

        // Create an instance of the policy domain
        policyDomains[idx] = newPolicyDomain(model->model.kind,
                                       per_policy_domain_total_nb_workpiles, per_policy_domain_total_nb_workers,
                                       per_policy_domain_total_nb_comp_targets, per_policy_domain_total_nb_schedulers);

        policyDomains[idx]->create(policyDomains[idx], NULL, schedulers,
                                   workers, compTargets, workpiles, allocators, memories);

        // This is only needed because we want to be able to
        // write generic code to find instances' backing arrays
        // given a module kind.
        size_t nb_ocr_modules = 7;
        ocr_module_instance modules_kinds[nb_ocr_modules];
        modules_kinds[0] = (ocr_module_instance){.kind = OCR_WORKER,	.nb_instances = per_policy_domain_total_nb_workers,	.instances = (void **) workers};
        modules_kinds[1] = (ocr_module_instance){.kind = OCR_COMP_TARGET,	.nb_instances = per_policy_domain_total_nb_comp_targets,	.instances = (void **) compTargets};
        modules_kinds[2] = (ocr_module_instance){.kind = OCR_WORKPILE,	.nb_instances = per_policy_domain_total_nb_workpiles,	.instances = (void **) workpiles};
        modules_kinds[3] = (ocr_module_instance){.kind = OCR_SCHEDULER,	.nb_instances = per_policy_domain_total_nb_schedulers,	.instances = (void **) schedulers};
        modules_kinds[4] = (ocr_module_instance){.kind = OCR_ALLOCATOR,	.nb_instances = totalNumAllocators,	.instances = (void **) allocators};
        modules_kinds[5] = (ocr_module_instance){.kind = OCR_MEMORY,	.nb_instances = totalNumMemories,	.instances = (void **) memories};
        modules_kinds[6] = (ocr_module_instance){.kind = OCR_POLICY,	.nb_instances = 1,			.instances = (void **) &(policyDomains[idx])};

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
