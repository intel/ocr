/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-macros.h"
#include "ocr-runtime.h"
#include "hc.h"

/******************************************************/
/* OCR-HC-PLACED SCHEDULER                            */
/******************************************************/

// Fwd declaration
ocrScheduler_t* newSchedulerHcPlaced(ocrSchedulerFactory_t * factory, ocrParamList_t *perInstance);

static void destructSchedulerFactoryHc(ocrSchedulerFactory_t * factory) {
    free(factory);
}

ocrSchedulerFactory_t * newOcrSchedulerFactoryHcPlaced(ocrParamList_t *perType) {
    ocrSchedulerFactoryHc_t* derived = (ocrSchedulerFactoryHc_t*) checkedMalloc(derived, sizeof(ocrSchedulerFactoryHc_t));
    ocrSchedulerFactory_t* base = (ocrSchedulerFactory_t*) derived;
    base->instantiate = newSchedulerHcPlaced;
    base->destruct =  destructSchedulerFactoryHc;
    return base;
}

static inline ocrWorkpile_t * pop_mapping_one_to_one (ocrScheduler_t* base, ocrWorker_t* w ) {
    ocrSchedulerHc_t* derived = (ocrSchedulerHc_t*) base;
    return derived->pools[get_worker_id(w) % derived->n_workers_per_scheduler ];
}

static inline ocrWorkpile_t * push_mapping_one_to_one (ocrScheduler_t* base, ocrWorker_t* w ) {
    ocrSchedulerHc_t* derived = (ocrSchedulerHc_t*) base;
    return derived->pools[get_worker_id(w) % derived->n_workers_per_scheduler];
}

ocrWorkpileIterator_t* steal_mapping_assert (ocrScheduler_t* base, ocrWorker_t* w ) {
    assert ( 0 && "We should not ask for a steal mapping from this scheduler");
    return NULL;
}

u8 hc_placed_scheduler_take (ocrScheduler_t *base, struct _ocrCost_t *cost, u32 *count,
                  ocrGuid_t *edts, struct _ocrPolicyCtx_t *context) {
    ocrWorker_t* w = NULL;
    ocrGuid_t wid = context->sourceObj;
    deguidify(getCurrentPD(), wid, (u64*)&w, NULL);

    ocrGuid_t popped = NULL_GUID;

    int worker_id = get_worker_id(w);

    ocrSchedulerHc_t* derived = (ocrSchedulerHc_t*) base;
    if ( worker_id >= derived->worker_id_begin && worker_id <= derived->worker_id_end ) {
        ocrWorkpile_t * wp_to_pop = pop_mapping_one_to_one(base, w);
        // TODO sagnak, just to get it to compile, I am trickling down the 'cost' though it most probably is not the same
        popped = wp_to_pop->fctPtrs->pop(wp_to_pop, cost);
        /*TODO sagnak I hard-coded a no intra-scheduler stealing here; BAD */
        if ( NULL_GUID == popped ) {
            // TODO sagnak steal from places
            ocrPolicyDomain_t* policyDomain = base->domain;
            popped = policyDomain->handIn(policyDomain, policyDomain, wid);
        }
    } else {
        // TODO sagnak oooh BAD BAD hardcoding yet again
        ocrWorkpile_t* victim = derived->pools[0];
        // TODO sagnak, just to get it to compile, I am trickling down the 'cost' though it most probably is not the same
        popped = victim->fctPtrs->steal(victim, cost);
    }
    // TODO sagnak, I am assuming the memory for count and edts are already allocated by here
    *count = 1;
    edts[0] = popped;
    return 0;
}

u8 hc_placed_scheduler_give (ocrScheduler_t* base, u32 count, ocrGuid_t* edts, struct _ocrPolicyCtx_t *context ) {
    ocrWorker_t* w = NULL;
    ocrGuid_t wid = context->sourceObj;
    deguidify(getCurrentPD(), wid, (u64*)&w, NULL);

    // TODO sagnak calculate which 'place' to push
    ocrWorkpile_t * wp_to_push = push_mapping_one_to_one(base, w);
    // TODO sagnak; I am assuming the count is the count of edt guids being passed
    u32 i = 0;
    for ( ; i < count; ++i ) {
        wp_to_push->fctPtrs->push(wp_to_push,edts[i]);
    }
    return 0;
}

/**!
 * Mapping function many-to-one to map a set of workpiles to a scheduler instance
 */
static void hc_ocr_module_map_workpiles_to_schedulers( ocrMappable_t * self_module, ocrMappableKind kind,
                                               u64 nb_instances, ocrMappable_t ** ptr_instances) {
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

static void destructSchedulerHc(ocrScheduler_t * scheduler) {
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

ocrScheduler_t* newSchedulerHcPlaced(ocrSchedulerFactory_t * factory, ocrParamList_t *perInstance) {
    // piggy-back on the regular HC scheduler but use different function pointers
    ocrSchedulerHc_t* derived = (ocrSchedulerHc_t*) malloc(sizeof(ocrSchedulerHc_t));
    ocrScheduler_t* base = (ocrScheduler_t*)derived;
    ocrMappable_t * module_base = (ocrMappable_t *) base;
    module_base->mapFct = hc_ocr_module_map_workpiles_to_schedulers;
    base->fctPtrs = &(factory->schedulerFcts);
    //TODO these need to be moved to the factory schedulerFcts
    base->fctPtrs->destruct = destructSchedulerHc;
    base->fctPtrs->takeEdt = hc_placed_scheduler_take;
    base->fctPtrs->giveEdt = hc_placed_scheduler_give;
    //TODO END

    paramListSchedulerHcInst_t *mapper = (paramListSchedulerHcInst_t*)perInstance;
    derived->worker_id_begin = mapper->worker_id_begin;
    derived->worker_id_end = mapper->worker_id_end;
    derived->n_workers_per_scheduler = 1 + derived->worker_id_end - derived->worker_id_begin;
    return base;
}
