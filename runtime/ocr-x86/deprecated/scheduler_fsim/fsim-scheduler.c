/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#if 0
#include <stdlib.h>

#include "ocr-runtime.h"
#include "fsim-scheduler.h"
#include "worker/fsim/fsim-worker.h"

static inline ocrWorkpile_t* xeSchedulerPopMappingToAssignedWork (ocrScheduler_t* self, u64 workerId ) {
    ocrSchedulerXE_t* xeDerived = (ocrSchedulerXE_t*)self;
    assert( workerId >= xeDerived->workerIdFirst && workerId <= xeDerived->workerIdLast && "worker does not seem of this domain");
    return self->workpiles[ 2 * (workerId % xeDerived->nWorkers) ];
}

static inline ocrWorkpile_t* xeSchedulerPopMappingToWorkShipping (ocrScheduler_t* self, u64 workerId ) {
    ocrSchedulerXE_t* xeDerived = (ocrSchedulerXE_t*)self;
    // TODO sagnak xeDerived->workerIdFirst hardcoding is nasty, the work shipped always goes to the first workers queue
    return self->workpiles[ 1 + 2 * (xeDerived->workerIdFirst % xeDerived->nWorkers) ];
}

static u8 xeSchedulerTake (ocrScheduler_t *self, struct _ocrCost_t *cost, u32 *count,
                           ocrGuid_t *edts, ocrPolicyCtx_t *context) {

    ocrWorkpile_t * workpileToPop = NULL;
    ocrGuid_t popped = NULL_GUID;
    u64 workerId = context->sourceId;

    // sagnak XE extracting working from its assigned queue for execution
    if ( PD_MSG_EDT_TAKE == context->type ) {
        ocrSchedulerXE_t* xeDerived = (ocrSchedulerXE_t*)self;
        assert ( workerId >= xeDerived->workerIdFirst && workerId <= xeDerived->workerIdLast && "workerId range discrepancy in xeSchedulerTake");

        ocrWorker_t* worker = NULL;
        deguidify(getCurrentPD(), context->sourceObj, (u64*)&(worker), NULL);
        assert ( ((ocrWorkerXE_t*)worker)->id == workerId && "worker id from object and context does not match");

        assert ( context->PD->guid == getCurrentPD()->guid && "source for this xeSchedulerTake part should originate within");

        // XE worker trying to extract 'executable work' from CE assigned workpile
        workpileToPop = xeSchedulerPopMappingToAssignedWork (self, workerId);
        // TODO sagnak this is true because XE and CE does not touch the workpile simultaneously
        // XE is woken up after the CE is done pushing the work here
        // TODO sagnak, just to get it to compile, I am trickling down the 'cost' though it most probably is not the same
        popped = workpileToPop->fctPtrs->pop(workpileToPop,cost);
    } else {
        // sagnak CE asking the XE policy domain to extract work to be shipped on its behalf

        assert(PD_MSG_PICKUP_EDT == context->type && "The else condition should be for CE to pickup work");
        assert(context->PD->guid != getCurrentPD()->guid && "source for this xeSchedulerTake part should originate from CE");
        // TODO sagnak NOT IDEAL, and the XE may be simultaneously pushing, therefore we 'steal' for synchronization
        workpileToPop = xeSchedulerPopMappingToWorkShipping (self, workerId);
        // TODO sagnak, just to get it to compile, I am trickling down the 'cost' though it most probably is not the same
        popped = workpileToPop->fctPtrs->steal(workpileToPop,cost);
    }

    // In this implementation we expect the caller to have
    // allocated memory for us since we can return at most one
    // guid (most likely store using the address of a local)
    if ( NULL_GUID != popped ) {
        *count = 1;
        *edts = popped;
    } else {
        *count = 0;
    }
    return 0;
}

static inline ocrWorkpile_t* xeSchedulerPushMappingToWorkShipping (ocrScheduler_t* self, u64 workerId ) {
    ocrSchedulerXE_t* xeDerived = (ocrSchedulerXE_t*)self;
    assert(workerId >= xeDerived->workerIdFirst && workerId <= xeDerived->workerIdLast && "worker does not seem of this domain");
    return self->workpiles[ 1 + 2 * (workerId % xeDerived->nWorkers) ];
}

static inline ocrWorkpile_t* xeSchedulerPushMappingToAssignedWork ( ocrScheduler_t* self ) {
    ocrSchedulerXE_t* xeDerived = (ocrSchedulerXE_t*)self;
    // TODO sagnak xeDerived->workerIdFirst hardcoding is nasty, the assigned work always goes to the first workers queue
    return self->workpiles[ 2 * (xeDerived->workerIdFirst % xeDerived->nWorkers) ];
}

