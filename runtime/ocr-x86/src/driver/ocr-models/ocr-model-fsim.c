/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ocr-runtime.h"
#include "ocr-config.h"
#include "ocr-guid.h"

/**
 * FSIM XE policy domain has:
 * one XE scheduler for all XEs
 * one worker for each XEs
 * one comp-target for each XEs
 * two workpile for each XEs, one for real work, one for CE messages
 * one memory for all XEs
 * one allocator for all XEs
 *
 * configures workers to have IDs of (#CE..#CE+#XE]
 *
 */

// Fwd declarations
static ocr_model_policy_t * createXeModelPolicies ( u64 nb_CEs, u64 nb_XE_per_CEs );
static ocr_model_policy_t * createCeModelPolicies ( u64 nb_CEs, u64 nb_XE_per_CEs );
static ocr_model_policy_t * createCeMasteredModelPolicy ( u64 nb_XE_per_CEs );

void ocrModelInitFsim(char * mdFile) {
    // sagnak TODO handle nb_CEs <= 1 case
    u64 nb_CEs = 2;
    u64 nb_XE_per_CEs = 3;
    u64 nb_XEs = nb_XE_per_CEs * nb_CEs;

    ocr_model_policy_t * xe_policy_models = createXeModelPolicies ( nb_CEs, nb_XE_per_CEs );
    ocr_model_policy_t * ce_policy_models = NULL;
    if ( nb_CEs > 1 )
        ce_policy_models = createCeModelPolicies ( nb_CEs-1, nb_XE_per_CEs );
    ocr_model_policy_t * ce_mastered_policy_model = createCeMasteredModelPolicy( nb_XE_per_CEs );

    ocrPolicyDomain_t ** xe_policy_domains = instantiateModel(xe_policy_models);
    ocrPolicyDomain_t ** ce_policy_domains = NULL;
    if ( nb_CEs > 1 )
        ce_policy_domains = instantiateModel(ce_policy_models);
    ocrPolicyDomain_t ** ce_mastered_policy_domain = instantiateModel(ce_mastered_policy_model);

    n_root_policy_nodes = nb_CEs;
    root_policies = (ocrPolicyDomain_t**) malloc(sizeof(ocrPolicyDomain_t*)*nb_CEs);

    root_policies[0] = ce_mastered_policy_domain[0];

    u64 idx = 0;
    for ( idx = 0; idx < nb_CEs-1; ++idx ) {
        root_policies[idx+1] = ce_policy_domains[idx];
    }

    idx = 0;
    ce_mastered_policy_domain[0]->n_successors = nb_XE_per_CEs;
    ce_mastered_policy_domain[0]->successors = &(xe_policy_domains[idx*nb_XE_per_CEs]);
    ce_mastered_policy_domain[0]->n_predecessors = 0;
    ce_mastered_policy_domain[0]->predecessors = NULL;

    for ( idx = 1; idx < nb_CEs; ++idx ) {
        ocrPolicyDomain_t *curr = ce_policy_domains[idx-1];

        curr->n_successors = nb_XE_per_CEs;
        curr->successors = &(xe_policy_domains[idx*nb_XE_per_CEs]);
        curr->n_predecessors = 0;
        curr->predecessors = NULL;
    }

    // sagnak: should this instead be recursive?
    for ( idx = 0; idx < nb_XE_per_CEs; ++idx ) {
        ocrPolicyDomain_t *curr = xe_policy_domains[idx];

        curr->n_successors = 0;
        curr->successors = NULL;
        curr->n_predecessors = 1;
        curr->predecessors = ce_mastered_policy_domain;
    }

    for ( idx = nb_XE_per_CEs; idx < nb_XEs; ++idx ) {
        ocrPolicyDomain_t *curr = xe_policy_domains[idx];

        curr->n_successors = 0;
        curr->successors = NULL;
        curr->n_predecessors = 1;
        curr->predecessors = &(ce_policy_domains[idx/nb_XE_per_CEs-1]);
    }

    ce_mastered_policy_domain[0]->start(ce_mastered_policy_domain[0]);

    for ( idx = 1; idx < nb_CEs; ++idx ) {
        ocrPolicyDomain_t *curr = ce_policy_domains[idx-1];
        curr->start(curr);
    }
    for ( idx = 0; idx < nb_XEs; ++idx ) {
        ocrPolicyDomain_t *curr = xe_policy_domains[idx];
        curr->start(curr);
    }

    master_worker = ce_mastered_policy_domain[0]->workers[0];
}

