/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_SCHEDULER_HC

#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"
#include "ocr-workpile.h"
#include "scheduler/hc/hc-scheduler.h"

/******************************************************/
/* Support structures                                 */
/******************************************************/
static inline void workpileIteratorReset(hcWorkpileIterator_t *base) {
    base->curr = ((base->id) + 1) % base->mod;
}

static inline bool workpileIteratorHasNext(hcWorkpileIterator_t * base) {
    return base->id != base->curr;
}

static inline ocrWorkpile_t * workpileIteratorNext(hcWorkpileIterator_t * base) {
    u64 current = base->curr;
    ocrWorkpile_t * toBeReturned = base->workpiles[current];
    base->curr = (current+1) % base->mod;
    return toBeReturned;
}

static inline void initWorkpileIterator(hcWorkpileIterator_t *base, u64 id,
                                        u64 workpileCount, ocrWorkpile_t ** workpiles ) {
    
    base->workpiles = workpiles;
    base->id = id;
    base->mod = workpileCount;
    // The 'curr' field is initialized by reset
    workpileIteratorReset(base);
}

/******************************************************/
/* OCR-HC SCHEDULER                                   */
/******************************************************/

static inline ocrWorkpile_t * popMappingOneToOne (ocrScheduler_t* base, u64 workerId ) {
    u64 idx = (workerId - ((ocrSchedulerHc_t*)base)->workerIdFirst);
    return base->workpiles[idx];
}

static inline ocrWorkpile_t * pushMappingOneToOne (ocrScheduler_t* base, u64 workerId ) {
    u64 idx = (workerId - ((ocrSchedulerHc_t*)base)->workerIdFirst);
    return base->workpiles[idx];
}

static inline hcWorkpileIterator_t* stealMappingOneToAllButSelf (ocrScheduler_t* base, u64 workerId ) {
    u64 idx = (workerId - ((ocrSchedulerHc_t*)base)->workerIdFirst);
    ocrSchedulerHc_t* derived = (ocrSchedulerHc_t*) base;
    hcWorkpileIterator_t * stealIterator = derived->stealIterators[idx];
    workpileIteratorReset(stealIterator);
    return stealIterator;
}

void hcSchedulerDestruct(ocrScheduler_t * scheduler) {
    // Destruct the workpiles
    u64 count = self->workpileCount;
    for(i = 0; i < count; ++i) {
        self->workpiles[i]->fctPtrs->destruct(self->workpiles[i]);
    }
    runtimeChunkFree((u64)(scheduler->workpiles), NULL);
    
    // Free the workpile steal iterator cache
    ocrSchedulerHc_t * derived = (ocrSchedulerHc_t *) scheduler;
    u64 workpileCount = scheduler->workpileCount;
    hcWorkpileIterator_t ** stealIterators = derived->stealIterators;
    
    while(i < workpileCount) {
        workpileIteratorDestruct(stealIterators[i]);
        i++;
    }
    free(stealIterators);
    // free self (workpiles are not allocated by the scheduler)
    free(scheduler);
}

void hcSchedulerStart(ocrScheduler_t * self, ocrPolicyDomain_t * PD) {
    
    // Get a GUID
    guidify(PD, (u64)self, &(self->guid), OCR_GUID_SCHEDULER);
    ocrSchedulerHc_t * derived = (ocrSchedulerHc_t *) self;
    
    u64 workpileCount = self->workpileCount;
    ocrWorkpile_t ** workpiles = self->workpiles;
    
    // allocate steal iterator cache. Use pdMalloc since this is something
    // local to the policy domain and that will never be shared
    hcWorkpileIterator_t ** stealIteratorsCache = PD->pdMalloc(
        PD, sizeof(hcWorkpileIterator_t)*workpileCount);
    
    // Initialize steal iterator cache
    u64 i = 0;
    while(i < workpileCount) {
        // Note: here we assume workpile 'i' will match worker 'i' => Not great
        initWorkpileIterator(stealIteratorsCache[i], i, workpileCount, workpiles);
        ++i;
    }
    derived->stealIterators = stealIteratorsCache;
}

void hcSchedulerStop(ocrScheduler_t * self) {
    ocrPolicyDomain_t *pd = NULL;
    ocrPolicyMsg_t msg;
    getCurrentEnv(&pd, NULL, &msg);
    
    // Stop the workpiles
    u64 count = self->workpileCount;
    for(i = 0; i < count; ++i) {
        self->workpiles[i]->fctPtrs->stop(self->workpiles[i]);
    }

    // We need to destroy the stealIterators now because pdFree does not
    // exist after stop
    ocrSchedulerHc_t * derived = (ocrSchedulerHc_t *) self;
    pd->pdFree(pd, derived->stealIterators);
    
    // Destroy the GUID
    
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_DESTROY
    msg.type = PD_MSG_GUID_DESTROY | PD_MSG_REQUEST;
    PD_MSG_FIELD(guid.guid) = self->guid;
    PD_MSG_FIELD(guid.metaDataPtr) = self;
    PD_MSG_FIELD(properties) = 0;
    pd->sendMessage(pd, &msg, false);
#undef PD_MSG
#undef PD_TYPE
    self->guid = UNINITIALIZED_GUID;
}