// XE scheduler give can be called from three sites:
// first being the ocrEdtSchedule, pushing user created work when executing user code
// those tasks should end up in the workpile designated for buffering work for the CE to take
// the second being, XE pushing a 'message task' which should be handed out to the CE policy domain
// third being the CE worker pushing work onto 'executable task' workpile
// the differentiation between {1,2} and {3} is through the worker id
// the differentiation between {1} and {2} is through a runtime check of the underlying task type
static u8 xeSchedulerGive (ocrScheduler_t* self, u32 count, ocrGuid_t* edts, struct _ocrPolicyCtx_t *context ) {
    u32 i = 0;
    for ( ; i < count; ++i ) {

        if ( PD_MSG_EDT_GIVE == context->type ) {
            ocrPolicyDomain_t * pd = context->PD;
            assert( pd->guid == context->PD->guid && pd->guid == getCurrentPD()->guid && "source for this xeSchedulerGive part should originate within");

            u64 workerId = context->sourceId;
            ocrSchedulerXE_t* xeDerived = (ocrSchedulerXE_t*)self;
            assert ( workerId >= xeDerived->workerIdFirst && workerId <= xeDerived->workerIdLast && "workerID discrepancy in xeSchedulerGive");

            ocrWorker_t* worker = NULL;
            deguidify(getCurrentPD(), context->sourceObj, (u64*)&(worker), NULL);
            assert ( ((ocrWorkerXE_t*)worker)->id == workerId && "worker id from object and context does not match");

            ocrPolicyCtx_t * alarmContext = context->clone(context);
#ifdef SYNCHRONOUS_GIVE
            alarmContext->type = PD_MSG_EDT_GIVE;
            alarmContext->PD = pd;
            alarmContext->sourceObj = alarmContext->sourcePD = pd->guid;
            pd->giveEdt(pd, 1, &(edts[i]), alarmContext);
#else
            alarmContext->type = PD_MSG_TAKE_MY_WORK;
            alarmContext->PD = pd;
            alarmContext->sourceObj = alarmContext->sourcePD = pd->guid;
            // TODO sagnak Multiple XE push mappings, do not use the class push_mapping indirection
            ocrWorkpile_t* wpToPush = xeSchedulerPushMappingToWorkShipping(self, workerId);
            wpToPush->fctPtrs->push(wpToPush, edts[i]);
            pd->giveEdt(pd, 0, NULL, alarmContext);
#endif
        } else {
            assert(PD_MSG_INJECT_EDT == context->type && "The else condition should be for CE to inject work");
            assert(context->sourcePD != getCurrentPD()->guid && "source for this xeSchedulerGive part should originate from CE");
            ocrWorkpile_t* wpToPush = xeSchedulerPushMappingToAssignedWork (self);
            wpToPush->fctPtrs->push(wpToPush, edts[i]);
        }
    }

    return 0;
}

void xeSchedulerDestruct(ocrScheduler_t * scheduler) {
    // just free self, workpiles are not allocated by the scheduler
    free(scheduler);
}

static ocrScheduler_t* newSchedulerXE (ocrSchedulerFactory_t * factory, ocrParamList_t *perInstance) {
    ocrSchedulerXE_t* xeDerived = (ocrSchedulerXE_t*) checkedMalloc(xeDerived, sizeof(ocrSchedulerXE_t));
    ocrScheduler_t* base = (ocrScheduler_t*)xeDerived;
    base->fctPtrs = &(factory->schedulerFcts);
    paramListSchedulerXEInst_t *mapper = (paramListSchedulerXEInst_t*)perInstance;
    xeDerived->workerIdFirst = mapper->workerIdFirst;
    xeDerived->workerIdLast  = mapper->workerIdLast;
    xeDerived->nWorkers = mapper->workerIdLast - mapper->workerIdFirst + 1;
    return base;
}

static void destructSchedulerFactoryXE (ocrSchedulerFactory_t * factory) {
    free(factory);
}

static void xeSchedulerStartAssert (ocrScheduler_t * self, ocrPolicyDomain_t * PD) {
    assert ( 0 && "xe scheduler start should not have been called");
}

static u8 xeSchedulerYieldAssert (ocrScheduler_t* self, ocrGuid_t workerGuid,
                                  ocrGuid_t yieldingEdtGuid, ocrGuid_t eventToYieldForGuid,
                                  ocrGuid_t * returnGuid,
                                  ocrPolicyCtx_t *context) {
    assert ( 0 && "xeSchedulerYieldAssert should not have been called");
    return 0;
}