static ocr_model_policy_t * createXeModelPolicies ( u64 nb_CEs, u64 nb_XEs_per_CE ) {
    u64 nb_per_xe_schedulers = 1;
    u64 nb_per_xe_workers = 1;
    u64 nb_per_xe_comp_targets = 1;
    u64 nb_per_xe_workpiles = 2;
    u64 nb_per_xe_memories = 1;
    u64 nb_per_xe_allocators = 1;

    u64 nb_XEs = nb_CEs * nb_XEs_per_CE;

    // there are #XE instances of a model
    ocr_model_policy_t * xePolicyModel = (ocr_model_policy_t *) malloc(sizeof(ocr_model_policy_t));
    xePolicyModel->model.kind = OCR_POLICY_XE;
    xePolicyModel->model.nb_instances = nb_XEs;
    xePolicyModel->model.perTypeConfig = NULL;
    xePolicyModel->model.perInstanceConfig = NULL;

    xePolicyModel->nb_scheduler_types = 1;
    xePolicyModel->nb_worker_types = 1;
    xePolicyModel->nb_comp_target_types = 1;
    xePolicyModel->nb_workpile_types = 1;
    xePolicyModel->numMemTypes = 1;
    xePolicyModel->numAllocTypes = 1;

    // XE scheduler
    u64 index_ce, index_xe;
    u64 index_config = 0;
    u64 worker_id_offset = 1;
    void** scheduler_configurations = malloc(sizeof(scheduler_configuration*)*nb_XEs*nb_per_xe_schedulers);
    for ( index_config = 0; index_config < nb_XEs; ++index_config ) {
        scheduler_configurations[index_config] = (scheduler_configuration*) malloc(sizeof(scheduler_configuration));
    }

    index_config = 0;
    for ( index_ce = 0; index_ce < nb_CEs; ++index_ce ) {
        for ( index_xe = 0; index_xe < nb_XEs_per_CE; ++index_xe, ++index_config ) {
            scheduler_configuration* curr_config = (scheduler_configuration*)scheduler_configurations[index_config];
            curr_config->worker_id_begin = index_xe + worker_id_offset;
            curr_config->worker_id_end = index_xe + worker_id_offset;
        }
        worker_id_offset += (1 + nb_XEs_per_CE); // because nothing says nasty code like your own strength reduction
    }

    index_config = 0;
    u64 n_all_workers = nb_XEs*nb_per_xe_workers;
    void** worker_configurations = malloc(sizeof(worker_configuration*)*n_all_workers );
    for ( index_config = 0; index_config < n_all_workers; ++index_config ) {
        worker_configurations[index_config] = (worker_configuration*) malloc(sizeof(worker_configuration));
    }

    index_config = 0;
    worker_id_offset = 1;
    for ( index_ce = 0; index_ce < nb_CEs; ++index_ce ) {
        for ( index_xe = 0; index_xe < nb_XEs_per_CE; ++index_xe, ++index_config ) {
            worker_configuration* curr_config = (worker_configuration*)worker_configurations[index_config];
            curr_config->worker_id = index_xe + worker_id_offset;
        }
        worker_id_offset += (1 + nb_XEs_per_CE); // because nothing says nasty code like your own strength reduction
    }

    xePolicyModel->schedulers = newModel ( ocr_scheduler_xe_kind, nb_per_xe_schedulers, NULL, scheduler_configurations );
    xePolicyModel->workers    = newModel ( ocr_worker_xe_kind, nb_per_xe_workers, NULL, worker_configurations );
    xePolicyModel->computes  = newModel ( ocr_comp_target_xe_kind, nb_per_xe_comp_targets, NULL, NULL );
    xePolicyModel->workpiles  = newModel ( ocr_workpile_xe_kind, nb_per_xe_workpiles, NULL, NULL );
    xePolicyModel->memories   = newModel ( ocrMemPlatformXEKind, nb_per_xe_memories, NULL, NULL );

    // XE allocator
    ocrAllocatorModel_t *xeAllocator = (ocrAllocatorModel_t*)malloc(sizeof(ocrAllocatorModel_t));
    xeAllocator->model.perTypeConfig = NULL;
    xeAllocator->model.perInstanceConfig = NULL;
    xeAllocator->model.kind = ocrAllocatorXEKind;
    xeAllocator->model.nb_instances = nb_per_xe_allocators;
    xeAllocator->sizeManaged = gHackTotalMemSize;
    xePolicyModel->allocators = xeAllocator;

    // Defines how ocr modules are bound together
    u64 nb_module_mappings = 5;
    ocr_module_mapping_t * xeMapping =
        (ocr_module_mapping_t *) malloc(sizeof(ocr_module_mapping_t) * nb_module_mappings);
    // Note: this doesn't bind modules magically. You need to have a mapping function defined
    //       and set in the targeted implementation (see ocr_scheduler_hc implementation for reference).
    //       These just make sure the mapping functions you have defined are called
    xeMapping[0] = build_ocr_module_mapping(MANY_TO_ONE_MAPPING, OCR_WORKPILE, OCR_SCHEDULER);
    xeMapping[1] = build_ocr_module_mapping(ONE_TO_ONE_MAPPING, OCR_WORKER, OCR_COMP_TARGET);
    xeMapping[2] = build_ocr_module_mapping(ONE_TO_MANY_MAPPING, OCR_SCHEDULER, OCR_WORKER);
    xeMapping[3] = build_ocr_module_mapping(MANY_TO_ONE_MAPPING, OCR_MEMORY, OCR_ALLOCATOR);
    xeMapping[4] = build_ocr_module_mapping(ONE_TO_ONE_MAPPING, OCR_SCHEDULER, OCR_POLICY);
    xePolicyModel->nb_mappings = nb_module_mappings;
    xePolicyModel->mappings = xeMapping;

    return xePolicyModel;
}
/**
 * FSIM CE policy domains has:
 * one CE scheduler for all CEs
 * one worker for each CEs
 * one comp-target for each CEs
 * two workpile for each CEs, one for real work, one for XE messages
 * one memory for all CEs
 * one allocator for all CEs
 */

