/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_SCHEDULER_CE

#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-runtime-types.h"
#include "ocr-sysboot.h"
#include "ocr-workpile.h"
#include "scheduler/ce/ce-scheduler.h"
// TODO: This relies on data in ce-worker (its ID to do the mapping)
// This is non-portable (CE scheduler does not work with non
// CE worker) but works for now
#include "worker/ce/ce-worker.h" // NON PORTABLE FOR NOW

/******************************************************/
/* Support structures                                 */
/******************************************************/
static inline void workpileIteratorReset(ceWorkpileIterator_t *base) {
    base->curr = ((base->id) + 1) % base->mod;
}

static inline bool workpileIteratorHasNext(ceWorkpileIterator_t * base) {
    return base->id != base->curr;
}

static inline ocrWorkpile_t * workpileIteratorNext(ceWorkpileIterator_t * base) {
    u64 current = base->curr;
    ocrWorkpile_t * toBeReturned = base->workpiles[current];
    base->curr = (current+1) % base->mod;
    return toBeReturned;
}

static inline void initWorkpileIterator(ceWorkpileIterator_t *base, u64 id,
                                        u64 workpileCount, ocrWorkpile_t ** workpiles ) {
    
    base->workpiles = workpiles;
    base->id = id;
    base->mod = workpileCount;
    // The 'curr' field is initialized by reset
    workpileIteratorReset(base);
}

/******************************************************/
/* OCR-CE SCHEDULER                                   */
/******************************************************/

static inline ocrWorkpile_t * popMappingOneToOne (ocrScheduler_t* base, u64 workerId ) {
    return base->workpiles[0];
}

static inline ocrWorkpile_t * pushMappingOneToOne (ocrScheduler_t* base, u64 workerId ) {
    return base->workpiles[0];
}

static inline ceWorkpileIterator_t* stealMappingOneToAllButSelf (ocrScheduler_t* base, u64 workerId ) {
    ocrSchedulerCe_t* derived = (ocrSchedulerCe_t*) base;
    ceWorkpileIterator_t * stealIterator = &(derived->stealIterators[0]);
    workpileIteratorReset(stealIterator);
    return stealIterator;
}

void ceSchedulerDestruct(ocrScheduler_t * self) {
    u64 i;
    // Destruct the workpiles
    u64 count = self->workpileCount;
    for(i = 0; i < count; ++i) {
        self->workpiles[i]->fcts.destruct(self->workpiles[i]);
    }
    runtimeChunkFree((u64)(self->workpiles), NULL);
    
    runtimeChunkFree((u64)self, NULL);
}

void ceSchedulerBegin(ocrScheduler_t * self, ocrPolicyDomain_t * PD) {
    // Nothing to do locally
    u64 workpileCount = self->workpileCount;
    u64 i;
    for(i = 0; i < workpileCount; ++i) {
        self->workpiles[i]->fcts.begin(self->workpiles[i], PD);
    }
}

void ceSchedulerStart(ocrScheduler_t * self, ocrPolicyDomain_t * PD) {
    
    // Get a GUID
    guidify(PD, (u64)self, &(self->fguid), OCR_GUID_SCHEDULER);
    self->pd = PD;
    ocrSchedulerCe_t * derived = (ocrSchedulerCe_t *) self;
    
    u64 workpileCount = self->workpileCount;
    u64 i;
    for(i = 0; i < workpileCount; ++i) {
        self->workpiles[i]->fcts.start(self->workpiles[i], PD);
    }
    
    ocrWorkpile_t ** workpiles = self->workpiles;
    
    // allocate steal iterator cache. Use pdMalloc since this is something
    // local to the policy domain and that will never be shared
    ceWorkpileIterator_t * stealIteratorsCache = PD->pdMalloc(
        PD, sizeof(ceWorkpileIterator_t)*workpileCount);
    
    // Initialize steal iterator cache
    i = 0;
    while(i < workpileCount) {
        // Note: here we assume workpile 'i' will match worker 'i' => Not great
        initWorkpileIterator(&stealIteratorsCache[i], i, workpileCount, workpiles);
        ++i;
    }
    derived->stealIterators = stealIteratorsCache;
}

