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

#include "ocr-macros.h"
#include "ocr-runtime.h"
#include "hc.h"


/******************************************************/
/* OCR-HC SCHEDULER                                   */
/******************************************************/

// Fwd declaration
ocrScheduler_t* newSchedulerHc(ocrSchedulerFactory_t * factory, ocrParamList_t *perInstance);

void destructSchedulerFactoryHc(ocrSchedulerFactory_t * factory) {
    free(factory);
}

ocrSchedulerFactory_t * newOcrSchedulerFactoryHc(ocrParamList_t *perType) {
    ocrSchedulerFactoryHc_t* derived = (ocrSchedulerFactoryHc_t*) checkedMalloc(derived, sizeof(ocrSchedulerFactoryHc_t));
    ocrSchedulerFactory_t* base = (ocrSchedulerFactory_t*) derived;
    base->instantiate = newSchedulerHc;
    base->destruct =  destructSchedulerFactoryHc;
    return base;
}

inline ocrWorkpile_t * pop_mapping_one_to_one (ocrScheduler_t* base, ocrWorker_t* w ) {
    ocrSchedulerHc_t* derived = (ocrSchedulerHc_t*) base;
    return derived->pools[get_worker_id(w) % derived->n_workers_per_scheduler ];
}

inline ocrWorkpile_t * push_mapping_one_to_one (ocrScheduler_t* base, ocrWorker_t* w ) {
    ocrSchedulerHc_t* derived = (ocrSchedulerHc_t*) base;
    return derived->pools[get_worker_id(w) % derived->n_workers_per_scheduler];
}

ocrWorkpileIterator_t* steal_mapping_one_to_all_but_self (ocrScheduler_t* base, ocrWorker_t* w ) {
    ocrSchedulerHc_t* derived = (ocrSchedulerHc_t*) base;
    ocrWorkpileIterator_t * steal_iterator = derived->steal_iterators[get_worker_id(w)];
    steal_iterator->reset(steal_iterator);
    return steal_iterator;
}


u8 (*takeEdt)(ocrScheduler_t *self, ocrCost_t *cost, u32 *count,
                  ocrGuid_t *edts, ocrPolicyCtx_t *context);

u8 ocrScheduler_t *self, ocrCost_t *cost, u32 *count,
                  ocrGuid_t *edts, ocrPolicyCtx_t *context_ {
// ocrGuid_t hc_scheduler_take (ocrScheduler_t* base, ocrGuid_t wid ) {
    ocrWorker_t* w = NULL;
    deguidify(getCurrentPD(), wid, (u64*)&w, NULL);
    // First try to pop
    // ocrWorkpile_t * wp_to_pop = self->pop_mapping(self, w);
    ocrWorkpile_t * wp_to_pop = pop_mapping_one_to_one(self, w);
    ocrGuid_t popped = wp_to_pop->pop(wp_to_pop);
    if ( NULL_GUID == popped ) {
        // If popping failed, try to steal
        ocrWorkpileIterator_t* it = steal_mapping_one_to_all_but_self(base, w);
        while ( it->hasNext(it) && (NULL_GUID == popped)) {
            ocrWorkpile_t * next = it->next(it);
            popped = next->fctPtrs->steal(next);
        }
        // Note that we do not need to destruct the workpile
        // iterator as the HC implementation caches them.
    }
    return popped;
}

void hc_scheduler_give (ocrScheduler_t* base, ocrGuid_t wid, ocrGuid_t tid ) {
    ocrWorker_t* w = NULL;
    deguidify(getCurrentPD(), wid, (u64*)&w, NULL);

    ocrWorkpile_t * wp_to_push = push_mapping_one_to_one(base, w);
    wp_to_push->push(wp_to_push,tid);
}

/**!
 * Mapping function many-to-one to map a set of workpiles to a scheduler instance
 */
void hc_ocr_module_map_workpiles_to_schedulers(void * self_module, ocrMappableKind kind,
                                               u64 nb_instances, void ** ptr_instances) {
    // Checking mapping conforms to what we're expecting in this implementation
    assert(kind == OCR_WORKPILE);
    // allocate steal iterator cache
    ocrWorkpileIterator_t ** steal_iterators_cache = checkedMalloc(steal_iterators_cache, sizeof(ocrWorkpileIterator_t *)*nb_instances);
    ocrSchedulerHc_t * scheduler = (ocrSchedulerHc_t *) self_module;
    scheduler->n_pools = nb_instances;
    scheduler->pools = (ocrWorkpile_t **)ptr_instances;
    // Initialize steal iterator cache
    int i = 0;
    while(i < nb_instances) {
        // Note: here we assume workpile 'i' will match worker 'i' => Not great
        steal_iterators_cache[i] = workpileIteratorConstructor(i, nb_instances, (ocrWorkpile_t **)ptr_instances);
        i++;
    }
    scheduler->steal_iterators = steal_iterators_cache;
}

void destructSchedulerHc(ocrScheduler_t * scheduler) {
    // Free the workpile steal iterator cache
    ocrSchedulerHc_t * hc_scheduler = (ocrSchedulerHc_t *) scheduler;
    int nb_instances = hc_scheduler->n_pools;
    ocrWorkpileIterator_t ** steal_iterators = hc_scheduler->steal_iterators;
    int i = 0;
    while(i < nb_instances) {
        workpile_iterator_destructor(steal_iterators[i]);
        i++;
    }
    free(steal_iterators);
    // free self (workpiles are not allocated by the scheduler)
    free(scheduler);
}