void CEModelPoliciesHelper ( ocr_model_policy_t * cePolicyModel ) {
    u64 nb_ce_comp_targets = 1;
    u64 nb_ce_memories = 1;
    u64 nb_ce_allocators = 1;

    cePolicyModel->nb_scheduler_types = 1;
    cePolicyModel->nb_worker_types = 1;
    cePolicyModel->nb_comp_target_types = 1;
    cePolicyModel->nb_workpile_types = 2;
    cePolicyModel->numMemTypes = 1;
    cePolicyModel->numAllocTypes = 1;

    cePolicyModel->computes = newModel ( ocr_comp_target_ce_kind, nb_ce_comp_targets, NULL, NULL );
    cePolicyModel->memories  = newModel ( ocrMemPlatformCEKind, nb_ce_memories, NULL, NULL );

    // CE workpile
    ocr_model_t * ceWorkpiles = (ocr_model_t *) malloc(sizeof(ocr_model_t)*2);
    ceWorkpiles[0] = (ocr_model_t){.kind =    ocr_workpile_ce_work_kind, .nb_instances = 1, .perTypeConfig = NULL, .perInstanceConfig = NULL };
    ceWorkpiles[1] = (ocr_model_t){.kind = ocr_workpile_ce_message_kind, .nb_instances = 1, .perTypeConfig = NULL, .perInstanceConfig = NULL };
    cePolicyModel->workpiles = ceWorkpiles;

    // CE allocator
    ocrAllocatorModel_t *ceAllocator = (ocrAllocatorModel_t*)malloc(sizeof(ocrAllocatorModel_t));
    ceAllocator->model.perTypeConfig = NULL;
    ceAllocator->model.perInstanceConfig = NULL;
    ceAllocator->model.kind = ocrAllocatorXEKind;
    ceAllocator->model.nb_instances = nb_ce_allocators;
    ceAllocator->sizeManaged = gHackTotalMemSize;
    cePolicyModel->allocators = ceAllocator;

    // Defines how ocr modules are bound together
    u64 nb_module_mappings = 5;
    ocr_module_mapping_t * ceMapping =
        (ocr_module_mapping_t *) malloc(sizeof(ocr_module_mapping_t) * nb_module_mappings);
    // Note: this doesn't bind modules magically. You need to have a mapping function defined
    //       and set in the targeted implementation (see ocr_scheduler_hc implementation for reference).
    //       These just make sure the mapping functions you have defined are called
    ceMapping[0] = build_ocr_module_mapping(MANY_TO_ONE_MAPPING, OCR_WORKPILE, OCR_SCHEDULER);
    ceMapping[1] = build_ocr_module_mapping(ONE_TO_ONE_MAPPING, OCR_WORKER, OCR_COMP_TARGET);
    ceMapping[2] = build_ocr_module_mapping(ONE_TO_MANY_MAPPING, OCR_SCHEDULER, OCR_WORKER);
    ceMapping[3] = build_ocr_module_mapping(MANY_TO_ONE_MAPPING, OCR_MEMORY, OCR_ALLOCATOR);
    ceMapping[4] = build_ocr_module_mapping(ONE_TO_ONE_MAPPING, OCR_SCHEDULER, OCR_POLICY);
    cePolicyModel->nb_mappings = nb_module_mappings;
    cePolicyModel->mappings = ceMapping;


}