void ceSchedulerStop(ocrScheduler_t * self) {
    ocrPolicyDomain_t *pd = NULL;
    ocrPolicyMsg_t msg;
    getCurrentEnv(&pd, NULL, NULL, &msg);
    
    // Stop the workpiles
    u64 i = 0;
    u64 count = self->workpileCount;
    for(i = 0; i < count; ++i) {
        self->workpiles[i]->fcts.stop(self->workpiles[i]);
    }

    // We need to destroy the stealIterators now because pdFree does not
    // exist after stop
    ocrSchedulerCe_t * derived = (ocrSchedulerCe_t *) self;
    pd->pdFree(pd, derived->stealIterators);
    
    // Destroy the GUID
    
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_DESTROY
    msg.type = PD_MSG_GUID_DESTROY | PD_MSG_REQUEST;
    PD_MSG_FIELD(guid) = self->fguid;
    PD_MSG_FIELD(guid.metaDataPtr) = self;
    PD_MSG_FIELD(properties) = 0;
    RESULT_ASSERT(pd->processMessage(pd, &msg, false), ==, 0);
#undef PD_MSG
#undef PD_TYPE
    self->fguid.guid = UNINITIALIZED_GUID;
}

void ceSchedulerFinish(ocrScheduler_t *self) {
    u64 i = 0;
    u64 count = self->workpileCount;
    for(i = 0; i < count; ++i) {
        self->workpiles[i]->fcts.finish(self->workpiles[i]);
    }
    // Nothing to do locally
}

// What is this for?
#if 0
static u8 ceSchedulerYield (ocrScheduler_t* self, ocrGuid_t workerGuid,
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
    while((result = eventToYieldFor->fcts.get(eventToYieldFor, 0)) == ERROR_GUID) {
        u32 count;
        ocrGuid_t taskGuid;
        pd->takeEdt(pd, NULL, &count, &taskGuid, ctx);
        ASSERT(count <= 1); // >1 not yet supported
        if (count != 0) {
            ocrTask_t* task = NULL;
            deguidify(pd, taskGuid, (u64*)&(task), NULL);
            worker->fcts.execute(worker, task, taskGuid, yieldingEdtGuid);
        }
    }
    *returnGuid = result;
    ctx->destruct(ctx);
    return 0;
}
#endif /* if 0 */

u8 ceSchedulerTake (ocrScheduler_t *self, u32 *count, ocrFatGuid_t *edts) {
    // Source must be a worker guid and we rely on indices to map
    // workers to workpiles (one-to-one)
    // TODO: This is a non-portable assumption but will do for now.
    ocrWorker_t *worker = NULL;
    ocrWorkerCe_t *ceWorker = NULL;
    getCurrentEnv(NULL, &worker, NULL, NULL);
    ceWorker = (ocrWorkerCe_t*)worker;

    if(*count == 0) return 1; // No room to put anything

    ASSERT(edts != NULL); // Array should be allocated at least
    
    u64 workerId = ceWorker->id;
    // First try to pop
    ocrWorkpile_t * wpToPop = popMappingOneToOne(self, workerId);
    // TODO sagnak, just to get it to compile, I am trickling down the 'cost' though it most probably is not the same
    // TODO: Add cost again
    ocrFatGuid_t popped = wpToPop->fcts.pop(wpToPop, POP_WORKPOPTYPE, NULL);
#if 0 //CE does not steal (yet)
    if(NULL_GUID == popped.guid) {
        // If popping failed, try to steal
        ceWorkpileIterator_t* it = stealMappingOneToAllButSelf(self, workerId);
        while(workpileIteratorHasNext(it) && (NULL_GUID == popped.guid)) {
            ocrWorkpile_t * next = workpileIteratorNext(it);
            // TODO sagnak, just to get it to compile, I am trickling down the 'cost' though it most probably is not the same
            // TODO: Add cost again
            popped = next->fcts.pop(next, STEAL_WORKPOPTYPE, NULL);
        }
    }
#endif
    // In this implementation we expect the caller to have
    // allocated memory for us since we can return at most one
    // guid (most likely store using the address of a local)
    if(NULL_GUID != popped.guid) {
        *count = 1;
        edts[0] = popped;
    } else {
        *count = 0;
    }
    return 0;
}

