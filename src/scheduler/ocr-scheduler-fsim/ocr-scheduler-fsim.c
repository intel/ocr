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

#include "ocr-runtime.h"
#include "fsim.h"


/******************************************************/
/* OCR-FSIM SCHEDULER                                 */
/******************************************************/

void xe_scheduler_create(ocr_scheduler_t * base, void * per_type_configuration, void * per_instance_configuration) {
    xe_scheduler_t* derived = (xe_scheduler_t*) base;
    fsim_scheduler_configuration *mapper = (fsim_scheduler_configuration*)per_instance_configuration;

    derived->worker_id_begin = mapper->worker_id_begin;
    derived->worker_id_end = mapper->worker_id_end;
    derived->n_workers_per_scheduler = 1 + derived->worker_id_end - derived->worker_id_begin;
}

void xe_scheduler_destruct(ocr_scheduler_t * scheduler) {
    // just free self, workpiles are not allocated by the scheduler
    free(scheduler);
}

ocr_workpile_t * xe_scheduler_pop_mapping_one_to_one (ocr_scheduler_t* base, ocr_worker_t* w ) {
    xe_scheduler_t* derived = (xe_scheduler_t*) base;
    return derived->pools[get_worker_id(w) % derived->n_workers_per_scheduler];
}

ocr_workpile_t * xe_scheduler_push_mapping_one_to_one (ocr_scheduler_t* base, ocr_worker_t* w ) {
    xe_scheduler_t* derived = (xe_scheduler_t*) base;
    return derived->pools[get_worker_id(w) % derived->n_workers_per_scheduler];
}

ocrGuid_t xe_scheduler_take (ocr_scheduler_t* base, ocrGuid_t wid ) {
    ocr_worker_t* w = NULL;
    globalGuidProvider->getVal(globalGuidProvider, wid, (u64*)&w, NULL);

    // try popping 'local'
    ocr_workpile_t * wp_to_pop = base->pop_mapping(base, w);
    ocrGuid_t popped = wp_to_pop->pop(wp_to_pop);

    // XEs do not steal from 'local' scheduler
    // TODO sagnak: do not 'hardcode' not-stealing from 'local' scheduler by omitting this part
    // it bakes in the assumption that there is only one workpile per XE scheduler, which may not be true
    
    // XEs do not take from 'non-local' scheduler from same policy domain
    // TODO sagnak: do not 'hardcode' not-stealing from 'non-local' scheduler by omitting this part
    // it bakes in the assumption that there is only one scheduler per XE policy domain, which may not be true

	// try stealing across 'policy domains'
    if ( NULL_GUID == popped ) {
        /*
        ocrGuid_t policy_domain_guid = base->domain;
        ocr_policy_domain_t* policy_domain = NULL; 
        globalGuidProvider->getVal(globalGuidProvider, policy_domain_guid, (u64*)&policy_domain, NULL);
        popped = policy_domain->take(policy_domain);
        */
    }
    return popped;
}

void xe_scheduler_give (ocr_scheduler_t* base, ocrGuid_t wid, ocrGuid_t tid ) {
    ocr_worker_t* w = NULL;
    globalGuidProvider->getVal(globalGuidProvider, wid, (u64*)&w, NULL);

    ocr_workpile_t * wp_to_push = base->push_mapping(base, w);
    wp_to_push->push(wp_to_push,tid);
}

/**!
 * Mapping function many-to-one to map a set of workpiles to a scheduler instance
 */
void xe_ocr_module_map_workpiles_to_schedulers(void * self_module, ocr_module_kind kind,
        size_t nb_instances, void ** ptr_instances) {
    // Checking mapping conforms to what we're expecting in this implementation
    assert(kind == OCR_WORKPILE);
    xe_scheduler_t * scheduler = (xe_scheduler_t *) self_module;
    scheduler->n_pools = nb_instances;
    scheduler->pools = (ocr_workpile_t **)ptr_instances;
}

ocr_scheduler_t* xe_scheduler_constructor() {
    xe_scheduler_t* derived = (xe_scheduler_t*) malloc(sizeof(xe_scheduler_t));
    ocr_scheduler_t* base = (ocr_scheduler_t*)derived;
    ocr_module_t * module_base = (ocr_module_t *) base;
    module_base->map_fct = xe_ocr_module_map_workpiles_to_schedulers;
    base -> create = xe_scheduler_create;
    base -> destruct = xe_scheduler_destruct;
    base -> pop_mapping = xe_scheduler_pop_mapping_one_to_one;
    base -> push_mapping = xe_scheduler_push_mapping_one_to_one;
// XEs do not steal on this configuration, this is not necessarily hardcoding
    base -> steal_mapping = NULL;
    base -> take = xe_scheduler_take;
    base -> give = xe_scheduler_give;
    return base;
}

