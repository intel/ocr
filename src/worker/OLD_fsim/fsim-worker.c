/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#if 0

#include "debug.h"
#include "ocr-comp-platform.h"
#include "ocr-runtime.h"
#include "ocr-types.h"
#include "ocr-worker.h"
#include "worker/fsim/fsim-worker.h"

#include <pthread.h>
#include <stdio.h>

#define DEBUG_TYPE WORKER

// TODO sagnak.fsim
static void associateCompPlatformAndXEWorker(ocrPolicyDomain_t * policy, ocrWorker_t * worker) {
    // This function must only be used when the contextFactory has its PD set
    ocrPolicyCtx_t * ctx = policy->contextFactory->instantiate(policy->contextFactory, NULL);
    ctx->sourcePD = policy->guid;
    ctx->PD = policy;
    ctx->sourceObj = worker->guid;
    ctx->sourceId = ((ocrWorkerXE_t *) worker)->id;
    setCurrentPD(policy);
    setCurrentWorkerContext(ctx);
}

static void associateCompPlatformAndCEWorker(ocrPolicyDomain_t * policy, ocrWorker_t * worker) {
    // This function must only be used when the contextFactory has its PD set
    ocrPolicyCtx_t * ctx = policy->contextFactory->instantiate(policy->contextFactory, NULL);
    ctx->sourcePD = policy->guid;
    ctx->PD = policy;
    ctx->sourceObj = worker->guid;
    ctx->sourceId = ((ocrWorkerCE_t *) worker)->id;
    setCurrentPD(policy);
    setCurrentWorkerContext(ctx);
}

void destructXEWorker ( ocrWorker_t * base ) {
    ocrWorkerXE_t * xeWorker = (ocrWorkerXE_t *) base;
    pthread_cond_destroy( &xeWorker->isRunningCond );
    pthread_mutex_destroy( &xeWorker->isRunningMutex );

    ocrGuidProvider_t * guidProvider = getCurrentPD()->guidProvider;
    guidProvider->fctPtrs->releaseGuid(guidProvider, base->guid);
    free(base);
}

void destructCEWorker ( ocrWorker_t * base ) {
    ocrGuidProvider_t * guidProvider = getCurrentPD()->guidProvider;
    guidProvider->fctPtrs->releaseGuid(guidProvider, base->guid);
    free(base);
}

void* xeWorkerComputationRoutine (void * arg);

void* ceWorkerComputationRoutine (void * arg);

void* ceMasterWorkerComputationRoutine (void * arg);

// TODO sagnak.fsim copy/paste HC
void xeStartWorker(ocrWorker_t * base, ocrPolicyDomain_t * policy) {
    ocrWorkerXE_t * xeWorker = (ocrWorkerXE_t *) base;
    xeWorker->run = true;

    // Starts everybody, the first comp-platform has specific
    // code to represent the master thread.
    u64 computeCount = base->computeCount;
    // What the compute target will execute
    launchArg_t * launchArg = malloc(sizeof(launchArg_t));
    launchArg->routine = xeWorkerComputationRoutine;
    launchArg->arg = base;
    launchArg->PD = policy;
    u64 i = 0;
    for(i = 0; i < computeCount; i++) {
        base->computes[i]->fctPtrs->start(base->computes[i], policy, launchArg);
    }
}

void ceStartWorker(ocrWorker_t * base, ocrPolicyDomain_t * policy) {
    ocrWorkerCE_t * ceWorker = (ocrWorkerCE_t *) base;
    ceWorker->run = true;

    // Starts everybody, the first comp-platform has specific
    // code to represent the master thread.
    u64 computeCount = base->computeCount;
    // What the compute target will execute
    launchArg_t * launchArg = malloc(sizeof(launchArg_t));
    launchArg->routine = (ceWorker->id == 0) ? ceMasterWorkerComputationRoutine : ceWorkerComputationRoutine;
    launchArg->arg = base;
    launchArg->PD = policy;
    u64 i = 0;
    for(i = 0; i < computeCount; i++) {
        base->computes[i]->fctPtrs->start(base->computes[i], policy, launchArg);
    }

    if (ceWorker->id == 0) {
        // Worker zero doesn't start the underlying thread since it is
        // falling through after that start. However, it stills need
        // to set its local storage data.
        associateCompPlatformAndCEWorker(policy, base);
    }
}