u8 ceSchedulerGive (ocrScheduler_t* base, u32* count, ocrFatGuid_t* edts) {
    // Source must be a worker guid and we rely on indices to map
    // workers to workpiles (one-to-one)
    // TODO: This is a non-portable assumption but will do for now.
    ocrWorker_t *worker = NULL;
    ocrWorkerCe_t *ceWorker = NULL;
    getCurrentEnv(NULL, &worker, NULL, NULL);
    ceWorker = (ocrWorkerCe_t*)worker;
    
    // Source must be a worker guid
    u64 workerId = ceWorker->id;
    ocrWorkpile_t * wpToPush = pushMappingOneToOne(base, workerId);
    u32 i = 0;
    for ( ; i < *count; ++i ) {
        wpToPush->fcts.push(wpToPush, PUSH_WORKPUSHTYPE, edts[i]);
        edts[i].guid = NULL_GUID;
    }
    *count = 0;
    return 0;
}

ocrScheduler_t* newSchedulerCe(ocrSchedulerFactory_t * factory, ocrParamList_t *perInstance) {
    ocrSchedulerCe_t* derived = (ocrSchedulerCe_t*) runtimeChunkAlloc(
        sizeof(ocrSchedulerCe_t), NULL);
    
    ocrScheduler_t* base = (ocrScheduler_t*)derived;
    base->fguid.guid = UNINITIALIZED_GUID;
    base->fguid.metaDataPtr = base;
    base->pd = NULL;
    base->workpiles = NULL;
    base->workpileCount = 0;
    base->fcts = factory->schedulerFcts;
    
    return base;
}

void destructSchedulerFactoryCe(ocrSchedulerFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrSchedulerFactory_t * newOcrSchedulerFactoryCe(ocrParamList_t *perType) {
    ocrSchedulerFactoryCe_t* derived = (ocrSchedulerFactoryCe_t*) runtimeChunkAlloc(
        sizeof(ocrSchedulerFactoryCe_t), NULL);
    
    ocrSchedulerFactory_t* base = (ocrSchedulerFactory_t*) derived;
    base->instantiate = &newSchedulerCe;
    base->destruct = &destructSchedulerFactoryCe;
    base->schedulerFcts.begin = FUNC_ADDR(void (*)(ocrScheduler_t*, ocrPolicyDomain_t*), ceSchedulerBegin);
    base->schedulerFcts.start = FUNC_ADDR(void (*)(ocrScheduler_t*, ocrPolicyDomain_t*), ceSchedulerStart);
    base->schedulerFcts.stop = FUNC_ADDR(void (*)(ocrScheduler_t*), ceSchedulerStop);
    base->schedulerFcts.finish = FUNC_ADDR(void (*)(ocrScheduler_t*), ceSchedulerFinish);
    base->schedulerFcts.destruct = FUNC_ADDR(void (*)(ocrScheduler_t*), ceSchedulerDestruct);
    base->schedulerFcts.takeEdt = FUNC_ADDR(u8 (*)(ocrScheduler_t*, u32*, ocrFatGuid_t*), ceSchedulerTake);
    base->schedulerFcts.giveEdt = FUNC_ADDR(u8 (*)(ocrScheduler_t*, u32*, ocrFatGuid_t*), ceSchedulerGive);
    return base;
}

#endif /* ENABLE_SCHEDULER_CE */