void ce_scheduler_create(ocr_scheduler_t * base, void * per_type_configuration, void * per_instance_configuration) {
    ce_scheduler_t* derived = (ce_scheduler_t*) base;
    fsim_scheduler_configuration *mapper = (fsim_scheduler_configuration*)per_instance_configuration;

    derived->worker_id_begin = mapper->worker_id_begin;
    derived->worker_id_end = mapper->worker_id_end;
    derived->n_workers_per_scheduler = 1 + derived->worker_id_end - derived->worker_id_begin;
}

void ce_scheduler_destruct(ocr_scheduler_t * scheduler) {
    // just free self, workpiles are not allocated by the scheduler
    free(scheduler);
}

ocr_workpile_t * ce_scheduler_pop_mapping_one_to_one (ocr_scheduler_t* base, ocr_worker_t* w ) {
    ce_scheduler_t* derived = (ce_scheduler_t*) base;
    return derived->pools[get_worker_id(w) % derived->n_workers_per_scheduler];
}

ocr_workpile_t * ce_scheduler_push_mapping_one_to_one (ocr_scheduler_t* base, ocr_worker_t* w ) {
    ce_scheduler_t* derived = (ce_scheduler_t*) base;
    return derived->pools[get_worker_id(w) % derived->n_workers_per_scheduler];
}

workpile_iterator_t* ce_scheduler_steal_mapping_one_to_all_but_self (ocr_scheduler_t* base, ocr_worker_t* w ) {
    ce_scheduler_t* derived = (ce_scheduler_t*) base;
    return workpile_iterator_constructor(get_worker_id(w) % derived->n_workers_per_scheduler, derived->n_pools, derived->pools);
}

ocrGuid_t ce_scheduler_take (ocr_scheduler_t* base, ocrGuid_t wid ) {
    ocr_worker_t* w = NULL;
    globalGuidProvider->getVal(globalGuidProvider, wid, (u64*)&w, NULL);

    ocr_workpile_t * wp_to_pop = base->pop_mapping(base, w);
    ocrGuid_t popped = wp_to_pop->pop(wp_to_pop);
    if ( NULL_GUID == popped ) {
        workpile_iterator_t* it = base->steal_mapping(base, w);
        while ( it->hasNext(it) && (NULL_GUID == popped)) {
            ocr_workpile_t * next = it->next(it);
            popped = next->steal(next);
        }
        workpile_iterator_destructor(it);
    }
    return popped;
}

void ce_scheduler_give (ocr_scheduler_t* base, ocrGuid_t wid, ocrGuid_t tid ) {
    ocr_worker_t* w = NULL;
    globalGuidProvider->getVal(globalGuidProvider, wid, (u64*)&w, NULL);

    ocr_workpile_t * wp_to_push = base->push_mapping(base, w);
    wp_to_push->push(wp_to_push,tid);
}

/**!
 * Mapping function many-to-one to map a set of workpiles to a scheduler instance
 */
void ce_ocr_module_map_workpiles_to_schedulers(void * self_module, ocr_module_kind kind,
        size_t nb_instances, void ** ptr_instances) {
    // Checking mapping conforms to what we're expecting in this implementation
    assert(kind == OCR_WORKPILE);
    ce_scheduler_t * scheduler = (ce_scheduler_t *) self_module;
    scheduler->n_pools = nb_instances;
    scheduler->pools = (ocr_workpile_t **)ptr_instances;
}

ocr_scheduler_t* ce_scheduler_constructor() {
    ce_scheduler_t* derived = (ce_scheduler_t*) malloc(sizeof(ce_scheduler_t));
    ocr_scheduler_t* base = (ocr_scheduler_t*)derived;
    ocr_module_t * module_base = (ocr_module_t *) base;
    module_base->map_fct = ce_ocr_module_map_workpiles_to_schedulers;
    base -> create = ce_scheduler_create;
    base -> destruct = ce_scheduler_destruct;
    base -> pop_mapping = ce_scheduler_pop_mapping_one_to_one;
    base -> push_mapping = ce_scheduler_push_mapping_one_to_one;
    base -> steal_mapping = ce_scheduler_steal_mapping_one_to_all_but_self;
    base -> take = ce_scheduler_take;
    base -> give = ce_scheduler_give;
    return base;
}