void xeFinishWorker(ocrWorker_t * base) {
    assert( 0 && "what is XE worker finish suppossed to do?");
    ocrWorkerXE_t * xeWorker = (ocrWorkerXE_t *) base;
    xeWorker->run = false;
    DPRINTF(DEBUG_LVL_INFO, "Finishing worker routine %d\n", xeWorker->id);
}

void ceFinishWorker(ocrWorker_t * base) {
    assert( 0 && "what is CE worker finish suppossed to do?");
    ocrWorkerCE_t * ceWorker = (ocrWorkerCE_t *) base;
    ceWorker->run = false;
    DPRINTF(DEBUG_LVL_INFO, "Finishing worker routine %d\n", ceWorker->id);
}

void xeStopWorker(ocrWorker_t * base) {
    assert( 0 && "what is XE worker stop suppossed to do?");
    u64 computeCount = base->computeCount;
    u64 i = 0;
    for(i = 0; i < computeCount; i++) {
        base->computes[i]->fctPtrs->stop(base->computes[i]);
    }
    DPRINTF(DEBUG_LVL_INFO, "Stopping worker %d\n", ((ocrWorkerXE_t *)base)->id);
}

void ceStopWorker(ocrWorker_t * base) {
    assert( 0 && "what is CE worker stop suppossed to do?");
    u64 computeCount = base->computeCount;
    u64 i = 0;
    for(i = 0; i < computeCount; i++) {
        base->computes[i]->fctPtrs->stop(base->computes[i]);
    }
    DPRINTF(DEBUG_LVL_INFO, "Stopping worker %d\n", ((ocrWorkerCE_t *)base)->id);
}

bool xeIsRunningWorker(ocrWorker_t * base) {
    ocrWorkerXE_t * xeWorker = (ocrWorkerXE_t *) base;
    return xeWorker->run;
}

bool ceIsRunningWorker(ocrWorker_t * base) {
    ocrWorkerCE_t * ceWorker = (ocrWorkerCE_t *) base;
    return ceWorker->run;
}

ocrGuid_t xeGetCurrentEDT (ocrWorker_t * base) {
    ocrWorkerXE_t * xeWorker = (ocrWorkerXE_t *) base;
    return xeWorker->currentEDTGuid;
}

ocrGuid_t ceGetCurrentEDT (ocrWorker_t * base) {
    ocrWorkerCE_t * ceWorker = (ocrWorkerCE_t *) base;
    return ceWorker->currentEDTGuid;
}

void xeSetCurrentEDT (ocrWorker_t * base, ocrGuid_t curr_edt_guid) {
    ocrWorkerXE_t * xeWorker = (ocrWorkerXE_t *) base;
    xeWorker->currentEDTGuid = curr_edt_guid;
}

void ceSetCurrentEDT (ocrWorker_t * base, ocrGuid_t curr_edt_guid) {
    ocrWorkerCE_t * ceWorker = (ocrWorkerCE_t *) base;
    ceWorker->currentEDTGuid = curr_edt_guid;
}

ocrWorker_t* newWorkerXE (ocrWorkerFactory_t * factory, ocrParamList_t * perInstance) {
    ocrWorkerXE_t * worker = checkedMalloc(worker, sizeof(ocrWorkerXE_t));
    worker->run = false;
    worker->id = ((paramListWorkerXEInst_t*)perInstance)->workerId;
    worker->currentEDTGuid = NULL_GUID;
    ocrWorker_t * base = (ocrWorker_t *) worker;
    base->guid = UNINITIALIZED_GUID;
    guidify(getCurrentPD(), (u64)base, &(base->guid), OCR_GUID_WORKER);
    base->routine = xeWorkerComputationRoutine;
    base->fctPtrs = &(factory->workerFcts);

    pthread_cond_init( &worker->isRunningCond, NULL);
    pthread_mutex_init( &worker->isRunningMutex, NULL);

    return base;
}

