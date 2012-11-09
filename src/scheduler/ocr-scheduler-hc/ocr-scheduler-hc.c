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
#include "hc.h"

/******************************************************/
/* OCR-HC SCHEDULER                                   */
/******************************************************/

void hc_scheduler_create(ocr_scheduler_t * scheduler, void * configuration) {
}

void hc_scheduler_destruct(ocr_scheduler_t * scheduler) {
    // just free self, workpiles are not allocated by the scheduler
    free(scheduler);
}

ocr_workpile_t * hc_scheduler_pop_mapping_one_to_one (ocr_scheduler_t* base, ocr_worker_t* w ) {
    hc_scheduler_t* derived = (hc_scheduler_t*) base;
    return derived->pools[get_worker_id(w)];
}

ocr_workpile_t * hc_scheduler_push_mapping_one_to_one (ocr_scheduler_t* base, ocr_worker_t* w ) {
    hc_scheduler_t* derived = (hc_scheduler_t*) base;
    return derived->pools[get_worker_id(w)];
}

workpile_iterator_t* hc_scheduler_steal_mapping_one_to_all_but_self (ocr_scheduler_t* base, ocr_worker_t* w ) {
    hc_scheduler_t* derived = (hc_scheduler_t*) base;
    return workpile_iterator_constructor(get_worker_id(w), derived->n_pools, derived->pools);
}

ocrGuid_t hc_scheduler_take (ocr_scheduler_t* base, ocrGuid_t wid ) {
    ocr_worker_t* w = (ocr_worker_t*) deguidify(wid);
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

void hc_scheduler_give (ocr_scheduler_t* base, ocrGuid_t wid, ocrGuid_t tid ) {
    ocr_worker_t* w = (ocr_worker_t*) deguidify(wid);
    ocr_workpile_t * wp_to_push = base->push_mapping(base, w);
    wp_to_push->push(wp_to_push,tid);
}

/**!
 * Mapping function many-to-one to map a set of workpiles to a scheduler instance
 */
void hc_ocr_module_map_workpiles_to_schedulers(void * self_module, ocr_module_kind kind,
        size_t nb_instances, void ** ptr_instances) {
    // Checking mapping conforms to what we're expecting in this implementation
    assert(kind == OCR_WORKPILE);
    hc_scheduler_t * scheduler = (hc_scheduler_t *) self_module;
    scheduler->n_pools = nb_instances;
    scheduler->pools = (ocr_workpile_t **)ptr_instances;
}

ocr_scheduler_t* hc_scheduler_constructor() {
    hc_scheduler_t* derived = (hc_scheduler_t*) malloc(sizeof(hc_scheduler_t));
    ocr_scheduler_t* base = (ocr_scheduler_t*)derived;
    ocr_module_t * module_base = (ocr_module_t *) base;
    module_base->map_fct = hc_ocr_module_map_workpiles_to_schedulers;
    base -> create = hc_scheduler_create;
    base -> destruct = hc_scheduler_destruct;
    base -> pop_mapping = hc_scheduler_pop_mapping_one_to_one;
    base -> push_mapping = hc_scheduler_push_mapping_one_to_one;
    base -> steal_mapping = hc_scheduler_steal_mapping_one_to_all_but_self;
    base -> take = hc_scheduler_take;
    base -> give = hc_scheduler_give;
    return base;
}