static void xeSchedulerStop(ocrScheduler_t * self) {
    // nothing to do here
    assert ( 0 && "xe scheduler stop should not have been called");
}

ocrSchedulerFactory_t * newOcrSchedulerFactoryXE (ocrParamList_t *perType) {
    ocrSchedulerFactoryXE_t* xeFactoryDerived = (ocrSchedulerFactoryXE_t*) checkedMalloc(xeFactoryDerived, sizeof(ocrSchedulerFactoryXE_t));
    ocrSchedulerFactory_t* baseFactory = (ocrSchedulerFactory_t*) xeFactoryDerived;
    baseFactory->instantiate = newSchedulerXE;
    baseFactory->destruct = destructSchedulerFactoryXE;
    baseFactory->schedulerFcts.start = xeSchedulerStartAssert;
    baseFactory->schedulerFcts.stop = xeSchedulerStop;
    baseFactory->schedulerFcts.yield = xeSchedulerYieldAssert;
    baseFactory->schedulerFcts.destruct = xeSchedulerDestruct;
    baseFactory->schedulerFcts.takeEdt = xeSchedulerTake;
    baseFactory->schedulerFcts.giveEdt = xeSchedulerGive;
    return baseFactory;
}

static u8 ceSchedulerTake (ocrScheduler_t *self, struct _ocrCost_t *cost, u32 *count,
                           ocrGuid_t *edts, struct _ocrPolicyCtx_t *context) {


    ocrGuid_t workerId = context->sourceObj;
    ocrSchedulerCE_t* ceDerived = (ocrSchedulerCE_t*) self;
    ocrWorkpile_t * wpToPop = NULL;

    if ( PD_MSG_MSG_TAKE == context->type ) {
        // if I am the CE popping from my own message stash
        assert(workerId >= ceDerived->workerIdFirst && workerId <= ceDerived->workerIdLast && " ce worker does not seem of this domain");

        ocrWorker_t* worker = NULL;
        deguidify(getCurrentPD(), context->sourceObj, (u64*)&(worker), NULL);
        assert ( ((ocrWorkerCE_t*)worker)->id == workerId && "ce worker id from object and context does not match");

        assert ( context->sourcePD == getCurrentPD()->guid && "source for this ceSchedulerTake part should originate within");

        wpToPop = self->workpiles[ 1 + 2 * (workerId % ceDerived->nWorkers) ];
    } else {
        // if I am the CE popping from my own work stash on behalf of XEs

        assert(PD_MSG_EDT_TAKE == context->type && "The else condition should be for CE to picking work for XE");
        assert(context->sourcePD == getCurrentPD()->guid && "source for this ceSchedulerTake part should originate from within");

        wpToPop = self->workpiles[ 2 * (workerId % ceDerived->nWorkers) ];
    }

    // TODO fix synchronization errors, using 'steal' for now rather than a 'pop'
    // TODO sagnak, just to get it to compile, I am trickling down the 'cost' though it most probably is not the same
    ocrGuid_t popped = wpToPop->fctPtrs->steal(wpToPop, cost);

    if ( NULL_GUID != popped ) {
        *count = 1;
        edts[0] = popped;
    } else {
        *count = 0;
    }
    return 0;
}

static inline ocrWorkpile_t * ceSchedulerPushMappingToWork (ocrScheduler_t* self, u64 workerId ) {
    ocrSchedulerCE_t* ceDerived = (ocrSchedulerCE_t*) self;
    assert(workerId >= ceDerived->workerIdFirst && workerId <= ceDerived->workerIdLast && "worker does not seem of this domain");
    return self->workpiles[ 2 * (workerId % ceDerived->nWorkers) ];
}

static inline ocrWorkpile_t * ceSchedulerPushMappingToMessages (ocrScheduler_t* self, u64 workerId ) {
    ocrSchedulerCE_t* ceDerived = (ocrSchedulerCE_t*) self;
    // TODO sagnak; god awful hardcoding, BAD
    return self->workpiles[ 1 + 2 * (ceDerived->workerIdFirst % ceDerived->nWorkers) ];
}