ocrWorker_t* newWorkerCE (ocrWorkerFactory_t * factory, ocrParamList_t * perInstance) {
    ocrWorkerCE_t * worker = checkedMalloc(worker, sizeof(ocrWorkerCE_t));
    worker->run = false;
    worker->id = ((paramListWorkerCEInst_t*)perInstance)->workerId;
    worker->currentEDTGuid = NULL_GUID;
    ocrWorker_t * base = (ocrWorker_t *) worker;
    base->guid = UNINITIALIZED_GUID;
    guidify(getCurrentPD(), (u64)base, &(base->guid), OCR_GUID_WORKER);
    base->routine = ceWorkerComputationRoutine;
    base->fctPtrs = &(factory->workerFcts);
    return base;
}

static int xeGetWorkerId (ocrWorker_t * worker) {
    ocrWorkerXE_t * xeWorker = (ocrWorkerXE_t *) worker;
    return xeWorker->id;
}

static int ceGetWorkerId (ocrWorker_t * worker) {
    ocrWorkerCE_t * ceWorker = (ocrWorkerCE_t *) worker;
    return ceWorker->id;
}

static void xeExecuteWorker(ocrWorker_t * worker, ocrTask_t* task, ocrGuid_t taskGuid, ocrGuid_t currentTaskGuid) {
    worker->fctPtrs->setCurrentEDT(worker, taskGuid);
    task->fctPtrs->execute(task);
    worker->fctPtrs->setCurrentEDT(worker, currentTaskGuid);
}

static void ceExecuteWorker(ocrWorker_t * worker, ocrTask_t* task, ocrGuid_t taskGuid, ocrGuid_t currentTaskGuid) {
    worker->fctPtrs->setCurrentEDT(worker, taskGuid);
    task->fctPtrs->execute(task);
    worker->fctPtrs->setCurrentEDT(worker, currentTaskGuid);
}

void xeWorkerLoop(ocrPolicyDomain_t * pd, ocrWorker_t * worker) {
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
        } else {
            // TODO sagnak, should this instead be done within the scheduler?
            ocrPolicyCtx_t * alarmContext = orgCtx->clone(orgCtx);
            alarmContext->type = PD_MSG_GIVE_ME_WORK;

            // there is no work left and the message has been sent, so turn of the worker
            // baseWorker->stop(baseWorker);
            ocrWorkerXE_t* xeDerivedWorker = (ocrWorkerXE_t*)worker;
            pthread_mutex_lock(&xeDerivedWorker->isRunningMutex);
            if ( worker->fctPtrs->isRunning(worker) ) {
                // give the work to the XE scheduler, which in turn should give it to the CE
                // through policy domain hand out and the scheduler differentiates tasks by type (RTTI) like
                pd->giveEdt(pd, 0, NULL, alarmContext);
                pthread_cond_wait(&xeDerivedWorker->isRunningCond, &xeDerivedWorker->isRunningMutex);
            }
            pthread_mutex_unlock(&xeDerivedWorker->isRunningMutex);
        }
    }
    ctx->destruct(ctx);
}