void hcSchedulerFinish(ocrScheduler_t *self) {
    u64 count = self->workpileCount;
    for(i = 0; i < count; ++i) {
        self->workpiles[i]->fctPtrs->finish(self->workpiles[i]);
    }
    // Nothing to do locally
}

// What is this for?
#if 0
static u8 hcSchedulerYield (ocrScheduler_t* self, ocrGuid_t workerGuid,
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
        ASSERT(count <= 1); // >1 not yet supported
        if (count != 0) {
            ocrTask_t* task = NULL;
            deguidify(pd, taskGuid, (u64*)&(task), NULL);
            worker->fctPtrs->execute(worker, task, taskGuid, yieldingEdtGuid);
        }
    }
    *returnGuid = result;
    ctx->destruct(ctx);
    return 0;
}
#endif /* if 0 */

u8 hcSchedulerTake (ocrScheduler_t *self, u32 *count, ocrFatGuid_t *edts) {
    // Source must be a worker guid and we rely on indices to map
    // workers to workpiles (one-to-one)
    // TODO: This is a non-portable assumption but will do for now.
    u64 workerId = context->sourceId;
    // First try to pop
    ocrWorkpile_t * wp_to_pop = popMappingOneToOne(self, workerId);
    // TODO sagnak, just to get it to compile, I am trickling down the 'cost' though it most probably is not the same
    ocrGuid_t popped = wp_to_pop->fctPtrs->pop(wp_to_pop,cost);
    if ( NULL_GUID == popped ) {
        // If popping failed, try to steal
        hcWorkpileIterator_t* it = stealMappingOneToAllButSelf(self, workerId);
        while ( workpileIteratorHasNext(it) && (NULL_GUID == popped)) {
            ocrWorkpile_t * next = workpileIteratorNext(it);
            // TODO sagnak, just to get it to compile, I am trickling down the 'cost' though it most probably is not the same
            popped = next->fctPtrs->steal(next, cost);
        }
    }
    // In this implementation we expect the caller to have
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

u8 hcSchedulerGive (ocrScheduler_t* base, u32* count, ocrFatGuid_t* edts) {
    // Source must be a worker guid
    u64 workerId = context->sourceId;
    ocrWorkpile_t * wp_to_push = pushMappingOneToOne(base, workerId);
    u32 i = 0;
    for ( ; i < count; ++i ) {
        wp_to_push->fctPtrs->push(wp_to_push,edts[i]);
    }
    return 0;
}

ocrScheduler_t* newSchedulerHc(ocrSchedulerFactory_t * factory, ocrParamList_t *perInstance) {
    ocrSchedulerHc_t* derived = (ocrSchedulerHc_t*) runtimeChunkAlloc(
        sizeof(ocrSchedulerHc_t), NULL);
    
    ocrScheduler_t* base = (ocrScheduler_t*)derived;
    base->guid = UNINITIALIZED_GUID;
    base->pd = NULL;
    base->workpiles = NULL;
    base->workpileCount = 0;
    base->fctPtrs = &(factory->schedulerFcts);
    
    paramListSchedulerHcInst_t *mapper = (paramListSchedulerHcInst_t*)perInstance;
    derived->workerIdFirst = mapper->workerIdFirst;
    
    return base;
}

void destructSchedulerFactoryHc(ocrSchedulerFactory_t * factory) {
    runtimeChunkFree(factory);
}

ocrSchedulerFactory_t * newOcrSchedulerFactoryHc(ocrParamList_t *perType) {
    ocrSchedulerFactoryHc_t* derived = (ocrSchedulerFactoryHc_t*) runtimeChunkAlloc(
        sizeof(ocrSchedulerFactoryHc_t), NULL);
    
    ocrSchedulerFactory_t* base = (ocrSchedulerFactory_t*) derived;
    base->instantiate = &newSchedulerHc;
    base->destruct = &destructSchedulerFactoryHc;
    base->schedulerFcts.start = &hcSchedulerStart;
    base->schedulerFcts.stop = &hcSchedulerStop;
    base->schedulerFcts.finish = &hcSchedulerFinish;
    base->schedulerFcts.destruct = &hcSchedulerDestruct;
    base->schedulerFcts.takeEdt = &hcSchedulerTake;
    base->schedulerFcts.giveEdt = &hcSchedulerGive;
    return base;
}

#endif /* ENABLE_SCHEDULER_HC */
