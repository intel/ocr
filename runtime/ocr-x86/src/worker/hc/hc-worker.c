/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "debug.h"
#include "ocr-comp-platform.h"
#include "ocr-runtime.h"
#include "ocr-types.h"
#include "ocr-worker.h"
#include "worker/hc/hc-worker.h"

#include <pthread.h>
#include <stdio.h>

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#include "ocr-statistics-callbacks.h"
#endif

#define DEBUG_TYPE WORKER

/******************************************************/
/* OCR-HC WORKER                                      */
/******************************************************/

static void associate_comp_platform_and_worker(ocrPolicyDomain_t * policy, ocrWorker_t * worker) {
    // This function must only be used when the contextFactory has its PD set
    ocrPolicyCtx_t * ctx = policy->contextFactory->instantiate(policy->contextFactory, NULL);
    ctx->sourcePD = policy->guid;
    ctx->PD = policy;
    ctx->sourceObj = worker->guid;
    ctx->sourceId = ((ocrWorkerHc_t *) worker)->id;
    setCurrentPD(policy);
    setCurrentWorkerContext(ctx);
}

void destructWorkerHc ( ocrWorker_t * base ) {
    ocrGuidProvider_t * guidProvider = getCurrentPD()->guidProvider;
    guidProvider->fctPtrs->releaseGuid(guidProvider, base->guid);
    free(base);
}

/**
 * The computation worker routine that asks work to the scheduler
 */
void * worker_computation_routine(void * arg);
void * master_worker_computation_routine(void * arg);

void hcStartWorker(ocrWorker_t * base, ocrPolicyDomain_t * policy) {
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) base;
    hcWorker->run = true;

    // Starts everybody, the first comp-platform has specific
    // code to represent the master thread.
    u64 computeCount = base->computeCount;
    // What the compute target will execute
    launchArg_t * launchArg = malloc(sizeof(launchArg_t));
    launchArg->routine = (hcWorker->id == 0) ? master_worker_computation_routine : worker_computation_routine;
    launchArg->arg = base;
    launchArg->PD = policy;
    u64 i = 0;
    for(i = 0; i < computeCount; i++) {
        base->computes[i]->fctPtrs->start(base->computes[i], policy, launchArg);
#ifdef OCR_ENABLE_STATISTICS
        statsWORKER_START(policy, base->guid, base, base->computes[i]->guid, base->computes[i]);
#endif
    }

    if (hcWorker->id == 0) {
        // Worker zero doesn't start the underlying thread since it is
        // falling through after that start. However, it stills need
        // to set its local storage data.
        associate_comp_platform_and_worker(policy, base);
    }
}

void hcFinishWorker(ocrWorker_t * base) {
    // Do not recursively stop comp-target, this will be
    // done when the policy domain stops and allow thread '0'
    // to join with the other threads
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) base;
    hcWorker->run = false;
    DPRINTF(DEBUG_LVL_INFO, "Finishing worker routine %d\n", hcWorker->id);
}

void hcStopWorker(ocrWorker_t * base) {
    u64 computeCount = base->computeCount;
    u64 i = 0;
#ifdef OCR_ENABLE_STATISTICS
    ocrPolicyDomain_t *pd = getCurrentPD();
#endif
    // This code should be called by the master thread to join others
    for(i = 0; i < computeCount; i++) {
        base->computes[i]->fctPtrs->stop(base->computes[i]);
#ifdef OCR_ENABLE_STATISTICS
        statsWORKER_STOP(pd, base->guid, base, base->computes[i]->guid, base->computes[i]);
#endif
    }
    DPRINTF(DEBUG_LVL_INFO, "Stopping worker %d\n", ((ocrWorkerHc_t *)base)->id);
}

bool hc_is_running_worker(ocrWorker_t * base) {
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) base;
    return hcWorker->run;
}

ocrGuid_t hc_getCurrentEDT (ocrWorker_t * base) {
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) base;
    return hcWorker->currentEDTGuid;
}

void hc_setCurrentEDT (ocrWorker_t * base, ocrGuid_t curr_edt_guid) {
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) base;
    hcWorker->currentEDTGuid = curr_edt_guid;
}

/**
 * Builds an instance of a HC worker
 */