void ceWorkerLoop(ocrPolicyDomain_t * pd, ocrWorker_t * worker) {
    // Build and cache a take context
    ocrPolicyCtx_t * orgCtx = getCurrentWorkerContext();
    ocrPolicyCtx_t * ctx = orgCtx->clone(orgCtx);
    ctx->type = PD_MSG_MSG_TAKE;
    // Entering the worker loop
    while(worker->fctPtrs->isRunning(worker)) {
        ocrGuid_t messageTaskGuid;
        ocrGuid_t taskGuid;
        u32 count;
        pd->takeEdt(pd, NULL, &count, &messageTaskGuid, ctx);
        // remove this when we can take a bunch and make sure there's
        // an agreement whether it's the callee or the caller that
        // allocates the taskGuid array
        ASSERT(count <= 1);
        if (count != 0) {

            ocrMessageTaskFSIM_t* messageTask = NULL; 
            deguidify(pd, messageTaskGuid, (u64*)&(messageTask), NULL);

            ocrPolicyCtx_t* messageContext = messageTask->message;

            switch (messageContext->type) {

                case PD_MSG_GIVE_ME_WORK:
                    {
                        ocrPolicyCtx_t * edtExtractContext = orgCtx->clone(orgCtx);
                        edtExtractContext->type = PD_MSG_EDT_TAKE;

                        u32 otherCount;
                        pd->takeEdt(pd, NULL, &otherCount, &taskGuid, edtExtractContext);

                        if ( NULL_GUID != taskGuid) {
                            ocrPolicyDomain_t* targetDomain = messageContext->PD;
                            ocrPolicyCtx_t * edtInjectContext = orgCtx->clone(orgCtx);
                            edtInjectContext->type = PD_MSG_INJECT_EDT;

                            targetDomain->giveEdt(targetDomain, 1, &taskGuid,edtInjectContext);
                            
                            // now that the XE has work, it may be restarted
                            // targetWorker->start(targetWorker);
                            //
                            ocrWorkerXE_t* xeWorkerToWakeUp = NULL;
                            deguidify(pd, messageContext->sourceObj, (u64*)&(xeWorkerToWakeUp), NULL);
                            pthread_mutex_lock(&xeWorkerToWakeUp->isRunningMutex);
                            pthread_cond_signal(&xeWorkerToWakeUp->isRunningCond);
                            pthread_mutex_unlock(&xeWorkerToWakeUp->isRunningMutex);
                        } else {
                            ocrPolicyCtx_t * msgBufferContext = orgCtx->clone(orgCtx);
                            msgBufferContext->type = PD_MSG_MSG_GIVE;
                            
                            // if there was no successful task handing to XE, do not lose the message
                            pd->giveEdt(pd, 1, &messageTaskGuid, msgBufferContext);
                        }
                    }
                    break;
                case PD_MSG_TAKE_MY_WORK:
                    {
                        ocrPolicyCtx_t * edtPickupContext = orgCtx->clone(orgCtx);
                        edtPickupContext->type = PD_MSG_PICKUP_EDT;

                        ocrPolicyDomain_t* targetDomain = messageContext->PD;
                        u32 otherCount;
                        targetDomain->takeEdt( targetDomain, NULL, &otherCount, &taskGuid, edtPickupContext);
                        if ( NULL_GUID != taskGuid ) {
                            ocrPolicyCtx_t * edtStoreContext = orgCtx->clone(orgCtx);
                            edtStoreContext->type = PD_MSG_EDT_GIVE;

                            pd->giveEdt(pd, 1, &taskGuid, edtStoreContext);
                        } else {
                            ocrPolicyCtx_t * msgBufferContext = orgCtx->clone(orgCtx);
                            msgBufferContext->type = PD_MSG_MSG_GIVE;
                            
                            // if there was no successful task handing to XE, do not lose the message
                            pd->giveEdt(pd, 1, &messageTaskGuid, msgBufferContext);
                        }
                    }
                    break;
                default: 
                    {
                        assert(0 && "should not see anything besides PD_MSG_GIVE_ME_WORK or PD_MSG_TAKE_MY_WORK");
                    }
            };

            // ceWorker->setCurrentEDT(ceWorker,messageTaskGuid);
            // curr_task->execute(curr_task);
            // ceWorker->setCurrentEDT(ceWorker, NULL_GUID);

            // worker->fctPtrs->execute(worker, task, taskGuid, NULL_GUID);
            // task->fctPtrs->destruct(task);
        }
    }
    ctx->destruct(ctx);
}

void * xeWorkerComputationRoutine (void * arg) {
    // Need to pass down a data-structure
    launchArg_t * launchArg = (launchArg_t *) arg;
    ocrPolicyDomain_t * pd = launchArg->PD;
    ocrWorker_t * worker = (ocrWorker_t *) launchArg->arg;
    // associate current thread with the worker
    associateCompPlatformAndXEWorker(pd, worker);
    // Setting up this worker context to takeEdts
    // This assumes workers are not relocatable
    DPRINTF(DEBUG_LVL_INFO, "Starting scheduler routine of worker %d\n", xeGetWorkerId(worker));
    xeWorkerLoop(pd, worker);
    return NULL;
}

void * ceWorkerComputationRoutine (void * arg) {
    // Need to pass down a data-structure
    launchArg_t * launchArg = (launchArg_t *) arg;
    ocrPolicyDomain_t * pd = launchArg->PD;
    ocrWorker_t * worker = (ocrWorker_t *) launchArg->arg;
    // associate current thread with the worker
    associateCompPlatformAndCEWorker(pd, worker);
    // Setting up this worker context to takeEdts
    // This assumes workers are not relocatable
    DPRINTF(DEBUG_LVL_INFO, "Starting scheduler routine of worker %d\n", ceGetWorkerId(worker));
    ceWorkerLoop(pd, worker);
    return NULL;
}