ocrScheduler_t* newSchedulerHc(ocrSchedulerFactory_t * factory, ocrParamList_t *perInstance) {
    ocrSchedulerHc_t* derived = (ocrSchedulerHc_t*) checkedMalloc(derived, sizeof(ocrSchedulerHc_t));
    ocrScheduler_t* base = (ocrScheduler_t*)derived;
    ocrMappable_t * module_base = (ocrMappable_t *) base;
    module_base->mapFct = hc_ocr_module_map_workpiles_to_schedulers;
    base->fctPtrs = &(factory->schedulerFcts);
    //TODO these need to be moved to the factory schedulerFcts
    base->fctPtrs->destruct = destructSchedulerHc;
    base->fctPtrs->pop_mapping = hc_scheduler_pop_mapping_one_to_one;
    base->fctPtrs->push_mapping = hc_scheduler_push_mapping_one_to_one;
    base->fctPtrs->steal_mapping = hc_scheduler_steal_mapping_one_to_all_but_self;
    base->fctPtrs->takeEdt = hc_scheduler_take;
    base->fctPtrs->giveEdt = hc_scheduler_give;
    //TODO END
    paramListSchedulerHcInst_t *mapper = (paramListSchedulerHcInst_t*)perInstance;
    derived->worker_id_begin = mapper->worker_id_begin;
    derived->worker_id_end = mapper->worker_id_end;
    derived->n_workers_per_scheduler = 1 + derived->worker_id_end - derived->worker_id_begin;

    return base;
}


/******************************************************/
/* OCR-HC-PLACED SCHEDULER                            */
/******************************************************/

ocrWorkpileIterator_t* steal_mapping_assert (ocrScheduler_t* base, ocrWorker_t* w ) {
    assert ( 0 && "We should not ask for a steal mapping from this scheduler");
    return NULL;
}

ocrGuid_t hc_placed_scheduler_take (ocrScheduler_t* base, ocrGuid_t wid ) {
    ocrWorker_t* w = NULL;
    deguidify(getCurrentPD(), wid, (u64*)&w, NULL);

    ocrGuid_t popped = NULL_GUID;

    int worker_id = get_worker_id(w);

    ocrSchedulerHc_t* derived = (ocrSchedulerHc_t*) base;
    if ( worker_id >= derived->worker_id_begin && worker_id <= derived->worker_id_end ) {
        ocrWorkpile_t * wp_to_pop = base->fctPtrs->pop_mapping(base, w);
        popped = wp_to_pop->fctPtrs->pop(wp_to_pop);
        /*TODO sagnak I hard-coded a no intra-scheduler stealing here; BAD */
        if ( NULL_GUID == popped ) {
            // TODO sagnak steal from places
            ocrPolicyDomain_t* policyDomain = base->domain;
            popped = policyDomain->handIn(policyDomain, policyDomain, wid);
        }
    } else {
        // TODO sagnak oooh BAD BAD hardcoding yet again
        ocrWorkpile_t* victim = derived->pools[0];
        popped = victim->fctPtrs->steal(victim);
    }

    return popped;
}

void hc_placed_scheduler_give (ocrScheduler_t* base, ocrGuid_t wid, ocrGuid_t tid ) {
    ocrWorker_t* w = NULL;
    deguidify(getCurrentPD(), wid, (u64*)&w, NULL);

    // TODO sagnak calculate which 'place' to push
    ocrWorkpile_t * wp_to_push = push_mapping_one_to_one(base, w);
    wp_to_push->push(wp_to_push,tid);
}

ocrScheduler_t* newSchedulerHcPlaced(ocrSchedulerFactory_t * factory, ocrParamList_t *perInstance) {
    // piggy-back on the regular HC scheduler but use different function pointers
    ocrSchedulerHc_t* derived = (ocrSchedulerHc_t*) malloc(sizeof(ocrSchedulerHc_t));
    ocrScheduler_t* base = (ocrScheduler_t*)derived;
    ocrMappable_t * module_base = (ocrMappable_t *) base;
    module_base->mapFct = hc_ocr_module_map_workpiles_to_schedulers;
    base->fctPtrs = &(factory->schedulerFcts);
    //TODO these need to be moved to the factory schedulerFcts
    base->fctPtrs->destruct = destructSchedulerHc;
    base->fctPtrs->pop_mapping = hc_scheduler_pop_mapping_one_to_one;
    base->fctPtrs->push_mapping = hc_scheduler_push_mapping_one_to_one;
    base->fctPtrs->steal_mapping = hc_scheduler_steal_mapping_assert;
    base->fctPtrs->takeEdt = hc_placed_scheduler_take;
    base->fctPtrs->giveEdt = hc_placed_scheduler_give;
    //TODO END

    paramListSchedulerHcInst_t *mapper = (paramListSchedulerHcInst_t*)perInstance;
    derived->worker_id_begin = mapper->worker_id_begin;
    derived->worker_id_end = mapper->worker_id_end;
    derived->n_workers_per_scheduler = 1 + derived->worker_id_end - derived->worker_id_begin;
    return base;
}

ocrSchedulerFactory_t * newOcrSchedulerFactoryHcPlaced(ocrParamList_t *perType) {
    ocrSchedulerFactoryHc_t* derived = (ocrSchedulerFactoryHc_t*) checkedMalloc(derived, sizeof(ocrSchedulerFactoryHc_t));
    ocrSchedulerFactory_t* base = (ocrSchedulerFactory_t*) derived;
    base->instantiate = newSchedulerHcPlaced;
    base->destruct =  destructSchedulerFactoryHc;
    return base;
}