ocrWorker_t* newWorkerHc (ocrWorkerFactory_t * factory, ocrParamList_t * perInstance) {
    ocrWorkerHc_t * worker = checkedMalloc(worker, sizeof(ocrWorkerHc_t));
    worker->run = false;
    worker->id = ((paramListWorkerHcInst_t*)perInstance)->workerId;
    worker->currentEDTGuid = NULL_GUID;
    ocrWorker_t * base = (ocrWorker_t *) worker;
    base->guid = UNINITIALIZED_GUID;
    guidify(getCurrentPD(), (u64)base, &(base->guid), OCR_GUID_WORKER);
    base->routine = worker_computation_routine;
    base->fctPtrs = &(factory->workerFcts);
    return base;
}

// Convenient to have an id to index workers in pools
int get_worker_id(ocrWorker_t * worker) {
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) worker;
    return hcWorker->id;
}

static void hcExecuteWorker(ocrWorker_t * worker, ocrTask_t* task, ocrGuid_t taskGuid, ocrGuid_t currentTaskGuid) {
    worker->fctPtrs->setCurrentEDT(worker, taskGuid);
    task->fctPtrs->execute(task);
    worker->fctPtrs->setCurrentEDT(worker, currentTaskGuid);
}

void worker_loop(ocrPolicyDomain_t * pd, ocrWorker_t * worker) {
    // Build and cache a take context
    ocrPolicyCtx_t * orgCtx = getCurrentWorkerContext();
    ocrPolicyCtx_t * ctx = orgCtx->clone(orgCtx);
    ctx->type = PD_MSG_EDT_TAKE;
    // Entering the worker loop
    while(worker->fctPtrs->isRunning(worker)) {
        ocrGuid_t taskGuid;
        u32 count;
        pd->takeEdt(pd, NULL, &count, &taskGuid, ctx);
        // remove this when we can take a bunch and make sure there's
        // an agreement whether it's the callee or the caller that
        // allocates the taskGuid array
        ASSERT(count <= 1);
        if (count != 0) {
            ocrTask_t* task = NULL;
            deguidify(pd, taskGuid, (u64*)&(task), NULL);
            worker->fctPtrs->execute(worker, task, taskGuid, NULL_GUID);
            task->fctPtrs->destruct(task);
        }
    }
    ctx->destruct(ctx);
}

void * worker_computation_routine(void * arg) {
    // Need to pass down a data-structure
    launchArg_t * launchArg = (launchArg_t *) arg;
    ocrPolicyDomain_t * pd = launchArg->PD;
    ocrWorker_t * worker = (ocrWorker_t *) launchArg->arg;
    // associate current thread with the worker
    associate_comp_platform_and_worker(pd, worker);
    // Setting up this worker context to takeEdts
    // This assumes workers are not relocatable
    DPRINTF(DEBUG_LVL_INFO, "Starting scheduler routine of worker %d\n", get_worker_id(worker));
    worker_loop(pd, worker);
    return NULL;
}

void * master_worker_computation_routine(void * arg) {
    launchArg_t * launchArg = (launchArg_t *) arg;
    ocrPolicyDomain_t * pd = launchArg->PD;
    ocrWorker_t * worker = (ocrWorker_t *) launchArg->arg;
    DPRINTF(DEBUG_LVL_INFO, "Starting scheduler routine of master worker %d\n", get_worker_id(worker));
    worker_loop(pd, worker);
    return NULL;
}


/******************************************************/
/* OCR-HC WORKER FACTORY                              */
/******************************************************/

void destructWorkerFactoryHc(ocrWorkerFactory_t * factory) {
    free(factory);
}

ocrWorkerFactory_t * newOcrWorkerFactoryHc(ocrParamList_t * perType) {
    ocrWorkerFactoryHc_t* derived = (ocrWorkerFactoryHc_t*) checkedMalloc(derived, sizeof(ocrWorkerFactoryHc_t));
    ocrWorkerFactory_t* base = (ocrWorkerFactory_t*) derived;
    base->instantiate = newWorkerHc;
    base->destruct =  destructWorkerFactoryHc;
    base->workerFcts.destruct = destructWorkerHc;
    base->workerFcts.start = hcStartWorker;
    base->workerFcts.execute = hcExecuteWorker;
    base->workerFcts.finish = hcFinishWorker;
    base->workerFcts.stop = hcStopWorker;
    base->workerFcts.isRunning = hc_is_running_worker;
    base->workerFcts.getCurrentEDT = hc_getCurrentEDT;
    base->workerFcts.setCurrentEDT = hc_setCurrentEDT;
    return base;
}