void * ceMasterWorkerComputationRoutine (void * arg) {
    launchArg_t * launchArg = (launchArg_t *) arg;
    ocrPolicyDomain_t * pd = launchArg->PD;
    ocrWorker_t * worker = (ocrWorker_t *) launchArg->arg;
    DPRINTF(DEBUG_LVL_INFO, "Starting scheduler routine of master worker %d\n", ceGetWorkerId(worker));
    ceWorkerLoop(pd, worker);
    return NULL;
}

void destructWorkerFactoryXE(ocrWorkerFactory_t * factory) {
    free(factory);
}

void destructWorkerFactoryCE(ocrWorkerFactory_t * factory) {
    free(factory);
}

ocrWorkerFactory_t * newOcrWorkerFactoryXE (ocrParamList_t * perType) {
    ocrWorkerFactoryXE_t* derived = (ocrWorkerFactoryXE_t*) checkedMalloc(derived, sizeof(ocrWorkerFactoryXE_t));
    ocrWorkerFactory_t* base = (ocrWorkerFactory_t*) derived;
    base->instantiate = newWorkerXE;
    base->destruct =  destructWorkerFactoryXE;
    base->workerFcts.destruct = destructXEWorker;
    base->workerFcts.start = xeStartWorker;
    base->workerFcts.execute = xeExecuteWorker;
    base->workerFcts.finish = xeFinishWorker;
    base->workerFcts.stop = xeStopWorker;
    base->workerFcts.isRunning = xeIsRunningWorker;
    base->workerFcts.getCurrentEDT = xeGetCurrentEDT;
    base->workerFcts.setCurrentEDT = xeSetCurrentEDT;
    return base;
}

ocrWorkerFactory_t * newOcrWorkerFactoryCE (ocrParamList_t * perType) {
    ocrWorkerFactoryCE_t* derived = (ocrWorkerFactoryCE_t*) checkedMalloc(derived, sizeof(ocrWorkerFactoryCE_t));
    ocrWorkerFactory_t* base = (ocrWorkerFactory_t*) derived;
    base->instantiate = newWorkerCE;
    base->destruct =  destructWorkerFactoryCE;
    base->workerFcts.destruct = destructCEWorker;
    base->workerFcts.start = ceStartWorker;
    base->workerFcts.execute = ceExecuteWorker;
    base->workerFcts.finish = ceFinishWorker;
    base->workerFcts.stop = ceStopWorker;
    base->workerFcts.isRunning = ceIsRunningWorker;
    base->workerFcts.getCurrentEDT = ceGetCurrentEDT;
    base->workerFcts.setCurrentEDT = ceSetCurrentEDT;
    return base;
}

#if 0
/******************************************************/
/* OCR-FSIM-XE WORKER                                 */
/******************************************************/

// Fwd declaration
ocrWorker_t* newWorkerFsimXE(ocrWorkerFactory_t * factory, ocrParamList_t * perInstance);

void destructWorkerFactoryFsimXE(ocrWorkerFactory_t * factory) {
    free(factory);
}

ocrWorkerFactory_t * newOcrWorkerFactoryFsimXE(ocrParamList_t * perType) {
    ocrWorkerFactoryFsimXE_t* derived = (ocrWorkerFactoryFsimXE_t*) checkedMalloc(derived, sizeof(ocrWorkerFactoryFsimXE_t));
    ocrWorkerFactory_t* base = (ocrWorkerFactory_t*) derived;
    base->instantiate = newWorkerFsimXE;
    base->destruct = destructWorkerFactoryFsimXE;
    return base;
}

void xe_stop_worker(ocrWorker_t * base) {
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) base;
    hcWorker->run = false;

    ocrWorkerFsimXE_t * xeWorker = (ocrWorkerFsimXE_t *) base;
    pthread_mutex_lock( &xeWorker->isRunningMutex );
    pthread_cond_signal ( &xeWorker->isRunningCond );
    pthread_mutex_unlock( &xeWorker->isRunningMutex );

    log_worker(INFO, "Stopping worker %d\n", hcWorker->id);
}