static ocr_model_policy_t * createCeModelPolicies ( u64 nb_CEs, u64 nb_XEs_per_CE ) {
    ocr_model_policy_t * cePolicyModel = (ocr_model_policy_t *) malloc(sizeof(ocr_model_policy_t));

    u64 nb_ce_schedulers = 1;
    u64 nb_ce_workers = 1;

    cePolicyModel->model.kind = ocr_policy_ce_kind;
    cePolicyModel->model.perTypeConfig = NULL;
    cePolicyModel->model.perInstanceConfig = NULL;
    cePolicyModel->model.nb_instances = nb_CEs;

    u64 index_config = 0, n_all_schedulers = nb_ce_schedulers*nb_CEs;
    void** scheduler_configurations = malloc(sizeof(scheduler_configuration*)*n_all_schedulers);
    for ( index_config = 0; index_config < n_all_schedulers; ++index_config ) {
        scheduler_configurations[index_config] = (scheduler_configuration*) malloc(sizeof(scheduler_configuration));
        scheduler_configuration* curr_config = (scheduler_configuration*)scheduler_configurations[index_config];
        curr_config->worker_id_begin = (1+nb_XEs_per_CE) * (1+index_config);
        curr_config->worker_id_end = (1+nb_XEs_per_CE) * (1+index_config) + nb_ce_workers - 1 ;
    }

    index_config = 0;
    u64 n_all_workers = nb_ce_workers*nb_CEs;
    void** worker_configurations = malloc(sizeof(worker_configuration*)*n_all_workers );
    for ( index_config = 0; index_config < n_all_workers; ++index_config ) {
        worker_configurations[index_config] = (worker_configuration*) malloc(sizeof(worker_configuration));
        worker_configuration* curr_config = (worker_configuration*)worker_configurations[index_config];
        curr_config->worker_id = (1+nb_XEs_per_CE) * (1+index_config);
    }

    cePolicyModel->schedulers = newModel ( ocr_scheduler_ce_kind, nb_ce_schedulers, NULL, scheduler_configurations);
    cePolicyModel->workers = newModel ( ocr_worker_ce_kind, nb_ce_workers, NULL, worker_configurations);

    CEModelPoliciesHelper(cePolicyModel);
    return cePolicyModel;
}

static ocr_model_policy_t * createCeMasteredModelPolicy ( u64 nb_XEs_per_CE ) {
    ocr_model_policy_t * cePolicyModel = (ocr_model_policy_t *) malloc(sizeof(ocr_model_policy_t));

    u64 nb_ce_schedulers = 1;
    u64 nb_ce_workers = 1;

    cePolicyModel->model.kind = ocr_policy_ce_mastered_kind;
    cePolicyModel->model.perTypeConfig = NULL;
    cePolicyModel->model.perInstanceConfig = NULL;
    cePolicyModel->model.nb_instances = 1;

    // Mastered-CE scheduler
    u64 index_config = 0, n_all_schedulers = nb_ce_schedulers;
    void** scheduler_configurations = malloc(sizeof(scheduler_configuration*)*n_all_schedulers);
    for ( index_config = 0; index_config < n_all_schedulers; ++index_config ) {
        scheduler_configurations[index_config] = (scheduler_configuration*) malloc(sizeof(scheduler_configuration));
        scheduler_configuration* curr_config = (scheduler_configuration*)scheduler_configurations[index_config];
        curr_config->worker_id_begin = 0;
        curr_config->worker_id_end = curr_config->worker_id_begin + nb_ce_workers - 1;
    }

    index_config = 0;
    u64 n_all_workers = nb_ce_workers;
    void** worker_configurations = malloc(sizeof(worker_configuration*)*n_all_workers );
    for ( index_config = 0; index_config < n_all_workers; ++index_config ) {
        worker_configurations[index_config] = (worker_configuration*) malloc(sizeof(worker_configuration));
        worker_configuration* curr_config = (worker_configuration*)worker_configurations[index_config];
        curr_config->worker_id = index_config;
    }

    cePolicyModel->schedulers = newModel ( ocr_scheduler_ce_kind, nb_ce_schedulers, NULL, scheduler_configurations);
    cePolicyModel->workers = newModel ( ocr_worker_ce_kind, nb_ce_workers, NULL, worker_configurations);

    CEModelPoliciesHelper(cePolicyModel);
    return cePolicyModel;
}
