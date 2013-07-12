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
#include "debug.h"

/******************************************************/
/* OCR-HC SCHEDULER                                   */
/******************************************************/

static inline ocrWorkpile_t * pop_mapping_one_to_one (ocrScheduler_t* base, u64 workerId ) {
    u64 idx = (workerId - ((ocrSchedulerHc_t*)base)->worker_id_first);
    return base->workpiles[idx];
}

static inline ocrWorkpile_t * push_mapping_one_to_one (ocrScheduler_t* base, u64 workerId ) {
    u64 idx = (workerId - ((ocrSchedulerHc_t*)base)->worker_id_first);
    return base->workpiles[idx];
}

static inline ocrWorkpileIterator_t* steal_mapping_one_to_all_but_self (ocrScheduler_t* base, u64 workerId ) {
    u64 idx = (workerId - ((ocrSchedulerHc_t*)base)->worker_id_first);
    ocrSchedulerHc_t* derived = (ocrSchedulerHc_t*) base;
    ocrWorkpileIterator_t * steal_iterator = derived->steal_iterators[idx];
    steal_iterator->reset(steal_iterator);
    return steal_iterator;
}

void hcSchedulerStart(ocrScheduler_t * self, ocrPolicyDomain_t * PD) {
    ocrSchedulerHc_t * derived = (ocrSchedulerHc_t *) self;
    u64 workpileCount = self->workpileCount;
    ocrWorkpile_t ** workpiles = self->workpiles;
    // allocate steal iterator cache
    ocrWorkpileIterator_t ** steal_iterators_cache = checkedMalloc(steal_iterators_cache, sizeof(ocrWorkpileIterator_t *)*workpileCount);
    // Initialize steal iterator cache
    u64 i = 0;
    while(i < workpileCount) {
        // Note: here we assume workpile 'i' will match worker 'i' => Not great
        steal_iterators_cache[i] = workpileIteratorConstructor(i, workpileCount, workpiles);
        i++;
    }
    derived->steal_iterators = steal_iterators_cache;
}

void hcSchedulerStop(ocrScheduler_t * self) {
  // nothing to do here
}

u8 hcSchedulerYield (ocrScheduler_t* self, ocrGuid_t workerGuid, 
                       ocrGuid_t yieldingEdtGuid, ocrGuid_t eventToYieldForGuid, 
                       ocrGuid_t * returnGuid,
                       ocrPolicyCtx_t *context) {
    // We do not yet take advantage of knowing which EDT we are yielding for.
    ocrPolicyDomain_t * pd = context->PD;
    ocrWorker_t * worker = NULL;
    deguidify(pd, workerGuid, (u64*)&(worker), NULL);
    // Retrieve currently executing edt's guid
    ocrEvent_t * eventToYieldFor = NULL;
    deguidify(pd, eventToYieldForGuid, (u64*)&(eventToYieldFor), NULL);

    ocrPolicyCtx_t * orgCtx = getCurrentWorkerContext();
    ocrPolicyCtx_t * ctx = orgCtx->clone(orgCtx);
    ctx->type = PD_MSG_EDT_TAKE;

    ocrGuid_t result = ERROR_GUID;
    //This only works for single events, not latches
    ASSERT(isEventSingleGuid(eventToYieldForGuid));
    while((result = eventToYieldFor->fctPtrs->get(eventToYieldFor, 0)) == ERROR_GUID) {
        u32 count;
        ocrGuid_t taskGuid;
        pd->takeEdt(pd, NULL, &count, &taskGuid, ctx);
        ocrTask_t* task = NULL;
        deguidify(pd, taskGuid, (u64*)&(task), NULL);
        if (taskGuid != NULL_GUID) {
            worker->fctPtrs->execute(worker, task, taskGuid, yieldingEdtGuid);
        }
    }
    *returnGuid = result;
    ctx->destruct(ctx);
    return 0;
}

static u8 hcSchedulerTake (ocrScheduler_t *self, struct _ocrCost_t *cost, u32 *count,
                  ocrGuid_t *edts, ocrPolicyCtx_t *context) {
    // In this implementation (getCurrentPD == context->sourcePD)
    // Source must be a worker guid and we rely on indices to map
    // workers to workpiles (one-to-one)
    u64 workerId = context->sourceId;
    // First try to pop
    ocrWorkpile_t * wp_to_pop = pop_mapping_one_to_one(self, workerId);
    // TODO sagnak, just to get it to compile, I am trickling down the 'cost' though it most probably is not the same
    ocrGuid_t popped = wp_to_pop->fctPtrs->pop(wp_to_pop,cost);
    if ( NULL_GUID == popped ) {
        // If popping failed, try to steal
        ocrWorkpileIterator_t* it = steal_mapping_one_to_all_but_self(self, workerId);
        while ( it->hasNext(it) && (NULL_GUID == popped)) {
            ocrWorkpile_t * next = it->next(it);
            // TODO sagnak, just to get it to compile, I am trickling down the 'cost' though it most probably is not the same
            popped = next->fctPtrs->steal(next, cost);
        }
        // Note that we do not need to destruct the workpile
        // iterator as the HC implementation caches them.
    }
    // Int this implementation we expect the caller to have
    // allocated memory for us since we can return at most one
    // guid (most likely store using the address of a local)
    if (NULL_GUID != popped) {
      *count = 1;
      *edts = popped;
    } else {
      *count = 0;
    }
    return 0;
}

u8 hcSchedulerGive (ocrScheduler_t* base, u32 count, ocrGuid_t* edts, struct _ocrPolicyCtx_t *context ) {
    // Source must be a worker guid
    u64 workerId = context->sourceId;
    ocrWorkpile_t * wp_to_push = push_mapping_one_to_one(base, workerId);
    u32 i = 0;
    for ( ; i < count; ++i ) {
        wp_to_push->fctPtrs->push(wp_to_push,edts[i]);
    }
    return 0;
}

static void destructSchedulerHc(ocrScheduler_t * scheduler) {
    // Free the workpile steal iterator cache
    ocrSchedulerHc_t * derived = (ocrSchedulerHc_t *) scheduler;
    u64 workpileCount = scheduler->workpileCount;
    ocrWorkpileIterator_t ** steal_iterators = derived->steal_iterators;
    // Workpiles are deallocated by the policy domain
    u64 i = 0;
    while(i < workpileCount) {
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
    base->fctPtrs = &(factory->schedulerFcts);
    paramListSchedulerHcInst_t *mapper = (paramListSchedulerHcInst_t*)perInstance;
    derived->worker_id_first = mapper->worker_id_first;
    return base;
}

static void destructSchedulerFactoryHc(ocrSchedulerFactory_t * factory) {
    free(factory);
}

ocrSchedulerFactory_t * newOcrSchedulerFactoryHc(ocrParamList_t *perType) {
    ocrSchedulerFactoryHc_t* derived = (ocrSchedulerFactoryHc_t*) checkedMalloc(derived, sizeof(ocrSchedulerFactoryHc_t));
    ocrSchedulerFactory_t* base = (ocrSchedulerFactory_t*) derived;
    base->instantiate = newSchedulerHc;
    base->destruct = destructSchedulerFactoryHc;
    base->schedulerFcts.start = hcSchedulerStart;
    base->schedulerFcts.stop = hcSchedulerStop;
    base->schedulerFcts.yield = hcSchedulerYield;
    base->schedulerFcts.destruct = destructSchedulerHc;
    base->schedulerFcts.takeEdt = hcSchedulerTake;
    base->schedulerFcts.giveEdt = hcSchedulerGive;
    return base;
}