ocrWorker_t* newWorkerFsimXE (ocrWorkerFactory_t * factory, ocrParamList_t * perInstance) {
    // Piggy-back on HC worker, modifying some of the implementation function
    ocrWorkerFsimXE_t * xeWorker = (ocrWorkerFsimXE_t *) malloc(sizeof(ocrWorkerFsimXE_t));
    ocrWorkerHc_t * hcWorker = &(xeWorker->hcBase);
    hcWorker->run = false;
    hcWorker->id = ((paramListWorkerFsimInst_t*)perInstance)->worker_id;
    hcWorker->currentEDTGuid = NULL_GUID;

    ocrWorker_t * base = (ocrWorker_t *) hcWorker;
    ocrMappable_t* module_base = (ocrMappable_t*) base;
    module_base->mapFct = hc_ocr_module_map_scheduler_to_worker;

    base->guid = UNINITIALIZED_GUID;
    guidify(getCurrentPD(), (u64)base, &(base->guid), OCR_GUID_WORKER);

    base->scheduler = NULL;
    base->routine = xe_worker_computation_routine;
    base->fctPtrs = &(factory->workerFcts);
    //TODO these need to be moved to the factory schedulerFcts
    base->fctPtrs->destruct = xe_worker_destruct;
    base->fctPtrs->start = hc_start_worker;
    base->fctPtrs->stop = xe_stop_worker;
    base->fctPtrs->isRunning = hc_is_running_worker;
    base->fctPtrs->getCurrentEDT = hc_getCurrentEDT;
    base->fctPtrs->setCurrentEDT = hc_setCurrentEDT;
    //TODO END

    pthread_cond_init( &xeWorker->isRunningCond, NULL);
    pthread_mutex_init( &xeWorker->isRunningMutex, NULL);
    return base;
}


/******************************************************/
/* OCR-FSIM-CE WORKER                                 */
/******************************************************/

// Fwd declaration
ocrWorker_t* newWorkerFsimCE(ocrWorkerFactory_t * factory, ocrParamList_t * perInstance);

void destructWorkerFactoryFsimCE(ocrWorkerFactory_t * factory) {
    free(factory);
}

ocrWorkerFactory_t * newOcrWorkerFactoryFsimCE(ocrParamList_t * perType) {
    ocrWorkerFactoryFsimCE_t* derived = (ocrWorkerFactoryFsimCE_t*) checkedMalloc(derived, sizeof(ocrWorkerFactoryFsimCE_t));
    ocrWorkerFactory_t* base = (ocrWorkerFactory_t*) derived;
    base->instantiate = newWorkerFsimCE;
    base->destruct =  destructWorkerFactoryFsimCE;
    return base;
}

ocrWorker_t* newWorkerFsimCE (ocrWorkerFactory_t * factory, ocrParamList_t * perInstance) {
    // Piggy-back on HC worker, modifying the routine
    ocrWorkerHc_t * worker = (ocrWorkerHc_t *) malloc(sizeof(ocrWorkerHc_t));
    worker->run = false;
    worker->id = ((paramListWorkerFsimInst_t*)perInstance)->worker_id;
    worker->currentEDTGuid = NULL_GUID;

    ocrWorker_t * base = (ocrWorker_t *) worker;
    ocrMappable_t* module_base = (ocrMappable_t*) base;
    module_base->mapFct = hc_ocr_module_map_scheduler_to_worker;

    base->guid = UNINITIALIZED_GUID;
    guidify(getCurrentPD(), (u64)base, &(base->guid), OCR_GUID_WORKER);

    base->scheduler = NULL;
    base->routine = ce_worker_computation_routine;
    base->fctPtrs = (ocrWorkerFcts_t*) malloc(sizeof(ocrWorkerFcts_t));
    base->fctPtrs->destruct = destructWorkerHc;
    base->fctPtrs->start = hc_start_worker;
    base->fctPtrs->stop = hc_stop_worker;
    base->fctPtrs->isRunning = hc_is_running_worker;
    base->fctPtrs->getCurrentEDT = hc_getCurrentEDT;
    base->fctPtrs->setCurrentEDT = hc_setCurrentEDT;

    return base;
}
#endif

#endif 