// this can be called from three different sites:
// one being the initial(from master) work that CE should dissipate
// other being the XEs giving a 'message task' for the CE to be notified
// last being the CEs giving a 'message task' to itself if it can not serve the message
u8 ceSchedulerGive (ocrScheduler_t* self, u32 count, ocrGuid_t* edts, struct _ocrPolicyCtx_t *context ) {
    u32 i = 0;
    for ( ; i < count; ++i ) {
        ocrGuid_t workerId = context->sourceObj;

        ocrGuid_t taskGuid = edts[i];

        ocrWorkpile_t * workpileToPush = NULL;
        if ( PD_MSG_MSG_GIVE == context->type ) {
            // CE buffering unanswered messages
            // TODO sagnak Multiple XE push mappings, do not use the class push_mapping indirection
            // TODO this is some foreign XE worker, what is the point in trickling down the worker id?
            // TODO sagnak this should be a LOCKED data structure
            // TODO there is no way to pick which 'message task pool' to go to for now :(
            workpileToPush = ceSchedulerPushMappingToMessages(self, workerId);
        } else {
            assert(PD_MSG_EDT_GIVE == context->type && "The else condition should be for master CE pushing initial work");
            // either master CE worker picking initial work
            ocrSchedulerCE_t* ceDerived = (ocrSchedulerCE_t*) self;
            assert(workerId >= ceDerived->workerIdFirst && workerId <= ceDerived->workerIdLast && " ce worker does not seem of this domain");

            ocrWorker_t* worker = NULL;
            deguidify(getCurrentPD(), context->sourceObj, (u64*)&(worker), NULL);
            assert ( ((ocrWorkerCE_t*)worker)->id == workerId && "ce worker id from object and context does not match");

            assert ( context->sourcePD == getCurrentPD()->guid && "source for this ceSchedulerTake part should originate within");

            workpileToPush = ceSchedulerPushMappingToWork(self, workerId);
        }

        workpileToPush->fctPtrs->push(workpileToPush,taskGuid);
    }
    return 0;
}


void ceSchedulerDestruct(ocrScheduler_t * scheduler) {
    // just free self, workpiles are not allocated by the scheduler
    free(scheduler);
}

static ocrScheduler_t* newSchedulerCE (ocrSchedulerFactory_t * factory, ocrParamList_t *perInstance) {
    ocrSchedulerCE_t* ceDerived = (ocrSchedulerCE_t*) checkedMalloc(ceDerived, sizeof(ocrSchedulerCE_t));
    ocrScheduler_t* base = (ocrScheduler_t*)ceDerived;
    base->fctPtrs = &(factory->schedulerFcts);
    paramListSchedulerCEInst_t *mapper = (paramListSchedulerCEInst_t*)perInstance;
    ceDerived->workerIdFirst = mapper->workerIdFirst;
    ceDerived->workerIdLast  = mapper->workerIdLast;
    ceDerived->nWorkers = mapper->workerIdLast - mapper->workerIdFirst + 1;
    return base;
}

static void destructSchedulerFactoryCE (ocrSchedulerFactory_t * factory) {
    free(factory);
}

static void ceSchedulerStartAssert (ocrScheduler_t * self, ocrPolicyDomain_t * PD) {
    assert ( 0 && "ce scheduler start should not have been called");
}

static u8 ceSchedulerYieldAssert (ocrScheduler_t* self, ocrGuid_t workerGuid,
                                  ocrGuid_t yieldingEdtGuid, ocrGuid_t eventToYieldForGuid,
                                  ocrGuid_t * returnGuid,
                                  ocrPolicyCtx_t *context) {
    assert ( 0 && "ceSchedulerYieldAssert should not have been called");
    return 0;
}

static void ceSchedulerStop(ocrScheduler_t * self) {
    // nothing to do here
    assert ( 0 && "ce scheduler stop should not have been called");
}

ocrSchedulerFactory_t * newOcrSchedulerFactoryCE (ocrParamList_t *perType) {
    ocrSchedulerFactoryCE_t* ceFactoryDerived = (ocrSchedulerFactoryCE_t*) checkedMalloc(ceFactoryDerived, sizeof(ocrSchedulerFactoryCE_t));
    ocrSchedulerFactory_t* baseFactory = (ocrSchedulerFactory_t*) ceFactoryDerived;
    baseFactory->instantiate = newSchedulerCE;
    baseFactory->destruct = destructSchedulerFactoryCE;
    baseFactory->schedulerFcts.start = ceSchedulerStartAssert;
    baseFactory->schedulerFcts.stop = ceSchedulerStop;
    baseFactory->schedulerFcts.yield = ceSchedulerYieldAssert;
    baseFactory->schedulerFcts.destruct = ceSchedulerDestruct;
    baseFactory->schedulerFcts.takeEdt = ceSchedulerTake;
    baseFactory->schedulerFcts.giveEdt = ceSchedulerGive;
    return baseFactory;
}
#endif
